#include "stdafx.h"
#include "h1_sound.h"

#include "h1_cache_file.h"
#include "h1_log.h"
#include "h1_map_loader.h"

#include "game/game.h"
#include "render/render_cameras.h"

#include <mmsystem.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#pragma comment(lib, "winmm.lib")

/* constants */

enum
{
	k_h1_sound_output_rate = 44100,
	k_h1_sound_buffer_frames = 1024,
	k_h1_sound_buffer_count = 4,
	k_h1_sound_maximum_detail_voices = 16,
	k_h1_sound_xbox_adpcm_block_size = 36,
	k_h1_sound_xbox_adpcm_block_samples = 64,
};

enum e_h1_sound_compression
{
	_h1_sound_compression_none = 0,
	_h1_sound_compression_xbox_adpcm = 1,
};

enum e_h1_looping_sound_track_flags
{
	_h1_looping_sound_track_fade_in_at_start_bit = 0,
};

constexpr real32 k_h1_sound_master_gain = 1.f;
// how long the listener may go without a rendered frame before the sounds fade out (menus, loading)
constexpr real32 k_h1_sound_listener_timeout = 0.25f;
constexpr real32 k_h1_sound_default_fade_duration = 0.5f;

// minimum and maximum distance of every sound class (sound_classes.c), used when a sound leaves them zero
static const real_bounds k_h1_sound_class_distances[] =
{
	{ 1.4f, 8.f }, { 8.f, 120.f }, { 0.f, 0.f }, { 0.f, 0.f }, { 4.f, 70.f }, { 1.f, 9.f },
	{ 1.f, 9.f }, { 1.f, 9.f }, { 1.f, 9.f }, { 1.f, 9.f }, { 1.f, 9.f }, { 0.f, 0.f },
	{ 0.f, 0.f }, { 0.5f, 3.f }, { 0.5f, 3.f }, { 0.5f, 3.f }, { 0.f, 0.f }, { 0.f, 0.f },
	{ 0.9f, 10.f }, { 3.f, 20.f }, { 0.f, 0.f }, { 0.f, 0.f }, { 1.4f, 8.f }, { 1.4f, 8.f },
	{ 0.f, 0.f }, { 0.f, 0.f }, { 0.9f, 5.f }, { 0.9f, 5.f }, { 0.9f, 5.f }, { 0.9f, 5.f },
	{ 0.5f, 3.f }, { 0.f, 0.f }, { 0.9f, 5.f }, { 0.9f, 5.f }, { 0.9f, 5.f }, { 0.5f, 3.f },
	{ 0.f, 0.f }, { 0.f, 0.f }, { 0.f, 0.f }, { 0.5f, 3.f }, { 0.f, 0.f }, { 0.f, 0.f },
	{ 0.f, 0.f }, { 0.f, 0.f }, { 3.f, 20.f }, { 2.f, 5.f }, { 3.f, 20.f }, { 3.f, 20.f },
	{ 0.f, 0.f }, { 0.f, 0.f }, { 3.f, 20.f },
};

static const int16 k_h1_ima_step_table[89] =
{
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
	130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060,
	1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
	7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static const int8 k_h1_ima_index_table[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };

/* structures */

// one playable variation of a sound: the permutation and every permutation chained after it
struct s_h1_sound_permutation
{
	std::vector<int16> samples;
	real32 gain;
	int32 pitch_range;
};

// a pitch range's variations (sound_pitch_range): the ones played for pitches within its bend bounds
struct s_h1_sound_pitch_range
{
	real_bounds bend_bounds;
	real32 playback_rate;
	int32 first_permutation;
	int32 permutation_count;
};

struct s_h1_sound_data
{
	int32 channel_count;
	int32 sample_rate;
	real32 gain;
	real32 minimum_distance;
	real32 maximum_distance;
	real32 zero_gain_modifier;	// the gain at a scale of 0 and of 1 (sound_scale_value)
	real32 one_gain_modifier;
	real32 zero_pitch_modifier;	// the pitch at a scale of 0 and of 1
	real32 one_pitch_modifier;
	real32 maximum_bend_per_second;
	std::vector<s_h1_sound_permutation> permutations;	// grouped by pitch range
	std::vector<s_h1_sound_pitch_range> pitch_ranges;
};

struct s_h1_voice
{
	std::shared_ptr<const s_h1_sound_data> sound;
	int32 permutation;
	double position;
	bool looping;
	bool finished;			// set by the mixer when a one shot sound ends
	bool remove;			// set by the game thread, the mixer drops the voice
	uint32 random;
	real32 target_gain[2];	// set by the game thread
	real32 gain[2];			// what the mixer last applied
	volatile real32 pitch;	// set by the game thread, the mixer plays at it (and picks the pitch range of the next variation by it)
	int32 pitch_range;
};

struct s_h1_looping_sound
{
	datum definition_index;
	bool positional;
	real_point3d position;
	real32 fade;
	real32 fade_in_rate;
	real32 fade_out_rate;
	bool stopping;
	bool held = true;		// an attached sound's object function is active
	real32 scale = 1.f;
	real32 maximum_distance;
	std::vector<std::shared_ptr<s_h1_voice>> track_voices;
	std::vector<real32> track_gains;
	std::vector<real32> track_minimum_distances;
	std::vector<real32> track_pitches;		// bent toward the scale's pitch at the sound's maximum bend
	std::vector<real32> detail_timers;
};

struct s_h1_detail_voice
{
	std::shared_ptr<s_h1_voice> voice;
	real_point3d position;
	real32 gain;
	real32 maximum_distance = FLT_MAX;
	bool two_dimensional = false;	// heard at its gain wherever the listener is (scripted sounds without a source object)
	datum scripted_sound_index = NONE;	// started by a script, which can stop it
};

struct s_h1_sound_globals
{
	bool active;
	const h1_sbsp* structure_bsp;
	std::unordered_map<datum, std::shared_ptr<const s_h1_sound_data>> sounds;

	// game thread
	std::unique_ptr<s_h1_looping_sound> background;
	std::vector<std::unique_ptr<s_h1_looping_sound>> stopping_backgrounds;
	std::vector<std::unique_ptr<s_h1_looping_sound>> positional_sounds;
	std::unordered_map<int32, std::unique_ptr<s_h1_looping_sound>> attached_sounds;
	int32 next_attached_handle;
	std::vector<s_h1_detail_voice> detail_voices;
	int16 background_sound_index;
	uint32 random;
	LARGE_INTEGER last_update;

	// listener, set by the renderer
	bool listener_valid;
	real_point3d listener_point;
	real_vector3d listener_forward;
	real_vector3d listener_up;
	LARGE_INTEGER listener_time;

	// mixer thread
	std::mutex voices_lock;
	std::vector<std::shared_ptr<s_h1_voice>> voices;
	std::thread mixer_thread;
	std::atomic<bool> mixer_running;
	std::atomic<real32> master_target;
	real32 master_gain;
};

/* globals */

static s_h1_sound_globals g_h1_sound;

/* prototypes */

static void h1_sound_mixer_main(void);
static void h1_sound_mix(int16* output, int32 frame_count);
static std::shared_ptr<const s_h1_sound_data> h1_sound_data_get(datum sound_index);
static bool h1_sound_decode_permutation(const h1_snd* sound, const h1_snd_pitch_ranges_permutations* permutation, int32 channel_count, std::vector<int16>& samples);
static void h1_sound_decode_xbox_adpcm(const uint8* source, uint32 size, int32 channel_count, std::vector<int16>& samples);
static uint32 h1_sound_random(uint32* seed);
static real32 h1_sound_random_range(real32 lower, real32 upper);
static std::shared_ptr<s_h1_voice> h1_sound_voice_new(datum sound_index, bool looping);
static int32 h1_sound_pitch_range_find(const s_h1_sound_data* sound, real32 pitch, int32 old_range);
static int32 h1_sound_permutation_next(const s_h1_sound_data* sound, int32 pitch_range, int32 old_permutation, uint32* random);
static void h1_sound_voice_set_gain(s_h1_voice* voice, real32 left, real32 right);
static std::unique_ptr<s_h1_looping_sound> h1_looping_sound_new(datum definition_index, const real_point3d* position);
static void h1_looping_sound_start_voices(s_h1_looping_sound* loop);
static void h1_looping_sound_stop_voices(s_h1_looping_sound* loop);
static bool h1_looping_sound_update(s_h1_looping_sound* loop, real32 dt);
static void h1_sound_spatialize(const real_point3d* point, real32 minimum_distance, real32 maximum_distance, real32 gain, real32* out_left, real32* out_right);
static int32 h1_sound_cluster_get(const real_point3d* point);
template<typename t_attachment>
static void h1_sound_attachments_add(const h1_tag_block<t_attachment>& attachments, const real_point3d* position);

/* public code */

void h1_sound_begin(void)
{
	h1_sound_dispose();

	g_h1_sound.random = GetTickCount();
	g_h1_sound.background_sound_index = NONE;
	g_h1_sound.listener_valid = false;
	QueryPerformanceCounter(&g_h1_sound.last_update);

	const datum bsp_index = g_h1_cache_file->structure_bsp_tag_get(h1_maps_structure_bsp_index());
	g_h1_sound.structure_bsp = bsp_index != NONE ? (const h1_sbsp*)g_h1_cache_file->tag_get('sbsp', bsp_index) : NULL;
	if (!g_h1_sound.structure_bsp)
	{
		return;
	}

	// decode everything up front so cluster changes never wait on the decoder
	for (int32 i = 0; i < g_h1_sound.structure_bsp->background_sound_palette.count; i++)
	{
		const h1_sbsp_background_sound_palette* entry = g_h1_cache_file->block_get(g_h1_sound.structure_bsp->background_sound_palette, i);
		h1_looping_sound_new(entry->background_sound.index, NULL);
	}

	// sound scenery and looping sounds attached to scenery
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();
	for (int32 i = 0; i < scenario->sound_scenery.count; i++)
	{
		const h1_scnr_sound_scenery* placement = g_h1_cache_file->block_get(scenario->sound_scenery, i);
		const h1_scnr_sound_scenery_palette* palette = g_h1_cache_file->block_get(scenario->sound_scenery_palette, placement->palette_index);
		const h1_ssce* sound_scenery = palette ? (const h1_ssce*)g_h1_cache_file->tag_get(palette->name) : NULL;
		if (sound_scenery)
		{
			h1_sound_attachments_add(sound_scenery->attachments, &placement->position);
		}
	}
	for (int32 i = 0; i < scenario->scenery.count; i++)
	{
		const h1_scnr_scenery* placement = g_h1_cache_file->block_get(scenario->scenery, i);
		const h1_scnr_scenery_palette* palette = g_h1_cache_file->block_get(scenario->scenery_palette, placement->palette_index);
		const h1_scen* scenery = palette ? (const h1_scen*)g_h1_cache_file->tag_get(palette->name) : NULL;
		if (scenery)
		{
			h1_sound_attachments_add(scenery->attachments, &placement->position);
		}
	}

	size_t decoded_size = 0;
	for (const auto& sound : g_h1_sound.sounds)
	{
		for (const s_h1_sound_permutation& permutation : sound.second->permutations)
		{
			decoded_size += permutation.samples.size() * sizeof(int16);
		}
	}
	h1_log("sound: %d sounds decoded (%u KB), %d background sounds, %d positional looping sounds",
		(int32)g_h1_sound.sounds.size(), (uint32)(decoded_size / 1024), g_h1_sound.structure_bsp->background_sound_palette.count, (int32)g_h1_sound.positional_sounds.size());

	g_h1_sound.active = true;
	g_h1_sound.master_gain = 0.f;
	g_h1_sound.master_target = 0.f;
	g_h1_sound.mixer_running = true;
	g_h1_sound.mixer_thread = std::thread(h1_sound_mixer_main);
	return;
}

void h1_sound_dispose(void)
{
	if (g_h1_sound.mixer_thread.joinable())
	{
		g_h1_sound.mixer_running = false;
		g_h1_sound.mixer_thread.join();
	}

	g_h1_sound.active = false;
	g_h1_sound.structure_bsp = NULL;
	g_h1_sound.background.reset();
	g_h1_sound.stopping_backgrounds.clear();
	g_h1_sound.positional_sounds.clear();
	g_h1_sound.attached_sounds.clear();
	g_h1_sound.detail_voices.clear();
	g_h1_sound.voices.clear();
	g_h1_sound.sounds.clear();
	return;
}

void h1_sound_listener_set(const render_camera* camera)
{
	g_h1_sound.listener_point = camera->point;
	g_h1_sound.listener_forward = camera->forward;
	g_h1_sound.listener_up = camera->up;
	g_h1_sound.listener_valid = true;
	QueryPerformanceCounter(&g_h1_sound.listener_time);
	return;
}

void h1_sound_update(void)
{
	if (!g_h1_sound.active)
	{
		return;
	}

	LARGE_INTEGER now, frequency;
	QueryPerformanceCounter(&now);
	QueryPerformanceFrequency(&frequency);
	const real32 dt = MIN((real32)(now.QuadPart - g_h1_sound.last_update.QuadPart) / (real32)frequency.QuadPart, 0.1f);
	g_h1_sound.last_update = now;

	// nothing is rendered in menus or while loading: fade everything out and hold
	const real32 listener_age = (real32)(now.QuadPart - g_h1_sound.listener_time.QuadPart) / (real32)frequency.QuadPart;
	const bool playing = h1_maps_active() && game_in_progress() && !game_is_ui_shell() && g_h1_sound.listener_valid && listener_age < k_h1_sound_listener_timeout;
	g_h1_sound.master_target = playing ? k_h1_sound_master_gain : 0.f;
	if (!playing)
	{
		return;
	}

	std::lock_guard<std::mutex> lock(g_h1_sound.voices_lock);

	// background sound of the cluster the camera is in, outside the bsp the last one keeps playing
	const int32 cluster_index = h1_sound_cluster_get(&g_h1_sound.listener_point);
	const h1_sbsp_clusters* cluster = cluster_index != NONE ? g_h1_cache_file->block_get(g_h1_sound.structure_bsp->clusters, cluster_index) : NULL;
	if (cluster && cluster->background_sound_index != g_h1_sound.background_sound_index)
	{
		g_h1_sound.background_sound_index = cluster->background_sound_index;
		if (g_h1_sound.background)
		{
			g_h1_sound.background->stopping = true;
			g_h1_sound.stopping_backgrounds.push_back(std::move(g_h1_sound.background));
		}
		const h1_sbsp_background_sound_palette* entry = g_h1_cache_file->block_get(g_h1_sound.structure_bsp->background_sound_palette, cluster->background_sound_index);
		if (entry && entry->background_sound.index != NONE)
		{
			g_h1_sound.background = h1_looping_sound_new(entry->background_sound.index, NULL);
		}
	}

	if (g_h1_sound.background)
	{
		h1_looping_sound_update(g_h1_sound.background.get(), dt);
	}
	for (size_t i = 0; i < g_h1_sound.stopping_backgrounds.size();)
	{
		if (!h1_looping_sound_update(g_h1_sound.stopping_backgrounds[i].get(), dt))
		{
			g_h1_sound.stopping_backgrounds.erase(g_h1_sound.stopping_backgrounds.begin() + i);
			continue;
		}
		i++;
	}
	for (const auto& loop : g_h1_sound.positional_sounds)
	{
		h1_looping_sound_update(loop.get(), dt);
	}
	for (const auto& entry : g_h1_sound.attached_sounds)
	{
		h1_looping_sound_update(entry.second.get(), dt);
	}

	// detail sounds stay where they started while the listener moves
	for (size_t i = 0; i < g_h1_sound.detail_voices.size();)
	{
		s_h1_detail_voice* detail = &g_h1_sound.detail_voices[i];
		if (detail->voice->finished)
		{
			detail->voice->remove = true;
			g_h1_sound.detail_voices.erase(g_h1_sound.detail_voices.begin() + i);
			continue;
		}
		real32 left = detail->gain, right = detail->gain;
		if (!detail->two_dimensional)
		{
			h1_sound_spatialize(&detail->position, detail->voice->sound->minimum_distance, detail->maximum_distance, detail->gain, &left, &right);
		}
		h1_sound_voice_set_gain(detail->voice.get(), left, right);
		i++;
	}

	// finished one shot voices nobody tracks any more
	for (size_t i = 0; i < g_h1_sound.voices.size();)
	{
		if (g_h1_sound.voices[i]->remove)
		{
			g_h1_sound.voices.erase(g_h1_sound.voices.begin() + i);
			continue;
		}
		i++;
	}
	return;
}

void h1_sound_impulse(datum sound_index, const real_point3d* position, real32 scale)
{
	if (!g_h1_sound.active || sound_index == NONE)
	{
		return;
	}

	std::lock_guard<std::mutex> lock(g_h1_sound.voices_lock);
	std::shared_ptr<s_h1_voice> voice = h1_sound_voice_new(sound_index, false);
	if (!voice)
	{
		return;
	}

	s_h1_detail_voice impulse;
	impulse.voice = voice;
	impulse.gain = voice->sound->gain * scale;
	impulse.position = *position;
	impulse.maximum_distance = voice->sound->maximum_distance > 0.f ? voice->sound->maximum_distance : FLT_MAX;
	g_h1_sound.detail_voices.push_back(impulse);
	return;
}

void h1_sound_scripted_start(datum sound_index, const real_point3d* position, real32 scale)
{
	if (!g_h1_sound.active || sound_index == NONE)
	{
		return;
	}
	h1_sound_scripted_stop(sound_index);

	std::lock_guard<std::mutex> lock(g_h1_sound.voices_lock);
	std::shared_ptr<s_h1_voice> voice = h1_sound_voice_new(sound_index, false);
	if (!voice)
	{
		return;
	}
	s_h1_detail_voice impulse;
	impulse.voice = voice;
	impulse.gain = voice->sound->gain * scale;
	impulse.two_dimensional = position == NULL;
	impulse.position = position ? *position : real_point3d{};
	impulse.maximum_distance = voice->sound->maximum_distance > 0.f ? voice->sound->maximum_distance : FLT_MAX;
	impulse.scripted_sound_index = sound_index;
	g_h1_sound.detail_voices.push_back(impulse);
	return;
}

void h1_sound_scripted_stop(datum sound_index)
{
	for (size_t i = 0; i < g_h1_sound.detail_voices.size();)
	{
		if (g_h1_sound.detail_voices[i].scripted_sound_index == sound_index)
		{
			g_h1_sound.detail_voices[i].voice->remove = true;
			g_h1_sound.detail_voices.erase(g_h1_sound.detail_voices.begin() + i);
			continue;
		}
		i++;
	}
	return;
}

bool h1_sound_listener_point_get(real_point3d* out_point)
{
	if (!g_h1_sound.listener_valid)
	{
		return false;
	}
	*out_point = g_h1_sound.listener_point;
	return true;
}

real32 h1_sound_duration(datum sound_index)
{
	std::shared_ptr<const s_h1_sound_data> sound = sound_index != NONE ? h1_sound_data_get(sound_index) : NULL;
	if (!sound || sound->sample_rate <= 0 || sound->channel_count <= 0)
	{
		return 0.f;
	}
	size_t longest = 0;
	for (const s_h1_sound_permutation& permutation : sound->permutations)
	{
		longest = MAX(longest, permutation.samples.size());
	}
	return (real32)longest / (real32)(sound->sample_rate * sound->channel_count);
}

int32 h1_sound_looping_attached_new(datum looping_sound_index)
{
	if (!g_h1_sound.active || looping_sound_index == NONE)
	{
		return 0;
	}
	std::lock_guard<std::mutex> lock(g_h1_sound.voices_lock);
	const real_point3d origin = {};
	std::unique_ptr<s_h1_looping_sound> loop = h1_looping_sound_new(looping_sound_index, &origin);
	if (!loop)
	{
		return 0;
	}
	loop->held = false;
	const int32 handle = ++g_h1_sound.next_attached_handle;
	g_h1_sound.attached_sounds[handle] = std::move(loop);
	return handle;
}

void h1_sound_looping_attached_update(int32 handle, const real_point3d* position, bool audible, real32 scale)
{
	std::lock_guard<std::mutex> lock(g_h1_sound.voices_lock);
	auto found = g_h1_sound.attached_sounds.find(handle);
	if (found == g_h1_sound.attached_sounds.end())
	{
		return;
	}
	found->second->position = *position;
	found->second->held = audible;
	found->second->scale = PIN(scale, 0.f, 1.f);
	return;
}

void h1_sound_looping_attached_delete(int32 handle)
{
	std::lock_guard<std::mutex> lock(g_h1_sound.voices_lock);
	auto found = g_h1_sound.attached_sounds.find(handle);
	if (found == g_h1_sound.attached_sounds.end())
	{
		return;
	}
	found->second->stopping = true;
	g_h1_sound.stopping_backgrounds.push_back(std::move(found->second));
	g_h1_sound.attached_sounds.erase(found);
	return;
}

/* private code */

// looping sounds attached to an object play at its origin
template<typename t_attachment>
static void h1_sound_attachments_add(const h1_tag_block<t_attachment>& attachments, const real_point3d* position)
{
	for (int32 i = 0; i < attachments.count; i++)
	{
		const t_attachment* attachment = g_h1_cache_file->block_get(attachments, i);
		if (attachment->type.group_tag == 'lsnd' && attachment->type.index != NONE)
		{
			std::unique_ptr<s_h1_looping_sound> loop = h1_looping_sound_new(attachment->type.index, position);
			if (loop)
			{
				g_h1_sound.positional_sounds.push_back(std::move(loop));
			}
		}
	}
	return;
}

static std::unique_ptr<s_h1_looping_sound> h1_looping_sound_new(datum definition_index, const real_point3d* position)
{
	const h1_lsnd* definition = (const h1_lsnd*)g_h1_cache_file->tag_get('lsnd', definition_index);
	if (!definition)
	{
		return NULL;
	}

	std::unique_ptr<s_h1_looping_sound> loop = std::make_unique<s_h1_looping_sound>();
	loop->definition_index = definition_index;
	loop->positional = position != NULL;
	loop->position = position ? *position : real_point3d{};
	loop->stopping = false;
	loop->fade = 0.f;
	loop->fade_in_rate = 1.f / k_h1_sound_default_fade_duration;
	loop->fade_out_rate = 1.f / k_h1_sound_default_fade_duration;
	loop->maximum_distance = definition->maximum_distance;

	for (int32 i = 0; i < definition->tracks.count; i++)
	{
		const h1_lsnd_tracks* track = g_h1_cache_file->block_get(definition->tracks, i);
		std::shared_ptr<const s_h1_sound_data> sound = h1_sound_data_get(track->loop.index);
		h1_sound_data_get(track->start.index);
		loop->track_gains.push_back(track->gain);
		loop->track_minimum_distances.push_back(sound ? sound->minimum_distance : 1.f);
		if (sound)
		{
			loop->maximum_distance = MAX(loop->maximum_distance, sound->maximum_distance);
		}
		if (track->fade_in_duration > 0.f && TEST_BIT(track->flags, _h1_looping_sound_track_fade_in_at_start_bit))
		{
			loop->fade_in_rate = MIN(loop->fade_in_rate, 1.f / track->fade_in_duration);
		}
		if (track->fade_out_duration > 0.f)
		{
			loop->fade_out_rate = MIN(loop->fade_out_rate, 1.f / track->fade_out_duration);
		}
	}

	for (int32 i = 0; i < definition->detail_sounds.count; i++)
	{
		const h1_lsnd_detail_sounds* detail = g_h1_cache_file->block_get(definition->detail_sounds, i);
		h1_sound_data_get(detail->sound.index);
		loop->detail_timers.push_back(h1_sound_random_range(detail->random_period_bounds.lower, detail->random_period_bounds.upper));
	}
	return loop;
}

static void h1_looping_sound_start_voices(s_h1_looping_sound* loop)
{
	const h1_lsnd* definition = (const h1_lsnd*)g_h1_cache_file->tag_get('lsnd', loop->definition_index);
	loop->track_voices.clear();
	loop->track_pitches.clear();
	for (int32 i = 0; i < definition->tracks.count; i++)
	{
		const h1_lsnd_tracks* track = g_h1_cache_file->block_get(definition->tracks, i);
		loop->track_voices.push_back(h1_sound_voice_new(track->loop.index, true));
		loop->track_pitches.push_back(0.f);
	}
	return;
}

static void h1_looping_sound_stop_voices(s_h1_looping_sound* loop)
{
	for (const auto& voice : loop->track_voices)
	{
		if (voice)
		{
			voice->remove = true;
		}
	}
	loop->track_voices.clear();
	return;
}

// returns false once a stopping sound has faded out
static bool h1_looping_sound_update(s_h1_looping_sound* loop, real32 dt)
{
	const h1_lsnd* definition = (const h1_lsnd*)g_h1_cache_file->tag_get('lsnd', loop->definition_index);

	real32 distance = 0.f;
	if (loop->positional)
	{
		real_vector3d offset;
		vector_from_points3d(&g_h1_sound.listener_point, &loop->position, &offset);
		distance = magnitude3d(&offset);
	}
	const bool audible = !loop->stopping && loop->held && (!loop->positional || distance < loop->maximum_distance);

	if (audible && loop->track_voices.empty())
	{
		h1_looping_sound_start_voices(loop);
		// the tracks' start sounds play once as they begin (an attached sound's, where it is)
		if (loop->positional && loop->fade <= 0.f)
		{
			for (int32 i = 0; i < definition->tracks.count; i++)
			{
				const h1_lsnd_tracks* track = g_h1_cache_file->block_get(definition->tracks, i);
				std::shared_ptr<s_h1_voice> voice = track->start.index != NONE ? h1_sound_voice_new(track->start.index, false) : NULL;
				if (voice)
				{
					s_h1_detail_voice start;
					start.voice = voice;
					start.gain = track->gain * voice->sound->gain * loop->scale;
					start.position = loop->position;
					start.maximum_distance = loop->maximum_distance;
					g_h1_sound.detail_voices.push_back(start);
				}
			}
		}
	}

	loop->fade = audible
		? MIN(loop->fade + dt * loop->fade_in_rate, 1.f)
		: MAX(loop->fade - dt * loop->fade_out_rate, 0.f);

	if (!audible && loop->fade <= 0.f)
	{
		h1_looping_sound_stop_voices(loop);
		return !loop->stopping;
	}

	for (size_t i = 0; i < loop->track_voices.size(); i++)
	{
		s_h1_voice* voice = loop->track_voices[i].get();
		if (!voice)
		{
			continue;
		}
		// sound_scale_value: the sound's gain between its zero and one modifiers by the scale
		const real32 scale_gain = (voice->sound->one_gain_modifier - voice->sound->zero_gain_modifier) * loop->scale + voice->sound->zero_gain_modifier;

		// and its pitch, which bends there no faster than the sound allows (limit_pitch)
		const real32 desired_pitch = (voice->sound->one_pitch_modifier - voice->sound->zero_pitch_modifier) * loop->scale + voice->sound->zero_pitch_modifier;
		if (i < loop->track_pitches.size())
		{
			real32 pitch = loop->track_pitches[i];
			if (voice->sound->maximum_bend_per_second > 1.f && pitch > 0.f)
			{
				const real32 bend = powf(voice->sound->maximum_bend_per_second, dt);
				pitch = desired_pitch > pitch ? MIN(desired_pitch, pitch * bend) : MAX(desired_pitch, pitch / bend);
			}
			else
			{
				pitch = desired_pitch;
			}
			loop->track_pitches[i] = pitch;
			voice->pitch = pitch;
		}
		const real32 gain = loop->track_gains[i] * voice->sound->gain * loop->fade * scale_gain;
		real32 left = gain, right = gain;
		if (loop->positional)
		{
			h1_sound_spatialize(&loop->position, loop->track_minimum_distances[i], loop->maximum_distance, gain, &left, &right);
		}
		h1_sound_voice_set_gain(voice, left, right);
	}

	// detail sounds play around the source (around the listener for background sounds)
	if (audible)
	{
		for (int32 i = 0; i < definition->detail_sounds.count && i < (int32)loop->detail_timers.size(); i++)
		{
			loop->detail_timers[i] -= dt;
			if (loop->detail_timers[i] > 0.f)
			{
				continue;
			}

			const h1_lsnd_detail_sounds* detail = g_h1_cache_file->block_get(definition->detail_sounds, i);
			loop->detail_timers[i] = MAX(h1_sound_random_range(detail->random_period_bounds.lower, detail->random_period_bounds.upper) * definition->one_detail_sound_period, 0.1f);
			if (g_h1_sound.detail_voices.size() >= k_h1_sound_maximum_detail_voices)
			{
				continue;
			}

			std::shared_ptr<s_h1_voice> voice = h1_sound_voice_new(detail->sound.index, false);
			if (!voice)
			{
				continue;
			}

			const real32 yaw = h1_sound_random_range(detail->yaw_bounds.lower, detail->yaw_bounds.upper);
			const real32 pitch = h1_sound_random_range(detail->pitch_bounds.lower, detail->pitch_bounds.upper);
			const real32 offset_distance = h1_sound_random_range(detail->distance_bounds.lower, detail->distance_bounds.upper);
			const real_point3d* origin = loop->positional ? &loop->position : &g_h1_sound.listener_point;

			s_h1_detail_voice detail_voice;
			detail_voice.voice = voice;
			detail_voice.gain = detail->gain * voice->sound->gain * loop->fade;
			detail_voice.position.x = origin->x + cosf(yaw) * cosf(pitch) * offset_distance;
			detail_voice.position.y = origin->y + sinf(yaw) * cosf(pitch) * offset_distance;
			detail_voice.position.z = origin->z + sinf(pitch) * offset_distance;
			g_h1_sound.detail_voices.push_back(detail_voice);
		}
	}
	return true;
}

static std::shared_ptr<s_h1_voice> h1_sound_voice_new(datum sound_index, bool looping)
{
	std::shared_ptr<const s_h1_sound_data> sound = h1_sound_data_get(sound_index);
	if (!sound || sound->permutations.empty())
	{
		return NULL;
	}

	std::shared_ptr<s_h1_voice> voice = std::make_shared<s_h1_voice>();
	voice->sound = sound;
	voice->random = h1_sound_random(&g_h1_sound.random);
	voice->pitch = 1.f;
	voice->pitch_range = h1_sound_pitch_range_find(sound.get(), 1.f, NONE);
	voice->permutation = h1_sound_permutation_next(sound.get(), voice->pitch_range, NONE, &voice->random);
	voice->position = 0.0;
	voice->looping = looping;
	voice->finished = false;
	voice->remove = false;
	voice->target_gain[0] = voice->target_gain[1] = 0.f;
	voice->gain[0] = voice->gain[1] = 0.f;

	// callers hold the voices lock
	g_h1_sound.voices.push_back(voice);
	return voice;
}

// sound_definition_find_pitch_range_by_pitch: the old range while the pitch is within its bend bounds, else the first range it's
// within, else the closest one
static int32 h1_sound_pitch_range_find(const s_h1_sound_data* sound, real32 pitch, int32 old_range)
{
	const int32 count = (int32)sound->pitch_ranges.size();
	if (VALID_INDEX(old_range, count))
	{
		const s_h1_sound_pitch_range* range = &sound->pitch_ranges[old_range];
		if (range->permutation_count > 0 && range->bend_bounds.lower <= pitch && pitch <= range->bend_bounds.upper)
		{
			return old_range;
		}
	}
	int32 result = 0;
	real32 closest_ratio = FLT_MAX;
	for (int32 i = 0; i < count; i++)
	{
		const s_h1_sound_pitch_range* range = &sound->pitch_ranges[i];
		if (range->permutation_count <= 0)
		{
			continue;
		}
		if (range->bend_bounds.lower <= pitch && pitch <= range->bend_bounds.upper)
		{
			return i;
		}
		const real32 ratio = range->bend_bounds.upper < pitch ? pitch / MAX(range->bend_bounds.upper, 0.001f) : range->bend_bounds.lower / MAX(pitch, 0.001f);
		if (ratio < closest_ratio)
		{
			closest_ratio = ratio;
			result = i;
		}
	}
	return result;
}

// a random variation of the pitch range, never the old one twice in a row
static int32 h1_sound_permutation_next(const s_h1_sound_data* sound, int32 pitch_range, int32 old_permutation, uint32* random)
{
	const s_h1_sound_pitch_range* range = &sound->pitch_ranges[pitch_range];
	const int32 count = range->permutation_count;
	const int32 old = old_permutation - range->first_permutation;
	if (count > 1 && VALID_INDEX(old, count))
	{
		return range->first_permutation + (old + 1 + (int32)(h1_sound_random(random) % (count - 1))) % count;
	}
	return range->first_permutation + (int32)(h1_sound_random(random) % MAX(count, 1));
}

static void h1_sound_voice_set_gain(s_h1_voice* voice, real32 left, real32 right)
{
	voice->target_gain[0] = left;
	voice->target_gain[1] = right;
	return;
}

// inverse distance rolloff from the minimum distance (directsound 3d), panned by the listener's right vector
static void h1_sound_spatialize(const real_point3d* point, real32 minimum_distance, real32 maximum_distance, real32 gain, real32* out_left, real32* out_right)
{
	real_vector3d offset;
	vector_from_points3d(&g_h1_sound.listener_point, point, &offset);
	const real32 distance = magnitude3d(&offset);

	real32 attenuation = 0.f;
	if (distance < maximum_distance)
	{
		attenuation = distance > minimum_distance ? minimum_distance / distance : 1.f;
		// halo drops sounds past their maximum distance, fade the last part instead of cutting off
		const real32 fade_start = maximum_distance * 0.8f;
		if (distance > fade_start)
		{
			attenuation *= (maximum_distance - distance) / (maximum_distance - fade_start);
		}
	}

	real32 pan = 0.f;
	if (distance > 0.001f)
	{
		real_vector3d right;
		cross_product3d(&g_h1_sound.listener_forward, &g_h1_sound.listener_up, &right);
		normalize3d(&right);
		pan = dot_product3d(&offset, &right) / distance;
		// close sounds surround the listener
		pan *= MIN(distance / MAX(minimum_distance, 0.001f), 1.f);
	}

	*out_left = gain * attenuation * (1.f - 0.7f * MAX(pan, 0.f));
	*out_right = gain * attenuation * (1.f - 0.7f * MAX(-pan, 0.f));
	return;
}

static int32 h1_sound_cluster_get(const real_point3d* point)
{
	const h1_sbsp* bsp = g_h1_sound.structure_bsp;
	const h1_sbsp_collision_bsp* collision = g_h1_cache_file->block_get(bsp->collision_bsp, 0);
	if (!collision || collision->bsp3d_nodes.count <= 0)
	{
		return NONE;
	}

	int32 node_index = 0;
	for (int32 guard = 0; node_index >= 0 && guard < collision->bsp3d_nodes.count; guard++)
	{
		const h1_sbsp_collision_bsp_bsp3d_nodes* node = g_h1_cache_file->block_get(collision->bsp3d_nodes, node_index);
		const h1_sbsp_collision_bsp_planes* plane = node ? g_h1_cache_file->block_get(collision->planes, node->plane) : NULL;
		if (!plane)
		{
			return NONE;
		}
		const real32 side = plane->plane.n.i * point->x + plane->plane.n.j * point->y + plane->plane.n.k * point->z - plane->plane.d;
		node_index = side >= 0.f ? node->front_child : node->back_child;
	}

	if (node_index == NONE || node_index >= 0)
	{
		return NONE;
	}

	const h1_sbsp_leaves* leaf = g_h1_cache_file->block_get(bsp->leaves, node_index & 0x7FFFFFFF);
	return leaf ? leaf->cluster : NONE;
}

static std::shared_ptr<const s_h1_sound_data> h1_sound_data_get(datum sound_index)
{
	if (sound_index == NONE)
	{
		return NULL;
	}

	auto found = g_h1_sound.sounds.find(sound_index);
	if (found != g_h1_sound.sounds.end())
	{
		return found->second;
	}

	std::shared_ptr<s_h1_sound_data> data;
	const h1_snd* sound = (const h1_snd*)g_h1_cache_file->tag_get('snd!', sound_index);
	if (sound && sound->pitch_ranges.count > 0)
	{
		data = std::make_shared<s_h1_sound_data>();
		data->channel_count = sound->encoding == 1 ? 2 : 1;
		data->sample_rate = sound->sample_rate == 1 ? 44100 : 22050;
		data->gain = sound->gain_modifier > 0.f ? sound->gain_modifier : 1.f;
		data->zero_gain_modifier = sound->gain_modifier_2;
		data->one_gain_modifier = sound->gain_modifier_3;
		data->zero_pitch_modifier = sound->pitch_modifier > 0.f ? sound->pitch_modifier : 1.f;
		data->one_pitch_modifier = sound->pitch_modifier_2 > 0.f ? sound->pitch_modifier_2 : 1.f;
		data->maximum_bend_per_second = sound->maximum_bend_per_second;

		const real_bounds* class_distances = VALID_INDEX(sound->f_class, NUMBEROF(k_h1_sound_class_distances)) ? &k_h1_sound_class_distances[sound->f_class] : NULL;
		data->minimum_distance = sound->minimum_distance > 0.f ? sound->minimum_distance : (class_distances ? class_distances->lower : 1.f);
		data->maximum_distance = sound->maximum_distance > 0.f ? sound->maximum_distance : (class_distances ? class_distances->upper : 10.f);
		data->minimum_distance = MAX(data->minimum_distance, 0.1f);
		data->maximum_distance = MAX(data->maximum_distance, data->minimum_distance + 0.1f);

		for (int32 range_index = 0; range_index < sound->pitch_ranges.count; range_index++)
		{
		const h1_snd_pitch_ranges* pitch_range = g_h1_cache_file->block_get(sound->pitch_ranges, range_index);
		s_h1_sound_pitch_range range;
		range.bend_bounds = pitch_range->bend_bounds;
		if (range.bend_bounds.lower <= 0.f && range.bend_bounds.upper <= 0.f)
		{
			range.bend_bounds = { 0.f, FLT_MAX };
		}
		range.playback_rate = pitch_range->playback_rate > 0.f ? pitch_range->playback_rate : 1.f;
		range.first_permutation = (int32)data->permutations.size();

		// the first actual permutation count permutations are the variations, the rest continue them
		const int32 permutation_count = pitch_range->permutations.count;
		const int32 variation_count = pitch_range->actual_permutation_count > 0 ? MIN((int32)pitch_range->actual_permutation_count, permutation_count) : permutation_count;
		for (int32 i = 0; i < variation_count; i++)
		{
			s_h1_sound_permutation variation;
			variation.gain = 1.f;
			int32 permutation_index = i;
			for (int32 guard = 0; permutation_index != NONE && guard < permutation_count; guard++)
			{
				const h1_snd_pitch_ranges_permutations* permutation = g_h1_cache_file->block_get(pitch_range->permutations, permutation_index);
				if (!permutation || !h1_sound_decode_permutation(sound, permutation, data->channel_count, variation.samples))
				{
					break;
				}
				if (guard == 0)
				{
					variation.gain = permutation->gain;
				}
				permutation_index = permutation->next_permutation_index;
			}
			if (variation.samples.size() >= (size_t)data->channel_count * 2)
			{
				variation.pitch_range = range_index;
				data->permutations.push_back(std::move(variation));
			}
		}
		range.permutation_count = (int32)data->permutations.size() - range.first_permutation;
		data->pitch_ranges.push_back(range);
		}

		if (data->permutations.empty())
		{
			h1_log("sound: %s has no playable permutations", g_h1_cache_file->tag_name_get(sound_index));
			data.reset();
		}
	}

	g_h1_sound.sounds[sound_index] = data;
	return data;
}

static bool h1_sound_decode_permutation(const h1_snd* sound, const h1_snd_pitch_ranges_permutations* permutation, int32 channel_count, std::vector<int16>& samples)
{
	if (permutation->sample_size <= 0 || (uint64)permutation->sample_offset + (uint32)permutation->sample_size > g_h1_cache_file->file_size())
	{
		return false;
	}
	const uint8* source = g_h1_cache_file->file_data() + permutation->sample_offset;
	const uint32 size = (uint32)permutation->sample_size;

	switch (permutation->compression)
	{
	case _h1_sound_compression_none:
	{
		const size_t first = samples.size();
		samples.resize(first + size / sizeof(int16));
		csmemcpy(&samples[first], source, (size / sizeof(int16)) * sizeof(int16));
		return true;
	}
	case _h1_sound_compression_xbox_adpcm:
		h1_sound_decode_xbox_adpcm(source, size, channel_count, samples);
		return true;
	default:
		return false;
	}
}

// xbox adpcm: per channel 36 byte blocks (predictor, step index, 64 nibbles), channels interleave every 4 bytes
static void h1_sound_decode_xbox_adpcm(const uint8* source, uint32 size, int32 channel_count, std::vector<int16>& samples)
{
	const uint32 block_size = k_h1_sound_xbox_adpcm_block_size * channel_count;
	const uint32 block_count = size / block_size;
	const size_t first = samples.size();
	samples.resize(first + (size_t)block_count * k_h1_sound_xbox_adpcm_block_samples * channel_count);
	int16* output = &samples[first];

	for (uint32 block = 0; block < block_count; block++)
	{
		const uint8* data = source + block * block_size;
		for (int32 channel = 0; channel < channel_count; channel++)
		{
			int32 predictor = *(const int16*)(data + channel * 4);
			int32 step_index = MIN((int32)data[channel * 4 + 2], 88);
			int16* channel_output = output + channel;

			for (int32 chunk = 0; chunk < 8; chunk++)
			{
				const uint8* bytes = data + 4 * channel_count + (chunk * channel_count + channel) * 4;
				for (int32 i = 0; i < 8; i++)
				{
					const int32 nibble = (bytes[i / 2] >> ((i & 1) * 4)) & 0xF;
					const int32 step = k_h1_ima_step_table[step_index];
					int32 difference = step >> 3;
					if (nibble & 1) difference += step >> 2;
					if (nibble & 2) difference += step >> 1;
					if (nibble & 4) difference += step;
					predictor += (nibble & 8) ? -difference : difference;
					predictor = PIN(predictor, -32768, 32767);
					step_index = PIN(step_index + k_h1_ima_index_table[nibble], 0, 88);
					channel_output[(chunk * 8 + i) * channel_count] = (int16)predictor;
				}
			}
		}
		output += k_h1_sound_xbox_adpcm_block_samples * channel_count;
	}
	return;
}

static uint32 h1_sound_random(uint32* seed)
{
	*seed = *seed * 1664525u + 1013904223u;
	return *seed >> 8;
}

static real32 h1_sound_random_range(real32 lower, real32 upper)
{
	const real32 t = (real32)(h1_sound_random(&g_h1_sound.random) & 0xFFFF) / 65535.f;
	return lower + (upper - lower) * t;
}

static void h1_sound_mixer_main(void)
{
	HANDLE event = CreateEvent(NULL, FALSE, FALSE, NULL);
	WAVEFORMATEX format = {};
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 2;
	format.nSamplesPerSec = k_h1_sound_output_rate;
	format.wBitsPerSample = 16;
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

	HWAVEOUT wave_out = NULL;
	if (waveOutOpen(&wave_out, WAVE_MAPPER, &format, (DWORD_PTR)event, 0, CALLBACK_EVENT) != MMSYSERR_NOERROR)
	{
		h1_log("sound: could not open the wave output");
		CloseHandle(event);
		return;
	}

	static int16 buffers[k_h1_sound_buffer_count][k_h1_sound_buffer_frames * 2];
	WAVEHDR headers[k_h1_sound_buffer_count] = {};
	for (int32 i = 0; i < k_h1_sound_buffer_count; i++)
	{
		csmemset(buffers[i], 0, sizeof(buffers[i]));
		headers[i].lpData = (LPSTR)buffers[i];
		headers[i].dwBufferLength = sizeof(buffers[i]);
		waveOutPrepareHeader(wave_out, &headers[i], sizeof(WAVEHDR));
		waveOutWrite(wave_out, &headers[i], sizeof(WAVEHDR));
	}

	// development: with a file named h1_sound_capture in the game folder the first 20 seconds of output go to h1_sound_capture.wav
	HANDLE capture = INVALID_HANDLE_VALUE;
	uint32 capture_size = 0;
	DWORD written;
	if (GetFileAttributesW(L"h1_sound_capture") != INVALID_FILE_ATTRIBUTES)
	{
		capture = CreateFileW(L"h1_sound_capture.wav", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		uint8 header[44] = {};
		WriteFile(capture, header, sizeof(header), &written, NULL);
	}

	while (g_h1_sound.mixer_running)
	{
		WaitForSingleObject(event, 50);
		for (int32 i = 0; i < k_h1_sound_buffer_count; i++)
		{
			if (headers[i].dwFlags & WHDR_DONE)
			{
				h1_sound_mix(buffers[i], k_h1_sound_buffer_frames);
				waveOutWrite(wave_out, &headers[i], sizeof(WAVEHDR));
				if (capture != INVALID_HANDLE_VALUE && capture_size < k_h1_sound_output_rate * 4 * 20)
				{
					WriteFile(capture, buffers[i], sizeof(buffers[i]), &written, NULL);
					capture_size += sizeof(buffers[i]);
					if (capture_size >= k_h1_sound_output_rate * 4 * 20)
					{
						// header: RIFF, WAVE fmt, data
						const uint32 riff_size = 36 + capture_size;
						const uint32 format_size = 16;
						SetFilePointer(capture, 0, NULL, FILE_BEGIN);
						WriteFile(capture, "RIFF", 4, &written, NULL);
						WriteFile(capture, &riff_size, 4, &written, NULL);
						WriteFile(capture, "WAVEfmt ", 8, &written, NULL);
						WriteFile(capture, &format_size, 4, &written, NULL);
						WriteFile(capture, &format, 16, &written, NULL);
						WriteFile(capture, "data", 4, &written, NULL);
						WriteFile(capture, &capture_size, 4, &written, NULL);
						CloseHandle(capture);
						capture = INVALID_HANDLE_VALUE;
					}
				}
			}
		}
	}

	if (capture != INVALID_HANDLE_VALUE)
	{
		CloseHandle(capture);
	}

	waveOutReset(wave_out);
	for (int32 i = 0; i < k_h1_sound_buffer_count; i++)
	{
		waveOutUnprepareHeader(wave_out, &headers[i], sizeof(WAVEHDR));
	}
	waveOutClose(wave_out);
	CloseHandle(event);
	return;
}

static void h1_sound_mix(int16* output, int32 frame_count)
{
	static real32 accumulator[k_h1_sound_buffer_frames * 2];
	csmemset(accumulator, 0, sizeof(real32) * frame_count * 2);

	// master gain ramps over a quarter second
	const real32 master_start = g_h1_sound.master_gain;
	const real32 master_step = (real32)frame_count / (k_h1_sound_output_rate * 0.25f);
	const real32 master_target = g_h1_sound.master_target;
	real32 master_end = master_start < master_target ? MIN(master_start + master_step, master_target) : MAX(master_start - master_step, master_target);
	g_h1_sound.master_gain = master_end;

	{
		std::lock_guard<std::mutex> lock(g_h1_sound.voices_lock);
		for (const auto& voice_pointer : g_h1_sound.voices)
		{
			s_h1_voice* voice = voice_pointer.get();
			if (voice->finished || voice->remove)
			{
				continue;
			}

			const s_h1_sound_data* sound = voice->sound.get();
			const int32 channel_count = sound->channel_count;
			const real32 pitch = voice->pitch > 0.f ? voice->pitch : 1.f;
			const double step = (double)sound->sample_rate / k_h1_sound_output_rate * pitch * sound->pitch_ranges[voice->pitch_range].playback_rate;
			const real32 start_gain[2] = { voice->gain[0] * master_start, voice->gain[1] * master_start };
			const real32 end_gain[2] = { voice->target_gain[0] * master_end, voice->target_gain[1] * master_end };
			voice->gain[0] = voice->target_gain[0];
			voice->gain[1] = voice->target_gain[1];

			for (int32 frame = 0; frame < frame_count; frame++)
			{
				const s_h1_sound_permutation* permutation = &sound->permutations[voice->permutation];
				const int32 permutation_frames = (int32)(permutation->samples.size() / channel_count);
				int32 index = (int32)voice->position;
				if (index + 1 >= permutation_frames)
				{
					if (!voice->looping)
					{
						voice->finished = true;
						break;
					}
					// next variation, never the same one twice in a row, from the pitch range of the voice's pitch
					voice->position -= (double)(permutation_frames - 1);
					const int32 range = h1_sound_pitch_range_find(sound, pitch, voice->pitch_range);
					voice->permutation = h1_sound_permutation_next(sound, range, range == voice->pitch_range ? voice->permutation : NONE, &voice->random);
					voice->pitch_range = range;
					permutation = &sound->permutations[voice->permutation];
					index = MIN((int32)voice->position, (int32)(permutation->samples.size() / channel_count) - 2);
				}

				const real32 fraction = (real32)(voice->position - index);
				const real32 t = (real32)frame / frame_count;
				const real32 scale = permutation->gain / 32768.f;
				const int16* samples = &permutation->samples[index * channel_count];
				const real32 first = samples[0] + (samples[channel_count] - samples[0]) * fraction;
				const real32 second = channel_count == 2 ? samples[1] + (samples[3] - samples[1]) * fraction : first;
				accumulator[frame * 2 + 0] += first * scale * (start_gain[0] + (end_gain[0] - start_gain[0]) * t);
				accumulator[frame * 2 + 1] += second * scale * (start_gain[1] + (end_gain[1] - start_gain[1]) * t);
				voice->position += step;
			}
		}
	}

	for (int32 i = 0; i < frame_count * 2; i++)
	{
		output[i] = (int16)PIN((int32)(accumulator[i] * 32767.f), -32768, 32767);
	}
	return;
}

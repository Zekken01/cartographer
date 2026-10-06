#include "stdafx.h"
#include "h1_recordings.h"

#include "h1_cache_file.h"
#include "h1_log.h"
#include "h1_vehicle_physics.h"

#include "math/real_math.h"
#include "objects/objects.h"
#include "objects/object_types.h"
#include "units/unit_control.h"
#include "units/units.h"

/*
* recorded_animations.c and recorded_animation_playback.c: halo 1's recorded unit control (cutscene actors walking their paths,
* vehicles flying theirs). A recording is a stream of control events (throttle, facing/aiming/looking angle deltas, flags,
* animation state) applied one tick at a time; the decoded halo 1 unit control drives the halo 2 unit through unit_control.
* Every recording in the shipped maps uses the current codec (version 4).
*/

/* ---------- constants */

enum
{
	k_h1_maximum_recording_threads = 64,
	k_h1_recorded_animation_version = 4,
};

enum
{
	_playback_end = 1,
	_playback_animation_state_set,
	_playback_aiming_speed_set,
	_playback_control_flags_set,
	_playback_weapon_index_set,
	_playback_throttle_set,
	_playback_vector_char_difference_set,
	_playback_vector_short_difference_set = 15,
	k_playback_event_type_count = 23,
};

enum
{
	_time_delta_zero,
	_time_delta_one,
	_time_delta_byte,
	_time_delta_word,
};

enum
{
	_control_vector_facing_bit,
	_control_vector_aiming_bit,
	_control_vector_looking_bit,
};

enum
{
	_h1_unit_control_crouch_bit = 0,
	_h1_unit_control_jump_bit,
};

/* ---------- structures */

// unit_control_data, as the recordings write it
#pragma pack(push, 1)
struct s_h1_recorded_unit_control
{
	uint8 animation_state;
	uint8 aiming_speed;
	uint16 control_flags;
	int16 weapon_index;
	int16 grenade_index;	// version 2
	int16 zoom_level;		// version 3
	int16 pad;
	real_vector3d throttle;
	real32 primary_trigger;	// version 1
	real_vector3d facing_vector;
	real_vector3d aiming_vector;
	real_vector3d looking_vector;
};
#pragma pack(pop)
static_assert(sizeof(s_h1_recorded_unit_control) == 0x40);

struct s_h1_direction_controller
{
	int16 yaw;
	int16 pitch;
};

struct s_h1_playback_state
{
	s_h1_direction_controller facing;
	s_h1_direction_controller aiming;
	s_h1_direction_controller looking;
};

struct s_h1_recording_thread
{
	bool active;
	bool finished;
	bool killed;
	bool delete_on_complete;
	bool hover_on_complete;				// recording_play_and_hover: a vehicle hovers where the recording leaves it
	datum unit_index;
	int16 ticks_left;
	int32 relative_ticks;
	const uint8* event_stream;
	const uint8* event_stream_end;
	s_h1_recorded_unit_control control;
	s_h1_playback_state state;
	int16 animation_index;
};

/* ---------- globals */

static s_h1_recording_thread g_h1_recordings[k_h1_maximum_recording_threads];

/* ---------- prototypes */

static s_h1_recording_thread* h1_recording_thread_get(datum unit_index);
static void h1_recording_unit_control_initialize(s_h1_recorded_unit_control* control, const uint8** stream, int32 unit_control_data_version);
static bool h1_recording_apply_event_stream(s_h1_recording_thread* thread);
static void h1_recording_unit_control(const s_h1_recording_thread* thread);

/* ---------- public code */

void h1_recordings_reset(void)
{
	csmemset(g_h1_recordings, 0, sizeof(g_h1_recordings));
	return;
}

bool h1_recording_play(datum unit_index, int16 animation_index, bool delete_on_complete, bool hover_on_complete)
{
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();
	const h1_scnr_recorded_animations* animation = g_h1_cache_file->block_get(scenario->recorded_animations, animation_index);
	if (!animation || unit_index == NONE)
	{
		return false;
	}
	if (!object_try_and_get_and_verify_type(unit_index, _object_mask_unit))
	{
		h1_log("recordings: %s on an object that isn't a unit", animation->name);
		return false;
	}
	if (animation->version != k_h1_recorded_animation_version)
	{
		h1_log("recordings: %s is version %d", animation->name, animation->version);
		return false;
	}

	s_h1_recording_thread* thread = h1_recording_thread_get(unit_index);
	if (thread && !thread->finished)
	{
		h1_log("recordings: trying to play %s while another is playing", animation->name);
		return false;
	}
	for (int32 i = 0; !thread && i < k_h1_maximum_recording_threads; i++)
	{
		thread = !g_h1_recordings[i].active ? &g_h1_recordings[i] : NULL;
	}
	const uint8* stream = (const uint8*)g_h1_cache_file->data_get(animation->recorded_animation_event_stream);
	if (!thread || !stream)
	{
		return false;
	}

	*thread = {};
	thread->active = true;
	thread->unit_index = unit_index;
	thread->ticks_left = animation->length_of_animation;
	thread->animation_index = animation_index;
	thread->delete_on_complete = delete_on_complete;
	thread->hover_on_complete = hover_on_complete;
	// unit_set_actively_controlled: halo 2 only takes the control of a unit it's told is actively controlled (and powers a vehicle's
	// driver seat for it)
	SET_BIT(((unit_datum*)object_get(unit_index))->unit.unit_flags, 1, true);
	thread->event_stream = stream;
	thread->event_stream_end = stream + animation->recorded_animation_event_stream.size;
	// recorded_animation_initialize_event_stream: the unit control, then the playback state
	h1_recording_unit_control_initialize(&thread->control, &thread->event_stream, animation->unit_control_data_version);
	csmemcpy(&thread->state, thread->event_stream, sizeof(thread->state));
	thread->event_stream += sizeof(thread->state);
	return true;
}

void h1_recording_kill(datum unit_index)
{
	s_h1_recording_thread* thread = h1_recording_thread_get(unit_index);
	if (thread)
	{
		thread->finished = true;
		thread->killed = true;
	}
	return;
}

int16 h1_recording_time(datum unit_index)
{
	const s_h1_recording_thread* thread = h1_recording_thread_get(unit_index);
	return thread && !thread->finished ? thread->ticks_left : 0;
}

bool h1_recording_controlling_unit(datum unit_index)
{
	const s_h1_recording_thread* thread = h1_recording_thread_get(unit_index);
	return thread && !thread->finished;
}

// recorded_animations_update, once per tick
void h1_recordings_update(void)
{
	for (s_h1_recording_thread& thread : g_h1_recordings)
	{
		if (!thread.active)
		{
			continue;
		}
		if (!object_try_and_get_and_verify_type(thread.unit_index, _object_mask_unit))
		{
			thread.active = false;
			continue;
		}
		if (!thread.finished)
		{
			thread.ticks_left--;
			const bool finished = !h1_recording_apply_event_stream(&thread);
			thread.relative_ticks++;
			h1_recording_unit_control(&thread);
			thread.finished = finished;
		}
		else
		{
			// the unit stops where the recording left it
			thread.control.throttle = *global_zero_vector3d;
			thread.control.control_flags = 0;
			thread.control.primary_trigger = 0.f;
			h1_recording_unit_control(&thread);
			unit_datum* unit = (unit_datum*)object_get(thread.unit_index);
			if (unit->unit.actor_index == NONE && unit->unit.player_index == NONE)
			{
				SET_BIT(unit->unit.unit_flags, 1, false);
			}
			if (thread.delete_on_complete)
			{
				object_delete(thread.unit_index);
			}
			else if (thread.hover_on_complete)
			{
				h1_vehicle_hover_set(thread.unit_index, true);
			}
			thread.active = false;
		}
	}
	return;
}

/* ---------- private code */

static s_h1_recording_thread* h1_recording_thread_get(datum unit_index)
{
	for (s_h1_recording_thread& thread : g_h1_recordings)
	{
		if (thread.active && thread.unit_index == unit_index)
		{
			return &thread;
		}
	}
	return NULL;
}

// recorded_animation_initialize_unit_control: the fields of each unit control data version
static void h1_recording_unit_control_initialize(s_h1_recorded_unit_control* control, const uint8** stream, int32 unit_control_data_version)
{
	csmemset(control, 0, sizeof(*control));
	control->zoom_level = NONE;
	auto read = [stream](void* destination, uint32 size)
	{
		if (destination)
		{
			csmemcpy(destination, *stream, size);
		}
		*stream += size;
	};
	// version 0
	read(&control->animation_state, 1);
	read(&control->aiming_speed, 1);
	read(&control->control_flags, 2);
	read(&control->weapon_index, 2);
	read(NULL, 2);
	read(&control->throttle, sizeof(real_vector2d));
	read(&control->facing_vector, sizeof(real_vector3d));
	read(&control->aiming_vector, sizeof(real_vector3d));
	read(&control->looking_vector, sizeof(real_vector3d));
	if (unit_control_data_version > 1) read(&control->primary_trigger, 4);
	if (unit_control_data_version > 2) read(&control->grenade_index, 2);
	if (unit_control_data_version > 3) read(&control->zoom_level, 2);
	return;
}

static void h1_direction_controller_update(s_h1_direction_controller* controller, int16 delta_yaw, int16 delta_pitch)
{
	controller->yaw += delta_yaw;
	if (controller->yaw > 1000)
	{
		controller->yaw -= 1000;
	}
	else if (controller->yaw < -1000)
	{
		controller->yaw += 1000;
	}
	controller->pitch += delta_pitch;
	return;
}

static void h1_direction_controller_vector(const s_h1_direction_controller* controller, real_vector3d* out_vector)
{
	const real32 yaw = (real32)controller->yaw * 0.0031415927f;
	const real32 pitch = (real32)controller->pitch * 0.0031415927f;
	out_vector->i = cosf(yaw) * cosf(pitch);
	out_vector->j = sinf(yaw) * cosf(pitch);
	out_vector->k = sinf(pitch);
	return;
}

// apply_vector_char_difference / apply_vector_short_difference
static void h1_recording_apply_vectors(s_h1_recording_thread* thread, int32 vectors, int16 delta_yaw, int16 delta_pitch)
{
	s_h1_playback_state* state = &thread->state;
	s_h1_recorded_unit_control* control = &thread->control;
	if (TEST_BIT(vectors, _control_vector_facing_bit))
	{
		h1_direction_controller_update(&state->facing, delta_yaw, delta_pitch);
		h1_direction_controller_vector(&state->facing, &control->facing_vector);
	}
	if (TEST_BIT(vectors, _control_vector_aiming_bit))
	{
		if (TEST_BIT(vectors, _control_vector_facing_bit))
		{
			state->aiming = state->facing;
			control->aiming_vector = control->facing_vector;
		}
		else
		{
			h1_direction_controller_update(&state->aiming, delta_yaw, delta_pitch);
			h1_direction_controller_vector(&state->aiming, &control->aiming_vector);
		}
	}
	if (TEST_BIT(vectors, _control_vector_looking_bit))
	{
		if (TEST_BIT(vectors, _control_vector_facing_bit))
		{
			state->looking = state->facing;
			control->looking_vector = control->facing_vector;
		}
		else if (TEST_BIT(vectors, _control_vector_aiming_bit))
		{
			state->looking = state->aiming;
			control->looking_vector = control->aiming_vector;
		}
		else
		{
			h1_direction_controller_update(&state->looking, delta_yaw, delta_pitch);
			h1_direction_controller_vector(&state->looking, &control->looking_vector);
		}
	}
	return;
}

// recorded_animation_apply_event_stream: the events up to this tick, false once the stream has ended
static bool h1_recording_apply_event_stream(s_h1_recording_thread* thread)
{
	int32* ticks = &thread->relative_ticks;
	uint8 event_type = _playback_end;
	uint16 time_delta = 0;
	while (thread->event_stream < thread->event_stream_end)
	{
		const uint8* header = thread->event_stream;
		event_type = header[0] >> 2;
		uint32 header_size = 1;
		switch (header[0] & 3)
		{
		case _time_delta_zero: time_delta = 0; break;
		case _time_delta_one: time_delta = 1; break;
		case _time_delta_byte: time_delta = header[1]; header_size = 2; break;
		default: csmemcpy(&time_delta, header + 1, sizeof(time_delta)); header_size = 3; break;
		}
		if (*ticks < time_delta || event_type == _playback_end)
		{
			break;
		}
		thread->event_stream += header_size;
		const uint8* data = thread->event_stream;
		s_h1_recorded_unit_control* control = &thread->control;
		switch (event_type)
		{
		case _playback_animation_state_set:
			control->animation_state = data[0];
			thread->event_stream += 1;
			break;
		case _playback_aiming_speed_set:
			control->aiming_speed = data[0];
			thread->event_stream += 1;
			break;
		case _playback_control_flags_set:
			csmemcpy(&control->control_flags, data, 2);
			thread->event_stream += 2;
			break;
		case _playback_weapon_index_set:
			csmemcpy(&control->weapon_index, data, 2);
			thread->event_stream += 2;
			break;
		case _playback_throttle_set:
			csmemcpy(&control->throttle, data, sizeof(real_vector2d));
			control->throttle.k = 0.f;
			thread->event_stream += sizeof(real_vector2d);
			break;
		default:
			if (event_type >= _playback_vector_short_difference_set && event_type < k_playback_event_type_count)
			{
				int16 deltas[2];
				csmemcpy(deltas, data, sizeof(deltas));
				h1_recording_apply_vectors(thread, event_type - _playback_vector_short_difference_set, deltas[0], deltas[1]);
				thread->event_stream += sizeof(deltas);
			}
			else if (event_type >= _playback_vector_char_difference_set && event_type < _playback_vector_short_difference_set)
			{
				h1_recording_apply_vectors(thread, event_type - _playback_vector_char_difference_set, (int8)data[0], (int8)data[1]);
				thread->event_stream += 2;
			}
			break;
		}
		*ticks -= time_delta;
	}
	return !(event_type == _playback_end && *ticks == time_delta) && thread->event_stream < thread->event_stream_end;
}

// unit_control: the halo 1 control as halo 2's (unit_control_data_new's defaults for the rest)
static void h1_recording_unit_control(const s_h1_recording_thread* thread)
{
	unit_control_data data;
	Memory::GetAddress<void(__cdecl*)(unit_control_data*)>(0x138C92)(&data);
	const s_h1_recorded_unit_control* control = &thread->control;
	data.aiming_speed = control->aiming_speed;
	data.control_flags = 0;
	if (TEST_BIT(control->control_flags, _h1_unit_control_crouch_bit)) data.control_flags |= FLAG(0);
	if (TEST_BIT(control->control_flags, _h1_unit_control_jump_bit)) data.control_flags |= FLAG(1);
	data.throttle = control->throttle;
	data.primary_trigger = control->primary_trigger;
	auto normalized = [](const real_vector3d* vector, real_vector3d* out)
	{
		*out = *vector;
		if (normalize3d(out) == 0.f)
		{
			*out = *global_forward3d;
		}
	};
	normalized(&control->facing_vector, &data.facing_vector);
	normalized(&control->aiming_vector, &data.aiming_vector);
	normalized(&control->looking_vector, &data.looking_vector);
	unit_control(thread->unit_index, &data);
	return;
}

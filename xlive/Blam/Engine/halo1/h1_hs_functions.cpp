#include "stdafx.h"
#include "h1_hs.h"
#include "h1_hs_internal.h"

#include "h1_cache_file.h"
#include "h1_camera.h"
#include "h1_log.h"
#include "h1_map_loader.h"
#include "h1_sound.h"

#include "effects/player_effects.h"
#include "game/game.h"
#include "game/game_options.h"
#include "game/game_time.h"
#include "game/players.h"
#include "main/main.h"
#include "math/real_math.h"
#include "objects/object_types.h"
#include "objects/objects.h"

#include <vector>

/*
* The halo 1 script library (hs.c's function definitions, hs_library_external.c): the procedures of the functions the runtime
* evaluates as macro functions. Their halo 1 behavior where the systems they drive exist (players, trigger volumes, sounds,
* the screen, the game's flow, which halo 2's own functions do); the type's default for the rest, logged once per map.
*/

namespace h1_hs
{

/* ---------- constants */

enum
{
	k_maximum_object_lists = 256,
};

/* ---------- structures */

struct s_object_list
{
	bool used;
	int16 reference_count;
	std::vector<datum> objects;
};

struct s_scripted_looping_sound
{
	datum definition_index;
	int32 handle;
	datum object_index;
	real32 scale;
};

struct s_hs_library_globals
{
	s_object_list object_lists[k_maximum_object_lists];
	std::vector<datum> named_objects;			// scenario object name index: the object it names
	std::vector<real32> device_group_values;
	std::vector<s_scripted_looping_sound> looping_sounds;
	std::vector<std::pair<datum, int32>> scripted_sound_end_times;	// sound_impulse_time
};

/* ---------- globals */

static s_hs_library_globals g_hs_library;

/* ---------- halo 2 functions */

#define H2_FUNCTION(offset, type) Memory::GetAddress<type>(offset)

typedef void (__cdecl* t_void)(void);
typedef bool (__cdecl* t_bool)(void);
typedef void (__cdecl* t_void_bool)(bool);
typedef bool (__cdecl* t_bool_bool)(bool);
typedef void (__cdecl* t_void_datum)(datum);
typedef real32 (__cdecl* t_real_datum)(datum);
typedef void (__cdecl* t_fade)(real32, real32, real32, int16);

/* ---------- arguments */

#define ARGUMENT_BOOLEAN(i) ((uint8)arguments[(i)] != 0)
#define ARGUMENT_REAL(i) (*(const real32*)&arguments[(i)])
#define ARGUMENT_SHORT(i) ((int16)arguments[(i)])
#define ARGUMENT_LONG(i) (arguments[(i)])
#define ARGUMENT_STRING(i) ((const char*)arguments[(i)])

static int32 hs_real_result(real32 value)
{
	return *(int32*)&value;
}

/* ---------- object lists (object_lists.c) */

int32 object_list_new(void)
{
	for (int32 list_index = 0; list_index < k_maximum_object_lists; list_index++)
	{
		s_object_list* list = &g_hs_library.object_lists[list_index];
		if (!list->used)
		{
			list->used = true;
			list->reference_count = 0;
			list->objects.clear();
			return list_index;
		}
	}
	h1_log("hs: ran out of object lists");
	return NONE;
}

static s_object_list* object_list_get_internal(int32 list_index)
{
	return VALID_INDEX(list_index, k_maximum_object_lists) && g_hs_library.object_lists[list_index].used ? &g_hs_library.object_lists[list_index] : NULL;
}

void object_list_add(int32 list_index, datum object_index)
{
	s_object_list* list = object_list_get_internal(list_index);
	if (list)
	{
		list->objects.push_back(object_index);
	}
	return;
}

int16 object_list_count(int32 list_index)
{
	const s_object_list* list = object_list_get_internal(list_index);
	return list ? (int16)list->objects.size() : 0;
}

datum object_list_get(int32 list_index, int16 index)
{
	const s_object_list* list = object_list_get_internal(list_index);
	return list && VALID_INDEX(index, (int16)list->objects.size()) ? list->objects[index] : NONE;
}

void object_list_add_reference(int32 list_index)
{
	s_object_list* list = object_list_get_internal(list_index);
	if (list)
	{
		list->reference_count++;
	}
	return;
}

void object_list_remove_reference(int32 list_index)
{
	s_object_list* list = object_list_get_internal(list_index);
	if (list && list->reference_count > 0)
	{
		list->reference_count--;
	}
	return;
}

void object_list_gc(void)
{
	for (int32 list_index = 0; list_index < k_maximum_object_lists; list_index++)
	{
		s_object_list* list = &g_hs_library.object_lists[list_index];
		if (list->used && list->reference_count == 0)
		{
			list->used = false;
			list->objects.clear();
		}
	}
	return;
}

// ai_script.c object_list_from_ai_reference: the ai's actors' units (no ai yet: none)
int32 object_list_from_ai_reference(int32 ai_reference)
{
	return object_list_new();
}

/* ---------- objects */

datum object_index_from_name_index(int16 name_index)
{
	if (!VALID_INDEX(name_index, (int16)g_hs_library.named_objects.size()))
	{
		return NONE;
	}
	const datum object_index = g_hs_library.named_objects[name_index];
	return object_index != NONE && object_try_and_get(object_index) ? object_index : NONE;
}

bool hs_object_type_mask_test(int16 object_type_index, datum object_index)
{
	if (!VALID_INDEX(object_type_index, k_number_of_hs_object_types) || object_index == NONE || !object_try_and_get(object_index))
	{
		return false;
	}
	return TEST_BIT(hs_object_type_masks[object_type_index], object_get_type(object_index));
}

static bool hs_object_exists(datum object_index)
{
	return object_index != NONE && object_try_and_get(object_index) != NULL;
}

// the object's bounding sphere center, what halo 1 tests trigger volumes with
static bool hs_object_center(datum object_index, real_point3d* out_center)
{
	if (!hs_object_exists(object_index))
	{
		return false;
	}
	*out_center = object_get(object_index)->object.center;
	return true;
}

static bool hs_trigger_volume_test_object(int16 trigger_volume_index, datum object_index)
{
	real_point3d center;
	return hs_object_center(object_index, &center) && h1_maps_trigger_volume_test_point(trigger_volume_index, &center);
}

static bool hs_trigger_volume_test_objects(int16 trigger_volume_index, int32 list_index, bool all)
{
	bool result = all;
	const int16 count = object_list_count(list_index);
	for (int16 i = 0; i < count; i++)
	{
		if (hs_trigger_volume_test_object(trigger_volume_index, object_list_get(list_index, i)))
		{
			if (!result)
			{
				return true;
			}
		}
		else if (result)
		{
			return false;
		}
	}
	return result;
}

static int32 hs_players(void)
{
	const int32 list_index = object_list_new();
	for (int32 user_index = 0; user_index < 4; user_index++)
	{
		const datum player_index = player_index_from_user_index(user_index);
		const player_datum* player = player_index != NONE ? player_get(player_index) : NULL;
		if (player && player->unit_index != NONE)
		{
			object_list_add(list_index, player->unit_index);
		}
	}
	return list_index;
}

// hs_library_external.c hs_object_orient: the object to a cutscene flag's position and facing (players through their controls)
static void hs_object_orient(datum object_index, int16 cutscene_flag_index, bool set_position, bool set_facing)
{
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();
	const h1_scnr_cutscene_flags* flag = g_h1_cache_file->block_get(scenario->cutscene_flags, cutscene_flag_index);
	if (!hs_object_exists(object_index) || !flag)
	{
		return;
	}
	const real32 cos_pitch = cosf(flag->facing.pitch);
	real_vector3d forward = { cosf(flag->facing.yaw) * cos_pitch, sinf(flag->facing.yaw) * cos_pitch, sinf(flag->facing.pitch) };

	// halo 2's own: object_reset and its unit reset, the unit's desired facing, player_teleport, player_control_set_facing,
	// object_set_position
	H2_FUNCTION(0x135FD4, t_void_datum)(object_index);
	H2_FUNCTION(0x14A47E, t_void_datum)(object_index);
	bool player = false;
	uint8* unit = (uint8*)object_try_and_get_and_verify_type(object_index, _object_mask_unit);
	if (unit)
	{
		if (set_facing)
		{
			*(real_vector3d*)(unit + 0x15C) = forward;
			*(real_vector3d*)(unit + 0x168) = forward;
			*(real_vector3d*)(unit + 0x18C) = forward;
		}
		const datum player_index = H2_FUNCTION(0x51432, datum(__cdecl*)(datum))(object_index);
		if (player_index != NONE)
		{
			player = true;
			if (set_position)
			{
				H2_FUNCTION(0x56EA6, void(__cdecl*)(datum, datum, const real_point3d*))(player_index, NONE, &flag->position);
			}
			const int16 user_index = *(int16*)((uint8*)player_get(player_index) + 0x24);
			if (set_facing && user_index != NONE)
			{
				H2_FUNCTION(0x90977, void(__cdecl*)(int32, const real_vector3d*))(user_index, &forward);
			}
		}
	}
	H2_FUNCTION(0x136B7F, void(__cdecl*)(datum, const real_point3d*, const real_vector3d*, const real_vector3d*, int32))(
		object_index,
		set_position && !player ? &flag->position : NULL,
		set_facing && !player ? &forward : NULL,
		NULL,
		0);
	return;
}

/* ---------- sounds */

static void hs_sound_impulse_start(datum sound_index, datum object_index, real32 scale)
{
	if (sound_index == NONE)
	{
		return;
	}
	real_point3d position;
	const bool positional = object_index != NONE && hs_object_exists(object_index);
	if (positional)
	{
		object_get_origin(object_index, &position, false);
	}
	h1_sound_scripted_start(sound_index, positional ? &position : NULL, PIN(scale, 0.f, 1.f));

	const int32 end_time = (int32)game_time_get() + (int32)(h1_sound_duration(sound_index) * 30.f);
	for (auto& entry : g_hs_library.scripted_sound_end_times)
	{
		if (entry.first == sound_index)
		{
			entry.second = end_time;
			return;
		}
	}
	g_hs_library.scripted_sound_end_times.push_back({ sound_index, end_time });
	return;
}

static int32 hs_sound_impulse_time(datum sound_index)
{
	for (const auto& entry : g_hs_library.scripted_sound_end_times)
	{
		if (entry.first == sound_index)
		{
			return MAX(entry.second - (int32)game_time_get(), 0);
		}
	}
	return 0;
}

static void hs_sound_impulse_stop(datum sound_index)
{
	h1_sound_scripted_stop(sound_index);
	for (auto& entry : g_hs_library.scripted_sound_end_times)
	{
		if (entry.first == sound_index)
		{
			entry.second = 0;
		}
	}
	return;
}

static void hs_sound_looping_stop(datum definition_index)
{
	for (size_t i = 0; i < g_hs_library.looping_sounds.size();)
	{
		if (g_hs_library.looping_sounds[i].definition_index == definition_index)
		{
			h1_sound_looping_attached_delete(g_hs_library.looping_sounds[i].handle);
			g_hs_library.looping_sounds.erase(g_hs_library.looping_sounds.begin() + i);
			continue;
		}
		i++;
	}
	return;
}

static void hs_sound_looping_start(datum definition_index, datum object_index, real32 scale)
{
	if (definition_index == NONE)
	{
		return;
	}
	hs_sound_looping_stop(definition_index);
	const int32 handle = h1_sound_looping_attached_new(definition_index);
	if (handle)
	{
		g_hs_library.looping_sounds.push_back({ definition_index, handle, object_index, PIN(scale, 0.f, 1.f) });
	}
	return;
}

static void hs_sound_looping_set_scale(datum definition_index, real32 scale)
{
	for (s_scripted_looping_sound& sound : g_hs_library.looping_sounds)
	{
		if (sound.definition_index == definition_index)
		{
			sound.scale = PIN(scale, 0.f, 1.f);
		}
	}
	return;
}

// the scripts' looping sounds follow their objects (or the listener, unspatialized)
static void hs_sound_looping_update(void)
{
	for (const s_scripted_looping_sound& sound : g_hs_library.looping_sounds)
	{
		real_point3d position;
		if (sound.object_index != NONE && hs_object_exists(sound.object_index))
		{
			object_get_origin(sound.object_index, &position, false);
		}
		else if (!h1_sound_listener_point_get(&position))
		{
			continue;
		}
		h1_sound_looping_attached_update(sound.handle, &position, true, sound.scale);
	}
	return;
}

/* ---------- procedures */

static int32 hs_not(const int32* arguments) { return !ARGUMENT_BOOLEAN(0); }
static int32 hs_print(const int32* arguments) { h1_log("hs: print: %s", ARGUMENT_STRING(0)); return 0; }
static int32 hs_players_procedure(const int32* arguments) { return hs_players(); }

static int32 hs_volume_test_object(const int32* arguments) { return hs_trigger_volume_test_object(ARGUMENT_SHORT(0), ARGUMENT_LONG(1)); }
static int32 hs_volume_test_objects(const int32* arguments) { return hs_trigger_volume_test_objects(ARGUMENT_SHORT(0), ARGUMENT_LONG(1), false); }
static int32 hs_volume_test_objects_all(const int32* arguments) { return hs_trigger_volume_test_objects(ARGUMENT_SHORT(0), ARGUMENT_LONG(1), true); }

static int32 hs_volume_teleport_players_not_inside(const int32* arguments)
{
	const int32 players = hs_players();
	for (int16 i = 0; i < object_list_count(players); i++)
	{
		const datum unit_index = object_list_get(players, i);
		if (!hs_trigger_volume_test_object(ARGUMENT_SHORT(0), unit_index))
		{
			hs_object_orient(unit_index, ARGUMENT_SHORT(1), true, true);
		}
	}
	return 0;
}

static int32 hs_object_teleport(const int32* arguments) { hs_object_orient(ARGUMENT_LONG(0), ARGUMENT_SHORT(1), true, true); return 0; }
static int32 hs_object_set_facing(const int32* arguments) { hs_object_orient(ARGUMENT_LONG(0), ARGUMENT_SHORT(1), false, true); return 0; }

static int32 hs_object_destroy(const int32* arguments)
{
	if (hs_object_exists(ARGUMENT_LONG(0)))
	{
		object_delete(ARGUMENT_LONG(0));
	}
	return 0;
}

static int32 hs_list_get(const int32* arguments) { return object_list_get(ARGUMENT_LONG(0), ARGUMENT_SHORT(1)); }
static int32 hs_list_count(const int32* arguments) { return object_list_count(ARGUMENT_LONG(0)); }

static int32 hs_random_range(const int32* arguments)
{
	const int16 lower = ARGUMENT_SHORT(0);
	const int16 upper = ARGUMENT_SHORT(1);
	return upper > lower ? lower + rand() % (upper - lower) : lower;
}

static int32 hs_real_random_range(const int32* arguments)
{
	const real32 lower = ARGUMENT_REAL(0);
	const real32 upper = ARGUMENT_REAL(1);
	return hs_real_result(lower + (upper - lower) * ((real32)rand() / (real32)RAND_MAX));
}

static int32 hs_unit_get_health(const int32* arguments)
{
	return hs_real_result(hs_object_exists(ARGUMENT_LONG(0)) ? H2_FUNCTION(0x184477, t_real_datum)(ARGUMENT_LONG(0)) : -1.f);
}

static int32 hs_unit_get_shield(const int32* arguments)
{
	return hs_real_result(hs_object_exists(ARGUMENT_LONG(0)) ? H2_FUNCTION(0x18447C, t_real_datum)(ARGUMENT_LONG(0)) : -1.f);
}

static int32 hs_unit_kill(const int32* arguments)
{
	if (hs_object_exists(ARGUMENT_LONG(0)))
	{
		H2_FUNCTION(0x13B514, t_void_datum)(ARGUMENT_LONG(0));
	}
	return 0;
}

static int32 hs_unit_kill_silent(const int32* arguments)
{
	if (hs_object_exists(ARGUMENT_LONG(0)))
	{
		H2_FUNCTION(0x13B547, t_void_datum)(ARGUMENT_LONG(0));
	}
	return 0;
}

static int32 hs_device_group_get(const int32* arguments)
{
	const int16 group_index = ARGUMENT_SHORT(0);
	return hs_real_result(VALID_INDEX(group_index, (int16)g_hs_library.device_group_values.size()) ? g_hs_library.device_group_values[group_index] : 0.f);
}

static int32 hs_device_group_set(const int32* arguments)
{
	const int16 group_index = ARGUMENT_SHORT(0);
	if (VALID_INDEX(group_index, (int16)g_hs_library.device_group_values.size()))
	{
		g_hs_library.device_group_values[group_index] = PIN(ARGUMENT_REAL(1), 0.f, 1.f);
	}
	return true;
}

static int32 hs_game_time(const int32* arguments) { return (int32)game_time_get(); }
static int32 hs_game_difficulty_get(const int32* arguments) { return game_options_get()->difficulty; }
static int32 hs_game_is_cooperative(const int32* arguments) { return game_is_cooperative(); }
static int32 hs_structure_bsp_index(const int32* arguments) { return h1_maps_structure_bsp_index(); }

static int32 hs_switch_bsp(const int32* arguments)
{
	const int16 bsp_index = ARGUMENT_SHORT(0);
	h1_log("hs: switch_bsp %d (from %d)", bsp_index, h1_maps_structure_bsp_index());
	if (bsp_index != h1_maps_structure_bsp_index() && VALID_INDEX(bsp_index, g_h1_cache_file->structure_bsp_count()))
	{
		main_switch_structure_bsp_request(bsp_index);
	}
	return 0;
}

static int32 hs_fade_in(const int32* arguments)
{
	scripted_player_effect_screen_fade_in(ARGUMENT_REAL(0), ARGUMENT_REAL(1), ARGUMENT_REAL(2), ARGUMENT_SHORT(3));
	return 0;
}

static int32 hs_fade_out(const int32* arguments)
{
	H2_FUNCTION(0xA3CCA, t_fade)(ARGUMENT_REAL(0), ARGUMENT_REAL(1), ARGUMENT_REAL(2), ARGUMENT_SHORT(3));
	return 0;
}

static int32 hs_game_won(const int32* arguments) { h1_log("hs: game_won"); H2_FUNCTION(0x49FD7, t_void)(); return 0; }
static int32 hs_game_lost(const int32* arguments) { h1_log("hs: game_lost"); H2_FUNCTION(0x49905, t_void_bool)(true); return 0; }
static int32 hs_game_revert(const int32* arguments) { H2_FUNCTION(0x3951E, t_void)(); return 0; }
static int32 hs_game_save(const int32* arguments) { H2_FUNCTION(0x9E4FB, t_void)(); return 0; }
static int32 hs_game_save_cancel(const int32* arguments) { H2_FUNCTION(0x9E3EE, t_void)(); return 0; }
static int32 hs_game_save_no_timeout(const int32* arguments) { H2_FUNCTION(0x9E4CB, t_void)(); return 0; }
static int32 hs_game_saving(const int32* arguments) { return H2_FUNCTION(0x9E3D0, t_bool)(); }
static int32 hs_game_reverted(const int32* arguments) { return H2_FUNCTION(0x3015D, t_bool)(); }
static int32 hs_game_safe_to_save(const int32* arguments) { return H2_FUNCTION(0x9E66E, t_bool)(); }
static int32 hs_game_safe_to_speak(const int32* arguments) { return H2_FUNCTION(0x9E34C, t_bool)(); }
static int32 hs_game_all_quiet(const int32* arguments) { return H2_FUNCTION(0x9E376, t_bool)(); }

static int32 hs_cinematic_start(const int32* arguments) { H2_FUNCTION(0x3A6D0, t_void)(); return 0; }
static int32 hs_cinematic_stop(const int32* arguments) { H2_FUNCTION(0x3A8C9, t_void)(); return 0; }
static int32 hs_cinematic_skip_start_internal(const int32* arguments) { H2_FUNCTION(0x3A74B, t_void)(); return 0; }
static int32 hs_cinematic_skip_stop_internal(const int32* arguments) { H2_FUNCTION(0x3A755, t_void)(); return 0; }
static int32 hs_cinematic_show_letterbox(const int32* arguments) { H2_FUNCTION(0x3A75F, t_void_bool)(ARGUMENT_BOOLEAN(0)); return 0; }

static int32 hs_camera_control(const int32* arguments) { h1_camera_control(ARGUMENT_BOOLEAN(0)); return 0; }
static int32 hs_camera_set(const int32* arguments) { h1_camera_set(ARGUMENT_SHORT(0), ARGUMENT_SHORT(1), NONE); return 0; }
static int32 hs_camera_set_relative(const int32* arguments) { h1_camera_set(ARGUMENT_SHORT(0), ARGUMENT_SHORT(1), ARGUMENT_LONG(2)); return 0; }
static int32 hs_camera_set_first_person(const int32* arguments) { h1_camera_set_first_person(ARGUMENT_LONG(0)); return 0; }
static int32 hs_camera_time(const int32* arguments) { return h1_camera_time(); }

static int32 hs_players_unzoom_all(const int32* arguments) { H2_FUNCTION(0x9127B, t_void)(); return 0; }
static int32 hs_player_enable_input(const int32* arguments) { H2_FUNCTION(0x51464, t_void_bool)(ARGUMENT_BOOLEAN(0)); return 0; }
static int32 hs_player_camera_control(const int32* arguments) { return H2_FUNCTION(0x90B97, t_bool_bool)(ARGUMENT_BOOLEAN(0)); }
static int32 hs_show_hud(const int32* arguments) { return H2_FUNCTION(0x224801, t_bool_bool)(ARGUMENT_BOOLEAN(0)); }
static int32 hs_show_hud_help_text(const int32* arguments) { return H2_FUNCTION(0x220C62, t_bool_bool)(ARGUMENT_BOOLEAN(0)); }

static int32 hs_player_action_test_reset(const int32* arguments) { H2_FUNCTION(0x912B7, t_void)(); return 0; }
#define HS_PLAYER_ACTION_TEST(name, offset) static int32 name(const int32* arguments) { return H2_FUNCTION(offset, t_bool)(); }
HS_PLAYER_ACTION_TEST(hs_player_action_test_jump, 0x91342)
HS_PLAYER_ACTION_TEST(hs_player_action_test_primary_trigger, 0x9134F)
HS_PLAYER_ACTION_TEST(hs_player_action_test_grenade_trigger, 0x9136B)
HS_PLAYER_ACTION_TEST(hs_player_action_test_zoom, 0x91387)
HS_PLAYER_ACTION_TEST(hs_player_action_test_action, 0x912F7)
HS_PLAYER_ACTION_TEST(hs_player_action_test_accept, 0x912C5)
HS_PLAYER_ACTION_TEST(hs_player_action_test_back, 0x91417)
HS_PLAYER_ACTION_TEST(hs_player_action_test_look_relative_up, 0x913ED)
HS_PLAYER_ACTION_TEST(hs_player_action_test_look_relative_down, 0x913FB)
HS_PLAYER_ACTION_TEST(hs_player_action_test_look_relative_left, 0x913D1)
HS_PLAYER_ACTION_TEST(hs_player_action_test_look_relative_right, 0x913DF)
HS_PLAYER_ACTION_TEST(hs_player_action_test_look_relative_all_directions, 0x913C1)
HS_PLAYER_ACTION_TEST(hs_player_action_test_move_relative_all_directions, 0x913B1)

static int32 hs_player_effect_set_max_translation(const int32* arguments)
{
	H2_FUNCTION(0xA3ED7, void(__cdecl*)(real32, real32, real32))(ARGUMENT_REAL(0), ARGUMENT_REAL(1), ARGUMENT_REAL(2));
	return 0;
}

static int32 hs_player_effect_set_max_rotation(const int32* arguments)
{
	H2_FUNCTION(0xA3B9F, void(__cdecl*)(real32, real32, real32))(ARGUMENT_REAL(0), ARGUMENT_REAL(1), ARGUMENT_REAL(2));
	return 0;
}

static int32 hs_player_effect_start(const int32* arguments)
{
	H2_FUNCTION(0xA3BF5, void(__cdecl*)(real32, real32))(ARGUMENT_REAL(0), ARGUMENT_REAL(1));
	return 0;
}

static int32 hs_player_effect_stop(const int32* arguments)
{
	H2_FUNCTION(0xA3C30, void(__cdecl*)(real32))(ARGUMENT_REAL(0));
	return 0;
}

static int32 hs_sound_impulse_start_procedure(const int32* arguments) { hs_sound_impulse_start(ARGUMENT_LONG(0), ARGUMENT_LONG(1), ARGUMENT_REAL(2)); return 0; }
static int32 hs_sound_impulse_time_procedure(const int32* arguments) { return hs_sound_impulse_time(ARGUMENT_LONG(0)); }
static int32 hs_sound_impulse_stop_procedure(const int32* arguments) { hs_sound_impulse_stop(ARGUMENT_LONG(0)); return 0; }
static int32 hs_sound_looping_start_procedure(const int32* arguments) { hs_sound_looping_start(ARGUMENT_LONG(0), ARGUMENT_LONG(1), ARGUMENT_REAL(2)); return 0; }
static int32 hs_sound_looping_stop_procedure(const int32* arguments) { hs_sound_looping_stop(ARGUMENT_LONG(0)); return 0; }
static int32 hs_sound_looping_set_scale_procedure(const int32* arguments) { hs_sound_looping_set_scale(ARGUMENT_LONG(0), ARGUMENT_REAL(1)); return 0; }

// functions that do nothing in halo 1's release builds, or nothing visible here, and are done
static int32 hs_nothing(const int32* arguments) { return 0; }

struct s_hs_procedure_binding
{
	const char* name;
	hs_function_procedure procedure;
};

static const s_hs_procedure_binding k_hs_procedures[] =
{
	{ "not", hs_not },
	{ "print", hs_print },
	{ "players", hs_players_procedure },
	{ "volume_teleport_players_not_inside", hs_volume_teleport_players_not_inside },
	{ "volume_test_object", hs_volume_test_object },
	{ "volume_test_objects", hs_volume_test_objects },
	{ "volume_test_objects_all", hs_volume_test_objects_all },
	{ "object_teleport", hs_object_teleport },
	{ "object_set_facing", hs_object_set_facing },
	{ "object_destroy", hs_object_destroy },
	{ "list_get", hs_list_get },
	{ "list_count", hs_list_count },
	{ "random_range", hs_random_range },
	{ "real_random_range", hs_real_random_range },
	{ "unit_get_health", hs_unit_get_health },
	{ "unit_get_shield", hs_unit_get_shield },
	{ "unit_kill", hs_unit_kill },
	{ "unit_kill_silent", hs_unit_kill_silent },
	{ "device_group_get", hs_device_group_get },
	{ "device_group_set", hs_device_group_set },
	{ "device_group_set_immediate", hs_device_group_set },
	{ "game_time", hs_game_time },
	{ "game_difficulty_get", hs_game_difficulty_get },
	{ "game_difficulty_get_real", hs_game_difficulty_get },
	{ "game_is_cooperative", hs_game_is_cooperative },
	{ "structure_bsp_index", hs_structure_bsp_index },
	{ "switch_bsp", hs_switch_bsp },
	{ "fade_in", hs_fade_in },
	{ "fade_out", hs_fade_out },
	{ "game_won", hs_game_won },
	{ "game_lost", hs_game_lost },
	{ "game_revert", hs_game_revert },
	{ "game_save", hs_game_save },
	{ "game_save_totally_unsafe", hs_game_save },
	{ "game_save_cancel", hs_game_save_cancel },
	{ "game_save_no_timeout", hs_game_save_no_timeout },
	{ "game_saving", hs_game_saving },
	{ "game_reverted", hs_game_reverted },
	{ "game_safe_to_save", hs_game_safe_to_save },
	{ "game_safe_to_speak", hs_game_safe_to_speak },
	{ "game_all_quiet", hs_game_all_quiet },
	{ "cinematic_start", hs_cinematic_start },
	{ "cinematic_stop", hs_cinematic_stop },
	{ "cinematic_skip_start_internal", hs_cinematic_skip_start_internal },
	{ "cinematic_skip_stop_internal", hs_cinematic_skip_stop_internal },
	{ "cinematic_show_letterbox", hs_cinematic_show_letterbox },
	{ "camera_control", hs_camera_control },
	{ "camera_set", hs_camera_set },
	{ "camera_set_relative", hs_camera_set_relative },
	{ "camera_set_first_person", hs_camera_set_first_person },
	{ "camera_time", hs_camera_time },
	{ "players_unzoom_all", hs_players_unzoom_all },
	{ "player_enable_input", hs_player_enable_input },
	{ "player_camera_control", hs_player_camera_control },
	{ "show_hud", hs_show_hud },
	{ "show_hud_help_text", hs_show_hud_help_text },
	{ "player_action_test_reset", hs_player_action_test_reset },
	{ "player_action_test_jump", hs_player_action_test_jump },
	{ "player_action_test_primary_trigger", hs_player_action_test_primary_trigger },
	{ "player_action_test_grenade_trigger", hs_player_action_test_grenade_trigger },
	{ "player_action_test_zoom", hs_player_action_test_zoom },
	{ "player_action_test_action", hs_player_action_test_action },
	{ "player_action_test_accept", hs_player_action_test_accept },
	{ "player_action_test_back", hs_player_action_test_back },
	{ "player_action_test_look_relative_up", hs_player_action_test_look_relative_up },
	{ "player_action_test_look_relative_down", hs_player_action_test_look_relative_down },
	{ "player_action_test_look_relative_left", hs_player_action_test_look_relative_left },
	{ "player_action_test_look_relative_right", hs_player_action_test_look_relative_right },
	{ "player_action_test_look_relative_all_directions", hs_player_action_test_look_relative_all_directions },
	{ "player_action_test_move_relative_all_directions", hs_player_action_test_move_relative_all_directions },
	{ "player_effect_set_max_translation", hs_player_effect_set_max_translation },
	{ "player_effect_set_max_rotation", hs_player_effect_set_max_rotation },
	{ "player_effect_start", hs_player_effect_start },
	{ "player_effect_stop", hs_player_effect_stop },
	{ "sound_impulse_start", hs_sound_impulse_start_procedure },
	{ "sound_impulse_time", hs_sound_impulse_time_procedure },
	{ "sound_impulse_stop", hs_sound_impulse_stop_procedure },
	{ "sound_looping_start", hs_sound_looping_start_procedure },
	{ "sound_looping_stop", hs_sound_looping_stop_procedure },
	{ "sound_looping_set_scale", hs_sound_looping_set_scale_procedure },
	{ "sound_looping_predict", hs_nothing },
	{ "sound_impulse_predict", hs_nothing },
	{ "object_type_predict", hs_nothing },
	{ "objects_predict", hs_nothing },
	{ "garbage_collect_now", hs_nothing },
	{ "texture_cache_flush", hs_nothing },
	{ "sound_cache_flush", hs_nothing },
	{ "cls", hs_nothing },
};

/* ---------- public code */

void hs_functions_initialize(void)
{
	for (int16 function_index = 0; function_index < hs_function_count(); function_index++)
	{
		hs_function_get(function_index)->procedure = NULL;
	}
	for (const s_hs_procedure_binding& binding : k_hs_procedures)
	{
		const int16 function_index = hs_find_function_by_name(binding.name);
		if (function_index == NONE)
		{
			h1_log("hs: the library's %s isn't in the function table", binding.name);
			continue;
		}
		hs_function_get(function_index)->procedure = binding.procedure;
	}
	return;
}

void hs_functions_initialize_for_new_map(void)
{
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();
	for (s_object_list& list : g_hs_library.object_lists)
	{
		list.used = false;
		list.objects.clear();
	}
	g_hs_library.named_objects.assign(scenario->object_names.count, NONE);
	g_hs_library.device_group_values.assign(scenario->device_groups.count, 0.f);
	for (int32 i = 0; i < scenario->device_groups.count; i++)
	{
		g_hs_library.device_group_values[i] = g_h1_cache_file->block_get(scenario->device_groups, i)->initial_value;
	}
	g_hs_library.looping_sounds.clear();
	g_hs_library.scripted_sound_end_times.clear();
	h1_camera_reset();
	return;
}

void hs_functions_dispose_from_old_map(void)
{
	for (const s_scripted_looping_sound& sound : g_hs_library.looping_sounds)
	{
		h1_sound_looping_attached_delete(sound.handle);
	}
	g_hs_library.looping_sounds.clear();
	g_hs_library.scripted_sound_end_times.clear();
	g_hs_library.named_objects.clear();
	g_hs_library.device_group_values.clear();
	h1_camera_reset();
	return;
}

void hs_functions_update(void)
{
	hs_sound_looping_update();
	return;
}

} // namespace h1_hs

void h1_hs_object_name_set(int16 name_index, datum object_index)
{
	if (VALID_INDEX(name_index, (int16)h1_hs::g_hs_library.named_objects.size()))
	{
		h1_hs::g_hs_library.named_objects[name_index] = object_index;
	}
	return;
}

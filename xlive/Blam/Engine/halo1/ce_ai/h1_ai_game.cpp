#include "stdafx.h"
#include "h1_ai.h"

#include "../h1_cache_file.h"
#include "../h1_hs_internal.h"
#include "../h1_log.h"
#include "../h1_map_loader.h"

#include "game/game.h"
#include "game/game_options.h"
#include "game/players.h"
#include "memory/data.h"
#include "objects/object_types.h"
#include "objects/objects.h"
#include "units/units.h"

#include "h1_ai_internal.h"
#include "h1_ai_objects.h"

/*
* game.c's calls into halo 1's AI, and the game pieces of halo 1's engine the AI reads: its time (halo 1 ticks), difficulty,
* players (a mirror of carto's), the globals tag, script object lists (the scripts') and the inventory of units it creates.
*/

namespace h1_ai
{

/* ---------- globals */

data_array* player_data = NULL;
static bool g_ai_running = false;
static bool g_ai_initialized = false;
// actors_update and the rest of ai_update: on once the engine functions they call are implemented over carto
static bool g_ai_update_enabled = true;
static int32 g_last_ai_time = NONE;

/* ---------- game.c */

long game_time_get(void)
{
	return h1_ai_game_time();
}

short game_difficulty_level_get(void)
{
	const s_game_options* options = ::game_options_get();
	return options ? (short)PIN(options->difficulty, 0, NUMBER_OF_GAME_DIFFICULTY_LEVELS - 1) : (short)_game_difficulty_level_normal;
}

short game_connection(void)
{
	return 0;
}

boolean player_input_enabled(void)
{
	return TRUE;
}

// game_globals.c: halo 1's globals tag
struct game_globals* scenario_get_game_globals(void)
{
	static datum s_globals_index = NONE;
	static const c_h1_cache_file* s_cache_file = NULL;
	if (s_cache_file != g_h1_cache_file)
	{
		s_cache_file = g_h1_cache_file;
		s_globals_index = g_h1_cache_file ? g_h1_cache_file->tag_find('matg', "globals\\globals") : NONE;
	}
	return s_globals_index != NONE ? (struct game_globals*)tag_get('matg', s_globals_index) : NULL;
}

tag tag_get_group_tag(datum tag_index)
{
	const h1_cache_file_tag_instance* instance = g_h1_cache_file && tag_index != NONE ? g_h1_cache_file->tag_instance_get(tag_index) : NULL;
	return instance ? instance->group_tag : (tag)NONE;
}

c_void_pointer tag_block_address(const tag_block* block)
{
	return { block && block->count > 0 && g_h1_cache_file ? g_h1_cache_file->address_get(block->address, 1) : NULL };
}

c_void_pointer tag_data_address(const tag_data* data)
{
	return { data && data->size > 0 && g_h1_cache_file ? g_h1_cache_file->address_get(data->address, 1) : NULL };
}

/* ---------- players: halo 1's player datums of carto's players */

static void players_mirror_update(void)
{
	if (!player_data)
	{
		player_data = game_state_data_new("players", 16, sizeof(player_datum));
	}
	data_make_valid(player_data);
	::data_iterator iterator;
	::iterator_new(&iterator, ::player_data_get());
	while (const ::player_datum* h2_player = (const ::player_datum*)::iterator_next(&iterator))
	{
		const int16 absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.index);
		if (absolute_index >= player_data->maximum_count)
		{
			continue;
		}
		player_datum* player = (player_datum*)((uint8*)player_data->data + absolute_index * player_data->size);
		player->identifier = DATUM_INDEX_TO_IDENTIFIER(iterator.index);
		player->local_player_index = 0;
		player->team_index = _game_team_player;
		player->unit_index = h2_player->unit_index;
		player->dead_unit_index = NONE;
		const object_datum* unit = player->unit_index != NONE ? (const object_datum*)object_try_and_get_and_verify_type(player->unit_index, _object_mask_unit) : NULL;
		player->cluster_index = unit ? unit->object.location.cluster_index : (short)NONE;
		player->aim_assist_unit_index = NONE;
		player->aim_assist_timestamp = NONE;
		player_data->actual_count++;
		if (absolute_index + 1 > player_data->count)
		{
			player_data->count = absolute_index + 1;
		}
	}
	return;
}

const uint32* players_get_combined_pvs(void)
{
	// every cluster: the players see everything until halo 1's potentially visible sets are read
	static uint32 s_all_clusters[BIT_VECTOR_SIZE_IN_LONGS(MAXIMUM_CLUSTERS_PER_STRUCTURE)];
	csmemset(s_all_clusters, 0xFF, sizeof(s_all_clusters));
	return s_all_clusters;
}

/* ---------- script object lists: the scripts' */

long object_list_new(void)
{
	return h1_hs::object_list_new();
}

void object_list_add(long object_list_index, long object_index)
{
	h1_hs::object_list_add(object_list_index, object_index);
	return;
}

long object_list_get_first(long object_list_index, long* reference_index)
{
	*reference_index = 0;
	return object_list_get_next(object_list_index, reference_index);
}

long object_list_get_next(long object_list_index, long* reference_index)
{
	if (*reference_index >= h1_hs::object_list_count(object_list_index))
	{
		return NONE;
	}
	return (h1_hs::object_list_get)(object_list_index, (int16)(*reference_index)++);
}

/* ---------- units the AI creates: their inventory */

boolean unit_add_weapon_to_inventory(long unit_index, long weapon_index, long is_starting_weapon)
{
	return ::unit_add_weapon_to_inventory(unit_index, weapon_index, (::e_weapon_addition_method)(is_starting_weapon ? 1 : 0));
}

short unit_add_grenade_type_to_inventory(long unit_index, short grenade_type, short grenade_count)
{
	::unit_datum* unit = (::unit_datum*)::object_try_and_get_and_verify_type(unit_index, FLAG(::_object_type_biped) | FLAG(::_object_type_vehicle));
	if (!unit || !VALID_INDEX(grenade_type, NUMBER_OF_UNIT_GRENADE_TYPES))
	{
		return 0;
	}
	const int32 count = MIN(unit->unit.grenade_counts[grenade_type] + grenade_count, 127);
	const short added = (short)(count - unit->unit.grenade_counts[grenade_type]);
	unit->unit.grenade_counts[grenade_type] = (int8)count;
	return added;
}

boolean unit_add_equipment_to_inventory(long unit_index, long equipment_index, short replace)
{
	return FALSE;
}

// color_math.c rgb_colors_interpolate: from the lower bound to the upper (flags bit 0: through hue, saturation and value)
union real_rgb_color* rgb_colors_interpolate(union real_rgb_color* rgb_result, unsigned long flags, union real_rgb_color const* rgb_lower_bound,
	union real_rgb_color const* rgb_upper_bound, real u)
{
	rgb_result->red = rgb_lower_bound->red + (rgb_upper_bound->red - rgb_lower_bound->red) * u;
	rgb_result->green = rgb_lower_bound->green + (rgb_upper_bound->green - rgb_lower_bound->green) * u;
	rgb_result->blue = rgb_lower_bound->blue + (rgb_upper_bound->blue - rgb_lower_bound->blue) * u;
	return rgb_result;
}

/* ---------- the AI's lifecycle */

static void ai_tick(void)
{
	h1_ai_objects_update();
	players_mirror_update();
	if (g_ai_update_enabled)
	{
		ai_update();
		h1_ai_units_control_update();
	}
	return;
}

} // namespace h1_ai

void h1_ai_initialize_for_new_map(void)
{
	if (!h1_ai::g_ai_initialized)
	{
		h1_ai::g_ai_initialized = true;
		h1_ai::real_math_initialize();
		h1_ai::ai_initialize();
		h1_ai::game_allegiance_initialize();
	}
	h1_ai::h1_ai_objects_initialize_for_new_map();
	h1_ai::h1_ai_objects_update();
	h1_ai::players_mirror_update();
	h1_ai::game_allegiance_initialize_for_new_map();
	h1_ai::ai_initialize_for_new_map();
	h1_ai::g_ai_running = true;
	h1_ai::g_last_ai_time = h1_ai::h1_ai_game_time();
	h1_log("ai: initialized for %d encounters", h1_ai::global_scenario_get()->ai_encounters.count);
	return;
}

void h1_ai_dispose_from_old_map(void)
{
	if (!h1_ai::g_ai_running)
	{
		return;
	}
	h1_ai::ai_dispose_from_old_map();
	h1_ai::game_allegiance_dispose_from_old_map();
	h1_ai::g_ai_running = false;
	return;
}

void h1_ai_place(void)
{
	if (!h1_ai::g_ai_running)
	{
		return;
	}
	h1_ai::h1_ai_objects_update();
	h1_ai::ai_place();
	int32 actor_count = 0;
	h1_ai::data_iterator iterator;
	h1_ai::data_iterator_new(&iterator, h1_ai::actor_data);
	while (h1_ai::data_iterator_next(&iterator))
	{
		actor_count++;
	}
	h1_log("ai: placed %d actors", actor_count);
	return;
}

void h1_ai_update(void)
{
	if (!h1_ai::g_ai_running)
	{
		return;
	}
	// halo 1's ticks in halo 2's (30 a second)
	const int32 time = h1_ai::h1_ai_game_time();
	if (time == h1_ai::g_last_ai_time)
	{
		return;
	}
	h1_ai::g_last_ai_time = time;
	h1_ai::ai_tick();
	return;
}

// units.c unit_delete's ai_handle_deleted_object: the unit part's delete of halo 2's units
static object_delete_t g_h2_unit_delete = NULL;

static void h1_ai_unit_delete_hook(datum object_index)
{
	if (h1_ai::g_ai_running && h1_maps_active())
	{
		h1_ai::ai_handle_deleted_object(object_index);
		h1_ai::h1_ai_object_mirror_forget(object_index);
	}
	g_h2_unit_delete(object_index);
	return;
}

void h1_ai_apply_patches(void)
{
	object_type_definition* biped_type = object_type_definition_get(_object_type_biped);
	for (int32 i = 0; i < k_max_object_type_inheritence; i++)
	{
		object_type_definition* part = biped_type->part_definitions[i];
		if (part && part->group_tag == 'unit' && part->object_delete)
		{
			g_h2_unit_delete = part->object_delete;
			part->object_delete = h1_ai_unit_delete_hook;
			break;
		}
	}
	h1_log("ai: unit delete %s", g_h2_unit_delete ? "hooked" : "not hooked");
	return;
}

// units.c unit_damage_aftermath: bipeds' deaths and damage to the AI (damage categories aren't halo 2's: none)
void h1_ai_object_damaged(datum object_index, datum owner_object_index, const real_vector3d* direction, real32 body_vitality_before,
	real32 shield_vitality_before)
{
	const ::unit_datum* unit = h1_ai::g_ai_running ? (const ::unit_datum*)::object_try_and_get_and_verify_type(object_index, FLAG(::_object_type_biped)) : NULL;
	if (!unit || !h1_ai::ai_globals->ai_initialized_for_map)
	{
		return;
	}
	const bool dead = unit->object.object_damage_flags.test(::_object_is_dead_bit);
	const real32 total_damage = MAX(body_vitality_before - unit->object.body_vitality, 0.f) + MAX(shield_vitality_before - unit->object.shield_vitality, 0.f);
	// the mirror sees the damage now
	h1_ai::h1_ai_objects_update();
	if (dead)
	{
		if (body_vitality_before > 0.f)
		{
			h1_ai::ai_handle_death(object_index, owner_object_index, 0);
		}
	}
	else if (total_damage > 0.f)
	{
		h1_ai::real_vector3d velocity = *(const h1_ai::real_vector3d*)direction;
		h1_ai::ai_handle_damage(object_index, owner_object_index, 0, total_damage, &velocity, FALSE);
	}
	return;
}

bool h1_ai_running(void)
{
	return h1_ai::g_ai_running;
}

void h1_ai_script_place(int32 ai_index)
{
	if (h1_ai::g_ai_running)
	{
		h1_ai::ai_scripting_place(ai_index);
	}
	return;
}

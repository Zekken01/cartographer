#include "stdafx.h"
#include "h1_first_person_weapon.h"

#include "h1_cache_file.h"
#include "h1_log.h"
#include "h1_map_loader.h"
#include "h1_sound.h"
#include "h1_weapon_logic.h"
#include "h1_weapons.h"

#include "game/game_time.h"
#include "game/players.h"
#include "interface/first_person_weapons.h"
#include "items/weapons.h"
#include "math/matrix_math.h"
#include "math/real_math.h"
#include "objects/objects.h"
#include "render/render.h"
#include "tag_files/tag_groups.h"
#include "units/units.h"

/* constants */

enum
{
	k_h1_ticks_per_second = 30,
	k_h1_maximum_first_person_nodes = 64,
	k_h1_first_person_overlay_frames = 9,
	k_h1_first_person_messages_per_tick = 16,
};

#define k_h1_first_person_firing_push_back_velocity 0.05f
#define k_h1_pi 3.14159265f

// first_person_weapons.c
enum : int16
{
	_h1_fp_state_idle = 0,
	_h1_fp_state_overheating,
	_h1_fp_state_overheating_again,
	_h1_fp_state_overheated,
	_h1_fp_state_charged,
	_h1_fp_state_posing,
	_h1_fp_state_primary_fire,
	_h1_fp_state_secondary_fire,
	_h1_fp_state_primary_misfire,
	_h1_fp_state_secondary_misfire,
	_h1_fp_state_melee,
	_h1_fp_state_light_on,
	_h1_fp_state_light_off,
	_h1_fp_state_reload_while_empty,
	_h1_fp_state_reload_while_full,
	_h1_fp_state_shotgun_enter_reload,
	_h1_fp_state_shotgun_exit_reload_empty,
	_h1_fp_state_shotgun_exit_reload_full,
	_h1_fp_state_put_away,
	_h1_fp_state_ready,
	_h1_fp_state_throw_grenade,
	_h1_fp_state_throw_grenade_overheated,
	_h1_fp_state_overheated_exit,
	_h1_fp_state_overheating_super_recoil,
};

// weapons.h first person animations
enum : int16
{
	_h1_fp_animation_idle = 0,
	_h1_fp_animation_posing,
	_h1_fp_animation_primary_fire,
	_h1_fp_animation_moving,
	_h1_fp_animation_overlays,
	_h1_fp_animation_light_on,
	_h1_fp_animation_light_off,
	_h1_fp_animation_reload_while_empty,
	_h1_fp_animation_reload_while_full,
	_h1_fp_animation_overheated,
	_h1_fp_animation_ready,
	_h1_fp_animation_put_away,
	_h1_fp_animation_overcharged,
	_h1_fp_animation_melee,
	_h1_fp_animation_secondary_fire,
	_h1_fp_animation_overcharged_jitter,
	_h1_fp_animation_throw_grenade,
	_h1_fp_animation_ammunition,
	_h1_fp_animation_primary_misfire,
	_h1_fp_animation_secondary_misfire,
	_h1_fp_animation_throw_grenade_overheated,
	_h1_fp_animation_overheating,
	_h1_fp_animation_overheating_again,
	_h1_fp_animation_shotgun_enter,
	_h1_fp_animation_shotgun_exit_empty,
	_h1_fp_animation_shotgun_exit_full,
	_h1_fp_animation_overheated_exit,
	_h1_fp_animation_overheated_supercharge_enter,
};

enum : int16
{
	_h1_animation_base = 0,
	_h1_animation_overlay,
	_h1_animation_replacement,
};

enum : int16
{
	_h1_animation_running = 0,
	_h1_animation_key_frame,
	_h1_animation_will_restart_on_next_frame,
	_h1_animation_restarted,
	_h1_animation_looped,
};

enum : int16
{
	_h1_weapon_type_shotgun = 1,
	_h1_weapon_type_needler = 2,
	_h1_weapon_type_plasma_pistol = 3,
};

enum : int16
{
	_h1_shotgun_reload_type_first_round = 0,
	_h1_shotgun_reload_type_last_round,
	_h1_shotgun_reload_type_first_and_last_round,
};

/* structures */

struct s_h1_fp_animation_state
{
	int16 index;
	int16 frame_index;
};

// first_person_weapons.c first_person_weapon (one local player)
struct s_h1_first_person_weapon
{
	bool visible;
	datum unit_index;
	datum weapon_index;
	datum graph_index;		// halo 1 first person animations
	datum model_index;		// halo 1 first person model
	datum hands_index;		// halo 1 hands
	int16 state;
	int16 ticks_until_pose;
	int16 ticks_idle;
	s_h1_fp_animation_state state_animation;
	s_h1_fp_animation_state moving_animation;
	int16 jitter_animation_index;
	real32 jitter_frame_index;
	real32 firing_push_back;
	real32 firing_push_back_velocity;
	real_vector2d position;
	real_vector2d position_velocity;
	real_vector2d turning;
	real_vector2d turning_velocity;
	bool rendered;
	real_euler_angles2d render_facing;
	real_euler_angles2d last_render_facing;
	uint32 last_render_tick;
	int16 interpolation_frame_index;
	int16 interpolation_frame_count;
	real_orientation node_orientations[k_h1_maximum_first_person_nodes];
	real_orientation original_node_orientations[k_h1_maximum_first_person_nodes];
	real_matrix4x3 node_matrices[k_h1_maximum_first_person_nodes];
	real_orientation pose[k_h1_maximum_first_person_nodes];			// the last tick's pose
	real_orientation previous_pose[k_h1_maximum_first_person_nodes];	// the tick before's
	bool pose_valid;
	LARGE_INTEGER pose_time;
	int16 weapon_node_remapping_table[k_h1_maximum_first_person_nodes];
	int16 hands_node_remapping_table[k_h1_maximum_first_person_nodes];
	bool shotgun_empty;
	int16 shotgun_shells_to_reload;
	int16 shotgun_reload_type;
	string_id last_h2_action;
	uint32 random_seed;
};

/* globals */

static s_h1_first_person_weapon g_h1_fp = {};

/* prototypes */

static const h1_antr* h1_fp_graph(void);
static const h1_weap* h1_fp_weapon_definition(datum weapon_index);
static int16 h1_fp_animation_index(const h1_antr* graph, int16 animation_type);
static datum h1_fp_unit_current_weapon(datum unit_index);
static void h1_fp_forget_weapon(void);
static void h1_fp_switch_weapons(void);
static void h1_fp_set_state(int16 new_state);
static void h1_fp_next_state(void);
static void h1_fp_message(int16 message_type);
static void h1_fp_tick(void);
static void h1_fp_start_interpolation(int16 frame_count);
static void h1_fp_compute_pose(void);
static void h1_fp_build_node_matrices(void);
static bool h1_fp_build_remapping_table(datum h1_model_index, const h1_antr* graph, int16* table);
static int16 h1_fp_state_from_message(int16 message_type);
static int16 h1_fp_animation_type_from_state(int16 state);
static int16 h1_fp_animation_update(const h1_antr* graph, s_h1_fp_animation_state* state, datum* sound_index);
static real32 h1_fp_random_real(void);

static void h1_animation_get_node_orientations(const h1_antr_animations* animation, int16 frame_index, real_orientation* orientations);
static void h1_overlay_animation_apply(const h1_antr_animations* animation, int16 frame_index, real32 scale, real_orientation* orientations);
static void h1_overlay_animation_apply_continuous_scaled(const h1_antr_animations* animation, real32 frame_index, real32 scale, real_orientation* orientations);
static void h1_quaternions_multiply(const real_quaternion* q0, const real_quaternion* q1, real_quaternion* result);
static void h1_quaternions_interpolate(const real_quaternion* q0, const real_quaternion* q1, real32 t, real_quaternion* result);
static void h1_quaternion_normalize(real_quaternion* q);
static void h1_matrix4x3_from_orientation(real_matrix4x3* matrix, const real_orientation* orientation);
static bool h1_accelerate_to_position(real32* position, real32* velocity, real32 target_position, real32 maximum_velocity, real32 acceleration,
	real32 minimum_position, real32 maximum_position);

/* public code */

void h1_first_person_weapon_reset(void)
{
	// first_person_weapons_initialize_for_new_map
	g_h1_fp = {};
	g_h1_fp.unit_index = NONE;
	g_h1_fp.weapon_index = NONE;
	g_h1_fp.graph_index = NONE;
	g_h1_fp.model_index = NONE;
	g_h1_fp.hands_index = NONE;
	g_h1_fp.state = NONE;
	g_h1_fp.state_animation.index = NONE;
	g_h1_fp.moving_animation.index = NONE;
	g_h1_fp.jitter_animation_index = NONE;
	g_h1_fp.random_seed = 0x2468ACE1u;
	return;
}

void h1_first_person_weapon_tick(void)
{
	if (!h1_maps_active() || !g_h1_cache_file)
	{
		return;
	}
	const datum player_index = player_index_from_user_index(0);
	const player_datum* player = player_index != NONE ? player_get(player_index) : NULL;
	const datum unit_index = player ? player->unit_index : NONE;
	if (unit_index == NONE)
	{
		return;
	}
	if (g_h1_fp.unit_index != unit_index)
	{
		// first_person_weapon_new_unit
		g_h1_fp.rendered = false;
		g_h1_fp.unit_index = unit_index;
		h1_fp_switch_weapons();
	}

	// first_person_weapon_message_from_weapon: the messages of the weapon in the unit's hands
	const datum current_weapon = h1_fp_unit_current_weapon(unit_index);
	if (current_weapon != NONE)
	{
		e_h1_first_person_weapon_message messages[k_h1_first_person_messages_per_tick];
		const int32 count = h1_weapon_logic_first_person_messages_take(current_weapon, messages, k_h1_first_person_messages_per_tick);
		for (int32 i = 0; i < count; i++)
		{
			h1_fp_message(messages[i]);
		}
	}
	// first_person_weapon_message_from_unit: halo 2's unit throws grenades and lowers the weapon, its first person action says so
	const string_id action = first_person_weapon_action_get(0);
	if (action != g_h1_fp.last_h2_action)
	{
		// (its melee comes from h1_weapon_logic, for every strike)
		if (action == string_id_find_or_add("throw_grenade"))
		{
			h1_fp_message(_h1_first_person_weapon_message_throw_grenade);
		}
		else if (action == string_id_find_or_add("put_away"))
		{
			// weapons.c weapon_put_away: halo 2's unit lowers the weapon before the swap
			h1_fp_message(_h1_first_person_weapon_message_put_away);
		}
		g_h1_fp.last_h2_action = action;
	}

	if (g_h1_fp.weapon_index == NONE || g_h1_fp.weapon_index != current_weapon)
	{
		h1_fp_switch_weapons();
	}
	h1_fp_tick();
	if (g_h1_fp.weapon_index != NONE)
	{
		h1_fp_compute_pose();
	}
	return;
}

bool h1_first_person_weapon_meleeing(datum unit_index)
{
	return unit_index != NONE && g_h1_fp.unit_index == unit_index && g_h1_fp.state == _h1_fp_state_melee;
}

datum h1_first_person_weapon_unit_get(void)
{
	const datum player_index = player_index_from_user_index(0);
	const player_datum* player = player_index != NONE ? player_get(player_index) : NULL;
	return player ? player->unit_index : NONE;
}

bool h1_first_person_weapon_model_nodes_get(datum h1_model_index, datum weapon_object_index, real_matrix4x3* node_matrices, int32 node_count)
{
	if (g_h1_fp.weapon_index == NONE || g_h1_fp.state_animation.index == NONE || !h1_fp_graph())
	{
		return false;
	}
	const int16* table;
	if (h1_model_index == g_h1_fp.hands_index)
	{
		table = g_h1_fp.hands_node_remapping_table;
	}
	else if (h1_model_index == g_h1_fp.model_index && weapon_object_index == g_h1_fp.weapon_index)
	{
		table = g_h1_fp.weapon_node_remapping_table;
	}
	else
	{
		return false;
	}

	// first_person_weapon_render_update: the matrices at this frame's camera
	h1_fp_build_node_matrices();
	const h1_antr* graph = h1_fp_graph();
	for (int32 i = 0; i < node_count && i < k_h1_maximum_first_person_nodes; i++)
	{
		const int16 graph_node = table[i];
		node_matrices[i] = g_h1_fp.node_matrices[VALID_INDEX(graph_node, MIN(graph->nodes.count, (int32)k_h1_maximum_first_person_nodes)) ? graph_node : 0];
	}
	return true;
}

/* private code */

static const h1_antr* h1_fp_graph(void)
{
	return g_h1_fp.graph_index != NONE ? (const h1_antr*)g_h1_cache_file->tag_get('antr', g_h1_fp.graph_index) : NULL;
}

static const h1_weap* h1_fp_weapon_definition(datum weapon_index)
{
	const weapon_datum* weapon = (const weapon_datum*)object_try_and_get_and_verify_type(weapon_index, _object_mask_weapon);
	const datum h1_weapon_index = weapon ? h1_weapon_h1_get(weapon->definition_index) : NONE;
	return h1_weapon_index != NONE ? (const h1_weap*)g_h1_cache_file->tag_get('weap', h1_weapon_index) : NULL;
}

// the animation of a first person animation type, NONE when the graph has none
static int16 h1_fp_animation_index(const h1_antr* graph, int16 animation_type)
{
	if (!graph || graph->first_person_weapons.count <= 0)
	{
		return NONE;
	}
	const h1_antr_first_person_weapons* weapon_animations = g_h1_cache_file->block_get(graph->first_person_weapons, 0);
	if (animation_type < 0 || animation_type >= weapon_animations->animations.count)
	{
		return NONE;
	}
	const int16 animation_index = g_h1_cache_file->block_get(weapon_animations->animations, animation_type)->animation_index;
	return VALID_INDEX(animation_index, graph->animations.count) ? animation_index : (int16)NONE;
}

static datum h1_fp_unit_current_weapon(datum unit_index)
{
	const unit_datum* unit = (const unit_datum*)object_try_and_get_and_verify_type(unit_index, _object_mask_unit);
	if (!unit || unit->unit.weapon_indices[0] == NONE)
	{
		return NONE;
	}
	return unit_inventory_get_weapon(unit_index, unit->unit.weapon_indices[0]);
}

static void h1_fp_forget_weapon(void)
{
	g_h1_fp.visible = false;
	g_h1_fp.weapon_index = NONE;
	return;
}

// first_person_weapons.c first_person_weapon_switch_weapons
static void h1_fp_switch_weapons(void)
{
	g_h1_fp.weapon_index = NONE;
	g_h1_fp.graph_index = NONE;
	const datum weapon_index = h1_fp_unit_current_weapon(g_h1_fp.unit_index);
	const h1_weap* definition = weapon_index != NONE ? h1_fp_weapon_definition(weapon_index) : NULL;
	if (!definition || definition->first_person_model.index == NONE || definition->first_person_animations.index == NONE)
	{
		return;
	}
	const h1_antr* graph = (const h1_antr*)g_h1_cache_file->tag_get('antr', definition->first_person_animations.index);
	if (!graph || graph->first_person_weapons.count <= 0)
	{
		return;
	}

	const datum h1_globals_index = g_h1_cache_file->tag_find('matg', "globals\\globals");
	const h1_matg* globals = h1_globals_index != NONE ? (const h1_matg*)g_h1_cache_file->tag_get('matg', h1_globals_index) : NULL;
	const h1_matg_first_person_interface* first_person_interface = globals && globals->first_person_interface.count > 0 ?
		g_h1_cache_file->block_get(globals->first_person_interface, 0) : NULL;
	const datum hands_index = first_person_interface ? first_person_interface->first_person_hands.index : NONE;
	const bool hands_valid = hands_index != NONE && h1_fp_build_remapping_table(hands_index, graph, g_h1_fp.hands_node_remapping_table);
	const bool weapon_valid = h1_fp_build_remapping_table(definition->first_person_model.index, graph, g_h1_fp.weapon_node_remapping_table);
	if (!weapon_valid || !hands_valid)
	{
		h1_log("first person: %s's first person model or the hands miss nodes of its animations", definition->label);
		return;
	}

	g_h1_fp.weapon_index = weapon_index;
	g_h1_fp.pose_valid = false;
	g_h1_fp.graph_index = definition->first_person_animations.index;
	g_h1_fp.model_index = definition->first_person_model.index;
	g_h1_fp.hands_index = hands_index;
	g_h1_fp.state = NONE;
	g_h1_fp.state_animation.index = NONE;
	g_h1_fp.moving_animation.index = NONE;
	g_h1_fp.jitter_animation_index = NONE;
	g_h1_fp.firing_push_back = 0.f;
	g_h1_fp.firing_push_back_velocity = 0.f;
	g_h1_fp.ticks_idle = 0;
	h1_fp_set_state(_h1_fp_state_idle);
	g_h1_fp.interpolation_frame_count = 0;
	g_h1_fp.visible = true;
	return;
}

// first_person_weapons.c first_person_weapon_start_interpolation
static void h1_fp_start_interpolation(int16 frame_count)
{
	const h1_antr* graph = h1_fp_graph();
	if (!graph)
	{
		return;
	}
	csmemcpy(g_h1_fp.original_node_orientations, g_h1_fp.node_orientations,
		sizeof(real_orientation) * MIN(graph->nodes.count, (int32)k_h1_maximum_first_person_nodes));
	if (frame_count >= g_h1_fp.interpolation_frame_count - g_h1_fp.interpolation_frame_index)
	{
		g_h1_fp.interpolation_frame_index = 0;
		g_h1_fp.interpolation_frame_count = frame_count;
	}
	return;
}

// first_person_weapons.c first_person_weapon_set_state
static void h1_fp_set_state(int16 new_state)
{
	const weapon_datum* weapon = g_h1_fp.weapon_index != NONE ?
		(const weapon_datum*)object_try_and_get_and_verify_type(g_h1_fp.weapon_index, _object_mask_weapon) : NULL;
	const h1_weap* definition = weapon ? h1_fp_weapon_definition(g_h1_fp.weapon_index) : NULL;
	const bool overheated = weapon && weapon->weapon.heat > 0.f && definition && weapon->weapon.heat >= definition->overheated_threshold;

	if (overheated)
	{
		if (new_state == _h1_fp_state_ready)
		{
			new_state = _h1_fp_state_overheating_again;
		}
		else if (new_state == _h1_fp_state_throw_grenade)
		{
			new_state = _h1_fp_state_throw_grenade_overheated;
		}
	}

	switch (new_state)
	{
	case _h1_fp_state_ready:
		if (g_h1_fp.state == _h1_fp_state_ready)
		{
			new_state = NONE;
		}
		break;
	case _h1_fp_state_light_off:
	case _h1_fp_state_light_on:
		if (g_h1_fp.state != _h1_fp_state_idle && g_h1_fp.state != _h1_fp_state_posing)
		{
			new_state = NONE;
		}
		break;
	case _h1_fp_state_primary_fire:
	case _h1_fp_state_secondary_fire:
	case _h1_fp_state_primary_misfire:
	case _h1_fp_state_secondary_misfire:
		if (g_h1_fp.state != _h1_fp_state_idle && g_h1_fp.state != _h1_fp_state_posing && g_h1_fp.state != _h1_fp_state_primary_fire &&
			g_h1_fp.state != _h1_fp_state_charged && g_h1_fp.state != _h1_fp_state_shotgun_enter_reload &&
			g_h1_fp.state != _h1_fp_state_overheated_exit && g_h1_fp.state != _h1_fp_state_shotgun_exit_reload_empty &&
			g_h1_fp.state != _h1_fp_state_shotgun_exit_reload_full && g_h1_fp.state != _h1_fp_state_reload_while_empty &&
			g_h1_fp.state != _h1_fp_state_reload_while_full)
		{
			new_state = NONE;
		}
		break;
	}

	if (new_state == NONE || !definition)
	{
		return;
	}
	if (definition->weapon_type == _h1_weapon_type_plasma_pistol && new_state == _h1_fp_state_overheated && !overheated)
	{
		new_state = _h1_fp_state_idle;
	}
	const int16 animation_type = h1_fp_animation_type_from_state(new_state);
	int16 interpolation_frame_count;
	if (definition->weapon_type == _h1_weapon_type_shotgun && g_h1_fp.state == _h1_fp_state_shotgun_exit_reload_empty)
	{
		interpolation_frame_count = 0;
	}
	else
	{
		switch (new_state)
		{
		case _h1_fp_state_overheated:
		case _h1_fp_state_melee:
		case _h1_fp_state_ready:
			interpolation_frame_count = 0;
			break;
		case _h1_fp_state_primary_fire:
		case _h1_fp_state_secondary_fire:
		case _h1_fp_state_primary_misfire:
		case _h1_fp_state_secondary_misfire:
			interpolation_frame_count = 3;
			break;
		default:
			interpolation_frame_count = 6;
			break;
		}
	}

	const int16 animation_index = h1_fp_animation_index(h1_fp_graph(), animation_type);
	if (animation_index == NONE)
	{
		return;
	}
	if (interpolation_frame_count > 0)
	{
		h1_fp_start_interpolation(interpolation_frame_count);
	}
	g_h1_fp.state = new_state;
	g_h1_fp.state_animation.index = animation_index;
	g_h1_fp.state_animation.frame_index = 0;
	return;
}

// first_person_weapons.c first_person_weapon_next_state
static void h1_fp_next_state(void)
{
	const h1_weap* definition = h1_fp_weapon_definition(g_h1_fp.weapon_index);
	int16 new_state = NONE;
	switch (g_h1_fp.state)
	{
	case _h1_fp_state_idle:
	case _h1_fp_state_posing:
	case _h1_fp_state_primary_fire:
	case _h1_fp_state_secondary_fire:
	case _h1_fp_state_primary_misfire:
	case _h1_fp_state_secondary_misfire:
	case _h1_fp_state_melee:
	case _h1_fp_state_light_off:
	case _h1_fp_state_light_on:
	case _h1_fp_state_shotgun_exit_reload_empty:
	case _h1_fp_state_shotgun_exit_reload_full:
	case _h1_fp_state_ready:
	case _h1_fp_state_throw_grenade:
	case _h1_fp_state_overheated_exit:
		new_state = _h1_fp_state_idle;
		break;
	case _h1_fp_state_overheating:
	case _h1_fp_state_overheating_again:
	case _h1_fp_state_throw_grenade_overheated:
	case _h1_fp_state_overheating_super_recoil:
		new_state = _h1_fp_state_overheated;
		break;
	case _h1_fp_state_put_away:
		g_h1_fp.state_animation.frame_index--;
		break;
	case _h1_fp_state_shotgun_enter_reload:
		if (definition && definition->weapon_type == _h1_weapon_type_shotgun &&
			g_h1_fp.shotgun_reload_type == _h1_shotgun_reload_type_first_and_last_round)
		{
			new_state = g_h1_fp.shotgun_empty ? _h1_fp_state_shotgun_exit_reload_empty : _h1_fp_state_shotgun_exit_reload_full;
		}
		else
		{
			new_state = _h1_fp_state_idle;
		}
		break;
	case _h1_fp_state_reload_while_empty:
	case _h1_fp_state_reload_while_full:
		if (!definition || definition->weapon_type != _h1_weapon_type_shotgun || !g_h1_fp.shotgun_reload_type || g_h1_fp.shotgun_reload_type == NONE)
		{
			new_state = _h1_fp_state_idle;
		}
		else
		{
			new_state = g_h1_fp.shotgun_empty ? _h1_fp_state_shotgun_exit_reload_empty : _h1_fp_state_shotgun_exit_reload_full;
		}
		break;
	}
	if (new_state != NONE)
	{
		h1_fp_set_state(new_state);
	}
	return;
}

// first_person_weapons.c first_person_weapon_message
static void h1_fp_message(int16 message_type)
{
	int16 state = NONE;
	switch (message_type)
	{
	case _h1_first_person_weapon_message_drop:
		h1_fp_forget_weapon();
		break;
	case _h1_first_person_weapon_message_ready:
		h1_fp_switch_weapons();
		break;
	case _h1_first_person_weapon_message_primary_fire:
		g_h1_fp.firing_push_back_velocity += k_h1_first_person_firing_push_back_velocity;
		break;
	}

	const weapon_datum* weapon = g_h1_fp.weapon_index != NONE ?
		(const weapon_datum*)object_try_and_get_and_verify_type(g_h1_fp.weapon_index, _object_mask_weapon) : NULL;
	const h1_weap* definition = weapon ? h1_fp_weapon_definition(g_h1_fp.weapon_index) : NULL;
	if (definition && definition->weapon_type == _h1_weapon_type_shotgun && definition->magazines.count > 0 &&
		(message_type == _h1_first_person_weapon_message_reload_while_empty || message_type == _h1_first_person_weapon_message_reload_while_full))
	{
		const h1_weap_magazines* magazine_definition = g_h1_cache_file->block_get(definition->magazines, 0);
		const int16 rounds_loaded = weapon->weapon.magazines[0].rounds_loaded;
		const int16 rounds_total = weapon->weapon.magazines[0].rounds_inventory;
		if (g_h1_fp.state == _h1_fp_state_shotgun_enter_reload || g_h1_fp.state == _h1_fp_state_overheated_exit ||
			g_h1_fp.state == _h1_fp_state_shotgun_exit_reload_empty || g_h1_fp.state == _h1_fp_state_shotgun_exit_reload_full ||
			g_h1_fp.state == _h1_fp_state_reload_while_empty || g_h1_fp.state == _h1_fp_state_reload_while_full)
		{
			g_h1_fp.shotgun_reload_type = MIN(magazine_definition->rounds_loaded_maximum - rounds_loaded, (int32)rounds_total) == 1 ?
				(int16)_h1_shotgun_reload_type_last_round : (int16)NONE;
		}
		else
		{
			const int16 shells_to_reload = (int16)MIN(magazine_definition->rounds_loaded_maximum - rounds_loaded, (int32)rounds_total);
			g_h1_fp.shotgun_shells_to_reload = shells_to_reload;
			g_h1_fp.shotgun_empty = rounds_loaded == 0;
			g_h1_fp.shotgun_reload_type = (int16)(shells_to_reload != 1 ? _h1_shotgun_reload_type_first_round : _h1_shotgun_reload_type_first_and_last_round);
		}
		switch (g_h1_fp.shotgun_reload_type)
		{
		case _h1_shotgun_reload_type_first_round:
		case _h1_shotgun_reload_type_first_and_last_round:
			state = _h1_fp_state_shotgun_enter_reload;
			break;
		case NONE:
			state = _h1_fp_state_reload_while_empty;
			break;
		}
	}

	if (state == NONE)
	{
		state = h1_fp_state_from_message(message_type);
	}
	if (state != NONE)
	{
		h1_fp_set_state(state);
	}
	if (message_type == _h1_first_person_weapon_message_ready)
	{
		g_h1_fp.interpolation_frame_count = 0;
	}
	return;
}

// first_person_weapons.c first_person_weapon_update
static void h1_fp_tick(void)
{
	const unit_datum* unit = (const unit_datum*)object_try_and_get_and_verify_type(g_h1_fp.unit_index, _object_mask_unit);
	const weapon_datum* weapon = g_h1_fp.weapon_index != NONE ?
		(const weapon_datum*)object_try_and_get_and_verify_type(g_h1_fp.weapon_index, _object_mask_weapon) : NULL;
	if (g_h1_fp.weapon_index != NONE && !weapon)
	{
		h1_fp_forget_weapon();
	}
	const h1_weap* definition = weapon ? h1_fp_weapon_definition(g_h1_fp.weapon_index) : NULL;
	const h1_antr* graph = h1_fp_graph();
	if (!unit || !weapon || !definition || !graph || g_h1_fp.state_animation.index == NONE)
	{
		return;
	}

	const bool overheated = weapon->weapon.heat > 0.f && weapon->weapon.heat >= definition->overheated_threshold;
	if (g_h1_fp.state == _h1_fp_state_overheated || g_h1_fp.state == _h1_fp_state_overheating)
	{
		if (overheated && definition->heat_loss_per_second > 0.f &&
			(weapon->weapon.heat - definition->heat_recovery_threshold) / (definition->heat_loss_per_second / k_h1_ticks_per_second) <= 1.f)
		{
			h1_fp_set_state(_h1_fp_state_overheated_exit);
		}
		if (!overheated)
		{
			h1_fp_set_state(_h1_fp_state_idle);
		}
	}

	datum sound_index = NONE;
	const int16 result = h1_fp_animation_update(graph, &g_h1_fp.state_animation, &sound_index);
	if (result == _h1_animation_will_restart_on_next_frame)
	{
		h1_fp_next_state();
	}
	if (sound_index != NONE)
	{
		h1_sound_impulse(sound_index, &weapon->object.position, 1.f);
	}

	const real32 throttle = sqrtf(unit->unit.throttle.i * unit->unit.throttle.i + unit->unit.throttle.j * unit->unit.throttle.j +
		unit->unit.throttle.k * unit->unit.throttle.k);
	const bool moving = throttle > 0.1f;
	if (g_h1_fp.moving_animation.index != NONE)
	{
		h1_fp_animation_update(graph, &g_h1_fp.moving_animation, NULL);
		if (!moving)
		{
			if (g_h1_fp.state == _h1_fp_state_idle)
			{
				h1_fp_start_interpolation(6);
			}
			g_h1_fp.moving_animation.index = NONE;
		}
	}
	else if (moving)
	{
		g_h1_fp.moving_animation.frame_index = 0;
		g_h1_fp.moving_animation.index = h1_fp_animation_index(graph, _h1_fp_animation_moving);
	}

	if (g_h1_fp.jitter_animation_index == NONE)
	{
		if (g_h1_fp.state == _h1_fp_state_charged)
		{
			g_h1_fp.jitter_frame_index = 0.f;
			g_h1_fp.jitter_animation_index = h1_fp_animation_index(graph, _h1_fp_animation_overcharged_jitter);
		}
	}
	else if (g_h1_fp.state == _h1_fp_state_charged)
	{
		const h1_antr_animations* animation = g_h1_cache_file->block_get(graph->animations, g_h1_fp.jitter_animation_index);
		g_h1_fp.jitter_frame_index = fmodf((weapon->weapon.overcharged + 1.f) * 2.f + g_h1_fp.jitter_frame_index, (real32)MAX(animation->frame_count, 1));
	}
	else
	{
		g_h1_fp.jitter_animation_index = NONE;
	}

	if (g_h1_fp.rendered)
	{
		h1_accelerate_to_position(&g_h1_fp.position.i, &g_h1_fp.position_velocity.i, unit->unit.throttle.i, 0.08f, 0.5f, -1.f, 1.f);
		h1_accelerate_to_position(&g_h1_fp.position.j, &g_h1_fp.position_velocity.j, unit->unit.throttle.j, 0.08f, 0.5f, -1.f, 1.f);

		// the facing change since the last tick (render frames move the last facing on once a tick)
		auto signed_angular_difference = [](real32 a, real32 b) -> real32
		{
			real32 difference = b - a;
			while (difference > k_h1_pi) difference -= 2.f * k_h1_pi;
			while (difference < -k_h1_pi) difference += 2.f * k_h1_pi;
			return difference;
		};
		real_vector2d turning;
		turning.i = PIN(signed_angular_difference(g_h1_fp.last_render_facing.yaw, g_h1_fp.render_facing.yaw) * 30.f, -1.f, 1.f);
		turning.j = PIN(signed_angular_difference(g_h1_fp.last_render_facing.pitch, g_h1_fp.render_facing.pitch) * -30.f, -1.f, 1.f);
		g_h1_fp.last_render_facing = g_h1_fp.render_facing;
		h1_accelerate_to_position(&g_h1_fp.turning.i, &g_h1_fp.turning_velocity.i, turning.i, 0.03f, 0.2f, -1.f, 1.f);
		h1_accelerate_to_position(&g_h1_fp.turning.j, &g_h1_fp.turning_velocity.j, turning.j, 0.03f, 0.2f, -1.f, 1.f);
	}

	h1_accelerate_to_position(&g_h1_fp.firing_push_back, &g_h1_fp.firing_push_back_velocity, 0.f, 0.01f, 0.2f, 0.f, 1.f);
	if (g_h1_fp.firing_push_back == 1.f)
	{
		g_h1_fp.firing_push_back_velocity = 0.f;
	}

	if (g_h1_fp.interpolation_frame_count > 0)
	{
		g_h1_fp.interpolation_frame_index++;
		if (g_h1_fp.interpolation_frame_index >= g_h1_fp.interpolation_frame_count)
		{
			g_h1_fp.interpolation_frame_count = 0;
		}
	}

	if (unit->unit.target_info.primary_auto_aim_level != 0.f || unit->unit.current_zoom_level >= 0 || g_h1_fp.firing_push_back != 0.f ||
		g_h1_fp.position.i != 0.f || g_h1_fp.position.j != 0.f || g_h1_fp.turning.i != 0.f || g_h1_fp.turning.j != 0.f)
	{
		g_h1_fp.ticks_idle = 0;
		if (g_h1_fp.state == _h1_fp_state_posing)
		{
			h1_fp_set_state(_h1_fp_state_idle);
		}
	}
	else if (g_h1_fp.state == _h1_fp_state_idle)
	{
		const datum h1_globals_index = g_h1_cache_file->tag_find('matg', "globals\\globals");
		const h1_matg* globals = h1_globals_index != NONE ? (const h1_matg*)g_h1_cache_file->tag_get('matg', h1_globals_index) : NULL;
		const h1_matg_player_information* player_information = globals && globals->player_information.count > 0 ?
			g_h1_cache_file->block_get(globals->player_information, 0) : NULL;
		if (player_information)
		{
			if (!g_h1_fp.ticks_until_pose)
			{
				const real32 seconds = player_information->first_person_idle_time.lower +
					(player_information->first_person_idle_time.upper - player_information->first_person_idle_time.lower) * h1_fp_random_real();
				g_h1_fp.ticks_until_pose = (int16)(seconds * k_h1_ticks_per_second);
			}
			g_h1_fp.ticks_idle++;
			if (g_h1_fp.ticks_idle > g_h1_fp.ticks_until_pose)
			{
				g_h1_fp.ticks_until_pose = 0;
				if (h1_fp_random_real() >= player_information->first_person_skip_fraction)
				{
					h1_fp_set_state(_h1_fp_state_posing);
				}
			}
		}
	}
	else
	{
		g_h1_fp.ticks_idle = 0;
	}
	return;
}

// first_person_weapons.c first_person_weapon_build_node_matrices
// first_person_weapon_build_node_matrices up to the orientations: the tick's pose (the last pose kept for the frames between)
static void h1_fp_compute_pose(void)
{
	const h1_antr* graph = h1_fp_graph();
	const weapon_datum* weapon = (const weapon_datum*)object_try_and_get_and_verify_type(g_h1_fp.weapon_index, _object_mask_weapon);
	const h1_weap* definition = weapon ? h1_fp_weapon_definition(g_h1_fp.weapon_index) : NULL;
	if (!graph || !weapon || !definition || g_h1_fp.state_animation.index == NONE)
	{
		return;
	}
	const int32 node_count = MIN(graph->nodes.count, (int32)k_h1_maximum_first_person_nodes);

	const h1_antr_animations* state_animation = g_h1_cache_file->block_get(graph->animations, g_h1_fp.state_animation.index);
	h1_animation_get_node_orientations(state_animation, g_h1_fp.state_animation.frame_index, g_h1_fp.node_orientations);

	// the ammunition counter (the rounds loaded pick the frame)
	const int16 ammunition_index = h1_fp_animation_index(graph, _h1_fp_animation_ammunition);
	if (ammunition_index != NONE)
	{
		const h1_antr_animations* ammunition = g_h1_cache_file->block_get(graph->animations, ammunition_index);
		const int16 rounds_loaded = weapon->weapon.magazines[0].rounds_loaded;
		if (rounds_loaded < ammunition->frame_count)
		{
			h1_overlay_animation_apply(ammunition, rounds_loaded, 1.f, g_h1_fp.node_orientations);
		}
	}

	if (g_h1_fp.moving_animation.index != NONE)
	{
		h1_overlay_animation_apply(g_h1_cache_file->block_get(graph->animations, g_h1_fp.moving_animation.index), g_h1_fp.moving_animation.frame_index,
			1.f, g_h1_fp.node_orientations);
	}

	if (g_h1_fp.jitter_animation_index != NONE)
	{
		h1_overlay_animation_apply_continuous_scaled(g_h1_cache_file->block_get(graph->animations, g_h1_fp.jitter_animation_index),
			g_h1_fp.jitter_frame_index, weapon->weapon.overcharged + 0.5f, g_h1_fp.node_orientations);
	}

	const int16 overlay_index = h1_fp_animation_index(graph, _h1_fp_animation_overlays);
	if (overlay_index != NONE)
	{
		const h1_antr_animations* overlay = g_h1_cache_file->block_get(graph->animations, overlay_index);
		if (overlay->frame_count >= k_h1_first_person_overlay_frames)
		{
			if (g_h1_fp.position.i > 0.f) h1_overlay_animation_apply(overlay, 0, g_h1_fp.position.i, g_h1_fp.node_orientations);
			else if (g_h1_fp.position.i < 0.f) h1_overlay_animation_apply(overlay, 1, -g_h1_fp.position.i, g_h1_fp.node_orientations);
			if (g_h1_fp.position.j > 0.f) h1_overlay_animation_apply(overlay, 3, g_h1_fp.position.j, g_h1_fp.node_orientations);
			else if (g_h1_fp.position.j < 0.f) h1_overlay_animation_apply(overlay, 2, -g_h1_fp.position.j, g_h1_fp.node_orientations);
			if (g_h1_fp.turning.i > 0.f) h1_overlay_animation_apply(overlay, 4, g_h1_fp.turning.i, g_h1_fp.node_orientations);
			else if (g_h1_fp.turning.i < 0.f) h1_overlay_animation_apply(overlay, 5, -g_h1_fp.turning.i, g_h1_fp.node_orientations);
			if (g_h1_fp.turning.j > 0.f) h1_overlay_animation_apply(overlay, 7, g_h1_fp.turning.j, g_h1_fp.node_orientations);
			else if (g_h1_fp.turning.j < 0.f) h1_overlay_animation_apply(overlay, 6, -g_h1_fp.turning.j, g_h1_fp.node_orientations);
			if (g_h1_fp.firing_push_back > 0.f) h1_overlay_animation_apply(overlay, 8, g_h1_fp.firing_push_back, g_h1_fp.node_orientations);
		}
	}

	// model_animations.c interpolate_node_orientations: from the pose the last state left
	real_orientation* orientations = g_h1_fp.pose;
	csmemcpy(g_h1_fp.previous_pose, g_h1_fp.pose, sizeof(real_orientation) * node_count);
	csmemcpy(orientations, g_h1_fp.node_orientations, sizeof(real_orientation) * node_count);
	if (g_h1_fp.interpolation_frame_count > 0)
	{
		const real32 fraction = (real32)(g_h1_fp.interpolation_frame_index + 1) / (real32)g_h1_fp.interpolation_frame_count;
		for (int32 i = 0; i < node_count; i++)
		{
			const real_orientation* original = &g_h1_fp.original_node_orientations[i];
			real_orientation* target = &orientations[i];
			target->scale = original->scale * (1.f - fraction) + target->scale * fraction;
			h1_quaternions_interpolate(&original->rotation, &target->rotation, fraction, &target->rotation);
			h1_quaternion_normalize(&target->rotation);
			target->translation.x = original->translation.x * (1.f - fraction) + target->translation.x * fraction;
			target->translation.y = original->translation.y * (1.f - fraction) + target->translation.y * fraction;
			target->translation.z = original->translation.z * (1.f - fraction) + target->translation.z * fraction;
		}
	}

	if (!g_h1_fp.pose_valid)
	{
		csmemcpy(g_h1_fp.previous_pose, g_h1_fp.pose, sizeof(real_orientation) * node_count);
		g_h1_fp.pose_valid = true;
	}
	QueryPerformanceCounter(&g_h1_fp.pose_time);
	return;
}

// first_person_weapon_build_node_matrices at this frame's camera, the pose between the last two ticks (render_interpolation.c)
static void h1_fp_build_node_matrices(void)
{
	const render_camera* camera = &render_get()->camera;
	real_euler_angles2d facing;
	facing.yaw = atan2f(camera->forward.j, camera->forward.i);
	facing.pitch = atan2f(camera->forward.k, sqrtf(camera->forward.i * camera->forward.i + camera->forward.j * camera->forward.j));
	if (!g_h1_fp.rendered)
	{
		g_h1_fp.last_render_facing = facing;
	}
	g_h1_fp.render_facing = facing;
	g_h1_fp.rendered = true;

	const h1_antr* graph = h1_fp_graph();
	if (!graph || !g_h1_fp.pose_valid)
	{
		return;
	}
	const int32 node_count = MIN(graph->nodes.count, (int32)k_h1_maximum_first_person_nodes);
	LARGE_INTEGER now, frequency;
	QueryPerformanceCounter(&now);
	QueryPerformanceFrequency(&frequency);
	const real32 fraction = PIN((real32)(now.QuadPart - g_h1_fp.pose_time.QuadPart) / (real32)frequency.QuadPart * k_h1_ticks_per_second, 0.f, 1.f);
	real_orientation orientations[k_h1_maximum_first_person_nodes];
	for (int32 i = 0; i < node_count; i++)
	{
		const real_orientation* a = &g_h1_fp.previous_pose[i];
		const real_orientation* b = &g_h1_fp.pose[i];
		h1_quaternions_interpolate(&a->rotation, &b->rotation, fraction, &orientations[i].rotation);
		h1_quaternion_normalize(&orientations[i].rotation);
		orientations[i].translation.x = a->translation.x + (b->translation.x - a->translation.x) * fraction;
		orientations[i].translation.y = a->translation.y + (b->translation.y - a->translation.y) * fraction;
		orientations[i].translation.z = a->translation.z + (b->translation.z - a->translation.z) * fraction;
		orientations[i].scale = a->scale + (b->scale - a->scale) * fraction;
	}

	// model_animations.c animation_graph_node_matrices_from_orientations at the camera, parents before children
	real_matrix4x3 root_matrix;
	matrix4x3_rotation_from_vectors(&root_matrix, &camera->forward, &camera->up);
	root_matrix.position = camera->point;
	root_matrix.scale = 1.f;
	int16 node_indices[k_h1_maximum_first_person_nodes * 2];
	int32 read_index = 0;
	int32 write_index = 1;
	node_indices[0] = 0;
	while (read_index != write_index && node_count > 0)
	{
		const int16 node_index = node_indices[read_index++];
		const h1_antr_nodes* node = g_h1_cache_file->block_get(graph->nodes, node_index);
		const real_matrix4x3* parent = node_index == 0 || !VALID_INDEX(node->parent_node_index, node_count) ? &root_matrix : &g_h1_fp.node_matrices[node->parent_node_index];
		real_matrix4x3 local;
		h1_matrix4x3_from_orientation(&local, &orientations[node_index]);
		matrix4x3_multiply(parent, &local, &g_h1_fp.node_matrices[node_index]);
		if (VALID_INDEX(node->next_sibling_node_index, node_count) && write_index < NUMBEROF(node_indices))
		{
			node_indices[write_index++] = node->next_sibling_node_index;
		}
		if (VALID_INDEX(node->first_child_node_index, node_count) && write_index < NUMBEROF(node_indices))
		{
			node_indices[write_index++] = node->first_child_node_index;
		}
	}
	return;
}

// first_person_weapons.c model_build_remapping_table_for_animation_graph: every model node's graph node by name
static bool h1_fp_build_remapping_table(datum h1_model_index, const h1_antr* graph, int16* table)
{
	const h1_mode* model = (const h1_mode*)g_h1_cache_file->tag_get('mode', h1_model_index);
	if (!model)
	{
		return false;
	}
	bool valid = true;
	for (int32 model_node_index = 0; model_node_index < model->nodes.count && model_node_index < k_h1_maximum_first_person_nodes; model_node_index++)
	{
		const h1_mode_nodes* model_node = g_h1_cache_file->block_get(model->nodes, model_node_index);
		int16 found = NONE;
		for (int16 node_index = 0; node_index < graph->nodes.count; node_index++)
		{
			if (strcmp(model_node->name, g_h1_cache_file->block_get(graph->nodes, node_index)->name) == 0)
			{
				found = node_index;
				break;
			}
		}
		if (found == NONE)
		{
			valid = false;
		}
		table[model_node_index] = found;
	}
	return valid;
}

static int16 h1_fp_state_from_message(int16 message_type)
{
	switch (message_type)
	{
	case _h1_first_person_weapon_message_primary_fire: return _h1_fp_state_primary_fire;
	case _h1_first_person_weapon_message_secondary_fire: return _h1_fp_state_secondary_fire;
	case _h1_first_person_weapon_message_primary_misfire: return _h1_fp_state_primary_misfire;
	case _h1_first_person_weapon_message_secondary_misfire: return _h1_fp_state_secondary_misfire;
	case _h1_first_person_weapon_message_melee: return _h1_fp_state_melee;
	case _h1_first_person_weapon_message_light_on: return _h1_fp_state_light_on;
	case _h1_first_person_weapon_message_light_off: return _h1_fp_state_light_off;
	case _h1_first_person_weapon_message_reload_while_empty: return _h1_fp_state_reload_while_empty;
	case _h1_first_person_weapon_message_reload_while_full: return _h1_fp_state_reload_while_full;
	case _h1_first_person_weapon_message_put_away: return _h1_fp_state_put_away;
	case _h1_first_person_weapon_message_ready: return _h1_fp_state_ready;
	case _h1_first_person_weapon_message_charged: return _h1_fp_state_charged;
	case _h1_first_person_weapon_message_overheating: return _h1_fp_state_overheating;
	case _h1_first_person_weapon_message_throw_grenade: return _h1_fp_state_throw_grenade;
	case _h1_first_person_weapon_message_overheating_super_recoil: return _h1_fp_state_overheating_super_recoil;
	}
	return NONE;
}

static int16 h1_fp_animation_type_from_state(int16 state)
{
	switch (state)
	{
	case _h1_fp_state_idle: return _h1_fp_animation_idle;
	case _h1_fp_state_overheated: return _h1_fp_animation_overheated;
	case _h1_fp_state_charged: return _h1_fp_animation_overcharged;
	case _h1_fp_state_posing: return _h1_fp_animation_posing;
	case _h1_fp_state_primary_fire: return _h1_fp_animation_primary_fire;
	case _h1_fp_state_secondary_fire: return _h1_fp_animation_secondary_fire;
	case _h1_fp_state_primary_misfire: return _h1_fp_animation_primary_misfire;
	case _h1_fp_state_secondary_misfire: return _h1_fp_animation_secondary_misfire;
	case _h1_fp_state_melee: return _h1_fp_animation_melee;
	case _h1_fp_state_light_on: return _h1_fp_animation_light_on;
	case _h1_fp_state_light_off: return _h1_fp_animation_light_off;
	case _h1_fp_state_reload_while_empty: return _h1_fp_animation_reload_while_empty;
	case _h1_fp_state_reload_while_full: return _h1_fp_animation_reload_while_full;
	case _h1_fp_state_put_away: return _h1_fp_animation_put_away;
	case _h1_fp_state_ready: return _h1_fp_animation_ready;
	case _h1_fp_state_throw_grenade: return _h1_fp_animation_throw_grenade;
	case _h1_fp_state_throw_grenade_overheated: return _h1_fp_animation_throw_grenade_overheated;
	case _h1_fp_state_overheating: return _h1_fp_animation_overheating;
	case _h1_fp_state_overheating_again: return _h1_fp_animation_overheating_again;
	case _h1_fp_state_shotgun_enter_reload: return _h1_fp_animation_shotgun_enter;
	case _h1_fp_state_shotgun_exit_reload_empty: return _h1_fp_animation_shotgun_exit_empty;
	case _h1_fp_state_shotgun_exit_reload_full: return _h1_fp_animation_shotgun_exit_full;
	case _h1_fp_state_overheated_exit: return _h1_fp_animation_overheated_exit;
	case _h1_fp_state_overheating_super_recoil: return _h1_fp_animation_overheated_supercharge_enter;
	}
	return NONE;
}

// model_animations.c animation_update_internal (render only): a frame per tick, the sound at its frame
static int16 h1_fp_animation_update(const h1_antr* graph, s_h1_fp_animation_state* state, datum* sound_index)
{
	if (sound_index)
	{
		*sound_index = NONE;
	}
	if (!VALID_INDEX(state->index, graph->animations.count))
	{
		return _h1_animation_running;
	}
	const h1_antr_animations* animation = g_h1_cache_file->block_get(graph->animations, state->index);
	if (sound_index && animation->sound_index != NONE && animation->sound_frame_index == state->frame_index &&
		VALID_INDEX(animation->sound_index, graph->sound_references.count))
	{
		*sound_index = g_h1_cache_file->block_get(graph->sound_references, animation->sound_index)->sound.index;
	}

	int16 result = _h1_animation_running;
	state->frame_index++;
	if (state->frame_index >= animation->frame_count)
	{
		if (animation->loop_frame_index > 0)
		{
			state->frame_index = MIN(animation->loop_frame_index, (int16)(animation->frame_count - 1));
			result = _h1_animation_looped;
		}
		else
		{
			// animation_choose_random_permutation_internal: from the parent animation, the first permutation the random weight reaches
			const int16 first_index = VALID_INDEX(animation->main_animation_index, graph->animations.count) ? animation->main_animation_index : state->index;
			const real32 random = h1_fp_random_real();
			int16 animation_index = first_index;
			while (VALID_INDEX(animation_index, graph->animations.count))
			{
				const h1_antr_animations* permutation = g_h1_cache_file->block_get(graph->animations, animation_index);
				if (random <= permutation->relative_weight || permutation->next_animation_index == NONE)
				{
					break;
				}
				animation_index = permutation->next_animation_index;
			}
			state->index = VALID_INDEX(animation_index, graph->animations.count) ? animation_index : first_index;
			state->frame_index = 0;
			result = _h1_animation_restarted;
		}
	}
	else if (state->frame_index + 1 == animation->frame_count && animation->loop_frame_index == 0)
	{
		result = _h1_animation_will_restart_on_next_frame;
	}
	else if (state->frame_index == animation->key_frame_index || state->frame_index == animation->second_key_frame_index)
	{
		result = _h1_animation_key_frame;
	}
	return result;
}

static real32 h1_fp_random_real(void)
{
	g_h1_fp.random_seed = g_h1_fp.random_seed * 1664525u + 1013904223u;
	return (real32)(g_h1_fp.random_seed >> 8) / (real32)(1u << 24);
}

static bool h1_animation_node_flag(int32 flags_0, int32 flags_1, int32 node_index)
{
	return node_index < 32 ? TEST_BIT(flags_0, node_index) : TEST_BIT(flags_1, node_index - 32);
}

static void h1_quaternion_decompress_8byte(const uint8* data, real_quaternion* quaternion)
{
	const int16* q = (const int16*)data;
	const real32 scale = 1.f / 32767.f;
	quaternion->v = { q[0] * scale, q[1] * scale, q[2] * scale };
	quaternion->w = q[3] * scale;
	return;
}

// model_animations.c animation_get_node_orientations (uncompressed halo 1 animations)
static void h1_animation_get_node_orientations(const h1_antr_animations* animation, int16 frame_index, real_orientation* orientations)
{
	const uint8* data = (const uint8*)g_h1_cache_file->data_get(animation->frame_data);
	const uint8* default_data = (const uint8*)g_h1_cache_file->data_get(animation->default_data);
	if (data)
	{
		data += PIN(frame_index, 0, MAX(animation->frame_count - 1, 0)) * animation->frame_size;
	}
	for (int32 node_index = 0; node_index < animation->node_count && node_index < k_h1_maximum_first_person_nodes; node_index++)
	{
		real_orientation* orientation = &orientations[node_index];
		const uint8** source = h1_animation_node_flag(animation->node_rotation_flags_0, animation->node_rotation_flags_1, node_index) ? &data : &default_data;
		if (*source)
		{
			h1_quaternion_decompress_8byte(*source, &orientation->rotation);
			*source += 8;
		}
		source = h1_animation_node_flag(animation->node_transformation_flags_0, animation->node_transformation_flags_1, node_index) ? &data : &default_data;
		if (*source)
		{
			csmemcpy(&orientation->translation, *source, sizeof(real_point3d));
			*source += sizeof(real_point3d);
		}
		source = h1_animation_node_flag(animation->node_scale_flags_0, animation->node_scale_flags_1, node_index) ? &data : &default_data;
		if (*source)
		{
			orientation->scale = *(const real32*)*source;
			*source += sizeof(real32);
		}
	}
	return;
}

// model_animations.c overlay_animation_apply_scaled (overlay_animation_apply at a scale of one)
static void h1_overlay_animation_apply(const h1_antr_animations* animation, int16 frame_index, real32 scale, real_orientation* orientations)
{
	if (animation->type != _h1_animation_overlay || frame_index < 0 || frame_index >= animation->frame_count)
	{
		return;
	}
	const uint8* data = (const uint8*)g_h1_cache_file->data_get(animation->frame_data);
	if (!data)
	{
		return;
	}
	data += frame_index * animation->frame_size;
	const real_quaternion identity = { { 0.f, 0.f, 0.f }, 1.f };
	for (int32 node_index = 0; node_index < animation->node_count && node_index < k_h1_maximum_first_person_nodes; node_index++)
	{
		real_orientation* orientation = &orientations[node_index];
		if (h1_animation_node_flag(animation->node_rotation_flags_0, animation->node_rotation_flags_1, node_index))
		{
			real_quaternion rotation;
			h1_quaternion_decompress_8byte(data, &rotation);
			data += 8;
			if (scale != 1.f)
			{
				h1_quaternions_interpolate(&identity, &rotation, scale, &rotation);
			}
			h1_quaternions_multiply(&rotation, &orientation->rotation, &orientation->rotation);
		}
		if (h1_animation_node_flag(animation->node_transformation_flags_0, animation->node_transformation_flags_1, node_index))
		{
			const real_point3d* translation = (const real_point3d*)data;
			data += sizeof(real_point3d);
			orientation->translation.x += translation->x * scale;
			orientation->translation.y += translation->y * scale;
			orientation->translation.z += translation->z * scale;
		}
		if (h1_animation_node_flag(animation->node_scale_flags_0, animation->node_scale_flags_1, node_index))
		{
			const real32 node_scale = *(const real32*)data;
			data += sizeof(real32);
			orientation->scale *= node_scale * scale + (1.f - scale);
		}
	}
	return;
}

// model_animations.c overlay_animation_apply_continuous_scaled: between two frames
static void h1_overlay_animation_apply_continuous_scaled(const h1_antr_animations* animation, real32 real_frame_index, real32 scale, real_orientation* orientations)
{
	if (animation->type != _h1_animation_overlay || animation->frame_count <= 0)
	{
		return;
	}
	real32 fraction = fmodf(real_frame_index, 1.f);
	int16 frame_index = (int16)floorf(real_frame_index);
	if (frame_index >= animation->frame_count)
	{
		frame_index = animation->frame_count - 1;
		fraction = 1.f;
	}
	frame_index = MAX(frame_index, (int16)0);
	const int16 next_frame_index = frame_index == animation->frame_count - 1 ? (int16)0 : (int16)(frame_index + 1);
	const uint8* base = (const uint8*)g_h1_cache_file->data_get(animation->frame_data);
	if (!base)
	{
		return;
	}
	const uint8* data = base + frame_index * animation->frame_size;
	const uint8* next_data = base + next_frame_index * animation->frame_size;
	const real_quaternion identity = { { 0.f, 0.f, 0.f }, 1.f };
	for (int32 node_index = 0; node_index < animation->node_count && node_index < k_h1_maximum_first_person_nodes; node_index++)
	{
		real_orientation* orientation = &orientations[node_index];
		if (h1_animation_node_flag(animation->node_rotation_flags_0, animation->node_rotation_flags_1, node_index))
		{
			real_quaternion this_rotation, next_rotation, rotation;
			h1_quaternion_decompress_8byte(data, &this_rotation);
			h1_quaternion_decompress_8byte(next_data, &next_rotation);
			data += 8;
			next_data += 8;
			h1_quaternions_interpolate(&this_rotation, &next_rotation, fraction, &rotation);
			h1_quaternion_normalize(&rotation);
			h1_quaternions_interpolate(&identity, &rotation, scale, &rotation);
			h1_quaternions_multiply(&rotation, &orientation->rotation, &orientation->rotation);
		}
		if (h1_animation_node_flag(animation->node_transformation_flags_0, animation->node_transformation_flags_1, node_index))
		{
			const real_point3d* a = (const real_point3d*)data;
			const real_point3d* b = (const real_point3d*)next_data;
			data += sizeof(real_point3d);
			next_data += sizeof(real_point3d);
			orientation->translation.x += (a->x + (b->x - a->x) * fraction) * scale;
			orientation->translation.y += (a->y + (b->y - a->y) * fraction) * scale;
			orientation->translation.z += (a->z + (b->z - a->z) * fraction) * scale;
		}
		if (h1_animation_node_flag(animation->node_scale_flags_0, animation->node_scale_flags_1, node_index))
		{
			const real32 a = *(const real32*)data;
			const real32 b = *(const real32*)next_data;
			data += sizeof(real32);
			next_data += sizeof(real32);
			orientation->scale *= (a + (b - a) * fraction) * scale + (1.f - scale);
		}
	}
	return;
}

// real_math.c quaternions_multiply
static void h1_quaternions_multiply(const real_quaternion* q0, const real_quaternion* q1, real_quaternion* result)
{
	const real_quaternion a = *q0;
	const real_quaternion b = *q1;
	result->v.i = a.v.j * b.v.k + a.v.i * b.w + a.w * b.v.i - a.v.k * b.v.j;
	result->v.j = a.w * b.v.j + a.v.j * b.w + a.v.k * b.v.i - a.v.i * b.v.k;
	result->v.k = a.v.k * b.w + a.v.i * b.v.j + a.w * b.v.k - a.v.j * b.v.i;
	result->w = a.w * b.w - a.v.i * b.v.i - a.v.j * b.v.j - a.v.k * b.v.k;
	return;
}

// real_math.c quaternions_interpolate (the shorter way round)
static void h1_quaternions_interpolate(const real_quaternion* q0, const real_quaternion* q1, real32 t, real_quaternion* result)
{
	const real32 v = 1.f - t;
	if (q0->v.i * q1->v.i + q0->v.j * q1->v.j + q0->v.k * q1->v.k + q0->w * q1->w < 0.f)
	{
		t = -t;
	}
	const real_quaternion a = *q0;
	const real_quaternion b = *q1;
	result->v.i = a.v.i * v + b.v.i * t;
	result->v.j = a.v.j * v + b.v.j * t;
	result->v.k = a.v.k * v + b.v.k * t;
	result->w = a.w * v + b.w * t;
	return;
}

static void h1_quaternion_normalize(real_quaternion* q)
{
	const real32 length = sqrtf(q->v.i * q->v.i + q->v.j * q->v.j + q->v.k * q->v.k + q->w * q->w);
	if (length > 0.f)
	{
		q->v.i /= length;
		q->v.j /= length;
		q->v.k /= length;
		q->w /= length;
	}
	return;
}

// matrix_math.c matrix4x3_from_orientation (matrix4x3_rotation_from_quaternion)
static void h1_matrix4x3_from_orientation(real_matrix4x3* matrix, const real_orientation* orientation)
{
	const real_quaternion* q = &orientation->rotation;
	real32 scale = q->v.i * q->v.i + q->v.j * q->v.j + q->v.k * q->v.k + q->w * q->w;
	scale = scale != 0.f ? 2.f / scale : 0.f;
	const real32 x = scale * q->v.i;
	const real32 y = scale * q->v.j;
	const real32 z = scale * q->v.k;
	const real32 wx = q->w * x, wy = q->w * y, wz = q->w * z;
	const real32 xx = q->v.i * x, xy = q->v.i * y, xz = q->v.i * z;
	const real32 yy = q->v.j * y, yz = q->v.j * z, zz = q->v.k * z;
	matrix->vectors.forward = { 1.f - (yy + zz), xy - wz, xz + wy };
	matrix->vectors.left = { xy + wz, 1.f - (xx + zz), yz - wx };
	matrix->vectors.up = { xz - wy, yz + wx, 1.f - (xx + yy) };
	matrix->scale = orientation->scale;
	matrix->position = orientation->translation;
	return;
}

// real_math.c accelerate_to_position (not periodic)
static bool h1_accelerate_to_position(real32* position, real32* velocity, real32 target_position, real32 maximum_velocity, real32 acceleration,
	real32 minimum_position, real32 maximum_position)
{
	real32 current_velocity = *velocity;
	const real32 current_position = *position;
	const real32 delta = target_position - current_position;
	const real32 limit = MIN(maximum_velocity, acceleration);
	real32 output_position;
	bool result = false;
	if (fabsf(delta - current_velocity) <= limit)
	{
		current_velocity = 0.f;
		output_position = PIN(target_position, minimum_position, maximum_position);
		result = true;
	}
	else
	{
		const real32 braking_distance = (maximum_velocity + maximum_velocity) * fabsf(delta);
		real32 speed = braking_distance >= acceleration * acceleration ? acceleration : sqrtf(braking_distance);
		if (delta < 0.f)
		{
			speed = -speed;
		}
		real32 step = speed - current_velocity;
		if (fabsf(step) > maximum_velocity)
		{
			step = step < 0.f ? -maximum_velocity : maximum_velocity;
		}
		current_velocity += step;
		output_position = PIN(step * 0.5f + current_velocity + current_position, minimum_position, maximum_position);
	}
	*velocity = current_velocity;
	*position = output_position;
	return result;
}

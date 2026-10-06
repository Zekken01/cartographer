#include "stdafx.h"
#include "h1_ai_internal.h"

/*
* units.c: the functions of it the AI calls.
*/

namespace h1_ai
{

/* ---------- units/units.c: its declarations */

/* ---------- headers */

/* ---------- constants */

enum
{
	_unit_region_destroyed_head_bit = 9,
};

enum
{
	_unit_weapon_overlay_primary_recoil = 1,
	_unit_weapon_overlay_secondary_recoil,
	_unit_weapon_overlay_primary_charged,
	_unit_weapon_overlay_secondary_charged,
	_unit_weapon_overlay_primary_chamber,
	_unit_weapon_overlay_secondary_chamber,
};

enum
{
	NUMBER_OF_UNIT_ANIMATION_IMPULSES = 14,
	_unit_seat_unknown8_bit = 8,
};

enum
{
	_vehicle_seat_desire_not_driver,
	_vehicle_seat_desire_gunner,
	_vehicle_seat_desire_passenger,
	_vehicle_seat_desire_driver,
	_vehicle_seat_desire_any,
	NUMBER_OF_VEHICLE_SEAT_DESIRE_TYPES,
};

enum
{
	_unit_record_damage_driver_seat_type = 9,
};

enum
{
	_unit_debug_function_active_bit = 2,
};

enum
{
	_unit_damage_animation_soft_ping = 0,
	_unit_damage_animation_hard_ping,
	_unit_damage_animation_soft_kill,
	_unit_damage_animation_hard_kill,
};

enum
{
	_unit_damage_direction_front = 0,
	_unit_damage_direction_left,
	_unit_damage_direction_right,
	_unit_damage_direction_back,
};

enum
{
	_unit_damage_part_head = 2,
};

/* January computes these direction-cone limits from single-precision pi, then
   promotes the rounded results for the double-precision fabs comparisons. */
#define UNIT_DAMAGE_REAR_CONE_ANGLE 0.7853981852531433
#define UNIT_DAMAGE_FRONT_CONE_ANGLE 2.159845009446144

#define UNIT_DAMAGE_AFTERMATH_ANIMATION_FLAGS_MASK \
	(FLAG(_object_being_damaged_region_destroyed_bit) | \
	 FLAG(_object_being_damaged_shield_depleted_bit) | \
	 FLAG(_object_being_damaged_force_hard_ping_bit))

enum
{
	_damage_definition_pings_resistant_units_bit = 2,
	_damage_definition_does_not_ping_units_bit = 4,
};

/* ---------- macros */

#define unit_get_current_weapon_index(unit_index) unit_inventory_get_weapon((unit_index), unit_get((unit_index))->unit.current_weapon_index)
#define unit_get_desired_weapon_index(unit_index) unit_inventory_get_weapon((unit_index), unit_get((unit_index))->unit.desired_weapon_index)

/* ---------- structures */

struct unit_globals
{
	short next_timer;
	short highest_timer;
	boolean used_time;
	char pad[3];
};

struct unit_acceleration_plan
{
	boolean at_rest;
	char pad[3];
	real initial_position;
	real initial_velocity;
	real acceleration;
	real acceleration_time;
	real coast_time;
	real deceleration;
	real deceleration_time;
};
typedef char unit_acceleration_plan_size_check[
	sizeof(struct unit_acceleration_plan) == 0x20 ? 1 : -1];

typedef char unit_control_data_size_assert[
	sizeof(struct unit_control_data) == 0x40 ? 1 : -1];

struct unit_initial_weapon
{
	struct tag_reference weapon;
	long unused[5];
};

typedef char unit_initial_weapon_size_assert[
	sizeof(struct unit_initial_weapon) == 0x24 ? 1 : -1];

/* This tag-block element layout is carried locally because this bounded wave
 * does not own the shared game-globals header. */
struct game_globals_falling_damage
{
	byte unused0[0x48];
	long unknown48;
	byte unused4c[0x20];
	struct tag_reference flaming_death_damage_effect;
	byte unused1[0x1C];
};
typedef char game_globals_falling_damage_size_check[
	sizeof(struct game_globals_falling_damage) == 0x98 ? 1 : -1];
typedef char game_globals_flaming_death_offset_check[
	offsetof(struct game_globals_falling_damage, flaming_death_damage_effect) == 0x6C ? 1 : -1];

/* ---------- prototypes */

void unit_detach_from_parent(
	long unit_index);
void unit_start_flaming_to_death(
	long unit_index,
	long attacker_object_index);
void unit_flame_to_death(
	long unit_index);
boolean unit_unsuspecting(
	long unit_index,
	real_point3d const *point);
void unit_cause_melee_damage(
	long unit_index,
	boolean melee_hit,
	long target_object_index,
	short node_index,
	short region_index,
	short material_index,
	real_vector3d const *object_normal);

static char const *base_seat_label_get(short base_seat_index);
static short seat_label_to_base_seat_index(char const *seat_label);
static char const *base_weapon_label_get(short base_weapon_index);

static void unit_refresh_illumination(long unit_index);

static boolean unit_euler_axis_doplan(
	struct unit_acceleration_plan *plan,
	real delta_time,
	real position,
	real *new_position,
	real velocity,
	real *new_velocity);
static void unit_euler_axis_buildplan(
	real position,
	real velocity,
	real maximum_velocity,
	real maximum_acceleration,
	struct unit_acceleration_plan *plan);
static void unit_euler_axis_couple(
	struct unit_acceleration_plan *first_plan,
	struct unit_acceleration_plan *second_plan,
	real maximum_velocity,
	real maximum_acceleration);

static void unit_melee_sound(
	long unit_index,
	short material_type,
	long damage_effect_index);
static void unit_update_driver_and_gunner(
	long unit_index);
static short unit_first_free_weapon_index(
	long unit_index);

static short unit_animation_impulse_get_index(
	short animation_impulse,
	short *interpolation_frame_count);
static boolean unit_can_play_animation_impulse(
	long unit_index,
	short animation_impulse);

static boolean unit_animation_aiming_screen(
	struct unit_animation *animation);
static boolean unit_animation_state_can_be_entered_without_animation(
	short state);
static short unit_animation_compute_interpolation_frame_count(
	short new_state,
	short old_state);
static boolean unit_animation_overlay_action_loops(
	struct unit_animation *animation);
static boolean unit_animation_state_interruptable(
	struct unit_animation *animation,
	short desired_state);
static boolean unit_animation_state_loops(
	struct unit_animation *animation);
static long unit_animation_state_get_aiming_screen_index(
	short state);
static void unit_set_animation(
	long unit_index,
	long animation_graph_index,
	short animation_index);
static short unit_animation_update(
	long unit_index,
	long animation_graph_index,
	struct animation_state *animation);
static char const *unit_get_current_weapon_label(
	long unit_index);
static void unit_align_facing(
	long unit_index,
	real_vector2d const *alignment_vector);

static short unit_weapon_next_index(long unit_index, short current_index, short delta);
static void unit_ready_desired_weapon(
	long unit_index,
	boolean immediate);

static void unit_throw_grenade_move_to_hand(long unit_index);

static boolean unit_animation_weapon_ik(
	struct unit_animation *animation);
static boolean unit_animation_vehicle_ik(
	struct unit_animation *animation);
static boolean unit_animation_busy(struct unit_animation *animation);

static boolean unit_set_or_test_seat_and_weapon_label(
	long object_index,
	char const *seat_label,
	char const *weapon_label,
	boolean change_flag);

static boolean unit_animation_set_state(
	long unit_index,
	short new_state);

static boolean unit_vectors_are_valid(long unit_index);
static void unit_throw_grenade_release(long unit_index, boolean premature);

static void unit_seat_update(long object_index);

static char const *unit_get_seat_label(long object_index);

static void unit_cause_continuous_melee_damage(long unit_index);

static long unit_get_weapon(struct unit_datum *unit, short index);
static void unit_drop_item(long unit_index, long item_index);
/* port/linux/game/network_objects.c's */
boolean network_objects_creating_host_object(void);
/* network_game_globals.c's */
boolean network_game_distributed_client(void);
static void unit_drop_grenades(
	long unit_index);
static void unit_drop_inventory_weapons(
	long unit_index);

static void unit_verify_vectors(long unit_index, char const *debugstring);
static void unit_running_blind(long unit_index, real_vector3d *run_vector);

static boolean unit_integrated_night_vision_is_active(long unit_index);

void unit_detach_from_parent(
	long unit_index);

extern char const *base_seat_labels[NUMBER_OF_UNIT_BASE_SEATS];

/* ---------- globals */

short magic_base_animation_seat_index = NONE;

static struct unit_globals *unit_globals;

static struct profile_section unit_update_section = {"unit_update", NONE, TRUE};

boolean debug_objects_unit_mouth_apeture;
boolean debug_objects_unit_seats;
boolean debug_objects_unit_vectors;
boolean stun_enable;
boolean debug_damage_taken;
boolean debug_unit_illumination;
boolean debug_unit_animations;
boolean debug_unit_all_animations;

/* ---------- public code */

enum
{
	_unit_function_none = 0,
	_unit_function_driver_seat_power,
	_unit_function_gunner_seat_power,
	_unit_function_aiming_change,
	_unit_function_mouth_aperture,
	_unit_function_integrated_light_power,
	_unit_function_can_blink,
	_unit_function_shield_sapping,
	NUMBER_OF_UNIT_FUNCTION_MODES,
};

// HCEX_Release.pdb and the September 2001 map both name this private
// helper unit_add_initial_weapons (file-static; no cachebeta public).

static void unit_ping_animation(
	long unit_index,
	boolean killed,
	boolean feign_death,
	boolean suppress_random_death_frame,
	boolean suppress_hard_ping,
	boolean force_hard_ping,
	real damage_direction_angle,
	short damage_part,
	real_vector2d const *alignment_vector);

enum
{
	_scenario_unit_dead_bit = 0,
};

struct scenario_unit_datum
{
	real body_vitality;
	unsigned long flags;
};
typedef char scenario_unit_datum_size_check[
	sizeof(struct scenario_unit_datum) == 0x08 ? 1 : -1];

/* port: a swap's weapon out: the one the player chose, though the weapon

/* ---------- private code */

char const *base_seat_labels[NUMBER_OF_UNIT_BASE_SEATS] = {"asleep", "alert", "stand", "crouch", "flee", "flaming"};

/* insert verbatim into source/units/units.c immediately after the closing
   brace of unit_cause_melee_damage (line 8403) and before
   'static void unit_align_facing(' */
enum
{
	_collision_result_breakable_surface_bit = 3,
};

/* the globals' first multiplayer weapon, NONE where there is none: a

/* port: the melee damage of a unit with no weapon (a gametype's loadout of
none): its own, else (a player's biped has none: players always had a
weapon) the blow of the globals' first multiplayer weapon, the assault
rifle's. network_damage.c takes it as the player's. */

// TODO: Fix

/* Verify the public seat-helper declaration without perturbing this legacy
 * translation unit's authenticated function-declaration order. */
/* the distributed netcode (port/linux/game/network_objects.c): a client's
unit carries the host's weapons, the same objects, moved in and out as the
host's unit had them (the host has applied the game's rules) */

void unit_network_forget_weapon(long unit_index, short slot);

/* the weapon into the slot, as unit_add_weapon_to_inventory puts one in */

/* the slot's weapon out onto the ground, as unit_drop_current_weapon drops
one (where the host has it the objects' states say) */

/* the slot emptied, its weapon about to be deleted */

/* ---------- units/units.c: its functions */

boolean unit_get_seat_entrance_point(
	long unit_index,
	long target_unit_index,
	short seat_index,
	real_point3d *entry_position,
	real_point3d *exit_position,
	real_point3d *seat_transform)
{
	struct unit_datum *unit = unit_get(unit_index);
	struct unit_definition *unit_definition =
		unit_definition_get(unit->definition_index);
	struct model *model = model_definition_get(
		unit_definition->object.model.index);
	real_matrix4x3 entrance_matrix;
	struct object_marker seat_marker;
	real_matrix4x3 root_matrix;
	struct object_marker enter_hint_marker;
	char enter_hint_marker_name[256];
	struct animation_graph *animation_graph = animation_graph_definition_get(
		unit_definition->object.animation_graph.index);
	struct unit_datum *target_unit = unit_get(target_unit_index);
	struct unit_definition *target_unit_definition =
		unit_definition_get(target_unit->definition_index);
	struct unit_seat *seat = TAG_BLOCK_GET_ELEMENT(
		&target_unit_definition->unit.seats,
		seat_index,
		struct unit_seat);
	short animation_seat_index;
	boolean result;

	result = FALSE;
	animation_seat_index = 0;
	if (animation_graph->unit_seats.count > 0)
	{
		do
		{
			struct animation_graph_unit_seat *animation_seat = TAG_BLOCK_GET_ELEMENT(
				&animation_graph->unit_seats,
				animation_seat_index,
				struct animation_graph_unit_seat);

			if (!_stricmp(animation_seat->label, seat->label))
			{
				if (animation_seat_index != NONE)
				{
					animation_seat = TAG_BLOCK_GET_ELEMENT(
						&animation_graph->unit_seats,
						animation_seat_index,
						struct animation_graph_unit_seat);

					if (animation_seat->animations.count >
						_unit_seat_animation_seat_enter)
					{
						short animation_index = animation_graph_animation_index_get(
							&animation_seat->animations)
								[_unit_seat_animation_seat_enter].animation_index;

						if (animation_index != NONE)
						{
							struct animation *animation = TAG_BLOCK_GET_ELEMENT(
								&animation_graph->animations,
								animation_index,
								struct animation);

							object_get_marker_by_name(
								target_unit_index,
								seat->marker_name,
								&seat_marker,
								1);
							animation_get_root_matrix(model, animation, 0, &root_matrix);
							matrix4x3_multiply(
								&seat_marker.matrix,
								&root_matrix,
								&entrance_matrix);

							csstrcpy(enter_hint_marker_name, seat->marker_name);
							csstrcat(enter_hint_marker_name, " enter-hint");
							object_get_marker_by_name(
								target_unit_index,
								enter_hint_marker_name,
								&enter_hint_marker,
								1);

							if (exit_position)
							{
								*exit_position = seat_marker.matrix.position;
							}
							if (entry_position)
							{
								*entry_position = entrance_matrix.position;
							}
							if (seat_transform)
							{
								*seat_transform = enter_hint_marker.matrix.position;
							}

							result = TRUE;
						}
					}
				}

				break;
			}

			animation_seat_index++;
		}
		while (animation_seat_index < animation_graph->unit_seats.count);
	}

	return result;
}

boolean unit_get_melee_range_and_ticks(
	long unit_index,
	boolean secondary,
	short *melee_tick,
	real *attack_time,
	short *frame_count,
	real *damage_time)
{
	boolean result;
	struct unit_datum *unit = unit_get(unit_index);
	struct unit_definition *unit_definition =
		unit_definition_get(unit->definition_index);
	struct animation_graph *animation_graph;
	struct animation_graph_unit_seat *animation_seat;
	struct animation_graph_weapon_class *weapon_class;
	long weapon_class_animation_index;
	short animation_index;

	model_definition_get(unit_definition->object.model.index);
	animation_graph = animation_graph_definition_get(
		unit_definition->object.animation_graph.index);
	animation_seat = TAG_BLOCK_GET_ELEMENT(
		&animation_graph->unit_seats,
		unit->unit.animation.seat_index,
		struct animation_graph_unit_seat);
	weapon_class = TAG_BLOCK_GET_ELEMENT(
		&animation_seat->weapon_classes,
		unit->unit.animation.weapon_index,
		struct animation_graph_weapon_class);

	weapon_class_animation_index = secondary ?
		_unit_weapon_class_animation_melee_airborne :
		_unit_weapon_class_animation_melee_attack;
	if (VALID_INDEX(
		weapon_class_animation_index,
		weapon_class->animations.count))
	{
		animation_index = animation_graph_animation_index_get(
			&weapon_class->animations)
				[weapon_class_animation_index].animation_index;
	}
	else
	{
		animation_index = NONE;
	}

	result = FALSE;
	if (animation_index != NONE)
	{
		struct animation *animation = TAG_BLOCK_GET_ELEMENT(
			&animation_graph->animations,
			animation_index,
			struct animation);

		animation_get_x_offsets(animation, attack_time, damage_time);
		if (melee_tick)
		{
			*melee_tick = animation->private_key_frame_index;
		}
		if (frame_count)
		{
			*frame_count = animation->frame_count;
		}

		result = TRUE;
	}

	return result;
}

boolean unit_controllable(
	long unit_index)
{
	return TEST_FLAG(unit_get(unit_index)->unit.flags, _unit_controllable_bit);
}

boolean unit_is_busy(
	long unit_index)
{
	struct unit_datum *unit = unit_get(unit_index);

	return unit_animation_busy(&unit->unit.animation);
}

boolean unit_seat_filled(
	long unit_index,
	short seat_index)
{
	struct object_iterator iterator;
	struct unit_datum *unit;
	boolean filled = FALSE;

	object_iterator_new(&iterator, _object_mask_unit, 0);

	while ((unit = (struct unit_datum *)object_iterator_next(&iterator))!=NULL)
	{
		if (unit->object.parent_object_index==unit_index && unit->unit.parent_seat_index==seat_index)
		{
			filled = TRUE;
			break;
		}
	}

	return filled;
}

boolean unit_seat_is_driver(
	long unit_index,
	short seat_index)
{
	struct unit_definition *unit_definition = unit_definition_get(unit_get(unit_index)->definition_index);
	boolean is_driver = FALSE;

	if (seat_index>=0 && seat_index<unit_definition->unit.seats.count)
	{
		struct unit_seat *seat = TAG_BLOCK_GET_ELEMENT(&unit_definition->unit.seats, seat_index, struct unit_seat);
		is_driver = TEST_FLAG(seat->flags, _unit_seat_driver_bit);
	}

	return is_driver;
}

boolean unit_seat_is_gunner(
	long unit_index,
	short seat_index)
{
	struct unit_definition *unit_definition = unit_definition_get(unit_get(unit_index)->definition_index);
	boolean is_gunner = FALSE;

	if (seat_index>=0 && seat_index<unit_definition->unit.seats.count)
	{
		struct unit_seat *seat = TAG_BLOCK_GET_ELEMENT(&unit_definition->unit.seats, seat_index, struct unit_seat);
		is_gunner = TEST_FLAG(seat->flags, _unit_seat_gunner_bit);
	}

	return is_gunner;
}

boolean unit_seat_allow_noncombatants(
	long unit_index,
	short seat_index)
{
	struct unit_definition *unit_definition = unit_definition_get(unit_get(unit_index)->definition_index);
	boolean allow_noncombatants = FALSE;

	if (seat_index>=0 && seat_index<unit_definition->unit.seats.count)
	{
		struct unit_seat *seat = TAG_BLOCK_GET_ELEMENT(&unit_definition->unit.seats, seat_index, struct unit_seat);
		allow_noncombatants = TEST_FLAG(seat->flags, _unit_seat_allows_noncombatants_bit);
	}

	return allow_noncombatants;
}

short unit_get_grenade_count(
	long unit_index,
	short grenade_type)
{
	struct unit_datum *unit = unit_get(unit_index);

	if (grenade_type!=NONE)
	{
		match_assert("c:\\halo\\SOURCE\\units\\units.c", 7847, grenade_type==NONE || (grenade_type>=0 && grenade_type<NUMBER_OF_UNIT_GRENADE_TYPES));

		return unit->unit.grenade_counts[grenade_type];
	}

	return 0;
}

short unit_get_current_grenade_type(
	long unit_index)
{
	struct unit_datum *unit = unit_get(unit_index);

	match_assert("c:\\halo\\SOURCE\\units\\units.c", 7864, unit->unit.current_grenade_index==NONE || (unit->unit.current_grenade_index>=0 && unit->unit.current_grenade_index<NUMBER_OF_UNIT_GRENADE_TYPES));

	return unit->unit.current_grenade_index;
}

void unit_get_facing_vector(
	long unit_index,
	real_vector3d *facing_vector)
{
	object_get_orientation(unit_index, facing_vector, NULL);

	return;
}

void unit_get_center_of_mass(
	long unit_index,
	real_point3d *center_of_mass)
{
	struct object_marker body_marker;

	object_get_marker_by_name(unit_index, "body", &body_marker, 1);
	*center_of_mass = body_marker.matrix.position;

	return;
}

void unit_get_aiming_vector(
	long unit_index,
	real_vector3d *aiming_vector)
{
	*aiming_vector = unit_get(unit_index)->unit.aiming_vector;

	return;
}

boolean unit_can_see_point(
	long unit_index,
	real_point3d const *point,
	real field_of_view)
{
	boolean can_see_point = FALSE;

	if (unit_index!=NONE)
	{
		struct unit_datum *unit = unit_get(unit_index);
		struct object_marker head_marker;
		real_vector3d direction;

		object_get_marker_by_name(unit_index, "head", &head_marker, 1);
		vector_from_points3d(&head_marker.matrix.position, point, &direction);
		normalize3d(&direction);

		if (dot_product3d(&direction, &unit->unit.looking_vector)>cosine(field_of_view))
		{
			can_see_point = TRUE;
		}
	}

	return can_see_point;
}

boolean unit_has_animation_to_enter_seat(
	long unit_index,
	long target_unit_index,
	short seat_index)
{
	struct unit_datum *target_unit = unit_get(target_unit_index);
	struct unit_definition *target_unit_definition = unit_definition_get(target_unit->definition_index);
	boolean has_animation = FALSE;

	if (seat_index>=0 && seat_index<target_unit_definition->unit.seats.count)
	{
		struct unit_datum *unit = unit_get(unit_index);

		if (unit->object.type==_object_type_vehicle)
		{
			has_animation = TRUE;
		}
		else
		{
			struct unit_seat *seat = TAG_BLOCK_GET_ELEMENT(&target_unit_definition->unit.seats, seat_index, struct unit_seat);

			if (unit_set_or_test_seat_and_weapon_label(unit_index, seat->label, NULL, FALSE))
			{
				has_animation = TRUE;
			}
		}
	}

	return has_animation;
}

void unit_get_looking_vector(
	long unit_index,
	real_vector3d *looking_vector)
{
	*looking_vector = unit_get(unit_index)->unit.looking_vector;

	return;
}

boolean unit_flying_through_air(
	long unit_index)
{
	struct unit_datum *unit = unit_get(unit_index);
	boolean flying_through_air = FALSE;

	if (unit->object.type==_object_type_biped)
	{
		flying_through_air = biped_flying_through_air(unit_index);
	}

	return flying_through_air;
}

short vehicle_scripting_find_available_seats(
	long vehicle_index,
	char const *seat_substring_name,
	short seat_desire_type,
	short *seat_indices,
	short maximum_seat_count)
{
	struct unit_datum *vehicle = unit_get(vehicle_index);
	struct unit_definition *vehicle_definition = unit_definition_get(vehicle->definition_index);
	boolean match_all_seats;
	short available_seat_count;
	short seat_index;

	match_assert("c:\\halo\\SOURCE\\units\\units.c", 6031, seat_substring_name);
	match_assert(
		"c:\\halo\\SOURCE\\units\\units.c",
		6032,
		(seat_desire_type == NONE) || ((seat_desire_type >= 0) && (seat_desire_type < NUMBER_OF_VEHICLE_SEAT_DESIRE_TYPES)));

	match_all_seats = !seat_substring_name || csstrlen(seat_substring_name)==0;
	available_seat_count = 0;

	for (seat_index = 0; seat_index<vehicle_definition->unit.seats.count; ++seat_index)
	{
		struct unit_seat *seat = TAG_BLOCK_GET_ELEMENT(
			&vehicle_definition->unit.seats,
			seat_index,
			struct unit_seat);
		boolean seat_matches_desire = TRUE;
		char lower_seat_name[256];

		if (available_seat_count>=maximum_seat_count)
		{
			break;
		}

		csstrcpy(lower_seat_name, seat->label);
		strlwr(lower_seat_name);

		if (!match_all_seats && !strstr(lower_seat_name, seat_substring_name))
		{
			continue;
		}

		switch (seat_desire_type)
		{
		case _vehicle_seat_desire_not_driver:
			seat_matches_desire = !TEST_FLAG(seat->flags, _unit_seat_driver_bit);
			break;

		case _vehicle_seat_desire_gunner:
			seat_matches_desire = TEST_FLAG(seat->flags, _unit_seat_gunner_bit);
			break;

		case _vehicle_seat_desire_passenger:
			seat_matches_desire =
				!TEST_FLAG(seat->flags, _unit_seat_driver_bit) &&
				!TEST_FLAG(seat->flags, _unit_seat_gunner_bit);
			break;

		case _vehicle_seat_desire_driver:
			seat_matches_desire = TEST_FLAG(seat->flags, _unit_seat_driver_bit);
			break;

		default:
			break;
		}

		if (seat_matches_desire && !unit_seat_filled(vehicle_index, seat_index))
		{
			seat_indices[available_seat_count++] = seat_index;
		}
	}

	return available_seat_count;
}

void unit_get_head_position(
	long unit_index,
	union real_point3d *head_position)
{
	struct object_marker head_marker;

	object_get_marker_by_name(unit_index, "head", &head_marker, 1);
	*head_position = head_marker.matrix.position;

	return;
}

void unit_get_camera_position(
	long unit_index,
	real_point3d *camera_position)
{
	struct object_marker marker;

	struct unit_datum *unit = unit_get(unit_index);
	struct unit_definition *unit_definition = unit_definition_get(unit->definition_index);

	if (unit->object.parent_object_index!=NONE || TEST_FLAG(unit->object.damage_flags, _object_dead_bit) || unit->object.type!=_object_type_biped)
	{
		if (unit->object.parent_object_index==NONE)
		{
			if (unit->unit.gunner_object_index==NONE)
			{
				object_get_marker_by_name(unit_index, "head", &marker, 1);
				*camera_position = marker.matrix.position;
			}
			else
			{
				struct unit_datum *gunner_unit = unit_get(unit->unit.gunner_object_index);
				struct unit_seat *parent_seat = TAG_BLOCK_GET_ELEMENT(&unit_definition->unit.seats, gunner_unit->unit.parent_seat_index, struct unit_seat);

				object_get_marker_by_name(unit_index, parent_seat->marker_name, &marker, 1);
				*camera_position = marker.matrix.position;
			}
		}
		else
		{
			struct object_datum *object = object_get(unit->object.parent_object_index);

			*camera_position = object->object.position;

			if (TEST_FLAG(_object_mask_unit, object->object.type) && unit->unit.parent_seat_index!=NONE)
			{
				struct unit_definition *parent_unit_definition = unit_definition_get(object->definition_index);
				struct unit_seat *parent_seat = TAG_BLOCK_GET_ELEMENT(&parent_unit_definition->unit.seats, unit->unit.parent_seat_index, struct unit_seat);

				if (object->object.type!=_object_type_vehicle || parent_seat->camera.marker_name[0])
				{
					object_get_marker_by_name(unit->object.parent_object_index, parent_seat->camera.marker_name, &marker, 1);
					*camera_position = marker.matrix.position;
				}
			}
		}
	}
	else
	{
		biped_get_sight_position(unit_index, 0.f, NULL, NULL, NULL, camera_position);
	}

	return;
}

void unit_estimate_position(
	long unit_index,
	short estimate_mode,
	real_point3d const *body_position,
	real_vector3d *desired_facing,
	real_vector3d *desired_gun_offset,
	real_point3d *estimated_position)
{
	struct unit_datum *unit;
	real_point3d reference_position;

	unit = unit_get(unit_index);

	match_assert(
		"c:\\halo\\SOURCE\\units\\units.c",
		5297,
		body_position && estimated_position);
	match_assert(
		"c:\\halo\\SOURCE\\units\\units.c",
		5298,
		(estimate_mode >= 0) &&
		(estimate_mode < NUMBER_OF_UNIT_ESTIMATE_POSITION_MODES));

	if (unit->object.parent_object_index == NONE &&
		!TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
	{
		if (unit->object.type == _object_type_biped)
		{
			biped_get_sight_position(
				unit_index,
				estimate_mode,
				body_position,
				desired_facing,
				desired_gun_offset,
				estimated_position);

			return;
		}
	}
	else if (unit->object.type == _object_type_biped &&
		unit->object.parent_object_index != NONE)
	{
		struct object_datum *parent_object =
			object_get(unit->object.parent_object_index);

		if (parent_object->object.type == _object_type_vehicle &&
			vehicle_find_pathfinding_surface_index(
				unit->object.parent_object_index,
				&reference_position) != NONE)
		{
			goto apply_delta;
		}
	}

	object_get_origin(unit_index, &reference_position);

apply_delta:
	unit_get_camera_position(unit_index, estimated_position);
	add_vectors3d(
		vector_from_points3d(
			&reference_position,
			body_position,
			(real_vector3d *)&reference_position),
		(real_vector3d *)estimated_position,
		(real_vector3d *)estimated_position);

	return;
}

boolean unit_clip_to_aiming_bounds(
	long unit_index,
	real_vector3d *vector,
	boolean use_aiming_screen)
{

	boolean aiming;
	real_rectangle2d *bounds;

	struct unit_datum *unit = unit_get(unit_index);
	boolean result = FALSE;

	if (use_aiming_screen)
	{
		aiming = unit->unit.animation.aiming_with_euler_screen;
		bounds = &unit->unit.animation.aiming_screen_bounds;
	}
	else
	{
		aiming = unit->unit.animation.looking_with_euler_screen;
		bounds = &unit->unit.animation.looking_screen_bounds;
	}

	match_assert_valid_real_normal3d("c:\\halo\\SOURCE\\units\\units.c", 5608, vector);

	if (aiming)
	{
		real_matrix4x3 matrix;
		real_euler_angles2d relative_aiming_angles;
		real_vector3d relative_vector;

		matrix.scale = 1.f;
		object_get_orientation(unit_index, &matrix.forward, &matrix.up);
		cross_product3d(&matrix.up, &matrix.forward, &matrix.left);
		matrix.position = *global_origin3d;
		matrix4x3_inverse_transform_normal(&matrix, vector, &relative_vector);

		match_assert_valid_real_vector3d("c:\\halo\\SOURCE\\units\\units.c", 5626, &relative_vector);

		euler_angles2d_from_vector3d(&relative_aiming_angles, &relative_vector);

		match_assert_valid_real("c:\\halo\\SOURCE\\units\\units.c", 5629, relative_aiming_angles.pitch);
		match_assert_valid_real("c:\\halo\\SOURCE\\units\\units.c", 5630, relative_aiming_angles.yaw);

		if (relative_aiming_angles.yaw<bounds->x0)
		{
			relative_aiming_angles.yaw = bounds->x0;
			result = TRUE;
		}
		else
		{
			if (relative_aiming_angles.yaw>bounds->x1)
			{
				relative_aiming_angles.yaw = bounds->x1;
				result = TRUE;
			}
		}

		if (relative_aiming_angles.pitch<bounds->y0)
		{
			result = TRUE;
			relative_aiming_angles.pitch = bounds->y0;
		}
		else
		{
			if (relative_aiming_angles.pitch>bounds->y1)
			{
				result = TRUE;
				relative_aiming_angles.pitch = bounds->y1;
			}
		}

		if (result)
		{
			match_assert_valid_real("c:\\halo\\SOURCE\\units\\units.c", 5661, relative_aiming_angles.pitch);
			match_assert_valid_real("c:\\halo\\SOURCE\\units\\units.c", 5662, relative_aiming_angles.yaw);
			vector3d_from_euler_angles2d(&relative_vector, &relative_aiming_angles);

			match_assert_valid_real_vector3d("c:\\halo\\SOURCE\\units\\units.c", 5665, &relative_vector);
			matrix4x3_transform_normal(&matrix, &relative_vector, vector);
			match_assert_valid_real_vector3d("c:\\halo\\SOURCE\\units\\units.c", 5667, vector);
		}
	}

	return result;
}

long unit_inventory_get_weapon(
	long unit_index, 
	short index)
{
	struct unit_datum *unit = unit_get(unit_index);

	return unit_get_weapon(unit, index);
}

static boolean unit_animation_busy(
	struct unit_animation *animation)
{
	boolean result = FALSE;

	switch (animation->state)
	{
	case _unit_state_hard_ping:
	case _unit_state_dying_airborne:
	case _unit_state_dying:
	case _unit_state_entering_seat:
	case _unit_state_exiting_seat:
	case _unit_state_ai_impulse:
	case _unit_state_melee_attack:
	case _unit_state_melee_airborne:
	case _unit_state_melee_continuous:
	case _unit_state_throw_grenade:
	case _unit_state_resurrect_front:
	case _unit_state_resurrect_back:
	case _unit_state_leap_start:
	case _unit_state_leap_melee:
		result = TRUE;
		break;
	default:
		break;
	}

	return result;
}

static boolean unit_set_or_test_seat_and_weapon_label(
	long object_index,
	char const *seat_label,
	char const *weapon_label,
	boolean change_flag)
{
	short seat_index;

	struct unit_datum *unit = unit_get(object_index);
	struct unit_definition *unit_definition = unit_definition_get(unit->definition_index);
	struct animation_graph *animation_graph = animation_graph_definition_get(unit_definition->object.animation_graph.index);
	boolean result = FALSE;

	for (seat_index = 0; seat_index<animation_graph->unit_seats.count; ++seat_index)
	{
		struct animation_graph_unit_seat *unit_seat = TAG_BLOCK_GET_ELEMENT(&animation_graph->unit_seats, seat_index, struct animation_graph_unit_seat);

		if (!seat_label || !_stricmp(seat_label, unit_seat->label))
		{
			short weapon_class_index;

			for (weapon_class_index = 0; weapon_class_index<unit_seat->weapon_classes.count; ++weapon_class_index)
			{
				short weapon_type_index;

				struct animation_graph_weapon_class *weapon_class = TAG_BLOCK_GET_ELEMENT(&unit_seat->weapon_classes, weapon_class_index, struct animation_graph_weapon_class);

				for (weapon_type_index = 0; weapon_type_index<weapon_class->weapon_types.count; ++weapon_type_index)
				{
					struct animation_graph_weapon_type *weapon_type = TAG_BLOCK_GET_ELEMENT(&weapon_class->weapon_types, weapon_type_index, struct animation_graph_weapon_type);
					
					if (!weapon_label ||
						!strcmp(weapon_label, "unarmed") &&
						weapon_type->label[0]=='\0'||
						!_stricmp(weapon_label, weapon_type->label))
					{
						if (change_flag)
						{
							long anim_2 =
								unit_seat->animations.count <= 2 ?
								NONE :
								animation_graph_animation_index_get(&unit_seat->animations)[2].animation_index;
							boolean showing_acceleration;
							
							if (anim_2==NONE)
							{
								long anim_3 =
									unit_seat->animations.count <= 3 ?
									NONE :
									animation_graph_animation_index_get(&unit_seat->animations)[3].animation_index;
								
								if (anim_3==NONE)
								{
									long anim_4 =
										unit_seat->animations.count <= 4 ?
										NONE :
										animation_graph_animation_index_get(&unit_seat->animations)[4].animation_index;

									if (anim_4==NONE)
									{
										showing_acceleration = FALSE;
										goto acceleration_determined;
									}
								}
							}
							showing_acceleration = TRUE;
							acceleration_determined:

							if (unit->unit.animation.state!=_unit_state_user_animation)
							{
								unit->unit.animation.state = NONE;
							}

							unit->unit.animation.seat_index = seat_index;
							unit->unit.animation.base_seat_index = seat_label_to_base_seat_index(seat_label);
							unit->unit.animation.weapon_index = weapon_class_index;
							unit->unit.animation.weapon_type_index = weapon_type_index;

							SET_FLAG(unit->unit.animation.flags, _unit_animation_showing_acceleration_bit, showing_acceleration);
						}

						result = TRUE;
						break;
					}
				}
			}
		}
	}

	return result;
}

static long unit_get_weapon(
	struct unit_datum *unit,
	short index)
{
	long result = NONE;

	if (index!=NONE)
	{
		match_assert("c:\\halo\\SOURCE\\units\\units.c", 8371, index>=0 && index<MAXIMUM_WEAPONS_PER_UNIT);
		result = unit->unit.weapon_object_indices[index];
	}

	return result;
}

static short seat_label_to_base_seat_index(
	char const *seat_label)
{
	short seat_index;
	short result = NONE;

	for (seat_index = 0; seat_index<NUMBER_OF_UNIT_BASE_SEATS; ++seat_index)
	{
		if (!_stricmp(seat_label, base_seat_labels[seat_index]))
		{
			result = seat_index;
			break;
		}
	}

	return result;
}

} // namespace h1_ai

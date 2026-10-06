#include "stdafx.h"
#include "h1_ai_internal.h"

/*
* bipeds.c: the functions of it the AI calls.
*/

namespace h1_ai
{

/* ---------- units/bipeds.c: its declarations */

/* ---------- headers */

#define REAL_MATH_EXTERNAL_POINT_FROM_LINE3D
#define REAL_MATH_EXTERNAL_REAL_RANDOM_RANGE

/* port: an unarmed player's melee's length, in ticks (a weapon's is about
this: its first person melee animation, sped up a quarter) */
#define UNARMED_MELEE_TICKS 16

/* ---------- constants */

enum
{
	_material_effect_biped_walk = 0,
	_material_effect_biped_run,
	_material_effect_biped_sliding,
	_material_effect_biped_shuffle,
	_material_effect_biped_jump,
	_material_effect_biped_jump_land,
};

enum
{
	_unit_animation_action_melee = 7,
};

enum
{
	biped_state_idle = 0,
	biped_state_moving,
	biped_state_unknown,
	NUMBER_OF_BIPED_STATES,
};

enum
{
	_biped_physics_in_airborne_bit = 0,
	_biped_physics_in_slipping_bit,
	_biped_physics_in_crouched_bit,
	_biped_physics_in_trying_to_stand_bit,
	_biped_physics_in_flying_bit,
	_biped_physics_in_absolute_movement_bit,
	_biped_physics_in_no_collision_bit,
	_biped_physics_in_dead_bit,
	_biped_physics_in_pass_through_bipeds_bit,
	_biped_physics_in_climb_anything_bit,
};

enum
{
	_biped_physics_out_airborne_bit = 0,
	_biped_physics_out_slipping_bit,
	_biped_physics_out_cannot_stand_bit,
	_biped_physics_out_splatter_bit,
	_biped_physics_out_volatile_collision_bit,
};

enum
{
	_unit_melee_attack_none = 0,
	_unit_melee_attack_starting,
	_unit_melee_attack_dangerous,
	_unit_melee_attack_impact,
	_unit_melee_attack_continuous,
};

enum
{
	_animation_frame_info_none = 0,
	_animation_frame_info_xy_translation,
	_animation_frame_info_xy_translation_yaw_rotation,
	_animation_frame_info_xyz_translation_yaw_rotation,
	NUMBER_OF_ANIMATION_FRAME_INFO_TYPES,
};

enum
{
	_collision_surface_two_sided_bit = 0,
	_collision_surface_invisible_bit,
	_collision_surface_climbable_bit,
	_collision_surface_breakable_bit,
};

/* ---------- macros */

#define BIPED_CLIMBING_SNAP_ANGLE ((real)(10.0*M_PI/180.0))
#define MINIMUM_SLIPPING_FOOTSTEP_VELOCITY_SQUARED (1.f/900.f)

/* ---------- structures */

struct biped_contact_point
{
	byte unused[32];
	char marker_name[32];
};

struct game_globals_falling_damage
{
	long falling_unused[2];
	real falling_distance_lower_bound;
	real falling_distance_upper_bound;
	struct tag_reference falling_damage;
	long terminal_velocity_unused[2];
	real maximum_distance;
	struct tag_reference maximum_distance_damage;
	struct tag_reference vehicle_hit_environment_damage_effect;
	struct tag_reference vehicle_killed_unit_damage_effect;
	struct tag_reference vehicle_collision_damage;
	struct tag_reference flaming_death_damage;
	long unused7c[4];
	real runtime_maximum_falling_velocity;
	real runtime_minimum_damage_velocity;
	real runtime_maximum_damage_velocity;
};

struct biped_physics
{
	long biped_index;
	word in_flags;
	word pad6;
	real_point3d position;
	real_vector3d forward;
	real_vector3d aiming;
	real_vector3d velocity;
	real crouch_velocity;
	real_vector3d movement_desired;
	real movement_penalty;
	real acceleration_maximum;
	real airborne_acceleration_maximum;
	real height;
	real width;
	real ground_tangential_velocity_max;
	real ground_tangential_angle;
	real minimum_normal_k;
	real downhill_k0;
	real downhill_k1;
	real downhill_velocity_scale;
	real uphill_k0;
	real uphill_k1;
	real uphill_velocity_scale;
	real_plane3d ground_plane;
	long existing_support_surface_index;
	real gravity;
	long bumped_object_index;
	long elevator_object_index;
	word out_flags;
	word padA2;
	long support_surface_index;
	long stick_surface_index;
	real_point3d new_position;
	real_vector3d new_velocity;
	real landing_velocity;
	real collision_velocity;
};

struct animation_frame_info_dx_dy
{
	real dx;
	real dy;
};

struct animation_frame_info_dx_dy_dyaw
{
	real dx;
	real dy;
	real dyaw;
};

struct animation_frame_info_dx_dy_dz_dyaw
{
	real dx;
	real dy;
	real dz;
	real dyaw;
};

struct vehicle_runtime_datum
{
	long definition_index;
	struct _object_datum object;
	struct _unit_datum unit;
	struct
	{
		word flags;
		short reserved;
		byte airborne_ticks;
	} vehicle;
};

struct scenario_object_datum
{
	short palette_entry_index;
	short name_index;
	word placement_flags;
	short variant_number;
	real_point3d position;
	real_euler_angles3d rotation;
	word on_bsp_flags;
	word misc_flags;
	unsigned long unused;
};

struct scenario_object_permutation
{
	unsigned long change_colors[4];
	byte region_permutations[8];
	unsigned long unused[2];
};

struct scenario_unit_datum
{
	real body_vitality;
	unsigned long flags;
};

struct scenario_biped_datum
{
	struct scenario_object_datum object;
	struct scenario_object_permutation permutation;
	struct scenario_unit_datum unit;
};

/* ---------- prototypes */

static void biped_make_footstep(
	long biped_index,
	short event_index,
	short contact_point_index);
static void biped_bumped_object(
	long biped_index,
	long object_index,
	real_vector3d const *old_velocity);
static void biped_falling_damage(
	long biped_index,
	real collision_velocity);
static void biped_start_landing(
	long biped_index,
	real landing_velocity);
static void biped_update_jumping(
	long biped_index,
	struct unit_animation_update_data *animation);
static void biped_update_physics(
	struct biped_physics *physics);
static void biped_snap_facing(
	long biped_index);

/* ---------- globals */

boolean debug_biped_physics = FALSE;
boolean debug_biped_skip_update = FALSE;
boolean debug_biped_skip_collision = FALSE;
boolean debug_biped_limp_body_disable = FALSE;
boolean rider_ejection = TRUE;

static struct profile_section biped_update_section = {"biped_update", NONE, TRUE};

extern boolean debug_objects_biped_autoaim_pills;
extern boolean debug_objects_biped_physics_pills;

static real_vector3d const fudge_vectors[27] =
{
	{ { 0.f, 0.f, 0.f } },
	{ { 1.f, 0.f, 0.f } },
	{ { 0.f, 0.f, 1.f } },
	{ { 0.70710677f, 0.f, 0.70710677f } },
	{ { 0.57735026f, 0.57735026f, 0.57735026f } },
	{ { 0.57735026f, -0.57735026f, 0.57735026f } },
	{ { 0.70710677f, 0.70710677f, 0.f } },
	{ { 0.70710677f, -0.70710677f, 0.f } },
	{ { 0.f, 0.70710677f, 0.70710677f } },
	{ { 0.f, -0.70710677f, 0.70710677f } },
	{ { -1.f, 0.f, 0.f } },
	{ { 0.f, 1.f, 0.f } },
	{ { 0.f, -1.f, 0.f } },
	{ { -0.70710677f, -0.70710677f, 0.f } },
	{ { -0.70710677f, 0.70710677f, 0.f } },
	{ { -0.70710677f, 0.f, 0.70710677f } },
	{ { -0.57735026f, -0.57735026f, 0.57735026f } },
	{ { -0.57735026f, 0.57735026f, 0.57735026f } },
	{ { 0.f, 0.f, -1.f } },
	{ { 0.70710677f, 0.f, -0.70710677f } },
	{ { -0.70710677f, 0.f, -0.70710677f } },
	{ { 0.f, 0.70710677f, -0.70710677f } },
	{ { 0.f, -0.70710677f, -0.70710677f } },
	{ { 0.57735026f, 0.57735026f, -0.57735026f } },
	{ { 0.57735026f, -0.57735026f, -0.57735026f } },
	{ { -0.57735026f, -0.57735026f, -0.57735026f } },
	{ { -0.57735026f, 0.57735026f, -0.57735026f } },
};

/* ---------- public code */

/* ---------- private code */

/* ---------- units/bipeds.c: its functions */

void biped_get_physics_pill(
	long biped_index,
	real_point3d *base,
	real *height,
	real *width)
{
	struct biped_datum *biped = biped_get(biped_index);
	struct biped_definition *definition = biped_definition_get(biped->definition_index);

	object_get_origin(biped_index, base);
	if (!TEST_FLAG(definition->biped.flags, _biped_pill_centered_at_origin_bit))
		base->z += definition->biped.collision_radius;

	if (!TEST_FLAG(definition->biped.flags, _biped_spherical_bit) &&
		(biped->unit.player_index!=NONE ||
		TEST_FLAG(biped->object.flags, _object_movie_star_bit)))
	{
		*height = definition->biped.collision_height_standing +
			(definition->biped.collision_height_crouching -
			definition->biped.collision_height_standing)*biped->biped.crouch -
			2.f*definition->biped.collision_radius;
	}
	else
	{
		*height = 0.f;
	}
	*width = definition->biped.collision_radius;

	return;
}

void biped_build_flying_axes(
	real_vector3d const *forward_vector,
	real_vector3d *left_vector,
	real_vector3d *up_vector)
{
	match_assert("c:\\halo\\SOURCE\\units\\bipeds.c", 2993, forward_vector && left_vector && up_vector);

	*up_vector = *global_up3d;
	cross_product3d(up_vector, forward_vector, left_vector);
	if (normalize3d(left_vector)==0.f)
	{
		*up_vector = *global_forward3d;
		cross_product3d(up_vector, forward_vector, left_vector);
		normalize3d(left_vector);
	}

	cross_product3d(forward_vector, left_vector, up_vector);
	normalize3d(up_vector);

	match_vassert("c:\\halo\\SOURCE\\units\\bipeds.c", 3008,
		valid_real_vector3d_axes3(forward_vector, left_vector, up_vector),
		csprintf(
			temporary,
			"%s, %s, %s: assert_valid_real_vector3d_axes3(%f, %f, %f / %f, %f, %f / %f, %f, %f)",
			"forward_vector",
			"left_vector",
			"up_vector",
			forward_vector->i, forward_vector->j, forward_vector->k,
			up_vector->i, up_vector->j, up_vector->k,
			left_vector->i, left_vector->j, left_vector->k));

	return;
}

static long biped_find_ground_surface(
	long object_index,
	real_vector3d const *direction,
	real distance,
	real_point3d *point,
	real_vector3d *normal)
{
	struct biped_datum *biped = biped_get(object_index);
	struct collision_bsp *collision_bsp = global_collision_bsp_get();
	long surface_index = NONE;
	real_vector3d vector;
	real_point3d origin;
	struct collision_bsp_test_vector_result result;

	match_assert("c:\\halo\\SOURCE\\units\\bipeds.c", 1146, global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
	global_current_collision_users[global_current_collision_user_depth++] = _collision_user_bipeds;

	object_get_origin(object_index, &origin);
	/* Preserve January's inline schedule without owning point_from_line3d here. */
	origin.x = global_up3d->i*0.4f + origin.x;
	origin.y = global_up3d->j*0.4f + origin.y;
	origin.z = global_up3d->k*0.4f + origin.z;
	scale_vector3d(direction, distance, &vector);

	if (collision_bsp_test_vector(
		FLAG(_collision_test_front_facing_surfaces_bit),
		collision_bsp,
		0,
		NULL,
		&origin,
		&vector,
		REAL_MAX,
		&result))
	{
		surface_index = result.surface_index;
		if (point)
		{
			real_point3d const *line_point = &origin;
			real_vector3d const *line_vector = &vector;
			real line_t = result.t;

			point->x = line_vector->i*line_t + line_point->x;
			point->y = line_vector->j*line_t + line_point->y;
			point->z = line_vector->k*line_t + line_point->z;
		}
		if (normal)
			*normal = result.plane->n;
	}

	match_assert("c:\\halo\\SOURCE\\units\\bipeds.c", 1168, global_current_collision_user_depth > 1);
	--global_current_collision_user_depth;

	return surface_index;
}

long biped_approximate_surface_index(
	long biped_index,
	real_point3d *point)
{
	return biped_find_ground_surface(biped_index, global_down3d, 2.f, point, NULL);
}

long biped_find_pathfinding_surface_index(
	long biped_index,
	real_point3d *pathfinding_point)
{
	struct biped_datum *biped = biped_get(biped_index);
	struct biped_definition *definition = biped_definition_get(biped->definition_index);

	if (TEST_FLAG(definition->biped.flags, _biped_flying_bit) &&
		!TEST_FLAG(biped->object.damage_flags, _object_dead_bit))
	{
		biped->biped.pathfinding_surface_index = NONE;
		object_get_origin(biped_index, pathfinding_point);
	}
	else if (biped->biped.pathfinding_surface_index==NONE)
	{
		long time = game_time_get();

		if (time>biped->biped.last_pathfinding_attempt_time)
		{
			struct collision_bsp *collision_bsp = global_collision_bsp_get();
			real_point3d point = biped->biped.pathfinding_point;

			biped->biped.last_pathfinding_attempt_time = time;
			if (biped->biped.support_surface_index!=NONE)
			{
				real_point2d closest_point;

				collision_surface_find_closest_point2d(
					collision_bsp,
					biped->biped.support_surface_index,
					_z,
					TRUE,
					(real_point2d const *)&biped->biped.pathfinding_point,
					&closest_point);
				collision_surface_project_point2d(
					collision_bsp,
					biped->biped.support_surface_index,
					_z,
					TRUE,
					&closest_point,
					&point);
				biped->biped.pathfinding_surface_index = biped->biped.support_surface_index;
			}
			else if (biped->biped.last_pathfinding_surface_index!=NONE &&
				collision_surface_test_point2d(
					collision_bsp,
					biped->biped.last_pathfinding_surface_index,
					_z,
					TRUE,
					(real_point2d const *)&biped->biped.pathfinding_point))
			{
				biped->biped.pathfinding_surface_index = biped->biped.last_pathfinding_surface_index;
				collision_surface_project_point2d(
					collision_bsp,
					biped->biped.last_pathfinding_surface_index,
					_z,
					TRUE,
					(real_point2d const *)&biped->biped.pathfinding_point,
					&point);
				biped->biped.pathfinding_surface_index = biped->biped.last_pathfinding_surface_index;
			}

			if (biped->biped.pathfinding_surface_index==NONE)
				biped->biped.pathfinding_surface_index = biped_find_ground_surface(biped_index, global_down3d, 2.f, &point, NULL);

			if (biped->biped.pathfinding_surface_index!=NONE)
			{
				biped->biped.pathfinding_point = point;
				biped->biped.last_pathfinding_surface_index = biped->biped.pathfinding_surface_index;
			}
		}
	}

	match_assert("c:\\halo\\SOURCE\\units\\bipeds.c", 1255, pathfinding_point);
	*pathfinding_point = biped->biped.pathfinding_point;

	return biped->biped.pathfinding_surface_index;
}

boolean biped_flying_through_air(
	long biped_index)
{
	struct biped_datum *biped;
	struct biped_definition *definition;

	biped = biped_get(biped_index);
	definition = biped_definition_get(biped->definition_index);

	return biped->biped.airborne_ticks>3 &&
		(!TEST_FLAG(definition->biped.flags, _biped_flying_bit) ||
		TEST_FLAG(biped->object.damage_flags, _object_dead_bit));
}

void biped_get_sight_position(
	long biped_index,
	short estimate_mode,
	real_point3d const *estimated_body_position,
	real_vector3d *desired_facing,
	real_vector3d const *desired_gun_offset,
	real_point3d *sight_position)
{
	struct biped_datum *biped = biped_get(biped_index);
	struct biped_definition *definition = biped_definition_get(biped->definition_index);

	match_assert("c:\\halo\\SOURCE\\units\\bipeds.c", 759,
		(estimate_mode == _unit_estimate_none) ||
		(estimated_body_position != NULL));
	match_assert("c:\\halo\\SOURCE\\units\\bipeds.c", 760,
		(estimate_mode != _unit_estimate_gun_position) ||
		(desired_facing != NULL));

	if (estimate_mode==_unit_estimate_none)
		object_get_origin(biped_index, sight_position);
	else
		*sight_position = *estimated_body_position;

	if (estimate_mode==_unit_estimate_gun_position)
	{
		real_vector3d left;

		match_assert("c:\\halo\\SOURCE\\units\\bipeds.c", 775, desired_facing && desired_gun_offset);

		left.i = -desired_facing->j;
		left.j = desired_facing->i;
		left.k = 0.f;

		{
			real forward_distance = desired_gun_offset->i;

			sight_position->x += desired_facing->i*forward_distance;
			sight_position->y += desired_facing->j*forward_distance;
			sight_position->z += desired_facing->k*forward_distance;
			{
				real sideways_distance = desired_gun_offset->j;

				sight_position->x += left.i*sideways_distance;
				sight_position->y += left.j*sideways_distance;
				sight_position->z += left.k*sideways_distance;
			}
		}
		sight_position->z += desired_gun_offset->k;
	}
	else
	{
		real crouch;

		switch (estimate_mode)
		{
		case _unit_estimate_head_standing:
			crouch = 0.f;
			break;

		case _unit_estimate_head_crouching:
			crouch = 1.f;
			break;

		default:
			crouch = biped->biped.crouch;
			break;
		}

		sight_position->z += (1.f-crouch)*definition->biped.standing_camera_height +
			crouch*definition->biped.crouching_camera_height;
	}

	return;
}

} // namespace h1_ai

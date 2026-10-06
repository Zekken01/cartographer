#include "stdafx.h"
#include "h1_ai_internal.h"

/*
* vehicles.c: the functions of it the AI calls.
*/

namespace h1_ai
{

/* ---------- units/vehicles.c: its declarations */

/* ---------- headers */

/* ---------- constants */

enum
{
	_vehicle_type_human_tank = 0,
	_vehicle_type_human_jeep,
	_vehicle_type_human_boat,
	_vehicle_type_human_plane,
	_vehicle_type_alien_scout,
	_vehicle_type_alien_fighter,
	_vehicle_type_turret,
	NUMBER_OF_VEHICLE_TYPES
};

/* ---------- macros */

/* 0.8 degrees in radians. Spelled as a float literal because MSVC folds
constant expressions in double and only rounds at the final assignment, so
((real)(0.8*_pi/180)) yields 0x3c64c388 where January has 0x3c64c389 -- and
2.0*it then differs in the double constant too. */
#define VEHICLE_ANGULAR_ACCELERATION 0.0139626344f

/* ---------- structures */

struct vehicle_definition
{
	struct unit_definition unit;
	unsigned long flags;
	short vehicle_type;
	short pad2f6;
	/* 0x2f8 is a four-real 'speed' block and 0x308 a two-real 'turn' block;
	their sub-field names are not recovered, so they keep offset names. */
	real unknown2f8;
	real unknown2fc;
	real unknown300;
	real unknown304;
	real unknown308;
	real unknown30c;
	real wheel_circumference;
	real unknown314;
	real unknown318;
	short function_modes[4];
	byte unknown324[0xc];
	real unknown330;
	real unknown334;
	byte unused338[8];
	real unknown340;
	real unknown344;
	byte unused348[0x1c];
	real unknown364;
	byte unknown368[0x48];
	struct tag_reference suspension_sound;
	struct tag_reference crash_sound;
	struct tag_reference material_effects;
	struct tag_reference effect;
};

struct game_globals_falling_damage
{
	byte unused0[0x2c];
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

struct physics_mass_point_definition
{
	byte unused0[0x20];
	short powered_mass_point_index;
	short pad22;
	unsigned long flags;
	byte unused28[0x10];
	real_point3d position;
	byte unused44[0xc];
	real_vector3d normal;
	byte unused5c[0xc];
	real unknown68;
	byte unused6c[0x14];
};

struct vehicle_animation
{
	struct animation_aiming_screen_bounds steering_screen_bounds;
	long unused[0x11];
	struct tag_block animations;
	struct tag_block suspensions;
};

struct vehicle_suspension
{
	short mass_point_index;
	short animation_index;
	real unknown4;
	real unknown8;
	byte unknownc[8];
};

struct scenario_object_permutation;
struct scenario_unit;

struct scenario_vehicle
{
	byte unused0[0x28];
	byte permutation[0x20];
	byte unit[0x4];
};

/* ---------- prototypes */

/* NOTE: code_001a5e50 and code_001a6290 are file statics in January, but they
are not reconstructed yet. They are declared here rather than defined so that
code_001a8800 can call them: January passes their arguments on the stack, and a
declared-but-undefined static would give MSVC a body to inspect and a register
convention to invent. The relocation carries the name either way, which is what
the comparator checks. */

extern boolean debug_objects_vehicle_powered_mass_points;
extern real global_gravity;

/* ---------- globals */

static struct profile_section vehicle_update_section = {"vehicle_update", NONE, TRUE};

/* ---------- code */

/* NOTE: the vehicle function enum is not recovered from January. These names
describe what each case computes, read off the disassembly. They are descriptive,
not authentic. */

enum
{
	_vehicle_function_none = 0,
	_vehicle_function_speed_absolute,
	_vehicle_function_speed_forward,
	_vehicle_function_speed_reverse,
	_vehicle_function_slide_absolute,
	_vehicle_function_slide_left,
	_vehicle_function_slide_right,
	_vehicle_function_speed_or_slide,
	_vehicle_function_turn_absolute,
	_vehicle_function_turn_left,
	_vehicle_function_turn_right,
	_vehicle_function_flag2,
	_vehicle_function_flag3,
	_vehicle_function_unused13,
	_vehicle_function_velocity_absolute,
	_vehicle_function_velocity_moving,
	_vehicle_function_velocity_sliding,
	_vehicle_function_velocity_forward,
	_vehicle_function_velocity_up,
	_vehicle_function_velocity_up_alternate,
	_vehicle_function_left_tread_position,
	_vehicle_function_right_tread_position,
	_vehicle_function_speed_minus_turn,
	_vehicle_function_speed_plus_turn,
	_vehicle_function_wheel_position_a,
	_vehicle_function_wheel_position_b,
	_vehicle_function_wheel_position_c,
	_vehicle_function_wheel_position_d,
	_vehicle_function_speed_absolute_a,
	_vehicle_function_speed_absolute_b,
	_vehicle_function_speed_absolute_c,
	_vehicle_function_speed_absolute_d,
	_vehicle_function_sideslip,
	_vehicle_function_unknown444,
	_vehicle_function_unknown448,
	_vehicle_function_speed_blend,
	_vehicle_function_boost,
	NUMBER_OF_VEHICLE_FUNCTIONS
};

static real_vector3d *compute_acceleration(
	real_vector3d *a,
	real_vector3d *b,
	real_vector3d *result,
	real maximum,
	real minimum);

/* Full semantic reconstruction. January's vehicle_update is 2320 bytes; this
body has the same padded size and 98 relocations. The remaining residual is
instruction scheduling, branch layout, and relocation placement. Keep the
file-static call topology intact while closing it. */

/* ---------- units/vehicles.c: its functions */

boolean vehicle_causes_collision_damage(
	long vehicle_index)
{
	struct vehicle_datum *vehicle = vehicle_datum_get(vehicle_index);
	struct vehicle_definition *definition = vehicle_specific_definition_get(
		vehicle->definition_index);

	return TEST_FLAG(definition->flags, 7);
}

long vehicle_find_pathfinding_surface_index(
	long vehicle_index,
	real_point3d *position)
{
	struct vehicle_datum *vehicle = vehicle_datum_get(vehicle_index);
	struct vehicle_definition *definition = vehicle_specific_definition_get(
		vehicle->definition_index);
	long surface_index = NONE;

	object_get_origin(vehicle_index, position);

	switch (definition->vehicle_type)
	{
		case _vehicle_type_human_tank:
		case _vehicle_type_human_jeep:
		case _vehicle_type_alien_scout:
		case _vehicle_type_turret:
		{
			struct collision_bsp *bsp = global_collision_bsp_get();
			struct collision_bsp_test_vector_result result;
			real_point3d origin;
			real_vector3d vector;

			object_get_origin(vehicle_index, &origin);

			point_from_line3d(&origin, global_up3d, 0.4f, &origin);

			add_vectors3d(global_down3d, global_down3d, &vector);

			if (collision_bsp_test_vector(1, bsp, 0, NULL, &origin, &vector, FLT_MAX, &result))
			{
				surface_index = result.surface_index;

				point_from_line3d(&origin, &vector, result.t, position);
			}
			break;
		}
	}

	return surface_index;
}

boolean vehicle_stuck(
	long vehicle_index,
	real_vector3d *direction)
{
	struct vehicle_datum *vehicle = vehicle_datum_get(vehicle_index);
	boolean stuck = FALSE;

	if (vehicle->vehicle.stuck_mass_point_flags)
	{
		struct physics_instance instance;

		if (physics_instance_new(&instance, vehicle_index))
		{
			real_point3d center = *global_origin3d;
			short mass_point_count = 0;
			short mass_point_index;

			for (mass_point_index = 0;
				mass_point_index<instance.physics->mass_points.count;
				mass_point_index++)
			{
				if (TEST_FLAG(vehicle->vehicle.stuck_mass_point_flags, mass_point_index))
				{
					struct physics_mass_point_definition *mass_point = TAG_BLOCK_GET_ELEMENT(
						&instance.physics->mass_points, mass_point_index,
						struct physics_mass_point_definition);

					add_vectors3d((real_vector3d *)&center, (real_vector3d *)&mass_point->position, (real_vector3d *)&center);
					mass_point_count++;
				}
			}

			if (mass_point_count>0)
			{
				real scale = 1.0f/mass_point_count;
				real_point3d center_in_world;
				real_point3d origin;

				center.x *= scale;
				center.y *= scale;
				center.z *= scale;

				matrix4x3_transform_point(&instance.world_matrix, &center, &center_in_world);
				object_get_origin(vehicle_index, &origin);
				subtract_vectors3d((real_vector3d *)&center_in_world, (real_vector3d *)&origin, direction);

				if (normalize3d(direction)!=0.0f)
					stuck = TRUE;
			}
		}
	}

	return stuck;
}

} // namespace h1_ai

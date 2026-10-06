#include "stdafx.h"
#include "h1_vehicle_physics.h"

#include "h1_animations.h"
#include "h1_cache_file.h"
#include "h1_log.h"
#include "h1_map_loader.h"
#include "h1_effects.h"
#include "h1_objects.h"
#include "h1_projectile_logic.h"
#include "h1_sound.h"

#include "game/game_time.h"
#include "math/matrix_math.h"
#include "math/real_math.h"
#include "objects/object_types.h"
#include "objects/objects.h"
#include "physics/collisions.h"
#include "physics/physics_constants.h"
#include "units/units.h"
#include "units/vehicles.h"
#include "h2_tag_definitions_generated.h"

#include <unordered_map>

/*
* vehicles.c vehicle_update and physics.c physics_update_new on havok: halo 1 vehicles move by halo 1's own physics, read from
* their halo 1 vehicle and physics tags. Every halo 1 tick the vehicle's speed, turn, slide and hover follow its driver's
* controls; its mass points find the ground below them (a ray of their radius) and give ground springs, friction, powered
* friction and anti gravity, and the summed force and torque change its velocities. Halo 2's havok rigid body takes those
* velocities and keeps colliding the hull with the world; halo 2's own vehicle forces have nothing to push (the built vehicle
* has no friction or anti gravity points). Halo 2 vehicles keep halo 2's physics.
*/

/* ---------- constants */

enum
{
	k_h1_maximum_mass_points = 32,
	k_h1_ticks_per_second = 30,
};

enum e_h1_vehicle_type
{
	_h1_vehicle_type_human_tank = 0,
	_h1_vehicle_type_human_jeep,
	_h1_vehicle_type_human_boat,
	_h1_vehicle_type_human_plane,
	_h1_vehicle_type_alien_scout,
	_h1_vehicle_type_alien_fighter,
	_h1_vehicle_type_turret,
};

enum
{
	_friction_type_point = 0,
	_friction_type_forward,
	_friction_type_left,
	_friction_type_up,
};

enum
{
	_point_at_rest_bit = 0,
	_point_on_ground_bit,
	_point_on_volatile_surface_bit,
	_point_in_water_bit,
	_point_antigraving_bit,
};

enum
{
	_powered_mass_point_ground_friction_bit = 0,
	_powered_mass_point_water_friction_bit,
	_powered_mass_point_air_friction_bit,
	_powered_mass_point_water_lift_bit,
	_powered_mass_point_air_lift_bit,
	_powered_mass_point_thrust_bit,
	_powered_mass_point_antigrav_bit,
};

// vehicle definition flags
enum
{
	_h1_vehicle_flag_control_opposite_speed_sets_brake_bit = 4,
};

// vehicle state flags
enum
{
	_h1_vehicle_crouching_bit = 2,
	_h1_vehicle_braking_bit = 3,
};

constexpr real32 k_h1_global_gravity = 0.0035651792f;	// world units per tick per tick

/* ---------- structures */

struct s_h1_friction
{
	real_vector3d friction;
	real_vector3d parallel;
	real_vector3d perpendicular;
};

struct s_h1_mass_point
{
	uint32 flags;
	real_point3d position;
	real_vector3d forward;
	real_vector3d up;
	real_vector3d radius;
	real_vector3d velocity, angular_velocity;
	real_vector3d velocity_relative_to_ground;
	real_plane3d ground_plane;
	real32 ground_depth;
	int16 ground_material_type;			// halo 1's material type of the ground, NONE without
	real32 normal_force_magnitude;
	real_vector3d normal_force;
	s_h1_friction ground_friction;
	s_h1_friction air_friction;
	real_vector3d powered_force;
	real_vector3d force;
	real_vector3d torque;
};

// units.c unit function modes
enum
{
	_h1_unit_function_none = 0,
	_h1_unit_function_driver_seat_power,
	_h1_unit_function_gunner_seat_power,
	_h1_unit_function_aiming_change,
	_h1_unit_function_mouth_aperture,
	_h1_unit_function_integrated_light_power,
	_h1_unit_function_can_blink,
	_h1_unit_function_shield_sapping,
};

// vehicles.c vehicle function modes
enum
{
	_h1_vehicle_function_none = 0,
	_h1_vehicle_function_speed_absolute,
	_h1_vehicle_function_speed_forward,
	_h1_vehicle_function_speed_reverse,
	_h1_vehicle_function_slide_absolute,
	_h1_vehicle_function_slide_left,
	_h1_vehicle_function_slide_right,
	_h1_vehicle_function_speed_or_slide,
	_h1_vehicle_function_turn_absolute,
	_h1_vehicle_function_turn_left,
	_h1_vehicle_function_turn_right,
	_h1_vehicle_function_flag2,
	_h1_vehicle_function_flag3,
	_h1_vehicle_function_unused13,
	_h1_vehicle_function_velocity_absolute,
	_h1_vehicle_function_velocity_moving,
	_h1_vehicle_function_velocity_sliding,
	_h1_vehicle_function_velocity_forward,
	_h1_vehicle_function_velocity_up,
	_h1_vehicle_function_velocity_up_alternate,
	_h1_vehicle_function_left_tread_position,
	_h1_vehicle_function_right_tread_position,
	_h1_vehicle_function_speed_minus_turn,
	_h1_vehicle_function_speed_plus_turn,
	_h1_vehicle_function_wheel_position_a,
	_h1_vehicle_function_wheel_position_b,
	_h1_vehicle_function_wheel_position_c,
	_h1_vehicle_function_wheel_position_d,
	_h1_vehicle_function_speed_absolute_a,
	_h1_vehicle_function_speed_absolute_b,
	_h1_vehicle_function_speed_absolute_c,
	_h1_vehicle_function_speed_absolute_d,
	_h1_vehicle_function_sideslip,
	_h1_vehicle_function_hover,
	_h1_vehicle_function_thrust,
	_h1_vehicle_function_speed_blend,
	_h1_vehicle_function_boost,
};

struct s_h1_powered_mass_point
{
	real32 ground_friction_velocity;
	real32 air_friction_velocity;
	real32 thrust_fraction;
	real32 antigrav_fraction;
	real_quaternion rotation;
	real_matrix3x3 rotation_matrix;
};

struct s_h1_vehicle_state
{
	real32 speed;
	real32 slide;
	real32 turn;
	real32 wheel;
	real32 left_tread;
	real32 right_tread;
	real32 hover;
	uint32 flags;
	uint8 airborne_ticks;
	uint8 on_ground_ticks;
	real_vector3d linear_velocity;		// halo 1's, world units per tick, at the object's origin
	real_vector3d angular_velocity;		// radians per tick
	real32 leftover_ticks;
	bool at_rest;						// _object_at_rest_bit: halo 1 skips its physics until the vehicle wakes
	uint8 suspension[8];				// each suspension animation's compression, 0 extended, 0xFF compressed
	bool on_ground;						// a mass point touched the ground in the last tick
	int8 base_animation;				// e_h1_vehicle_base_animation: idle, opening (held open) or closing (held shut)
	int16 base_frame;
	bool had_driver;
	real32 base_leftover_ticks;
	bool suspension_sounded;			// the suspension sound played since the last update
	bool commanded;						// the velocity halo 1 gave havok last update (havok collides: what it changed is the crash)
	real_vector3d commanded_velocity;
};

/* ---------- globals */

static std::unordered_map<datum, s_h1_vehicle_state> g_h1_vehicle_states;

/* ---------- prototypes */

static datum h1_vehicle_driver_get(datum vehicle_index);
static void h1_vehicle_animation_state_set(datum vehicle_index, const s_h1_vehicle_state* state);
static bool h1_vehicle_suspension_update(datum vehicle_index, const h1_vehi* h1_vehicle, const h1_phys* physics, s_h1_vehicle_state* state);
static void h1_vehicle_slipping_effects(datum vehicle_index, const h1_vehi* h1_vehicle, const h1_phys* physics, const s_h1_mass_point* mass_points);
static void h1_vehicle_ghost_effect(datum vehicle_index, const h1_vehi* h1_vehicle);
static void h1_material_effect_new(datum definition_index, int16 effect_index, int16 material_type, const real_point3d* position, const real_vector3d* normal, real32 scale);
static void h1_vehicle_tick(datum vehicle_index, const h1_vehi* h1_vehicle, const h1_phys* physics, s_h1_vehicle_state* state);
static void h1_vehicle_base_animation_update(datum vehicle_index, const h1_vehi* h1_vehicle, s_h1_vehicle_state* state);
static void h1_physics_update(datum vehicle_index, const h1_phys* physics, s_h1_vehicle_state* state, s_h1_powered_mass_point* powered_mass_points,
	const real_vector3d* magic_force, const real_vector3d* magic_torque, s_h1_mass_point* mass_points);

/* ---------- small math */

static real32 h1_pin_fraction(real32 value, real32 begin, real32 end)
{
	if (begin == end)
	{
		return value >= end ? 1.f : 0.f;
	}
	return PIN((value - begin) / (end - begin), 0.f, 1.f);
}

static void h1_vector_scale_add(real_vector3d* out, const real_vector3d* vector, real32 scale)
{
	out->i += vector->i * scale;
	out->j += vector->j * scale;
	out->k += vector->k * scale;
	return;
}

static void h1_components_from_normal(const real_vector3d* vector, const real_vector3d* normal, real_vector3d* out_parallel, real_vector3d* out_perpendicular)
{
	const real32 d = dot_product3d(vector, normal);
	*out_parallel = { normal->i * d, normal->j * d, normal->k * d };
	*out_perpendicular = { vector->i - out_parallel->i, vector->j - out_parallel->j, vector->k - out_parallel->k };
	return;
}

// physics_variables.c
struct s_h1_speed_parameters
{
	real32 positive_scale;
	real32 negative_scale;
	real32 acceleration;
	real32 deceleration;
};

static void h1_speed_update(real32* speed, const s_h1_speed_parameters* parameters, real32 delta)
{
	const real32 magnitude = fabsf(delta);
	const real32 acceleration = magnitude * parameters->acceleration;
	const real32 deceleration = magnitude * parameters->deceleration;
	if (delta > 0.f)
	{
		if (*speed <= -deceleration) *speed += deceleration;
		else if (*speed >= 0.f) *speed += acceleration;
		else *speed = (*speed / deceleration + 1.f) * acceleration;
		*speed = MIN(*speed, magnitude * parameters->positive_scale);
	}
	else if (delta < 0.f)
	{
		if (*speed >= deceleration) *speed -= deceleration;
		else if (*speed <= 0.f) *speed -= acceleration;
		else *speed = (*speed / deceleration - 1.f) * acceleration;
		*speed = MAX(-magnitude * parameters->negative_scale, *speed);
	}
	return;
}

static void h1_speed_update_seek(real32* speed, const s_h1_speed_parameters* parameters, real32 target, real32 delta)
{
	if (*speed > target)
	{
		h1_speed_update(speed, parameters, -delta);
		if (*speed <= target) *speed = target;
	}
	else if (*speed < target)
	{
		h1_speed_update(speed, parameters, delta);
		if (*speed >= target) *speed = target;
	}
	return;
}

static void h1_position_update_seek(real32* position, const real32* limits, real32 target, real32 delta)
{
	const real32 direction = target > *position ? 1.f : (target < *position ? -1.f : 0.f);
	if (direction != 0.f)
	{
		*position = PIN(*position + direction * delta, limits[1], limits[0]);
		const real32 after = target > *position ? 1.f : (target < *position ? -1.f : 0.f);
		if (after == direction)
		{
			return;
		}
	}
	*position = target;
	return;
}

/* ---------- public code */

void h1_vehicle_physics_reset(void)
{
	g_h1_vehicle_states.clear();
	return;
}

bool h1_vehicle_physics_update(datum vehicle_index)
{
	if (!h1_maps_active())
	{
		return false;
	}
	const object_datum* object = object_try_and_get(vehicle_index);
	const datum h1_definition_index = object ? h1_objects_h1_definition_get(object->definition_index) : NONE;
	const h1_vehi* h1_vehicle = h1_definition_index != NONE ? (const h1_vehi*)g_h1_cache_file->tag_get('vehi', h1_definition_index) : NULL;
	const h1_phys* physics = h1_vehicle ? (const h1_phys*)g_h1_cache_file->tag_get('phys', h1_vehicle->physics.index) : NULL;
	if (!physics || physics->mass <= 0.f || physics->mass_points.count <= 0)
	{
		return false;
	}

	// objects.c: a halo 1 object without body vitality takes no damage (object_cannot_take_damage's flag)
	const h1_coll* collision = (const h1_coll*)g_h1_cache_file->tag_get('coll', h1_vehicle->collision_model.index);
	if (collision && collision->maximum_body_vitality <= 0.f)
	{
		*(uint16*)((uint8*)object + 0x10A) |= 0x80;
	}

	auto found = g_h1_vehicle_states.find(vehicle_index);
	if (found == g_h1_vehicle_states.end())
	{
		found = g_h1_vehicle_states.emplace(vehicle_index, s_h1_vehicle_state{}).first;
	}
	s_h1_vehicle_state* state = &found->second;

	// its base animation, at halo 1's 30 ticks a second whether it's at rest or not
	state->base_leftover_ticks += game_tick_length() * k_h1_ticks_per_second;
	while (state->base_leftover_ticks >= 1.f)
	{
		state->base_leftover_ticks -= 1.f;
		h1_vehicle_base_animation_update(vehicle_index, h1_vehicle, state);
	}

	// riders ride, a vehicle on a parent doesn't move by itself
	if (object->object.parent_object_index != NONE)
	{
		state->linear_velocity = *global_zero_vector3d;
		state->angular_velocity = *global_zero_vector3d;
		h1_vehicle_animation_state_set(vehicle_index, state);
		return true;
	}

	// havok's velocities (per second, at the center of mass) as halo 1's (per tick, at the origin)
	const real_matrix4x3* body_matrix = object_get_node_matrix(vehicle_index, 0);
	real_vector3d havok_linear, havok_angular;
	object_get_velocities(vehicle_index, &havok_linear, &havok_angular);
	real_point3d center_of_mass;
	matrix4x3_transform_point(body_matrix, &physics->center_of_mass, &center_of_mass);
	real_vector3d origin_to_center;
	vector_from_points3d(&object->object.position, &center_of_mass, &origin_to_center);
	real_vector3d spin;
	cross_product3d(&havok_angular, &origin_to_center, &spin);
	state->linear_velocity = { (havok_linear.i - spin.i) / k_h1_ticks_per_second, (havok_linear.j - spin.j) / k_h1_ticks_per_second, (havok_linear.k - spin.k) / k_h1_ticks_per_second };
	state->angular_velocity = { havok_angular.i / k_h1_ticks_per_second, havok_angular.j / k_h1_ticks_per_second, havok_angular.k / k_h1_ticks_per_second };

	// create_crashing_effects: what havok's collisions took from the velocity halo 1 gave it, on the ground, without the suspension's
	// sound (halo 1 plays one or the other)
	if (state->commanded && state->on_ground && !state->suspension_sounded && h1_vehicle->crash_sound.index != NONE)
	{
		// (havok's contacts with the ground nudge the vertical velocity every step: halo 1's landings are the suspension's)
		real_vector3d delta;
		vector_from_points3d((const real_point3d*)&state->commanded_velocity, (const real_point3d*)&state->linear_velocity, &delta);
		delta.k = 0.f;
		const real32 speed = magnitude3d(&delta);
		if (speed > 0.02f)
		{
			h1_sound_impulse(h1_vehicle->crash_sound.index, &object->object.position, PIN((speed - 0.02f) * 45.454544f, 0.f, 1.f));
		}
	}
	state->suspension_sounded = false;
	state->commanded = false;

	// at rest until its controls or something moving it (a hit, a push) wake it
	if (state->at_rest)
	{
		const unit_datum* unit = (const unit_datum*)object_get_and_verify_type(vehicle_index, _object_mask_unit);
		const bool controlled = unit->unit.driver_seat_power > 0.f || h1_vehicle_driver_get(vehicle_index) != NONE ||
			unit->unit.throttle.i != 0.f || unit->unit.throttle.j != 0.f || unit->unit.throttle.k != 0.f;
		const bool moved = magnitude_squared3d(&state->linear_velocity) > 0.0011111111f || magnitude_squared3d(&state->angular_velocity) > 0.0027415568f;
		if (!controlled && !moved)
		{
			const real_vector3d zero = *global_zero_vector3d;
			real_vector3d hold = { 0.f, 0.f, physics_constants_get()->gravity * game_tick_length() };
			Memory::GetAddress<void(__cdecl*)(datum, const real_vector3d*, const real_vector3d*)>(0x135123)(vehicle_index, &hold, &zero);
			state->leftover_ticks = 0.f;
			h1_vehicle_animation_state_set(vehicle_index, state);
			return true;
		}
		state->at_rest = false;
	}

	// halo 1 ticks at 30 a second
	state->leftover_ticks += game_tick_length() * k_h1_ticks_per_second;
	while (state->leftover_ticks >= 1.f)
	{
		state->leftover_ticks -= 1.f;
		h1_vehicle_tick(vehicle_index, h1_vehicle, physics, state);
	}

	// halo 1's velocities as havok's; havok adds its gravity over the tick it steps
	real_vector3d angular = { state->angular_velocity.i * k_h1_ticks_per_second, state->angular_velocity.j * k_h1_ticks_per_second, state->angular_velocity.k * k_h1_ticks_per_second };
	cross_product3d(&angular, &origin_to_center, &spin);
	real_vector3d linear =
	{
		state->linear_velocity.i * k_h1_ticks_per_second + spin.i,
		state->linear_velocity.j * k_h1_ticks_per_second + spin.j,
		state->linear_velocity.k * k_h1_ticks_per_second + spin.k + physics_constants_get()->gravity * game_tick_length(),
	};
	Memory::GetAddress<void(__cdecl*)(datum, const real_vector3d*, const real_vector3d*)>(0x135123)(vehicle_index, &linear, &angular);
	state->commanded_velocity = state->linear_velocity;
	state->commanded = true;
	h1_vehicle_animation_state_set(vehicle_index, state);
	return true;
}

// units.c unit_export_function_values and vehicles.c vehicle_export_function_values (halo 1 vehicles only)
void h1_vehicle_functions_export(datum vehicle_index, real32* incoming)
{
	const object_datum* object = object_try_and_get(vehicle_index);
	const datum h1_definition_index = object ? h1_objects_h1_definition_get(object->definition_index) : NONE;
	const h1_vehi* definition = h1_definition_index != NONE ? (const h1_vehi*)g_h1_cache_file->tag_get('vehi', h1_definition_index) : NULL;
	const vehicle_datum* vehicle = definition ? (const vehicle_datum*)object_try_and_get_and_verify_type(vehicle_index, _object_mask_vehicle) : NULL;
	if (!vehicle)
	{
		return;
	}
	auto found = g_h1_vehicle_states.find(vehicle_index);
	const s_h1_vehicle_state empty_state = {};
	const s_h1_vehicle_state* state = found != g_h1_vehicle_states.end() ? &found->second : &empty_state;

	const int16 unit_modes[4] = { definition->a_in_2, definition->b_in_2, definition->c_in_2, definition->d_in_2 };
	for (int32 i = 0; i < 4; i++)
	{
		real32 value = 0.f;
		switch (unit_modes[i])
		{
		case _h1_unit_function_none:
			continue;
		case _h1_unit_function_driver_seat_power:
			value = vehicle->unit.driver_seat_power;
			break;
		case _h1_unit_function_gunner_seat_power:
			value = vehicle->unit.gunner_seat_power;
			break;
		case _h1_unit_function_can_blink:
			value = 1.f;
			break;
		default:
			// aiming change, mouth aperture, integrated light power and shield sapping
			break;
		}
		incoming[i] = value;
	}

	// halo 1's velocity, per tick
	real_vector3d velocity, angular_velocity;
	object_get_velocities(vehicle_index, &velocity, &angular_velocity);
	scale_vector3d(&velocity, 1.f / k_h1_ticks_per_second, &velocity);
	const real32 forward_speed = fabsf(definition->maximum_forward_speed);
	const real32 reverse_speed = fabsf(definition->maximum_reverse_speed);
	const real32 maximum_speed = MAX(forward_speed, reverse_speed);
	const real32 left_slide = fabsf(definition->maximum_left_slide);
	const real32 right_slide = fabsf(definition->maximum_right_slide);
	const real32 maximum_slide = MAX(left_slide, right_slide);
	// the tag's turns are in degrees, halo 1's turn in radians
	const real32 left_turn = fabsf(DEGREES_TO_RADIANS(definition->maximum_left_turn));
	const real32 right_turn = fabsf(DEGREES_TO_RADIANS(definition->maximum_right_turn_negative));
	const real32 maximum_turn = MAX(left_turn, right_turn);
	auto ratio = [](real32 value, real32 maximum) { return maximum > 0.f ? value / maximum : 0.f; };

	const int16 vehicle_modes[4] = { definition->a_in_3, definition->b_in_3, definition->c_in_3, definition->d_in_3 };
	for (int32 i = 0; i < 4; i++)
	{
		real32 value = 0.f;
		switch (vehicle_modes[i])
		{
		case _h1_vehicle_function_none:
			continue;
		case _h1_vehicle_function_speed_absolute:
		case _h1_vehicle_function_speed_absolute_a:
		case _h1_vehicle_function_speed_absolute_b:
		case _h1_vehicle_function_speed_absolute_c:
		case _h1_vehicle_function_speed_absolute_d:
			value = ratio(fabsf(state->speed), maximum_speed);
			break;
		case _h1_vehicle_function_speed_forward:
			value = ratio(MAX(state->speed, 0.f), forward_speed);
			break;
		case _h1_vehicle_function_speed_reverse:
			value = ratio(fabsf(MIN(state->speed, 0.f)), reverse_speed);
			break;
		case _h1_vehicle_function_slide_absolute:
			value = ratio(fabsf(state->slide), maximum_slide);
			break;
		case _h1_vehicle_function_slide_left:
			value = ratio(fabsf(state->slide), left_slide);
			break;
		case _h1_vehicle_function_slide_right:
			value = ratio(fabsf(state->slide), right_slide);
			break;
		case _h1_vehicle_function_speed_or_slide:
			value = MAX(ratio(fabsf(state->speed), maximum_speed), ratio(fabsf(state->slide), maximum_slide));
			break;
		case _h1_vehicle_function_turn_absolute:
			value = ratio(fabsf(state->turn), maximum_turn);
			break;
		case _h1_vehicle_function_turn_left:
			value = ratio(fabsf(state->turn), left_turn);
			break;
		case _h1_vehicle_function_turn_right:
			value = ratio(fabsf(state->turn), right_turn);
			break;
		case _h1_vehicle_function_flag2:
			value = TEST_BIT(state->flags, 2) ? 1.f : 0.f;
			break;
		case _h1_vehicle_function_flag3:
			value = TEST_BIT(state->flags, 3) ? 1.f : 0.f;
			break;
		case _h1_vehicle_function_velocity_absolute:
			value = ratio(magnitude3d(&velocity), maximum_speed);
			break;
		case _h1_vehicle_function_velocity_moving:
			// on the ground
			value = state->airborne_ticks == 0 ? ratio(magnitude3d(&velocity), maximum_speed) : 0.f;
			break;
		case _h1_vehicle_function_velocity_forward:
			value = ratio(fabsf(dot_product3d(&velocity, &vehicle->object.forward)), maximum_speed);
			break;
		case _h1_vehicle_function_velocity_up:
		case _h1_vehicle_function_velocity_up_alternate:
			value = ratio(fabsf(dot_product3d(&velocity, &vehicle->object.up)), maximum_speed);
			break;
		case _h1_vehicle_function_left_tread_position:
			value = ratio(state->left_tread, definition->wheel_circumference);
			break;
		case _h1_vehicle_function_right_tread_position:
			value = ratio(state->right_tread, definition->wheel_circumference);
			break;
		case _h1_vehicle_function_speed_minus_turn:
			value = ratio(fabsf(state->speed - state->turn), maximum_speed);
			break;
		case _h1_vehicle_function_speed_plus_turn:
			value = ratio(fabsf(state->speed + state->turn), maximum_speed);
			break;
		case _h1_vehicle_function_wheel_position_a:
		case _h1_vehicle_function_wheel_position_b:
		case _h1_vehicle_function_wheel_position_c:
		case _h1_vehicle_function_wheel_position_d:
			value = ratio(state->wheel, definition->wheel_circumference);
			break;
		case _h1_vehicle_function_sideslip:
		{
			const real32 along = dot_product3d(&velocity, &vehicle->object.forward);
			const real_vector3d across = { velocity.i - vehicle->object.forward.i * along, velocity.j - vehicle->object.forward.j * along, velocity.k - vehicle->object.forward.k * along };
			value = magnitude3d(&across) * (1.f / 0.3f);
			value *= value;
		}
			break;
		case _h1_vehicle_function_hover:
			value = state->hover;
			break;
		case _h1_vehicle_function_speed_blend:
		{
			const real32 dot_speed = ratio(fabsf(dot_product3d(&velocity, &vehicle->object.forward)), maximum_speed);
			const real32 forward_value = ratio(fabsf(state->speed), forward_speed);
			const real32 blend = PIN(((real32)state->airborne_ticks * 0.2f + 1.f) * 0.5f, 0.f, 1.f);
			value = dot_speed * (1.f - blend) + forward_value * blend;
		}
			break;
		default:
			// sliding velocity (halo 1's sliding flag), thrust and boost (halo 1's planes)
			break;
		}
		incoming[i] = PIN(value, 0.f, 1.f);
	}
	return;
}

/* ---------- private code */

// the unit in the vehicle's driver seat (halo 2 keeps the driver's controls on the driver), NONE without one
static datum h1_vehicle_driver_get(datum vehicle_index)
{
	const object_datum* vehicle = object_get(vehicle_index);
	const h2x_vehi* definition = (const h2x_vehi*)tag_get('vehi', vehicle->definition_index);
	for (datum child_index = vehicle->object.first_child_object_index; child_index != NONE; child_index = object_get(child_index)->object.next_object_index)
	{
		const unit_datum* child = (const unit_datum*)object_try_and_get_and_verify_type(child_index, _object_mask_unit);
		if (child && VALID_INDEX(child->unit.parent_seat_index, definition->seats.count) && TEST_BIT(definition->seats[child->unit.parent_seat_index]->flags, 2))
		{
			return child_index;
		}
	}
	return NONE;
}

// units.c's vehicle states opening and closing: the driver getting out opens the vehicle (the scorpion's hatch), its enter animation
// done closes it; both hold their last frame, the vehicle idles otherwise (each halo 1 tick)
static void h1_vehicle_base_animation_update(datum vehicle_index, const h1_vehi* h1_vehicle, s_h1_vehicle_state* state)
{
	const datum driver_index = h1_vehicle_driver_get(vehicle_index);
	const unit_datum* driver = driver_index != NONE ? (const unit_datum*)object_get(driver_index) : NULL;
	bool driver_in = false;
	if (driver)
	{
		// in once its seat's enter animation is done
		const uint8* manager = (const uint8*)driver + *(const int16*)((const uint8*)driver + 0x12A);
		const datum graph_index = *(const datum*)(manager + 0x68);
		const int16 animation_index = *(const int16*)(manager + 6);
		const h2x_jmad* graph = graph_index != NONE ? (const h2x_jmad*)tag_get('jmad', graph_index) : NULL;
		const char* name = graph && VALID_INDEX(animation_index, graph->animations.count) ? string_id_get_string_const(graph->animations[animation_index]->name) : "";
		driver_in = !strstr(name, "_enter") && !strstr(name, "_exit");
		// unit_exit_seat: the driver starting to get out opens it
		if (strstr(name, "_exit") && state->base_animation != _h1_vehicle_base_opening)
		{
			state->base_animation = _h1_vehicle_base_opening;
			state->base_frame = 0;
		}
	}
	if (state->had_driver && !driver && state->base_animation != _h1_vehicle_base_opening)
	{
		state->base_animation = _h1_vehicle_base_opening;
		state->base_frame = 0;
	}
	else if (driver_in && state->base_animation != _h1_vehicle_base_closing)
	{
		state->base_animation = _h1_vehicle_base_closing;
		state->base_frame = 0;
	}
	state->had_driver = driver != NULL;

	int16 frame_count = 0;
	if (h1_animation_vehicle_base_get(h1_vehicle->animation_graph.index, (e_h1_vehicle_base_animation)state->base_animation, &frame_count) != NONE)
	{
		// opening and closing hold their last frame, the idle loops
		state->base_frame = state->base_animation == _h1_vehicle_base_idle ? (int16)((state->base_frame + 1) % MAX(frame_count, (int16)1)) :
			MIN((int16)(state->base_frame + 1), (int16)(frame_count - 1));
	}
	return;
}

// halo 2's vehicle animations (vehicle_preprocess_node_orientations alike) read the vehicle's turn, speed, wheel and suspension:
// halo 1's, as halo 2's own drive doesn't run
static void h1_vehicle_animation_state_set(datum vehicle_index, const s_h1_vehicle_state* state)
{
	vehicle_datum* vehicle = vehicle_get(vehicle_index);
	vehicle->vehicle.speed = state->speed;
	vehicle->vehicle.slide = state->slide;
	vehicle->vehicle.turn = state->turn;
	vehicle->vehicle.wheel = state->wheel;
	vehicle->vehicle.rear_wheel = state->wheel;
	vehicle->vehicle.left_tread = state->left_tread;
	vehicle->vehicle.right_tread = state->right_tread;
	csmemcpy(vehicle->vehicle.suspension, state->suspension, sizeof(state->suspension));
	// halo 2's vehicle overlays need the animation's weapon class and type, from the weapon a halo 2 vehicle holds: halo 1's vehicles
	// hold none, their overlays are any class and type
	const int16 manager_offset = *(int16*)((uint8*)vehicle + 0x12A);
	if (manager_offset != NONE)
	{
		uint8* manager = (uint8*)vehicle + manager_offset;
		const object_datum* object = object_get(vehicle_index);
		const datum h1_definition_index = h1_objects_h1_definition_get(object->definition_index);
		const h1_vehi* h1_vehicle = h1_definition_index != NONE ? (const h1_vehi*)g_h1_cache_file->tag_get('vehi', h1_definition_index) : NULL;
		*(string_id*)(manager + 0x74) = h1_vehicle ? h1_animation_vehicle_weapon_class(h1_vehicle->animation_graph.index) : string_id_find_or_add("any");
		*(string_id*)(manager + 0x78) = string_id_find_or_add("any");
	}
	return;
}

// update_suspension: each suspension animation's mass point probes the ground along its normal over the animation's ground depths,
// halfway from the last compression to the new one
static bool h1_vehicle_suspension_update(datum vehicle_index, const h1_vehi* h1_vehicle, const h1_phys* physics, s_h1_vehicle_state* state)
{
	const h1_antr* graph = h1_vehicle->animation_graph.index != NONE ? (const h1_antr*)g_h1_cache_file->tag_get('antr', h1_vehicle->animation_graph.index) : NULL;
	if (!graph || graph->vehicles.count <= 0)
	{
		return false;
	}
	const h1_antr_vehicles* animation = g_h1_cache_file->block_get(graph->vehicles, 0);
	const object_datum* object = object_get(vehicle_index);
	real_matrix4x3 matrix;
	matrix4x3_from_point_and_vectors(&matrix, &object->object.position, &object->object.forward, &object->object.up);
	real32 maximum_shift = 0.f;
	for (int32 i = 0; i < animation->suspension_animations.count && i < NUMBEROF(state->suspension); i++)
	{
		const h1_antr_vehicles_suspension_animations* suspension = g_h1_cache_file->block_get(animation->suspension_animations, i);
		if (!VALID_INDEX(suspension->mass_point_index, physics->mass_points.count) || suspension->animation_index == NONE)
		{
			continue;
		}
		const h1_phys_mass_points* mass_point = g_h1_cache_file->block_get(physics->mass_points, suspension->mass_point_index);
		const real32 current = state->suspension[i] == 0xFF ? 1.f : state->suspension[i] * (1.f / 255.f);

		real_point3d point;
		real_vector3d normal;
		matrix4x3_transform_point(&matrix, &mass_point->position, &point);
		matrix4x3_transform_vector(&matrix, &mass_point->up, &normal);
		const real32 extent = suspension->full_extension_ground_depth - suspension->full_compression_ground_depth;
		// the ground depths are the ground's below the mass point's center (negative): the probe runs from twice the travel above
		// full extension's to full extension's, compressed for the first half
		const real32 offset = suspension->full_compression_ground_depth - extent;
		const real_point3d start = { point.x + normal.i * offset, point.y + normal.j * offset, point.z + normal.k * offset };
		real_vector3d vector;
		scale_vector3d(&normal, extent + extent, &vector);

		collision_result collision;
		const uint32 flags = FLAG(_collision_test_structure_bit) | FLAG(_collision_test_instanced_geometry_bit) | FLAG(_collision_test_objects_bit);
		const real32 t = collision_test_vector(flags, &start, &vector, vehicle_index, NONE, &collision) ? collision.t : 1.f;
		const real32 shift = PIN((1.f - t) + (1.f - t), 0.f, 1.f);
		maximum_shift = MAX(maximum_shift, shift - current);
		state->suspension[i] = (uint8)(int32)(PIN((shift + current) * 0.5f, 0.f, 1.f) * 255.f);
	}

	// a hard landing on the suspension
	if (h1_vehicle->suspension_sound.index != NONE && maximum_shift > 0.3f)
	{
		h1_sound_impulse(h1_vehicle->suspension_sound.index, &object->object.position, PIN((maximum_shift - 0.3f) * (1.f / (0.9f - 0.3f)), 0.f, 1.f));
		return true;
	}
	return false;
}

// material_effects.c material_effect_new: the material effects' effect and sound for the material at the point, a hundredth along the
// normal
static void h1_material_effect_new(datum definition_index, int16 effect_index, int16 material_type, const real_point3d* position, const real_vector3d* normal, real32 scale)
{
	// material effects (foot): effects (0x1C) of materials (0x30): an effect and a sound
	const h1_tag_block<uint8>* effects = definition_index != NONE ? (const h1_tag_block<uint8>*)g_h1_cache_file->tag_get('foot', definition_index) : NULL;
	if (!effects || !VALID_INDEX(effect_index, effects->count) || material_type == NONE)
	{
		return;
	}
	const uint8* effect = (const uint8*)g_h1_cache_file->block_get(*effects, 0) + effect_index * 0x1C;
	const h1_tag_block<uint8>* materials = (const h1_tag_block<uint8>*)effect;
	if (!VALID_INDEX(material_type, materials->count))
	{
		return;
	}
	const uint8* material = (const uint8*)g_h1_cache_file->block_get(*materials, 0) + material_type * 0x30;
	const h1_tag_reference* material_effect = (const h1_tag_reference*)material;
	const h1_tag_reference* material_sound = (const h1_tag_reference*)(material + 0x10);

	const real_point3d point = { position->x + normal->i * 0.01f, position->y + normal->j * 0.01f, position->z + normal->k * 0.01f };
	if (material_effect->index != NONE)
	{
		const char* marker_name = "";
		h1_effect_new_from_markers(material_effect->index, NONE, 1, &marker_name, &point, normal, scale, 0.f);
	}
	if (material_sound->index != NONE)
	{
		h1_sound_impulse(material_sound->index, &point, scale);
	}
	return;
}

// create_slipping_effects: every mass point sliding over the ground kicks up its material (the tires' effects, or the hull's for
// the metallic ones)
static void h1_vehicle_slipping_effects(datum vehicle_index, const h1_vehi* h1_vehicle, const h1_phys* physics, const s_h1_mass_point* mass_points)
{
	if (h1_vehicle->material_effects.index == NONE)
	{
		return;
	}
	for (int32 i = 0; i < physics->mass_points.count && i < k_h1_maximum_mass_points; i++)
	{
		const s_h1_mass_point* mass_point = &mass_points[i];
		if (!TEST_BIT(mass_point->flags, _point_on_ground_bit))
		{
			continue;
		}
		const real32 speed = magnitude3d(&mass_point->velocity_relative_to_ground);
		if (speed <= 0.03f)
		{
			continue;
		}
		const h1_phys_mass_points* definition = g_h1_cache_file->block_get(physics->mass_points, i);
		const real32 depth = mass_point->ground_depth - definition->radius + 0.003f;
		const real_point3d position =
		{
			mass_point->position.x + mass_point->ground_plane.n.i * depth,
			mass_point->position.y + mass_point->ground_plane.n.j * depth,
			mass_point->position.z + mass_point->ground_plane.n.k * depth,
		};
		real_vector3d normal;
		scale_vector3d(&mass_point->velocity_relative_to_ground, 0.8660254f / speed, &normal);
		h1_vector_scale_add(&normal, &mass_point->ground_plane.n, 0.5f);
		h1_material_effect_new(h1_vehicle->material_effects.index, TEST_BIT(definition->flags, 0) ? 10 : 9, mass_point->ground_material_type,
			&position, &normal, PIN((speed - 0.03f) * 4.5454545f, 0.f, 1.f));
	}
	return;
}

// create_ghost_effect: while driven, each hover thruster's effect where a ray in a 15 degree cone about it meets the ground, stronger
// the nearer and the more it points down
static void h1_vehicle_ghost_effect(datum vehicle_index, const h1_vehi* h1_vehicle)
{
	const unit_datum* unit = (const unit_datum*)object_get_and_verify_type(vehicle_index, _object_mask_unit);
	if (h1_vehicle->effect.index == NONE || unit->unit.driver_seat_power <= 0.f)
	{
		return;
	}
	object_marker markers[15];
	const int16 marker_count = object_get_markers_by_string_id(vehicle_index, string_id_find_or_add("hover_thrusters"), markers, NUMBEROF(markers));
	for (int16 m = 0; m < marker_count; m++)
	{
		const real_vector3d* forward = &markers[m].matrix.vectors.forward;
		// a direction in the cone about the thruster
		real_vector3d side, other;
		const real_vector3d reference = fabsf(forward->k) < 0.9f ? real_vector3d{ 0.f, 0.f, 1.f } : real_vector3d{ 1.f, 0.f, 0.f };
		cross_product3d(forward, &reference, &side);
		normalize3d(&side);
		cross_product3d(forward, &side, &other);
		const real32 angle = DEGREES_TO_RADIANS(15.f) * (real32)rand() / (real32)RAND_MAX;
		const real32 around = 2.f * _pi * (real32)rand() / (real32)RAND_MAX;
		real_vector3d direction = *forward;
		scale_vector3d(&direction, cosf(angle), &direction);
		h1_vector_scale_add(&direction, &side, sinf(angle) * cosf(around));
		h1_vector_scale_add(&direction, &other, sinf(angle) * sinf(around));

		collision_result collision;
		const uint32 flags = FLAG(_collision_test_structure_bit) | FLAG(_collision_test_instanced_geometry_bit);
		if (!collision_test_vector(flags, &markers[m].matrix.position, &direction, vehicle_index, NONE, &collision))
		{
			continue;
		}
		const real32 scale = PIN(-forward->k * (1.f - collision.t) * unit->unit.driver_seat_power, 0.f, 1.f);
		if (scale <= 0.f)
		{
			continue;
		}
		const real_vector3d* normal = &collision.fog_plane.n;
		real_vector3d reflected = direction;
		h1_vector_scale_add(&reflected, normal, -2.f * dot_product3d(&direction, normal));
		const char* const marker_names[4] = { "incident", "normal", "reflected", "midpoint" };
		const real_point3d midpoint =
		{
			(markers[m].matrix.position.x + collision.point.x) * 0.5f,
			(markers[m].matrix.position.y + collision.point.y) * 0.5f,
			(markers[m].matrix.position.z + collision.point.z) * 0.5f,
		};
		const real_point3d marker_points[4] = { collision.point, collision.point, collision.point, midpoint };
		const real_vector3d marker_forwards[4] = { { -direction.i, -direction.j, -direction.k }, *normal, reflected, reflected };
		h1_effect_new_from_markers(h1_vehicle->effect.index, NONE, 4, marker_names, marker_points, marker_forwards, scale, scale);
	}
	return;
}

// vehicle_update for one halo 1 tick
static void h1_vehicle_tick(datum vehicle_index, const h1_vehi* h1_vehicle, const h1_phys* physics, s_h1_vehicle_state* state)
{
	const unit_datum* unit = (const unit_datum*)object_get_and_verify_type(vehicle_index, _object_mask_unit);
	const real_vector3d* forward = &unit->object.forward;
	const real_vector3d* up = &unit->object.up;
	// the vehicle's controls are its driver's
	const datum driver_index = h1_vehicle_driver_get(vehicle_index);
	const unit_datum* driver = driver_index != NONE ? (const unit_datum*)object_get(driver_index) : NULL;
	// without a driver its own (a recording or a script controlling the vehicle: recorded_animations.c's unit_control)
	const real_vector3d* throttle = driver ? &driver->unit.throttle : &unit->unit.throttle;
	const bool own_facing = !driver && magnitude_squared3d(&unit->unit.desired_facing_vector) > 0.5f;
	const real_vector3d* desired_facing = driver ? &driver->unit.desired_facing_vector : own_facing ? &unit->unit.desired_facing_vector : forward;

	// braking: controls opposite the speed
	SET_BIT(state->flags, _h1_vehicle_braking_bit,
		TEST_BIT(h1_vehicle->flags_3, _h1_vehicle_flag_control_opposite_speed_sets_brake_bit) &&
		((throttle->i > 0.f && state->speed < 0.f) || (throttle->i < 0.f && state->speed > 0.f)));

	real_vector3d left;
	cross_product3d(up, forward, &left);
	const real32 steering = atan2f(dot_product3d(&left, desired_facing), dot_product3d(desired_facing, forward));

	const s_h1_speed_parameters* speed_parameters = (const s_h1_speed_parameters*)&h1_vehicle->maximum_forward_speed;
	const s_h1_speed_parameters* slide_parameters = (const s_h1_speed_parameters*)&h1_vehicle->maximum_left_slide;
	if (TEST_BIT(state->flags, _h1_vehicle_braking_bit))
	{
		h1_speed_update_seek(&state->speed, speed_parameters, 0.f, 1.f);
	}
	else
	{
		h1_speed_update_seek(&state->speed, speed_parameters, throttle->i, 1.f);
		h1_speed_update_seek(&state->slide, slide_parameters, throttle->j, 1.f);
	}
	if (h1_vehicle->type != _h1_vehicle_type_human_tank)
	{
		const real32 limits[2] = { h1_vehicle->maximum_left_turn, h1_vehicle->maximum_right_turn_negative };
		const real32 desired = PIN(state->speed < 0.f ? -steering : steering, DEGREES_TO_RADIANS(limits[1]), DEGREES_TO_RADIANS(limits[0]));
		h1_position_update_seek(&state->turn, limits, desired, DEGREES_TO_RADIANS(h1_vehicle->turn_rate) / k_h1_ticks_per_second);
	}
	else if (state->speed == 0.f)
	{
		h1_speed_update_seek(&state->turn, speed_parameters, 0.f, 1.f);
	}
	else
	{
		h1_speed_update_seek(&state->turn, speed_parameters, PIN(steering * 0.63661975f, -1.f, 1.f) * h1_vehicle->maximum_forward_speed, 2.f);
	}

	s_h1_powered_mass_point powered[k_h1_maximum_mass_points] = {};
	s_h1_mass_point mass_points[k_h1_maximum_mass_points];
	for (int32 i = 0; i < k_h1_maximum_mass_points; i++)
	{
		powered[i].rotation = { 0.f, 0.f, 0.f, 1.f };
	}
	real_vector3d magic_force = *global_zero_vector3d;
	real_vector3d magic_torque = *global_zero_vector3d;
	bool use_powered = true;

	switch (h1_vehicle->type)
	{
	case _h1_vehicle_type_human_jeep:
	{
		if (h1_vehicle->wheel_circumference > 0.f)
		{
			state->wheel = fmodf(state->wheel + state->speed, h1_vehicle->wheel_circumference);
			if (state->wheel < 0.f) state->wheel += h1_vehicle->wheel_circumference;
		}
		if (physics->powered_mass_points.count == 2)
		{
			const real32 angle = state->turn * 0.5f;
			powered[0].ground_friction_velocity = state->speed;
			powered[0].rotation = { 0.f, 0.f, sinf(angle), cosf(angle) };
			powered[1].ground_friction_velocity = state->speed;
			powered[1].rotation = { 0.f, 0.f, -sinf(angle), cosf(angle) };
		}
		else
		{
			use_powered = false;
		}
		break;
	}
	case _h1_vehicle_type_human_tank:
	{
		const real32 left_speed = state->speed - state->turn;
		const real32 right_speed = state->turn + state->speed;
		// the treads' positions (their shaders scroll by them)
		if (h1_vehicle->wheel_circumference > 0.f)
		{
			state->left_tread = fmodf(state->left_tread + left_speed, h1_vehicle->wheel_circumference);
			if (state->left_tread < 0.f) state->left_tread += h1_vehicle->wheel_circumference;
			state->right_tread = fmodf(state->right_tread + right_speed, h1_vehicle->wheel_circumference);
			if (state->right_tread < 0.f) state->right_tread += h1_vehicle->wheel_circumference;
		}
		if (physics->powered_mass_points.count == 2)
		{
			powered[0].ground_friction_velocity = left_speed;
			powered[1].ground_friction_velocity = right_speed;
		}
		else
		{
			use_powered = false;
		}
		break;
	}
	case _h1_vehicle_type_alien_scout:
	{
		// update_alien_scout_physics
		const real32 antigrav = unit->unit.driver_seat_power;
		for (int32 i = 0; i < physics->powered_mass_points.count && i < k_h1_maximum_mass_points; i++)
		{
			powered[i].antigrav_fraction = antigrav;
		}
		if (up->k > -0.2f)
		{
			const real_vector3d* angular_velocity = &state->angular_velocity;
			real_matrix4x3 vehicle_matrix;
			matrix4x3_from_point_and_vectors(&vehicle_matrix, &unit->object.position, forward, up);
			const real_vector3d local_velocity = { dot_product3d(&state->linear_velocity, forward), dot_product3d(&state->linear_velocity, &left), dot_product3d(&state->linear_velocity, up) };
			if (state->hover > 0.f)
			{
				real32 maximum_speed = h1_vehicle->maximum_forward_speed;
				if (TEST_BIT(state->flags, _h1_vehicle_braking_bit)) maximum_speed *= 0.8f;
				real32 maximum_acceleration = h1_vehicle->speed_acceleration;
				real_vector3d acceleration = { maximum_speed * throttle->i - local_velocity.i, maximum_speed * throttle->j - local_velocity.j, 0.f };
				if (state->on_ground_ticks > 0 && fabsf(steering) > 0.785398185f)
				{
					maximum_acceleration *= 1.f - MIN(state->on_ground_ticks * 0.05f, 0.98f);
				}
				const real32 magnitude = magnitude3d(&acceleration);
				if (magnitude > maximum_acceleration && magnitude > 0.f)
				{
					scale_vector3d(&acceleration, maximum_acceleration / magnitude, &acceleration);
				}
				real_vector3d world_acceleration = *global_zero_vector3d;
				h1_vector_scale_add(&world_acceleration, forward, acceleration.i);
				h1_vector_scale_add(&world_acceleration, &left, acceleration.j);
				h1_vector_scale_add(&magic_force, &world_acceleration, physics->mass * state->hover);

				const real32 current = dot_product3d(up, angular_velocity);
				const real32 sign = steering != 0.f ? (steering < 0.f ? -1.f : 1.f) : 0.f;
				real32 desired = sqrtf(fabsf(steering) * 0.0069813174f) * sign;
				if (fabsf(desired) > 0.0001f && steering / desired < 2.f)
				{
					desired = steering * 0.5f;
				}
				const real32 error = PIN(desired - current, -0.0034906587f, 0.0034906587f);
				h1_vector_scale_add(&magic_torque, up, error * physics->zz_moment * state->hover);
			}
			if (state->hover < 1.f)
			{
				// levelling and pitch/roll control while it isn't hovering
				real_vector2d forward2d = { forward->i, forward->j };
				real_vector2d left2d = { left.i, left.j };
				normalize2d(&forward2d);
				normalize2d(&left2d);
				real32 torque_a, torque_b;
				if (up->k > 0.f)
				{
					const real_vector2d up2d = { up->i, up->j };
					const real_vector2d angular2d = { angular_velocity->i, angular_velocity->j };
					real_vector2d level =
					{
						-dot_product2d(&up2d, &forward2d) - 15.f * dot_product2d(&angular2d, &left2d),
						-dot_product2d(&up2d, &left2d) + 15.f * dot_product2d(&angular2d, &forward2d),
					};
					const real32 sign_a = throttle->i * level.i != 0.f ? (throttle->i * level.i < 0.f ? -1.f : 1.f) : 0.f;
					const real32 sign_b = throttle->j * level.j != 0.f ? (throttle->j * level.j < 0.f ? -1.f : 1.f) : 0.f;
					const real32 control_a = throttle->i * PIN(fabsf(level.i) * sign_a + 1.f, 0.3f, 2.5f) * 0.0015514038f;
					const real32 control_b = throttle->j * PIN(fabsf(level.j) * sign_b + 1.f, 0.3f, 2.5f) * 0.0015514038f;
					const real32 level_scale = (1.f - up->k) * 0.0038785094f;
					torque_a = level_scale * level.i + control_a;
					torque_b = level_scale * level.j + control_b;
				}
				else
				{
					torque_a = throttle->i * 0.0015514038f;
					torque_b = throttle->j * 0.0015514038f;
				}
				real_vector3d torque = *global_zero_vector3d;
				h1_vector_scale_add(&torque, &left, physics->yy_moment * torque_a);
				h1_vector_scale_add(&torque, forward, -(physics->xx_moment * torque_b));
				h1_vector_scale_add(&magic_torque, &torque, 1.f - state->hover);
			}
			scale_vector3d(&magic_force, antigrav, &magic_force);
			scale_vector3d(&magic_torque, antigrav, &magic_torque);
		}
		break;
	}
	default:
		use_powered = false;
		break;
	}

	h1_physics_update(vehicle_index, physics, state, use_powered ? powered : NULL, &magic_force, &magic_torque, mass_points);
	state->suspension_sounded |= h1_vehicle_suspension_update(vehicle_index, h1_vehicle, physics, state);
	h1_vehicle_slipping_effects(vehicle_index, h1_vehicle, physics, mass_points);
	if (h1_vehicle->type == _h1_vehicle_type_alien_scout || h1_vehicle->type == _h1_vehicle_type_alien_fighter)
	{
		h1_vehicle_ghost_effect(vehicle_index, h1_vehicle);
	}

	// compute_airborne_ticks
	if (state->airborne_ticks < 0xFF) state->airborne_ticks++;
	bool on_ground = false;
	state->on_ground = false;
	for (int32 i = 0; i < physics->mass_points.count && i < k_h1_maximum_mass_points; i++)
	{
		state->on_ground |= TEST_BIT(mass_points[i].flags, _point_on_ground_bit);
	}
	for (int32 i = 0; i < physics->mass_points.count && i < k_h1_maximum_mass_points; i++)
	{
		if (TEST_BIT(mass_points[i].flags, _point_on_ground_bit))
		{
			state->airborne_ticks = 0;
			if (state->on_ground_ticks < 0xFF) state->on_ground_ticks++;
			on_ground = true;
			break;
		}
		if (TEST_BIT(mass_points[i].flags, _point_antigraving_bit))
		{
			state->airborne_ticks = 0;
		}
	}
	if (!on_ground)
	{
		state->on_ground_ticks = 0;
	}

	// the scout's hover follows its grounded hover pads
	if (h1_vehicle->type == _h1_vehicle_type_alien_scout)
	{
		int32 powered_count = 0, grounded_count = 0;
		for (int32 i = 0; i < physics->mass_points.count && i < k_h1_maximum_mass_points; i++)
		{
			const h1_phys_mass_points* definition = g_h1_cache_file->block_get(physics->mass_points, i);
			if (definition->powered_mass_point_index != NONE)
			{
				powered_count++;
				grounded_count += TEST_BIT(mass_points[i].flags, _point_antigraving_bit) ? 1 : 0;
			}
		}
		const real32 ratio = powered_count > 0 ? (real32)grounded_count / (real32)powered_count : 0.f;
		const real32 target = PIN(ratio * MAX(up->k, 0.4f), 0.f, 1.f);
		state->hover = PIN(target, state->hover - 0.1f, state->hover + 0.1f);
	}
	return;
}

static void h1_friction_evaluate(int16 friction_type, real32 parallel_scale, real32 perpendicular_scale, s_h1_friction* friction, const real_vector3d* forward, const real_vector3d* up)
{
	if (friction_type == _friction_type_point)
	{
		friction->parallel = friction->friction;
		friction->perpendicular = *global_zero_vector3d;
		return;
	}
	real_vector3d axis;
	switch (friction_type)
	{
	case _friction_type_forward: axis = *forward; break;
	case _friction_type_left: cross_product3d(up, forward, &axis); break;
	default: axis = *up; break;
	}
	h1_components_from_normal(&friction->friction, &axis, &friction->parallel, &friction->perpendicular);
	scale_vector3d(&friction->parallel, parallel_scale, &friction->parallel);
	scale_vector3d(&friction->perpendicular, perpendicular_scale, &friction->perpendicular);
	add_vectors3d(&friction->parallel, &friction->perpendicular, &friction->friction);
	return;
}

// compute_ground_plane: the surface within the mass point's radius below it
static void h1_mass_point_ground(datum vehicle_index, s_h1_mass_point* mass_point, real32 radius)
{
	mass_point->ground_depth = 0.f;
	mass_point->ground_material_type = NONE;
	// from a radius above the point (a point sunk into the ground still finds it) to a radius below it
	const real_point3d start = { mass_point->position.x, mass_point->position.y, mass_point->position.z + radius };
	real_vector3d probe = { 0.f, 0.f, -2.f * radius };
	collision_result collision;
	const uint32 flags = FLAG(_collision_test_structure_bit) | FLAG(_collision_test_instanced_geometry_bit) | FLAG(_collision_test_objects_bit);
	if (collision_test_vector(flags, &start, &probe, vehicle_index, NONE, &collision))
	{
		mass_point->ground_plane = collision.fog_plane;
		mass_point->ground_material_type = h1_projectile_logic_collision_material_type(&collision);
		// the distance to the surface's plane along its normal, from the hit straight below
		const real32 distance = (collision.t * 2.f * radius - radius) * fabsf(collision.fog_plane.n.k);
		mass_point->ground_depth = radius - distance;
	}
	return;
}

// physics_compute_new and physics_update_new, without halo 1's sweep (havok collides)
static void h1_physics_update(datum vehicle_index, const h1_phys* physics, s_h1_vehicle_state* state, s_h1_powered_mass_point* powered_mass_points,
	const real_vector3d* magic_force, const real_vector3d* magic_torque, s_h1_mass_point* mass_points)
{
	const object_datum* object = object_get(vehicle_index);
	const int32 mass_point_count = MIN(physics->mass_points.count, (int32)k_h1_maximum_mass_points);
	const real32 gravity = physics->gravity_scale * k_h1_global_gravity;

	real_matrix4x3 world_matrix;
	matrix4x3_from_point_and_vectors(&world_matrix, &object->object.position, &object->object.forward, &object->object.up);

	if (powered_mass_points)
	{
		for (int32 i = 0; i < physics->powered_mass_points.count && i < k_h1_maximum_mass_points; i++)
		{
			// the rotation of the powered mass point's frame (a positive turn turns the front wheels left)
			const real_quaternion* q = &powered_mass_points[i].rotation;
			real_matrix4x3 rotation;
			matrix4x3_rotation_from_quaternion(&rotation, q);
			real_matrix3x3* m = &powered_mass_points[i].rotation_matrix;
			m->forward = rotation.forward;
			m->left = rotation.left;
			m->up = rotation.up;
		}
	}

	real_vector3d total_force = { 0.f, 0.f, -physics->mass * gravity };
	real_vector3d total_torque = *global_zero_vector3d;
	for (int32 index = 0; index < mass_point_count; index++)
	{
		const h1_phys_mass_points* definition = g_h1_cache_file->block_get(physics->mass_points, index);
		s_h1_mass_point* mass_point = &mass_points[index];
		csmemset(mass_point, 0, sizeof(*mass_point));
		const h1_phys_powered_mass_points* powered_definition = NULL;
		const s_h1_powered_mass_point* powered = NULL;
		if (definition->powered_mass_point_index != NONE && powered_mass_points && definition->powered_mass_point_index < k_h1_maximum_mass_points)
		{
			powered_definition = g_h1_cache_file->block_get(physics->powered_mass_points, definition->powered_mass_point_index);
			powered = &powered_mass_points[definition->powered_mass_point_index];
		}

		matrix4x3_transform_point(&world_matrix, &definition->position, &mass_point->position);
		real_vector3d local_forward = definition->forward, local_up = definition->up;
		if (powered)
		{
			const real_matrix3x3* m = &powered->rotation_matrix;
			local_forward =
			{
				definition->forward.i * m->forward.i + definition->forward.j * m->left.i + definition->forward.k * m->up.i,
				definition->forward.i * m->forward.j + definition->forward.j * m->left.j + definition->forward.k * m->up.j,
				definition->forward.i * m->forward.k + definition->forward.j * m->left.k + definition->forward.k * m->up.k,
			};
			local_up =
			{
				definition->up.i * m->forward.i + definition->up.j * m->left.i + definition->up.k * m->up.i,
				definition->up.i * m->forward.j + definition->up.j * m->left.j + definition->up.k * m->up.j,
				definition->up.i * m->forward.k + definition->up.j * m->left.k + definition->up.k * m->up.k,
			};
		}
		matrix4x3_transform_vector(&world_matrix, &local_forward, &mass_point->forward);
		matrix4x3_transform_vector(&world_matrix, &local_up, &mass_point->up);

		vector_from_points3d(&object->object.position, &mass_point->position, &mass_point->radius);
		cross_product3d(&state->angular_velocity, &mass_point->radius, &mass_point->velocity);
		add_vectors3d(&state->linear_velocity, &mass_point->velocity, &mass_point->velocity);

		h1_mass_point_ground(vehicle_index, mass_point, definition->radius);
		if (mass_point->ground_depth > 0.f)
		{
			const real32 normal_velocity = dot_product3d(&mass_point->velocity, &mass_point->ground_plane.n);
			mass_point->normal_force_magnitude = physics->mass * (k_h1_global_gravity / physics->ground_depth * mass_point->ground_depth - normal_velocity * physics->ground_damp_fraction);
			scale_vector3d(&mass_point->ground_plane.n, mass_point->normal_force_magnitude, &mass_point->normal_force);

			scale_vector3d(&mass_point->ground_plane.n, -normal_velocity, &mass_point->velocity_relative_to_ground);
			add_vectors3d(&mass_point->velocity_relative_to_ground, &mass_point->velocity, &mass_point->velocity_relative_to_ground);

			const real32 ground_scale = -definition->mass * physics->ground_friction;
			scale_vector3d(&mass_point->velocity_relative_to_ground, ground_scale, &mass_point->ground_friction.friction);

			if (powered_definition && TEST_BIT(powered_definition->flags, _powered_mass_point_ground_friction_bit) && powered->ground_friction_velocity != 0.f)
			{
				const real32 fraction = h1_pin_fraction(mass_point->ground_plane.n.k, physics->ground_normal_k0, physics->ground_normal_k1);
				const real32 alignment = PIN(dot_product3d(&mass_point->up, &mass_point->ground_plane.n), 0.f, 1.f);
				const real32 weight = alignment * alignment * fraction * fraction * ground_scale;
				real_vector3d powered_velocity;
				scale_vector3d(&mass_point->forward, -powered->ground_friction_velocity, &powered_velocity);
				real_vector3d projected;
				scale_vector3d(&mass_point->ground_plane.n, -dot_product3d(&powered_velocity, &mass_point->ground_plane.n), &projected);
				add_vectors3d(&projected, &powered_velocity, &projected);
				add_vectors3d(&mass_point->velocity_relative_to_ground, &projected, &mass_point->velocity_relative_to_ground);
				h1_vector_scale_add(&mass_point->ground_friction.friction, &projected, weight);
			}
			h1_friction_evaluate(definition->friction_type, definition->friction_parallel_scale, definition->friction_perpendicular_scale,
				&mass_point->ground_friction, &mass_point->forward, &mass_point->up);
		}

		// air friction
		{
			const real32 air_scale = -definition->mass * physics->air_friction;
			real_vector3d velocity = mass_point->velocity;
			if (powered_definition && TEST_BIT(powered_definition->flags, _powered_mass_point_air_friction_bit) && powered->air_friction_velocity != 0.f)
			{
				h1_vector_scale_add(&velocity, &mass_point->forward, -powered->air_friction_velocity);
			}
			scale_vector3d(&velocity, air_scale, &mass_point->air_friction.friction);
			h1_friction_evaluate(definition->friction_type, definition->friction_parallel_scale, definition->friction_perpendicular_scale,
				&mass_point->air_friction, &mass_point->forward, &mass_point->up);
		}

		SET_BIT(mass_point->flags, _point_at_rest_bit, magnitude_squared3d(&mass_point->velocity) < 0.0011111111f);
		SET_BIT(mass_point->flags, _point_on_ground_bit, mass_point->ground_depth > 0.f);

		if (powered_definition)
		{
			if (TEST_BIT(powered_definition->flags, _powered_mass_point_thrust_bit))
			{
				h1_vector_scale_add(&mass_point->powered_force, &mass_point->forward, powered->thrust_fraction * physics->mass);
			}
			if (TEST_BIT(powered_definition->flags, _powered_mass_point_antigrav_bit))
			{
				const real32 probe_length = definition->radius + powered_definition->antigrav_height;
				real_vector3d probe = { 0.f, 0.f, -probe_length };
				collision_result collision;
				const uint32 flags = FLAG(_collision_test_structure_bit) | FLAG(_collision_test_instanced_geometry_bit) | FLAG(_collision_test_objects_bit);
				if (collision_test_vector(flags, &mass_point->position, &probe, vehicle_index, NONE, &collision))
				{
					const real32 height = probe_length * collision.t - definition->radius;
					const real32 alignment = h1_pin_fraction(mass_point->up.k, powered_definition->antigrav_normal_k0, powered_definition->antigrav_normal_k1);
					const real32 ground_effect = height > 0.f ? 1.f - height / powered_definition->antigrav_height : 1.f;
					const real32 magnitude = (ground_effect * ground_effect * k_h1_global_gravity -
						dot_product3d(&collision.fog_plane.n, &mass_point->velocity) * powered_definition->antigrav_damp_fraction) *
						powered->antigrav_fraction * powered_definition->antigrav_strength * physics->mass * alignment;
					h1_vector_scale_add(&mass_point->powered_force, &collision.fog_plane.n, magnitude);
					SET_BIT(mass_point->flags, _point_antigraving_bit, true);
				}
			}
		}

		add_vectors3d(&mass_point->normal_force, &mass_point->ground_friction.friction, &mass_point->force);
		add_vectors3d(&mass_point->force, &mass_point->air_friction.friction, &mass_point->force);
		add_vectors3d(&mass_point->force, &mass_point->powered_force, &mass_point->force);
		cross_product3d(&mass_point->radius, &mass_point->force, &mass_point->torque);
		add_vectors3d(&total_force, &mass_point->force, &total_force);
		add_vectors3d(&total_torque, &mass_point->torque, &total_torque);
	}
	add_vectors3d(&total_force, magic_force, &total_force);
	add_vectors3d(&total_torque, magic_torque, &total_torque);

	// physics_update_new: the accelerations change the velocities (the inverse inertia in world space)
	h1_vector_scale_add(&state->linear_velocity, &total_force, 1.f / physics->mass);
	const h1_phys_inertial_matrix_and_inverse* inverse = g_h1_cache_file->block_get(physics->inertial_matrix_and_inverse, 1);
	if (inverse)
	{
		const real_matrix3x3* local_inverse = (const real_matrix3x3*)inverse;
		real_vector3d left;
		cross_product3d(&object->object.up, &object->object.forward, &left);
		// to local, the inverse inertia, back to world
		const real_vector3d local_torque = { dot_product3d(&total_torque, &object->object.forward), dot_product3d(&total_torque, &left), dot_product3d(&total_torque, &object->object.up) };
		const real_vector3d local_acceleration =
		{
			local_torque.i * local_inverse->forward.i + local_torque.j * local_inverse->left.i + local_torque.k * local_inverse->up.i,
			local_torque.i * local_inverse->forward.j + local_torque.j * local_inverse->left.j + local_torque.k * local_inverse->up.j,
			local_torque.i * local_inverse->forward.k + local_torque.j * local_inverse->left.k + local_torque.k * local_inverse->up.k,
		};
		h1_vector_scale_add(&state->angular_velocity, &object->object.forward, local_acceleration.i);
		h1_vector_scale_add(&state->angular_velocity, &left, local_acceleration.j);
		h1_vector_scale_add(&state->angular_velocity, &object->object.up, local_acceleration.k);
	}

	// physics_update_new: at rest when every mass point rests, three are on the ground and nothing accelerates it
	int32 at_rest_count = 0, on_ground_count = 0;
	for (int32 index = 0; index < mass_point_count; index++)
	{
		at_rest_count += TEST_BIT(mass_points[index].flags, _point_at_rest_bit) ? 1 : 0;
		on_ground_count += TEST_BIT(mass_points[index].flags, _point_on_ground_bit) ? 1 : 0;
	}
	real_vector3d linear_acceleration;
	scale_vector3d(&total_force, 1.f / physics->mass, &linear_acceleration);
	state->at_rest = at_rest_count == mass_point_count && on_ground_count >= 3 &&
		magnitude_squared3d(&state->linear_velocity) <= 0.0011111111f &&
		magnitude_squared3d(&state->angular_velocity) <= 0.0027415568f &&
		magnitude_squared3d(&linear_acceleration) <= 0.00000030864197f;
	return;
}

static object_update_t g_h2_vehicle_update = NULL;

// the vehicle part's update: halo 2's (seats, animations, its own physics for halo 2 vehicles), then halo 1's physics for halo 1
// vehicles
static object_preprocess_node_orientations_t g_h2_vehicle_preprocess_node_orientations = NULL;

// halo 2's vehicle update measures its own suspension between halo 1's updates: halo 1's state again just before the vehicle animates
static void h1_vehicle_preprocess_node_orientations_hook(datum vehicle_index, uint8* node_flags, int32 node_count, real_orientation* orientations)
{
	auto found = h1_maps_active() ? g_h1_vehicle_states.find(vehicle_index) : g_h1_vehicle_states.end();
	if (found != g_h1_vehicle_states.end())
	{
		h1_vehicle_animation_state_set(vehicle_index, &found->second);
	}
	// halo 2 plays no base animation on halo 1's vehicles: the vehicle's idle, opening or closing frame first, its overlays after
	{
		const vehicle_datum* vehicle = found != g_h1_vehicle_states.end() ? (const vehicle_datum*)object_try_and_get_and_verify_type(vehicle_index, _object_mask_vehicle) : NULL;
		const datum h1_definition_index = vehicle ? h1_objects_h1_definition_get(vehicle->definition_index) : NONE;
		const h1_vehi* h1_vehicle = h1_definition_index != NONE ? (const h1_vehi*)g_h1_cache_file->tag_get('vehi', h1_definition_index) : NULL;
		int16 frame_count = 0;
		const int16 animation_index = h1_vehicle ?
			h1_animation_vehicle_base_get(h1_vehicle->animation_graph.index, (e_h1_vehicle_base_animation)found->second.base_animation, &frame_count) : (int16)NONE;
		if (animation_index != NONE)
		{
			h1_animation_base_frame_apply(h1_vehicle->animation_graph.index, animation_index, found->second.base_frame, orientations, node_count);
		}
	}
	g_h2_vehicle_preprocess_node_orientations(vehicle_index, node_flags, node_count, orientations);

	// halo 2 doesn't aim its vehicles with the aiming overlay: the turret follows the vehicle's aim as halo 1's does
	const vehicle_datum* vehicle = found != g_h1_vehicle_states.end() ? (const vehicle_datum*)object_try_and_get_and_verify_type(vehicle_index, _object_mask_vehicle) : NULL;
	const datum h1_definition_index = vehicle ? h1_objects_h1_definition_get(vehicle->definition_index) : NONE;
	const h1_vehi* h1_vehicle = h1_definition_index != NONE ? (const h1_vehi*)g_h1_cache_file->tag_get('vehi', h1_definition_index) : NULL;
	if (h1_vehicle)
	{
		real_vector3d left;
		cross_product3d(&vehicle->object.up, &vehicle->object.forward, &left);
		const real_vector3d* aim = &vehicle->unit.aiming_vector;
		const real32 yaw = atan2f(dot_product3d(aim, &left), dot_product3d(aim, &vehicle->object.forward));
		const real32 pitch = asinf(PIN(dot_product3d(aim, &vehicle->object.up), -1.f, 1.f));
		h1_animation_vehicle_aim_apply(h1_vehicle->animation_graph.index, yaw, pitch, orientations, node_count);
	}
	return;
}

static bool h1_vehicle_update_hook(datum vehicle_index)
{
	const bool result = g_h2_vehicle_update(vehicle_index);
	h1_vehicle_physics_update(vehicle_index);
	return result;
}

void h1_vehicle_physics_apply_patches(void)
{
	object_type_definition* vehicle_type = object_type_definition_get(_object_type_vehicle);
	for (int32 i = 0; i < k_max_object_type_inheritence; i++)
	{
		object_type_definition* part = vehicle_type->part_definitions[i];
		if (part && part->group_tag == 'vehi' && part->object_update)
		{
			g_h2_vehicle_update = part->object_update;
			part->object_update = h1_vehicle_update_hook;
			if (part->object_preprocess_node_orientations)
			{
				g_h2_vehicle_preprocess_node_orientations = part->object_preprocess_node_orientations;
				part->object_preprocess_node_orientations = h1_vehicle_preprocess_node_orientations_hook;
			}
			break;
		}
	}
	return;
}

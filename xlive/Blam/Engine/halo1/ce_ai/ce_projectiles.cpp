#include "stdafx.h"
#include "h1_ai_internal.h"

/*
* projectiles.c: the functions of it the AI calls.
*/

namespace h1_ai
{

/* ---------- items/projectiles.c: its declarations */

/* ---------- headers */

/* ---------- constants */

enum projectile_datum_flags
{
	_projectile_has_nonzero_angular_velocity_bit = 0,
	_projectile_tracer_bit = 1,
	_projectile_collided_once_bit = 2,
	_projectile_attached_bit = 3,
	_projectile_stopped_after_collision_bit = 4,
	_projectile_counting_down_bit = 5,
	_projectile_already_super_exploded_bit = 6,
	_projectile_will_super_explode_bit = 7,
	NUMBER_OF_PROJECTILE_DATUM_FLAGS,
};

enum projectile_definition_flags
{
	_projectile_oriented_along_velocity_bit = 0,
	_projectile_aim_ballistic_bit = 1,
	_projectile_detonation_max_time_if_attached_bit = 2,
	_projectile_super_combining_explosion_bit = 3,
	_projectile_combine_initial_velocity_with_parent_velocity_bit = 4,
	_projectile_random_detonation_time_when_attached_bit = 5,
	_projectile_minimum_unattached_detonation_time = 6,
	NUMBER_OF_PROJECTILE_DEFINITION_FLAGS,
};

enum projectile_export_function_mode
{
	_projectile_export_function_none = 0,
	_projectile_export_function_range_remaining,
	_projectile_export_function_time_remaining,
	_projectile_export_function_tracer,
	NUMBER_OF_PROJECTILE_EXPORT_FUNCTION_MODES,
};

enum projectile_action
{
	_projectile_action_none = 0,
	_projectile_action_detonate,
	_projectile_action_disappear,
	NUMBER_OF_PROJECTILE_ACTIONS,
};

enum projectile_detonation_timer_starts
{
	_projectile_detonation_timer_starts_immediately = 0,
	_projectile_detonation_timer_starts_after_first_bounce,
	_projectile_detonation_timer_starts_when_at_rest,
	NUMBER_OF_PROJECTILE_DETONATION_TIMER_STARTS,
};

enum projectile_potential_response_flags
{
	_projectile_potential_response_only_against_units_bit = 0,
	NUMBER_OF_PROJECTILE_POTENTIAL_RESPONSE_FLAGS,
};

/* TU-local copies: no shared header declares these yet.  The effect vector
 * enum duplicates objects/damage.c, the spatial effect enum duplicates
 * ai/actors.c and ai/ai.c, the surface flag duplicates ai/path.c and
 * ai/path_smoothing.c, and the periodic function enum duplicates
 * math/periodic_functions.c. */
enum
{
	_effect_vector_normal = 0,
	_effect_vector_incident,
	_effect_vector_negative_incident,
	_effect_vector_reflected,
	_effect_vector_gravity,
	NUMBER_OF_EFFECT_MARKERS,
};

enum
{
	_ai_spatial_effect_environmental_noise = 0,
	_ai_spatial_effect_weapon_impact,
	_ai_spatial_effect_weapon_detonation,
	NUMBER_OF_AI_SPATIAL_EFFECTS,
};

enum
{
	_collision_surface_breakable_bit = 3,
};

enum
{
	_periodic_function_one = 0,
	_periodic_function_zero,
	_periodic_function_cosine,
	_periodic_function_cosine_variable_period,
	_periodic_function_diagonal_wave,
	_periodic_function_diagonal_wave_variable_period,
	_periodic_function_slide,
	_periodic_function_slide_variable_period,
	_periodic_function_noise,
	_periodic_function_jitter,
	_periodic_function_wander,
	_periodic_function_spark,
	NUMBER_OF_PERIODIC_FUNCTIONS,
};

enum
{
	MAXIMUM_PROJECTILE_COLLISIONS_PER_UPDATE = 10,
	MAXIMUM_COMBINING_PROJECTILES = 6,
};

/* ---------- macros */

/* ---------- structures */

typedef char projectile_runtime_arming_time_delta_offset_assert[
	offsetof(struct projectile_datum, projectile.arming_time_delta) == 0x1FC
		? 1
		: -1];
typedef char projectile_runtime_odometer_offset_assert[
	offsetof(struct projectile_datum, projectile.odometer) == 0x200
		? 1
		: -1];
typedef char projectile_runtime_deceleration_offset_assert[
	offsetof(struct projectile_datum, projectile.deceleration) == 0x20C
		? 1
		: -1];
typedef char projectile_runtime_rotation_axis_offset_assert[
	offsetof(struct projectile_datum, projectile.rotation_axis) == 0x214
		? 1
		: -1];
typedef char projectile_runtime_rotation_cosine_offset_assert[
	offsetof(struct projectile_datum, projectile.rotation_cosine) == 0x224
		? 1
		: -1];

/* ---------- prototypes */

boolean projectile_aim_linear(
	real base_velocity,
	real_point3d const *origin,
	real_point3d const *target_point,
	real_vector3d *result_aim_vector,
	real *result_velocity,
	real *result_ticks,
	real *result_distance);

static real projectile_calculate_deceleration_from_distances(
	struct projectile_definition const *definition,
	real minimum_distance,
	real maximum_distance);
static void projectile_adjust_for_angular_velocity_change(
	long projectile_index);
static void projectile_calculate_deceleration(
	long projectile_index);
static void projectile_set_action(
	long projectile_index,
	short action);
static void projectile_effect_new(
	long projectile_index,
	long definition_index,
	struct collision_result const *collision,
	real_point3d const *marker_points,
	real_vector3d const *marker_forwards,
	real scale,
	real material_effect_scale);
static boolean projectile_collision_test_line(
	long projectile_index,
	real_point3d const *new_position,
	struct collision_result *collision);
static void projectile_detonate(
	long projectile_index,
	boolean first_collision,
	real time_left);
static void projectile_collision(
	long projectile_index,
	struct collision_result *collision,
	real_point3d *new_position,
	real_vector3d *new_velocity,
	real time_left);

/* ---------- globals */

static real const seconds_per_tick = 1.0f / TICKS_PER_SECOND;

static struct profile_section projectile_update_section = {"projectile_update", NONE, TRUE};

static char const *effect_marker_names[NUMBER_OF_EFFECT_MARKERS] =
{
	"normal",
	"incident",
	"negative incident",
	"reflection",
	"gravity"
};

/* ---------- public code */

/* ---------- private code */

/* ---------- items/projectiles.c: its functions */

real projectile_get_ballistic_acceleration(
	struct projectile_definition const *definition)
{
	return -(definition->projectile.air_gravity_scale * global_gravity);
}

boolean projectile_aim_ballistic(
	real base_velocity,
	real gravity_scale,
	real_point3d const *origin,
	real_point3d const *target_point,
	real *target_velocity_min,
	real *target_ballistic_fraction_min,
	real *forced_velocity,
	boolean lob,
	real_vector3d *result_aim_vector,
	real *result_velocity,
	real *result_ticks,
	real *result_distance,
	real *result_vertical_velocity,
	real *result_horizontal_velocity)
{
	boolean result = TRUE;
	boolean solution = FALSE;
	real_vector3d delta;
	real_vector3d aim_vector;
	real acceleration, a, b, c;
	real acceleration_height;
	real horizontal_distance_squared;
	real t_squared_max, t_max, v_min, t;
	real v_max = base_velocity;
	real v_desired, v_desired_sq;
	real distance, vertical_velocity, horizontal_velocity;

	delta.i = target_point->x - origin->x;
	delta.j = target_point->y - origin->y;
	delta.k = target_point->z - origin->z;

	horizontal_distance_squared = delta.i * delta.i + delta.j * delta.j;
	acceleration = MAX(0.f, global_gravity * gravity_scale);
	a = acceleration * acceleration * 0.25f;
	c = horizontal_distance_squared + delta.k * delta.k;

	match_assert(
		"c:\\halo\\SOURCE\\items\\projectiles.c",
		760,
		4.0f * a * c > 0.0f);
	b = -square_root(4.0f * a * c);
	t_squared_max = -(b / (2.0f * a));
	match_assert(
		"c:\\halo\\SOURCE\\items\\projectiles.c",
		764,
		t_squared_max >= 0.0f);
	t_max = square_root(t_squared_max);
	acceleration_height = acceleration * delta.k;
	v_min = acceleration_height - b < 0.0f
		? 0.0f
		: square_root(acceleration_height - b);

	if (forced_velocity)
	{
		v_desired = *forced_velocity;
	}
	else
	{
		v_desired = v_max;

		if (target_ballistic_fraction_min)
		{
			if (*target_ballistic_fraction_min > 0.0f)
			{
				real t_desired = t_max * *target_ballistic_fraction_min;
				real t_desired_squared = t_desired * t_desired;
				real b_desired =
					-(c / t_desired_squared + a * t_desired_squared);

				v_desired_sq = acceleration_height - b_desired;
				match_assert(
					"c:\\halo\\SOURCE\\items\\projectiles.c",
					806,
					v_desired_sq > 0.0f);
				if (v_max > square_root(v_desired_sq))
					v_desired = square_root(v_desired_sq);
			}
		}
	}

	if (v_desired >= v_min)
	{
		real b_desired = acceleration_height - v_desired * v_desired;
		real discriminant = b_desired * b_desired - 4.0f * a * c;

		if (b_desired < 0.0f && discriminant >= 0.0f)
		{
			real t_squared =
				(square_root(discriminant) * (lob ? 1 : -1) - b_desired) /
				(2.0f * a);

			if (t_squared > 0.0f)
			{
				t = square_root(t_squared);
				solution = TRUE;
			}
		}
	}

	if (!solution)
	{
		result = FALSE;
		t = t_max;
		v_desired = v_min;
	}

	aim_vector.i = delta.i / t;
	aim_vector.j = delta.j / t;
	aim_vector.k =
		delta.k / t + t * acceleration * 0.5f;
	horizontal_velocity = square_root(
		aim_vector.i * aim_vector.i + aim_vector.j * aim_vector.j);
	vertical_velocity = aim_vector.k;
	distance = t * v_desired;

	if (normalize3d(&aim_vector) == 0.0f)
	{
		result = FALSE;
		aim_vector = delta;
		if (normalize3d(&aim_vector) == 0.0f)
			aim_vector = *global_up3d;
	}

	match_assert(
		"c:\\halo\\SOURCE\\items\\projectiles.c",
		867,
		result_aim_vector);
	*result_aim_vector = aim_vector;

	if (result_distance)
		*result_distance = distance;
	if (result_velocity)
		*result_velocity = v_desired;
	if (result_vertical_velocity)
		*result_vertical_velocity = vertical_velocity;
	if (result_horizontal_velocity)
		*result_horizontal_velocity = horizontal_velocity;
	if (result_ticks)
		*result_ticks = t;

	return result;
}

boolean projectile_aim_linear(
	real base_velocity,
	real_point3d const *origin,
	real_point3d const *target_point,
	real_vector3d *result_aim_vector,
	real *result_velocity,
	real *result_ticks,
	real *result_distance)
{
	real_vector3d aim_vector;
	real distance;
	real ticks;

	aim_vector.i = target_point->x - origin->x;
	aim_vector.j = target_point->y - origin->y;
	aim_vector.k = target_point->z - origin->z;
	distance = normalize3d(&aim_vector);

	if (base_velocity > 0.0f)
		ticks = distance / base_velocity;
	else
		ticks = 0.0f;

	match_assert(
		"c:\\halo\\SOURCE\\items\\projectiles.c",
		921,
		result_aim_vector);
	*result_aim_vector = aim_vector;

	if (result_distance)
		*result_distance = distance;
	if (result_velocity)
		*result_velocity = base_velocity;
	if (result_ticks)
		*result_ticks = ticks;

	return TRUE;
}

boolean projectile_aim(
	struct projectile_definition const *definition,
	real_point3d const *origin,
	real_point3d const *target_point,
	real const *override_velocity_max,
	real *target_velocity_min,
	real *target_ballistic_fraction_min,
	real *forced_velocity,
	boolean lob,
	real_vector3d *result_aim_vector,
	real *result_velocity,
	real *result_ticks,
	real *result_distance,
	boolean *result_linear)
{
	real base_velocity;
	boolean result;

	if (!override_velocity_max)
		base_velocity = definition->projectile.initial_velocity;
	else
		base_velocity = *override_velocity_max;

	if (TEST_FLAG(definition->projectile.flags, _projectile_aim_ballistic_bit) &&
		definition->projectile.air_gravity_scale > 0.0f)
	{
		result = projectile_aim_ballistic(
			base_velocity,
			definition->projectile.air_gravity_scale,
			origin,
			target_point,
			target_velocity_min,
			target_ballistic_fraction_min,
			forced_velocity,
			lob,
			result_aim_vector,
			result_velocity,
			result_ticks,
			result_distance,
			NULL,
			NULL);

		if (result_linear)
			*result_linear = FALSE;
	}
	else
	{
		result = projectile_aim_linear(
			base_velocity,
			origin,
			target_point,
			result_aim_vector,
			result_velocity,
			result_ticks,
			result_distance);

		if (result_linear)
			*result_linear = TRUE;
	}

	return result;
}

real projectile_estimate_time_to_target(
	struct projectile_definition const *definition,
	real target_distance)
{
	real time_to_target = 0.0f;

	if (definition->projectile.initial_velocity > 0.0f)
		time_to_target = target_distance / definition->projectile.initial_velocity;

	return time_to_target;
}

} // namespace h1_ai

#include "stdafx.h"
#include "h1_projectile_logic.h"

#include "h1_cache_file.h"
#include "h1_effects.h"
#include "h1_log.h"
#include "h1_map_loader.h"
#include "h1_objects.h"
#include "h1_projectiles.h"
#include "h1_render_shaders.h"
#include "h1_structure_bsp.h"

#include "game/game_time.h"
#include "math/real_math.h"
#include "objects/damage.h"
#include "objects/object_types.h"
#include "objects/objects.h"
#include "physics/collisions.h"
#include "units/units.h"

#include <unordered_map>

/* constants */

enum
{
	k_h1_ticks_per_second = 30,
	k_h1_maximum_projectile_collisions_per_update = 10,
	k_h1_maximum_combining_projectiles = 6,
	k_h1_material_type_count = 33,
	k_h1_impact_markers = 5,
};

// halo 1 gravity a tick squared (physics.c global_gravity)
static const real32 k_h1_gravity = 0.0035651792f;

// projectiles.c projectile datum flags
enum
{
	_h1_projectile_tracer_bit = 1,
	_h1_projectile_collided_once_bit = 2,
	_h1_projectile_attached_bit = 3,
	_h1_projectile_stopped_after_collision_bit = 4,
	_h1_projectile_counting_down_bit = 5,
	_h1_projectile_already_super_exploded_bit = 6,
	_h1_projectile_will_super_explode_bit = 7,
	// halo 1's object flags kept here: at rest, wholly under media, nonzero angular velocity
	_h1_projectile_at_rest_bit = 16,
	_h1_projectile_under_media_bit = 17,
	_h1_projectile_has_nonzero_angular_velocity_bit = 18,
};

// projectile_definitions.h
enum
{
	_h1_projectile_oriented_along_velocity_bit = 0,
	_h1_projectile_aim_ballistic_bit,
	_h1_projectile_detonation_max_time_if_attached_bit,
	_h1_projectile_super_combining_explosion_bit,
};

enum
{
	_h1_projectile_action_none = 0,
	_h1_projectile_action_detonate,
	_h1_projectile_action_disappear,
};

enum
{
	_h1_detonation_timer_starts_immediately = 0,
	_h1_detonation_timer_starts_after_first_bounce,
	_h1_detonation_timer_starts_when_at_rest,
};

enum
{
	_h1_material_response_disappear = 0,
	_h1_material_response_detonate,
	_h1_material_response_reflect,
	_h1_material_response_overpenetrate,
	_h1_material_response_attach,
};

enum
{
	_h1_material_effect_scale_damage = 0,
	_h1_material_effect_scale_angle,
};

enum
{
	_h1_potential_response_only_against_units_bit = 0,
};

enum
{
	_h1_projectile_export_function_none = 0,
	_h1_projectile_export_function_range_remaining,
	_h1_projectile_export_function_time_remaining,
	_h1_projectile_export_function_tracer,
};

// projectiles.c effect markers
enum
{
	_h1_effect_vector_normal = 0,
	_h1_effect_vector_incident,
	_h1_effect_vector_negative_incident,
	_h1_effect_vector_reflected,
	_h1_effect_vector_gravity,
};
static const char* const k_h1_impact_marker_names[k_h1_impact_markers] = { "normal", "incident", "negative incident", "reflection", "gravity" };
static const char* const k_h1_detonation_marker_names[2] = { "", "gravity" };

// halo 2's collision results
enum
{
	_h2_collision_result_none = 0,
	_h2_collision_result_structure,
	_h2_collision_result_media,
	_h2_collision_result_instanced_geometry,
	_h2_collision_result_object,
};

// halo 2's projectile collision test (FUN_005464ca): structure, media, instanced geometry, objects and its own
static const uint32 k_h2_projectile_collision_flags = 0x2480000F;

/* structures */

// projectiles.c _projectile_datum, with halo 1's velocities (a tick's) and the object flags halo 2 doesn't keep
struct s_h1_projectile
{
	datum definition_index;	// halo 1's
	uint32 flags;
	int16 action;
	int16 hit_material_type;
	datum ignore_object_index;
	datum target_object_index;
	real32 detonation_timer;
	real32 detonation_timer_delta;
	real32 arming_time;
	real32 arming_time_delta;
	real32 odometer;
	real32 deceleration_timer;
	real32 deceleration_timer_delta;
	real32 deceleration;
	real32 maximum_damage_distance;
	real_vector3d rotation_axis;
	real32 rotation_sine;
	real32 rotation_cosine;
	real_vector3d velocity;
	real_vector3d angular_velocity;
};

// the collision_result halo 1's projectile code reads, from halo 2's
struct s_h1_collision
{
	int32 type;
	real32 t;
	real_point3d point;
	real_vector3d normal;
	int16 material_type;	// halo 1's
	datum object_index;
	int16 node_index;
	int16 region_index;
	int16 material_index;	// the object's (halo 2's)
	s_location location;
};

/* globals */

static object_update_t g_h2_projectile_update = NULL;
static std::unordered_map<datum, s_h1_projectile> g_h1_projectiles;
static uint32 g_h1_projectile_random_seed = 0x9E3779B9u;

/* prototypes */

static bool h1_projectile_update_hook(datum projectile_index);
static s_h1_projectile* h1_projectile_get(datum projectile_index, datum* h1_definition_index);
static void h1_projectile_initialize(datum projectile_index, s_h1_projectile* projectile, const real_vector3d* velocity);
static void h1_projectile_update(datum projectile_index, s_h1_projectile* projectile);
static void h1_projectile_set_action(s_h1_projectile* projectile, int16 action);
static void h1_projectile_adjust_for_angular_velocity_change(s_h1_projectile* projectile);
static void h1_projectile_calculate_deceleration(s_h1_projectile* projectile, const h1_proj* definition);
static bool h1_projectile_collision_test_line(datum projectile_index, const s_h1_projectile* projectile, const h1_proj* definition,
	const real_point3d* new_position, s_h1_collision* collision);
static void h1_projectile_collision(datum projectile_index, s_h1_projectile* projectile, const h1_proj* definition, s_h1_collision* collision,
	real_point3d* new_position, real_vector3d* new_velocity);
static void h1_projectile_detonate(datum projectile_index, s_h1_projectile* projectile, const h1_proj* definition);
static void h1_projectile_effect_new(datum projectile_index, datum effect_index, const s_h1_collision* collision, const real_point3d* points,
	const real_vector3d* forwards, real32 scale, real32 material_effect_scale);
static const h1_proj_material_responses* h1_projectile_material_response(const h1_proj* definition, int16 material_type);
static void h1_object_move(datum object_index, const real_point3d* position, const real_vector3d* forward, const real_vector3d* up);
static void h1_object_set_velocities(datum object_index, const s_h1_projectile* projectile);
static int16 h1_global_material_to_material_type(int16 global_material_index);
static datum h1_object_owner_get(datum object_index);
static real32 h1_projectile_random(void);
static real32 h1_projectile_random_range(real32 lower, real32 upper);
static void h1_rotate_vector_about_axis(real_vector3d* vector, const real_vector3d* axis, real32 sine, real32 cosine);
static void h1_random_vector_in_cone(const real_vector3d* axis, real32 angle, real_vector3d* result);
static real32 h1_magnitude(const real_vector3d* vector);
static real32 h1_normalize(real_vector3d* vector);
static real32 h1_dot(const real_vector3d* a, const real_vector3d* b);
static real_vector3d h1_cross(const real_vector3d* a, const real_vector3d* b);

/* public code */

void h1_projectile_logic_apply_patches(void)
{
	// the projectile part of the projectile object type: halo 1's projectiles run halo 1's update instead of halo 2's
	object_type_definition* projectile_type = object_type_definition_get(_object_type_projectile);
	for (int32 i = 0; i < k_max_object_type_inheritence; i++)
	{
		object_type_definition* part = projectile_type->part_definitions[i];
		if (part && part->group_tag == 'proj' && part->object_update)
		{
			g_h2_projectile_update = part->object_update;
			part->object_update = h1_projectile_update_hook;
			break;
		}
	}
	return;
}

void h1_projectile_logic_reset(void)
{
	g_h1_projectiles.clear();
	return;
}

void h1_projectile_logic_new(datum projectile_index, real32 inherited_velocity, datum target_object_index, bool tracer)
{
	datum h1_definition_index = NONE;
	s_h1_projectile* projectile = h1_projectile_get(projectile_index, &h1_definition_index);
	const object_datum* object = (const object_datum*)object_try_and_get_and_verify_type(projectile_index, _object_mask_projectile);
	if (!projectile || !object)
	{
		return;
	}
	const h1_proj* definition = (const h1_proj*)g_h1_cache_file->tag_get('proj', h1_definition_index);
	const real32 speed = inherited_velocity + definition->initial_velocity;
	const real_vector3d velocity = { object->object.forward.i * speed, object->object.forward.j * speed, object->object.forward.k * speed };
	h1_projectile_initialize(projectile_index, projectile, &velocity);
	projectile->target_object_index = target_object_index;
	SET_BIT(projectile->flags, _h1_projectile_tracer_bit, tracer);
	return;
}

void h1_projectile_logic_area_damage(datum h1_damage_effect_index, datum owner_object_index, const real_point3d* point, const real_vector3d* forward,
	real32 scale)
{
	typedef void(__cdecl* t_damage_data_new)(s_damage_data* damage, datum definition_index);
	typedef void(__cdecl* t_damage_owner_from_object)(datum object_index, s_damage_owner* owner);
	typedef int32(__cdecl* t_area_of_effect_cause_damage)(s_damage_data* damage, datum ignore_object_index);

	const datum definition_index = h1_damage_effect_build(h1_damage_effect_index);
	if (definition_index == NONE)
	{
		return;
	}
	s_damage_data damage;
	Memory::GetAddress<t_damage_data_new>(0x175BAC)(&damage, definition_index);
	if (object_try_and_get(owner_object_index))
	{
		Memory::GetAddress<t_damage_owner_from_object>(0x175C14)(owner_object_index, &damage.owner);
	}
	damage.scale = scale;
	damage.origin = *point;
	damage.epicenter = *point;
	damage.direction = *forward;
	Memory::GetAddress<t_area_of_effect_cause_damage>(0x17868D)(&damage, NONE);
	return;
}

bool h1_projectile_logic_function_value(datum object_index, int16 function_input, real32* value)
{
	datum h1_definition_index = NONE;
	auto found = g_h1_projectiles.find(object_index);
	if (found == g_h1_projectiles.end() || !h1_projectile_get(object_index, &h1_definition_index))
	{
		return false;
	}
	const s_h1_projectile* projectile = &found->second;
	const h1_proj* definition = (const h1_proj*)g_h1_cache_file->tag_get('proj', h1_definition_index);
	switch (function_input)
	{
	case _h1_projectile_export_function_range_remaining:
		*value = definition->maximum_range != 0.f ? projectile->odometer / definition->maximum_range : 0.f;
		return true;
	case _h1_projectile_export_function_time_remaining:
		*value = projectile->detonation_timer;
		return true;
	case _h1_projectile_export_function_tracer:
		*value = TEST_BIT(projectile->flags, _h1_projectile_tracer_bit) ? 1.f : 0.f;
		return true;
	default:
		return false;
	}
}

/* private code */

static bool h1_projectile_update_hook(datum projectile_index)
{
	datum h1_definition_index = NONE;
	s_h1_projectile* projectile = h1_maps_active() ? h1_projectile_get(projectile_index, &h1_definition_index) : NULL;
	if (!projectile)
	{
		return g_h2_projectile_update(projectile_index);
	}
	if (projectile->definition_index == NONE)
	{
		// thrown by halo 2 (grenades): the forward times the halo 1 initial velocity plus what the thrower gave it, halo 2's velocities
		// being a second's
		const object_datum* object = object_get(projectile_index);
		const h1_proj* definition = (const h1_proj*)g_h1_cache_file->tag_get('proj', h1_definition_index);
		const real_vector3d& forward = object->object.forward;
		const real_vector3d& thrown = object->object.translational_velocity;
		const real_vector3d velocity =
		{
			forward.i * definition->initial_velocity + (thrown.i - forward.i * definition->initial_velocity) / k_h1_ticks_per_second,
			forward.j * definition->initial_velocity + (thrown.j - forward.j * definition->initial_velocity) / k_h1_ticks_per_second,
			forward.k * definition->initial_velocity + (thrown.k - forward.k * definition->initial_velocity) / k_h1_ticks_per_second
		};
		h1_projectile_initialize(projectile_index, projectile, &velocity);
	}
	h1_projectile_update(projectile_index, projectile);
	return true;
}

// the halo 1 projectile state of a halo 2 projectile built from a halo 1 projectile (made empty, its definition NONE, when new)
static s_h1_projectile* h1_projectile_get(datum projectile_index, datum* h1_definition_index)
{
	const object_datum* object = (const object_datum*)object_try_and_get_and_verify_type(projectile_index, _object_mask_projectile);
	*h1_definition_index = object ? h1_objects_h1_definition_get(object->definition_index) : NONE;
	if (*h1_definition_index == NONE || !g_h1_cache_file->tag_get('proj', *h1_definition_index))
	{
		return NULL;
	}
	auto found = g_h1_projectiles.find(projectile_index);
	if (found != g_h1_projectiles.end())
	{
		return &found->second;
	}
	// forget the projectiles that are gone now and then
	if (g_h1_projectiles.size() >= 256)
	{
		for (auto it = g_h1_projectiles.begin(); it != g_h1_projectiles.end();)
		{
			it = object_try_and_get_and_verify_type(it->first, _object_mask_projectile) ? std::next(it) : g_h1_projectiles.erase(it);
		}
	}
	s_h1_projectile* projectile = &g_h1_projectiles[projectile_index];
	csmemset(projectile, 0, sizeof(*projectile));
	projectile->definition_index = NONE;
	return projectile;
}

// projectiles.c projectile_new
static void h1_projectile_initialize(datum projectile_index, s_h1_projectile* projectile, const real_vector3d* velocity)
{
	datum h1_definition_index = NONE;
	h1_projectile_get(projectile_index, &h1_definition_index);
	const h1_proj* definition = (const h1_proj*)g_h1_cache_file->tag_get('proj', h1_definition_index);
	const object_datum* object = object_get(projectile_index);

	csmemset(projectile, 0, sizeof(*projectile));
	projectile->definition_index = h1_definition_index;
	projectile->flags = FLAG(_h1_projectile_tracer_bit);
	projectile->target_object_index = NONE;
	projectile->action = _h1_projectile_action_none;
	projectile->hit_material_type = NONE;
	const datum owner_object_index = h1_object_owner_get(projectile_index);
	projectile->ignore_object_index = owner_object_index != NONE ? object_get_ultimate_parent(owner_object_index) : NONE;

	real32 detonation_ticks = TEST_BIT(definition->flags_2, _h1_projectile_detonation_max_time_if_attached_bit) ?
		definition->timer.lower : h1_projectile_random_range(definition->timer.lower, definition->timer.upper);
	detonation_ticks *= k_h1_ticks_per_second;
	if (detonation_ticks >= 1.f)
	{
		projectile->detonation_timer_delta = 1.f / detonation_ticks;
	}
	const real32 arming_ticks = definition->arming_time * k_h1_ticks_per_second;
	if (arming_ticks >= 1.f)
	{
		projectile->arming_time_delta = 1.f / arming_ticks;
	}

	projectile->velocity = *velocity;
	projectile->angular_velocity =
	{
		object->object.angular_velocity.i / k_h1_ticks_per_second,
		object->object.angular_velocity.j / k_h1_ticks_per_second,
		object->object.angular_velocity.k / k_h1_ticks_per_second
	};
	h1_projectile_adjust_for_angular_velocity_change(projectile);
	h1_projectile_calculate_deceleration(projectile, definition);
	h1_object_set_velocities(projectile_index, projectile);
	return;
}

// projectiles.c projectile_update for a halo 2 tick: a halo 1 tick's motion scaled to the tick's length
static void h1_projectile_update(datum projectile_index, s_h1_projectile* projectile)
{
	const h1_proj* definition = (const h1_proj*)g_h1_cache_file->tag_get('proj', projectile->definition_index);
	const real32 tick_fraction = game_tick_length() * k_h1_ticks_per_second;
	real32 time_remaining = tick_fraction;
	int16 collision_count = 0;

	projectile->arming_time += projectile->arming_time_delta * tick_fraction;
	projectile->deceleration_timer += projectile->deceleration_timer_delta * tick_fraction;

	bool detonation_timer_running;
	switch (definition->detonation_timer_starts)
	{
	case _h1_detonation_timer_starts_after_first_bounce:
		// (falls through to the next case in halo 1)
	case _h1_detonation_timer_starts_when_at_rest:
		detonation_timer_running = TEST_BIT(projectile->flags, _h1_projectile_stopped_after_collision_bit);
		break;
	default:
		detonation_timer_running = true;
		break;
	}
	if (TEST_BIT(projectile->flags, _h1_projectile_counting_down_bit) || TEST_BIT(projectile->flags, _h1_projectile_attached_bit))
	{
		detonation_timer_running = true;
	}
	if (detonation_timer_running)
	{
		SET_BIT(projectile->flags, _h1_projectile_counting_down_bit, true);
		projectile->detonation_timer += projectile->detonation_timer_delta * tick_fraction;
		if (projectile->detonation_timer >= 1.f)
		{
			h1_projectile_set_action(projectile, _h1_projectile_action_detonate);
		}
	}

	const object_datum* object = object_get(projectile_index);
	while (time_remaining > 0.f &&
		(projectile->action == _h1_projectile_action_none ||
			(projectile->action == _h1_projectile_action_detonate && projectile->arming_time_delta != 0.f && projectile->arming_time < 1.f)) &&
		!TEST_BIT(projectile->flags, _h1_projectile_attached_bit) &&
		!TEST_BIT(projectile->flags, _h1_projectile_at_rest_bit) &&
		object->object.parent_object_index == NONE)
	{
		const real32 speed = h1_magnitude(&projectile->velocity);
		real_vector3d new_velocity = projectile->velocity;
		real_vector3d average_velocity = projectile->velocity;
		real32 final_speed = speed;
		real32 average_speed = speed;
		bool moved = false;

		// guidance toward the target, wandering when far
		if (projectile->target_object_index != NONE && definition->guided_angular_velocity > 0.f && object_try_and_get(projectile->target_object_index))
		{
			const object_datum* target = object_get(projectile->target_object_index);
			const real32 angular_velocity = definition->guided_angular_velocity * (1.f / k_h1_ticks_per_second) * tick_fraction;
			const real_vector3d to_target =
			{
				target->object.center.x - object->object.center.x,
				target->object.center.y - object->object.center.y,
				target->object.center.z - object->object.center.z
			};
			const real32 target_distance = h1_magnitude(&to_target);
			const real32 wander_scale = target_distance > 10.f ? 1.f : target_distance > 2.f ? PIN((target_distance - 2.f) * 0.125f, 0.f, 1.f) : 0.f;
			real_point3d target_point;
			object_get_center_of_mass(projectile->target_object_index, &target_point);
			const int32 identifier = (int32)((projectile_index >> 16) & 0xFFFF);
			const real32 wander_yaw = _pi - h1_periodic_function_evaluate(10, (real32)((game_time_get() + 3 * identifier) & 0xFFFF) * (1.f / 90.f)) * (_pi / 2.f);
			const real32 wander_pitch = h1_periodic_function_evaluate(10, (real32)((game_time_get() + 7 * identifier) & 0xFFFF) * (1.f / 90.f)) * (_pi * 2.f);
			const real_vector3d wander_direction = { cosf(wander_yaw) * cosf(wander_pitch), sinf(wander_yaw) * cosf(wander_pitch), sinf(wander_pitch) };
			target_point.x += wander_direction.i * wander_scale;
			target_point.y += wander_direction.j * wander_scale;
			target_point.z += wander_direction.k * wander_scale;
			const real_vector3d target_vector =
			{
				target_point.x - object->object.position.x, target_point.y - object->object.position.y, target_point.z - object->object.position.z
			};
			real_vector3d rotation_axis = h1_cross(&projectile->velocity, &target_vector);
			if (h1_dot(&target_vector, &projectile->velocity) > 0.f && h1_normalize(&rotation_axis) > 0.f)
			{
				h1_rotate_vector_about_axis(&new_velocity, &rotation_axis, sinf(angular_velocity), cosf(angular_velocity));
			}
		}

		// deceleration to the final velocity over the damage range
		if (projectile->deceleration_timer >= 1.f)
		{
			if (speed > definition->final_velocity && projectile->deceleration != 0.f)
			{
				final_speed = speed - projectile->deceleration * time_remaining;
				if (final_speed <= definition->final_velocity)
				{
					const real32 fraction = (speed - definition->final_velocity) / (projectile->deceleration * time_remaining);
					final_speed = definition->final_velocity * 0.99f;
					average_speed = (final_speed + speed) * fraction * 0.5f + (1.f - fraction) * definition->final_velocity;
					const real32 scale = final_speed / speed;
					new_velocity = { new_velocity.i * scale, new_velocity.j * scale, new_velocity.k * scale };
					average_velocity.i = (new_velocity.i + projectile->velocity.i) * fraction * 0.5f + (1.f - fraction) * new_velocity.i;
					average_velocity.j = (new_velocity.j + projectile->velocity.j) * fraction * 0.5f + (1.f - fraction) * new_velocity.j;
					average_velocity.k = (new_velocity.k + projectile->velocity.k) * fraction * 0.5f + (1.f - fraction) * new_velocity.k;
				}
				else
				{
					average_speed = speed - projectile->deceleration * time_remaining * 0.5f;
					const real32 scale = final_speed / speed;
					new_velocity = { new_velocity.i * scale, new_velocity.j * scale, new_velocity.k * scale };
					average_velocity.i = (new_velocity.i + projectile->velocity.i) * 0.5f;
					average_velocity.j = (new_velocity.j + projectile->velocity.j) * 0.5f;
					average_velocity.k = (new_velocity.k + projectile->velocity.k) * 0.5f;
				}
			}
			else if (definition->maximum_range == 0.f && definition->timer.upper == 0.f && definition->minimum_velocity <= definition->final_velocity &&
				(projectile->deceleration != 0.f || projectile->odometer >= projectile->maximum_damage_distance))
			{
				h1_projectile_set_action(projectile, _h1_projectile_action_disappear);
			}
			else if (speed < definition->final_velocity && speed > 0.f)
			{
				const real32 scale = definition->final_velocity / speed * 0.99f;
				new_velocity = { new_velocity.i * scale, new_velocity.j * scale, new_velocity.k * scale };
			}
		}

		// gravity
		const real32 gravity_acceleration = k_h1_gravity *
			(TEST_BIT(projectile->flags, _h1_projectile_under_media_bit) ? definition->water_gravity_scale : definition->air_gravity_scale);
		new_velocity.k -= gravity_acceleration * time_remaining;
		average_velocity.k -= gravity_acceleration * time_remaining * 0.5f;

		// the maximum range
		real32 distance_fraction = 1.f;
		if (definition->maximum_range != 0.f && average_speed * time_remaining + projectile->odometer > definition->maximum_range)
		{
			distance_fraction = average_speed != 0.f ? (definition->maximum_range - projectile->odometer) / average_speed * time_remaining : 0.f;
			h1_projectile_set_action(projectile, _h1_projectile_action_detonate);
		}

		const real32 step = distance_fraction * time_remaining;
		real_point3d new_position =
		{
			object->object.position.x + average_velocity.i * step,
			object->object.position.y + average_velocity.j * step,
			object->object.position.z + average_velocity.k * step
		};

		s_h1_collision collision;
		if (collision_count != k_h1_maximum_projectile_collisions_per_update && projectile->action != _h1_projectile_action_disappear &&
			(moved = true) && h1_projectile_collision_test_line(projectile_index, projectile, definition, &new_position, &collision))
		{
			time_remaining = time_remaining * (1.f - collision.t);
			new_velocity.k += gravity_acceleration * time_remaining;
			if (final_speed != 0.f)
			{
				const real32 restored_speed = MIN(time_remaining * projectile->deceleration + final_speed, speed);
				const real32 scale = restored_speed / final_speed;
				new_velocity = { new_velocity.i * scale, new_velocity.j * scale, new_velocity.k * scale };
			}
			if (collision.normal.k > 0.3f)
			{
				SET_BIT(projectile->flags, _h1_projectile_collided_once_bit, true);
			}
			projectile->ignore_object_index = NONE;
			h1_projectile_collision(projectile_index, projectile, definition, &collision, &new_position, &new_velocity);
			collision_count++;
			if (TEST_BIT(projectile->flags, _h1_projectile_attached_bit))
			{
				moved = false;
			}
		}
		else
		{
			if (collision_count == k_h1_maximum_projectile_collisions_per_update)
			{
				h1_projectile_set_action(projectile, _h1_projectile_action_detonate);
			}
			time_remaining = 0.f;
		}

		if (moved)
		{
			const real_vector3d movement =
			{
				new_position.x - object->object.position.x, new_position.y - object->object.position.y, new_position.z - object->object.position.z
			};
			projectile->odometer += h1_magnitude(&movement);

			// the orientation: along the velocity, or turned by the angular velocity
			real_vector3d forward = object->object.forward;
			real_vector3d up = object->object.up;
			if (TEST_BIT(definition->flags_2, _h1_projectile_oriented_along_velocity_bit) &&
				(projectile->velocity.i != 0.f || projectile->velocity.j != 0.f || projectile->velocity.k != 0.f))
			{
				real_vector3d along = projectile->velocity;
				if (h1_normalize(&along) > 0.f)
				{
					forward = along;
					const real_vector3d left = h1_cross(&up, &forward);
					up = h1_cross(&forward, &left);
					if (h1_normalize(&up) == 0.f)
					{
						perpendicular3d(&forward, &up);
						h1_normalize(&up);
					}
				}
				h1_rotate_vector_about_axis(&up, &forward, projectile->rotation_sine, projectile->rotation_cosine);
			}
			else if (TEST_BIT(projectile->flags, _h1_projectile_has_nonzero_angular_velocity_bit))
			{
				h1_rotate_vector_about_axis(&forward, &projectile->rotation_axis, projectile->rotation_sine, projectile->rotation_cosine);
				h1_rotate_vector_about_axis(&up, &projectile->rotation_axis, projectile->rotation_sine, projectile->rotation_cosine);
				h1_normalize(&forward);
				const real_vector3d left = h1_cross(&up, &forward);
				up = h1_cross(&forward, &left);
				h1_normalize(&up);
			}
			h1_object_move(projectile_index, &new_position, &forward, &up);
			projectile->velocity = new_velocity;
			h1_object_set_velocities(projectile_index, projectile);
			object = object_get(projectile_index);
		}
	}

	switch (projectile->action)
	{
	case _h1_projectile_action_detonate:
		if (projectile->arming_time_delta != 0.f && projectile->arming_time < 1.f)
		{
			break;
		}
		h1_projectile_detonate(projectile_index, projectile, definition);
		g_h1_projectiles.erase(projectile_index);
		object_delete(projectile_index);
		break;
	case _h1_projectile_action_disappear:
		g_h1_projectiles.erase(projectile_index);
		object_delete(projectile_index);
		break;
	}
	return;
}

static void h1_projectile_set_action(s_h1_projectile* projectile, int16 action)
{
	if (action > projectile->action)
	{
		projectile->action = action;
	}
	return;
}

// projectiles.c projectile_adjust_for_angular_velocity_change
static void h1_projectile_adjust_for_angular_velocity_change(s_h1_projectile* projectile)
{
	const real32 magnitude = h1_magnitude(&projectile->angular_velocity);
	if (magnitude != 0.f)
	{
		SET_BIT(projectile->flags, _h1_projectile_has_nonzero_angular_velocity_bit, true);
		projectile->rotation_axis =
		{
			projectile->angular_velocity.i / magnitude, projectile->angular_velocity.j / magnitude, projectile->angular_velocity.k / magnitude
		};
		projectile->rotation_sine = sinf(magnitude);
		projectile->rotation_cosine = cosf(magnitude);
	}
	else
	{
		SET_BIT(projectile->flags, _h1_projectile_has_nonzero_angular_velocity_bit, false);
		projectile->rotation_sine = 0.f;
		projectile->rotation_cosine = 1.f;
	}
	return;
}

// projectiles.c projectile_calculate_deceleration (the maximum damage distance is the water range's in air too, as in halo 1)
static void h1_projectile_calculate_deceleration(s_h1_projectile* projectile, const h1_proj* definition)
{
	const bool under_media = TEST_BIT(projectile->flags, _h1_projectile_under_media_bit);
	const real_bounds& range = under_media ? definition->water_damage_range : definition->air_damage_range;
	const real32 distance_delta = range.upper - range.lower;
	projectile->deceleration = definition->initial_velocity != definition->final_velocity && distance_delta != 0.f ?
		(definition->initial_velocity * definition->initial_velocity - definition->final_velocity * definition->final_velocity) / (2.f * distance_delta) : 0.f;
	projectile->maximum_damage_distance = definition->water_damage_range.upper;
	if (range.lower > 0.f && definition->initial_velocity != 0.f)
	{
		projectile->deceleration_timer_delta = range.lower / definition->initial_velocity;
		return;
	}
	projectile->deceleration_timer = 1.f;
	projectile->deceleration_timer_delta = 0.f;
	return;
}

// projectiles.c projectile_collision_test_line: the line, then two lines the collision radius to either side
static bool h1_projectile_collision_test_line(datum projectile_index, const s_h1_projectile* projectile, const h1_proj* definition,
	const real_point3d* new_position, s_h1_collision* collision)
{
	const object_datum* object = object_get(projectile_index);
	auto test = [&](const real_point3d* p0, const real_point3d* p1) -> bool
	{
		collision_result result;
		csmemset(&result, 0, sizeof(result));
		if (!collision_test_line(k_h2_projectile_collision_flags, p0, p1, projectile->ignore_object_index, projectile_index, &result))
		{
			return false;
		}
		collision->type = result.type;
		collision->t = result.t;
		collision->point = result.point;
		collision->normal = result.fog_plane.n;
		collision->material_type = h1_global_material_to_material_type(result.global_material_index);
		collision->object_index = result.type == _h2_collision_result_object ? result.object_index : NONE;
		collision->node_index = result.matrix_index;
		collision->region_index = result.field_44;
		collision->material_index = result.field_5A;
		collision->location = result.locations[0];
		return true;
	};
	if (test(&object->object.position, new_position))
	{
		return true;
	}
	if (definition->collision_radius < k_real_epsilon)
	{
		return false;
	}
	real_vector3d forward =
	{
		new_position->x - object->object.position.x, new_position->y - object->object.position.y, new_position->z - object->object.position.z
	};
	const real_vector3d up = { 0.f, 0.f, 1.f };
	real_vector3d left = h1_cross(&up, &forward);
	if (h1_normalize(&left) == 0.f)
	{
		left = { 0.f, 1.f, 0.f };
	}
	const real32 radius = definition->collision_radius;
	const real_point3d& p0 = object->object.position;
	const real_point3d p0_left = { p0.x + left.i * radius, p0.y + left.j * radius, p0.z + left.k * radius };
	const real_point3d p1_left = { new_position->x + left.i * radius, new_position->y + left.j * radius, new_position->z + left.k * radius };
	const real_point3d p0_right = { p0.x - left.i * radius, p0.y - left.j * radius, p0.z - left.k * radius };
	const real_point3d p1_right = { new_position->x - left.i * radius, new_position->y - left.j * radius, new_position->z - left.k * radius };
	return test(&p0_left, &p1_left) || test(&p0_right, &p1_right);
}

// projectiles.c projectile_collision: the impact damage, the material's response (disappear, detonate, reflect, overpenetrate,
// attach) and the impact effects
static void h1_projectile_collision(datum projectile_index, s_h1_projectile* projectile, const h1_proj* definition, s_h1_collision* collision,
	real_point3d* new_position, real_vector3d* new_velocity)
{
	int16 material_type = collision->material_type;
	real32 effect_scale = 1.f;
	real32 material_effect_scale = 0.f;

	real_vector3d direction = *new_velocity;
	const real32 speed = h1_normalize(&direction);
	if (speed == 0.f)
	{
		direction = { 0.f, 0.f, 1.f };
	}
	real32 damage_scale = 1.f;
	if (definition->final_velocity != definition->initial_velocity)
	{
		damage_scale = PIN((speed - definition->final_velocity) / (definition->initial_velocity - definition->final_velocity), 0.f, 1.f);
	}

	// the impact damage of the object hit
	if (collision->type == _h2_collision_result_object && definition->impact_damage.index != NONE)
	{
		typedef void(__cdecl* t_damage_data_new)(s_damage_data* damage, datum definition_index);
		typedef void(__cdecl* t_damage_owner_from_object)(datum object_index, s_damage_owner* owner);
		const datum damage_definition_index = h1_damage_effect_build(definition->impact_damage.index);
		const datum owner_object_index = h1_object_owner_get(projectile_index);
		if (damage_definition_index != NONE)
		{
			s_damage_data damage;
			Memory::GetAddress<t_damage_data_new>(0x175BAC)(&damage, damage_definition_index);
			damage.flags = (e_damage_data_flags)(damage.flags | FLAG(_damage_from_weapon_bit));
			if (object_try_and_get(owner_object_index))
			{
				Memory::GetAddress<t_damage_owner_from_object>(0x175C14)(owner_object_index, &damage.owner);
			}
			damage.scale = damage_scale;
			damage.epicenter = collision->point;
			damage.origin = collision->point;
			damage.direction = direction;
			damage.material_type = NONE;
			object_cause_damage(&damage, collision->object_index, collision->node_index, collision->region_index, collision->material_index, &collision->normal);
			// (the shield or body material halo 2 took the damage with)
			const int16 damage_material_type = h1_global_material_to_material_type(damage.material_type);
			if (damage_material_type != NONE)
			{
				material_type = damage_material_type;
			}
			material_effect_scale = damage.material_effect_scale;
		}
	}

	projectile->hit_material_type = material_type;
	const h1_proj_material_responses* material_response = h1_projectile_material_response(definition, material_type);
	const real32 velocity_noise = material_response ? material_response->velocity_noise : 0.f;
	const real32 angular_noise = material_response ? material_response->angular_noise : 0.f;

	const real32 impact_velocity = -h1_dot(new_velocity, &collision->normal) + h1_projectile_random_range(-velocity_noise, 0.f);
	real_vector3d velocity_direction = *new_velocity;
	h1_normalize(&velocity_direction);
	const real32 impact_angle = acosf(PIN(h1_dot(&velocity_direction, &collision->normal), -1.f, 1.f)) - _pi / 2.f +
		h1_projectile_random_range(-angular_noise, angular_noise);

	int16 response = _h1_material_response_disappear;
	datum effect_definition_index = NONE;
	if (material_response)
	{
		const bool potential =
			material_response->response_2 != 0 &&
			(material_response->between.upper == 0.f || (impact_angle >= material_response->between.lower && impact_angle <= material_response->between.upper)) &&
			(material_response->and.upper == 0.f || (impact_velocity >= material_response->and.lower && impact_velocity <= material_response->and.upper)) &&
			(!TEST_BIT(material_response->flags_2, _h1_potential_response_only_against_units_bit) ||
				(collision->type == _h2_collision_result_object && object_try_and_get_and_verify_type(collision->object_index, _object_mask_unit))) &&
			h1_projectile_random() >= material_response->skip_fraction;
		response = potential ? material_response->response_2 : material_response->response;
		effect_definition_index = potential ? material_response->effect_2.index : material_response->effect.index;
	}

	*new_position = collision->point;

	if (response == _h1_material_response_overpenetrate)
	{
		if (collision->type == _h2_collision_result_media)
		{
			SET_BIT(projectile->flags, _h1_projectile_under_media_bit, !TEST_BIT(projectile->flags, _h1_projectile_under_media_bit));
			h1_projectile_calculate_deceleration(projectile, definition);
			new_position->x -= collision->normal.i * 0.001f;
			new_position->y -= collision->normal.j * 0.001f;
			new_position->z -= collision->normal.k * 0.001f;
		}
		else if (collision->type == _h2_collision_result_object)
		{
			const real32 scale = 1.f - (material_response ? material_response->initial_friction : 0.f);
			*new_velocity = { new_velocity->i * scale, new_velocity->j * scale, new_velocity->k * scale };
			projectile->ignore_object_index = collision->object_index;
		}
		else if (definition->timer.upper != 0.f)
		{
			SET_BIT(projectile->flags, _h1_projectile_collided_once_bit, true);
			SET_BIT(projectile->flags, _h1_projectile_stopped_after_collision_bit, true);
			response = _h1_material_response_attach;
		}
		else
		{
			response = _h1_material_response_detonate;
		}
	}

	if (response == _h1_material_response_reflect)
	{
		// component_vectors_from_direction3d: the part along the normal and the rest
		const real32 along = h1_dot(new_velocity, &collision->normal);
		const real_vector3d parallel = { collision->normal.i * along, collision->normal.j * along, collision->normal.k * along };
		const real_vector3d perpendicular = { new_velocity->i - parallel.i, new_velocity->j - parallel.j, new_velocity->k - parallel.k };
		const real32 perpendicular_scale = 1.f - material_response->perpendicular_friction;
		const real32 parallel_scale = 1.f - material_response->parallel_friction;
		*new_velocity =
		{
			perpendicular_scale * perpendicular.i - parallel_scale * parallel.i,
			perpendicular_scale * perpendicular.j - parallel_scale * parallel.j,
			perpendicular_scale * perpendicular.k - parallel_scale * parallel.k
		};
	}
	else if (response != _h1_material_response_overpenetrate)
	{
		*new_velocity = { 0.f, 0.f, 0.f };
	}

	if (angular_noise != 0.f)
	{
		const real32 magnitude = h1_magnitude(new_velocity);
		if (magnitude != 0.f)
		{
			real_vector3d axis = { new_velocity->i / magnitude, new_velocity->j / magnitude, new_velocity->k / magnitude };
			h1_random_vector_in_cone(&axis, angular_noise, &axis);
			*new_velocity = { axis.i * magnitude, axis.j * magnitude, axis.k * magnitude };
		}
	}
	if (velocity_noise != 0.f)
	{
		real_vector3d axis = *new_velocity;
		const real32 magnitude = h1_normalize(&axis);
		if (magnitude != 0.f)
		{
			const real32 noisy = h1_projectile_random_range(-velocity_noise, velocity_noise) + magnitude;
			*new_velocity = { axis.i * noisy, axis.j * noisy, axis.k * noisy };
		}
	}

	const real32 speed_squared = h1_dot(new_velocity, new_velocity);
	if (response != _h1_material_response_attach && speed_squared < definition->minimum_velocity * definition->minimum_velocity)
	{
		h1_projectile_set_action(projectile, _h1_projectile_action_detonate);
	}
	if (speed_squared < k_real_epsilon)
	{
		SET_BIT(projectile->flags, _h1_projectile_stopped_after_collision_bit, true);
		if (collision->normal.k > 0.3f)
		{
			SET_BIT(projectile->flags, _h1_projectile_at_rest_bit, true);
			if (definition->timer.upper == 0.f)
			{
				h1_projectile_set_action(projectile, _h1_projectile_action_detonate);
			}
		}
	}

	if (material_response && material_response->scale_effects_by == _h1_material_effect_scale_angle)
	{
		effect_scale = impact_angle * (2.f / _pi);
	}
	else if (material_response)
	{
		effect_scale = damage_scale;
	}
	effect_scale = PIN(effect_scale, 0.f, 1.f);
	material_effect_scale = PIN(material_effect_scale, 0.f, 1.f);

	// the impact effects at the point: along the normal, the incident and reflected directions and down
	real_point3d marker_points[k_h1_impact_markers];
	real_vector3d marker_forwards[k_h1_impact_markers];
	marker_forwards[_h1_effect_vector_incident] = { -direction.i, -direction.j, -direction.k };
	marker_forwards[_h1_effect_vector_negative_incident] = direction;
	marker_forwards[_h1_effect_vector_gravity] = { 0.f, 0.f, -1.f };
	marker_forwards[_h1_effect_vector_normal] = collision->normal;
	const real32 reflection = 2.f * h1_dot(&direction, &collision->normal);
	marker_forwards[_h1_effect_vector_reflected] =
	{
		direction.i - collision->normal.i * reflection, direction.j - collision->normal.j * reflection, direction.k - collision->normal.k * reflection
	};
	for (int32 i = 0; i < k_h1_impact_markers; i++)
	{
		marker_points[i] = collision->point;
	}
	if (impact_velocity > 1.f / 120.f)
	{
		h1_projectile_effect_new(projectile_index, effect_definition_index, collision, marker_points, marker_forwards, effect_scale, material_effect_scale);
	}
	if (!TEST_BIT(projectile->flags, _h1_projectile_counting_down_bit) &&
		(TEST_BIT(projectile->flags, _h1_projectile_stopped_after_collision_bit) || response == _h1_material_response_attach))
	{
		h1_projectile_effect_new(projectile_index, definition->detonation_started.index, collision, marker_points, marker_forwards, effect_scale,
			material_effect_scale);
	}

	switch (response)
	{
	case _h1_material_response_disappear:
		h1_projectile_set_action(projectile, _h1_projectile_action_disappear);
		break;
	case _h1_material_response_detonate:
		h1_projectile_set_action(projectile, _h1_projectile_action_detonate);
		break;
	case _h1_material_response_attach:
	{
		// the needles: enough of them stuck in one object set each other off
		if (collision->type == _h2_collision_result_object && TEST_BIT(definition->flags_2, _h1_projectile_super_combining_explosion_bit))
		{
			const object_datum* hit = object_get(collision->object_index);
			int16 combining_projectile_count = 0;
			for (datum child_index = hit->object.first_child_object_index; child_index != NONE;)
			{
				const object_datum* child = object_get(child_index);
				auto found = g_h1_projectiles.find(child_index);
				if (child->definition_index == object_get(projectile_index)->definition_index && found != g_h1_projectiles.end() &&
					!TEST_BIT(found->second.flags, _h1_projectile_already_super_exploded_bit))
				{
					found->second.arming_time = 0.f;
					found->second.detonation_timer = 0.f;
					combining_projectile_count++;
				}
				if (combining_projectile_count >= k_h1_maximum_combining_projectiles)
				{
					SET_BIT(projectile->flags, _h1_projectile_will_super_explode_bit, true);
					break;
				}
				child_index = child->object.next_object_index;
			}
		}
		projectile->velocity = { 0.f, 0.f, 0.f };
		projectile->angular_velocity = { 0.f, 0.f, 0.f };
		SET_BIT(projectile->flags, _h1_projectile_attached_bit, true);
		SET_BIT(projectile->flags, _h1_projectile_at_rest_bit, true);
		h1_object_move(projectile_index, new_position, NULL, NULL);
		h1_object_set_velocities(projectile_index, projectile);
		if (collision->type == _h2_collision_result_object)
		{
			object_attach_to_node(collision->object_index, projectile_index, collision->node_index);
		}
		if (TEST_BIT(definition->flags_2, _h1_projectile_detonation_max_time_if_attached_bit))
		{
			const real32 detonation_ticks = definition->timer.upper * k_h1_ticks_per_second;
			if (detonation_ticks >= 1.f)
			{
				projectile->detonation_timer_delta = 1.f / detonation_ticks;
			}
		}
		break;
	}
	default:
		break;
	}
	return;
}

// projectiles.c projectile_detonate: the detonation (or super detonation) effect, the attached damage and the material's
// detonation effect
static void h1_projectile_detonate(datum projectile_index, s_h1_projectile* projectile, const h1_proj* definition)
{
	typedef void(__cdecl* t_object_detach)(datum object_index);

	datum effect_definition_index = definition->effect.index;
	const object_datum* object = object_get(projectile_index);
	const datum owner_object_index = h1_object_owner_get(projectile_index);

	// the needles: more than enough in one biped explode together from its origin
	if (TEST_BIT(definition->flags_2, _h1_projectile_super_combining_explosion_bit) && !TEST_BIT(projectile->flags, _h1_projectile_already_super_exploded_bit) &&
		object->object.parent_object_index != NONE)
	{
		const datum parent_index = object->object.parent_object_index;
		const object_datum* parent = object_get(parent_index);
		int16 combining_projectile_count = 0;
		for (datum child_index = parent->object.first_child_object_index; child_index != NONE; child_index = object_get(child_index)->object.next_object_index)
		{
			auto found = g_h1_projectiles.find(child_index);
			if (object_get(child_index)->definition_index == object->definition_index && found != g_h1_projectiles.end() &&
				!TEST_BIT(found->second.flags, _h1_projectile_already_super_exploded_bit))
			{
				combining_projectile_count++;
			}
		}
		if (object_try_and_get_and_verify_type(parent_index, _object_mask_biped) && combining_projectile_count > k_h1_maximum_combining_projectiles)
		{
			for (datum child_index = parent->object.first_child_object_index; child_index != NONE; child_index = object_get(child_index)->object.next_object_index)
			{
				auto found = g_h1_projectiles.find(child_index);
				if (object_get(child_index)->definition_index == object->definition_index && found != g_h1_projectiles.end() &&
					!TEST_BIT(found->second.flags, _h1_projectile_already_super_exploded_bit))
				{
					s_h1_projectile* child = &found->second;
					if (combining_projectile_count <= k_h1_maximum_combining_projectiles)
					{
						SET_BIT(child->flags, _h1_projectile_already_super_exploded_bit, true);
						child->detonation_timer *= h1_projectile_random();
						child->arming_time *= h1_projectile_random();
					}
					else
					{
						child->detonation_timer = 0.f;
						child->arming_time = 0.f;
					}
					combining_projectile_count--;
				}
			}
			effect_definition_index = definition->super_detonation.index;
			real_point3d parent_origin;
			object_get_origin(parent_index, &parent_origin, false);
			Memory::GetAddress<t_object_detach>(0x137A84)(projectile_index);
			h1_object_move(projectile_index, &parent_origin, NULL, NULL);
			object = object_get(projectile_index);
		}
	}

	real_point3d marker_points[2];
	real_vector3d marker_forwards[2];
	object_get_origin(projectile_index, &marker_points[0], false);
	marker_forwards[0] = object->object.forward;
	marker_points[1] = marker_points[0];
	marker_forwards[1] = { 0.f, 0.f, -1.f };
	h1_effect_new_from_markers(effect_definition_index, owner_object_index, 2, k_h1_detonation_marker_names, marker_points, marker_forwards, 0.f, 0.f);

	// stuck in an object: its attached damage
	if (object->object.parent_object_index != NONE && definition->attached_detonation_damage.index != NONE)
	{
		typedef void(__cdecl* t_damage_data_new)(s_damage_data* damage, datum definition_index);
		typedef void(__cdecl* t_damage_owner_from_object)(datum object_index, s_damage_owner* owner);
		const datum damage_definition_index = h1_damage_effect_build(definition->attached_detonation_damage.index);
		if (damage_definition_index != NONE)
		{
			s_damage_data damage;
			Memory::GetAddress<t_damage_data_new>(0x175BAC)(&damage, damage_definition_index);
			damage.flags = (e_damage_data_flags)(damage.flags | FLAG(_damage_from_weapon_bit));
			damage.direction = object->object.forward;
			damage.origin = marker_points[0];
			damage.epicenter = damage.origin;
			if (object_try_and_get(owner_object_index))
			{
				Memory::GetAddress<t_damage_owner_from_object>(0x175C14)(owner_object_index, &damage.owner);
			}
			object_cause_damage(&damage, object->object.parent_object_index, NONE, NONE, NONE, NULL);
		}
	}

	// the material it hit last: its detonation effect
	if (projectile->hit_material_type != NONE)
	{
		const h1_proj_material_responses* material_response = h1_projectile_material_response(definition, projectile->hit_material_type);
		if (material_response)
		{
			h1_effect_new_from_markers(material_response->detonation_effect.index, owner_object_index, 2, k_h1_detonation_marker_names, marker_points,
				marker_forwards, 0.f, 0.f);
		}
	}
	return;
}

// projectiles.c projectile_effect_new (attached to the object hit in halo 1; here at the point)
static void h1_projectile_effect_new(datum projectile_index, datum effect_index, const s_h1_collision* collision, const real_point3d* points,
	const real_vector3d* forwards, real32 scale, real32 material_effect_scale)
{
	(void)collision;
	h1_effect_new_from_markers(effect_index, h1_object_owner_get(projectile_index), k_h1_impact_markers, k_h1_impact_marker_names, points, forwards,
		scale, material_effect_scale);
	return;
}

// the material's response, none (halo 1's default: disappear without an effect) past the definition's
static const h1_proj_material_responses* h1_projectile_material_response(const h1_proj* definition, int16 material_type)
{
	return VALID_INDEX(material_type, definition->material_responses.count) ? g_h1_cache_file->block_get(definition->material_responses, material_type) : NULL;
}

// object_translate and the orientation (halo 2's object set position, FUN_00536b7f)
static void h1_object_move(datum object_index, const real_point3d* position, const real_vector3d* forward, const real_vector3d* up)
{
	typedef void(__cdecl* t_object_set_position)(datum object_index, const real_point3d* position, const real_vector3d* forward, const real_vector3d* up,
		const s_location* location);
	Memory::GetAddress<t_object_set_position>(0x136B7F)(object_index, position, forward, up, NULL);
	return;
}

// halo 2's object velocities (a second's, FUN_00535123) from the projectile's
static void h1_object_set_velocities(datum object_index, const s_h1_projectile* projectile)
{
	typedef void(__cdecl* t_object_set_velocities)(datum object_index, const real_vector3d* translational, const real_vector3d* angular);
	const real_vector3d translational =
	{
		projectile->velocity.i * k_h1_ticks_per_second, projectile->velocity.j * k_h1_ticks_per_second, projectile->velocity.k * k_h1_ticks_per_second
	};
	const real_vector3d angular =
	{
		projectile->angular_velocity.i * k_h1_ticks_per_second,
		projectile->angular_velocity.j * k_h1_ticks_per_second,
		projectile->angular_velocity.k * k_h1_ticks_per_second
	};
	Memory::GetAddress<t_object_set_velocities>(0x135123)(object_index, &translational, &angular);
	return;
}

// the halo 1 material type of a halo 2 global material (the structure and the objects were built with h1_material_type_to_global_material)
static int16 h1_global_material_to_material_type(int16 global_material_index)
{
	if (global_material_index == NONE)
	{
		return NONE;
	}
	for (int16 material_type = 0; material_type < k_h1_material_type_count; material_type++)
	{
		if (h1_material_type_to_global_material(material_type) == global_material_index)
		{
			return material_type;
		}
	}
	return NONE;
}

// the object that fired the projectile (its damage owner)
static datum h1_object_owner_get(datum object_index)
{
	const object_datum* object = object_try_and_get(object_index);
	return object ? object->object.damage_owner_object_index : NONE;
}

static real32 h1_projectile_random(void)
{
	g_h1_projectile_random_seed = g_h1_projectile_random_seed * 1664525u + 1013904223u;
	return (real32)(g_h1_projectile_random_seed >> 8) / (real32)(1u << 24);
}

static real32 h1_projectile_random_range(real32 lower, real32 upper)
{
	return lower + (upper - lower) * h1_projectile_random();
}

// rotate_vector_about_axis
static void h1_rotate_vector_about_axis(real_vector3d* vector, const real_vector3d* axis, real32 sine, real32 cosine)
{
	const real32 along = h1_dot(vector, axis) * (1.f - cosine);
	const real_vector3d cross = h1_cross(axis, vector);
	*vector =
	{
		vector->i * cosine + cross.i * sine + axis->i * along,
		vector->j * cosine + cross.j * sine + axis->j * along,
		vector->k * cosine + cross.k * sine + axis->k * along
	};
	return;
}

// random_vector_in_cone3d: the axis turned by up to the angle about a random perpendicular
static void h1_random_vector_in_cone(const real_vector3d* axis, real32 angle, real_vector3d* result)
{
	real_vector3d perpendicular;
	perpendicular3d(axis, &perpendicular);
	h1_normalize(&perpendicular);
	h1_rotate_vector_about_axis(&perpendicular, axis, sinf(h1_projectile_random() * _pi * 2.f), cosf(h1_projectile_random() * _pi * 2.f));
	real_vector3d turned = *axis;
	const real32 turn = h1_projectile_random() * angle;
	h1_rotate_vector_about_axis(&turned, &perpendicular, sinf(turn), cosf(turn));
	*result = turned;
	return;
}

static real32 h1_magnitude(const real_vector3d* vector)
{
	return sqrtf(vector->i * vector->i + vector->j * vector->j + vector->k * vector->k);
}

static real32 h1_normalize(real_vector3d* vector)
{
	const real32 magnitude = h1_magnitude(vector);
	if (magnitude > 0.f)
	{
		vector->i /= magnitude;
		vector->j /= magnitude;
		vector->k /= magnitude;
	}
	return magnitude;
}

static real32 h1_dot(const real_vector3d* a, const real_vector3d* b)
{
	return a->i * b->i + a->j * b->j + a->k * b->k;
}

static real_vector3d h1_cross(const real_vector3d* a, const real_vector3d* b)
{
	return { a->j * b->k - a->k * b->j, a->k * b->i - a->i * b->k, a->i * b->j - a->j * b->i };
}

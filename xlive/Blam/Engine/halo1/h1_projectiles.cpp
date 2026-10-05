#include "stdafx.h"
#include "h1_projectiles.h"

#include "h1_cache_file.h"
#include "h1_effects.h"
#include "h1_items.h"
#include "h1_log.h"
#include "h1_object_tags.h"
#include "h1_objects.h"
#include "h1_runtime.h"
#include "h1_structure_bsp.h"
#include "h2_tag_definitions_generated.h"

#include "game/game_globals.h"

#include <unordered_map>

/* constants */

enum
{
	k_h2_object_type_projectile = 5,
	k_h1_material_type_count = 33,
};

// projectile_definitions.h: disappear, detonate, reflect, overpenetrate, attach
// items/projectile_definition.h: impact (detonate), fizzle, overpenetrate, attach, bounce
static const int16 k_h1_projectile_response_to_h2[] = { 1, 0, 4, 2, 3 };

/* prototypes */

static int16 h1_projectile_response_get(int16 h1_response);
static datum h1_effect_first_damage_effect(datum h1_effect_index);
static void h1_reference_none(tag_reference* reference);

// the halo 1 damage effect of each halo 2 damage effect built
static std::unordered_map<datum, datum> g_h1_damage_effects;

/* public code */

void h1_projectiles_reset(void)
{
	g_h1_damage_effects.clear();
	return;
}

datum h1_damage_effect_h1_get(datum h2_damage_effect_index)
{
	auto found = g_h1_damage_effects.find(h2_damage_effect_index);
	return found != g_h1_damage_effects.end() ? found->second : NONE;
}

datum h1_damage_effect_build(datum h1_damage_effect_index)
{
	const h1_jpt* h1_damage = (const h1_jpt*)g_h1_cache_file->tag_get('jpt!', h1_damage_effect_index);
	if (!h1_damage)
	{
		return NONE;
	}

	char name[256];
	sprintf_s(name, "halo1\\%s", g_h1_cache_file->tag_name_get(h1_damage_effect_index));
	const datum existing = h1_runtime_tag_find('jpt!', name);
	if (existing != NONE)
	{
		g_h1_damage_effects[existing] = h1_damage_effect_index;
		return existing;
	}

	h2x_jpt* damage = NULL;
	const datum damage_index = h1_runtime_tag_new('jpt!', name, &damage);
	if (damage_index == NONE)
	{
		return NONE;
	}
	g_h1_damage_effects[damage_index] = h1_damage_effect_index;

	// both games keep the same side effects, categories and the halo 1 flags as the first halo 2 flags
	damage->radius = h1_damage->radius;
	damage->cutoff_scale = h1_damage->cutoff_scale;
	damage->flags = h1_damage->flags & FLAG(0);
	damage->side_effect = h1_damage->side_effect;
	damage->category = h1_damage->category;
	damage->flags_2 = h1_damage->flags_2 & (FLAG(13) - 1);
	damage->area_of_effect_core_radius = h1_damage->aoe_core_radius;
	damage->damage_lower_bound = h1_damage->damage_lower_bound;
	damage->damage_upper_bound = h1_damage->damage_upper_bound;
	damage->active_camouflage_damage = h1_damage->active_camouflage_damage;
	damage->stun = h1_damage->stun;
	damage->maximum_stun = h1_damage->maximum_stun;
	damage->stun_time = h1_damage->stun_time;
	damage->instantaneous_acceleration = h1_damage->instantaneous_acceleration;
	damage->rider_direct_damage_scale = 1.f;
	damage->rider_maximum_transfer_damage_scale = 1.f;
	damage->rider_minimum_transfer_damage_scale = 1.f;
	damage->general_damage = _string_id_empty_string;
	damage->specific_damage = _string_id_empty_string;

	// camera impulse and shaking
	damage->duration = h1_damage->duration_4;
	damage->fade_function = h1_damage->fade_function_4;
	damage->rotation = h1_damage->rotation;
	damage->pushback = h1_damage->pushback;
	damage->jitter = h1_damage->jitter;
	damage->duration_2 = h1_damage->duration_5;
	damage->falloff_function = h1_damage->falloff_function;
	damage->random_translation = h1_damage->random_translation;
	damage->random_rotation = h1_damage->random_rotation;
	damage->wobble_function = h1_damage->wobble_function;
	damage->wobble_function_period = h1_damage->wobble_function_period;
	damage->wobble_weight = h1_damage->wobble_weight;
	h1_reference_none(&damage->sound);

	// breaking effect
	damage->forward_velocity = h1_damage->forward_velocity;
	damage->forward_radius = h1_damage->forward_radius;
	damage->forward_exponent = h1_damage->forward_exponent;
	damage->outward_velocity = h1_damage->outward_velocity;
	damage->outward_radius = h1_damage->outward_radius;
	damage->outward_exponent = h1_damage->outward_exponent;
	return damage_index;
}

datum h1_projectile_definition_build(datum h1_projectile_index)
{
	const h1_proj* h1_projectile = (const h1_proj*)g_h1_cache_file->tag_get('proj', h1_projectile_index);
	if (!h1_projectile)
	{
		return NONE;
	}

	char name[256];
	sprintf_s(name, "halo1\\%s", g_h1_cache_file->tag_name_get(h1_projectile_index));
	const datum existing = h1_runtime_tag_find('proj', name);
	if (existing != NONE)
	{
		return existing;
	}

	// bullets and plasma bolts have no model, they're seen through their attachments
	const h1_mode* h1_model = h1_projectile->model.index != NONE ? (const h1_mode*)g_h1_cache_file->tag_get('mode', h1_projectile->model.index) : NULL;

	s_h1_object_tags tags;
	tags.render_model = h1_object_render_model_build(h1_model, name);
	tags.collision_model = NONE;
	tags.physics_model = NONE;
	tags.animation_graph = NONE;
	tags.disappear_distance = 100.f;
	const datum model_index = tags.render_model != NONE ? h1_object_model_build(&tags, h1_model, NULL, name) : NONE;
	if (model_index == NONE)
	{
		return NONE;
	}

	h2x_proj* projectile = NULL;
	const datum projectile_index = h1_runtime_tag_new('proj', name, &projectile);
	if (projectile_index == NONE)
	{
		return NONE;
	}

	// object
	projectile->object_type = k_h2_object_type_projectile;
	projectile->flags = h1_projectile->flags;
	projectile->bounding_radius = h1_projectile->bounding_radius;
	projectile->bounding_offset = h1_projectile->bounding_offset;
	projectile->acceleration_scale = h1_projectile->acceleration_scale;
	projectile->default_model_variant = _string_id_default;
	h1_runtime_reference_set(&projectile->model, 'hlmt', model_index);
	h1_reference_none(&projectile->crate_object);
	h1_reference_none(&projectile->modifier_shader);
	h1_reference_none(&projectile->creation_effect);
	h1_reference_none(&projectile->material_effects);
	projectile->apply_collision_damage_scale = 1.f;
	projectile->game_acceleration = { 2.5f, 4.5f };
	projectile->game_scale = { 0.2f, 1.25f };
	projectile->absolute_acceleration = { 2.5f, 10.f };
	projectile->absolute_scale = { 0.2f, 1.25f };
	projectile->hud_text_message_index = NONE;

	// projectile: oriented along velocity, ballistic aiming, detonation max time if attached, super combining explosion
	projectile->flags_2 = h1_projectile->flags_2 & (FLAG(4) - 1);
	projectile->detonation_timer_starts = h1_projectile->detonation_timer_starts;
	projectile->impact_noise = h1_projectile->impact_noise;
	projectile->ai_perception_radius = h1_projectile->ai_perception_radius;
	projectile->collision_radius = h1_projectile->collision_radius;
	projectile->arming_time = h1_projectile->arming_time;
	projectile->danger_radius = h1_projectile->danger_radius;
	projectile->timer = h1_projectile->timer;
	projectile->minimum_velocity = h1_projectile->minimum_velocity;
	projectile->maximum_range = h1_projectile->maximum_range;
	projectile->detonation_noise = h1_projectile->detonation_noise;
	h1_reference_none(&projectile->detonation_started);
	// halo 2 calls for its detonation effect only when it has one: the stub stands in for the halo 1 effect (h1_effects)
	const datum stub_effect = h1_projectile->effect.index != NONE ? h1_effects_stub_effect_get() : NONE;
	h1_runtime_reference_set(&projectile->airborne_detonation_effect, stub_effect != NONE ? 'effe' : (tag_group)NONE, stub_effect);
	h1_runtime_reference_set(&projectile->ground_detonation_effect, stub_effect != NONE ? 'effe' : (tag_group)NONE, stub_effect);
	h1_reference_none(&projectile->attached_detonation_damage);
	const datum super_stub_effect = h1_projectile->super_detonation.index != NONE ? h1_effects_stub_effect_get() : NONE;
	h1_runtime_reference_set(&projectile->super_detonation, super_stub_effect != NONE ? 'effe' : (tag_group)NONE, super_stub_effect);
	h1_reference_none(&projectile->super_detonation_damage);
	h1_reference_none(&projectile->detonation_sound);
	h1_reference_none(&projectile->attached_super_detonation_damage);
	h1_reference_none(&projectile->flyby_sound);
	h1_reference_none(&projectile->impact_effect);
	h1_reference_none(&projectile->impact_damage);
	h1_reference_none(&projectile->boarding_detonation_damage);
	h1_reference_none(&projectile->boarding_attached_detonation_damage);

	// halo 1 deals the detonation damage from the detonation effect
	const datum h1_detonation_damage = h1_effect_first_damage_effect(h1_projectile->effect.index);
	const datum detonation_damage = h1_detonation_damage != NONE ? h1_damage_effect_build(h1_detonation_damage) : NONE;
	h1_runtime_reference_set(&projectile->detonation_damage, detonation_damage != NONE ? 'jpt!' : (tag_group)NONE, detonation_damage);
	if (h1_projectile->attached_detonation_damage.index != NONE)
	{
		const datum attached_damage = h1_damage_effect_build(h1_projectile->attached_detonation_damage.index);
		h1_runtime_reference_set(&projectile->attached_detonation_damage, attached_damage != NONE ? 'jpt!' : (tag_group)NONE, attached_damage);
	}
	if (h1_projectile->impact_damage.index != NONE)
	{
		const datum impact_damage = h1_damage_effect_build(h1_projectile->impact_damage.index);
		h1_runtime_reference_set(&projectile->impact_damage, impact_damage != NONE ? 'jpt!' : (tag_group)NONE, impact_damage);
	}
	projectile->material_effect_radius = 0.5f;

	projectile->air_gravity_scale = h1_projectile->air_gravity_scale;
	projectile->air_damage_range = h1_projectile->air_damage_range;
	projectile->water_gravity_scale = h1_projectile->water_gravity_scale;
	projectile->water_damage_range = h1_projectile->water_damage_range;
	projectile->initial_velocity = h1_projectile->initial_velocity;
	projectile->final_velocity = h1_projectile->final_velocity;
	projectile->guided_angular_velocity_lower = h1_projectile->guided_angular_velocity;
	projectile->guided_angular_velocity_upper = h1_projectile->guided_angular_velocity;
	projectile->runtime_acceleration_bound_inverse = 1.f;

	// halo 1 keeps a response per material type, halo 2 per global material
	const int32 response_count = MIN(h1_projectile->material_responses.count, (int32)k_h1_material_type_count);
	h2x_proj_material_responses* responses = h1_runtime_block_new(&projectile->material_responses, response_count);
	for (int32 i = 0; i < response_count; i++)
	{
		const h1_proj_material_responses* h1_response = g_h1_cache_file->block_get(h1_projectile->material_responses, i);
		h2x_proj_material_responses* response = &responses[i];
		const int16 global_material_index = h1_material_type_to_global_material((int16)i);
		response->flags = h1_response->flags & FLAG(0);
		response->default_response = h1_projectile_response_get(h1_response->response);
		h1_reference_none(&response->do_not_use_old_effect);
		response->global_material_name = h1_global_material_name(global_material_index);
		response->global_material_index = global_material_index;
		// halo 1 has no potential response when it is 0 (disappear), halo 2 takes it by its chance
		const bool has_potential_response = h1_response->response_2 != 0;
		response->potential_response = has_potential_response ? h1_projectile_response_get(h1_response->response_2) : response->default_response;
		response->response_flags = h1_response->flags_2 & FLAG(0);
		response->chance_fraction = has_potential_response ? 1.f - h1_response->skip_fraction : 0.f;
		response->between_angle = h1_response->between;
		response->and_velocity = h1_response->and;
		h1_reference_none(&response->old_effect);
		response->scale_effects_by = h1_response->scale_effects_by;
		response->angular_noise = h1_response->angular_noise;
		response->velocity_noise = h1_response->velocity_noise;
		h1_reference_none(&response->old_effect_2);
		response->initial_friction = h1_response->initial_friction;
		response->maximum_distance = h1_response->maximum_distance;
		response->parallel_friction = h1_response->parallel_friction;
		response->perpendicular_friction = h1_response->perpendicular_friction;
	}

	h1_objects_bind(projectile_index, h1_projectile_index);
	h1_log("projectiles: built %s (damage %s)", name, detonation_damage != NONE ? "yes" : "no");
	return projectile_index;
}

void h1_grenades_build(void)
{
	const datum h1_globals_index = g_h1_cache_file->tag_find('matg', "globals\\globals");
	const h1_matg* h1_globals = h1_globals_index != NONE ? (const h1_matg*)g_h1_cache_file->tag_get('matg', h1_globals_index) : NULL;
	h2x_matg* globals = (h2x_matg*)scenario_get_game_globals();
	if (!h1_globals || !globals)
	{
		return;
	}

	const int32 count = MIN(h1_globals->grenades.count, globals->grenades.count);
	for (int32 i = 0; i < count; i++)
	{
		const h1_matg_grenades* h1_grenade = g_h1_cache_file->block_get(h1_globals->grenades, i);
		h2x_matg_grenades* grenade = globals->grenades[i];
		const datum equipment = h1_grenade->equipment.index != NONE ? h1_equipment_definition_build(h1_grenade->equipment.index) : NONE;
		const datum projectile = h1_grenade->projectile.index != NONE ? h1_projectile_definition_build(h1_grenade->projectile.index) : NONE;
		if (projectile == NONE)
		{
			continue;
		}
		grenade->maximum_count = h1_grenade->maximum_count;
		h1_runtime_reference_set(&grenade->projectile, 'proj', projectile);
		if (equipment != NONE)
		{
			h1_runtime_reference_set(&grenade->equipment, 'eqip', equipment);
		}
		h1_log("grenades: %d is %s", i, g_h1_cache_file->tag_name_get(h1_grenade->projectile.index));
	}
	return;
}

/* private code */

static int16 h1_projectile_response_get(int16 h1_response)
{
	return VALID_INDEX(h1_response, NUMBEROF(k_h1_projectile_response_to_h2)) ? k_h1_projectile_response_to_h2[h1_response] : 0;
}

static datum h1_effect_first_damage_effect(datum h1_effect_index)
{
	const h1_effe* effect = (const h1_effe*)g_h1_cache_file->tag_get('effe', h1_effect_index);
	if (!effect)
	{
		return NONE;
	}
	for (int32 i = 0; i < effect->events.count; i++)
	{
		const h1_effe_events* event = g_h1_cache_file->block_get(effect->events, i);
		for (int32 j = 0; j < event->parts.count; j++)
		{
			const h1_effe_events_parts* part = g_h1_cache_file->block_get(event->parts, j);
			if (part->type.group_tag == 'jpt!' && part->type.index != NONE)
			{
				return part->type.index;
			}
		}
	}
	return NONE;
}

static void h1_reference_none(tag_reference* reference)
{
	h1_runtime_reference_set(reference, (tag_group)NONE, NONE);
	return;
}

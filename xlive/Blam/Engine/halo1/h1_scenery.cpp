#include "stdafx.h"
#include "h1_scenery.h"
#include "h1_game_state.h"

#include "h1_cache_file.h"
#include "h1_effects.h"
#include "h1_log.h"
#include "h1_map_loader.h"
#include "h1_objects.h"
#include "h1_projectiles.h"
#include "h1_runtime.h"
#include "h2_tag_definitions_generated.h"

#include "objects/damage.h"
#include "physics/collision_bsp_definition.h"
#include "structures/structure_bsp_definitions.h"

#include <vector>

/* constants */

enum
{
	k_h1_maximum_scenery_regions = 8,
	k_h1_material_type_count = 33,
};

// damage.c object damage flags kept for scenery
enum
{
	_h1_scenery_shield_depleted_bit = 0,
	_h1_scenery_shield_charging_bit,
	_h1_scenery_passed_shield_damage_threshold_bit,
};

static const real32 k_h1_scenery_ticks_per_second = 30.f;
static const real32 k_h1_damage_decay_per_tick = 0.016666668f;

/* structures */

struct s_h1_scenery_instance_binding
{
	int32 instance_index;
	int16 definition_index;
	int16 shield_off_definition_index;
	int16 material_offset;
};

struct s_h1_scenery_state
{
	datum definition_index;
	datum collision_model_index;
	s_h1_scenery_instance_binding instance;
	real_point3d center;
	real32 body_vitality;
	real32 shield_vitality;
	real32 current_body_damage;
	real32 current_shield_damage;
	real32 recent_body_damage;
	real32 recent_shield_damage;
	int32 body_damage_decay_timer;
	int32 shield_damage_decay_timer;
	int16 shield_stun_ticks;
	uint32 damage_flags;
	int16 region_permutations[k_h1_maximum_scenery_regions];
	bool has_region_permutations;
	s_h1_object_functions functions;
};

struct s_h1_scenery_globals
{
	std::vector<s_h1_scenery_instance_binding> bindings;	// by placement index
	std::vector<s_h1_scenery_state> states;				// by placement index, built on the first update
	bool initialized;
	real32 leftover_ticks;
	LARGE_INTEGER last_update;
	uint32 random;
};

/* globals */

static s_h1_scenery_globals g_h1_scenery = { {}, {}, false, 0.f, {}, 0x5EED1234u };
H1_GAME_STATE_VARIABLE(g_h1_scenery);

typedef void(__cdecl* t_breakable_surface_damage)(int32 instance_index, int32 breakable_surface_index, s_damage_data* damage, int32 surface_index);
static t_breakable_surface_damage p_breakable_surface_damage = NULL;
typedef void(__cdecl* t_breakable_surfaces_area_damage)(s_damage_data* damage);
static t_breakable_surfaces_area_damage p_breakable_surfaces_area_damage = NULL;

/* prototypes */

static void __cdecl h1_breakable_surface_damage(int32 instance_index, int32 breakable_surface_index, s_damage_data* damage, int32 surface_index);
static void __cdecl h1_breakable_surfaces_area_damage(s_damage_data* damage);
static void h1_scenery_initialize(void);
static void h1_scenery_tick(s_h1_scenery_state* state);
static void h1_scenery_cause_damage(s_h1_scenery_state* state, const s_damage_data* damage, int16 material_index, real32 scale);
static void h1_scenery_deplete_shield(s_h1_scenery_state* state);
static void h1_scenery_shield_regions(s_h1_scenery_state* state, bool active);
static void h1_scenery_effect(const s_h1_scenery_state* state, const h1_tag_reference* effect);
static real32 h1_scenery_random_range(real32 lower, real32 upper);

/* public code */

void h1_scenery_reset(void)
{
	g_h1_scenery.bindings.clear();
	g_h1_scenery.states.clear();
	g_h1_scenery.initialized = false;
	g_h1_scenery.leftover_ticks = 0.f;
	return;
}

void h1_scenery_instance_register(int32 instance_index, int32 placement_index, int16 definition_index, int16 shield_off_definition_index, int16 material_offset)
{
	if (placement_index < 0)
	{
		return;
	}
	if ((int32)g_h1_scenery.bindings.size() <= placement_index)
	{
		g_h1_scenery.bindings.resize(placement_index + 1, { NONE, NONE, NONE, 0 });
	}
	g_h1_scenery.bindings[placement_index] = { instance_index, definition_index, shield_off_definition_index, material_offset };
	return;
}

void h1_scenery_apply_patches(void)
{
	DETOUR_ATTACH(p_breakable_surface_damage, Memory::GetAddress<t_breakable_surface_damage>(0xB211A), h1_breakable_surface_damage);
	DETOUR_ATTACH(p_breakable_surfaces_area_damage, Memory::GetAddress<t_breakable_surfaces_area_damage>(0xB23DA), h1_breakable_surfaces_area_damage);
	return;
}

void h1_scenery_update(void)
{
	LARGE_INTEGER now, frequency;
	QueryPerformanceCounter(&now);
	QueryPerformanceFrequency(&frequency);
	real32 dt = (real32)(now.QuadPart - g_h1_scenery.last_update.QuadPart) / (real32)frequency.QuadPart;
	g_h1_scenery.last_update = now;
	if (!h1_maps_active() || !g_h1_cache_file)
	{
		return;
	}
	if (!g_h1_scenery.initialized)
	{
		h1_scenery_initialize();
		dt = 0.f;
	}

	// objects.c objects_update: damage once per tick
	g_h1_scenery.leftover_ticks += PIN(dt, 0.f, 0.1f) * k_h1_scenery_ticks_per_second;
	const int32 ticks = (int32)g_h1_scenery.leftover_ticks;
	g_h1_scenery.leftover_ticks -= (real32)ticks;
	for (int32 placement_index = 0; placement_index < (int32)g_h1_scenery.states.size(); placement_index++)
	{
		s_h1_scenery_state* state = &g_h1_scenery.states[placement_index];
		if (state->definition_index == NONE)
		{
			continue;
		}
		for (int32 tick = 0; tick < ticks; tick++)
		{
			h1_scenery_tick(state);
		}

		// objects.c object_export_function_values and object_compute_function_values
		s_h1_object_vitality vitality;
		vitality.body_vitality = state->body_vitality;
		vitality.shield_vitality = state->shield_vitality;
		vitality.current_body_damage = state->current_body_damage;
		vitality.current_shield_damage = state->current_shield_damage;
		vitality.dead = false;
		vitality.has_forward = false;
		h1_object_functions_export(state->definition_index, &vitality, &state->functions);
		h1_object_functions_update(state->definition_index, placement_index, &state->functions);
	}
	return;
}

void h1_scenery_render_state(int32 placement_index, const real32** out_function_values, const real_rgb_color** out_change_colors, const int16** out_region_permutations)
{
	*out_function_values = NULL;
	*out_change_colors = NULL;
	*out_region_permutations = NULL;
	if (!VALID_INDEX(placement_index, (int32)g_h1_scenery.states.size()))
	{
		return;
	}
	const s_h1_scenery_state* state = &g_h1_scenery.states[placement_index];
	if (state->definition_index == NONE)
	{
		return;
	}
	*out_function_values = state->functions.outgoing;
	*out_change_colors = state->functions.colors;
	*out_region_permutations = state->has_region_permutations ? state->region_permutations : NULL;
	return;
}

/* private code */

// physics/breakable_surfaces.cpp breakable surface damage: halo 2 hands projectile hits on breakable instanced geometry surfaces
// here; the surfaces of halo 1 scenery that takes damage are flagged breakable (h1_structure_bsp)
static void __cdecl h1_breakable_surface_damage(int32 instance_index, int32 breakable_surface_index, s_damage_data* damage, int32 surface_index)
{
	if (h1_maps_active() && g_h1_cache_file && instance_index != NONE)
	{
		for (s_h1_scenery_state& state : g_h1_scenery.states)
		{
			if (state.definition_index == NONE || state.instance.instance_index != instance_index)
			{
				continue;
			}
			// the collision material of the surface hit
			int16 material_index = NONE;
			const structure_bsp* bsp = global_structure_bsp_get();
			const structure_instanced_geometry_instance* instance = bsp ? bsp->instanced_geometry_instances[instance_index] : NULL;
			const structure_instanced_geometry_definition* definition = instance ? bsp->instanced_geometry_definitions[instance->instance_definition] : NULL;
			if (definition && VALID_INDEX(surface_index, definition->collision_info.surfaces.count))
			{
				const collision_surface* surface = (const collision_surface*)tag_block_get_element_with_size((const s_tag_block*)&definition->collision_info.surfaces, surface_index, sizeof(collision_surface));
				material_index = surface->material_index - state.instance.material_offset;
			}
			h1_scenery_cause_damage(&state, damage, material_index, damage->scale);
			return;
		}
		if (breakable_surface_index == NONE)
		{
			return;
		}
	}
	p_breakable_surface_damage(instance_index, breakable_surface_index, damage, surface_index);
	return;
}

// damage.c area_of_effect_cause_damage for halo 1 scenery, after halo 2's area damage of breakable surfaces
static void __cdecl h1_breakable_surfaces_area_damage(s_damage_data* damage)
{
	p_breakable_surfaces_area_damage(damage);
	if (!h1_maps_active() || !g_h1_cache_file || !damage || damage->definition_index == NONE)
	{
		return;
	}
	const h2x_jpt* definition = (const h2x_jpt*)tag_get('jpt!', damage->definition_index);
	if (!definition || definition->radius.upper <= 0.f)
	{
		return;
	}
	for (s_h1_scenery_state& state : g_h1_scenery.states)
	{
		if (state.definition_index == NONE)
		{
			continue;
		}
		const real_vector3d offset = { state.center.x - damage->epicenter.x, state.center.y - damage->epicenter.y, state.center.z - damage->epicenter.z };
		const real32 distance = sqrtf(offset.i * offset.i + offset.j * offset.j + offset.k * offset.k);
		if (distance >= definition->radius.upper)
		{
			continue;
		}
		real32 scale = 1.f;
		if (distance > definition->radius.lower && definition->radius.upper > definition->radius.lower)
		{
			scale = 1.f - (distance - definition->radius.lower) / (definition->radius.upper - definition->radius.lower);
		}
		h1_scenery_cause_damage(&state, damage, NONE, scale);
	}
	return;
}

// objects.c object_new for scenery placements: vitality (damage.c object_initialize_vitality), change colors, the placed permutation
static void h1_scenery_initialize(void)
{
	g_h1_scenery.initialized = true;
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();
	g_h1_scenery.states.assign(scenario->scenery.count, s_h1_scenery_state{});
	for (int32 i = 0; i < scenario->scenery.count; i++)
	{
		s_h1_scenery_state* state = &g_h1_scenery.states[i];
		state->definition_index = NONE;
		const h1_scnr_scenery* placement = g_h1_cache_file->block_get(scenario->scenery, i);
		const h1_scnr_scenery_palette* palette = g_h1_cache_file->block_get(scenario->scenery_palette, placement->palette_index);
		const h1_scen* scenery = palette ? (const h1_scen*)g_h1_cache_file->tag_get(palette->name) : NULL;
		if (!scenery)
		{
			continue;
		}
		state->definition_index = palette->name.index;
		state->collision_model_index = scenery->collision_model.index;
		state->instance = VALID_INDEX(i, (int32)g_h1_scenery.bindings.size()) ? g_h1_scenery.bindings[i] : s_h1_scenery_instance_binding{ NONE, NONE, NONE, 0 };
		state->center = placement->position;
		state->body_damage_decay_timer = NONE;
		state->shield_damage_decay_timer = NONE;

		const h1_coll* model = state->collision_model_index != NONE ? (const h1_coll*)g_h1_cache_file->tag_get('coll', state->collision_model_index) : NULL;
		state->body_vitality = model && model->maximum_body_vitality > 0.f ? 1.f : 0.f;
		state->shield_vitality = model && model->maximum_shield_vitality > 0.f ? 1.f : 0.f;

		// the instance's world bounding sphere center is the object's center
		const structure_bsp* bsp = global_structure_bsp_get();
		if (bsp && state->instance.instance_index != NONE && state->instance.instance_index < bsp->instanced_geometry_instances.count)
		{
			const structure_instanced_geometry_instance* instance = bsp->instanced_geometry_instances[state->instance.instance_index];
			state->center = *(const real_point3d*)((const uint8*)instance + 0x3C);
		}

		const h1_mode* render_model = scenery->model.index != NONE ? (const h1_mode*)g_h1_cache_file->tag_get('mode', scenery->model.index) : NULL;
		const int16 placed_permutation = MAX(placement->desired_permutation, (int16)0);
		for (int32 r = 0; r < k_h1_maximum_scenery_regions; r++)
		{
			state->region_permutations[r] = placed_permutation;
		}
		state->has_region_permutations = render_model && model && model->maximum_shield_vitality > 0.f;

		h1_object_functions_new(state->definition_index, &placement->position, &state->functions);
	}
	return;
}

// damage.c object_damage_update for scenery: shields recharge after their stun, recent damage decays
static void h1_scenery_tick(s_h1_scenery_state* state)
{
	const h1_coll* model = state->collision_model_index != NONE ? (const h1_coll*)g_h1_cache_file->tag_get('coll', state->collision_model_index) : NULL;
	if (!model)
	{
		return;
	}

	SET_BIT(state->damage_flags, _h1_scenery_shield_charging_bit, false);
	if (model->maximum_shield_vitality > 0.f && state->shield_vitality < 1.f)
	{
		if (state->shield_stun_ticks == 0)
		{
			if (TEST_BIT(state->damage_flags, _h1_scenery_shield_depleted_bit))
			{
				h1_scenery_effect(state, &model->shield_recharging_effect);
				SET_BIT(state->damage_flags, _h1_scenery_shield_depleted_bit, false);
				h1_scenery_shield_regions(state, true);
			}
			SET_BIT(state->damage_flags, _h1_scenery_shield_charging_bit, true);
			state->shield_vitality += model->shield_recharge_rate;
			if (state->shield_vitality > 1.f)
			{
				SET_BIT(state->damage_flags, _h1_scenery_shield_charging_bit, false);
				state->shield_vitality = 1.f;
			}
		}
		else
		{
			state->shield_stun_ticks--;
		}
	}

	if (state->body_damage_decay_timer != NONE)
	{
		const int32 decay_timer = ++state->body_damage_decay_timer;
		if (decay_timer >= 0)
		{
			state->current_body_damage -= k_h1_damage_decay_per_tick;
		}
		if (decay_timer >= (int32)k_h1_scenery_ticks_per_second * 2)
		{
			state->recent_body_damage -= k_h1_damage_decay_per_tick;
		}
		state->current_body_damage = MAX(0.f, state->current_body_damage);
		state->recent_body_damage = MAX(0.f, state->recent_body_damage);
		if (state->current_body_damage == 0.f && state->recent_body_damage == 0.f)
		{
			state->body_damage_decay_timer = NONE;
		}
	}
	if (state->shield_damage_decay_timer != NONE)
	{
		const int32 decay_timer = ++state->shield_damage_decay_timer;
		if (decay_timer >= 0)
		{
			state->current_shield_damage -= k_h1_damage_decay_per_tick;
		}
		if (decay_timer >= (int32)k_h1_scenery_ticks_per_second * 2)
		{
			state->recent_shield_damage -= k_h1_damage_decay_per_tick;
		}
		state->current_shield_damage = MAX(0.f, state->current_shield_damage);
		state->recent_shield_damage = MAX(0.f, state->recent_shield_damage);
		if (state->current_shield_damage == 0.f && state->recent_shield_damage == 0.f)
		{
			state->shield_damage_decay_timer = NONE;
		}
	}
	return;
}

// damage.c object_cause_damage, object_damage_shield and object_damage_body for scenery (it never dies)
static void h1_scenery_cause_damage(s_h1_scenery_state* state, const s_damage_data* damage, int16 material_index, real32 scale)
{
	const h1_coll* model = state->collision_model_index != NONE ? (const h1_coll*)g_h1_cache_file->tag_get('coll', state->collision_model_index) : NULL;
	const h2x_jpt* definition = damage && damage->definition_index != NONE ? (const h2x_jpt*)tag_get('jpt!', damage->definition_index) : NULL;
	if (!model || !definition || (model->maximum_body_vitality <= 0.f && model->maximum_shield_vitality <= 0.f))
	{
		return;
	}

	// the halo 1 damage effect it was built from has the material modifiers, halo 2 damage effects hurt every material alike
	const datum h1_damage_index = h1_damage_effect_h1_get(damage->definition_index);
	const h1_jpt* h1_damage = h1_damage_index != NONE ? (const h1_jpt*)g_h1_cache_file->tag_get('jpt!', h1_damage_index) : NULL;
	const real32* material_modifiers = h1_damage ? (const real32*)((const uint8*)h1_damage + 0x200) : NULL;

	scale = PIN(scale, 0.f, 1.f);
	const real32 random_damage = h1_scenery_random_range(definition->damage_upper_bound.lower, definition->damage_upper_bound.upper);
	real32 total_damage = (1.f - scale) * definition->damage_lower_bound + random_damage * scale;
	if (damage->multiplier > 0.f)
	{
		total_damage *= damage->multiplier;
	}
	if (total_damage <= 0.f)
	{
		return;
	}

	// the material hit, else the indirect damage material
	const h1_coll_materials* material = NULL;
	if (VALID_INDEX(material_index, model->materials.count))
	{
		material = g_h1_cache_file->block_get(model->materials, material_index);
	}
	else if (VALID_INDEX(model->indirect_damage_material_index, model->materials.count))
	{
		material = g_h1_cache_file->block_get(model->materials, model->indirect_damage_material_index);
	}
	const real32 shield_leak_fraction = material ? material->shield_leak_percentage : 0.f;
	const real32 shield_damage_multiplier = material ? material->shield_damage_multiplier : 1.f;
	const real32 body_damage_multiplier = material ? material->body_damage_multiplier : 1.f;
	const int16 body_material_type = material ? material->material_type : 0;

	// object_damage_shield
	real32 shield_damage = total_damage;
	if (state->shield_vitality > 0.f && model->maximum_shield_vitality > 0.f)
	{
		const real32 inverse_maximum_shield_vitality = 1.f / model->maximum_shield_vitality;
		shield_damage = MAX((1.f - shield_leak_fraction) * total_damage, 0.f);
		total_damage -= shield_damage;

		real32 actual_shield_damage = shield_damage_multiplier * shield_damage;
		if (material_modifiers && VALID_INDEX(model->shield_material_type, k_h1_material_type_count))
		{
			actual_shield_damage *= material_modifiers[model->shield_material_type];
		}
		const bool negligible_damage = actual_shield_damage < k_real_epsilon;

		const real32 normalized_shield_damage = actual_shield_damage * inverse_maximum_shield_vitality;
		if (normalized_shield_damage > state->shield_vitality)
		{
			const real32 excess_damage = actual_shield_damage - model->maximum_shield_vitality * state->shield_vitality;
			if (excess_damage > 0.f)
			{
				total_damage += excess_damage;
			}
			state->shield_vitality = 0.f;
			h1_scenery_deplete_shield(state);
		}
		else
		{
			state->shield_vitality -= normalized_shield_damage;
			if (!TEST_BIT(state->damage_flags, _h1_scenery_passed_shield_damage_threshold_bit) && state->shield_vitality < model->shield_damaged_threshold)
			{
				h1_scenery_effect(state, &model->shield_damaged_effect);
				SET_BIT(state->damage_flags, _h1_scenery_passed_shield_damage_threshold_bit, true);
			}
		}

		if (!negligible_damage)
		{
			state->shield_damage_decay_timer = 0;
			if (!TEST_BIT(state->damage_flags, _h1_scenery_shield_depleted_bit))
			{
				state->current_shield_damage = 1.f;
			}
			state->recent_shield_damage = MIN(state->recent_shield_damage + normalized_shield_damage, 1.f);
		}
	}
	else
	{
		shield_damage = 0.f;
		state->shield_vitality = 0.f;
	}
	if (model->maximum_shield_vitality > 0.f && (shield_damage >= model->minimum_stun_damage || state->shield_vitality == 0.f))
	{
		state->shield_stun_ticks = (int16)(model->stun_time * k_h1_scenery_ticks_per_second);
	}

	// object_damage_body (scenery isn't destroyed)
	if (total_damage > 0.f && model->maximum_body_vitality > 0.f)
	{
		real32 actual_damage = body_damage_multiplier * total_damage / model->maximum_body_vitality;
		if (material_modifiers && VALID_INDEX(body_material_type, k_h1_material_type_count))
		{
			actual_damage *= material_modifiers[body_material_type];
		}
		if (actual_damage > 0.f)
		{
			state->body_vitality = MAX(state->body_vitality - actual_damage, 0.f);
			state->body_damage_decay_timer = 0;
			state->current_body_damage = MIN(state->current_body_damage + actual_damage, 1.f);
			state->recent_body_damage = MIN(state->recent_body_damage + actual_damage, 1.f);
		}
	}
	return;
}

// damage.c object_deplete_shield
static void h1_scenery_deplete_shield(s_h1_scenery_state* state)
{
	if (TEST_BIT(state->damage_flags, _h1_scenery_shield_depleted_bit))
	{
		return;
	}
	const h1_coll* model = (const h1_coll*)g_h1_cache_file->tag_get('coll', state->collision_model_index);
	if (model)
	{
		h1_scenery_effect(state, &model->shield_depleted_effect);
	}
	state->current_shield_damage = 0.f;
	SET_BIT(state->damage_flags, _h1_scenery_shield_depleted_bit, true);
	SET_BIT(state->damage_flags, _h1_scenery_passed_shield_damage_threshold_bit, false);
	h1_scenery_shield_regions(state, false);
	return;
}

// damage.c object_permutation_shield_regions: the model regions missing while the shield is down draw their second permutation,
// and the instance collides as the collision model's second bsp
static void h1_scenery_shield_regions(s_h1_scenery_state* state, bool active)
{
	const h1_coll* model = (const h1_coll*)g_h1_cache_file->tag_get('coll', state->collision_model_index);
	const h1_scen* scenery = (const h1_scen*)g_h1_cache_file->tag_get('obje', state->definition_index);
	const h1_mode* render_model = scenery && scenery->model.index != NONE ? (const h1_mode*)g_h1_cache_file->tag_get('mode', scenery->model.index) : NULL;
	if (!model)
	{
		return;
	}
	for (int32 i = 0; i < model->regions.count; i++)
	{
		const h1_coll_regions* region = g_h1_cache_file->block_get(model->regions, i);
		if (!TEST_BIT(region->flags, _h1_region_missing_when_shield_is_zero_bit) || region->permutations.count <= 1 || !render_model)
		{
			continue;
		}
		// the model region of the same name
		for (int32 r = 0; r < render_model->regions.count && r < k_h1_maximum_scenery_regions; r++)
		{
			if (_stricmp(g_h1_cache_file->block_get(render_model->regions, r)->name, region->name) == 0)
			{
				state->region_permutations[r] = active ? 0 : 1;
			}
		}
	}

	structure_bsp* bsp = global_structure_bsp_get();
	if (bsp && state->instance.instance_index != NONE && state->instance.shield_off_definition_index != NONE &&
		state->instance.instance_index < bsp->instanced_geometry_instances.count)
	{
		structure_instanced_geometry_instance* instance = bsp->instanced_geometry_instances[state->instance.instance_index];
		instance->instance_definition = (uint16)(active ? state->instance.definition_index : state->instance.shield_off_definition_index);
	}
	return;
}

// damage.c damage_effect_new_on_object: at the object's center
static void h1_scenery_effect(const s_h1_scenery_state* state, const h1_tag_reference* effect)
{
	if (effect->index == NONE)
	{
		return;
	}
	const real_vector3d up = { 0.f, 0.f, 1.f };
	h1_effect_new_unattached(effect->index, &state->center, &up);
	return;
}

static real32 h1_scenery_random_range(real32 lower, real32 upper)
{
	g_h1_scenery.random = g_h1_scenery.random * 1664525u + 1013904223u;
	return lower + (upper - lower) * (real32)(g_h1_scenery.random >> 8) / 16777216.f;
}

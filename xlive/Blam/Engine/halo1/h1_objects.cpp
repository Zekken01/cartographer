#include "stdafx.h"
#include "h1_objects.h"
#include "h1_game_state.h"

#include "h1_cache_file.h"
#include "h1_effects.h"
#include "h1_log.h"
#include "h1_render_shaders.h"
#include "h1_vehicle_physics.h"
#include "h1_devices.h"
#include "h1_weapons.h"
#include "h1_weapon_logic.h"

#include "items/weapons.h"
#include "h1_render.h"
#include "h1_render_models.h"

#include "main/interpolator.h"
#include "math/matrix_math.h"
#include "objects/objects.h"
#include "units/units.h"
#include "objects/object_placement.h"
#include "camera/director.h"
#include "game/game.h"
#include "game/players.h"
#include "game/game_time.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

/* constants */

enum
{
	k_h1_maximum_object_nodes = 64,
};

/* structures */

struct s_h1_object_binding
{
	datum h1_definition_index;
	datum h1_model_index;
};



/* globals */

static std::unordered_map<datum, s_h1_object_binding> g_h1_object_bindings;
H1_GAME_STATE_VARIABLE(g_h1_object_bindings);
// the objects halo 2 renders: submitted this frame and drawn
static std::vector<datum> g_h1_objects_submitted;
static std::vector<datum> g_h1_objects_visible;
// the functions of each object, its change colors chosen when it was first drawn (where it was created)
static std::unordered_map<datum, s_h1_object_functions> g_h1_object_functions;
H1_GAME_STATE_VARIABLE(g_h1_object_functions);

/* public code */

void h1_objects_reset(void)
{
	g_h1_object_bindings.clear();
	g_h1_object_functions.clear();
	return;
}

void h1_objects_bind(datum h2_definition_index, datum h1_definition_index)
{
	// every halo 1 object definition starts with the object fields, the model is at 0x28
	const h1_vehi* h1_object = (const h1_vehi*)g_h1_cache_file->tag_get('obje', h1_definition_index);
	if (!h1_object)
	{
		return;
	}
	// objects without a model (bullets) are bound for their attachments and effects, nothing is drawn
	g_h1_object_bindings[h2_definition_index] = { h1_definition_index, h1_object->model.index };
	return;
}

int32 h1_objects_bound_definitions(datum* out_definitions, int32 maximum_count)
{
	int32 count = 0;
	for (const auto& binding : g_h1_object_bindings)
	{
		if (count < maximum_count)
		{
			out_definitions[count++] = binding.first;
		}
	}
	return count;
}

void h1_object_change_colors_choose(datum h1_definition_index, const real_point3d* position, real_rgb_color out_colors[4])
{
	// every halo 1 object definition starts with the object fields, the change colors are at 0x164
	const h1_scen* definition = h1_definition_index != NONE ? (const h1_scen*)g_h1_cache_file->tag_get('obje', h1_definition_index) : NULL;
	for (int32 i = 0; i < 4; i++)
	{
		// the placement's color, white
		out_colors[i] = { 1.f, 1.f, 1.f };
		if (!definition || i >= definition->change_colors.count)
		{
			continue;
		}
		const h1_scen_change_colors* change_color = g_h1_cache_file->block_get(definition->change_colors, i);
		const real32 weight = fmodf(fabsf(position->x * 315.89313f + position->y * 587.12946f + position->z * 744.12415f + (real32)i * 431.12894f), 1.f);
		for (int32 j = 0; j < change_color->permutations.count; j++)
		{
			const h1_scen_change_colors_permutations* permutation = g_h1_cache_file->block_get(change_color->permutations, j);
			if (weight <= permutation->weight)
			{
				h1_rgb_colors_interpolate(&out_colors[i], 1, &permutation->color_lower_bound, &permutation->color_upper_bound, fmodf(fabsf(position->y) + (real32)i * 0.71211f, 1.f));
				break;
			}
		}
		out_colors[i].red = PIN(out_colors[i].red, 0.f, 1.f);
		out_colors[i].green = PIN(out_colors[i].green, 0.f, 1.f);
		out_colors[i].blue = PIN(out_colors[i].blue, 0.f, 1.f);
	}
	return;
}

datum h1_objects_h1_definition_get(datum h2_definition_index)
{
	auto found = g_h1_object_bindings.find(h2_definition_index);
	return found != g_h1_object_bindings.end() ? found->second.h1_definition_index : NONE;
}

bool h1_objects_definition_bound(datum definition_index)
{
	return g_h1_object_bindings.find(definition_index) != g_h1_object_bindings.end();
}

bool h1_objects_render_replaced(datum object_index)
{
	if (g_h1_object_bindings.empty())
	{
		return false;
	}
	const object_datum* object = object_try_and_get(object_index);
	return object && g_h1_object_bindings.find(object->definition_index) != g_h1_object_bindings.end();
}

void h1_objects_render_submit(datum object_index, bool first_person)
{
	if (!first_person && std::find(g_h1_objects_submitted.begin(), g_h1_objects_submitted.end(), object_index) == g_h1_objects_submitted.end())
	{
		g_h1_objects_submitted.push_back(object_index);
	}
	return;
}

void h1_objects_render_frame_begin(void)
{
	g_h1_objects_visible.swap(g_h1_objects_submitted);
	g_h1_objects_submitted.clear();
	return;
}

// the unit a local player sees through in first person: halo 2 doesn't draw it or what it holds
static bool h1_object_is_first_person_unit(datum object_index)
{
	const datum ultimate_parent = object_get_ultimate_parent(object_index);
	for (int32 user_index = 0; user_index < k_number_of_users; user_index++)
	{
		const datum player_index = player_index_from_user_index(user_index);
		const player_datum* player = player_index != NONE ? player_get(player_index) : NULL;
		if (player && player->unit_index != NONE && player->unit_index == ultimate_parent && director_get_perspective(user_index) == _director_mode_game)
		{
			return true;
		}
	}
	return false;
}

bool h1_object_held_in_first_person(datum object_index)
{
	return h1_object_is_first_person_unit(object_index);
}

void h1_objects_render(e_h1_render_pass pass, real32 game_time)
{
	if (g_h1_object_bindings.empty())
	{
		return;
	}

	for (const datum object_index : g_h1_objects_visible)
	{
		const object_datum* object = (const object_datum*)object_try_and_get(object_index);
		auto found = object ? g_h1_object_bindings.find(object->definition_index) : g_h1_object_bindings.end();
		if (found == g_h1_object_bindings.end() || h1_object_is_first_person_unit(object_index))
		{
			continue;
		}
		const s_h1_object_binding* binding = &found->second;
		const h1_mode* model = (const h1_mode*)g_h1_cache_file->tag_get('mode', binding->h1_model_index);
		if (!model)
		{
			continue;
		}

		// the object's nodes are the halo 1 model's nodes
		const real_matrix4x3* node_matrices = NULL;
		int32 node_count = 0;
		if (!halo_interpolator_interpolate_object_node_matrices(object_index, &node_matrices, &node_count))
		{
			node_matrices = object_get_node_matrices(object_index, &node_count);
		}
		node_count = MIN(MIN(node_count, model->nodes.count), (int32)k_h1_maximum_object_nodes);
		if (!node_matrices || node_count <= 0)
		{
			continue;
		}

		real_matrix4x3 skinning_matrices[k_h1_maximum_object_nodes];
		for (int32 i = 0; i < node_count; i++)
		{
			const h1_mode_nodes* node = g_h1_cache_file->block_get(model->nodes, i);
			matrix4x3_multiply(&node_matrices[i], (const real_matrix4x3*)&node->inverse_scale, &skinning_matrices[i]);
		}

		s_h1_render_lighting lighting;
		// lights_prepare_for_object_static: from its bounding sphere's center (moving, sampled there alone)
		const datum h1_definition_index = h1_objects_h1_definition_get(object->definition_index);
		const uint16 h1_object_flags = h1_definition_index != NONE ? *(const uint16*)((const uint8*)g_h1_cache_file->tag_get('obje', h1_definition_index) + 2) : 0;
		h1_render_lighting_for_object(&object->object.center, object->object.radius, false, TEST_BIT(h1_object_flags, 2), &lighting);
		const s_h1_object_functions* functions = h1_object_functions_get(object_index);
		if (!functions)
		{
			continue;
		}
		// a camouflaged unit's camouflage covers it and what it holds
		const unit_datum* camouflaged = (const unit_datum*)object_try_and_get_and_verify_type(object_index, _object_mask_unit);
		if (!camouflaged && object->object.parent_object_index != NONE)
		{
			camouflaged = (const unit_datum*)object_try_and_get_and_verify_type(object->object.parent_object_index, _object_mask_unit);
		}
		const real32 camouflage = camouflaged ? camouflaged->unit.active_camouflage : 0.f;
		h1_render_model_draw_skinned(binding->h1_model_index, 0, skinning_matrices, node_count, &lighting, pass, game_time, functions->colors, functions->outgoing,
			camouflage);
	}
	return;
}

// objects.c OBJECT_INCOMING_FUNCTION_GET_VALUE: 1 to 4 are the incoming values, past them halo 1 reads on into the outgoing values
static real32 h1_object_function_value(const s_h1_object_functions* functions, int16 index)
{
	if (index >= 1 && index <= 4)
	{
		return functions->incoming[index - 1];
	}
	if (index >= 5 && index <= 8)
	{
		return functions->outgoing[index - 5];
	}
	return 0.f;
}

void h1_object_functions_new(datum h1_definition_index, const real_point3d* position, s_h1_object_functions* functions)
{
	memset(functions, 0, sizeof(*functions));
	h1_object_change_colors_choose(h1_definition_index, position, functions->base_colors);
	memcpy(functions->colors, functions->base_colors, sizeof(functions->colors));
	return;
}

// objects.c object_export_function_values
void h1_object_functions_export(datum h1_definition_index, const s_h1_object_vitality* vitality, s_h1_object_functions* functions)
{
	const h1_scen* definition = h1_definition_index != NONE ? (const h1_scen*)g_h1_cache_file->tag_get('obje', h1_definition_index) : NULL;
	if (!definition)
	{
		return;
	}
	const int16 modes[4] = { definition->a_in, definition->b_in, definition->c_in, definition->d_in };
	for (int32 i = 0; i < 4; i++)
	{
		real32 value = 0.f;
		switch (modes[i])
		{
		case _h1_object_function_none:
			continue;
		case _h1_object_function_body_vitality:
			value = vitality->body_vitality;
			break;
		case _h1_object_function_shield_vitality:
			value = MIN(vitality->shield_vitality, 1.f);
			break;
		case _h1_object_function_recent_body_damage:
			value = vitality->current_body_damage;
			break;
		case _h1_object_function_recent_shield_damage:
			value = vitality->current_shield_damage;
			break;
		case _h1_object_function_random_constant:
			if (functions->incoming[i] == 1.f)
			{
				value = (real32)rand() / (real32)RAND_MAX;
			}
			break;
		case _h1_object_function_alive:
			value = vitality->dead ? 0.f : 1.f;
			break;
		case _h1_object_function_compass:
			// the heading from the scenario's north, a turn mapped to 0 to 1 (straight up and down keeps the value)
			if (vitality->has_forward && fabsf(vitality->forward.k) < 0.995f)
			{
				const h1_scnr* scenario = g_h1_cache_file->scenario_get();
				real32 difference = atan2f(vitality->forward.i, vitality->forward.j) - (scenario ? scenario->local_north : 0.f);
				while (difference > _pi) difference -= 2.f * _pi;
				while (difference <= -_pi) difference += 2.f * _pi;
				value = PIN(0.15915494f * difference + 0.5f, 0.f, 1.f);
			}
			else
			{
				value = functions->incoming[i];
			}
			break;
		default:
			// umbrella shields, shield stun and region damage
			value = 0.f;
			break;
		}
		functions->incoming[i] = value;
	}
	return;
}

// objects.c object_compute_function_values and object_compute_change_colors
void h1_object_functions_update(datum h1_definition_index, int32 absolute_index, s_h1_object_functions* functions)
{
	const h1_scen* definition = h1_definition_index != NONE ? (const h1_scen*)g_h1_cache_file->tag_get('obje', h1_definition_index) : NULL;
	if (!definition)
	{
		return;
	}

	// halo 1 game time is in 30 hz ticks, halo 2's in 60 hz ones
	const real32 game_ticks = (real32)game_time_get() * 0.5f;
	const real32 huh = (57.f * (real32)absolute_index + game_ticks) * 0.033333335f;
	for (int32 function_index = 0; function_index < definition->functions.count && function_index < 4; function_index++)
	{
		const h1_scen_functions* function = g_h1_cache_file->block_get(definition->functions, function_index);
		bool function_is_active = true;
		real32 period = function->inverse_period;
		if (function->scale_period_by)
		{
			const real32 function_value = h1_object_function_value(functions, function->scale_period_by);
			if (function_value > 0.f)
			{
				period = period / function_value;
			}
		}

		real32 value = h1_periodic_function_evaluate(function->function, huh * period);
		if (function->scale_function_by)
		{
			value *= h1_object_function_value(functions, function->scale_function_by);
		}
		if (TEST_BIT(function->flags, _h1_object_function_invert_bit))
		{
			value = 1.f - value;
		}
		if (function->wobble_magnitude != 0.f)
		{
			const real32 wobble = h1_periodic_function_evaluate(function->wobble_function, huh * function->wobble_period);
			value += 2.f * function->wobble_magnitude * (wobble - 0.5f);
		}
		if (function->square_wave_threshold != 0.f)
		{
			value = value > function->square_wave_threshold ? 1.f : 0.f;
		}
		if (function->step_count > 1)
		{
			value = floorf((real32)function->step_count * value) * function->inverse_step;
		}
		if (function->inverse_sawtooth > 0.f)
		{
			value = fmodf(value, function->inverse_sawtooth);
		}
		if (function->add)
		{
			value = MIN(value + h1_object_function_value(functions, function->add), 1.f);
		}
		if (function->scale_result_by)
		{
			value *= h1_object_function_value(functions, function->scale_result_by);
		}

		real32 output = h1_transition_function_evaluate(function->map_to, value);
		if (function->scale_by > 0.f)
		{
			output *= function->scale_by;
		}
		if (function->bounds_mode == _h1_object_function_scale_to_fit_bounds)
		{
			output = output * (function->bounds.upper - function->bounds.lower) + function->bounds.lower;
			if (function->bounds.lower + k_real_epsilon >= output)
			{
				function_is_active = TEST_BIT(function->flags, _h1_object_function_does_not_deactivate_below_lower_bound_bit);
			}
		}
		else
		{
			if (function->bounds.lower + k_real_epsilon >= output)
			{
				output = function->bounds.lower;
				function_is_active = TEST_BIT(function->flags, _h1_object_function_does_not_deactivate_below_lower_bound_bit);
			}
			if (output > function->bounds.upper)
			{
				output = function->bounds.upper;
			}
			if (function->bounds_mode == _h1_object_function_clip_to_bounds_and_normalize)
			{
				output = (output - function->bounds.lower) * function->inverse_bounds;
			}
		}
		if (function->turn_off_with_index != NONE && VALID_INDEX(function->turn_off_with_index, 32) && !TEST_BIT(functions->active_flags, function->turn_off_with_index))
		{
			function_is_active = false;
		}
		// halo 1 tests the additive flag with the bounds mode's value
		if (TEST_BIT(function->flags, _h1_object_function_clip_to_bounds_and_normalize))
		{
			output = fmodf(output + functions->outgoing[function_index], 1.f);
		}
		functions->outgoing[function_index] = output;
		SET_BIT(functions->active_flags, function_index, function_is_active);
	}

	// change colors scaled or darkened by a function (each update starts from the colors the object was given)
	memcpy(functions->colors, functions->base_colors, sizeof(functions->colors));
	if (TEST_BIT(definition->runtime_flags, _h1_object_runtime_scaled_change_colors_bit))
	{
		for (int32 i = 0; i < definition->change_colors.count && i < 4; i++)
		{
			const h1_scen_change_colors* change_color = g_h1_cache_file->block_get(definition->change_colors, i);
			if (change_color->scale_by)
			{
				h1_rgb_colors_interpolate(&functions->colors[i], change_color->scale_flags, &change_color->color_lower_bound, &change_color->color_upper_bound,
					h1_object_function_value(functions, change_color->scale_by));
			}
			if (change_color->darken_by)
			{
				const real32 scale = h1_object_function_value(functions, change_color->darken_by);
				functions->colors[i].red *= scale;
				functions->colors[i].green *= scale;
				functions->colors[i].blue *= scale;
			}
			functions->colors[i].red = PIN(functions->colors[i].red, 0.f, 1.f);
			functions->colors[i].green = PIN(functions->colors[i].green, 0.f, 1.f);
			functions->colors[i].blue = PIN(functions->colors[i].blue, 0.f, 1.f);
		}
	}
	return;
}

// weapons.c weapon_export_function_values: the weapon modes fill the inputs from halo 2's weapon state
static void h1_weapon_functions_export(const weapon_datum* weapon, datum object_index, datum h1_weapon_index, s_h1_object_functions* functions)
{
	const h1_weap* definition = (const h1_weap*)g_h1_cache_file->tag_get('weap', h1_weapon_index);
	if (!definition)
	{
		return;
	}
	const int16 modes[4] = { definition->a_in_3, definition->b_in_3, definition->c_in_3, definition->d_in_3 };
	for (int32 i = 0; i < 4; i++)
	{
		if (modes[i] == _h1_weapon_function_none)
		{
			continue;
		}
		real32 value = 0.f;
		const int32 trigger_index = modes[i] == _h1_weapon_function_secondary_ammunition || modes[i] == _h1_weapon_function_secondary_rate_of_fire ||
			modes[i] == _h1_weapon_function_secondary_ejection_port || modes[i] == _h1_weapon_function_secondary_charged || modes[i] == _h1_weapon_function_secondary_firing ? 1 : 0;
		const bool trigger_valid = trigger_index < definition->triggers.count;
		switch (modes[i])
		{
		case _h1_weapon_function_ready:
			value = 1.f;
			break;
		case _h1_weapon_function_heat:
			value = weapon->weapon.heat;
			break;
		case _h1_weapon_function_overheated:
			if (definition->heat_recovery_threshold != 1.f && weapon->weapon.heat >= definition->overheated_threshold)
			{
				value = (weapon->weapon.heat - definition->heat_recovery_threshold) / (1.f - definition->heat_recovery_threshold);
			}
			break;
		case _h1_weapon_function_illumination:
			for (int32 t = 0; t < definition->triggers.count && t < k_weapon_barrel_count; t++)
			{
				value = MAX(value, weapon->weapon.barrels[t].illumination);
			}
			value = MAX(value, definition->heat_illumination * weapon->weapon.heat);
			break;
		case _h1_weapon_function_primary_ammunition:
		case _h1_weapon_function_secondary_ammunition:
		{
			// the magazine's rounds loaded out of its maximum, from halo 1's weapon state
			s_h1_weapon_interface_state weapon_state;
			if (trigger_index < definition->magazines.count && h1_weapon_logic_interface_state(object_index, &weapon_state) &&
				trigger_index < weapon_state.magazine_count && weapon_state.magazines[trigger_index].rounds_loaded_maximum > 0)
			{
				value = (real32)weapon_state.magazines[trigger_index].rounds_loaded / (real32)weapon_state.magazines[trigger_index].rounds_loaded_maximum;
			}
		}
			break;
		case _h1_weapon_function_primary_ejection_port:
		case _h1_weapon_function_secondary_ejection_port:
			value = trigger_valid ? weapon->weapon.barrels[trigger_index].ejection_port_position : 0.f;
			break;
		case _h1_weapon_function_primary_rate_of_fire:
		case _h1_weapon_function_secondary_rate_of_fire:
			value = trigger_valid ? weapon->weapon.barrels[trigger_index].rate_of_fire : 0.f;
			break;
		case _h1_weapon_function_primary_firing:
		case _h1_weapon_function_secondary_firing:
			// halo 1 counts 30 hz ticks, halo 2 60 hz ones
			value = trigger_valid && (int32)game_time_get() - weapon->weapon.game_time_last_fired <= 2 ? weapon->weapon.barrels[trigger_index].rate_of_fire : 0.f;
			break;
		case _h1_weapon_function_age:
			value = weapon->weapon.age;
			break;
		case _h1_weapon_function_primary_charged:
		case _h1_weapon_function_secondary_charged:
			value = h1_weapon_logic_charged_fraction(object_index, (int16)trigger_index);
			break;
		default:
			// charged fraction and the integrated light (halo 2 keeps the flashlight on the unit)
			value = 0.f;
			break;
		}
		functions->incoming[i] = PIN(value, 0.f, 1.f);
	}
	return;
}

void h1_objects_update_functions(void)
{
	if (g_h1_object_bindings.empty())
	{
		return;
	}
	object_iterator iterator;
	object_iterator_new(&iterator, _object_mask_all, 0);
	while (const object_datum* object = (const object_datum*)object_iterator_next(&iterator))
	{
		auto found = g_h1_object_bindings.find(object->definition_index);
		if (found == g_h1_object_bindings.end())
		{
			continue;
		}
		const s_h1_object_binding* binding = &found->second;
		const datum object_index = iterator.index;
		auto found_functions = g_h1_object_functions.find(object_index);
		if (found_functions == g_h1_object_functions.end())
		{
			s_h1_object_functions created;
			h1_object_functions_new(binding->h1_definition_index, &object->object.position, &created);
			found_functions = g_h1_object_functions.insert({ object_index, created }).first;
		}
		s_h1_object_functions* functions = &found_functions->second;

		// halo 2 keeps the object's vitality and damage
		s_h1_object_vitality vitality;
		vitality.body_vitality = object->object.body_vitality;
		vitality.shield_vitality = object->object.shield_vitality;
		vitality.current_body_damage = object->object.current_body_damage;
		vitality.current_shield_damage = object->object.current_shield_damage;
		vitality.dead = false;
		// a held weapon points where its holder aims
		vitality.has_forward = true;
		vitality.forward = object->object.forward;
		const unit_datum* holder = object->object.parent_object_index != NONE ?
			(const unit_datum*)object_try_and_get_and_verify_type(object_get_ultimate_parent(object_index), _object_mask_unit) : NULL;
		if (holder)
		{
			vitality.forward = holder->unit.aiming_vector;
		}
		h1_object_functions_export(binding->h1_definition_index, &vitality, functions);
		h1_vehicle_functions_export(object_index, functions->incoming);
		h1_device_functions_export(object_index, functions->incoming);
		const datum h1_weapon_index = h1_weapon_h1_get(object->definition_index);
		if (h1_weapon_index != NONE)
		{
			h1_weapon_functions_export((const weapon_datum*)object, object_index, h1_weapon_index, functions);
		}
		h1_object_functions_update(binding->h1_definition_index, DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index), functions);
	}

	// objects that are gone
	for (auto it = g_h1_object_functions.begin(); it != g_h1_object_functions.end();)
	{
		it = object_try_and_get(it->first) ? std::next(it) : g_h1_object_functions.erase(it);
	}
	return;
}

const s_h1_object_functions* h1_object_functions_get(datum object_index)
{
	auto found = g_h1_object_functions.find(object_index);
	return found != g_h1_object_functions.end() ? &found->second : NULL;
}

void h1_object_change_colors_set(datum object_index, const real_rgb_color colors[4])
{
	auto found = g_h1_object_functions.find(object_index);
	if (found != g_h1_object_functions.end())
	{
		memcpy(found->second.base_colors, colors, sizeof(found->second.base_colors));
	}
	return;
}

bool h1_object_function_value_get(datum object_index, int16 function_index, real32* out_value)
{
	*out_value = 1.f;
	if (function_index == NONE)
	{
		return true;
	}
	const s_h1_object_functions* functions = h1_object_functions_get(object_index);
	if (!functions || !VALID_INDEX(function_index, 4))
	{
		return true;
	}
	*out_value = functions->outgoing[function_index];
	return TEST_BIT(functions->active_flags, function_index);
}

#include "stdafx.h"
#include "h1_objects.h"

#include "h1_cache_file.h"
#include "h1_effects.h"
#include "h1_log.h"
#include "h1_render.h"
#include "h1_render_models.h"

#include "main/interpolator.h"
#include "math/matrix_math.h"
#include "objects/objects.h"
#include "objects/object_placement.h"
#include "game/game.h"

#include <unordered_map>

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

struct s_h1_object_change_colors
{
	real_rgb_color colors[4];
};

/* globals */

static std::unordered_map<datum, s_h1_object_binding> g_h1_object_bindings;
// the change colors each object was given when it was first drawn (where it was created)
static std::unordered_map<datum, s_h1_object_change_colors> g_h1_object_change_colors;

/* public code */

void h1_objects_reset(void)
{
	g_h1_object_bindings.clear();
	g_h1_object_change_colors.clear();
	return;
}

void h1_objects_bind(datum h2_definition_index, datum h1_definition_index)
{
	// every halo 1 object definition starts with the object fields, the model is at 0x28
	const h1_vehi* h1_object = (const h1_vehi*)g_h1_cache_file->tag_get('obje', h1_definition_index);
	if (!h1_object || h1_object->model.index == NONE)
	{
		return;
	}
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

void h1_objects_render(e_h1_render_pass pass, real32 game_time)
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
		h1_render_lighting_at(&node_matrices[0].position, &lighting);
		auto colors = g_h1_object_change_colors.find(object_index);
		if (colors == g_h1_object_change_colors.end())
		{
			s_h1_object_change_colors chosen;
			h1_object_change_colors_choose(binding->h1_definition_index, &object->object.position, chosen.colors);
			colors = g_h1_object_change_colors.insert({ object_index, chosen }).first;
		}
		h1_render_model_draw_skinned(binding->h1_model_index, 0, skinning_matrices, node_count, &lighting, pass, game_time, colors->second.colors);
	}
	return;
}

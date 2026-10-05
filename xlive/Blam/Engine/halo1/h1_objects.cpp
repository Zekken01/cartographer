#include "stdafx.h"
#include "h1_objects.h"

#include "h1_cache_file.h"
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

/* globals */

static std::unordered_map<datum, s_h1_object_binding> g_h1_object_bindings;

/* public code */

void h1_objects_reset(void)
{
	g_h1_object_bindings.clear();
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
		h1_render_model_draw_skinned(binding->h1_model_index, 0, skinning_matrices, node_count, &lighting, pass, game_time);
	}
	return;
}

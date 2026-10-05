#include "stdafx.h"
#include "h1_first_person.h"

#include "h1_cache_file.h"
#include "h1_log.h"
#include "h1_map_loader.h"
#include "h1_object_tags.h"
#include "h1_objects.h"
#include "h1_render.h"
#include "h1_render_models.h"
#include "h1_runtime.h"
#include "h2_tag_definitions_generated.h"

#include "game/game_globals.h"
#include "interface/first_person_weapons.h"
#include "math/matrix_math.h"
#include "rasterizer/dx9/rasterizer_dx9_main.h"
#include "render/render.h"

#include <unordered_map>
#include <vector>

/* constants */

enum
{
	k_h1_maximum_first_person_models = 4,
	k_h1_maximum_first_person_nodes = 64,
};

// first person geometry is drawn into the nearest part of the depth range so the world never covers it
static const real32 k_h1_first_person_depth_range = 0.02f;

/* structures */

struct s_h1_first_person_model
{
	datum h1_model_index;
	datum object_index;
	int32 node_count;
	real_matrix4x3 nodes[k_h1_maximum_first_person_nodes];
};

/* globals */

static std::unordered_map<datum, datum> g_h1_first_person_models;	// halo 2 render model: halo 1 model
static std::vector<s_h1_first_person_model> g_h1_first_person_frame;

/* public code */

void h1_first_person_reset(void)
{
	g_h1_first_person_models.clear();
	g_h1_first_person_frame.clear();
	return;
}

void h1_first_person_model_register(datum h2_render_model_index, datum h1_model_index)
{
	if (h2_render_model_index != NONE && h1_model_index != NONE)
	{
		g_h1_first_person_models[h2_render_model_index] = h1_model_index;
	}
	return;
}

int32 h1_first_person_models_submit(int32 user_index, s_first_person_model_data* models, int32 model_count)
{
	g_h1_first_person_frame.clear();
	if (!h1_maps_active() || g_h1_first_person_models.empty() || !models)
	{
		return model_count;
	}
	int32 kept_count = 0;
	for (int32 i = 0; i < model_count && i < k_h1_maximum_first_person_models; i++)
	{
		auto found = g_h1_first_person_models.find(models[i].render_model_index);
		const h1_mode* h1_model = found != g_h1_first_person_models.end() ? (const h1_mode*)g_h1_cache_file->tag_get('mode', found->second) : NULL;
		if (!h1_model)
		{
			if (found == g_h1_first_person_models.end())
			{
				// a halo 2 model stays for halo 2
				if (kept_count != i)
				{
					csmemcpy(&models[kept_count], &models[i], sizeof(s_first_person_model_data));
				}
				kept_count++;
			}
			continue;
		}
		s_h1_first_person_model model;
		model.h1_model_index = found->second;
		model.object_index = models[i].object_index;
		model.node_count = MIN(h1_model->nodes.count, (int32)k_h1_maximum_first_person_nodes);
		csmemcpy(model.nodes, models[i].nodes, sizeof(real_matrix4x3) * model.node_count);
		g_h1_first_person_frame.push_back(model);
	}
	return kept_count;
}

void h1_first_person_render(real32 game_time)
{
	if (g_h1_first_person_frame.empty())
	{
		return;
	}
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	D3DVIEWPORT9 viewport;
	device->GetViewport(&viewport);
	D3DVIEWPORT9 first_person_viewport = viewport;
	first_person_viewport.MinZ = 0.f;
	first_person_viewport.MaxZ = k_h1_first_person_depth_range;
	device->SetViewport(&first_person_viewport);

	// lit by the structure at the camera
	s_h1_render_lighting lighting;
	h1_render_lighting_at(&global_window_parameters_get()->camera.point, &lighting);

	for (int32 pass = _h1_render_pass_opaque; pass <= _h1_render_pass_transparent; pass++)
	{
		for (const s_h1_first_person_model& model : g_h1_first_person_frame)
		{
			const h1_mode* h1_model = (const h1_mode*)g_h1_cache_file->tag_get('mode', model.h1_model_index);
			real_matrix4x3 skinning_matrices[k_h1_maximum_first_person_nodes];
			for (int32 i = 0; i < model.node_count; i++)
			{
				const h1_mode_nodes* node = g_h1_cache_file->block_get(h1_model->nodes, i);
				matrix4x3_multiply(&model.nodes[i], (const real_matrix4x3*)&node->inverse_scale, &skinning_matrices[i]);
			}
			// the weapon's functions (ammunition counters, heat) and change colors
			const s_h1_object_functions* functions = h1_object_functions_get(model.object_index);
			h1_render_model_draw_skinned(model.h1_model_index, 0, skinning_matrices, model.node_count, &lighting, (e_h1_render_pass)pass, game_time,
				functions ? functions->colors : NULL, functions ? functions->outgoing : NULL);
		}
	}

	device->SetViewport(&viewport);
	return;
}

void h1_first_person_hands_build(void)
{
	const datum h1_globals_index = g_h1_cache_file->tag_find('matg', "globals\\globals");
	const h1_matg* h1_globals = h1_globals_index != NONE ? (const h1_matg*)g_h1_cache_file->tag_get('matg', h1_globals_index) : NULL;
	h2x_matg* globals = (h2x_matg*)scenario_get_game_globals();
	const h1_matg_first_person_interface* interface_definition = h1_globals ? g_h1_cache_file->block_get(h1_globals->first_person_interface, 0) : NULL;
	const h1_mode* h1_hands = interface_definition ? (const h1_mode*)g_h1_cache_file->tag_get('mode', interface_definition->first_person_hands.index) : NULL;
	if (!globals || !h1_hands)
	{
		h1_log("first person: no halo 1 hands");
		return;
	}

	char name[256];
	sprintf_s(name, "halo1\\%s", g_h1_cache_file->tag_name_get(interface_definition->first_person_hands.index));
	datum hands = h1_runtime_tag_find('mode', name);
	if (hands == NONE)
	{
		hands = h1_object_render_model_build(h1_hands, name);
	}
	if (hands == NONE)
	{
		return;
	}
	h1_first_person_model_register(hands, interface_definition->first_person_hands.index);
	for (int32 i = 0; i < globals->player_representation.count; i++)
	{
		h2x_matg_player_representation* representation = globals->player_representation[i];
		h1_runtime_reference_set(&representation->first_person_hands_model, 'mode', hands);
		h1_runtime_reference_set(&representation->first_person_body_model, (tag_group)NONE, NONE);
	}
	h1_log("first person: hands are %s", name);
	return;
}

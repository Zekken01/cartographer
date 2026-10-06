#pragma once
#include "h1_render_shaders.h"

/*
* Halo 1 Xbox models (mode): every model of the cache file is decoded once into a shared vertex and
* index buffer (triangle strips), and drawn by region permutation at the highest level of detail.
*/

bool h1_render_models_initialize(void);
void h1_render_models_dispose(void);

// draws the parts of a model whose shaders belong to the pass
// change colors are the object's four change colors, white when NULL
// function values are its four outgoing function values (0 when NULL), region permutations override the permutation per region
void h1_render_model_draw(datum model_tag_index, int16 permutation, const real_matrix4x3* object_to_world, const s_h1_render_lighting* lighting, e_h1_render_pass pass, bool sky, real32 game_time,
	const real_rgb_color* change_colors = NULL, const real32* function_values = NULL, const int16* region_permutations = NULL);

// draws a model whose vertices follow node matrices (model space to world space per node)
// (camouflage: an active camouflaged unit's, render_objects.c: the opaque pass only lays down its depth, the transparent pass
// draws it under the camouflage when it's partly camouflaged)
void h1_render_model_draw_skinned(datum model_tag_index, int16 permutation, const real_matrix4x3* node_matrices, int32 node_count, const s_h1_render_lighting* lighting, e_h1_render_pass pass, real32 game_time,
	const real_rgb_color* change_colors = NULL, const real32* function_values = NULL, real32 camouflage = 0.f, real32 hyper_stealth = 0.f);

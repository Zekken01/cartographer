#pragma once
#include "h1_render_shaders.h"

/*
* Halo 1 Xbox models (mode): every model of the cache file is decoded once into a shared vertex and
* index buffer (triangle strips), and drawn by region permutation at the highest level of detail.
*/

bool h1_render_models_initialize(void);
void h1_render_models_dispose(void);

// draws the parts of a model whose shaders belong to the pass
void h1_render_model_draw(datum model_tag_index, int16 permutation, const real_matrix4x3* object_to_world, const s_h1_render_lighting* lighting, e_h1_render_pass pass, bool sky, real32 game_time);

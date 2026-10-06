#pragma once
#include "h1_render_shaders.h"

/*
* Halo 1 first person: Halo 2 animates the first person hands and weapon models built from the Halo 1 first person models and
* animation graphs (h1_weapons, h1_animations) and builds their node matrices; their Halo 1 geometry is drawn here with them.
*/

struct s_first_person_model_data;

// first person geometry is drawn into the nearest part of the depth range so the world never covers it
static const real32 k_h1_first_person_depth_range = 0.02f;

// forgets the first person models (a new map is being built)
void h1_first_person_reset(void);

// a halo 2 render model built from a halo 1 first person model (the hands or a weapon's)
void h1_first_person_model_register(datum h2_render_model_index, datum h1_model_index);

// the first person models halo 2 built this frame (interface/first_person_weapons first_person_weapon_build_models): the halo 1
// ones are kept for h1_first_person_render and taken out of the list, the count of the models left for halo 2 is returned
int32 h1_first_person_models_submit(int32 user_index, s_first_person_model_data* models, int32 model_count);

// the world matrix of a marker of the halo 1 first person model of an object (the local player's weapon), false if it has none
bool h1_first_person_marker_get(datum object_index, const char* marker_name, real_matrix4x3* out_matrix);

// draws the halo 1 first person models in front of the scene (after the transparent geometry)
// the first person weapon isn't drawn (zoomed): what is drawn at its markers neither
bool h1_first_person_hidden(void);
void h1_first_person_render(real32 game_time);

// the halo 1 hands of the globals become the halo 2 first person hands (no first person body)
void h1_first_person_hands_build(void);

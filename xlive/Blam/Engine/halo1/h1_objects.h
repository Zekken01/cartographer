#pragma once
#include "h1_render_shaders.h"

/*
* Halo 1 objects built from Halo 1 tags.
*
* Halo 1 vehicles become Halo 2 definitions made only from their Halo 1 tags (h1_vehicles), with a
* render model that carries the Halo 1 nodes and markers but no geometry. Halo 2 simulates them and
* computes their (Halo 1) node matrices; the Halo 1 model is drawn here with its own shaders.
*/

// forgets every binding (a new map is being built)
void h1_objects_reset(void);

// objects with the halo 2 definition draw the halo 1 definition's model
void h1_objects_bind(datum h2_definition_index, datum h1_definition_index);

// halo 2 definitions bound so far (for the simulation definition table)
int32 h1_objects_bound_definitions(datum* out_definitions, int32 maximum_count);

// true for halo 2 definitions built from halo 1 objects
bool h1_objects_definition_bound(datum definition_index);

// true when the object draws its halo 1 model instead of a halo 2 render model
bool h1_objects_render_replaced(datum object_index);

// draws the halo 1 models of every bound object
void h1_objects_render(e_h1_render_pass pass, real32 game_time);

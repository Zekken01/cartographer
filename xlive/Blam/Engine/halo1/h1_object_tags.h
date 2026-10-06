#pragma once

/*
* Halo 2 object tags built from Halo 1 object tags.
*
*   render model  carries the Halo 1 nodes, regions and markers but no geometry (h1_objects draws the model)
*   collision     the Halo 1 collision model
*   model         ties them together with a default variant, materials, damage and collision regions
* Halo 1 names become string ids (added to the string table when Halo 2 doesn't have them).
*/

struct h1_mode;
struct h1_coll;

struct s_h1_object_tags
{
	datum render_model;
	datum collision_model;
	datum physics_model;	// NONE: none
	datum animation_graph;	// NONE: none
	real32 disappear_distance;
};

datum h1_object_render_model_build(const h1_mode* h1_model, const char* name);
datum h1_object_collision_model_build(const h1_coll* h1_collision, const h1_mode* h1_model, const char* name);
datum h1_object_model_build(const s_h1_object_tags* tags, const h1_mode* h1_model, const h1_coll* h1_collision, const char* name);
// a physics model of keyframed bodies over the collision model's triangles (each region's first permutation), one per node
// with collision: halo 2's bipeds and vehicles collide only with physics
datum h1_object_physics_model_build(const h1_coll* h1_collision, const h1_mode* h1_model, datum collision_model_index, const char* name);

// the name of a halo 2 global material (matg materials)
string_id h1_global_material_name(int16 global_material_index);

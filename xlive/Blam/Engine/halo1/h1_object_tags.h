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
struct h2x_phmo_triangles;
struct h2x_phmo_lists;
struct h2x_phmo_list_shapes;
struct h2x_phmo;

#include <vector>

// halo 2's physics model shapes and motions (phmo)
enum
{
	k_h2_physics_shape_type_triangle = 3,
	k_h2_physics_shape_type_list = 14,
	k_h2_physics_shape_type_mopp = 15,
	k_h2_physics_motion_type_keyframed = 4,
	k_h2_physics_list_children = 4,
	// FUN_004e08de copies a list's children to a stack array of 34 when it rebuilds a body's shape
	k_h2_physics_list_maximum_children = 32,
};

struct s_h2_physics_shape
{
	int16 type;
	int16 index;
};

struct s_h1_physics_triangle
{
	real_point3d points[3];
};

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
// the triangles of a built collision model's bsps (each region's first permutation) by the model node they move with, in its space
void h1_collision_model_node_triangles(datum collision_model_index, std::vector<int16>& out_nodes, std::vector<std::vector<s_h1_physics_triangle>>& out_triangles);
// every node's default matrix in its model
void h1_model_default_node_matrices(const h1_mode* h1_model, std::vector<real_matrix4x3>& out_matrices);
// a physics model triangle (its bounds grown by it)
void h1_physics_triangle_set(h2x_phmo_triangles* triangle, const s_h1_physics_triangle* source, real_rectangle3d* bounds);
// the physics model's only list (over the leaves) and mopp (over the list, from the leaves' bounds): the mopp shape
s_h2_physics_shape h1_physics_mopp_build(h2x_phmo* physics, const std::vector<std::pair<int16, int16>>& leaves, const std::vector<real_rectangle3d>& leaf_bounds);
// the lists and list shapes of a tree of lists over the leaves (added to the counts)
void h1_physics_list_tree_count(int32 leaf_count, int32* list_count, int32* list_shape_count);
// lists of up to 32 over the leaves up to one root, which is left in the level (more than four children come from the list shapes)
void h1_physics_list_tree_build(h2x_phmo_lists* lists, int32* list_index, h2x_phmo_list_shapes* list_shapes, int32* list_shape_index, std::vector<std::pair<int16, int16>>& level);

// the name of a halo 2 global material (matg materials)
string_id h1_global_material_name(int16 global_material_index);

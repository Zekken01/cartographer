#pragma once

/*
* Havok MOPP code generation for Halo 2 structure physics.
*
* Halo 2 runs Havok queries (vehicles, physics objects, items) against the structure through a
* MOPP bounding volume tree. Structure terminals are 0x20000000 | collision surface and
* 0x40000000 | instanced geometry instance; instanced geometry definitions have their own tree
* with plain surface indices. The bytecode follows the interpreter in halo2.exe (FUN_006df589):
* we emit a binary tree of axis split nodes at the 8 bit precision level, which is conservative
* and always correct.
*
* Blob layout: float3 origin, float scale (to 24 bit coordinates), 0x10 flags (0xFF),
* 0x20 size, 0x30 bytecode.
*/

#include <vector>

struct collision_bsp;

enum
{
	k_mopp_structure_surface_key = 0x20000000,
	k_mopp_instanced_geometry_key = 0x40000000,
};

struct s_h1_mopp_item
{
	uint32 key;
	real_rectangle3d bounds;
};

// adds every surface of the collision bsp, keys are key_base | surface index
void h1_mopp_collect_surfaces(const collision_bsp* bsp, uint32 key_base, std::vector<s_h1_mopp_item>& items);

// bounds of a collision bsp, transformed into world space when a matrix is given
bool h1_mopp_collision_bsp_bounds(const collision_bsp* bsp, const real_matrix4x3* matrix, real_rectangle3d* out_bounds);

// builds a mopp blob over the items (free with h1_mopp_free)
bool h1_mopp_build(const std::vector<s_h1_mopp_item>& items, uint8** out_blob, uint32* out_size, real_point3d* out_bounds_min, real_point3d* out_bounds_max);

void h1_mopp_free(uint8* blob);

#pragma once

/*
* Havok MOPP code generation for Halo 2 structure physics.
*
* Halo 2 runs Havok queries (vehicles, physics objects, items) against the structure through a
* MOPP bounding volume tree whose terminals are collision surface indices (0x20000000 | surface).
* The bytecode format follows the interpreter in halo2.exe (FUN_006df589): we emit a binary tree
* of axis split nodes at the 8 bit precision level, which is conservative and always correct.
*
* Blob layout: float3 origin, float scale (to 24 bit coordinates), 0x10 flags (0xFF),
* 0x20 size, 0x30 bytecode.
*/

struct collision_bsp;

// builds the mopp blob for every surface of the collision bsp, returns the blob and its bounds
bool h1_mopp_build_for_collision_bsp(const collision_bsp* bsp, uint8** out_blob, uint32* out_size, real_point3d* out_bounds_min, real_point3d* out_bounds_max);

void h1_mopp_free(uint8* blob);

#pragma once

/*
* Builds a Halo 2 structure bsp (and its structure lightmap) from a Halo 1 structure bsp.
* Collision, leaves, clusters, portals and visibility are carried over. Cluster render geometry
* stays empty: Halo 1 structure geometry is drawn by the Halo 1 structure renderer (h1_render).
*/

// writes the new tag data into the given (existing) sbsp and ltmp tag instances
bool h1_structure_bsp_build(int32 h1_bsp_index, datum h2_structure_bsp_index, datum h2_lightmap_index);

// Halo 2 global material index for a Halo 1 material type
int16 h1_material_type_to_global_material(int16 h1_material_type);

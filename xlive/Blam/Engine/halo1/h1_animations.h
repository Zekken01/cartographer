#pragma once

/*
* Halo 1 animation graphs (antr) as Halo 2 animation graphs (jmad).
*
* Every Halo 1 animation is decoded (8 byte quaternions, translations and scales per frame, the rest from its default data)
* and written with the Halo 2 uncompressed animated codec (8): a 32 byte header, per node float quaternions, translations and
* scales for every frame, then the rotation, translation and scale node flag bit vectors. Base animations carry every node,
* overlays only their animated nodes.
*/

// the halo 2 first person animation graph of a halo 1 first person weapon animation graph: the skeleton is the halo 1 graph's
// nodes (the arms and the weapon), its first person weapon animations become the halo 2 first person actions and overlays
datum h1_first_person_animation_graph_build(datum h1_animation_graph_index, const char* name);

#pragma once

/*
* Halo 1 renderer.
*
* Draws the Halo 1 structure bsp, sky and scenery with the Halo 2 device inside the Halo 2 scene,
* using Halo 2's camera so depth is shared with everything Halo 2 draws.
*/

// called before the halo 2 opaque structure pass of a scene
void h1_render_structure_opaque(void);

// called with the halo 2 transparent geometry
void h1_render_structure_transparent(void);

// called instead of the halo 2 sky, returns true if the halo 1 sky was drawn
bool h1_render_sky(void);

// frees every Direct3D resource built for the loaded Halo 1 map
void h1_render_dispose(void);

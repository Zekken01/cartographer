#pragma once

/*
* Halo 1 renderer.
*
* Draws the Halo 1 structure bsp, sky and scenery with the Halo 2 device inside the Halo 2 scene,
* using Halo 2's camera so depth is shared with everything Halo 2 draws.
*/

// keeps the device's state (render, sampler and texture stage states, shaders, constants) across a halo 1 draw: halo 2's
// rasterizer caches the states it set, so a state the halo 1 renderer changes and leaves would show in what halo 2 draws next
// (the interface's menus)
class c_h1_render_state_guard
{
public:
	c_h1_render_state_guard(void);
	~c_h1_render_state_guard(void);

private:
	struct IDirect3DStateBlock9* m_state_block;
};

// called before the halo 2 opaque structure pass of a scene
void h1_render_structure_opaque(void);

// called with the halo 2 transparent geometry
void h1_render_structure_transparent(void);

// called instead of the halo 2 sky, returns true if the halo 1 sky was drawn
bool h1_render_sky(void);

// object lighting at a point: the lightmap material under it and its lightmap sample
bool h1_render_lighting_at(const real_point3d* point, struct s_h1_render_lighting* out_lighting, bool brighten = false);
// object_lights.c lights_prepare_for_object_static: an object's lighting from its bounding sphere's center, and (with corners) its
// four corners around it, averaged
void h1_render_lighting_for_object(const real_point3d* center, real32 radius, bool corners, bool brighten, struct s_h1_render_lighting* out_lighting);

// object_lights.c light_particle: the lightmap's light (brightened a tenth) and the diffuse texture's color of the structure under
// a point, grey without one
void h1_render_light_particle(const real_point3d* point, real_rgb_color* out_light, real_rgb_color* out_diffuse);

// the opaque lightmapped structure triangles (three points each), for decals
int32 h1_render_structure_triangle_count(void);
const real_point3d* h1_render_structure_triangle_get(int32 index);

// frees every Direct3D resource built for the loaded Halo 1 map
void h1_render_dispose(void);

#pragma once

/*
* Halo 1 fog for the Halo 1 renderer: the atmospheric fog of the sky the camera's cluster sees (its indoor fog without one),
* moved towards over the camera's last 15 units like halo 1 does, and the planar fog of the camera cluster's fog plane
* (scenario.c scenario_get_atmospheric_fog, structures.c structure_get_planar_fog). The values are then adjusted the way
* rasterizer_xbox.c _rasterizer_window_begin does before turning them into shader constants.
*/

enum e_h1_fog_shader_mode
{
	_h1_fog_shader_mode_none,			// structure: fogged afterwards by the environment fog pass
	_h1_fog_shader_mode_model,			// opaque model: planar fog per pixel, atmospheric fog for the whole object
	_h1_fog_shader_mode_transparent,	// transparent: fog transmittance fades the shader by its framebuffer blend function
};

// recomputes the fog for the window being rendered
void h1_fog_update(void);

// forgets the per player fog state of the previous map
void h1_fog_reset(void);

// true when there is fog to draw
bool h1_fog_active(void);

// sets the shared fog pixel shader constants (c100 - c105) for a draw; centroid is the drawn object's center for models
void h1_fog_set_shader_constants(e_h1_fog_shader_mode mode, const real_point3d* centroid);

// sets the environment fog pass constants (c106 - c108) and returns the camera eye densities
void h1_fog_set_environment_fog_constants(void);

// the halo 1 fog density textures from the globals' rasterizer data (atmospheric, planar), NULL when missing
IDirect3DBaseTexture9* h1_fog_density_texture(bool planar);

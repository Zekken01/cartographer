#pragma once

/*
* Halo 1 shader emulation for the Halo 2 Direct3D 9 device.
*
* shader_environment: the Xbox diffuse texture pass combiners times the lightmap
* shader_model: base, detail and multipurpose maps with object lighting
* shader_transparent_generic: the Xbox register combiner stages, interpreted by a data driven pixel shader
* shader_transparent_chicago(_extended): the map chain with color and alpha functions
* shader_transparent_glass/water/plasma/meter: approximations
*/

struct s_h1_render_lighting
{
	real_rgb_color ambient;
	real_vector3d light0_direction;
	real_rgb_color light0_color;
	real_vector3d light1_direction;
	real_rgb_color light1_color;
};

enum e_h1_render_pass
{
	_h1_render_pass_opaque,
	_h1_render_pass_transparent,
};

// compiles the shared shaders, returns false on failure
bool h1_render_shaders_initialize(void);
void h1_render_shaders_dispose(void);

// world to clip and view depth constants for the current halo 2 camera
// if sky is set the geometry is moved with the camera and pushed to the far plane
void h1_render_set_camera_constants(const real_matrix4x3* object_to_world, bool sky);

// the pass a shader draws in
e_h1_render_pass h1_render_shader_pass(uint32 shader_group);

// how many times geometry with this shader is drawn (water draws the background and the reflection separately)
int32 h1_render_shader_subpass_count(uint32 shader_group);

// binds the pixel shader, textures, constants and blend state for a shader, returns false if it (or this subpass) isn't drawn
bool h1_render_shader_bind(uint32 shader_group, datum shader_index, const s_h1_render_lighting* lighting, IDirect3DBaseTexture9* lightmap, real32 game_time, int32 subpass = 0);

// the function values (a, b, c, d out) and change colors of the object being drawn, NULL outside objects (structure, sky);
// halo 1 objects without functions export 0 and white
void h1_render_shader_object_animation_set(const real32* function_values, const real_rgb_color* change_colors);

// restores the default opaque render state after a bound shader
void h1_render_shader_unbind(void);

IDirect3DTexture9* h1_render_default_texture(int32 index);	// 0 white, 1 gray, 2 black, 3 flat normal
IDirect3DVertexDeclaration9* h1_render_vertex_declaration(void);
IDirect3DVertexShader9* h1_render_vertex_shader(void);

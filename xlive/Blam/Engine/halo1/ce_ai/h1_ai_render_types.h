#pragma once

/*
* render.h and rasterizer_geometry.h: the render types halo 1's structure bsp definition holds (their layouts only).
*/

namespace h1_ai
{

enum
{
	MAXIMUM_RENDERED_DISTANT_LIGHTS = 2,
	MAXIMUM_RENDERED_POINT_LIGHTS = 2,
};

struct render_distant_light
{
	real_rgb_color color;
	real_vector3d direction;
};

struct render_lighting
{
	real_rgb_color ambient_color;
	short distant_light_count;
	word pad;
	struct render_distant_light distant_lights[MAXIMUM_RENDERED_DISTANT_LIGHTS];
	short point_light_count;
	word pad1;
	long point_light_indices[MAXIMUM_RENDERED_POINT_LIGHTS];
	real_argb_color reflection_tint_color;
	real_vector3d shadow_vector;
	real_rgb_color shadow_color;
};

struct vertex_buffer
{
	short type;
	word pad;
	long count;
	long offset;
	void* base_address;
	void* hardware_format;
};

} // namespace h1_ai

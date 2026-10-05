#include "stdafx.h"
#include "h1_fog.h"

#include "h1_bitmaps.h"
#include "h1_cache_file.h"

#include "rasterizer/dx9/rasterizer_dx9_main.h"
#include "render/render.h"

/* constants */

enum
{
	k_h1_fog_maximum_local_players = 4,
	k_h1_fog_constants_register = 100,
	k_h1_environment_fog_constants_register = 106,
};

enum e_h1_planar_fog_mode
{
	_h1_planar_fog_mode_off = 0,
	_h1_planar_fog_mode_normal,
	_h1_planar_fog_mode_fully_fogged,
};

enum
{
	_h1_fog_definition_is_water_bit = 0,
	_h1_fog_definition_atmosphere_dominant_bit,
	_h1_fog_definition_screen_effect_only_bit,
};

/* structures */

// render.h struct render_fog
struct s_h1_render_fog
{
	real_rgb_color atmospheric_color;
	real32 atmospheric_maximum_density;
	real32 atmospheric_minimum_distance;
	real32 atmospheric_maximum_distance;
	int16 planar_mode;
	real_plane3d plane;
	real_rgb_color planar_color;
	real32 planar_maximum_density;
	real32 planar_maximum_distance;
	real32 planar_maximum_depth;
	uint32 fog_definition_flags;
	bool active;
};

// scenario.c struct atmospheric_fog_state, one per local player
struct s_h1_atmospheric_fog_state
{
	bool valid;
	real_point3d camera_point;
	real32 start_distance;
	real32 opaque_distance;
	real32 maximum_density;
	real_rgb_color color;
};

/* globals */

static s_h1_render_fog g_h1_fog = {};
static s_h1_atmospheric_fog_state g_h1_fog_states[k_h1_fog_maximum_local_players] = {};
static real_point3d g_h1_fog_camera_point = {};
static real_vector3d g_h1_fog_camera_forward = { 1.f, 0.f, 0.f };

/* prototypes */

static const h1_sky* h1_fog_sky_get(int16 sky_index);
static void h1_fog_atmospheric_get(int32 local_player_index, int16 sky_index, const real_point3d* camera_point);
static void h1_fog_planar_get(int16 cluster_index);
static real32 h1_fog_pin(real32 value, real32 minimum, real32 maximum);
static void h1_fog_interpolate(real32* current, real32 desired, real32 maximum_speed);
static real32 h1_fog_plane_distance(const real_plane3d* plane, const real_point3d* point);
static void h1_fog_plane_from_point_and_normal(real_plane3d* plane, const real_point3d* point, const real_vector3d* normal);

/* public code */

void h1_fog_reset(void)
{
	csmemset(g_h1_fog_states, 0, sizeof(g_h1_fog_states));
	csmemset(&g_h1_fog, 0, sizeof(g_h1_fog));
	return;
}

void h1_fog_update(void)
{
	const s_frame* frame = global_window_parameters_get();
	const s_render* render = render_get();
	const render_camera* camera = &frame->camera;
	g_h1_fog_camera_point = camera->point;
	g_h1_fog_camera_forward = camera->forward;

	// the camera's cluster and the sky it sees (structure_visibility.c structure_visibility_find_camera)
	const h1_sbsp* bsp = (const h1_sbsp*)g_h1_cache_file->tag_get('sbsp', g_h1_cache_file->structure_bsp_tag_get(0));
	const int16 cluster_index = bsp && VALID_INDEX(render->cluster_index, bsp->clusters.count) ? (int16)render->cluster_index : NONE;
	const h1_sbsp_clusters* cluster = cluster_index != NONE ? g_h1_cache_file->block_get(bsp->clusters, cluster_index) : NULL;
	const int16 visible_sky_index = cluster ? cluster->sky : NONE;

	// render.c render_window
	csmemset(&g_h1_fog, 0, sizeof(g_h1_fog));
	const int32 local_player_index = !frame->is_texture_camera && VALID_INDEX(frame->window_bound_index, k_h1_fog_maximum_local_players) ? frame->window_bound_index : NONE;
	h1_fog_atmospheric_get(local_player_index, visible_sky_index, &camera->point);
	h1_fog_planar_get(cluster_index);
	if (g_h1_fog.atmospheric_maximum_distance != 0.f && visible_sky_index == NONE &&
		g_h1_fog.planar_maximum_distance > g_h1_fog.atmospheric_maximum_distance)
	{
		g_h1_fog.planar_maximum_distance = g_h1_fog.atmospheric_maximum_distance;
	}

	// rasterizer_xbox.c _rasterizer_window_begin
	g_h1_fog.active = g_h1_fog.atmospheric_maximum_distance != 0.f;
	if (g_h1_fog.atmospheric_maximum_density <= 0.f)
	{
		g_h1_fog.atmospheric_maximum_density = 1.f;
	}
	if (g_h1_fog.atmospheric_maximum_distance == 0.f)
	{
		g_h1_fog.atmospheric_maximum_density = 0.f;
		g_h1_fog.atmospheric_minimum_distance = camera->z_far;
		g_h1_fog.atmospheric_maximum_distance = camera->z_far * 2.f;
	}
	if (g_h1_fog.planar_maximum_density <= 0.f)
	{
		g_h1_fog.planar_maximum_density = 1.f;
	}
	if (g_h1_fog.planar_mode != _h1_planar_fog_mode_off && !TEST_BIT(g_h1_fog.fog_definition_flags, _h1_fog_definition_screen_effect_only_bit))
	{
		g_h1_fog.active = true;
		if (g_h1_fog.planar_mode == _h1_planar_fog_mode_fully_fogged)
		{
			g_h1_fog.planar_maximum_depth = 1.f;
			h1_fog_plane_from_point_and_normal(&g_h1_fog.plane, &camera->point, &camera->forward);
			g_h1_fog.plane.d += camera->z_far;
		}
	}
	else
	{
		g_h1_fog.planar_mode = _h1_planar_fog_mode_off;
		g_h1_fog.planar_maximum_density = 0.f;
		g_h1_fog.planar_maximum_distance = 1.f;
		g_h1_fog.planar_maximum_depth = 1.f;
		g_h1_fog.planar_color = { 1.f, 1.f, 1.f };
		h1_fog_plane_from_point_and_normal(&g_h1_fog.plane, &camera->point, &camera->forward);
	}
	if (g_h1_fog.atmospheric_maximum_distance <= g_h1_fog.atmospheric_minimum_distance)
	{
		g_h1_fog.atmospheric_maximum_distance = g_h1_fog.atmospheric_minimum_distance + 0.0001f;
	}
	if (g_h1_fog.planar_maximum_distance <= 0.f)
	{
		g_h1_fog.planar_maximum_distance = 1.f;
	}
	if (g_h1_fog.planar_maximum_depth <= 0.f)
	{
		g_h1_fog.planar_maximum_depth = 1.f;
	}
	return;
}

bool h1_fog_active(void)
{
	return g_h1_fog.active;
}

// the vertex shader fog constants c[-88] to c[-85] of rasterizer_xbox.c, then what the pixel shaders need besides them
void h1_fog_set_shader_constants(e_h1_fog_shader_mode mode, const real_point3d* centroid)
{
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	const real_vector3d* forward = &g_h1_fog_camera_forward;
	const real_point3d* camera = &g_h1_fog_camera_point;

	const real32 inverse_atmospheric_range = 1.f / (g_h1_fog.atmospheric_maximum_distance - g_h1_fog.atmospheric_minimum_distance);
	const real32 plane_distance = h1_fog_plane_distance(&g_h1_fog.plane, camera);
	const real32 view_distance = camera->x * forward->i + camera->y * forward->j + camera->z * forward->k;
	const real32 inverse_planar_distance = 1.f / g_h1_fog.planar_maximum_distance;
	const real32 inverse_planar_depth = 1.f / g_h1_fog.planar_maximum_depth;

	real32 constants[6][4] = {};
	constants[0][0] = forward->i * inverse_atmospheric_range;
	constants[0][1] = forward->j * inverse_atmospheric_range;
	constants[0][2] = forward->k * inverse_atmospheric_range;
	constants[0][3] = -((g_h1_fog.atmospheric_minimum_distance + view_distance) * inverse_atmospheric_range);
	constants[1][0] = -(g_h1_fog.plane.n.i * inverse_planar_depth);
	constants[1][1] = -(g_h1_fog.plane.n.j * inverse_planar_depth);
	constants[1][2] = -(g_h1_fog.plane.n.k * inverse_planar_depth);
	constants[1][3] = g_h1_fog.plane.d * inverse_planar_depth;
	constants[2][0] = forward->i * inverse_planar_distance;
	constants[2][1] = forward->j * inverse_planar_distance;
	constants[2][2] = forward->k * inverse_planar_distance;
	constants[2][3] = -(inverse_planar_distance * view_distance);
	constants[3][0] = h1_fog_pin(g_h1_fog.atmospheric_maximum_density, 0.f, 1.f);
	constants[3][1] = h1_fog_pin(-(inverse_planar_depth * plane_distance), 0.f, 1.f);
	constants[3][2] = h1_fog_pin(g_h1_fog.planar_maximum_density, 0.f, 1.f);
	constants[3][3] = (real32)mode;

	// models: the atmospheric fog of the object's center and the color the planar fog blends to (rasterizer_xbox_models.c)
	if (mode == _h1_fog_shader_mode_model && centroid)
	{
		const real32 camera_distance =
			(centroid->x - camera->x) * forward->i +
			(centroid->y - camera->y) * forward->j +
			(centroid->z - camera->z) * forward->k;
		real32 planar_fog_fraction = h1_fog_pin(plane_distance / g_h1_fog.atmospheric_maximum_distance, 0.f, 1.f);
		const real32 fog_density = h1_fog_pin((camera_distance - g_h1_fog.atmospheric_minimum_distance) * inverse_atmospheric_range, 0.f, 1.f) *
			g_h1_fog.atmospheric_maximum_density;
		if (TEST_BIT(g_h1_fog.fog_definition_flags, _h1_fog_definition_atmosphere_dominant_bit))
		{
			planar_fog_fraction = 1.f;
		}
		const real_rgb_color* planar = &g_h1_fog.planar_color;
		const real_rgb_color* atmospheric = &g_h1_fog.atmospheric_color;
		constants[4][0] = atmospheric->red * fog_density;
		constants[4][1] = atmospheric->green * fog_density;
		constants[4][2] = atmospheric->blue * fog_density;
		constants[4][3] = fog_density;
		// cc0 and its error color together carry a signed color
		constants[5][0] = h1_fog_pin(planar->red - fog_density * (atmospheric->red * (1.f - planar_fog_fraction) + planar->red * planar_fog_fraction), -1.f, 1.f);
		constants[5][1] = h1_fog_pin(planar->green - fog_density * (atmospheric->green * (1.f - planar_fog_fraction) + planar->green * planar_fog_fraction), -1.f, 1.f);
		constants[5][2] = h1_fog_pin(planar->blue - fog_density * (atmospheric->blue * (1.f - planar_fog_fraction) + planar->blue * planar_fog_fraction), -1.f, 1.f);
	}
	device->SetPixelShaderConstantF(k_h1_fog_constants_register, &constants[0][0], 6);
	return;
}

// rasterizer_xbox_environment_fog.c _rasterizer_environment_fog_begin
void h1_fog_set_environment_fog_constants(void)
{
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	const real32 distance = h1_fog_plane_distance(&g_h1_fog.plane, &g_h1_fog_camera_point);
	real32 atmospheric_eye_density = h1_fog_pin(distance / g_h1_fog.atmospheric_maximum_distance, 0.f, 1.f);
	const real32 planar_eye_density = h1_fog_pin(-(distance / g_h1_fog.planar_maximum_depth), 0.f, 1.f);
	if (TEST_BIT(g_h1_fog.fog_definition_flags, _h1_fog_definition_atmosphere_dominant_bit))
	{
		atmospheric_eye_density = 1.f;
	}

	const real32 constants[3][4] =
	{
		{ atmospheric_eye_density, planar_eye_density, h1_fog_pin(g_h1_fog.atmospheric_maximum_density, 0.f, 1.f), h1_fog_pin(g_h1_fog.planar_maximum_density, 0.f, 1.f) },
		{ g_h1_fog.atmospheric_color.red, g_h1_fog.atmospheric_color.green, g_h1_fog.atmospheric_color.blue, 1.f },
		{ g_h1_fog.planar_color.red, g_h1_fog.planar_color.green, g_h1_fog.planar_color.blue, 1.f },
	};
	device->SetPixelShaderConstantF(k_h1_environment_fog_constants_register, &constants[0][0], 3);
	return;
}

IDirect3DBaseTexture9* h1_fog_density_texture(bool planar)
{
	const datum bitmap_index = g_h1_cache_file->tag_find('bitm', planar ? "rasterizer\\planar fog density" : "rasterizer\\atmospheric fog density");
	return bitmap_index != NONE ? h1_bitmap_texture_get(bitmap_index, 0) : NULL;
}

/* private code */

static const h1_sky* h1_fog_sky_get(int16 sky_index)
{
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();
	const h1_scnr_skies* reference = scenario ? g_h1_cache_file->block_get(scenario->skies, sky_index) : NULL;
	return reference ? (const h1_sky*)g_h1_cache_file->tag_get(reference->sky) : NULL;
}

// scenario.c scenario_get_atmospheric_fog
static void h1_fog_atmospheric_get(int32 local_player_index, int16 sky_index, const real_point3d* camera_point)
{
	s_h1_atmospheric_fog_state local_state = {};
	s_h1_atmospheric_fog_state* state = local_player_index != NONE ? &g_h1_fog_states[local_player_index] : &local_state;

	// without a sky the camera is indoors and gets the indoor fog of the first sky
	const h1_sky* sky = h1_fog_sky_get(sky_index == NONE ? 0 : sky_index);
	if (sky)
	{
		const real_rgb_color* color = sky_index == NONE ? &sky->indoor_fog_color : &sky->outdoor_fog_color;
		const real32 maximum_density = sky_index == NONE ? sky->indoor_fog_maximum_density : sky->outdoor_fog_maximum_density;
		const real32 start_distance = sky_index == NONE ? sky->indoor_fog_start_distance : sky->outdoor_fog_start_distance;
		const real32 opaque_distance = sky_index == NONE ? sky->indoor_fog_opaque_distance : sky->outdoor_fog_opaque_distance;

		const real32 dx = camera_point->x - state->camera_point.x;
		const real32 dy = camera_point->y - state->camera_point.y;
		const real32 dz = camera_point->z - state->camera_point.z;
		real32 distance = sqrtf(dx * dx + dy * dy + dz * dz);
		if (local_player_index != NONE && distance < 15.f && state->valid && opaque_distance != 0.f && state->opaque_distance != 0.f)
		{
			h1_fog_interpolate(&state->start_distance, start_distance, distance);
			h1_fog_interpolate(&state->opaque_distance, opaque_distance, distance);
			distance *= 0.05f;
			h1_fog_interpolate(&state->maximum_density, maximum_density, distance);
			h1_fog_interpolate(&state->color.red, color->red, distance);
			h1_fog_interpolate(&state->color.green, color->green, distance);
			h1_fog_interpolate(&state->color.blue, color->blue, distance);
		}
		else
		{
			state->start_distance = start_distance;
			state->opaque_distance = opaque_distance;
			state->maximum_density = maximum_density;
			state->color = *color;
			state->valid = true;
		}
		state->camera_point = *camera_point;
	}

	g_h1_fog.atmospheric_color = state->color;
	g_h1_fog.atmospheric_maximum_density = state->maximum_density;
	g_h1_fog.atmospheric_minimum_distance = state->start_distance;
	if (state->opaque_distance != 0.f)
	{
		g_h1_fog.atmospheric_maximum_distance = MAX(state->start_distance + 0.0001f, state->opaque_distance);
	}
	else
	{
		g_h1_fog.atmospheric_maximum_distance = 0.f;
	}
	return;
}

// structures.c structure_get_planar_fog (the fog screen of the sky's indoor fog is not drawn)
static void h1_fog_planar_get(int16 cluster_index)
{
	g_h1_fog.planar_mode = _h1_planar_fog_mode_off;
	g_h1_fog.fog_definition_flags = 0;

	const h1_sbsp* bsp = (const h1_sbsp*)g_h1_cache_file->tag_get('sbsp', g_h1_cache_file->structure_bsp_tag_get(0));
	const h1_sbsp_clusters* cluster = bsp && cluster_index != NONE ? g_h1_cache_file->block_get(bsp->clusters, cluster_index) : NULL;
	if (!cluster || cluster->fog == NONE)
	{
		return;
	}

	const bool has_plane = TEST_BIT((uint16)cluster->fog, 15);
	const h1_sbsp_fog_planes* fog_plane = has_plane ? g_h1_cache_file->block_get(bsp->fog_planes, cluster->fog & SHORT_MAX) : NULL;
	const int16 fog_region_index = has_plane ? (fog_plane ? fog_plane->front_region_index : NONE) : (int16)(cluster->fog & SHORT_MAX);
	const h1_sbsp_fog_regions* fog_region = fog_region_index != NONE ? g_h1_cache_file->block_get(bsp->fog_regions, fog_region_index) : NULL;
	const h1_sbsp_fog_palette* palette = fog_region && fog_region->fog_palette_index != NONE ? g_h1_cache_file->block_get(bsp->fog_palette, fog_region->fog_palette_index) : NULL;
	const h1_fog* definition = palette ? (const h1_fog*)g_h1_cache_file->tag_get('fog ', palette->fog.index) : NULL;
	if (!definition)
	{
		return;
	}

	if (has_plane)
	{
		g_h1_fog.planar_mode = _h1_planar_fog_mode_normal;
		g_h1_fog.plane = fog_plane->plane;
	}
	else
	{
		g_h1_fog.planar_mode = _h1_planar_fog_mode_fully_fogged;
	}
	g_h1_fog.planar_color = definition->color;
	g_h1_fog.planar_maximum_density = definition->maximum_density;
	g_h1_fog.planar_maximum_distance = definition->opaque_distance;
	g_h1_fog.planar_maximum_depth = definition->opaque_depth;
	g_h1_fog.fog_definition_flags = definition->flags;
	return;
}

static real32 h1_fog_pin(real32 value, real32 minimum, real32 maximum)
{
	return value < minimum ? minimum : (value > maximum ? maximum : value);
}

// real_math.h interpolate_scalar
static void h1_fog_interpolate(real32* current, real32 desired, real32 maximum_speed)
{
	*current += h1_fog_pin(desired - *current, -maximum_speed, maximum_speed);
	return;
}

static real32 h1_fog_plane_distance(const real_plane3d* plane, const real_point3d* point)
{
	return plane->n.i * point->x + plane->n.j * point->y + plane->n.k * point->z - plane->d;
}

static void h1_fog_plane_from_point_and_normal(real_plane3d* plane, const real_point3d* point, const real_vector3d* normal)
{
	plane->n = *normal;
	plane->d = normal->i * point->x + normal->j * point->y + normal->k * point->z;
	return;
}

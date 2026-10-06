#include "stdafx.h"
#include "h1_render.h"
#include "h1_scenario_objects.h"

#include "h1_bitmaps.h"
#include "h1_cache_file.h"
#include "h1_effects.h"
#include "h1_fog.h"
#include "h1_log.h"
#include "h1_map_loader.h"
#include "h1_render_models.h"
#include "h1_render_shaders.h"
#include "h1_objects.h"
#include "h1_scenery.h"
#include "h1_first_person.h"
#include "h1_hud.h"
#include "h1_weapons.h"
#include "h1_runtime.h"
#include "h1_sound.h"

#include "game/game_time.h"
#include "rasterizer/rasterizer_globals.h"
#include "rasterizer/dx9/rasterizer_dx9.h"
#include "rasterizer/dx9/rasterizer_dx9_main.h"
#include "render/render.h"

/* structures */

struct s_h1_structure_vertex
{
	real32 position[3];
	real32 normal[3];
	real32 texcoord[2];
	real32 lightmap_texcoord[2];
};
static_assert(sizeof(s_h1_structure_vertex) == 40);

struct s_h1_structure_draw
{
	datum shader_index;
	uint32 shader_group;
	int32 lightmap_bitmap_index;	// NONE when the material has no lightmap
	int32 base_vertex;
	int32 vertex_count;
	int32 first_index;
	int32 triangle_count;
	int32 lighting_material_index;
	int16 breakable_surface_index;	// NONE when the material can't break
};

struct s_h1_scenery_instance
{
	datum model_index;
	int16 permutation;
	real_matrix4x3 matrix;
	s_h1_render_lighting lighting;
	real_rgb_color change_colors[4];
	int32 placement_index;
};

// cpu copy of the lightmapped structure, used to light objects from the surface below them
struct s_h1_lighting_triangle
{
	real_point3d points[3];
	real_point2d lightmap_texcoords[3];
	real_point2d texcoords[3];
	real_vector3d normals[3];
	real_vector3d incident_radiosity[3];
	int32 lightmap_bitmap_index;
	int32 material_index;
	datum diffuse_bitmap_tag_index;	// the environment shader's base map (object_lights.c sample_diffuse_texture)
	int16 diffuse_bitmap_index;
};

struct s_h1_lighting_material
{
	real_rgb_color ambient;
	real_rgb_color light0_color;
	real_vector3d light0_direction;
	real_rgb_color light1_color;
	real_vector3d light1_direction;
};

struct s_h1_render_globals
{
	bool initialized;
	bool failed;
	int16 structure_bsp_index;		// the halo 1 structure bsp the structure draws were built for
	datum lightmap_bitmap_tag;
	datum sky_model_index;
	real_rgb_color sky_fog_color;
	s_h1_render_lighting lighting;
	IDirect3DVertexBuffer9* vertex_buffer;
	IDirect3DIndexBuffer9* index_buffer;
	IDirect3DStateBlock9* state_block;
	std::vector<s_h1_structure_draw> draws;
	std::vector<s_h1_scenery_instance> scenery;
	std::vector<s_h1_lighting_triangle> lighting_triangles;
	std::vector<s_h1_lighting_material> lighting_materials;
};

/* globals */

extern int32 g_h1_render_debug_mode;
static s_h1_render_globals g_h1_render{};

/* prototypes */

static bool h1_render_initialize(void);
static bool h1_render_structure_initialize(void);
static void h1_render_scenery_initialize(void);
static bool h1_render_begin(void);
static void h1_render_end(void);
static void h1_render_structure_pass(e_h1_render_pass pass);
static void h1_render_structure_fog_pass(void);
static void h1_unpack_normal(uint32 packed, real32* out);
static real32 h1_render_game_time(void);
static bool h1_render_breakable_surface_extant(int16 breakable_surface_index);

/* public code */

c_h1_render_state_guard::c_h1_render_state_guard(void) :
	m_state_block(NULL)
{
	IDirect3DDevice9Ex* device = h1_maps_active() ? rasterizer_dx9_device_get_interface() : NULL;
	if (device && FAILED(device->CreateStateBlock(D3DSBT_ALL, &m_state_block)))
	{
		m_state_block = NULL;
	}
	return;
}

c_h1_render_state_guard::~c_h1_render_state_guard(void)
{
	if (m_state_block)
	{
		m_state_block->Apply();
		m_state_block->Release();

		// halo 2's own state caches (its render states, vertex and pixel shader constants) skip setting what they hold: halo 2
		// functions the halo 1 renderer calls set them with the device, and the device went back, so they take the device's again
		IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
		DWORD* render_state_cache = Memory::GetAddress<DWORD*>(0xA4B1A0);
		for (int32 state = D3DRS_ZENABLE; state <= D3DRS_BLENDOPALPHA; state++)
		{
			DWORD value;
			if (SUCCEEDED(device->GetRenderState((D3DRENDERSTATETYPE)state, &value)))
			{
				render_state_cache[state] = value;
			}
		}
		device->GetVertexShaderConstantF(0, (real32*)rasterizer_get_main_vertex_shader_cache(), 256);
		device->GetPixelShaderConstantF(0, (real32*)rasterizer_get_main_pixel_shader_cache(), 32);
	}
	return;
}


void h1_render_structure_opaque(void)
{
	if (!h1_render_begin())
	{
		return;
	}

	const s_frame* frame = global_window_parameters_get();
	if (frame->window_bound_index == 0)
	{
		h1_sound_listener_set(&frame->camera);
		h1_effects_update();
		h1_scenery_update();
		h1_weapons_update();
	}

	h1_fog_update();
	if (g_h1_render_debug_mode != 9)
	{
		h1_render_structure_pass(_h1_render_pass_opaque);
		h1_effects_render_decals();
		h1_render_structure_fog_pass();
	}


	const real32 game_time = h1_render_game_time();
	h1_objects_render_frame_begin();
	for (const s_h1_scenery_instance& instance : g_h1_render.scenery)
	{
		const real32* function_values;
		const real_rgb_color* change_colors;
		const int16* region_permutations;
		h1_scenery_render_state(instance.placement_index, &function_values, &change_colors, &region_permutations);
		h1_render_model_draw(instance.model_index, instance.permutation, &instance.matrix, &instance.lighting, _h1_render_pass_opaque, false, game_time,
			change_colors ? change_colors : instance.change_colors, function_values, region_permutations);
	}
	h1_objects_render(_h1_render_pass_opaque, game_time);

	h1_render_end();
	return;
}

void h1_render_structure_transparent(void)
{
	if (!h1_render_begin())
	{
		return;
	}

	h1_render_structure_pass(_h1_render_pass_transparent);

	const real32 game_time = h1_render_game_time();
	for (const s_h1_scenery_instance& instance : g_h1_render.scenery)
	{
		const real32* function_values;
		const real_rgb_color* change_colors;
		const int16* region_permutations;
		h1_scenery_render_state(instance.placement_index, &function_values, &change_colors, &region_permutations);
		h1_render_model_draw(instance.model_index, instance.permutation, &instance.matrix, &instance.lighting, _h1_render_pass_transparent, false, game_time,
			change_colors ? change_colors : instance.change_colors, function_values, region_permutations);
	}
	h1_objects_render(_h1_render_pass_transparent, game_time);
	h1_effects_render();
	h1_first_person_render(game_time);
	h1_effects_render_lens_flares();

	h1_render_end();
	return;
}

bool h1_render_sky(void)
{
	if (!h1_render_begin())
	{
		return false;
	}

	bool drawn = false;
	if (g_h1_render.sky_model_index != NONE)
	{
		IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
		// the sky sits on the far plane behind everything already drawn
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		device->SetRenderState(D3DRS_COLORWRITEENABLE1, 0);

		// halo 1 clears to the sky's fog color before drawing the sky, the sky layers blend over it
		{
			D3DVIEWPORT9 viewport;
			device->GetViewport(&viewport);
			const real_rgb_color* color = &g_h1_render.sky_fog_color;
			const D3DCOLOR diffuse = D3DCOLOR_COLORVALUE(color->red, color->green, color->blue, 1.f);
			struct { real32 x, y, z, rhw; D3DCOLOR color; } quad[4] =
			{
				{ (real32)viewport.X - 0.5f, (real32)viewport.Y - 0.5f, 0.99999f, 1.f, diffuse },
				{ (real32)(viewport.X + viewport.Width) - 0.5f, (real32)viewport.Y - 0.5f, 0.99999f, 1.f, diffuse },
				{ (real32)viewport.X - 0.5f, (real32)(viewport.Y + viewport.Height) - 0.5f, 0.99999f, 1.f, diffuse },
				{ (real32)(viewport.X + viewport.Width) - 0.5f, (real32)(viewport.Y + viewport.Height) - 0.5f, 0.99999f, 1.f, diffuse },
			};
			device->SetVertexShader(NULL);
			device->SetPixelShader(NULL);
			device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
			device->SetTexture(0, NULL);
			device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
			device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
			device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
			device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
			device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
			device->SetRenderState(D3DRS_LIGHTING, FALSE);
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
			device->SetVertexDeclaration(h1_render_vertex_declaration());
			device->SetVertexShader(h1_render_vertex_shader());
		}

		const real32 game_time = h1_render_game_time();
		s_h1_render_lighting sky_lighting = {};
		sky_lighting.ambient = { 1.f, 1.f, 1.f };
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		h1_render_model_draw(g_h1_render.sky_model_index, 0, NULL, &sky_lighting, _h1_render_pass_opaque, true, game_time);
		h1_render_model_draw(g_h1_render.sky_model_index, 0, NULL, &sky_lighting, _h1_render_pass_transparent, true, game_time);
		drawn = true;
	}

	h1_render_end();
	return drawn;
}

void h1_render_dispose(void)
{
	h1_hud_dispose();
	if (g_h1_render.vertex_buffer) g_h1_render.vertex_buffer->Release();
	if (g_h1_render.index_buffer) g_h1_render.index_buffer->Release();
	if (g_h1_render.state_block) g_h1_render.state_block->Release();
	g_h1_render.vertex_buffer = NULL;
	g_h1_render.index_buffer = NULL;
	g_h1_render.state_block = NULL;
	std::vector<s_h1_structure_draw>().swap(g_h1_render.draws);
	h1_fog_reset();
	h1_effects_reset();
	std::vector<s_h1_scenery_instance>().swap(g_h1_render.scenery);
	std::vector<s_h1_lighting_triangle>().swap(g_h1_render.lighting_triangles);
	std::vector<s_h1_lighting_material>().swap(g_h1_render.lighting_materials);
	g_h1_render.initialized = false;
	g_h1_render.failed = false;

	h1_render_models_dispose();
	h1_render_shaders_dispose();
	h1_bitmaps_dispose();
	return;
}

/* private code */

static real32 h1_render_game_time(void)
{
	return (real32)game_time_get() / 60.f;
}

static bool h1_render_begin(void)
{
	if (!h1_maps_active() || !g_h1_cache_file)
	{
		return false;
	}

	if (!g_h1_render.initialized && !g_h1_render.failed)
	{
		g_h1_render.failed = !h1_render_initialize();
		g_h1_render.initialized = !g_h1_render.failed;
	}

	if (!g_h1_render.initialized)
	{
		return false;
	}

	// a switched structure bsp: its draws and lighting
	if (g_h1_render.structure_bsp_index != h1_maps_structure_bsp_index())
	{
		if (g_h1_render.vertex_buffer) g_h1_render.vertex_buffer->Release();
		if (g_h1_render.index_buffer) g_h1_render.index_buffer->Release();
		g_h1_render.vertex_buffer = NULL;
		g_h1_render.index_buffer = NULL;
		g_h1_render.draws.clear();
		g_h1_render.lighting_triangles.clear();
		g_h1_render.lighting_materials.clear();
		h1_fog_reset();
		if (!h1_render_structure_initialize())
		{
			g_h1_render.failed = true;
			g_h1_render.initialized = false;
			return false;
		}
	}

	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	g_h1_render.state_block->Capture();

	device->SetVertexDeclaration(h1_render_vertex_declaration());
	device->SetVertexShader(h1_render_vertex_shader());
	device->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	device->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
	device->SetRenderState(D3DRS_COLORWRITEENABLE1, 0xF);
	device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
	device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	device->SetRenderState(D3DRS_FOGENABLE, FALSE);
	device->SetRenderState(D3DRS_DEPTHBIAS, 0);
	device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, 0);

	for (DWORD stage = 0; stage < 8; stage++)
	{
		device->SetSamplerState(stage, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		device->SetSamplerState(stage, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
		device->SetSamplerState(stage, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
		device->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
		device->SetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(stage, D3DSAMP_MAXANISOTROPY, 8);
		device->SetSamplerState(stage, D3DSAMP_SRGBTEXTURE, FALSE);
		device->SetSamplerState(stage, D3DSAMP_MIPMAPLODBIAS, 0);
	}
	return true;
}

static void h1_render_end(void)
{
	g_h1_render.state_block->Apply();
	return;
}

extern bool g_h1_render_debug_camera;
extern real_point3d g_h1_render_debug_camera_position;
extern real32 g_h1_render_debug_camera_yaw;
extern real32 g_h1_render_debug_camera_pitch;

static bool h1_render_initialize(void)
{
	// development: h1_debug.txt lines "mode <1 flat, 2 base, 3 lightmap, 4 detail>" and "camera x y z yaw pitch" (degrees)
	{
		g_h1_render_debug_mode = 0;
		g_h1_render_debug_camera = false;
		FILE* file = _wfsopen(L"h1_debug.txt", L"r", _SH_DENYNO);
		if (file)
		{
			char line[128];
			while (fgets(line, sizeof(line), file))
			{
				real32 x, y, z, yaw, pitch;
				int32 mode;
				if (sscanf_s(line, "mode %d", &mode) == 1)
				{
					g_h1_render_debug_mode = mode;
				}
				else if (sscanf_s(line, "camera %f %f %f %f %f", &x, &y, &z, &yaw, &pitch) == 5)
				{
					g_h1_render_debug_camera = true;
					g_h1_render_debug_camera_position = { x, y, z };
					g_h1_render_debug_camera_yaw = DEGREES_TO_RADIANS(yaw);
					g_h1_render_debug_camera_pitch = DEGREES_TO_RADIANS(pitch);
				}
			}
			fclose(file);
		}
	}

	if (!h1_render_shaders_initialize())
	{
		h1_log("render: failed to create shaders");
		return false;
	}

	if (FAILED(rasterizer_dx9_device_get_interface()->CreateStateBlock(D3DSBT_ALL, &g_h1_render.state_block)))
	{
		h1_log("render: failed to create a state block");
		return false;
	}

	if (!h1_render_structure_initialize())
	{
		return false;
	}

	if (!h1_render_models_initialize())
	{
		return false;
	}

	h1_render_scenery_initialize();
	return true;
}

static bool h1_render_structure_initialize(void)
{
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	const D3DPOOL pool = rasterizer_globals_get()->use_d3d9_ex ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;

	g_h1_render.structure_bsp_index = h1_maps_structure_bsp_index();
	const h1_sbsp* bsp = (const h1_sbsp*)g_h1_cache_file->tag_get('sbsp', g_h1_cache_file->structure_bsp_tag_get(g_h1_render.structure_bsp_index));
	if (!bsp)
	{
		h1_log("render: no structure bsp");
		return false;
	}
	g_h1_render.lightmap_bitmap_tag = bsp->lightmap_bitmaps.index;

	// lighting used for objects and unlightmapped surfaces
	g_h1_render.lighting.ambient = bsp->default_ambient_color;
	g_h1_render.lighting.light0_color = bsp->default_distant_light_0_color;
	g_h1_render.lighting.light0_direction = bsp->default_distant_light_0_direction;
	g_h1_render.lighting.light1_color = bsp->default_distant_light_1_color;
	g_h1_render.lighting.light1_direction = bsp->default_distant_light_1_direction;
	g_h1_render.lighting.reflection_tint = bsp->default_reflection_tint;

	int32 vertex_count = 0;
	int32 index_count = 0;
	for (int32 i = 0; i < bsp->lightmaps.count; i++)
	{
		const h1_sbsp_lightmaps* lightmap = g_h1_cache_file->block_get(bsp->lightmaps, i);
		for (int32 j = 0; j < lightmap->materials.count; j++)
		{
			const h1_sbsp_lightmaps_materials* material = g_h1_cache_file->block_get(lightmap->materials, j);
			vertex_count += material->count;
			index_count += material->surface_count * 3;
		}
	}

	if (vertex_count == 0 || index_count == 0)
	{
		h1_log("render: structure has no geometry");
		return false;
	}

	if (FAILED(device->CreateVertexBuffer(vertex_count * sizeof(s_h1_structure_vertex), D3DUSAGE_WRITEONLY, 0, pool, &g_h1_render.vertex_buffer, NULL)) ||
		FAILED(device->CreateIndexBuffer(index_count * sizeof(uint32), D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, pool, &g_h1_render.index_buffer, NULL)))
	{
		h1_log("render: failed to create structure buffers (%d vertices, %d indices)", vertex_count, index_count);
		return false;
	}

	s_h1_structure_vertex* vertices = NULL;
	uint32* indices = NULL;
	if (FAILED(g_h1_render.vertex_buffer->Lock(0, 0, (void**)&vertices, 0)) ||
		FAILED(g_h1_render.index_buffer->Lock(0, 0, (void**)&indices, 0)))
	{
		h1_log("render: failed to lock structure buffers");
		return false;
	}

	int32 vertex_cursor = 0;
	int32 index_cursor = 0;
	for (int32 i = 0; i < bsp->lightmaps.count; i++)
	{
		const h1_sbsp_lightmaps* lightmap = g_h1_cache_file->block_get(bsp->lightmaps, i);
		for (int32 j = 0; j < lightmap->materials.count; j++)
		{
			const h1_sbsp_lightmaps_materials* material = g_h1_cache_file->block_get(lightmap->materials, j);
			const int32 material_vertex_count = material->count;

			const int32 lighting_material_index = (int32)g_h1_render.lighting_materials.size();
			{
				s_h1_lighting_material lighting_material;
				lighting_material.ambient = material->ambient_color;
				lighting_material.light0_color = material->distant_light_count > 0 ? material->distant_light_0_color : real_rgb_color{ 0.f, 0.f, 0.f };
				lighting_material.light0_direction = material->distant_light_0_direction;
				lighting_material.light1_color = material->distant_light_count > 1 ? material->distant_light_1_color : real_rgb_color{ 0.f, 0.f, 0.f };
				lighting_material.light1_direction = material->distant_light_1_direction;
				g_h1_render.lighting_materials.push_back(lighting_material);
			}

			// object_lights.c sample_diffuse_texture: the environment shader's base map, the material's permutation of it
			datum diffuse_bitmap_tag_index = NONE;
			int16 diffuse_bitmap_index = NONE;
			if (material->shader.group_tag == 'senv' && material->shader.index != NONE)
			{
				const h1_senv* environment = (const h1_senv*)g_h1_cache_file->tag_get('senv', material->shader.index);
				const h1_bitm* base_map = environment && environment->base_map.index != NONE ? (const h1_bitm*)g_h1_cache_file->tag_get('bitm', environment->base_map.index) : NULL;
				if (base_map && base_map->bitmaps.count > 0)
				{
					diffuse_bitmap_tag_index = environment->base_map.index;
					diffuse_bitmap_index = (int16)(material->shader_permutation % base_map->bitmaps.count);
				}
			}

			// xbox: 32 byte compressed rendered vertices followed by 8 byte compressed lightmap vertices
			const uint8* data = (const uint8*)g_h1_cache_file->data_get(material->compressed_vertices);
			const bool has_lightmap_vertices = lightmap->bitmap != NONE &&
				material->compressed_vertices.size >= material_vertex_count * 40;
			if (!data || material->compressed_vertices.size < material_vertex_count * 32)
			{
				continue;
			}

			for (int32 v = 0; v < material_vertex_count; v++)
			{
				const uint8* source = data + v * 32;
				s_h1_structure_vertex* vertex = &vertices[vertex_cursor + v];
				csmemcpy(vertex->position, source, sizeof(vertex->position));
				h1_unpack_normal(*(const uint32*)(source + 12), vertex->normal);
				csmemcpy(vertex->texcoord, source + 24, sizeof(vertex->texcoord));
				if (has_lightmap_vertices)
				{
					const int16* lightmap_texcoord = (const int16*)(data + material_vertex_count * 32 + v * 8 + 4);
					vertex->lightmap_texcoord[0] = (real32)lightmap_texcoord[0] / 32767.f;
					vertex->lightmap_texcoord[1] = (real32)lightmap_texcoord[1] / 32767.f;
				}
				else
				{
					vertex->lightmap_texcoord[0] = 0.f;
					vertex->lightmap_texcoord[1] = 0.f;
				}
			}

			s_h1_structure_draw draw;
			draw.shader_index = material->shader.index;
			draw.shader_group = material->shader.group_tag;
			draw.lightmap_bitmap_index = has_lightmap_vertices ? lightmap->bitmap : NONE;
			draw.base_vertex = vertex_cursor;
			draw.vertex_count = material_vertex_count;
			draw.first_index = index_cursor;
			draw.triangle_count = 0;
			draw.lighting_material_index = lighting_material_index;
			draw.breakable_surface_index = material->breakable_surface;

			for (int32 s = 0; s < material->surface_count; s++)
			{
				const h1_sbsp_surfaces* surface = g_h1_cache_file->block_get(bsp->surfaces, material->surfaces_index + s);
				if (!surface)
				{
					break;
				}
				indices[index_cursor++] = (uint32)(uint16)surface->vertex_a;
				indices[index_cursor++] = (uint32)(uint16)surface->vertex_b;
				indices[index_cursor++] = (uint32)(uint16)surface->vertex_c;
				draw.triangle_count++;

				if (has_lightmap_vertices && h1_render_shader_pass(draw.shader_group) == _h1_render_pass_opaque)
				{
					const uint16 triangle_vertices[3] = { (uint16)surface->vertex_a, (uint16)surface->vertex_b, (uint16)surface->vertex_c };
					s_h1_lighting_triangle triangle;
					bool valid = true;
					for (int32 k = 0; k < 3; k++)
					{
						if (triangle_vertices[k] >= material_vertex_count)
						{
							valid = false;
							break;
						}
						const s_h1_structure_vertex* vertex = &vertices[vertex_cursor + triangle_vertices[k]];
						triangle.points[k] = { vertex->position[0], vertex->position[1], vertex->position[2] };
						triangle.lightmap_texcoords[k] = { vertex->lightmap_texcoord[0], vertex->lightmap_texcoord[1] };
						triangle.texcoords[k] = { vertex->texcoord[0], vertex->texcoord[1] };
						triangle.normals[k] = { vertex->normal[0], vertex->normal[1], vertex->normal[2] };
						// the compressed lightmap vertex starts with the incident radiosity (11:11:10)
						real32 radiosity[3];
						h1_unpack_normal(*(const uint32*)(data + material_vertex_count * 32 + triangle_vertices[k] * 8), radiosity);
						triangle.incident_radiosity[k] = { radiosity[0], radiosity[1], radiosity[2] };
					}
					triangle.lightmap_bitmap_index = lightmap->bitmap;
					triangle.material_index = lighting_material_index;
					triangle.diffuse_bitmap_tag_index = diffuse_bitmap_tag_index;
					triangle.diffuse_bitmap_index = diffuse_bitmap_index;
					if (valid)
					{
						g_h1_render.lighting_triangles.push_back(triangle);
					}
				}
			}

			vertex_cursor += material_vertex_count;
			g_h1_render.draws.push_back(draw);
		}
	}

	g_h1_render.vertex_buffer->Unlock();
	g_h1_render.index_buffer->Unlock();

	h1_log("render: structure has %d vertices, %d triangles in %d draws", vertex_count, index_count / 3, (int32)g_h1_render.draws.size());
	return true;
}

static void h1_render_scenery_initialize(void)
{
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();

	// sky
	g_h1_render.sky_model_index = NONE;
	if (scenario->skies.count > 0)
	{
		const h1_scnr_skies* sky_reference = g_h1_cache_file->block_get(scenario->skies, 0);
		const h1_sky* sky = (const h1_sky*)g_h1_cache_file->tag_get(sky_reference->sky);
		if (sky)
		{
			g_h1_render.sky_model_index = sky->model.index;
			g_h1_render.sky_fog_color = sky->outdoor_fog_color;
		}
	}

	// scenery placements
	for (int32 i = 0; i < scenario->scenery.count; i++)
	{
		const h1_scnr_scenery* placement = g_h1_cache_file->block_get(scenario->scenery, i);
		const h1_scnr_scenery_palette* palette = g_h1_cache_file->block_get(scenario->scenery_palette, placement->palette_index);
		if (!palette || h1_scenery_placement_is_object(i))
		{
			continue;
		}
		const h1_scen* scenery = (const h1_scen*)g_h1_cache_file->tag_get(palette->name);
		if (!scenery || scenery->model.index == NONE)
		{
			continue;
		}

		s_h1_scenery_instance instance;
		instance.model_index = scenery->model.index;
		instance.permutation = MAX(placement->desired_permutation, (int16)0);
		h1_matrix_from_euler(&placement->rotation, &placement->position, &instance.matrix);
		h1_render_lighting_at(&placement->position, &instance.lighting);
		h1_object_change_colors_choose(palette->name.index, &placement->position, instance.change_colors);
		instance.placement_index = i;
		g_h1_render.scenery.push_back(instance);
	}

	h1_log("render: sky model %08x, %d scenery instances", g_h1_render.sky_model_index, (int32)g_h1_render.scenery.size());
	return;
}

static void h1_render_structure_pass(e_h1_render_pass pass)
{
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	device->SetStreamSource(0, g_h1_render.vertex_buffer, 0, sizeof(s_h1_structure_vertex));
	device->SetIndices(g_h1_render.index_buffer);
	h1_render_set_camera_constants(NULL, false);
	h1_render_shader_fog_context_set(true, NULL);

	const real32 game_time = h1_render_game_time();
	for (const s_h1_structure_draw& draw : g_h1_render.draws)
	{
		if (draw.triangle_count <= 0 || h1_render_shader_pass(draw.shader_group) != pass)
		{
			continue;
		}
		// halo 1 stops drawing a material once its breakable surface is broken (structure_render.c)
		if (!h1_render_breakable_surface_extant(draw.breakable_surface_index))
		{
			continue;
		}

		IDirect3DBaseTexture9* lightmap = draw.lightmap_bitmap_index != NONE ?
			h1_bitmap_texture_get(g_h1_render.lightmap_bitmap_tag, draw.lightmap_bitmap_index) :
			NULL;

		// surfaces without a lightmap use their material's radiosity lighting
		s_h1_render_lighting material_lighting = g_h1_render.lighting;
		if (!lightmap && VALID_INDEX(draw.lighting_material_index, (int32)g_h1_render.lighting_materials.size()))
		{
			const s_h1_lighting_material* material = &g_h1_render.lighting_materials[draw.lighting_material_index];
			material_lighting.ambient = material->ambient;
			material_lighting.light0_color = material->light0_color;
			material_lighting.light0_direction = material->light0_direction;
			material_lighting.light1_color = material->light1_color;
			material_lighting.light1_direction = material->light1_direction;
		}

		for (int32 subpass = 0; subpass < h1_render_shader_subpass_count(draw.shader_group); subpass++)
		{
			if (h1_render_shader_bind(draw.shader_group, draw.shader_index, &material_lighting, lightmap, game_time, subpass))
			{
				if (g_h1_render_debug_mode == 6)
				{
					// development: a distinct color per draw
					const int32 draw_index = (int32)(&draw - g_h1_render.draws.data());
					const real32 debug_color[4] = { 6.f, (real32)(((draw_index + 1) * 67) % 256) / 255.f, (real32)(((draw_index + 1) * 151) % 256) / 255.f, (real32)(((draw_index + 1) * 211) % 256) / 255.f };
					device->SetPixelShaderConstantF(3, debug_color, 1);
					static bool x_logged = false;
					if (!x_logged && draw_index == (int32)g_h1_render.draws.size() - 1) x_logged = true;
					if (!x_logged)
					{
						h1_log("draw %d: %.4s %s lightmap %d triangles %d color %d %d %d", draw_index, (const char*)&draw.shader_group, g_h1_cache_file->tag_name_get(draw.shader_index), draw.lightmap_bitmap_index, draw.triangle_count,
							(draw_index * 67) % 256, (draw_index * 151) % 256, (draw_index * 211) % 256);
					}
				}
				device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, draw.base_vertex, 0, draw.vertex_count, draw.first_index, draw.triangle_count);
				h1_render_shader_unbind();
			}
		}
	}
	return;
}

static void h1_unpack_normal(uint32 packed, real32* out)
{
	// 11:11:10 signed
	const int32 x = (int32)(packed << 21) >> 21;
	const int32 y = (int32)(packed << 10) >> 21;
	const int32 z = (int32)packed >> 22;
	out[0] = (real32)x / 1023.f;
	out[1] = (real32)y / 1023.f;
	out[2] = (real32)z / 511.f;
	return;
}


int32 h1_render_structure_triangle_count(void)
{
	return (int32)g_h1_render.lighting_triangles.size();
}

const real_point3d* h1_render_structure_triangle_get(int32 index)
{
	return g_h1_render.lighting_triangles[index].points;
}

// the lightmapped structure triangle under a point (within 10 units down), its barycentric weights
static const s_h1_lighting_triangle* h1_render_lighting_triangle_below(const real_point3d* point, real32* weights)
{
	const s_h1_lighting_triangle* best = NULL;
	real32 best_z = -FLT_MAX;
	for (const s_h1_lighting_triangle& triangle : g_h1_render.lighting_triangles)
	{
		const real_point3d& a = triangle.points[0];
		const real_point3d& b = triangle.points[1];
		const real_point3d& c = triangle.points[2];
		const real32 denominator = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
		if (fabsf(denominator) < 1e-6f)
		{
			continue;
		}
		const real32 w0 = ((b.y - c.y) * (point->x - c.x) + (c.x - b.x) * (point->y - c.y)) / denominator;
		const real32 w1 = ((c.y - a.y) * (point->x - c.x) + (a.x - c.x) * (point->y - c.y)) / denominator;
		const real32 w2 = 1.f - w0 - w1;
		if (w0 < 0.f || w1 < 0.f || w2 < 0.f)
		{
			continue;
		}
		const real32 z = w0 * a.z + w1 * b.z + w2 * c.z;
		if (z <= point->z && z >= point->z - 10.f && z > best_z)
		{
			best = &triangle;
			best_z = z;
			weights[0] = w0;
			weights[1] = w1;
			weights[2] = w2;
		}
	}
	return best;
}

void h1_render_light_particle(const real_point3d* point, real_rgb_color* out_light, real_rgb_color* out_diffuse)
{
	*out_light = { 0.5f, 0.5f, 0.5f };
	*out_diffuse = { 0.5f, 0.5f, 0.5f };
	real32 weights[3];
	const s_h1_lighting_triangle* triangle = h1_render_lighting_triangle_below(point, weights);
	if (!triangle)
	{
		return;
	}
	auto shade2 = [&](const real_point2d* values) -> real_point2d
	{
		return { weights[0] * values[0].x + weights[1] * values[1].x + weights[2] * values[2].x, weights[0] * values[0].y + weights[1] * values[1].y + weights[2] * values[2].y };
	};
	const real_point2d lightmap_texcoord = shade2(triangle->lightmap_texcoords);
	real_rgb_color light;
	if (g_h1_render.lightmap_bitmap_tag != NONE &&
		h1_bitmap_sample_lod(g_h1_render.lightmap_bitmap_tag, triangle->lightmap_bitmap_index, lightmap_texcoord.x, lightmap_texcoord.y, 1.f, &light))
	{
		out_light->red = MIN(light.red + 0.1f, 1.f);
		out_light->green = MIN(light.green + 0.1f, 1.f);
		out_light->blue = MIN(light.blue + 0.1f, 1.f);
	}
	const real_point2d texcoord = shade2(triangle->texcoords);
	if (triangle->diffuse_bitmap_tag_index != NONE)
	{
		h1_bitmap_sample_lod(triangle->diffuse_bitmap_tag_index, triangle->diffuse_bitmap_index, texcoord.x, texcoord.y, 0.3f, out_diffuse);
	}
	return;
}

// lighting from the lightmapped structure surface below a point: the material's radiosity lights scaled by the lightmap
// object_lights.c lights_distant_lighting_at_point: the structure under the point (10 units down) lights it from its lightmap, its
// incident radiosity and its diffuse color (build_distant_lights), the bsp's default lighting without one
void h1_render_lighting_at(const real_point3d* point, s_h1_render_lighting* out_lighting)
{
	if (g_h1_render.lighting.ambient.red != 0.f)
	{
		*out_lighting = g_h1_render.lighting;
	}
	else
	{
		// default_object_lighting
		out_lighting->ambient = { 0.2f, 0.2f, 0.2f };
		out_lighting->light0_color = { 1.f, 1.f, 1.f };
		out_lighting->light0_direction = { -0.577f, -0.577f, -0.577f };
		out_lighting->light1_color = { 0.4f, 0.4f, 0.5f };
		out_lighting->light1_direction = { 0.f, 0.f, 1.f };
		out_lighting->reflection_tint = { 0.5f, 1.f, 1.f, 1.f };
	}

	real32 weights[3] = {};
	const s_h1_lighting_triangle* best = h1_render_lighting_triangle_below(point, weights);
	if (!best || best->diffuse_bitmap_tag_index == NONE)
	{
		return;
	}

	auto shade2 = [&](const real_point2d* values) -> real_point2d
	{
		return { weights[0] * values[0].x + weights[1] * values[1].x + weights[2] * values[2].x, weights[0] * values[0].y + weights[1] * values[1].y + weights[2] * values[2].y };
	};
	auto shade3 = [&](const real_vector3d* values) -> real_vector3d
	{
		return
		{
			weights[0] * values[0].i + weights[1] * values[1].i + weights[2] * values[2].i,
			weights[0] * values[0].j + weights[1] * values[1].j + weights[2] * values[2].j,
			weights[0] * values[0].k + weights[1] * values[1].k + weights[2] * values[2].k
		};
	};
	real_rgb_color diffuse_color;
	real_rgb_color lightmap_color;
	const real_point2d texcoord = shade2(best->texcoords);
	const real_point2d lightmap_texcoord = shade2(best->lightmap_texcoords);
	if (!h1_bitmap_sample_lod(best->diffuse_bitmap_tag_index, best->diffuse_bitmap_index, texcoord.x, texcoord.y, 0.3f, &diffuse_color) ||
		!h1_bitmap_sample_lod(g_h1_render.lightmap_bitmap_tag, best->lightmap_bitmap_index, lightmap_texcoord.x, lightmap_texcoord.y, 1.f, &lightmap_color))
	{
		return;
	}
	real_vector3d surface_normal = shade3(best->normals);
	normalize3d(&surface_normal);
	real_vector3d radiosity[3];
	real32 lengths[3];
	for (int32 i = 0; i < 3; i++)
	{
		radiosity[i] = best->incident_radiosity[i];
		lengths[i] = normalize3d(&radiosity[i]);
	}
	real_vector3d radiosity_normal = shade3(radiosity);
	normalize3d(&radiosity_normal);
	(void)lengths;

	// build_distant_lights
	const real32 brightness = lightmap_color.red * 0.299f + lightmap_color.green * 0.587f + lightmap_color.blue * 0.114f;
	out_lighting->ambient = { lightmap_color.red * 0.4f + 0.03f, lightmap_color.green * 0.4f + 0.03f, lightmap_color.blue * 0.4f + 0.03f };
	out_lighting->light0_color = lightmap_color;
	out_lighting->light0_direction = { -radiosity_normal.i, -radiosity_normal.j, -radiosity_normal.k };
	out_lighting->light1_color = { diffuse_color.red * brightness, diffuse_color.green * brightness, diffuse_color.blue * brightness };
	out_lighting->light1_direction = surface_normal;
	out_lighting->reflection_tint.alpha = PIN(brightness * 1.5f + 0.25f, 0.f, 1.f);
	out_lighting->reflection_tint.red = PIN(diffuse_color.red * 3.f + 0.5f, 0.f, 1.f) * PIN(lightmap_color.red * 2.f + 0.25f, 0.f, 1.f);
	out_lighting->reflection_tint.green = PIN(diffuse_color.green * 3.f + 0.5f, 0.f, 1.f) * PIN(lightmap_color.green * 2.f + 0.25f, 0.f, 1.f);
	out_lighting->reflection_tint.blue = PIN(diffuse_color.blue * 3.f + 0.5f, 0.f, 1.f) * PIN(lightmap_color.blue * 2.f + 0.25f, 0.f, 1.f);
	return;
}

// halo 2 keeps a bit per breakable surface of the structure of each bsp, set while the surface is intact (breakable_surfaces.cpp)
static bool h1_render_breakable_surface_extant(int16 breakable_surface_index)
{
	if (breakable_surface_index < 0 || breakable_surface_index >= 256)
	{
		return true;
	}

	const uint8* breakable_surface_globals = *Memory::GetAddress<uint8**>(0x4D1298);
	const int16 structure_bsp_index = *Memory::GetAddress<int16*>(0x4119A4);
	if (!breakable_surface_globals || structure_bsp_index < 0 || structure_bsp_index >= 16)
	{
		return true;
	}

	const uint32* extant_bits = (const uint32*)(breakable_surface_globals + 1 + structure_bsp_index * 0x20);
	return TEST_BIT(extant_bits[breakable_surface_index >> 5], breakable_surface_index & 31);
}

// rasterizer_xbox_environment_fog.c: the opaque structure again, blending the fog over it where the depth is equal
static void h1_render_structure_fog_pass(void)
{
	if (!h1_fog_active())
	{
		return;
	}

	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	device->SetStreamSource(0, g_h1_render.vertex_buffer, 0, sizeof(s_h1_structure_vertex));
	device->SetIndices(g_h1_render.index_buffer);
	h1_render_set_camera_constants(NULL, false);
	if (!h1_render_environment_fog_bind())
	{
		return;
	}

	for (const s_h1_structure_draw& draw : g_h1_render.draws)
	{
		if (draw.triangle_count <= 0 || h1_render_shader_pass(draw.shader_group) != _h1_render_pass_opaque || !h1_render_breakable_surface_extant(draw.breakable_surface_index))
		{
			continue;
		}
		device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, draw.base_vertex, 0, draw.vertex_count, draw.first_index, draw.triangle_count);
	}

	device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	device->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
	h1_render_shader_unbind();
	return;
}

#include "stdafx.h"
#include "h1_render.h"

#include "h1_bitmaps.h"
#include "h1_cache_file.h"
#include "h1_log.h"
#include "h1_map_loader.h"
#include "h1_render_models.h"
#include "h1_render_shaders.h"

#include "game/game_time.h"
#include "rasterizer/rasterizer_globals.h"
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
};

struct s_h1_scenery_instance
{
	datum model_index;
	int16 permutation;
	real_matrix4x3 matrix;
	s_h1_render_lighting lighting;
};

// cpu copy of the lightmapped structure, used to light objects from the surface below them
struct s_h1_lighting_triangle
{
	real_point3d points[3];
	real_point2d lightmap_texcoords[3];
	int32 lightmap_bitmap_index;
	int32 material_index;
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
static void h1_unpack_normal(uint32 packed, real32* out);
static void h1_matrix_from_euler(const real_euler_angles3d* angles, const real_point3d* position, real_matrix4x3* out);
static real32 h1_render_game_time(void);
static void h1_render_lighting_at(const real_point3d* point, s_h1_render_lighting* out_lighting);

/* public code */

void h1_render_structure_opaque(void)
{
	if (!h1_render_begin())
	{
		return;
	}

	if (g_h1_render_debug_mode != 9)
	{
		h1_render_structure_pass(_h1_render_pass_opaque);
	}


	const real32 game_time = h1_render_game_time();
	for (const s_h1_scenery_instance& instance : g_h1_render.scenery)
	{
		h1_render_model_draw(instance.model_index, instance.permutation, &instance.matrix, &instance.lighting, _h1_render_pass_opaque, false, game_time);
	}

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
		h1_render_model_draw(instance.model_index, instance.permutation, &instance.matrix, &instance.lighting, _h1_render_pass_transparent, false, game_time);
	}

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
	if (g_h1_render.vertex_buffer) g_h1_render.vertex_buffer->Release();
	if (g_h1_render.index_buffer) g_h1_render.index_buffer->Release();
	if (g_h1_render.state_block) g_h1_render.state_block->Release();
	g_h1_render.vertex_buffer = NULL;
	g_h1_render.index_buffer = NULL;
	g_h1_render.state_block = NULL;
	std::vector<s_h1_structure_draw>().swap(g_h1_render.draws);
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

	const h1_sbsp* bsp = (const h1_sbsp*)g_h1_cache_file->tag_get('sbsp', g_h1_cache_file->structure_bsp_tag_get(0));
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
					}
					triangle.lightmap_bitmap_index = lightmap->bitmap;
					triangle.material_index = lighting_material_index;
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
		if (!palette)
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

	const real32 game_time = h1_render_game_time();
	for (const s_h1_structure_draw& draw : g_h1_render.draws)
	{
		if (draw.triangle_count <= 0 || h1_render_shader_pass(draw.shader_group) != pass)
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

		if (h1_render_shader_bind(draw.shader_group, draw.shader_index, &material_lighting, lightmap, game_time))
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

// yaw about z, then pitch (up), then roll about forward; matrix rows are forward, left, up and position
static void h1_matrix_from_euler(const real_euler_angles3d* angles, const real_point3d* position, real_matrix4x3* out)
{
	const real32 cy = cosf(angles->yaw), sy = sinf(angles->yaw);
	const real32 cp = cosf(angles->pitch), sp = sinf(angles->pitch);
	const real32 cr = cosf(angles->roll), sr = sinf(angles->roll);

	// M = Rz(yaw) * Ry(-pitch) * Rx(roll)
	const real32 m[3][3] =
	{
		{ cy * cp, cy * sp * sr - sy * cr, cy * sp * cr + sy * sr },
		{ sy * cp, sy * sp * sr + cy * cr, sy * sp * cr - cy * sr },
		{ sp, -cp * sr, -cp * cr },
	};

	out->scale = 1.f;
	for (int32 row = 0; row < 3; row++)
	{
		// row = image of basis vector = column of M
		out->n[row][0] = m[0][row];
		out->n[row][1] = m[1][row];
		out->n[row][2] = m[2][row];
	}
	// up must be the cross of forward and left (right handed)
	out->n[2][0] = out->n[0][1] * out->n[1][2] - out->n[0][2] * out->n[1][1];
	out->n[2][1] = out->n[0][2] * out->n[1][0] - out->n[0][0] * out->n[1][2];
	out->n[2][2] = out->n[0][0] * out->n[1][1] - out->n[0][1] * out->n[1][0];
	out->n[3][0] = position->x;
	out->n[3][1] = position->y;
	out->n[3][2] = position->z;
	return;
}

// lighting from the lightmapped structure surface below a point: the material's radiosity lights scaled by the lightmap
static void h1_render_lighting_at(const real_point3d* point, s_h1_render_lighting* out_lighting)
{
	*out_lighting = g_h1_render.lighting;

	const s_h1_lighting_triangle* best = NULL;
	real32 best_z = -FLT_MAX;
	real32 best_weights[3] = {};
	const real32 test_z = point->z + 0.5f;

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
		if (z <= test_z && z > best_z)
		{
			best = &triangle;
			best_z = z;
			best_weights[0] = w0;
			best_weights[1] = w1;
			best_weights[2] = w2;
		}
	}

	if (!best)
	{
		// nothing below, use the brightest material lighting so objects aren't black
		out_lighting->ambient = { 0.35f, 0.35f, 0.35f };
		out_lighting->light0_color = { 0.7f, 0.7f, 0.7f };
		out_lighting->light0_direction = { -0.577f, -0.577f, -0.577f };
		out_lighting->light1_color = { 0.2f, 0.2f, 0.25f };
		out_lighting->light1_direction = { 0.f, 0.f, 1.f };
		return;
	}

	const s_h1_lighting_material* material = &g_h1_render.lighting_materials[best->material_index];
	out_lighting->ambient = material->ambient;
	out_lighting->light0_color = material->light0_color;
	out_lighting->light0_direction = material->light0_direction;
	out_lighting->light1_color = material->light1_color;
	out_lighting->light1_direction = material->light1_direction;

	real_rgb_color sample;
	const real32 u = best_weights[0] * best->lightmap_texcoords[0].x + best_weights[1] * best->lightmap_texcoords[1].x + best_weights[2] * best->lightmap_texcoords[2].x;
	const real32 v = best_weights[0] * best->lightmap_texcoords[0].y + best_weights[1] * best->lightmap_texcoords[1].y + best_weights[2] * best->lightmap_texcoords[2].y;
	if (h1_bitmap_sample(g_h1_render.lightmap_bitmap_tag, best->lightmap_bitmap_index, u, v, &sample))
	{
		// shadowed ground darkens the direct light, the lightmap color tints the ambient term
		const real32 brightness = PIN((sample.red * 0.3f + sample.green * 0.59f + sample.blue * 0.11f) * 1.5f, 0.f, 1.f);
		out_lighting->light0_color.red *= brightness;
		out_lighting->light0_color.green *= brightness;
		out_lighting->light0_color.blue *= brightness;
		out_lighting->ambient.red = MAX(out_lighting->ambient.red, sample.red * 0.5f);
		out_lighting->ambient.green = MAX(out_lighting->ambient.green, sample.green * 0.5f);
		out_lighting->ambient.blue = MAX(out_lighting->ambient.blue, sample.blue * 0.5f);
	}
	return;
}

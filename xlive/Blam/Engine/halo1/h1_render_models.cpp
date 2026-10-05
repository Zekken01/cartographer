#include "stdafx.h"
#include "h1_render_models.h"

#include "h1_cache_file.h"
#include "h1_log.h"

#include "math/matrix_math.h"
#include "rasterizer/rasterizer_globals.h"
#include "rasterizer/dx9/rasterizer_dx9_main.h"

#include <unordered_map>

/* structures */

struct s_h1_model_vertex
{
	real32 position[3];
	real32 normal[3];
	real32 texcoord[2];
	real32 lightmap_texcoord[2];
};
static_assert(sizeof(s_h1_model_vertex) == 40);

// what skinning needs of every vertex, parallel to the vertex buffer
struct s_h1_model_skin_vertex
{
	real_point3d position;
	real_vector3d normal;
	real32 texcoord[2];
	uint8 nodes[2];
	real32 node0_weight;
};

struct s_h1_model_part
{
	int16 shader_index;		// into the model's shader block
	int32 base_vertex;
	int32 vertex_count;
	int32 first_index;
	int32 index_count;
};

struct s_h1_model_geometry
{
	int32 first_part;
	int32 part_count;
};

struct s_h1_model
{
	int32 first_geometry;
	int32 geometry_count;
};

/* globals */

static IDirect3DVertexBuffer9* g_h1_model_vertex_buffer = NULL;
static IDirect3DIndexBuffer9* g_h1_model_index_buffer = NULL;
static std::unordered_map<datum, s_h1_model> g_h1_models;
static std::vector<s_h1_model_geometry> g_h1_model_geometries;
static std::vector<s_h1_model_part> g_h1_model_parts;
static std::vector<s_h1_model_skin_vertex> g_h1_model_skin_vertices;

// skinned objects are transformed on the cpu into this buffer every frame
enum { k_h1_skinned_vertex_capacity = 0x10000 };
static IDirect3DVertexBuffer9* g_h1_skinned_vertex_buffer = NULL;
static int32 g_h1_skinned_vertex_cursor = 0;

/* prototypes */

static void h1_unpack_normal(uint32 packed, real32* out);

/* public code */

bool h1_render_models_initialize(void)
{
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	const D3DPOOL pool = rasterizer_globals_get()->use_d3d9_ex ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;

	// count
	int32 vertex_count = 0;
	int32 index_count = 0;
	for (int32 i = 0; i < g_h1_cache_file->tag_count(); i++)
	{
		const h1_cache_file_tag_instance* instance = g_h1_cache_file->tag_instance_get_by_absolute_index(i);
		if (instance->group_tag != 'mode')
		{
			continue;
		}
		const h1_mode* model = (const h1_mode*)g_h1_cache_file->tag_get('mode', instance->tag_index);
		if (!model)
		{
			continue;
		}
		for (int32 g = 0; g < model->geometries.count; g++)
		{
			const h1_mode_geometries* geometry = g_h1_cache_file->block_get(model->geometries, g);
			for (int32 p = 0; p < geometry->parts.count; p++)
			{
				const h1_mode_geometries_parts* part = g_h1_cache_file->block_get(geometry->parts, p);
				vertex_count += part->vertex_count;
				index_count += part->triangle_count + 2;
			}
		}
	}

	if (vertex_count == 0)
	{
		return true;
	}

	if (FAILED(device->CreateVertexBuffer(vertex_count * sizeof(s_h1_model_vertex), D3DUSAGE_WRITEONLY, 0, pool, &g_h1_model_vertex_buffer, NULL)) ||
		FAILED(device->CreateIndexBuffer(index_count * sizeof(uint16), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, pool, &g_h1_model_index_buffer, NULL)))
	{
		h1_log("models: failed to create buffers (%d vertices, %d indices)", vertex_count, index_count);
		return false;
	}

	s_h1_model_vertex* vertices = NULL;
	uint16* indices = NULL;
	if (FAILED(g_h1_model_vertex_buffer->Lock(0, 0, (void**)&vertices, 0)) ||
		FAILED(g_h1_model_index_buffer->Lock(0, 0, (void**)&indices, 0)))
	{
		return false;
	}

	int32 vertex_cursor = 0;
	int32 index_cursor = 0;
	int32 model_count = 0;
	g_h1_model_skin_vertices.resize(vertex_count);
	for (int32 i = 0; i < g_h1_cache_file->tag_count(); i++)
	{
		const h1_cache_file_tag_instance* instance = g_h1_cache_file->tag_instance_get_by_absolute_index(i);
		if (instance->group_tag != 'mode')
		{
			continue;
		}
		const h1_mode* model = (const h1_mode*)g_h1_cache_file->tag_get('mode', instance->tag_index);
		if (!model)
		{
			continue;
		}

		const real32 u_scale = model->base_map_u_scale != 0.f ? model->base_map_u_scale : 1.f;
		const real32 v_scale = model->base_map_v_scale != 0.f ? model->base_map_v_scale : 1.f;

		s_h1_model entry;
		entry.first_geometry = (int32)g_h1_model_geometries.size();
		entry.geometry_count = model->geometries.count;

		for (int32 g = 0; g < model->geometries.count; g++)
		{
			const h1_mode_geometries* geometry = g_h1_cache_file->block_get(model->geometries, g);
			s_h1_model_geometry geometry_entry = { (int32)g_h1_model_parts.size(), 0 };

			for (int32 p = 0; p < geometry->parts.count; p++)
			{
				const h1_mode_geometries_parts* part = g_h1_cache_file->block_get(geometry->parts, p);
				const int32 part_vertex_count = part->vertex_count;
				const int32 part_index_count = part->triangle_count + 2;

				// xbox: +0x64 points at the part's D3DVertexBuffer header (Common, Data, Lock) whose Data is the
				// tag cache address of the 32 byte compressed vertices; triangle strip indices are in the tag cache too
				const uint32* vertex_buffer_header = (const uint32*)g_h1_cache_file->address_get((uint32)part->vertex_offset, 12);
				const uint8* source_vertices = vertex_buffer_header ?
					(const uint8*)g_h1_cache_file->address_get(vertex_buffer_header[1], part_vertex_count * 32) :
					NULL;
				const uint16* source_indices = (const uint16*)g_h1_cache_file->address_get((uint32)part->triangle_offset, part_index_count * sizeof(uint16));
				if (!source_vertices || !source_indices || part_vertex_count <= 0 || part->triangle_count <= 0)
				{
					continue;
				}

				for (int32 v = 0; v < part_vertex_count; v++)
				{
					const uint8* source = source_vertices + v * 32;
					s_h1_model_vertex* vertex = &vertices[vertex_cursor + v];
					csmemcpy(vertex->position, source, sizeof(vertex->position));
					h1_unpack_normal(*(const uint32*)(source + 12), vertex->normal);
					const int16* texcoord = (const int16*)(source + 24);
					vertex->texcoord[0] = (real32)texcoord[0] / 32767.f * u_scale;
					vertex->texcoord[1] = (real32)texcoord[1] / 32767.f * v_scale;
					vertex->lightmap_texcoord[0] = 0.f;
					vertex->lightmap_texcoord[1] = 0.f;

					// nodes are stored times 3, the weight of the first node is a byte
					s_h1_model_skin_vertex* skin = &g_h1_model_skin_vertices[vertex_cursor + v];
					csmemcpy(&skin->position, vertex->position, sizeof(skin->position));
					csmemcpy(&skin->normal, vertex->normal, sizeof(skin->normal));
					skin->texcoord[0] = vertex->texcoord[0];
					skin->texcoord[1] = vertex->texcoord[1];
					skin->nodes[0] = source[28] / 3;
					skin->nodes[1] = source[29] / 3;
					skin->node0_weight = (real32)source[30] / 255.f;
				}

				for (int32 n = 0; n < part_index_count; n++)
				{
					indices[index_cursor + n] = source_indices[n];
				}

				s_h1_model_part part_entry;
				part_entry.shader_index = part->shader_index;
				part_entry.base_vertex = vertex_cursor;
				part_entry.vertex_count = part_vertex_count;
				part_entry.first_index = index_cursor;
				part_entry.index_count = part_index_count;
				g_h1_model_parts.push_back(part_entry);
				geometry_entry.part_count++;

				vertex_cursor += part_vertex_count;
				index_cursor += part_index_count;
			}
			g_h1_model_geometries.push_back(geometry_entry);
		}

		g_h1_models[instance->tag_index] = entry;
		model_count++;
	}

	g_h1_model_vertex_buffer->Unlock();
	g_h1_model_index_buffer->Unlock();
	if (FAILED(device->CreateVertexBuffer(k_h1_skinned_vertex_capacity * sizeof(s_h1_model_vertex), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &g_h1_skinned_vertex_buffer, NULL)))
	{
		h1_log("models: failed to create the skinned vertex buffer");
	}
	g_h1_skinned_vertex_cursor = 0;
	h1_log("models: %d models, %d vertices, %d indices", model_count, vertex_cursor, index_cursor);
	return true;
}

void h1_render_models_dispose(void)
{
	if (g_h1_model_vertex_buffer) g_h1_model_vertex_buffer->Release();
	if (g_h1_model_index_buffer) g_h1_model_index_buffer->Release();
	if (g_h1_skinned_vertex_buffer) g_h1_skinned_vertex_buffer->Release();
	g_h1_model_vertex_buffer = NULL;
	g_h1_model_index_buffer = NULL;
	g_h1_skinned_vertex_buffer = NULL;
	g_h1_models.clear();
	std::vector<s_h1_model_skin_vertex>().swap(g_h1_model_skin_vertices);
	std::vector<s_h1_model_geometry>().swap(g_h1_model_geometries);
	std::vector<s_h1_model_part>().swap(g_h1_model_parts);
	return;
}

// halo 1 objects without functions or change colors export 0 and white
static const real32 k_h1_object_function_values[4] = { 0.f, 0.f, 0.f, 0.f };
static const real_rgb_color k_h1_object_change_colors[4] = { { 1.f, 1.f, 1.f }, { 1.f, 1.f, 1.f }, { 1.f, 1.f, 1.f }, { 1.f, 1.f, 1.f } };

void h1_render_model_draw(datum model_tag_index, int16 permutation, const real_matrix4x3* object_to_world, const s_h1_render_lighting* lighting, e_h1_render_pass pass, bool sky, real32 game_time)
{
	auto found = g_h1_models.find(model_tag_index);
	if (found == g_h1_models.end() || !g_h1_model_vertex_buffer)
	{
		return;
	}

	const h1_mode* model = (const h1_mode*)g_h1_cache_file->tag_get('mode', model_tag_index);
	if (!model)
	{
		return;
	}

	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	device->SetStreamSource(0, g_h1_model_vertex_buffer, 0, sizeof(s_h1_model_vertex));
	device->SetIndices(g_h1_model_index_buffer);
	h1_render_set_camera_constants(object_to_world, sky);

	const s_h1_model* entry = &found->second;
	for (int32 r = 0; r < model->regions.count; r++)
	{
		const h1_mode_regions* region = g_h1_cache_file->block_get(model->regions, r);
		if (region->permutations.count <= 0)
		{
			continue;
		}
		const int16 permutation_index = (int16)PIN(permutation, 0, region->permutations.count - 1);
		const h1_mode_regions_permutations* region_permutation = g_h1_cache_file->block_get(region->permutations, permutation_index);
		const int32 geometry_index = region_permutation->super_high_index;
		if (!VALID_INDEX(geometry_index, entry->geometry_count))
		{
			continue;
		}

		const s_h1_model_geometry* geometry = &g_h1_model_geometries[entry->first_geometry + geometry_index];
		for (int32 p = 0; p < geometry->part_count; p++)
		{
			const s_h1_model_part* part = &g_h1_model_parts[geometry->first_part + p];
			const h1_mode_shaders* shader_reference = g_h1_cache_file->block_get(model->shaders, part->shader_index);
			if (!shader_reference || h1_render_shader_pass(shader_reference->shader.group_tag) != pass)
			{
				continue;
			}

			if (!sky)
			{
				h1_render_shader_object_animation_set(k_h1_object_function_values, k_h1_object_change_colors);
			}
			for (int32 subpass = 0; subpass < h1_render_shader_subpass_count(shader_reference->shader.group_tag); subpass++)
			{
				if (h1_render_shader_bind(shader_reference->shader.group_tag, shader_reference->shader.index, lighting, NULL, game_time, subpass))
				{
					device->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, part->base_vertex, 0, part->vertex_count, part->first_index, part->index_count - 2);
					h1_render_shader_unbind();
				}
			}
			h1_render_shader_object_animation_set(NULL, NULL);
		}
	}
	return;
}

void h1_render_model_draw_skinned(datum model_tag_index, int16 permutation, const real_matrix4x3* node_matrices, int32 node_count, const s_h1_render_lighting* lighting, e_h1_render_pass pass, real32 game_time)
{
	auto found = g_h1_models.find(model_tag_index);
	if (found == g_h1_models.end() || !g_h1_skinned_vertex_buffer || node_count <= 0)
	{
		return;
	}
	const h1_mode* model = (const h1_mode*)g_h1_cache_file->tag_get('mode', model_tag_index);
	if (!model)
	{
		return;
	}

	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	device->SetIndices(g_h1_model_index_buffer);
	// vertices are skinned into world space
	h1_render_set_camera_constants(NULL, false);

	const s_h1_model* entry = &found->second;
	for (int32 r = 0; r < model->regions.count; r++)
	{
		const h1_mode_regions* region = g_h1_cache_file->block_get(model->regions, r);
		if (region->permutations.count <= 0)
		{
			continue;
		}
		const int16 permutation_index = (int16)PIN(permutation, 0, region->permutations.count - 1);
		const h1_mode_regions_permutations* region_permutation = g_h1_cache_file->block_get(region->permutations, permutation_index);
		const int32 geometry_index = region_permutation->super_high_index;
		if (!VALID_INDEX(geometry_index, entry->geometry_count))
		{
			continue;
		}

		const s_h1_model_geometry* geometry = &g_h1_model_geometries[entry->first_geometry + geometry_index];
		for (int32 p = 0; p < geometry->part_count; p++)
		{
			const s_h1_model_part* part = &g_h1_model_parts[geometry->first_part + p];
			const h1_mode_shaders* shader_reference = g_h1_cache_file->block_get(model->shaders, part->shader_index);
			if (!shader_reference || h1_render_shader_pass(shader_reference->shader.group_tag) != pass || part->vertex_count > k_h1_skinned_vertex_capacity)
			{
				continue;
			}

			// append to the dynamic buffer, starting over when it is full
			DWORD lock_flags = D3DLOCK_NOOVERWRITE;
			if (g_h1_skinned_vertex_cursor + part->vertex_count > k_h1_skinned_vertex_capacity)
			{
				g_h1_skinned_vertex_cursor = 0;
				lock_flags = D3DLOCK_DISCARD;
			}
			s_h1_model_vertex* vertices = NULL;
			if (FAILED(g_h1_skinned_vertex_buffer->Lock(g_h1_skinned_vertex_cursor * sizeof(s_h1_model_vertex), part->vertex_count * sizeof(s_h1_model_vertex), (void**)&vertices, lock_flags)))
			{
				continue;
			}
			for (int32 v = 0; v < part->vertex_count; v++)
			{
				const s_h1_model_skin_vertex* source = &g_h1_model_skin_vertices[part->base_vertex + v];
				const real_matrix4x3* first = &node_matrices[source->nodes[0] < node_count ? source->nodes[0] : 0];
				const real_matrix4x3* second = &node_matrices[source->nodes[1] < node_count ? source->nodes[1] : 0];
				const real32 weight = source->node0_weight;

				real_point3d a, b;
				real_vector3d na, nb;
				matrix4x3_transform_point(first, &source->position, &a);
				matrix4x3_transform_normal(first, &source->normal, &na);
				if (weight < 1.f)
				{
					matrix4x3_transform_point(second, &source->position, &b);
					matrix4x3_transform_normal(second, &source->normal, &nb);
					a.x = a.x * weight + b.x * (1.f - weight);
					a.y = a.y * weight + b.y * (1.f - weight);
					a.z = a.z * weight + b.z * (1.f - weight);
					na.i = na.i * weight + nb.i * (1.f - weight);
					na.j = na.j * weight + nb.j * (1.f - weight);
					na.k = na.k * weight + nb.k * (1.f - weight);
				}

				s_h1_model_vertex* vertex = &vertices[v];
				vertex->position[0] = a.x; vertex->position[1] = a.y; vertex->position[2] = a.z;
				vertex->normal[0] = na.i; vertex->normal[1] = na.j; vertex->normal[2] = na.k;
				vertex->texcoord[0] = source->texcoord[0];
				vertex->texcoord[1] = source->texcoord[1];
				vertex->lightmap_texcoord[0] = 0.f;
				vertex->lightmap_texcoord[1] = 0.f;
			}
			g_h1_skinned_vertex_buffer->Unlock();

			device->SetStreamSource(0, g_h1_skinned_vertex_buffer, 0, sizeof(s_h1_model_vertex));
			h1_render_shader_object_animation_set(k_h1_object_function_values, k_h1_object_change_colors);
			for (int32 subpass = 0; subpass < h1_render_shader_subpass_count(shader_reference->shader.group_tag); subpass++)
			{
				if (h1_render_shader_bind(shader_reference->shader.group_tag, shader_reference->shader.index, lighting, NULL, game_time, subpass))
				{
					device->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, g_h1_skinned_vertex_cursor, 0, part->vertex_count, part->first_index, part->index_count - 2);
					h1_render_shader_unbind();
				}
			}
			h1_render_shader_object_animation_set(NULL, NULL);
			g_h1_skinned_vertex_cursor += part->vertex_count;
		}
	}
	return;
}

/* private code */

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

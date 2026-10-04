#include "stdafx.h"
#include "h1_structure_bsp.h"

#include "h1_cache_file.h"
#include "h1_log.h"
#include "h1_mopp.h"
#include "h1_runtime.h"

#include <unordered_map>
#include <vector>
#include "h2_tag_definitions_generated.h"

#include "cache/cache_files.h"
#include "scenario/scenario_definitions.h"
#include "scenario/scenario_definitions.h"
#include "structures/structure_bsp_definitions.h"

/* constants */

// Halo 1 material types (in order) to Halo 2 globals\globals materials
static const int16 k_h1_material_type_to_global_material[] =
{
	48,		// dirt -> tough_terrain_dirt
	52,		// sand -> tough_terrain_sand
	102,	// stone -> hard_terrain_stone
	26,		// snow -> soft_terrain_snow
	46,		// wood -> tough_organic_wood
	54,		// metal hollow -> hard_metal_thin
	54,		// metal thin -> hard_metal_thin
	69,		// metal thick -> hard_metal_thick
	36,		// rubber -> tough_inorganic_rubber
	107,	// glass -> brittle_glass
	126,	// force field -> brittle_elec_for
	11,		// grunt -> soft_organic_flesh_grunt
	77,		// hunter armor -> hard_metal_thick_cov_hunter
	13,		// hunter skin -> soft_organic_flesh_hunter
	10,		// elite -> soft_organic_flesh_elite
	12,		// jackal -> soft_organic_flesh_jackal
	117,	// jackal energy shield -> brittle_elec_cov
	15,		// engineer skin -> soft_organic_flesh_bugger
	117,	// engineer force field -> brittle_elec_cov
	43,		// flood combat form -> tough_floodflesh_combatform
	44,		// flood carrier form -> tough_floodflesh_carrierform
	56,		// cyborg armor -> hard_metal_thin_hum_masterchief
	56,		// cyborg energy shield -> hard_metal_thin_hum_masterchief
	35,		// human armor -> tough_inorganic_armor_hum
	9,		// human skin -> soft_organic_flesh_human
	67,		// sentinel -> hard_metal_thin_for_sentinel
	68,		// monitor -> hard_metal_thin_for_monitor
	39,		// plastic -> tough_inorganic_plastic
	3,		// water -> liquid_thin_water
	18,		// leaves -> soft_organic_plant_leafy
	63,		// elite energy shield -> hard_metal_thin_cov_elite
	101,	// ice -> hard_terrain_ice
	77,		// hunter shield -> hard_metal_thick_cov_hunter
};

/* prototypes */

static int16 h1_index_to_h2_short(int32 index);
static void h1_collision_bsp_build(collision_bsp* destination, const h1_sbsp_collision_bsp* source, int16 material_offset);
static void h1_structure_material_set(structure_collision_material* material, int16 h1_material_type);
static int32 h1_instanced_geometry_build(structure_bsp* bsp, const structure_bsp* host_bsp, int32 structure_material_count, std::vector<real_rectangle3d>& out_instance_bounds);

/* public code */

int16 h1_material_type_to_global_material(int16 h1_material_type)
{
	return VALID_INDEX(h1_material_type, NUMBEROF(k_h1_material_type_to_global_material)) ?
		k_h1_material_type_to_global_material[h1_material_type] :
		0;
}

bool h1_structure_bsp_build(int32 h1_bsp_index, datum h2_structure_bsp_index, datum h2_lightmap_index)
{
	const h1_sbsp* source = (const h1_sbsp*)g_h1_cache_file->tag_get('sbsp', g_h1_cache_file->structure_bsp_tag_get(h1_bsp_index));
	if (!source)
	{
		h1_log("bsp: missing halo 1 structure bsp %d", h1_bsp_index);
		return false;
	}

	cache_file_tag_instance* bsp_instance = cache_get_tag_instance(h2_structure_bsp_index);
	const structure_bsp* host_bsp = (const structure_bsp*)tag_get('sbsp', h2_structure_bsp_index);

	uint32 bsp_offset;
	structure_bsp* bsp = (structure_bsp*)h1_runtime_allocate(sizeof(structure_bsp), &bsp_offset);

	// collision materials (scenery collision models append theirs, see h1_instanced_geometry_build)
	const int32 material_count = source->collision_materials.count;
	structure_collision_material* materials = h1_runtime_block_new(&bsp->collision_materials, material_count);
	for (int32 i = 0; i < material_count; i++)
	{
		const h1_sbsp_collision_materials* h1_material = g_h1_cache_file->block_get(source->collision_materials, i);
		h1_structure_material_set(&materials[i], h1_material->material_type);
	}

	// collision bsp
	collision_bsp* collision = NULL;
	if (source->collision_bsp.count > 0)
	{
		collision = h1_runtime_block_new(&bsp->collision, 1);
		h1_collision_bsp_build(collision, g_h1_cache_file->block_get(source->collision_bsp, 0), 0);
	}

	bsp->vehicle_z_limits.lower = source->vehicle_floor;
	bsp->vehicle_z_limits.upper = source->vehicle_ceiling;
	bsp->world_bounds.x0 = source->world_bounds_x.lower;
	bsp->world_bounds.x1 = source->world_bounds_x.upper;
	bsp->world_bounds.y0 = source->world_bounds_y.lower;
	bsp->world_bounds.y1 = source->world_bounds_y.upper;
	bsp->world_bounds.z0 = source->world_bounds_z.lower;
	bsp->world_bounds.z1 = source->world_bounds_z.upper;

	// leaves, the halo 1 surface references index render surfaces that don't exist in the halo 2 structure
	const int32 leaf_count = source->leaves.count;
	structure_leaf* leaves = h1_runtime_block_new(&bsp->leaves, leaf_count);
	for (int32 i = 0; i < leaf_count; i++)
	{
		const h1_sbsp_leaves* h1_leaf = g_h1_cache_file->block_get(source->leaves, i);
		leaves[i].cluster = h1_leaf->cluster;
		leaves[i].surface_reference_count = 0;
		leaves[i].first_surface_reference_index = 0;
	}

	// clusters
	const int32 cluster_count = source->clusters.count;
	structure_cluster* clusters = (structure_cluster*)h1_runtime_block_allocate(&bsp->clusters, sizeof(structure_cluster), cluster_count);
	for (int32 i = 0; i < cluster_count; i++)
	{
		const h1_sbsp_clusters* h1_cluster = g_h1_cache_file->block_get(source->clusters, i);
		structure_cluster* cluster = &clusters[i];

		cluster->section_block_info.block_offset = NONE;
		cluster->section_block_info.geometry_tag_index = h2_structure_bsp_index;
		cluster->section_block_info.geometry_cache_index = NONE;

		// bounds from the subclusters
		real_rectangle3d bounds = { FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX };
		for (int32 j = 0; j < h1_cluster->subclusters.count; j++)
		{
			const h1_sbsp_clusters_subclusters* subcluster = g_h1_cache_file->block_get(h1_cluster->subclusters, j);
			bounds.x0 = MIN(bounds.x0, subcluster->world_bounds_x.lower);
			bounds.x1 = MAX(bounds.x1, subcluster->world_bounds_x.upper);
			bounds.y0 = MIN(bounds.y0, subcluster->world_bounds_y.lower);
			bounds.y1 = MAX(bounds.y1, subcluster->world_bounds_y.upper);
			bounds.z0 = MIN(bounds.z0, subcluster->world_bounds_z.lower);
			bounds.z1 = MAX(bounds.z1, subcluster->world_bounds_z.upper);
		}
		if (h1_cluster->subclusters.count == 0)
		{
			bounds = bsp->world_bounds;
		}
		cluster->bounds = bounds;

		cluster->scenario_sky_index = (uint8)h1_cluster->sky;
		cluster->scenario_visible_sky_index = (uint8)h1_cluster->sky;
		cluster->media_index = 0xFF;
		cluster->scenario_atmospheric_fog_index = 0xFF;
		cluster->planar_fog_designator = 0xFF;
		cluster->visible_fog_plane_index = 0xFF;
		cluster->background_sound_index = (uint16)NONE;
		cluster->sound_environment_index = (uint16)NONE;
		cluster->weather_index = (uint16)NONE;
		// acoustics: +0x78 ambience sound cluster, +0x7A reverb sound cluster, +0x7C unused (-1)
		*(int16*)((uint8*)cluster + 0x78) = 0;
		*(int16*)((uint8*)cluster + 0x7A) = 0;
		*(int16*)((uint8*)cluster + 0x7C) = NONE;

		const int32 portal_count = h1_cluster->portals.count;
		uint16* portal_indices = h1_runtime_block_new(&cluster->portal_indices, portal_count);
		for (int32 j = 0; j < portal_count; j++)
		{
			portal_indices[j] = g_h1_cache_file->block_get(h1_cluster->portals, j)->portal;
		}
	}

	// cluster portals
	const int32 portal_count = source->cluster_portals.count;
	cluster_portal* portals = h1_runtime_block_new(&bsp->cluster_portals, portal_count);
	for (int32 i = 0; i < portal_count; i++)
	{
		const h1_sbsp_cluster_portals* h1_portal = g_h1_cache_file->block_get(source->cluster_portals, i);
		cluster_portal* portal = &portals[i];
		portal->back_cluster = h1_portal->back_cluster;
		portal->front_cluster = h1_portal->front_cluster;
		portal->plane_index = h1_portal->plane_index;
		portal->centroid = h1_portal->centroid;
		portal->bounding_radius = h1_portal->bounding_radius;
		portal->flags = (e_cluster_portal_flags)(h1_portal->flags & (_cluster_portal_ai_cannot_hear_through_this | _cluster_portal_one_way | _cluster_portal_door | _cluster_portal_no_way));

		const int32 vertex_count = h1_portal->vertices.count;
		real_point3d* vertices = h1_runtime_block_new(&portal->vertices, vertex_count);
		for (int32 j = 0; j < vertex_count; j++)
		{
			vertices[j] = g_h1_cache_file->block_get(h1_portal->vertices, j)->point;
		}
	}

	// a single ambience and reverb sound cluster containing every cluster
	{
		s_tag_block* sound_cluster_blocks[] = { (s_tag_block*)&bsp->ambience_sound_clusters, (s_tag_block*)&bsp->reverb_sound_clusters };
		for (int32 i = 0; i < NUMBEROF(sound_cluster_blocks); i++)
		{
			s_structure_sound_cluster* sound_cluster = h1_runtime_block_new<s_structure_sound_cluster>(sound_cluster_blocks[i], 1);
			uint16* interior = h1_runtime_block_new(&sound_cluster->interior_cluster_indices, cluster_count);
			for (int32 j = 0; j < cluster_count; j++)
			{
				interior[j] = (uint16)j;
			}
		}
	}

	// scenery collision as instanced geometry
	std::vector<real_rectangle3d> instance_bounds;
	const int32 definition_count = h1_instanced_geometry_build(bsp, host_bsp, material_count, instance_bounds);
	const int32 instance_count = (int32)instance_bounds.size();
	for (int32 i = 0; i < cluster_count; i++)
	{
		structure_cluster* cluster = &clusters[i];
		std::vector<uint16> indices;
		for (int32 j = 0; j < instance_count; j++)
		{
			const real_rectangle3d& a = cluster->bounds;
			const real_rectangle3d& b = instance_bounds[j];
			if (a.x0 <= b.x1 && b.x0 <= a.x1 && a.y0 <= b.y1 && b.y0 <= a.y1 && a.z0 <= b.z1 && b.z0 <= a.z1)
			{
				indices.push_back((uint16)j);
			}
		}
		uint16* cluster_indices = h1_runtime_block_new(&cluster->instanced_geometry_indices, (int32)indices.size());
		for (size_t j = 0; j < indices.size(); j++)
		{
			cluster_indices[j] = indices[j];
		}
	}

	// havok queries the structure through a mopp tree over the collision surfaces and instances
	if (collision)
	{
		std::vector<s_h1_mopp_item> items;
		h1_mopp_collect_surfaces(collision, k_mopp_structure_surface_key, items);
		for (int32 i = 0; i < instance_count; i++)
		{
			items.push_back({ k_mopp_instanced_geometry_key | (uint32)i, instance_bounds[i] });
		}

		uint8* mopp = NULL;
		uint32 mopp_size = 0;
		if (h1_mopp_build(items, &mopp, &mopp_size, &bsp->structure_physics.mopp_bounds_min, &bsp->structure_physics.mopp_bounds_max))
		{
			h1_runtime_data_set(&bsp->structure_physics.mopp_code, mopp, mopp_size);
			h1_mopp_free(mopp);
			h1_log("bsp: %u bytes of mopp code", mopp_size);
		}
	}

	// potentially visible set, two bit vectors per cluster; everything is visible for now
	{
		const int32 row_size = ((cluster_count + 31) / 32) * sizeof(uint32) * 2;
		h1_runtime_data_set(&bsp->cluster_data, NULL, row_size * cluster_count);
		csmemset(cache_get_tag_data(bsp->cluster_data.data), 0xFF, row_size * cluster_count);
	}

	// one owner cluster per scenario sky
	{
		const scenario* h2_scenario = (const scenario*)tag_get('scnr', cache_files_get_tags_header()->scenario_index);
		const int32 sky_count = MAX(h2_scenario->skies.count, 1);
		uint16* owners = h1_runtime_block_new(&bsp->sky_owner_cluster, sky_count);
		for (int32 i = 0; i < sky_count; i++)
		{
			owners[i] = (uint16)NONE;
		}
	}

	// these hold a single entry with empty blocks on the host, keep them
	bsp->detail_objects = host_bsp->detail_objects;
	bsp->portal_device_map = host_bsp->portal_device_map;
	bsp->water_definitions = host_bsp->water_definitions;

	// the geometry preload walks the decorator placement without checking the count, it always has one entry
	h1_runtime_block_new(&bsp->decorator_placement, 1);

	// sound distances between clusters
	{
		s_structure_audibility* audibility = h1_runtime_block_new(&bsp->audibility, 1);
		const int32 pair_count = (cluster_count * (cluster_count - 1)) / 2;
		audibility->cluster_distance_bounds.lower = 0.f;
		audibility->cluster_distance_bounds.upper = 100.f;
		h1_runtime_block_new(&audibility->ai_deafening_pas, (pair_count + 31) / 32);
		h1_runtime_block_new(&audibility->cluster_distances, pair_count);
	}

	bsp->decorators.group = (tag_group)NONE;
	bsp->decorators.index = NONE;

	bsp_instance->data_offset = bsp_offset;
	bsp_instance->size = sizeof(structure_bsp);

	// structure lightmap: a single lightmap group without geometry
	{
		cache_file_tag_instance* lightmap_instance = cache_get_tag_instance(h2_lightmap_index);
		const h2x_ltmp* host_lightmap = (const h2x_ltmp*)tag_get('ltmp', h2_lightmap_index);

		uint32 lightmap_offset;
		h2x_ltmp* lightmap = (h2x_ltmp*)h1_runtime_allocate(sizeof(h2x_ltmp), &lightmap_offset);
		lightmap->search_distance_lower_bound = 2.f;
		lightmap->search_distance_upper_bound = 4.f;
		lightmap->luminels_per_world_unit = 6.f;

		h2x_ltmp_lightmap_groups* group = h1_runtime_block_new(&lightmap->lightmap_groups, 1);
		const h2x_ltmp_lightmap_groups* host_group = host_lightmap->lightmap_groups.count > 0 ? host_lightmap->lightmap_groups[0] : NULL;
		group->flags = host_group ? host_group->flags : 0;
		group->bitmap_group = host_group ? host_group->bitmap_group : tag_reference{ (tag_group)NONE, NONE };

		h2x_ltmp_lightmap_groups_clusters* lightmap_clusters = h1_runtime_block_new(&group->clusters, cluster_count);
		h2x_ltmp_lightmap_groups_cluster_render_info* render_info = h1_runtime_block_new(&group->cluster_render_info, cluster_count);
		for (int32 i = 0; i < cluster_count; i++)
		{
			lightmap_clusters[i].resource_block_offset = NONE;
			lightmap_clusters[i].owner_tag = h2_lightmap_index;
			render_info[i].bitmap_index = NONE;
			render_info[i].palette_index = NONE;
		}

		h2x_ltmp_lightmap_groups_poop_definitions* poop_definitions = h1_runtime_block_new(&group->poop_definitions, definition_count);
		for (int32 i = 0; i < definition_count; i++)
		{
			poop_definitions[i].resource_block_offset = NONE;
			poop_definitions[i].owner_tag = h2_lightmap_index;
		}
		// the geometry preload follows every instance bucket ref into the buckets without a bounds check
		// (an unsigned bucket index), so the instances share one empty bucket
		h2x_ltmp_lightmap_groups_geometry_buckets* bucket = h1_runtime_block_new(&group->geometry_buckets, instance_count > 0 ? 1 : 0);
		if (bucket)
		{
			bucket->resource_block_offset = NONE;
			bucket->owner_tag = h2_lightmap_index;
		}
		h2x_ltmp_lightmap_groups_instance_render_info* instance_render_info = h1_runtime_block_new(&group->instance_render_info, instance_count);
		h2x_ltmp_lightmap_groups_instance_bucket_refs* instance_bucket_refs = h1_runtime_block_new(&group->instance_bucket_refs, instance_count);
		for (int32 i = 0; i < instance_count; i++)
		{
			instance_render_info[i].bitmap_index = NONE;
			instance_render_info[i].palette_index = NONE;
			instance_bucket_refs[i].bucket_index = 0;
		}

		lightmap_instance->data_offset = lightmap_offset;
		lightmap_instance->size = sizeof(h2x_ltmp);
	}

	h1_log("bsp: %d collision materials, %d leaves, %d clusters, %d portals, %d scenery collision definitions, %d instances",
		bsp->collision_materials.count, leaf_count, cluster_count, portal_count, definition_count, instance_count);
	return true;
}

/* private code */

// Halo 1 flags indices with the sign bit (leaf indices, flipped planes), Halo 2 uses the sign bit of a short
static int16 h1_index_to_h2_short(int32 index)
{
	if (index < 0)
	{
		return (int16)(0x8000 | (index & 0x7FFF));
	}
	return (int16)index;
}

static int32 h1_index_to_h2_bsp3d_child(int32 index)
{
	if (index == NONE)
	{
		return 0xFFFFFF;
	}
	if (index < 0)
	{
		return 0x800000 | (index & 0x7FFFFF);
	}
	return index;
}

static void h1_collision_bsp_build(collision_bsp* destination, const h1_sbsp_collision_bsp* source, int16 material_offset)
{
	// bsp3d nodes
	{
		const int32 count = source->bsp3d_nodes.count;
		uint8* nodes = (uint8*)h1_runtime_block_allocate(&destination->bsp3d.nodes, sizeof(bsp3d_node), count);
		for (int32 i = 0; i < count; i++)
		{
			const h1_sbsp_collision_bsp_bsp3d_nodes* node = g_h1_cache_file->block_get(source->bsp3d_nodes, i);
			uint8* out = &nodes[i * sizeof(bsp3d_node)];
			const int16 plane = (int16)node->plane;
			const int32 back = h1_index_to_h2_bsp3d_child(node->back_child);
			const int32 front = h1_index_to_h2_bsp3d_child(node->front_child);
			csmemcpy(out, &plane, sizeof(plane));
			csmemcpy(out + 2, &back, 3);
			csmemcpy(out + 5, &front, 3);
		}
	}

	// planes
	{
		const int32 count = source->planes.count;
		real_plane3d* planes = h1_runtime_block_new<real_plane3d>(&destination->bsp3d.planes, count);
		for (int32 i = 0; i < count; i++)
		{
			planes[i] = g_h1_cache_file->block_get(source->planes, i)->plane;
		}
	}

	// leaves
	{
		const int32 count = source->leaves.count;
		collision_leaf* leaves = h1_runtime_block_new<collision_leaf>(&destination->leaves, count);
		for (int32 i = 0; i < count; i++)
		{
			const h1_sbsp_collision_bsp_leaves* leaf = g_h1_cache_file->block_get(source->leaves, i);
			leaves[i].flags = (uint8)leaf->flags;
			leaves[i].bsp_2d_reference_count = (int8)leaf->bsp2d_reference_count;
			leaves[i].first_bsp2d_reference_index = (uint16)leaf->first_bsp2d_reference;
		}
	}

	// bsp2d references
	{
		const int32 count = source->bsp2d_references.count;
		bsp2d_reference* references = h1_runtime_block_new<bsp2d_reference>(&destination->bsp_2d_references, count);
		for (int32 i = 0; i < count; i++)
		{
			const h1_sbsp_collision_bsp_bsp2d_references* reference = g_h1_cache_file->block_get(source->bsp2d_references, i);
			references[i].plane_designator = h1_index_to_h2_short(reference->plane);
			references[i].root_index = h1_index_to_h2_short(reference->bsp2d_node);
		}
	}

	// bsp2d nodes
	{
		const int32 count = source->bsp2d_nodes.count;
		bsp2d_node* nodes = h1_runtime_block_new<bsp2d_node>(&destination->bsp2d.nodes, count);
		for (int32 i = 0; i < count; i++)
		{
			const h1_sbsp_collision_bsp_bsp2d_nodes* node = g_h1_cache_file->block_get(source->bsp2d_nodes, i);
			nodes[i].plane = node->plane;
			nodes[i].child_indices[0] = h1_index_to_h2_short(node->left_child);
			nodes[i].child_indices[1] = h1_index_to_h2_short(node->right_child);
		}
	}

	// surfaces
	{
		const int32 count = source->surfaces.count;
		collision_surface* surfaces = h1_runtime_block_new<collision_surface>(&destination->surfaces, count);
		for (int32 i = 0; i < count; i++)
		{
			const h1_sbsp_collision_bsp_surfaces* surface = g_h1_cache_file->block_get(source->surfaces, i);
			surfaces[i].plane_designator = h1_index_to_h2_short(surface->plane);
			surfaces[i].first_edge_index = (uint16)surface->first_edge;
			surfaces[i].flags = surface->flags;
			surfaces[i].breakable_surface_index = surface->breakable_surface;
			surfaces[i].material_index = surface->material >= 0 ? surface->material + material_offset : surface->material;
		}
	}

	// edges
	{
		const int32 count = source->edges.count;
		collision_edge* edges = h1_runtime_block_new<collision_edge>(&destination->edges, count);
		for (int32 i = 0; i < count; i++)
		{
			const h1_sbsp_collision_bsp_edges* edge = g_h1_cache_file->block_get(source->edges, i);
			edges[i].vertex_indices[0] = (uint16)edge->start_vertex;
			edges[i].vertex_indices[1] = (uint16)edge->end_vertex;
			edges[i].edge_indices[0] = (uint16)edge->forward_edge;
			edges[i].edge_indices[1] = (uint16)edge->reverse_edge;
			edges[i].surface_indices[0] = (int16)edge->left_surface;
			edges[i].surface_indices[1] = (int16)edge->right_surface;
		}
	}

	// vertices
	{
		const int32 count = source->vertices.count;
		collision_vertex* vertices = h1_runtime_block_new<collision_vertex>(&destination->vertices, count);
		for (int32 i = 0; i < count; i++)
		{
			const h1_sbsp_collision_bsp_vertices* vertex = g_h1_cache_file->block_get(source->vertices, i);
			vertices[i].point = vertex->point;
			vertices[i].first_edge_index = (uint16)vertex->first_edge;
			vertices[i].sink = 0;
		}
	}
	return;
}

static void h1_structure_material_set(structure_collision_material* material, int16 h1_material_type)
{
	material->old_shader.group = (tag_group)NONE;
	material->old_shader.index = NONE;
	material->new_shader.group = (tag_group)NONE;
	material->new_shader.index = NONE;
	material->global_material_index = h1_material_type_to_global_material(h1_material_type);
	material->conveyor_surface_index = (uint16)NONE;
	return;
}

// the collision bsp of a halo 1 collision model (node 0), the layout matches the structure collision bsp
static const h1_sbsp_collision_bsp* h1_collision_model_bsp_get(const h1_coll* model)
{
	const h1_coll_nodes* node = g_h1_cache_file->block_get(model->nodes, 0);
	if (!node || node->bsps.count <= 0)
	{
		return NULL;
	}
	static_assert(sizeof(h1_coll_nodes_bsps) == sizeof(h1_sbsp_collision_bsp));
	return (const h1_sbsp_collision_bsp*)g_h1_cache_file->block_get(node->bsps, 0);
}

static int32 h1_instanced_geometry_build(structure_bsp* bsp, const structure_bsp* host_bsp, int32 structure_material_count, std::vector<real_rectangle3d>& out_instance_bounds)
{
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();

	struct s_placement { datum model; real_matrix4x3 matrix; };
	std::vector<datum> models;
	std::unordered_map<datum, int32> model_definitions;
	std::vector<s_placement> placements;

	for (int32 i = 0; i < scenario->scenery.count; i++)
	{
		const h1_scnr_scenery* placement = g_h1_cache_file->block_get(scenario->scenery, i);
		const h1_scnr_scenery_palette* palette = g_h1_cache_file->block_get(scenario->scenery_palette, placement->palette_index);
		const h1_scen* scenery = palette ? (const h1_scen*)g_h1_cache_file->tag_get(palette->name) : NULL;
		const h1_coll* model = scenery ? (const h1_coll*)g_h1_cache_file->tag_get(scenery->collision_model) : NULL;
		if (!model || !h1_collision_model_bsp_get(model))
		{
			continue;
		}

		if (model_definitions.find(scenery->collision_model.index) == model_definitions.end())
		{
			model_definitions[scenery->collision_model.index] = (int32)models.size();
			models.push_back(scenery->collision_model.index);
		}

		s_placement entry;
		entry.model = scenery->collision_model.index;
		h1_matrix_from_euler(&placement->rotation, &placement->position, &entry.matrix);
		placements.push_back(entry);
	}

	if (models.empty())
	{
		return 0;
	}

	// the havok shapes of a definition are copied from the host and given our bounds and mopp
	const h2x_sbsp_instanced_geometry_definitions_bsp_physics* physics_template = NULL;
	if (host_bsp->instanced_geometry_definitions.count > 0)
	{
		const structure_instanced_geometry_definition* host_definition = host_bsp->instanced_geometry_definitions[0];
		if (host_definition->bsp_physics.count > 0)
		{
			physics_template = (const h2x_sbsp_instanced_geometry_definitions_bsp_physics*)tag_block_get_element_with_size((const s_tag_block*)&host_definition->bsp_physics, 0, sizeof(h2x_sbsp_instanced_geometry_definitions_bsp_physics));
		}
	}
	if (!physics_template)
	{
		h1_log("bsp: the host has no instanced geometry physics, scenery has no collision");
		return 0;
	}

	// collision materials of every model follow the structure's
	std::vector<int16> material_offsets;
	int32 total_materials = structure_material_count;
	for (datum model_index : models)
	{
		const h1_coll* model = (const h1_coll*)g_h1_cache_file->tag_get('coll', model_index);
		material_offsets.push_back((int16)total_materials);
		total_materials += model->materials.count;
	}
	{
		const structure_collision_material* structure_materials = (const structure_collision_material*)tag_block_get_element_with_size((const s_tag_block*)&bsp->collision_materials, 0, sizeof(structure_collision_material));
		std::vector<structure_collision_material> copy(structure_materials, structure_materials + structure_material_count);
		structure_collision_material* materials = h1_runtime_block_new(&bsp->collision_materials, total_materials);
		for (int32 i = 0; i < structure_material_count; i++)
		{
			materials[i] = copy[i];
		}
		for (size_t m = 0; m < models.size(); m++)
		{
			const h1_coll* model = (const h1_coll*)g_h1_cache_file->tag_get('coll', models[m]);
			for (int32 i = 0; i < model->materials.count; i++)
			{
				h1_structure_material_set(&materials[material_offsets[m] + i], g_h1_cache_file->block_get(model->materials, i)->material_type);
			}
		}
	}

	// definitions
	const int32 definition_count = (int32)models.size();
	structure_instanced_geometry_definition* definitions = h1_runtime_block_new(&bsp->instanced_geometry_definitions, definition_count);
	for (int32 i = 0; i < definition_count; i++)
	{
		const h1_coll* model = (const h1_coll*)g_h1_cache_file->tag_get('coll', models[i]);
		structure_instanced_geometry_definition* definition = &definitions[i];

		definition->render_info.block.block_offset = NONE;
		definition->render_info.block.geometry_cache_index = NONE;
		definition->checksum = (int32)models[i];
		h1_collision_bsp_build(&definition->collision_info, h1_collision_model_bsp_get(model), material_offsets[i]);

		real_rectangle3d bounds;
		h1_mopp_collision_bsp_bounds(&definition->collision_info, NULL, &bounds);
		definition->bounding_sphere_center = { (bounds.x0 + bounds.x1) * 0.5f, (bounds.y0 + bounds.y1) * 0.5f, (bounds.z0 + bounds.z1) * 0.5f };
		const real32 dx = bounds.x1 - bounds.x0, dy = bounds.y1 - bounds.y0, dz = bounds.z1 - bounds.z0;
		definition->bounding_sphere_radius = 0.5f * sqrtf(dx * dx + dy * dy + dz * dz);

		// render leaves mirror the collision leaves
		const int32 leaf_count = definition->collision_info.leaves.count;
		structure_leaf* leaves = h1_runtime_block_new(&definition->render_leaves, leaf_count);
		for (int32 j = 0; j < leaf_count; j++)
		{
			leaves[j].cluster = 0;
		}

		// havok shape over the definition's surfaces
		std::vector<s_h1_mopp_item> items;
		h1_mopp_collect_surfaces(&definition->collision_info, 0, items);
		uint8* mopp = NULL;
		uint32 mopp_size = 0;
		real_point3d mopp_min, mopp_max;
		if (h1_mopp_build(items, &mopp, &mopp_size, &mopp_min, &mopp_max))
		{
			h2x_sbsp_instanced_geometry_definitions_bsp_physics* physics = (h2x_sbsp_instanced_geometry_definitions_bsp_physics*)h1_runtime_block_allocate(
				(s_tag_block*)&definition->bsp_physics, sizeof(h2x_sbsp_instanced_geometry_definitions_bsp_physics), 1);
			*physics = *physics_template;
			physics->user_data = 0;
			physics->user_data_2 = 0;
			physics->user_data_3 = 0;
			physics->center = { definition->bounding_sphere_center.x, definition->bounding_sphere_center.y, definition->bounding_sphere_center.z };
			physics->half_extent = { dx * 0.5f, dy * 0.5f, dz * 0.5f };
			physics->runtime_model_tag.group = (tag_group)NONE;
			physics->runtime_model_tag.index = NONE;
			h1_runtime_data_set(&physics->mopp_code_data, mopp, mopp_size);
			// the last field holds the mopp origin x in tool built maps
			*(real32*)((uint8*)physics + 0x70) = *(const real32*)mopp;
			h1_mopp_free(mopp);
		}
	}

	// instances
	const int32 instance_count = (int32)placements.size();
	structure_instanced_geometry_instance* instances = h1_runtime_block_new(&bsp->instanced_geometry_instances, instance_count);
	for (int32 i = 0; i < instance_count; i++)
	{
		const s_placement* placement = &placements[i];
		const int32 definition_index = model_definitions[placement->model];
		structure_instanced_geometry_instance* instance = &instances[i];
		instance->scale = 1.f;
		csmemcpy(&instance->transform, &placement->matrix.n[0][0], sizeof(instance->transform));
		instance->position = { placement->matrix.n[3][0], placement->matrix.n[3][1], placement->matrix.n[3][2] };
		instance->instance_definition = (uint16)definition_index;
		instance->checksum = definitions[definition_index].checksum;
		instance->name = _string_id_empty_string;

		real_rectangle3d bounds;
		h1_mopp_collision_bsp_bounds(&definitions[definition_index].collision_info, &placement->matrix, &bounds);
		out_instance_bounds.push_back(bounds);
	}

	return definition_count;
}

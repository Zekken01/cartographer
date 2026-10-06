#include "stdafx.h"
#include "h1_object_tags.h"

#include "h1_cache_file.h"
#include "h1_runtime.h"
#include "h1_structure_bsp.h"
#include "h2_tag_definitions_generated.h"

#include "game/game_globals.h"
#include "math/matrix_math.h"
#include "game/materials.h"
#include "physics/collision_bsp_definition.h"
#include "tag_files/tag_groups.h"

#include <vector>

/* prototypes */

static const char* h1_name_last_component(const char* path);
static string_id h1_string_id(const char* string);
static int16 h1_model_node_find(const h1_mode* h1_model, const char* name);
static real_quaternion h1_quaternion_to_h2(const real_quaternion* rotation);

/* public code */

datum h1_object_render_model_build(const h1_mode* h1_model, const char* name)
{
	h2x_mode* model = NULL;
	const datum model_index = h1_runtime_tag_new('mode', name, &model);
	if (model_index == NONE)
	{
		return NONE;
	}

	model->name = h1_string_id(h1_name_last_component(name));
	model->node_list_checksum = h1_model ? h1_model->node_list_checksum : 0;
	// no geometry: every level of detail and permutation section is NONE
	model->l1_section_group_index_super_low = NONE;
	model->l2_section_group_index_low = NONE;
	model->l3_section_group_index_medium = NONE;
	model->l4_section_group_index_high = NONE;
	model->l5_section_group_index_super_high = NONE;
	model->l6_section_group_index_hollywood = NONE;

	// objects without a halo 1 model (bullets) get a single node
	if (!h1_model)
	{
		h2x_mode_nodes* node = h1_runtime_block_new(&model->nodes, 1);
		node->name = h1_string_id("frame");
		node->parent_node_index = NONE;
		node->first_child_node_index = NONE;
		node->next_sibling_node_index = NONE;
		node->import_node_index = NONE;
		node->default_rotation = { 0.f, 0.f, 0.f, 1.f };
		real_matrix4x3 identity;
		matrix4x3_identity(&identity);
		csmemcpy(&node->inverse_scale, &identity, sizeof(real_matrix4x3));
		return model_index;
	}

	h2x_mode_regions* regions = h1_runtime_block_new(&model->regions, h1_model->regions.count);
	for (int32 r = 0; r < h1_model->regions.count; r++)
	{
		const h1_mode_regions* h1_region = g_h1_cache_file->block_get(h1_model->regions, r);
		regions[r].name = h1_string_id(h1_region->name);
		h2x_mode_regions_permutations* permutations = h1_runtime_block_new(&regions[r].permutations, h1_region->permutations.count);
		for (int32 p = 0; p < h1_region->permutations.count; p++)
		{
			const h1_mode_regions_permutations* h1_permutation = g_h1_cache_file->block_get(h1_region->permutations, p);
			permutations[p].name = h1_string_id(h1_permutation->name);
			permutations[p].l1_section_index_super_low = NONE;
			permutations[p].l2_section_index_low = NONE;
			permutations[p].l3_section_index_medium = NONE;
			permutations[p].l4_section_index_high = NONE;
			permutations[p].l5_section_index_super_high = NONE;
			permutations[p].l6_section_index_hollywood = NONE;
		}
	}

	h2x_mode_nodes* nodes = h1_runtime_block_new(&model->nodes, h1_model->nodes.count);
	for (int32 i = 0; i < h1_model->nodes.count; i++)
	{
		const h1_mode_nodes* h1_node = g_h1_cache_file->block_get(h1_model->nodes, i);
		nodes[i].name = h1_string_id(h1_node->name);
		nodes[i].parent_node_index = h1_node->parent_node_index;
		nodes[i].first_child_node_index = h1_node->first_child_node_index;
		nodes[i].next_sibling_node_index = h1_node->next_sibling_node_index;
		nodes[i].import_node_index = NONE;
		nodes[i].default_translation = h1_node->default_translation;
		nodes[i].default_rotation = h1_quaternion_to_h2(&h1_node->default_rotation);
		csmemcpy(&nodes[i].inverse_scale, &h1_node->inverse_scale, sizeof(real_matrix4x3));
		nodes[i].distance_from_parent = h1_node->node_distance_from_parent;
	}

	// markers
	h2x_mode_marker_groups* groups = h1_runtime_block_new(&model->marker_groups, h1_model->markers.count);
	for (int32 i = 0; i < h1_model->markers.count; i++)
	{
		const h1_mode_markers* h1_marker = g_h1_cache_file->block_get(h1_model->markers, i);
		// halo 2 looks markers up by its own names, which spell halo 1's spaces as underscores ("ground_point")
		char marker_name[32];
		strncpy_s(marker_name, h1_marker->name, _TRUNCATE);
		for (char* c = marker_name; *c; c++)
		{
			if (*c == ' ')
			{
				*c = '_';
			}
		}
		groups[i].name = h1_string_id(marker_name);
		h2x_mode_marker_groups_markers* markers = h1_runtime_block_new(&groups[i].markers, h1_marker->instances.count);
		for (int32 j = 0; j < h1_marker->instances.count; j++)
		{
			const h1_mode_markers_instances* instance = g_h1_cache_file->block_get(h1_marker->instances, j);
			markers[j].region_index = instance->region_index;
			markers[j].permutation_index = instance->permutation_index;
			markers[j].node_index = instance->node_index;
			markers[j].translation = instance->translation;
			markers[j].rotation = h1_quaternion_to_h2(&instance->rotation);
			markers[j].scale = 1.f;
		}
	}
	return model_index;
}

datum h1_object_collision_model_build(const h1_coll* h1_collision, const h1_mode* h1_model, const char* name)
{
	h2x_coll* collision = NULL;
	const datum collision_index = h1_runtime_tag_new('coll', name, &collision);
	if (collision_index == NONE)
	{
		return NONE;
	}

	h2x_coll_materials* materials = h1_runtime_block_new(&collision->materials, h1_collision->materials.count);
	for (int32 i = 0; i < h1_collision->materials.count; i++)
	{
		materials[i].name = h1_string_id(g_h1_cache_file->block_get(h1_collision->materials, i)->name);
	}

	h2x_coll_nodes* nodes = h1_runtime_block_new(&collision->nodes, h1_collision->nodes.count);
	for (int32 i = 0; i < h1_collision->nodes.count; i++)
	{
		const h1_coll_nodes* h1_node = g_h1_cache_file->block_get(h1_collision->nodes, i);
		nodes[i].name = h1_string_id(h1_node->name);
		nodes[i].parent_node_index = h1_node->parent_node_index;
		nodes[i].next_sibling_node_index = h1_node->next_sibling_node_index;
		nodes[i].first_child_node_index = h1_node->first_child_node_index;
	}

	// halo 1 keeps a bsp per region permutation in every node, halo 2 keeps the node's bsps in the region permutation
	h2x_coll_regions* regions = h1_runtime_block_new(&collision->regions, h1_collision->regions.count);
	for (int32 r = 0; r < h1_collision->regions.count; r++)
	{
		const h1_coll_regions* h1_region = g_h1_cache_file->block_get(h1_collision->regions, r);
		regions[r].name = h1_string_id(h1_region->name);
		const int32 permutation_count = MAX(h1_region->permutations.count, 1);
		h2x_coll_regions_permutations* permutations = h1_runtime_block_new(&regions[r].permutations, permutation_count);
		for (int32 p = 0; p < permutation_count; p++)
		{
			const h1_coll_regions_permutations* h1_permutation = g_h1_cache_file->block_get(h1_region->permutations, p);
			permutations[p].name = h1_permutation ? h1_string_id(h1_permutation->name) : _string_id_default;

			std::vector<int32> bsp_nodes;
			for (int32 n = 0; n < h1_collision->nodes.count; n++)
			{
				const h1_coll_nodes* h1_node = g_h1_cache_file->block_get(h1_collision->nodes, n);
				if (h1_node->region_index == r && h1_node->bsps.count > 0)
				{
					bsp_nodes.push_back(n);
				}
			}
			h2x_coll_regions_permutations_bsps* bsps = h1_runtime_block_new(&permutations[p].bsps, (int32)bsp_nodes.size());
			for (size_t b = 0; b < bsp_nodes.size(); b++)
			{
				const h1_coll_nodes* h1_node = g_h1_cache_file->block_get(h1_collision->nodes, bsp_nodes[b]);
				const h1_coll_nodes_bsps* h1_bsp = g_h1_cache_file->block_get(h1_node->bsps, MIN(p, h1_node->bsps.count - 1));
				// collision nodes are the model's nodes
				const int16 model_node = h1_model_node_find(h1_model, h1_node->name);
				bsps[b].node_index = model_node != NONE ? model_node : (int16)bsp_nodes[b];
				h1_collision_bsp_build((collision_bsp*)&bsps[b].bsp_3d_nodes, (const h1_sbsp_collision_bsp*)h1_bsp, 0);
			}
		}
	}

	h2x_coll_pathfinding_spheres* spheres = h1_runtime_block_new(&collision->pathfinding_spheres, h1_collision->pathfinding_spheres.count);
	for (int32 i = 0; i < h1_collision->pathfinding_spheres.count; i++)
	{
		const h1_coll_pathfinding_spheres* h1_sphere = g_h1_cache_file->block_get(h1_collision->pathfinding_spheres, i);
		spheres[i].node_index = h1_sphere->node_index;
		spheres[i].center = h1_sphere->center;
		spheres[i].radius = h1_sphere->radius;
	}
	return collision_index;
}

datum h1_object_model_build(const s_h1_object_tags* tags, const h1_mode* h1_model, const h1_coll* h1_collision, const char* name)
{
	h2x_hlmt* model = NULL;
	const datum model_index = h1_runtime_tag_new('hlmt', name, &model);
	if (model_index == NONE)
	{
		return NONE;
	}

	h1_runtime_reference_set(&model->render_model, 'mode', tags->render_model);
	h1_runtime_reference_set(&model->collision_model, tags->collision_model != NONE ? 'coll' : (tag_group)NONE, tags->collision_model);
	h1_runtime_reference_set(&model->animation, tags->animation_graph != NONE ? 'jmad' : (tag_group)NONE, tags->animation_graph);
	h1_runtime_reference_set(&model->physics, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&model->physics_model, tags->physics_model != NONE ? 'phmo' : (tag_group)NONE, tags->physics_model);
	h1_runtime_reference_set(&model->default_dialogue, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&model->active_camo_shader, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&model->hologram_shader, (tag_group)NONE, NONE);
	model->disappear_distance = tags->disappear_distance;
	model->begin_fade_distance = tags->disappear_distance * 0.9f;
	model->node_list_checksum = h1_model ? h1_model->node_list_checksum : 0;

	// one variant using every region's first permutation
	const int32 region_count = h1_model ? MIN(h1_model->regions.count, 16) : 0;
	h2x_hlmt_variants* variant = h1_runtime_block_new(&model->variants, 1);
	variant->name = _string_id_default;
	int8* runtime_region_indices = &variant->runtime_model_region_0_index;
	for (int32 r = 0; r < 16; r++)
	{
		runtime_region_indices[r] = r < region_count ? (int8)r : NONE;
	}
	h2x_hlmt_variants_regions* variant_regions = h1_runtime_block_new(&variant->regions, region_count);
	for (int32 r = 0; r < region_count; r++)
	{
		const h1_mode_regions* h1_region = g_h1_cache_file->block_get(h1_model->regions, r);
		const h1_mode_regions_permutations* h1_permutation = g_h1_cache_file->block_get(h1_region->permutations, 0);
		variant_regions[r].region_name = h1_string_id(h1_region->name);
		variant_regions[r].runtime_model_region_index = (int8)r;
		variant_regions[r].parent_variant_index = NONE;
		h2x_hlmt_variants_regions_permutations* permutation = h1_runtime_block_new(&variant_regions[r].permutations, 1);
		permutation->permutation_name = h1_permutation ? h1_string_id(h1_permutation->name) : _string_id_default;
		permutation->runtime_model_permutation_index = 0;
		permutation->probability = 1.f;
		int8* state_indices = &permutation->runtime_state_permutation_index_0;
		for (int32 s = 0; s < 5; s++)
		{
			state_indices[s] = NONE;
		}
	}

	// materials of the collision model
	// objects without a collision model (halo 1 grenades) have no materials, damage or collision regions
	if (!h1_collision)
	{
		goto nodes;
	}
	{
	h2x_hlmt_materials* materials = h1_runtime_block_new(&model->materials, h1_collision->materials.count);
	for (int32 i = 0; i < h1_collision->materials.count; i++)
	{
		const h1_coll_materials* h1_material = g_h1_cache_file->block_get(h1_collision->materials, i);
		const int16 global_material = h1_material_type_to_global_material(h1_material->material_type);
		materials[i].material_name = h1_string_id(h1_material->name);
		materials[i].damage_section_index = NONE;
		materials[i].collision_global_material_index = global_material;
		materials[i].damage_global_material_index = global_material;
		materials[i].global_material_name = h1_global_material_name(global_material);
		materials[i].global_material_index = global_material;
	}

	// body vitality of the collision model
	h2x_hlmt_new_damage_info* damage = h1_runtime_block_new(&model->new_damage_info, 1);
	damage->indirect_damage_section_index = NONE;
	damage->maximum_vitality = h1_collision->maximum_body_vitality > 0.f ? h1_collision->maximum_body_vitality : 1.f;
	// halo 1's stun and recharge are the shield's (its body doesn't recharge): halo 2's second set, with its runtime recharge
	// velocity (fraction per second)
	damage->maximum_shield_vitality = h1_collision->maximum_shield_vitality;
	damage->minimum_stun_damage_2 = h1_collision->minimum_stun_damage;
	damage->stun_time_2 = h1_collision->stun_time;
	damage->recharge_time_2 = h1_collision->recharge_time;
	damage->shield_recharge_velocity = h1_collision->recharge_time > 0.f ? 1.f / h1_collision->recharge_time : 0.f;
	damage->shield_damaged_threshold = h1_collision->shield_damaged_threshold;
	damage->shield_global_material_index = NONE;
	damage->indirect_global_material_index = NONE;
	h1_runtime_reference_set(&damage->shield_damaged_first_person_shader, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&damage->shield_damaged_shader, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&damage->shield_damaged_effect, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&damage->shield_depleted_effect, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&damage->shield_recharging_effect, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&damage->overshield_first_person_shader, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&damage->overshield_shader, (tag_group)NONE, NONE);

	// collision regions map to the collision model's regions
	const h2x_coll* collision = (const h2x_coll*)tag_get('coll', tags->collision_model);
	h2x_hlmt_collision_regions* collision_regions = h1_runtime_block_new(&model->collision_regions, collision->regions.count);
	for (int32 r = 0; r < collision->regions.count; r++)
	{
		const h2x_coll_regions* region = collision->regions[r];
		collision_regions[r].name = region->name;
		collision_regions[r].collision_region_index = (int8)r;
		collision_regions[r].physics_region_index = tags->physics_model != NONE && r == 0 ? 0 : NONE;
		h2x_hlmt_collision_regions_permutations* permutations = h1_runtime_block_new(&collision_regions[r].permutations, region->permutations.count);
		for (int32 p = 0; p < region->permutations.count; p++)
		{
			permutations[p].name = region->permutations[p]->name;
			permutations[p].collision_permutation_index = (int8)p;
			permutations[p].physics_permutation_index = tags->physics_model != NONE && r == 0 && p == 0 ? 0 : NONE;
		}
	}

	}
nodes:
	if (!h1_model)
	{
		h2x_hlmt_nodes* node = h1_runtime_block_new(&model->nodes, 1);
		node->name = h1_string_id("frame");
		node->parent_node_index = NONE;
		node->first_child_node_index = NONE;
		node->next_sibling_node_index = NONE;
		node->default_rotation = { 0.f, 0.f, 0.f, 1.f };
		real_matrix4x3 identity;
		matrix4x3_identity(&identity);
		csmemcpy(&node->default_inverse_scale, &identity, sizeof(real_matrix4x3));
		return model_index;
	}
	h2x_hlmt_nodes* nodes = h1_runtime_block_new(&model->nodes, h1_model->nodes.count);
	for (int32 i = 0; i < h1_model->nodes.count; i++)
	{
		const h1_mode_nodes* h1_node = g_h1_cache_file->block_get(h1_model->nodes, i);
		nodes[i].name = h1_string_id(h1_node->name);
		nodes[i].parent_node_index = h1_node->parent_node_index;
		nodes[i].first_child_node_index = h1_node->first_child_node_index;
		nodes[i].next_sibling_node_index = h1_node->next_sibling_node_index;
		nodes[i].default_translation = h1_node->default_translation;
		nodes[i].default_rotation = h1_quaternion_to_h2(&h1_node->default_rotation);
		csmemcpy(&nodes[i].default_inverse_scale, &h1_node->inverse_scale, sizeof(real_matrix4x3));
	}
	return model_index;
}

/* private code */

// halo 1 builds rotation matrices from the conjugate of its quaternions (a node's stored inverse matrix is the
// transpose of the matrix halo 2 builds from the same quaternion), halo 2 from the quaternion itself
static real_quaternion h1_quaternion_to_h2(const real_quaternion* rotation)
{
	real_quaternion result;
	result.v.i = -rotation->v.i;
	result.v.j = -rotation->v.j;
	result.v.k = -rotation->v.k;
	result.w = rotation->w;
	return result;
}

static const char* h1_name_last_component(const char* path)
{
	const char* last = strrchr(path, '\\');
	return last ? last + 1 : path;
}
static string_id h1_string_id(const char* string)
{
	return string && *string ? string_id_find_or_add(string) : _string_id_empty_string;
}
string_id h1_global_material_name(int16 global_material_index)
{
	s_game_globals* globals = scenario_get_game_globals();
	if (!globals || !VALID_INDEX(global_material_index, globals->materials.count))
	{
		return _string_id_empty_string;
	}
	const s_global_material_definition* material = (const s_global_material_definition*)tag_block_get_element_with_size(
		&globals->materials, global_material_index, sizeof(s_global_material_definition));
	return material->name;
}
static int16 h1_model_node_find(const h1_mode* h1_model, const char* name)
{
	for (int32 i = 0; i < h1_model->nodes.count; i++)
	{
		if (_stricmp(g_h1_cache_file->block_get(h1_model->nodes, i)->name, name) == 0)
		{
			return (int16)i;
		}
	}
	return NONE;
}

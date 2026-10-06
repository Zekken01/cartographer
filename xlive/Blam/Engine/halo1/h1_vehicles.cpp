#include "stdafx.h"
#include "h1_vehicles.h"

#include "h1_animations.h"
#include "h1_cache_file.h"
#include "h1_log.h"
#include "h1_objects.h"
#include "h1_runtime.h"
#include "h1_structure_bsp.h"
#include "h1_weapons.h"
#include "h2_tag_definitions_generated.h"

#include "game/game_globals.h"
#include "game/materials.h"
#include "math/matrix_math.h"
#include "physics/collision_bsp_definition.h"
#include "tag_files/tag_groups.h"

#include <string>
#include <vector>

/* constants */

enum
{
	k_h2_shape_type_sphere = 0,
	k_h2_shape_type_list = 14,
	k_h2_object_type_vehicle = 1,
	k_h2_rigid_body_motion_type_dynamic = 2,
	k_maximum_list_children = 4,
};

// halo 2 havok shape vtables (halo2.exe), the same in every physics model
constexpr uint32 k_havok_sphere_shape = 0xC3045C;
constexpr uint32 k_havok_convex_translate_shape = 0xC304A0;
constexpr uint32 k_havok_list_shape = 0xC308DC;

// friction point flags
enum
{
	_friction_point_powered_bit = 1,
	_friction_point_front_turning_bit = 2,
	_friction_point_rear_turning_bit = 3,
	_friction_point_attached_to_e_brake_bit = 4,
	_friction_point_bit_5 = 5,		// every halo 2 wheel has it
};

constexpr int32 k_h2_phantom_maximum_spheres = 8;

/* structures */

struct s_h1_vehicle_tags
{
	datum render_model;
	datum collision_model;
	datum physics_model;
	datum animation_graph;
	datum model;
};

/* prototypes */

static const char* h1_name_last_component(const char* path);
static string_id h1_string_id(const char* string);
static string_id h1_marker_string_id(const char* name);
static real_quaternion h1_quaternion_to_h2(const real_quaternion* rotation);
static datum h1_camera_track_build(datum h1_track_index);
static datum h1_render_model_build(const h1_mode* h1_model, const char* name, const h1_phys* h1_physics, const h1_vehi* h1_vehicle);
static datum h1_collision_model_build(const h1_coll* h1_collision, const h1_mode* h1_model, const char* name);
static datum h1_physics_model_build(const h1_phys* h1_physics, const h1_mode* h1_model, const h1_coll* h1_collision, const char* name);
static datum h1_model_build(const s_h1_vehicle_tags* tags, const h1_mode* h1_model, const h1_coll* h1_collision, const char* name);
static void h1_vehicle_fields_build(h2x_vehi* vehicle, const h1_vehi* h1_vehicle, const h1_phys* h1_physics, const h1_mode* h1_model, datum model_index);
static string_id h1_global_material_name(int16 global_material_index);
static int16 h1_model_node_find(const h1_mode* h1_model, const char* name);
static bool h1_mass_point_is_wheel(const h1_phys* h1_physics, const h1_phys_mass_points* mass_point);
static bool h1_mass_point_is_antigrav(const h1_phys* h1_physics, const h1_phys_mass_points* mass_point);
static std::string h1_mass_point_marker_name(const h1_phys_mass_points* mass_point);
static std::string h1_seat_animation_name(const char* h1_label);

/* public code */

datum h1_vehicle_build(datum h1_vehicle_index)
{
	const char* h1_name = g_h1_cache_file->tag_name_get(h1_vehicle_index);
	char name[256];
	sprintf_s(name, "halo1\\%s", h1_name);
	const datum existing = h1_runtime_tag_find('vehi', name);
	if (existing != NONE)
	{
		return existing;
	}

	const h1_vehi* h1_vehicle = (const h1_vehi*)g_h1_cache_file->tag_get('vehi', h1_vehicle_index);
	const h1_mode* h1_model = h1_vehicle ? (const h1_mode*)g_h1_cache_file->tag_get('mode', h1_vehicle->model.index) : NULL;
	const h1_coll* h1_collision = h1_vehicle ? (const h1_coll*)g_h1_cache_file->tag_get('coll', h1_vehicle->collision_model.index) : NULL;
	const h1_phys* h1_physics = h1_vehicle ? (const h1_phys*)g_h1_cache_file->tag_get('phys', h1_vehicle->physics.index) : NULL;
	if (!h1_model || !h1_collision || !h1_physics)
	{
		h1_log("vehicles: %s is missing its model, collision model or physics", h1_name);
		return NONE;
	}

	s_h1_vehicle_tags tags;
	tags.render_model = h1_render_model_build(h1_model, name, h1_physics, h1_vehicle);
	tags.collision_model = h1_collision_model_build(h1_collision, h1_model, name);
	tags.physics_model = h1_physics_model_build(h1_physics, h1_model, h1_collision, name);
	tags.animation_graph = h1_animation_graph_build(h1_vehicle->animation_graph.index, h1_model, name, h1_physics);
	tags.model = h1_model_build(&tags, h1_model, h1_collision, name);
	if (tags.render_model == NONE || tags.collision_model == NONE || tags.physics_model == NONE || tags.model == NONE)
	{
		return NONE;
	}

	h2x_vehi* vehicle = NULL;
	const datum vehicle_index = h1_runtime_tag_new('vehi', name, &vehicle);
	if (vehicle_index == NONE)
	{
		return NONE;
	}
	h1_vehicle_fields_build(vehicle, h1_vehicle, h1_physics, h1_model, tags.model);
	h1_objects_bind(vehicle_index, h1_vehicle_index);

	h1_log("vehicles: built %s (%d nodes, %d seats, %d friction points, %d anti gravity points)",
		name, h1_model->nodes.count, vehicle->seats.count, vehicle->friction_points.count, vehicle->anti_gravity_points.count);
	return vehicle_index;
}

/* private code */

static const char* h1_name_last_component(const char* path)
{
	const char* last = strrchr(path, '\\');
	return last ? last + 1 : path;
}

static string_id h1_string_id(const char* string)
{
	return string && *string ? string_id_find_or_add(string) : _string_id_empty_string;
}

// halo 2's camera track (trak): the halo 1 track's control points, offsets from the camera marker and orientations (halo 2's
// quaternions are halo 1's conjugates)
struct s_h2_camera_track_control_point
{
	real_vector3d position;
	real_quaternion orientation;
};
static_assert(sizeof(s_h2_camera_track_control_point) == 28);

struct s_h2_camera_track
{
	uint32 flags;
	tag_block<s_h2_camera_track_control_point> control_points;
};
static_assert(sizeof(s_h2_camera_track) == 12);

static datum h1_camera_track_build(datum h1_track_index)
{
	if (h1_track_index == NONE)
	{
		return NONE;
	}
	char name[256];
	sprintf_s(name, "halo1\\%s", g_h1_cache_file->tag_name_get(h1_track_index));
	const datum existing = h1_runtime_tag_find('trak', name);
	if (existing != NONE)
	{
		return existing;
	}
	// halo 1's track: flags, then its control points (a position and an orientation, padded to 0x3C)
	const uint8* h1_track = (const uint8*)g_h1_cache_file->tag_get('trak', h1_track_index);
	if (!h1_track)
	{
		return NONE;
	}
	const h1_tag_block<uint8>* h1_points = (const h1_tag_block<uint8>*)(h1_track + 4);
	const uint8* h1_point_data = h1_points->count > 0 ? (const uint8*)g_h1_cache_file->block_get(*h1_points, 0) : NULL;
	if (!h1_point_data)
	{
		return NONE;
	}

	s_h2_camera_track* track = NULL;
	const datum track_index = h1_runtime_tag_new('trak', name, &track);
	if (track_index == NONE)
	{
		return NONE;
	}
	track->flags = *(const uint32*)h1_track;
	s_h2_camera_track_control_point* points = h1_runtime_block_new(&track->control_points, h1_points->count);
	for (int32 i = 0; i < h1_points->count; i++)
	{
		const real32* h1_point = (const real32*)(h1_point_data + i * 0x3C);
		points[i].position = { h1_point[0], h1_point[1], h1_point[2] };
		points[i].orientation = { { -h1_point[3], -h1_point[4], -h1_point[5] }, h1_point[6] };
	}
	return track_index;
}

// halo 1 node and marker rotations build matrices from their conjugate (h1_object_tags)
static real_quaternion h1_quaternion_to_h2(const real_quaternion* rotation)
{
	return { { -rotation->v.i, -rotation->v.j, -rotation->v.k }, rotation->w };
}

// halo 2 looks markers up by its own names, which spell halo 1's spaces as underscores (h1_object_tags, the effects' locations)
static string_id h1_marker_string_id(const char* name)
{
	std::string marker = name ? name : "";
	for (char& c : marker)
	{
		if (c == ' ')
		{
			c = '_';
		}
	}
	return h1_string_id(marker.c_str());
}

// the name of a halo 2 global material (matg materials)
static string_id h1_global_material_name(int16 global_material_index)
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

static bool h1_mass_point_is_antigrav(const h1_phys* h1_physics, const h1_phys_mass_points* mass_point)
{
	const h1_phys_powered_mass_points* powered = g_h1_cache_file->block_get(h1_physics->powered_mass_points, mass_point->powered_mass_point_index);
	return powered && powered->antigrav_strength > 0.f;
}

static bool h1_mass_point_is_wheel(const h1_phys* h1_physics, const h1_phys_mass_points* mass_point)
{
	return mass_point->powered_mass_point_index != NONE && !h1_mass_point_is_antigrav(h1_physics, mass_point);
}

// the halo 2 entry marker of a halo 1 seat
static std::string h1_seat_entry_marker_name(const char* seat_marker_name)
{
	return std::string(seat_marker_name) + " enter";
}

static std::string h1_mass_point_marker_name(const h1_phys_mass_points* mass_point)
{
	char buffer[64];
	return h1_mass_point_marker_name(mass_point->name, buffer);
}

// halo 1 seat labels ("W-driver") are the riders' modes, as the converted biped graphs name them (h1_animations)
static std::string h1_seat_animation_name(const char* h1_label)
{
	std::string label = h1_label;
	for (char& c : label)
	{
		c = (c == '-' || c == ' ') ? '_' : (char)tolower((uint8)c);
	}
	return label;
}

static datum h1_render_model_build(const h1_mode* h1_model, const char* name, const h1_phys* h1_physics, const h1_vehi* h1_vehicle)
{
	h2x_mode* model = NULL;
	const datum model_index = h1_runtime_tag_new('mode', name, &model);
	if (model_index == NONE)
	{
		return NONE;
	}

	model->name = h1_string_id(h1_name_last_component(name));
	model->node_list_checksum = h1_model->node_list_checksum;
	// no geometry: every level of detail and permutation section is NONE
	model->l1_section_group_index_super_low = NONE;
	model->l2_section_group_index_low = NONE;
	model->l3_section_group_index_medium = NONE;
	model->l4_section_group_index_high = NONE;
	model->l5_section_group_index_super_high = NONE;
	model->l6_section_group_index_hollywood = NONE;

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

	// markers: the halo 1 markers, and one at every wheel or hover pad mass point for friction and anti gravity points
	std::vector<const h1_phys_mass_points*> powered_points;
	for (int32 i = 0; i < h1_physics->mass_points.count; i++)
	{
		const h1_phys_mass_points* mass_point = g_h1_cache_file->block_get(h1_physics->mass_points, i);
		if (mass_point->powered_mass_point_index != NONE)
		{
			powered_points.push_back(mass_point);
		}
	}
	h2x_mode_marker_groups* groups = h1_runtime_block_new(&model->marker_groups, h1_model->markers.count + (int32)powered_points.size() + h1_vehicle->seats.count);
	for (int32 i = 0; i < h1_model->markers.count; i++)
	{
		const h1_mode_markers* h1_marker = g_h1_cache_file->block_get(h1_model->markers, i);
		groups[i].name = h1_marker_string_id(h1_marker->name);
		h2x_mode_marker_groups_markers* markers = h1_runtime_block_new(&groups[i].markers, h1_marker->instances.count);
		for (int32 j = 0; j < h1_marker->instances.count; j++)
		{
			const h1_mode_markers_instances* instance = g_h1_cache_file->block_get(h1_marker->instances, j);
			markers[j].region_index = instance->region_index;
			markers[j].permutation_index = instance->permutation_index;
			markers[j].node_index = instance->node_index;
			markers[j].translation = instance->translation;
			markers[j].rotation = instance->rotation;
			markers[j].scale = 1.f;
		}
	}
	for (size_t i = 0; i < powered_points.size(); i++)
	{
		const h1_phys_mass_points* mass_point = powered_points[i];
		h2x_mode_marker_groups* group = &groups[h1_model->markers.count + i];
		group->name = h1_string_id(h1_mass_point_marker_name(mass_point).c_str());
		h2x_mode_marker_groups_markers* marker = h1_runtime_block_new(&group->markers, 1);
		const int16 node_index = VALID_INDEX(mass_point->model_node, h1_model->nodes.count) ? mass_point->model_node : 0;
		const h1_mode_nodes* node = g_h1_cache_file->block_get(h1_model->nodes, node_index);
		marker->region_index = NONE;
		marker->permutation_index = NONE;
		marker->node_index = (int8)node_index;
		// mass points are in model space, markers in their node's space
		matrix4x3_transform_point((const real_matrix4x3*)&node->inverse_scale, &mass_point->position, &marker->translation);
		marker->rotation = { 0.f, 0.f, 0.f, 1.f };
		marker->scale = 1.f;
	}

	// unit_find_nearby_seat: a seat is entered within a world unit of where the player's biped starts its enter animation, or of the
	// seat; each seat's entry markers ("<seat marker> enter") are those two points
	const datum h1_globals_index = g_h1_cache_file->tag_find('matg', "globals\\globals");
	const h1_matg* h1_globals = h1_globals_index != NONE ? (const h1_matg*)g_h1_cache_file->tag_get('matg', h1_globals_index) : NULL;
	const h1_matg_player_information* player_information = h1_globals ? g_h1_cache_file->block_get(h1_globals->player_information, 0) : NULL;
	const h1_scen* rider = player_information && player_information->unit.index != NONE ? (const h1_scen*)g_h1_cache_file->tag_get('obje', player_information->unit.index) : NULL;
	for (int32 i = 0; i < h1_vehicle->seats.count; i++)
	{
		const h1_vehi_seats* seat = g_h1_cache_file->block_get(h1_vehicle->seats, i);
		h2x_mode_marker_groups* group = &groups[h1_model->markers.count + (int32)powered_points.size() + i];
		group->name = h1_marker_string_id(h1_seat_entry_marker_name(seat->marker_name).c_str());

		const h1_mode_markers_instances* seat_marker = NULL;
		for (int32 m = 0; m < h1_model->markers.count && !seat_marker; m++)
		{
			const h1_mode_markers* h1_marker = g_h1_cache_file->block_get(h1_model->markers, m);
			if (_stricmp(h1_marker->name, seat->marker_name) == 0 && h1_marker->instances.count > 0)
			{
				seat_marker = g_h1_cache_file->block_get(h1_marker->instances, 0);
			}
		}
		if (!seat_marker)
		{
			continue;
		}
		real_point3d entrance;
		const bool has_entrance = rider && h1_animation_seat_enter_root_get(rider->animation_graph.index, seat->label, &entrance);
		h2x_mode_marker_groups_markers* markers = h1_runtime_block_new(&group->markers, has_entrance ? 2 : 1);
		for (int32 m = 0; m < (has_entrance ? 2 : 1); m++)
		{
			markers[m].region_index = NONE;
			markers[m].permutation_index = NONE;
			markers[m].node_index = seat_marker->node_index;
			markers[m].translation = seat_marker->translation;
			markers[m].rotation = seat_marker->rotation;
			markers[m].scale = 1.f;
		}
		if (has_entrance)
		{
			// the entrance in the seat marker's space, the marker in its node's
			real_matrix4x3 rotation;
			matrix4x3_rotation_from_quaternion(&rotation, &seat_marker->rotation);
			real_vector3d offset;
			matrix4x3_transform_vector(&rotation, (const real_vector3d*)&entrance, &offset);
			markers[1].translation = { seat_marker->translation.x + offset.i, seat_marker->translation.y + offset.j, seat_marker->translation.z + offset.k };
		}
	}
	return model_index;
}

static datum h1_collision_model_build(const h1_coll* h1_collision, const h1_mode* h1_model, const char* name)
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

static datum h1_physics_model_build(const h1_phys* h1_physics, const h1_mode* h1_model, const h1_coll* h1_collision, const char* name)
{
	h2x_phmo* physics = NULL;
	const datum physics_index = h1_runtime_tag_new('phmo', name, &physics);
	if (physics_index == NONE)
	{
		return NONE;
	}

	physics->mass = h1_physics->mass;
	physics->low_frequency_deactivation_scale = 1.f;
	physics->high_frequency_deactivation_scale = 1.f;

	// the hull material is the collision model's first material
	const h1_coll_materials* h1_material = g_h1_cache_file->block_get(h1_collision->materials, 0);
	h2x_phmo_materials* material = h1_runtime_block_new(&physics->materials, 1);
	material->name = h1_string_id(h1_material ? h1_material->name : "hull");
	// havok resolves the material through its global material name
	material->global_material_name = h1_global_material_name(h1_material_type_to_global_material(h1_material ? h1_material->material_type : 0));
	material->phantom_type_index = NONE;

	// the hull: every mass point that isn't a wheel or hover pad (those are friction and anti gravity points)
	std::vector<const h1_phys_mass_points*> hull_points;
	for (int32 i = 0; i < h1_physics->mass_points.count; i++)
	{
		const h1_phys_mass_points* mass_point = g_h1_cache_file->block_get(h1_physics->mass_points, i);
		if (!h1_mass_point_is_wheel(h1_physics, mass_point))
		{
			hull_points.push_back(mass_point);
		}
	}
	if (hull_points.empty())
	{
		for (int32 i = 0; i < h1_physics->mass_points.count; i++)
		{
			hull_points.push_back(g_h1_cache_file->block_get(h1_physics->mass_points, i));
		}
	}
	const int32 sphere_count = MIN((int32)hull_points.size(), k_maximum_list_children * k_maximum_list_children);

	// halo 2 puts the rigid body at the object's origin: shapes and the center of mass are in model space (the root node is offset from it)
	const real_matrix4x3* model_to_root = global_identity4x3;

	h2x_phmo_spheres* spheres = h1_runtime_block_new(&physics->spheres, sphere_count);
	real_rectangle3d bounds = { FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX };
	for (int32 i = 0; i < sphere_count; i++)
	{
		const h1_phys_mass_points* mass_point = hull_points[i];
		const real32 radius = MAX(mass_point->radius, 0.05f);
		h2x_phmo_spheres* sphere = &spheres[i];
		sphere->name = h1_string_id(mass_point->name);
		sphere->material_index = 0;
		sphere->flags = 72;
		sphere->relative_mass_scale = 1.f;
		sphere->friction = 0.8f;
		sphere->restitution = 0.5f;
		sphere->volume = 4.f / 3.f * _pi * radius * radius * radius;
		sphere->mass = mass_point->mass;
		sphere->mass_distribution_index = NONE;
		sphere->phantom_type_index = NONE;
		sphere->runtime_code_pointer = k_havok_sphere_shape;
		sphere->count = 128;
		sphere->radius = radius;
		sphere->runtime_code_pointer_2 = k_havok_convex_translate_shape;
		sphere->count_2 = 128;
		sphere->rotation_i = { 1.f, 0.f, 0.f };
		sphere->rotation_j = { 0.f, 1.f, 0.f };
		sphere->rotation_k = { 0.f, 0.f, 1.f };
		real_point3d position;
		matrix4x3_transform_point(model_to_root, &mass_point->position, &position);
		sphere->translation = { position.x, position.y, position.z };

		bounds.x0 = MIN(bounds.x0, position.x - radius); bounds.x1 = MAX(bounds.x1, position.x + radius);
		bounds.y0 = MIN(bounds.y0, position.y - radius); bounds.y1 = MAX(bounds.y1, position.y + radius);
		bounds.z0 = MIN(bounds.z0, position.z - radius); bounds.z1 = MAX(bounds.z1, position.z + radius);
	}

	// spheres go into lists of at most four, and those into a list when there are more than four
	int16 shape_type = k_h2_shape_type_sphere;
	int16 shape_index = 0;
	if (sphere_count > 1)
	{
		const int32 leaf_count = (sphere_count + k_maximum_list_children - 1) / k_maximum_list_children;
		const int32 list_count = leaf_count > 1 ? leaf_count + 1 : 1;
		h2x_phmo_lists* lists = h1_runtime_block_new(&physics->lists, list_count);
		auto list_set = [](h2x_phmo_lists* list, int32 child_count)
		{
			list->runtime_code_pointer = k_havok_list_shape;
			list->count = 128;
			list->child_shapes_size = child_count;
			list->child_shapes_capacity = 0x80000000 | k_maximum_list_children;
			int16* children = &list->shape_type_0;
			for (int32 c = 0; c < k_maximum_list_children; c++)
			{
				children[c * 4 + 0] = 0;
				children[c * 4 + 1] = c < child_count ? 0 : NONE;
			}
		};
		for (int32 l = 0; l < leaf_count; l++)
		{
			const int32 first = l * k_maximum_list_children;
			const int32 count = MIN(k_maximum_list_children, sphere_count - first);
			list_set(&lists[l], count);
			int16* children = &lists[l].shape_type_0;
			for (int32 c = 0; c < count; c++)
			{
				children[c * 4 + 0] = k_h2_shape_type_sphere;
				children[c * 4 + 1] = (int16)(first + c);
			}
		}
		if (leaf_count > 1)
		{
			h2x_phmo_lists* root = &lists[leaf_count];
			list_set(root, leaf_count);
			int16* children = &root->shape_type_0;
			for (int32 c = 0; c < leaf_count; c++)
			{
				children[c * 4 + 0] = k_h2_shape_type_list;
				children[c * 4 + 1] = (int16)c;
			}
		}
		shape_type = k_h2_shape_type_list;
		shape_index = (int16)(list_count - 1);
	}

	h2x_phmo_rigid_bodies* body = h1_runtime_block_new(&physics->rigid_bodies, 1);
	body->node_index = 0;
	body->region_index = 0;
	body->permutation_index = 0;
	body->bounding_sphere_offset = { (bounds.x0 + bounds.x1) * 0.5f, (bounds.y0 + bounds.y1) * 0.5f, (bounds.z0 + bounds.z1) * 0.5f };
	const real32 dx = (bounds.x1 - bounds.x0) * 0.5f, dy = (bounds.y1 - bounds.y0) * 0.5f, dz = (bounds.z1 - bounds.z0) * 0.5f;
	body->bounding_sphere_radius = sqrtf(dx * dx + dy * dy + dz * dz);
	body->motion_type = k_h2_rigid_body_motion_type_dynamic;
	body->no_phantom_power_alternative_rigid_body_index = NONE;
	body->size = 4;
	body->inertia_tensor_scale = 1.f;
	body->angular_damping = 0.05f;
	body->shape_type = shape_type;
	body->shape_index = shape_index;
	body->mass = h1_physics->mass;
	real_point3d center_of_mass;
	matrix4x3_transform_point(model_to_root, &h1_physics->center_of_mass, &center_of_mass);
	body->center_of_mass = { center_of_mass.x, center_of_mass.y, center_of_mass.z };
	body->inertia_tensor_x = { h1_physics->xx_moment, 0.f, 0.f };
	body->inertia_tensor_y = { 0.f, h1_physics->yy_moment, 0.f };
	body->inertia_tensor_z = { 0.f, 0.f, h1_physics->zz_moment };
	body->collision_quality_override_type = NONE;

	// one region and permutation holding the rigid body
	const h1_mode_regions* h1_region = g_h1_cache_file->block_get(h1_model->regions, 0);
	h2x_phmo_regions* region = h1_runtime_block_new(&physics->regions, 1);
	region->name = h1_string_id(h1_region ? h1_region->name : "hull");
	h2x_phmo_regions_permutations* permutation = h1_runtime_block_new(&region->permutations, 1);
	permutation->name = _string_id_default;
	h1_runtime_block_new(&permutation->rigid_bodies, 1)->rigid_body_index = 0;

	h2x_phmo_nodes* nodes = h1_runtime_block_new(&physics->nodes, h1_model->nodes.count);
	for (int32 i = 0; i < h1_model->nodes.count; i++)
	{
		const h1_mode_nodes* h1_node = g_h1_cache_file->block_get(h1_model->nodes, i);
		nodes[i].name = h1_string_id(h1_node->name);
		nodes[i].parent_index = h1_node->parent_node_index;
		nodes[i].sibling_index = h1_node->next_sibling_node_index;
		nodes[i].child_index = h1_node->first_child_node_index;
	}
	return physics_index;
}

static datum h1_model_build(const s_h1_vehicle_tags* tags, const h1_mode* h1_model, const h1_coll* h1_collision, const char* name)
{
	h2x_hlmt* model = NULL;
	const datum model_index = h1_runtime_tag_new('hlmt', name, &model);
	if (model_index == NONE)
	{
		return NONE;
	}

	h1_runtime_reference_set(&model->render_model, 'mode', tags->render_model);
	h1_runtime_reference_set(&model->collision_model, 'coll', tags->collision_model);
	h1_runtime_reference_set(&model->animation, 'jmad', tags->animation_graph);
	h1_runtime_reference_set(&model->physics, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&model->physics_model, 'phmo', tags->physics_model);
	h1_runtime_reference_set(&model->default_dialogue, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&model->active_camo_shader, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&model->hologram_shader, (tag_group)NONE, NONE);
	model->disappear_distance = 165.f;
	model->begin_fade_distance = 155.f;
	model->node_list_checksum = h1_model->node_list_checksum;

	// one variant using every region's first permutation
	const int32 region_count = MIN(h1_model->regions.count, 16);
	h2x_hlmt_variants* variant = h1_runtime_block_new(&model->variants, 1);
	variant->name = _string_id_default;
	// no halo 2 dialogue (a zeroed reference crashes the units' speech)
	variant->dialogue_sound_effect = _string_id_empty_string;
	h1_runtime_reference_set(&variant->dialogue, (tag_group)NONE, NONE);
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
	// halo 1's stun and recharge are the shield's: halo 2's second set, with its recharge velocity (fraction per second)
	damage->maximum_shield_vitality = h1_collision->maximum_shield_vitality;
	damage->minimum_stun_damage_2 = h1_collision->minimum_stun_damage;
	damage->stun_time_2 = h1_collision->stun_time;
	damage->recharge_time_2 = h1_collision->recharge_time;
	damage->shield_recharge_velocity = h1_collision->recharge_time > 0.f ? 1.f / h1_collision->recharge_time : 0.f;
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
		collision_regions[r].physics_region_index = r == 0 ? 0 : NONE;
		h2x_hlmt_collision_regions_permutations* permutations = h1_runtime_block_new(&collision_regions[r].permutations, region->permutations.count);
		for (int32 p = 0; p < region->permutations.count; p++)
		{
			permutations[p].name = region->permutations[p]->name;
			permutations[p].collision_permutation_index = (int8)p;
			permutations[p].physics_permutation_index = r == 0 && p == 0 ? 0 : NONE;
		}
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

static void h1_vehicle_fields_build(h2x_vehi* vehicle, const h1_vehi* h1_vehicle, const h1_phys* h1_physics, const h1_mode* h1_model, datum model_index)
{
	// object
	vehicle->object_type = k_h2_object_type_vehicle;
	vehicle->bounding_radius = h1_vehicle->bounding_radius;
	vehicle->bounding_offset = h1_vehicle->bounding_offset;
	vehicle->acceleration_scale = h1_vehicle->acceleration_scale;
	vehicle->default_model_variant = _string_id_default;
	h1_runtime_reference_set(&vehicle->model, 'hlmt', model_index);
	h1_runtime_reference_set(&vehicle->crate_object, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&vehicle->modifier_shader, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&vehicle->creation_effect, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&vehicle->material_effects, (tag_group)NONE, NONE);
	vehicle->apply_collision_damage_scale = 1.f;
	vehicle->game_acceleration = { 2.5f, 4.5f };
	vehicle->game_scale = { 0.2f, 1.25f };
	vehicle->absolute_acceleration = { 2.5f, 10.f };
	vehicle->absolute_scale = { 0.2f, 1.25f };
	vehicle->hud_text_message_index = h1_vehicle->hud_text_message_index;

	// unit
	vehicle->default_team = h1_vehicle->default_team;
	vehicle->constant_sound_volume = h1_vehicle->constant_sound_volume;
	h1_runtime_reference_set(&vehicle->integrated_light_toggle, (tag_group)NONE, NONE);
	vehicle->camera_field_of_view = h1_vehicle->camera_field_of_view;
	vehicle->camera_stiffness = h1_vehicle->camera_stiffness;
	vehicle->camera_marker_name = h1_marker_string_id(h1_vehicle->camera_marker_name);
	vehicle->camera_submerged_marker_name = h1_marker_string_id(h1_vehicle->camera_submerged_marker_name);
	vehicle->pitch_auto_level = h1_vehicle->pitch_auto_level;
	vehicle->pitch_range = h1_vehicle->pitch_range;
	// halo 1 scales the unit's acceleration (its acceleration overlays: the warthog's turret sways) per tick squared, halo 2 per
	// second squared
	vehicle->acceleration_range =
	{
		h1_vehicle->seat_acceleration_scale.i / (30.f * 30.f),
		h1_vehicle->seat_acceleration_scale.j / (30.f * 30.f),
		h1_vehicle->seat_acceleration_scale.k / (30.f * 30.f),
	};
	vehicle->soft_ping_threshold = h1_vehicle->soft_ping_threshold;
	vehicle->soft_ping_interrupt_time = h1_vehicle->soft_ping_interrupt_time;
	vehicle->hard_ping_threshold = h1_vehicle->hard_ping_threshold;
	vehicle->hard_ping_interrupt_time = h1_vehicle->hard_ping_interrupt_time;
	vehicle->hard_death_threshold = h1_vehicle->hard_death_threshold;
	vehicle->feign_death_threshold = h1_vehicle->feign_death_threshold;
	vehicle->feign_death_time = h1_vehicle->feign_death_time;
	vehicle->distance_of_evade_animation = h1_vehicle->distance_of_evade_animation;
	vehicle->distance_of_dive_animation = h1_vehicle->distance_of_dive_animation;
	vehicle->stunned_movement_threshold = h1_vehicle->stunned_movement_threshold;
	vehicle->feign_death_chance = h1_vehicle->feign_death_chance;
	vehicle->feign_repeat_chance = h1_vehicle->feign_repeat_chance;
	h1_runtime_reference_set(&vehicle->spawned_turret_character, (tag_group)NONE, NONE);
	vehicle->aiming_velocity_maximum = h1_vehicle->aiming_velocity_maximum;
	vehicle->aiming_acceleration_maximum = h1_vehicle->aiming_acceleration_maximum;
	vehicle->casual_aiming_modifier = h1_vehicle->casual_aiming_modifier;
	vehicle->looking_velocity_maximum = h1_vehicle->looking_velocity_maximum;
	vehicle->looking_acceleration_maximum = h1_vehicle->looking_acceleration_maximum;
	h1_runtime_reference_set(&vehicle->melee_damage, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&vehicle->boarding_melee_damage, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&vehicle->boarding_melee_response, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&vehicle->landing_melee_damage, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&vehicle->flurry_melee_damage, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&vehicle->obstacle_smash_damage, (tag_group)NONE, NONE);
	vehicle->motion_sensor_blip_size = h1_vehicle->motion_sensor_blip_size;
	vehicle->grenade_velocity = h1_vehicle->grenade_velocity;

	h2x_vehi_powered_seats* powered_seats = h1_runtime_block_new(&vehicle->powered_seats, MIN(h1_vehicle->powered_seats.count, 2));
	for (int32 i = 0; i < vehicle->powered_seats.count; i++)
	{
		const h1_vehi_powered_seats* h1_powered = g_h1_cache_file->block_get(h1_vehicle->powered_seats, i);
		powered_seats[i].driver_powerup_time = h1_powered->driver_powerup_time;
		powered_seats[i].driver_powerdown_time = h1_powered->driver_powerdown_time;
	}

	// halo 2's seats carry what halo 1's don't (the enter prompt, the seat camera tracks, the seat hud): the host's vehicle of the
	// same kind, its seat in the same role
	static const char* const k_host_vehicles[] =
	{
		"objects\\vehicles\\scorpion\\scorpion",		// human tank
		"objects\\vehicles\\warthog\\warthog",			// human jeep
		"objects\\vehicles\\warthog\\warthog",			// human boat
		"objects\\vehicles\\banshee\\banshee",			// human plane
		"objects\\vehicles\\ghost\\ghost",				// alien scout
		"objects\\vehicles\\banshee\\banshee",			// alien fighter
		"objects\\vehicles\\c_turret_ap\\c_turret_ap",	// turret
	};
	const datum host_vehicle_index = tag_loaded('vehi', k_host_vehicles[VALID_INDEX(h1_vehicle->type, NUMBEROF(k_host_vehicles)) ? h1_vehicle->type : 1]);
	const h2x_vehi* host_vehicle = host_vehicle_index != NONE ? (const h2x_vehi*)tag_get('vehi', host_vehicle_index) : NULL;
	auto host_seat_get = [host_vehicle](uint32 flags) -> const h2x_vehi_seats*
	{
		// driver (bit 2), gunner (bit 3), or a seat that's neither
		const uint32 role = flags & (FLAG(2) | FLAG(3));
		for (int32 i = 0; host_vehicle && i < host_vehicle->seats.count; i++)
		{
			const h2x_vehi_seats* seat = host_vehicle->seats[i];
			if ((role == 0 && (seat->flags & (FLAG(2) | FLAG(3))) == 0) || (role != 0 && (seat->flags & role) != 0))
			{
				return seat;
			}
		}
		return host_vehicle && host_vehicle->seats.count > 0 ? host_vehicle->seats[0] : NULL;
	};

	// the unit's weapons (the ghost's guns): halo 1 weapons the vehicle holds, its gunning seat fires them
	std::vector<datum> weapons;
	for (int32 i = 0; i < h1_vehicle->weapons.count; i++)
	{
		const h1_vehi_weapons* h1_weapon = g_h1_cache_file->block_get(h1_vehicle->weapons, i);
		const datum weapon_index = h1_weapon->weapon.index != NONE ? h1_weapon_definition_build(h1_weapon->weapon.index) : NONE;
		if (weapon_index != NONE)
		{
			weapons.push_back(weapon_index);
		}
	}
	h2x_vehi_weapons* vehicle_weapons = h1_runtime_block_new(&vehicle->weapons, (int32)weapons.size());
	for (size_t i = 0; i < weapons.size(); i++)
	{
		h1_runtime_reference_set(&vehicle_weapons[i].weapon, 'weap', weapons[i]);
	}

	h2x_vehi_seats* seats = h1_runtime_block_new(&vehicle->seats, h1_vehicle->seats.count);
	for (int32 i = 0; i < h1_vehicle->seats.count; i++)
	{
		const h1_vehi_seats* h1_seat = g_h1_cache_file->block_get(h1_vehicle->seats, i);
		h2x_vehi_seats* seat = &seats[i];
		// the first 11 seat flags are the same in both games
		seat->flags = h1_seat->flags & 0x7FF;
		seat->seat_animation = h1_string_id(h1_seat_animation_name(h1_seat->label).c_str());
		seat->seat_marker_name = h1_marker_string_id(h1_seat->marker_name);
		seat->entry_marker_s_name = h1_marker_string_id(h1_seat_entry_marker_name(h1_seat->marker_name).c_str());
		seat->ping_scale = 1.f;
		seat->turnover_time = 0.65f;
		// halo 1 scales the seat's acceleration per tick squared, halo 2 per second squared
		constexpr real32 k_ticks_per_second_squared = 30.f * 30.f;
		seat->acceleration_range =
		{
			h1_seat->acceleration_scale.i / k_ticks_per_second_squared,
			h1_seat->acceleration_scale.j / k_ticks_per_second_squared,
			h1_seat->acceleration_scale.k / k_ticks_per_second_squared,
		};
		seat->acceleration_action_scale = 1.f;
		seat->boarding_seat_index = NONE;
		seat->listener_interpolation_factor = 0.6f;
		seat->yaw_rate_bounds = { h1_seat->yaw_rate, h1_seat->yaw_rate };
		seat->pitch_rate_bounds = { h1_seat->pitch_rate, h1_seat->pitch_rate };
		seat->camera_marker_name = h1_marker_string_id(h1_seat->camera_marker_name);
		seat->camera_submerged_marker_name = h1_marker_string_id(h1_seat->camera_submerged_marker_name);
		seat->pitch_auto_level = h1_seat->pitch_auto_level;
		seat->pitch_range = h1_seat->pitch_range;
		seat->yaw = h1_seat->yaw;
		h1_runtime_reference_set(&seat->built_in_gunner, (tag_group)NONE, NONE);
		// halo 1 enters within a world unit, facing any way
		seat->entry_radius = 1.f;
		seat->entry_marker_cone_angle = _pi;
		seat->entry_marker_facing_angle = _pi;
		seat->maximum_relative_velocity = 3.f;
		seat->invisible_seat_region_index = NONE;

		// the seat's own camera tracks, the host's without them
		std::vector<datum> tracks;
		for (int32 t = 0; t < h1_seat->camera_tracks.count; t++)
		{
			const datum track = h1_camera_track_build(g_h1_cache_file->block_get(h1_seat->camera_tracks, t)->track.index);
			if (track != NONE)
			{
				tracks.push_back(track);
			}
		}

		const h2x_vehi_seats* host_seat = host_seat_get(h1_seat->flags);
		if (host_seat)
		{
			seat->enter_seat_string = host_seat->enter_seat_string;
			if (tracks.empty())
			{
				seat->camera_tracks = host_seat->camera_tracks;
			}
			seat->unit_hud_interface = host_seat->unit_hud_interface;
			seat->ai_scariness = host_seat->ai_scariness;
			seat->ai_seat_type = host_seat->ai_seat_type;
		}
		if (!tracks.empty())
		{
			h2x_vehi_seats_camera_tracks* seat_tracks = h1_runtime_block_new(&seat->camera_tracks, (int32)tracks.size());
			for (size_t t = 0; t < tracks.size(); t++)
			{
				h1_runtime_reference_set(&seat_tracks[t].track, 'trak', tracks[t]);
			}
		}
	}

	// vehicle
	vehicle->type = h1_vehicle->type;
	vehicle->maximum_forward_speed = h1_vehicle->maximum_forward_speed;
	vehicle->maximum_reverse_speed = h1_vehicle->maximum_reverse_speed;
	vehicle->speed_acceleration = h1_vehicle->speed_acceleration;
	vehicle->speed_deceleration = h1_vehicle->speed_deceleration;
	vehicle->maximum_left_turn = h1_vehicle->maximum_left_turn;
	vehicle->maximum_right_turn_negative = h1_vehicle->maximum_right_turn_negative;
	vehicle->wheel_circumference = h1_vehicle->wheel_circumference;
	vehicle->turn_rate = h1_vehicle->turn_rate;
	vehicle->blur_speed = h1_vehicle->blur_speed;
	vehicle->maximum_left_slide = h1_vehicle->maximum_left_slide;
	vehicle->maximum_right_slide = h1_vehicle->maximum_right_slide;
	vehicle->slide_acceleration = h1_vehicle->slide_acceleration;
	vehicle->slide_deceleration = h1_vehicle->slide_deceleration;
	vehicle->minimum_flipping_angular_velocity = h1_vehicle->minimum_flipping_angular_velocity;
	vehicle->maximum_flipping_angular_velocity = h1_vehicle->maximum_flipping_angular_velocity;
	vehicle->fixed_gun_yaw = h1_vehicle->fixed_gun_yaw;
	vehicle->fixed_gun_pitch = h1_vehicle->fixed_gun_pitch;
	vehicle->overdampen_cusp_angle = 5.f;
	vehicle->overdampen_exponent = 1.2f;
	h1_runtime_reference_set(&vehicle->suspension_sound, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&vehicle->crash_sound, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&vehicle->unknown_6, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&vehicle->special_effect, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&vehicle->unknown_effect, (tag_group)NONE, NONE);

	// engine: a single forward gear whose torque gives the halo 1 acceleration at the wheels
	const real32 wheel_radius = h1_vehicle->wheel_circumference > 0.f ? h1_vehicle->wheel_circumference / (2.f * _pi) : 0.4f;
	const real32 drive_torque = h1_physics->mass * MAX(h1_vehicle->speed_acceleration, 0.5f) * wheel_radius * 10.f;
	vehicle->engine_moment = 3000.f;
	vehicle->engine_maximum_angular_velocity = MAX(h1_vehicle->maximum_forward_speed, 1.f);
	h2x_vehi_gears* gears = h1_runtime_block_new(&vehicle->gears, 2);
	const real32 gear_ratios[] = { -0.7f, 1.f };
	for (int32 g = 0; g < 2; g++)
	{
		gears[g].maximum_torque = -drive_torque;
		gears[g].peak_torque_scale = 0.4f;
		gears[g].past_peak_torque_exponent = 1.25f;
		gears[g].torque_at_maximum_angular_velocity = -drive_torque * 1.1f;
		gears[g].torque_at_2x_maximum_angular_velocity = -drive_torque * 2.f;
		gears[g].engine_up_shift_scale = 1.f;
		gears[g].gear_ratio = gear_ratios[g];
		gears[g].minimum_time_to_down_shift = 0.2f;
		gears[g].engine_down_shift_scale = 0.5f;
	}
	vehicle->flying_torque_scale = 1.f;
	vehicle->seat_entrance_acceleration_scale = 3.f;
	vehicle->seat_exit_acceleration_scale = 4.f;
	vehicle->thrust_scale = 1.f;

	// ground contact of the halo 1 physics
	vehicle->ground_friction = 0.85f;
	vehicle->ground_depth = h1_physics->ground_depth;
	vehicle->ground_damp_factor = 1.5f;
	vehicle->ground_moving_friction = 0.014f;
	vehicle->ground_maximum_slope_0 = h1_physics->ground_normal_k1;
	vehicle->ground_maximum_slope_1 = h1_physics->ground_normal_k0;
	vehicle->gravity_scale = 1.f;

	// wheels and hover pads at their mass point markers
	int32 wheel_count = 0, antigrav_count = 0;
	real32 largest_radius = 0.f;
	for (int32 i = 0; i < h1_physics->mass_points.count; i++)
	{
		const h1_phys_mass_points* mass_point = g_h1_cache_file->block_get(h1_physics->mass_points, i);
		wheel_count += h1_mass_point_is_wheel(h1_physics, mass_point) ? 1 : 0;
		antigrav_count += h1_mass_point_is_antigrav(h1_physics, mass_point) ? 1 : 0;
		largest_radius = MAX(largest_radius, mass_point->radius);
	}
	vehicle->radius = largest_radius;

	h2x_vehi_friction_points* friction_points = h1_runtime_block_new(&vehicle->friction_points, wheel_count);
	h2x_vehi_anti_gravity_points* antigrav_points = h1_runtime_block_new(&vehicle->anti_gravity_points, antigrav_count);
	int32 wheel_index = 0, antigrav_index = 0;
	for (int32 i = 0; i < h1_physics->mass_points.count; i++)
	{
		const h1_phys_mass_points* mass_point = g_h1_cache_file->block_get(h1_physics->mass_points, i);
		const h1_phys_powered_mass_points* powered = g_h1_cache_file->block_get(h1_physics->powered_mass_points, mass_point->powered_mass_point_index);
		const string_id marker = h1_string_id(h1_mass_point_marker_name(mass_point).c_str());
		if (h1_mass_point_is_wheel(h1_physics, mass_point))
		{
			h2x_vehi_friction_points* point = &friction_points[wheel_index++];
			point->marker_name = marker;
			// the "front" powered mass point steers
			point->flags = FLAG(_friction_point_powered_bit) | FLAG(_friction_point_attached_to_e_brake_bit) | FLAG(_friction_point_bit_5);
			if (powered && _stricmp(powered->name, "front") == 0)
			{
				point->flags = FLAG(_friction_point_powered_bit) | FLAG(_friction_point_front_turning_bit) | FLAG(_friction_point_bit_5);
			}
			point->fraction_of_total_mass = 100.f * mass_point->mass / MAX(h1_physics->mass, 1.f);
			point->radius = mass_point->radius;
			point->damaged_radius = mass_point->radius * 0.4f;
			point->friction_type = 1;
			point->moving_friction_velocity_differential = 7.f;
			point->collision_global_material_name = _string_id_empty_string;
			point->collision_global_material_index = NONE;
			point->model_state_destroyed = 4;
			point->region_name = _string_id_empty_string;
			point->region_index = NONE;
		}
		else if (h1_mass_point_is_antigrav(h1_physics, mass_point))
		{
			h2x_vehi_anti_gravity_points* point = &antigrav_points[antigrav_index++];
			point->marker_name = marker;
			point->antigrav_strength = powered->antigrav_strength;
			point->antigrav_offset = powered->antigrav_offset;
			point->antigrav_height = powered->antigrav_height;
			// halo 1's damping is a fraction per tick, halo 2's vehicles all damp at 1
			point->antigrav_damp_factor = 1.f;
			point->antigrav_normal_k1 = powered->antigrav_normal_k1;
			point->antigrav_normal_k0 = powered->antigrav_normal_k0;
			point->radius = mass_point->radius;
			point->damage_source_region_index = NONE;
			point->damage_source_region_name = _string_id_empty_string;
		}
	}

	// halo 2's wheels touch the ground through a havok multi sphere phantom at the wheels (vehicle space, as the mass points):
	// the host's warthog phantom with the halo 1 wheels' spheres
	const datum host_warthog_index = tag_loaded('vehi', "objects\\vehicles\\warthog\\warthog");
	const h2x_vehi* host_warthog = host_warthog_index != NONE ? (const h2x_vehi*)tag_get('vehi', host_warthog_index) : NULL;
	if (wheel_count > 0 && host_warthog && host_warthog->phantom_shapes.count > 0)
	{
		h2x_vehi_phantom_shapes* phantom = h1_runtime_block_new(&vehicle->phantom_shapes, 1);
		*phantom = *host_warthog->phantom_shapes[0];
		real_vector3d* sphere_centers = &phantom->sphere_0;
		uint32 sphere_count = 0;
		real_rectangle3d bounds = { FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX };
		for (int32 i = 0; i < h1_physics->mass_points.count && sphere_count < k_h2_phantom_maximum_spheres; i++)
		{
			const h1_phys_mass_points* mass_point = g_h1_cache_file->block_get(h1_physics->mass_points, i);
			if (!h1_mass_point_is_wheel(h1_physics, mass_point))
			{
				continue;
			}
			// each sphere is a center and a radius (w)
			real_vector3d* center = (real_vector3d*)((uint8*)sphere_centers + sphere_count * 16);
			*center = { mass_point->position.x, mass_point->position.y, mass_point->position.z };
			*(real32*)(center + 1) = mass_point->radius;
			bounds.x0 = MIN(bounds.x0, mass_point->position.x - mass_point->radius); bounds.x1 = MAX(bounds.x1, mass_point->position.x + mass_point->radius);
			bounds.y0 = MIN(bounds.y0, mass_point->position.y - mass_point->radius); bounds.y1 = MAX(bounds.y1, mass_point->position.y + mass_point->radius);
			bounds.z0 = MIN(bounds.z0, mass_point->position.z - mass_point->radius); bounds.z1 = MAX(bounds.z1, mass_point->position.z + mass_point->radius);
			sphere_count++;
		}
		for (uint32 i = sphere_count; i < k_h2_phantom_maximum_spheres; i++)
		{
			csmemset((uint8*)sphere_centers + i * 16, 0, 16);
		}
		phantom->number_of_spheres = sphere_count;
		phantom->x0 = bounds.x0; phantom->x1 = bounds.x1;
		phantom->y0 = bounds.y0; phantom->y1 = bounds.y1;
		phantom->z0 = bounds.z0; phantom->z1 = bounds.z1;
	}

	// halo 1's physics moves the vehicle (h1_vehicle_physics): halo 2's vehicle forces get nothing to push
	vehicle->friction_points.count = 0;
	vehicle->anti_gravity_points.count = 0;
	vehicle->phantom_shapes.count = 0;
	return;
}

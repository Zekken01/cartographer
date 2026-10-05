#include "stdafx.h"
#include "h1_equipment.h"

#include "h1_cache_file.h"
#include "h1_items.h"
#include "h1_log.h"
#include "h1_objects.h"
#include "h1_runtime.h"
#include "h2_tag_definitions_generated.h"

#include "scenario/scenario_definitions.h"

#include <unordered_map>

/* constants */

enum e_h2_netgame_item_classification
{
	_h2_classification_weapon = 0,
	_h2_classification_primary_light_land,
	_h2_classification_secondary_light_land,
	_h2_classification_primary_heavy_land,
	_h2_classification_primary_flying,
	_h2_classification_secondary_heavy_land,
	_h2_classification_primary_turret,
	_h2_classification_secondary_turret,
	_h2_classification_grenade,
	_h2_classification_powerup,
};

struct s_h1_object_substitute
{
	const char* h1_name_part;	// matched against the halo 1 tag name
	uint32 h2_group;
	const char* h2_name;
	int8 classification;
};

// first match wins, so more specific names come first
static const s_h1_object_substitute k_h1_object_substitutes[] =
{
	{ "assault rifle",		'weap', "objects\\weapons\\rifle\\smg\\smg",									_h2_classification_weapon },
	{ "plasma pistol",		'weap', "objects\\weapons\\pistol\\plasma_pistol\\plasma_pistol",				_h2_classification_weapon },
	{ "plasma rifle",		'weap', "objects\\weapons\\rifle\\plasma_rifle\\plasma_rifle",					_h2_classification_weapon },
	// the fuel rod (flak cannon) isn't in the host's simulation definition table, so it wouldn't replicate
	{ "plasma_cannon",		'weap', "objects\\weapons\\support_high\\rocket_launcher\\rocket_launcher",	_h2_classification_weapon },
	{ "plasma grenade",		'eqip', "objects\\weapons\\grenade\\plasma_grenade\\plasma_grenade",			_h2_classification_grenade },
	{ "frag grenade",		'eqip', "objects\\weapons\\grenade\\frag_grenade\\frag_grenade",				_h2_classification_grenade },
	{ "pistol",				'weap', "objects\\weapons\\pistol\\magnum\\magnum",								_h2_classification_weapon },
	{ "sniper rifle",		'weap', "objects\\weapons\\rifle\\sniper_rifle\\sniper_rifle",					_h2_classification_weapon },
	{ "rocket launcher",	'weap', "objects\\weapons\\support_high\\rocket_launcher\\rocket_launcher",	_h2_classification_weapon },
	{ "shotgun",			'weap', "objects\\weapons\\rifle\\shotgun\\shotgun",							_h2_classification_weapon },
	{ "needler",			'weap', "objects\\weapons\\pistol\\needler\\needler",							_h2_classification_weapon },
	{ "flamethrower",		'weap', "objects\\weapons\\support_low\\brute_shot\\brute_shot",				_h2_classification_weapon },
	{ "active camouflage",	'eqip', "objects\\powerups\\active_camouflage\\active_camouflage",				_h2_classification_powerup },
	{ "over shield",		'eqip', "objects\\powerups\\over_shield\\over_shield",							_h2_classification_powerup },
	{ "overshield",			'eqip', "objects\\powerups\\over_shield\\over_shield",							_h2_classification_powerup },
	{ "rwarthog",			'vehi', "objects\\vehicles\\warthog\\warthog",									_h2_classification_primary_light_land },
	{ "warthog",			'vehi', "objects\\vehicles\\warthog\\warthog",									_h2_classification_primary_light_land },
	{ "ghost",				'vehi', "objects\\vehicles\\ghost\\ghost",										_h2_classification_secondary_light_land },
	{ "scorpion",			'vehi', "objects\\vehicles\\scorpion\\scorpion",								_h2_classification_primary_heavy_land },
	{ "banshee",			'vehi', "objects\\vehicles\\banshee\\banshee",									_h2_classification_primary_flying },
	{ "c gun turret",		'vehi', "objects\\vehicles\\c_turret_ap\\c_turret_ap",							_h2_classification_primary_turret },
};

/* globals */

static std::unordered_map<datum, datum> g_h1_collection_cache;
static std::unordered_map<datum, int8> g_h1_collection_classification;

/* prototypes */

static const s_h1_object_substitute* h1_object_substitute_get(const char* h1_name);
static datum h1_item_collection_get(datum h1_collection_index, int8* out_classification);
static void h1_simulation_definition_table_extend(scenario* h2_scenario);
static datum h1_vehicle_collection_get(const char* h1_vehicle_name, int8* out_classification);
static e_item_spawn_game_type h1_equipment_game_type(int16 h1_game_type);
static bool h1_netgame_item_rest_pose(datum h1_collection_index, const real_point3d* point, real_point3d* out_position, real_euler_angles3d* out_orientation);

/* public code */

void h1_equipment_build(scenario* h2_scenario, const h1_scnr* h1_scenario)
{
	g_h1_collection_cache.clear();
	g_h1_collection_classification.clear();
	h1_objects_reset();

	const int32 h1_item_count = h1_scenario->netgame_equipment.count;

	// vehicles placed for multiplayer
	int32 vehicle_count = 0;
	for (int32 i = 0; i < h1_scenario->vehicles.count; i++)
	{
		const h1_scnr_vehicles* vehicle = g_h1_cache_file->block_get(h1_scenario->vehicles, i);
		if (vehicle->multiplayer_spawn_flags != 0 || vehicle->multiplayer_team_index >= 0)
		{
			vehicle_count++;
		}
	}

	h2x_scnr_netgame_equipment* equipment = (h2x_scnr_netgame_equipment*)h1_runtime_block_allocate(
		&h2_scenario->netgame_equipment, sizeof(h2x_scnr_netgame_equipment), h1_item_count + vehicle_count);
	int32 count = 0;

	for (int32 i = 0; i < h1_item_count; i++)
	{
		const h1_scnr_netgame_equipment* source = g_h1_cache_file->block_get(h1_scenario->netgame_equipment, i);
		int8 classification = _h2_classification_weapon;
		const datum collection = h1_item_collection_get(source->item_collection.index, &classification);
		if (collection == NONE)
		{
			continue;
		}

		h2x_scnr_netgame_equipment* item = &equipment[count++];
		// flags: levitate
		item->flags = source->flags & FLAG(0);
		item->game_type_1 = h1_equipment_game_type(source->type_0);
		item->game_type_2 = h1_equipment_game_type(source->type_1);
		item->game_type_3 = h1_equipment_game_type(source->type_2);
		item->game_type_4 = h1_equipment_game_type(source->type_3);
		item->team_index = source->team_index;
		item->spawn_time = source->spawn_time;
		item->classification = classification;
		item->position = source->position;
		item->orientation.yaw = source->facing;
		// halo 1 drops a netgame item that isn't created at rest and lays its ground point marker on the surface below,
		// halo 2 leaves it standing where it was placed: place it as it comes to rest
		if (!TEST_BIT(source->flags, 0))
		{
			h1_netgame_item_rest_pose(source->item_collection.index, &source->position, &item->position, &item->orientation);
		}
		h1_runtime_reference_set(&item->item_vehicle_collection, classification == _h2_classification_grenade || classification == _h2_classification_powerup || classification == _h2_classification_weapon ? 'itmc' : 'vehc', collection);
	}

	for (int32 i = 0; i < h1_scenario->vehicles.count; i++)
	{
		const h1_scnr_vehicles* vehicle = g_h1_cache_file->block_get(h1_scenario->vehicles, i);
		if (vehicle->multiplayer_spawn_flags == 0 && vehicle->multiplayer_team_index < 0)
		{
			continue;
		}
		const h1_scnr_vehicle_palette* palette = g_h1_cache_file->block_get(h1_scenario->vehicle_palette, vehicle->palette_index);
		if (!palette)
		{
			continue;
		}

		int8 classification = _h2_classification_primary_light_land;
		const datum collection = h1_vehicle_collection_get(g_h1_cache_file->tag_name_get(palette->name.index), &classification);
		if (collection == NONE)
		{
			continue;
		}

		h2x_scnr_netgame_equipment* item = &equipment[count++];
		item->game_type_1 = item_spawn_game_type_all_game_types;
		item->team_index = vehicle->multiplayer_team_index;
		item->classification = classification;
		item->position = vehicle->position;
		item->orientation.yaw = vehicle->rotation.yaw;
		item->orientation.pitch = vehicle->rotation.pitch;
		item->orientation.roll = vehicle->rotation.roll;
		h1_runtime_reference_set(&item->item_vehicle_collection, 'vehc', collection);
	}
	h2_scenario->netgame_equipment.count = count;

	// starting equipment
	const int32 starting_count = h1_scenario->starting_equipment.count;
	if (starting_count > 0)
	{
		h2x_scnr_starting_equipment* starting = (h2x_scnr_starting_equipment*)h1_runtime_block_allocate(
			&h2_scenario->starting_equipment, sizeof(h2x_scnr_starting_equipment), starting_count);
		for (int32 i = 0; i < starting_count; i++)
		{
			const h1_scnr_starting_equipment* source = g_h1_cache_file->block_get(h1_scenario->starting_equipment, i);
			h2x_scnr_starting_equipment* destination = &starting[i];
			// flags: no grenades, plasma grenades
			destination->flags = source->flags & (FLAG(0) | FLAG(1));
			destination->game_type_1 = h1_equipment_game_type(source->type_0);
			destination->game_type_2 = h1_equipment_game_type(source->type_1);
			destination->game_type_3 = h1_equipment_game_type(source->type_2);
			destination->game_type_4 = h1_equipment_game_type(source->type_3);

			const h1_tag_reference* sources[] = { &source->item_collection_1, &source->item_collection_2, &source->item_collection_3, &source->item_collection_4, &source->item_collection_5, &source->item_collection_6 };
			tag_reference* destinations[] = { &destination->item_collection_1, &destination->item_collection_2, &destination->item_collection_3, &destination->item_collection_4, &destination->item_collection_5, &destination->item_collection_6 };
			for (int32 j = 0; j < NUMBEROF(sources); j++)
			{
				int8 classification;
				const datum collection = h1_item_collection_get(sources[j]->index, &classification);
				h1_runtime_reference_set(destinations[j], collection != NONE ? 'itmc' : (tag_group)NONE, collection);
			}
		}
	}

	h1_simulation_definition_table_extend(h2_scenario);
	h1_log("equipment: %d netgame items and vehicles from %d halo 1 items and %d vehicles, %d starting equipment", count, h1_item_count, vehicle_count, starting_count);
	return;
}

/* private code */

static const s_h1_object_substitute* h1_object_substitute_get(const char* h1_name)
{
	for (int32 i = 0; i < NUMBEROF(k_h1_object_substitutes); i++)
	{
		if (strstr(h1_name, k_h1_object_substitutes[i].h1_name_part))
		{
			return &k_h1_object_substitutes[i];
		}
	}
	return NULL;
}

static e_item_spawn_game_type h1_equipment_game_type(int16 h1_game_type)
{
	// halo 1 and halo 2 share the game type enum layout (terminator became juggernaut)
	switch (h1_game_type)
	{
	case 6: return item_spawn_game_type_juggernaut;
	case 7: return item_spawn_game_type_stub;
	default: return VALID_INDEX(h1_game_type, 15) ? (e_item_spawn_game_type)h1_game_type : item_spawn_game_type_game_type_none;
	}
}

// a halo 2 item collection equivalent to a halo 1 item collection
static datum h1_item_collection_get(datum h1_collection_index, int8* out_classification)
{
	*out_classification = _h2_classification_weapon;
	if (h1_collection_index == NONE)
	{
		return NONE;
	}

	auto found = g_h1_collection_cache.find(h1_collection_index);
	if (found != g_h1_collection_cache.end())
	{
		const datum collection = found->second;
		if (collection != NONE)
		{
			*out_classification = g_h1_collection_classification[collection];
		}
		return collection;
	}

	const h1_itmc* h1_collection = (const h1_itmc*)g_h1_cache_file->tag_get('itmc', h1_collection_index);
	datum result = NONE;
	if (h1_collection)
	{
		struct s_permutation { real32 weight; datum item; uint32 group; };
		std::vector<s_permutation> permutations;
		int8 classification = _h2_classification_weapon;

		for (int32 i = 0; i < h1_collection->item_permutations.count; i++)
		{
			const h1_itmc_item_permutations* permutation = g_h1_cache_file->block_get(h1_collection->item_permutations, i);
			const h1_cache_file_tag_instance* h1_item = g_h1_cache_file->tag_instance_get(permutation->item.index);
			if (h1_item && h1_item->group_tag == 'eqip')
			{
				const datum h2_equipment = h1_equipment_definition_build(permutation->item.index);
				if (h2_equipment != NONE)
				{
					const h1_eqip* h1_equipment = (const h1_eqip*)g_h1_cache_file->tag_get('eqip', permutation->item.index);
					permutations.push_back({ permutation->weight > 0.f ? permutation->weight : 1.f, h2_equipment, 'eqip' });
					classification = (int8)(h1_equipment->powerup_type == 6 ? _h2_classification_grenade : _h2_classification_powerup);
				}
				continue;
			}
			const s_h1_object_substitute* substitute = h1_object_substitute_get(g_h1_cache_file->tag_name_get(permutation->item.index));
			if (!substitute || substitute->h2_group == 'vehi')
			{
				continue;
			}
			const datum h2_item = h1_runtime_tag_find(substitute->h2_group, substitute->h2_name);
			if (h2_item == NONE)
			{
				h1_log("equipment: missing halo 2 tag %s", substitute->h2_name);
				continue;
			}
			permutations.push_back({ permutation->weight > 0.f ? permutation->weight : 1.f, h2_item, substitute->h2_group });
			classification = substitute->classification;
		}

		if (!permutations.empty())
		{
			char name[256];
			sprintf_s(name, "halo1\\%s", g_h1_cache_file->tag_name_get(h1_collection_index));

			h2x_itmc* collection = NULL;
			result = h1_runtime_tag_new('itmc', name, &collection);
			if (result != NONE)
			{
				h2x_itmc_item_permutations* items = h1_runtime_block_new(&collection->item_permutations, (int32)permutations.size());
				for (size_t i = 0; i < permutations.size(); i++)
				{
					items[i].weight = permutations[i].weight;
					h1_runtime_reference_set(&items[i].item, permutations[i].group, permutations[i].item);
					items[i].variant_name = _string_id_empty_string;
				}
				collection->spawn_time = h1_collection->spawn_time_in_seconds_0_default;
				g_h1_collection_classification[result] = classification;
			}
			*out_classification = classification;
		}
	}

	g_h1_collection_cache[h1_collection_index] = result;
	return result;
}

static datum h1_vehicle_collection_get(const char* h1_vehicle_name, int8* out_classification)
{
	const s_h1_object_substitute* substitute = h1_object_substitute_get(h1_vehicle_name);
	if (!substitute || substitute->h2_group != 'vehi')
	{
		h1_log("equipment: no halo 2 vehicle for %s", h1_vehicle_name);
		return NONE;
	}
	*out_classification = substitute->classification;

	char name[256];
	sprintf_s(name, "halo1\\vehicle collections\\%s", substitute->h2_name);
	const datum existing = h1_runtime_tag_find('vehc', name);
	if (existing != NONE)
	{
		return existing;
	}

	const datum vehicle = h1_runtime_tag_find('vehi', substitute->h2_name);
	if (vehicle == NONE)
	{
		h1_log("equipment: missing halo 2 tag %s", substitute->h2_name);
		return NONE;
	}

	h2x_vehc* collection = NULL;
	const datum result = h1_runtime_tag_new('vehc', name, &collection);
	if (result != NONE)
	{
		h2x_vehc_vehicle_permutations* permutation = h1_runtime_block_new(&collection->vehicle_permutations, 1);
		permutation->weight = 1.f;
		h1_runtime_reference_set(&permutation->vehicle, 'vehi', vehicle);
		permutation->variant_name = _string_id_default;
		collection->spawn_time = 30;
	}
	return result;
}

// objects only replicate when their definition is in the scenario's simulation definition table
static void h1_simulation_definition_table_extend(scenario* h2_scenario)
{
	datum definitions[64];
	const int32 definition_count = h1_objects_bound_definitions(definitions, NUMBEROF(definitions));
	if (definition_count <= 0)
	{
		return;
	}

	s_tag_block* table = &h2_scenario->simulation_definition_table;
	const int32 old_count = table->count;
	const s_scenario_simulation_definition_table_element* old_elements = old_count > 0 ?
		(const s_scenario_simulation_definition_table_element*)tag_block_get_element_with_size(table, 0, sizeof(s_scenario_simulation_definition_table_element)) :
		NULL;
	const int32 new_count = MIN(old_count + definition_count, (int32)k_maximum_simulation_definition_table_elements_per_scenario);

	std::vector<s_scenario_simulation_definition_table_element> elements(old_elements, old_elements + old_count);
	for (int32 i = 0; i < definition_count && (int32)elements.size() < new_count; i++)
	{
		elements.push_back({ definitions[i] });
	}
	s_scenario_simulation_definition_table_element* destination = (s_scenario_simulation_definition_table_element*)h1_runtime_block_allocate(
		table, sizeof(s_scenario_simulation_definition_table_element), (int32)elements.size());
	csmemcpy(destination, elements.data(), elements.size() * sizeof(s_scenario_simulation_definition_table_element));
	h1_log("equipment: simulation definition table %d -> %d", old_count, (int32)elements.size());
	return;
}

// a frame as halo 1 stores it: forward, left and up axes and a position
struct s_h1_frame
{
	real_vector3d axes[3];
	real_point3d position;
};

static real_vector3d h1_frame_rotate(const s_h1_frame* frame, const real_vector3d* vector)
{
	real_vector3d result;
	result.i = frame->axes[0].i * vector->i + frame->axes[1].i * vector->j + frame->axes[2].i * vector->k;
	result.j = frame->axes[0].j * vector->i + frame->axes[1].j * vector->j + frame->axes[2].j * vector->k;
	result.k = frame->axes[0].k * vector->i + frame->axes[1].k * vector->j + frame->axes[2].k * vector->k;
	return result;
}

static s_h1_frame h1_frame_multiply(const s_h1_frame* a, const s_h1_frame* b)
{
	s_h1_frame result;
	for (int32 i = 0; i < 3; i++)
	{
		result.axes[i] = h1_frame_rotate(a, &b->axes[i]);
	}
	const real_vector3d position = { b->position.x, b->position.y, b->position.z };
	const real_vector3d offset = h1_frame_rotate(a, &position);
	result.position = { a->position.x + offset.i, a->position.y + offset.j, a->position.z + offset.k };
	return result;
}

// halo 1 builds its rotation matrices from the conjugate of the quaternion
static s_h1_frame h1_frame_from_quaternion(const real_quaternion* rotation, const real_point3d* position)
{
	const real32 x = -rotation->v.i, y = -rotation->v.j, z = -rotation->v.k, w = rotation->w;
	s_h1_frame frame;
	frame.axes[0] = { 1.f - 2.f * (y * y + z * z), 2.f * (x * y + z * w), 2.f * (x * z - y * w) };
	frame.axes[1] = { 2.f * (x * y - z * w), 1.f - 2.f * (x * x + z * z), 2.f * (y * z + x * w) };
	frame.axes[2] = { 2.f * (x * z + y * w), 2.f * (y * z - x * w), 1.f - 2.f * (x * x + y * y) };
	frame.position = *position;
	return frame;
}

static real_vector3d h1_cross(const real_vector3d* a, const real_vector3d* b)
{
	return { a->j * b->k - a->k * b->j, a->k * b->i - a->i * b->k, a->i * b->j - a->j * b->i };
}

static void h1_normalize(real_vector3d* vector)
{
	const real32 length = sqrtf(vector->i * vector->i + vector->j * vector->j + vector->k * vector->k);
	if (length > 0.0001f)
	{
		vector->i /= length;
		vector->j /= length;
		vector->k /= length;
	}
	return;
}

// the pose halo 1 gives an item resting on flat ground at the point (item_align_to_normal_and_point): the ground point marker
// turned the shortest way onto the ground normal and moved onto the point, the object following the marker
static bool h1_netgame_item_rest_pose(datum h1_collection_index, const real_point3d* point, real_point3d* out_position, real_euler_angles3d* out_orientation)
{
	const h1_itmc* collection = (const h1_itmc*)g_h1_cache_file->tag_get('itmc', h1_collection_index);
	if (!collection || collection->item_permutations.count <= 0)
	{
		return false;
	}

	// every item of the collection has to rest the same way
	const h1_eqip* equipment = NULL;
	for (int32 i = 0; i < collection->item_permutations.count; i++)
	{
		const h1_itmc_item_permutations* permutation = g_h1_cache_file->block_get(collection->item_permutations, i);
		const h1_cache_file_tag_instance* instance = g_h1_cache_file->tag_instance_get(permutation->item.index);
		if (!instance || instance->group_tag != 'eqip')
		{
			return false;
		}
		const h1_eqip* candidate = (const h1_eqip*)g_h1_cache_file->tag_get('eqip', permutation->item.index);
		if (!candidate || (equipment && (candidate->model.index != equipment->model.index || candidate->scale != equipment->scale)))
		{
			return false;
		}
		equipment = candidate;
	}

	const h1_mode* model = (const h1_mode*)g_h1_cache_file->tag_get('mode', equipment->model.index);
	if (!model)
	{
		return false;
	}
	const h1_mode_markers_instances* marker = NULL;
	for (int32 i = 0; i < model->markers.count && !marker; i++)
	{
		const h1_mode_markers* group = g_h1_cache_file->block_get(model->markers, i);
		if (_stricmp(group->name, "ground point") == 0 && group->instances.count > 0)
		{
			marker = g_h1_cache_file->block_get(group->instances, 0);
		}
	}
	if (!marker || marker->node_index < 0 || marker->node_index >= model->nodes.count)
	{
		return false;
	}

	// the marker in object space: its node's default pose (through the parents), then the marker, scaled with the item
	s_h1_frame marker_frame = h1_frame_from_quaternion(&marker->rotation, &marker->translation);
	int16 node_index = marker->node_index;
	for (int32 depth = 0; node_index >= 0 && node_index < model->nodes.count && depth < model->nodes.count; depth++)
	{
		const h1_mode_nodes* node = g_h1_cache_file->block_get(model->nodes, node_index);
		const s_h1_frame node_frame = h1_frame_from_quaternion(&node->default_rotation, &node->default_translation);
		marker_frame = h1_frame_multiply(&node_frame, &marker_frame);
		node_index = node->parent_node_index;
	}
	const real32 scale = equipment->scale != 0.f ? equipment->scale : 1.f;
	marker_frame.position = { marker_frame.position.x * scale, marker_frame.position.y * scale, marker_frame.position.z * scale };

	// the marker's up turned the shortest way onto the ground normal carries its forward along
	const real_vector3d normal = { 0.f, 0.f, 1.f };
	const real_vector3d marker_forward = marker_frame.axes[0];
	const real_vector3d marker_up = marker_frame.axes[2];
	real_vector3d forward;
	const real32 half_angle_scale = sqrtf(2.f * (marker_up.k + 1.f));
	if (half_angle_scale > 0.01f)
	{
		real_vector3d axis = h1_cross(&marker_up, &normal);
		axis = { axis.i / half_angle_scale, axis.j / half_angle_scale, axis.k / half_angle_scale };
		const real32 w = half_angle_scale * 0.5f;
		const real_vector3d a = h1_cross(&axis, &marker_forward);
		const real_vector3d b = h1_cross(&axis, &a);
		forward = { marker_forward.i + 2.f * (w * a.i + b.i), marker_forward.j + 2.f * (w * a.j + b.j), marker_forward.k + 2.f * (w * a.k + b.k) };
	}
	else
	{
		const real_vector3d cross = h1_cross(&normal, &marker_forward);
		forward = h1_cross(&cross, &normal);
	}
	h1_normalize(&forward);

	s_h1_frame ground_frame;
	ground_frame.axes[0] = forward;
	ground_frame.axes[1] = h1_cross(&normal, &forward);
	ground_frame.axes[2] = normal;
	ground_frame.position = *point;

	// object = ground * inverse(marker)
	s_h1_frame inverse_marker;
	inverse_marker.axes[0] = { marker_frame.axes[0].i, marker_frame.axes[1].i, marker_frame.axes[2].i };
	inverse_marker.axes[1] = { marker_frame.axes[0].j, marker_frame.axes[1].j, marker_frame.axes[2].j };
	inverse_marker.axes[2] = { marker_frame.axes[0].k, marker_frame.axes[1].k, marker_frame.axes[2].k };
	const real_vector3d marker_position = { marker_frame.position.x, marker_frame.position.y, marker_frame.position.z };
	const real_vector3d inverse_position = h1_frame_rotate(&inverse_marker, &marker_position);
	inverse_marker.position = { -inverse_position.i, -inverse_position.j, -inverse_position.k };
	const s_h1_frame object = h1_frame_multiply(&ground_frame, &inverse_marker);

	// halo 2 orients netgame items by yaw, pitch and roll in radians (z, then y negated, then x)
	const real_vector3d& f = object.axes[0];
	const real_vector3d& l = object.axes[1];
	const real_vector3d& u = object.axes[2];
	const real32 pitch = asinf(f.k < -1.f ? -1.f : (f.k > 1.f ? 1.f : f.k));
	real32 yaw, roll;
	if (cosf(pitch) > 0.0001f)
	{
		yaw = atan2f(f.j, f.i);
		roll = atan2f(l.k, u.k);
	}
	else
	{
		yaw = atan2f(-l.i, l.j);
		roll = 0.f;
	}
	*out_position = object.position;
	out_orientation->yaw = yaw;
	out_orientation->pitch = pitch;
	out_orientation->roll = roll;
	return true;
}

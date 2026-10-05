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
		item->orientation.yaw = RADIANS_TO_DEGREES(source->facing);
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
		item->orientation.yaw = RADIANS_TO_DEGREES(vehicle->rotation.yaw);
		item->orientation.pitch = RADIANS_TO_DEGREES(vehicle->rotation.pitch);
		item->orientation.roll = RADIANS_TO_DEGREES(vehicle->rotation.roll);
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

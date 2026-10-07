#include "stdafx.h"
#include "h1_scenario_objects.h"
#include "h1_game_state.h"

#include "h1_animations.h"
#include "h1_bipeds.h"
#include "h1_cache_file.h"
#include "h1_hs.h"
#include "h1_items.h"
#include "h1_log.h"
#include "h1_devices.h"
#include "h1_hud.h"
#include "h1_object_tags.h"
#include "h1_objects.h"
#include "h1_runtime.h"
#include "h1_vehicles.h"
#include "h1_weapons.h"
#include "h2_tag_definitions_generated.h"

#include "game/game.h"
#include "game/game_options.h"
#include "math/real_math.h"
#include "objects/damage.h"
#include "objects/object_placement.h"
#include "objects/object_types.h"
#include "objects/objects.h"

#include <unordered_map>
#include <vector>

/*
* The halo 1 scenario's object placements (bipeds, vehicles, weapons, equipment, machines, controls, light fixtures and the
* scenery the structure doesn't carry as instances) in campaign games: object_types_place_all at the start, then the scripts'
* object_create by name. Weapons and equipment are the halo 1 items; the others are objects built from their halo 1 tags (model
* nodes, markers and collision, drawn by the halo 1 renderer) that keep their halo 1 object type for the scripts.
*/

/* ---------- constants */

enum
{
	k_h2_machine_type_gear = 2,
};

enum e_h1_object_type
{
	_h1_object_type_biped = 0,
	_h1_object_type_vehicle,
	_h1_object_type_weapon,
	_h1_object_type_equipment,
	_h1_object_type_garbage,
	_h1_object_type_projectile,
	_h1_object_type_scenery,
	_h1_object_type_machine,
	_h1_object_type_control,
	_h1_object_type_light_fixture,
	_h1_object_type_placeholder,
	_h1_object_type_sound_scenery,
	k_h1_object_type_count
};

enum
{
	_h1_placement_not_automatic_bit = 0,
};

/* ---------- structures */

// every scenario placement starts with these
struct s_h1_placement
{
	int16 palette_index;
	int16 name_index;
	uint16 placement_flags;
	int16 desired_permutation;
	real_point3d position;
	real_euler_angles3d rotation;
};

struct s_h1_placement_type
{
	int16 h1_object_type;
	uint32 placements_offset;	// in the scenario
	uint32 palette_offset;
	uint32 placement_size;
	uint32 palette_size;
};

struct s_h1_scenario_objects_globals
{
	std::unordered_map<datum, datum> definitions;			// halo 1 object definition: its halo 2 one
	std::unordered_map<datum, datum> static_definitions;	// the ones without havok collision (h1_placement_static)
	std::unordered_map<datum, int16> object_types;			// objects placed from the scenario: their halo 1 type
};

/* ---------- globals */

// scenario_get_object_type_scenario_datums / palette, by halo 1 object type
static const s_h1_placement_type k_h1_placement_types[] =
{
	{ _h1_object_type_biped, 0x228, 0x234, 0x78, 0x30 },
	{ _h1_object_type_vehicle, 0x240, 0x24C, 0x78, 0x30 },
	{ _h1_object_type_weapon, 0x270, 0x27C, 0x5C, 0x30 },
	{ _h1_object_type_equipment, 0x258, 0x264, 0x28, 0x30 },
	{ _h1_object_type_scenery, 0x210, 0x21C, 0x48, 0x30 },
	{ _h1_object_type_machine, 0x294, 0x2A0, 0x40, 0x30 },
	{ _h1_object_type_control, 0x2AC, 0x2B8, 0x40, 0x30 },
	{ _h1_object_type_light_fixture, 0x2C4, 0x2D0, 0x58, 0x30 },
};

static s_h1_scenario_objects_globals g_h1_scenario_objects;
// game state: the placed objects' halo 1 types (the definitions built for them stay built)
static c_h1_game_state_variable<decltype(g_h1_scenario_objects.object_types)> g_h1_scenario_object_types_game_state(&g_h1_scenario_objects.object_types);

/* ---------- prototypes */

static const s_h1_placement_type* h1_placement_type_get(int16 h1_object_type);
static const s_h1_placement* h1_placement_get(const s_h1_placement_type* type, int32 placement_index);
static datum h1_placement_definition_get(const s_h1_placement_type* type, const s_h1_placement* placement);
static bool h1_placement_wanted(const s_h1_placement_type* type, const s_h1_placement* placement);
static datum h1_object_shell_definition_build(datum h1_definition_index, bool collision = true);
static bool h1_placement_static(const s_h1_placement_type* type, const s_h1_placement* placement);
static datum h1_placement_object_new(const s_h1_placement_type* type, int32 placement_index);

/* ---------- public code */

void h1_scenario_objects_build(void)
{
	g_h1_scenario_objects.definitions.clear();
	g_h1_scenario_objects.static_definitions.clear();
	g_h1_scenario_objects.object_types.clear();
	h1_cinematic_titles_reset();
	h1_hud_text_reset();
	h1_hud_nav_points_reset();
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();
	if (scenario->type != 0)
	{
		// multiplayer maps place their items and vehicles through the netgame equipment
		return;
	}

	int32 definition_count = 0;
	for (const s_h1_placement_type& type : k_h1_placement_types)
	{
		const h1_tag_block<uint8>* placements = (const h1_tag_block<uint8>*)((const uint8*)scenario + type.placements_offset);
		for (int32 i = 0; i < placements->count; i++)
		{
			const s_h1_placement* placement = h1_placement_get(&type, i);
			if (!placement || !h1_placement_wanted(&type, placement))
			{
				continue;
			}
			const datum h1_definition_index = h1_placement_definition_get(&type, placement);
			if (h1_definition_index != NONE && h1_placement_static(&type, placement))
			{
				if (!g_h1_scenario_objects.static_definitions.count(h1_definition_index))
				{
					g_h1_scenario_objects.static_definitions[h1_definition_index] = h1_object_shell_definition_build(h1_definition_index, false);
				}
				continue;
			}
			if (h1_definition_index == NONE || g_h1_scenario_objects.definitions.count(h1_definition_index))
			{
				continue;
			}
			datum definition_index;
			switch (type.h1_object_type)
			{
			case _h1_object_type_weapon:
				definition_index = h1_weapon_definition_build(h1_definition_index);
				break;
			case _h1_object_type_biped:
				definition_index = h1_biped_definition_build(h1_definition_index);
				break;
			case _h1_object_type_equipment:
				definition_index = h1_equipment_definition_build(h1_definition_index);
				break;
			case _h1_object_type_vehicle:
				definition_index = h1_vehicle_build(h1_definition_index);
				break;
			default:
				definition_index = h1_object_shell_definition_build(h1_definition_index);
				break;
			}
			g_h1_scenario_objects.definitions[h1_definition_index] = definition_index;
			definition_count += definition_index != NONE;
		}
	}
	// the animation graphs the scripts play (custom_animation, scenery_animation_start), on the skeleton of their models
	int32 graph_count = 0;
	for (int32 i = 0; i < scenario->references.count; i++)
	{
		const h1_scnr_references* reference = g_h1_cache_file->block_get(scenario->references, i);
		if (reference && reference->reference.index != NONE && g_h1_cache_file->tag_get('antr', reference->reference.index))
		{
			graph_count += h1_scenario_animation_graph_get(reference->reference.index, true) != NONE;
		}
	}
	h1_log("objects: %d object definitions for the scenario's placements, %d script animation graphs", definition_count, graph_count);
	return;
}

void h1_scenario_objects_place(void)
{
	g_h1_scenario_objects.object_types.clear();
	h1_devices_reset();
	if (g_h1_scenario_objects.definitions.empty())
	{
		return;
	}
	int32 count = 0;
	for (const s_h1_placement_type& type : k_h1_placement_types)
	{
		const h1_tag_block<uint8>* placements = (const h1_tag_block<uint8>*)((const uint8*)g_h1_cache_file->scenario_get() + type.placements_offset);
		for (int32 i = 0; i < placements->count; i++)
		{
			const s_h1_placement* placement = h1_placement_get(&type, i);
			if (placement && h1_placement_wanted(&type, placement) && !TEST_BIT(placement->placement_flags, _h1_placement_not_automatic_bit))
			{
				count += h1_placement_object_new(&type, i) != NONE;
			}
		}
	}
	h1_log("objects: placed %d objects", count);
	return;
}

datum h1_scenario_object_definition_get(datum h1_definition_index)
{
	auto found = g_h1_scenario_objects.definitions.find(h1_definition_index);
	if (found != g_h1_scenario_objects.definitions.end())
	{
		return found->second;
	}
	const h1_cache_file_tag_instance* instance = h1_definition_index != NONE ? g_h1_cache_file->tag_instance_get(h1_definition_index) : NULL;
	if (!instance)
	{
		return NONE;
	}
	datum definition_index;
	switch (instance->group_tag)
	{
	case 'weap': definition_index = h1_weapon_definition_build(h1_definition_index); break;
	case 'bipd': definition_index = h1_biped_definition_build(h1_definition_index); break;
	case 'eqip': definition_index = h1_equipment_definition_build(h1_definition_index); break;
	case 'vehi': definition_index = h1_vehicle_build(h1_definition_index); break;
	default: definition_index = h1_object_shell_definition_build(h1_definition_index); break;
	}
	g_h1_scenario_objects.definitions[h1_definition_index] = definition_index;
	return definition_index;
}

void h1_scenario_object_type_set(datum object_index, datum h1_definition_index)
{
	const h1_cache_file_tag_instance* instance = h1_definition_index != NONE ? g_h1_cache_file->tag_instance_get(h1_definition_index) : NULL;
	if (!instance)
	{
		return;
	}
	static const uint32 k_groups[] = { 'bipd', 'vehi', 'weap', 'eqip', 'garb', 'proj', 'scen', 'mach', 'ctrl', 'lifi', 'plac', 'ssce' };
	for (int16 type = 0; type < NUMBEROF(k_groups); type++)
	{
		if (instance->group_tag == k_groups[type])
		{
			g_h1_scenario_objects.object_types[object_index] = type;
			break;
		}
	}
	return;
}

datum h1_scenario_object_new_by_name(int16 name_index)
{
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();
	const h1_scnr_object_names* name = g_h1_cache_file->block_get(scenario->object_names, name_index);
	const s_h1_placement_type* type = name ? h1_placement_type_get(name->type) : NULL;
	return type ? h1_placement_object_new(type, name->placement_index) : NONE;
}

datum h1_scenario_animation_graph_get(datum h1_animation_graph_index, bool build)
{
	if (h1_animation_graph_index == NONE)
	{
		return NONE;
	}
	char name[256];
	sprintf_s(name, "halo1\\%s", g_h1_cache_file->tag_name_get(h1_animation_graph_index));
	if (!build)
	{
		return h1_runtime_tag_find('jmad', name);
	}

	// cache files drop the graph's own node list: the skeleton is the model its animations were made for (their node list
	// checksum and node count)
	const h1_antr* h1_graph = (const h1_antr*)g_h1_cache_file->tag_get('antr', h1_animation_graph_index);
	const h1_antr_animations* first = h1_graph ? g_h1_cache_file->block_get(h1_graph->animations, 0) : NULL;
	const h1_mode* model = NULL;
	for (int32 i = 0; first && i < g_h1_cache_file->tag_count(); i++)
	{
		const h1_cache_file_tag_instance* instance = g_h1_cache_file->tag_instance_get_by_absolute_index(i);
		const h1_mode* candidate = instance && instance->group_tag == 'mode' ? (const h1_mode*)g_h1_cache_file->tag_get('mode', instance->tag_index) : NULL;
		if (candidate && candidate->node_list_checksum == first->node_list_checksum && candidate->nodes.count == first->node_count)
		{
			model = candidate;
			break;
		}
	}
	if (!model)
	{
		h1_log("objects: no model for the animation graph %s", name);
		return NONE;
	}
	return h1_animation_graph_build(h1_animation_graph_index, model, name);
}

int16 h1_scenario_object_type_get(datum object_index)
{
	auto found = g_h1_scenario_objects.object_types.find(object_index);
	return found != g_h1_scenario_objects.object_types.end() ? found->second : NONE;
}

void h1_scenario_object_delete(datum object_index)
{
	g_h1_scenario_objects.object_types.erase(object_index);
	if (object_try_and_get(object_index))
	{
		object_delete(object_index);
	}
	return;
}

/* ---------- private code */

static const s_h1_placement_type* h1_placement_type_get(int16 h1_object_type)
{
	for (const s_h1_placement_type& type : k_h1_placement_types)
	{
		if (type.h1_object_type == h1_object_type)
		{
			return &type;
		}
	}
	return NULL;
}

static const s_h1_placement* h1_placement_get(const s_h1_placement_type* type, int32 placement_index)
{
	const h1_tag_block<uint8>* placements = (const h1_tag_block<uint8>*)((const uint8*)g_h1_cache_file->scenario_get() + type->placements_offset);
	if (!VALID_INDEX(placement_index, placements->count))
	{
		return NULL;
	}
	return (const s_h1_placement*)g_h1_cache_file->address_get(placements->address + type->placement_size * placement_index, type->placement_size);
}

static datum h1_placement_definition_get(const s_h1_placement_type* type, const s_h1_placement* placement)
{
	const h1_tag_block<uint8>* palette = (const h1_tag_block<uint8>*)((const uint8*)g_h1_cache_file->scenario_get() + type->palette_offset);
	if (!VALID_INDEX(placement->palette_index, palette->count))
	{
		return NONE;
	}
	const h1_tag_reference* reference = (const h1_tag_reference*)g_h1_cache_file->address_get(palette->address + type->palette_size * placement->palette_index, sizeof(h1_tag_reference));
	return reference ? reference->index : NONE;
}

// the structure carries the unnamed scenery as instances (h1_structure_bsp), named scenery is the scripts' objects
static bool h1_placement_wanted(const s_h1_placement_type* type, const s_h1_placement* placement)
{
	return type->h1_object_type != _h1_object_type_scenery || placement->name_index != NONE;
}

bool h1_scenery_placement_is_object(int32 placement_index)
{
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();
	const h1_scnr_scenery* placement = scenario && scenario->type == 0 ? g_h1_cache_file->block_get(scenario->scenery, placement_index) : NULL;
	return placement && placement->name_index != NONE;
}

bool h1_scenery_placement_collides_as_object(int32 placement_index)
{
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();
	const h1_scnr_scenery* placement = scenario && scenario->type == 0 ? g_h1_cache_file->block_get(scenario->scenery, placement_index) : NULL;
	return placement && placement->name_index != NONE && TEST_BIT(placement->placement_flags, _h1_placement_not_automatic_bit);
}

// a named scenery that's always there: the scripts' object (attaching, animating) without havok collision, which the structure
// keeps (bodies placed in its seats would be inside its havok body)
static bool h1_placement_static(const s_h1_placement_type* type, const s_h1_placement* placement)
{
	return type->h1_object_type == _h1_object_type_scenery && placement->name_index != NONE &&
		!TEST_BIT(placement->placement_flags, _h1_placement_not_automatic_bit);
}

// a halo 2 scenery object definition carrying the halo 1 object's model nodes, markers and collision, bound to the halo 1
// definition for the halo 1 renderer
static datum h1_object_shell_definition_build(datum h1_definition_index, bool collision)
{
	const char* h1_name = g_h1_cache_file->tag_name_get(h1_definition_index);
	char name[256];
	sprintf_s(name, collision ? "halo1\\%s" : "halo1\\%s static", h1_name);
	const datum existing = h1_runtime_tag_find('scen', name);
	if (existing != NONE)
	{
		return existing;
	}

	// every halo 1 object definition starts with the object fields
	const h1_scen* h1_object = (const h1_scen*)g_h1_cache_file->tag_get('obje', h1_definition_index);
	const h1_mode* h1_model = h1_object ? (const h1_mode*)g_h1_cache_file->tag_get('mode', h1_object->model.index) : NULL;
	const h1_coll* h1_collision = h1_object && collision ? (const h1_coll*)g_h1_cache_file->tag_get('coll', h1_object->collision_model.index) : NULL;
	if (!h1_model)
	{
		h1_log("objects: %s has no model", h1_name);
		return NONE;
	}

	s_h1_object_tags tags;
	tags.render_model = h1_object_render_model_build(h1_model, name);
	tags.collision_model = h1_collision ? h1_object_collision_model_build(h1_collision, h1_model, name) : NONE;
	tags.physics_model = h1_object_physics_model_build(h1_collision, h1_model, tags.collision_model, name);
	tags.animation_graph = h1_object->animation_graph.index != NONE ? h1_animation_graph_build(h1_object->animation_graph.index, h1_model, name) : NONE;
	tags.disappear_distance = 200.f;
	const datum model_index = tags.render_model != NONE ? h1_object_model_build(&tags, h1_model, h1_collision, name) : NONE;
	if (model_index == NONE)
	{
		return NONE;
	}

	// with collision it's a halo 2 machine (an inert gear): halo 2 gives havok bodies, what its bipeds and vehicles collide with,
	// only to bipeds, vehicles, machines, crates and creatures (FUN_004a150d's object type mask)
	if (tags.physics_model != NONE)
	{
		h2x_mach* machine = NULL;
		const datum machine_index = h1_runtime_tag_new('mach', name, &machine);
		if (machine_index == NONE)
		{
			return NONE;
		}
		tag_reference* references[] = { &machine->model, &machine->crate_object, &machine->modifier_shader, &machine->creation_effect, &machine->material_effects,
			&machine->open_up, &machine->close_down, &machine->opened, &machine->closed, &machine->depowered, &machine->repowered, &machine->delay_effect };
		for (tag_reference* reference : references)
		{
			reference->group = (tag_group)NONE;
			reference->index = NONE;
		}
		machine->object_type = _object_type_machine;
		machine->bounding_radius = h1_object->bounding_radius;
		machine->bounding_offset = h1_object->bounding_offset;
		machine->acceleration_scale = h1_object->acceleration_scale;
		machine->default_model_variant = _string_id_default;
		h1_runtime_reference_set(&machine->model, 'hlmt', model_index);
		machine->apply_collision_damage_scale = 1.f;
		machine->hud_text_message_index = NONE;
		machine->type = k_h2_machine_type_gear;
		machine->elevator_node = NONE;
		h1_objects_bind(machine_index, h1_definition_index);
		return machine_index;
	}

	h2x_scen* scenery = NULL;
	const datum scenery_index = h1_runtime_tag_new('scen', name, &scenery);
	if (scenery_index == NONE)
	{
		return NONE;
	}
	tag_reference* references[] = { &scenery->model, &scenery->crate_object, &scenery->modifier_shader, &scenery->creation_effect, &scenery->material_effects };
	for (tag_reference* reference : references)
	{
		reference->group = (tag_group)NONE;
		reference->index = NONE;
	}
	scenery->object_type = _object_type_scenery;
	scenery->bounding_radius = h1_object->bounding_radius;
	scenery->bounding_offset = h1_object->bounding_offset;
	scenery->acceleration_scale = h1_object->acceleration_scale;
	scenery->default_model_variant = _string_id_default;
	h1_runtime_reference_set(&scenery->model, 'hlmt', model_index);
	scenery->apply_collision_damage_scale = 1.f;
	scenery->hud_text_message_index = NONE;
	h1_objects_bind(scenery_index, h1_definition_index);
	return scenery_index;
}

// object_new_from_scenario
static datum h1_placement_object_new(const s_h1_placement_type* type, int32 placement_index)
{
	const s_h1_placement* placement = h1_placement_get(type, placement_index);
	if (!placement)
	{
		return NONE;
	}
	if (placement->name_index != NONE)
	{
		const datum existing = h1_hs_object_index_from_name_index(placement->name_index);
		if (existing != NONE)
		{
			return existing;
		}
	}
	const datum h1_definition_index = h1_placement_definition_get(type, placement);
	const auto& definitions = h1_placement_static(type, placement) ? g_h1_scenario_objects.static_definitions : g_h1_scenario_objects.definitions;
	auto found = h1_definition_index != NONE ? definitions.find(h1_definition_index) : definitions.end();
	if (found == definitions.end() || found->second == NONE)
	{
		return NONE;
	}

	s_damage_owner damage_owner;
	damage_owner.owner_player_index = NONE;
	damage_owner.owner_object_index = NONE;
	damage_owner.owner_team_index = (e_game_team)NONE;
	damage_owner.pad = 0;
	object_placement_data data;
	object_placement_data_new(&data, found->second, NONE, &damage_owner);
	data.position = placement->position;
	vectors3d_from_euler_angles3d(&data.forward, &data.up, &placement->rotation);
	const datum object_index = object_new(&data);
	if (object_index == NONE)
	{
		return NONE;
	}
	g_h1_scenario_objects.object_types[object_index] = type->h1_object_type;
	h1_device_place(object_index, type->h1_object_type, placement);
	// biped_place: a biped placed dead (scenario_biped flags bit 0) is a body (unit_kill_silent); the named ones are the scripts'
	// (attached to seats and posed by custom animations, as a30's lifepod riders): halo 2's kill would detach them and make them
	// ragdolls, so they stay posable
	if (type->h1_object_type == _h1_object_type_biped && TEST_BIT(*(const uint32*)((const uint8*)placement + 0x4C), 0) &&
		placement->name_index == NONE)
	{
		Memory::GetAddress<void(__cdecl*)(datum)>(0x13B547)(object_index);
	}
	if (placement->name_index != NONE)
	{
		h1_hs_object_name_set(placement->name_index, object_index);
	}
	return object_index;
}

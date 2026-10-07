#include "stdafx.h"
#include "h1_devices.h"
#include "h1_game_state.h"

#include "h1_animations.h"
#include "h1_cache_file.h"
#include "h1_effects.h"
#include "h1_hud.h"
#include "h1_log.h"
#include "h1_map_loader.h"
#include "h1_objects.h"
#include "h1_sound.h"

#include "game/game_time.h"
#include "game/players.h"
#include "input/input_abstraction.h"
#include "units/units.h"
#include "math/real_math.h"
#include "objects/object_types.h"
#include "objects/objects.h"
#include "physics/collisions.h"

#include <unordered_map>
#include <vector>

/* constants */

enum
{
	k_h1_ticks_per_second = 30,

	// devices.c device flags
	_h1_device_position_reversed_bit = 0,
	_h1_device_not_usable_bit,
	_h1_device_position_changed_bit,

	// device group flags
	_h1_device_group_can_change_only_once_bit = 0,
	_h1_device_group_changed_once_bit,
	_h1_device_group_runtime_bit,

	// scenario device placement flags
	_h1_scenario_device_initially_open_bit = 0,
	_h1_scenario_device_initially_off_bit,
	_h1_scenario_device_changes_only_once_bit,
	_h1_scenario_device_position_reversed_bit,
	_h1_scenario_device_not_usable_bit,

	// machine flags (the scenario's and the datum's)
	_h1_machine_does_not_operate_automatically_bit = 0,
	_h1_machine_one_sided_bit,
	_h1_machine_never_appears_locked_bit,
	_h1_machine_opened_by_melee_attack_bit,

	// machine definition flags
	_h1_machine_definition_is_elevator_bit = 2,

	// device definition flags
	_h1_device_position_loops_bit = 0,
	_h1_device_position_animation_not_interpolated_bit,

	// device function modes
	_h1_device_function_none = 0,
	_h1_device_function_power,
	_h1_device_function_change_in_power,
	_h1_device_function_position,
	_h1_device_function_change_in_position,
	_h1_device_function_locked,
	_h1_device_function_delay,

	// device animations
	_h1_device_animation_position = 0,
	_h1_device_animation_power,

	// machine types
	_h1_machine_type_door = 0,
	_h1_machine_type_platform = 2,

	// halo 1 object types (scenario placements)
	_h1_object_type_machine = 7,
	_h1_object_type_control = 8,
	_h1_object_type_light_fixture = 9,
};

// the device part of a halo 1 device definition (after the object part)
constexpr uint32 k_h1_device_definition_offset = 0x17C;
// the machine part (after the device part)
constexpr uint32 k_h1_machine_definition_offset = 0x290;

/* structures */

struct s_h1_device_definition
{
	uint32 flags;
	real32 power_transition_time;
	real32 power_acceleration_time;
	real32 powered_position_transition_time;
	real32 powered_position_acceleration_time;
	real32 depowered_position_transition_time;
	real32 depowered_position_acceleration_time;
	int16 function_modes[4];
	h1_tag_reference positive_start_effect;
	h1_tag_reference negative_start_effect;
	h1_tag_reference positive_stop_effect;
	h1_tag_reference negative_stop_effect;
	h1_tag_reference depowered_effect;
	h1_tag_reference repowered_effect;
	real32 delay_time;
	uint32 delay_unused[2];
	h1_tag_reference delay_effect;
	real32 automatic_activation_radius;
	uint32 unused[21];
	real32 runtime_maximum_power_acceleration;
	real32 runtime_maximum_power_velocity;
	real32 runtime_maximum_depowered_position_acceleration;
	real32 runtime_maximum_depowered_position_velocity;
	real32 runtime_maximum_powered_position_acceleration;
	real32 runtime_maximum_powered_position_velocity;
	real32 runtime_delay_ticks;
};
static_assert(sizeof(s_h1_device_definition) == 0x114);

struct s_h1_machine_definition
{
	int16 type;
	uint16 flags;
	real32 door_open_time;
	uint32 unused1[20];
	int16 collision_response;
	int16 elevator_node_index;
	uint32 unused2[13];
	int32 runtime_door_open_ticks;
};
static_assert(sizeof(s_h1_machine_definition) == 0x94);

// the control part of a halo 1 control definition (after the device part)
constexpr uint32 k_h1_control_definition_offset = 0x290;

struct s_h1_control_definition
{
	int16 type;				// toggle switch, on button, off button, call button
	int16 triggers_when;	// touched by player, destroyed
	real32 call_value;
	uint8 unused[0x50];
	h1_tag_reference on_effect;
	h1_tag_reference off_effect;
	h1_tag_reference denied_effect;
};

enum
{
	// control datum flags (control_place: the scenario control's)
	_h1_control_usable_from_both_sides_bit = 0,

	// hud.c's state messages
	_h1_hud_message_touch_device = 2,
	_h1_hud_message_custom_device,
};

// the device part shared by the scenario's machines, controls and light fixtures
struct s_h1_scenario_device
{
	int16 power_group_index;
	int16 position_group_index;
	uint32 flags;
};

struct s_h1_device_group
{
	uint16 flags;
	real32 actual_value;
};

struct s_h1_device
{
	datum h1_definition_index;
	int16 h1_object_type;
	uint32 flags;
	int16 power_group_index;
	real32 power;
	real32 power_velocity;
	int16 position_group_index;
	real32 position;
	real32 position_velocity;
	int16 delay_ticks;
	uint32 machine_flags;
	int32 door_open_ticks;
	uint32 control_flags;
	int16 custom_name_index;
	bool elevator_position_valid;
	real_point3d elevator_position;		// the elevator node's position the update before
};

struct s_h1_devices_globals
{
	std::vector<s_h1_device_group> groups;
	std::unordered_map<datum, s_h1_device> devices;
	real32 leftover_ticks;
	int32 last_game_time;
	int32 tick;				// halo 1 ticks run (machine_update's every fourth tick door test)
	bool action_held;		// the local player's action button the tick before
};

/* globals */

static s_h1_devices_globals g_h1_devices;
H1_GAME_STATE_VARIABLE(g_h1_devices);
static void h1_devices_game_state_save(int32 slot) { return; }
static void h1_devices_game_state_restored(void)
{
	for (auto& entry : g_h1_devices.devices)
	{
		SET_BIT(entry.second.flags, _h1_device_position_changed_bit, true);
		entry.second.elevator_position_valid = false;
	}
	return;
}
static c_h1_game_state_procedures g_h1_devices_game_state_fixup(h1_devices_game_state_save, h1_devices_game_state_save, h1_devices_game_state_restored);
static object_preprocess_node_orientations_t g_h2_scenery_preprocess_node_orientations = NULL;
static object_preprocess_node_orientations_t g_h2_device_preprocess_node_orientations = NULL;

/* prototypes */

static const s_h1_device_definition* h1_device_definition_get(const s_h1_device* device);
static bool h1_device_can_change_position(const s_h1_device* device);
static bool h1_device_frontfacing(datum object_index, const s_h1_device* device, const real_vector3d* facing);
static void h1_control_toggle(datum object_index, s_h1_device* device);
static const s_h1_machine_definition* h1_machine_definition_get(const s_h1_device* device);
static s_h1_device* h1_device_get(datum object_index);
static int16 h1_device_group_new(real32 initial_value, uint16 flags);
static void h1_device_tick(datum object_index, s_h1_device* device);
static void h1_machine_tick(datum object_index, s_h1_device* device);
static void h1_machine_elevator_update(datum object_index, s_h1_device* device);
static void h1_device_effect_new(datum object_index, datum effect_index);
static bool h1_accelerate_to_position(real32* position, real32* velocity, real32 target_position, real32 maximum_velocity, real32 acceleration,
	real32 minimum_position, real32 maximum_position, bool periodic);
static void h1_device_preprocess_node_orientations_hook(datum object_index, uint8* node_flags, int32 node_count, real_orientation* orientations);
static void h1_device_scenery_preprocess_node_orientations_hook(datum object_index, uint8* node_flags, int32 node_count, real_orientation* orientations);
static void h1_device_machine_preprocess_node_orientations_hook(datum object_index, uint8* node_flags, int32 node_count, real_orientation* orientations);

/* public code */

void h1_devices_reset(void)
{
	g_h1_devices.action_held = false;
	g_h1_devices.groups.clear();
	g_h1_devices.devices.clear();
	g_h1_devices.leftover_ticks = 0.f;
	g_h1_devices.last_game_time = NONE;
	if (!g_h1_cache_file)
	{
		return;
	}
	// create_initial_device_groups
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();
	for (int32 i = 0; i < scenario->device_groups.count; i++)
	{
		const h1_scnr_device_groups* group = g_h1_cache_file->block_get(scenario->device_groups, i);
		h1_device_group_new(group->initial_value, TEST_BIT(group->flags, 0) ? FLAG(_h1_device_group_can_change_only_once_bit) : 0);
	}
	return;
}

void h1_device_place(datum object_index, int16 h1_object_type, const void* placement)
{
	const object_datum* object = object_try_and_get(object_index);
	if (!object || (h1_object_type != _h1_object_type_machine && h1_object_type != _h1_object_type_control && h1_object_type != _h1_object_type_light_fixture))
	{
		return;
	}
	const s_h1_scenario_device* scenario_device = (const s_h1_scenario_device*)((const uint8*)placement + 0x28);
	s_h1_device device = {};
	device.h1_definition_index = h1_objects_h1_definition_get(object->definition_index);
	device.h1_object_type = h1_object_type;
	device.power_group_index = scenario_device->power_group_index != NONE && VALID_INDEX(scenario_device->power_group_index, (int16)g_h1_devices.groups.size()) ?
		scenario_device->power_group_index :
		h1_device_group_new(TEST_BIT(scenario_device->flags, _h1_scenario_device_initially_off_bit) ? 0.f : 1.f, FLAG(_h1_device_group_runtime_bit));
	device.position_group_index = scenario_device->position_group_index != NONE && VALID_INDEX(scenario_device->position_group_index, (int16)g_h1_devices.groups.size()) ?
		scenario_device->position_group_index :
		h1_device_group_new(TEST_BIT(scenario_device->flags, _h1_scenario_device_initially_open_bit) ? 1.f : 0.f,
			FLAG(_h1_device_group_runtime_bit) | (TEST_BIT(scenario_device->flags, _h1_scenario_device_changes_only_once_bit) ? FLAG(_h1_device_group_can_change_only_once_bit) : 0));
	device.power = g_h1_devices.groups[device.power_group_index].actual_value;
	device.position = g_h1_devices.groups[device.position_group_index].actual_value;
	SET_BIT(device.flags, _h1_device_position_reversed_bit, TEST_BIT(scenario_device->flags, _h1_scenario_device_position_reversed_bit));
	SET_BIT(device.flags, _h1_device_not_usable_bit, TEST_BIT(scenario_device->flags, _h1_scenario_device_not_usable_bit));
	SET_BIT(device.flags, _h1_device_position_changed_bit, true);
	if (h1_object_type == _h1_object_type_machine)
	{
		// machine_place: the scenario machine's flags follow its device part
		device.machine_flags = *(const uint32*)((const uint8*)placement + 0x30) & 0xF;
	}
	if (h1_object_type == _h1_object_type_control)
	{
		// control_place: usable from both sides, its custom name (the scenario's custom object names, from 1)
		const h1_scnr_controls* control = (const h1_scnr_controls*)placement;
		SET_BIT(device.control_flags, _h1_control_usable_from_both_sides_bit, TEST_BIT(control->control_flags, 0));
		device.custom_name_index = control->custom_object_name_index - 1;
	}
	g_h1_devices.devices[object_index] = device;
	return;
}

void h1_devices_update(void)
{
	if (!h1_maps_active() || g_h1_devices.devices.empty())
	{
		return;
	}
	const int32 game_time = (int32)game_time_get();
	if (g_h1_devices.last_game_time != NONE && game_time > g_h1_devices.last_game_time)
	{
		g_h1_devices.leftover_ticks += (real32)(game_time - g_h1_devices.last_game_time) * game_tick_length() * k_h1_ticks_per_second;
	}
	g_h1_devices.last_game_time = game_time;

	while (g_h1_devices.leftover_ticks >= 1.f)
	{
		g_h1_devices.leftover_ticks -= 1.f;
		g_h1_devices.tick++;
		for (auto it = g_h1_devices.devices.begin(); it != g_h1_devices.devices.end();)
		{
			if (!object_try_and_get(it->first))
			{
				it = g_h1_devices.devices.erase(it);
				continue;
			}
			h1_device_tick(it->first, &it->second);
			if (it->second.h1_object_type == _h1_object_type_machine)
			{
				h1_machine_tick(it->first, &it->second);
			}
			it++;
		}
	}

	// the moved devices' nodes again
	for (auto& entry : g_h1_devices.devices)
	{
		if (TEST_BIT(entry.second.flags, _h1_device_position_changed_bit))
		{
			SET_BIT(entry.second.flags, _h1_device_position_changed_bit, false);
			// machine_update's object_translate: awake, its nodes again
			object_wake(entry.first);
			object_compute_node_matrices_with_children(entry.first);
			// halo 2 places a machine's keyframed havok bodies at its nodes when it makes them, and never moves them: made again at
			// the nodes where they are now (FUN_004e6deb deletes, FUN_004a150d makes the object's havok component)
			object_datum* object = object_get(entry.first);
			if (object->object.havok_datum != NONE)
			{
				Memory::GetAddress<void(__cdecl*)(datum)>(0xE6DEB)(object->object.havok_datum);
				object->object.havok_datum = NONE;
				Memory::GetAddress<void(__cdecl*)(datum)>(0xA150D)(entry.first);
			}
		}
		if (entry.second.h1_object_type == _h1_object_type_machine)
		{
			h1_machine_elevator_update(entry.first, &entry.second);
		}
	}
	h1_controls_player_update();
	return;
}

void h1_device_functions_export(datum object_index, real32* incoming)
{
	const s_h1_device* device = h1_device_get(object_index);
	const s_h1_device_definition* definition = device ? h1_device_definition_get(device) : NULL;
	if (!definition)
	{
		return;
	}
	for (int32 i = 0; i < 4; i++)
	{
		real32 value = 0.f;
		switch (definition->function_modes[i])
		{
		case _h1_device_function_none:
			continue;
		case _h1_device_function_power:
			value = device->power;
			break;
		case _h1_device_function_change_in_power:
			value = device->power_velocity != 0.f && definition->runtime_maximum_power_velocity != 0.f ?
				fabsf(device->power_velocity) / definition->runtime_maximum_power_velocity : 0.f;
			break;
		case _h1_device_function_position:
			value = device->position;
			break;
		case _h1_device_function_change_in_position:
			value = device->position_velocity != 0.f && definition->runtime_maximum_powered_position_velocity != 0.f ?
				fabsf(device->position_velocity) / definition->runtime_maximum_powered_position_velocity : 0.f;
			break;
		case _h1_device_function_locked:
			value = device->power == 0.f ? 1.f : 0.f;
			if (device->h1_object_type == _h1_object_type_machine && device->position_group_index != NONE)
			{
				const s_h1_device_group* group = &g_h1_devices.groups[device->position_group_index];
				if (TEST_BIT(device->machine_flags, _h1_machine_does_not_operate_automatically_bit) || TEST_BIT(device->machine_flags, _h1_machine_one_sided_bit))
				{
					value = 1.f;
				}
				if (TEST_BIT(group->flags, _h1_device_group_can_change_only_once_bit) && TEST_BIT(group->flags, _h1_device_group_changed_once_bit))
				{
					value = 1.f;
				}
				if (device->position == 1.f || TEST_BIT(device->machine_flags, _h1_machine_never_appears_locked_bit))
				{
					value = 0.f;
				}
			}
			break;
		case _h1_device_function_delay:
			value = definition->runtime_delay_ticks <= 0.f || device->delay_ticks == definition->runtime_delay_ticks ? 0.f :
				device->delay_ticks / definition->runtime_delay_ticks;
			break;
		default:
			break;
		}
		incoming[i] = value;
	}
	return;
}

void h1_controls_player_update(void)
{
	const datum player_index = player_index_from_user_index(0);
	const datum unit_index = player_index != NONE ? player_get(player_index)->unit_index : NONE;
	const unit_datum* unit = unit_index != NONE ? (const unit_datum*)object_try_and_get_and_verify_type(unit_index, _object_mask_unit) : NULL;
	const bool action = input_abstraction_controller_button_test(_controller0, _button_touch_device);
	const bool pressed = action && !g_h1_devices.action_held;
	g_h1_devices.action_held = action;
	if (!unit || unit->object.parent_object_index != NONE)
	{
		return;
	}

	// player_examine_nearby_objects: the controls whose bounding spheres meet the unit's, player_examine_nearby_device: the one its
	// aim meets from its front that can change
	real_point3d camera;
	unit_get_camera_position(unit_index, &camera);
	for (auto& entry : g_h1_devices.devices)
	{
		s_h1_device* device = &entry.second;
		const object_datum* control = device->h1_object_type == _h1_object_type_control ? object_try_and_get(entry.first) : NULL;
		if (!control || distance3d(&control->object.center, &unit->object.center) > control->object.radius + unit->object.radius)
		{
			continue;
		}
		real_vector3d to_center;
		vector_from_points3d(&camera, &control->object.center, &to_center);
		const real32 c = magnitude_squared3d(&to_center) - control->object.radius * control->object.radius;
		const real32 b = -dot_product3d(&unit->unit.aiming_vector, &to_center);
		const bool aimed = c < 0.f || (b < 0.f && b * b - magnitude_squared3d(&unit->unit.aiming_vector) * c > 0.f);
		if (!aimed || !h1_device_frontfacing(entry.first, device, &unit->unit.aiming_vector) || !h1_device_can_change_position(device))
		{
			continue;
		}

		// hud.c's touch device message: the control's custom name, or its definition's icon text
		const uint8* definition = (const uint8*)g_h1_cache_file->tag_get('ctrl', device->h1_definition_index);
		std::wstring name;
		const uint8* hud_globals = NULL;
		const h1_scnr* scenario = g_h1_cache_file->scenario_get();
		const datum names_index = device->custom_name_index != NONE ? scenario->custom_object_names.index : NONE;
		int16 string_index = device->custom_name_index;
		if (names_index == NONE && definition)
		{
			// the hud globals' alternate icon text
			const h1_matg* globals = (const h1_matg*)g_h1_cache_file->tag_get('matg', g_h1_cache_file->tag_find('matg', "globals\\globals"));
			const h1_matg_interface_bitmaps* interface_bitmaps = globals ? g_h1_cache_file->block_get(globals->interface_bitmaps, 0) : NULL;
			hud_globals = interface_bitmaps ? (const uint8*)g_h1_cache_file->tag_get('hudg', interface_bitmaps->hud_globals.index) : NULL;
			string_index = *(const int16*)(definition + 0x13C);
		}
		const datum list_index = names_index != NONE ? names_index : hud_globals ? ((const h1_tag_reference*)(hud_globals + 0xB4))->index : NONE;
		const h1_tag_block<h1_tag_data>* strings = list_index != NONE ? (const h1_tag_block<h1_tag_data>*)g_h1_cache_file->tag_get('ustr', list_index) : NULL;
		const h1_tag_data* string = strings && VALID_INDEX(string_index, strings->count) ? g_h1_cache_file->block_get(*strings, string_index) : NULL;
		const wchar_t* text = string ? (const wchar_t*)g_h1_cache_file->data_get(*string) : NULL;
		if (text)
		{
			name.assign(text, wcsnlen(text, string->size / sizeof(wchar_t)));
		}
		h1_hud_set_state_message((int16)(device->custom_name_index != NONE ? _h1_hud_message_custom_device : _h1_hud_message_touch_device), name.c_str());

		// player_handle_action: device_touched, control_touched (a control that triggers when touched)
		if (pressed && definition && ((const s_h1_control_definition*)(definition + k_h1_control_definition_offset))->triggers_when == 0)
		{
			h1_control_toggle(entry.first, device);
		}
		break;
	}
	return;
}

real32 h1_device_group_get(int16 group_index)
{
	return VALID_INDEX(group_index, (int16)g_h1_devices.groups.size()) ? g_h1_devices.groups[group_index].actual_value : 0.f;
}

bool h1_device_group_set(int16 group_index, real32 value)
{
	if (!VALID_INDEX(group_index, (int16)g_h1_devices.groups.size()))
	{
		return false;
	}
	value = PIN(value, 0.f, 1.f);
	s_h1_device_group* group = &g_h1_devices.groups[group_index];
	if (group->actual_value == value ||
		(TEST_BIT(group->flags, _h1_device_group_can_change_only_once_bit) && TEST_BIT(group->flags, _h1_device_group_changed_once_bit)))
	{
		return false;
	}
	group->actual_value = value;
	SET_BIT(group->flags, _h1_device_group_changed_once_bit, true);
	for (auto& entry : g_h1_devices.devices)
	{
		const s_h1_device_definition* definition = h1_device_definition_get(&entry.second);
		if (entry.second.power_group_index == group_index && definition)
		{
			h1_device_effect_new(entry.first, value > 0.f ? definition->repowered_effect.index : definition->depowered_effect.index);
		}
	}
	return true;
}

void h1_device_group_set_immediate(int16 group_index, real32 value)
{
	if (!VALID_INDEX(group_index, (int16)g_h1_devices.groups.size()))
	{
		return;
	}
	value = PIN(value, 0.f, 1.f);
	g_h1_devices.groups[group_index].actual_value = value;
	for (auto& entry : g_h1_devices.devices)
	{
		s_h1_device* device = &entry.second;
		if (device->power_group_index == group_index)
		{
			SET_BIT(device->flags, _h1_device_position_changed_bit, true);
			device->power = value;
			device->power_velocity = 0.f;
		}
		if (device->position_group_index == group_index)
		{
			SET_BIT(device->flags, _h1_device_position_changed_bit, true);
			device->position = value;
			device->position_velocity = 0.f;
		}
	}
	return;
}

real32 h1_device_get_position(datum object_index)
{
	const s_h1_device* device = h1_device_get(object_index);
	return device ? device->position : 0.f;
}

real32 h1_device_get_power(datum object_index)
{
	const s_h1_device* device = h1_device_get(object_index);
	return device ? device->power : 0.f;
}

bool h1_device_set_position(datum object_index, real32 position)
{
	const s_h1_device* device = h1_device_get(object_index);
	return device && device->position_group_index != NONE ? h1_device_group_set(device->position_group_index, position) : false;
}

void h1_device_set_position_immediate(datum object_index, real32 position)
{
	const s_h1_device* device = h1_device_get(object_index);
	if (device && device->position_group_index != NONE)
	{
		h1_device_group_set_immediate(device->position_group_index, position);
	}
	return;
}

void h1_device_set_power(datum object_index, real32 power)
{
	s_h1_device* device = h1_device_get(object_index);
	if (device)
	{
		SET_BIT(device->flags, _h1_device_position_changed_bit, true);
		device->power = power;
		h1_device_group_set(device->power_group_index, power);
	}
	return;
}

void h1_device_set_never_appears_locked(datum object_index, bool never_appears_locked)
{
	s_h1_device* device = h1_device_get(object_index);
	if (device && device->h1_object_type == _h1_object_type_machine)
	{
		SET_BIT(device->machine_flags, _h1_machine_never_appears_locked_bit, never_appears_locked);
	}
	return;
}

void h1_device_one_sided_set(datum object_index, bool one_sided)
{
	s_h1_device* device = h1_device_get(object_index);
	if (device && device->h1_object_type == _h1_object_type_machine)
	{
		SET_BIT(device->machine_flags, _h1_machine_one_sided_bit, one_sided);
	}
	return;
}

void h1_device_operates_automatically_set(datum object_index, bool operates_automatically)
{
	s_h1_device* device = h1_device_get(object_index);
	if (device && device->h1_object_type == _h1_object_type_machine)
	{
		SET_BIT(device->machine_flags, _h1_machine_does_not_operate_automatically_bit, !operates_automatically);
	}
	return;
}

void h1_device_group_change_only_once_more_set(int16 group_index, bool change_only_once_more)
{
	if (VALID_INDEX(group_index, (int16)g_h1_devices.groups.size()))
	{
		s_h1_device_group* group = &g_h1_devices.groups[group_index];
		SET_BIT(group->flags, _h1_device_group_can_change_only_once_bit, change_only_once_more);
		SET_BIT(group->flags, _h1_device_group_changed_once_bit, false);
	}
	return;
}

void h1_devices_apply_patches(void)
{
	// the device shells are scenery, or machines when they collide
	object_type_definition* scenery_type = object_type_definition_get(_object_type_scenery);
	for (int32 i = 0; i < k_max_object_type_inheritence; i++)
	{
		object_type_definition* part = scenery_type->part_definitions[i];
		if (part && part->group_tag == 'scen')
		{
			g_h2_scenery_preprocess_node_orientations = part->object_preprocess_node_orientations;
			part->object_preprocess_node_orientations = h1_device_scenery_preprocess_node_orientations_hook;
			break;
		}
	}
	object_type_definition* machine_type = object_type_definition_get(_object_type_machine);
	for (int32 i = 0; i < k_max_object_type_inheritence; i++)
	{
		object_type_definition* part = machine_type->part_definitions[i];
		if (part && part->group_tag == 'mach')
		{
			g_h2_device_preprocess_node_orientations = part->object_preprocess_node_orientations;
			part->object_preprocess_node_orientations = h1_device_machine_preprocess_node_orientations_hook;
			break;
		}
	}
	return;
}

/* private code */

// devices.c device_can_change_position: its position group may change (once if only once), it's usable, it's powered
static bool h1_device_can_change_position(const s_h1_device* device)
{
	if (!VALID_INDEX(device->position_group_index, (int16)g_h1_devices.groups.size()))
	{
		return false;
	}
	const s_h1_device_group* position_group = &g_h1_devices.groups[device->position_group_index];
	if (TEST_BIT(position_group->flags, _h1_device_group_can_change_only_once_bit) && TEST_BIT(position_group->flags, _h1_device_group_changed_once_bit))
	{
		return false;
	}
	if (TEST_BIT(device->flags, _h1_device_not_usable_bit))
	{
		return false;
	}
	return h1_device_group_get(device->power_group_index) == 1.f;
}

// devices.c device_frontfacing: a control usable from one side faces the one aiming at it when its front marker doesn't face away
static bool h1_device_frontfacing(datum object_index, const s_h1_device* device, const real_vector3d* facing)
{
	if (TEST_BIT(device->control_flags, _h1_control_usable_from_both_sides_bit))
	{
		return true;
	}
	object_marker marker;
	if (object_get_markers_by_string_id(object_index, string_id_find_or_add("front"), &marker, 1) != 1)
	{
		return true;
	}
	return !(dot_product3d(facing, &marker.matrix.forward) > 0.f);
}

// device_controls.c control_toggle: the control's type sets its position group, its effect the on, off or denied one
static void h1_control_toggle(datum object_index, s_h1_device* device)
{
	const uint8* definition = (const uint8*)g_h1_cache_file->tag_get('ctrl', device->h1_definition_index);
	if (!definition || !VALID_INDEX(device->position_group_index, (int16)g_h1_devices.groups.size()))
	{
		return;
	}
	const s_h1_control_definition* control = (const s_h1_control_definition*)(definition + k_h1_control_definition_offset);
	real32 desired_value;
	switch (control->type)
	{
	case 0: desired_value = g_h1_devices.groups[device->position_group_index].actual_value > 0.5f ? 0.f : 1.f; break;
	case 1: desired_value = 1.f; break;
	case 2: desired_value = 0.f; break;
	default: desired_value = control->call_value; break;
	}
	if (h1_device_group_set(device->position_group_index, desired_value))
	{
		h1_device_effect_new(object_index, desired_value > 0.5f ? control->on_effect.index : control->off_effect.index);
	}
	else
	{
		h1_device_effect_new(object_index, control->denied_effect.index);
	}
	return;
}

static const s_h1_device_definition* h1_device_definition_get(const s_h1_device* device)
{
	const uint8* definition = device->h1_definition_index != NONE ? (const uint8*)g_h1_cache_file->tag_get('obje', device->h1_definition_index) : NULL;
	return definition ? (const s_h1_device_definition*)(definition + k_h1_device_definition_offset) : NULL;
}

static const s_h1_machine_definition* h1_machine_definition_get(const s_h1_device* device)
{
	const uint8* definition = device->h1_object_type == _h1_object_type_machine && device->h1_definition_index != NONE ?
		(const uint8*)g_h1_cache_file->tag_get('obje', device->h1_definition_index) : NULL;
	return definition ? (const s_h1_machine_definition*)(definition + k_h1_machine_definition_offset) : NULL;
}

static s_h1_device* h1_device_get(datum object_index)
{
	auto found = g_h1_devices.devices.find(object_index);
	return found != g_h1_devices.devices.end() ? &found->second : NULL;
}

static int16 h1_device_group_new(real32 initial_value, uint16 flags)
{
	g_h1_devices.groups.push_back({ flags, initial_value });
	return (int16)(g_h1_devices.groups.size() - 1);
}

// device_update
static void h1_device_tick(datum object_index, s_h1_device* device)
{
	const s_h1_device_definition* definition = h1_device_definition_get(device);
	if (!definition)
	{
		return;
	}
	if (device->power_group_index != NONE)
	{
		const s_h1_device_group* group = &g_h1_devices.groups[device->power_group_index];
		if (group->actual_value != device->power || device->power_velocity != 0.f)
		{
			const real32 old_power = device->power;
			h1_accelerate_to_position(&device->power, &device->power_velocity, group->actual_value, definition->runtime_maximum_power_acceleration,
				definition->runtime_maximum_power_velocity, 0.f, 1.f, false);
			if (old_power != device->power)
			{
				SET_BIT(device->flags, _h1_device_position_changed_bit, true);
			}
		}
	}
	if (device->position_group_index == NONE)
	{
		return;
	}
	const s_h1_device_group* group = &g_h1_devices.groups[device->position_group_index];
	if (group->actual_value == device->position && device->position_velocity == 0.f)
	{
		device->delay_ticks = 0;
		return;
	}
	const real32 maximum_acceleration = (1.f - device->power) * definition->runtime_maximum_depowered_position_acceleration +
		device->power * definition->runtime_maximum_powered_position_acceleration;
	const real32 maximum_velocity = (1.f - device->power) * definition->runtime_maximum_depowered_position_velocity +
		device->power * definition->runtime_maximum_powered_position_velocity;
	const bool moving_forward = device->position_velocity > 0.f;
	if (device->delay_ticks < definition->runtime_delay_ticks && device->position == 0.f && !(group->actual_value < device->position))
	{
		if (++device->delay_ticks == 1)
		{
			h1_device_effect_new(object_index, definition->delay_effect.index);
		}
		return;
	}
	const real32 old_velocity = device->position_velocity;
	const real32 old_position = device->position;
	if (fabsf(device->position_velocity) > maximum_velocity)
	{
		device->position_velocity = moving_forward ? maximum_velocity : -maximum_velocity;
	}
	if (h1_accelerate_to_position(&device->position, &device->position_velocity, group->actual_value, maximum_acceleration, maximum_velocity, 0.f, 1.f,
		TEST_BIT(definition->flags, _h1_device_position_loops_bit)))
	{
		h1_device_effect_new(object_index, moving_forward ? definition->positive_stop_effect.index : definition->negative_stop_effect.index);
	}
	else if (device->position_velocity != 0.f && old_velocity * device->position_velocity <= 0.f)
	{
		h1_device_effect_new(object_index, device->position_velocity > old_velocity ? definition->positive_start_effect.index : definition->negative_start_effect.index);
	}
	if (old_position != device->position)
	{
		SET_BIT(device->flags, _h1_device_position_changed_bit, true);
	}
	return;
}

// machine_update: platforms run around their position, doors open for the bipeds near them and close after their open time
static void h1_machine_tick(datum object_index, s_h1_device* device)
{
	const s_h1_device_definition* definition = h1_device_definition_get(device);
	const s_h1_machine_definition* machine = h1_machine_definition_get(device);
	const object_datum* object = object_try_and_get(object_index);
	if (!definition || !machine || !object)
	{
		return;
	}
	if (machine->type == _h1_machine_type_platform)
	{
		device->position += (1.f - device->power) * definition->runtime_maximum_depowered_position_velocity + device->power * definition->runtime_maximum_powered_position_velocity;
		if (device->position >= 1.f)
		{
			device->position -= 1.f;
		}
		device->position_velocity = 0.f;
		SET_BIT(device->flags, _h1_device_position_changed_bit, true);
		if (device->position_group_index != NONE)
		{
			g_h1_devices.groups[device->position_group_index].actual_value = device->position;
		}
	}

	if (!TEST_BIT(device->machine_flags, _h1_machine_does_not_operate_automatically_bit) && machine->type == _h1_machine_type_door &&
		((g_h1_devices.tick + DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index)) & 3) == 0)
	{
		const real32 radius = definition->automatic_activation_radius < k_real_epsilon ? object->object.radius : definition->automatic_activation_radius;
		bool activate = false;
		c_object_iterator<object_datum> iterator;
		iterator.begin(FLAG(_object_type_biped), 0);
		while (iterator.next())
		{
			// objects_in_sphere: the bipeds whose bounding spheres meet the activation sphere
			const object_datum* biped = iterator.get_datum();
			if (distance3d(&biped->object.center, &object->object.center) > radius + biped->object.radius)
			{
				continue;
			}
			// a unit that can't open doors automatically (its unit definition's flag)
			const datum h1_biped_index = h1_objects_h1_definition_get(biped->definition_index);
			const uint8* h1_biped = h1_biped_index != NONE ? (const uint8*)g_h1_cache_file->tag_get('unit', h1_biped_index) : NULL;
			if (h1_biped && TEST_BIT(*(const uint32*)(h1_biped + 0x17C), 14))
			{
				continue;
			}
			// one sided doors stay shut from behind while closed, for the player's friends
			real_vector3d offset;
			vector_from_points3d(&object->object.center, &biped->object.center, &offset);
			const unit_datum* unit = (const unit_datum*)biped;
			const bool friend_of_player = unit->unit.unit_team == _game_team_player || unit->unit.unit_team == _game_team_human;
			if (TEST_BIT(device->machine_flags, _h1_machine_one_sided_bit) && device->position == 0.f && friend_of_player &&
				dot_product3d(&offset, &object->object.forward) > 0.f)
			{
				continue;
			}
			activate = true;
		}
		if (activate)
		{
			if (device->position_group_index != NONE)
			{
				h1_device_group_set(device->position_group_index, 1.f);
			}
			device->door_open_ticks = -3;
		}
	}
	if (machine->type == _h1_machine_type_door)
	{
		if (device->position == 1.f)
		{
			if (++device->door_open_ticks > machine->runtime_door_open_ticks && device->position_group_index != NONE)
			{
				h1_device_group_set(device->position_group_index, 0.f);
			}
		}
		else
		{
			device->door_open_ticks = 0;
		}
	}
	return;
}

// machine_update's elevator: the bipeds standing on an elevator (their ground below is it, bipeds.c's elevator object) move with its
// elevator node
static void h1_machine_elevator_update(datum object_index, s_h1_device* device)
{
	const s_h1_machine_definition* machine = h1_machine_definition_get(device);
	const object_datum* object = object_try_and_get(object_index);
	if (!machine || !object || !TEST_BIT(machine->flags, _h1_machine_definition_is_elevator_bit) || machine->elevator_node_index == NONE)
	{
		return;
	}
	const real_matrix4x3* node_matrix = object_get_node_matrix(object_index, machine->elevator_node_index);
	if (!node_matrix)
	{
		return;
	}
	real_vector3d offset = *global_zero_vector3d;
	if (device->elevator_position_valid)
	{
		vector_from_points3d(&device->elevator_position, &node_matrix->position, &offset);
	}
	device->elevator_position = node_matrix->position;
	device->elevator_position_valid = true;
	if (offset.i == 0.f && offset.j == 0.f && offset.k == 0.f)
	{
		return;
	}
	c_object_iterator<object_datum> iterator;
	iterator.begin(FLAG(_object_type_biped), 0);
	while (iterator.next())
	{
		const object_datum* biped = iterator.get_datum();
		if (biped->object.parent_object_index != NONE ||
			distance3d(&biped->object.center, &object->object.center) > object->object.radius + biped->object.radius)
		{
			continue;
		}
		// standing on it: the ground just below the biped (where it was) is the elevator (where it is now)
		const real_point3d start = { biped->object.position.x, biped->object.position.y, biped->object.position.z + 0.25f + MAX(offset.k, 0.f) };
		const real_vector3d probe = { 0.f, 0.f, -0.75f - MAX(offset.k, 0.f) + MIN(offset.k, 0.f) };
		collision_result collision;
		const uint32 flags = FLAG(_collision_test_objects_bit) | FLAG(4 + _object_type_machine);
		if (!collision_test_vector(flags, &start, &probe, iterator.get_index(), NONE, &collision) || collision.object_index != object_index)
		{
			continue;
		}
		real_point3d position;
		point_from_line3d(&biped->object.position, &offset, 1.f, &position);
		Memory::GetAddress<void(__cdecl*)(datum, const real_point3d*, const real_vector3d*, const real_vector3d*, int32)>(0x136B7F)(
			iterator.get_index(), &position, NULL, NULL, 0);
	}
	return;
}

// device_effect_new: an effect from the device or a sound at it
static void h1_device_effect_new(datum object_index, datum effect_index)
{
	if (effect_index == NONE)
	{
		return;
	}
	const h1_cache_file_tag_instance* instance = g_h1_cache_file->tag_instance_get(effect_index);
	if (!instance)
	{
		return;
	}
	if (instance->group_tag == 'snd!')
	{
		real_point3d position;
		object_get_origin(object_index, &position, false);
		h1_sound_impulse(effect_index, &position, 1.f);
	}
	else if (instance->group_tag == 'effe')
	{
		h1_effect_new_on_object(effect_index, object_index);
	}
	return;
}

// real_math.c accelerate_to_position (devices pass their maximum acceleration as its maximum velocity and their maximum velocity as
// its acceleration)
static bool h1_accelerate_to_position(real32* position, real32* velocity, real32 target_position, real32 maximum_velocity, real32 acceleration,
	real32 minimum_position, real32 maximum_position, bool periodic)
{
	real32 current_velocity = *velocity;
	real32 delta = target_position - *position;
	if (periodic)
	{
		const real32 half_range = (maximum_position - minimum_position) * 0.5f;
		if (delta > half_range)
		{
			delta -= half_range + half_range;
		}
		else if (delta < -half_range)
		{
			delta += half_range + half_range;
		}
	}
	const real32 limit = MIN(maximum_velocity, acceleration);
	if (fabsf(delta - current_velocity) <= limit)
	{
		*velocity = 0.f;
		*position = PIN(target_position, minimum_position, maximum_position);
		return true;
	}
	const real32 braking_distance = (maximum_velocity + maximum_velocity) * fabsf(delta);
	real32 speed = braking_distance >= acceleration * acceleration ? acceleration : sqrtf(braking_distance);
	if (delta < 0.f)
	{
		speed = -speed;
	}
	const real32 step = PIN(speed - current_velocity, -maximum_velocity, maximum_velocity);
	current_velocity += step;
	real32 new_position = step * 0.5f + current_velocity + *position;
	if (periodic)
	{
		if (new_position < minimum_position)
		{
			new_position += maximum_position - minimum_position;
		}
		else if (new_position > maximum_position)
		{
			new_position -= maximum_position - minimum_position;
		}
	}
	*velocity = current_velocity;
	*position = PIN(new_position, minimum_position, maximum_position);
	return false;
}

static void h1_device_scenery_preprocess_node_orientations_hook(datum object_index, uint8* node_flags, int32 node_count, real_orientation* orientations)
{
	if (g_h2_scenery_preprocess_node_orientations)
	{
		g_h2_scenery_preprocess_node_orientations(object_index, node_flags, node_count, orientations);
	}
	h1_device_preprocess_node_orientations_hook(object_index, node_flags, node_count, orientations);
	return;
}

static void h1_device_machine_preprocess_node_orientations_hook(datum object_index, uint8* node_flags, int32 node_count, real_orientation* orientations)
{
	if (g_h2_device_preprocess_node_orientations)
	{
		g_h2_device_preprocess_node_orientations(object_index, node_flags, node_count, orientations);
	}
	h1_device_preprocess_node_orientations_hook(object_index, node_flags, node_count, orientations);
	return;
}

// device_preprocess_node_orientations: the position animation at the device's position, the power animation at its power
static void h1_device_preprocess_node_orientations_hook(datum object_index, uint8* node_flags, int32 node_count, real_orientation* orientations)
{
	const s_h1_device* device = h1_maps_active() ? h1_device_get(object_index) : NULL;
	const s_h1_device_definition* definition = device ? h1_device_definition_get(device) : NULL;
	const h1_scen* h1_object = device ? (const h1_scen*)g_h1_cache_file->tag_get('obje', device->h1_definition_index) : NULL;
	if (!definition || !h1_object || h1_object->animation_graph.index == NONE)
	{
		return;
	}
	const h1_antr* graph = (const h1_antr*)g_h1_cache_file->tag_get('antr', h1_object->animation_graph.index);
	const h1_antr_devices* device_animations = graph && graph->devices.count > 0 ? g_h1_cache_file->block_get(graph->devices, 0) : NULL;
	if (!device_animations)
	{
		return;
	}
	const bool loops = TEST_BIT(definition->flags, _h1_device_position_loops_bit);
	if (device_animations->animations.count > _h1_device_animation_position)
	{
		const int16 animation_index = g_h1_cache_file->block_get(device_animations->animations, _h1_device_animation_position)->animation_index;
		if (VALID_INDEX(animation_index, graph->animations.count))
		{
			const h1_antr_animations* animation = g_h1_cache_file->block_get(graph->animations, animation_index);
			const real32 position = TEST_BIT(device->flags, _h1_device_position_reversed_bit) ? 1.f - device->position : device->position;
			const real32 frame = position * (real32)(loops ? animation->frame_count : animation->frame_count - 1);
			h1_animation_device_apply(h1_object->animation_graph.index, _h1_device_animation_position, frame, loops,
				!TEST_BIT(definition->flags, _h1_device_position_animation_not_interpolated_bit), orientations, node_count);
		}
	}
	if (device_animations->animations.count > _h1_device_animation_power)
	{
		const int16 animation_index = g_h1_cache_file->block_get(device_animations->animations, _h1_device_animation_power)->animation_index;
		if (VALID_INDEX(animation_index, graph->animations.count))
		{
			const h1_antr_animations* animation = g_h1_cache_file->block_get(graph->animations, animation_index);
			h1_animation_device_apply(h1_object->animation_graph.index, _h1_device_animation_power, (real32)animation->frame_count * device->power, false, true,
				orientations, node_count);
		}
	}
	return;
}

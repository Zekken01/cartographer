#include "stdafx.h"
#include "h1_devices.h"

#include "h1_animations.h"
#include "h1_cache_file.h"
#include "h1_effects.h"
#include "h1_log.h"
#include "h1_map_loader.h"
#include "h1_objects.h"
#include "h1_sound.h"

#include "game/game_time.h"
#include "math/real_math.h"
#include "objects/object_types.h"
#include "objects/objects.h"

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
};

struct s_h1_devices_globals
{
	std::vector<s_h1_device_group> groups;
	std::unordered_map<datum, s_h1_device> devices;
	real32 leftover_ticks;
	int32 last_game_time;
};

/* globals */

static s_h1_devices_globals g_h1_devices;
static object_preprocess_node_orientations_t g_h2_scenery_preprocess_node_orientations = NULL;

/* prototypes */

static const s_h1_device_definition* h1_device_definition_get(const s_h1_device* device);
static const s_h1_machine_definition* h1_machine_definition_get(const s_h1_device* device);
static s_h1_device* h1_device_get(datum object_index);
static int16 h1_device_group_new(real32 initial_value, uint16 flags);
static void h1_device_tick(datum object_index, s_h1_device* device);
static void h1_machine_tick(datum object_index, s_h1_device* device);
static void h1_device_effect_new(datum object_index, datum effect_index);
static bool h1_accelerate_to_position(real32* position, real32* velocity, real32 target_position, real32 maximum_velocity, real32 acceleration,
	real32 minimum_position, real32 maximum_position, bool periodic);
static void h1_device_preprocess_node_orientations_hook(datum object_index, uint8* node_flags, int32 node_count, real_orientation* orientations);

/* public code */

void h1_devices_reset(void)
{
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
			object_compute_node_matrices_with_children(entry.first);
		}
	}
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
	object_type_definition* scenery_type = object_type_definition_get(_object_type_scenery);
	for (int32 i = 0; i < k_max_object_type_inheritence; i++)
	{
		object_type_definition* part = scenery_type->part_definitions[i];
		if (part && part->group_tag == 'scen')
		{
			g_h2_scenery_preprocess_node_orientations = part->object_preprocess_node_orientations;
			part->object_preprocess_node_orientations = h1_device_preprocess_node_orientations_hook;
			break;
		}
	}
	return;
}

/* private code */

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

	static int32 s_tick = 0;
	s_tick++;
	if (!TEST_BIT(device->machine_flags, _h1_machine_does_not_operate_automatically_bit) && machine->type == _h1_machine_type_door &&
		((s_tick + DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index)) & 3) == 0)
	{
		const real32 radius = definition->automatic_activation_radius < k_real_epsilon ? object->object.radius : definition->automatic_activation_radius;
		bool activate = false;
		c_object_iterator<object_datum> iterator;
		iterator.begin(FLAG(_object_type_biped), 0);
		while (iterator.next())
		{
			const object_datum* biped = iterator.get_datum();
			if (distance_squared3d(&biped->object.center, &object->object.center) > radius * radius)
			{
				continue;
			}
			// one sided doors stay shut from behind while closed
			real_vector3d offset;
			vector_from_points3d(&object->object.center, &biped->object.center, &offset);
			if (TEST_BIT(device->machine_flags, _h1_machine_one_sided_bit) && device->position == 0.f && dot_product3d(&offset, &object->object.forward) > 0.f)
			{
				continue;
			}
			activate = true;
			break;
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

// device_preprocess_node_orientations: the position animation at the device's position, the power animation at its power
static void h1_device_preprocess_node_orientations_hook(datum object_index, uint8* node_flags, int32 node_count, real_orientation* orientations)
{
	if (g_h2_scenery_preprocess_node_orientations)
	{
		g_h2_scenery_preprocess_node_orientations(object_index, node_flags, node_count, orientations);
	}
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

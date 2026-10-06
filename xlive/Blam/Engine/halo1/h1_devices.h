#pragma once

/*
* Halo 1 devices (devices.c, device_machines.c) on the scenario's device shells: device groups, power and position seeking at
* halo 1's tick, machines (doors opening for the bipeds near them, platforms), the position and power animations and function
* inputs, and the scripts' device functions.
*/

// the scenario's device groups at their initial values, no devices (before the scenario's objects are placed)
void h1_devices_reset(void);

// device_add_scenario_information and machine_place: a device placed from its scenario placement (machines, controls and light
// fixtures share the device part at +0x28)
void h1_device_place(datum object_index, int16 h1_object_type, const void* placement);

// device_update and machine_update for every device, at halo 1's 30 ticks a second
void h1_devices_update(void);

// device_export_function_values: the device's a to d inputs (nothing for other objects)
void h1_device_functions_export(datum object_index, real32* incoming);

real32 h1_device_group_get(int16 group_index);
// device_group_set_desired_value (true when it changed)
bool h1_device_group_set(int16 group_index, real32 value);
// device_group_set_actual_value: its devices jump there
void h1_device_group_set_immediate(int16 group_index, real32 value);

real32 h1_device_get_position(datum object_index);
real32 h1_device_get_power(datum object_index);
bool h1_device_set_position(datum object_index, real32 position);
void h1_device_set_position_immediate(datum object_index, real32 position);
void h1_device_set_power(datum object_index, real32 power);
void h1_device_set_never_appears_locked(datum object_index, bool never_appears_locked);
void h1_device_one_sided_set(datum object_index, bool one_sided);
void h1_device_operates_automatically_set(datum object_index, bool operates_automatically);
void h1_device_group_change_only_once_more_set(int16 group_index, bool change_only_once_more);

// players.c player_examine_nearby_device and player_handle_action's touch device, device_controls.c control_touched: the local
// player's control it aims at shows hud.c's touch device message, the action button toggles it
void h1_controls_player_update(void);

// the scenery object type's node orientations: a device's position and power animations
void h1_devices_apply_patches(void);

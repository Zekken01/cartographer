#pragma once
#pragma pack(push, 1)

struct h1_scnr_skies
{
	h1_tag_reference sky; // 0x0
};
ASSERT_STRUCT_SIZE(h1_scnr_skies, 0x10);

struct h1_scnr_child_scenarios
{
	h1_tag_reference child_scenario; // 0x0
	int8 pad_10[16];
};
ASSERT_STRUCT_SIZE(h1_scnr_child_scenarios, 0x20);

struct h1_scnr_predicted_resources
{
	int16 type; // 0x0
	int16 resource_index; // 0x2
	datum tag_index; // 0x4
};
ASSERT_STRUCT_SIZE(h1_scnr_predicted_resources, 0x8);

struct h1_scnr_functions
{
	uint32 flags; // 0x0
	char name[32]; // 0x4
	real32 period; // 0x24
	int16 scale_period_by_index; // 0x28
	int16 function; // 0x2a
	int16 scale_function_by_index; // 0x2c
	int16 wobble_function; // 0x2e
	real32 wobble_period; // 0x30
	real32 wobble_magnitude; // 0x34
	real32 square_wave_threshold; // 0x38
	int16 step_count; // 0x3c
	int16 map_to; // 0x3e
	int16 sawtooth_count; // 0x40
	int16 unknown; // 0x42
	int16 scale_result_by_index; // 0x44
	int16 bounds_mode; // 0x46
	real_bounds bounds; // 0x48
	int8 pad_50[4];
	int16 unknown_2; // 0x54
	int16 turn_off_with_index; // 0x56
	int8 pad_58[32];
};
ASSERT_STRUCT_SIZE(h1_scnr_functions, 0x78);

struct h1_scnr_comments
{
	real_point3d position; // 0x0
	int8 pad_c[16];
	h1_tag_data comment; // 0x1c
};
ASSERT_STRUCT_SIZE(h1_scnr_comments, 0x30);

struct h1_scnr_object_names
{
	char name[32]; // 0x0
	int16 type; // 0x20
	int16 placement_index; // 0x22
};
ASSERT_STRUCT_SIZE(h1_scnr_object_names, 0x24);

struct h1_scnr_scenery
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint16 placement_flags; // 0x4
	int16 desired_permutation; // 0x6
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	uint16 bsp_flags; // 0x20
	int16 unknown; // 0x22
	int8 pad_24[36];
};
ASSERT_STRUCT_SIZE(h1_scnr_scenery, 0x48);

struct h1_scnr_scenery_palette
{
	h1_tag_reference name; // 0x0
	int8 pad_10[32];
};
ASSERT_STRUCT_SIZE(h1_scnr_scenery_palette, 0x30);

struct h1_scnr_bipeds
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint16 placement_flags; // 0x4
	int16 desired_permutation; // 0x6
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	uint16 bsp_flags; // 0x20
	int16 unknown; // 0x22
	int8 pad_24[36];
	real32 body_vitality_percentage; // 0x48
	uint32 flags; // 0x4c
	int8 pad_50[40];
};
ASSERT_STRUCT_SIZE(h1_scnr_bipeds, 0x78);

struct h1_scnr_biped_palette
{
	h1_tag_reference name; // 0x0
	int8 pad_10[32];
};
ASSERT_STRUCT_SIZE(h1_scnr_biped_palette, 0x30);

struct h1_scnr_vehicles
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint16 placement_flags; // 0x4
	int16 desired_permutation; // 0x6
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	uint16 bsp_flags; // 0x20
	int16 unknown; // 0x22
	int8 pad_24[36];
	real32 body_vitality_percentage; // 0x48
	uint32 flags; // 0x4c
	int8 pad_50[8];
	int8 multiplayer_team_index; // 0x58
	int8 unknown_2; // 0x59
	uint16 multiplayer_spawn_flags; // 0x5a
	int8 pad_5c[28];
};
ASSERT_STRUCT_SIZE(h1_scnr_vehicles, 0x78);

struct h1_scnr_vehicle_palette
{
	h1_tag_reference name; // 0x0
	int8 pad_10[32];
};
ASSERT_STRUCT_SIZE(h1_scnr_vehicle_palette, 0x30);

struct h1_scnr_equipment
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint16 placement_flags; // 0x4
	int16 desired_permutation; // 0x6
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	uint16 bsp_flags; // 0x20
	uint16 misc_flags; // 0x22
	int8 pad_24[4];
};
ASSERT_STRUCT_SIZE(h1_scnr_equipment, 0x28);

struct h1_scnr_equipment_palette
{
	h1_tag_reference name; // 0x0
	int8 pad_10[32];
};
ASSERT_STRUCT_SIZE(h1_scnr_equipment_palette, 0x30);

struct h1_scnr_weapons
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint16 placement_flags; // 0x4
	int16 desired_permutation; // 0x6
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	uint16 bsp_flags; // 0x20
	int16 unknown; // 0x22
	int8 pad_24[36];
	int16 rounds_left; // 0x48
	int16 rounds_loaded; // 0x4a
	uint16 weapon_flags; // 0x4c
	int16 unknown_2; // 0x4e
	int8 pad_50[12];
};
ASSERT_STRUCT_SIZE(h1_scnr_weapons, 0x5c);

struct h1_scnr_weapon_palette
{
	h1_tag_reference name; // 0x0
	int8 pad_10[32];
};
ASSERT_STRUCT_SIZE(h1_scnr_weapon_palette, 0x30);

struct h1_scnr_device_groups
{
	char name[32]; // 0x0
	real32 initial_value; // 0x20
	uint32 flags; // 0x24
	int8 pad_28[12];
};
ASSERT_STRUCT_SIZE(h1_scnr_device_groups, 0x34);

struct h1_scnr_machines
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint16 placement_flags; // 0x4
	int16 desired_permutation; // 0x6
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	uint16 bsp_flags; // 0x20
	int16 unknown; // 0x22
	int8 pad_24[4];
	int16 power_group_index; // 0x28
	int16 position_group_index; // 0x2a
	uint32 device_flags; // 0x2c
	uint32 machine_flags; // 0x30
	int8 pad_34[12];
};
ASSERT_STRUCT_SIZE(h1_scnr_machines, 0x40);

struct h1_scnr_machine_palette
{
	h1_tag_reference name; // 0x0
	int8 pad_10[32];
};
ASSERT_STRUCT_SIZE(h1_scnr_machine_palette, 0x30);

struct h1_scnr_controls
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint16 placement_flags; // 0x4
	int16 desired_permutation; // 0x6
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	uint16 bsp_flags; // 0x20
	int16 unknown; // 0x22
	int8 pad_24[4];
	int16 power_group_index; // 0x28
	int16 position_group_index; // 0x2a
	uint32 device_flags; // 0x2c
	uint32 control_flags; // 0x30
	int16 custom_object_name_index; // 0x34
	int16 unknown_2; // 0x36
	int8 pad_38[8];
};
ASSERT_STRUCT_SIZE(h1_scnr_controls, 0x40);

struct h1_scnr_control_palette
{
	h1_tag_reference name; // 0x0
	int8 pad_10[32];
};
ASSERT_STRUCT_SIZE(h1_scnr_control_palette, 0x30);

struct h1_scnr_light_fixtures
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint16 placement_flags; // 0x4
	int16 desired_permutation; // 0x6
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	uint16 bsp_flags; // 0x20
	int16 unknown; // 0x22
	int8 pad_24[4];
	int16 power_group_index; // 0x28
	int16 position_group_index; // 0x2a
	uint32 device_flags; // 0x2c
	real_rgb_color color; // 0x30
	real32 intensity; // 0x3c
	real32 falloff_angle; // 0x40
	real32 cutoff_angle; // 0x44
	int8 pad_48[16];
};
ASSERT_STRUCT_SIZE(h1_scnr_light_fixtures, 0x58);

struct h1_scnr_light_fixtures_palette
{
	h1_tag_reference name; // 0x0
	int8 pad_10[32];
};
ASSERT_STRUCT_SIZE(h1_scnr_light_fixtures_palette, 0x30);

struct h1_scnr_sound_scenery
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint16 placement_flags; // 0x4
	int16 desired_permutation; // 0x6
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	uint16 bsp_flags; // 0x20
	int16 unknown; // 0x22
	int8 pad_24[4];
};
ASSERT_STRUCT_SIZE(h1_scnr_sound_scenery, 0x28);

struct h1_scnr_sound_scenery_palette
{
	h1_tag_reference name; // 0x0
	int8 pad_10[32];
};
ASSERT_STRUCT_SIZE(h1_scnr_sound_scenery_palette, 0x30);

struct h1_scnr_player_starting_profile
{
	char name[32]; // 0x0
	real32 starting_health_modifier; // 0x20
	real32 starting_shield_modifier; // 0x24
	h1_tag_reference primary_weapon; // 0x28
	int16 primary_rounds_loaded; // 0x38
	int16 primary_rounds_total; // 0x3a
	h1_tag_reference secondary_weapon; // 0x3c
	int16 secondary_rounds_loaded; // 0x4c
	int16 secondary_rounds_total; // 0x4e
	int8 starting_fragmentation_grenade_count; // 0x50
	int8 starting_plasma_grenade_count; // 0x51
	int8 starting_grenade_2_count; // 0x52
	int8 starting_grenade_3_count; // 0x53
	int8 pad_54[20];
};
ASSERT_STRUCT_SIZE(h1_scnr_player_starting_profile, 0x68);

struct h1_scnr_player_starting_locations
{
	real_point3d position; // 0x0
	real32 facing; // 0xc
	int16 team_index; // 0x10
	int16 bsp_index; // 0x12
	int16 type_0; // 0x14
	int16 type_1; // 0x16
	int16 type_2; // 0x18
	int16 type_3; // 0x1a
	int8 pad_1c[24];
};
ASSERT_STRUCT_SIZE(h1_scnr_player_starting_locations, 0x34);

struct h1_scnr_trigger_volumes
{
	int16 runtime_unknown; // 0x0
	int16 unknown; // 0x2
	char name[32]; // 0x4
	real32 parameters_0; // 0x24
	real32 parameters_1; // 0x28
	real32 parameters_2; // 0x2c
	real_vector3d forward; // 0x30
	real_vector3d up; // 0x3c
	real_point3d position; // 0x48
	real_point3d extents; // 0x54
};
ASSERT_STRUCT_SIZE(h1_scnr_trigger_volumes, 0x60);

struct h1_scnr_recorded_animations
{
	char name[32]; // 0x0
	int8 version; // 0x20
	int8 raw_animation_data; // 0x21
	int8 unit_control_data_version; // 0x22
	int8 unknown; // 0x23
	int16 length_of_animation; // 0x24
	int16 unknown_2; // 0x26
	int8 pad_28[4];
	h1_tag_data recorded_animation_event_stream; // 0x2c
};
ASSERT_STRUCT_SIZE(h1_scnr_recorded_animations, 0x40);

struct h1_scnr_netgame_flags
{
	real_point3d position; // 0x0
	real32 facing; // 0xc
	int16 type; // 0x10
	int16 usage_id; // 0x12
	h1_tag_reference weapon_group; // 0x14
	int8 pad_24[112];
};
ASSERT_STRUCT_SIZE(h1_scnr_netgame_flags, 0x94);

struct h1_scnr_netgame_equipment
{
	uint32 flags; // 0x0
	int16 type_0; // 0x4
	int16 type_1; // 0x6
	int16 type_2; // 0x8
	int16 type_3; // 0xa
	int16 team_index; // 0xc
	int16 spawn_time; // 0xe
	int32 unknown; // 0x10
	int8 pad_14[44];
	real_point3d position; // 0x40
	real32 facing; // 0x4c
	h1_tag_reference item_collection; // 0x50
	int8 pad_60[48];
};
ASSERT_STRUCT_SIZE(h1_scnr_netgame_equipment, 0x90);

struct h1_scnr_starting_equipment
{
	uint32 flags; // 0x0
	int16 type_0; // 0x4
	int16 type_1; // 0x6
	int16 type_2; // 0x8
	int16 type_3; // 0xa
	int8 pad_c[48];
	h1_tag_reference item_collection_1; // 0x3c
	h1_tag_reference item_collection_2; // 0x4c
	h1_tag_reference item_collection_3; // 0x5c
	h1_tag_reference item_collection_4; // 0x6c
	h1_tag_reference item_collection_5; // 0x7c
	h1_tag_reference item_collection_6; // 0x8c
	int8 pad_9c[48];
};
ASSERT_STRUCT_SIZE(h1_scnr_starting_equipment, 0xcc);

struct h1_scnr_bsp_switch_trigger_volumes
{
	int16 trigger_volume_index; // 0x0
	int16 source_index; // 0x2
	int16 destination_index; // 0x4
	int16 runtime_unknown; // 0x6
};
ASSERT_STRUCT_SIZE(h1_scnr_bsp_switch_trigger_volumes, 0x8);

struct h1_scnr_decals
{
	int16 decal_type_index; // 0x0
	int8 yaw_127_127; // 0x2
	int8 pitch_127_127; // 0x3
	real_point3d position; // 0x4
};
ASSERT_STRUCT_SIZE(h1_scnr_decals, 0x10);

struct h1_scnr_decals_palette
{
	h1_tag_reference reference; // 0x0
};
ASSERT_STRUCT_SIZE(h1_scnr_decals_palette, 0x10);

struct h1_scnr_detail_object_collection_palette
{
	h1_tag_reference name; // 0x0
	int8 pad_10[32];
};
ASSERT_STRUCT_SIZE(h1_scnr_detail_object_collection_palette, 0x30);

struct h1_scnr_actor_palette
{
	h1_tag_reference reference; // 0x0
};
ASSERT_STRUCT_SIZE(h1_scnr_actor_palette, 0x10);

struct h1_scnr_encounters_squads_move_positions
{
	real_point3d position; // 0x0
	real32 facing; // 0xc
	real32 weight; // 0x10
	real_bounds time; // 0x14
	int16 animation_index; // 0x1c
	int8 sequence_id; // 0x1e
	int8 unknown; // 0x1f
	int8 pad_20[8];
	int16 cluster_index; // 0x28
	int16 unknown_2; // 0x2a
	int8 pad_2c[32];
	int32 surface_index; // 0x4c
};
ASSERT_STRUCT_SIZE(h1_scnr_encounters_squads_move_positions, 0x50);

struct h1_scnr_encounters_squads_starting_locations
{
	real_point3d position; // 0x0
	real32 facing; // 0xc
	int16 cluster_index; // 0x10
	int8 sequence_id; // 0x12
	uint8 flags; // 0x13
	int16 return_state; // 0x14
	int16 initial_state; // 0x16
	int16 actor_type_index; // 0x18
	int16 command_list_index; // 0x1a
};
ASSERT_STRUCT_SIZE(h1_scnr_encounters_squads_starting_locations, 0x1c);

struct h1_scnr_encounters_squads
{
	char name[32]; // 0x0
	int16 actor_type_index; // 0x20
	int16 platoon_index; // 0x22
	int16 initial_state; // 0x24
	int16 return_state; // 0x26
	uint32 flags; // 0x28
	int16 unique_leader_type; // 0x2c
	int16 unknown; // 0x2e
	int8 pad_30[28];
	int16 unknown_2; // 0x4c
	int16 maneuver_to_squad_index; // 0x4e
	real32 squad_delay_time; // 0x50
	uint32 attacking; // 0x54
	uint32 attacking_search; // 0x58
	uint32 attacking_guard; // 0x5c
	uint32 defending; // 0x60
	uint32 defending_search; // 0x64
	uint32 defending_guard; // 0x68
	uint32 pursuing; // 0x6c
	int8 pad_70[12];
	int16 normal_difficulty_count; // 0x7c
	int16 insane_difficulty_count; // 0x7e
	int16 major_upgrade; // 0x80
	int16 unknown_3; // 0x82
	int16 respawn_minimum_actors; // 0x84
	int16 respawn_maximum_actors; // 0x86
	int16 respawn_total; // 0x88
	int16 unknown_4; // 0x8a
	real_bounds respawn_delay; // 0x8c
	int8 pad_94[48];
	h1_tag_block<h1_scnr_encounters_squads_move_positions> move_positions; // 0xc4
	h1_tag_block<h1_scnr_encounters_squads_starting_locations> starting_locations; // 0xd0
	int8 pad_dc[12];
};
ASSERT_STRUCT_SIZE(h1_scnr_encounters_squads, 0xe8);

struct h1_scnr_encounters_platoons
{
	char name[32]; // 0x0
	uint32 flags; // 0x20
	int8 pad_24[12];
	int16 change_attacking_defending_state_when; // 0x30
	int16 happens_to_index; // 0x32
	int8 pad_34[8];
	int16 maneuver_when; // 0x3c
	int16 happens_to_index_2; // 0x3e
	int8 pad_40[108];
};
ASSERT_STRUCT_SIZE(h1_scnr_encounters_platoons, 0xac);

struct h1_scnr_encounters_firing_positions
{
	real_point3d position; // 0x0
	int16 group_index; // 0xc
	int16 cluster_index; // 0xe
	int8 pad_10[4];
	int32 surface_index; // 0x14
};
ASSERT_STRUCT_SIZE(h1_scnr_encounters_firing_positions, 0x18);

struct h1_scnr_encounters_player_starting_locations
{
	real_point3d position; // 0x0
	real32 facing; // 0xc
	int16 team_index; // 0x10
	int16 bsp_index; // 0x12
	int16 type_0; // 0x14
	int16 type_1; // 0x16
	int16 type_2; // 0x18
	int16 type_3; // 0x1a
	int8 pad_1c[24];
};
ASSERT_STRUCT_SIZE(h1_scnr_encounters_player_starting_locations, 0x34);

struct h1_scnr_encounters
{
	char name[32]; // 0x0
	uint32 flags; // 0x20
	int16 team_index; // 0x24
	int16 one; // 0x26
	int16 search_behavior; // 0x28
	int16 manual_bsp_index; // 0x2a
	real_bounds respawn_delay; // 0x2c
	int8 pad_34[72];
	int16 unknown; // 0x7c
	int16 computed_bsp_index; // 0x7e
	h1_tag_block<h1_scnr_encounters_squads> squads; // 0x80
	h1_tag_block<h1_scnr_encounters_platoons> platoons; // 0x8c
	h1_tag_block<h1_scnr_encounters_firing_positions> firing_positions; // 0x98
	h1_tag_block<h1_scnr_encounters_player_starting_locations> player_starting_locations; // 0xa4
};
ASSERT_STRUCT_SIZE(h1_scnr_encounters, 0xb0);

struct h1_scnr_command_lists_commands
{
	int16 atom_type; // 0x0
	int16 atom_modifier; // 0x2
	real32 parameter1; // 0x4
	real32 parameter2; // 0x8
	int16 point_1_index; // 0xc
	int16 point_2_index; // 0xe
	int16 animation_index; // 0x10
	int16 script_index; // 0x12
	int16 recording_index; // 0x14
	int16 command_index; // 0x16
	int16 object_name_index; // 0x18
	int16 unknown; // 0x1a
	int8 pad_1c[4];
};
ASSERT_STRUCT_SIZE(h1_scnr_command_lists_commands, 0x20);

struct h1_scnr_command_lists_points
{
	real_point3d position; // 0x0
	int32 surface_index; // 0xc
	int8 pad_10[4];
};
ASSERT_STRUCT_SIZE(h1_scnr_command_lists_points, 0x14);

struct h1_scnr_command_lists
{
	char name[32]; // 0x0
	uint32 flags; // 0x20
	int8 pad_24[8];
	int16 manual_bsp_index; // 0x2c
	int16 computed_bsp_index; // 0x2e
	h1_tag_block<h1_scnr_command_lists_commands> commands; // 0x30
	h1_tag_block<h1_scnr_command_lists_points> points; // 0x3c
	int8 pad_48[24];
};
ASSERT_STRUCT_SIZE(h1_scnr_command_lists, 0x60);

struct h1_scnr_ai_animation_references
{
	char animation_name[32]; // 0x0
	h1_tag_reference animation_graph; // 0x20
	int8 pad_30[12];
};
ASSERT_STRUCT_SIZE(h1_scnr_ai_animation_references, 0x3c);

struct h1_scnr_ai_script_references
{
	char script_name[32]; // 0x0
	int8 pad_20[8];
};
ASSERT_STRUCT_SIZE(h1_scnr_ai_script_references, 0x28);

struct h1_scnr_ai_recording_references
{
	char recording_name[32]; // 0x0
	int8 pad_20[8];
};
ASSERT_STRUCT_SIZE(h1_scnr_ai_recording_references, 0x28);

struct h1_scnr_ai_conversations_participants
{
	int16 unknown; // 0x0
	uint16 flags; // 0x2
	int16 selection_type; // 0x4
	int16 actor_type; // 0x6
	int16 use_this_object_index; // 0x8
	int16 set_new_name_index; // 0xa
	int8 pad_c[12];
	int16 variants_0; // 0x18
	int16 variants_1; // 0x1a
	int16 variants_2; // 0x1c
	int16 variants_3; // 0x1e
	int16 variants_4; // 0x20
	int16 variants_5; // 0x22
	char encounter_name[32]; // 0x24
	int32 encounter_index; // 0x44
	int8 pad_48[12];
};
ASSERT_STRUCT_SIZE(h1_scnr_ai_conversations_participants, 0x54);

struct h1_scnr_ai_conversations_lines
{
	uint16 flags; // 0x0
	int16 participant_index; // 0x2
	int16 addressee; // 0x4
	int16 addressee_participant_index; // 0x6
	int8 pad_8[4];
	real32 line_delay_time; // 0xc
	int8 pad_10[12];
	h1_tag_reference variant_1; // 0x1c
	h1_tag_reference variant_2; // 0x2c
	h1_tag_reference variant_3; // 0x3c
	h1_tag_reference variant_4; // 0x4c
	h1_tag_reference variant_5; // 0x5c
	h1_tag_reference variant_6; // 0x6c
};
ASSERT_STRUCT_SIZE(h1_scnr_ai_conversations_lines, 0x7c);

struct h1_scnr_ai_conversations
{
	char name[32]; // 0x0
	uint16 flags; // 0x20
	int16 unknown; // 0x22
	real32 trigger_distance; // 0x24
	real32 run_to_player_distance; // 0x28
	int8 pad_2c[36];
	h1_tag_block<h1_scnr_ai_conversations_participants> participants; // 0x50
	h1_tag_block<h1_scnr_ai_conversations_lines> lines; // 0x5c
	int8 pad_68[12];
};
ASSERT_STRUCT_SIZE(h1_scnr_ai_conversations, 0x74);

struct h1_scnr_scripts
{
	char name[32]; // 0x0
	int16 script_type; // 0x20
	int16 return_type; // 0x22
	datum root_expression_index; // 0x24
	int8 pad_28[52];
};
ASSERT_STRUCT_SIZE(h1_scnr_scripts, 0x5c);

struct h1_scnr_globals
{
	char name[32]; // 0x0
	int16 type; // 0x20
	int16 unknown; // 0x22
	int8 pad_24[4];
	datum initialization_expression_index; // 0x28
	int8 pad_2c[48];
};
ASSERT_STRUCT_SIZE(h1_scnr_globals, 0x5c);

struct h1_scnr_references
{
	int8 pad_0[24];
	h1_tag_reference reference; // 0x18
};
ASSERT_STRUCT_SIZE(h1_scnr_references, 0x28);

struct h1_scnr_source_files
{
	char name[32]; // 0x0
	h1_tag_data source; // 0x20
};
ASSERT_STRUCT_SIZE(h1_scnr_source_files, 0x34);

struct h1_scnr_cutscene_flags
{
	int8 pad_0[4];
	char name[32]; // 0x4
	real_point3d position; // 0x24
	real_euler_angles2d facing; // 0x30
	int8 pad_38[36];
};
ASSERT_STRUCT_SIZE(h1_scnr_cutscene_flags, 0x5c);

struct h1_scnr_cutscene_camera_points
{
	int8 pad_0[4];
	char name[32]; // 0x4
	int8 pad_24[4];
	real_point3d position; // 0x28
	real_euler_angles3d orientation; // 0x34
	real32 field_of_view; // 0x40
	int8 pad_44[36];
};
ASSERT_STRUCT_SIZE(h1_scnr_cutscene_camera_points, 0x68);

struct h1_scnr_cutscene_titles
{
	int8 pad_0[4];
	char name[32]; // 0x4
	int8 pad_24[4];
	rectangle2d text_bounds_on_screen; // 0x28
	int16 string_index; // 0x30
	int16 unknown; // 0x32
	int16 justification; // 0x34
	int16 unknown_2; // 0x36
	int8 pad_38[4];
	uint32 text_color; // 0x3c
	uint32 shadow_color; // 0x40
	real32 fade_in_time; // 0x44
	real32 up_time; // 0x48
	real32 fade_out_time; // 0x4c
	int8 pad_50[16];
};
ASSERT_STRUCT_SIZE(h1_scnr_cutscene_titles, 0x60);

struct h1_scnr_structure_bsps
{
	uint32 structure_bsp_offset; // 0x0
	uint32 structure_bsp_size; // 0x4
	uint32 structure_bsp_address; // 0x8
	int8 pad_c[4];
	h1_tag_reference structure_bsp; // 0x10
};
ASSERT_STRUCT_SIZE(h1_scnr_structure_bsps, 0x20);

struct h1_scnr
{
	h1_tag_reference don_t_use; // 0x0
	h1_tag_reference won_t_use; // 0x10
	h1_tag_reference can_t_use; // 0x20
	h1_tag_block<h1_scnr_skies> skies; // 0x30
	int16 type; // 0x3c
	uint16 flags; // 0x3e
	h1_tag_block<h1_scnr_child_scenarios> child_scenarios; // 0x40
	real32 local_north; // 0x4c
	int8 pad_50[156];
	h1_tag_block<h1_scnr_predicted_resources> predicted_resources; // 0xec
	h1_tag_block<h1_scnr_functions> functions; // 0xf8
	h1_tag_data editor_scenario_data; // 0x104
	h1_tag_block<h1_scnr_comments> comments; // 0x118
	int8 pad_124[224];
	h1_tag_block<h1_scnr_object_names> object_names; // 0x204
	h1_tag_block<h1_scnr_scenery> scenery; // 0x210
	h1_tag_block<h1_scnr_scenery_palette> scenery_palette; // 0x21c
	h1_tag_block<h1_scnr_bipeds> bipeds; // 0x228
	h1_tag_block<h1_scnr_biped_palette> biped_palette; // 0x234
	h1_tag_block<h1_scnr_vehicles> vehicles; // 0x240
	h1_tag_block<h1_scnr_vehicle_palette> vehicle_palette; // 0x24c
	h1_tag_block<h1_scnr_equipment> equipment; // 0x258
	h1_tag_block<h1_scnr_equipment_palette> equipment_palette; // 0x264
	h1_tag_block<h1_scnr_weapons> weapons; // 0x270
	h1_tag_block<h1_scnr_weapon_palette> weapon_palette; // 0x27c
	h1_tag_block<h1_scnr_device_groups> device_groups; // 0x288
	h1_tag_block<h1_scnr_machines> machines; // 0x294
	h1_tag_block<h1_scnr_machine_palette> machine_palette; // 0x2a0
	h1_tag_block<h1_scnr_controls> controls; // 0x2ac
	h1_tag_block<h1_scnr_control_palette> control_palette; // 0x2b8
	h1_tag_block<h1_scnr_light_fixtures> light_fixtures; // 0x2c4
	h1_tag_block<h1_scnr_light_fixtures_palette> light_fixtures_palette; // 0x2d0
	h1_tag_block<h1_scnr_sound_scenery> sound_scenery; // 0x2dc
	h1_tag_block<h1_scnr_sound_scenery_palette> sound_scenery_palette; // 0x2e8
	int8 pad_2f4[84];
	h1_tag_block<h1_scnr_player_starting_profile> player_starting_profile; // 0x348
	h1_tag_block<h1_scnr_player_starting_locations> player_starting_locations; // 0x354
	h1_tag_block<h1_scnr_trigger_volumes> trigger_volumes; // 0x360
	h1_tag_block<h1_scnr_recorded_animations> recorded_animations; // 0x36c
	h1_tag_block<h1_scnr_netgame_flags> netgame_flags; // 0x378
	h1_tag_block<h1_scnr_netgame_equipment> netgame_equipment; // 0x384
	h1_tag_block<h1_scnr_starting_equipment> starting_equipment; // 0x390
	h1_tag_block<h1_scnr_bsp_switch_trigger_volumes> bsp_switch_trigger_volumes; // 0x39c
	h1_tag_block<h1_scnr_decals> decals; // 0x3a8
	h1_tag_block<h1_scnr_decals_palette> decals_palette; // 0x3b4
	h1_tag_block<h1_scnr_detail_object_collection_palette> detail_object_collection_palette; // 0x3c0
	int8 pad_3cc[84];
	h1_tag_block<h1_scnr_actor_palette> actor_palette; // 0x420
	h1_tag_block<h1_scnr_encounters> encounters; // 0x42c
	h1_tag_block<h1_scnr_command_lists> command_lists; // 0x438
	h1_tag_block<h1_scnr_ai_animation_references> ai_animation_references; // 0x444
	h1_tag_block<h1_scnr_ai_script_references> ai_script_references; // 0x450
	h1_tag_block<h1_scnr_ai_recording_references> ai_recording_references; // 0x45c
	h1_tag_block<h1_scnr_ai_conversations> ai_conversations; // 0x468
	h1_tag_data script_syntax_data; // 0x474
	h1_tag_data script_string_data; // 0x488
	h1_tag_block<h1_scnr_scripts> scripts; // 0x49c
	h1_tag_block<h1_scnr_globals> globals; // 0x4a8
	h1_tag_block<h1_scnr_references> references; // 0x4b4
	h1_tag_block<h1_scnr_source_files> source_files; // 0x4c0
	int8 pad_4cc[24];
	h1_tag_block<h1_scnr_cutscene_flags> cutscene_flags; // 0x4e4
	h1_tag_block<h1_scnr_cutscene_camera_points> cutscene_camera_points; // 0x4f0
	h1_tag_block<h1_scnr_cutscene_titles> cutscene_titles; // 0x4fc
	int8 pad_508[108];
	h1_tag_reference custom_object_names; // 0x574
	h1_tag_reference ingame_help_text; // 0x584
	h1_tag_reference hud_messages; // 0x594
	h1_tag_block<h1_scnr_structure_bsps> structure_bsps; // 0x5a4
};
ASSERT_STRUCT_SIZE(h1_scnr, 0x5b0);

struct h1_sbsp_collision_materials
{
	h1_tag_reference shader; // 0x0
	int16 unknown; // 0x10
	int16 material_type; // 0x12
};
ASSERT_STRUCT_SIZE(h1_sbsp_collision_materials, 0x14);

struct h1_sbsp_collision_bsp_bsp3d_nodes
{
	int32 plane; // 0x0
	int32 back_child; // 0x4
	int32 front_child; // 0x8
};
ASSERT_STRUCT_SIZE(h1_sbsp_collision_bsp_bsp3d_nodes, 0xc);

struct h1_sbsp_collision_bsp_planes
{
	real_plane3d plane; // 0x0
};
ASSERT_STRUCT_SIZE(h1_sbsp_collision_bsp_planes, 0x10);

struct h1_sbsp_collision_bsp_leaves
{
	uint16 flags; // 0x0
	int16 bsp2d_reference_count; // 0x2
	int32 first_bsp2d_reference; // 0x4
};
ASSERT_STRUCT_SIZE(h1_sbsp_collision_bsp_leaves, 0x8);

struct h1_sbsp_collision_bsp_bsp2d_references
{
	int32 plane; // 0x0
	int32 bsp2d_node; // 0x4
};
ASSERT_STRUCT_SIZE(h1_sbsp_collision_bsp_bsp2d_references, 0x8);

struct h1_sbsp_collision_bsp_bsp2d_nodes
{
	real_plane2d plane; // 0x0
	int32 left_child; // 0xc
	int32 right_child; // 0x10
};
ASSERT_STRUCT_SIZE(h1_sbsp_collision_bsp_bsp2d_nodes, 0x14);

struct h1_sbsp_collision_bsp_surfaces
{
	int32 plane; // 0x0
	int32 first_edge; // 0x4
	uint8 flags; // 0x8
	int8 breakable_surface; // 0x9
	int16 material; // 0xa
};
ASSERT_STRUCT_SIZE(h1_sbsp_collision_bsp_surfaces, 0xc);

struct h1_sbsp_collision_bsp_edges
{
	int32 start_vertex; // 0x0
	int32 end_vertex; // 0x4
	int32 forward_edge; // 0x8
	int32 reverse_edge; // 0xc
	int32 left_surface; // 0x10
	int32 right_surface; // 0x14
};
ASSERT_STRUCT_SIZE(h1_sbsp_collision_bsp_edges, 0x18);

struct h1_sbsp_collision_bsp_vertices
{
	real_point3d point; // 0x0
	int32 first_edge; // 0xc
};
ASSERT_STRUCT_SIZE(h1_sbsp_collision_bsp_vertices, 0x10);

struct h1_sbsp_collision_bsp
{
	h1_tag_block<h1_sbsp_collision_bsp_bsp3d_nodes> bsp3d_nodes; // 0x0
	h1_tag_block<h1_sbsp_collision_bsp_planes> planes; // 0xc
	h1_tag_block<h1_sbsp_collision_bsp_leaves> leaves; // 0x18
	h1_tag_block<h1_sbsp_collision_bsp_bsp2d_references> bsp2d_references; // 0x24
	h1_tag_block<h1_sbsp_collision_bsp_bsp2d_nodes> bsp2d_nodes; // 0x30
	h1_tag_block<h1_sbsp_collision_bsp_surfaces> surfaces; // 0x3c
	h1_tag_block<h1_sbsp_collision_bsp_edges> edges; // 0x48
	h1_tag_block<h1_sbsp_collision_bsp_vertices> vertices; // 0x54
};
ASSERT_STRUCT_SIZE(h1_sbsp_collision_bsp, 0x60);

struct h1_sbsp_nodes
{
	int16 nodes_0; // 0x0
	int16 nodes_1; // 0x2
	int16 nodes_2; // 0x4
};
ASSERT_STRUCT_SIZE(h1_sbsp_nodes, 0x6);

struct h1_sbsp_leaves
{
	int16 vertices_0; // 0x0
	int16 vertices_1; // 0x2
	int16 vertices_2; // 0x4
	int16 unknown; // 0x6
	int16 cluster; // 0x8
	int16 surface_reference_count; // 0xa
	int32 surface_references_index; // 0xc
};
ASSERT_STRUCT_SIZE(h1_sbsp_leaves, 0x10);

struct h1_sbsp_leaf_surfaces
{
	int32 surface_index; // 0x0
	int32 node_index; // 0x4
};
ASSERT_STRUCT_SIZE(h1_sbsp_leaf_surfaces, 0x8);

struct h1_sbsp_surfaces
{
	int16 vertex_a; // 0x0
	int16 vertex_b; // 0x2
	int16 vertex_c; // 0x4
};
ASSERT_STRUCT_SIZE(h1_sbsp_surfaces, 0x6);

struct h1_sbsp_lightmaps_materials
{
	h1_tag_reference shader; // 0x0
	int16 shader_permutation; // 0x10
	uint16 flags; // 0x12
	int32 surfaces_index; // 0x14
	int32 surface_count; // 0x18
	real_point3d centroid; // 0x1c
	real_rgb_color ambient_color; // 0x28
	int16 distant_light_count; // 0x34
	int16 unknown; // 0x36
	real_rgb_color distant_light_0_color; // 0x38
	real_vector3d distant_light_0_direction; // 0x44
	real_rgb_color distant_light_1_color; // 0x50
	real_vector3d distant_light_1_direction; // 0x5c
	int8 pad_68[12];
	real_argb_color reflection_tint; // 0x74
	real_vector3d shadow_vector; // 0x84
	real_rgb_color shadow_color; // 0x90
	real_plane3d plane; // 0x9c
	int16 breakable_surface; // 0xac
	int16 unknown_2; // 0xae
	int16 vertex_type; // 0xb0
	int16 unknown_3; // 0xb2
	int32 count; // 0xb4
	int32 offset; // 0xb8
	int8 pad_bc[4];
	uint32 vertices_index_pointer; // 0xc0
	int16 vertex_type_2; // 0xc4
	int16 unknown_4; // 0xc6
	int32 count_2; // 0xc8
	int32 offset_2; // 0xcc
	int8 pad_d0[4];
	uint32 vertices_index_pointer_2; // 0xd4
	h1_tag_data uncompressed_vertices; // 0xd8
	h1_tag_data compressed_vertices; // 0xec
};
ASSERT_STRUCT_SIZE(h1_sbsp_lightmaps_materials, 0x100);

struct h1_sbsp_lightmaps
{
	int16 bitmap; // 0x0
	int16 unknown; // 0x2
	int8 pad_4[16];
	h1_tag_block<h1_sbsp_lightmaps_materials> materials; // 0x14
};
ASSERT_STRUCT_SIZE(h1_sbsp_lightmaps, 0x20);

struct h1_sbsp_lens_flares
{
	h1_tag_reference lens_flare; // 0x0
};
ASSERT_STRUCT_SIZE(h1_sbsp_lens_flares, 0x10);

struct h1_sbsp_lens_flare_markers
{
	real_point3d position; // 0x0
	int8 direction_i_component; // 0xc
	int8 direction_j_component; // 0xd
	int8 direction_k_component; // 0xe
	int8 lens_flare_index; // 0xf
};
ASSERT_STRUCT_SIZE(h1_sbsp_lens_flare_markers, 0x10);

struct h1_sbsp_clusters_predicted_resources
{
	int16 type; // 0x0
	int16 resource_index; // 0x2
	datum tag_index; // 0x4
};
ASSERT_STRUCT_SIZE(h1_sbsp_clusters_predicted_resources, 0x8);

struct h1_sbsp_clusters_subclusters_surface_indices
{
	int32 index; // 0x0
};
ASSERT_STRUCT_SIZE(h1_sbsp_clusters_subclusters_surface_indices, 0x4);

struct h1_sbsp_clusters_subclusters
{
	real_bounds world_bounds_x; // 0x0
	real_bounds world_bounds_y; // 0x8
	real_bounds world_bounds_z; // 0x10
	h1_tag_block<h1_sbsp_clusters_subclusters_surface_indices> surface_indices; // 0x18
};
ASSERT_STRUCT_SIZE(h1_sbsp_clusters_subclusters, 0x24);

struct h1_sbsp_clusters_surface_indices
{
	int32 index; // 0x0
};
ASSERT_STRUCT_SIZE(h1_sbsp_clusters_surface_indices, 0x4);

struct h1_sbsp_clusters_mirrors_vertices
{
	real_point3d point; // 0x0
};
ASSERT_STRUCT_SIZE(h1_sbsp_clusters_mirrors_vertices, 0xc);

struct h1_sbsp_clusters_mirrors
{
	real_plane3d plane; // 0x0
	int8 pad_10[20];
	h1_tag_reference shader; // 0x24
	h1_tag_block<h1_sbsp_clusters_mirrors_vertices> vertices; // 0x34
};
ASSERT_STRUCT_SIZE(h1_sbsp_clusters_mirrors, 0x40);

struct h1_sbsp_clusters_portals
{
	int16 portal; // 0x0
};
ASSERT_STRUCT_SIZE(h1_sbsp_clusters_portals, 0x2);

struct h1_sbsp_clusters
{
	int16 sky; // 0x0
	int16 fog; // 0x2
	int16 background_sound_index; // 0x4
	int16 sound_environment_index; // 0x6
	int16 weather_index; // 0x8
	int16 transition_structure_bsp; // 0xa
	int16 first_decal_index; // 0xc
	int16 decal_count; // 0xe
	int8 pad_10[24];
	h1_tag_block<h1_sbsp_clusters_predicted_resources> predicted_resources; // 0x28
	h1_tag_block<h1_sbsp_clusters_subclusters> subclusters; // 0x34
	int16 first_lens_flare_marker_index; // 0x40
	int16 lens_flare_marker_count; // 0x42
	h1_tag_block<h1_sbsp_clusters_surface_indices> surface_indices; // 0x44
	h1_tag_block<h1_sbsp_clusters_mirrors> mirrors; // 0x50
	h1_tag_block<h1_sbsp_clusters_portals> portals; // 0x5c
};
ASSERT_STRUCT_SIZE(h1_sbsp_clusters, 0x68);

struct h1_sbsp_cluster_portals_vertices
{
	real_point3d point; // 0x0
};
ASSERT_STRUCT_SIZE(h1_sbsp_cluster_portals_vertices, 0xc);

struct h1_sbsp_cluster_portals
{
	int16 front_cluster; // 0x0
	int16 back_cluster; // 0x2
	int32 plane_index; // 0x4
	real_point3d centroid; // 0x8
	real32 bounding_radius; // 0x14
	uint32 flags; // 0x18
	int8 pad_1c[24];
	h1_tag_block<h1_sbsp_cluster_portals_vertices> vertices; // 0x34
};
ASSERT_STRUCT_SIZE(h1_sbsp_cluster_portals, 0x40);

struct h1_sbsp_breakable_surfaces
{
	real_point3d centroid; // 0x0
	real32 radius; // 0xc
	int32 collision_surface_index; // 0x10
	int8 pad_14[28];
};
ASSERT_STRUCT_SIZE(h1_sbsp_breakable_surfaces, 0x30);

struct h1_sbsp_fog_planes_vertices
{
	real_point3d point; // 0x0
};
ASSERT_STRUCT_SIZE(h1_sbsp_fog_planes_vertices, 0xc);

struct h1_sbsp_fog_planes
{
	int16 front_region_index; // 0x0
	int16 material_type; // 0x2
	real_plane3d plane; // 0x4
	h1_tag_block<h1_sbsp_fog_planes_vertices> vertices; // 0x14
};
ASSERT_STRUCT_SIZE(h1_sbsp_fog_planes, 0x20);

struct h1_sbsp_fog_regions
{
	int8 pad_0[36];
	int16 fog_palette_index; // 0x24
	int16 weather_palette_index; // 0x26
};
ASSERT_STRUCT_SIZE(h1_sbsp_fog_regions, 0x28);

struct h1_sbsp_fog_palette
{
	char name[32]; // 0x0
	h1_tag_reference fog; // 0x20
	int8 pad_30[4];
	char fog_scale_function[32]; // 0x34
	int8 pad_54[52];
};
ASSERT_STRUCT_SIZE(h1_sbsp_fog_palette, 0x88);

struct h1_sbsp_weather_palette
{
	char name[32]; // 0x0
	h1_tag_reference particle_system; // 0x20
	int8 pad_30[4];
	char particle_system_scale_function[32]; // 0x34
	int8 pad_54[44];
	h1_tag_reference wind; // 0x80
	real_vector3d wind_direction; // 0x90
	real32 wind_magnitude; // 0x9c
	int8 pad_a0[4];
	char wind_scale_function[32]; // 0xa4
	int8 pad_c4[44];
};
ASSERT_STRUCT_SIZE(h1_sbsp_weather_palette, 0xf0);

struct h1_sbsp_weather_polyhedra_planes
{
	real_plane3d plane; // 0x0
};
ASSERT_STRUCT_SIZE(h1_sbsp_weather_polyhedra_planes, 0x10);

struct h1_sbsp_weather_polyhedra
{
	real_point3d bounding_sphere_center; // 0x0
	real32 bounding_sphere_radius; // 0xc
	int8 pad_10[4];
	h1_tag_block<h1_sbsp_weather_polyhedra_planes> planes; // 0x14
};
ASSERT_STRUCT_SIZE(h1_sbsp_weather_polyhedra, 0x20);

struct h1_sbsp_pathfinding_surfaces
{
	int8 data; // 0x0
};
ASSERT_STRUCT_SIZE(h1_sbsp_pathfinding_surfaces, 0x1);

struct h1_sbsp_pathfinding_edges
{
	int8 midpoint; // 0x0
};
ASSERT_STRUCT_SIZE(h1_sbsp_pathfinding_edges, 0x1);

struct h1_sbsp_background_sound_palette
{
	char name[32]; // 0x0
	h1_tag_reference background_sound; // 0x20
	int8 pad_30[4];
	char scale_function[32]; // 0x34
	int8 pad_54[32];
};
ASSERT_STRUCT_SIZE(h1_sbsp_background_sound_palette, 0x74);

struct h1_sbsp_sound_environment_palette
{
	char name[32]; // 0x0
	h1_tag_reference sound_environment; // 0x20
	int8 pad_30[32];
};
ASSERT_STRUCT_SIZE(h1_sbsp_sound_environment_palette, 0x50);

struct h1_sbsp_markers
{
	char name[32]; // 0x0
	real_quaternion rotation; // 0x20
	real_point3d position; // 0x30
};
ASSERT_STRUCT_SIZE(h1_sbsp_markers, 0x3c);

struct h1_sbsp_detail_objects_cells
{
	int16 cell_x; // 0x0
	int16 cell_y; // 0x2
	int16 cell_z; // 0x4
	int16 offset_z; // 0x6
	int32 valid_layers_flags; // 0x8
	int32 start_index; // 0xc
	int32 count_index; // 0x10
	int8 pad_14[12];
};
ASSERT_STRUCT_SIZE(h1_sbsp_detail_objects_cells, 0x20);

struct h1_sbsp_detail_objects_instances
{
	int8 position_x; // 0x0
	int8 position_y; // 0x1
	int8 position_z; // 0x2
	int8 data; // 0x3
	int16 color; // 0x4
};
ASSERT_STRUCT_SIZE(h1_sbsp_detail_objects_instances, 0x6);

struct h1_sbsp_detail_objects_counts
{
	int16 count; // 0x0
};
ASSERT_STRUCT_SIZE(h1_sbsp_detail_objects_counts, 0x2);

struct h1_sbsp_detail_objects_z_reference_vectors
{
	real_quaternion z_reference; // 0x0
};
ASSERT_STRUCT_SIZE(h1_sbsp_detail_objects_z_reference_vectors, 0x10);

struct h1_sbsp_detail_objects
{
	h1_tag_block<h1_sbsp_detail_objects_cells> cells; // 0x0
	h1_tag_block<h1_sbsp_detail_objects_instances> instances; // 0xc
	h1_tag_block<h1_sbsp_detail_objects_counts> counts; // 0x18
	h1_tag_block<h1_sbsp_detail_objects_z_reference_vectors> z_reference_vectors; // 0x24
	int8 pad_30[16];
};
ASSERT_STRUCT_SIZE(h1_sbsp_detail_objects, 0x40);

struct h1_sbsp_runtime_decals
{
	real_point3d position; // 0x0
	int16 decal_type; // 0xc
	int8 yaw; // 0xe
	int8 pitch; // 0xf
};
ASSERT_STRUCT_SIZE(h1_sbsp_runtime_decals, 0x10);

struct h1_sbsp_leaf_map_leaves_faces_vertices
{
	real_point2d vertex; // 0x0
};
ASSERT_STRUCT_SIZE(h1_sbsp_leaf_map_leaves_faces_vertices, 0x8);

struct h1_sbsp_leaf_map_leaves_faces
{
	int32 node_index; // 0x0
	h1_tag_block<h1_sbsp_leaf_map_leaves_faces_vertices> vertices; // 0x4
};
ASSERT_STRUCT_SIZE(h1_sbsp_leaf_map_leaves_faces, 0x10);

struct h1_sbsp_leaf_map_leaves_portal_indices
{
	int32 portal_index; // 0x0
};
ASSERT_STRUCT_SIZE(h1_sbsp_leaf_map_leaves_portal_indices, 0x4);

struct h1_sbsp_leaf_map_leaves
{
	h1_tag_block<h1_sbsp_leaf_map_leaves_faces> faces; // 0x0
	h1_tag_block<h1_sbsp_leaf_map_leaves_portal_indices> portal_indices; // 0xc
};
ASSERT_STRUCT_SIZE(h1_sbsp_leaf_map_leaves, 0x18);

struct h1_sbsp_leaf_map_portals_vertices
{
	real_point3d point; // 0x0
};
ASSERT_STRUCT_SIZE(h1_sbsp_leaf_map_portals_vertices, 0xc);

struct h1_sbsp_leaf_map_portals
{
	int32 plane_index; // 0x0
	int32 back_leaf_index; // 0x4
	int32 front_leaf_index; // 0x8
	h1_tag_block<h1_sbsp_leaf_map_portals_vertices> vertices; // 0xc
};
ASSERT_STRUCT_SIZE(h1_sbsp_leaf_map_portals, 0x18);

struct h1_sbsp
{
	h1_tag_reference lightmap_bitmaps; // 0x0
	real32 vehicle_floor; // 0x10
	real32 vehicle_ceiling; // 0x14
	int8 pad_18[20];
	real_rgb_color default_ambient_color; // 0x2c
	int8 pad_38[4];
	real_rgb_color default_distant_light_0_color; // 0x3c
	real_vector3d default_distant_light_0_direction; // 0x48
	real_rgb_color default_distant_light_1_color; // 0x54
	real_vector3d default_distant_light_1_direction; // 0x60
	int8 pad_6c[12];
	real_argb_color default_reflection_tint; // 0x78
	real_vector3d default_shadow_vector; // 0x88
	real_rgb_color default_shadow_color; // 0x94
	int8 pad_a0[4];
	h1_tag_block<h1_sbsp_collision_materials> collision_materials; // 0xa4
	h1_tag_block<h1_sbsp_collision_bsp> collision_bsp; // 0xb0
	h1_tag_block<h1_sbsp_nodes> nodes; // 0xbc
	real_bounds world_bounds_x; // 0xc8
	real_bounds world_bounds_y; // 0xd0
	real_bounds world_bounds_z; // 0xd8
	h1_tag_block<h1_sbsp_leaves> leaves; // 0xe0
	h1_tag_block<h1_sbsp_leaf_surfaces> leaf_surfaces; // 0xec
	h1_tag_block<h1_sbsp_surfaces> surfaces; // 0xf8
	h1_tag_block<h1_sbsp_lightmaps> lightmaps; // 0x104
	int8 pad_110[12];
	h1_tag_block<h1_sbsp_lens_flares> lens_flares; // 0x11c
	h1_tag_block<h1_sbsp_lens_flare_markers> lens_flare_markers; // 0x128
	h1_tag_block<h1_sbsp_clusters> clusters; // 0x134
	h1_tag_data cluster_data; // 0x140
	h1_tag_block<h1_sbsp_cluster_portals> cluster_portals; // 0x154
	int8 pad_160[12];
	h1_tag_block<h1_sbsp_breakable_surfaces> breakable_surfaces; // 0x16c
	h1_tag_block<h1_sbsp_fog_planes> fog_planes; // 0x178
	h1_tag_block<h1_sbsp_fog_regions> fog_regions; // 0x184
	h1_tag_block<h1_sbsp_fog_palette> fog_palette; // 0x190
	int8 pad_19c[24];
	h1_tag_block<h1_sbsp_weather_palette> weather_palette; // 0x1b4
	h1_tag_block<h1_sbsp_weather_polyhedra> weather_polyhedra; // 0x1c0
	int8 pad_1cc[24];
	h1_tag_block<h1_sbsp_pathfinding_surfaces> pathfinding_surfaces; // 0x1e4
	h1_tag_block<h1_sbsp_pathfinding_edges> pathfinding_edges; // 0x1f0
	h1_tag_block<h1_sbsp_background_sound_palette> background_sound_palette; // 0x1fc
	h1_tag_block<h1_sbsp_sound_environment_palette> sound_environment_palette; // 0x208
	h1_tag_data sound_pas_data; // 0x214
	int8 pad_228[24];
	h1_tag_block<h1_sbsp_markers> markers; // 0x240
	h1_tag_block<h1_sbsp_detail_objects> detail_objects; // 0x24c
	h1_tag_block<h1_sbsp_runtime_decals> runtime_decals; // 0x258
	int8 pad_264[12];
	h1_tag_block<h1_sbsp_leaf_map_leaves> leaf_map_leaves; // 0x270
	h1_tag_block<h1_sbsp_leaf_map_portals> leaf_map_portals; // 0x27c
};
ASSERT_STRUCT_SIZE(h1_sbsp, 0x288);

struct h1_sky_shader_functions
{
	int8 pad_0[4];
	char global_function_name[32]; // 0x4
};
ASSERT_STRUCT_SIZE(h1_sky_shader_functions, 0x24);

struct h1_sky_animations
{
	int16 animation_index; // 0x0
	int16 unknown; // 0x2
	real32 period; // 0x4
	int8 pad_8[28];
};
ASSERT_STRUCT_SIZE(h1_sky_animations, 0x24);

struct h1_sky_lights
{
	h1_tag_reference lens_flare; // 0x0
	char lens_flare_marker_name[32]; // 0x10
	int8 pad_30[28];
	uint32 flags; // 0x4c
	real_rgb_color color; // 0x50
	real32 power; // 0x5c
	real32 test_distance; // 0x60
	int8 pad_64[4];
	real_euler_angles2d direction; // 0x68
	real32 diameter; // 0x70
};
ASSERT_STRUCT_SIZE(h1_sky_lights, 0x74);

struct h1_sky
{
	h1_tag_reference model; // 0x0
	h1_tag_reference animation_graph; // 0x10
	int8 pad_20[24];
	real_rgb_color indoor_ambient_radiosity_color; // 0x38
	real32 indoor_ambient_radiosity_power; // 0x44
	real_rgb_color outdoor_ambient_radiosity_color; // 0x48
	real32 outdoor_ambient_radiosity_power; // 0x54
	real_rgb_color outdoor_fog_color; // 0x58
	int8 pad_64[8];
	real32 outdoor_fog_maximum_density; // 0x6c
	real32 outdoor_fog_start_distance; // 0x70
	real32 outdoor_fog_opaque_distance; // 0x74
	real_rgb_color indoor_fog_color; // 0x78
	int8 pad_84[8];
	real32 indoor_fog_maximum_density; // 0x8c
	real32 indoor_fog_start_distance; // 0x90
	real32 indoor_fog_opaque_distance; // 0x94
	h1_tag_reference indoor_fog_screen; // 0x98
	int8 pad_a8[4];
	h1_tag_block<h1_sky_shader_functions> shader_functions; // 0xac
	h1_tag_block<h1_sky_animations> animations; // 0xb8
	h1_tag_block<h1_sky_lights> lights; // 0xc4
};
ASSERT_STRUCT_SIZE(h1_sky, 0xd0);

struct h1_bitm_sequences_sprites
{
	int16 bitmap_index; // 0x0
	int16 unknown; // 0x2
	int8 pad_4[4];
	real32 left; // 0x8
	real32 right; // 0xc
	real32 top; // 0x10
	real32 bottom; // 0x14
	real_point2d registration_point; // 0x18
};
ASSERT_STRUCT_SIZE(h1_bitm_sequences_sprites, 0x20);

struct h1_bitm_sequences
{
	char name[32]; // 0x0
	int16 first_bitmap_index; // 0x20
	int16 bitmap_count; // 0x22
	int8 pad_24[16];
	h1_tag_block<h1_bitm_sequences_sprites> sprites; // 0x34
};
ASSERT_STRUCT_SIZE(h1_bitm_sequences, 0x40);

struct h1_bitm_bitmaps
{
	char signature[4]; // 0x0
	int16 width; // 0x4
	int16 height; // 0x6
	int16 depth; // 0x8
	int16 type; // 0xa
	int16 format; // 0xc
	uint16 flags; // 0xe
	point2d registration_point; // 0x10
	int16 mipmap_count; // 0x14
	int16 unknown; // 0x16
	uint32 pixels_offset; // 0x18
	uint32 pixels_size; // 0x1c
	datum f_datum; // 0x20
	uint32 pointer; // 0x24
	int8 pad_28[4];
	int8 unknown_2; // 0x2c
	int8 unknown_3; // 0x2d
	int8 unknown_4; // 0x2e
	int8 unknown_5; // 0x2f
};
ASSERT_STRUCT_SIZE(h1_bitm_bitmaps, 0x30);

struct h1_bitm
{
	int16 type; // 0x0
	int16 format; // 0x2
	int16 usage; // 0x4
	uint16 flags; // 0x6
	real32 detail_fade_factor; // 0x8
	real32 sharpen_amount; // 0xc
	real32 bump_height; // 0x10
	int16 sprite_budget_size; // 0x14
	int16 sprite_budget_count; // 0x16
	int16 color_plate_width; // 0x18
	int16 color_plate_height; // 0x1a
	h1_tag_data compressed_color_plate_data; // 0x1c
	h1_tag_data processed_pixel_data; // 0x30
	real32 blur_filter_size; // 0x44
	real32 alpha_bias; // 0x48
	int16 mipmap_count; // 0x4c
	int16 sprite_usage; // 0x4e
	int16 sprite_spacing; // 0x50
	int16 unknown; // 0x52
	h1_tag_block<h1_bitm_sequences> sequences; // 0x54
	h1_tag_block<h1_bitm_bitmaps> bitmaps; // 0x60
};
ASSERT_STRUCT_SIZE(h1_bitm, 0x6c);

struct h1_senv
{
	uint16 flags; // 0x0
	int16 detail_level; // 0x2
	real32 power; // 0x4
	real_rgb_color color_of_emitted_light; // 0x8
	real_rgb_color tint_color; // 0x14
	uint16 flags_2; // 0x20
	int16 material_type; // 0x22
	int16 shader_type; // 0x24
	int16 unknown; // 0x26
	uint16 flags_3; // 0x28
	int16 type; // 0x2a
	real32 lens_flare_spacing; // 0x2c
	h1_tag_reference lens_flare; // 0x30
	int8 pad_40[44];
	uint16 flags_4; // 0x6c
	int16 unknown_2; // 0x6e
	int8 pad_70[24];
	h1_tag_reference base_map; // 0x88
	int8 pad_98[24];
	int16 detail_map_function; // 0xb0
	int16 unknown_3; // 0xb2
	real32 primary_detail_map_scale; // 0xb4
	h1_tag_reference primary_detail_map; // 0xb8
	real32 secondary_detail_map_scale; // 0xc8
	h1_tag_reference secondary_detail_map; // 0xcc
	int8 pad_dc[24];
	int16 micro_detail_map_function; // 0xf4
	int16 unknown_4; // 0xf6
	real32 micro_detail_map_scale; // 0xf8
	h1_tag_reference micro_detail_map; // 0xfc
	real_rgb_color material_color; // 0x10c
	int8 pad_118[12];
	real32 bump_map_scale; // 0x124
	h1_tag_reference bump_map; // 0x128
	real_point2d bump_map_scale_2; // 0x138
	int8 pad_140[16];
	int16 u_animation_function; // 0x150
	int16 unknown_5; // 0x152
	real32 u_animation_period; // 0x154
	real32 u_animation_scale; // 0x158
	int16 v_animation_function; // 0x15c
	int16 unknown_6; // 0x15e
	real32 v_animation_period; // 0x160
	real32 v_animation_scale; // 0x164
	int8 pad_168[24];
	uint16 flags_5; // 0x180
	int16 unknown_7; // 0x182
	int8 pad_184[24];
	real_rgb_color primary_on_color; // 0x19c
	real_rgb_color primary_off_color; // 0x1a8
	int16 primary_animation_function; // 0x1b4
	int16 unknown_8; // 0x1b6
	real32 primary_animation_period; // 0x1b8
	real32 primary_animation_phase; // 0x1bc
	int8 pad_1c0[24];
	real_rgb_color secondary_on_color; // 0x1d8
	real_rgb_color secondary_off_color; // 0x1e4
	int16 secondary_animation_function; // 0x1f0
	int16 unknown_9; // 0x1f2
	real32 secondary_animation_period; // 0x1f4
	real32 secondary_animation_phase; // 0x1f8
	int8 pad_1fc[24];
	real_rgb_color plasma_on_color; // 0x214
	real_rgb_color plasma_off_color; // 0x220
	int16 plasma_animation_function; // 0x22c
	int16 unknown_10; // 0x22e
	real32 plasma_animation_period; // 0x230
	real32 plasma_animation_phase; // 0x234
	int8 pad_238[24];
	real32 map_scale; // 0x250
	h1_tag_reference map; // 0x254
	int8 pad_264[24];
	uint16 flags_6; // 0x27c
	int16 unknown_11; // 0x27e
	int8 pad_280[16];
	real32 brightness; // 0x290
	int8 pad_294[20];
	real_rgb_color perpendicular_color; // 0x2a8
	real_rgb_color parallel_color; // 0x2b4
	int8 pad_2c0[16];
	uint16 flags_7; // 0x2d0
	int16 type_2; // 0x2d2
	real32 lightmap_brightness_scale; // 0x2d4
	int8 pad_2d8[28];
	real32 perpendicular_brightness; // 0x2f4
	real32 parallel_brightness; // 0x2f8
	int8 pad_2fc[40];
	h1_tag_reference reflection_cube_map; // 0x324
	int8 pad_334[16];
};
ASSERT_STRUCT_SIZE(h1_senv, 0x344);

struct h1_soso
{
	uint16 flags; // 0x0
	int16 detail_level; // 0x2
	real32 power; // 0x4
	real_rgb_color color_of_emitted_light; // 0x8
	real_rgb_color tint_color; // 0x14
	uint16 flags_2; // 0x20
	int16 material_type; // 0x22
	int16 shader_type; // 0x24
	int16 unknown; // 0x26
	uint16 flags_3; // 0x28
	int16 unknown_2; // 0x2a
	int8 pad_2c[12];
	real32 translucency; // 0x38
	int8 pad_3c[16];
	int16 change_color_source; // 0x4c
	int16 unknown_3; // 0x4e
	int8 pad_50[28];
	uint16 flags_4; // 0x6c
	int16 unknown_4; // 0x6e
	int16 color_source; // 0x70
	int16 animation_function; // 0x72
	real32 animation_period; // 0x74
	real_rgb_color animation_color_lower_bound; // 0x78
	real_rgb_color animation_color_upper_bound; // 0x84
	int8 pad_90[12];
	real32 map_u_scale; // 0x9c
	real32 map_v_scale; // 0xa0
	h1_tag_reference base_map; // 0xa4
	int8 pad_b4[8];
	h1_tag_reference multipurpose_map; // 0xbc
	int8 pad_cc[8];
	int16 detail_function; // 0xd4
	int16 detail_mask; // 0xd6
	real32 detail_map_scale; // 0xd8
	h1_tag_reference detail_map; // 0xdc
	real32 detail_map_v_scale; // 0xec
	int8 pad_f0[12];
	int16 u_animation_source; // 0xfc
	int16 u_animation_function; // 0xfe
	real32 u_animation_period; // 0x100
	real32 u_animation_phase; // 0x104
	real32 u_animation_scale; // 0x108
	int16 v_animation_source; // 0x10c
	int16 v_animation_function; // 0x10e
	real32 v_animation_period; // 0x110
	real32 v_animation_phase; // 0x114
	real32 v_animation_scale; // 0x118
	int16 rotation_animation_source; // 0x11c
	int16 rotation_animation_function; // 0x11e
	real32 rotation_animation_period; // 0x120
	real32 rotation_animation_phase; // 0x124
	real32 rotation_animation_scale; // 0x128
	real_point2d rotation_animation_center; // 0x12c
	int8 pad_134[8];
	real32 reflection_falloff_distance; // 0x13c
	real32 reflection_cutoff_distance; // 0x140
	real32 perpendicular_brightness; // 0x144
	real_rgb_color perpendicular_tint_color; // 0x148
	real32 parallel_brightness; // 0x154
	real_rgb_color parallel_tint_color; // 0x158
	h1_tag_reference reflection_cube_map; // 0x164
	int8 pad_174[16];
	real32 runtime_unknown; // 0x184
	int8 pad_188[48];
};
ASSERT_STRUCT_SIZE(h1_soso, 0x1b8);

struct h1_schi_extra_layers
{
	h1_tag_reference shader; // 0x0
};
ASSERT_STRUCT_SIZE(h1_schi_extra_layers, 0x10);

struct h1_schi_maps
{
	uint16 flags; // 0x0
	int16 unknown; // 0x2
	int8 pad_4[40];
	int16 color_function; // 0x2c
	int16 alpha_function; // 0x2e
	int8 pad_30[36];
	real32 map_u_scale; // 0x54
	real32 map_v_scale; // 0x58
	real32 map_u_offset; // 0x5c
	real32 map_v_offset; // 0x60
	real32 map_rotation; // 0x64
	real32 mipmap_bias; // 0x68
	h1_tag_reference map; // 0x6c
	int8 pad_7c[40];
	int16 u_animation_source; // 0xa4
	int16 u_animation_function; // 0xa6
	real32 u_animation_period; // 0xa8
	real32 u_animation_phase; // 0xac
	real32 u_animation_scale; // 0xb0
	int16 v_animation_source; // 0xb4
	int16 v_animation_function; // 0xb6
	real32 v_animation_period; // 0xb8
	real32 v_animation_phase; // 0xbc
	real32 v_animation_scale; // 0xc0
	int16 rotation_animation_source; // 0xc4
	int16 rotation_animation_function; // 0xc6
	real32 rotation_animation_period; // 0xc8
	real32 rotation_animation_phase; // 0xcc
	real32 rotation_animation_scale; // 0xd0
	real_point2d rotation_animation_center; // 0xd4
};
ASSERT_STRUCT_SIZE(h1_schi_maps, 0xdc);

struct h1_schi
{
	uint16 flags; // 0x0
	int16 detail_level; // 0x2
	real32 power; // 0x4
	real_rgb_color color_of_emitted_light; // 0x8
	real_rgb_color tint_color; // 0x14
	uint16 flags_2; // 0x20
	int16 material_type; // 0x22
	int16 shader_type; // 0x24
	int16 unknown; // 0x26
	int8 numeric_counter_limit; // 0x28
	uint8 flags_3; // 0x29
	int16 first_map_type; // 0x2a
	int16 framebuffer_blend_function; // 0x2c
	int16 framebuffer_fade_mode; // 0x2e
	int16 framebuffer_fade_source; // 0x30
	int16 unknown_2; // 0x32
	real32 lens_flare_spacing; // 0x34
	h1_tag_reference lens_flare; // 0x38
	h1_tag_block<h1_schi_extra_layers> extra_layers; // 0x48
	h1_tag_block<h1_schi_maps> maps; // 0x54
	uint32 extra_flags; // 0x60
	int8 pad_64[8];
};
ASSERT_STRUCT_SIZE(h1_schi, 0x6c);

struct h1_scex_extra_layers
{
	h1_tag_reference shader; // 0x0
};
ASSERT_STRUCT_SIZE(h1_scex_extra_layers, 0x10);

struct h1_scex_f_4_stage_maps
{
	uint16 flags; // 0x0
	int16 unknown; // 0x2
	int8 pad_4[40];
	int16 color_function; // 0x2c
	int16 alpha_function; // 0x2e
	int8 pad_30[36];
	real32 map_u_scale; // 0x54
	real32 map_v_scale; // 0x58
	real32 map_u_offset; // 0x5c
	real32 map_v_offset; // 0x60
	real32 map_rotation; // 0x64
	real32 mipmap_bias; // 0x68
	h1_tag_reference map; // 0x6c
	int8 pad_7c[40];
	int16 u_animation_source; // 0xa4
	int16 u_animation_function; // 0xa6
	real32 u_animation_period; // 0xa8
	real32 u_animation_phase; // 0xac
	real32 u_animation_scale; // 0xb0
	int16 v_animation_source; // 0xb4
	int16 v_animation_function; // 0xb6
	real32 v_animation_period; // 0xb8
	real32 v_animation_phase; // 0xbc
	real32 v_animation_scale; // 0xc0
	int16 rotation_animation_source; // 0xc4
	int16 rotation_animation_function; // 0xc6
	real32 rotation_animation_period; // 0xc8
	real32 rotation_animation_phase; // 0xcc
	real32 rotation_animation_scale; // 0xd0
	real_point2d rotation_animation_center; // 0xd4
};
ASSERT_STRUCT_SIZE(h1_scex_f_4_stage_maps, 0xdc);

struct h1_scex_f_2_stage_maps
{
	uint16 flags; // 0x0
	int16 unknown; // 0x2
	int8 pad_4[40];
	int16 color_function; // 0x2c
	int16 alpha_function; // 0x2e
	int8 pad_30[36];
	real32 map_u_scale; // 0x54
	real32 map_v_scale; // 0x58
	real32 map_u_offset; // 0x5c
	real32 map_v_offset; // 0x60
	real32 map_rotation; // 0x64
	real32 mipmap_bias; // 0x68
	h1_tag_reference map; // 0x6c
	int8 pad_7c[40];
	int16 u_animation_source; // 0xa4
	int16 u_animation_function; // 0xa6
	real32 u_animation_period; // 0xa8
	real32 u_animation_phase; // 0xac
	real32 u_animation_scale; // 0xb0
	int16 v_animation_source; // 0xb4
	int16 v_animation_function; // 0xb6
	real32 v_animation_period; // 0xb8
	real32 v_animation_phase; // 0xbc
	real32 v_animation_scale; // 0xc0
	int16 rotation_animation_source; // 0xc4
	int16 rotation_animation_function; // 0xc6
	real32 rotation_animation_period; // 0xc8
	real32 rotation_animation_phase; // 0xcc
	real32 rotation_animation_scale; // 0xd0
	real_point2d rotation_animation_center; // 0xd4
};
ASSERT_STRUCT_SIZE(h1_scex_f_2_stage_maps, 0xdc);

struct h1_scex
{
	uint16 flags; // 0x0
	int16 detail_level; // 0x2
	real32 power; // 0x4
	real_rgb_color color_of_emitted_light; // 0x8
	real_rgb_color tint_color; // 0x14
	uint16 flags_2; // 0x20
	int16 material_type; // 0x22
	int16 shader_type; // 0x24
	int16 unknown; // 0x26
	int8 numeric_counter_limit; // 0x28
	uint8 flags_3; // 0x29
	int16 first_map_type; // 0x2a
	int16 framebuffer_blend_function; // 0x2c
	int16 framebuffer_fade_mode; // 0x2e
	int16 framebuffer_fade_source; // 0x30
	int16 unknown_2; // 0x32
	real32 lens_flare_spacing; // 0x34
	h1_tag_reference lens_flare; // 0x38
	h1_tag_block<h1_scex_extra_layers> extra_layers; // 0x48
	h1_tag_block<h1_scex_f_4_stage_maps> f_4_stage_maps; // 0x54
	h1_tag_block<h1_scex_f_2_stage_maps> f_2_stage_maps; // 0x60
	uint32 extra_flags; // 0x6c
	int8 pad_70[8];
};
ASSERT_STRUCT_SIZE(h1_scex, 0x78);

struct h1_sotr_extra_layers
{
	h1_tag_reference shader; // 0x0
};
ASSERT_STRUCT_SIZE(h1_sotr_extra_layers, 0x10);

struct h1_sotr_maps
{
	uint16 flags; // 0x0
	int16 unknown; // 0x2
	real32 map_u_scale; // 0x4
	real32 map_v_scale; // 0x8
	real32 map_u_offset; // 0xc
	real32 map_v_offset; // 0x10
	real32 map_rotation; // 0x14
	real32 mipmap_bias; // 0x18
	h1_tag_reference map; // 0x1c
	int16 u_animation_source; // 0x2c
	int16 u_animation_function; // 0x2e
	real32 u_animation_period; // 0x30
	real32 u_animation_phase; // 0x34
	real32 u_animation_scale; // 0x38
	int16 v_animation_source; // 0x3c
	int16 v_animation_function; // 0x3e
	real32 v_animation_period; // 0x40
	real32 v_animation_phase; // 0x44
	real32 v_animation_scale; // 0x48
	int16 rotation_animation_source; // 0x4c
	int16 rotation_animation_function; // 0x4e
	real32 rotation_animation_period; // 0x50
	real32 rotation_animation_phase; // 0x54
	real32 rotation_animation_scale; // 0x58
	real_point2d rotation_animation_center; // 0x5c
};
ASSERT_STRUCT_SIZE(h1_sotr_maps, 0x64);

struct h1_sotr_stages
{
	uint16 flags; // 0x0
	int16 unknown; // 0x2
	int16 color0_source; // 0x4
	int16 color0_animation_function; // 0x6
	real32 color0_animation_period; // 0x8
	real_argb_color color0_animation_lower_bound; // 0xc
	real_argb_color color0_animation_upper_bound; // 0x1c
	real_argb_color color1; // 0x2c
	int16 input_a; // 0x3c
	int16 input_a_mapping; // 0x3e
	int16 input_b; // 0x40
	int16 input_b_mapping; // 0x42
	int16 input_c; // 0x44
	int16 input_c_mapping; // 0x46
	int16 input_d; // 0x48
	int16 input_d_mapping; // 0x4a
	int16 output_ab; // 0x4c
	int16 output_ab_function; // 0x4e
	int16 output_cd; // 0x50
	int16 output_cd_function; // 0x52
	int16 output_ab_cd_mux_sum; // 0x54
	int16 output_mapping; // 0x56
	int16 input_a_2; // 0x58
	int16 input_a_mapping_2; // 0x5a
	int16 input_b_2; // 0x5c
	int16 input_b_mapping_2; // 0x5e
	int16 input_c_2; // 0x60
	int16 input_c_mapping_2; // 0x62
	int16 input_d_2; // 0x64
	int16 input_d_mapping_2; // 0x66
	int16 output_ab_2; // 0x68
	int16 output_cd_2; // 0x6a
	int16 output_ab_cd_mux_sum_2; // 0x6c
	int16 output_mapping_2; // 0x6e
};
ASSERT_STRUCT_SIZE(h1_sotr_stages, 0x70);

struct h1_sotr
{
	uint16 flags; // 0x0
	int16 detail_level; // 0x2
	real32 power; // 0x4
	real_rgb_color color_of_emitted_light; // 0x8
	real_rgb_color tint_color; // 0x14
	uint16 flags_2; // 0x20
	int16 material_type; // 0x22
	int16 shader_type; // 0x24
	int16 unknown; // 0x26
	int8 numeric_counter_limit; // 0x28
	uint8 flags_3; // 0x29
	int16 first_map_type; // 0x2a
	int16 framebuffer_blend_function; // 0x2c
	int16 framebuffer_fade_mode; // 0x2e
	int16 framebuffer_fade_source; // 0x30
	int16 unknown_2; // 0x32
	real32 lens_flare_spacing; // 0x34
	h1_tag_reference lens_flare; // 0x38
	h1_tag_block<h1_sotr_extra_layers> extra_layers; // 0x48
	h1_tag_block<h1_sotr_maps> maps; // 0x54
	h1_tag_block<h1_sotr_stages> stages; // 0x60
};
ASSERT_STRUCT_SIZE(h1_sotr, 0x6c);

struct h1_swat_ripples
{
	int16 unknown; // 0x0
	int16 unknown_2; // 0x2
	real32 contribution_factor; // 0x4
	int8 pad_8[32];
	real32 animation_angle; // 0x28
	real32 animation_velocity; // 0x2c
	real_vector2d map_offset; // 0x30
	int16 map_repeats; // 0x38
	int16 map_index; // 0x3a
	int8 pad_3c[16];
};
ASSERT_STRUCT_SIZE(h1_swat_ripples, 0x4c);

struct h1_swat
{
	uint16 flags; // 0x0
	int16 detail_level; // 0x2
	real32 power; // 0x4
	real_rgb_color color_of_emitted_light; // 0x8
	real_rgb_color tint_color; // 0x14
	uint16 flags_2; // 0x20
	int16 material_type; // 0x22
	int16 shader_type; // 0x24
	int16 unknown; // 0x26
	uint16 flags_3; // 0x28
	int16 unknown_2; // 0x2a
	int8 pad_2c[32];
	h1_tag_reference base_map; // 0x4c
	int8 pad_5c[16];
	real32 view_perpendicular_brightness; // 0x6c
	real_rgb_color view_perpendicular_tint_color; // 0x70
	real32 view_parallel_brightness; // 0x7c
	real_rgb_color view_parallel_tint_color; // 0x80
	int8 pad_8c[16];
	h1_tag_reference reflection_map; // 0x9c
	int8 pad_ac[16];
	real32 ripple_animation_angle; // 0xbc
	real32 ripple_animation_velocity; // 0xc0
	real32 ripple_scale; // 0xc4
	h1_tag_reference ripple_maps; // 0xc8
	int16 ripple_mipmap_levels; // 0xd8
	int16 unknown_3; // 0xda
	real32 ripple_mipmap_fade_factor; // 0xdc
	real32 ripple_mipmap_detail_bias; // 0xe0
	int8 pad_e4[64];
	h1_tag_block<h1_swat_ripples> ripples; // 0x124
	int8 pad_130[16];
};
ASSERT_STRUCT_SIZE(h1_swat, 0x140);

struct h1_sgla
{
	uint16 flags; // 0x0
	int16 detail_level; // 0x2
	real32 power; // 0x4
	real_rgb_color color_of_emitted_light; // 0x8
	real_rgb_color tint_color; // 0x14
	uint16 flags_2; // 0x20
	int16 material_type; // 0x22
	int16 shader_type; // 0x24
	int16 unknown; // 0x26
	uint16 flags_3; // 0x28
	int16 unknown_2; // 0x2a
	int8 pad_2c[40];
	real_rgb_color background_tint_color; // 0x54
	real32 background_tint_map_scale; // 0x60
	h1_tag_reference background_tint_map; // 0x64
	int8 pad_74[20];
	int16 unknown_3; // 0x88
	int16 reflection_type; // 0x8a
	real32 perpendicular_brightness; // 0x8c
	real_rgb_color perpendicular_tint_color; // 0x90
	real32 parallel_brightness; // 0x9c
	real_rgb_color parallel_tint_color; // 0xa0
	h1_tag_reference reflection_map; // 0xac
	real32 bump_map_scale; // 0xbc
	h1_tag_reference bump_map; // 0xc0
	int8 pad_d0[132];
	real32 diffuse_map_scale; // 0x154
	h1_tag_reference diffuse_map; // 0x158
	real32 diffuse_detail_map_scale; // 0x168
	h1_tag_reference diffuse_detail_map; // 0x16c
	int8 pad_17c[32];
	real32 specular_map_scale; // 0x19c
	h1_tag_reference specular_map; // 0x1a0
	real32 specular_detail_map_scale; // 0x1b0
	h1_tag_reference specular_detail_map; // 0x1b4
	int8 pad_1c4[28];
};
ASSERT_STRUCT_SIZE(h1_sgla, 0x1e0);

struct h1_smet
{
	uint16 flags; // 0x0
	int16 detail_level; // 0x2
	real32 power; // 0x4
	real_rgb_color color_of_emitted_light; // 0x8
	real_rgb_color tint_color; // 0x14
	uint16 flags_2; // 0x20
	int16 material_type; // 0x22
	int16 shader_type; // 0x24
	int16 unknown; // 0x26
	uint16 flags_3; // 0x28
	int16 unknown_2; // 0x2a
	int8 pad_2c[32];
	h1_tag_reference map; // 0x4c
	int8 pad_5c[32];
	real_rgb_color gradient_minimum_color; // 0x7c
	real_rgb_color gradient_maximum_color; // 0x88
	real_rgb_color background_color; // 0x94
	real_rgb_color flash_color; // 0xa0
	real_rgb_color tint_color_2; // 0xac
	real32 meter_transparency; // 0xb8
	real32 background_transparency; // 0xbc
	int8 pad_c0[24];
	int16 meter_brightness_source; // 0xd8
	int16 flash_brightness_source; // 0xda
	int16 value_source; // 0xdc
	int16 gradient_source; // 0xde
	int16 flash_extension_source; // 0xe0
	int16 unknown_3; // 0xe2
	int8 pad_e4[32];
};
ASSERT_STRUCT_SIZE(h1_smet, 0x104);

struct h1_spla
{
	uint16 flags; // 0x0
	int16 detail_level; // 0x2
	real32 power; // 0x4
	real_rgb_color color_of_emitted_light; // 0x8
	real_rgb_color tint_color; // 0x14
	uint16 flags_2; // 0x20
	int16 material_type; // 0x22
	int16 shader_type; // 0x24
	int16 unknown; // 0x26
	int16 unknown_2; // 0x28
	int16 unknown_3; // 0x2a
	int16 intensity_source; // 0x2c
	int16 unknown_4; // 0x2e
	real32 intensity_exponent; // 0x30
	int16 offset_source; // 0x34
	int16 unknown_5; // 0x36
	real32 offset_amount; // 0x38
	real32 offset_exponent; // 0x3c
	int8 pad_40[32];
	real32 perpendicular_brightness; // 0x60
	real_rgb_color perpendicular_tint_color; // 0x64
	real32 parallel_brightness; // 0x70
	real_rgb_color parallel_tint_color; // 0x74
	int16 tint_color_source; // 0x80
	int16 unknown_6; // 0x82
	int8 pad_84[32];
	int16 unknown_7; // 0xa4
	int16 unknown_8; // 0xa6
	int8 pad_a8[24];
	real32 primary_animation_period; // 0xc0
	real_vector3d primary_animation_direction; // 0xc4
	real32 primary_noise_map_scale; // 0xd0
	h1_tag_reference primary_noise_map; // 0xd4
	int8 pad_e4[36];
	real32 secondary_animation_period; // 0x108
	real_vector3d secondary_animation_direction; // 0x10c
	real32 secondary_noise_map_scale; // 0x118
	h1_tag_reference secondary_noise_map; // 0x11c
	int8 pad_12c[32];
};
ASSERT_STRUCT_SIZE(h1_spla, 0x14c);

struct h1_mode_markers_instances
{
	int8 region_index; // 0x0
	int8 permutation_index; // 0x1
	int8 node_index; // 0x2
	int8 unknown; // 0x3
	real_point3d translation; // 0x4
	real_quaternion rotation; // 0x10
};
ASSERT_STRUCT_SIZE(h1_mode_markers_instances, 0x20);

struct h1_mode_markers
{
	char name[32]; // 0x0
	int16 magic_identifier; // 0x20
	int16 unknown; // 0x22
	int8 pad_24[16];
	h1_tag_block<h1_mode_markers_instances> instances; // 0x34
};
ASSERT_STRUCT_SIZE(h1_mode_markers, 0x40);

struct h1_mode_nodes
{
	char name[32]; // 0x0
	int16 next_sibling_node_index; // 0x20
	int16 first_child_node_index; // 0x22
	int16 parent_node_index; // 0x24
	int16 unknown; // 0x26
	real_point3d default_translation; // 0x28
	real_quaternion default_rotation; // 0x34
	real32 node_distance_from_parent; // 0x44
	int8 pad_48[32];
	real32 inverse_scale; // 0x68
	real_vector3d inverse_forward; // 0x6c
	real_vector3d inverse_left; // 0x78
	real_vector3d inverse_up; // 0x84
	real_point3d inverse_position; // 0x90
};
ASSERT_STRUCT_SIZE(h1_mode_nodes, 0x9c);

struct h1_mode_regions_permutations_markers
{
	char name[32]; // 0x0
	int16 node_index; // 0x20
	int16 unknown; // 0x22
	real_quaternion rotation; // 0x24
	real_point3d translation; // 0x34
	int8 pad_40[16];
};
ASSERT_STRUCT_SIZE(h1_mode_regions_permutations_markers, 0x50);

struct h1_mode_regions_permutations
{
	char name[32]; // 0x0
	uint32 flags; // 0x20
	int8 pad_24[28];
	int16 super_low_index; // 0x40
	int16 low_index; // 0x42
	int16 medium_index; // 0x44
	int16 high_index; // 0x46
	int16 super_high_index; // 0x48
	int16 unknown; // 0x4a
	h1_tag_block<h1_mode_regions_permutations_markers> markers; // 0x4c
};
ASSERT_STRUCT_SIZE(h1_mode_regions_permutations, 0x58);

struct h1_mode_regions
{
	char name[32]; // 0x0
	int8 pad_20[4];
	int16 permutation_number; // 0x24
	int16 unknown; // 0x26
	int8 pad_28[24];
	h1_tag_block<h1_mode_regions_permutations> permutations; // 0x40
};
ASSERT_STRUCT_SIZE(h1_mode_regions, 0x4c);

struct h1_mode_geometries_parts_uncompressed_vertices
{
	real_point3d position; // 0x0
	real_vector3d normal; // 0xc
	real_vector3d binormal; // 0x18
	real_vector3d tangent; // 0x24
	real_point2d texture_coords; // 0x30
	int16 node0_index; // 0x38
	int16 node1_index; // 0x3a
	real32 node0_weight; // 0x3c
	real32 node1_weight; // 0x40
};
ASSERT_STRUCT_SIZE(h1_mode_geometries_parts_uncompressed_vertices, 0x44);

struct h1_mode_geometries_parts_compressed_vertices
{
	real_point3d position; // 0x0
	int32 normal_11_11_10_bit; // 0xc
	int32 binormal_11_11_10_bit; // 0x10
	int32 tangent_11_11_10_bit; // 0x14
	int16 texture_coordinate_u_16_bit; // 0x18
	int16 texture_coordinate_v_16_bit; // 0x1a
	int8 node0_index_x3; // 0x1c
	int8 node1_index_x3; // 0x1d
	int16 node0_weight_16_bit; // 0x1e
};
ASSERT_STRUCT_SIZE(h1_mode_geometries_parts_compressed_vertices, 0x20);

struct h1_mode_geometries_parts_triangles
{
	int16 vertex0_index; // 0x0
	int16 vertex1_index; // 0x2
	int16 vertex2_index; // 0x4
};
ASSERT_STRUCT_SIZE(h1_mode_geometries_parts_triangles, 0x6);

struct h1_mode_geometries_parts
{
	uint32 flags; // 0x0
	int16 shader_index; // 0x4
	int8 prev_filthy_part_index; // 0x6
	int8 next_filthy_part_index; // 0x7
	int16 centroid_primary_node; // 0x8
	int16 centroid_secondary_node; // 0xa
	real32 centroid_primary_weight; // 0xc
	real32 centroid_secondary_weight; // 0x10
	real_point3d centroid; // 0x14
	h1_tag_block<h1_mode_geometries_parts_uncompressed_vertices> uncompressed_vertices; // 0x20
	h1_tag_block<h1_mode_geometries_parts_compressed_vertices> compressed_vertices; // 0x2c
	h1_tag_block<h1_mode_geometries_parts_triangles> triangles; // 0x38
	int32 runtime_unknown_required; // 0x44
	int32 triangle_count; // 0x48
	int32 triangle_offset; // 0x4c
	int32 triangle_offset_2; // 0x50
	int16 vertex_type; // 0x54
	int16 unknown; // 0x56
	int32 vertex_count; // 0x58
	int32 unknown_2; // 0x5c
	uint32 vertex_pointer; // 0x60
	int32 vertex_offset; // 0x64
};
ASSERT_STRUCT_SIZE(h1_mode_geometries_parts, 0x68);

struct h1_mode_geometries
{
	uint32 flags; // 0x0
	int8 pad_4[32];
	h1_tag_block<h1_mode_geometries_parts> parts; // 0x24
};
ASSERT_STRUCT_SIZE(h1_mode_geometries, 0x30);

struct h1_mode_shaders
{
	h1_tag_reference shader; // 0x0
	int16 permutation; // 0x10
	int16 unknown; // 0x12
	int8 pad_14[12];
};
ASSERT_STRUCT_SIZE(h1_mode_shaders, 0x20);

struct h1_mode
{
	uint32 flags; // 0x0
	int32 node_list_checksum; // 0x4
	real32 super_high_detail_cutoff; // 0x8
	real32 high_detail_cutoff; // 0xc
	real32 medium_detail_cutoff; // 0x10
	real32 low_detail_cutoff; // 0x14
	real32 super_low_cutoff; // 0x18
	int16 super_high_detail_node_count; // 0x1c
	int16 high_detail_node_count; // 0x1e
	int16 medium_detail_node_count; // 0x20
	int16 low_detail_node_count; // 0x22
	int16 super_low_detail_node_count; // 0x24
	int16 unknown; // 0x26
	int8 pad_28[8];
	real32 base_map_u_scale; // 0x30
	real32 base_map_v_scale; // 0x34
	int8 pad_38[116];
	h1_tag_block<h1_mode_markers> markers; // 0xac
	h1_tag_block<h1_mode_nodes> nodes; // 0xb8
	h1_tag_block<h1_mode_regions> regions; // 0xc4
	h1_tag_block<h1_mode_geometries> geometries; // 0xd0
	h1_tag_block<h1_mode_shaders> shaders; // 0xdc
};
ASSERT_STRUCT_SIZE(h1_mode, 0xe8);

struct h1_mod2_markers_instances
{
	int8 region_index; // 0x0
	int8 permutation_index; // 0x1
	int8 node_index; // 0x2
	int8 unknown; // 0x3
	real_point3d translation; // 0x4
	real_quaternion rotation; // 0x10
};
ASSERT_STRUCT_SIZE(h1_mod2_markers_instances, 0x20);

struct h1_mod2_markers
{
	char name[32]; // 0x0
	int16 magic_identifier; // 0x20
	int16 unknown; // 0x22
	int8 pad_24[16];
	h1_tag_block<h1_mod2_markers_instances> instances; // 0x34
};
ASSERT_STRUCT_SIZE(h1_mod2_markers, 0x40);

struct h1_mod2_nodes
{
	char name[32]; // 0x0
	int16 next_sibling_node_index; // 0x20
	int16 first_child_node_index; // 0x22
	int16 parent_node_index; // 0x24
	int16 unknown; // 0x26
	real_point3d default_translation; // 0x28
	real_quaternion default_rotation; // 0x34
	real32 node_distance_from_parent; // 0x44
	int8 pad_48[32];
	real32 inverse_scale; // 0x68
	real_vector3d inverse_forward; // 0x6c
	real_vector3d inverse_left; // 0x78
	real_vector3d inverse_up; // 0x84
	real_point3d inverse_position; // 0x90
};
ASSERT_STRUCT_SIZE(h1_mod2_nodes, 0x9c);

struct h1_mod2_regions_permutations_markers
{
	char name[32]; // 0x0
	int16 node_index; // 0x20
	int16 unknown; // 0x22
	real_quaternion rotation; // 0x24
	real_point3d translation; // 0x34
	int8 pad_40[16];
};
ASSERT_STRUCT_SIZE(h1_mod2_regions_permutations_markers, 0x50);

struct h1_mod2_regions_permutations
{
	char name[32]; // 0x0
	uint32 flags; // 0x20
	int8 pad_24[28];
	int16 super_low_index; // 0x40
	int16 low_index; // 0x42
	int16 medium_index; // 0x44
	int16 high_index; // 0x46
	int16 super_high_index; // 0x48
	int16 unknown; // 0x4a
	h1_tag_block<h1_mod2_regions_permutations_markers> markers; // 0x4c
};
ASSERT_STRUCT_SIZE(h1_mod2_regions_permutations, 0x58);

struct h1_mod2_regions
{
	char name[32]; // 0x0
	int8 pad_20[32];
	h1_tag_block<h1_mod2_regions_permutations> permutations; // 0x40
};
ASSERT_STRUCT_SIZE(h1_mod2_regions, 0x4c);

struct h1_mod2_geometries_parts_uncompressed_vertices
{
	real_point3d position; // 0x0
	real_vector3d normal; // 0xc
	real_vector3d binormal; // 0x18
	real_vector3d tangent; // 0x24
	real_point2d texture_coords; // 0x30
	int16 node0_index; // 0x38
	int16 node1_index; // 0x3a
	real32 node0_weight; // 0x3c
	real32 node1_weight; // 0x40
};
ASSERT_STRUCT_SIZE(h1_mod2_geometries_parts_uncompressed_vertices, 0x44);

struct h1_mod2_geometries_parts_compressed_vertices
{
	real_point3d position; // 0x0
	int32 normal_11_11_10_bit; // 0xc
	int32 binormal_11_11_10_bit; // 0x10
	int32 tangent_11_11_10_bit; // 0x14
	int16 texture_coordinate_u_16_bit; // 0x18
	int16 texture_coordinate_v_16_bit; // 0x1a
	int8 node0_index_x3; // 0x1c
	int8 node1_index_x3; // 0x1d
	int16 node0_weight_16_bit; // 0x1e
};
ASSERT_STRUCT_SIZE(h1_mod2_geometries_parts_compressed_vertices, 0x20);

struct h1_mod2_geometries_parts_triangles
{
	int16 vertex0_index; // 0x0
	int16 vertex1_index; // 0x2
	int16 vertex2_index; // 0x4
};
ASSERT_STRUCT_SIZE(h1_mod2_geometries_parts_triangles, 0x6);

struct h1_mod2_geometries_parts
{
	uint32 flags; // 0x0
	int16 shader_index; // 0x4
	int8 prev_filthy_part_index; // 0x6
	int8 next_filthy_part_index; // 0x7
	int16 centroid_primary_node; // 0x8
	int16 centroid_secondary_node; // 0xa
	real32 centroid_primary_weight; // 0xc
	real32 centroid_secondary_weight; // 0x10
	real_point3d centroid; // 0x14
	h1_tag_block<h1_mod2_geometries_parts_uncompressed_vertices> uncompressed_vertices; // 0x20
	h1_tag_block<h1_mod2_geometries_parts_compressed_vertices> compressed_vertices; // 0x2c
	h1_tag_block<h1_mod2_geometries_parts_triangles> triangles; // 0x38
	int32 runtime_unknown_required; // 0x44
	int32 triangle_count; // 0x48
	int32 triangle_offset; // 0x4c
	int32 triangle_offset_2; // 0x50
	int16 vertex_type; // 0x54
	int16 unknown; // 0x56
	int32 vertex_count; // 0x58
	int32 unknown_2; // 0x5c
	uint32 vertex_pointer; // 0x60
	int32 vertex_offset; // 0x64
	int8 unknown_3; // 0x68
	int8 unknown_4; // 0x69
	int8 unknown_5; // 0x6a
	int8 number_of_nodes; // 0x6b
	int8 local_node_index_0; // 0x6c
	int8 local_node_index_1; // 0x6d
	int8 local_node_index_2; // 0x6e
	int8 local_node_index_3; // 0x6f
	int8 local_node_index_4; // 0x70
	int8 local_node_index_5; // 0x71
	int8 local_node_index_6; // 0x72
	int8 local_node_index_7; // 0x73
	int8 local_node_index_8; // 0x74
	int8 local_node_index_9; // 0x75
	int8 local_node_index_10; // 0x76
	int8 local_node_index_11; // 0x77
	int8 local_node_index_12; // 0x78
	int8 local_node_index_13; // 0x79
	int8 local_node_index_14; // 0x7a
	int8 local_node_index_15; // 0x7b
	int8 local_node_index_16; // 0x7c
	int8 local_node_index_17; // 0x7d
	int8 local_node_index_18; // 0x7e
	int8 local_node_index_19; // 0x7f
	int8 local_node_index_20; // 0x80
	int8 local_node_index_21; // 0x81
	int8 local_node_index_22; // 0x82
	int8 local_node_index_23; // 0x83
};
ASSERT_STRUCT_SIZE(h1_mod2_geometries_parts, 0x84);

struct h1_mod2_geometries
{
	uint32 flags; // 0x0
	int8 pad_4[32];
	h1_tag_block<h1_mod2_geometries_parts> parts; // 0x24
};
ASSERT_STRUCT_SIZE(h1_mod2_geometries, 0x30);

struct h1_mod2_shaders
{
	h1_tag_reference shader; // 0x0
	int16 permutation; // 0x10
	int16 unknown; // 0x12
	int8 pad_14[12];
};
ASSERT_STRUCT_SIZE(h1_mod2_shaders, 0x20);

struct h1_mod2
{
	uint32 flags; // 0x0
	int32 node_list_checksum; // 0x4
	real32 super_high_detail_cutoff; // 0x8
	real32 high_detail_cutoff; // 0xc
	real32 medium_detail_cutoff; // 0x10
	real32 low_detail_cutoff; // 0x14
	real32 super_low_cutoff; // 0x18
	int16 super_high_detail_node_count; // 0x1c
	int16 high_detail_node_count; // 0x1e
	int16 medium_detail_node_count; // 0x20
	int16 low_detail_node_count; // 0x22
	int16 super_low_detail_node_count; // 0x24
	int16 unknown; // 0x26
	int8 pad_28[8];
	real32 base_map_u_scale; // 0x30
	real32 base_map_v_scale; // 0x34
	int8 pad_38[116];
	h1_tag_block<h1_mod2_markers> markers; // 0xac
	h1_tag_block<h1_mod2_nodes> nodes; // 0xb8
	h1_tag_block<h1_mod2_regions> regions; // 0xc4
	h1_tag_block<h1_mod2_geometries> geometries; // 0xd0
	h1_tag_block<h1_mod2_shaders> shaders; // 0xdc
};
ASSERT_STRUCT_SIZE(h1_mod2, 0xe8);

struct h1_coll_materials
{
	char name[32]; // 0x0
	uint32 flags; // 0x20
	int16 material_type; // 0x24
	int16 unknown; // 0x26
	real32 shield_leak_percentage; // 0x28
	real32 shield_damage_multiplier; // 0x2c
	int8 pad_30[12];
	real32 body_damage_multiplier; // 0x3c
	int8 pad_40[8];
};
ASSERT_STRUCT_SIZE(h1_coll_materials, 0x48);

struct h1_coll_regions_permutations
{
	char name[32]; // 0x0
};
ASSERT_STRUCT_SIZE(h1_coll_regions_permutations, 0x20);

struct h1_coll_regions
{
	char name[32]; // 0x0
	uint32 flags; // 0x20
	int8 pad_24[4];
	real32 damage_threshold; // 0x28
	int8 pad_2c[12];
	h1_tag_reference destroyed_effect; // 0x38
	h1_tag_block<h1_coll_regions_permutations> permutations; // 0x48
};
ASSERT_STRUCT_SIZE(h1_coll_regions, 0x54);

struct h1_coll_modifiers
{
	int8 pad_0[52];
};
ASSERT_STRUCT_SIZE(h1_coll_modifiers, 0x34);

struct h1_coll_pathfinding_spheres
{
	int16 node_index; // 0x0
	int16 unknown; // 0x2
	int8 pad_4[12];
	real_point3d center; // 0x10
	real32 radius; // 0x1c
};
ASSERT_STRUCT_SIZE(h1_coll_pathfinding_spheres, 0x20);

struct h1_coll_nodes_bsps_bsp3d_nodes
{
	int32 plane; // 0x0
	int32 back_child; // 0x4
	int32 front_child; // 0x8
};
ASSERT_STRUCT_SIZE(h1_coll_nodes_bsps_bsp3d_nodes, 0xc);

struct h1_coll_nodes_bsps_planes
{
	real_plane3d plane; // 0x0
};
ASSERT_STRUCT_SIZE(h1_coll_nodes_bsps_planes, 0x10);

struct h1_coll_nodes_bsps_leaves
{
	uint16 flags; // 0x0
	int16 bsp2d_reference_count; // 0x2
	int32 first_bsp2d_reference; // 0x4
};
ASSERT_STRUCT_SIZE(h1_coll_nodes_bsps_leaves, 0x8);

struct h1_coll_nodes_bsps_bsp2d_references
{
	int32 plane; // 0x0
	int32 bsp2d_node; // 0x4
};
ASSERT_STRUCT_SIZE(h1_coll_nodes_bsps_bsp2d_references, 0x8);

struct h1_coll_nodes_bsps_bsp2d_nodes
{
	real_plane2d plane; // 0x0
	int32 left_child; // 0xc
	int32 right_child; // 0x10
};
ASSERT_STRUCT_SIZE(h1_coll_nodes_bsps_bsp2d_nodes, 0x14);

struct h1_coll_nodes_bsps_surfaces
{
	int32 plane; // 0x0
	int32 first_edge; // 0x4
	uint8 flags; // 0x8
	int8 breakable_surface; // 0x9
	int16 material; // 0xa
};
ASSERT_STRUCT_SIZE(h1_coll_nodes_bsps_surfaces, 0xc);

struct h1_coll_nodes_bsps_edges
{
	int32 start_vertex; // 0x0
	int32 end_vertex; // 0x4
	int32 forward_edge; // 0x8
	int32 reverse_edge; // 0xc
	int32 left_surface; // 0x10
	int32 right_surface; // 0x14
};
ASSERT_STRUCT_SIZE(h1_coll_nodes_bsps_edges, 0x18);

struct h1_coll_nodes_bsps_vertices
{
	real_point3d point; // 0x0
	int32 first_edge; // 0xc
};
ASSERT_STRUCT_SIZE(h1_coll_nodes_bsps_vertices, 0x10);

struct h1_coll_nodes_bsps
{
	h1_tag_block<h1_coll_nodes_bsps_bsp3d_nodes> bsp3d_nodes; // 0x0
	h1_tag_block<h1_coll_nodes_bsps_planes> planes; // 0xc
	h1_tag_block<h1_coll_nodes_bsps_leaves> leaves; // 0x18
	h1_tag_block<h1_coll_nodes_bsps_bsp2d_references> bsp2d_references; // 0x24
	h1_tag_block<h1_coll_nodes_bsps_bsp2d_nodes> bsp2d_nodes; // 0x30
	h1_tag_block<h1_coll_nodes_bsps_surfaces> surfaces; // 0x3c
	h1_tag_block<h1_coll_nodes_bsps_edges> edges; // 0x48
	h1_tag_block<h1_coll_nodes_bsps_vertices> vertices; // 0x54
};
ASSERT_STRUCT_SIZE(h1_coll_nodes_bsps, 0x60);

struct h1_coll_nodes
{
	char name[32]; // 0x0
	int16 region_index; // 0x20
	int16 parent_node_index; // 0x22
	int16 next_sibling_node_index; // 0x24
	int16 first_child_node_index; // 0x26
	int8 pad_28[8];
	int16 unknown; // 0x30
	int16 unknown_name; // 0x32
	h1_tag_block<h1_coll_nodes_bsps> bsps; // 0x34
};
ASSERT_STRUCT_SIZE(h1_coll_nodes, 0x40);

struct h1_coll
{
	uint32 flags; // 0x0
	int16 indirect_damage_material_index; // 0x4
	int16 unknown; // 0x6
	real32 maximum_body_vitality; // 0x8
	real32 body_system_shock; // 0xc
	int8 pad_10[52];
	real32 friendly_damage_resistance; // 0x44
	int8 pad_48[40];
	h1_tag_reference localized_damage_effect; // 0x70
	real32 area_damage_effect_threshold; // 0x80
	h1_tag_reference area_damage_effect; // 0x84
	real32 body_damaged_threshold; // 0x94
	h1_tag_reference body_damaged_effect; // 0x98
	h1_tag_reference body_depleted_effect; // 0xa8
	real32 body_destroyed_threshold; // 0xb8
	h1_tag_reference body_destroyed_effect; // 0xbc
	real32 maximum_shield_vitality; // 0xcc
	int16 unknown_2; // 0xd0
	int16 shield_material_type; // 0xd2
	int8 pad_d4[24];
	int16 shield_failure_function; // 0xec
	int16 unknown_3; // 0xee
	real32 shield_failure_threshold; // 0xf0
	real32 failing_shield_leak_fraction; // 0xf4
	int8 pad_f8[16];
	real32 minimum_stun_damage; // 0x108
	real32 stun_time; // 0x10c
	real32 recharge_time; // 0x110
	int8 pad_114[112];
	real32 shield_damaged_threshold; // 0x184
	h1_tag_reference shield_damaged_effect; // 0x188
	h1_tag_reference shield_depleted_effect; // 0x198
	h1_tag_reference shield_recharging_effect; // 0x1a8
	int8 pad_1b8[8];
	real32 shield_recharge_rate; // 0x1c0
	int8 pad_1c4[112];
	h1_tag_block<h1_coll_materials> materials; // 0x234
	h1_tag_block<h1_coll_regions> regions; // 0x240
	h1_tag_block<h1_coll_modifiers> modifiers; // 0x24c
	int8 pad_258[16];
	real_bounds bounds_x; // 0x268
	real_bounds bounds_y; // 0x270
	real_bounds bounds_z; // 0x278
	h1_tag_block<h1_coll_pathfinding_spheres> pathfinding_spheres; // 0x280
	h1_tag_block<h1_coll_nodes> nodes; // 0x28c
};
ASSERT_STRUCT_SIZE(h1_coll, 0x298);

struct h1_scen_attachments
{
	h1_tag_reference type; // 0x0
	char marker[32]; // 0x10
	int16 primary_scale; // 0x30
	int16 secondary_scale; // 0x32
	int16 change_color; // 0x34
	int16 unknown; // 0x36
	int8 pad_38[16];
};
ASSERT_STRUCT_SIZE(h1_scen_attachments, 0x48);

struct h1_scen_widgets
{
	h1_tag_reference reference; // 0x0
	int8 pad_10[16];
};
ASSERT_STRUCT_SIZE(h1_scen_widgets, 0x20);

struct h1_scen_functions
{
	uint32 flags; // 0x0
	real32 period; // 0x4
	int16 scale_period_by; // 0x8
	int16 function; // 0xa
	int16 scale_function_by; // 0xc
	int16 wobble_function; // 0xe
	real32 wobble_period; // 0x10
	real32 wobble_magnitude; // 0x14
	real32 square_wave_threshold; // 0x18
	int16 step_count; // 0x1c
	int16 map_to; // 0x1e
	int16 sawtooth_count; // 0x20
	int16 add; // 0x22
	int16 scale_result_by; // 0x24
	int16 bounds_mode; // 0x26
	real_bounds bounds; // 0x28
	int8 pad_30[4];
	int16 unknown; // 0x34
	int16 turn_off_with_index; // 0x36
	real32 scale_by; // 0x38
	int8 pad_3c[252];
	real32 inverse_bounds; // 0x138
	real32 inverse_sawtooth; // 0x13c
	real32 inverse_step; // 0x140
	real32 inverse_period; // 0x144
	char usage[32]; // 0x148
};
ASSERT_STRUCT_SIZE(h1_scen_functions, 0x168);

struct h1_scen_change_colors_permutations
{
	real32 weight; // 0x0
	real_rgb_color color_lower_bound; // 0x4
	real_rgb_color color_upper_bound; // 0x10
};
ASSERT_STRUCT_SIZE(h1_scen_change_colors_permutations, 0x1c);

struct h1_scen_change_colors
{
	int16 darken_by; // 0x0
	int16 scale_by; // 0x2
	uint32 scale_flags; // 0x4
	real_rgb_color color_lower_bound; // 0x8
	real_rgb_color color_upper_bound; // 0x14
	h1_tag_block<h1_scen_change_colors_permutations> permutations; // 0x20
};
ASSERT_STRUCT_SIZE(h1_scen_change_colors, 0x2c);

struct h1_scen_predicted_resources
{
	int16 type; // 0x0
	int16 resource_index; // 0x2
	datum tag_index; // 0x4
};
ASSERT_STRUCT_SIZE(h1_scen_predicted_resources, 0x8);

struct h1_scen
{
	int16 object_type; // 0x0
	uint16 flags; // 0x2
	real32 bounding_radius; // 0x4
	real_point3d bounding_offset; // 0x8
	real_point3d origin_offset; // 0x14
	real32 acceleration_scale; // 0x20
	uint32 runtime_flags; // 0x24
	h1_tag_reference model; // 0x28
	h1_tag_reference animation_graph; // 0x38
	int8 pad_48[40];
	h1_tag_reference collision_model; // 0x70
	h1_tag_reference physics; // 0x80
	h1_tag_reference modifier_shader; // 0x90
	h1_tag_reference creation_effect; // 0xa0
	int8 pad_b0[84];
	real32 render_bounding_radius; // 0x104
	int16 a_in; // 0x108
	int16 b_in; // 0x10a
	int16 c_in; // 0x10c
	int16 d_in; // 0x10e
	int8 pad_110[44];
	int16 hud_text_message_index; // 0x13c
	int16 forced_shader_permutation_index; // 0x13e
	h1_tag_block<h1_scen_attachments> attachments; // 0x140
	h1_tag_block<h1_scen_widgets> widgets; // 0x14c
	h1_tag_block<h1_scen_functions> functions; // 0x158
	h1_tag_block<h1_scen_change_colors> change_colors; // 0x164
	h1_tag_block<h1_scen_predicted_resources> predicted_resources; // 0x170
	int8 pad_17c[128];
};
ASSERT_STRUCT_SIZE(h1_scen, 0x1fc);

struct h1_itmc_item_permutations
{
	int8 pad_0[32];
	real32 weight; // 0x20
	h1_tag_reference item; // 0x24
	int8 pad_34[32];
};
ASSERT_STRUCT_SIZE(h1_itmc_item_permutations, 0x54);

struct h1_itmc
{
	h1_tag_block<h1_itmc_item_permutations> item_permutations; // 0x0
	int16 spawn_time_in_seconds_0_default; // 0xc
	int16 unknown; // 0xe
	int8 pad_10[76];
};
ASSERT_STRUCT_SIZE(h1_itmc, 0x5c);

struct h1_fog
{
	uint32 flags; // 0x0
	int8 pad_4[84];
	real32 maximum_density; // 0x58
	int8 pad_5c[4];
	real32 opaque_distance; // 0x60
	int8 pad_64[4];
	real32 opaque_depth; // 0x68
	int8 pad_6c[8];
	real32 distance_to_water_plane; // 0x74
	real_rgb_color color; // 0x78
	uint16 flags_2; // 0x84
	int16 layer_count; // 0x86
	real_bounds distance_gradient; // 0x88
	real_bounds density_gradient; // 0x90
	real32 start_distance_from_fog_plane; // 0x98
	int8 pad_9c[4];
	uint32 color_2; // 0xa0
	real32 rotation_multiplier; // 0xa4
	real32 strafing_multiplier; // 0xa8
	real32 zoom_multiplier; // 0xac
	int8 pad_b0[8];
	real32 map_scale; // 0xb8
	h1_tag_reference map; // 0xbc
	real32 animation_period; // 0xcc
	int8 pad_d0[4];
	real_bounds wind_velocity; // 0xd4
	real_bounds wind_period; // 0xdc
	real32 wind_acceleration_weight; // 0xe4
	real32 wind_perpendicular_weight; // 0xe8
	int8 pad_ec[8];
	h1_tag_reference background_sound; // 0xf4
	h1_tag_reference sound_environment; // 0x104
	int8 pad_114[120];
};
ASSERT_STRUCT_SIZE(h1_fog, 0x18c);

struct h1_ligh
{
	uint32 flags; // 0x0
	real32 radius; // 0x4
	real_bounds radius_modifier; // 0x8
	real32 falloff_angle; // 0x10
	real32 cutoff_angle; // 0x14
	real32 lens_flare_only_radius; // 0x18
	real32 cosine_falloff_angle; // 0x1c
	real32 cosine_cutoff_angle; // 0x20
	real32 sine_falloff_angle; // 0x24
	real32 sine_cutoff_angle; // 0x28
	int8 pad_2c[8];
	uint32 interpolation_flags; // 0x34
	real_argb_color color_lower_bound; // 0x38
	real_argb_color color_upper_bound; // 0x48
	int8 pad_58[12];
	h1_tag_reference primary_cube_map; // 0x64
	int16 unknown; // 0x74
	int16 texture_animation_function; // 0x76
	real32 texture_animation_period; // 0x78
	h1_tag_reference secondary_cube_map; // 0x7c
	int16 unknown_2; // 0x8c
	int16 yaw_function; // 0x8e
	real32 yaw_period; // 0x90
	int16 unknown_3; // 0x94
	int16 roll_function; // 0x96
	real32 roll_period; // 0x98
	int16 unknown_4; // 0x9c
	int16 pitch_function; // 0x9e
	real32 pitch_period; // 0xa0
	int8 pad_a4[8];
	h1_tag_reference lens_flare; // 0xac
	int8 pad_bc[24];
	real32 intensity; // 0xd4
	real_rgb_color color; // 0xd8
	int8 pad_e4[16];
	real32 duration; // 0xf4
	int16 unknown_5; // 0xf8
	int16 falloff_function; // 0xfa
	int8 pad_fc[100];
};
ASSERT_STRUCT_SIZE(h1_ligh, 0x160);

struct h1_lens_reflections
{
	uint16 flags; // 0x0
	int16 unknown; // 0x2
	int16 bitmap_index; // 0x4
	int16 unknown_2; // 0x6
	int8 pad_8[20];
	real32 position; // 0x1c
	real32 rotation_offset; // 0x20
	int8 pad_24[4];
	real_bounds radius; // 0x28
	int16 radius_scaled_by; // 0x30
	int16 unknown_3; // 0x32
	real_bounds brightness; // 0x34
	int16 brightness_scaled_by; // 0x3c
	int16 unknown_4; // 0x3e
	real_argb_color tint_color; // 0x40
	real_argb_color color_lower_bound; // 0x50
	real_argb_color color_upper_bound; // 0x60
	uint16 flags_2; // 0x70
	int16 animation_function; // 0x72
	real32 animation_period; // 0x74
	real32 animation_phase; // 0x78
	int8 pad_7c[4];
};
ASSERT_STRUCT_SIZE(h1_lens_reflections, 0x80);

struct h1_lens
{
	real32 falloff_angle; // 0x0
	real32 cutoff_angle; // 0x4
	real32 cosine_falloff_angle; // 0x8
	real32 cosine_cutoff_angle; // 0xc
	real32 occlusion_radius; // 0x10
	int16 occlusion_offset_direction; // 0x14
	int16 unknown; // 0x16
	real32 near_fade_distance; // 0x18
	real32 far_fade_distance; // 0x1c
	h1_tag_reference bitmap; // 0x20
	uint16 flags; // 0x30
	int16 unknown_2; // 0x32
	int8 pad_34[76];
	int16 rotation_function; // 0x80
	int16 unknown_3; // 0x82
	real32 rotation_function_scale; // 0x84
	int8 pad_88[24];
	real32 horizontal_scale; // 0xa0
	real32 vertical_scale; // 0xa4
	int8 pad_a8[28];
	h1_tag_block<h1_lens_reflections> reflections; // 0xc4
	int8 pad_d0[32];
};
ASSERT_STRUCT_SIZE(h1_lens, 0xf0);

struct h1_itmc_2_item_permutations
{
	int8 pad_0[32];
	real32 weight; // 0x20
	h1_tag_reference item; // 0x24
	int8 pad_34[32];
};
ASSERT_STRUCT_SIZE(h1_itmc_2_item_permutations, 0x54);

struct h1_itmc_2
{
	h1_tag_block<h1_itmc_2_item_permutations> item_permutations; // 0x0
	int16 spawn_time_in_seconds_0_default; // 0xc
	int16 unknown; // 0xe
	int8 pad_10[76];
};
ASSERT_STRUCT_SIZE(h1_itmc_2, 0x5c);

struct h1_vehi_attachments
{
	h1_tag_reference type; // 0x0
	char marker[32]; // 0x10
	int16 primary_scale; // 0x30
	int16 secondary_scale; // 0x32
	int16 change_color; // 0x34
	int16 unknown; // 0x36
	int8 pad_38[16];
};
ASSERT_STRUCT_SIZE(h1_vehi_attachments, 0x48);

struct h1_vehi_widgets
{
	h1_tag_reference reference; // 0x0
	int8 pad_10[16];
};
ASSERT_STRUCT_SIZE(h1_vehi_widgets, 0x20);

struct h1_vehi_functions
{
	uint32 flags; // 0x0
	real32 period; // 0x4
	int16 scale_period_by; // 0x8
	int16 function; // 0xa
	int16 scale_function_by; // 0xc
	int16 wobble_function; // 0xe
	real32 wobble_period; // 0x10
	real32 wobble_magnitude; // 0x14
	real32 square_wave_threshold; // 0x18
	int16 step_count; // 0x1c
	int16 map_to; // 0x1e
	int16 sawtooth_count; // 0x20
	int16 add; // 0x22
	int16 scale_result_by; // 0x24
	int16 bounds_mode; // 0x26
	real_bounds bounds; // 0x28
	int8 pad_30[4];
	int16 unknown; // 0x34
	int16 turn_off_with_index; // 0x36
	real32 scale_by; // 0x38
	int8 pad_3c[252];
	real32 inverse_bounds; // 0x138
	real32 inverse_sawtooth; // 0x13c
	real32 inverse_step; // 0x140
	real32 inverse_period; // 0x144
	char usage[32]; // 0x148
};
ASSERT_STRUCT_SIZE(h1_vehi_functions, 0x168);

struct h1_vehi_change_colors_permutations
{
	real32 weight; // 0x0
	real_rgb_color color_lower_bound; // 0x4
	real_rgb_color color_upper_bound; // 0x10
};
ASSERT_STRUCT_SIZE(h1_vehi_change_colors_permutations, 0x1c);

struct h1_vehi_change_colors
{
	int16 darken_by; // 0x0
	int16 scale_by; // 0x2
	uint32 scale_flags; // 0x4
	real_rgb_color color_lower_bound; // 0x8
	real_rgb_color color_upper_bound; // 0x14
	h1_tag_block<h1_vehi_change_colors_permutations> permutations; // 0x20
};
ASSERT_STRUCT_SIZE(h1_vehi_change_colors, 0x2c);

struct h1_vehi_predicted_resources
{
	int16 type; // 0x0
	int16 resource_index; // 0x2
	datum tag_index; // 0x4
};
ASSERT_STRUCT_SIZE(h1_vehi_predicted_resources, 0x8);

struct h1_vehi_camera_tracks
{
	h1_tag_reference track; // 0x0
	int8 pad_10[12];
};
ASSERT_STRUCT_SIZE(h1_vehi_camera_tracks, 0x1c);

struct h1_vehi_new_hud_interfaces
{
	h1_tag_reference unit_hud_interface; // 0x0
	int8 pad_10[32];
};
ASSERT_STRUCT_SIZE(h1_vehi_new_hud_interfaces, 0x30);

struct h1_vehi_dialogue_variants
{
	int16 variant_number; // 0x0
	int16 unknown; // 0x2
	int8 pad_4[4];
	h1_tag_reference dialogue; // 0x8
};
ASSERT_STRUCT_SIZE(h1_vehi_dialogue_variants, 0x18);

struct h1_vehi_powered_seats
{
	int8 pad_0[4];
	real32 driver_powerup_time; // 0x4
	real32 driver_powerdown_time; // 0x8
	int8 pad_c[56];
};
ASSERT_STRUCT_SIZE(h1_vehi_powered_seats, 0x44);

struct h1_vehi_weapons
{
	h1_tag_reference weapon; // 0x0
	int8 pad_10[20];
};
ASSERT_STRUCT_SIZE(h1_vehi_weapons, 0x24);

struct h1_vehi_seats_camera_tracks
{
	h1_tag_reference track; // 0x0
	int8 pad_10[12];
};
ASSERT_STRUCT_SIZE(h1_vehi_seats_camera_tracks, 0x1c);

struct h1_vehi_seats_unit_hud_interface
{
	h1_tag_reference unit_hud_interface; // 0x0
	int8 pad_10[32];
};
ASSERT_STRUCT_SIZE(h1_vehi_seats_unit_hud_interface, 0x30);

struct h1_vehi_seats
{
	uint32 flags; // 0x0
	char label[32]; // 0x4
	char marker_name[32]; // 0x24
	int8 pad_44[32];
	real_vector3d acceleration_scale; // 0x64
	int8 pad_70[12];
	real32 yaw_rate; // 0x7c
	real32 pitch_rate; // 0x80
	char camera_marker_name[32]; // 0x84
	char camera_submerged_marker_name[32]; // 0xa4
	real32 pitch_auto_level; // 0xc4
	real_bounds pitch_range; // 0xc8
	h1_tag_block<h1_vehi_seats_camera_tracks> camera_tracks; // 0xd0
	h1_tag_block<h1_vehi_seats_unit_hud_interface> unit_hud_interface; // 0xdc
	int8 pad_e8[4];
	int16 hud_text_message_index; // 0xec
	int16 unknown; // 0xee
	real_bounds yaw; // 0xf0
	h1_tag_reference built_in_gunner; // 0xf8
	int8 pad_108[20];
};
ASSERT_STRUCT_SIZE(h1_vehi_seats, 0x11c);

struct h1_vehi
{
	int16 object_type; // 0x0
	uint16 flags; // 0x2
	real32 bounding_radius; // 0x4
	real_point3d bounding_offset; // 0x8
	real_point3d origin_offset; // 0x14
	real32 acceleration_scale; // 0x20
	uint32 runtime_flags; // 0x24
	h1_tag_reference model; // 0x28
	h1_tag_reference animation_graph; // 0x38
	int8 pad_48[40];
	h1_tag_reference collision_model; // 0x70
	h1_tag_reference physics; // 0x80
	h1_tag_reference modifier_shader; // 0x90
	h1_tag_reference creation_effect; // 0xa0
	int8 pad_b0[84];
	real32 render_bounding_radius; // 0x104
	int16 a_in; // 0x108
	int16 b_in; // 0x10a
	int16 c_in; // 0x10c
	int16 d_in; // 0x10e
	int8 pad_110[44];
	int16 hud_text_message_index; // 0x13c
	int16 forced_shader_permutation_index; // 0x13e
	h1_tag_block<h1_vehi_attachments> attachments; // 0x140
	h1_tag_block<h1_vehi_widgets> widgets; // 0x14c
	h1_tag_block<h1_vehi_functions> functions; // 0x158
	h1_tag_block<h1_vehi_change_colors> change_colors; // 0x164
	h1_tag_block<h1_vehi_predicted_resources> predicted_resources; // 0x170
	uint32 flags_2; // 0x17c
	int16 default_team; // 0x180
	int16 constant_sound_volume; // 0x182
	real32 rider_damage_fraction; // 0x184
	h1_tag_reference integrated_light_toggle; // 0x188
	int16 a_in_2; // 0x198
	int16 b_in_2; // 0x19a
	int16 c_in_2; // 0x19c
	int16 d_in_2; // 0x19e
	real32 camera_field_of_view; // 0x1a0
	real32 camera_stiffness; // 0x1a4
	char camera_marker_name[32]; // 0x1a8
	char camera_submerged_marker_name[32]; // 0x1c8
	real32 pitch_auto_level; // 0x1e8
	real_bounds pitch_range; // 0x1ec
	h1_tag_block<h1_vehi_camera_tracks> camera_tracks; // 0x1f4
	real_vector3d seat_acceleration_scale; // 0x200
	int8 pad_20c[12];
	real32 soft_ping_threshold; // 0x218
	real32 soft_ping_interrupt_time; // 0x21c
	real32 hard_ping_threshold; // 0x220
	real32 hard_ping_interrupt_time; // 0x224
	real32 hard_death_threshold; // 0x228
	real32 feign_death_threshold; // 0x22c
	real32 feign_death_time; // 0x230
	real32 distance_of_evade_animation; // 0x234
	real32 distance_of_dive_animation; // 0x238
	int8 pad_23c[4];
	real32 stunned_movement_threshold; // 0x240
	real32 feign_death_chance; // 0x244
	real32 feign_repeat_chance; // 0x248
	h1_tag_reference spawned_actor; // 0x24c
	short_bounds spawned_actor_count; // 0x25c
	real32 spawned_velocity; // 0x260
	real32 aiming_velocity_maximum; // 0x264
	real32 aiming_acceleration_maximum; // 0x268
	real32 casual_aiming_modifier; // 0x26c
	real32 looking_velocity_maximum; // 0x270
	real32 looking_acceleration_maximum; // 0x274
	int8 pad_278[8];
	real32 ai_vehicle_radius; // 0x280
	real32 ai_danger_radius; // 0x284
	h1_tag_reference melee_damage; // 0x288
	int16 motion_sensor_blip_size; // 0x298
	int16 unknown; // 0x29a
	int8 pad_29c[12];
	h1_tag_block<h1_vehi_new_hud_interfaces> new_hud_interfaces; // 0x2a8
	h1_tag_block<h1_vehi_dialogue_variants> dialogue_variants; // 0x2b4
	real32 grenade_velocity; // 0x2c0
	int16 grenade_type; // 0x2c4
	int16 grenade_count; // 0x2c6
	int16 soft_ping_interrupt_ticks; // 0x2c8
	int16 hard_ping_interrupt_ticks; // 0x2ca
	h1_tag_block<h1_vehi_powered_seats> powered_seats; // 0x2cc
	h1_tag_block<h1_vehi_weapons> weapons; // 0x2d8
	h1_tag_block<h1_vehi_seats> seats; // 0x2e4
	uint32 flags_3; // 0x2f0
	int16 type; // 0x2f4
	int16 unknown_2; // 0x2f6
	real32 maximum_forward_speed; // 0x2f8
	real32 maximum_reverse_speed; // 0x2fc
	real32 speed_acceleration; // 0x300
	real32 speed_deceleration; // 0x304
	real32 maximum_left_turn; // 0x308
	real32 maximum_right_turn_negative; // 0x30c
	real32 wheel_circumference; // 0x310
	real32 turn_rate; // 0x314
	real32 blur_speed; // 0x318
	int16 a_in_3; // 0x31c
	int16 b_in_3; // 0x31e
	int16 c_in_3; // 0x320
	int16 d_in_3; // 0x322
	int8 pad_324[12];
	real32 maximum_left_slide; // 0x330
	real32 maximum_right_slide; // 0x334
	real32 slide_acceleration; // 0x338
	real32 slide_deceleration; // 0x33c
	real32 minimum_flipping_angular_velocity; // 0x340
	real32 maximum_flipping_angular_velocity; // 0x344
	int8 pad_348[24];
	real32 fixed_gun_yaw; // 0x360
	real32 fixed_gun_pitch; // 0x364
	int8 pad_368[24];
	real32 ai_sideslip_distance; // 0x380
	real32 ai_destination_radius; // 0x384
	real32 ai_avoidance_distance; // 0x388
	real32 ai_pathfinding_radius; // 0x38c
	real32 ai_charge_repeat_timeout; // 0x390
	real32 ai_strafing_abort_range; // 0x394
	real_bounds ai_oversteering_bounds; // 0x398
	real32 ai_steering_maximum; // 0x3a0
	real32 ai_throttle_maximum; // 0x3a4
	real32 ai_move_position_time; // 0x3a8
	int8 pad_3ac[4];
	h1_tag_reference suspension_sound; // 0x3b0
	h1_tag_reference crash_sound; // 0x3c0
	h1_tag_reference material_effects; // 0x3d0
	h1_tag_reference effect; // 0x3e0
};
ASSERT_STRUCT_SIZE(h1_vehi, 0x3f0);

struct h1_eqip_attachments
{
	h1_tag_reference type; // 0x0
	char marker[32]; // 0x10
	int16 primary_scale; // 0x30
	int16 secondary_scale; // 0x32
	int16 change_color; // 0x34
	int16 unknown; // 0x36
	int8 pad_38[16];
};
ASSERT_STRUCT_SIZE(h1_eqip_attachments, 0x48);

struct h1_eqip_widgets
{
	h1_tag_reference reference; // 0x0
	int8 pad_10[16];
};
ASSERT_STRUCT_SIZE(h1_eqip_widgets, 0x20);

struct h1_eqip_functions
{
	uint32 flags; // 0x0
	real32 period; // 0x4
	int16 scale_period_by; // 0x8
	int16 function; // 0xa
	int16 scale_function_by; // 0xc
	int16 wobble_function; // 0xe
	real32 wobble_period; // 0x10
	real32 wobble_magnitude; // 0x14
	real32 square_wave_threshold; // 0x18
	int16 step_count; // 0x1c
	int16 map_to; // 0x1e
	int16 sawtooth_count; // 0x20
	int16 add; // 0x22
	int16 scale_result_by; // 0x24
	int16 bounds_mode; // 0x26
	real_bounds bounds; // 0x28
	int8 pad_30[4];
	int16 unknown; // 0x34
	int16 turn_off_with_index; // 0x36
	real32 scale_by; // 0x38
	int8 pad_3c[252];
	real32 inverse_bounds; // 0x138
	real32 inverse_sawtooth; // 0x13c
	real32 inverse_step; // 0x140
	real32 inverse_period; // 0x144
	char usage[32]; // 0x148
};
ASSERT_STRUCT_SIZE(h1_eqip_functions, 0x168);

struct h1_eqip_change_colors_permutations
{
	real32 weight; // 0x0
	real_rgb_color color_lower_bound; // 0x4
	real_rgb_color color_upper_bound; // 0x10
};
ASSERT_STRUCT_SIZE(h1_eqip_change_colors_permutations, 0x1c);

struct h1_eqip_change_colors
{
	int16 darken_by; // 0x0
	int16 scale_by; // 0x2
	uint32 scale_flags; // 0x4
	real_rgb_color color_lower_bound; // 0x8
	real_rgb_color color_upper_bound; // 0x14
	h1_tag_block<h1_eqip_change_colors_permutations> permutations; // 0x20
};
ASSERT_STRUCT_SIZE(h1_eqip_change_colors, 0x2c);

struct h1_eqip_predicted_resources
{
	int16 type; // 0x0
	int16 resource_index; // 0x2
	datum tag_index; // 0x4
};
ASSERT_STRUCT_SIZE(h1_eqip_predicted_resources, 0x8);

struct h1_eqip
{
	int16 object_type; // 0x0
	uint16 flags; // 0x2
	real32 bounding_radius; // 0x4
	real_point3d bounding_offset; // 0x8
	real_point3d origin_offset; // 0x14
	real32 acceleration_scale; // 0x20
	uint32 runtime_flags; // 0x24
	h1_tag_reference model; // 0x28
	h1_tag_reference animation_graph; // 0x38
	int8 pad_48[40];
	h1_tag_reference collision_model; // 0x70
	h1_tag_reference physics; // 0x80
	h1_tag_reference modifier_shader; // 0x90
	h1_tag_reference creation_effect; // 0xa0
	int8 pad_b0[84];
	real32 render_bounding_radius; // 0x104
	int16 a_in; // 0x108
	int16 b_in; // 0x10a
	int16 c_in; // 0x10c
	int16 d_in; // 0x10e
	int8 pad_110[44];
	int16 hud_text_message_index; // 0x13c
	int16 forced_shader_permutation_index; // 0x13e
	h1_tag_block<h1_eqip_attachments> attachments; // 0x140
	h1_tag_block<h1_eqip_widgets> widgets; // 0x14c
	h1_tag_block<h1_eqip_functions> functions; // 0x158
	h1_tag_block<h1_eqip_change_colors> change_colors; // 0x164
	h1_tag_block<h1_eqip_predicted_resources> predicted_resources; // 0x170
	uint32 flags_2; // 0x17c
	int16 message_index; // 0x180
	int16 sort_order; // 0x182
	real32 scale; // 0x184
	int16 hud_message_value_scale; // 0x188
	int16 unknown; // 0x18a
	int8 pad_18c[16];
	int16 a_in_2; // 0x19c
	int16 b_in_2; // 0x19e
	int16 c_in_2; // 0x1a0
	int16 d_in_2; // 0x1a2
	int8 pad_1a4[164];
	h1_tag_reference material_effects; // 0x248
	h1_tag_reference collision_sound; // 0x258
	int8 pad_268[120];
	real_bounds detonation_delay; // 0x2e0
	h1_tag_reference detonating_effect; // 0x2e8
	h1_tag_reference detonation_effect; // 0x2f8
	int16 powerup_type; // 0x308
	int16 grenade_type; // 0x30a
	real32 powerup_time; // 0x30c
	h1_tag_reference pickup_sound; // 0x310
	int8 pad_320[144];
};
ASSERT_STRUCT_SIZE(h1_eqip, 0x3b0);

struct h1_weap_attachments
{
	h1_tag_reference type; // 0x0
	char marker[32]; // 0x10
	int16 primary_scale; // 0x30
	int16 secondary_scale; // 0x32
	int16 change_color; // 0x34
	int16 unknown; // 0x36
	int8 pad_38[16];
};
ASSERT_STRUCT_SIZE(h1_weap_attachments, 0x48);

struct h1_weap_widgets
{
	h1_tag_reference reference; // 0x0
	int8 pad_10[16];
};
ASSERT_STRUCT_SIZE(h1_weap_widgets, 0x20);

struct h1_weap_functions
{
	uint32 flags; // 0x0
	real32 period; // 0x4
	int16 scale_period_by; // 0x8
	int16 function; // 0xa
	int16 scale_function_by; // 0xc
	int16 wobble_function; // 0xe
	real32 wobble_period; // 0x10
	real32 wobble_magnitude; // 0x14
	real32 square_wave_threshold; // 0x18
	int16 step_count; // 0x1c
	int16 map_to; // 0x1e
	int16 sawtooth_count; // 0x20
	int16 add; // 0x22
	int16 scale_result_by; // 0x24
	int16 bounds_mode; // 0x26
	real_bounds bounds; // 0x28
	int8 pad_30[4];
	int16 unknown; // 0x34
	int16 turn_off_with_index; // 0x36
	real32 scale_by; // 0x38
	int8 pad_3c[252];
	real32 inverse_bounds; // 0x138
	real32 inverse_sawtooth; // 0x13c
	real32 inverse_step; // 0x140
	real32 inverse_period; // 0x144
	char usage[32]; // 0x148
};
ASSERT_STRUCT_SIZE(h1_weap_functions, 0x168);

struct h1_weap_change_colors_permutations
{
	real32 weight; // 0x0
	real_rgb_color color_lower_bound; // 0x4
	real_rgb_color color_upper_bound; // 0x10
};
ASSERT_STRUCT_SIZE(h1_weap_change_colors_permutations, 0x1c);

struct h1_weap_change_colors
{
	int16 darken_by; // 0x0
	int16 scale_by; // 0x2
	uint32 scale_flags; // 0x4
	real_rgb_color color_lower_bound; // 0x8
	real_rgb_color color_upper_bound; // 0x14
	h1_tag_block<h1_weap_change_colors_permutations> permutations; // 0x20
};
ASSERT_STRUCT_SIZE(h1_weap_change_colors, 0x2c);

struct h1_weap_predicted_resources
{
	int16 type; // 0x0
	int16 resource_index; // 0x2
	datum tag_index; // 0x4
};
ASSERT_STRUCT_SIZE(h1_weap_predicted_resources, 0x8);

struct h1_weap_predicted_resources_2
{
	int16 type; // 0x0
	int16 resource_index; // 0x2
	datum tag_index; // 0x4
};
ASSERT_STRUCT_SIZE(h1_weap_predicted_resources_2, 0x8);

struct h1_weap_magazines_magazines
{
	int16 rounds; // 0x0
	int16 unknown; // 0x2
	int8 pad_4[8];
	h1_tag_reference equipment; // 0xc
};
ASSERT_STRUCT_SIZE(h1_weap_magazines_magazines, 0x1c);

struct h1_weap_magazines
{
	uint32 flags; // 0x0
	int16 rounds_recharged; // 0x4
	int16 rounds_total_initial; // 0x6
	int16 rounds_total_maximum; // 0x8
	int16 rounds_loaded_maximum; // 0xa
	int8 pad_c[8];
	real32 reload_time; // 0x14
	int16 rounds_reloaded; // 0x18
	int16 unknown; // 0x1a
	real32 chamber_time; // 0x1c
	int8 pad_20[24];
	h1_tag_reference reloading_effect; // 0x38
	h1_tag_reference chambering_effect; // 0x48
	int8 pad_58[12];
	h1_tag_block<h1_weap_magazines_magazines> magazines; // 0x64
};
ASSERT_STRUCT_SIZE(h1_weap_magazines, 0x70);

struct h1_weap_triggers_firing_effects
{
	int16 shot_count_lower_bound; // 0x0
	int16 shot_count_upper_bound; // 0x2
	int8 pad_4[32];
	h1_tag_reference firing_effect; // 0x24
	h1_tag_reference misfire_effect; // 0x34
	h1_tag_reference empty_effect; // 0x44
	h1_tag_reference firing_damage; // 0x54
	h1_tag_reference misfire_damage; // 0x64
	h1_tag_reference empty_damage; // 0x74
};
ASSERT_STRUCT_SIZE(h1_weap_triggers_firing_effects, 0x84);

struct h1_weap_triggers
{
	uint32 flags; // 0x0
	real_bounds rounds_per_second; // 0x4
	real32 acceleration_time; // 0xc
	real32 deceleration_time; // 0x10
	real32 blurred_rate_of_fire; // 0x14
	int8 pad_18[8];
	int16 magazine_index; // 0x20
	int16 rounds_per_shot; // 0x22
	int16 minimum_rounds_loaded; // 0x24
	int16 rounds_between_tracers; // 0x26
	int8 pad_28[4];
	int16 unknown; // 0x2c
	int16 firing_noise; // 0x2e
	real_bounds error; // 0x30
	real32 acceleration_time_2; // 0x38
	real32 deceleration_time_2; // 0x3c
	int8 pad_40[8];
	real32 charging_time; // 0x48
	real32 charged_time; // 0x4c
	int16 overcharged_action; // 0x50
	int16 unknown_2; // 0x52
	real32 charged_illumination; // 0x54
	real32 spew_time; // 0x58
	h1_tag_reference charging_effect; // 0x5c
	int16 distribution_function; // 0x6c
	int16 projectiles_per_shot; // 0x6e
	real32 distribution_angle; // 0x70
	int8 pad_74[4];
	real32 minimum_error; // 0x78
	real_bounds error_angle; // 0x7c
	real_point3d first_person_offset; // 0x84
	int8 pad_90[4];
	h1_tag_reference projectile; // 0x94
	real32 ejection_port_recovery_time; // 0xa4
	real32 illumination_recovery_time; // 0xa8
	int8 pad_ac[12];
	real32 heat_generated_per_round; // 0xb8
	real32 age_generated_per_round; // 0xbc
	int8 pad_c0[4];
	real32 overload_time; // 0xc4
	int8 pad_c8[40];
	real32 illumination_recovery_rate; // 0xf0
	real32 ejection_port_recovery_rate; // 0xf4
	real32 rate_of_fire_acceleration_rate; // 0xf8
	real32 rate_of_fire_deceleration_rate; // 0xfc
	real32 error_acceleration_rate; // 0x100
	real32 error_deceleration_rate; // 0x104
	h1_tag_block<h1_weap_triggers_firing_effects> firing_effects; // 0x108
};
ASSERT_STRUCT_SIZE(h1_weap_triggers, 0x114);

struct h1_weap
{
	int16 object_type; // 0x0
	uint16 flags; // 0x2
	real32 bounding_radius; // 0x4
	real_point3d bounding_offset; // 0x8
	real_point3d origin_offset; // 0x14
	real32 acceleration_scale; // 0x20
	uint32 runtime_flags; // 0x24
	h1_tag_reference model; // 0x28
	h1_tag_reference animation_graph; // 0x38
	int8 pad_48[40];
	h1_tag_reference collision_model; // 0x70
	h1_tag_reference physics; // 0x80
	h1_tag_reference modifier_shader; // 0x90
	h1_tag_reference creation_effect; // 0xa0
	int8 pad_b0[84];
	real32 render_bounding_radius; // 0x104
	int16 a_in; // 0x108
	int16 b_in; // 0x10a
	int16 c_in; // 0x10c
	int16 d_in; // 0x10e
	int8 pad_110[44];
	int16 hud_text_message_index; // 0x13c
	int16 forced_shader_permutation_index; // 0x13e
	h1_tag_block<h1_weap_attachments> attachments; // 0x140
	h1_tag_block<h1_weap_widgets> widgets; // 0x14c
	h1_tag_block<h1_weap_functions> functions; // 0x158
	h1_tag_block<h1_weap_change_colors> change_colors; // 0x164
	h1_tag_block<h1_weap_predicted_resources> predicted_resources; // 0x170
	uint32 flags_2; // 0x17c
	int16 message_index; // 0x180
	int16 sort_order; // 0x182
	real32 scale; // 0x184
	int16 hud_message_value_scale; // 0x188
	int16 unknown; // 0x18a
	int8 pad_18c[16];
	int16 a_in_2; // 0x19c
	int16 b_in_2; // 0x19e
	int16 c_in_2; // 0x1a0
	int16 d_in_2; // 0x1a2
	int8 pad_1a4[164];
	h1_tag_reference material_effects; // 0x248
	h1_tag_reference collision_sound; // 0x258
	int8 pad_268[120];
	real_bounds detonation_delay; // 0x2e0
	h1_tag_reference detonating_effect; // 0x2e8
	h1_tag_reference detonation_effect; // 0x2f8
	uint32 flags_3; // 0x308
	char label[32]; // 0x30c
	int16 secondary_trigger_mode; // 0x32c
	int16 maximum_alternate_shots_loaded; // 0x32e
	int16 a_in_3; // 0x330
	int16 b_in_3; // 0x332
	int16 c_in_3; // 0x334
	int16 d_in_3; // 0x336
	real32 ready_time; // 0x338
	h1_tag_reference ready_effect; // 0x33c
	real32 heat_recovery_threshold; // 0x34c
	real32 overheated_threshold; // 0x350
	real32 heat_detonation_threshold; // 0x354
	real32 heat_detonation_fraction; // 0x358
	real32 heat_loss_per_second; // 0x35c
	real32 heat_illumination; // 0x360
	int8 pad_364[16];
	h1_tag_reference overheated; // 0x374
	h1_tag_reference detonation; // 0x384
	h1_tag_reference player_melee_damage; // 0x394
	h1_tag_reference player_melee_response; // 0x3a4
	int8 pad_3b4[8];
	h1_tag_reference actor_firing_parameters; // 0x3bc
	real32 near_reticle_range; // 0x3cc
	real32 far_reticle_range; // 0x3d0
	real32 intersection_reticle_range; // 0x3d4
	int16 unknown_2; // 0x3d8
	int16 magnification_levels; // 0x3da
	real_bounds magnification_range; // 0x3dc
	real32 autoaim_angle; // 0x3e4
	real32 autoaim_range; // 0x3e8
	real32 magnetism_angle; // 0x3ec
	real32 magnetism_range; // 0x3f0
	real32 deviation_angle; // 0x3f4
	int8 pad_3f8[4];
	int16 movement_penalized; // 0x3fc
	int16 unknown_3; // 0x3fe
	real32 forward_movement_penalty; // 0x400
	real32 sideways_movement_penalty; // 0x404
	int8 pad_408[4];
	real32 minimum_target_range; // 0x40c
	real32 looking_time_modifier; // 0x410
	int8 pad_414[4];
	real32 light_power_on_time; // 0x418
	real32 light_power_off_time; // 0x41c
	h1_tag_reference light_power_on_effect; // 0x420
	h1_tag_reference light_power_off_effect; // 0x430
	real32 age_heat_recovery_penalty; // 0x440
	real32 age_rate_of_fire_penalty; // 0x444
	real32 age_misfire_start; // 0x448
	real32 age_misfire_chance; // 0x44c
	int8 pad_450[12];
	h1_tag_reference first_person_model; // 0x45c
	h1_tag_reference first_person_animations; // 0x46c
	int8 pad_47c[4];
	h1_tag_reference hud_interface; // 0x480
	h1_tag_reference pickup_sound; // 0x490
	h1_tag_reference zoom_in_sound; // 0x4a0
	h1_tag_reference zoom_out_sound; // 0x4b0
	int8 pad_4c0[12];
	real32 active_camo_ding; // 0x4cc
	real32 active_camo_regrowth_rate; // 0x4d0
	int8 pad_4d4[12];
	int16 unknown_4; // 0x4e0
	int16 weapon_type; // 0x4e2
	h1_tag_block<h1_weap_predicted_resources_2> predicted_resources_2; // 0x4e4
	h1_tag_block<h1_weap_magazines> magazines; // 0x4f0
	h1_tag_block<h1_weap_triggers> triggers; // 0x4fc
};
ASSERT_STRUCT_SIZE(h1_weap, 0x508);

struct h1_snd_pitch_ranges_permutations
{
	char name[32]; // 0x0
	real32 skip_fraction; // 0x20
	real32 gain; // 0x24
	int16 compression; // 0x28
	int16 next_permutation_index; // 0x2a
	uint32 samples_pointer; // 0x2c
	int8 pad_30[4];
	datum f_datum; // 0x34
	int32 buffer_size; // 0x38
	datum f_datum_2; // 0x3c
	int32 sample_size; // 0x40
	int8 pad_44[4];
	uint32 sample_offset; // 0x48
	int8 pad_4c[8];
	h1_tag_data mouth_data; // 0x54
	h1_tag_data subtitle_data; // 0x68
};
ASSERT_STRUCT_SIZE(h1_snd_pitch_ranges_permutations, 0x7c);

struct h1_snd_pitch_ranges
{
	char name[32]; // 0x0
	real32 natural_pitch; // 0x20
	real_bounds bend_bounds; // 0x24
	int16 actual_permutation_count; // 0x2c
	int16 unknown; // 0x2e
	real32 playback_rate; // 0x30
	int32 unknown_2; // 0x34
	int32 unknown_3; // 0x38
	h1_tag_block<h1_snd_pitch_ranges_permutations> permutations; // 0x3c
};
ASSERT_STRUCT_SIZE(h1_snd_pitch_ranges, 0x48);

struct h1_snd
{
	uint32 flags; // 0x0
	int16 f_class; // 0x4
	int16 sample_rate; // 0x6
	real32 minimum_distance; // 0x8
	real32 maximum_distance; // 0xc
	real32 skip_fraction; // 0x10
	real_bounds random_pitch_bounds; // 0x14
	real32 inner_cone_angle; // 0x1c
	real32 outer_cone_angle; // 0x20
	real32 outer_cone_gain; // 0x24
	real32 gain_modifier; // 0x28
	real32 maximum_bend_per_second; // 0x2c
	int8 pad_30[12];
	real32 skip_fraction_modifier; // 0x3c
	real32 gain_modifier_2; // 0x40
	real32 pitch_modifier; // 0x44
	int8 pad_48[12];
	real32 skip_fraction_modifier_2; // 0x54
	real32 gain_modifier_3; // 0x58
	real32 pitch_modifier_2; // 0x5c
	int8 pad_60[12];
	int16 encoding; // 0x6c
	int16 compression; // 0x6e
	h1_tag_reference promotion_sound; // 0x70
	int16 promotion_count; // 0x80
	int16 unknown; // 0x82
	int32 maximum_play_time; // 0x84
	int8 pad_88[8];
	int32 unknown_2; // 0x90
	int32 unknown_3; // 0x94
	h1_tag_block<h1_snd_pitch_ranges> pitch_ranges; // 0x98
};
ASSERT_STRUCT_SIZE(h1_snd, 0xa4);

struct h1_lsnd_tracks
{
	uint32 flags; // 0x0
	real32 gain; // 0x4
	real32 fade_in_duration; // 0x8
	real32 fade_out_duration; // 0xc
	int8 pad_10[32];
	h1_tag_reference start; // 0x30
	h1_tag_reference loop; // 0x40
	h1_tag_reference end; // 0x50
	int8 pad_60[32];
	h1_tag_reference alternate_loop; // 0x80
	h1_tag_reference alternate_end; // 0x90
};
ASSERT_STRUCT_SIZE(h1_lsnd_tracks, 0xa0);

struct h1_lsnd_detail_sounds
{
	h1_tag_reference sound; // 0x0
	real_bounds random_period_bounds; // 0x10
	real32 gain; // 0x18
	uint32 flags; // 0x1c
	int8 pad_20[48];
	real_bounds yaw_bounds; // 0x50
	real_bounds pitch_bounds; // 0x58
	real_bounds distance_bounds; // 0x60
};
ASSERT_STRUCT_SIZE(h1_lsnd_detail_sounds, 0x68);

struct h1_lsnd
{
	uint32 flags; // 0x0
	real32 zero_detail_sound_period; // 0x4
	real32 zero_runtime_unknown; // 0x8
	real32 zero_runtime_unknown_2; // 0xc
	real32 one_detail_sound_period; // 0x10
	real32 one_runtime_unknown; // 0x14
	real32 one_runtime_unknown_2; // 0x18
	int32 runtime_unknown; // 0x1c
	real32 maximum_distance; // 0x20
	int8 pad_24[8];
	h1_tag_reference continuous_damage_effect; // 0x2c
	h1_tag_block<h1_lsnd_tracks> tracks; // 0x3c
	h1_tag_block<h1_lsnd_detail_sounds> detail_sounds; // 0x48
};
ASSERT_STRUCT_SIZE(h1_lsnd, 0x54);

struct h1_ssce_attachments
{
	h1_tag_reference type; // 0x0
	char marker[32]; // 0x10
	int16 primary_scale; // 0x30
	int16 secondary_scale; // 0x32
	int16 change_color; // 0x34
	int16 unknown; // 0x36
	int8 pad_38[16];
};
ASSERT_STRUCT_SIZE(h1_ssce_attachments, 0x48);

struct h1_ssce_widgets
{
	h1_tag_reference reference; // 0x0
	int8 pad_10[16];
};
ASSERT_STRUCT_SIZE(h1_ssce_widgets, 0x20);

struct h1_ssce_functions
{
	uint32 flags; // 0x0
	real32 period; // 0x4
	int16 scale_period_by; // 0x8
	int16 function; // 0xa
	int16 scale_function_by; // 0xc
	int16 wobble_function; // 0xe
	real32 wobble_period; // 0x10
	real32 wobble_magnitude; // 0x14
	real32 square_wave_threshold; // 0x18
	int16 step_count; // 0x1c
	int16 map_to; // 0x1e
	int16 sawtooth_count; // 0x20
	int16 add; // 0x22
	int16 scale_result_by; // 0x24
	int16 bounds_mode; // 0x26
	real_bounds bounds; // 0x28
	int8 pad_30[4];
	int16 unknown; // 0x34
	int16 turn_off_with_index; // 0x36
	real32 scale_by; // 0x38
	int8 pad_3c[252];
	real32 inverse_bounds; // 0x138
	real32 inverse_sawtooth; // 0x13c
	real32 inverse_step; // 0x140
	real32 inverse_period; // 0x144
	char usage[32]; // 0x148
};
ASSERT_STRUCT_SIZE(h1_ssce_functions, 0x168);

struct h1_ssce_change_colors_permutations
{
	real32 weight; // 0x0
	real_rgb_color color_lower_bound; // 0x4
	real_rgb_color color_upper_bound; // 0x10
};
ASSERT_STRUCT_SIZE(h1_ssce_change_colors_permutations, 0x1c);

struct h1_ssce_change_colors
{
	int16 darken_by; // 0x0
	int16 scale_by; // 0x2
	uint32 scale_flags; // 0x4
	real_rgb_color color_lower_bound; // 0x8
	real_rgb_color color_upper_bound; // 0x14
	h1_tag_block<h1_ssce_change_colors_permutations> permutations; // 0x20
};
ASSERT_STRUCT_SIZE(h1_ssce_change_colors, 0x2c);

struct h1_ssce_predicted_resources
{
	int16 type; // 0x0
	int16 resource_index; // 0x2
	datum tag_index; // 0x4
};
ASSERT_STRUCT_SIZE(h1_ssce_predicted_resources, 0x8);

struct h1_ssce
{
	int16 object_type; // 0x0
	uint16 flags; // 0x2
	real32 bounding_radius; // 0x4
	real_point3d bounding_offset; // 0x8
	real_point3d origin_offset; // 0x14
	real32 acceleration_scale; // 0x20
	uint32 runtime_flags; // 0x24
	h1_tag_reference model; // 0x28
	h1_tag_reference animation_graph; // 0x38
	int8 pad_48[40];
	h1_tag_reference collision_model; // 0x70
	h1_tag_reference physics; // 0x80
	h1_tag_reference modifier_shader; // 0x90
	h1_tag_reference creation_effect; // 0xa0
	int8 pad_b0[84];
	real32 render_bounding_radius; // 0x104
	int16 a_in; // 0x108
	int16 b_in; // 0x10a
	int16 c_in; // 0x10c
	int16 d_in; // 0x10e
	int8 pad_110[44];
	int16 hud_text_message_index; // 0x13c
	int16 forced_shader_permutation_index; // 0x13e
	h1_tag_block<h1_ssce_attachments> attachments; // 0x140
	h1_tag_block<h1_ssce_widgets> widgets; // 0x14c
	h1_tag_block<h1_ssce_functions> functions; // 0x158
	h1_tag_block<h1_ssce_change_colors> change_colors; // 0x164
	h1_tag_block<h1_ssce_predicted_resources> predicted_resources; // 0x170
	int8 pad_17c[128];
};
ASSERT_STRUCT_SIZE(h1_ssce, 0x1fc);

#pragma pack(pop)

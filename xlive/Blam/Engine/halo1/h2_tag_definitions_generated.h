#pragma once
#pragma pack(push, 1)

struct h2x_scnr_skies
{
	tag_reference sky; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scnr_skies, 0x8);

struct h2x_scnr_child_scenarios
{
	tag_reference child_scenario; // 0x0
	int8 pad_8[16];
};
ASSERT_STRUCT_SIZE(h2x_scnr_child_scenarios, 0x18);

struct h2x_scnr_predicted_resources
{
	int16 type; // 0x0
	int16 resource_index; // 0x2
	datum tag_index; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_scnr_predicted_resources, 0x8);

struct h2x_scnr_comments
{
	real_point3d position; // 0x0
	int32 type; // 0xc
	char name[32]; // 0x10
	char comment[256]; // 0x30
};
ASSERT_STRUCT_SIZE(h2x_scnr_comments, 0x130);

struct h2x_scnr_environment_objects
{
	int16 bsp_index; // 0x0
	int16 unknown; // 0x2
	datum unique_id; // 0x4
	int8 pad_8[4];
	char object_definition_tag[4]; // 0xc
	int32 object; // 0x10
	int8 pad_14[44];
};
ASSERT_STRUCT_SIZE(h2x_scnr_environment_objects, 0x40);

struct h2x_scnr_object_names
{
	char name[32]; // 0x0
	int16 type; // 0x20
	int16 placement_index; // 0x22
};
ASSERT_STRUCT_SIZE(h2x_scnr_object_names, 0x24);

struct h2x_scnr_scenery_pathfinding_references
{
	int16 bsp_index; // 0x0
	int16 pathfinding_object_index; // 0x2
};
ASSERT_STRUCT_SIZE(h2x_scnr_scenery_pathfinding_references, 0x4);

struct h2x_scnr_scenery
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint32 placement_flags; // 0x4
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	real32 scale; // 0x20
	uint16 transform_flags; // 0x24
	uint16 manual_bsp_flags; // 0x26
	datum unique_id; // 0x28
	int16 origin_bsp_index; // 0x2c
	int8 type; // 0x2e
	int8 source; // 0x2f
	int8 bsp_policy; // 0x30
	int8 unknown; // 0x31
	int16 editor_folder_index; // 0x32
	string_id variant_name; // 0x34
	uint32 active_change_colors; // 0x38
	uint32 primary_color; // 0x3c
	uint32 secondary_color; // 0x40
	uint32 tertiary_color; // 0x44
	uint32 quaternary_color; // 0x48
	int16 pathfinding_policy; // 0x4c
	int16 lightmapping_policy; // 0x4e
	tag_block<h2x_scnr_scenery_pathfinding_references> pathfinding_references; // 0x50
	int16 unknown_2; // 0x58
	uint16 valid_multiplayer_games; // 0x5a
};
ASSERT_STRUCT_SIZE(h2x_scnr_scenery, 0x5c);

struct h2x_scnr_scenery_palette
{
	tag_reference scenery; // 0x0
	int8 pad_8[32];
};
ASSERT_STRUCT_SIZE(h2x_scnr_scenery_palette, 0x28);

struct h2x_scnr_bipeds
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint32 placement_flags; // 0x4
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	real32 scale; // 0x20
	uint16 transform_flags; // 0x24
	uint16 manual_bsp_flags; // 0x26
	datum unique_id; // 0x28
	int16 origin_bsp_index; // 0x2c
	int8 type; // 0x2e
	int8 source; // 0x2f
	int8 bsp_policy; // 0x30
	int8 unknown; // 0x31
	int16 editor_folder_index; // 0x32
	string_id variant_name; // 0x34
	uint32 active_change_colors; // 0x38
	uint32 primary_color; // 0x3c
	uint32 secondary_color; // 0x40
	uint32 tertiary_color; // 0x44
	uint32 quaternary_color; // 0x48
	real32 body_vitality_percentage; // 0x4c
	uint32 unit_flags; // 0x50
};
ASSERT_STRUCT_SIZE(h2x_scnr_bipeds, 0x54);

struct h2x_scnr_biped_palette
{
	tag_reference biped; // 0x0
	int8 pad_8[32];
};
ASSERT_STRUCT_SIZE(h2x_scnr_biped_palette, 0x28);

struct h2x_scnr_vehicles
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint32 placement_flags; // 0x4
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	real32 scale; // 0x20
	uint16 transform_flags; // 0x24
	uint16 manual_bsp_flags; // 0x26
	datum unique_id; // 0x28
	int16 origin_bsp_index; // 0x2c
	int8 type; // 0x2e
	int8 source; // 0x2f
	int8 bsp_policy; // 0x30
	int8 unknown; // 0x31
	int16 editor_folder_index; // 0x32
	string_id variant_name; // 0x34
	uint32 active_change_colors; // 0x38
	uint32 primary_color; // 0x3c
	uint32 secondary_color; // 0x40
	uint32 tertiary_color; // 0x44
	uint32 quaternary_color; // 0x48
	real32 body_vitality_percentage; // 0x4c
	uint32 unit_flags; // 0x50
};
ASSERT_STRUCT_SIZE(h2x_scnr_vehicles, 0x54);

struct h2x_scnr_vehicle_palette
{
	tag_reference vehicle; // 0x0
	int8 pad_8[32];
};
ASSERT_STRUCT_SIZE(h2x_scnr_vehicle_palette, 0x28);

struct h2x_scnr_equipment
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint32 placement_flags; // 0x4
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	real32 scale; // 0x20
	uint16 transform_flags; // 0x24
	uint16 manual_bsp_flags; // 0x26
	datum unique_id; // 0x28
	int16 origin_bsp_index; // 0x2c
	int8 type; // 0x2e
	int8 source; // 0x2f
	int8 bsp_policy; // 0x30
	int8 unknown; // 0x31
	int16 editor_folder_index; // 0x32
	uint32 equipment_flags; // 0x34
};
ASSERT_STRUCT_SIZE(h2x_scnr_equipment, 0x38);

struct h2x_scnr_equipment_palette
{
	tag_reference equipment; // 0x0
	int8 pad_8[32];
};
ASSERT_STRUCT_SIZE(h2x_scnr_equipment_palette, 0x28);

struct h2x_scnr_weapons
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint32 placement_flags; // 0x4
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	real32 scale; // 0x20
	uint16 transform_flags; // 0x24
	uint16 manual_bsp_flags; // 0x26
	datum unique_id; // 0x28
	int16 origin_bsp_index; // 0x2c
	int8 type; // 0x2e
	int8 source; // 0x2f
	int8 bsp_policy; // 0x30
	int8 unknown; // 0x31
	int16 editor_folder_index; // 0x32
	string_id variant_name; // 0x34
	uint32 active_change_colors; // 0x38
	uint32 primary_color; // 0x3c
	uint32 secondary_color; // 0x40
	uint32 tertiary_color; // 0x44
	uint32 quaternary_color; // 0x48
	int16 rounds_left; // 0x4c
	int16 rounds_loaded; // 0x4e
	uint32 weapon_flags; // 0x50
};
ASSERT_STRUCT_SIZE(h2x_scnr_weapons, 0x54);

struct h2x_scnr_weapon_palette
{
	tag_reference weapon; // 0x0
	int8 pad_8[32];
};
ASSERT_STRUCT_SIZE(h2x_scnr_weapon_palette, 0x28);

struct h2x_scnr_device_groups
{
	char name[32]; // 0x0
	real32 initial_value; // 0x20
	uint32 flags; // 0x24
};
ASSERT_STRUCT_SIZE(h2x_scnr_device_groups, 0x28);

struct h2x_scnr_machines_pathfinding_references
{
	int16 bsp_index; // 0x0
	int16 pathfinding_object_index; // 0x2
};
ASSERT_STRUCT_SIZE(h2x_scnr_machines_pathfinding_references, 0x4);

struct h2x_scnr_machines
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint32 placement_flags; // 0x4
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	real32 scale; // 0x20
	uint16 transform_flags; // 0x24
	uint16 manual_bsp_flags; // 0x26
	datum unique_id; // 0x28
	int16 origin_bsp_index; // 0x2c
	int8 type; // 0x2e
	int8 source; // 0x2f
	int8 bsp_policy; // 0x30
	int8 unknown; // 0x31
	int16 editor_folder_index; // 0x32
	int16 power_group_index; // 0x34
	int16 position_group_index; // 0x36
	uint32 device_flags; // 0x38
	uint32 machine_flags; // 0x3c
	tag_block<h2x_scnr_machines_pathfinding_references> pathfinding_references; // 0x40
};
ASSERT_STRUCT_SIZE(h2x_scnr_machines, 0x48);

struct h2x_scnr_machine_palette
{
	tag_reference machine; // 0x0
	int8 pad_8[32];
};
ASSERT_STRUCT_SIZE(h2x_scnr_machine_palette, 0x28);

struct h2x_scnr_controls
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint32 placement_flags; // 0x4
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	real32 scale; // 0x20
	uint16 transform_flags; // 0x24
	uint16 manual_bsp_flags; // 0x26
	datum unique_id; // 0x28
	int16 origin_bsp_index; // 0x2c
	int8 type; // 0x2e
	int8 source; // 0x2f
	int8 bsp_policy; // 0x30
	int8 unknown; // 0x31
	int16 editor_folder_index; // 0x32
	int16 power_group_index; // 0x34
	int16 position_group_index; // 0x36
	uint32 device_flags; // 0x38
	uint32 control_flags; // 0x3c
	int16 custom_object_name_index; // 0x40
	int16 unknown_2; // 0x42
};
ASSERT_STRUCT_SIZE(h2x_scnr_controls, 0x44);

struct h2x_scnr_control_palette
{
	tag_reference control; // 0x0
	int8 pad_8[32];
};
ASSERT_STRUCT_SIZE(h2x_scnr_control_palette, 0x28);

struct h2x_scnr_light_fixtures
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint32 placement_flags; // 0x4
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	real32 scale; // 0x20
	uint16 transform_flags; // 0x24
	uint16 manual_bsp_flags; // 0x26
	datum unique_id; // 0x28
	int16 origin_bsp_index; // 0x2c
	int8 type; // 0x2e
	int8 source; // 0x2f
	int8 bsp_policy; // 0x30
	int8 unknown; // 0x31
	int16 editor_folder_index; // 0x32
	int16 power_group_index; // 0x34
	int16 position_group_index; // 0x36
	uint32 device_flags; // 0x38
	real_rgb_color color; // 0x3c
	real32 intensity; // 0x48
	real32 falloff_angle; // 0x4c
	real32 cutoff_angle; // 0x50
};
ASSERT_STRUCT_SIZE(h2x_scnr_light_fixtures, 0x54);

struct h2x_scnr_light_fixtures_palette
{
	tag_reference light_fixture; // 0x0
	int8 pad_8[32];
};
ASSERT_STRUCT_SIZE(h2x_scnr_light_fixtures_palette, 0x28);

struct h2x_scnr_sound_scenery
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint32 placement_flags; // 0x4
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	real32 scale; // 0x20
	uint16 transform_flags; // 0x24
	uint16 manual_bsp_flags; // 0x26
	datum unique_id; // 0x28
	int16 origin_bsp_index; // 0x2c
	int8 type; // 0x2e
	int8 source; // 0x2f
	int8 bsp_policy; // 0x30
	int8 unknown; // 0x31
	int16 editor_folder_index; // 0x32
	int32 volume_type; // 0x34
	real32 height; // 0x38
	real_bounds override_distance_bounds; // 0x3c
	real_bounds override_cone_angle_bounds; // 0x44
	real32 override_outer_cone_gain; // 0x4c
};
ASSERT_STRUCT_SIZE(h2x_scnr_sound_scenery, 0x50);

struct h2x_scnr_sound_scenery_palette
{
	tag_reference sound_scenery; // 0x0
	int8 pad_8[32];
};
ASSERT_STRUCT_SIZE(h2x_scnr_sound_scenery_palette, 0x28);

struct h2x_scnr_light_volumes
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint32 placement_flags; // 0x4
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	real32 scale; // 0x20
	uint16 transform_flags; // 0x24
	uint16 manual_bsp_flags; // 0x26
	datum unique_id; // 0x28
	int16 origin_bsp_index; // 0x2c
	int8 type; // 0x2e
	int8 source; // 0x2f
	int8 bsp_policy; // 0x30
	int8 unknown; // 0x31
	int16 editor_folder_index; // 0x32
	int16 power_group_index; // 0x34
	int16 position_group_index; // 0x36
	uint32 device_flags; // 0x38
	int16 type_2; // 0x3c
	uint16 flags; // 0x3e
	int16 lightmap_type; // 0x40
	uint16 lightmap_flags; // 0x42
	real32 lightmap_half_life; // 0x44
	real32 lightmap_light_scale; // 0x48
	real_point3d target_point; // 0x4c
	real32 width; // 0x58
	real32 height_scale; // 0x5c
	real32 field_of_view; // 0x60
	real32 falloff_distance; // 0x64
	real32 cutoff_distance; // 0x68
};
ASSERT_STRUCT_SIZE(h2x_scnr_light_volumes, 0x6c);

struct h2x_scnr_light_volumes_palette
{
	tag_reference light_volume; // 0x0
	int8 pad_8[32];
};
ASSERT_STRUCT_SIZE(h2x_scnr_light_volumes_palette, 0x28);

struct h2x_scnr_player_starting_profile
{
	char name[32]; // 0x0
	real32 starting_health_damage; // 0x20
	real32 starting_shield_damage; // 0x24
	tag_reference primary_weapon; // 0x28
	int16 primary_rounds_loaded; // 0x30
	int16 primary_rounds_total; // 0x32
	tag_reference secondary_weapon; // 0x34
	int16 secondary_rounds_loaded; // 0x3c
	int16 secondary_rounds_total; // 0x3e
	uint8 starting_frag_grenade_count; // 0x40
	uint8 starting_plasma_grenade_count; // 0x41
	uint8 starting_grenade_3_count; // 0x42
	uint8 starting_grenade_4_count; // 0x43
};
ASSERT_STRUCT_SIZE(h2x_scnr_player_starting_profile, 0x44);

struct h2x_scnr_player_starting_locations
{
	real_point3d position; // 0x0
	real32 facing; // 0xc
	int16 team_designator; // 0x10
	int16 bsp_index; // 0x12
	int16 game_type_1; // 0x14
	int16 game_type_2; // 0x16
	int16 game_type_3; // 0x18
	int16 game_type_4; // 0x1a
	int16 spawn_type_0; // 0x1c
	int16 spawn_type_1; // 0x1e
	int16 spawn_type_2; // 0x20
	int16 spawn_type_3; // 0x22
	string_id unused_names_0; // 0x24
	string_id unused_names_1; // 0x28
	int16 campaign_player_type; // 0x2c
	int16 unknown; // 0x2e
	int8 pad_30[4];
};
ASSERT_STRUCT_SIZE(h2x_scnr_player_starting_locations, 0x34);

struct h2x_scnr_kill_trigger_volumes
{
	string_id name; // 0x0
	int16 object_name_index; // 0x4
	int16 node_index; // 0x6
	string_id node_name; // 0x8
	real_vector3d forward; // 0xc
	real_vector3d up; // 0x18
	real_point3d position; // 0x24
	real_point3d extents; // 0x30
	real32 unknown; // 0x3c
	int16 kill_trigger_volume_index; // 0x40
	int16 unknown_2; // 0x42
};
ASSERT_STRUCT_SIZE(h2x_scnr_kill_trigger_volumes, 0x44);

struct h2x_scnr_recorded_animations
{
	char name[32]; // 0x0
	int8 version; // 0x20
	int8 raw_animation_data; // 0x21
	int8 unit_control_data_version; // 0x22
	int8 unknown; // 0x23
	int16 length_of_animation; // 0x24
	int16 unknown_2; // 0x26
	int8 pad_28[4];
	tag_data recorded_animation_event_stream; // 0x2c
};
ASSERT_STRUCT_SIZE(h2x_scnr_recorded_animations, 0x34);

struct h2x_scnr_netgame_flags
{
	real_point3d position; // 0x0
	real32 facing; // 0xc
	int16 type; // 0x10
	int16 team_designator; // 0x12
	int16 identifier; // 0x14
	uint16 flags; // 0x16
	string_id spawn_object_name; // 0x18
	string_id spawn_marker_name; // 0x1c
};
ASSERT_STRUCT_SIZE(h2x_scnr_netgame_flags, 0x20);

struct h2x_scnr_netgame_equipment
{
	uint32 flags; // 0x0
	int16 game_type_1; // 0x4
	int16 game_type_2; // 0x6
	int16 game_type_3; // 0x8
	int16 game_type_4; // 0xa
	int16 team_index; // 0xc
	int16 spawn_time; // 0xe
	int16 respawn_on_empty_time; // 0x10
	int16 respawn_timer_starts; // 0x12
	int8 classification; // 0x14
	int8 unknown; // 0x15
	int8 unknown_2; // 0x16
	int8 unknown_3; // 0x17
	int8 pad_18[40];
	real_point3d position; // 0x40
	real_euler_angles3d orientation; // 0x4c
	tag_reference item_vehicle_collection; // 0x58
	int8 pad_60[48];
};
ASSERT_STRUCT_SIZE(h2x_scnr_netgame_equipment, 0x90);

struct h2x_scnr_starting_equipment
{
	uint32 flags; // 0x0
	int16 game_type_1; // 0x4
	int16 game_type_2; // 0x6
	int16 game_type_3; // 0x8
	int16 game_type_4; // 0xa
	int8 pad_c[48];
	tag_reference item_collection_1; // 0x3c
	tag_reference item_collection_2; // 0x44
	tag_reference item_collection_3; // 0x4c
	tag_reference item_collection_4; // 0x54
	tag_reference item_collection_5; // 0x5c
	tag_reference item_collection_6; // 0x64
	int8 pad_6c[48];
};
ASSERT_STRUCT_SIZE(h2x_scnr_starting_equipment, 0x9c);

struct h2x_scnr_bsp_switch_trigger_volumes
{
	int16 trigger_volume_index; // 0x0
	int16 source_bsp_index; // 0x2
	int16 destination_bsp_index; // 0x4
	int16 unknown; // 0x6
	int16 unknown_2; // 0x8
	int16 unknown_3; // 0xa
	int16 unknown_4; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_scnr_bsp_switch_trigger_volumes, 0xe);

struct h2x_scnr_decals
{
	int16 palette_index; // 0x0
	int8 yaw; // 0x2
	int8 pitch; // 0x3
	real_point3d position; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_scnr_decals, 0x10);

struct h2x_scnr_decals_palette
{
	tag_reference decal; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scnr_decals_palette, 0x8);

struct h2x_scnr_detail_object_collection_palette
{
	tag_reference detail_object_collection; // 0x0
	int8 pad_8[32];
};
ASSERT_STRUCT_SIZE(h2x_scnr_detail_object_collection_palette, 0x28);

struct h2x_scnr_style_palette
{
	tag_reference style; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scnr_style_palette, 0x8);

struct h2x_scnr_squad_groups
{
	char name[32]; // 0x0
	int16 parent_index; // 0x20
	int16 initial_order_index; // 0x22
};
ASSERT_STRUCT_SIZE(h2x_scnr_squad_groups, 0x24);

struct h2x_scnr_squads_starting_locations
{
	string_id name; // 0x0
	real_point3d position; // 0x4
	int16 reference_frame; // 0x10
	int16 unknown; // 0x12
	real_euler_angles2d facing; // 0x14
	uint32 flags; // 0x1c
	int16 character_type_index; // 0x20
	int16 initial_weapon_index; // 0x22
	int16 initial_secondary_weapon_index; // 0x24
	int16 unknown_2; // 0x26
	int16 vehicle_type_index; // 0x28
	int16 seat_type; // 0x2a
	int16 grenade_type; // 0x2c
	int16 swarm_count; // 0x2e
	string_id actor_variant_name; // 0x30
	string_id vehicle_variant_name; // 0x34
	real32 initial_movement_distance; // 0x38
	int16 emitter_vehicle_index; // 0x3c
	int16 initial_movement_mode; // 0x3e
	char placement_script[32]; // 0x40
	int16 placement_script_index; // 0x60
	int16 unknown_3; // 0x62
};
ASSERT_STRUCT_SIZE(h2x_scnr_squads_starting_locations, 0x64);

struct h2x_scnr_squads
{
	char name[32]; // 0x0
	uint32 flags; // 0x20
	int16 team; // 0x24
	int16 parent_squad_group_index; // 0x26
	real32 squad_delay_time; // 0x28
	int16 normal_difficulty_count; // 0x2c
	int16 insane_difficulty_count; // 0x2e
	int16 major_upgrade; // 0x30
	int16 unknown; // 0x32
	int16 vehicle_type_index; // 0x34
	int16 character_type_index; // 0x36
	int16 initial_zone_index; // 0x38
	int16 unknown_2; // 0x3a
	int16 initial_weapon_index; // 0x3c
	int16 initial_secondary_weapon_index; // 0x3e
	int16 grenade_type; // 0x40
	int16 initial_order_index; // 0x42
	string_id vehicle_variant; // 0x44
	tag_block<h2x_scnr_squads_starting_locations> starting_locations; // 0x48
	char placement_script[32]; // 0x50
	int16 placement_script_index; // 0x70
	int16 unknown_3; // 0x72
};
ASSERT_STRUCT_SIZE(h2x_scnr_squads, 0x74);

struct h2x_scnr_zones_firing_positions
{
	real_point3d position; // 0x0
	int16 reference_frame; // 0xc
	uint16 flags; // 0xe
	int16 area_index; // 0x10
	int16 cluster_index; // 0x12
	int16 unknown; // 0x14
	int16 unknown_2; // 0x16
	real_euler_angles2d normal; // 0x18
};
ASSERT_STRUCT_SIZE(h2x_scnr_zones_firing_positions, 0x20);

struct h2x_scnr_zones_areas_flight_hints
{
	int16 flight_hint_index; // 0x0
	int16 point_index; // 0x2
};
ASSERT_STRUCT_SIZE(h2x_scnr_zones_areas_flight_hints, 0x4);

struct h2x_scnr_zones_areas
{
	char name[32]; // 0x0
	uint32 area_flags; // 0x20
	real_point3d position; // 0x24
	int16 unknown; // 0x30
	int16 unknown_2; // 0x32
	real32 unknown_3; // 0x34
	int16 firing_position_start_index; // 0x38
	int16 firing_position_count; // 0x3a
	int16 unknown_4; // 0x3c
	int16 unknown_5; // 0x3e
	int8 pad_40[4];
	int8 unknown_6; // 0x44
	int8 unknown_7; // 0x45
	int8 unknown_8; // 0x46
	int8 unknown_9; // 0x47
	int16 unknown_10; // 0x48
	int16 unknown_11; // 0x4a
	int8 pad_4c[4];
	real32 unknown_12; // 0x50
	real32 unknown_13; // 0x54
	real32 unknown_14; // 0x58
	real32 unknown_15; // 0x5c
	real32 unknown_16; // 0x60
	real32 unknown_17; // 0x64
	real32 unknown_18; // 0x68
	real32 unknown_19; // 0x6c
	real32 unknown_20; // 0x70
	real32 unknown_21; // 0x74
	real32 unknown_22; // 0x78
	int16 manual_reference_frame; // 0x7c
	int16 unknown_23; // 0x7e
	tag_block<h2x_scnr_zones_areas_flight_hints> flight_hints; // 0x80
};
ASSERT_STRUCT_SIZE(h2x_scnr_zones_areas, 0x88);

struct h2x_scnr_zones
{
	char name[32]; // 0x0
	uint32 flags; // 0x20
	int16 manual_bsp_index; // 0x24
	int16 unknown; // 0x26
	tag_block<h2x_scnr_zones_firing_positions> firing_positions; // 0x28
	tag_block<h2x_scnr_zones_areas> areas; // 0x30
};
ASSERT_STRUCT_SIZE(h2x_scnr_zones, 0x38);

struct h2x_scnr_mission_scenes_trigger_conditions_triggers
{
	uint32 trigger_flags; // 0x0
	int16 trigger_index; // 0x4
	int16 unknown; // 0x6
};
ASSERT_STRUCT_SIZE(h2x_scnr_mission_scenes_trigger_conditions_triggers, 0x8);

struct h2x_scnr_mission_scenes_trigger_conditions
{
	int16 combination_rule; // 0x0
	int16 unknown; // 0x2
	tag_block<h2x_scnr_mission_scenes_trigger_conditions_triggers> triggers; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_scnr_mission_scenes_trigger_conditions, 0xc);

struct h2x_scnr_mission_scenes_roles_role_variants
{
	string_id variant_designation; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scnr_mission_scenes_roles_role_variants, 0x4);

struct h2x_scnr_mission_scenes_roles
{
	string_id name; // 0x0
	int16 group; // 0x4
	int16 unknown; // 0x6
	tag_block<h2x_scnr_mission_scenes_roles_role_variants> role_variants; // 0x8
};
ASSERT_STRUCT_SIZE(h2x_scnr_mission_scenes_roles, 0x10);

struct h2x_scnr_mission_scenes
{
	string_id name; // 0x0
	uint32 flags; // 0x4
	tag_block<h2x_scnr_mission_scenes_trigger_conditions> trigger_conditions; // 0x8
	tag_block<h2x_scnr_mission_scenes_roles> roles; // 0x10
};
ASSERT_STRUCT_SIZE(h2x_scnr_mission_scenes, 0x18);

struct h2x_scnr_character_palette
{
	tag_reference character; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scnr_character_palette, 0x8);

struct h2x_scnr_ai_animation_references
{
	char animation_name[32]; // 0x0
	tag_reference animation_graph; // 0x20
	int8 pad_28[12];
};
ASSERT_STRUCT_SIZE(h2x_scnr_ai_animation_references, 0x34);

struct h2x_scnr_ai_script_references
{
	char script_name[32]; // 0x0
	int8 pad_20[8];
};
ASSERT_STRUCT_SIZE(h2x_scnr_ai_script_references, 0x28);

struct h2x_scnr_ai_recording_references
{
	char recording_name[32]; // 0x0
	int8 pad_20[8];
};
ASSERT_STRUCT_SIZE(h2x_scnr_ai_recording_references, 0x28);

struct h2x_scnr_ai_conversations_participants
{
	int8 pad_0[8];
	int16 use_this_object_index; // 0x8
	int16 set_new_name_index; // 0xa
	int8 pad_c[24];
	char encounter_name[32]; // 0x24
	int8 pad_44[16];
};
ASSERT_STRUCT_SIZE(h2x_scnr_ai_conversations_participants, 0x54);

struct h2x_scnr_ai_conversations_lines
{
	uint16 flags; // 0x0
	int16 participant_index; // 0x2
	int16 addressee; // 0x4
	int16 addressee_participant_index; // 0x6
	int8 pad_8[4];
	real32 line_delay_time; // 0xc
	int8 pad_10[12];
	tag_reference variant_1; // 0x1c
	tag_reference variant_2; // 0x24
	tag_reference variant_3; // 0x2c
	tag_reference variant_4; // 0x34
	tag_reference variant_5; // 0x3c
	tag_reference variant_6; // 0x44
};
ASSERT_STRUCT_SIZE(h2x_scnr_ai_conversations_lines, 0x4c);

struct h2x_scnr_ai_conversations_unknown
{
	int8 unused;
};

struct h2x_scnr_ai_conversations
{
	char name[32]; // 0x0
	uint16 flags; // 0x20
	int16 unknown; // 0x22
	real32 trigger_distance; // 0x24
	real32 run_to_player_distance; // 0x28
	int8 pad_2c[36];
	tag_block<h2x_scnr_ai_conversations_participants> participants; // 0x50
	tag_block<h2x_scnr_ai_conversations_lines> lines; // 0x58
	tag_block<h2x_scnr_ai_conversations_unknown> unknown_2; // 0x60
};
ASSERT_STRUCT_SIZE(h2x_scnr_ai_conversations, 0x68);

struct h2x_scnr_scripts
{
	char name[32]; // 0x0
	int16 script_type; // 0x20
	int16 return_type; // 0x22
	datum root_expression; // 0x24
};
ASSERT_STRUCT_SIZE(h2x_scnr_scripts, 0x28);

struct h2x_scnr_globals
{
	char name[32]; // 0x0
	int16 type; // 0x20
	int16 unknown; // 0x22
	datum initialization_expression; // 0x24
};
ASSERT_STRUCT_SIZE(h2x_scnr_globals, 0x28);

struct h2x_scnr_script_references
{
	tag_reference reference; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scnr_script_references, 0x8);

struct h2x_scnr_scripting_data_point_sets_points
{
	char name[32]; // 0x0
	real_point3d position; // 0x20
	int16 reference_frame; // 0x2c
	int16 unknown; // 0x2e
	int32 surface_index; // 0x30
	real_euler_angles2d facing_direction; // 0x34
};
ASSERT_STRUCT_SIZE(h2x_scnr_scripting_data_point_sets_points, 0x3c);

struct h2x_scnr_scripting_data_point_sets
{
	char name[32]; // 0x0
	tag_block<h2x_scnr_scripting_data_point_sets_points> points; // 0x20
	int16 bsp_index; // 0x28
	int16 manual_reference_frame; // 0x2a
	uint32 flags; // 0x2c
};
ASSERT_STRUCT_SIZE(h2x_scnr_scripting_data_point_sets, 0x30);

struct h2x_scnr_scripting_data
{
	tag_block<h2x_scnr_scripting_data_point_sets> point_sets; // 0x0
	int8 pad_8[120];
};
ASSERT_STRUCT_SIZE(h2x_scnr_scripting_data, 0x80);

struct h2x_scnr_cutscene_flags
{
	int8 pad_0[4];
	char name[32]; // 0x4
	real_point3d position; // 0x24
	real_euler_angles2d facing; // 0x30
};
ASSERT_STRUCT_SIZE(h2x_scnr_cutscene_flags, 0x38);

struct h2x_scnr_cutscene_camera_points
{
	uint16 flags; // 0x0
	int16 type; // 0x2
	char name[32]; // 0x4
	int8 pad_24[4];
	real_point3d position; // 0x28
	real_euler_angles3d orientation; // 0x34
};
ASSERT_STRUCT_SIZE(h2x_scnr_cutscene_camera_points, 0x40);

struct h2x_scnr_cutscene_titles
{
	string_id name; // 0x0
	rectangle2d text_bounds; // 0x4
	int16 justification; // 0xc
	int16 font; // 0xe
	uint32 text_color; // 0x10
	uint32 shadow_color; // 0x14
	real32 fade_in_time; // 0x18
	real32 up_time; // 0x1c
	real32 fade_out_time; // 0x20
};
ASSERT_STRUCT_SIZE(h2x_scnr_cutscene_titles, 0x24);

struct h2x_scnr_structure_bsps
{
	uint32 structure_bsp_offset; // 0x0
	uint32 structure_bsp_size; // 0x4
	uint32 structure_bsp_address; // 0x8
	int8 pad_c[4];
	tag_reference structure_bsp; // 0x10
	tag_reference structure_lightmap; // 0x18
	int8 pad_20[4];
	real32 radiance_estimated_search_distance; // 0x24
	int8 pad_28[4];
	real32 luminels_per_world_unit; // 0x2c
	real32 output_white_reference; // 0x30
	int8 pad_34[8];
	uint16 flags; // 0x3c
	int16 unknown; // 0x3e
	int16 default_sky_index; // 0x40
	int16 unknown_2; // 0x42
};
ASSERT_STRUCT_SIZE(h2x_scnr_structure_bsps, 0x44);

struct h2x_scnr_scenario_resources_references
{
	tag_reference reference; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scnr_scenario_resources_references, 0x8);

struct h2x_scnr_scenario_resources_script_source
{
	tag_reference reference; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scnr_scenario_resources_script_source, 0x8);

struct h2x_scnr_scenario_resources_ai_resources
{
	tag_reference reference; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scnr_scenario_resources_ai_resources, 0x8);

struct h2x_scnr_scenario_resources
{
	tag_block<h2x_scnr_scenario_resources_references> references; // 0x0
	tag_block<h2x_scnr_scenario_resources_script_source> script_source; // 0x8
	tag_block<h2x_scnr_scenario_resources_ai_resources> ai_resources; // 0x10
};
ASSERT_STRUCT_SIZE(h2x_scnr_scenario_resources, 0x18);

struct h2x_scnr_old_structure_physics_environment_object_identifiers
{
	datum unique_id; // 0x0
	int16 origin_bsp_index; // 0x4
	int8 type; // 0x6
	int8 source; // 0x7
};
ASSERT_STRUCT_SIZE(h2x_scnr_old_structure_physics_environment_object_identifiers, 0x8);

struct h2x_scnr_old_structure_physics
{
	tag_data mopp_code; // 0x0
	tag_block<h2x_scnr_old_structure_physics_environment_object_identifiers> environment_object_identifiers; // 0x8
	int8 pad_10[4];
	real_point3d mopp_bounds_minimum; // 0x14
	real_point3d mopp_bounds_maximum; // 0x20
};
ASSERT_STRUCT_SIZE(h2x_scnr_old_structure_physics, 0x2c);

struct h2x_scnr_unit_seats_mapping
{
	datum unit; // 0x0
	uint32 seats; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_scnr_unit_seats_mapping, 0x8);

struct h2x_scnr_scenario_kill_triggers
{
	int16 trigger_volume_index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scnr_scenario_kill_triggers, 0x2);

struct h2x_scnr_script_expressions
{
	uint16 salt; // 0x0
	int16 opcode; // 0x2
	int16 value_type; // 0x4
	uint16 flags; // 0x6
	datum next_expression; // 0x8
	uint32 string_address; // 0xc
	int8 value_00_lsb; // 0x10
	int8 value_01_byte; // 0x11
	int8 value_02_byte; // 0x12
	int8 value_03_msb; // 0x13
};
ASSERT_STRUCT_SIZE(h2x_scnr_script_expressions, 0x14);

struct h2x_scnr_orders_primary_area_set
{
	int16 area_type; // 0x0
	int16 unknown; // 0x2
	int16 zone_index; // 0x4
	int16 area_index; // 0x6
};
ASSERT_STRUCT_SIZE(h2x_scnr_orders_primary_area_set, 0x8);

struct h2x_scnr_orders_secondary_area_set
{
	int16 area_type; // 0x0
	int16 unknown; // 0x2
	int16 zone_index; // 0x4
	int16 area_index; // 0x6
};
ASSERT_STRUCT_SIZE(h2x_scnr_orders_secondary_area_set, 0x8);

struct h2x_scnr_orders_secondary_set_trigger_triggers
{
	uint32 trigger_flags; // 0x0
	int16 trigger_index; // 0x4
	int16 unknown; // 0x6
};
ASSERT_STRUCT_SIZE(h2x_scnr_orders_secondary_set_trigger_triggers, 0x8);

struct h2x_scnr_orders_secondary_set_trigger
{
	int16 combination_rule; // 0x0
	int16 dialogue_type; // 0x2
	tag_block<h2x_scnr_orders_secondary_set_trigger_triggers> triggers; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_scnr_orders_secondary_set_trigger, 0xc);

struct h2x_scnr_orders_special_movement
{
	uint32 special_movement_1; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scnr_orders_special_movement, 0x4);

struct h2x_scnr_orders_order_endings_triggers
{
	uint32 trigger_flags; // 0x0
	int16 trigger_index; // 0x4
	int16 unknown; // 0x6
};
ASSERT_STRUCT_SIZE(h2x_scnr_orders_order_endings_triggers, 0x8);

struct h2x_scnr_orders_order_endings
{
	int16 next_order_index; // 0x0
	int16 combination_rule; // 0x2
	real32 delay_time; // 0x4
	int16 dialogue_type; // 0x8
	int16 unknown; // 0xa
	tag_block<h2x_scnr_orders_order_endings_triggers> triggers; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_scnr_orders_order_endings, 0x14);

struct h2x_scnr_orders
{
	char name[32]; // 0x0
	int16 style_index; // 0x20
	int16 unknown; // 0x22
	uint32 flags; // 0x24
	int16 force_combat_status; // 0x28
	int16 unknown_2; // 0x2a
	char entry_script[32]; // 0x2c
	int16 script_index; // 0x4c
	int16 follow_squad_index; // 0x4e
	real32 follow_radius; // 0x50
	tag_block<h2x_scnr_orders_primary_area_set> primary_area_set; // 0x54
	tag_block<h2x_scnr_orders_secondary_area_set> secondary_area_set; // 0x5c
	tag_block<h2x_scnr_orders_secondary_set_trigger> secondary_set_trigger; // 0x64
	tag_block<h2x_scnr_orders_special_movement> special_movement; // 0x6c
	tag_block<h2x_scnr_orders_order_endings> order_endings; // 0x74
};
ASSERT_STRUCT_SIZE(h2x_scnr_orders, 0x7c);

struct h2x_scnr_ai_triggers_conditions
{
	int16 rule_type; // 0x0
	int16 squad_index; // 0x2
	int16 squad_group_index; // 0x4
	int16 a; // 0x6
	real32 x; // 0x8
	int16 trigger_volume_index; // 0xc
	int16 unknown; // 0xe
	char exit_condition_script[32]; // 0x10
	int16 exit_condition_script_index; // 0x30
	int16 unknown_2; // 0x32
	uint32 flags; // 0x34
};
ASSERT_STRUCT_SIZE(h2x_scnr_ai_triggers_conditions, 0x38);

struct h2x_scnr_ai_triggers
{
	char name[32]; // 0x0
	uint32 trigger_flags; // 0x20
	int16 combination_rule; // 0x24
	int16 unknown; // 0x26
	tag_block<h2x_scnr_ai_triggers_conditions> conditions; // 0x28
};
ASSERT_STRUCT_SIZE(h2x_scnr_ai_triggers, 0x30);

struct h2x_scnr_background_sound_palette
{
	char name[32]; // 0x0
	tag_reference background_sound; // 0x20
	tag_reference inside_cluster_sound; // 0x28
	int8 pad_30[20];
	real32 cutoff_distance; // 0x44
	uint32 scale_flags; // 0x48
	real32 interior_scale; // 0x4c
	real32 portal_scale; // 0x50
	real32 exterior_scale; // 0x54
	real32 interpolation_speed; // 0x58
	int8 pad_5c[8];
};
ASSERT_STRUCT_SIZE(h2x_scnr_background_sound_palette, 0x64);

struct h2x_scnr_sound_environment_palette
{
	char name[32]; // 0x0
	tag_reference sound_environment; // 0x20
	real32 cutoff_distance; // 0x28
	real32 interpolation_speed; // 0x2c
	int8 pad_30[24];
};
ASSERT_STRUCT_SIZE(h2x_scnr_sound_environment_palette, 0x48);

struct h2x_scnr_weather_palette
{
	char name[32]; // 0x0
	tag_reference weather_system; // 0x20
	int16 unknown; // 0x28
	int16 unknown_2; // 0x2a
	int8 pad_2c[32];
	tag_reference wind; // 0x4c
	real_vector3d wind_direction; // 0x54
	real32 wind_magnitude; // 0x60
	int8 pad_64[4];
	char wind_scale_function[32]; // 0x68
};
ASSERT_STRUCT_SIZE(h2x_scnr_weather_palette, 0x88);

struct h2x_scnr_scenario_cluster_data_background_sounds
{
	int16 palette_index; // 0x0
	int16 unknown; // 0x2
};
ASSERT_STRUCT_SIZE(h2x_scnr_scenario_cluster_data_background_sounds, 0x4);

struct h2x_scnr_scenario_cluster_data_sound_environments
{
	int16 palette_index; // 0x0
	int16 unknown; // 0x2
};
ASSERT_STRUCT_SIZE(h2x_scnr_scenario_cluster_data_sound_environments, 0x4);

struct h2x_scnr_scenario_cluster_data_cluster_centroids
{
	real_point3d centroid; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scnr_scenario_cluster_data_cluster_centroids, 0xc);

struct h2x_scnr_scenario_cluster_data_weather_properties
{
	int16 palette_index; // 0x0
	int16 unknown; // 0x2
};
ASSERT_STRUCT_SIZE(h2x_scnr_scenario_cluster_data_weather_properties, 0x4);

struct h2x_scnr_scenario_cluster_data_atmospheric_fog_properties
{
	int16 palette_index; // 0x0
	int16 unknown; // 0x2
};
ASSERT_STRUCT_SIZE(h2x_scnr_scenario_cluster_data_atmospheric_fog_properties, 0x4);

struct h2x_scnr_scenario_cluster_data
{
	tag_reference bsp; // 0x0
	tag_block<h2x_scnr_scenario_cluster_data_background_sounds> background_sounds; // 0x8
	tag_block<h2x_scnr_scenario_cluster_data_sound_environments> sound_environments; // 0x10
	int32 bsp_checksum; // 0x18
	tag_block<h2x_scnr_scenario_cluster_data_cluster_centroids> cluster_centroids; // 0x1c
	tag_block<h2x_scnr_scenario_cluster_data_weather_properties> weather_properties; // 0x24
	tag_block<h2x_scnr_scenario_cluster_data_atmospheric_fog_properties> atmospheric_fog_properties; // 0x2c
};
ASSERT_STRUCT_SIZE(h2x_scnr_scenario_cluster_data, 0x34);

struct h2x_scnr_spawn_data_dynamic_spawn_overloads
{
	int16 overload_type; // 0x0
	int16 unknown; // 0x2
	real32 inner_radius; // 0x4
	real32 outer_radius; // 0x8
	real32 weight; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_scnr_spawn_data_dynamic_spawn_overloads, 0x10);

struct h2x_scnr_spawn_data_static_respawn_zones
{
	string_id name; // 0x0
	uint32 relevant_team; // 0x4
	uint32 relevant_games; // 0x8
	uint32 flags; // 0xc
	real_point3d position; // 0x10
	real32 lower_height; // 0x1c
	real32 upper_height; // 0x20
	real32 inner_radius; // 0x24
	real32 outer_radius; // 0x28
	real32 weight; // 0x2c
};
ASSERT_STRUCT_SIZE(h2x_scnr_spawn_data_static_respawn_zones, 0x30);

struct h2x_scnr_spawn_data_static_initial_spawn_zones
{
	string_id name; // 0x0
	uint32 relevant_team; // 0x4
	uint32 relevant_games; // 0x8
	uint32 flags; // 0xc
	real_point3d position; // 0x10
	real32 lower_height; // 0x1c
	real32 upper_height; // 0x20
	real32 inner_radius; // 0x24
	real32 outer_radius; // 0x28
	real32 weight; // 0x2c
};
ASSERT_STRUCT_SIZE(h2x_scnr_spawn_data_static_initial_spawn_zones, 0x30);

struct h2x_scnr_spawn_data
{
	real32 dynamic_spawn_lower_height; // 0x0
	real32 dynamic_spawn_upper_height; // 0x4
	real32 game_objective_reset_height; // 0x8
	int8 pad_c[60];
	tag_block<h2x_scnr_spawn_data_dynamic_spawn_overloads> dynamic_spawn_overloads; // 0x48
	tag_block<h2x_scnr_spawn_data_static_respawn_zones> static_respawn_zones; // 0x50
	tag_block<h2x_scnr_spawn_data_static_initial_spawn_zones> static_initial_spawn_zones; // 0x58
};
ASSERT_STRUCT_SIZE(h2x_scnr_spawn_data, 0x60);

struct h2x_scnr_crates
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint32 placement_flags; // 0x4
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	real32 scale; // 0x20
	uint16 transform_flags; // 0x24
	uint16 manual_bsp_flags; // 0x26
	datum unique_id; // 0x28
	int16 origin_bsp_index; // 0x2c
	int8 type; // 0x2e
	int8 source; // 0x2f
	int8 bsp_policy; // 0x30
	int8 unknown; // 0x31
	int16 editor_folder_index; // 0x32
	string_id variant_name; // 0x34
	uint32 active_change_colors; // 0x38
	uint32 primary_color; // 0x3c
	uint32 secondary_color; // 0x40
	uint32 tertiary_color; // 0x44
	uint32 quaternary_color; // 0x48
};
ASSERT_STRUCT_SIZE(h2x_scnr_crates, 0x4c);

struct h2x_scnr_crates_palette
{
	tag_reference crate; // 0x0
	int8 pad_8[32];
};
ASSERT_STRUCT_SIZE(h2x_scnr_crates_palette, 0x28);

struct h2x_scnr_atmospheric_fog_palette_mixers
{
	int8 pad_0[4];
	string_id atmospheric_fog_source; // 0x4
	string_id interpolator; // 0x8
	int16 unknown; // 0xc
	int16 unknown_2; // 0xe
};
ASSERT_STRUCT_SIZE(h2x_scnr_atmospheric_fog_palette_mixers, 0x10);

struct h2x_scnr_atmospheric_fog_palette
{
	string_id name; // 0x0
	real_rgb_color color; // 0x4
	real32 spread_distance; // 0x10
	int8 pad_14[4];
	real32 maximum_density; // 0x18
	real32 start_distance; // 0x1c
	real32 opaque_distance; // 0x20
	real_rgb_color color_2; // 0x24
	int8 pad_30[4];
	real32 maximum_density_2; // 0x34
	real32 start_distance_2; // 0x38
	real32 opaque_distance_2; // 0x3c
	int8 pad_40[4];
	real_rgb_color planar_color; // 0x44
	real32 planar_maximum_density; // 0x50
	real32 planar_override_amount; // 0x54
	real32 planar_minimum_distance_bias; // 0x58
	int8 pad_5c[44];
	real_rgb_color patchy_color; // 0x88
	int8 pad_94[12];
	real_bounds patchy_density; // 0xa0
	real_bounds patchy_distance; // 0xa8
	int8 pad_b0[32];
	tag_reference patchy_fog; // 0xd0
	tag_block<h2x_scnr_atmospheric_fog_palette_mixers> mixers; // 0xd8
	real32 amount; // 0xe0
	real32 threshold; // 0xe4
	real32 brightness; // 0xe8
	real32 gamma_power; // 0xec
	uint16 camera_immersion_flags; // 0xf0
	int16 unknown; // 0xf2
};
ASSERT_STRUCT_SIZE(h2x_scnr_atmospheric_fog_palette, 0xf4);

struct h2x_scnr_planar_fog_palette
{
	string_id name; // 0x0
	tag_reference planar_fog; // 0x4
	int16 unknown; // 0xc
	int16 unknown_2; // 0xe
};
ASSERT_STRUCT_SIZE(h2x_scnr_planar_fog_palette, 0x10);

struct h2x_scnr_flocks_sources
{
	real_point3d position; // 0x0
	real_euler_angles2d starting; // 0xc
	real32 radius; // 0x14
	real32 weight; // 0x18
};
ASSERT_STRUCT_SIZE(h2x_scnr_flocks_sources, 0x1c);

struct h2x_scnr_flocks_sinks
{
	real_point3d position; // 0x0
	real32 radius; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_scnr_flocks_sinks, 0x10);

struct h2x_scnr_flocks
{
	int16 bsp_index; // 0x0
	int16 unknown; // 0x2
	int16 bounding_trigger_volume; // 0x4
	uint16 flags; // 0x6
	real32 ecology_margin; // 0x8
	tag_block<h2x_scnr_flocks_sources> sources; // 0xc
	tag_block<h2x_scnr_flocks_sinks> sinks; // 0x14
	real32 production_frequency; // 0x1c
	real_bounds scale; // 0x20
	tag_reference creature; // 0x28
	short_bounds boid_count; // 0x30
	real32 neighborhood_radius; // 0x34
	real32 avoidance_radius; // 0x38
	real32 forward_scale; // 0x3c
	real32 alignment_scale; // 0x40
	real32 avoidance_scale; // 0x44
	real32 leveling_force_scale; // 0x48
	real32 sink_scale; // 0x4c
	real32 perception_angle; // 0x50
	real32 average_throttle; // 0x54
	real32 maximum_throttle; // 0x58
	real32 position_scale; // 0x5c
	real32 position_minimum_radius; // 0x60
	real32 position_maximum_radius; // 0x64
	real32 movement_weight_threshold; // 0x68
	real32 danger_radius; // 0x6c
	real32 danger_scale; // 0x70
	real32 random_offset_scale; // 0x74
	real_bounds random_offset_period; // 0x78
	string_id flock_name; // 0x80
};
ASSERT_STRUCT_SIZE(h2x_scnr_flocks, 0x84);

struct h2x_scnr_decorators_cache_blocks_resources
{
	int8 type; // 0x0
	int8 unknown; // 0x1
	int16 unknown_2; // 0x2
	int16 primary_locator; // 0x4
	int16 secondary_locator; // 0x6
	uint32 resource_data_size; // 0x8
	uint32 resource_data_offset; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_scnr_decorators_cache_blocks_resources, 0x10);

struct h2x_scnr_decorators_cache_blocks_cache_block_data_placements
{
	int32 internal_data_1; // 0x0
	int32 compressed_position; // 0x4
	uint32 tint_color; // 0x8
	uint32 lightmap_color; // 0xc
	int32 compressed_light_direction; // 0x10
	int32 compressed_light_2_direction; // 0x14
};
ASSERT_STRUCT_SIZE(h2x_scnr_decorators_cache_blocks_cache_block_data_placements, 0x18);

struct h2x_scnr_decorators_cache_blocks_cache_block_data_decal_vertices
{
	real_point3d position; // 0x0
	real_point2d texcoord_0; // 0xc
	real_point2d texcoord_1; // 0x14
	uint32 color; // 0x1c
};
ASSERT_STRUCT_SIZE(h2x_scnr_decorators_cache_blocks_cache_block_data_decal_vertices, 0x20);

struct h2x_scnr_decorators_cache_blocks_cache_block_data_decal_indices
{
	int16 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scnr_decorators_cache_blocks_cache_block_data_decal_indices, 0x2);

struct h2x_scnr_decorators_cache_blocks_cache_block_data_sprite_vertices
{
	real_point3d position; // 0x0
	real_vector3d offset; // 0xc
	real_vector3d axis; // 0x18
	real_point2d texcoord; // 0x24
	uint32 color; // 0x2c
};
ASSERT_STRUCT_SIZE(h2x_scnr_decorators_cache_blocks_cache_block_data_sprite_vertices, 0x30);

struct h2x_scnr_decorators_cache_blocks_cache_block_data_sprite_indices
{
	int16 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scnr_decorators_cache_blocks_cache_block_data_sprite_indices, 0x2);

struct h2x_scnr_decorators_cache_blocks_cache_block_data
{
	tag_block<h2x_scnr_decorators_cache_blocks_cache_block_data_placements> placements; // 0x0
	tag_block<h2x_scnr_decorators_cache_blocks_cache_block_data_decal_vertices> decal_vertices; // 0x8
	tag_block<h2x_scnr_decorators_cache_blocks_cache_block_data_decal_indices> decal_indices; // 0x10
	uint32 decal_vertex_buffer; // 0x18
	int8 pad_1c[16];
	tag_block<h2x_scnr_decorators_cache_blocks_cache_block_data_sprite_vertices> sprite_vertices; // 0x2c
	tag_block<h2x_scnr_decorators_cache_blocks_cache_block_data_sprite_indices> sprite_indices; // 0x34
	uint32 sprite_vertex_buffer; // 0x3c
	int8 pad_40[16];
};
ASSERT_STRUCT_SIZE(h2x_scnr_decorators_cache_blocks_cache_block_data, 0x50);

struct h2x_scnr_decorators_cache_blocks
{
	int32 resource_block_offset; // 0x0
	uint32 resource_block_size; // 0x4
	uint32 section_data_size; // 0x8
	uint32 resource_data_size; // 0xc
	tag_block<h2x_scnr_decorators_cache_blocks_resources> resources; // 0x10
	datum owner_tag; // 0x18
	int16 owner_tag_section_offset; // 0x1c
	int16 unknown; // 0x1e
	int32 unknown_2; // 0x20
	tag_block<h2x_scnr_decorators_cache_blocks_cache_block_data> cache_block_data; // 0x24
	int8 pad_2c[8];
};
ASSERT_STRUCT_SIZE(h2x_scnr_decorators_cache_blocks, 0x34);

struct h2x_scnr_decorators_groups
{
	int8 decorator_set_index; // 0x0
	int8 decorator_type; // 0x1
	int8 shader_index; // 0x2
	int8 compressed_radius; // 0x3
	int16 cluster; // 0x4
	int16 cache_block_index; // 0x6
	int16 decorator_start_index; // 0x8
	int16 decorator_count; // 0xa
	int16 vertex_start_offset; // 0xc
	int16 vertex_count; // 0xe
	int16 index_start_offset; // 0x10
	int16 index_count; // 0x12
	int32 compressed_bounding_center; // 0x14
};
ASSERT_STRUCT_SIZE(h2x_scnr_decorators_groups, 0x18);

struct h2x_scnr_decorators_cells
{
	int16 child_index; // 0x0
	int16 child_index_2; // 0x2
	int16 child_index_3; // 0x4
	int16 child_index_4; // 0x6
	int16 child_index_5; // 0x8
	int16 child_index_6; // 0xa
	int16 child_index_7; // 0xc
	int16 child_index_8; // 0xe
	int16 cache_block_index; // 0x10
	int16 group_count; // 0x12
	int32 group_start_index; // 0x14
};
ASSERT_STRUCT_SIZE(h2x_scnr_decorators_cells, 0x18);

struct h2x_scnr_decorators_decals
{
	int8 decorator_set_index; // 0x0
	int8 decorator_class; // 0x1
	int8 decorator_permutation; // 0x2
	int8 sprite_index; // 0x3
	real_point3d position; // 0x4
	real_vector3d left; // 0x10
	real_vector3d up; // 0x1c
	real_vector3d extents; // 0x28
	real_point3d previous_position; // 0x34
};
ASSERT_STRUCT_SIZE(h2x_scnr_decorators_decals, 0x40);

struct h2x_scnr_decorators
{
	real_point3d grid_origin; // 0x0
	int32 cell_count_per_dimension; // 0xc
	tag_block<h2x_scnr_decorators_cache_blocks> cache_blocks; // 0x10
	tag_block<h2x_scnr_decorators_groups> groups; // 0x18
	tag_block<h2x_scnr_decorators_cells> cells; // 0x20
	tag_block<h2x_scnr_decorators_decals> decals; // 0x28
};
ASSERT_STRUCT_SIZE(h2x_scnr_decorators, 0x30);

struct h2x_scnr_creatures
{
	int16 palette_index; // 0x0
	int16 name_index; // 0x2
	uint32 placement_flags; // 0x4
	real_point3d position; // 0x8
	real_euler_angles3d rotation; // 0x14
	real32 scale; // 0x20
	uint16 transform_flags; // 0x24
	uint16 manual_bsp_flags; // 0x26
	datum unique_id; // 0x28
	int16 origin_bsp_index; // 0x2c
	int8 type; // 0x2e
	int8 source; // 0x2f
	int8 bsp_policy; // 0x30
	int8 unknown; // 0x31
	int16 editor_folder_index; // 0x32
};
ASSERT_STRUCT_SIZE(h2x_scnr_creatures, 0x34);

struct h2x_scnr_creatures_palette
{
	tag_reference creature; // 0x0
	int8 pad_8[32];
};
ASSERT_STRUCT_SIZE(h2x_scnr_creatures_palette, 0x28);

struct h2x_scnr_decorator_palette
{
	tag_reference decorator_set; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scnr_decorator_palette, 0x8);

struct h2x_scnr_bsp_transition_volumes
{
	int16 unknown; // 0x0
	int16 bsp_index_key; // 0x2
	int16 trigger_volume_index; // 0x4
	int16 unknown_2; // 0x6
};
ASSERT_STRUCT_SIZE(h2x_scnr_bsp_transition_volumes, 0x8);

struct h2x_scnr_structure_bsp_lighting_lighting_points
{
	real_point3d position; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scnr_structure_bsp_lighting_lighting_points, 0xc);

struct h2x_scnr_structure_bsp_lighting
{
	tag_reference bsp; // 0x0
	tag_block<h2x_scnr_structure_bsp_lighting_lighting_points> lighting_points; // 0x8
};
ASSERT_STRUCT_SIZE(h2x_scnr_structure_bsp_lighting, 0x10);

struct h2x_scnr_editor_folders
{
	int32 parent_folder_index; // 0x0
	char name[256]; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_scnr_editor_folders, 0x104);

struct h2x_scnr_level_data_campaign_level_data
{
	int32 campaign_id; // 0x0
	int32 map_id; // 0x4
	tag_reference bitmap; // 0x8
	wchar_t english_name[16]; // 0x10
	int8 pad_30[32];
	wchar_t japanese_name[16]; // 0x50
	int8 pad_70[32];
	wchar_t german_name[16]; // 0x90
	int8 pad_b0[32];
	wchar_t french_name[16]; // 0xd0
	int8 pad_f0[32];
	wchar_t spanish_name[16]; // 0x110
	int8 pad_130[32];
	wchar_t italian_name[16]; // 0x150
	int8 pad_170[32];
	wchar_t korean_name[16]; // 0x190
	int8 pad_1b0[32];
	wchar_t chinese_name[16]; // 0x1d0
	int8 pad_1f0[32];
	wchar_t portuguese_name[16]; // 0x210
	int8 pad_230[32];
	wchar_t english_description[16]; // 0x250
	int8 pad_270[224];
	wchar_t japanese_description[16]; // 0x350
	int8 pad_370[224];
	wchar_t german_description[16]; // 0x450
	int8 pad_470[224];
	wchar_t french_description[16]; // 0x550
	int8 pad_570[224];
	wchar_t spanish_description[16]; // 0x650
	int8 pad_670[224];
	wchar_t italian_description[16]; // 0x750
	int8 pad_770[224];
	wchar_t korean_description[16]; // 0x850
	int8 pad_870[224];
	wchar_t chinese_description[16]; // 0x950
	int8 pad_970[224];
	wchar_t portuguese_description[16]; // 0xa50
	int8 pad_a70[224];
};
ASSERT_STRUCT_SIZE(h2x_scnr_level_data_campaign_level_data, 0xb50);

struct h2x_scnr_level_data_multiplayer
{
	int32 map_id; // 0x0
	tag_reference bitmap; // 0x4
	wchar_t english_name[16]; // 0xc
	int8 pad_2c[32];
	wchar_t japanese_name[16]; // 0x4c
	int8 pad_6c[32];
	wchar_t german_name[16]; // 0x8c
	int8 pad_ac[32];
	wchar_t french_name[16]; // 0xcc
	int8 pad_ec[32];
	wchar_t spanish_name[16]; // 0x10c
	int8 pad_12c[32];
	wchar_t italian_name[16]; // 0x14c
	int8 pad_16c[32];
	wchar_t korean_name[16]; // 0x18c
	int8 pad_1ac[32];
	wchar_t chinese_name[16]; // 0x1cc
	int8 pad_1ec[32];
	wchar_t portuguese_name[16]; // 0x20c
	int8 pad_22c[32];
	wchar_t english_description[16]; // 0x24c
	int8 pad_26c[224];
	wchar_t japanese_description[16]; // 0x34c
	int8 pad_36c[224];
	wchar_t german_description[16]; // 0x44c
	int8 pad_46c[224];
	wchar_t french_description[16]; // 0x54c
	int8 pad_56c[224];
	wchar_t spanish_description[16]; // 0x64c
	int8 pad_66c[224];
	wchar_t italian_description[16]; // 0x74c
	int8 pad_76c[224];
	wchar_t korean_description[16]; // 0x84c
	int8 pad_86c[224];
	wchar_t chinese_description[16]; // 0x94c
	int8 pad_96c[224];
	wchar_t portuguese_description[16]; // 0xa4c
	int8 pad_a6c[224];
	char path[256]; // 0xb4c
	int32 sort_order; // 0xc4c
	uint8 flags; // 0xc50
	int8 unknown; // 0xc51
	int8 unknown_2; // 0xc52
	int8 unknown_3; // 0xc53
	uint8 max_teams_none; // 0xc54
	uint8 max_teams_ctf; // 0xc55
	uint8 max_teams_slayer; // 0xc56
	uint8 max_teams_oddball; // 0xc57
	uint8 max_teams_koth; // 0xc58
	uint8 max_teams_race; // 0xc59
	uint8 max_teams_headhunter; // 0xc5a
	uint8 max_teams_juggernaut; // 0xc5b
	uint8 max_teams_territories; // 0xc5c
	uint8 max_teams_assault; // 0xc5d
	uint8 max_teams_stub_10; // 0xc5e
	uint8 max_teams_stub_11; // 0xc5f
	uint8 max_teams_stub_12; // 0xc60
	uint8 max_teams_stub_13; // 0xc61
	uint8 max_teams_stub_14; // 0xc62
	uint8 max_teams_stub_15; // 0xc63
};
ASSERT_STRUCT_SIZE(h2x_scnr_level_data_multiplayer, 0xc64);

struct h2x_scnr_level_data
{
	tag_reference level_description; // 0x0
	tag_block<h2x_scnr_level_data_campaign_level_data> campaign_level_data; // 0x8
	tag_block<h2x_scnr_level_data_multiplayer> multiplayer; // 0x10
};
ASSERT_STRUCT_SIZE(h2x_scnr_level_data, 0x18);

struct h2x_scnr_mission_dialogue
{
	tag_reference mission_dialogue; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scnr_mission_dialogue, 0x8);

struct h2x_scnr_interpolators
{
	string_id name; // 0x0
	string_id accelerator_name; // 0x4
	string_id multiplier_name; // 0x8
	tag_data function; // 0xc
	int16 unknown; // 0x14
	int16 unknown_2; // 0x16
};
ASSERT_STRUCT_SIZE(h2x_scnr_interpolators, 0x18);

struct h2x_scnr_screen_effect_references
{
	int8 pad_0[16];
	tag_reference screen_effect; // 0x10
	string_id primary_input; // 0x18
	string_id secondary_input; // 0x1c
	int16 unknown; // 0x20
	int16 unknown_2; // 0x22
};
ASSERT_STRUCT_SIZE(h2x_scnr_screen_effect_references, 0x24);

struct h2x_scnr_simulation_definition_table
{
	datum tag; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scnr_simulation_definition_table, 0x4);

struct h2x_scnr
{
	tag_reference do_not_use; // 0x0
	tag_block<h2x_scnr_skies> skies; // 0x8
	int16 type; // 0x10
	uint16 flags; // 0x12
	tag_block<h2x_scnr_child_scenarios> child_scenarios; // 0x14
	real32 local_north; // 0x1c
	tag_block<h2x_scnr_predicted_resources> predicted_resources; // 0x20
	uint32 functions_block; // 0x28
	uint32 functions_block_2; // 0x2c
	tag_data editor_scenario_data; // 0x30
	tag_block<h2x_scnr_comments> comments; // 0x38
	tag_block<h2x_scnr_environment_objects> environment_objects; // 0x40
	tag_block<h2x_scnr_object_names> object_names; // 0x48
	tag_block<h2x_scnr_scenery> scenery; // 0x50
	tag_block<h2x_scnr_scenery_palette> scenery_palette; // 0x58
	tag_block<h2x_scnr_bipeds> bipeds; // 0x60
	tag_block<h2x_scnr_biped_palette> biped_palette; // 0x68
	tag_block<h2x_scnr_vehicles> vehicles; // 0x70
	tag_block<h2x_scnr_vehicle_palette> vehicle_palette; // 0x78
	tag_block<h2x_scnr_equipment> equipment; // 0x80
	tag_block<h2x_scnr_equipment_palette> equipment_palette; // 0x88
	tag_block<h2x_scnr_weapons> weapons; // 0x90
	tag_block<h2x_scnr_weapon_palette> weapon_palette; // 0x98
	tag_block<h2x_scnr_device_groups> device_groups; // 0xa0
	tag_block<h2x_scnr_machines> machines; // 0xa8
	tag_block<h2x_scnr_machine_palette> machine_palette; // 0xb0
	tag_block<h2x_scnr_controls> controls; // 0xb8
	tag_block<h2x_scnr_control_palette> control_palette; // 0xc0
	tag_block<h2x_scnr_light_fixtures> light_fixtures; // 0xc8
	tag_block<h2x_scnr_light_fixtures_palette> light_fixtures_palette; // 0xd0
	tag_block<h2x_scnr_sound_scenery> sound_scenery; // 0xd8
	tag_block<h2x_scnr_sound_scenery_palette> sound_scenery_palette; // 0xe0
	tag_block<h2x_scnr_light_volumes> light_volumes; // 0xe8
	tag_block<h2x_scnr_light_volumes_palette> light_volumes_palette; // 0xf0
	tag_block<h2x_scnr_player_starting_profile> player_starting_profile; // 0xf8
	tag_block<h2x_scnr_player_starting_locations> player_starting_locations; // 0x100
	tag_block<h2x_scnr_kill_trigger_volumes> kill_trigger_volumes; // 0x108
	tag_block<h2x_scnr_recorded_animations> recorded_animations; // 0x110
	tag_block<h2x_scnr_netgame_flags> netgame_flags; // 0x118
	tag_block<h2x_scnr_netgame_equipment> netgame_equipment; // 0x120
	tag_block<h2x_scnr_starting_equipment> starting_equipment; // 0x128
	tag_block<h2x_scnr_bsp_switch_trigger_volumes> bsp_switch_trigger_volumes; // 0x130
	tag_block<h2x_scnr_decals> decals; // 0x138
	tag_block<h2x_scnr_decals_palette> decals_palette; // 0x140
	tag_block<h2x_scnr_detail_object_collection_palette> detail_object_collection_palette; // 0x148
	tag_block<h2x_scnr_style_palette> style_palette; // 0x150
	tag_block<h2x_scnr_squad_groups> squad_groups; // 0x158
	tag_block<h2x_scnr_squads> squads; // 0x160
	tag_block<h2x_scnr_zones> zones; // 0x168
	tag_block<h2x_scnr_mission_scenes> mission_scenes; // 0x170
	tag_block<h2x_scnr_character_palette> character_palette; // 0x178
	uint32 ai_pathfinding_data_block; // 0x180
	uint32 ai_pathfinding_data_block_2; // 0x184
	tag_block<h2x_scnr_ai_animation_references> ai_animation_references; // 0x188
	tag_block<h2x_scnr_ai_script_references> ai_script_references; // 0x190
	tag_block<h2x_scnr_ai_recording_references> ai_recording_references; // 0x198
	tag_block<h2x_scnr_ai_conversations> ai_conversations; // 0x1a0
	tag_data script_syntax_data; // 0x1a8
	tag_data script_string_data; // 0x1b0
	tag_block<h2x_scnr_scripts> scripts; // 0x1b8
	tag_block<h2x_scnr_globals> globals; // 0x1c0
	tag_block<h2x_scnr_script_references> script_references; // 0x1c8
	uint32 source_files_block; // 0x1d0
	uint32 source_files_block_2; // 0x1d4
	tag_block<h2x_scnr_scripting_data> scripting_data; // 0x1d8
	tag_block<h2x_scnr_cutscene_flags> cutscene_flags; // 0x1e0
	tag_block<h2x_scnr_cutscene_camera_points> cutscene_camera_points; // 0x1e8
	tag_block<h2x_scnr_cutscene_titles> cutscene_titles; // 0x1f0
	tag_reference custom_object_names; // 0x1f8
	tag_reference chapter_title_text; // 0x200
	tag_reference hud_messages; // 0x208
	tag_block<h2x_scnr_structure_bsps> structure_bsps; // 0x210
	tag_block<h2x_scnr_scenario_resources> scenario_resources; // 0x218
	tag_block<h2x_scnr_old_structure_physics> old_structure_physics; // 0x220
	tag_block<h2x_scnr_unit_seats_mapping> unit_seats_mapping; // 0x228
	tag_block<h2x_scnr_scenario_kill_triggers> scenario_kill_triggers; // 0x230
	tag_block<h2x_scnr_script_expressions> script_expressions; // 0x238
	tag_block<h2x_scnr_orders> orders; // 0x240
	tag_block<h2x_scnr_ai_triggers> ai_triggers; // 0x248
	tag_block<h2x_scnr_background_sound_palette> background_sound_palette; // 0x250
	tag_block<h2x_scnr_sound_environment_palette> sound_environment_palette; // 0x258
	tag_block<h2x_scnr_weather_palette> weather_palette; // 0x260
	uint32 null_block; // 0x268
	uint32 null_block_2; // 0x26c
	uint32 null_block_3; // 0x270
	uint32 null_block_4; // 0x274
	uint32 null_block_5; // 0x278
	uint32 null_block_6; // 0x27c
	uint32 null_block_7; // 0x280
	uint32 null_block_8; // 0x284
	uint32 null_block_9; // 0x288
	uint32 null_block_10; // 0x28c
	tag_block<h2x_scnr_scenario_cluster_data> scenario_cluster_data; // 0x290
	int32 object_salts_1; // 0x298
	int32 object_salts_2; // 0x29c
	int32 object_salts_3; // 0x2a0
	int32 object_salts_4; // 0x2a4
	int32 object_salts_5; // 0x2a8
	int32 object_salts_6; // 0x2ac
	int32 object_salts_7; // 0x2b0
	int32 object_salts_8; // 0x2b4
	int32 object_salts_9; // 0x2b8
	int32 object_salts_10; // 0x2bc
	int32 object_salts_11; // 0x2c0
	int32 object_salts_12; // 0x2c4
	int32 object_salts_13; // 0x2c8
	int32 object_salts_14; // 0x2cc
	int32 object_salts_15; // 0x2d0
	int32 object_salts_16; // 0x2d4
	int32 object_salts_17; // 0x2d8
	int32 object_salts_18; // 0x2dc
	int32 object_salts_19; // 0x2e0
	int32 object_salts_20; // 0x2e4
	int32 object_salts_21; // 0x2e8
	int32 object_salts_22; // 0x2ec
	int32 object_salts_23; // 0x2f0
	int32 object_salts_24; // 0x2f4
	int32 object_salts_25; // 0x2f8
	int32 object_salts_26; // 0x2fc
	int32 object_salts_27; // 0x300
	int32 object_salts_28; // 0x304
	int32 object_salts_29; // 0x308
	int32 object_salts_30; // 0x30c
	int32 object_salts_31; // 0x310
	int32 object_salts_32; // 0x314
	tag_block<h2x_scnr_spawn_data> spawn_data; // 0x318
	tag_reference sound_effect_collection; // 0x320
	tag_block<h2x_scnr_crates> crates; // 0x328
	tag_block<h2x_scnr_crates_palette> crates_palette; // 0x330
	tag_reference global_lighting; // 0x338
	tag_block<h2x_scnr_atmospheric_fog_palette> atmospheric_fog_palette; // 0x340
	tag_block<h2x_scnr_planar_fog_palette> planar_fog_palette; // 0x348
	tag_block<h2x_scnr_flocks> flocks; // 0x350
	tag_reference subtitles; // 0x358
	tag_block<h2x_scnr_decorators> decorators; // 0x360
	tag_block<h2x_scnr_creatures> creatures; // 0x368
	tag_block<h2x_scnr_creatures_palette> creatures_palette; // 0x370
	tag_block<h2x_scnr_decorator_palette> decorator_palette; // 0x378
	tag_block<h2x_scnr_bsp_transition_volumes> bsp_transition_volumes; // 0x380
	tag_block<h2x_scnr_structure_bsp_lighting> structure_bsp_lighting; // 0x388
	tag_block<h2x_scnr_editor_folders> editor_folders; // 0x390
	tag_block<h2x_scnr_level_data> level_data; // 0x398
	tag_reference territory_location_names; // 0x3a0
	int8 pad_3a8[8];
	tag_block<h2x_scnr_mission_dialogue> mission_dialogue; // 0x3b0
	tag_reference objectives; // 0x3b8
	tag_block<h2x_scnr_interpolators> interpolators; // 0x3c0
	uint32 shared_references_block; // 0x3c8
	uint32 shared_references_block_2; // 0x3cc
	tag_block<h2x_scnr_screen_effect_references> screen_effect_references; // 0x3d0
	tag_block<h2x_scnr_simulation_definition_table> simulation_definition_table; // 0x3d8
};
ASSERT_STRUCT_SIZE(h2x_scnr, 0x3e0);

struct h2x_sbsp_collision_materials
{
	tag_reference old_shader; // 0x0
	int16 global_material_index; // 0x8
	int16 conveyor_surface_index; // 0xa
	tag_reference new_shader; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_sbsp_collision_materials, 0x14);

struct h2x_sbsp_collision_bsp_bsp_3d_nodes
{
	int16 plane; // 0x0
	uint8 front_child_lower; // 0x2
	uint8 front_child_mid; // 0x3
	uint8 front_child_upper; // 0x4
	uint8 back_child_lower; // 0x5
	uint8 back_child_mid; // 0x6
	uint8 back_child_upper; // 0x7
};
ASSERT_STRUCT_SIZE(h2x_sbsp_collision_bsp_bsp_3d_nodes, 0x8);

struct h2x_sbsp_collision_bsp_planes
{
	real_plane3d plane; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_collision_bsp_planes, 0x10);

struct h2x_sbsp_collision_bsp_leaves
{
	uint8 flags; // 0x0
	uint8 bsp_2d_reference_count; // 0x1
	int16 first_bsp_2d_reference; // 0x2
};
ASSERT_STRUCT_SIZE(h2x_sbsp_collision_bsp_leaves, 0x4);

struct h2x_sbsp_collision_bsp_bsp_2d_references
{
	int16 plane; // 0x0
	int16 bsp_2d_node; // 0x2
};
ASSERT_STRUCT_SIZE(h2x_sbsp_collision_bsp_bsp_2d_references, 0x4);

struct h2x_sbsp_collision_bsp_bsp_2d_nodes
{
	real_plane2d plane; // 0x0
	int16 left_child; // 0xc
	int16 right_child; // 0xe
};
ASSERT_STRUCT_SIZE(h2x_sbsp_collision_bsp_bsp_2d_nodes, 0x10);

struct h2x_sbsp_collision_bsp_surfaces
{
	int16 plane; // 0x0
	int16 first_edge; // 0x2
	uint8 flags; // 0x4
	uint8 breakable_surface_index; // 0x5
	int16 material_index; // 0x6
};
ASSERT_STRUCT_SIZE(h2x_sbsp_collision_bsp_surfaces, 0x8);

struct h2x_sbsp_collision_bsp_edges
{
	int16 start_vertex; // 0x0
	int16 end_vertex; // 0x2
	int16 forward_edge; // 0x4
	int16 reverse_edge; // 0x6
	int16 left_surface; // 0x8
	int16 right_surface; // 0xa
};
ASSERT_STRUCT_SIZE(h2x_sbsp_collision_bsp_edges, 0xc);

struct h2x_sbsp_collision_bsp_vertices
{
	real_point3d point; // 0x0
	int16 first_edge; // 0xc
	int16 unknown; // 0xe
};
ASSERT_STRUCT_SIZE(h2x_sbsp_collision_bsp_vertices, 0x10);

struct h2x_sbsp_collision_bsp
{
	tag_block<h2x_sbsp_collision_bsp_bsp_3d_nodes> bsp_3d_nodes; // 0x0
	tag_block<h2x_sbsp_collision_bsp_planes> planes; // 0x8
	tag_block<h2x_sbsp_collision_bsp_leaves> leaves; // 0x10
	tag_block<h2x_sbsp_collision_bsp_bsp_2d_references> bsp_2d_references; // 0x18
	tag_block<h2x_sbsp_collision_bsp_bsp_2d_nodes> bsp_2d_nodes; // 0x20
	tag_block<h2x_sbsp_collision_bsp_surfaces> surfaces; // 0x28
	tag_block<h2x_sbsp_collision_bsp_edges> edges; // 0x30
	tag_block<h2x_sbsp_collision_bsp_vertices> vertices; // 0x38
};
ASSERT_STRUCT_SIZE(h2x_sbsp_collision_bsp, 0x40);

struct h2x_sbsp_leaves
{
	int16 cluster; // 0x0
	int16 surface_reference_count; // 0x2
	int32 first_surface_reference_index; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_sbsp_leaves, 0x8);

struct h2x_sbsp_surface_references
{
	int16 strip_index; // 0x0
	int16 lightmap_triangle_index; // 0x2
	int32 bsp_node_index; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_sbsp_surface_references, 0x8);

struct h2x_sbsp_cluster_portals_vertices
{
	real_point3d point; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_cluster_portals_vertices, 0xc);

struct h2x_sbsp_cluster_portals
{
	int16 back_cluster; // 0x0
	int16 front_cluster; // 0x2
	int32 plane_index; // 0x4
	real_point3d centroid; // 0x8
	real32 bounding_radius; // 0x14
	uint32 flags; // 0x18
	tag_block<h2x_sbsp_cluster_portals_vertices> vertices; // 0x1c
};
ASSERT_STRUCT_SIZE(h2x_sbsp_cluster_portals, 0x24);

struct h2x_sbsp_fog_planes
{
	int16 scenario_planar_fog_index; // 0x0
	int16 unknown; // 0x2
	real_plane3d plane; // 0x4
	uint16 flags; // 0x14
	int16 priority; // 0x16
};
ASSERT_STRUCT_SIZE(h2x_sbsp_fog_planes, 0x18);

struct h2x_sbsp_weather_palette
{
	char name[32]; // 0x0
	tag_reference weather_system; // 0x20
	int16 unknown; // 0x28
	int16 unknown_2; // 0x2a
	int8 pad_2c[32];
	tag_reference wind; // 0x4c
	real_vector3d wind_direction; // 0x54
	real32 wind_magnitude; // 0x60
	int8 pad_64[4];
	char wind_scale_function[32]; // 0x68
};
ASSERT_STRUCT_SIZE(h2x_sbsp_weather_palette, 0x88);

struct h2x_sbsp_weather_polyhedra_planes
{
	real_plane3d plane; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_weather_polyhedra_planes, 0x10);

struct h2x_sbsp_weather_polyhedra
{
	real_point3d bounding_sphere_center; // 0x0
	real32 bounding_sphere_radius; // 0xc
	tag_block<h2x_sbsp_weather_polyhedra_planes> planes; // 0x10
};
ASSERT_STRUCT_SIZE(h2x_sbsp_weather_polyhedra, 0x18);

struct h2x_sbsp_detail_objects_cells
{
	int16 unknown; // 0x0
	int16 unknown_2; // 0x2
	int16 unknown_3; // 0x4
	int16 unknown_4; // 0x6
	int32 unknown_5; // 0x8
	int32 unknown_6; // 0xc
	int32 unknown_7; // 0x10
	int8 pad_14[12];
};
ASSERT_STRUCT_SIZE(h2x_sbsp_detail_objects_cells, 0x20);

struct h2x_sbsp_detail_objects_instances
{
	int8 unknown; // 0x0
	int8 unknown_2; // 0x1
	int8 unknown_3; // 0x2
	int8 unknown_4; // 0x3
	int16 unknown_5; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_sbsp_detail_objects_instances, 0x6);

struct h2x_sbsp_detail_objects_counts
{
	int16 unknown; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_detail_objects_counts, 0x2);

struct h2x_sbsp_detail_objects_z_reference_vectors
{
	real32 unknown; // 0x0
	real32 unknown_2; // 0x4
	real32 unknown_3; // 0x8
	real32 unknown_4; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_sbsp_detail_objects_z_reference_vectors, 0x10);

struct h2x_sbsp_detail_objects
{
	tag_block<h2x_sbsp_detail_objects_cells> cells; // 0x0
	tag_block<h2x_sbsp_detail_objects_instances> instances; // 0x8
	tag_block<h2x_sbsp_detail_objects_counts> counts; // 0x10
	tag_block<h2x_sbsp_detail_objects_z_reference_vectors> z_reference_vectors; // 0x18
	int8 unknown; // 0x20
	int8 unknown_2; // 0x21
	int16 unknown_3; // 0x22
};
ASSERT_STRUCT_SIZE(h2x_sbsp_detail_objects, 0x24);

struct h2x_sbsp_clusters_compression_info
{
	real_bounds position_bounds_x; // 0x0
	real_bounds position_bounds_y; // 0x8
	real_bounds position_bounds_z; // 0x10
	real_bounds texcoord_bounds_x; // 0x18
	real_bounds texcoord_bounds_y; // 0x20
	real_bounds secondary_texcoord_bounds_x; // 0x28
	real_bounds secondary_texcoord_bounds_y; // 0x30
};
ASSERT_STRUCT_SIZE(h2x_sbsp_clusters_compression_info, 0x38);

struct h2x_sbsp_clusters_resources
{
	int8 type; // 0x0
	int8 unknown; // 0x1
	int16 unknown_2; // 0x2
	int16 primary_locator; // 0x4
	int16 secondary_locator; // 0x6
	uint32 resource_data_size; // 0x8
	uint32 resource_data_offset; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_sbsp_clusters_resources, 0x10);

struct h2x_sbsp_clusters_cluster_data_parts
{
	int16 type; // 0x0
	uint16 flags; // 0x2
	int16 material_index; // 0x4
	int16 strip_start_index; // 0x6
	int16 strip_length; // 0x8
	int16 first_subpart_index; // 0xa
	int16 subpart_count; // 0xc
	int8 maximum_nodes_vertex; // 0xe
	int8 contributing_compound_node_count; // 0xf
	real_point3d centroid_position; // 0x10
	int8 node_index_0; // 0x1c
	int8 node_index_1; // 0x1d
	int8 node_index_2; // 0x1e
	int8 node_index_3; // 0x1f
	real32 node_weight_0; // 0x20
	real32 node_weight_1; // 0x24
	real32 node_weight_2; // 0x28
	real32 lod_mipmap_magic_number; // 0x2c
	int8 pad_30[24];
};
ASSERT_STRUCT_SIZE(h2x_sbsp_clusters_cluster_data_parts, 0x48);

struct h2x_sbsp_clusters_cluster_data_subparts
{
	int16 indices_start_index; // 0x0
	int16 indices_length; // 0x2
	int16 visibility_bounds_index; // 0x4
	int16 part_index; // 0x6
};
ASSERT_STRUCT_SIZE(h2x_sbsp_clusters_cluster_data_subparts, 0x8);

struct h2x_sbsp_clusters_cluster_data_visibility_bounds
{
	real_point3d position; // 0x0
	real32 radius; // 0xc
	int8 node_0; // 0x10
	int8 unknown; // 0x11
	int8 unknown_2; // 0x12
	int8 unknown_3; // 0x13
};
ASSERT_STRUCT_SIZE(h2x_sbsp_clusters_cluster_data_visibility_bounds, 0x14);

struct h2x_sbsp_clusters_cluster_data_raw_vertices
{
	real_point3d position; // 0x0
	int32 old_node_index_0; // 0xc
	int32 old_node_index_1; // 0x10
	int32 old_node_index_2; // 0x14
	int32 old_node_index_3; // 0x18
	real32 node_weight_0; // 0x1c
	real32 node_weight_1; // 0x20
	real32 node_weight_2; // 0x24
	real32 node_weight_3; // 0x28
	int32 new_node_index_0; // 0x2c
	int32 new_node_index_1; // 0x30
	int32 new_node_index_2; // 0x34
	int32 new_node_index_3; // 0x38
	int32 use_new_node_indices; // 0x3c
	int32 adjusted_compound_node_index; // 0x40
	real_point2d texcoord; // 0x44
	real_vector3d normal; // 0x4c
	real_vector3d binormal; // 0x58
	real_vector3d tangent; // 0x64
	real_vector3d anisotropic_binormal; // 0x70
	real_point2d secondary_texcoord; // 0x7c
	real_rgb_color primary_lightmap_color; // 0x84
	real_point2d primary_lightmap_texcoord; // 0x90
	real_vector3d primary_lightmap_incident_direction; // 0x98
	int8 pad_a4[32];
};
ASSERT_STRUCT_SIZE(h2x_sbsp_clusters_cluster_data_raw_vertices, 0xc4);

struct h2x_sbsp_clusters_cluster_data_strip_indices
{
	int16 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_clusters_cluster_data_strip_indices, 0x2);

struct h2x_sbsp_clusters_cluster_data_mopp_reorder_table
{
	int16 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_clusters_cluster_data_mopp_reorder_table, 0x2);

struct h2x_sbsp_clusters_cluster_data_vertex_buffers
{
	uint32 vertex_buffer; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_clusters_cluster_data_vertex_buffers, 0x4);

struct h2x_sbsp_clusters_cluster_data
{
	tag_block<h2x_sbsp_clusters_cluster_data_parts> parts; // 0x0
	tag_block<h2x_sbsp_clusters_cluster_data_subparts> subparts; // 0x8
	tag_block<h2x_sbsp_clusters_cluster_data_visibility_bounds> visibility_bounds; // 0x10
	tag_block<h2x_sbsp_clusters_cluster_data_raw_vertices> raw_vertices; // 0x18
	tag_block<h2x_sbsp_clusters_cluster_data_strip_indices> strip_indices; // 0x20
	tag_data visibility_mopp_codes; // 0x28
	tag_block<h2x_sbsp_clusters_cluster_data_mopp_reorder_table> mopp_reorder_table; // 0x30
	tag_block<h2x_sbsp_clusters_cluster_data_vertex_buffers> vertex_buffers; // 0x38
	int8 pad_40[4];
};
ASSERT_STRUCT_SIZE(h2x_sbsp_clusters_cluster_data, 0x44);

struct h2x_sbsp_clusters_predicted_resources
{
	int16 type; // 0x0
	int16 resource_index; // 0x2
	datum tag_index; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_sbsp_clusters_predicted_resources, 0x8);

struct h2x_sbsp_clusters_portals
{
	int16 portal_index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_clusters_portals, 0x2);

struct h2x_sbsp_clusters_instanced_geometry_indices
{
	int16 instanced_geometry_index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_clusters_instanced_geometry_indices, 0x2);

struct h2x_sbsp_clusters_index_reorder_table
{
	int16 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_clusters_index_reorder_table, 0x2);

struct h2x_sbsp_clusters
{
	uint16 total_vertex_count; // 0x0
	uint16 total_triangle_count; // 0x2
	uint16 total_part_count; // 0x4
	uint16 shadow_casting_triangle_count; // 0x6
	uint16 shadow_casting_part_count; // 0x8
	uint16 opaque_point_count; // 0xa
	uint16 opaque_vertex_count; // 0xc
	uint16 opaque_part_count; // 0xe
	uint8 opaque_maximum_nodes_vertex; // 0x10
	uint8 transparent_maximum_nodes_vertex; // 0x11
	uint16 shadow_casting_rigid_triangle_count; // 0x12
	int16 geometry_classification; // 0x14
	uint16 geometry_compression_flags; // 0x16
	tag_block<h2x_sbsp_clusters_compression_info> compression_info; // 0x18
	uint8 hardware_node_count; // 0x20
	uint8 node_map_size; // 0x21
	uint16 software_plane_count; // 0x22
	uint16 total_subpart_count; // 0x24
	uint16 section_lighting_flags; // 0x26
	int32 resource_block_offset; // 0x28
	uint32 resource_block_size; // 0x2c
	uint32 section_data_size; // 0x30
	uint32 resource_data_size; // 0x34
	tag_block<h2x_sbsp_clusters_resources> resources; // 0x38
	datum owner_tag; // 0x40
	int16 owner_tag_section_offset; // 0x44
	int16 unknown; // 0x46
	int32 unknown_2; // 0x48
	tag_block<h2x_sbsp_clusters_cluster_data> cluster_data; // 0x4c
	real_bounds bounds_x; // 0x54
	real_bounds bounds_y; // 0x5c
	real_bounds bounds_z; // 0x64
	int8 scenario_sky_index; // 0x6c
	int8 media_index; // 0x6d
	int8 scenario_visible_sky_index; // 0x6e
	int8 scenario_atmospheric_fog_index; // 0x6f
	int8 planar_fog_designator; // 0x70
	int8 visible_fog_plane_index; // 0x71
	int16 background_sound_index; // 0x72
	int16 sound_environment_index; // 0x74
	int16 weather_index; // 0x76
	int16 transition_structure_bsp; // 0x78
	int16 unknown_3; // 0x7a
	int16 unknown_4; // 0x7c
	int16 unknown_5; // 0x7e
	uint16 flags; // 0x80
	int16 unknown_6; // 0x82
	tag_block<h2x_sbsp_clusters_predicted_resources> predicted_resources; // 0x84
	tag_block<h2x_sbsp_clusters_portals> portals; // 0x8c
	int32 checksum_from_structure; // 0x94
	tag_block<h2x_sbsp_clusters_instanced_geometry_indices> instanced_geometry_indices; // 0x98
	tag_block<h2x_sbsp_clusters_index_reorder_table> index_reorder_table; // 0xa0
	tag_data collision_mopp_codes; // 0xa8
};
ASSERT_STRUCT_SIZE(h2x_sbsp_clusters, 0xb0);

struct h2x_sbsp_materials_properties
{
	int16 type; // 0x0
	int16 integer_value; // 0x2
	real32 real_value; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_sbsp_materials_properties, 0x8);

struct h2x_sbsp_materials
{
	tag_reference old_shader; // 0x0
	tag_reference shader; // 0x8
	tag_block<h2x_sbsp_materials_properties> properties; // 0x10
	int8 pad_18[4];
	int8 breakable_surface_index; // 0x1c
	int8 unknown; // 0x1d
	int16 unknown_2; // 0x1e
};
ASSERT_STRUCT_SIZE(h2x_sbsp_materials, 0x20);

struct h2x_sbsp_sky_owner_cluster
{
	int16 cluster_owner; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_sky_owner_cluster, 0x2);

struct h2x_sbsp_conveyor_surfaces
{
	real_vector3d u; // 0x0
	real_vector3d v; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_sbsp_conveyor_surfaces, 0x18);

struct h2x_sbsp_breakable_surfaces
{
	int16 instanced_geometry_instance_index; // 0x0
	int16 breakable_surface_index; // 0x2
	real_point3d centroid; // 0x4
	real32 radius; // 0x10
	int32 collision_surface_index; // 0x14
};
ASSERT_STRUCT_SIZE(h2x_sbsp_breakable_surfaces, 0x18);

struct h2x_sbsp_pathfinding_data_sectors
{
	uint16 path_finding_sector_flags; // 0x0
	int16 hint_index; // 0x2
	int32 first_link; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_sectors, 0x8);

struct h2x_sbsp_pathfinding_data_links
{
	int16 vertex_1; // 0x0
	int16 vertex_2; // 0x2
	uint16 link_flags; // 0x4
	int16 hint_index; // 0x6
	int16 forward_link; // 0x8
	int16 reverse_link; // 0xa
	int16 left_sector; // 0xc
	int16 right_sector; // 0xe
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_links, 0x10);

struct h2x_sbsp_pathfinding_data_references
{
	int32 node_reference_or_sector_reference; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_references, 0x4);

struct h2x_sbsp_pathfinding_data_bsp_2d_nodes
{
	real_plane2d plane; // 0x0
	int32 left_child; // 0xc
	int32 right_child; // 0x10
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_bsp_2d_nodes, 0x14);

struct h2x_sbsp_pathfinding_data_surface_flags
{
	int32 flags; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_surface_flags, 0x4);

struct h2x_sbsp_pathfinding_data_vertices
{
	real_point3d point; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_vertices, 0xc);

struct h2x_sbsp_pathfinding_data_object_references_bsps
{
	int32 bsp_reference; // 0x0
	int32 first_sector; // 0x4
	int32 last_sector; // 0x8
	int16 node_index; // 0xc
	int16 unknown; // 0xe
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_object_references_bsps, 0x10);

struct h2x_sbsp_pathfinding_data_object_references_nodes
{
	int16 reference_frame_index; // 0x0
	uint8 projection_axis; // 0x2
	uint8 projection_sign; // 0x3
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_object_references_nodes, 0x4);

struct h2x_sbsp_pathfinding_data_object_references
{
	uint16 flags; // 0x0
	int16 unknown; // 0x2
	int32 first_sector; // 0x4
	int32 last_sector; // 0x8
	tag_block<h2x_sbsp_pathfinding_data_object_references_bsps> bsps; // 0xc
	tag_block<h2x_sbsp_pathfinding_data_object_references_nodes> nodes; // 0x14
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_object_references, 0x1c);

struct h2x_sbsp_pathfinding_data_pathfinding_hints
{
	int16 hint_type; // 0x0
	int16 next_hint_index; // 0x2
	int16 hint_data_0; // 0x4
	int16 hint_data_1; // 0x6
	int16 hint_data_2; // 0x8
	int16 hint_data_3; // 0xa
	int16 hint_data_4; // 0xc
	int16 hint_data_5; // 0xe
	int16 hint_data_6; // 0x10
	int16 hint_data_7; // 0x12
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_pathfinding_hints, 0x14);

struct h2x_sbsp_pathfinding_data_instanced_geometry_refs
{
	int16 pathfinding_object_index; // 0x0
	int16 unknown; // 0x2
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_instanced_geometry_refs, 0x4);

struct h2x_sbsp_pathfinding_data_user_placed_hints_point_geometry
{
	real_point3d point; // 0x0
	int16 reference_frame; // 0xc
	int16 unknown; // 0xe
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_user_placed_hints_point_geometry, 0x10);

struct h2x_sbsp_pathfinding_data_user_placed_hints_ray_geometry
{
	real_point3d point; // 0x0
	int16 reference_frame; // 0xc
	int16 unknown; // 0xe
	real_vector3d vector; // 0x10
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_user_placed_hints_ray_geometry, 0x1c);

struct h2x_sbsp_pathfinding_data_user_placed_hints_line_segment_geometry
{
	uint32 flags; // 0x0
	real_point3d point_0; // 0x4
	int16 reference_frame; // 0x10
	int16 unknown; // 0x12
	real_point3d point_1; // 0x14
	int16 reference_frame_2; // 0x20
	int16 unknown_2; // 0x22
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_user_placed_hints_line_segment_geometry, 0x24);

struct h2x_sbsp_pathfinding_data_user_placed_hints_parallelogram_geometry
{
	uint32 flags; // 0x0
	real_point3d point_0; // 0x4
	int16 reference_frame; // 0x10
	int16 unknown; // 0x12
	real_point3d point_1; // 0x14
	int16 reference_frame_2; // 0x20
	int16 unknown_2; // 0x22
	real_point3d point_2; // 0x24
	int16 reference_frame_3; // 0x30
	int16 unknown_3; // 0x32
	real_point3d point_3; // 0x34
	int16 reference_frame_4; // 0x40
	int16 unknown_4; // 0x42
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_user_placed_hints_parallelogram_geometry, 0x44);

struct h2x_sbsp_pathfinding_data_user_placed_hints_polygon_geometry_points
{
	real_point3d point; // 0x0
	int16 reference_frame; // 0xc
	int16 unknown; // 0xe
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_user_placed_hints_polygon_geometry_points, 0x10);

struct h2x_sbsp_pathfinding_data_user_placed_hints_polygon_geometry
{
	uint32 flags; // 0x0
	tag_block<h2x_sbsp_pathfinding_data_user_placed_hints_polygon_geometry_points> points; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_user_placed_hints_polygon_geometry, 0xc);

struct h2x_sbsp_pathfinding_data_user_placed_hints_jump_hints
{
	uint16 flags; // 0x0
	int16 geometry_index; // 0x2
	int16 force_jump_height; // 0x4
	uint16 control_flags; // 0x6
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_user_placed_hints_jump_hints, 0x8);

struct h2x_sbsp_pathfinding_data_user_placed_hints_climb_hints
{
	uint16 flags; // 0x0
	int16 geometry_index; // 0x2
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_user_placed_hints_climb_hints, 0x4);

struct h2x_sbsp_pathfinding_data_user_placed_hints_well_hints_points
{
	int16 type; // 0x0
	int16 unknown; // 0x2
	real_vector3d point; // 0x4
	int16 reference_frame; // 0x10
	int16 unknown_2; // 0x12
	int32 sector_index; // 0x14
	real_euler_angles2d normal; // 0x18
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_user_placed_hints_well_hints_points, 0x20);

struct h2x_sbsp_pathfinding_data_user_placed_hints_well_hints
{
	uint32 flags; // 0x0
	tag_block<h2x_sbsp_pathfinding_data_user_placed_hints_well_hints_points> points; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_user_placed_hints_well_hints, 0xc);

struct h2x_sbsp_pathfinding_data_user_placed_hints_flight_hints_points
{
	real_vector3d point; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_user_placed_hints_flight_hints_points, 0xc);

struct h2x_sbsp_pathfinding_data_user_placed_hints_flight_hints
{
	tag_block<h2x_sbsp_pathfinding_data_user_placed_hints_flight_hints_points> points; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_user_placed_hints_flight_hints, 0x8);

struct h2x_sbsp_pathfinding_data_user_placed_hints
{
	tag_block<h2x_sbsp_pathfinding_data_user_placed_hints_point_geometry> point_geometry; // 0x0
	tag_block<h2x_sbsp_pathfinding_data_user_placed_hints_ray_geometry> ray_geometry; // 0x8
	tag_block<h2x_sbsp_pathfinding_data_user_placed_hints_line_segment_geometry> line_segment_geometry; // 0x10
	tag_block<h2x_sbsp_pathfinding_data_user_placed_hints_parallelogram_geometry> parallelogram_geometry; // 0x18
	tag_block<h2x_sbsp_pathfinding_data_user_placed_hints_polygon_geometry> polygon_geometry; // 0x20
	tag_block<h2x_sbsp_pathfinding_data_user_placed_hints_jump_hints> jump_hints; // 0x28
	tag_block<h2x_sbsp_pathfinding_data_user_placed_hints_climb_hints> climb_hints; // 0x30
	tag_block<h2x_sbsp_pathfinding_data_user_placed_hints_well_hints> well_hints; // 0x38
	tag_block<h2x_sbsp_pathfinding_data_user_placed_hints_flight_hints> flight_hints; // 0x40
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data_user_placed_hints, 0x48);

struct h2x_sbsp_pathfinding_data
{
	tag_block<h2x_sbsp_pathfinding_data_sectors> sectors; // 0x0
	tag_block<h2x_sbsp_pathfinding_data_links> links; // 0x8
	tag_block<h2x_sbsp_pathfinding_data_references> references; // 0x10
	tag_block<h2x_sbsp_pathfinding_data_bsp_2d_nodes> bsp_2d_nodes; // 0x18
	tag_block<h2x_sbsp_pathfinding_data_surface_flags> surface_flags; // 0x20
	tag_block<h2x_sbsp_pathfinding_data_vertices> vertices; // 0x28
	tag_block<h2x_sbsp_pathfinding_data_object_references> object_references; // 0x30
	tag_block<h2x_sbsp_pathfinding_data_pathfinding_hints> pathfinding_hints; // 0x38
	tag_block<h2x_sbsp_pathfinding_data_instanced_geometry_refs> instanced_geometry_refs; // 0x40
	int32 structure_checksum; // 0x48
	int8 pad_4c[32];
	tag_block<h2x_sbsp_pathfinding_data_user_placed_hints> user_placed_hints; // 0x6c
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_data, 0x74);

struct h2x_sbsp_pathfinding_edges
{
	int8 midpoint; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_pathfinding_edges, 0x1);

struct h2x_sbsp_background_sound_palette
{
	char name[32]; // 0x0
	tag_reference background_sound; // 0x20
	tag_reference inside_cluster_sound; // 0x28
	int8 pad_30[20];
	real32 cutoff_distance; // 0x44
	uint32 scale_flags; // 0x48
	real32 interior_scale; // 0x4c
	real32 portal_scale; // 0x50
	real32 exterior_scale; // 0x54
	real32 interpolation_speed; // 0x58
	int8 pad_5c[8];
};
ASSERT_STRUCT_SIZE(h2x_sbsp_background_sound_palette, 0x64);

struct h2x_sbsp_sound_environment_palette
{
	char name[32]; // 0x0
	tag_reference sound_environment; // 0x20
	real32 cutoff_distance; // 0x28
	real32 interpolation_speed; // 0x2c
	int8 pad_30[24];
};
ASSERT_STRUCT_SIZE(h2x_sbsp_sound_environment_palette, 0x48);

struct h2x_sbsp_markers
{
	char name[32]; // 0x0
	real_quaternion rotation; // 0x20
	real_point3d position; // 0x30
};
ASSERT_STRUCT_SIZE(h2x_sbsp_markers, 0x3c);

struct h2x_sbsp_runtime_decals
{
	real_point3d position; // 0x0
	int16 decal_type; // 0xc
	int8 yaw; // 0xe
	int8 pitch; // 0xf
};
ASSERT_STRUCT_SIZE(h2x_sbsp_runtime_decals, 0x10);

struct h2x_sbsp_environment_object_palette
{
	tag_reference definition; // 0x0
	tag_reference model; // 0x8
	uint32 object_type; // 0x10
};
ASSERT_STRUCT_SIZE(h2x_sbsp_environment_object_palette, 0x14);

struct h2x_sbsp_environment_objects
{
	char name[32]; // 0x0
	real_quaternion rotation; // 0x20
	real_point3d translation; // 0x30
	int16 palette_index; // 0x3c
	int16 unknown; // 0x3e
	datum unique_id; // 0x40
	char exported_object_type[4]; // 0x44
	char scenario_object_name[32]; // 0x48
};
ASSERT_STRUCT_SIZE(h2x_sbsp_environment_objects, 0x68);

struct h2x_sbsp_instanced_geometry_definitions_compression_info
{
	real_bounds position_bounds_x; // 0x0
	real_bounds position_bounds_y; // 0x8
	real_bounds position_bounds_z; // 0x10
	real_bounds texcoord_bounds_x; // 0x18
	real_bounds texcoord_bounds_y; // 0x20
	real_bounds secondary_texcoord_bounds_x; // 0x28
	real_bounds secondary_texcoord_bounds_y; // 0x30
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_compression_info, 0x38);

struct h2x_sbsp_instanced_geometry_definitions_resources
{
	int8 type; // 0x0
	int8 unknown; // 0x1
	int16 unknown_2; // 0x2
	int16 primary_locator; // 0x4
	int16 secondary_locator; // 0x6
	uint32 resource_data_size; // 0x8
	uint32 resource_data_offset; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_resources, 0x10);

struct h2x_sbsp_instanced_geometry_definitions_render_data_parts
{
	int16 type; // 0x0
	uint16 flags; // 0x2
	int16 material_index; // 0x4
	int16 strip_start_index; // 0x6
	int16 strip_length; // 0x8
	int16 first_subpart_index; // 0xa
	int16 subpart_count; // 0xc
	int8 maximum_nodes_vertex; // 0xe
	int8 contributing_compound_node_count; // 0xf
	real_point3d position; // 0x10
	int8 node_index_0; // 0x1c
	int8 node_index_1; // 0x1d
	int8 node_index_2; // 0x1e
	int8 node_index_3; // 0x1f
	real32 node_weight_0; // 0x20
	real32 node_weight_1; // 0x24
	real32 node_weight_2; // 0x28
	real32 lod_mipmap_magic_number; // 0x2c
	int8 pad_30[24];
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_render_data_parts, 0x48);

struct h2x_sbsp_instanced_geometry_definitions_render_data_subparts
{
	int16 indices_start_index; // 0x0
	int16 indices_length; // 0x2
	int16 visibility_bounds_index; // 0x4
	int16 part_index; // 0x6
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_render_data_subparts, 0x8);

struct h2x_sbsp_instanced_geometry_definitions_render_data_visibility_bounds
{
	real_point3d position; // 0x0
	real32 radius; // 0xc
	int8 node_0; // 0x10
	int8 unknown; // 0x11
	int8 unknown_2; // 0x12
	int8 unknown_3; // 0x13
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_render_data_visibility_bounds, 0x14);

struct h2x_sbsp_instanced_geometry_definitions_render_data_raw_vertices
{
	real_point3d position; // 0x0
	int32 old_node_index_0; // 0xc
	int32 old_node_index_1; // 0x10
	int32 old_node_index_2; // 0x14
	int32 old_node_index_3; // 0x18
	real32 node_weight_0; // 0x1c
	real32 node_weight_1; // 0x20
	real32 node_weight_2; // 0x24
	real32 node_weight_3; // 0x28
	int32 new_node_index_0; // 0x2c
	int32 new_node_index_1; // 0x30
	int32 new_node_index_2; // 0x34
	int32 new_node_index_3; // 0x38
	int32 use_new_node_indices; // 0x3c
	int32 adjusted_compound_node_index; // 0x40
	real_point2d texcoord; // 0x44
	real_vector3d normal; // 0x4c
	real_vector3d binormal; // 0x58
	real_vector3d tangent; // 0x64
	real_vector3d anisotropic_binormal; // 0x70
	real_point2d secondary_texcoord; // 0x7c
	real_rgb_color primary_lightmap_color; // 0x84
	real_point2d primary_lightmap_texcoord; // 0x90
	real_vector3d primary_lightmap_incident_direction; // 0x98
	int8 pad_a4[32];
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_render_data_raw_vertices, 0xc4);

struct h2x_sbsp_instanced_geometry_definitions_render_data_strip_indices
{
	int16 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_render_data_strip_indices, 0x2);

struct h2x_sbsp_instanced_geometry_definitions_render_data_mopp_reorder_table
{
	int16 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_render_data_mopp_reorder_table, 0x2);

struct h2x_sbsp_instanced_geometry_definitions_render_data_vertex_buffers
{
	uint32 vertex_buffer; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_render_data_vertex_buffers, 0x4);

struct h2x_sbsp_instanced_geometry_definitions_render_data
{
	tag_block<h2x_sbsp_instanced_geometry_definitions_render_data_parts> parts; // 0x0
	tag_block<h2x_sbsp_instanced_geometry_definitions_render_data_subparts> subparts; // 0x8
	tag_block<h2x_sbsp_instanced_geometry_definitions_render_data_visibility_bounds> visibility_bounds; // 0x10
	tag_block<h2x_sbsp_instanced_geometry_definitions_render_data_raw_vertices> raw_vertices; // 0x18
	tag_block<h2x_sbsp_instanced_geometry_definitions_render_data_strip_indices> strip_indices; // 0x20
	tag_data visibility_mopp_codes; // 0x28
	tag_block<h2x_sbsp_instanced_geometry_definitions_render_data_mopp_reorder_table> mopp_reorder_table; // 0x30
	tag_block<h2x_sbsp_instanced_geometry_definitions_render_data_vertex_buffers> vertex_buffers; // 0x38
	int8 pad_40[4];
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_render_data, 0x44);

struct h2x_sbsp_instanced_geometry_definitions_index_reorder_table
{
	int16 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_index_reorder_table, 0x2);

struct h2x_sbsp_instanced_geometry_definitions_bsp_3d_nodes
{
	int8 pad_0[8];
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_bsp_3d_nodes, 0x8);

struct h2x_sbsp_instanced_geometry_definitions_planes
{
	real_plane3d plane; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_planes, 0x10);

struct h2x_sbsp_instanced_geometry_definitions_leaves
{
	uint8 flags; // 0x0
	uint8 bsp2d_reference_count; // 0x1
	int16 first_bsp2d_reference; // 0x2
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_leaves, 0x4);

struct h2x_sbsp_instanced_geometry_definitions_bsp_2d_references
{
	int16 plane; // 0x0
	int16 bsp_2d_node; // 0x2
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_bsp_2d_references, 0x4);

struct h2x_sbsp_instanced_geometry_definitions_bsp_2d_nodes
{
	real_plane2d plane; // 0x0
	int16 left_child; // 0xc
	int16 right_child; // 0xe
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_bsp_2d_nodes, 0x10);

struct h2x_sbsp_instanced_geometry_definitions_surfaces
{
	int16 plane; // 0x0
	int16 first_edge; // 0x2
	uint8 flags; // 0x4
	uint8 breakable_surface_index; // 0x5
	int16 material_index; // 0x6
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_surfaces, 0x8);

struct h2x_sbsp_instanced_geometry_definitions_edges
{
	int16 start_vertex; // 0x0
	int16 end_vertex; // 0x2
	int16 forward_edge; // 0x4
	int16 reverse_edge; // 0x6
	int16 left_surface; // 0x8
	int16 right_surface; // 0xa
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_edges, 0xc);

struct h2x_sbsp_instanced_geometry_definitions_vertices
{
	real_point3d point; // 0x0
	int16 first_edge; // 0xc
	int16 unknown; // 0xe
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_vertices, 0x10);

struct h2x_sbsp_instanced_geometry_definitions_bsp_physics
{
	uint32 runtime_code_pointer; // 0x0
	int16 size; // 0x4
	int16 count; // 0x6
	uint32 user_data; // 0x8
	uint32 unknown; // 0xc
	real_vector3d center; // 0x10
	real32 w_center; // 0x1c
	real_vector3d half_extent; // 0x20
	real32 w_half_extent; // 0x2c
	tag_reference runtime_model_tag; // 0x30
	int8 pad_38[8];
	uint32 runtime_code_pointer_2; // 0x40
	int16 size_2; // 0x44
	int16 count_2; // 0x46
	uint32 user_data_2; // 0x48
	uint32 unknown_2; // 0x4c
	uint32 runtime_code_pointer_3; // 0x50
	int16 size_3; // 0x54
	int16 count_3; // 0x56
	uint32 user_data_3; // 0x58
	uint32 unknown_3; // 0x5c
	uint32 unknown_4; // 0x60
	tag_data mopp_code_data; // 0x64
	int8 pad_6c[8];
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_bsp_physics, 0x74);

struct h2x_sbsp_instanced_geometry_definitions_render_leaves
{
	int16 cluster; // 0x0
	int16 surface_reference_count; // 0x2
	int32 first_surface_reference_index; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_render_leaves, 0x8);

struct h2x_sbsp_instanced_geometry_definitions_surface_references
{
	int16 strip_index; // 0x0
	int16 lightmap_triangle_index; // 0x2
	int32 bsp_node_index; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions_surface_references, 0x8);

struct h2x_sbsp_instanced_geometry_definitions
{
	uint16 total_vertex_count; // 0x0
	uint16 total_triangle_count; // 0x2
	uint16 total_part_count; // 0x4
	uint16 shadow_casting_triangle_count; // 0x6
	uint16 shadow_casting_part_count; // 0x8
	uint16 opaque_point_count; // 0xa
	uint16 opaque_vertex_count; // 0xc
	uint16 opaque_part_count; // 0xe
	uint8 opaque_maximum_nodes_vertex; // 0x10
	uint8 transparent_maximum_nodes_vertex; // 0x11
	uint16 shadow_casting_rigid_triangle_count; // 0x12
	int16 geometry_classification; // 0x14
	uint16 geometry_compression_flags; // 0x16
	tag_block<h2x_sbsp_instanced_geometry_definitions_compression_info> compression_info; // 0x18
	uint8 hardware_node_count; // 0x20
	uint8 node_map_size; // 0x21
	uint16 software_plane_count; // 0x22
	uint16 total_subpart_count; // 0x24
	uint16 section_lighting_flags; // 0x26
	int32 resource_block_offset; // 0x28
	uint32 resource_block_size; // 0x2c
	uint32 section_data_size; // 0x30
	uint32 resource_data_size; // 0x34
	tag_block<h2x_sbsp_instanced_geometry_definitions_resources> resources; // 0x38
	datum owner_tag; // 0x40
	int16 owner_tag_section_offset; // 0x44
	int16 unknown; // 0x46
	int32 unknown_2; // 0x48
	tag_block<h2x_sbsp_instanced_geometry_definitions_render_data> render_data; // 0x4c
	tag_block<h2x_sbsp_instanced_geometry_definitions_index_reorder_table> index_reorder_table; // 0x54
	int32 checksum; // 0x5c
	real_point3d bounding_sphere_center; // 0x60
	real32 bounding_sphere_radius; // 0x6c
	tag_block<h2x_sbsp_instanced_geometry_definitions_bsp_3d_nodes> bsp_3d_nodes; // 0x70
	tag_block<h2x_sbsp_instanced_geometry_definitions_planes> planes; // 0x78
	tag_block<h2x_sbsp_instanced_geometry_definitions_leaves> leaves; // 0x80
	tag_block<h2x_sbsp_instanced_geometry_definitions_bsp_2d_references> bsp_2d_references; // 0x88
	tag_block<h2x_sbsp_instanced_geometry_definitions_bsp_2d_nodes> bsp_2d_nodes; // 0x90
	tag_block<h2x_sbsp_instanced_geometry_definitions_surfaces> surfaces; // 0x98
	tag_block<h2x_sbsp_instanced_geometry_definitions_edges> edges; // 0xa0
	tag_block<h2x_sbsp_instanced_geometry_definitions_vertices> vertices; // 0xa8
	tag_block<h2x_sbsp_instanced_geometry_definitions_bsp_physics> bsp_physics; // 0xb0
	tag_block<h2x_sbsp_instanced_geometry_definitions_render_leaves> render_leaves; // 0xb8
	tag_block<h2x_sbsp_instanced_geometry_definitions_surface_references> surface_references; // 0xc0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_definitions, 0xc8);

struct h2x_sbsp_instanced_geometry_instances
{
	real32 scale; // 0x0
	real_vector3d forward; // 0x4
	real_vector3d left; // 0x10
	real_vector3d up; // 0x1c
	real_point3d position; // 0x28
	int16 instance_definition_index; // 0x34
	uint16 flags; // 0x36
	int8 pad_38[20];
	int32 checksum; // 0x4c
	string_id name; // 0x50
	int16 pathfinding_policy; // 0x54
	int16 lightmapping_policy; // 0x56
};
ASSERT_STRUCT_SIZE(h2x_sbsp_instanced_geometry_instances, 0x58);

struct h2x_sbsp_ambience_sound_clusters_enclosing_portal_designators
{
	int16 portal_designator; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_ambience_sound_clusters_enclosing_portal_designators, 0x2);

struct h2x_sbsp_ambience_sound_clusters_interior_cluster_indices
{
	int16 interior_cluster_index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_ambience_sound_clusters_interior_cluster_indices, 0x2);

struct h2x_sbsp_ambience_sound_clusters
{
	int16 unknown; // 0x0
	int16 unknown_2; // 0x2
	tag_block<h2x_sbsp_ambience_sound_clusters_enclosing_portal_designators> enclosing_portal_designators; // 0x4
	tag_block<h2x_sbsp_ambience_sound_clusters_interior_cluster_indices> interior_cluster_indices; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_sbsp_ambience_sound_clusters, 0x14);

struct h2x_sbsp_reverb_sound_clusters_enclosing_portal_designators
{
	int16 portal_designator; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_reverb_sound_clusters_enclosing_portal_designators, 0x2);

struct h2x_sbsp_reverb_sound_clusters_interior_cluster_indices
{
	int16 interior_cluster_index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_reverb_sound_clusters_interior_cluster_indices, 0x2);

struct h2x_sbsp_reverb_sound_clusters
{
	int16 unknown; // 0x0
	int16 unknown_2; // 0x2
	tag_block<h2x_sbsp_reverb_sound_clusters_enclosing_portal_designators> enclosing_portal_designators; // 0x4
	tag_block<h2x_sbsp_reverb_sound_clusters_interior_cluster_indices> interior_cluster_indices; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_sbsp_reverb_sound_clusters, 0x14);

struct h2x_sbsp_transparent_planes
{
	int16 section_index; // 0x0
	int16 part_index; // 0x2
	real_plane3d plane; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_sbsp_transparent_planes, 0x14);

struct h2x_sbsp_debug_info_clusters_lines
{
	int16 type; // 0x0
	int16 code; // 0x2
	int16 pad_thai; // 0x4
	int16 unknown; // 0x6
	real_point3d point_0; // 0x8
	real_point3d point_1; // 0x14
};
ASSERT_STRUCT_SIZE(h2x_sbsp_debug_info_clusters_lines, 0x20);

struct h2x_sbsp_debug_info_clusters_fog_plane_indices
{
	int32 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_debug_info_clusters_fog_plane_indices, 0x4);

struct h2x_sbsp_debug_info_clusters_visible_fog_plane_indices
{
	int32 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_debug_info_clusters_visible_fog_plane_indices, 0x4);

struct h2x_sbsp_debug_info_clusters_visible_fog_omission_cluster_indices
{
	int32 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_debug_info_clusters_visible_fog_omission_cluster_indices, 0x4);

struct h2x_sbsp_debug_info_clusters_containing_fog_zone_indices
{
	int32 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_debug_info_clusters_containing_fog_zone_indices, 0x4);

struct h2x_sbsp_debug_info_clusters
{
	uint16 errors; // 0x0
	uint16 warnings; // 0x2
	int8 pad_4[28];
	tag_block<h2x_sbsp_debug_info_clusters_lines> lines; // 0x20
	tag_block<h2x_sbsp_debug_info_clusters_fog_plane_indices> fog_plane_indices; // 0x28
	tag_block<h2x_sbsp_debug_info_clusters_visible_fog_plane_indices> visible_fog_plane_indices; // 0x30
	tag_block<h2x_sbsp_debug_info_clusters_visible_fog_omission_cluster_indices> visible_fog_omission_cluster_indices; // 0x38
	tag_block<h2x_sbsp_debug_info_clusters_containing_fog_zone_indices> containing_fog_zone_indices; // 0x40
};
ASSERT_STRUCT_SIZE(h2x_sbsp_debug_info_clusters, 0x48);

struct h2x_sbsp_debug_info_fog_planes_lines
{
	int16 type; // 0x0
	int16 code; // 0x2
	int16 pad_thai; // 0x4
	int16 unknown; // 0x6
	real_point3d point_0; // 0x8
	real_point3d point_1; // 0x14
};
ASSERT_STRUCT_SIZE(h2x_sbsp_debug_info_fog_planes_lines, 0x20);

struct h2x_sbsp_debug_info_fog_planes_intersected_cluster_indices
{
	int32 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_debug_info_fog_planes_intersected_cluster_indices, 0x4);

struct h2x_sbsp_debug_info_fog_planes_infinite_extent_cluster_indices
{
	int32 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_debug_info_fog_planes_infinite_extent_cluster_indices, 0x4);

struct h2x_sbsp_debug_info_fog_planes
{
	int32 fog_zone_index; // 0x0
	int8 pad_4[24];
	int32 connected_plane_designator; // 0x1c
	tag_block<h2x_sbsp_debug_info_fog_planes_lines> lines; // 0x20
	tag_block<h2x_sbsp_debug_info_fog_planes_intersected_cluster_indices> intersected_cluster_indices; // 0x28
	tag_block<h2x_sbsp_debug_info_fog_planes_infinite_extent_cluster_indices> infinite_extent_cluster_indices; // 0x30
};
ASSERT_STRUCT_SIZE(h2x_sbsp_debug_info_fog_planes, 0x38);

struct h2x_sbsp_debug_info_fog_zones_lines
{
	int16 type; // 0x0
	int16 code; // 0x2
	int16 pad_thai; // 0x4
	int16 unknown; // 0x6
	real_point3d point_0; // 0x8
	real_point3d point_1; // 0x14
};
ASSERT_STRUCT_SIZE(h2x_sbsp_debug_info_fog_zones_lines, 0x20);

struct h2x_sbsp_debug_info_fog_zones_immersed_cluster_indices
{
	int32 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_debug_info_fog_zones_immersed_cluster_indices, 0x4);

struct h2x_sbsp_debug_info_fog_zones_bounding_fog_plane_indices
{
	int32 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_debug_info_fog_zones_bounding_fog_plane_indices, 0x4);

struct h2x_sbsp_debug_info_fog_zones_collision_fog_plane_indices
{
	int32 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_debug_info_fog_zones_collision_fog_plane_indices, 0x4);

struct h2x_sbsp_debug_info_fog_zones
{
	int32 media_index_scenario_fog_plane; // 0x0
	int32 base_fog_plane_index; // 0x4
	int8 pad_8[24];
	tag_block<h2x_sbsp_debug_info_fog_zones_lines> lines; // 0x20
	tag_block<h2x_sbsp_debug_info_fog_zones_immersed_cluster_indices> immersed_cluster_indices; // 0x28
	tag_block<h2x_sbsp_debug_info_fog_zones_bounding_fog_plane_indices> bounding_fog_plane_indices; // 0x30
	tag_block<h2x_sbsp_debug_info_fog_zones_collision_fog_plane_indices> collision_fog_plane_indices; // 0x38
};
ASSERT_STRUCT_SIZE(h2x_sbsp_debug_info_fog_zones, 0x40);

struct h2x_sbsp_debug_info
{
	int8 pad_0[64];
	tag_block<h2x_sbsp_debug_info_clusters> clusters; // 0x40
	tag_block<h2x_sbsp_debug_info_fog_planes> fog_planes; // 0x48
	tag_block<h2x_sbsp_debug_info_fog_zones> fog_zones; // 0x50
};
ASSERT_STRUCT_SIZE(h2x_sbsp_debug_info, 0x58);

struct h2x_sbsp_breakable_surface_key_table
{
	int16 instance_geometry_index; // 0x0
	int16 breakable_surface_index; // 0x2
	int32 seed_surface_index; // 0x4
	real32 x0; // 0x8
	real32 x1; // 0xc
	real32 y0; // 0x10
	real32 y1; // 0x14
	real32 z0; // 0x18
	real32 z1; // 0x1c
};
ASSERT_STRUCT_SIZE(h2x_sbsp_breakable_surface_key_table, 0x20);

struct h2x_sbsp_water_definitions_section_parts
{
	int16 type; // 0x0
	uint16 flags; // 0x2
	int16 material_index; // 0x4
	int16 strip_start_index; // 0x6
	int16 strip_length; // 0x8
	int16 first_subpart_index; // 0xa
	int16 subpart_count; // 0xc
	int8 maximum_nodes_vertex; // 0xe
	int8 contributing_compound_node_count; // 0xf
	real_point3d position; // 0x10
	int8 node_index_0; // 0x1c
	int8 node_index_1; // 0x1d
	int8 node_index_2; // 0x1e
	int8 node_index_3; // 0x1f
	real32 node_weight_0; // 0x20
	real32 node_weight_1; // 0x24
	real32 node_weight_2; // 0x28
	real32 lod_mipmap_magic_number; // 0x2c
	int8 pad_30[24];
};
ASSERT_STRUCT_SIZE(h2x_sbsp_water_definitions_section_parts, 0x48);

struct h2x_sbsp_water_definitions_section_subparts
{
	int16 indices_start_index; // 0x0
	int16 indices_length; // 0x2
	int16 visibility_bounds_index; // 0x4
	int16 part_index; // 0x6
};
ASSERT_STRUCT_SIZE(h2x_sbsp_water_definitions_section_subparts, 0x8);

struct h2x_sbsp_water_definitions_section_visibility_bounds
{
	real_point3d position; // 0x0
	real32 radius; // 0xc
	int8 node_0; // 0x10
	int8 unknown; // 0x11
	int8 unknown_2; // 0x12
	int8 unknown_3; // 0x13
};
ASSERT_STRUCT_SIZE(h2x_sbsp_water_definitions_section_visibility_bounds, 0x14);

struct h2x_sbsp_water_definitions_section_raw_vertices
{
	real_point3d position; // 0x0
	int32 old_node_index_0; // 0xc
	int32 old_node_index_1; // 0x10
	int32 old_node_index_2; // 0x14
	int32 old_node_index_3; // 0x18
	real32 node_weight_0; // 0x1c
	real32 node_weight_1; // 0x20
	real32 node_weight_2; // 0x24
	real32 node_weight_3; // 0x28
	int32 new_node_index_0; // 0x2c
	int32 new_node_index_1; // 0x30
	int32 new_node_index_2; // 0x34
	int32 new_node_index_3; // 0x38
	int32 use_new_node_indices; // 0x3c
	int32 adjusted_compound_node_index; // 0x40
	real_point2d texcoord; // 0x44
	real_vector3d normal; // 0x4c
	real_vector3d binormal; // 0x58
	real_vector3d tangent; // 0x64
	real_vector3d anisotropic_binormal; // 0x70
	real_point2d secondary_texcoord; // 0x7c
	real_rgb_color primary_lightmap_color; // 0x84
	real_point2d primary_lightmap_texcoord; // 0x90
	real_vector3d primary_lightmap_incident_direction; // 0x98
	int8 pad_a4[32];
};
ASSERT_STRUCT_SIZE(h2x_sbsp_water_definitions_section_raw_vertices, 0xc4);

struct h2x_sbsp_water_definitions_section_strip_indices
{
	int16 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_water_definitions_section_strip_indices, 0x2);

struct h2x_sbsp_water_definitions_section_mopp_reorder_table
{
	int16 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_water_definitions_section_mopp_reorder_table, 0x2);

struct h2x_sbsp_water_definitions_section_vertex_buffers
{
	uint32 vertex_buffer; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_water_definitions_section_vertex_buffers, 0x4);

struct h2x_sbsp_water_definitions_section
{
	tag_block<h2x_sbsp_water_definitions_section_parts> parts; // 0x0
	tag_block<h2x_sbsp_water_definitions_section_subparts> subparts; // 0x8
	tag_block<h2x_sbsp_water_definitions_section_visibility_bounds> visibility_bounds; // 0x10
	tag_block<h2x_sbsp_water_definitions_section_raw_vertices> raw_vertices; // 0x18
	tag_block<h2x_sbsp_water_definitions_section_strip_indices> strip_indices; // 0x20
	tag_data visibility_mopp_codes; // 0x28
	tag_block<h2x_sbsp_water_definitions_section_mopp_reorder_table> mopp_reorder_table; // 0x30
	tag_block<h2x_sbsp_water_definitions_section_vertex_buffers> vertex_buffers; // 0x38
	int8 pad_40[4];
};
ASSERT_STRUCT_SIZE(h2x_sbsp_water_definitions_section, 0x44);

struct h2x_sbsp_water_definitions_resources
{
	int8 type; // 0x0
	int8 unknown; // 0x1
	int16 unknown_2; // 0x2
	int16 primary_locator; // 0x4
	int16 secondary_locator; // 0x6
	uint32 resource_data_size; // 0x8
	uint32 resource_data_offset; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_sbsp_water_definitions_resources, 0x10);

struct h2x_sbsp_water_definitions
{
	tag_reference shader; // 0x0
	tag_block<h2x_sbsp_water_definitions_section> section; // 0x8
	int32 resource_block_offset; // 0x10
	uint32 resource_block_size; // 0x14
	uint32 section_data_size; // 0x18
	uint32 resource_data_size; // 0x1c
	tag_block<h2x_sbsp_water_definitions_resources> resources; // 0x20
	datum owner_tag; // 0x28
	int16 owner_tag_section_offset; // 0x2c
	int16 unknown; // 0x2e
	int32 unknown_2; // 0x30
	real_rgb_color sun_spot_color; // 0x34
	real_rgb_color reflection_tint; // 0x40
	real_rgb_color refraction_tint; // 0x4c
	real_rgb_color horizon_color; // 0x58
	real32 sun_specular_power; // 0x64
	real32 reflection_bump_scale; // 0x68
	real32 refraction_bump_scale; // 0x6c
	real32 fresnel_scale; // 0x70
	real32 sun_dir_heading; // 0x74
	real32 sun_dir_pitch; // 0x78
	real32 fov; // 0x7c
	real32 aspect; // 0x80
	real32 height; // 0x84
	real32 farz; // 0x88
	real32 rotate_offset; // 0x8c
	real_vector2d center; // 0x90
	real_vector2d extents; // 0x98
	real32 fog_near; // 0xa0
	real32 fog_far; // 0xa4
	real32 dynamic_height_bias; // 0xa8
};
ASSERT_STRUCT_SIZE(h2x_sbsp_water_definitions, 0xac);

struct h2x_sbsp_portal_device_mapping_device_portal_associations
{
	datum unique_id; // 0x0
	int16 origin_bsp_index; // 0x4
	int8 type; // 0x6
	int8 source; // 0x7
	int16 first_game_portal_index; // 0x8
	int16 game_portal_count; // 0xa
};
ASSERT_STRUCT_SIZE(h2x_sbsp_portal_device_mapping_device_portal_associations, 0xc);

struct h2x_sbsp_portal_device_mapping_game_portal_to_portal_map
{
	int16 portal_index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_portal_device_mapping_game_portal_to_portal_map, 0x2);

struct h2x_sbsp_portal_device_mapping
{
	tag_block<h2x_sbsp_portal_device_mapping_device_portal_associations> device_portal_associations; // 0x0
	tag_block<h2x_sbsp_portal_device_mapping_game_portal_to_portal_map> game_portal_to_portal_map; // 0x8
};
ASSERT_STRUCT_SIZE(h2x_sbsp_portal_device_mapping, 0x10);

struct h2x_sbsp_audibility_encoded_door_pas
{
	int32 encoded_data; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_audibility_encoded_door_pas, 0x4);

struct h2x_sbsp_audibility_cluster_door_portal_encoded_pas
{
	int32 encoded_data; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_audibility_cluster_door_portal_encoded_pas, 0x4);

struct h2x_sbsp_audibility_ai_deafening_pas
{
	int32 encoded_data; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_audibility_ai_deafening_pas, 0x4);

struct h2x_sbsp_audibility_cluster_distances
{
	int8 encoded_data; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_audibility_cluster_distances, 0x1);

struct h2x_sbsp_audibility_machine_door_mapping
{
	int8 machine_door_index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_audibility_machine_door_mapping, 0x1);

struct h2x_sbsp_audibility
{
	int32 door_portal_count; // 0x0
	real_bounds cluster_distance_bounds; // 0x4
	tag_block<h2x_sbsp_audibility_encoded_door_pas> encoded_door_pas; // 0xc
	tag_block<h2x_sbsp_audibility_cluster_door_portal_encoded_pas> cluster_door_portal_encoded_pas; // 0x14
	tag_block<h2x_sbsp_audibility_ai_deafening_pas> ai_deafening_pas; // 0x1c
	tag_block<h2x_sbsp_audibility_cluster_distances> cluster_distances; // 0x24
	tag_block<h2x_sbsp_audibility_machine_door_mapping> machine_door_mapping; // 0x2c
};
ASSERT_STRUCT_SIZE(h2x_sbsp_audibility, 0x34);

struct h2x_sbsp_object_fake_lightprobes
{
	datum unique_id; // 0x0
	int16 origin_bsp_index; // 0x4
	int8 type; // 0x6
	int8 source; // 0x7
	real_rgb_color ambient; // 0x8
	real_vector3d shadow_direction; // 0x14
	real32 lighting_accuracy; // 0x20
	real32 shadow_opacity; // 0x24
	real_rgb_color primary_direction_color; // 0x28
	real_vector3d primary_direction; // 0x34
	real_rgb_color secondary_direction_color; // 0x40
	real_vector3d secondary_direction; // 0x4c
	int16 sh_index; // 0x58
	int16 unknown; // 0x5a
};
ASSERT_STRUCT_SIZE(h2x_sbsp_object_fake_lightprobes, 0x5c);

struct h2x_sbsp_decorators_cache_blocks_resources
{
	int8 type; // 0x0
	int8 unknown; // 0x1
	int16 unknown_2; // 0x2
	int16 primary_locator; // 0x4
	int16 secondary_locator; // 0x6
	uint32 resource_data_size; // 0x8
	uint32 resource_data_offset; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_sbsp_decorators_cache_blocks_resources, 0x10);

struct h2x_sbsp_decorators_cache_blocks_cache_block_data_placements
{
	int32 internal_data_1; // 0x0
	int32 compressed_position; // 0x4
	uint32 tint_color; // 0x8
	uint32 lightmap_color; // 0xc
	int32 compressed_light_direction; // 0x10
	int32 compressed_light_2_direction; // 0x14
};
ASSERT_STRUCT_SIZE(h2x_sbsp_decorators_cache_blocks_cache_block_data_placements, 0x18);

struct h2x_sbsp_decorators_cache_blocks_cache_block_data_decal_vertices
{
	real_point3d position; // 0x0
	real_point2d texcoord_0; // 0xc
	real_point2d texcoord_1; // 0x14
	uint32 color; // 0x1c
};
ASSERT_STRUCT_SIZE(h2x_sbsp_decorators_cache_blocks_cache_block_data_decal_vertices, 0x20);

struct h2x_sbsp_decorators_cache_blocks_cache_block_data_decal_indices
{
	int16 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_decorators_cache_blocks_cache_block_data_decal_indices, 0x2);

struct h2x_sbsp_decorators_cache_blocks_cache_block_data_sprite_vertices
{
	real_point3d position; // 0x0
	real_vector3d offset; // 0xc
	real_vector3d axis; // 0x18
	real_point2d texcoord; // 0x24
	uint32 color; // 0x2c
};
ASSERT_STRUCT_SIZE(h2x_sbsp_decorators_cache_blocks_cache_block_data_sprite_vertices, 0x30);

struct h2x_sbsp_decorators_cache_blocks_cache_block_data_sprite_indices
{
	int16 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_sbsp_decorators_cache_blocks_cache_block_data_sprite_indices, 0x2);

struct h2x_sbsp_decorators_cache_blocks_cache_block_data
{
	tag_block<h2x_sbsp_decorators_cache_blocks_cache_block_data_placements> placements; // 0x0
	tag_block<h2x_sbsp_decorators_cache_blocks_cache_block_data_decal_vertices> decal_vertices; // 0x8
	tag_block<h2x_sbsp_decorators_cache_blocks_cache_block_data_decal_indices> decal_indices; // 0x10
	uint32 decal_vertex_buffer; // 0x18
	int8 pad_1c[16];
	tag_block<h2x_sbsp_decorators_cache_blocks_cache_block_data_sprite_vertices> sprite_vertices; // 0x2c
	tag_block<h2x_sbsp_decorators_cache_blocks_cache_block_data_sprite_indices> sprite_indices; // 0x34
	uint32 sprite_vertex_buffer; // 0x3c
	int8 pad_40[16];
};
ASSERT_STRUCT_SIZE(h2x_sbsp_decorators_cache_blocks_cache_block_data, 0x50);

struct h2x_sbsp_decorators_cache_blocks
{
	int32 resource_block_offset; // 0x0
	uint32 resource_block_size; // 0x4
	uint32 section_data_size; // 0x8
	uint32 resource_data_size; // 0xc
	tag_block<h2x_sbsp_decorators_cache_blocks_resources> resources; // 0x10
	datum owner_tag; // 0x18
	int16 owner_tag_section_offset; // 0x1c
	int16 unknown; // 0x1e
	int32 unknown_2; // 0x20
	tag_block<h2x_sbsp_decorators_cache_blocks_cache_block_data> cache_block_data; // 0x24
	int8 pad_2c[8];
};
ASSERT_STRUCT_SIZE(h2x_sbsp_decorators_cache_blocks, 0x34);

struct h2x_sbsp_decorators_groups
{
	int8 decorator_set_index; // 0x0
	int8 decorator_type; // 0x1
	int8 shader_index; // 0x2
	int8 compressed_radius; // 0x3
	int16 cluster_index; // 0x4
	int16 cache_block_index; // 0x6
	int16 decorator_start_index; // 0x8
	int16 decorator_count; // 0xa
	int16 vertex_start_offset; // 0xc
	int16 vertex_count; // 0xe
	int16 index_start_offset; // 0x10
	int16 index_count; // 0x12
	int32 compressed_bounding_center; // 0x14
};
ASSERT_STRUCT_SIZE(h2x_sbsp_decorators_groups, 0x18);

struct h2x_sbsp_decorators_cells
{
	int16 child_index_0; // 0x0
	int16 child_index_1; // 0x2
	int16 child_index_2; // 0x4
	int16 child_index_3; // 0x6
	int16 child_index_4; // 0x8
	int16 child_index_5; // 0xa
	int16 child_index_6; // 0xc
	int16 child_index_7; // 0xe
	int16 cache_block_index; // 0x10
	int16 group_count; // 0x12
	int32 group_start_index; // 0x14
};
ASSERT_STRUCT_SIZE(h2x_sbsp_decorators_cells, 0x18);

struct h2x_sbsp_decorators_decals
{
	int8 decorator_set_index; // 0x0
	int8 decorator_class; // 0x1
	int8 decorator_permutation; // 0x2
	int8 sprite_index; // 0x3
	real_point3d position; // 0x4
	real_vector3d left; // 0x10
	real_vector3d up; // 0x1c
	real_vector3d extents; // 0x28
	real_point3d previous_position; // 0x34
};
ASSERT_STRUCT_SIZE(h2x_sbsp_decorators_decals, 0x40);

struct h2x_sbsp_decorators
{
	real_point3d grid_origin; // 0x0
	int32 cell_count_per_dimension; // 0xc
	tag_block<h2x_sbsp_decorators_cache_blocks> cache_blocks; // 0x10
	tag_block<h2x_sbsp_decorators_groups> groups; // 0x18
	tag_block<h2x_sbsp_decorators_cells> cells; // 0x20
	tag_block<h2x_sbsp_decorators_decals> decals; // 0x28
};
ASSERT_STRUCT_SIZE(h2x_sbsp_decorators, 0x30);

struct h2x_sbsp
{
	uint32 import_info_block; // 0x0
	uint32 import_info_block_2; // 0x4
	int32 bsp_checksum; // 0x8
	tag_block<h2x_sbsp_collision_materials> collision_materials; // 0xc
	tag_block<h2x_sbsp_collision_bsp> collision_bsp; // 0x14
	real32 vehicle_floor; // 0x1c
	real32 vehicle_ceiling; // 0x20
	uint32 unused_nodes_block; // 0x24
	uint32 unused_nodes_block_2; // 0x28
	tag_block<h2x_sbsp_leaves> leaves; // 0x2c
	real_bounds world_bounds_x; // 0x34
	real_bounds world_bounds_y; // 0x3c
	real_bounds world_bounds_z; // 0x44
	tag_block<h2x_sbsp_surface_references> surface_references; // 0x4c
	tag_data cluster_data; // 0x54
	tag_block<h2x_sbsp_cluster_portals> cluster_portals; // 0x5c
	tag_block<h2x_sbsp_fog_planes> fog_planes; // 0x64
	int8 pad_6c[24];
	tag_block<h2x_sbsp_weather_palette> weather_palette; // 0x84
	tag_block<h2x_sbsp_weather_polyhedra> weather_polyhedra; // 0x8c
	tag_block<h2x_sbsp_detail_objects> detail_objects; // 0x94
	tag_block<h2x_sbsp_clusters> clusters; // 0x9c
	tag_block<h2x_sbsp_materials> materials; // 0xa4
	tag_block<h2x_sbsp_sky_owner_cluster> sky_owner_cluster; // 0xac
	tag_block<h2x_sbsp_conveyor_surfaces> conveyor_surfaces; // 0xb4
	tag_block<h2x_sbsp_breakable_surfaces> breakable_surfaces; // 0xbc
	tag_block<h2x_sbsp_pathfinding_data> pathfinding_data; // 0xc4
	tag_block<h2x_sbsp_pathfinding_edges> pathfinding_edges; // 0xcc
	tag_block<h2x_sbsp_background_sound_palette> background_sound_palette; // 0xd4
	tag_block<h2x_sbsp_sound_environment_palette> sound_environment_palette; // 0xdc
	tag_data sound_pas_data; // 0xe4
	tag_block<h2x_sbsp_markers> markers; // 0xec
	tag_block<h2x_sbsp_runtime_decals> runtime_decals; // 0xf4
	tag_block<h2x_sbsp_environment_object_palette> environment_object_palette; // 0xfc
	tag_block<h2x_sbsp_environment_objects> environment_objects; // 0x104
	uint32 lightmaps_block; // 0x10c
	uint32 lightmaps_block_2; // 0x110
	int8 pad_114[4];
	uint32 leaf_map_leaves_block; // 0x118
	uint32 leaf_map_leaves_block_2; // 0x11c
	uint32 leaf_map_connections_block; // 0x120
	uint32 leaf_map_connections_block_2; // 0x124
	uint32 errors_block; // 0x128
	uint32 errors_block_2; // 0x12c
	uint32 precomputed_lighting_block; // 0x130
	uint32 precomputed_lighting_block_2; // 0x134
	tag_block<h2x_sbsp_instanced_geometry_definitions> instanced_geometry_definitions; // 0x138
	tag_block<h2x_sbsp_instanced_geometry_instances> instanced_geometry_instances; // 0x140
	tag_block<h2x_sbsp_ambience_sound_clusters> ambience_sound_clusters; // 0x148
	tag_block<h2x_sbsp_reverb_sound_clusters> reverb_sound_clusters; // 0x150
	tag_block<h2x_sbsp_transparent_planes> transparent_planes; // 0x158
	int8 pad_160[96];
	real32 vehicle_spherical_limit_radius; // 0x1c0
	real_point3d vehicle_spherical_limit_center; // 0x1c4
	tag_block<h2x_sbsp_debug_info> debug_info; // 0x1d0
	tag_reference decorators; // 0x1d8
	tag_data mopp_codes; // 0x1e0
	int8 pad_1e8[4];
	real_point3d mopp_bounds_minimum; // 0x1ec
	real_point3d mopp_bounds_maximum; // 0x1f8
	tag_data breakable_surface_mopp_codes; // 0x204
	tag_block<h2x_sbsp_breakable_surface_key_table> breakable_surface_key_table; // 0x20c
	tag_block<h2x_sbsp_water_definitions> water_definitions; // 0x214
	tag_block<h2x_sbsp_portal_device_mapping> portal_device_mapping; // 0x21c
	tag_block<h2x_sbsp_audibility> audibility; // 0x224
	tag_block<h2x_sbsp_object_fake_lightprobes> object_fake_lightprobes; // 0x22c
	tag_block<h2x_sbsp_decorators> decorators_2; // 0x234
};
ASSERT_STRUCT_SIZE(h2x_sbsp, 0x23c);

struct h2x_ltmp_lightmap_groups_section_palette
{
	int32 first_palette_color; // 0x0
	int8 pad_4[1020];
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_section_palette, 0x400);

struct h2x_ltmp_lightmap_groups_writable_palettes
{
	int32 first_palette_color; // 0x0
	int8 pad_4[1020];
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_writable_palettes, 0x400);

struct h2x_ltmp_lightmap_groups_clusters_compression_info
{
	real_bounds position_bounds_x; // 0x0
	real_bounds position_bounds_y; // 0x8
	real_bounds position_bounds_z; // 0x10
	real_bounds texcoord_bounds_x; // 0x18
	real_bounds texcoord_bounds_y; // 0x20
	real_bounds secondary_texcoord_bounds_x; // 0x28
	real_bounds secondary_texcoord_bounds_y; // 0x30
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_clusters_compression_info, 0x38);

struct h2x_ltmp_lightmap_groups_clusters_resources
{
	int8 type; // 0x0
	int8 unknown; // 0x1
	int16 unknown_2; // 0x2
	int16 primary_locator; // 0x4
	int16 secondary_locator; // 0x6
	uint32 resource_data_size; // 0x8
	uint32 resource_data_offset; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_clusters_resources, 0x10);

struct h2x_ltmp_lightmap_groups_clusters_cache_data_parts
{
	int16 type; // 0x0
	uint16 flags; // 0x2
	int16 material_index; // 0x4
	int16 strip_start_index; // 0x6
	int16 strip_length; // 0x8
	int16 first_subpart_index; // 0xa
	int16 subpart_count; // 0xc
	int8 maximum_nodes_vertex; // 0xe
	int8 contributing_compound_node_count; // 0xf
	real_point3d position; // 0x10
	int8 node_index_0; // 0x1c
	int8 node_index_1; // 0x1d
	int8 node_index_2; // 0x1e
	int8 node_index_3; // 0x1f
	real32 node_weight_0; // 0x20
	real32 node_weight_1; // 0x24
	real32 node_weight_2; // 0x28
	real32 lod_mipmap_magic_number; // 0x2c
	int8 pad_30[24];
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_clusters_cache_data_parts, 0x48);

struct h2x_ltmp_lightmap_groups_clusters_cache_data_subparts
{
	int16 indices_start_index; // 0x0
	int16 indices_length; // 0x2
	int16 visibility_bounds_index; // 0x4
	int16 part_index; // 0x6
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_clusters_cache_data_subparts, 0x8);

struct h2x_ltmp_lightmap_groups_clusters_cache_data_visibility_bounds
{
	real_point3d position; // 0x0
	real32 radius; // 0xc
	int8 node_0; // 0x10
	int8 unknown; // 0x11
	int16 unknown_2; // 0x12
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_clusters_cache_data_visibility_bounds, 0x14);

struct h2x_ltmp_lightmap_groups_clusters_cache_data_raw_vertices
{
	real_point3d position; // 0x0
	int32 old_node_index_0; // 0xc
	int32 old_node_index_1; // 0x10
	int32 old_node_index_2; // 0x14
	int32 old_node_index_3; // 0x18
	real32 old_node_weight_0; // 0x1c
	real32 old_node_weight_1; // 0x20
	real32 old_node_weight_2; // 0x24
	real32 old_node_weight_3; // 0x28
	int32 new_node_index_0; // 0x2c
	int32 new_node_index_1; // 0x30
	int32 new_node_index_2; // 0x34
	int32 new_node_index_3; // 0x38
	int32 use_new_node_indices; // 0x3c
	int32 adjusted_compound_node_index; // 0x40
	real_point2d texcoord; // 0x44
	real_vector3d normal; // 0x4c
	real_vector3d binormal; // 0x58
	real_vector3d tangent; // 0x64
	real_vector3d anisotropic_binormal; // 0x70
	real_point2d secondary_texcoord; // 0x7c
	real_rgb_color primary_lightmap_color; // 0x84
	real_point2d primary_lightmap_texcoord; // 0x90
	real_vector3d primary_lightmap_incident_direction; // 0x98
	int8 pad_a4[32];
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_clusters_cache_data_raw_vertices, 0xc4);

struct h2x_ltmp_lightmap_groups_clusters_cache_data_strip_indices
{
	int16 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_clusters_cache_data_strip_indices, 0x2);

struct h2x_ltmp_lightmap_groups_clusters_cache_data_mopp_reorder_table
{
	int16 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_clusters_cache_data_mopp_reorder_table, 0x2);

struct h2x_ltmp_lightmap_groups_clusters_cache_data_vertex_buffers
{
	uint32 vertex_buffer; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_clusters_cache_data_vertex_buffers, 0x4);

struct h2x_ltmp_lightmap_groups_clusters_cache_data
{
	tag_block<h2x_ltmp_lightmap_groups_clusters_cache_data_parts> parts; // 0x0
	tag_block<h2x_ltmp_lightmap_groups_clusters_cache_data_subparts> subparts; // 0x8
	tag_block<h2x_ltmp_lightmap_groups_clusters_cache_data_visibility_bounds> visibility_bounds; // 0x10
	tag_block<h2x_ltmp_lightmap_groups_clusters_cache_data_raw_vertices> raw_vertices; // 0x18
	tag_block<h2x_ltmp_lightmap_groups_clusters_cache_data_strip_indices> strip_indices; // 0x20
	tag_data visibility_mopp_code; // 0x28
	tag_block<h2x_ltmp_lightmap_groups_clusters_cache_data_mopp_reorder_table> mopp_reorder_table; // 0x30
	tag_block<h2x_ltmp_lightmap_groups_clusters_cache_data_vertex_buffers> vertex_buffers; // 0x38
	int8 pad_40[4];
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_clusters_cache_data, 0x44);

struct h2x_ltmp_lightmap_groups_clusters
{
	uint16 total_vertex_count; // 0x0
	uint16 total_triangle_count; // 0x2
	uint16 total_part_count; // 0x4
	uint16 shadow_casting_triangle_count; // 0x6
	uint16 shadow_casting_part_count; // 0x8
	uint16 opaque_point_count; // 0xa
	uint16 opaque_vertex_count; // 0xc
	uint16 opaque_part_count; // 0xe
	uint8 opague_maximum_nodes_vertex; // 0x10
	uint8 transparent_maximum_nodes_vertex; // 0x11
	uint16 shadow_casting_rigid_triangle_count; // 0x12
	int16 geometry_classification; // 0x14
	uint16 geometry_compression_flags; // 0x16
	tag_block<h2x_ltmp_lightmap_groups_clusters_compression_info> compression_info; // 0x18
	int8 hardware_node_count; // 0x20
	int8 node_map_size; // 0x21
	int16 software_plane_count; // 0x22
	int16 total_subpart_count; // 0x24
	uint16 section_lighting_flags; // 0x26
	int32 resource_block_offset; // 0x28
	uint32 resource_block_size; // 0x2c
	uint32 section_data_size; // 0x30
	uint32 resource_data_size; // 0x34
	tag_block<h2x_ltmp_lightmap_groups_clusters_resources> resources; // 0x38
	datum owner_tag; // 0x40
	int16 owner_tag_section_offset; // 0x44
	int16 unknown; // 0x46
	int32 unknown_2; // 0x48
	tag_block<h2x_ltmp_lightmap_groups_clusters_cache_data> cache_data; // 0x4c
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_clusters, 0x54);

struct h2x_ltmp_lightmap_groups_cluster_render_info
{
	int16 bitmap_index; // 0x0
	int8 palette_index; // 0x2
	int8 unknown; // 0x3
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_cluster_render_info, 0x4);

struct h2x_ltmp_lightmap_groups_poop_definitions_compression_info
{
	real_bounds position_bounds_x; // 0x0
	real_bounds position_bounds_y; // 0x8
	real_bounds position_bounds_z; // 0x10
	real_bounds texcoord_bounds_x; // 0x18
	real_bounds texcoord_bounds_y; // 0x20
	real_bounds secondary_texcoord_bounds_x; // 0x28
	real_bounds secondary_texcoord_bounds_y; // 0x30
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_poop_definitions_compression_info, 0x38);

struct h2x_ltmp_lightmap_groups_poop_definitions_resources
{
	int8 type; // 0x0
	int8 unknown; // 0x1
	int16 unknown_2; // 0x2
	int16 primary_locator; // 0x4
	int16 secondary_locator; // 0x6
	uint32 resource_data_size; // 0x8
	uint32 resource_data_offset; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_poop_definitions_resources, 0x10);

struct h2x_ltmp_lightmap_groups_poop_definitions_cache_data_parts
{
	int16 type; // 0x0
	uint16 flags; // 0x2
	int16 material_index; // 0x4
	int16 strip_start_index; // 0x6
	int16 strip_length; // 0x8
	int16 first_subpart_index; // 0xa
	int16 subpart_count; // 0xc
	int8 maximum_nodes_vertex; // 0xe
	int8 contributing_compound_node_count; // 0xf
	real_point3d position; // 0x10
	int8 node_index_0; // 0x1c
	int8 node_index_1; // 0x1d
	int8 node_index_2; // 0x1e
	int8 node_index_3; // 0x1f
	real32 node_weight_0; // 0x20
	real32 node_weight_1; // 0x24
	real32 node_weight_2; // 0x28
	real32 lod_mipmap_magic_number; // 0x2c
	int8 pad_30[24];
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_poop_definitions_cache_data_parts, 0x48);

struct h2x_ltmp_lightmap_groups_poop_definitions_cache_data_subparts
{
	int16 indices_start_index; // 0x0
	int16 indices_length; // 0x2
	int16 visibility_bounds_index; // 0x4
	int16 part_index; // 0x6
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_poop_definitions_cache_data_subparts, 0x8);

struct h2x_ltmp_lightmap_groups_poop_definitions_cache_data_visibility_bounds
{
	real_point3d position; // 0x0
	real32 radius; // 0xc
	int8 node_0; // 0x10
	int8 unknown; // 0x11
	int16 unknown_2; // 0x12
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_poop_definitions_cache_data_visibility_bounds, 0x14);

struct h2x_ltmp_lightmap_groups_poop_definitions_cache_data_raw_vertices
{
	real_point3d position; // 0x0
	int32 old_node_index_0; // 0xc
	int32 old_node_index_1; // 0x10
	int32 old_node_index_2; // 0x14
	int32 old_node_index_3; // 0x18
	real32 old_node_weight_0; // 0x1c
	real32 old_node_weight_1; // 0x20
	real32 old_node_weight_2; // 0x24
	real32 old_node_weight_3; // 0x28
	int32 new_node_index_0; // 0x2c
	int32 new_node_index_1; // 0x30
	int32 new_node_index_2; // 0x34
	int32 new_node_index_3; // 0x38
	int32 use_new_node_indices; // 0x3c
	int32 adjusted_compound_node_index; // 0x40
	real_point2d texcoord; // 0x44
	real_vector3d normal; // 0x4c
	real_vector3d binormal; // 0x58
	real_vector3d tangent; // 0x64
	real_vector3d anisotropic_binormal; // 0x70
	real_point2d secondary_texcoord; // 0x7c
	real_rgb_color primary_lightmap_color; // 0x84
	real_point2d primary_lightmap_texcoord; // 0x90
	real_vector3d primary_lightmap_incident_direction; // 0x98
	int8 pad_a4[32];
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_poop_definitions_cache_data_raw_vertices, 0xc4);

struct h2x_ltmp_lightmap_groups_poop_definitions_cache_data_strip_indices
{
	int16 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_poop_definitions_cache_data_strip_indices, 0x2);

struct h2x_ltmp_lightmap_groups_poop_definitions_cache_data_mopp_reorder_table
{
	int16 index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_poop_definitions_cache_data_mopp_reorder_table, 0x2);

struct h2x_ltmp_lightmap_groups_poop_definitions_cache_data_vertex_buffers
{
	uint32 vertex_buffer; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_poop_definitions_cache_data_vertex_buffers, 0x4);

struct h2x_ltmp_lightmap_groups_poop_definitions_cache_data
{
	tag_block<h2x_ltmp_lightmap_groups_poop_definitions_cache_data_parts> parts; // 0x0
	tag_block<h2x_ltmp_lightmap_groups_poop_definitions_cache_data_subparts> subparts; // 0x8
	tag_block<h2x_ltmp_lightmap_groups_poop_definitions_cache_data_visibility_bounds> visibility_bounds; // 0x10
	tag_block<h2x_ltmp_lightmap_groups_poop_definitions_cache_data_raw_vertices> raw_vertices; // 0x18
	tag_block<h2x_ltmp_lightmap_groups_poop_definitions_cache_data_strip_indices> strip_indices; // 0x20
	tag_data visibility_mopp_code; // 0x28
	tag_block<h2x_ltmp_lightmap_groups_poop_definitions_cache_data_mopp_reorder_table> mopp_reorder_table; // 0x30
	tag_block<h2x_ltmp_lightmap_groups_poop_definitions_cache_data_vertex_buffers> vertex_buffers; // 0x38
	int8 pad_40[4];
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_poop_definitions_cache_data, 0x44);

struct h2x_ltmp_lightmap_groups_poop_definitions
{
	uint16 total_vertex_count; // 0x0
	uint16 total_triangle_count; // 0x2
	uint16 total_part_count; // 0x4
	uint16 shadow_casting_triangle_count; // 0x6
	uint16 shadow_casting_part_count; // 0x8
	uint16 opaque_point_count; // 0xa
	uint16 opaque_vertex_count; // 0xc
	uint16 opaque_part_count; // 0xe
	uint8 opague_maximum_nodes_vertex; // 0x10
	uint8 transparent_maximum_nodes_vertex; // 0x11
	uint16 shadow_casting_rigid_triangle_count; // 0x12
	int16 geometry_classification; // 0x14
	uint16 geometry_compression_flags; // 0x16
	tag_block<h2x_ltmp_lightmap_groups_poop_definitions_compression_info> compression_info; // 0x18
	int8 hardware_node_count; // 0x20
	int8 node_map_size; // 0x21
	int16 software_plane_count; // 0x22
	int16 total_subpart_count; // 0x24
	uint16 section_lighting_flags; // 0x26
	int32 resource_block_offset; // 0x28
	uint32 resource_block_size; // 0x2c
	uint32 section_data_size; // 0x30
	uint32 resource_data_size; // 0x34
	tag_block<h2x_ltmp_lightmap_groups_poop_definitions_resources> resources; // 0x38
	datum owner_tag; // 0x40
	int16 owner_tag_section_offset; // 0x44
	int16 unknown; // 0x46
	int32 unknown_2; // 0x48
	tag_block<h2x_ltmp_lightmap_groups_poop_definitions_cache_data> cache_data; // 0x4c
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_poop_definitions, 0x54);

struct h2x_ltmp_lightmap_groups_lighting_environments
{
	real_point3d sample_point; // 0x0
	real32 red_coefficient; // 0xc
	real32 red_coefficient_2; // 0x10
	real32 red_coefficient_3; // 0x14
	real32 red_coefficient_4; // 0x18
	real32 red_coefficient_5; // 0x1c
	real32 red_coefficient_6; // 0x20
	real32 red_coefficient_7; // 0x24
	real32 red_coefficient_8; // 0x28
	real32 red_coefficient_9; // 0x2c
	real32 green_coefficient; // 0x30
	real32 green_coefficient_2; // 0x34
	real32 green_coefficient_3; // 0x38
	real32 green_coefficient_4; // 0x3c
	real32 green_coefficient_5; // 0x40
	real32 green_coefficient_6; // 0x44
	real32 green_coefficient_7; // 0x48
	real32 green_coefficient_8; // 0x4c
	real32 green_coefficient_9; // 0x50
	real32 blue_coefficient; // 0x54
	real32 blue_coefficient_2; // 0x58
	real32 blue_coefficient_3; // 0x5c
	real32 blue_coefficient_4; // 0x60
	real32 blue_coefficient_5; // 0x64
	real32 blue_coefficient_6; // 0x68
	real32 blue_coefficient_7; // 0x6c
	real32 blue_coefficient_8; // 0x70
	real32 blue_coefficient_9; // 0x74
	real_vector3d mean_incoming_light_direction; // 0x78
	real_point3d incoming_light_intensity; // 0x84
	int32 specular_bitmap_index; // 0x90
	real_vector3d rotation_axis; // 0x94
	real32 rotation_speed; // 0xa0
	real_vector3d bump_direction; // 0xa4
	real_rgb_color color_tint; // 0xb0
	int16 procedural_override; // 0xbc
	uint16 flags; // 0xbe
	real_vector3d procedural_parameter_0; // 0xc0
	real_vector3d procedural_parameter_1; // 0xcc
	real32 procedural_parameter_w; // 0xd8
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_lighting_environments, 0xdc);

struct h2x_ltmp_lightmap_groups_geometry_buckets_resources
{
	int8 type; // 0x0
	int8 unknown; // 0x1
	int16 unknown_2; // 0x2
	int16 primary_locator; // 0x4
	int16 secondary_locator; // 0x6
	uint32 resource_data_size; // 0x8
	uint32 resource_data_offset; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_geometry_buckets_resources, 0x10);

struct h2x_ltmp_lightmap_groups_geometry_buckets
{
	uint16 flags; // 0x0
	int16 unknown; // 0x2
	int8 pad_4[8];
	int32 resource_block_offset; // 0xc
	uint32 resource_block_size; // 0x10
	uint32 section_data_size; // 0x14
	uint32 resource_data_size; // 0x18
	tag_block<h2x_ltmp_lightmap_groups_geometry_buckets_resources> resources; // 0x1c
	datum owner_tag; // 0x24
	int16 owner_tag_section_offset; // 0x28
	int16 unknown_2; // 0x2a
	int32 unknown_3; // 0x2c
	uint32 cache_data_block; // 0x30
	uint32 cache_data_block_2; // 0x34
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_geometry_buckets, 0x38);

struct h2x_ltmp_lightmap_groups_instance_render_info
{
	int16 bitmap_index; // 0x0
	int8 palette_index; // 0x2
	int8 unknown; // 0x3
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_instance_render_info, 0x4);

struct h2x_ltmp_lightmap_groups_instance_bucket_refs_section_offsets
{
	int16 section_offset; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_instance_bucket_refs_section_offsets, 0x2);

struct h2x_ltmp_lightmap_groups_instance_bucket_refs
{
	int16 flags; // 0x0
	int16 bucket_index; // 0x2
	tag_block<h2x_ltmp_lightmap_groups_instance_bucket_refs_section_offsets> section_offsets; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_instance_bucket_refs, 0xc);

struct h2x_ltmp_lightmap_groups_scenery_object_info
{
	datum unique_id; // 0x0
	int16 origin_bsp_index; // 0x4
	int8 type; // 0x6
	int8 source; // 0x7
	int32 render_model_checksum; // 0x8
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_scenery_object_info, 0xc);

struct h2x_ltmp_lightmap_groups_scenery_object_bucket_refs_section_offsets
{
	int16 section_offset; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_scenery_object_bucket_refs_section_offsets, 0x2);

struct h2x_ltmp_lightmap_groups_scenery_object_bucket_refs
{
	int16 flags; // 0x0
	int16 bucket_index; // 0x2
	tag_block<h2x_ltmp_lightmap_groups_scenery_object_bucket_refs_section_offsets> section_offsets; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups_scenery_object_bucket_refs, 0xc);

struct h2x_ltmp_lightmap_groups
{
	int16 type; // 0x0
	uint16 flags; // 0x2
	int32 structure_checksum; // 0x4
	tag_block<h2x_ltmp_lightmap_groups_section_palette> section_palette; // 0x8
	tag_block<h2x_ltmp_lightmap_groups_writable_palettes> writable_palettes; // 0x10
	tag_reference bitmap_group; // 0x18
	tag_block<h2x_ltmp_lightmap_groups_clusters> clusters; // 0x20
	tag_block<h2x_ltmp_lightmap_groups_cluster_render_info> cluster_render_info; // 0x28
	tag_block<h2x_ltmp_lightmap_groups_poop_definitions> poop_definitions; // 0x30
	tag_block<h2x_ltmp_lightmap_groups_lighting_environments> lighting_environments; // 0x38
	tag_block<h2x_ltmp_lightmap_groups_geometry_buckets> geometry_buckets; // 0x40
	tag_block<h2x_ltmp_lightmap_groups_instance_render_info> instance_render_info; // 0x48
	tag_block<h2x_ltmp_lightmap_groups_instance_bucket_refs> instance_bucket_refs; // 0x50
	tag_block<h2x_ltmp_lightmap_groups_scenery_object_info> scenery_object_info; // 0x58
	tag_block<h2x_ltmp_lightmap_groups_scenery_object_bucket_refs> scenery_object_bucket_refs; // 0x60
};
ASSERT_STRUCT_SIZE(h2x_ltmp_lightmap_groups, 0x68);

struct h2x_ltmp
{
	real32 search_distance_lower_bound; // 0x0
	real32 search_distance_upper_bound; // 0x4
	real32 luminels_per_world_unit; // 0x8
	real32 output_white_reference; // 0xc
	real32 output_black_reference; // 0x10
	real32 output_schlick_reference; // 0x14
	real32 diffuse_map_scale; // 0x18
	real32 prt_sun_scale; // 0x1c
	real32 prt_sky_scale; // 0x20
	real32 prt_indirect_scale; // 0x24
	real32 prt_scale; // 0x28
	real32 prt_surface_light_scale; // 0x2c
	real32 prt_scenario_light_scale; // 0x30
	real32 lightprobe_interpolation_override; // 0x34
	int8 pad_38[72];
	tag_block<h2x_ltmp_lightmap_groups> lightmap_groups; // 0x80
	int8 pad_88[12];
	uint32 errors_block; // 0x94
	uint32 errors_block_2; // 0x98
	int8 pad_9c[104];
};
ASSERT_STRUCT_SIZE(h2x_ltmp, 0x104);

struct h2x_bitm_sequences_sprites
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
ASSERT_STRUCT_SIZE(h2x_bitm_sequences_sprites, 0x20);

struct h2x_bitm_sequences
{
	char name[32]; // 0x0
	int16 first_bitmap_index; // 0x20
	int16 bitmap_count; // 0x22
	int8 pad_24[16];
	tag_block<h2x_bitm_sequences_sprites> sprites; // 0x34
};
ASSERT_STRUCT_SIZE(h2x_bitm_sequences, 0x3c);

struct h2x_bitm_bitmaps
{
	char signature[4]; // 0x0
	int16 width; // 0x4
	int16 height; // 0x6
	int8 depth; // 0x8
	uint8 more_flags; // 0x9
	int16 type; // 0xa
	int16 format; // 0xc
	uint16 flags; // 0xe
	point2d registration_point; // 0x10
	int16 mipmap_count; // 0x14
	int8 lod_adjust; // 0x16
	int8 cache_usage; // 0x17
	int32 pixels_offset; // 0x18
	int32 lod1_offset; // 0x1c
	int32 lod2_offset; // 0x20
	int32 lod3_offset; // 0x24
	int32 unknown; // 0x28
	int32 unknown_2; // 0x2c
	int32 unknown_3; // 0x30
	int32 lod1_size; // 0x34
	int32 lod2_size; // 0x38
	int32 lod3_size; // 0x3c
	int8 pad_40[12];
	tag_reference f_datum; // 0x4c
	int8 pad_54[12];
	int32 low_detail_offset; // 0x60
	int32 low_detail_size; // 0x64
	int16 low_detail_format; // 0x68
	int16 low_detail_width; // 0x6a
	int16 low_detail_height; // 0x6c
	int16 low_detail_depth; // 0x6e
	int8 pad_70[4];
};
ASSERT_STRUCT_SIZE(h2x_bitm_bitmaps, 0x74);

struct h2x_bitm
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
	int8 pad_1c[16];
	real32 blur_filter_size; // 0x2c
	real32 alpha_bias; // 0x30
	int16 mipmap_count; // 0x34
	int16 sprite_usage; // 0x36
	int16 sprite_spacing; // 0x38
	int16 force_format; // 0x3a
	tag_block<h2x_bitm_sequences> sequences; // 0x3c
	tag_block<h2x_bitm_bitmaps> bitmaps; // 0x44
	int8 color_compression_quality; // 0x4c
	int8 alpha_compression_quality; // 0x4d
	int8 overlap; // 0x4e
	int8 color_subsampling; // 0x4f
};
ASSERT_STRUCT_SIZE(h2x_bitm, 0x50);

struct h2x_shad_runtime_properties
{
	tag_reference diffuse_map; // 0x0
	tag_reference lightmap_emissive_map; // 0x8
	real_rgb_color lightmap_emissive_color; // 0x10
	real32 lightmap_emissive_power; // 0x1c
	real32 lightmap_resolution_scale; // 0x20
	real32 lightmap_half_life; // 0x24
	real32 lightmap_diffuse_scale; // 0x28
	tag_reference alpha_test_map; // 0x2c
	tag_reference translucent_map; // 0x34
	real_rgb_color lightmap_transparent_color; // 0x3c
	real32 lightmap_transparent_alpha; // 0x48
	real32 lightmap_foliage_scale; // 0x4c
};
ASSERT_STRUCT_SIZE(h2x_shad_runtime_properties, 0x50);

struct h2x_shad_parameters_animation_properties
{
	int16 type; // 0x0
	int16 unknown; // 0x2
	string_id input_name; // 0x4
	string_id range_name; // 0x8
	real32 time_period; // 0xc
	tag_data animation_function; // 0x10
};
ASSERT_STRUCT_SIZE(h2x_shad_parameters_animation_properties, 0x18);

struct h2x_shad_parameters
{
	string_id name; // 0x0
	int16 type; // 0x4
	int16 unknown; // 0x6
	tag_reference bitmap; // 0x8
	real32 const_value; // 0x10
	real_rgb_color const_color; // 0x14
	tag_block<h2x_shad_parameters_animation_properties> animation_properties; // 0x20
};
ASSERT_STRUCT_SIZE(h2x_shad_parameters, 0x28);

struct h2x_shad_postprocess_definition_bitmaps
{
	datum bitmap_group; // 0x0
	int32 bitmap_index; // 0x4
	real32 log_bitmap_dimension; // 0x8
};
ASSERT_STRUCT_SIZE(h2x_shad_postprocess_definition_bitmaps, 0xc);

struct h2x_shad_postprocess_definition_pixel_constants
{
	uint32 color; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_shad_postprocess_definition_pixel_constants, 0x4);

struct h2x_shad_postprocess_definition_vertex_constants
{
	real_vector3d vector; // 0x0
	real32 w; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_shad_postprocess_definition_vertex_constants, 0x10);

struct h2x_shad_postprocess_definition_levels_of_detail
{
	int32 available_layer_flags; // 0x0
	int16 layers_block_index_data; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_shad_postprocess_definition_levels_of_detail, 0x6);

struct h2x_shad_postprocess_definition_layers
{
	int16 indices_block_index_data; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_shad_postprocess_definition_layers, 0x2);

struct h2x_shad_postprocess_definition_passes
{
	int16 indices_block_index_data; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_shad_postprocess_definition_passes, 0x2);

struct h2x_shad_postprocess_definition_implementations
{
	int16 bitmap_transforms_block_index_data; // 0x0
	int16 render_states_block_index_data; // 0x2
	int16 texture_states_block_index_data; // 0x4
	int16 pixel_constants_block_index_data; // 0x6
	int16 vertex_constants_block_index_data; // 0x8
};
ASSERT_STRUCT_SIZE(h2x_shad_postprocess_definition_implementations, 0xa);

struct h2x_shad_postprocess_definition_overlays
{
	string_id input_name; // 0x0
	string_id range_name; // 0x4
	real32 time_period_in_seconds; // 0x8
	tag_data function; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_shad_postprocess_definition_overlays, 0x14);

struct h2x_shad_postprocess_definition_overlay_references
{
	int16 overlay_index; // 0x0
	int16 transform_index; // 0x2
};
ASSERT_STRUCT_SIZE(h2x_shad_postprocess_definition_overlay_references, 0x4);

struct h2x_shad_postprocess_definition_animated_parameters
{
	int16 overlay_references_block_index_data; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_shad_postprocess_definition_animated_parameters, 0x2);

struct h2x_shad_postprocess_definition_animated_parameter_references
{
	int16 unknown; // 0x0
	int8 unknown_2; // 0x2
	int8 parameter_index; // 0x3
};
ASSERT_STRUCT_SIZE(h2x_shad_postprocess_definition_animated_parameter_references, 0x4);

struct h2x_shad_postprocess_definition_bitmap_properties
{
	int16 bitmap_index; // 0x0
	int16 animated_parameter_index; // 0x2
};
ASSERT_STRUCT_SIZE(h2x_shad_postprocess_definition_bitmap_properties, 0x4);

struct h2x_shad_postprocess_definition_color_properties
{
	real_rgb_color color; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_shad_postprocess_definition_color_properties, 0xc);

struct h2x_shad_postprocess_definition_value_properties
{
	real32 value; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_shad_postprocess_definition_value_properties, 0x4);

struct h2x_shad_postprocess_definition
{
	datum shader_template; // 0x0
	tag_block<h2x_shad_postprocess_definition_bitmaps> bitmaps; // 0x4
	tag_block<h2x_shad_postprocess_definition_pixel_constants> pixel_constants; // 0xc
	tag_block<h2x_shad_postprocess_definition_vertex_constants> vertex_constants; // 0x14
	tag_block<h2x_shad_postprocess_definition_levels_of_detail> levels_of_detail; // 0x1c
	tag_block<h2x_shad_postprocess_definition_layers> layers; // 0x24
	tag_block<h2x_shad_postprocess_definition_passes> passes; // 0x2c
	tag_block<h2x_shad_postprocess_definition_implementations> implementations; // 0x34
	tag_block<h2x_shad_postprocess_definition_overlays> overlays; // 0x3c
	tag_block<h2x_shad_postprocess_definition_overlay_references> overlay_references; // 0x44
	tag_block<h2x_shad_postprocess_definition_animated_parameters> animated_parameters; // 0x4c
	tag_block<h2x_shad_postprocess_definition_animated_parameter_references> animated_parameter_references; // 0x54
	tag_block<h2x_shad_postprocess_definition_bitmap_properties> bitmap_properties; // 0x5c
	tag_block<h2x_shad_postprocess_definition_color_properties> color_properties; // 0x64
	tag_block<h2x_shad_postprocess_definition_value_properties> value_properties; // 0x6c
	int8 pad_74[8];
};
ASSERT_STRUCT_SIZE(h2x_shad_postprocess_definition, 0x7c);

struct h2x_shad_predicted_resources
{
	int16 type; // 0x0
	int16 resource_index; // 0x2
	datum tag_index; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_shad_predicted_resources, 0x8);

struct h2x_shad_postprocess_properties
{
	int32 bitmap_group_index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_shad_postprocess_properties, 0x4);

struct h2x_shad
{
	tag_reference f_template; // 0x0
	string_id global_material_name; // 0x8
	tag_block<h2x_shad_runtime_properties> runtime_properties; // 0xc
	int16 type; // 0x14
	uint16 flags; // 0x16
	tag_block<h2x_shad_parameters> parameters; // 0x18
	tag_block<h2x_shad_postprocess_definition> postprocess_definition; // 0x20
	int8 pad_28[4];
	tag_block<h2x_shad_predicted_resources> predicted_resources; // 0x2c
	tag_reference light_response; // 0x34
	int16 shader_lod_bias; // 0x3c
	int16 specular_type; // 0x3e
	int16 lightmap_type; // 0x40
	int16 unknown; // 0x42
	real32 lightmap_specular_brightness; // 0x44
	real32 lightmap_ambient_bias; // 0x48
	tag_block<h2x_shad_postprocess_properties> postprocess_properties; // 0x4c
	real32 added_depth_bias_offset; // 0x54
	real32 added_depth_bias_slope_scale; // 0x58
};
ASSERT_STRUCT_SIZE(h2x_shad, 0x5c);

struct h2x_mode_compression_info
{
	real_bounds position_bounds_x; // 0x0
	real_bounds position_bounds_y; // 0x8
	real_bounds position_bounds_z; // 0x10
	real_bounds texcoord_bounds_x; // 0x18
	real_bounds texcoord_bounds_y; // 0x20
	real_bounds secondary_texcoord_bounds_x; // 0x28
	real_bounds secondary_texcoord_bounds_y; // 0x30
};
ASSERT_STRUCT_SIZE(h2x_mode_compression_info, 0x38);

struct h2x_mode_regions_permutations
{
	string_id name; // 0x0
	int16 l1_section_index_super_low; // 0x4
	int16 l2_section_index_low; // 0x6
	int16 l3_section_index_medium; // 0x8
	int16 l4_section_index_high; // 0xa
	int16 l5_section_index_super_high; // 0xc
	int16 l6_section_index_hollywood; // 0xe
};
ASSERT_STRUCT_SIZE(h2x_mode_regions_permutations, 0x10);

struct h2x_mode_regions
{
	string_id name; // 0x0
	int16 old_node_map_offset; // 0x4
	int16 old_node_map_size; // 0x6
	tag_block<h2x_mode_regions_permutations> permutations; // 0x8
};
ASSERT_STRUCT_SIZE(h2x_mode_regions, 0x10);

struct h2x_mode_sections_compression_info
{
	real_bounds position_bounds_x; // 0x0
	real_bounds position_bounds_y; // 0x8
	real_bounds position_bounds_z; // 0x10
	real_bounds texcoord_bounds_x; // 0x18
	real_bounds texcoord_bounds_y; // 0x20
	real_bounds secondary_texcoord_bounds_x; // 0x28
	real_bounds secondary_texcoord_bounds_y; // 0x30
};
ASSERT_STRUCT_SIZE(h2x_mode_sections_compression_info, 0x38);

struct h2x_mode_sections_resources
{
	int8 type; // 0x0
	int8 unknown; // 0x1
	int16 unknown_2; // 0x2
	int16 primary_locator; // 0x4
	int16 secondary_locator; // 0x6
	uint32 resource_data_size; // 0x8
	uint32 resource_data_offset; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_mode_sections_resources, 0x10);

struct h2x_mode_sections
{
	int16 global_geometry_classification; // 0x0
	int16 unknown; // 0x2
	uint16 total_vertex_count; // 0x4
	uint16 total_triangle_count; // 0x6
	uint16 total_part_count; // 0x8
	uint16 shadow_casting_triangle_count; // 0xa
	uint16 shadow_casting_part_count; // 0xc
	uint16 opaque_point_count; // 0xe
	uint16 opaque_vertex_count; // 0x10
	uint16 opaque_part_count; // 0x12
	uint8 opaque_maximum_nodes_vertex; // 0x14
	uint8 transparent_maximum_nodes_vertex; // 0x15
	uint16 shadow_casting_rigid_triangle_count; // 0x16
	int16 geometry_classification; // 0x18
	uint16 geometry_compression_flags; // 0x1a
	tag_block<h2x_mode_sections_compression_info> compression_info; // 0x1c
	uint8 hardware_node_count; // 0x24
	uint8 node_map_size; // 0x25
	uint16 software_plane_count; // 0x26
	uint16 total_subpart_count; // 0x28
	uint16 section_lighting_flags; // 0x2a
	int16 rigid_node_index; // 0x2c
	uint16 flags; // 0x2e
	uint32 section_data_block; // 0x30
	uint32 section_data_block_2; // 0x34
	int32 resource_block_offset; // 0x38
	uint32 resource_block_size; // 0x3c
	uint32 section_data_size; // 0x40
	uint32 resource_data_size; // 0x44
	tag_block<h2x_mode_sections_resources> resources; // 0x48
	datum owner_tag; // 0x50
	int16 owner_tag_section_offset; // 0x54
	int16 unknown_2; // 0x56
	int32 unknown_3; // 0x58
};
ASSERT_STRUCT_SIZE(h2x_mode_sections, 0x5c);

struct h2x_mode_invalid_section_pair_bits
{
	int32 bits; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_mode_invalid_section_pair_bits, 0x4);

struct h2x_mode_section_groups_compound_nodes
{
	int8 node_index_0; // 0x0
	int8 node_index_1; // 0x1
	int8 node_index_2; // 0x2
	int8 node_index_3; // 0x3
	real32 node_weight_0; // 0x4
	real32 node_weight_1; // 0x8
	real32 node_weight_2; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_mode_section_groups_compound_nodes, 0x10);

struct h2x_mode_section_groups
{
	uint16 detail_levels; // 0x0
	int16 unknown; // 0x2
	tag_block<h2x_mode_section_groups_compound_nodes> compound_nodes; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_mode_section_groups, 0xc);

struct h2x_mode_nodes
{
	string_id name; // 0x0
	int16 parent_node_index; // 0x4
	int16 first_child_node_index; // 0x6
	int16 next_sibling_node_index; // 0x8
	int16 import_node_index; // 0xa
	real_point3d default_translation; // 0xc
	real_quaternion default_rotation; // 0x18
	real32 inverse_scale; // 0x28
	real_vector3d inverse_forward; // 0x2c
	real_vector3d inverse_left; // 0x38
	real_vector3d inverse_up; // 0x44
	real_point3d inverse_position; // 0x50
	real32 distance_from_parent; // 0x5c
};
ASSERT_STRUCT_SIZE(h2x_mode_nodes, 0x60);

struct h2x_mode_old_node_map
{
	int8 node_index; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_mode_old_node_map, 0x1);

struct h2x_mode_marker_groups_markers
{
	int8 region_index; // 0x0
	int8 permutation_index; // 0x1
	int8 node_index; // 0x2
	int8 unknown; // 0x3
	real_point3d translation; // 0x4
	real_quaternion rotation; // 0x10
	real32 scale; // 0x20
};
ASSERT_STRUCT_SIZE(h2x_mode_marker_groups_markers, 0x24);

struct h2x_mode_marker_groups
{
	string_id name; // 0x0
	tag_block<h2x_mode_marker_groups_markers> markers; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_mode_marker_groups, 0xc);

struct h2x_mode_materials_properties
{
	int16 type; // 0x0
	int16 integer_value; // 0x2
	real32 real_value; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_mode_materials_properties, 0x8);

struct h2x_mode_materials
{
	tag_reference old_shader; // 0x0
	tag_reference shader; // 0x8
	tag_block<h2x_mode_materials_properties> properties; // 0x10
	int8 pad_18[4];
	int8 breakable_surface_index; // 0x1c
	int8 unknown; // 0x1d
	int16 unknown_2; // 0x1e
};
ASSERT_STRUCT_SIZE(h2x_mode_materials, 0x20);

struct h2x_mode_prt_info_lod_info_section_info
{
	int32 section_index; // 0x0
	uint32 pca_data_offset; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_mode_prt_info_lod_info_section_info, 0x8);

struct h2x_mode_prt_info_lod_info
{
	uint32 cluster_offset; // 0x0
	tag_block<h2x_mode_prt_info_lod_info_section_info> section_info; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_mode_prt_info_lod_info, 0xc);

struct h2x_mode_prt_info_cluster_basis
{
	real32 basis_data; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_mode_prt_info_cluster_basis, 0x4);

struct h2x_mode_prt_info_resources
{
	int8 type; // 0x0
	int8 unknown; // 0x1
	int16 unknown_2; // 0x2
	int16 primary_locator; // 0x4
	int16 secondary_locator; // 0x6
	uint32 resource_data_size; // 0x8
	uint32 resource_data_offset; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_mode_prt_info_resources, 0x10);

struct h2x_mode_prt_info
{
	uint16 sh_order; // 0x0
	uint16 number_of_clusters; // 0x2
	uint16 pca_vectors_per_cluster; // 0x4
	uint16 number_of_rays; // 0x6
	uint16 number_of_bounces; // 0x8
	uint16 material_index_for_sbsfc_scattering; // 0xa
	real32 length_scale; // 0xc
	uint16 number_of_lods_in_model; // 0x10
	uint16 unknown; // 0x12
	tag_block<h2x_mode_prt_info_lod_info> lod_info; // 0x14
	tag_block<h2x_mode_prt_info_cluster_basis> cluster_basis; // 0x1c
	uint32 raw_pca_data_block; // 0x24
	uint32 raw_pca_data_block_2; // 0x28
	uint32 vertex_buffers_block; // 0x2c
	uint32 vertex_buffers_block_2; // 0x30
	int32 resource_block_offset; // 0x34
	uint32 resource_block_size; // 0x38
	uint32 section_data_size; // 0x3c
	uint32 resource_data_size; // 0x40
	tag_block<h2x_mode_prt_info_resources> resources; // 0x44
	datum owner_tag; // 0x4c
	int16 owner_tag_section_offset; // 0x50
	int16 unknown_2; // 0x52
	int32 unknown_3; // 0x54
};
ASSERT_STRUCT_SIZE(h2x_mode_prt_info, 0x58);

struct h2x_mode_section_render_leaves_node_render_leaves_collision_leaves
{
	int16 cluster; // 0x0
	int16 surface_reference_count; // 0x2
	int32 first_surface_reference_index; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_mode_section_render_leaves_node_render_leaves_collision_leaves, 0x8);

struct h2x_mode_section_render_leaves_node_render_leaves_surface_references
{
	int16 strip_index; // 0x0
	int16 lightmap_triangle_index; // 0x2
	int32 bsp_node_index; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_mode_section_render_leaves_node_render_leaves_surface_references, 0x8);

struct h2x_mode_section_render_leaves_node_render_leaves
{
	tag_block<h2x_mode_section_render_leaves_node_render_leaves_collision_leaves> collision_leaves; // 0x0
	tag_block<h2x_mode_section_render_leaves_node_render_leaves_surface_references> surface_references; // 0x8
};
ASSERT_STRUCT_SIZE(h2x_mode_section_render_leaves_node_render_leaves, 0x10);

struct h2x_mode_section_render_leaves
{
	tag_block<h2x_mode_section_render_leaves_node_render_leaves> node_render_leaves; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_mode_section_render_leaves, 0x8);

struct h2x_mode
{
	string_id name; // 0x0
	uint16 flags; // 0x4
	int16 unknown; // 0x6
	int32 model_checksum; // 0x8
	uint32 import_info_block; // 0xc
	uint32 import_info_block_2; // 0x10
	tag_block<h2x_mode_compression_info> compression_info; // 0x14
	tag_block<h2x_mode_regions> regions; // 0x1c
	tag_block<h2x_mode_sections> sections; // 0x24
	tag_block<h2x_mode_invalid_section_pair_bits> invalid_section_pair_bits; // 0x2c
	tag_block<h2x_mode_section_groups> section_groups; // 0x34
	int8 l1_section_group_index_super_low; // 0x3c
	int8 l2_section_group_index_low; // 0x3d
	int8 l3_section_group_index_medium; // 0x3e
	int8 l4_section_group_index_high; // 0x3f
	int8 l5_section_group_index_super_high; // 0x40
	int8 l6_section_group_index_hollywood; // 0x41
	int16 unknown_2; // 0x42
	int32 node_list_checksum; // 0x44
	tag_block<h2x_mode_nodes> nodes; // 0x48
	tag_block<h2x_mode_old_node_map> old_node_map; // 0x50
	tag_block<h2x_mode_marker_groups> marker_groups; // 0x58
	tag_block<h2x_mode_materials> materials; // 0x60
	uint32 errors_block; // 0x68
	uint32 errors_block_2; // 0x6c
	real32 don_t_draw_over_camera_cosine_angle; // 0x70
	tag_block<h2x_mode_prt_info> prt_info; // 0x74
	tag_block<h2x_mode_section_render_leaves> section_render_leaves; // 0x7c
};
ASSERT_STRUCT_SIZE(h2x_mode, 0x84);

struct h2x_hlmt_variants_regions_permutations_states
{
	string_id permutation_name; // 0x0
	int8 runtime_model_permutation_index; // 0x4
	uint8 property_flags; // 0x5
	int16 state; // 0x6
	tag_reference looping_effect; // 0x8
	string_id looping_effect_marker_name; // 0x10
	real32 initial_probability; // 0x14
};
ASSERT_STRUCT_SIZE(h2x_hlmt_variants_regions_permutations_states, 0x18);

struct h2x_hlmt_variants_regions_permutations
{
	string_id permutation_name; // 0x0
	int8 runtime_model_permutation_index; // 0x4
	uint8 flags; // 0x5
	int16 unknown; // 0x6
	real32 probability; // 0x8
	tag_block<h2x_hlmt_variants_regions_permutations_states> states; // 0xc
	int8 runtime_state_permutation_index_0; // 0x14
	int8 runtime_state_permutation_index_1; // 0x15
	int8 runtime_state_permutation_index_2; // 0x16
	int8 runtime_state_permutation_index_3; // 0x17
	int8 runtime_state_permutation_index_4; // 0x18
	int8 unknown_2; // 0x19
	int16 unknown_3; // 0x1a
	int8 pad_1c[4];
};
ASSERT_STRUCT_SIZE(h2x_hlmt_variants_regions_permutations, 0x20);

struct h2x_hlmt_variants_regions
{
	string_id region_name; // 0x0
	int8 runtime_model_region_index; // 0x4
	uint8 runtime_flags; // 0x5
	int16 parent_variant_index; // 0x6
	tag_block<h2x_hlmt_variants_regions_permutations> permutations; // 0x8
	int16 sort_order; // 0x10
	int16 unknown; // 0x12
};
ASSERT_STRUCT_SIZE(h2x_hlmt_variants_regions, 0x14);

struct h2x_hlmt_variants_objects
{
	string_id parent_marker; // 0x0
	string_id child_marker; // 0x4
	tag_reference child_object; // 0x8
};
ASSERT_STRUCT_SIZE(h2x_hlmt_variants_objects, 0x10);

struct h2x_hlmt_variants
{
	string_id name; // 0x0
	int8 runtime_model_region_0_index; // 0x4
	int8 runtime_model_region_1_index; // 0x5
	int8 runtime_model_region_2_index; // 0x6
	int8 runtime_model_region_3_index; // 0x7
	int8 runtime_model_region_4_index; // 0x8
	int8 runtime_model_region_5_index; // 0x9
	int8 runtime_model_region_6_index; // 0xa
	int8 runtime_model_region_7_index; // 0xb
	int8 runtime_model_region_8_index; // 0xc
	int8 runtime_model_region_9_index; // 0xd
	int8 runtime_model_region_10_index; // 0xe
	int8 runtime_model_region_11_index; // 0xf
	int8 runtime_model_region_12_index; // 0x10
	int8 runtime_model_region_13_index; // 0x11
	int8 runtime_model_region_14_index; // 0x12
	int8 runtime_model_region_15_index; // 0x13
	tag_block<h2x_hlmt_variants_regions> regions; // 0x14
	tag_block<h2x_hlmt_variants_objects> objects; // 0x1c
	int8 pad_24[8];
	string_id dialogue_sound_effect; // 0x2c
	tag_reference dialogue; // 0x30
};
ASSERT_STRUCT_SIZE(h2x_hlmt_variants, 0x38);

struct h2x_hlmt_materials
{
	string_id material_name; // 0x0
	int16 material_type; // 0x4
	int16 damage_section_index; // 0x6
	int16 collision_global_material_index; // 0x8
	int16 damage_global_material_index; // 0xa
	string_id global_material_name; // 0xc
	int16 global_material_index; // 0x10
	int16 unknown; // 0x12
};
ASSERT_STRUCT_SIZE(h2x_hlmt_materials, 0x14);

struct h2x_hlmt_new_damage_info_damage_sections_instant_responses
{
	int16 response_type; // 0x0
	int16 constraint_damage_type; // 0x2
	uint32 flags; // 0x4
	real32 damage_threshold; // 0x8
	tag_reference transition_effect; // 0xc
	tag_reference transition_damage_effect; // 0x14
	string_id region; // 0x1c
	int16 new_state; // 0x20
	int16 runtime_region_index; // 0x22
	string_id effect_marker_name; // 0x24
	string_id damage_effect_marker_name; // 0x28
	real32 response_delay; // 0x2c
	tag_reference delay_effect; // 0x30
	string_id delay_effect_marker_name; // 0x38
	string_id constraint_group_name; // 0x3c
	string_id ejecting_seat_label; // 0x40
	real32 skip_fraction; // 0x44
	string_id destroyed_child_object_marker_name; // 0x48
	real32 total_damage_threshold; // 0x4c
};
ASSERT_STRUCT_SIZE(h2x_hlmt_new_damage_info_damage_sections_instant_responses, 0x50);

struct h2x_hlmt_new_damage_info_damage_sections
{
	string_id name; // 0x0
	uint32 flags; // 0x4
	real32 vitality_percentage; // 0x8
	tag_block<h2x_hlmt_new_damage_info_damage_sections_instant_responses> instant_responses; // 0xc
	uint32 null_block_1; // 0x14
	uint32 null_block_1_2; // 0x18
	uint32 null_block_2; // 0x1c
	uint32 null_block_2_2; // 0x20
	real32 stun_time; // 0x24
	real32 recharge_time; // 0x28
	real32 runtime_recharge_velocity; // 0x2c
	string_id resurrection_restored_region_name; // 0x30
	int16 ressurection_region_runtime_index; // 0x34
	int16 unknown; // 0x36
};
ASSERT_STRUCT_SIZE(h2x_hlmt_new_damage_info_damage_sections, 0x38);

struct h2x_hlmt_new_damage_info_nodes
{
	int16 runtime_damage_part; // 0x0
	int16 unknown; // 0x2
	int8 pad_4[12];
};
ASSERT_STRUCT_SIZE(h2x_hlmt_new_damage_info_nodes, 0x10);

struct h2x_hlmt_new_damage_info_damage_seats
{
	string_id seat_label; // 0x0
	real32 direct_damage_scale; // 0x4
	real32 damage_transfer_fall_off_radius; // 0x8
	real32 maximum_transfer_damage_scale; // 0xc
	real32 minimum_transfer_damage_scale; // 0x10
};
ASSERT_STRUCT_SIZE(h2x_hlmt_new_damage_info_damage_seats, 0x14);

struct h2x_hlmt_new_damage_info_damage_constraints
{
	string_id physics_model_constraint_name; // 0x0
	string_id damage_constraint_name; // 0x4
	string_id damage_constraint_group_name; // 0x8
	real32 group_probability_scale; // 0xc
	int16 constraint_type; // 0x10
	int16 constraint_index; // 0x12
};
ASSERT_STRUCT_SIZE(h2x_hlmt_new_damage_info_damage_constraints, 0x14);

struct h2x_hlmt_new_damage_info
{
	uint32 flags; // 0x0
	string_id indirect_global_material_name; // 0x4
	int16 indirect_damage_section_index; // 0x8
	int16 unknown; // 0xa
	int8 pad_c[4];
	int8 collision_damage_reporting_type; // 0x10
	int8 response_damage_reporting_type; // 0x11
	int16 unknown_2; // 0x12
	int8 pad_14[20];
	real32 maximum_vitality; // 0x28
	real32 minimum_stun_damage; // 0x2c
	real32 stun_time; // 0x30
	real32 recharge_time; // 0x34
	real32 recharge_fraction; // 0x38
	int8 pad_3c[64];
	tag_reference shield_damaged_first_person_shader; // 0x7c
	tag_reference shield_damaged_shader; // 0x84
	real32 maximum_shield_vitality; // 0x8c
	string_id shield_global_material_name; // 0x90
	real32 minimum_stun_damage_2; // 0x94
	real32 stun_time_2; // 0x98
	real32 recharge_time_2; // 0x9c
	real32 shield_damaged_threshold; // 0xa0
	tag_reference shield_damaged_effect; // 0xa4
	tag_reference shield_depleted_effect; // 0xac
	tag_reference shield_recharging_effect; // 0xb4
	tag_block<h2x_hlmt_new_damage_info_damage_sections> damage_sections; // 0xbc
	tag_block<h2x_hlmt_new_damage_info_nodes> nodes; // 0xc4
	int16 shield_global_material_index; // 0xcc
	int16 indirect_global_material_index; // 0xce
	real32 shield_recharge_velocity; // 0xd0
	real32 health_recharge_velocity; // 0xd4
	tag_block<h2x_hlmt_new_damage_info_damage_seats> damage_seats; // 0xd8
	tag_block<h2x_hlmt_new_damage_info_damage_constraints> damage_constraints; // 0xe0
	tag_reference overshield_first_person_shader; // 0xe8
	tag_reference overshield_shader; // 0xf0
};
ASSERT_STRUCT_SIZE(h2x_hlmt_new_damage_info, 0xf8);

struct h2x_hlmt_targets
{
	string_id marker_name; // 0x0
	real32 size; // 0x4
	real32 cone_angle; // 0x8
	int16 damage_section_index; // 0xc
	int16 variant_index; // 0xe
	real32 targeting_relevance; // 0x10
	uint32 lock_on_flags; // 0x14
	real32 lock_on_distance; // 0x18
};
ASSERT_STRUCT_SIZE(h2x_hlmt_targets, 0x1c);

struct h2x_hlmt_collision_regions_permutations
{
	string_id name; // 0x0
	uint8 flags; // 0x4
	int8 collision_permutation_index; // 0x5
	int8 physics_permutation_index; // 0x6
	int8 unknown; // 0x7
};
ASSERT_STRUCT_SIZE(h2x_hlmt_collision_regions_permutations, 0x8);

struct h2x_hlmt_collision_regions
{
	string_id name; // 0x0
	int8 collision_region_index; // 0x4
	int8 physics_region_index; // 0x5
	int16 unknown; // 0x6
	tag_block<h2x_hlmt_collision_regions_permutations> permutations; // 0x8
};
ASSERT_STRUCT_SIZE(h2x_hlmt_collision_regions, 0x10);

struct h2x_hlmt_nodes
{
	string_id name; // 0x0
	int16 parent_node_index; // 0x4
	int16 first_child_node_index; // 0x6
	int16 next_sibling_node_index; // 0x8
	int16 unknown; // 0xa
	real_point3d default_translation; // 0xc
	real_quaternion default_rotation; // 0x18
	real32 default_inverse_scale; // 0x28
	real_vector3d default_inverse_forward; // 0x2c
	real_vector3d default_inverse_left; // 0x38
	real_vector3d default_inverse_up; // 0x44
	real_point3d default_inverse_position; // 0x50
};
ASSERT_STRUCT_SIZE(h2x_hlmt_nodes, 0x5c);

struct h2x_hlmt_model_object_data
{
	int16 type; // 0x0
	int16 unknown; // 0x2
	real_point3d offset; // 0x4
	real32 radius; // 0x10
};
ASSERT_STRUCT_SIZE(h2x_hlmt_model_object_data, 0x14);

struct h2x_hlmt_scenario_load_parameters
{
	tag_reference scenario; // 0x0
	tag_data parameters; // 0x8
	int8 pad_10[32];
};
ASSERT_STRUCT_SIZE(h2x_hlmt_scenario_load_parameters, 0x30);

struct h2x_hlmt
{
	tag_reference render_model; // 0x0
	tag_reference collision_model; // 0x8
	tag_reference animation; // 0x10
	tag_reference physics; // 0x18
	tag_reference physics_model; // 0x20
	real32 disappear_distance; // 0x28
	real32 begin_fade_distance; // 0x2c
	int8 pad_30[4];
	real32 reduce_to_l1; // 0x34
	real32 reduce_to_l2; // 0x38
	real32 reduce_to_l3; // 0x3c
	real32 reduce_to_l4; // 0x40
	real32 reduce_to_l5; // 0x44
	int8 pad_48[4];
	int16 shadow_fade_distance; // 0x4c
	int16 unknown; // 0x4e
	tag_block<h2x_hlmt_variants> variants; // 0x50
	tag_block<h2x_hlmt_materials> materials; // 0x58
	tag_block<h2x_hlmt_new_damage_info> new_damage_info; // 0x60
	tag_block<h2x_hlmt_targets> targets; // 0x68
	tag_block<h2x_hlmt_collision_regions> collision_regions; // 0x70
	tag_block<h2x_hlmt_nodes> nodes; // 0x78
	uint32 node_list_checksum; // 0x80
	tag_block<h2x_hlmt_model_object_data> model_object_data; // 0x84
	tag_reference default_dialogue; // 0x8c
	tag_reference active_camo_shader; // 0x94
	uint32 flags; // 0x9c
	string_id default_dialogue_effect; // 0xa0
	uint32 render_only_node_flags_1; // 0xa4
	uint32 render_only_node_flags_2; // 0xa8
	uint32 render_only_node_flags_3; // 0xac
	uint32 render_only_node_flags_4; // 0xb0
	uint32 render_only_node_flags_5; // 0xb4
	uint32 render_only_node_flags_6; // 0xb8
	uint32 render_only_node_flags_7; // 0xbc
	uint32 render_only_node_flags_8; // 0xc0
	uint32 render_only_section_flags_1; // 0xc4
	uint32 render_only_section_flags_2; // 0xc8
	uint32 render_only_section_flags_3; // 0xcc
	uint32 render_only_section_flags_4; // 0xd0
	uint32 render_only_section_flags_5; // 0xd4
	uint32 render_only_section_flags_6; // 0xd8
	uint32 render_only_section_flags_7; // 0xdc
	uint32 render_only_section_flags_8; // 0xe0
	uint32 runtime_flags; // 0xe4
	tag_block<h2x_hlmt_scenario_load_parameters> scenario_load_parameters; // 0xe8
	tag_reference hologram_shader; // 0xf0
	string_id hologram_control_function; // 0xf8
};
ASSERT_STRUCT_SIZE(h2x_hlmt, 0xfc);

struct h2x_coll_materials
{
	string_id name; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_coll_materials, 0x4);

struct h2x_coll_regions_permutations_bsps_bsp_3d_nodes
{
	int16 plane; // 0x0
	uint8 front_child_lower; // 0x2
	uint8 front_child_mid; // 0x3
	uint8 front_child_upper; // 0x4
	uint8 back_child_lower; // 0x5
	uint8 back_child_mid; // 0x6
	uint8 back_child_upper; // 0x7
};
ASSERT_STRUCT_SIZE(h2x_coll_regions_permutations_bsps_bsp_3d_nodes, 0x8);

struct h2x_coll_regions_permutations_bsps_planes
{
	real_plane3d plane; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_coll_regions_permutations_bsps_planes, 0x10);

struct h2x_coll_regions_permutations_bsps_leaves
{
	uint8 flags; // 0x0
	uint8 bsp_2d_reference_count; // 0x1
	int16 first_bsp2d_reference; // 0x2
};
ASSERT_STRUCT_SIZE(h2x_coll_regions_permutations_bsps_leaves, 0x4);

struct h2x_coll_regions_permutations_bsps_bsp_2d_references
{
	int16 plane; // 0x0
	int16 bsp_2d_node; // 0x2
};
ASSERT_STRUCT_SIZE(h2x_coll_regions_permutations_bsps_bsp_2d_references, 0x4);

struct h2x_coll_regions_permutations_bsps_bsp_2d_nodes
{
	real_plane2d plane; // 0x0
	int16 left_child; // 0xc
	int16 right_child; // 0xe
};
ASSERT_STRUCT_SIZE(h2x_coll_regions_permutations_bsps_bsp_2d_nodes, 0x10);

struct h2x_coll_regions_permutations_bsps_surfaces
{
	int16 plane; // 0x0
	int16 first_edge; // 0x2
	uint8 flags; // 0x4
	uint8 breakable_surface; // 0x5
	int16 material; // 0x6
};
ASSERT_STRUCT_SIZE(h2x_coll_regions_permutations_bsps_surfaces, 0x8);

struct h2x_coll_regions_permutations_bsps_edges
{
	int16 start_vertex; // 0x0
	int16 end_vertex; // 0x2
	int16 forward_edge; // 0x4
	int16 reverse_edge; // 0x6
	int16 left_surface; // 0x8
	int16 right_surface; // 0xa
};
ASSERT_STRUCT_SIZE(h2x_coll_regions_permutations_bsps_edges, 0xc);

struct h2x_coll_regions_permutations_bsps_vertices
{
	real_point3d point; // 0x0
	int16 first_edge; // 0xc
	int16 unknown; // 0xe
};
ASSERT_STRUCT_SIZE(h2x_coll_regions_permutations_bsps_vertices, 0x10);

struct h2x_coll_regions_permutations_bsps
{
	int16 node_index; // 0x0
	int16 unknown; // 0x2
	tag_block<h2x_coll_regions_permutations_bsps_bsp_3d_nodes> bsp_3d_nodes; // 0x4
	tag_block<h2x_coll_regions_permutations_bsps_planes> planes; // 0xc
	tag_block<h2x_coll_regions_permutations_bsps_leaves> leaves; // 0x14
	tag_block<h2x_coll_regions_permutations_bsps_bsp_2d_references> bsp_2d_references; // 0x1c
	tag_block<h2x_coll_regions_permutations_bsps_bsp_2d_nodes> bsp_2d_nodes; // 0x24
	tag_block<h2x_coll_regions_permutations_bsps_surfaces> surfaces; // 0x2c
	tag_block<h2x_coll_regions_permutations_bsps_edges> edges; // 0x34
	tag_block<h2x_coll_regions_permutations_bsps_vertices> vertices; // 0x3c
};
ASSERT_STRUCT_SIZE(h2x_coll_regions_permutations_bsps, 0x44);

struct h2x_coll_regions_permutations_bsp_physics
{
	uint32 runtime_code_pointer; // 0x0
	int16 size; // 0x4
	int16 count; // 0x6
	uint32 user_data; // 0x8
	uint32 unknown; // 0xc
	real_vector3d center; // 0x10
	real32 w_center; // 0x1c
	real_vector3d half_extent; // 0x20
	real32 w_half_extent; // 0x2c
	tag_reference runtime_model_tag; // 0x30
	int8 pad_38[8];
	uint32 runtime_code_pointer_2; // 0x40
	int16 size_2; // 0x44
	int16 count_2; // 0x46
	uint32 user_data_2; // 0x48
	uint32 unknown_2; // 0x4c
	uint32 runtime_code_pointer_3; // 0x50
	int16 size_3; // 0x54
	int16 count_3; // 0x56
	uint32 user_data_3; // 0x58
	uint32 unknown_3; // 0x5c
	uint32 unknown_4; // 0x60
	tag_data mopp_code_data; // 0x64
	int8 pad_6c[8];
};
ASSERT_STRUCT_SIZE(h2x_coll_regions_permutations_bsp_physics, 0x74);

struct h2x_coll_regions_permutations
{
	string_id name; // 0x0
	tag_block<h2x_coll_regions_permutations_bsps> bsps; // 0x4
	tag_block<h2x_coll_regions_permutations_bsp_physics> bsp_physics; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_coll_regions_permutations, 0x14);

struct h2x_coll_regions
{
	string_id name; // 0x0
	tag_block<h2x_coll_regions_permutations> permutations; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_coll_regions, 0xc);

struct h2x_coll_pathfinding_spheres
{
	int16 node_index; // 0x0
	uint16 flags; // 0x2
	real_point3d center; // 0x4
	real32 radius; // 0x10
};
ASSERT_STRUCT_SIZE(h2x_coll_pathfinding_spheres, 0x14);

struct h2x_coll_nodes
{
	string_id name; // 0x0
	int16 unknown; // 0x4
	int16 parent_node_index; // 0x6
	int16 next_sibling_node_index; // 0x8
	int16 first_child_node_index; // 0xa
};
ASSERT_STRUCT_SIZE(h2x_coll_nodes, 0xc);

struct h2x_coll
{
	uint32 import_info_block; // 0x0
	uint32 import_info_block_2; // 0x4
	uint32 errors_block; // 0x8
	uint32 errors_block_2; // 0xc
	uint32 flags; // 0x10
	tag_block<h2x_coll_materials> materials; // 0x14
	tag_block<h2x_coll_regions> regions; // 0x1c
	tag_block<h2x_coll_pathfinding_spheres> pathfinding_spheres; // 0x24
	tag_block<h2x_coll_nodes> nodes; // 0x2c
};
ASSERT_STRUCT_SIZE(h2x_coll, 0x34);

struct h2x_scen_ai_properties
{
	uint32 flags; // 0x0
	string_id ai_type_name; // 0x4
	int8 pad_8[4];
	int16 ai_size; // 0xc
	int16 leap_jump_speed; // 0xe
};
ASSERT_STRUCT_SIZE(h2x_scen_ai_properties, 0x10);

struct h2x_scen_functions
{
	uint32 flags; // 0x0
	string_id import_name; // 0x4
	string_id export_name; // 0x8
	string_id turn_off_with; // 0xc
	real32 minimum_value; // 0x10
	tag_data default_function; // 0x14
	string_id scale_by; // 0x1c
};
ASSERT_STRUCT_SIZE(h2x_scen_functions, 0x20);

struct h2x_scen_attachments
{
	tag_reference type; // 0x0
	string_id marker; // 0x8
	int16 change_color; // 0xc
	int16 unknown; // 0xe
	string_id primary_scale; // 0x10
	string_id secondary_scale; // 0x14
};
ASSERT_STRUCT_SIZE(h2x_scen_attachments, 0x18);

struct h2x_scen_widgets
{
	tag_reference type; // 0x0
};
ASSERT_STRUCT_SIZE(h2x_scen_widgets, 0x8);

struct h2x_scen_old_functions
{
	int8 pad_0[76];
	string_id unknown; // 0x4c
};
ASSERT_STRUCT_SIZE(h2x_scen_old_functions, 0x50);

struct h2x_scen_change_colors_initial_permutations
{
	real32 weight; // 0x0
	real_rgb_color color_lower_bound; // 0x4
	real_rgb_color color_upper_bound; // 0x10
	string_id variant_name; // 0x1c
};
ASSERT_STRUCT_SIZE(h2x_scen_change_colors_initial_permutations, 0x20);

struct h2x_scen_change_colors_functions
{
	int8 pad_0[4];
	uint32 scale_flags; // 0x4
	real_rgb_color color_lower_bound; // 0x8
	real_rgb_color color_upper_bound; // 0x14
	string_id darken_by; // 0x20
	string_id scale_by; // 0x24
};
ASSERT_STRUCT_SIZE(h2x_scen_change_colors_functions, 0x28);

struct h2x_scen_change_colors
{
	tag_block<h2x_scen_change_colors_initial_permutations> initial_permutations; // 0x0
	tag_block<h2x_scen_change_colors_functions> functions; // 0x8
};
ASSERT_STRUCT_SIZE(h2x_scen_change_colors, 0x10);

struct h2x_scen_predicted_resources
{
	int16 type; // 0x0
	int16 resource_index; // 0x2
	datum tag_index; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_scen_predicted_resources, 0x8);

struct h2x_scen
{
	int16 object_type; // 0x0
	uint16 flags; // 0x2
	real32 bounding_radius; // 0x4
	real_point3d bounding_offset; // 0x8
	real32 acceleration_scale; // 0x14
	int16 lightmap_shadow_mode; // 0x18
	int8 sweetener_size; // 0x1a
	int8 unknown; // 0x1b
	uint32 runtime_flags; // 0x1c
	real32 dynamic_light_sphere_radius; // 0x20
	real_point3d dynamic_light_sphere_offset; // 0x24
	string_id default_model_variant; // 0x30
	tag_reference model; // 0x34
	tag_reference crate_object; // 0x3c
	tag_reference modifier_shader; // 0x44
	tag_reference creation_effect; // 0x4c
	tag_reference material_effects; // 0x54
	tag_block<h2x_scen_ai_properties> ai_properties; // 0x5c
	tag_block<h2x_scen_functions> functions; // 0x64
	real32 apply_collision_damage_scale; // 0x6c
	real_bounds game_acceleration; // 0x70
	real_bounds game_scale; // 0x78
	real_bounds absolute_acceleration; // 0x80
	real_bounds absolute_scale; // 0x88
	int16 hud_text_message_index; // 0x90
	int16 unknown_2; // 0x92
	tag_block<h2x_scen_attachments> attachments; // 0x94
	tag_block<h2x_scen_widgets> widgets; // 0x9c
	tag_block<h2x_scen_old_functions> old_functions; // 0xa4
	tag_block<h2x_scen_change_colors> change_colors; // 0xac
	tag_block<h2x_scen_predicted_resources> predicted_resources; // 0xb4
	int16 pathfinding_policy; // 0xbc
	uint16 flags_2; // 0xbe
	int16 lightmapping_policy; // 0xc0
	int16 unknown_3; // 0xc2
};
ASSERT_STRUCT_SIZE(h2x_scen, 0xc4);

struct h2x_sky_cube_map
{
	tag_reference cube_map_reference; // 0x0
	real32 power_scale; // 0x8
};
ASSERT_STRUCT_SIZE(h2x_sky_cube_map, 0xc);

struct h2x_sky_atmospheric_fog
{
	real_rgb_color color; // 0x0
	real32 maximum_density; // 0xc
	real32 start_distance; // 0x10
	real32 opaque_distance; // 0x14
};
ASSERT_STRUCT_SIZE(h2x_sky_atmospheric_fog, 0x18);

struct h2x_sky_secondary_fog
{
	real_rgb_color color; // 0x0
	real32 maximum_density; // 0xc
	real32 start_distance; // 0x10
	real32 opaque_distance; // 0x14
};
ASSERT_STRUCT_SIZE(h2x_sky_secondary_fog, 0x18);

struct h2x_sky_sky_fog
{
	real_rgb_color color; // 0x0
	real32 density; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_sky_sky_fog, 0x10);

struct h2x_sky_patchy_fog
{
	real_rgb_color color; // 0x0
	int8 pad_c[12];
	real_bounds density; // 0x18
	real_bounds distance; // 0x20
	int8 pad_28[32];
	tag_reference patchy_fog; // 0x48
};
ASSERT_STRUCT_SIZE(h2x_sky_patchy_fog, 0x50);

struct h2x_sky_lights_fog
{
	real_rgb_color color; // 0x0
	real32 maximum_density; // 0xc
	real32 start_distance; // 0x10
	real32 opaque_distance; // 0x14
	real_bounds cone; // 0x18
	real32 atmospheric_fog_influence; // 0x20
	real32 secondary_fog_influence; // 0x24
	real32 sky_fog_influence; // 0x28
};
ASSERT_STRUCT_SIZE(h2x_sky_lights_fog, 0x2c);

struct h2x_sky_lights_fog_opposite
{
	real_rgb_color color; // 0x0
	real32 maximum_density; // 0xc
	real32 start_distance; // 0x10
	real32 opaque_distance; // 0x14
	real_bounds cone; // 0x18
	real32 atmospheric_fog_influence; // 0x20
	real32 secondary_fog_influence; // 0x24
	real32 sky_fog_influence; // 0x28
};
ASSERT_STRUCT_SIZE(h2x_sky_lights_fog_opposite, 0x2c);

struct h2x_sky_lights_radiosity
{
	uint32 flags; // 0x0
	real_rgb_color color; // 0x4
	real32 power; // 0x10
	real32 test_distance; // 0x14
	int8 pad_18[12];
	real32 diameter; // 0x24
};
ASSERT_STRUCT_SIZE(h2x_sky_lights_radiosity, 0x28);

struct h2x_sky_lights
{
	real_vector3d direction_vector; // 0x0
	real_euler_angles2d direction; // 0xc
	tag_reference lens_flare; // 0x14
	tag_block<h2x_sky_lights_fog> fog; // 0x1c
	tag_block<h2x_sky_lights_fog_opposite> fog_opposite; // 0x24
	tag_block<h2x_sky_lights_radiosity> radiosity; // 0x2c
};
ASSERT_STRUCT_SIZE(h2x_sky_lights, 0x34);

struct h2x_sky_shader_functions
{
	int8 pad_0[4];
	char global_function_name[32]; // 0x4
};
ASSERT_STRUCT_SIZE(h2x_sky_shader_functions, 0x24);

struct h2x_sky_animations
{
	int16 animation_index; // 0x0
	int16 unknown; // 0x2
	real32 period; // 0x4
	int8 pad_8[28];
};
ASSERT_STRUCT_SIZE(h2x_sky_animations, 0x24);

struct h2x_sky
{
	tag_reference render_model; // 0x0
	tag_reference animation_graph; // 0x8
	uint32 flags; // 0x10
	real32 render_model_scale; // 0x14
	real32 movement_scale; // 0x18
	tag_block<h2x_sky_cube_map> cube_map; // 0x1c
	real_rgb_color indoor_ambient_color; // 0x24
	int8 pad_30[4];
	real_rgb_color outdoor_ambient_color; // 0x34
	int8 pad_40[4];
	real32 fog_spread_distance; // 0x44
	tag_block<h2x_sky_atmospheric_fog> atmospheric_fog; // 0x48
	tag_block<h2x_sky_secondary_fog> secondary_fog; // 0x50
	tag_block<h2x_sky_sky_fog> sky_fog; // 0x58
	tag_block<h2x_sky_patchy_fog> patchy_fog; // 0x60
	real32 bloom_override_amount; // 0x68
	real32 bloom_override_threshold; // 0x6c
	real32 bloom_override_brightness; // 0x70
	real32 bloom_override_gamma_power; // 0x74
	tag_block<h2x_sky_lights> lights; // 0x78
	real32 global_sky_rotation; // 0x80
	tag_block<h2x_sky_shader_functions> shader_functions; // 0x84
	tag_block<h2x_sky_animations> animations; // 0x8c
	int8 pad_94[12];
	real_rgb_color clear_color; // 0xa0
};
ASSERT_STRUCT_SIZE(h2x_sky, 0xac);

struct h2x_itmc_item_permutations
{
	real32 weight; // 0x0
	tag_reference item; // 0x4
	string_id variant_name; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_itmc_item_permutations, 0x10);

struct h2x_itmc
{
	tag_block<h2x_itmc_item_permutations> item_permutations; // 0x0
	int16 spawn_time; // 0x8
	int16 unknown; // 0xa
};
ASSERT_STRUCT_SIZE(h2x_itmc, 0xc);

struct h2x_vehc_vehicle_permutations
{
	real32 weight; // 0x0
	tag_reference vehicle; // 0x4
	string_id variant_name; // 0xc
};
ASSERT_STRUCT_SIZE(h2x_vehc_vehicle_permutations, 0x10);

struct h2x_vehc
{
	tag_block<h2x_vehc_vehicle_permutations> vehicle_permutations; // 0x0
	int16 spawn_time; // 0x8
	int16 unknown; // 0xa
};
ASSERT_STRUCT_SIZE(h2x_vehc, 0xc);

#pragma pack(pop)

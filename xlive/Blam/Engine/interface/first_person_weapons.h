#pragma once
#include "animations/animation_manager.h"

/* constants */

enum
{
	MAXIMUM_NUMBER_OF_FIRST_PERSON_MODELS = 4,
	MAXIMUM_NODES_PER_FIRST_PERSON_MODEL = 64,
};

/* enums */

enum e_first_person_weapons
{
	k_first_person_primary_weapon = 0,
	k_first_person_secondary_weapon,
	k_first_person_max_weapons,
};

enum e_first_person_weapon_flags
{
	_first_person_facing_valid_bit = 0,
	_first_person_node_orientations_valid_bit,
	_first_person_attachment_suspended_bit,
	_first_person_attachment_pending_bit,
	_first_person_lowering_weapon_bit,
	_first_person_raising_weapon_bit,
	_first_person_weapon_lowered_bit,
	_first_person_waiting_for_custom_animation_bit,
	_first_person_waiting_for_scripting_bit,
	_first_person_waiting_for_claw_bit,
};

enum e_first_person_weapon_data_flags : uint32
{
	_weapon_node_table_valid_bit = 1,
	_arm_node_table_valid_bit = 2,
};

/* structures */

struct s_first_person_model_data
{
	datum render_model_index;
	datum object_index;
	uint32 flags;
	real_matrix4x3 nodes[MAXIMUM_NODES_PER_FIRST_PERSON_MODEL];
};

struct first_person_weapon_data
{
	/*e_first_person_weapon_data_flags*/ uint32 flags;
	datum weapon_index;
	c_animation_manager animation_manager;
	int8 gap_90[12];	// unused? maybe a real_point3d?
	c_animation_channel move_animation_channel;
	c_animation_channel jitter_animation_channel;
	c_animation_id pitch_turn_animation_id;
	c_animation_id overlay_animation_id;
	c_animation_id ammo_animation_id;
	real32 field_E4;
	real32 field_E8;
	int16 field_EC;
	int16 maybe_framerate;
	int16 field_F0;
	int16 field_F2;
	int32 weapon_node_remapping_table_count;
	int32 hands_node_remapping_table_count;
	int32 weapon_node_remapping_table[MAXIMUM_NODES_PER_FIRST_PERSON_MODEL];
	int32 hands_node_remapping_table[MAXIMUM_NODES_PER_FIRST_PERSON_MODEL];
	int16 child_node_index;
	int16 alternate_parent_node_index;
	int32 node_orientations_count;
	int32 node_matrices_count;
	real_matrix4x3 nodes[MAXIMUM_NODES_PER_FIRST_PERSON_MODEL];
	datum sound_source_index;
	int16 field_1004;
	int16 pad;
};
ASSERT_STRUCT_SIZE(first_person_weapon_data, 4112);

struct first_person_weapon
{
	uint32 flags; /*e_first_person_weapon_flags*/
	datum unit_index;
	int8 character_type; /*e_character_type*/
	int8 pad[3];
	first_person_weapon_data weapon[k_first_person_max_weapons];
	c_interpolator_control rate_interpolator_control;
	real_vector2d position;
	real_vector2d position_velocity;
	real_vector2d turn;
	real_vector2d turning_velocity;
	real_euler_angles2d facing_angles;
	real_euler_angles2d facing_angles_delta;
	real_matrix4x3 estimated_root_matrix; // probably not identity matrix
	int32 adjustment_matrix_index;
	real_matrix4x3 adjustment_matrix;
};
ASSERT_STRUCT_SIZE(first_person_weapon, 8396);
ASSERT_STRUCT_OFFSET(first_person_weapon, estimated_root_matrix, 8288);

struct s_first_person_orientations
{
	real_orientation weapon_orientations[MAXIMUM_NODES_PER_FIRST_PERSON_MODEL];
	real_orientation hand_orientations[MAXIMUM_NODES_PER_FIRST_PERSON_MODEL];
};
ASSERT_STRUCT_SIZE(s_first_person_orientations, 4096);

/* prototypes */

void first_person_weapons_apply_patches(void);

// the action label the user's first person primary weapon animates
string_id first_person_weapon_action_get(int32 user_index);

s_first_person_model_data* first_person_model_data_get(uint32 user_index);

void first_person_weapons_toggle(bool state);

void __cdecl first_person_weapons_update(void);

int32 __cdecl first_person_weapon_build_models(int32 user_index, datum unit_index, int32 maximum_model_count, s_first_person_model_data* fp_model_data);

void first_person_weapon_apply_camera_effect(int32 user_index, real_matrix4x3* effect_matrix);

real_matrix4x3* first_person_weapon_get_relative_node_matrix(int32 user_index, datum weapon_index, int16 node_index);

real_matrix4x3* first_person_weapon_get_relative_node_matrix_interpolated(int32 user_index, datum weapon_index, int16 node_index);

void __cdecl first_person_weapon_get_worldspace_node_matrix(int32 user_index, datum weapon_index, int16 node_index, real_matrix4x3* out_matrix);

void __cdecl first_person_weapon_get_worldspace_node_matrix_interpolated(int32 user_index, datum weapon_index, int16 node_index, real_matrix4x3* out_matrix);

#include "stdafx.h"
#include "h1_bipeds.h"

#include "h1_animations.h"
#include "h1_cache_file.h"
#include "h1_log.h"
#include "h1_object_tags.h"
#include "h1_objects.h"
#include "h1_runtime.h"
#include "h2_tag_definitions_generated.h"

#include "game/game_globals.h"
#include "scenario/scenario.h"
#include "tag_files/tag_groups.h"

/*
* Halo 2 bipeds built from halo 1 bipeds. The halo 1 model (nodes, markers, collision), its animation graph converted (units'
* seats, weapon classes and aiming screens) and the halo 1 unit and biped values; halo 2 simulates and animates the biped and
* the halo 1 renderer draws its model. The fields halo 1 has no counterpart for (seats, camera tracks, physics shapes, contact
* points, melee responses) come from the host map's player biped.
*/

/* ---------- prototypes */

static datum h1_biped_template_get(void);
static string_id h1_biped_node_name(const h1_mode* h1_model, int16 node_index, const char* fallback);

/* ---------- public code */

datum h1_biped_definition_build(datum h1_biped_index)
{
	const char* h1_name = g_h1_cache_file->tag_name_get(h1_biped_index);
	char name[256];
	sprintf_s(name, "halo1\\%s", h1_name);
	const datum existing = h1_runtime_tag_find('bipd', name);
	if (existing != NONE)
	{
		return existing;
	}

	const h1_bipd* h1_biped = (const h1_bipd*)g_h1_cache_file->tag_get('bipd', h1_biped_index);
	const h1_mode* h1_model = h1_biped ? (const h1_mode*)g_h1_cache_file->tag_get('mode', h1_biped->model.index) : NULL;
	const h1_coll* h1_collision = h1_biped ? (const h1_coll*)g_h1_cache_file->tag_get('coll', h1_biped->collision_model.index) : NULL;
	const datum template_index = h1_biped_template_get();
	const h2x_bipd* template_biped = template_index != NONE ? (const h2x_bipd*)tag_get('bipd', template_index) : NULL;
	if (!h1_model || !template_biped)
	{
		h1_log("bipeds: %s is missing its model or the host's biped", h1_name);
		return NONE;
	}

	s_h1_object_tags tags;
	tags.render_model = h1_object_render_model_build(h1_model, name);
	tags.collision_model = h1_collision ? h1_object_collision_model_build(h1_collision, h1_model, name) : NONE;
	tags.physics_model = NONE;
	tags.animation_graph = h1_animation_graph_build(h1_biped->animation_graph.index, h1_model, name);
	tags.disappear_distance = 200.f;
	const datum model_index = tags.render_model != NONE ? h1_object_model_build(&tags, h1_model, h1_collision, name) : NONE;
	if (model_index == NONE || tags.animation_graph == NONE)
	{
		return NONE;
	}

	h2x_bipd* biped = NULL;
	const datum biped_index = h1_runtime_tag_new('bipd', name, &biped);
	if (biped_index == NONE)
	{
		return NONE;
	}
	*biped = *template_biped;

	// object: halo 2's attachments, widgets, functions and change colors drive halo 2 models, the halo 1 renderer runs halo 1's
	biped->bounding_radius = h1_biped->bounding_radius;
	biped->bounding_offset = h1_biped->bounding_offset;
	biped->acceleration_scale = h1_biped->acceleration_scale;
	biped->default_model_variant = _string_id_default;
	h1_runtime_reference_set(&biped->model, 'hlmt', model_index);
	h1_runtime_reference_set(&biped->crate_object, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&biped->modifier_shader, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&biped->creation_effect, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&biped->material_effects, (tag_group)NONE, NONE);
	biped->ai_properties.count = 0;
	biped->functions.count = 0;
	biped->attachments.count = 0;
	biped->widgets.count = 0;
	biped->old_functions.count = 0;
	biped->change_colors.count = 0;
	biped->predicted_resources.count = 0;
	biped->hud_text_message_index = h1_biped->hud_text_message_index;

	// unit
	biped->default_team = h1_biped->default_team;
	biped->constant_sound_volume = h1_biped->constant_sound_volume;
	h1_runtime_reference_set(&biped->integrated_light_toggle, (tag_group)NONE, NONE);
	biped->camera_field_of_view = h1_biped->camera_field_of_view;
	biped->camera_stiffness = h1_biped->camera_stiffness;
	// marker names as the models' (h1_object_tags.cpp): lowercase, spaces as underscores
	auto marker_string_id = [](const char* name) -> string_id
	{
		char marker[32];
		strncpy_s(marker, name, _TRUNCATE);
		for (char* c = marker; *c; c++)
		{
			*c = *c == ' ' ? '_' : (char)tolower((unsigned char)*c);
		}
		return string_id_find_or_add(marker);
	};
	biped->camera_marker_name = marker_string_id(h1_biped->camera_marker_name);
	biped->camera_submerged_marker_name = marker_string_id(h1_biped->camera_submerged_marker_name);
	biped->pitch_auto_level = h1_biped->pitch_auto_level;
	biped->pitch_range = h1_biped->pitch_range;
	biped->soft_ping_threshold = h1_biped->soft_ping_threshold;
	biped->soft_ping_interrupt_time = h1_biped->soft_ping_interrupt_time;
	biped->hard_ping_threshold = h1_biped->hard_ping_threshold;
	biped->hard_ping_interrupt_time = h1_biped->hard_ping_interrupt_time;
	biped->hard_death_threshold = h1_biped->hard_death_threshold;
	biped->feign_death_threshold = h1_biped->feign_death_threshold;
	biped->feign_death_time = h1_biped->feign_death_time;
	biped->distance_of_evade_animation = h1_biped->distance_of_evade_animation;
	biped->distance_of_dive_animation = h1_biped->distance_of_dive_animation;
	biped->stunned_movement_threshold = h1_biped->stunned_movement_threshold;
	biped->feign_death_chance = h1_biped->feign_death_chance;
	biped->feign_repeat_chance = h1_biped->feign_repeat_chance;
	h1_runtime_reference_set(&biped->spawned_turret_character, (tag_group)NONE, NONE);
	biped->aiming_velocity_maximum = h1_biped->aiming_velocity_maximum;
	biped->aiming_acceleration_maximum = h1_biped->aiming_acceleration_maximum;
	biped->casual_aiming_modifier = h1_biped->casual_aiming_modifier;
	biped->looking_velocity_maximum = h1_biped->looking_velocity_maximum;
	biped->looking_acceleration_maximum = h1_biped->looking_acceleration_maximum;
	biped->motion_sensor_blip_size = h1_biped->motion_sensor_blip_size;
	biped->grenade_velocity = h1_biped->grenade_velocity;
	biped->grenade_type = h1_biped->grenade_type;
	biped->grenade_count = h1_biped->grenade_count;
	biped->new_hud_interfaces.count = 0;
	biped->dialogue_variants.count = 0;

	// biped
	biped->moving_turning_speed = h1_biped->moving_turning_speed;
	biped->stationary_turning_threshold = h1_biped->stationary_turning_threshold;
	biped->cosine_stationary_turning_threshold = h1_biped->cosine_stationary_turning_threshold;
	// halo 1 keeps its jump velocity in world units per tick, halo 2 per second
	biped->jump_velocity = h1_biped->jump_velocity * 30.f;
	biped->maximum_soft_landing_time = h1_biped->maximum_soft_landing_time;
	biped->maximum_hard_landing_time = h1_biped->maximum_hard_landing_time;
	biped->minimum_soft_landing_velocity = h1_biped->minimum_soft_landing_velocity;
	biped->minimum_hard_landing_velocity = h1_biped->minimum_hard_landing_velocity;
	biped->maximum_hard_landing_velocity = h1_biped->maximum_hard_landing_velocity;
	biped->death_hard_landing_velocity = h1_biped->death_hard_landing_velocity;
	biped->standing_camera_height = h1_biped->standing_camera_height;
	biped->crouching_camera_height = h1_biped->crouching_camera_height;
	biped->crouch_transition_time = h1_biped->crouch_transition_time;
	biped->autoaim_width = h1_biped->autoaim_width;
	biped->pelvis_node_index = h1_biped->pelvis_node_index;
	biped->head_node_index = h1_biped->head_node_index;
	biped->physics_control_node_index = h1_biped->pelvis_node_index;
	biped->right_hand_node = h1_biped_node_name(h1_model, NONE, "bip01 r hand");
	biped->left_hand_node = h1_biped_node_name(h1_model, NONE, "bip01 l hand");
	biped->preferred_gun_node = biped->right_hand_node;
	biped->height_standing = h1_biped->standing_collision_height;
	biped->height_crouching = h1_biped->crouching_collision_height;
	biped->radius = h1_biped->collision_radius;
	biped->maximum_slope_angle = h1_biped->maximum_slope_angle;
	biped->downhill_falloff_angle = h1_biped->downhill_falloff_angle;
	biped->downhill_cutoff_angle = h1_biped->downhill_cutoff_angle;
	biped->uphill_falloff_angle = h1_biped->uphill_falloff_angle;
	biped->uphill_cutoff_angle = h1_biped->uphill_cutoff_angle;
	biped->downhill_velocity_scale = h1_biped->downhill_velocity_scale;
	biped->uphill_velocity_scale = h1_biped->uphill_velocity_scale;
	biped->cosine_maximum_slope_angle = h1_biped->cosine_maximum_slope_angle;
	biped->negative_sine_downhill_falloff_angle = h1_biped->negative_sine_downhill_falloff_angle;
	biped->negative_sine_downhill_cutoff_angle = h1_biped->negative_sine_downhill_cutoff_angle;
	biped->sine_uphill_falloff_angle = h1_biped->sine_uphill_falloff_angle;
	biped->sine_uphill_cutoff_angle = h1_biped->sine_uphill_cutoff_angle;
	biped->bank_angle = h1_biped->bank_angle;
	biped->bank_apply_time = h1_biped->bank_apply_time;
	biped->bank_decay_time = h1_biped->bank_decay_time;
	biped->pitch_ratio = h1_biped->pitch_ratio;
	biped->maximum_velocity = h1_biped->maximum_velocity;
	biped->maximum_sidestep_velocity = h1_biped->maximum_sidestep_velocity;
	biped->acceleration = h1_biped->acceleration;
	biped->deceleration = h1_biped->deceleration;
	biped->angular_velocity_maximum = h1_biped->angular_velocity_maximum;
	biped->angular_acceleration_maximum = h1_biped->angular_acceleration_maximum;
	biped->crouch_velocity_modifier = h1_biped->crouch_velocity_modifier;
	h1_runtime_reference_set(&biped->reanimation_character, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&biped->death_spawn_character, (tag_group)NONE, NONE);
	biped->death_spawn_count = 0;

	h1_objects_bind(biped_index, h1_biped_index);
	h1_log("bipeds: %s from the host's %s", name, tag_get_name(template_index));
	return biped_index;
}

void h1_bipeds_build_player(void)
{
	const datum h1_globals_index = g_h1_cache_file->tag_find('matg', "globals\\globals");
	const h1_matg* h1_globals = h1_globals_index != NONE ? (const h1_matg*)g_h1_cache_file->tag_get('matg', h1_globals_index) : NULL;
	const h1_matg_player_information* player_information = h1_globals ? g_h1_cache_file->block_get(h1_globals->player_information, 0) : NULL;
	h2x_matg* globals = (h2x_matg*)scenario_get_game_globals();
	if (!player_information || player_information->unit.index == NONE || !globals)
	{
		h1_log("bipeds: no halo 1 player unit");
		return;
	}
	const datum biped_index = h1_biped_definition_build(player_information->unit.index);
	if (biped_index == NONE)
	{
		return;
	}
	// every player representation is the halo 1 player
	for (int32 i = 0; i < globals->player_representation.count; i++)
	{
		h2x_matg_player_representation* representation = globals->player_representation[i];
		h1_runtime_reference_set(&representation->third_person_unit, 'bipd', biped_index);
		representation->third_person_variant = _string_id_default;
	}
	h1_log("bipeds: the players are %s", g_h1_cache_file->tag_name_get(player_information->unit.index));
	return;
}

/* ---------- private code */

// the host map's player biped, read before the player representations change
static datum h1_biped_template_get(void)
{
	static datum s_template_index = NONE;
	const h2x_matg* globals = (const h2x_matg*)scenario_get_game_globals();
	for (int32 i = 0; globals && i < globals->player_representation.count; i++)
	{
		const h2x_matg_player_representation* representation = globals->player_representation[i];
		const datum unit_index = representation->third_person_unit.index;
		if (unit_index != NONE && representation->third_person_unit.group == 'bipd' && !strstr(tag_get_name(unit_index), "halo1\\"))
		{
			s_template_index = unit_index;
			break;
		}
	}
	return s_template_index;
}

// the model's node with this name (halo 2 attaches held weapons at the hand nodes)
static string_id h1_biped_node_name(const h1_mode* h1_model, int16 node_index, const char* fallback)
{
	for (int32 i = 0; i < h1_model->nodes.count; i++)
	{
		const h1_mode_nodes* node = g_h1_cache_file->block_get(h1_model->nodes, i);
		if (!_stricmp(node->name, fallback))
		{
			return string_id_find_or_add(node->name);
		}
	}
	return string_id_find_or_add(fallback);
}

#include "stdafx.h"
#include "h1_animations.h"

#include "h1_cache_file.h"
#include "h1_log.h"
#include "h1_runtime.h"
#include "h2_tag_definitions_generated.h"

#include "tag_files/tag_groups.h"

#include <algorithm>
#include <string>
#include <vector>

/* constants */

enum
{
	k_h2_codec_uncompressed_animated = 8,
	k_h2_codec_header_size = 32,
	k_h2_animation_internal_flags = 0x80,
	k_h2_animation_compression = 2,
	k_h1_maximum_animation_nodes = 64,
};

enum e_h1_animation_type
{
	_h1_animation_base = 0,
	_h1_animation_overlay,
	_h1_animation_replacement,
};

// model_animation_definitions.c first_person_weapon_animation_list: the halo 2 first person labels of each halo 1 slot
// (the halo 2 graphs name them alike with underscores), NULL: none
static const char* const k_h1_first_person_animation_labels[] =
{
	"idle", "posing", "fire_1", "moving", "overlays", "light_off", "light_on", "reload_empty", "reload_full", "overheated",
	"ready", "put_away", "overcharged", "melee_strike_1", "fire_2", "overcharged_jitter", "throw_grenade", "ammunition",
	"misfire_1", "misfire_2", "throw_overheated", "overheating", "overheating_again", "reload_enter", "exit_empty", "exit_full",
	"o_h_exit", "o_h_s_enter",
};
enum
{
	k_h1_first_person_reload_empty = 7,
	k_h1_first_person_reload_full = 8,
	k_h1_first_person_shotgun_enter = 23,
	k_h1_first_person_shotgun_exit_empty = 24,
};

static const char* const k_h1_vehicle_animation_names[] = { "steering", "roll", "throttle", "velocity", "braking", "ground_speed" };

static const char* const k_h1_unit_seat_animation_names[] =
{
	"airborne_dead", "landing_dead", "acc_front_back", "acc_left_right", "acc_up_down", "push", "twist", "enter", "exit",
	"look", "talk", "emotions", NULL, "user0", "user1", "user2", "user3", "user4", "user5", "user6", "user7", "user8", "user9",
	"flying_front", "flying_back", "flying_left", "flying_right", "opening", "closing", "hovering",
};

static const char* const k_h1_weapon_class_animation_names[] =
{
	"idle", "gesture", "turn_left", "turn_right", "dive_front", "dive_back", "dive_left", "dive_right",
	"move_front", "move_back", "move_left", "move_right", "slide_front", "slide_back", "slide_left", "slide_right",
	"airborne", "land_soft", "land_hard", NULL, "throw_grenade", "disarm", "drop", "ready", "put_away",
	"aim_still_up", "aim_move_up", "surprise_front", "surprise_back", "berserk", "evade_left", "evade_right",
	"signal_move", "signal_attack", "warn", "stunned_front", "stunned_back", "stunned_left", "stunned_right",
	"melee", "celebrate", "panic", "melee_airborne", "flaming", "resurrect_front", "resurrect_back",
	"melee_continuous", "feeding", "leap_start", "leap_airborne", "leap_melee", "zapping",
};
enum
{
	k_h1_weapon_class_aim_still = 25,
	k_h1_weapon_class_aim_move = 26,
};

static const char* const k_h1_weapon_type_animation_names[] =
{
	"reload_1", "reload_2", "chamber_1", "chamber_2", "fire_1", "fire_2", "charged_1", "charged_2", "melee", "overheat",
};

/* structures */

struct s_h1_node_orientation
{
	real_quaternion rotation;
	real_point3d translation;
	real32 scale;
};

struct s_h1_animation_slot
{
	std::string label;
	int16 animation_index;
};

// a third person animation reached through a mode, weapon class and weapon type
struct s_h1_unit_animation_slot
{
	std::string mode;
	std::string weapon_class;
	std::string weapon_type;
	std::string label;
	int16 animation_index;
};

/* prototypes */

static bool h1_animation_node_flag(int32 flags_0, int32 flags_1, int32 node_index);
static void h1_animation_frame_decode(const h1_antr_animations* animation, int32 frame_index, s_h1_node_orientation* out_nodes);
static void h1_animation_encode(const h1_antr_animations* animation, h2x_jmad_animations* destination, int32 model_node_count);
static int16 h1_blend_screen_add(std::vector<h2x_jmad_blend_screens>& screens, const char* label, real32 right_yaw, real32 left_yaw, int16 right_count, int16 left_count, real32 down_pitch, real32 up_pitch, int16 down_count, int16 up_count);
static std::string h1_animation_label(const char* h1_name);
static void h1_animation_modes_build(h2x_jmad* graph, const std::vector<s_h1_unit_animation_slot>& slots);

/* public code */

datum h1_first_person_animation_graph_build(datum h1_animation_graph_index, const char* name)
{
	const h1_antr* h1_graph = h1_animation_graph_index != NONE ? (const h1_antr*)g_h1_cache_file->tag_get('antr', h1_animation_graph_index) : NULL;
	if (!h1_graph || h1_graph->first_person_weapons.count <= 0)
	{
		return NONE;
	}
	const datum existing = h1_runtime_tag_find('jmad', name);
	if (existing != NONE)
	{
		return existing;
	}

	h2x_jmad* graph = NULL;
	const datum graph_index = h1_runtime_tag_new('jmad', name, &graph);
	if (graph_index == NONE)
	{
		return NONE;
	}
	h1_runtime_reference_set(&graph->parent_animation_graph, (tag_group)NONE, NONE);

	// the skeleton is the graph's nodes: the arms of the hands model and the nodes of the weapon's first person model
	const int32 node_count = MIN(h1_graph->nodes.count, (int32)k_h1_maximum_animation_nodes);
	h2x_jmad_skeleton_nodes* nodes = h1_runtime_block_new(&graph->skeleton_nodes, node_count);
	for (int32 i = 0; i < node_count; i++)
	{
		const h1_antr_nodes* h1_node = g_h1_cache_file->block_get(h1_graph->nodes, i);
		nodes[i].name = string_id_find_or_add(h1_node->name);
		nodes[i].next_sibling_node_index = h1_node->next_sibling_node_index;
		nodes[i].first_child_node_index = h1_node->first_child_node_index;
		nodes[i].parent_node_index = h1_node->parent_node_index;
		nodes[i].node_joint_flags = (uint8)h1_node->node_joint_flags;
		nodes[i].base_vector = h1_node->base_vector;
		nodes[i].vector_range = h1_node->vector_range;
	}

	h2x_jmad_animations* animations = h1_runtime_block_new(&graph->animations, h1_graph->animations.count);
	for (int32 i = 0; i < h1_graph->animations.count; i++)
	{
		const h1_antr_animations* h1_animation = g_h1_cache_file->block_get(h1_graph->animations, i);
		h2x_jmad_animations* animation = &animations[i];
		std::string label = h1_animation->name;
		for (char& c : label)
		{
			c = (c == '-' || c == ' ') ? '_' : c;
		}
		animation->name = string_id_find_or_add(label.c_str());
		animation->node_list_checksum = h1_animation->node_list_checksum;
		animation->type = (int8)h1_animation->type;
		animation->frame_info_type = 0;
		animation->blend_screen_index = NONE;
		animation->node_count = (uint8)node_count;
		animation->frame_count = h1_animation->frame_count;
		animation->internal_flags = k_h2_animation_internal_flags;
		animation->desired_compression = k_h2_animation_compression;
		animation->current_compression = k_h2_animation_compression;
		animation->weight = h1_animation->weight;
		animation->loop_frame_index = h1_animation->loop_frame_index;
		animation->parent_animation_index = NONE;
		animation->next_animation_index = NONE;
		// halo 1's key frames are halo 2's frame events (the melee strike lands on the primary keyframe)
		{
			struct { int16 type; int32 frame; } events[] =
			{
				{ 0, h1_animation->key_frame_index },
				{ 1, h1_animation->second_key_frame_index },
				{ 2, h1_animation->left_foot_frame_index },
				{ 3, h1_animation->right_foot_frame_index },
			};
			int32 event_count = 0;
			for (const auto& event : events)
			{
				event_count += VALID_INDEX(event.frame, h1_animation->frame_count) && (event.type < 2 || event.frame > 0) ? 1 : 0;
			}
			if (event_count > 0)
			{
				h2x_jmad_animations_frame_events* frame_events = h1_runtime_block_new(&animation->frame_events, event_count);
				int32 event_index = 0;
				for (const auto& event : events)
				{
					if (VALID_INDEX(event.frame, h1_animation->frame_count) && (event.type < 2 || event.frame > 0))
					{
						frame_events[event_index].type = event.type;
						frame_events[event_index].frame = (int16)event.frame;
						event_index++;
					}
				}
			}
		}
		h1_animation_encode(h1_animation, animation, node_count);
	}

	// first person weapons.c: the slots of the first person weapon animations
	std::vector<s_h1_animation_slot> actions, overlays;
	const h1_antr_first_person_weapons* first_person = g_h1_cache_file->block_get(h1_graph->first_person_weapons, 0);
	auto slot_animation = [&](int32 slot) -> int16
	{
		if (slot >= first_person->animations.count)
		{
			return NONE;
		}
		const int16 index = g_h1_cache_file->block_get(first_person->animations, slot)->animation_index;
		return VALID_INDEX(index, h1_graph->animations.count) ? index : (int16)NONE;
	};
	for (int32 slot = 0; slot < first_person->animations.count && slot < NUMBEROF(k_h1_first_person_animation_labels); slot++)
	{
		const int16 animation_index = slot_animation(slot);
		if (animation_index == NONE || !k_h1_first_person_animation_labels[slot])
		{
			continue;
		}
		const bool overlay = animations[animation_index].type == _h1_animation_overlay;
		(overlay ? overlays : actions).push_back({ k_h1_first_person_animation_labels[slot], animation_index });
	}
	// halo 2's continuous reloads (the shotgun) name the halo 1 reloads and exit again
	if (slot_animation(k_h1_first_person_shotgun_enter) != NONE)
	{
		if (slot_animation(k_h1_first_person_reload_full) != NONE) actions.push_back({ "reload_continue_full", slot_animation(k_h1_first_person_reload_full) });
		if (slot_animation(k_h1_first_person_reload_empty) != NONE) actions.push_back({ "reload_continue_empty", slot_animation(k_h1_first_person_reload_empty) });
		if (slot_animation(k_h1_first_person_shotgun_exit_empty) != NONE) actions.push_back({ "reload_exit", slot_animation(k_h1_first_person_shotgun_exit_empty) });
	}

	// animation_graph_find_action and friends binary search the labels: sort by string id
	auto by_label = [](const s_h1_animation_slot& a, const s_h1_animation_slot& b)
	{
		return (uint32)string_id_find_or_add(a.label.c_str()) < (uint32)string_id_find_or_add(b.label.c_str());
	};
	std::sort(actions.begin(), actions.end(), by_label);
	std::sort(overlays.begin(), overlays.end(), by_label);

	// one mode, weapon class and weapon type: any
	h2x_jmad_modes* mode = h1_runtime_block_new(&graph->modes, 1);
	mode->label = string_id_find_or_add("any");
	h2x_jmad_modes_weapon_class* weapon_class = h1_runtime_block_new(&mode->weapon_class, 1);
	weapon_class->label = string_id_find_or_add("any");
	h2x_jmad_modes_weapon_class_weapon_type* weapon_type = h1_runtime_block_new(&weapon_class->weapon_type, 1);
	weapon_type->label = string_id_find_or_add("any");
	h2x_jmad_modes_weapon_class_weapon_type_actions* action_entries = h1_runtime_block_new(&weapon_type->actions, (int32)actions.size());
	for (size_t i = 0; i < actions.size(); i++)
	{
		action_entries[i].label = string_id_find_or_add(actions[i].label.c_str());
		action_entries[i].graph_index = NONE;
		action_entries[i].animation_index = actions[i].animation_index;
	}
	h2x_jmad_modes_weapon_class_weapon_type_overlays* overlay_entries = h1_runtime_block_new(&weapon_type->overlays, (int32)overlays.size());
	for (size_t i = 0; i < overlays.size(); i++)
	{
		overlay_entries[i].label = string_id_find_or_add(overlays[i].label.c_str());
		overlay_entries[i].graph_index = NONE;
		overlay_entries[i].animation_index = overlays[i].animation_index;
	}

	h1_log("animations: %s has %d nodes, %d animations, %d first person actions and %d overlays", name, node_count, h1_graph->animations.count,
		(int32)actions.size(), (int32)overlays.size());
	return graph_index;
}

datum h1_animation_graph_build(datum h1_animation_graph_index, const h1_mode* h1_model, const char* name, const h1_phys* h1_physics)
{
	const datum existing = h1_runtime_tag_find('jmad', name);
	if (existing != NONE)
	{
		return existing;
	}
	h2x_jmad* graph = NULL;
	const datum graph_index = h1_runtime_tag_new('jmad', name, &graph);
	if (graph_index == NONE)
	{
		return NONE;
	}
	h1_runtime_reference_set(&graph->parent_animation_graph, (tag_group)NONE, NONE);

	const h1_antr* h1_graph = h1_animation_graph_index != NONE ? (const h1_antr*)g_h1_cache_file->tag_get('antr', h1_animation_graph_index) : NULL;

	// the skeleton is the model's node list (units need as many graph nodes as model nodes), with the graph's joints; without a
	// model (the scripts' cinematic graphs) the graph's own nodes, which are its models'
	if (!h1_model && !h1_graph)
	{
		return graph_index;
	}
	const int32 node_count = MIN(h1_model ? h1_model->nodes.count : h1_graph->nodes.count, (int32)k_h1_maximum_animation_nodes);
	h2x_jmad_skeleton_nodes* nodes = h1_runtime_block_new(&graph->skeleton_nodes, node_count);
	for (int32 i = 0; i < node_count; i++)
	{
		const h1_antr_nodes* h1_graph_node = h1_graph && (!h1_model || h1_graph->nodes.count == h1_model->nodes.count) ? g_h1_cache_file->block_get(h1_graph->nodes, i) : NULL;
		if (h1_model)
		{
			const h1_mode_nodes* h1_node = g_h1_cache_file->block_get(h1_model->nodes, i);
			nodes[i].name = string_id_find_or_add(h1_node->name);
			nodes[i].next_sibling_node_index = h1_node->next_sibling_node_index;
			nodes[i].first_child_node_index = h1_node->first_child_node_index;
			nodes[i].parent_node_index = h1_node->parent_node_index;
			nodes[i].z_position = h1_node->default_translation.z;
		}
		else
		{
			nodes[i].name = string_id_find_or_add(h1_graph_node->name);
			nodes[i].next_sibling_node_index = h1_graph_node->next_sibling_node_index;
			nodes[i].first_child_node_index = h1_graph_node->first_child_node_index;
			nodes[i].parent_node_index = h1_graph_node->parent_node_index;
		}
		if (h1_graph_node)
		{
			nodes[i].node_joint_flags = (uint8)h1_graph_node->node_joint_flags;
			nodes[i].base_vector = h1_graph_node->base_vector;
			nodes[i].vector_range = h1_graph_node->vector_range;
		}
	}
	if (!h1_graph)
	{
		return graph_index;
	}

	h2x_jmad_animations* animations = h1_runtime_block_new(&graph->animations, h1_graph->animations.count);
	for (int32 i = 0; i < h1_graph->animations.count; i++)
	{
		const h1_antr_animations* h1_animation = g_h1_cache_file->block_get(h1_graph->animations, i);
		h2x_jmad_animations* animation = &animations[i];
		animation->name = string_id_find_or_add(h1_animation_label(h1_animation->name).c_str());
		animation->node_list_checksum = h1_model ? h1_model->node_list_checksum : h1_animation->node_list_checksum;
		animation->type = (int8)h1_animation->type;
		animation->frame_info_type = 0;
		animation->blend_screen_index = NONE;
		animation->node_count = (uint8)node_count;
		animation->frame_count = h1_animation->frame_count;
		animation->internal_flags = k_h2_animation_internal_flags;
		animation->desired_compression = k_h2_animation_compression;
		animation->current_compression = k_h2_animation_compression;
		animation->weight = h1_animation->weight;
		animation->loop_frame_index = h1_animation->loop_frame_index;
		animation->parent_animation_index = NONE;
		animation->next_animation_index = NONE;
		h1_animation_encode(h1_animation, animation, node_count);
	}

	std::vector<h2x_jmad_blend_screens> blend_screens;
	std::vector<s_h1_unit_animation_slot> slots;

	// vehicle overlays
	const h1_antr_vehicles* h1_vehicle = g_h1_cache_file->block_get(h1_graph->vehicles, 0);
	if (h1_vehicle)
	{
		for (int32 i = 0; i < h1_vehicle->animations.count && i < NUMBEROF(k_h1_vehicle_animation_names); i++)
		{
			const int16 animation_index = g_h1_cache_file->block_get(h1_vehicle->animations, i)->animation_index;
			if (!VALID_INDEX(animation_index, h1_graph->animations.count))
			{
				continue;
			}
			slots.push_back({ "any", "any", "any", k_h1_vehicle_animation_names[i], animation_index });
			// the vehicle unit's own mode, in any weapon class and its weapons' (h1_animation_vehicle_weapon_class)
			slots.push_back({ "combat", "any", "any", k_h1_vehicle_animation_names[i], animation_index });
			const string_id weapon_class = h1_animation_vehicle_weapon_class(h1_animation_graph_index);
			if (weapon_class != string_id_find_or_add("any"))
			{
				slots.push_back({ "combat", string_id_get_string_const(weapon_class), "any", k_h1_vehicle_animation_names[i], animation_index });
			}
			// steering is an aiming screen over the turn
			if (i == 0)
			{
				animations[animation_index].blend_screen_index = (int8)h1_blend_screen_add(blend_screens, "steering",
					h1_vehicle->right_yaw_per_frame, h1_vehicle->left_yaw_per_frame, h1_vehicle->right_frame_count, h1_vehicle->left_frame_count,
					h1_vehicle->down_pitch_per_frame, h1_vehicle->up_pitch_per_frame, h1_vehicle->down_pitch_frame_count, h1_vehicle->up_pitch_frame_count);
			}
		}

		// halo 2's vehicles resolve their (weaponless) weapon class through the weapon list
		h2x_jmad_weapon_list* weapon_list = h1_runtime_block_new(&graph->weapon_list, 1);
		weapon_list->weapon_name = string_id_find_or_add("any");
		weapon_list->weapon_class = string_id_find_or_add("any");

		// suspension at the wheel mass point markers, halo 1 ground depths are negative
		h2x_jmad_vehicle_suspension* suspension = h1_runtime_block_new(&graph->vehicle_suspension, h1_vehicle->suspension_animations.count);
		for (int32 i = 0; i < h1_vehicle->suspension_animations.count; i++)
		{
			const h1_antr_vehicles_suspension_animations* h1_suspension = g_h1_cache_file->block_get(h1_vehicle->suspension_animations, i);
			const h1_phys_mass_points* mass_point = h1_physics ? g_h1_cache_file->block_get(h1_physics->mass_points, h1_suspension->mass_point_index) : NULL;
			char label[64], marker[64];
			sprintf_s(label, "suspension:%d", i);
			suspension[i].label = string_id_find_or_add(label);
			suspension[i].graph_index = NONE;
			suspension[i].animation_index = VALID_INDEX(h1_suspension->animation_index, h1_graph->animations.count) ? h1_suspension->animation_index : NONE;
			suspension[i].marker_name = mass_point ? string_id_find_or_add(h1_mass_point_marker_name(mass_point->name, marker)) : _string_id_empty_string;
			suspension[i].full_extension_ground_depth = -h1_suspension->full_extension_ground_depth;
			suspension[i].full_compression_ground_depth = -h1_suspension->full_compression_ground_depth;
			suspension[i].destroyed_region_name = _string_id_empty_string;
			suspension[i].destroyed_full_extension_ground_depth = suspension[i].full_extension_ground_depth;
			suspension[i].destroyed_full_compression_ground_depth = suspension[i].full_compression_ground_depth;
		}
	}

	// unit seats, their weapon classes and weapon types
	for (int32 u = 0; u < h1_graph->units.count; u++)
	{
		const h1_antr_units* h1_unit = g_h1_cache_file->block_get(h1_graph->units, u);
		const std::string mode = _stricmp(h1_unit->label, "stand") == 0 ? "combat" : h1_animation_label(h1_unit->label);

		for (int32 i = 0; i < h1_unit->animations.count && i < NUMBEROF(k_h1_unit_seat_animation_names); i++)
		{
			const int16 animation_index = g_h1_cache_file->block_get(h1_unit->animations, i)->animation_index;
			if (k_h1_unit_seat_animation_names[i] && VALID_INDEX(animation_index, h1_graph->animations.count))
			{
				slots.push_back({ mode, "any", "any", k_h1_unit_seat_animation_names[i], animation_index });
			}
		}

		for (int32 w = 0; w < h1_unit->weapons.count; w++)
		{
			const h1_antr_units_weapons* h1_weapon = g_h1_cache_file->block_get(h1_unit->weapons, w);
			const std::string weapon_class = h1_weapon->name[0] ? h1_animation_label(h1_weapon->name) : "any";
			int16 aim_screen = NONE;

			for (int32 i = 0; i < h1_weapon->animations.count && i < NUMBEROF(k_h1_weapon_class_animation_names); i++)
			{
				const int16 animation_index = g_h1_cache_file->block_get(h1_weapon->animations, i)->animation_index;
				if (!k_h1_weapon_class_animation_names[i] || !VALID_INDEX(animation_index, h1_graph->animations.count))
				{
					continue;
				}
				slots.push_back({ mode, weapon_class, "any", k_h1_weapon_class_animation_names[i], animation_index });
				// aiming overlays are aiming screens over yaw and pitch
				if (i == k_h1_weapon_class_aim_still || i == k_h1_weapon_class_aim_move)
				{
					if (aim_screen == NONE)
					{
						aim_screen = h1_blend_screen_add(blend_screens, (mode + ":" + weapon_class + ":aim").c_str(),
							h1_weapon->right_yaw_per_frame, h1_weapon->left_yaw_per_frame, h1_weapon->right_frame_count, h1_weapon->left_frame_count,
							h1_weapon->down_pitch_per_frame, h1_weapon->up_pitch_per_frame, h1_weapon->down_pitch_frame_count, h1_weapon->up_pitch_frame_count);
					}
					animations[animation_index].blend_screen_index = (int8)aim_screen;
				}
			}

			for (int32 t = 0; t < h1_weapon->weapon_types.count; t++)
			{
				const h1_antr_units_weapons_weapon_types* h1_type = g_h1_cache_file->block_get(h1_weapon->weapon_types, t);
				const std::string weapon_type = h1_type->label[0] ? h1_animation_label(h1_type->label) : "any";
				for (int32 i = 0; i < h1_type->animations.count && i < NUMBEROF(k_h1_weapon_type_animation_names); i++)
				{
					const int16 animation_index = g_h1_cache_file->block_get(h1_type->animations, i)->animation_index;
					if (VALID_INDEX(animation_index, h1_graph->animations.count))
					{
						slots.push_back({ mode, weapon_class, weapon_type, k_h1_weapon_type_animation_names[i], animation_index });
					}
				}
			}
		}
	}

	h2x_jmad_blend_screens* screens = h1_runtime_block_new(&graph->blend_screens, (int32)blend_screens.size());
	for (size_t i = 0; i < blend_screens.size(); i++)
	{
		screens[i] = blend_screens[i];
	}
	h1_animation_modes_build(graph, slots);

	h1_log("animations: %s has %d animations, %d modes, %d blend screens, %d suspension animations", name, graph->animations.count, graph->modes.count, graph->blend_screens.count, graph->vehicle_suspension.count);
	return graph_index;
}

const char* h1_mass_point_marker_name(const char* mass_point_name, char(&buffer)[64])
{
	sprintf_s(buffer, "mass point %s", mass_point_name);
	return buffer;
}

/* private code */

static bool h1_animation_node_flag(int32 flags_0, int32 flags_1, int32 node_index)
{
	return node_index < 32 ? TEST_BIT(flags_0, node_index) : TEST_BIT(flags_1, node_index - 32);
}

// halo 1 uncompressed frames: per node an 8 byte quaternion, a translation and a scale, from the frame when the node is animated
// and from the default data otherwise; halo 1 quaternions build matrices from their conjugate (h1_quaternion_to_h2)
static void h1_animation_frame_decode(const h1_antr_animations* animation, int32 frame_index, s_h1_node_orientation* out_nodes)
{
	const uint8* frame = (const uint8*)g_h1_cache_file->data_get(animation->frame_data);
	const uint8* defaults = (const uint8*)g_h1_cache_file->data_get(animation->default_data);
	if (frame)
	{
		frame += frame_index * animation->frame_size;
	}

	for (int32 node = 0; node < animation->node_count && node < k_h1_maximum_animation_nodes; node++)
	{
		s_h1_node_orientation* orientation = &out_nodes[node];

		const bool rotated = h1_animation_node_flag(animation->node_rotation_flags_0, animation->node_rotation_flags_1, node);
		const uint8** rotation_source = rotated ? &frame : &defaults;
		if (*rotation_source)
		{
			const int16* q = (const int16*)*rotation_source;
			const real32 scale = 1.f / 32767.f;
			orientation->rotation = { -q[0] * scale, -q[1] * scale, -q[2] * scale, q[3] * scale };
			*rotation_source += 8;
		}

		const bool translated = h1_animation_node_flag(animation->node_transformation_flags_0, animation->node_transformation_flags_1, node);
		const uint8** translation_source = translated ? &frame : &defaults;
		if (*translation_source)
		{
			csmemcpy(&orientation->translation, *translation_source, sizeof(real_point3d));
			*translation_source += sizeof(real_point3d);
		}

		const bool scaled = h1_animation_node_flag(animation->node_scale_flags_0, animation->node_scale_flags_1, node);
		const uint8** scale_source = scaled ? &frame : &defaults;
		if (*scale_source)
		{
			orientation->scale = *(const real32*)*scale_source;
			*scale_source += sizeof(real32);
		}
	}
	return;
}

string_id h1_animation_vehicle_weapon_class(datum h1_animation_graph_index)
{
	const h1_antr* graph = h1_animation_graph_index != NONE ? (const h1_antr*)g_h1_cache_file->tag_get('antr', h1_animation_graph_index) : NULL;
	const h1_antr_units* unit = graph && graph->units.count > 0 ? g_h1_cache_file->block_get(graph->units, 0) : NULL;
	const h1_antr_units_weapons* weapon = unit && unit->weapons.count > 0 ? g_h1_cache_file->block_get(unit->weapons, 0) : NULL;
	return string_id_find_or_add(weapon && weapon->name[0] ? h1_animation_label(weapon->name).c_str() : "any");
}

bool h1_animation_seat_enter_root_get(datum h1_animation_graph_index, const char* seat_label, real_point3d* out_position)
{
	const h1_antr* graph = h1_animation_graph_index != NONE ? (const h1_antr*)g_h1_cache_file->tag_get('antr', h1_animation_graph_index) : NULL;
	if (!graph)
	{
		return false;
	}
	constexpr int32 k_seat_enter = 7;
	for (int32 u = 0; u < graph->units.count; u++)
	{
		const h1_antr_units* unit = g_h1_cache_file->block_get(graph->units, u);
		if (_stricmp(unit->label, seat_label) != 0 || unit->animations.count <= k_seat_enter)
		{
			continue;
		}
		const int16 animation_index = g_h1_cache_file->block_get(unit->animations, k_seat_enter)->animation_index;
		if (!VALID_INDEX(animation_index, graph->animations.count))
		{
			return false;
		}
		s_h1_node_orientation nodes[k_h1_maximum_animation_nodes];
		for (s_h1_node_orientation& node : nodes)
		{
			node = { { 0.f, 0.f, 0.f, 1.f }, { 0.f, 0.f, 0.f }, 1.f };
		}
		h1_animation_frame_decode(g_h1_cache_file->block_get(graph->animations, animation_index), 0, nodes);
		*out_position = nodes[0].translation;
		return true;
	}
	return false;
}

static void h1_animation_encode(const h1_antr_animations* animation, h2x_jmad_animations* destination, int32 model_node_count)
{
	const int32 node_count = MIN((int32)animation->node_count, (int32)k_h1_maximum_animation_nodes);
	const int32 frame_count = MAX((int32)animation->frame_count, 1);
	const bool base = animation->type == _h1_animation_base;

	// base animations carry every node's rotation and translation, overlays and replacements their animated nodes
	std::vector<int32> rotated, translated, scaled;
	for (int32 node = 0; node < node_count; node++)
	{
		if (base || h1_animation_node_flag(animation->node_rotation_flags_0, animation->node_rotation_flags_1, node)) rotated.push_back(node);
		if (base || h1_animation_node_flag(animation->node_transformation_flags_0, animation->node_transformation_flags_1, node)) translated.push_back(node);
		if (h1_animation_node_flag(animation->node_scale_flags_0, animation->node_scale_flags_1, node)) scaled.push_back(node);
	}

	std::vector<std::vector<s_h1_node_orientation>> frames(frame_count, std::vector<s_h1_node_orientation>(k_h1_maximum_animation_nodes));
	for (int32 f = 0; f < frame_count; f++)
	{
		for (s_h1_node_orientation& node : frames[f])
		{
			node = { { 0.f, 0.f, 0.f, 1.f }, { 0.f, 0.f, 0.f }, 1.f };
		}
		h1_animation_frame_decode(animation, f, frames[f].data());
		// q and -q are the same rotation, but halo 2 blends between frames: keep each node on the near side of its last frame
		// (the first frame on the positive w side, so blends between animations agree too)
		for (int32 node = 0; node < node_count; node++)
		{
			real_quaternion* q = &frames[f][node].rotation;
			const real32 side = f > 0 ?
				q->v.i * frames[f - 1][node].rotation.v.i + q->v.j * frames[f - 1][node].rotation.v.j + q->v.k * frames[f - 1][node].rotation.v.k + q->w * frames[f - 1][node].rotation.w :
				q->w;
			if (side < 0.f)
			{
				*q = { -q->v.i, -q->v.j, -q->v.k, -q->w };
			}
		}
	}

	const uint32 rotation_stride = frame_count * 16;
	const uint32 translation_stride = frame_count * 12;
	const uint32 scale_stride = frame_count * 4;
	const uint32 translation_offset = k_h2_codec_header_size + (uint32)rotated.size() * rotation_stride;
	const uint32 scale_offset = translation_offset + (uint32)translated.size() * translation_stride;
	const uint32 animated_size = scale_offset + (uint32)scaled.size() * scale_stride;
	const uint32 flags_vector_size = ((model_node_count + 31) >> 3) & ~3u;

	std::vector<uint8> data(animated_size + flags_vector_size * 3, 0);
	uint8* header = data.data();
	header[0] = k_h2_codec_uncompressed_animated;
	header[1] = (uint8)rotated.size();
	header[2] = (uint8)translated.size();
	header[3] = (uint8)scaled.size();
	const real32 header_floats[2] = { 0.f, 1.f };
	csmemcpy(header + 4, header_floats, sizeof(header_floats));
	const uint32 header_words[5] = { translation_offset, scale_offset, rotation_stride, translation_stride, scale_stride };
	csmemcpy(header + 12, header_words, sizeof(header_words));

	real32* rotations = (real32*)(data.data() + k_h2_codec_header_size);
	for (size_t n = 0; n < rotated.size(); n++)
	{
		for (int32 f = 0; f < frame_count; f++)
		{
			const real_quaternion* q = &frames[f][rotated[n]].rotation;
			real32* out = &rotations[(n * frame_count + f) * 4];
			out[0] = q->v.i; out[1] = q->v.j; out[2] = q->v.k; out[3] = q->w;
		}
	}
	real32* translations = (real32*)(data.data() + translation_offset);
	for (size_t n = 0; n < translated.size(); n++)
	{
		for (int32 f = 0; f < frame_count; f++)
		{
			const real_point3d* t = &frames[f][translated[n]].translation;
			real32* out = &translations[(n * frame_count + f) * 3];
			out[0] = t->x; out[1] = t->y; out[2] = t->z;
		}
	}
	real32* scales = (real32*)(data.data() + scale_offset);
	for (size_t n = 0; n < scaled.size(); n++)
	{
		for (int32 f = 0; f < frame_count; f++)
		{
			scales[n * frame_count + f] = frames[f][scaled[n]].scale;
		}
	}

	// node flags: rotation, translation, scale bit vectors
	uint8* flags = data.data() + animated_size;
	for (int32 node : rotated) flags[node >> 3] |= (uint8)(1 << (node & 7));
	for (int32 node : translated) flags[flags_vector_size + (node >> 3)] |= (uint8)(1 << (node & 7));
	for (int32 node : scaled) flags[flags_vector_size * 2 + (node >> 3)] |= (uint8)(1 << (node & 7));

	h1_runtime_data_set(&destination->resource, data.data(), (int32)data.size());
	// packed data sizes: node flags size and animated codec size, no static codec or movement data
	destination->unknown_3 = (int8)(flags_vector_size * 3);
	destination->unknown_8 = (int32)animated_size;
	return;
}

// halo 1 matches animation names in any case, halo 2's string ids are lower case
static std::string h1_animation_label(const char* h1_name)
{
	std::string label = h1_name;
	for (char& c : label)
	{
		c = (c == '-' || c == ' ') ? '_' : (char)tolower((uint8)c);
	}
	return label;
}

static int16 h1_blend_screen_add(std::vector<h2x_jmad_blend_screens>& screens, const char* label, real32 right_yaw, real32 left_yaw, int16 right_count, int16 left_count, real32 down_pitch, real32 up_pitch, int16 down_count, int16 up_count)
{
	h2x_jmad_blend_screens screen = {};
	screen.label = string_id_find_or_add(label);
	screen.right_yaw_per_frame = right_yaw;
	screen.left_yaw_per_frame = left_yaw;
	screen.right_frame_count = right_count;
	screen.left_frame_count = left_count;
	screen.down_pitch_per_frame = down_pitch;
	screen.up_pitch_per_frame = up_pitch;
	screen.down_pitch_frame_count = down_count;
	screen.up_pitch_frame_count = up_count;
	screens.push_back(screen);
	return (int16)(screens.size() - 1);
}

// the labels of a level of the graph once each, sorted by string id (halo 2 binary searches them)
static std::vector<std::string> h1_animation_labels_sorted(std::vector<std::string> labels)
{
	std::sort(labels.begin(), labels.end());
	labels.erase(std::unique(labels.begin(), labels.end()), labels.end());
	std::sort(labels.begin(), labels.end(), [](const std::string& a, const std::string& b)
	{
		return (uint32)string_id_find_or_add(a.c_str()) < (uint32)string_id_find_or_add(b.c_str());
	});
	return labels;
}

// nests the slots into modes, weapon classes and weapon types; overlays and actions by animation type
static void h1_animation_modes_build(h2x_jmad* graph, const std::vector<s_h1_unit_animation_slot>& slots)
{
	std::vector<std::string> mode_labels;
	for (const s_h1_unit_animation_slot& slot : slots) mode_labels.push_back(slot.mode);
	const std::vector<std::string> modes = h1_animation_labels_sorted(mode_labels);

	const h2x_jmad_animations* animations = graph->animations.count > 0 ? graph->animations[0] : NULL;
	h2x_jmad_modes* mode_entries = h1_runtime_block_new(&graph->modes, (int32)modes.size());
	for (size_t m = 0; m < modes.size(); m++)
	{
		mode_entries[m].label = string_id_find_or_add(modes[m].c_str());

		std::vector<std::string> class_labels;
		for (const s_h1_unit_animation_slot& slot : slots) if (slot.mode == modes[m]) class_labels.push_back(slot.weapon_class);
		const std::vector<std::string> classes = h1_animation_labels_sorted(class_labels);
		h2x_jmad_modes_weapon_class* class_entries = h1_runtime_block_new(&mode_entries[m].weapon_class, (int32)classes.size());
		for (size_t c = 0; c < classes.size(); c++)
		{
			class_entries[c].label = string_id_find_or_add(classes[c].c_str());

			std::vector<std::string> type_labels;
			for (const s_h1_unit_animation_slot& slot : slots) if (slot.mode == modes[m] && slot.weapon_class == classes[c]) type_labels.push_back(slot.weapon_type);
			const std::vector<std::string> types = h1_animation_labels_sorted(type_labels);
			h2x_jmad_modes_weapon_class_weapon_type* type_entries = h1_runtime_block_new(&class_entries[c].weapon_type, (int32)types.size());
			for (size_t t = 0; t < types.size(); t++)
			{
				type_entries[t].label = string_id_find_or_add(types[t].c_str());

				std::vector<const s_h1_unit_animation_slot*> actions, overlays;
				for (const s_h1_unit_animation_slot& slot : slots)
				{
					if (slot.mode != modes[m] || slot.weapon_class != classes[c] || slot.weapon_type != types[t])
					{
						continue;
					}
					const bool overlay = animations && animations[slot.animation_index].type == _h1_animation_overlay;
					std::vector<const s_h1_unit_animation_slot*>& list = overlay ? overlays : actions;
					// one animation per label
					if (std::find_if(list.begin(), list.end(), [&](const s_h1_unit_animation_slot* other) { return other->label == slot.label; }) == list.end())
					{
						list.push_back(&slot);
					}
				}
				auto by_label = [](const s_h1_unit_animation_slot* a, const s_h1_unit_animation_slot* b)
				{
					return (uint32)string_id_find_or_add(a->label.c_str()) < (uint32)string_id_find_or_add(b->label.c_str());
				};
				std::sort(actions.begin(), actions.end(), by_label);
				std::sort(overlays.begin(), overlays.end(), by_label);
				h2x_jmad_modes_weapon_class_weapon_type_actions* action_entries = h1_runtime_block_new(&type_entries[t].actions, (int32)actions.size());
				for (size_t a = 0; a < actions.size(); a++)
				{
					action_entries[a].label = string_id_find_or_add(actions[a]->label.c_str());
					action_entries[a].graph_index = NONE;
					action_entries[a].animation_index = actions[a]->animation_index;
				}
				h2x_jmad_modes_weapon_class_weapon_type_overlays* overlay_entries = h1_runtime_block_new(&type_entries[t].overlays, (int32)overlays.size());
				for (size_t o = 0; o < overlays.size(); o++)
				{
					overlay_entries[o].label = string_id_find_or_add(overlays[o]->label.c_str());
					overlay_entries[o].graph_index = NONE;
					overlay_entries[o].animation_index = overlays[o]->animation_index;
				}
			}
		}
	}
	return;
}

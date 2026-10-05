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

/* prototypes */

static bool h1_animation_node_flag(int32 flags_0, int32 flags_1, int32 node_index);
static void h1_animation_frame_decode(const h1_antr_animations* animation, int32 frame_index, s_h1_node_orientation* out_nodes);
static void h1_animation_encode(const h1_antr_animations* animation, h2x_jmad_animations* destination, int32 model_node_count);

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

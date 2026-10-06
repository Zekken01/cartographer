#include "stdafx.h"
#include "h1_items.h"

#include "h1_cache_file.h"
#include "h1_log.h"
#include "h1_map_loader.h"
#include "h1_sound.h"
#include "h1_object_tags.h"
#include "h1_objects.h"
#include "h1_runtime.h"
#include "h2_tag_definitions_generated.h"

#include "objects/objects.h"

#include <string>
#include <unordered_map>

/* constants */

enum
{
	k_h2_object_type_equipment = 3,
};

/* globals */

// the halo 1 strings halo 2's hud reads (h1_item_messages_get)
static std::unordered_map<string_id, std::wstring> g_h1_hud_strings;

/* private code */

// a unicode string list's string, empty when it has none
static std::wstring h1_unicode_string_get(const char* tag_name, int16 index)
{
	const datum list_index = g_h1_cache_file->tag_find('ustr', tag_name);
	const h1_tag_block<h1_tag_data>* strings = list_index != NONE ? (const h1_tag_block<h1_tag_data>*)g_h1_cache_file->tag_get('ustr', list_index) : NULL;
	if (!strings || !VALID_INDEX(index, strings->count))
	{
		return std::wstring();
	}
	const h1_tag_data* data = g_h1_cache_file->block_get(*strings, index);
	const wchar_t* text = data ? (const wchar_t*)g_h1_cache_file->data_get(*data) : NULL;
	return text ? std::wstring(text, wcsnlen(text, data->size / sizeof(wchar_t))) : std::wstring();
}

static string_id h1_hud_string_add(const char* name, const std::wstring& text)
{
	const string_id id = string_id_find_or_add(name);
	g_h1_hud_strings[id] = text;
	return id;
}

// the hud's string lookup (its globals' unicode strings): halo 1's item strings first
static void __cdecl h1_hud_string_get(string_id id, wchar_t* out_text)
{
	out_text[0] = L'\0';
	if (h1_maps_active())
	{
		auto found = g_h1_hud_strings.find(id);
		if (found != g_h1_hud_strings.end())
		{
			wcsncpy_s(out_text, 256, found->second.c_str(), _TRUNCATE);
			return;
		}
	}
	const uint8* hud_globals = *Memory::GetAddress<uint8**>(0x9765C8);
	const datum strings_index = hud_globals ? *(const datum*)(hud_globals + 0x3FC) : NONE;
	if (strings_index != NONE)
	{
		Memory::GetAddress<void(__cdecl*)(datum, string_id, wchar_t*)>(0x3E3AC)(strings_index, id, out_text);
	}
	return;
}

/* public code */

void h1_item_messages_get(datum h1_item_index, int16 h1_message_index, s_h1_item_messages* out_messages)
{
	// the item's name: its hud item message without the "Picked up" and the article
	const std::wstring picked_up = h1_unicode_string_get("ui\\hud\\hud_item_messages", h1_message_index);
	std::wstring item = picked_up;
	for (const wchar_t* prefix : { L"Picked up ", L"a ", L"an ", L"the " })
	{
		if (item.compare(0, wcslen(prefix), prefix) == 0)
		{
			item.erase(0, wcslen(prefix));
		}
	}
	if (item.empty() || item.find(L'%') != std::wstring::npos)
	{
		const char* name = g_h1_cache_file->tag_name_get(h1_item_index);
		const char* last = strrchr(name, '\\');
		const std::string short_name = last ? last + 1 : name;
		item.assign(short_name.begin(), short_name.end());
	}

	// halo 2's hold and action button glyphs
	const std::wstring hold_action = L"\xE47C \xE45A";
	char name[64];
	sprintf_s(name, "h1_pickup_%04X", DATUM_INDEX_TO_ABSOLUTE_INDEX(h1_item_index));
	out_messages->pickup = h1_hud_string_add(name, hold_action + L" to pick up " + item);
	sprintf_s(name, "h1_swap_%04X", DATUM_INDEX_TO_ABSOLUTE_INDEX(h1_item_index));
	out_messages->swap = h1_hud_string_add(name, hold_action + L" to swap for " + item);
	out_messages->picked_up = _string_id_empty_string;
	if (!picked_up.empty() && picked_up.find(L'%') == std::wstring::npos)
	{
		sprintf_s(name, "h1_picked_up_%04X", DATUM_INDEX_TO_ABSOLUTE_INDEX(h1_item_index));
		out_messages->picked_up = h1_hud_string_add(name, picked_up);
	}
	return;
}


datum h1_equipment_definition_build(datum h1_equipment_index)
{
	const char* h1_name = g_h1_cache_file->tag_name_get(h1_equipment_index);
	char name[256];
	sprintf_s(name, "halo1\\%s", h1_name);
	const datum existing = h1_runtime_tag_find('eqip', name);
	if (existing != NONE)
	{
		return existing;
	}

	const h1_eqip* h1_equipment = (const h1_eqip*)g_h1_cache_file->tag_get('eqip', h1_equipment_index);
	const h1_mode* h1_model = h1_equipment ? (const h1_mode*)g_h1_cache_file->tag_get('mode', h1_equipment->model.index) : NULL;
	const h1_coll* h1_collision = h1_equipment ? (const h1_coll*)g_h1_cache_file->tag_get('coll', h1_equipment->collision_model.index) : NULL;
	if (!h1_model)
	{
		h1_log("items: %s is missing its model", h1_name);
		return NONE;
	}

	s_h1_object_tags tags;
	tags.render_model = h1_object_render_model_build(h1_model, name);
	tags.collision_model = h1_collision ? h1_object_collision_model_build(h1_collision, h1_model, name) : NONE;
	tags.physics_model = NONE;
	tags.animation_graph = NONE;
	tags.disappear_distance = 80.f;
	const datum model_index = tags.render_model != NONE ? h1_object_model_build(&tags, h1_model, h1_collision, name) : NONE;
	if (model_index == NONE)
	{
		return NONE;
	}

	h2x_eqip* equipment = NULL;
	const datum equipment_index = h1_runtime_tag_new('eqip', name, &equipment);
	if (equipment_index == NONE)
	{
		return NONE;
	}

	// object
	equipment->object_type = k_h2_object_type_equipment;
	equipment->bounding_radius = h1_equipment->bounding_radius;
	equipment->bounding_offset = h1_equipment->bounding_offset;
	equipment->acceleration_scale = h1_equipment->acceleration_scale;
	equipment->default_model_variant = _string_id_default;
	h1_runtime_reference_set(&equipment->model, 'hlmt', model_index);
	h1_runtime_reference_set(&equipment->crate_object, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&equipment->modifier_shader, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&equipment->creation_effect, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&equipment->material_effects, (tag_group)NONE, NONE);
	equipment->apply_collision_damage_scale = 1.f;
	equipment->game_acceleration = { 2.5f, 4.5f };
	equipment->game_scale = { 0.2f, 1.25f };
	equipment->absolute_acceleration = { 2.5f, 10.f };
	equipment->absolute_scale = { 0.2f, 1.25f };
	equipment->hud_text_message_index = h1_equipment->hud_text_message_index;

	// item
	equipment->flags_2 = h1_equipment->flags_2;
	equipment->old_message_index = h1_equipment->message_index;
	equipment->sort_order = h1_equipment->sort_order;
	equipment->multiplayer_on_ground_scale = h1_equipment->scale > 0.f ? h1_equipment->scale : 1.f;
	equipment->campaign_on_ground_scale = equipment->multiplayer_on_ground_scale;
	h1_runtime_reference_set(&equipment->unknown_3, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&equipment->collision_sound, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&equipment->detonation_damage_effect, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&equipment->detonating_effect, (tag_group)NONE, NONE);
	h1_runtime_reference_set(&equipment->detonation_effect, (tag_group)NONE, NONE);
	equipment->detonation_delay = h1_equipment->detonation_delay;

	// equipment: both games share the powerup types (none, double speed, overshield, camouflage, vision, health, grenade)
	equipment->powerup_type = h1_equipment->powerup_type;
	equipment->grenade_type = h1_equipment->grenade_type;
	equipment->powerup_time = h1_equipment->powerup_time;
	// halo 2 can't play the halo 1 sound: h1_equipment_pickup_sound plays it
	h1_runtime_reference_set(&equipment->pickup_sound, (tag_group)NONE, NONE);

	h1_objects_bind(equipment_index, h1_equipment_index);
	h1_log("items: built %s (powerup %d, grenade %d, %.1f seconds)", name, equipment->powerup_type, equipment->grenade_type, equipment->powerup_time);
	return equipment_index;
}

// equipment.c equipment_definition_handle_pickup: halo 1 equipment's pickup sound, unspatialized
static bool h1_equipment_definition_pickup_sound(datum definition_index)
{
	const datum h1_definition_index = h1_maps_active() ? h1_objects_h1_definition_get(definition_index) : NONE;
	const h1_eqip* h1_equipment = h1_definition_index != NONE ? (const h1_eqip*)g_h1_cache_file->tag_get('eqip', h1_definition_index) : NULL;
	if (!h1_equipment)
	{
		return false;
	}
	if (h1_equipment->pickup_sound.index != NONE)
	{
		h1_sound_scripted_start(h1_equipment->pickup_sound.index, NULL, 1.f);
	}
	return true;
}

static void __cdecl h1_equipment_definition_handle_pickup(datum definition_index)
{
	if (!h1_equipment_definition_pickup_sound(definition_index))
	{
		INVOKE(0x17580B, 0x0, h1_equipment_definition_handle_pickup, definition_index);
	}
	return;
}

// equipment.c equipment_handle_pickup
static void __cdecl h1_equipment_handle_pickup(datum equipment_index)
{
	const object_datum* equipment = (const object_datum*)object_try_and_get(equipment_index);
	if (!equipment || !h1_equipment_definition_pickup_sound(equipment->definition_index))
	{
		INVOKE(0x1757BC, 0x0, h1_equipment_handle_pickup, equipment_index);
	}
	return;
}

void h1_items_apply_patches(void)
{
	PatchCall(Memory::GetAddress(0x56116), h1_equipment_handle_pickup);
	PatchCall(Memory::GetAddress(0x13E348), h1_equipment_handle_pickup);
	PatchCall(Memory::GetAddress(0x15EE1D), h1_equipment_definition_handle_pickup);
	PatchCall(Memory::GetAddress(0x1F97C9), h1_equipment_definition_handle_pickup);
	PatchCall(Memory::GetAddress(0x1F9824), h1_equipment_definition_handle_pickup);
	PatchCall(Memory::GetAddress(0x1F98B3), h1_equipment_definition_handle_pickup);
	WriteJmpTo(Memory::GetAddress(0x224B26), h1_hud_string_get);
	return;
}

#pragma once

/*
* Halo 1 equipment (powerups, health packs, grenades, ammunition) as Halo 2 equipment built only
* from the Halo 1 equipment tag, its model and collision model. The Halo 1 powerup type, grenade type
* and powerup time carry over unchanged (Halo 2 kept the Halo 1 powerup enum: health packs heal).
* The Halo 1 model is drawn by h1_objects.
*/

// the halo 2 equipment built from a halo 1 equipment tag, NONE on failure
datum h1_equipment_definition_build(datum h1_equipment_index);

// the items' pickup sounds play the halo 1 sounds their tags carry, halo 2's hud strings include the items' halo 1 prompts
void h1_items_apply_patches(void);

// hud.c hud_show_action_response's pickup and swap weapon prompts ("Hold <action> to pick up <item>", the hud messages'
// pickup and swap_weapon), and the item's hud item message ("Picked up an assault rifle"), as runtime string ids halo 2's
// hud looks up; the item is named by its hud item message
struct s_h1_item_messages
{
	string_id pickup;
	string_id swap;
	string_id picked_up;
};
void h1_item_messages_get(datum h1_item_index, int16 h1_message_index, s_h1_item_messages* out_messages);

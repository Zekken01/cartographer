#pragma once
/*
* first_person_weapons.c on Halo 1 maps: the local player's Halo 1 first person weapon runs Halo 1's first person state
* machine (the weapon's messages from h1_weapon_logic choose the animations) and its node matrices come from the Halo 1
* first person animation graph at the camera, for the weapon's first person model and the Halo 1 hands.
*/

// forgets the first person weapon (a new map)
void h1_first_person_weapon_reset(void);
// first_person_weapons_update: a halo 1 tick, after the weapon in the local player's hands updated (h1_weapon_logic)
void h1_first_person_weapon_tick(void);
// the messages the weapon in the local player's hands sent this tick (first_person_weapon_message_from_weapon), after its update
void h1_first_person_weapon_messages(void);
// the local player's unit, NONE without one
datum h1_first_person_weapon_unit_get(void);
// first_person_weapon_build_node_matrices for a halo 1 model drawn in first person (the weapon's model or the hands) of
// the weapon object: the model's node matrices, false when halo 1's first person weapon doesn't show it
bool h1_first_person_weapon_model_nodes_get(datum h1_model_index, datum weapon_object_index, real_matrix4x3* node_matrices, int32 node_count);
// the unit's halo 1 first person weapon is in its melee animation
bool h1_first_person_weapon_meleeing(datum unit_index);

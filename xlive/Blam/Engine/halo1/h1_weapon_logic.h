#pragma once
/*
* weapons.c on Halo 1 maps: the Halo 1 weapons run Halo 1's weapon update from the Halo 1 weapon tag (triggers, magazines,
* heat, age, charging, firing effects and projectiles). The Halo 2 weapon object stays the shell Halo 2 holds, readies,
* puts away, picks up and shows the ammunition of; its tag has no triggers or barrels and Halo 2's magazine update skips it.
*/

enum e_h1_first_person_weapon_message : int16
{
	_h1_first_person_weapon_message_primary_fire = 0,
	_h1_first_person_weapon_message_secondary_fire,
	_h1_first_person_weapon_message_primary_misfire,
	_h1_first_person_weapon_message_secondary_misfire,
	_h1_first_person_weapon_message_melee,
	_h1_first_person_weapon_message_light_on,
	_h1_first_person_weapon_message_light_off,
	_h1_first_person_weapon_message_shotgun_enter_reload,
	_h1_first_person_weapon_message_shotgun_exit_reload,
	_h1_first_person_weapon_message_reload_while_empty,
	_h1_first_person_weapon_message_reload_while_full,
	_h1_first_person_weapon_message_put_away,
	_h1_first_person_weapon_message_ready,
	_h1_first_person_weapon_message_drop,
	_h1_first_person_weapon_message_charged,
	_h1_first_person_weapon_message_overheating,
	_h1_first_person_weapon_message_overheating_super_recoil,
	_h1_first_person_weapon_message_throw_grenade,
	k_h1_first_person_weapon_message_count
};

// the halo 2 weapon update runs halo 1's for the halo 1 weapons, halo 2's magazine update skips them
void h1_weapon_logic_apply_patches(void);
// forgets every weapon's halo 1 state (a new map)
void h1_weapon_logic_reset(void);
// weapons.c weapon_trigger_get_charged_fraction, 0 for weapons without halo 1 state
real32 h1_weapon_logic_charged_fraction(datum weapon_index, int16 trigger_index);
// the first_person_weapons.c messages a weapon sent since the last call, oldest first: the count written
int32 h1_weapon_logic_first_person_messages_take(datum weapon_index, e_h1_first_person_weapon_message* messages, int32 maximum_count);

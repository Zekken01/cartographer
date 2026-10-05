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

// a message for the first person weapon, with the weapon's state when it was sent (first_person_weapon_message reads it then)
struct s_h1_first_person_weapon_message
{
	e_h1_first_person_weapon_message type;
	bool magazine_busy;		// the first magazine was reloading or unchambered (a shotgun's next shell)
};

// weapons.h weapon_interface_state: what the hud shows of a weapon
struct s_h1_weapon_interface_state
{
	real32 heat;
	real32 age;
	bool overheated;
	int16 magazine_count;
	struct
	{
		bool reloading;
		bool can_fire;
		int16 rounds_loaded;
		int16 rounds_loaded_maximum;
		int16 rounds_remaining;
		int16 rounds_remaining_maximum;
	} magazines[2];
};

// the halo 2 weapon update runs halo 1's for the halo 1 weapons, halo 2's magazine update skips them
void h1_weapon_logic_apply_patches(void);
// forgets every weapon's halo 1 state (a new map)
void h1_weapon_logic_reset(void);
// weapons.c weapon_trigger_get_charged_fraction, 0 for weapons without halo 1 state
real32 h1_weapon_logic_charged_fraction(datum weapon_index, int16 trigger_index);
// the first_person_weapons.c messages a weapon sent since the last call, oldest first: the count written
// weapon_datum flags: the weapon is overheated (until its heat drops below the recovery threshold) and is leaving it
bool h1_weapon_logic_overheated(datum weapon_index, bool* out_overheated_exit);

int32 h1_weapon_logic_first_person_messages_take(datum weapon_index, struct s_h1_first_person_weapon_message* messages, int32 maximum_count);
// weapons.c weapon_build_weapon_interface_state, false for weapons without halo 1 state
bool h1_weapon_logic_interface_state(datum weapon_index, s_h1_weapon_interface_state* state);

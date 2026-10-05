#pragma once

/*
* Halo 1 weapons as Halo 2 weapons built only from the Halo 1 weapon tag: its model and collision model, magazines,
* triggers (Halo 2 triggers and barrels), projectiles and damage effects built from the Halo 1 tags. Halo 2 simulates
* them; the Halo 1 model is drawn by h1_objects and the Halo 1 firing effects play in h1_effects.
*/

// the halo 2 weapon built from a halo 1 weapon tag, NONE on failure
datum h1_weapon_definition_build(datum h1_weapon_index);

// the halo 1 weapon a halo 2 weapon was built from, NONE for halo 2's own
datum h1_weapon_h1_get(datum h2_weapon_index);

// forgets the weapons built (a new map is being built)
void h1_weapons_reset(void);

// the multiplayer starting weapons (game variant weapon list) become the map's halo 1 weapons
void h1_weapons_build_multiplayer(void);

// halo 1 firing effects of the weapons built (once per frame)
void h1_weapons_update(void);

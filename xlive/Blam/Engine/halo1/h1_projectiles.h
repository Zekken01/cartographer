#pragma once

/*
* Halo 1 projectiles and damage effects as Halo 2 tags built from the Halo 1 tags, and the Halo 1 grenades put into the
* Halo 2 globals so units throw them. The Halo 1 model is drawn by h1_objects.
*/

// the halo 2 damage effect built from a halo 1 damage effect, NONE on failure
datum h1_damage_effect_build(datum h1_damage_effect_index);

// the halo 1 damage effect a halo 2 damage effect was built from, NONE for halo 2's own
datum h1_damage_effect_h1_get(datum h2_damage_effect_index);

// forgets the damage effects built (a new map is being built)
void h1_projectiles_reset(void);

// the halo 2 projectile built from a halo 1 projectile, NONE on failure
datum h1_projectile_definition_build(datum h1_projectile_index);

// the halo 1 grenades (equipment and projectile) replace the halo 2 grenades of the globals
void h1_grenades_build(void);

#pragma once

/*
* Halo 1 projectiles and damage effects as Halo 2 tags built from the Halo 1 tags, and the Halo 1 grenades put into the
* Halo 2 globals so units throw them. The Halo 1 model is drawn by h1_objects.
*/

// the halo 2 damage effect built from a halo 1 damage effect, NONE on failure
datum h1_damage_effect_build(datum h1_damage_effect_index);

// the halo 2 projectile built from a halo 1 projectile, NONE on failure
datum h1_projectile_definition_build(datum h1_projectile_index);

// the halo 1 grenades (equipment and projectile) replace the halo 2 grenades of the globals
void h1_grenades_build(void);

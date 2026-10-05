#pragma once

/*
* Halo 1 effects: the Halo 1 effect, particle and point physics code (effects.c, particles.c, point_physics.c,
* render_particles.c, render_sprite.c) running on the Halo 1 tags of the loaded map and drawn by the Halo 1 renderer.
* Halo 2 keeps simulating the objects, the damage and the projectiles; when a projectile built from a Halo 1 projectile
* detonates, its Halo 1 detonation effect plays here.
*/

// forgets every effect and particle (new map)
void h1_effects_reset(void);

// advances the effects and particles (once per frame)
void h1_effects_update(void);

// draws the particles (transparent pass)
void h1_effects_render(void);

// the halo 2 hooks
void h1_effects_apply_patches(void);

// a halo 1 effect at a point, its "gravity" marker pointing down; markers it names that don't exist use the point
void h1_effect_new_unattached(datum h1_effect_index, const real_point3d* point, const real_vector3d* forward);

// the empty halo 2 effect a halo 2 tag refers to where a halo 1 effect plays instead (built on first use)
datum h1_effects_stub_effect_get(void);

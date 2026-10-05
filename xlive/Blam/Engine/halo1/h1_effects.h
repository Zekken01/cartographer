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

// draws the particles and lens flares (transparent pass)
void h1_effects_render(void);

// draws the lens flares after everything (the first person weapon too), their occlusion tests against it
void h1_effects_render_lens_flares(void);

// draws the decals (after the opaque structure)
void h1_effects_render_decals(void);

// the dynamic lights of the frame for the environment and model shaders (pixel shader constants c110 to c125)
void h1_effects_set_light_constants(void);

// the halo 2 hooks
void h1_effects_apply_patches(void);

// a halo 1 effect at a point, its "gravity" marker pointing down; markers it names that don't exist use the point
void h1_effect_new_unattached(datum h1_effect_index, const real_point3d* point, const real_vector3d* forward);

// bitmap_utilities.c rgb_colors_interpolate: flags 1 interpolate in hsv, 2 the long way around the hue
void h1_rgb_colors_interpolate(real_rgb_color* result, uint32 flags, const real_rgb_color* lower, const real_rgb_color* upper, real32 t);

// periodic_functions.c transition_function_evaluate
real32 h1_transition_function_evaluate(int16 function, real32 t);

// a halo 1 effect at the markers of an object (each location at its marker), with the first person markers of the local player's
// weapon for its first person particles
void h1_effect_new_on_object(datum h1_effect_index, datum object_index);
// effects.c effect_new_from_object with the effect's a and b scales, or effect_new_looping (the effect follows the object and
// starts again until stopped): a handle for h1_effect_stop, 0 when none was made
int32 h1_effect_new_on_object_ex(datum h1_effect_index, datum object_index, real32 scale_a, real32 scale_b, bool looping);
// effects.c effect_stop: the effect makes no more events
void h1_effect_stop(int32 id);

// effects.c effect_new_unattached_from_markers: a halo 1 effect whose locations are the markers it names (the first marker for the
// unnamed and the unknown), its damage parts dealt by the owner (halo 1's projectiles: their impacts and detonations)
void h1_effect_new_from_markers(datum h1_effect_index, datum owner_object_index, int32 marker_count, const char* const* marker_names,
	const real_point3d* marker_points, const real_vector3d* marker_forwards, real32 scale_a, real32 scale_b);
// the empty halo 2 effect a halo 2 tag refers to where a halo 1 effect plays instead (built on first use)
datum h1_effects_stub_effect_get(void);

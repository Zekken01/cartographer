#pragma once
/*
* projectiles.c on Halo 1 maps: the Halo 1 projectile logic (flight, guidance, deceleration, gravity, collisions and their material
* responses, attaching, arming, timers and detonations) run on the Halo 1 projectile tags. Halo 2's projectile object stays as the
* shell (the object, its model and attachments, networking); its own projectile update is hooked out for Halo 1 projectiles.
*/

// the halo 2 hooks
void h1_projectile_logic_apply_patches(void);
// forgets every projectile's state (new map)
void h1_projectile_logic_reset(void);
// projectiles.c projectile_new for a projectile a halo 1 weapon fired: its velocity is its forward times its initial velocity plus
// the velocity it inherits (in halo 1 world units a tick), its target the aim assist's, a tracer or not
void h1_projectile_logic_new(datum projectile_index, real32 inherited_velocity, datum target_object_index, bool tracer);
// effects.c effect_generate_parts' damage part: halo 1 damage at a point, its area of effect from the owner
void h1_projectile_logic_area_damage(datum h1_damage_effect_index, datum owner_object_index, const real_point3d* point, const real_vector3d* forward,
	real32 scale);
// projectiles.c projectile_export_function_values: false when the object isn't a halo 1 projectile or the input isn't one of its
bool h1_projectile_logic_function_value(datum object_index, int16 function_input, real32* value);

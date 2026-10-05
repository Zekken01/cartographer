#pragma once

/*
* Halo 1 physics behaviour on Halo 1 maps.
*
* Ice: Halo 1 scales the ground friction of every mass point on ice to an eighth (physics.c,
* friction_evaluate with friction_parallel_scale * 0.125 and friction_perpendicular_scale * 0.125,
* matching the 0.125 ground friction scale of the ice material). A wheeled vehicle's horizontal
* acceleration all comes from that friction, so on ice only an eighth of the velocity change of a
* vehicle update is kept: grip, braking and acceleration all drop the way they do in Halo 1.
*/

void h1_physics_apply_patches(void);

#pragma once

/*
* Halo 1 vehicle physics on havok (vehicles.c vehicle_update, physics.c physics_compute_new/physics_update_new), read from the
* halo 1 vehicle and physics tags.
*/

void h1_vehicle_physics_reset(void);

// the vehicle update of a halo 1 vehicle: its halo 1 physics set its havok velocities, false for the others
bool h1_vehicle_physics_update(datum vehicle_index);

// the vehicle object type's update runs halo 1's after halo 2's for halo 1 vehicles
void h1_vehicle_physics_apply_patches(void);

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

// the unit and vehicle function inputs (a in to d in) of a halo 1 vehicle, after its object ones; nothing for other objects
void h1_vehicle_functions_export(datum vehicle_index, real32* incoming);

// units.c unit_open and unit_close of a halo 1 vehicle: its opening or closing base animation; false for other objects
bool h1_vehicle_base_animation_set(datum vehicle_index, bool open);

// vehicles.c vehicle_hover: a halo 1 plane holds where it is (its physics stop) until it stops hovering
void h1_vehicle_hover_set(datum vehicle_index, bool hover);

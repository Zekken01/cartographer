#pragma once

/*
* Halo 1 vehicles as Halo 2 vehicles built only from Halo 1 tags.
*
* For a Halo 1 vehicle (vehi) this creates, from its model, collision model and physics:
*   mode  render model carrying the Halo 1 nodes, regions and markers (geometry is drawn by h1_objects)
*   coll  collision model from the Halo 1 collision model
*   phmo  physics model: the Halo 1 mass points as Havok spheres on one rigid body
*   hlmt  model tying them together with variants, materials and damage
*   vehi  the vehicle: object, unit (seats, camera) and vehicle (speeds, wheels, anti gravity) fields
* Wheels (powered mass points with friction) become friction points and hover pads (powered mass points
* with anti gravity) become anti gravity points, on markers placed at the mass points.
*/

// the halo 2 vehicle built from a halo 1 vehicle, NONE on failure
datum h1_vehicle_build(datum h1_vehicle_index);

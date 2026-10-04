#pragma once

/*
* Multiplayer items and vehicles for Halo 1 maps.
*
* Halo 1 item collections and vehicle placements become Halo 2 item / vehicle collections (built at
* runtime, keeping the Halo 1 weights and spawn times) that reference the closest Halo 2 multiplayer
* weapon, equipment or vehicle from the shared resource database.
*/

struct scenario;
struct h1_scnr;

void h1_equipment_build(scenario* h2_scenario, const h1_scnr* h1_scenario);

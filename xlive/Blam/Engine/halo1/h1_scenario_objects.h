#pragma once

/*
* The halo 1 scenario's object placements in campaign games (object_types_place_all, object_new_by_name).
*/

// at scenario build: the object definitions of the scenario's placements
void h1_scenario_objects_build(void);

// at the start of a campaign game: every placement that's created automatically
void h1_scenario_objects_place(void);

// object_new_by_name: the named placement's object (the existing one when it's there), NONE when it can't be made
datum h1_scenario_object_new_by_name(int16 name_index);

// the halo 1 object type of an object placed from the scenario, NONE for the others
int16 h1_scenario_object_type_get(datum object_index);

// object_delete
void h1_scenario_object_delete(datum object_index);

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

// the halo 2 animation graph of a halo 1 animation graph the scripts play (built at scenario build when build is set)
datum h1_scenario_animation_graph_get(datum h1_animation_graph_index, bool build);

// the halo 1 object type of an object placed from the scenario, NONE for the others
int16 h1_scenario_object_type_get(datum object_index);

// the halo 2 definition of a halo 1 object tag (built the first time), NONE when it can't be built
datum h1_scenario_object_definition_get(datum h1_definition_index);

// an object made from a halo 1 tag outside the scenario's placements (the AI's units): its halo 1 type
void h1_scenario_object_type_set(datum object_index, datum h1_definition_index);

// object_delete
void h1_scenario_object_delete(datum object_index);

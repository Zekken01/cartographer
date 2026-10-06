#pragma once

/*
* Halo 1 scripting: the halo 1 scenario's compiled scripts, run by a port of halo 1's script runtime (hs_runtime.c) in campaign
* games of halo 1 maps.
*/

// hs_initialize_for_new_map: loads the scenario's scripts, evaluates its globals and starts its scripts' threads
void h1_hs_initialize_for_new_map(void);
void h1_hs_dispose_from_old_map(void);

// whether halo 1's scripts are running
bool h1_hs_running(void);

// hs_update: runs the threads due this tick, once per game tick
void h1_hs_update(void);

// wakes the script with this name
bool h1_hs_wake_by_name(const char* name);

// the object a scenario object name names (object_create, the scenario's placements), for the scripts
void h1_hs_object_name_set(int16 name_index, datum object_index);
// the object a scenario object name names, NONE when there isn't one
datum h1_hs_object_index_from_name_index(int16 name_index);

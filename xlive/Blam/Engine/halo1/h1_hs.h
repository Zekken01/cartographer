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

// game_sound.c scripted_sound_new and scripted_sound_time (the AI's conversations start theirs here too)
void h1_hs_sound_impulse_start(datum sound_index, datum object_index, real32 scale);
int32 h1_hs_sound_impulse_time(datum sound_index);
// sound_manager.c: a sound started, scripted dialog (its classes) plays until it ends (and a third of a second)
void h1_hs_sound_dialog_note(datum sound_index);
bool h1_hs_scripted_dialog_is_playing(void);

// the scripts' custom_animation (a halo 1 graph's animation by name on a unit or scenery) and its ticks left
bool h1_hs_custom_animation(datum object_index, datum h1_graph_index, const char* name, bool interpolate);
int16 h1_hs_animation_time(datum object_index);
// a unit's own graph's animation by name (halo 1's unit animations: the AI's impulses)
bool h1_hs_unit_animation_play(datum unit_index, const char* name, bool interpolate);

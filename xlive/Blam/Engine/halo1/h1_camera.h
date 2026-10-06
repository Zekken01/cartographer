#pragma once

/*
* Halo 1's scripted camera (camera_scripting.c): camera_control, camera_set, camera_set_relative, camera_time.
*/

struct s_observer_result;

void h1_camera_reset(void);

void h1_camera_control(bool enabled);
// to a scenario camera point over the transition (ticks), relative to an object's center when there's one
void h1_camera_set(int16 camera_point_index, int16 transition_ticks, datum relative_object_index);
void h1_camera_set_first_person(datum unit_index);
// camera_control is on: the director's perspective is scripted (its first person camera too)
bool h1_camera_scripted(void);
// the ticks left of the transition
int16 h1_camera_time(void);

// observer_update: the scripted camera replaces the observer's result while it's in control, true when it did
bool h1_camera_observer_override(int32 user_index, real32 dt, s_observer_result* result);

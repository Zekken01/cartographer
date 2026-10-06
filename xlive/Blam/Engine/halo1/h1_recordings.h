#pragma once

/*
* Halo 1 recorded animations (recorded unit control playback): recording_play, recording_kill, recording_time.
*/

void h1_recordings_reset(void);

// recorded_animation_play(_and_delete): the scenario's recorded animation drives the unit, false when it can't
bool h1_recording_play(datum unit_index, int16 animation_index, bool delete_on_complete, bool hover_on_complete = false);
void h1_recording_kill(datum unit_index);
// the ticks left of the unit's recording, 0 without one
int16 h1_recording_time(datum unit_index);
bool h1_recording_controlling_unit(datum unit_index);

// recorded_animations_update, once per game tick
void h1_recordings_update(void);

#pragma once

/*
* The halo 1 AI port's mirrors of carto's objects (h1_ai_objects.cpp).
*/

namespace h1_ai
{

// halo 1's game time: halo 2's in halo 1 ticks (30 a second)
int32 h1_ai_game_time(void);

void h1_ai_objects_initialize_for_new_map(void);
// at the start of an AI tick: every object's mirror filled from halo 2
void h1_ai_objects_update(void);
// an object halo 2 deleted
void h1_ai_object_mirror_forget(datum object_index);
// at the end of an AI tick: unit_update's control of the units halo 1 controls, to halo 2 (h1_ai_units.cpp)
void h1_ai_units_control_update(void);
void h1_ai_unit_control_update(long unit_index, struct unit_datum* unit, bool* controlled_last);

} // namespace h1_ai

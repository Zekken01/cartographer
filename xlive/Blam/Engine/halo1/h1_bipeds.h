#pragma once

/*
* Halo 2 bipeds built from halo 1 bipeds (tag translation: halo 2 simulates and animates them, the halo 1 renderer draws them).
*/

// the halo 2 biped of a halo 1 biped, NONE when it can't be built
datum h1_biped_definition_build(datum h1_biped_index);

// the halo 1 globals' player unit becomes every player representation's unit
void h1_bipeds_build_player(void);

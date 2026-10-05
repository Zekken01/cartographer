#pragma once

/*
* Halo 1 scenery placed by the scenario: the Halo 2 instanced geometry built from its collision model (h1_structure_bsp)
* stops projectiles, and its Halo 1 damage (damage.c: shields, body, stun, recharge, the shield regions that go missing),
* function values and change colors (objects.c) are kept here and drawn by h1_render.
*/

// damage_definitions.h damage region flags
enum
{
	_h1_region_missing_when_shield_is_zero_bit = 4,
};

// forgets every placement (a new map is being built)
void h1_scenery_reset(void);

// the instanced geometry instance built for a placement: its definition, the definition without the regions missing while the
// shield is down (NONE if there is none) and the first collision material of its collision model
void h1_scenery_instance_register(int32 instance_index, int32 placement_index, int16 definition_index, int16 shield_off_definition_index, int16 material_offset);

// damage, recharge and functions (once per frame)
void h1_scenery_update(void);

// the halo 2 hooks (breakable surface and area of effect damage)
void h1_scenery_apply_patches(void);

// how a placement draws: its function values, change colors and the permutation of each model region (NULL when it has none)
void h1_scenery_render_state(int32 placement_index, const real32** out_function_values, const real_rgb_color** out_change_colors, const int16** out_region_permutations);

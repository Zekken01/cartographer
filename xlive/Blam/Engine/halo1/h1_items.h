#pragma once

/*
* Halo 1 equipment (powerups, health packs, grenades, ammunition) as Halo 2 equipment built only
* from the Halo 1 equipment tag, its model and collision model. The Halo 1 powerup type, grenade type
* and powerup time carry over unchanged (Halo 2 kept the Halo 1 powerup enum: health packs heal).
* The Halo 1 model is drawn by h1_objects.
*/

// the halo 2 equipment built from a halo 1 equipment tag, NONE on failure
datum h1_equipment_definition_build(datum h1_equipment_index);

// the items' pickup sounds play the halo 1 sounds their tags carry
void h1_items_apply_patches(void);

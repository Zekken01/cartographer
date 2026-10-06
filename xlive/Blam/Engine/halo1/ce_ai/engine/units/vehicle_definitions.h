/*
VEHICLE_DEFINITIONS.H

header included in hcex build.
*/

#ifndef __VEHICLE_DEFINITIONS_H
#define __VEHICLE_DEFINITIONS_H
#pragma once

namespace h1_ai
{

/* ---------- headers */


/* ---------- constants */

enum
{
	VEHICLE_DEFINITION_TAG = 'vehi'
};

enum
{
	_vehicle_causes_collision_damage_bit = 7,
};

/* ---------- macros */

#undef vehicle_definition_get
#define vehicle_definition_get(index) ((struct unit_definition *)tag_get(VEHICLE_DEFINITION_TAG, index))
#undef vehicle_specific_definition_get
#define vehicle_specific_definition_get(index) ((struct vehicle_definition *)tag_get(VEHICLE_DEFINITION_TAG, index))

/* ---------- structures */

struct vehicle_definition;

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

/* ---------- public code */

} // namespace h1_ai

#endif // __VEHICLE_DEFINITIONS_H

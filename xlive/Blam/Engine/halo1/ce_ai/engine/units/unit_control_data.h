/*
UNIT_CONTROL_DATA.H
*/

#ifndef __UNIT_CONTROL_DATA_H
#define __UNIT_CONTROL_DATA_H
#pragma once

namespace h1_ai
{

/* ---------- headers */


/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct unit_control_data
{
	char animation_state;
	char aiming_speed;
	word control_flags;
	short weapon_index;
	short grenade_index;
	short zoom_level;
	word pad;
	real_vector3d throttle;
	real primary_trigger;
	real_vector3d facing_vector;
	real_vector3d aiming_vector;
	real_vector3d looking_vector;
};

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

} // namespace h1_ai

#endif // __UNIT_CONTROL_DATA_H

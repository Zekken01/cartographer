#pragma once
#include "h1_render_shaders.h"

/*
* Halo 1 objects built from Halo 1 tags.
*
* Halo 1 vehicles become Halo 2 definitions made only from their Halo 1 tags (h1_vehicles), with a
* render model that carries the Halo 1 nodes and markers but no geometry. Halo 2 simulates them and
* computes their (Halo 1) node matrices; the Halo 1 model is drawn here with its own shaders.
*/

// forgets every binding (a new map is being built)
void h1_objects_reset(void);

// objects with the halo 2 definition draw the halo 1 definition's model
void h1_objects_bind(datum h2_definition_index, datum h1_definition_index);

// halo 2 definitions bound so far (for the simulation definition table)
int32 h1_objects_bound_definitions(datum* out_definitions, int32 maximum_count);

// the halo 1 definition a halo 2 definition was built from, NONE if it wasn't
datum h1_objects_h1_definition_get(datum h2_definition_index);
// a local player's first person unit or what it holds (halo 2 doesn't draw them)
bool h1_object_held_in_first_person(datum object_index);

// true for halo 2 definitions built from halo 1 objects
bool h1_objects_definition_bound(datum definition_index);

// true when the object draws its halo 1 model instead of a halo 2 render model
bool h1_objects_render_replaced(datum object_index);

// an object halo 2 renders this frame (render_lod_new object_build_render_cache_and_info): only those are drawn, which
// leaves out the culled ones and the first person player's unit and weapons
void h1_objects_render_submit(datum object_index, bool first_person);

// the objects submitted since the last call become the ones drawn (each frame before the halo 1 objects draw)
void h1_objects_render_frame_begin(void);

// objects.h object function modes (the a in to d in of an object definition)
enum
{
	_h1_object_function_none = 0,
	_h1_object_function_body_vitality,
	_h1_object_function_shield_vitality,
	_h1_object_function_recent_body_damage,
	_h1_object_function_recent_shield_damage,
	_h1_object_function_random_constant,
	_h1_object_function_umbrella_shield_vitality,
	_h1_object_function_shield_stun,
	_h1_object_function_recent_umbrella_shield_vitality,
	_h1_object_function_umbrella_shield_stun,
	_h1_object_function_first_region_damage,
	_h1_object_function_last_region_damage = 17,
	_h1_object_function_alive,
	_h1_object_function_compass,
};

// weapon_export_function_mode.h
enum
{
	_h1_weapon_function_none = 0,
	_h1_weapon_function_heat,
	_h1_weapon_function_primary_ammunition,
	_h1_weapon_function_secondary_ammunition,
	_h1_weapon_function_primary_rate_of_fire,
	_h1_weapon_function_secondary_rate_of_fire,
	_h1_weapon_function_ready,
	_h1_weapon_function_primary_ejection_port,
	_h1_weapon_function_secondary_ejection_port,
	_h1_weapon_function_overheated,
	_h1_weapon_function_primary_charged,
	_h1_weapon_function_secondary_charged,
	_h1_weapon_function_illumination,
	_h1_weapon_function_age,
	_h1_weapon_function_integrated_light,
	_h1_weapon_function_primary_firing,
	_h1_weapon_function_secondary_firing,
};

// object_definitions.h function flags, bounds modes and runtime flags
enum
{
	_h1_object_function_invert_bit = 0,
	_h1_object_function_additive_bit,
	_h1_object_function_does_not_deactivate_below_lower_bound_bit,
};
enum
{
	_h1_object_function_clip_to_bounds = 0,
	_h1_object_function_clip_to_bounds_and_normalize,
	_h1_object_function_scale_to_fit_bounds,
};
enum
{
	_h1_object_runtime_scaled_change_colors_bit = 0,
};

// what an object's function inputs come from
struct s_h1_object_vitality
{
	real32 body_vitality;
	real32 shield_vitality;
	real32 current_body_damage;
	real32 current_shield_damage;
	bool dead;
	// the compass input's forward (node 0's in halo 1), none when has_forward is false
	bool has_forward;
	real_vector3d forward;
};

// objects.c: an object's incoming and outgoing function values, which are active, and its change colors
struct s_h1_object_functions
{
	real32 incoming[4];
	real32 outgoing[4];
	uint32 active_flags;
	real_rgb_color base_colors[4];
	real_rgb_color colors[4];
};

// a new object's functions: zero, with the change colors it gets where it was created
void h1_object_functions_new(datum h1_definition_index, const real_point3d* position, s_h1_object_functions* functions);

// objects.c object_export_function_values: the incoming values from the object's vitality and damage
void h1_object_functions_export(datum h1_definition_index, const s_h1_object_vitality* vitality, s_h1_object_functions* functions);

// objects.c object_compute_function_values and object_compute_change_colors (absolute index: the object's datum index)
void h1_object_functions_update(datum h1_definition_index, int32 absolute_index, s_h1_object_functions* functions);

// the functions of every bound object from its vitality, damage and (weapons) weapon state (once per frame)
void h1_objects_update_functions(void);

// an object's current functions, NULL when it isn't a halo 1 object
const s_h1_object_functions* h1_object_functions_get(datum object_index);

// objects.c object_get_function_value: an outgoing function value (function index NONE: 1), false when the function is off
bool h1_object_function_value_get(datum object_index, int16 function_index, real32* out_value);

// an object's base change colors (the AI's actor variants set theirs)
void h1_object_change_colors_set(datum object_index, const real_rgb_color colors[4]);

// objects.c object_choose_random_change_colors: the four change colors of a halo 1 object created at a position
void h1_object_change_colors_choose(datum h1_definition_index, const real_point3d* position, real_rgb_color out_colors[4]);

// draws the halo 1 models of every bound object
void h1_objects_render(e_h1_render_pass pass, real32 game_time);

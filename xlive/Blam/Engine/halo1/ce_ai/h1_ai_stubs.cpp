#include "stdafx.h"
#include "h1_ai_internal.h"

namespace h1_ai
{

// placeholders: tools/gen_ai_stubs.py (the port implements these over carto)

struct ai_debug_state ai_debug;
void ai_debug_actor_deleted(long actor_index)
{
	return;
}

char * ai_debug_describe_actor(long actor_index, long unit_index, boolean include_squad, char *buffer, long bufsize)
{
	return NULL;
}

void ai_debug_dispose(void)
{
	return;
}

void ai_debug_dispose_from_old_map(void)
{
	return;
}

struct path_debug_storage * ai_debug_get_path_storage(long actor_index)
{
	return NULL;
}

void ai_debug_idle_look_addprop(long prop_index, real weight)
{
	return;
}

void ai_debug_idle_look_clear(long unit_index)
{
	return;
}

void ai_debug_initialize(void)
{
	return;
}

void ai_debug_initialize_for_new_map(void)
{
	return;
}

void ai_debug_lineoffire_addpill(real_point3d const *base, real_vector3d const *directedheight, real width, boolean hit)
{
	return;
}

void ai_debug_lineoffire_new(real_point3d const *origin, real_vector3d const *vector)
{
	return;
}

void ai_debug_lineoffire_success(boolean success)
{
	return;
}

void ai_debug_lineofsight(real_point3d const *start, short start_key, real_point3d const *end, short end_key)
{
	return;
}

void ai_debug_select_actor(long encounter_index, long actor_index)
{
	return;
}

void ai_debug_select_encounter(long encounter_index)
{
	return;
}

void ai_debug_update(void)
{
	return;
}

struct ai_globals *ai_globals;
struct ai_profile_globals ai_profile;
void ai_profile_dispose(void)
{
	return;
}

void ai_profile_dispose_from_old_map(void)
{
	return;
}

void ai_profile_initialize(void)
{
	return;
}

void ai_profile_initialize_for_new_map(void)
{
	return;
}

void ai_profile_update(void)
{
	return;
}

void biped_accelerate(long biped_index, real_vector3d *acceleration)
{
	return;
}

boolean biped_fix_position(long biped_index, long line_of_sight_object_index, real_point3d const *new_position, real_point3d *final_position, real maximum_radius_fudge_factor, boolean fix_below_new_position, boolean dont_teleport, boolean use_radius_as_multiplier)
{
	return FALSE;
}

byte * breakable_surface_flags_get(void)
{
	return NULL;
}

cheat_globals cheat;

struct data_array *conversation_data;
boolean debug_ignore_broken_surfaces;
boolean debug_obstacle_path_finishing;
real_point3d debug_obstacle_path_goal_point;
long debug_obstacle_path_goal_surface_index;
real debug_obstacle_path_radius;
real_point3d debug_obstacle_path_start_point;
long debug_obstacle_path_start_surface_index;

boolean hs_wake_by_name(char const* name)
{
	return FALSE;
}

void object_compute_node_matrices_recursive(long object_index)
{
	return;
}


boolean object_mark_function(long object_index)
{
	return FALSE;
}

void object_marker_begin(void)
{
	return;
}

void object_marker_end(void)
{
	return;
}

void object_reset(long object_index)
{
	return;
}

struct data_array *prop_data;
boolean recorded_animation_controlling_unit(long unit_index)
{
	return FALSE;
}

boolean recorded_animation_play(long unit_index, short animation_index)
{
	return FALSE;
}

short scenario_get_animation_by_name(struct scenario const* scenario, char const* name)
{
	return 0;
}

void scripted_sound_new(long definition_index, long source_object_index, real scale)
{
	return;
}

long scripted_sound_time(long sound_index)
{
	return 0;
}

void unit_detach_from_parent(long unit_index)
{
	return;
}

boolean unit_enter_seat(long unit_index, long target_unit_index, short seat_index)
{
	return FALSE;
}

short unit_get_animation_frames_remaining(long unit_index, short *animation_state)
{
	return 0;
}

boolean unit_is_speaking(long unit_index)
{
	return FALSE;
}

boolean unit_leap_begin(long unit_index, real_vector2d const *alignment_vector)
{
	return FALSE;
}

boolean unit_melee_attack_begin(long unit_index, boolean continuous, real_vector2d const *alignment_vector)
{
	return FALSE;
}

boolean unit_scream(long unit_index, short scream_type)
{
	return FALSE;
}

void unit_speak(long unit_index, short play_type, struct unit_speech_item const *speech_item)
{
	return;
}

boolean unit_start_animation_impulse(long unit_index, short animation_impulse, real_vector2d *alignment_vector)
{
	return FALSE;
}

boolean unit_start_user_animation(long unit_index, long animation_graph_index, char const *animation_name, boolean interpolate)
{
	return FALSE;
}

boolean unit_test_animation_impulse(long unit_index, short animation_impulse)
{
	return FALSE;
}

short unit_test_speech(long unit_index, short priority, boolean allow_recursive_lookup, boolean allow_queue, long *unit_last_speech_time, short *vocalization_type_reference, long *sound_definition_index_reference)
{
	return 0;
}

boolean unit_try_and_exit_seat(long unit_index)
{
	return FALSE;
}

boolean vehicle_stuck(long vehicle_index, real_vector3d *direction)
{
	return FALSE;
}

void weapon_set_current_amount(long weapon_index, real current_amount)
{
	return;
}

void weapon_set_total_rounds(long weapon_index, short *rounds_array)
{
	return;
}

boolean sound_scripted_dialog_is_playing(void)
{
	return FALSE;
}

} // namespace h1_ai

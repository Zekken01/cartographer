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

long biped_approximate_surface_index(long biped_index, real_point3d *point)
{
	return NONE;
}

void biped_build_flying_axes(real_vector3d const *forward_vector, real_vector3d *left_vector, real_vector3d *up_vector)
{
	return;
}

long biped_find_pathfinding_surface_index(long biped_index, real_point3d *pathfinding_point)
{
	return NONE;
}

boolean biped_fix_position(long biped_index, long line_of_sight_object_index, real_point3d const *new_position, real_point3d *final_position, real maximum_radius_fudge_factor, boolean fix_below_new_position, boolean dont_teleport, boolean use_radius_as_multiplier)
{
	return FALSE;
}

void biped_get_physics_pill(long biped_index, real_point3d *base, real *height, real *width)
{
	return;
}

byte * breakable_surface_flags_get(void)
{
	return NULL;
}

cheat_globals cheat;

boolean collision_bsp_test_vector(unsigned long flags, struct collision_bsp const *bsp, short breakable_surface_count, byte const *breakable_surface_flags, real_point3d const *point, real_vector3d const *vector, real maximum_t, struct collision_bsp_test_vector_result *result)
{
	return FALSE;
}

boolean collision_surface_find_closest_point2d(struct collision_bsp const *bsp, long surface_index, short projection, boolean sign, real_point2d const *point, real_point2d *result)
{
	return FALSE;
}

real_point3d * collision_surface_project_point2d(struct collision_bsp const *bsp, long surface_index, short projection, boolean sign, real_point2d const *point, real_point3d *result)
{
	return NULL;
}

boolean collision_surface_test_line2d(struct collision_bsp const *bsp, long surface_index, short projection, boolean sign, real_point2d const *point, real_vector2d const *direction, struct collision_surface_test_line2d_result *result)
{
	return FALSE;
}

boolean collision_test_vector(unsigned long flags, real_point3d const *point, real_vector3d const *vector, long ignore_object_index, struct collision_result *collision)
{
	return FALSE;
}

struct data_array *conversation_data;
long datum_new_at_index(struct data_array* data, long index)
{
	return NONE;
}

boolean debug_ignore_broken_surfaces;
boolean debug_obstacle_path_finishing;
real_point3d debug_obstacle_path_goal_point;
long debug_obstacle_path_goal_surface_index;
real debug_obstacle_path_radius;
real_point3d debug_obstacle_path_start_point;
long debug_obstacle_path_start_surface_index;
short global_current_collision_user_depth;
short global_current_collision_users[MAXIMUM_COLLISION_USER_STACK_DEPTH];

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

boolean projectile_aim(struct projectile_definition const *definition, real_point3d const *origin, real_point3d const *target_point, real const *override_velocity_max, real *target_velocity_min, real *target_ballistic_fraction_min, real *forced_velocity, boolean lob, real_vector3d *result_aim_vector, real *result_velocity, real *result_ticks, real *result_distance, boolean *result_linear)
{
	return FALSE;
}

boolean projectile_aim_ballistic(real base_velocity, real gravity_scale, real_point3d const *origin, real_point3d const *target_point, real *target_velocity_min, real *target_ballistic_fraction_min, real *forced_velocity, boolean lob, real_vector3d *result_aim_vector, real *result_velocity, real *result_ticks, real *result_distance, real *result_vertical_velocity, real *result_horizontal_velocity)
{
	return FALSE;
}

real projectile_get_ballistic_acceleration(struct projectile_definition const *definition)
{
	return 0.f;
}

struct data_array *prop_data;
void qsort_4byte(long* elements, unsigned long element_count, boolean(*compare)(long, long))
{
	return;
}

boolean recorded_animation_controlling_unit(long unit_index)
{
	return FALSE;
}

boolean recorded_animation_play(long unit_index, short animation_index)
{
	return FALSE;
}

real scenario_fog_at_point(const struct location* viewer_location, const real_point3d* viewer_point, const real_point3d* point)
{
	return 0.f;
}

short scenario_get_animation_by_name(struct scenario const* scenario, char const* name)
{
	return 0;
}

boolean scenario_location_deafening(const struct location* location)
{
	return FALSE;
}

boolean scenario_location_underwater(const struct location* location, const real_point3d* position, short* optional_weather_palette_index)
{
	return FALSE;
}

boolean scenario_test_pvs(short cluster_index0, short cluster_index1)
{
	return FALSE;
}

void scripted_sound_new(long definition_index, long source_object_index, real scale)
{
	return;
}

long scripted_sound_time(long sound_index)
{
	return 0;
}

byte structure_bsp_get_cluster_encoded_sound_distance(struct structure_bsp* structure_bsp, short from_cluster_index, short to_cluster_index)
{
	return 0;
}

unsigned long* structure_bsp_get_cluster_pvs(struct structure_bsp* structure_bsp, short cluster_index)
{
	return NULL;
}

boolean unit_can_see_point(long unit_index, real_point3d const *point, real field_of_view)
{
	return FALSE;
}

boolean unit_clip_to_aiming_bounds(long unit_index, real_vector3d *vector, boolean use_aiming_screen)
{
	return FALSE;
}

void unit_control(long unit_index, struct unit_control_data const *control_data)
{
	return;
}

boolean unit_controllable(long unit_index)
{
	return FALSE;
}

void unit_detach_from_parent(long unit_index)
{
	return;
}

boolean unit_enter_seat(long unit_index, long target_unit_index, short seat_index)
{
	return FALSE;
}

void unit_estimate_position(long unit_index, short estimate_mode,  real_point3d const *body_position, real_vector3d *desired_facing, real_vector3d *desired_gun_offset, real_point3d *estimated_position)
{
	return;
}

boolean unit_flying_through_air(long unit_index)
{
	return FALSE;
}

void unit_get_aiming_vector(long unit_index, real_vector3d *aiming_vector)
{
	return;
}

short unit_get_animation_frames_remaining(long unit_index, short *animation_state)
{
	return 0;
}

void unit_get_camera_position(long unit_index, real_point3d *camera_position)
{
	return;
}

void unit_get_center_of_mass(long unit_index, real_point3d *center_of_mass)
{
	return;
}

short unit_get_current_grenade_type(long unit_index)
{
	return 0;
}

void unit_get_facing_vector(long unit_index, real_vector3d *facing_vector)
{
	return;
}

short unit_get_grenade_count(long unit_index, short grenade_type)
{
	return 0;
}

void unit_get_head_position(long unit_index, union real_point3d *head_position)
{
	return;
}

void unit_get_looking_vector(long unit_index, real_vector3d *looking_vector)
{
	return;
}

boolean unit_get_melee_range_and_ticks(long unit_index, boolean secondary, short *melee_tick, real *attack_time, short *frame_count, real *damage_time)
{
	return FALSE;
}

boolean unit_get_seat_entrance_point(long unit_index, long parent_unit_index, short seat_index, real_point3d *entrance_point, real_point3d *seat_point, real_point3d *hint_point)
{
	return FALSE;
}

boolean unit_has_animation_to_enter_seat(long unit_index, long target_unit_index, short seat_index)
{
	return FALSE;
}

long unit_inventory_get_weapon(long unit_index, short index)
{
	return 0;
}

boolean unit_is_busy(long object_index)
{
	return FALSE;
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

void unit_persistent_control(long unit_index, long control_ticks, unsigned long persistent_control_flags)
{
	return;
}

boolean unit_scream(long unit_index, short scream_type)
{
	return FALSE;
}

boolean unit_seat_allow_noncombatants(long unit_index, short seat_index)
{
	return FALSE;
}

boolean unit_seat_filled(long unit_index, short seat_index)
{
	return FALSE;
}

boolean unit_seat_is_driver(long unit_index, short seat_index)
{
	return FALSE;
}

boolean unit_seat_is_gunner(long unit_index, short seat_index)
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

void unit_start_running_blindly(long unit_index)
{
	return;
}

boolean unit_start_user_animation(long unit_index, long animation_graph_index, char const *animation_name, boolean interpolate)
{
	return FALSE;
}

void unit_stop_running_blindly(long unit_index)
{
	return;
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

boolean vehicle_causes_collision_damage(long vehicle_index)
{
	return FALSE;
}

long vehicle_find_pathfinding_surface_index(long vehicle_index, real_point3d *position)
{
	return NONE;
}

short vehicle_scripting_find_available_seats(long vehicle_index, char const *seat_substring_name, short seat_desire_type, short *seat_indices, short maximum_seat_count)
{
	return 0;
}

boolean vehicle_stuck(long vehicle_index, real_vector3d *direction)
{
	return FALSE;
}

boolean weapon_aim(long weapon_index, short trigger_index, real_point3d const *origin, real_point3d const *target_point, boolean lob, real_vector3d *result_aim_vector, real *result_ticks, real *result_distance, boolean *result_linear)
{
	return FALSE;
}

real weapon_definition_get_damage_potential(long weapon_definition_index, real *rate_of_fire)
{
	return 0.f;
}

real weapon_estimate_time_to_target(long weapon_index, short trigger_index, real distance)
{
	return 0.f;
}

void weapon_set_current_amount(long weapon_index, real current_amount)
{
	return;
}

void weapon_set_total_rounds(long weapon_index, short *rounds_array)
{
	return;
}

char const *dialogue_get_vocalization_name(short vocalization_type, boolean abbreviated)
{
	return "";
}

char const *unit_get_speech_priority_name(short priority)
{
	return "";
}

char const *unit_describe_speech(long unit_index, boolean abbreviated, long buffer_size, char *buffer)
{
	return "";
}

boolean sound_scripted_dialog_is_playing(void)
{
	return FALSE;
}

} // namespace h1_ai

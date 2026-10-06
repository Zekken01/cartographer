#pragma once

/*
* The halo 1 engine functions the AI port calls beyond its own sources, after halo 1's engine types: h1_ai_engine*.cpp
* implement them over carto (halo 2's objects) and the halo 1 cache file.
*/

namespace h1_ai
{

// game_sound.c: an impulse sound from an object's node (its handle, NONE when it doesn't play)
long object_impulse_sound_new(long object_index, long sound_definition_index, short node_index, real_point3d const* position, real_vector3d const* forward, real scale);

// structures.c, scenario.c
unsigned long* structure_bsp_get_cluster_pvs(struct structure_bsp* structure_bsp, short cluster_index);
byte structure_bsp_get_cluster_encoded_sound_distance(struct structure_bsp* structure_bsp, short from_cluster_index, short to_cluster_index);
boolean scenario_test_pvs(short cluster_index0, short cluster_index1);
boolean scenario_location_underwater(const struct location* location, const real_point3d* position, short* optional_weather_palette_index);
boolean scenario_location_deafening(const struct location* location);
real scenario_fog_at_point(const struct location* viewer_location, const real_point3d* viewer_point, const real_point3d* point);
void scenario_location_from_point(struct location* location, const real_point3d* point);
struct game_globals* scenario_get_game_globals(void);
short scenario_get_animation_by_name(struct scenario const* scenario, char const* name);

// game_sound.c, recorded_animations.c, hs, players
void scripted_sound_new(long definition_index, long source_object_index, real scale);
long scripted_sound_time(long sound_index);
boolean recorded_animation_play(long unit_index, short animation_index);
boolean recorded_animation_controlling_unit(long unit_index);
boolean hs_wake_by_name(char const* name);
boolean player_input_enabled(void);

// color math, sort, data
union real_rgb_color* rgb_colors_interpolate(union real_rgb_color* rgb_result, unsigned long flags, union real_rgb_color const* rgb_lower_bound,
	union real_rgb_color const* rgb_upper_bound, real u);
void qsort_4byte(long* elements, unsigned long element_count, boolean(*compare)(long, long));
long datum_new_at_index(struct data_array* data, long index);

// objects.c
void object_initialize_vitality(long object_index, real* custom_body_vitality, real* custom_shield_vitality);

} // namespace h1_ai

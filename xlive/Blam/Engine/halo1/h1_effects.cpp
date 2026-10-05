#include "stdafx.h"
#include "h1_effects.h"

#include "h1_bitmaps.h"
#include "h1_cache_file.h"
#include "h1_first_person.h"
#include "h1_fog.h"
#include "h1_log.h"
#include "h1_map_loader.h"
#include "h1_objects.h"
#include "h1_render.h"
#include "h1_render_shaders.h"
#include "h1_runtime.h"
#include "h1_sound.h"
#include "h2_tag_definitions_generated.h"

#include "game/game.h"
#include "objects/objects.h"
#include "physics/collisions.h"
#include "rasterizer/dx9/rasterizer_dx9_main.h"
#include "render/render.h"
#include "tag_files/tag_groups.h"

#include <algorithm>
#include <vector>

/* constants */

enum
{
	k_h1_maximum_effects = 256,
	k_h1_maximum_particles = 1024,
	k_h1_maximum_effect_events_per_update = 8,
	k_h1_maximum_effect_instances = 16,
	k_h1_maximum_split_screen_particle_count = 6,
	k_h1_maximum_point_physics_collisions = 3,
	k_h1_maximum_particles_per_event = 32,
	k_h1_maximum_particle_systems = 64,
	k_h1_maximum_system_particles = 1024,
	k_h1_maximum_sprites = 4096,
	k_h1_maximum_lights = 64,
	k_h1_maximum_shader_lights = 8,
	k_h1_maximum_attachments = 512,
	k_h1_maximum_attachment_markers = 4,
	k_h1_maximum_decals = 256,
	k_h1_maximum_decal_vertices = 1536,
};

// light_definitions.h
enum
{
	_h1_light_dynamic_bit = 0,
	_h1_light_no_specular_bit,
	_h1_light_dont_light_own_object_bit,
};

// decal_definitions.h
enum
{
	_h1_decal_geometry_inherited_by_next_decal_in_chain_bit = 0,
	_h1_decal_color_interpolate_in_hsv_bit = 1,
	_h1_decal_no_random_rotation_bit = 3,
	_h1_decal_preserve_aspect_bit = 8,
};

// lens_flare_definitions.h
enum
{
	_h1_lens_reflection_rotate_from_center_of_screen_bit = 0,
	_h1_lens_reflection_radius_not_scaled_by_distance_bit,
	_h1_lens_reflection_radius_scaled_by_occlusion_bit,
};

enum
{
	_h1_bitmap_type_sprites = 3,
};

// particle_system_definitions.h
enum
{
	_h1_pctl_type_type_states_loop_bit = 0,
	_h1_pctl_type_type_states_loop_forward_backward_bit,
	_h1_pctl_type_particle_states_loop_bit,
	_h1_pctl_type_particle_states_loop_forward_backward_bit,
	_h1_pctl_type_dies_in_water_bit,
	_h1_pctl_type_dies_in_air_bit,
	_h1_pctl_type_dies_on_ground_bit,
	_h1_pctl_type_rotational_sprites_animate_sideways_bit,
	_h1_pctl_type_disabled_bit,
	_h1_pctl_type_tint_by_effect_color_bit,
	_h1_pctl_type_initial_count_scales_bit,
	_h1_pctl_type_minimum_count_scales_bit,
	_h1_pctl_type_creation_rate_scales_bit,
	_h1_pctl_type_scale_scales_bit,
	_h1_pctl_type_animation_rate_scales_bit,
	_h1_pctl_type_rotation_rate_scales_bit,
};

// effects.c effect camera modes (the create field of effect particles)
enum
{
	_h1_effect_camera_mode_independent = 0,
	_h1_effect_camera_mode_first_person_only,
	_h1_effect_camera_mode_third_person_only,
	_h1_effect_camera_mode_both,
};

// effects.c
enum
{
	_h1_effect_part_world_down_bit = 0,
	_h1_effect_part_type_scale_bit = 5,

	_h1_effect_particle_attached_bit = 0,
	_h1_effect_particle_random_orientation_bit,
	_h1_effect_particle_tint_from_change_color_bit,
	_h1_effect_particle_tint_interpolate_hsv_bit,
	_h1_effect_particle_tint_do_it_the_hard_way_bit,

	_h1_effect_velocity_bit = 0,
	_h1_effect_velocity_delta_bit,
	_h1_effect_velocity_cone_bit,
	_h1_effect_angular_velocity_bit,
	_h1_effect_angular_velocity_delta_bit,
	_h1_effect_particle_count_bit,
	_h1_effect_particle_count_delta_bit,
	_h1_effect_particle_distribution_radius_bit,
	_h1_effect_particle_distribution_radius_delta_bit,
	_h1_effect_particle_radius_bit,
	_h1_effect_particle_radius_delta_bit,
	_h1_effect_particle_tint_bit,
};

enum
{
	_h1_effect_environment_anywhere = 0,
	_h1_effect_environment_air,
	_h1_effect_environment_water,
	_h1_effect_environment_vacuum,
};

enum
{
	_h1_effect_disposition_agnostic = 0,
	_h1_effect_disposition_violent,
	_h1_effect_disposition_nonviolent,
};

enum
{
	_h1_effect_distribution_start = 0,
	_h1_effect_distribution_end,
	_h1_effect_distribution_constant,
	_h1_effect_distribution_buildup,
	_h1_effect_distribution_falloff,
	_h1_effect_distribution_quadratic,
};

// particles.c
enum
{
	_h1_particle_animates_backwards_bit = 0,
	_h1_particle_at_rest_bit,
	_h1_particle_u_mirror_bit,
	_h1_particle_v_mirror_bit,
};

enum
{
	_h1_particle_definition_can_animate_backwards_bit = 0,
	_h1_particle_definition_animation_stops_at_rest_bit,
	_h1_particle_definition_animation_starts_on_random_frame_bit,
	_h1_particle_definition_animate_once_per_frame_bit,
	_h1_particle_definition_dies_at_rest_bit,
	_h1_particle_definition_dies_on_contact_with_structure_bit,
	_h1_particle_definition_tint_from_diffuse_texture_bit,
	_h1_particle_definition_dies_on_contact_with_water_bit,
	_h1_particle_definition_dies_on_contact_with_air_bit,
	_h1_particle_definition_self_illuminated_bit,
	_h1_particle_definition_random_horizontal_mirroring_bit,
	_h1_particle_definition_random_vertical_mirroring_bit,
};

enum
{
	_h1_particle_state_next_sequence_initial = 0,
	_h1_particle_state_next_sequence_looping,
	_h1_particle_state_still_looping,
	_h1_particle_state_next_sequence_final,
};

// point_physics.c
enum
{
	_h1_point_physics_flamethrower_collision_bit = 0,
	_h1_point_physics_structure_collisions_bit,
	_h1_point_physics_water_collisions_bit,
	_h1_point_physics_simple_wind_bit,
	_h1_point_physics_damped_wind_bit,
	_h1_point_physics_no_gravity_bit,

	_h1_point_physics_in_air_bit = 0,
	_h1_point_physics_in_water_bit,
	_h1_point_physics_collided_with_structure_bit,
	_h1_point_physics_collided_with_water_bit,
};

// render_sprite.c
enum
{
	_h1_sprite_screen_facing = 0,
	_h1_sprite_parallel_to_direction,
	_h1_sprite_perpendicular_to_direction,
};

// shader_effect flags
enum
{
	_h1_shader_effect_uses_nonlinear_tint_bit = 1,
};

static const real32 k_h1_global_gravity = 0.0035651792f;		// world units per tick per tick
static const real32 k_h1_air_mass_over_radius_cubed = 0.0011f * 118613.34f;
static const real32 k_h1_ticks_per_second = 30.f;
static const real32 k_h1_particle_collision_scale_upper = 1.5f;
static const real32 k_h1_particle_collision_scale_lower = 0.5f;
static const real32 k_h1_decal_offset = 1.f / 128.f;					// decals sit off their surface (the xbox pushed them 1/256 in depth)
static const real32 k_h1_decal_maximum_wrap_angle = 80.f * (_pi / 180.f);	// surfaces turned further from the hit surface aren't decaled
static const real32 k_h1_lens_flare_occlusion_rate = 12.f;			// lens flares fade in and out of occlusion over 1/12 second

static const char* const k_h1_projectile_effect_marker_names[] = { "", "gravity" };

/* structures */

struct s_h1_effect_location
{
	real_point3d position;
	real_vector3d forward;
	real_vector3d up;
	bool first_person;	// at the local player's first person weapon (_effect_location_first_person_bit)
};

struct s_h1_effect
{
	datum definition_index;
	real_vector3d velocity;
	real32 scale_a;
	real32 scale_b;
	real_rgb_color color;
	int16 event_index;
	bool in_duration;
	real32 event_time;
	real32 event_duration;
	real32 last_event_fraction;
	uint8 particle_counts[k_h1_maximum_particles_per_event];
	std::vector<std::vector<s_h1_effect_location>> locations;
	bool loop;		// an object's attached effect: it stops after the last event and starts again
	bool stopped;
	bool first_person;	// on the local player's first person weapon: it has first person instances
	int32 id;		// h1_effect_new_on_object_ex's handle, 0 for none
	datum follow_object_index;	// effects.c effect_new_looping: the effect follows its object and restarts until stopped
};

struct s_h1_particle
{
	datum definition_index;
	uint16 flags;
	uint8 state;
	real32 age;
	real32 lifespan;
	real32 frame_time;
	real32 frame_span;
	int16 sequence_index;
	int16 frame_index;
	real_point3d position;
	real_vector3d direction;
	real_vector3d velocity;
	real32 rotation;
	real32 angular_velocity;
	real32 radius;
	real_argb_color color;
};

struct s_h1_new_particle
{
	datum definition_index;
	real_point3d position;
	real_vector3d direction;
	real_vector3d velocity;
	real32 rotation;
	real32 angular_velocity;
	real32 radius;
	real_argb_color color;
};

// the vertex layout of the halo 1 renderer: the particle color rides in the normal, its alpha in the lightmap coordinates
struct s_h1_particle_vertex
{
	real32 position[3];
	real32 color[3];
	real32 texcoord[2];
	real32 alpha[2];
};
static_assert(sizeof(s_h1_particle_vertex) == 40);

// shader_definitions.h struct shader_effect_definition (the shader of a particle and of a particle system particle state)
struct s_h1_shader_effect
{
	int8 shader[0x28];
	uint16 flags;
	int16 framebuffer_blend_function;
	int16 framebuffer_fade_mode;
	uint16 primary_map_flags;
	int8 reserved_before_secondary_map[28];
	h1_tag_reference secondary_map;
	int16 secondary_map_anchor;
	uint16 secondary_map_flags;
	int8 secondary_map_animation[0x38];
	real32 secondary_map_radius;
	real32 secondary_map_zsprite_radius_scale;
	int8 reserved_after[20];
};
static_assert(sizeof(s_h1_shader_effect) == 0xB4);
static_assert(offsetof(s_h1_shader_effect, secondary_map) == 0x4C);

// particle_system_definitions.h
struct s_h1_pctl_physics_constant
{
	real32 k;
};
static_assert(sizeof(s_h1_pctl_physics_constant) == 4);

// scale, animation rate, rotation rate, color (argb)
struct s_h1_pctl_randomized_variables
{
	real32 scale;
	real32 animation_rate;
	real32 rotation_rate;
	real_argb_color color;
};
static_assert(sizeof(s_h1_pctl_randomized_variables) == 0x1C);

// the randomized multipliers, the radius multiplier, the minimum particle count, the particle creation rate
struct s_h1_pctl_type_variables
{
	s_h1_pctl_randomized_variables particle_state_randomized_multipliers;
	real32 radius_multiplier;
	real32 minimum_particle_count;
	real32 particle_creation_rate;
};
static_assert(sizeof(s_h1_pctl_type_variables) == 0x28);

struct s_h1_pctl_type_state
{
	char name[32];
	real_bounds duration;
	real_bounds transition_time;
	uint32 flags;
	s_h1_pctl_type_variables variables;
	int8 unused[84];
	int16 particle_creation_physics;
	int16 particle_update_physics;
	h1_tag_block<s_h1_pctl_physics_constant> physics_constants;
};
static_assert(sizeof(s_h1_pctl_type_state) == 0xC0);

struct s_h1_pctl_particle_state
{
	char name[32];
	real_bounds duration;
	real_bounds transition_time;
	h1_tag_reference bitmaps;
	int16 sequence_index;
	int16 pad;
	int32 unused;
	real_bounds scale;
	real_bounds animation_rate;
	real_bounds rotation_rate;
	real_argb_color color_lower_bound;
	real_argb_color color_upper_bound;
	real32 radius;
	h1_tag_reference point_physics;
	int8 reserved[0x24];
	s_h1_shader_effect shader;
	int8 reserved_after[0xC];
};
static_assert(sizeof(s_h1_pctl_particle_state) == 0x178);
static_assert(offsetof(s_h1_pctl_particle_state, shader) == 0xB8);

struct s_h1_pctl_type
{
	char name[32];
	uint32 flags;
	int16 initial_particle_count;
	int16 pad;
	int16 complex_sprite_render_mode;
	int16 sprite_render_mode;
	real32 radius;
	int8 reserved[0x24];
	int16 initial_particle_creation_physics;
	int16 pad2;
	uint32 physics_flags;
	h1_tag_block<s_h1_pctl_physics_constant> physics_constants;
	h1_tag_block<s_h1_pctl_type_state> type_states;
	h1_tag_block<s_h1_pctl_particle_state> particle_states;
};
static_assert(sizeof(s_h1_pctl_type) == 0x80);

struct s_h1_pctl
{
	int8 reserved[0x38];
	h1_tag_reference system_update_point_physics;
	int16 system_update_physics;
	int16 pad;
	uint32 physics_flags;
	h1_tag_block<s_h1_pctl_physics_constant> physics_constants;
	h1_tag_block<s_h1_pctl_type> types;
};
static_assert(sizeof(s_h1_pctl) == 0x68);

// particle_systems.h
struct s_h1_ps_particle
{
	bool valid;
	bool states_moving_forward;
	int16 state_index;
	int16 transition_state_index;
	real32 time_left_in_state;
	real32 state_length;
	real_point3d position;
	real_vector3d velocity;
	real_vector3d axis;
	real32 rotation;
	real32 sprite_index;
	s_h1_pctl_randomized_variables randomized_variables;
	s_h1_pctl_randomized_variables transition_randomized_variables;
};

struct s_h1_ps_type
{
	int16 state_index;
	int16 transition_state_index;
	real32 time_left_in_state;
	real32 state_length;
	s_h1_pctl_type_variables variables;
	real32 fractional_particle_count;
	bool states_moving_forward;
	std::vector<s_h1_ps_particle> particles;
};

struct s_h1_particle_system
{
	datum definition_index;
	bool active;
	bool initializing;
	real32 scale;
	real_point3d position;
	real_vector3d velocity;
	real_argb_color color;
	real_rgb_color lighting;
	s_h1_ps_type types[4];
};

// a sprite queued for drawing (render_sprite.c build_sprite), drawn in batches of one shader and bitmap
struct s_h1_sprite
{
	const s_h1_shader_effect* shader;
	datum bitmap_tag_index;
	int16 bitmap_index;
	s_h1_particle_vertex vertices[6];
};

// object_lights.c struct light_datum (the parts the halo 1 renderer uses)
struct s_h1_light
{
	datum definition_index;
	bool attached;				// attached lights live with their object, unattached ones for the definition's duration
	real32 age_ticks;
	real32 intensity_scale;
	real_point3d position;
	real_vector3d forward;
	real_vector3d up;
	// lights_preprocess_scene
	real_rgb_color color;
	real32 radius;
	real32 intensity;
	// how much of the lens flare's occlusion point the camera sees (the last occlusion query that finished)
	real32 occlusion_fraction;
	uint32 id;				// matches the occlusion queries to the light
	bool first_person;		// at the local player's first person weapon: tested in the first person depth range
};

// the occlusion queries of a lens flare (the occlusion point depth tested, and all its pixels)
struct s_h1_flare_query
{
	IDirect3DQuery9* visible;
	IDirect3DQuery9* total;
	uint32 light_id;
	bool pending;
};

// an effect or a light of a halo 1 object definition's attachments, kept while the object lives (objects.c object_attachments_new)
struct s_h1_attachment
{
	datum object_index;
	int16 attachment_index;
	uint32 group_tag;
	bool seen;
	s_h1_effect effect;
	s_h1_light light;
};

// effects/decals.c struct decal_datum, its vertices drawn by the particle shader
struct s_h1_decal
{
	datum definition_index;
	datum bitmap_tag_index;
	int16 bitmap_index;
	real32 age;
	real32 lifetime;
	real32 decay_time;
	real32 intensity;
	std::vector<s_h1_particle_vertex> vertices;
};

struct s_h1_effects_globals
{
	std::vector<s_h1_effect> effects;
	std::vector<s_h1_particle> particles;
	uint32 random;
	int32 particle_update_ticks;
	real32 particle_leftover_ticks;
	LARGE_INTEGER last_update;
	datum stub_effect_index;
	std::vector<s_h1_particle_vertex> vertices;
	std::vector<s_h1_particle_system> particle_systems;
	std::vector<s_h1_sprite> sprites;
	std::vector<s_h1_light> lights;
	std::vector<s_h1_attachment> attachments;
	std::vector<s_h1_decal> decals;
	real32 attachment_leftover_ticks;
	real32 frame_dt;
	uint32 next_light_id;
	std::vector<s_h1_flare_query> flare_queries;
	real32 light_constants[2 * k_h1_maximum_shader_lights][4];
};

/* globals */

static s_h1_effects_globals g_h1_effects = { {}, {}, 0x1234567u, 0, 0.f, {}, NONE, {}, {}, {}, {}, {}, {}, 0.f, 0.f, 0, {}, {} };

typedef void(__cdecl* t_projectile_detonation_effect_new)(datum definition_index, const real_point3d* point, const real_vector3d* forward, void* owner, bool super_detonation, bool airborne);
static t_projectile_detonation_effect_new p_projectile_detonation_effect_new = NULL;

/* prototypes */

static void __cdecl h1_projectile_detonation_effect_new(datum definition_index, const real_point3d* point, const real_vector3d* forward, void* owner, bool super_detonation, bool airborne);

static uint32 h1_effects_random(void);
static real32 h1_effects_random_real(void);
static real32 h1_effects_random_range(real32 lower, real32 upper);
static int32 h1_effects_random_integer(int32 lower, int32 upper);
static real_vector3d h1_effects_random_direction(void);
static void h1_rotate_vector_about_axis(real_vector3d* v, const real_vector3d* axis, real32 sine, real32 cosine);
static real_vector3d h1_cross(const real_vector3d* a, const real_vector3d* b);
static real32 h1_dot(const real_vector3d* a, const real_vector3d* b);
static real32 h1_magnitude(const real_vector3d* v);
static void h1_normalize(real_vector3d* v);
static real_vector3d h1_perpendicular(const real_vector3d* v);

static void h1_effect_set_event(s_h1_effect* effect, int16 event_index);
static bool h1_effect_update(s_h1_effect* effect, real32 dt);
static void h1_effect_generate_parts(s_h1_effect* effect);
static void h1_effect_generate_particles(s_h1_effect* effect);
static real32 h1_effect_scale(const s_h1_effect* effect, real32 value, uint32 scale_a_flags, uint32 scale_b_flags, int32 bit);
static real32 h1_effect_random_range(const s_h1_effect* effect, real32 lower, real32 upper, uint32 scale_a_flags, uint32 scale_b_flags, int32 first_bit);
static void h1_effect_random_translational_velocity(const s_h1_effect* effect, const real_vector3d* forward, real_vector3d* direction, real_vector3d* velocity,
	real32 lower, real32 upper, real32 cone_angle, uint32 scale_a_flags, uint32 scale_b_flags);
static real32 h1_effect_distribution_integral(int16 function, real32 fraction);

static void h1_particle_new(const s_h1_new_particle* data);
static bool h1_particle_update(s_h1_particle* particle, real32 dt);
static void h1_particle_spawn_effect(const s_h1_particle* particle, const h1_tag_reference* reference, real32 scale);
static bool h1_particle_next_sequence(s_h1_particle* particle);
static bool h1_particle_next_frame(s_h1_particle* particle);
static real32 h1_particle_radius(const s_h1_particle* particle);
static uint32 h1_point_physics_update(const h1_pphy* physics, real_point3d* position, real_vector3d* velocity, real_vector3d* collision_normal, real32 radius, real32 dt);

static void h1_sprite_build(const s_h1_shader_effect* shader, datum bitmap_tag_index, int16 mode, int16 sequence_index, int16 sprite_index,
	const real_point3d* origin, const real_vector3d* direction, real32 rotation, real32 scale, const real_argb_color* color, real32 fade, bool u_mirror, bool v_mirror);
static void h1_sprite_build_rotational(const s_h1_shader_effect* shader, datum bitmap_tag_index, bool sideways_rotation_animates, int16 first_sequence_index, int16 sprite_index,
	const real_point3d* origin, const real_vector3d* axis, real32 rotation, real32 scale, const real_argb_color* color, real32 fade);
static void h1_particles_build_sprites(void);
static void h1_particle_systems_build_sprites(void);
static void h1_sprites_draw(void);
static void h1_particle_system_new(datum definition_index, const real_point3d* position, const real_vector3d* velocity, const real_argb_color* color, real32 scale);
static bool h1_particle_system_update(s_h1_particle_system* system, real32 dt);
static int32 h1_particle_system_particle_count(void);

static void h1_light_new_unattached(datum definition_index, const real_point3d* position, const real_vector3d* forward, real32 scale);
static void h1_attachment_effect_build_locations(s_h1_effect* effect, datum object_index, const object_datum* object, const char* attachment_marker);
static real32 h1_transition_function(int16 function, real32 t);
static void h1_lights_update(real32 dt);
static void h1_attachments_update(real32 dt);
static void h1_decal_new(datum definition_index, const real_point3d* origin, const real_vector3d* velocity, real32 radius_modifier);
static void h1_decals_update(real32 dt);
static void h1_lens_flares_render(void);
static void h1_lens_flare_occlusion_test(s_h1_light* light, const real_point3d* point, real32 radius);

/* public code */

void h1_effects_apply_patches(void)
{
	DETOUR_ATTACH(p_projectile_detonation_effect_new, Memory::GetAddress<t_projectile_detonation_effect_new>(0x146753), h1_projectile_detonation_effect_new);
	return;
}

void h1_effects_reset(void)
{
	g_h1_effects.effects.clear();
	g_h1_effects.particles.clear();
	g_h1_effects.particle_systems.clear();
	g_h1_effects.sprites.clear();
	g_h1_effects.lights.clear();
	g_h1_effects.attachments.clear();
	g_h1_effects.decals.clear();
	for (s_h1_flare_query& query : g_h1_effects.flare_queries)
	{
		if (query.visible) query.visible->Release();
		if (query.total) query.total->Release();
	}
	g_h1_effects.flare_queries.clear();
	g_h1_effects.attachment_leftover_ticks = 0.f;
	memset(g_h1_effects.light_constants, 0, sizeof(g_h1_effects.light_constants));
	g_h1_effects.particle_leftover_ticks = 0.f;
	g_h1_effects.stub_effect_index = NONE;
	QueryPerformanceCounter(&g_h1_effects.last_update);
	return;
}

datum h1_effects_stub_effect_get(void)
{
	if (g_h1_effects.stub_effect_index == NONE)
	{
		h2x_effe* effect = NULL;
		g_h1_effects.stub_effect_index = h1_runtime_tag_new('effe', "halo1\\effects\\halo 1 effect", &effect);
	}
	return g_h1_effects.stub_effect_index;
}

void h1_effect_new_unattached(datum h1_effect_index, const real_point3d* point, const real_vector3d* forward)
{
	const h1_effe* definition = h1_effect_index != NONE ? (const h1_effe*)g_h1_cache_file->tag_get('effe', h1_effect_index) : NULL;
	if (!definition || definition->events.count <= 0 || g_h1_effects.effects.size() >= k_h1_maximum_effects)
	{
		return;
	}

	s_h1_effect effect = {};
	effect.definition_index = h1_effect_index;
	effect.velocity = { 0.f, 0.f, 0.f };
	effect.scale_a = 0.f;
	effect.scale_b = 0.f;
	effect.color = { 1.f, 1.f, 1.f };

	// the projectile markers: the point facing forward, "gravity" facing down; unnamed and unknown markers are the first
	real_vector3d marker_forwards[2] = { *forward, { 0.f, 0.f, -1.f } };
	h1_normalize(&marker_forwards[0]);
	effect.locations.resize(definition->locations.count);
	for (int32 i = 0; i < definition->locations.count; i++)
	{
		const h1_effe_locations* location = g_h1_cache_file->block_get(definition->locations, i);
		int32 marker_index = 0;
		if (location->marker_name[0])
		{
			for (int32 j = 0; j < NUMBEROF(k_h1_projectile_effect_marker_names); j++)
			{
				if (_stricmp(location->marker_name, k_h1_projectile_effect_marker_names[j]) == 0)
				{
					marker_index = j;
					break;
				}
			}
		}
		s_h1_effect_location instance = {};
		instance.position = *point;
		instance.forward = marker_forwards[marker_index];
		instance.up = h1_perpendicular(&instance.forward);
		h1_normalize(&instance.up);
		effect.locations[i].push_back(instance);
	}

	h1_effect_set_event(&effect, 0);
	if (h1_effect_update(&effect, 0.f))
	{
		g_h1_effects.effects.push_back(std::move(effect));
	}
	return;
}

void h1_effects_update(void)
{
	LARGE_INTEGER now, frequency;
	QueryPerformanceCounter(&now);
	QueryPerformanceFrequency(&frequency);
	real32 dt = (real32)(now.QuadPart - g_h1_effects.last_update.QuadPart) / (real32)frequency.QuadPart;
	g_h1_effects.last_update = now;
	if (!h1_maps_active() || !g_h1_cache_file)
	{
		return;
	}
	dt = PIN(dt, 0.f, 0.1f);
	g_h1_effects.frame_dt = dt;

	h1_objects_update_functions();
	h1_attachments_update(dt);
	for (size_t i = 0; i < g_h1_effects.effects.size();)
	{
		s_h1_effect* effect = &g_h1_effects.effects[i];
		if (effect->follow_object_index != NONE)
		{
			// effects.c effect_update of a looping effect: it follows its object and starts again at the loop start event
			const object_datum* object = (const object_datum*)object_try_and_get(effect->follow_object_index);
			if (!object)
			{
				effect->follow_object_index = NONE;
				effect->loop = false;
				effect->stopped = true;
			}
			else
			{
				h1_attachment_effect_build_locations(effect, effect->follow_object_index, object, "");
				effect->velocity =
				{
					object->object.translational_velocity.i / k_h1_ticks_per_second,
					object->object.translational_velocity.j / k_h1_ticks_per_second,
					object->object.translational_velocity.k / k_h1_ticks_per_second
				};
				if (effect->stopped)
				{
					const h1_effe* definition = (const h1_effe*)g_h1_cache_file->tag_get('effe', effect->definition_index);
					effect->stopped = false;
					h1_effect_set_event(effect, definition && VALID_INDEX(definition->loop_start_event_index, definition->events.count) ?
						definition->loop_start_event_index : (int16)0);
				}
			}
		}
		if (effect->stopped && !effect->loop && effect->follow_object_index == NONE && effect->id != 0)
		{
			g_h1_effects.effects.erase(g_h1_effects.effects.begin() + i);
			continue;
		}
		if (!h1_effect_update(&g_h1_effects.effects[i], dt))
		{
			g_h1_effects.effects.erase(g_h1_effects.effects.begin() + i);
			continue;
		}
		i++;
	}

	// particles.c particles_update: ticks for the particles animating once per frame
	g_h1_effects.particle_leftover_ticks += dt * k_h1_ticks_per_second;
	g_h1_effects.particle_update_ticks = (int32)g_h1_effects.particle_leftover_ticks;
	g_h1_effects.particle_leftover_ticks -= (real32)g_h1_effects.particle_update_ticks;
	for (size_t i = 0; i < g_h1_effects.particles.size();)
	{
		if (!h1_particle_update(&g_h1_effects.particles[i], dt))
		{
			g_h1_effects.particles[i] = g_h1_effects.particles.back();
			g_h1_effects.particles.pop_back();
			continue;
		}
		i++;
	}
	for (size_t i = 0; i < g_h1_effects.particle_systems.size();)
	{
		if (!h1_particle_system_update(&g_h1_effects.particle_systems[i], dt))
		{
			g_h1_effects.particle_systems.erase(g_h1_effects.particle_systems.begin() + i);
			continue;
		}
		i++;
	}
	h1_lights_update(dt);
	h1_decals_update(dt);
	return;
}

// render_particles.c render_particles and particle_systems.c particle_systems_render
void h1_effects_render(void)
{
	if (!g_h1_effects.particles.empty() || !g_h1_effects.particle_systems.empty())
	{
		h1_particles_build_sprites();
		h1_particle_systems_build_sprites();
		h1_sprites_draw();
	}
	return;
}

/* private code */

// items/projectiles: a projectile built from a halo 1 projectile plays its halo 1 detonation effect (projectiles.c projectile_detonate)
static void __cdecl h1_projectile_detonation_effect_new(datum definition_index, const real_point3d* point, const real_vector3d* forward, void* owner, bool super_detonation, bool airborne)
{
	const datum h1_projectile_index = h1_maps_active() ? h1_objects_h1_definition_get(definition_index) : NONE;
	const h1_proj* h1_projectile = h1_projectile_index != NONE ? (const h1_proj*)g_h1_cache_file->tag_get('proj', h1_projectile_index) : NULL;
	if (!h1_projectile)
	{
		p_projectile_detonation_effect_new(definition_index, point, forward, owner, super_detonation, airborne);
		return;
	}

	const datum effect_index = super_detonation && h1_projectile->super_detonation.index != NONE ? h1_projectile->super_detonation.index : h1_projectile->effect.index;
	h1_effect_new_unattached(effect_index, point, forward);
	return;
}

static uint32 h1_effects_random(void)
{
	g_h1_effects.random = g_h1_effects.random * 0x19660D + 0x3C6EF35F;
	return g_h1_effects.random >> 16;
}

static real32 h1_effects_random_real(void)
{
	return (real32)h1_effects_random() / 65536.f;
}

static real32 h1_effects_random_range(real32 lower, real32 upper)
{
	return lower + (upper - lower) * h1_effects_random_real();
}

static int32 h1_effects_random_integer(int32 lower, int32 upper)
{
	return upper > lower ? lower + (int32)(h1_effects_random() % (uint32)(upper - lower)) : lower;
}

static real_vector3d h1_effects_random_direction(void)
{
	const real32 z = h1_effects_random_range(-1.f, 1.f);
	const real32 angle = h1_effects_random_range(0.f, 2.f * _pi);
	const real32 r = sqrtf(MAX(1.f - z * z, 0.f));
	return { r * cosf(angle), r * sinf(angle), z };
}

static void h1_rotate_vector_about_axis(real_vector3d* v, const real_vector3d* axis, real32 sine, real32 cosine)
{
	const real_vector3d cross = h1_cross(axis, v);
	const real32 dot = h1_dot(axis, v) * (1.f - cosine);
	v->i = v->i * cosine + cross.i * sine + axis->i * dot;
	v->j = v->j * cosine + cross.j * sine + axis->j * dot;
	v->k = v->k * cosine + cross.k * sine + axis->k * dot;
	return;
}

static real_vector3d h1_cross(const real_vector3d* a, const real_vector3d* b)
{
	return { a->j * b->k - a->k * b->j, a->k * b->i - a->i * b->k, a->i * b->j - a->j * b->i };
}

static real32 h1_dot(const real_vector3d* a, const real_vector3d* b)
{
	return a->i * b->i + a->j * b->j + a->k * b->k;
}

static real32 h1_magnitude(const real_vector3d* v)
{
	return sqrtf(h1_dot(v, v));
}

static void h1_normalize(real_vector3d* v)
{
	const real32 length = h1_magnitude(v);
	if (length > 0.0001f)
	{
		v->i /= length;
		v->j /= length;
		v->k /= length;
	}
	return;
}

static real_vector3d h1_perpendicular(const real_vector3d* v)
{
	const real32 x = fabsf(v->i), y = fabsf(v->j), z = fabsf(v->k);
	if (x <= y && x <= z)
	{
		return { 0.f, -v->k, v->j };
	}
	if (y <= z)
	{
		return { -v->k, 0.f, v->i };
	}
	return { -v->j, v->i, 0.f };
}

static void h1_rgb_to_hsv(const real_rgb_color* rgb, real32* hue, real32* saturation, real32* value)
{
	const real32 maximum = MAX(rgb->red, MAX(rgb->green, rgb->blue));
	const real32 minimum = MIN(rgb->red, MIN(rgb->green, rgb->blue));
	const real32 delta = maximum - minimum;
	*value = maximum;
	*saturation = maximum > 0.f ? delta / maximum : 0.f;
	if (delta <= 0.f)
	{
		*hue = 0.f;
		return;
	}
	real32 h;
	if (maximum == rgb->red) h = (rgb->green - rgb->blue) / delta;
	else if (maximum == rgb->green) h = 2.f + (rgb->blue - rgb->red) / delta;
	else h = 4.f + (rgb->red - rgb->green) / delta;
	h /= 6.f;
	*hue = h < 0.f ? h + 1.f : h;
	return;
}

static void h1_hsv_to_rgb(real32 hue, real32 saturation, real32 value, real_rgb_color* rgb)
{
	const real32 h = (hue - floorf(hue)) * 6.f;
	const int32 sector = (int32)h % 6;
	const real32 f = h - floorf(h);
	const real32 p = value * (1.f - saturation);
	const real32 q = value * (1.f - saturation * f);
	const real32 t = value * (1.f - saturation * (1.f - f));
	switch (sector)
	{
	case 0: *rgb = { value, t, p }; break;
	case 1: *rgb = { q, value, p }; break;
	case 2: *rgb = { p, value, t }; break;
	case 3: *rgb = { p, q, value }; break;
	case 4: *rgb = { t, p, value }; break;
	default: *rgb = { value, p, q }; break;
	}
	return;
}

// bitmap_utilities.c rgb_colors_interpolate: flags 1 interpolate in hsv, 2 the long way around the hue
void h1_rgb_colors_interpolate(real_rgb_color* result, uint32 flags, const real_rgb_color* lower, const real_rgb_color* upper, real32 t)
{
	if (TEST_BIT(flags, 0))
	{
		real32 h0, s0, v0, h1, s1, v1;
		h1_rgb_to_hsv(lower, &h0, &s0, &v0);
		h1_rgb_to_hsv(upper, &h1, &s1, &v1);
		if ((fabsf(h0 - h1) > 0.5f) != TEST_BIT(flags, 1))
		{
			if (h0 < h1) h0 += 1.f;
			else h1 += 1.f;
		}
		real32 hue = (1.f - t) * h0 + t * h1;
		if (hue > 1.f) hue -= 1.f;
		h1_hsv_to_rgb(hue, (1.f - t) * s0 + t * s1, (1.f - t) * v0 + t * v1, result);
		return;
	}
	result->red = (1.f - t) * lower->red + t * upper->red;
	result->green = (1.f - t) * lower->green + t * upper->green;
	result->blue = (1.f - t) * lower->blue + t * upper->blue;
	return;
}

// effects.c effect_set_event
static void h1_effect_set_event(s_h1_effect* effect, int16 event_index)
{
	const h1_effe* definition = (const h1_effe*)g_h1_cache_file->tag_get('effe', effect->definition_index);
	const h1_effe_events* event = definition ? g_h1_cache_file->block_get(definition->events, event_index) : NULL;
	if (!event)
	{
		return;
	}
	effect->in_duration = false;
	effect->event_index = event_index;
	effect->event_time = 0.f;
	effect->event_duration = h1_effects_random_range(event->delay_bounds.lower, event->delay_bounds.upper);
	return;
}

// effects.c effect_update, false once the effect is over
static bool h1_effect_update(s_h1_effect* effect, real32 dt)
{
	const h1_effe* definition = (const h1_effe*)g_h1_cache_file->tag_get('effe', effect->definition_index);
	if (!definition)
	{
		return false;
	}

	for (int32 iteration = 0; dt >= 0.f && iteration < k_h1_maximum_effect_events_per_update && !effect->stopped; iteration++)
	{
		bool event_completed;
		if (effect->event_duration - effect->event_time <= dt)
		{
			dt -= effect->event_duration - effect->event_time;
			event_completed = true;
			effect->event_time = effect->event_duration;
		}
		else
		{
			effect->event_time += dt;
			event_completed = false;
			dt = -1.f;
		}

		if (effect->in_duration)
		{
			h1_effect_generate_particles(effect);
			if (!event_completed)
			{
				continue;
			}

			int16 next_event_index = effect->event_index + 1;
			while (next_event_index < definition->events.count &&
				h1_effects_random_real() < g_h1_cache_file->block_get(definition->events, next_event_index)->skip_fraction)
			{
				next_event_index++;
			}
			if (next_event_index >= definition->events.count)
			{
				if (effect->loop)
				{
					effect->stopped = true;
					return true;
				}
				return false;
			}
			h1_effect_set_event(effect, next_event_index);
		}
		else if (event_completed)
		{
			const h1_effe_events* event = g_h1_cache_file->block_get(definition->events, effect->event_index);
			effect->in_duration = true;
			effect->event_time = 0.f;
			effect->last_event_fraction = -1.f;
			effect->event_duration = h1_effects_random_range(event->duration_bounds.lower, event->duration_bounds.upper);

			for (int32 i = 0; i < event->particles.count && i < k_h1_maximum_particles_per_event; i++)
			{
				const h1_effe_events_particles* particles = g_h1_cache_file->block_get(event->particles, i);
				uint8 count = (uint8)(int32)h1_effect_random_range(effect, (real32)particles->count.lower, (real32)particles->count.upper,
					particles->a_scales_values, particles->b_scales_values, _h1_effect_particle_count_bit);
				effect->particle_counts[i] = count;
			}
			h1_effect_generate_parts(effect);
		}
	}
	return true;
}

// effects.c effect_generate_parts: sounds, particle systems, lights and decals; damage is halo 2's and objects aren't made
static void h1_effect_generate_parts(s_h1_effect* effect)
{
	const h1_effe* definition = (const h1_effe*)g_h1_cache_file->tag_get('effe', effect->definition_index);
	const h1_effe_events* event = g_h1_cache_file->block_get(definition->events, effect->event_index);
	for (int32 i = 0; i < event->parts.count; i++)
	{
		const h1_effe_events_parts* part = g_h1_cache_file->block_get(event->parts, i);
		if (!VALID_INDEX(part->location_index, (int32)effect->locations.size()) || part->type.index == NONE ||
			part->violence_mode == _h1_effect_disposition_nonviolent ||
			part->create_in == _h1_effect_environment_water || part->create_in == _h1_effect_environment_vacuum)
		{
			continue;
		}

		for (const s_h1_effect_location& instance : effect->locations[part->location_index])
		{
			if (instance.first_person)
			{
				continue;
			}
			real_point3d point = instance.position;
			real_vector3d forward = instance.forward;
			if (TEST_BIT(part->flags, _h1_effect_part_world_down_bit))
			{
				forward = { 0.f, 0.f, -1.f };
			}
			const real32 scale = h1_effect_scale(effect, 1.f, part->a_scales_values, part->b_scales_values, _h1_effect_part_type_scale_bit);

			switch (part->runtime_base_group_tag)
			{
			case 'snd!':
				h1_sound_impulse(part->type.index, &point, scale);
				break;
			case 'pctl':
			{
				real_vector3d direction, velocity;
				h1_effect_random_translational_velocity(effect, &forward, &direction, &velocity, part->velocity_bounds.lower, part->velocity_bounds.upper,
					part->velocity_cone_angle, part->a_scales_values, part->b_scales_values);
				velocity.i += effect->velocity.i;
				velocity.j += effect->velocity.j;
				velocity.k += effect->velocity.k;
				const real_argb_color tint = { 1.f, effect->color.red, effect->color.green, effect->color.blue };
				h1_particle_system_new(part->type.index, &point, &velocity, &tint, scale);
				break;
			}
			case 'ligh':
				h1_light_new_unattached(part->type.index, &point, &forward, scale);
				break;
			case 'deca':
			{
				real_vector3d direction, velocity;
				h1_effect_random_translational_velocity(effect, &forward, &direction, &velocity, part->velocity_bounds.lower, part->velocity_bounds.upper,
					part->velocity_cone_angle, part->a_scales_values, part->b_scales_values);
				const real32 radius_modifier = h1_effects_random_range(part->radius_modifier_bounds.lower, part->radius_modifier_bounds.upper);
				h1_decal_new(part->type.index, &point, &velocity, radius_modifier);
				break;
			}
			default:
				break;
			}
		}
	}
	return;
}

// effects.c effect_generate_particles
static void h1_effect_generate_particles(s_h1_effect* effect)
{
	const h1_effe* definition = (const h1_effe*)g_h1_cache_file->tag_get('effe', effect->definition_index);
	const h1_effe_events* event = g_h1_cache_file->block_get(definition->events, effect->event_index);
	const real32 last_event_fraction = effect->last_event_fraction;
	const real32 event_fraction = effect->event_duration > 0.f ? effect->event_time / effect->event_duration : 1.f;

	for (int32 i = 0; i < event->particles.count && i < k_h1_maximum_particles_per_event; i++)
	{
		const h1_effe_events_particles* particles = g_h1_cache_file->block_get(event->particles, i);
		if (!VALID_INDEX(particles->location_index, (int32)effect->locations.size()) ||
			particles->violence_mode == _h1_effect_disposition_nonviolent ||
			particles->create_in == _h1_effect_environment_water || particles->create_in == _h1_effect_environment_vacuum)
		{
			continue;
		}

		const int32 last_count = (int32)(h1_effect_distribution_integral(particles->distribution_function, last_event_fraction) * effect->particle_counts[i]);
		const int32 count_delta = (int32)(h1_effect_distribution_integral(particles->distribution_function, event_fraction) * effect->particle_counts[i]) - last_count;
		if (count_delta <= 0)
		{
			continue;
		}

		// effects.c effect_location_get_next_instance: first person only particles (and both, for the local player's effect)
		// come from the first person instances, the rest from the third person ones; the particles only third person views
		// draw (and only first person views draw) aren't seen by the local player looking through the weapon (and the others)
		const bool first_person_instances = particles->create == _h1_effect_camera_mode_first_person_only ||
			(particles->create == _h1_effect_camera_mode_both && effect->first_person);
		if ((particles->create == _h1_effect_camera_mode_third_person_only && effect->first_person) ||
			(particles->create == _h1_effect_camera_mode_first_person_only && !effect->first_person))
		{
			continue;
		}
		for (const s_h1_effect_location& instance : effect->locations[particles->location_index])
		{
			if (instance.first_person != first_person_instances)
			{
				continue;
			}
			// the instance's frame: forward, left, up
			const real_vector3d left = h1_cross(&instance.up, &instance.forward);
			auto transform_vector = [&](const real_vector3d* v) -> real_vector3d
			{
				return {
					instance.forward.i * v->i + left.i * v->j + instance.up.i * v->k,
					instance.forward.j * v->i + left.j * v->j + instance.up.j * v->k,
					instance.forward.k * v->i + left.k * v->j + instance.up.k * v->k };
			};

			for (int32 n = 0; n < count_delta; n++)
			{
				s_h1_new_particle data;
				const real32 emission_radius = h1_effect_random_range(effect, particles->distribution_radius.lower, particles->distribution_radius.upper,
					particles->a_scales_values, particles->b_scales_values, _h1_effect_particle_distribution_radius_bit);
				const real_vector3d random_offset = h1_effects_random_direction();
				const real_vector3d offset = { particles->relative_offset.x, particles->relative_offset.y, particles->relative_offset.z };
				const real_vector3d relative_offset = transform_vector(&offset);
				data.position.x = instance.position.x + random_offset.i * emission_radius + relative_offset.i;
				data.position.y = instance.position.y + random_offset.j * emission_radius + relative_offset.j;
				data.position.z = instance.position.z + random_offset.k * emission_radius + relative_offset.k;

				real_vector3d direction, velocity;
				h1_effect_random_translational_velocity(effect, &particles->relative_direction_vector, &direction, &velocity,
					particles->velocity.lower, particles->velocity.upper, particles->velocity_cone_angle, particles->a_scales_values, particles->b_scales_values);
				data.direction = transform_vector(&direction);
				data.velocity = transform_vector(&velocity);
				data.velocity.i += effect->velocity.i * k_h1_ticks_per_second;
				data.velocity.j += effect->velocity.j * k_h1_ticks_per_second;
				data.velocity.k += effect->velocity.k * k_h1_ticks_per_second;

				data.definition_index = particles->particle_type.index;
				data.radius = h1_effect_random_range(effect, particles->radius.lower, particles->radius.upper,
					particles->a_scales_values, particles->b_scales_values, _h1_effect_particle_radius_bit);
				data.angular_velocity = h1_effect_random_range(effect, particles->angular_velocity.lower, particles->angular_velocity.upper,
					particles->a_scales_values, particles->b_scales_values, _h1_effect_angular_velocity_bit);
				data.rotation = TEST_BIT(particles->flags, _h1_effect_particle_random_orientation_bit) ? h1_effects_random_range(0.f, 2.f * _pi) : 0.f;

				real32 t;
				if (TEST_BIT(particles->a_scales_values, _h1_effect_particle_tint_bit) || TEST_BIT(particles->b_scales_values, _h1_effect_particle_tint_bit))
				{
					t = h1_effect_scale(effect, 1.f, particles->a_scales_values, particles->b_scales_values, _h1_effect_particle_tint_bit);
				}
				else
				{
					t = h1_effects_random_real();
				}
				const real_rgb_color tint_lower = { particles->tint_lower_bound.red, particles->tint_lower_bound.green, particles->tint_lower_bound.blue };
				const real_rgb_color tint_upper = { particles->tint_upper_bound.red, particles->tint_upper_bound.green, particles->tint_upper_bound.blue };
				real_rgb_color tint;
				h1_rgb_colors_interpolate(&tint, (particles->flags >> _h1_effect_particle_tint_interpolate_hsv_bit) & 3, &tint_lower, &tint_upper, t);
				data.color.red = tint.red;
				data.color.green = tint.green;
				data.color.blue = tint.blue;
				data.color.alpha = (1.f - t) * particles->tint_lower_bound.alpha + particles->tint_upper_bound.alpha * t;
				if (TEST_BIT(particles->flags, _h1_effect_particle_tint_from_change_color_bit))
				{
					data.color.red *= effect->color.red;
					data.color.green *= effect->color.green;
					data.color.blue *= effect->color.blue;
				}
				h1_particle_new(&data);
			}
		}
	}
	effect->last_event_fraction = event_fraction;
	return;
}

static real32 h1_effect_scale(const s_h1_effect* effect, real32 value, uint32 scale_a_flags, uint32 scale_b_flags, int32 bit)
{
	if (TEST_BIT(scale_a_flags, bit))
	{
		value *= effect->scale_a;
	}
	if (TEST_BIT(scale_b_flags, bit))
	{
		value *= effect->scale_b;
	}
	return value;
}

static real32 h1_effect_random_range(const s_h1_effect* effect, real32 lower, real32 upper, uint32 scale_a_flags, uint32 scale_b_flags, int32 first_bit)
{
	const real32 base = h1_effect_scale(effect, lower, scale_a_flags, scale_b_flags, first_bit);
	const real32 range = h1_effect_scale(effect, upper - lower, scale_a_flags, scale_b_flags, first_bit + 1);
	return h1_effects_random_range(0.f, range) + base;
}

static void h1_effect_random_translational_velocity(const s_h1_effect* effect, const real_vector3d* forward, real_vector3d* direction, real_vector3d* velocity,
	real32 lower, real32 upper, real32 cone_angle, uint32 scale_a_flags, uint32 scale_b_flags)
{
	const real32 magnitude = h1_effect_random_range(effect, lower, upper, scale_a_flags, scale_b_flags, _h1_effect_velocity_bit);
	cone_angle = h1_effect_scale(effect, cone_angle, scale_a_flags, scale_b_flags, _h1_effect_velocity_cone_bit);
	const real32 angle = h1_effects_random_real() * cone_angle;
	*direction = *forward;
	if (angle != 0.f)
	{
		const real_vector3d axis = h1_effects_random_direction();
		h1_rotate_vector_about_axis(direction, &axis, sinf(angle), cosf(angle));
	}
	velocity->i = magnitude * direction->i;
	velocity->j = magnitude * direction->j;
	velocity->k = magnitude * direction->k;
	return;
}

static real32 h1_effect_distribution_integral(int16 function, real32 fraction)
{
	if (fraction == -1.f)
	{
		return 0.f;
	}
	switch (function)
	{
	case _h1_effect_distribution_start: return 1.f;
	case _h1_effect_distribution_end: return fraction < 1.f ? 0.f : 1.f;
	case _h1_effect_distribution_buildup: return fraction * fraction;
	case _h1_effect_distribution_falloff: return (2.f - fraction) * fraction;
	case _h1_effect_distribution_quadratic: return (3.f - (fraction + fraction)) * fraction * fraction;
	default: return fraction;
	}
}

// particles.c particle_new (particles attached to objects start where they are and stay unattached)
static void h1_particle_new(const s_h1_new_particle* data)
{
	const h1_part* definition = data->definition_index != NONE ? (const h1_part*)g_h1_cache_file->tag_get('part', data->definition_index) : NULL;
	if (!definition || g_h1_effects.particles.size() >= k_h1_maximum_particles)
	{
		return;
	}

	s_h1_particle particle = {};
	particle.flags = 0;
	if (TEST_BIT(definition->flags, _h1_particle_definition_can_animate_backwards_bit))
	{
		particle.flags |= h1_effects_random() & FLAG(_h1_particle_animates_backwards_bit);
	}
	if (TEST_BIT(definition->flags, _h1_particle_definition_random_horizontal_mirroring_bit))
	{
		particle.flags |= h1_effects_random() & FLAG(_h1_particle_u_mirror_bit);
	}
	if (TEST_BIT(definition->flags, _h1_particle_definition_random_vertical_mirroring_bit))
	{
		particle.flags |= h1_effects_random() & FLAG(_h1_particle_v_mirror_bit);
	}
	particle.definition_index = data->definition_index;
	particle.state = _h1_particle_state_next_sequence_initial;
	particle.lifespan = h1_effects_random_range(definition->lifespan.lower, definition->lifespan.upper);
	particle.frame_span = definition->animation_rate.upper != 0.f ? 1.f / h1_effects_random_range(definition->animation_rate.lower, definition->animation_rate.upper) : FLT_MAX;
	particle.frame_time = -1.f;
	particle.position = data->position;
	particle.direction = data->direction;
	particle.rotation = data->rotation;
	particle.radius = data->radius;
	particle.color = data->color;
	particle.velocity = data->velocity;
	particle.angular_velocity = data->angular_velocity;

	// lit by the surface below it unless it lights itself
	if (!TEST_BIT(definition->flags, _h1_particle_definition_self_illuminated_bit))
	{
		s_h1_render_lighting lighting;
		h1_render_lighting_at(&data->position, &lighting);
		particle.color.red *= MIN(lighting.ambient.red + lighting.light0_color.red + lighting.light1_color.red, 1.f);
		particle.color.green *= MIN(lighting.ambient.green + lighting.light0_color.green + lighting.light1_color.green, 1.f);
		particle.color.blue *= MIN(lighting.ambient.blue + lighting.light0_color.blue + lighting.light1_color.blue, 1.f);
	}

	if (!h1_particle_next_sequence(&particle))
	{
		return;
	}
	const h1_bitm* bitmap_group = (const h1_bitm*)g_h1_cache_file->tag_get('bitm', definition->bitmap.index);
	const h1_bitm_sequences* sequence = g_h1_cache_file->block_get(bitmap_group->sequences, particle.sequence_index);
	const int16 sprite_count = sequence ? (int16)sequence->sprites.count : 0;
	if (TEST_BIT(definition->flags, _h1_particle_definition_animation_starts_on_random_frame_bit))
	{
		particle.frame_index = (int16)((TEST_BIT(particle.flags, _h1_particle_animates_backwards_bit) ? 1 : NONE) + h1_effects_random_integer(0, sprite_count));
	}
	else if (TEST_BIT(particle.flags, _h1_particle_animates_backwards_bit))
	{
		particle.frame_index = sprite_count;
	}
	else
	{
		particle.frame_index = NONE;
	}
	g_h1_effects.particles.push_back(particle);
	return;
}

// particles.c particle_die: the particle's effect plays where it ends
static void h1_particle_spawn_effect(const s_h1_particle* particle, const h1_tag_reference* reference, real32 scale)
{
	if (reference->index == NONE)
	{
		return;
	}
	if (reference->group_tag == 'effe')
	{
		real_vector3d forward = particle->direction;
		if (h1_magnitude(&forward) <= 0.f)
		{
			forward = { 1.f, 0.f, 0.f };
		}
		h1_effect_new_unattached(reference->index, &particle->position, &forward);
	}
	else if (reference->group_tag == 'snd!')
	{
		h1_sound_impulse(reference->index, &particle->position, scale);
	}
	return;
}

// particles.c particle_next_sequence, false when the particle died
static bool h1_particle_next_sequence(s_h1_particle* particle)
{
	const h1_part* definition = (const h1_part*)g_h1_cache_file->tag_get('part', particle->definition_index);
	const h1_bitm* bitmap_group = definition ? (const h1_bitm*)g_h1_cache_file->tag_get('bitm', definition->bitmap.index) : NULL;
	if (!bitmap_group)
	{
		return false;
	}

	particle->sequence_index = NONE;
	if (particle->state == _h1_particle_state_next_sequence_initial)
	{
		if (definition->initial_sequence_count > 0)
		{
			particle->sequence_index = (int16)(definition->first_sequence_index + h1_effects_random_integer(0, definition->initial_sequence_count));
		}
		particle->state++;
	}
	if ((particle->sequence_index == NONE && particle->state == _h1_particle_state_next_sequence_looping) || particle->state == _h1_particle_state_still_looping)
	{
		if (particle->state == _h1_particle_state_next_sequence_looping)
		{
			particle->state = _h1_particle_state_still_looping;
		}
		if (particle->age < particle->lifespan && definition->looping_sequence_count > 0)
		{
			particle->sequence_index = (int16)(definition->initial_sequence_count + definition->first_sequence_index +
				h1_effects_random_integer(0, definition->looping_sequence_count));
		}
		else
		{
			particle->state++;
		}
	}
	if (particle->sequence_index == NONE && particle->state == _h1_particle_state_next_sequence_final)
	{
		if (definition->final_sequence_count > 0)
		{
			particle->sequence_index = (int16)(definition->looping_sequence_count + definition->initial_sequence_count + definition->first_sequence_index +
				h1_effects_random_integer(0, definition->final_sequence_count));
		}
		particle->state++;
	}
	if (particle->sequence_index == NONE || bitmap_group->sequences.count <= 0)
	{
		h1_particle_spawn_effect(particle, &definition->death_effect, 0.f);
		return false;
	}
	particle->sequence_index = (int16)PIN(particle->sequence_index, 0, bitmap_group->sequences.count - 1);
	return true;
}

// particles.c particle_next_frame
static bool h1_particle_next_frame(s_h1_particle* particle)
{
	const h1_part* definition = (const h1_part*)g_h1_cache_file->tag_get('part', particle->definition_index);
	const h1_bitm* bitmap_group = (const h1_bitm*)g_h1_cache_file->tag_get('bitm', definition->bitmap.index);
	particle->frame_time = 0.f;
	if (TEST_BIT(particle->flags, _h1_particle_animates_backwards_bit))
	{
		if (particle->frame_index > 0)
		{
			particle->frame_index--;
			return true;
		}
		if (!h1_particle_next_sequence(particle))
		{
			return false;
		}
		const h1_bitm_sequences* sequence = g_h1_cache_file->block_get(bitmap_group->sequences, particle->sequence_index);
		particle->frame_index = (int16)(sequence->sprites.count - 1);
		return true;
	}

	const h1_bitm_sequences* sequence = g_h1_cache_file->block_get(bitmap_group->sequences, particle->sequence_index);
	if (sequence && particle->frame_index + 1 < sequence->sprites.count)
	{
		particle->frame_index++;
		return true;
	}
	const bool result = h1_particle_next_sequence(particle);
	particle->frame_index = 0;
	return result;
}

static real32 h1_particle_radius(const s_h1_particle* particle)
{
	const h1_part* definition = (const h1_part*)g_h1_cache_file->tag_get('part', particle->definition_index);
	const real32 t = particle->lifespan > 0.f ? particle->age / particle->lifespan : 1.f;
	return ((definition->radius_animation.upper - definition->radius_animation.lower) * t + definition->radius_animation.lower) * particle->radius;
}

// particles.c particles_update for one particle, false when it's gone
static bool h1_particle_update(s_h1_particle* particle, real32 dt)
{
	const h1_part* definition = (const h1_part*)g_h1_cache_file->tag_get('part', particle->definition_index);
	if (!definition)
	{
		return false;
	}

	const bool was_new = particle->age == 0.f;
	particle->age += dt;
	if (particle->age >= particle->lifespan && !was_new && !definition->final_sequence_count)
	{
		h1_particle_spawn_effect(particle, &definition->death_effect, 0.f);
		return false;
	}

	// frame time
	if (!TEST_BIT(definition->flags, _h1_particle_definition_animation_stops_at_rest_bit) || !TEST_BIT(particle->flags, _h1_particle_at_rest_bit))
	{
		if (TEST_BIT(definition->flags, _h1_particle_definition_animate_once_per_frame_bit))
		{
			if (dt != 0.f && g_h1_effects.particle_update_ticks > 0 && !h1_particle_next_frame(particle))
			{
				return false;
			}
		}
		else
		{
			bool alive = true;
			if (particle->frame_time == -1.f)
			{
				alive = h1_particle_next_frame(particle);
				particle->frame_time = 0.f;
			}
			real32 remaining = dt;
			while (remaining > 0.f && alive)
			{
				const real32 frame_time_remaining = particle->frame_span - particle->frame_time;
				if (frame_time_remaining <= remaining)
				{
					alive = h1_particle_next_frame(particle);
					remaining -= frame_time_remaining;
				}
				else
				{
					particle->frame_time += remaining;
					break;
				}
			}
			if (!alive)
			{
				return false;
			}
		}
	}

	// physics
	if (!TEST_BIT(particle->flags, _h1_particle_at_rest_bit))
	{
		const h1_pphy* physics = (const h1_pphy*)g_h1_cache_file->tag_get('pphy', definition->physics.index);
		bool settled = false;
		if (physics)
		{
			real_vector3d collision_normal = { 0.f, 0.f, 1.f };
			const uint32 collision_flags = h1_point_physics_update(physics, &particle->position, &particle->velocity, &collision_normal, h1_particle_radius(particle), dt);
			if (TEST_BIT(collision_flags, _h1_point_physics_collided_with_structure_bit))
			{
				if (definition->collision_effect.index != NONE)
				{
					const real32 scale = PIN((h1_magnitude(&particle->velocity) - k_h1_particle_collision_scale_lower) /
						(k_h1_particle_collision_scale_upper - k_h1_particle_collision_scale_lower), 0.f, 1.f);
					h1_particle_spawn_effect(particle, &definition->collision_effect, scale);
				}
				if (TEST_BIT(definition->flags, _h1_particle_definition_dies_on_contact_with_structure_bit))
				{
					if (definition->collision_effect.index == NONE)
					{
						h1_particle_spawn_effect(particle, &definition->death_effect, 0.f);
					}
					return false;
				}
			}
			if (TEST_BIT(collision_flags, _h1_point_physics_in_air_bit) && TEST_BIT(definition->flags, _h1_particle_definition_dies_on_contact_with_air_bit))
			{
				h1_particle_spawn_effect(particle, &definition->death_effect, 0.f);
				return false;
			}
			if (TEST_BIT(collision_flags, _h1_point_physics_collided_with_structure_bit) || TEST_BIT(collision_flags, _h1_point_physics_collided_with_water_bit))
			{
				if (collision_normal.k > 0.8f)
				{
					settled = true;
				}
				particle->frame_span += definition->contact_deterioration;
			}
		}
		if (h1_dot(&particle->velocity, &particle->velocity) < 0.0625f)
		{
			if (settled)
			{
				if (TEST_BIT(definition->flags, _h1_particle_definition_dies_at_rest_bit))
				{
					h1_particle_spawn_effect(particle, &definition->death_effect, 0.f);
					return false;
				}
				SET_BIT(particle->flags, _h1_particle_at_rest_bit, true);
			}
		}
		else
		{
			particle->direction = particle->velocity;
		}
		particle->rotation += dt * particle->angular_velocity;
	}
	return true;
}

// point_physics.c point_physics_update (no wind, never under water)
static uint32 h1_point_physics_update(const h1_pphy* physics, real_point3d* position, real_vector3d* velocity, real_vector3d* collision_normal, real32 radius, real32 dt)
{
	uint32 result = 0;
	if (dt == 0.f)
	{
		return result;
	}

	const real32 radius_squared = radius * radius;
	const real32 radius_cubed = radius_squared * radius;
	real32 mass = (physics->mass_over_radius_cubed + k_h1_air_mass_over_radius_cubed) * radius_cubed;
	real32 buoyancy_scale = physics->air_gravity_scale;
	const real32 friction = physics->air_friction * radius_squared;
	SET_BIT(result, _h1_point_physics_in_air_bit, true);
	if (TEST_BIT(physics->flags, _h1_point_physics_no_gravity_bit))
	{
		buoyancy_scale = 0.f;
	}

	velocity->k += k_h1_global_gravity * k_h1_ticks_per_second * k_h1_ticks_per_second * buoyancy_scale * dt;

	real32 t;
	if (mass == 0.f)
	{
		t = friction == 0.f ? 0.f : 1.f;
	}
	else
	{
		t = PIN(dt / mass * friction, 0.f, 1.f);
	}
	velocity->i -= velocity->i * t;
	velocity->j -= velocity->j * t;
	velocity->k -= velocity->k * t;

	const bool collides = TEST_BIT(physics->flags, _h1_point_physics_structure_collisions_bit);
	for (int32 i = 0; dt != 0.f && i < k_h1_maximum_point_physics_collisions; i++)
	{
		const real_vector3d delta = { velocity->i * dt, velocity->j * dt, velocity->k * dt };
		collision_result collision;
		if (!collides || !collision_test_vector(FLAG(_collision_test_structure_bit), position, &delta, NONE, NONE, &collision))
		{
			position->x += delta.i;
			position->y += delta.j;
			position->z += delta.k;
			break;
		}

		SET_BIT(result, _h1_point_physics_collided_with_structure_bit, true);
		const real_vector3d normal = collision.fog_plane.n;
		if (collision_normal)
		{
			*collision_normal = normal;
		}

		// component_vectors_from_normal3d: the part along the normal and the part along the surface
		const real32 along = h1_dot(velocity, &normal);
		const real_vector3d parallel = { normal.i * along, normal.j * along, normal.k * along };
		const real_vector3d perpendicular = { velocity->i - parallel.i, velocity->j - parallel.j, velocity->k - parallel.k };
		velocity->i = (1.f - physics->surface_friction) * perpendicular.i - parallel.i * physics->elasticity;
		velocity->j = (1.f - physics->surface_friction) * perpendicular.j - parallel.j * physics->elasticity;
		velocity->k = (1.f - physics->surface_friction) * perpendicular.k - parallel.k * physics->elasticity;

		const real32 offset = MIN(radius, 0.005f);
		position->x = normal.i * offset + collision.point.x;
		position->y = normal.j * offset + collision.point.y;
		position->z = normal.k * offset + collision.point.z;
		dt -= collision.t * dt;
	}
	return result;
}

// render_sprite.c build_sprite: one sprite of a bitmap group sequence at a world point, queued for h1_effects_render
static void h1_sprite_build(const s_h1_shader_effect* shader, datum bitmap_tag_index, int16 mode, int16 sequence_index, int16 sprite_index,
	const real_point3d* origin, const real_vector3d* direction, real32 rotation, real32 scale, const real_argb_color* color, real32 fade, bool u_mirror, bool v_mirror)
{
	const h1_bitm* bitmap_group = (const h1_bitm*)g_h1_cache_file->tag_get('bitm', bitmap_tag_index);
	const h1_bitm_sequences* sequence = bitmap_group ? g_h1_cache_file->block_get(bitmap_group->sequences, sequence_index) : NULL;
	const h1_bitm_sequences_sprites* sprite = sequence ? g_h1_cache_file->block_get(sequence->sprites, sprite_index) : NULL;
	const h1_bitm_bitmaps* bitmap = sprite ? g_h1_cache_file->block_get(bitmap_group->bitmaps, sprite->bitmap_index) : NULL;
	if (!bitmap || g_h1_effects.sprites.size() >= k_h1_maximum_sprites)
	{
		return;
	}

	const s_frame* frame = global_window_parameters_get();
	const render_camera* camera = &frame->camera;
	const real_vector3d camera_left = h1_cross(&camera->up, &camera->forward);
	const real_vector3d to_origin = { origin->x - camera->point.x, origin->y - camera->point.y, origin->z - camera->point.z };

	// the sprite's plane
	real_vector3d basis_x = { -camera_left.i, -camera_left.j, -camera_left.k };
	real_vector3d basis_y = camera->up;
	const bool has_direction = direction && h1_magnitude(direction) > 0.f;
	if (mode == _h1_sprite_parallel_to_direction && has_direction)
	{
		basis_x = *direction;
		h1_normalize(&basis_x);
		basis_y = h1_cross(&to_origin, &basis_x);
		h1_normalize(&basis_y);
	}
	else if (mode == _h1_sprite_perpendicular_to_direction && has_direction)
	{
		const real_vector3d world_up = { 0.f, 0.f, 1.f };
		const real_vector3d world_left = { 0.f, 1.f, 0.f };
		const real_vector3d* reference = &world_up;
		const real32 dot = h1_dot(direction, reference);
		if (dot * dot > h1_dot(direction, direction) * 0.99f)
		{
			reference = &world_left;
		}
		basis_x = h1_cross(direction, reference);
		h1_normalize(&basis_x);
		real_vector3d up = *direction;
		h1_normalize(&up);
		basis_y = h1_cross(&basis_x, &up);
	}

	if (shader->framebuffer_fade_mode && mode != _h1_sprite_screen_facing)
	{
		const real_vector3d normal = h1_cross(&basis_x, &basis_y);
		const real32 facing = fabsf(h1_dot(&normal, &to_origin) / MAX(h1_magnitude(&to_origin), 0.0001f));
		fade *= shader->framebuffer_fade_mode == 2 ? 1.f - facing : facing;
	}

	real32 alpha;
	if (shader->framebuffer_blend_function && !TEST_BIT(shader->flags, _h1_shader_effect_uses_nonlinear_tint_bit))
	{
		alpha = fade;
	}
	else
	{
		alpha = color->alpha * fade;
	}

	// a screen facing sprite without a scale covers the same pixels at any distance
	if (mode == _h1_sprite_screen_facing && scale == 0.f)
	{
		const real32 viewport_height = (real32)MAX(camera->viewport_bounds.bottom - camera->viewport_bounds.top, 1);
		scale = h1_dot(&to_origin, &camera->forward) * 2.f * tanf(camera->vertical_field_of_view * 0.5f) / viewport_height;
	}
	scale *= (real32)bitmap->width;

	const real32 sine = rotation != 0.f ? sinf(rotation) : 0.f;
	const real32 cosine = rotation != 0.f ? cosf(rotation) : 1.f;
	s_h1_particle_vertex quad[4];
	for (int32 vertex = 0; vertex < 4; vertex++)
	{
		const real32 u = (((vertex >> 1) ^ vertex) & 1) ? sprite->right : sprite->left;
		const real32 v = (vertex & 2) ? sprite->top : sprite->bottom;
		const real32 offset_x = u - (sprite->left + sprite->registration_point.x);
		const real32 offset_y = (sprite->registration_point.y + sprite->top) - v;
		real32 x = offset_x * cosine - offset_y * sine;
		real32 y = offset_y * cosine + offset_x * sine;
		if (u_mirror)
		{
			x = -x;
		}
		if (v_mirror)
		{
			y = -y;
		}
		s_h1_particle_vertex* out = &quad[vertex];
		out->position[0] = (basis_x.i * x + basis_y.i * y) * scale + origin->x;
		out->position[1] = (basis_x.j * x + basis_y.j * y) * scale + origin->y;
		out->position[2] = (basis_x.k * x + basis_y.k * y) * scale + origin->z;
		out->color[0] = color->red;
		out->color[1] = color->green;
		out->color[2] = color->blue;
		out->texcoord[0] = u;
		out->texcoord[1] = v;
		out->alpha[0] = alpha;
		out->alpha[1] = 0.f;
	}

	// the corners go around the quad: 0 1 2, 0 2 3
	s_h1_sprite queued;
	queued.shader = shader;
	queued.bitmap_tag_index = bitmap_tag_index;
	queued.bitmap_index = sprite->bitmap_index;
	queued.vertices[0] = quad[0];
	queued.vertices[1] = quad[1];
	queued.vertices[2] = quad[2];
	queued.vertices[3] = quad[0];
	queued.vertices[4] = quad[2];
	queued.vertices[5] = quad[3];
	g_h1_effects.sprites.push_back(queued);
	return;
}

// render_sprite.c build_sprite_rotational: the face and the edge sequences of a sprite spinning about an axis, weighted by how the axis faces the camera
static void h1_sprite_build_rotational(const s_h1_shader_effect* shader, datum bitmap_tag_index, bool sideways_rotation_animates, int16 first_sequence_index, int16 sprite_index,
	const real_point3d* origin, const real_vector3d* axis, real32 rotation, real32 scale, const real_argb_color* color, real32 fade)
{
	const h1_bitm* bitmap_group = (const h1_bitm*)g_h1_cache_file->tag_get('bitm', bitmap_tag_index);
	if (!bitmap_group)
	{
		return;
	}
	const s_frame* frame = global_window_parameters_get();
	const render_camera* camera = &frame->camera;
	const real_vector3d camera_left = h1_cross(&camera->up, &camera->forward);
	const real_vector3d to_origin = { origin->x - camera->point.x, origin->y - camera->point.y, origin->z - camera->point.z };

	const real32 quarter_circle = _pi / 2.f;
	const real32 lengths = h1_magnitude(&to_origin) * h1_magnitude(axis);
	const real32 angle = (lengths > 0.f ? acosf(PIN(h1_dot(&to_origin, axis) / lengths, -1.f, 1.f)) : quarter_circle) - quarter_circle;
	real32 fraction = PIN(angle * angle / (quarter_circle * quarter_circle), 0.f, 1.f);

	if (fraction > 0.05f)
	{
		const h1_bitm_sequences* sequence = g_h1_cache_file->block_get(bitmap_group->sequences, first_sequence_index + 1);
		const int16 sprite_count = sequence ? (int16)sequence->sprites.count : 0;
		int16 edge_sprite_index = sprite_index;
		real32 sprite_rotation = rotation;
		bool mirror = false;
		if (sideways_rotation_animates && sprite_count > 0)
		{
			edge_sprite_index = (int16)(fmodf(sprite_count / (2.f * _pi) * rotation + 0.5f, (real32)sprite_count) + sprite_index);
			sprite_rotation = 0.f;
			if (angle < 0.f)
			{
				edge_sprite_index = sprite_count - sprite_index;
			}
		}
		else if (angle < 0.f)
		{
			mirror = true;
		}
		h1_sprite_build(shader, bitmap_tag_index, _h1_sprite_screen_facing, first_sequence_index + 1, edge_sprite_index, origin, NULL, sprite_rotation, scale, color, fraction * fade, mirror, false);
	}

	fraction = 1.f - fraction;
	if (fraction > 0.05f)
	{
		const h1_bitm_sequences* sequence = g_h1_cache_file->block_get(bitmap_group->sequences, first_sequence_index);
		const int16 sprite_count = sequence ? (int16)sequence->sprites.count : 0;
		const real32 axis_x = -h1_dot(axis, &camera_left);
		const real32 axis_y = h1_dot(axis, &camera->up);
		const int16 face_sprite_index = sprite_count > 0 ? (int16)fmodf(sprite_count / (2.f * _pi) * rotation + 0.5f, (real32)sprite_count) : 0;
		h1_sprite_build(shader, bitmap_tag_index, _h1_sprite_screen_facing, first_sequence_index, face_sprite_index, origin, NULL, atan2f(axis_y, axis_x), scale, color, fraction * fade, false, false);
	}
	return;
}

// render_particles.c render_particles
static void h1_particles_build_sprites(void)
{
	const s_frame* frame = global_window_parameters_get();
	const render_camera* camera = &frame->camera;
	const real32 viewport_height = (real32)MAX(camera->viewport_bounds.bottom - camera->viewport_bounds.top, 1);
	const real32 pixels_per_unit = viewport_height * 0.5f / tanf(camera->vertical_field_of_view * 0.5f);

	for (s_h1_particle& particle : g_h1_effects.particles)
	{
		const h1_part* definition = (const h1_part*)g_h1_cache_file->tag_get('part', particle.definition_index);
		if (!definition || particle.sequence_index == NONE || particle.frame_index < 0)
		{
			continue;
		}
		const real_vector3d to_particle = { particle.position.x - camera->point.x, particle.position.y - camera->point.y, particle.position.z - camera->point.z };
		const real32 depth = h1_dot(&to_particle, &camera->forward);
		if (depth <= camera->z_near)
		{
			continue;
		}

		const real32 radius = h1_particle_radius(&particle);
		const real32 diameter = radius * 2.f * pixels_per_unit / depth;
		if (diameter <= definition->fade_end_size)
		{
			continue;
		}
		real32 scale = radius * 2.f * definition->sprite_size;
		if (definition->minimum_size > 0.f && diameter < definition->minimum_size)
		{
			scale *= definition->minimum_size / diameter;
		}
		real32 fade = 1.f;
		const real32 remaining_life = particle.lifespan - particle.age;
		if (definition->fade_in_time > 0.f && particle.age < definition->fade_in_time)
		{
			fade = particle.age / definition->fade_in_time;
		}
		if (definition->fade_out_time > 0.f && remaining_life < definition->fade_out_time)
		{
			fade *= MAX(remaining_life, 0.f) / definition->fade_out_time;
		}
		const s_h1_shader_effect* shader = (const s_h1_shader_effect*)((const uint8*)definition + 0xB0);
		h1_sprite_build(shader, definition->bitmap.index, definition->orientation, particle.sequence_index, particle.frame_index, &particle.position, &particle.direction,
			particle.rotation, scale, &particle.color, fade, TEST_BIT(particle.flags, _h1_particle_u_mirror_bit), TEST_BIT(particle.flags, _h1_particle_v_mirror_bit));
	}
	return;
}

// particle_systems.c particle_system_render
static void h1_particle_systems_build_sprites(void)
{
	for (s_h1_particle_system& system : g_h1_effects.particle_systems)
	{
		const s_h1_pctl* definition = (const s_h1_pctl*)g_h1_cache_file->tag_get('pctl', system.definition_index);
		if (!definition)
		{
			continue;
		}
		for (int32 type_index = 0; type_index < definition->types.count && type_index < 4; type_index++)
		{
			const s_h1_pctl_type* type_definition = g_h1_cache_file->block_get(definition->types, type_index);
			s_h1_ps_type* type = &system.types[type_index];
			if (type->state_index == NONE || TEST_BIT(type_definition->flags, _h1_pctl_type_disabled_bit))
			{
				continue;
			}
			const s_h1_pctl_randomized_variables* multipliers = &type->variables.particle_state_randomized_multipliers;
			for (s_h1_ps_particle& particle : type->particles)
			{
				if (!particle.valid || particle.state_index == NONE)
				{
					continue;
				}
				const s_h1_pctl_particle_state* state_definition = g_h1_cache_file->block_get(type_definition->particle_states, particle.state_index);
				const s_h1_pctl_particle_state* transition_state_definition = particle.transition_state_index != NONE ?
					g_h1_cache_file->block_get(type_definition->particle_states, particle.transition_state_index) : NULL;
				if (!state_definition)
				{
					continue;
				}

				real32 state_weight = 1.f;
				real32 transition_weight = 0.f;
				real32 scale;
				real_argb_color color;
				if (!transition_state_definition)
				{
					scale = particle.randomized_variables.scale * multipliers->scale;
					color.alpha = particle.randomized_variables.color.alpha * multipliers->color.alpha;
					color.red = particle.randomized_variables.color.red * multipliers->color.red;
					color.green = particle.randomized_variables.color.green * multipliers->color.green;
					color.blue = particle.randomized_variables.color.blue * multipliers->color.blue;
				}
				else
				{
					state_weight = PIN(particle.time_left_in_state / particle.state_length, 0.f, 1.f);
					transition_weight = 1.f - state_weight;
					const s_h1_pctl_randomized_variables* a = &particle.transition_randomized_variables;
					const s_h1_pctl_randomized_variables* b = &particle.randomized_variables;
					scale = (a->scale * transition_weight + b->scale * state_weight) * multipliers->scale;
					color.alpha = (a->color.alpha * transition_weight + b->color.alpha * state_weight) * multipliers->color.alpha;
					color.red = (a->color.red * transition_weight + b->color.red * state_weight) * multipliers->color.red;
					color.green = (a->color.green * transition_weight + b->color.green * state_weight) * multipliers->color.green;
					color.blue = (a->color.blue * transition_weight + b->color.blue * state_weight) * multipliers->color.blue;
					if (state_definition->shader.framebuffer_blend_function == transition_state_definition->shader.framebuffer_blend_function &&
						state_definition->shader.primary_map_flags == transition_state_definition->shader.primary_map_flags &&
						state_definition->sequence_index == transition_state_definition->sequence_index)
					{
						state_weight = 1.f;
						transition_weight = 0.f;
					}
				}

				const bool rotational = type_definition->complex_sprite_render_mode == 1;
				const h1_bitm* bitmap_group = (const h1_bitm*)g_h1_cache_file->tag_get('bitm', state_definition->bitmaps.index);
				const h1_bitm_sequences* sequence = bitmap_group ? g_h1_cache_file->block_get(bitmap_group->sequences, state_definition->sequence_index + (rotational ? 1 : 0)) : NULL;
				if (!sequence || sequence->sprites.count <= 0)
				{
					continue;
				}
				int16 sprite_index;
				if (particle.sprite_index == -1.f)
				{
					particle.sprite_index = (real32)h1_effects_random_integer(0, sequence->sprites.count);
					sprite_index = (int16)particle.sprite_index;
				}
				else
				{
					sprite_index = (int16)((int32)particle.sprite_index % sequence->sprites.count);
					if (sprite_index < 0)
					{
						sprite_index += (int16)sequence->sprites.count;
					}
				}

				for (int32 pass = 0; pass < 2; pass++)
				{
					const s_h1_pctl_particle_state* pass_state = pass == 0 ? state_definition : transition_state_definition;
					const real32 weight = pass == 0 ? state_weight : transition_weight;
					if (!pass_state || weight <= 0.01f)
					{
						continue;
					}
					real_argb_color lit_color = color;
					if (!state_definition->shader.framebuffer_blend_function)
					{
						lit_color.red *= system.lighting.red;
						lit_color.green *= system.lighting.green;
						lit_color.blue *= system.lighting.blue;
					}
					if (rotational)
					{
						h1_sprite_build_rotational(&pass_state->shader, pass_state->bitmaps.index, TEST_BIT(type_definition->flags, _h1_pctl_type_rotational_sprites_animate_sideways_bit),
							pass_state->sequence_index, sprite_index, &particle.position, &particle.axis, particle.rotation, scale, &lit_color, weight);
					}
					else
					{
						h1_sprite_build(&pass_state->shader, pass_state->bitmaps.index, type_definition->sprite_render_mode, pass_state->sequence_index, sprite_index,
							&particle.position, &particle.axis, particle.rotation, scale, &lit_color, weight, false, false);
					}
				}
			}
		}
	}
	return;
}

// draws the queued sprites in batches of one shader and bitmap
static void h1_sprites_draw(void)
{
	if (g_h1_effects.sprites.empty())
	{
		return;
	}
	std::stable_sort(g_h1_effects.sprites.begin(), g_h1_effects.sprites.end(), [](const s_h1_sprite& a, const s_h1_sprite& b)
	{
		if (a.shader != b.shader) return a.shader < b.shader;
		if (a.bitmap_tag_index != b.bitmap_tag_index) return a.bitmap_tag_index < b.bitmap_tag_index;
		return a.bitmap_index < b.bitmap_index;
	});

	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	h1_render_set_camera_constants(NULL, false);
	device->SetVertexDeclaration(h1_render_vertex_declaration());
	device->SetVertexShader(h1_render_vertex_shader());

	size_t start = 0;
	while (start < g_h1_effects.sprites.size())
	{
		const s_h1_sprite& first = g_h1_effects.sprites[start];
		size_t end = start;
		g_h1_effects.vertices.clear();
		while (end < g_h1_effects.sprites.size() && g_h1_effects.sprites[end].shader == first.shader &&
			g_h1_effects.sprites[end].bitmap_tag_index == first.bitmap_tag_index && g_h1_effects.sprites[end].bitmap_index == first.bitmap_index)
		{
			g_h1_effects.vertices.insert(g_h1_effects.vertices.end(), g_h1_effects.sprites[end].vertices, g_h1_effects.sprites[end].vertices + 6);
			end++;
		}

		const s_h1_shader_effect* shader = first.shader;
		IDirect3DBaseTexture9* texture = h1_bitmap_texture_get(first.bitmap_tag_index, first.bitmap_index);
		IDirect3DBaseTexture9* secondary = shader->secondary_map.index != NONE && shader->secondary_map_anchor != 2 ? h1_bitmap_texture_get(shader->secondary_map.index, 0) : NULL;
		if (h1_render_particle_shader_bind(shader->framebuffer_blend_function, TEST_BIT(shader->flags, _h1_shader_effect_uses_nonlinear_tint_bit),
			shader->primary_map_flags, texture, secondary, shader->secondary_map_flags))
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, (UINT)(g_h1_effects.vertices.size() / 3), g_h1_effects.vertices.data(), sizeof(s_h1_particle_vertex));
			h1_render_shader_unbind();
		}
		start = end;
	}
	g_h1_effects.sprites.clear();
	return;
}

// particle_systems.c particle_system_new_unattached
static void h1_particle_system_new(datum definition_index, const real_point3d* position, const real_vector3d* velocity, const real_argb_color* color, real32 scale)
{
	const s_h1_pctl* definition = definition_index != NONE ? (const s_h1_pctl*)g_h1_cache_file->tag_get('pctl', definition_index) : NULL;
	if (!definition || g_h1_effects.particle_systems.size() >= k_h1_maximum_particle_systems)
	{
		return;
	}

	s_h1_particle_system system = {};
	system.definition_index = definition_index;
	system.position = *position;
	system.velocity = *velocity;
	system.color = *color;
	system.scale = scale;
	system.active = true;
	s_h1_render_lighting lighting;
	h1_render_lighting_at(position, &lighting);
	system.lighting.red = MIN(lighting.ambient.red + lighting.light0_color.red + lighting.light1_color.red, 1.f);
	system.lighting.green = MIN(lighting.ambient.green + lighting.light0_color.green + lighting.light1_color.green, 1.f);
	system.lighting.blue = MIN(lighting.ambient.blue + lighting.light0_color.blue + lighting.light1_color.blue, 1.f);

	// particle_system_initialize
	system.initializing = true;
	for (int32 type_index = 0; type_index < definition->types.count && type_index < 4; type_index++)
	{
		const s_h1_pctl_type* type_definition = g_h1_cache_file->block_get(definition->types, type_index);
		s_h1_ps_type* type = &system.types[type_index];
		const s_h1_pctl_type_state* state_definition = g_h1_cache_file->block_get(type_definition->type_states, 0);
		if (!state_definition)
		{
			return;
		}
		type->state_index = 0;
		type->transition_state_index = NONE;
		type->states_moving_forward = true;
		const real32 duration = h1_effects_random_range(state_definition->duration.lower, state_definition->duration.upper);
		type->time_left_in_state = duration;
		type->state_length = duration;
	}
	for (int32 type_index = definition->types.count; type_index < 4; type_index++)
	{
		system.types[type_index].state_index = NONE;
	}
	if (h1_particle_system_update(&system, 0.001f))
	{
		g_h1_effects.particle_systems.push_back(std::move(system));
	}
	return;
}

static void h1_pctl_randomize_variables(const s_h1_pctl_type* type_definition, s_h1_pctl_randomized_variables* variables, int16 state_index)
{
	const s_h1_pctl_particle_state* state_definition = g_h1_cache_file->block_get(type_definition->particle_states, state_index);
	if (!state_definition)
	{
		return;
	}
	const real32 color_fraction = h1_effects_random_real();
	variables->animation_rate = h1_effects_random_range(state_definition->animation_rate.lower, state_definition->animation_rate.upper);
	variables->rotation_rate = h1_effects_random_range(state_definition->rotation_rate.lower, state_definition->rotation_rate.upper);
	variables->scale = h1_effects_random_range(state_definition->scale.lower, state_definition->scale.upper);
	variables->color.alpha = h1_effects_random_range(state_definition->color_lower_bound.alpha, state_definition->color_upper_bound.alpha);
	variables->color.red = (state_definition->color_upper_bound.red - state_definition->color_lower_bound.red) * color_fraction + state_definition->color_lower_bound.red;
	variables->color.green = (state_definition->color_upper_bound.green - state_definition->color_lower_bound.green) * color_fraction + state_definition->color_lower_bound.green;
	variables->color.blue = (state_definition->color_upper_bound.blue - state_definition->color_lower_bound.blue) * color_fraction + state_definition->color_lower_bound.blue;
	return;
}

static real32 h1_pctl_physics_constant(const s_h1_pctl_type* type_definition, int32 index)
{
	const s_h1_pctl_physics_constant* constant = g_h1_cache_file->block_get(type_definition->physics_constants, index);
	return constant ? constant->k : 0.f;
}

// particle_systems.c particle_system_new_particles
static void h1_particle_system_new_particles(s_h1_particle_system* system, const s_h1_pctl_type* type_definition, s_h1_ps_type* type, real32 dt)
{
	const s_h1_pctl_type_state* state_definition = system->initializing ? NULL : g_h1_cache_file->block_get(type_definition->type_states, type->state_index);
	int32 target_particle_count;
	if (system->initializing)
	{
		target_particle_count = TEST_BIT(type_definition->flags, _h1_pctl_type_initial_count_scales_bit) ?
			(int32)((real32)type_definition->initial_particle_count * system->scale + 0.5f) : type_definition->initial_particle_count;
	}
	else
	{
		const real32 particle_count = dt * type->variables.particle_creation_rate;
		const int32 whole_particle_count = (int32)particle_count;
		target_particle_count = (int32)type->particles.size() + whole_particle_count;
		type->fractional_particle_count += particle_count - (real32)whole_particle_count;
		if (type->fractional_particle_count > 1.f)
		{
			target_particle_count++;
			type->fractional_particle_count -= 1.f;
		}
	}

	int32 particles_created = 0;
	while ((int32)type->particles.size() < target_particle_count && particles_created < 128 && h1_particle_system_particle_count() < k_h1_maximum_system_particles)
	{
		s_h1_ps_particle particle = {};
		particle.valid = true;
		particle.state_index = NONE;
		particle.transition_state_index = NONE;
		particle.states_moving_forward = true;
		particle.sprite_index = -1.f;
		particle.rotation = h1_effects_random_range(0.f, _pi * 2.f);

		const int16 creation_function = system->initializing ? type_definition->initial_particle_creation_physics : (state_definition ? state_definition->particle_creation_physics : 0);
		if (creation_function == 1)
		{
			// explosion
			const real32 xy_spread = h1_pctl_physics_constant(type_definition, 0);
			const real32 z_spread = h1_pctl_physics_constant(type_definition, 1);
			const real32 intensity = h1_pctl_physics_constant(type_definition, 2);
			particle.velocity = h1_effects_random_direction();
			particle.velocity.i *= xy_spread;
			particle.velocity.j *= xy_spread;
			particle.velocity.k *= z_spread;
			particle.position.x = system->position.x + particle.velocity.i;
			particle.position.y = system->position.y + particle.velocity.j;
			particle.position.z = system->position.z + particle.velocity.k;
			particle.axis = { particle.velocity.i, particle.velocity.j, 0.f };
			particle.velocity.i = particle.velocity.i * intensity + system->velocity.i;
			particle.velocity.j = particle.velocity.j * intensity + system->velocity.j;
			particle.velocity.k = particle.velocity.k * intensity + system->velocity.k;
			const real_vector3d up = { 0.f, 0.f, 1.f };
			h1_rotate_vector_about_axis(&particle.axis, &up, 1.f, 0.f);
		}
		else if (creation_function == 2)
		{
			// jet: an unattached system's marker faces nowhere
			const real32 velocity = h1_pctl_physics_constant(type_definition, 0) / k_h1_ticks_per_second;
			const real32 spread_fraction = h1_pctl_physics_constant(type_definition, 1);
			const real32 rotates_up = h1_pctl_physics_constant(type_definition, 2);
			const real32 spread_scale = velocity * spread_fraction;
			const real_vector3d spread = h1_effects_random_direction();
			particle.velocity.i = spread.i * spread_scale + system->velocity.i;
			particle.velocity.j = spread.j * spread_scale + system->velocity.j;
			particle.velocity.k = spread.k * spread_scale + system->velocity.k;
			particle.position = system->position;
			const real_vector3d up = { 0.f, 0.f, 1.f };
			const real_vector3d zero = { 0.f, 0.f, 0.f };
			particle.axis = rotates_up != 0.f ? h1_cross(&particle.velocity, &up) : h1_cross(&zero, &particle.velocity);
		}
		else
		{
			particle.position = system->position;
			particle.velocity = system->velocity;
		}
		type->particles.push_back(particle);
		particles_created++;
	}

	if ((real32)type->particles.size() < type->variables.minimum_particle_count)
	{
		type->time_left_in_state *= powf(0.3f, dt * k_h1_ticks_per_second);
	}
	return;
}

static int32 h1_particle_system_particle_count(void)
{
	int32 count = 0;
	for (const s_h1_particle_system& system : g_h1_effects.particle_systems)
	{
		for (const s_h1_ps_type& type : system.types)
		{
			count += (int32)type.particles.size();
		}
	}
	return count;
}

// particle_systems.c particle_system_update, false once the system is over
static bool h1_particle_system_update(s_h1_particle_system* system, real32 dt)
{
	const s_h1_pctl* definition = (const s_h1_pctl*)g_h1_cache_file->tag_get('pctl', system->definition_index);
	if (!definition)
	{
		return false;
	}

	// default and explosion system physics move an unattached system with its point physics
	const h1_pphy* system_physics = (const h1_pphy*)g_h1_cache_file->tag_get('pphy', definition->system_update_point_physics.index);
	if (system_physics)
	{
		h1_point_physics_update(system_physics, &system->position, &system->velocity, NULL, 1.f, dt);
	}

	int32 live_type_count = 0;
	for (int32 type_index = 0; type_index < definition->types.count && type_index < 4; type_index++)
	{
		const s_h1_pctl_type* type_definition = g_h1_cache_file->block_get(definition->types, type_index);
		s_h1_ps_type* type = &system->types[type_index];
		if (TEST_BIT(type_definition->flags, _h1_pctl_type_disabled_bit))
		{
			continue;
		}

		type->time_left_in_state -= dt;
		while (type->state_index != NONE)
		{
			const s_h1_pctl_type_state* state_definition = g_h1_cache_file->block_get(type_definition->type_states, type->state_index);
			if (type->time_left_in_state < 0.f)
			{
				real32 duration;
				if (type->transition_state_index == NONE)
				{
					// particle_system_next_type_state_index (an unattached system doesn't loop)
					const int16 step = type->states_moving_forward ? 1 : -1;
					const int16 next_state_index = type->state_index + step;
					type->transition_state_index = next_state_index;
					if (next_state_index < 0 || next_state_index >= type_definition->type_states.count)
					{
						type->state_index = NONE;
						type->transition_state_index = NONE;
					}
					duration = h1_effects_random_range(state_definition->transition_time.lower, state_definition->transition_time.upper);
				}
				else
				{
					type->state_index = type->transition_state_index;
					type->transition_state_index = NONE;
					const s_h1_pctl_type_state* transition_state_definition = g_h1_cache_file->block_get(type_definition->type_states, type->state_index);
					duration = h1_effects_random_range(transition_state_definition->duration.lower, transition_state_definition->duration.upper);
				}
				type->state_length = duration;
				type->time_left_in_state += duration;
				continue;
			}

			if (type->transition_state_index == NONE)
			{
				type->variables = state_definition->variables;
			}
			else
			{
				const s_h1_pctl_type_state* transition_state_definition = g_h1_cache_file->block_get(type_definition->type_states, type->transition_state_index);
				const real32 t = PIN(type->time_left_in_state / type->state_length, 0.f, 1.f);
				const real32* a = (const real32*)&state_definition->variables;
				const real32* b = (const real32*)&transition_state_definition->variables;
				real32* variables = (real32*)&type->variables;
				for (int32 i = 0; i < 10; i++)
				{
					variables[i] = a[i] * t + b[i] * (1.f - t);
				}
			}
			s_h1_pctl_randomized_variables* multipliers = &type->variables.particle_state_randomized_multipliers;
			if (TEST_BIT(type_definition->flags, _h1_pctl_type_tint_by_effect_color_bit))
			{
				multipliers->color.alpha *= system->color.alpha;
				multipliers->color.red *= system->color.red;
				multipliers->color.green *= system->color.green;
				multipliers->color.blue *= system->color.blue;
			}
			if (TEST_BIT(type_definition->flags, _h1_pctl_type_minimum_count_scales_bit)) type->variables.minimum_particle_count *= system->scale;
			if (TEST_BIT(type_definition->flags, _h1_pctl_type_creation_rate_scales_bit)) type->variables.particle_creation_rate *= system->scale;
			if (TEST_BIT(type_definition->flags, _h1_pctl_type_scale_scales_bit)) multipliers->scale *= system->scale;
			if (TEST_BIT(type_definition->flags, _h1_pctl_type_animation_rate_scales_bit)) multipliers->animation_rate *= system->scale;
			if (TEST_BIT(type_definition->flags, _h1_pctl_type_rotation_rate_scales_bit)) multipliers->rotation_rate *= system->scale;
			break;
		}

		if (type->state_index == NONE)
		{
			continue;
		}

		if (system->active)
		{
			h1_particle_system_new_particles(system, type_definition, type, dt);
		}

		const s_h1_pctl_type_state* type_state_definition = g_h1_cache_file->block_get(type_definition->type_states, type->state_index);
		for (size_t i = 0; i < type->particles.size();)
		{
			s_h1_ps_particle* particle = &type->particles[i];
			particle->time_left_in_state -= dt;
			if (particle->state_index == NONE && type_definition->particle_states.count > 0)
			{
				particle->state_index = 0;
				const s_h1_pctl_particle_state* state_definition = g_h1_cache_file->block_get(type_definition->particle_states, 0);
				const real32 duration = h1_effects_random_range(state_definition->duration.lower, state_definition->duration.upper);
				particle->time_left_in_state = duration;
				particle->state_length = duration;
				h1_pctl_randomize_variables(type_definition, &particle->randomized_variables, 0);
			}
			if (!particle->valid)
			{
				particle->state_index = NONE;
			}

			while (particle->state_index != NONE)
			{
				const s_h1_pctl_particle_state* state_definition = g_h1_cache_file->block_get(type_definition->particle_states, particle->state_index);
				if (particle->time_left_in_state >= 0.f)
				{
					break;
				}
				real32 duration;
				if (particle->transition_state_index == NONE)
				{
					// particle_system_next_particle_state_index
					const int16 step = particle->states_moving_forward ? 1 : -1;
					const int16 state_index = particle->state_index;
					const int16 next_state_index = state_index + step;
					particle->transition_state_index = next_state_index;
					if (next_state_index < 0 || next_state_index >= type_definition->particle_states.count)
					{
						const int32 state_count = type_definition->particle_states.count;
						if (TEST_BIT(type_definition->flags, _h1_pctl_type_particle_states_loop_bit) && state_count > 0)
						{
							if (TEST_BIT(type_definition->flags, _h1_pctl_type_particle_states_loop_forward_backward_bit))
							{
								particle->transition_state_index = (int16)PIN(state_index - step, 0, state_count - 1);
								particle->states_moving_forward = !particle->states_moving_forward;
							}
							else
							{
								particle->transition_state_index = 0;
							}
						}
						else
						{
							particle->state_index = NONE;
							particle->transition_state_index = NONE;
						}
					}
					duration = h1_effects_random_range(state_definition->transition_time.lower, state_definition->transition_time.upper);
				}
				else
				{
					particle->state_index = particle->transition_state_index;
					particle->transition_state_index = NONE;
					const s_h1_pctl_particle_state* transition_state_definition = g_h1_cache_file->block_get(type_definition->particle_states, particle->state_index);
					duration = h1_effects_random_range(transition_state_definition->duration.lower, transition_state_definition->duration.upper);
				}
				particle->state_length = duration;
				particle->time_left_in_state += duration;
				if (particle->transition_state_index != NONE)
				{
					h1_pctl_randomize_variables(type_definition, &particle->transition_randomized_variables, particle->transition_state_index);
				}
				else
				{
					particle->randomized_variables = particle->transition_randomized_variables;
				}
			}

			if (particle->state_index == NONE)
			{
				type->particles.erase(type->particles.begin() + i);
				continue;
			}

			const s_h1_pctl_randomized_variables* multipliers = &type->variables.particle_state_randomized_multipliers;
			const s_h1_pctl_particle_state* state_definition = g_h1_cache_file->block_get(type_definition->particle_states, particle->state_index);
			const s_h1_pctl_particle_state* transition_state_definition = particle->transition_state_index != NONE ?
				g_h1_cache_file->block_get(type_definition->particle_states, particle->transition_state_index) : NULL;
			real32 radius = type_definition->radius * type->variables.radius_multiplier * state_definition->radius;
			h1_pphy interpolated_physics;
			const h1_pphy* physics = (const h1_pphy*)g_h1_cache_file->tag_get('pphy', state_definition->point_physics.index);
			if (!transition_state_definition)
			{
				particle->rotation += particle->randomized_variables.rotation_rate * multipliers->rotation_rate * dt;
				particle->sprite_index += particle->randomized_variables.animation_rate * multipliers->animation_rate * dt;
			}
			else
			{
				const real32 t = PIN(particle->time_left_in_state / particle->state_length, 0.f, 1.f);
				particle->rotation += (particle->transition_randomized_variables.rotation_rate * (1.f - t) + particle->randomized_variables.rotation_rate * t) * multipliers->rotation_rate * dt;
				particle->sprite_index += (particle->transition_randomized_variables.animation_rate * (1.f - t) + particle->randomized_variables.animation_rate * t) * multipliers->animation_rate * dt;
				radius = type_definition->radius * type->variables.radius_multiplier * (state_definition->radius * t + transition_state_definition->radius * (1.f - t));
				const h1_pphy* transition_physics = (const h1_pphy*)g_h1_cache_file->tag_get('pphy', transition_state_definition->point_physics.index);
				if (physics && transition_physics)
				{
					// point_physics_definition_interpolate
					interpolated_physics = *physics;
					interpolated_physics.mass_over_radius_cubed = physics->mass_over_radius_cubed * t + transition_physics->mass_over_radius_cubed * (1.f - t);
					interpolated_physics.water_gravity_scale = physics->water_gravity_scale * t + transition_physics->water_gravity_scale * (1.f - t);
					interpolated_physics.air_gravity_scale = physics->air_gravity_scale * t + transition_physics->air_gravity_scale * (1.f - t);
					interpolated_physics.density = physics->density * t + transition_physics->density * (1.f - t);
					interpolated_physics.air_friction = physics->air_friction * t + transition_physics->air_friction * (1.f - t);
					interpolated_physics.water_friction = physics->water_friction * t + transition_physics->water_friction * (1.f - t);
					interpolated_physics.surface_friction = physics->surface_friction * t + transition_physics->surface_friction * (1.f - t);
					interpolated_physics.elasticity = physics->elasticity * t + transition_physics->elasticity * (1.f - t);
					physics = &interpolated_physics;
				}
			}

			// particle_system_update_particle_default
			if (type_state_definition && type_state_definition->particle_update_physics == 0 && physics)
			{
				const uint32 collision_flags = h1_point_physics_update(physics, &particle->position, &particle->velocity, NULL, radius, dt);
				if ((TEST_BIT(collision_flags, _h1_point_physics_in_air_bit) && TEST_BIT(type_definition->flags, _h1_pctl_type_dies_in_air_bit)) ||
					(TEST_BIT(collision_flags, _h1_point_physics_collided_with_structure_bit) && TEST_BIT(type_definition->flags, _h1_pctl_type_dies_on_ground_bit)))
				{
					particle->valid = false;
				}
			}
			i++;
		}
		live_type_count++;
	}
	system->initializing = false;
	return live_type_count > 0;
}

/* lights, attachments, decals and lens flares */

real32 h1_transition_function_evaluate(int16 function, real32 t)
{
	return h1_transition_function(function, t);
}

// periodic_functions.c transition_function_evaluate
static real32 h1_transition_function(int16 function, real32 t)
{
	t = PIN(t, 0.f, 1.f);
	switch (function)
	{
	case 1: return sqrtf(t);									// early
	case 2: return sqrtf(sqrtf(t));								// very early
	case 3: return t * t;										// late
	case 4: return (t * t) * (t * t);							// very late
	case 5: return (sinf(t * _pi - _pi * 0.5f) + 1.f) * 0.5f;	// cosine
	case 6: return 1.f;											// one
	case 7: return 0.f;											// zero
	default: return t;											// linear
	}
}

// object_lights.c light_new_unattached: every unattached light is dynamic
static void h1_light_new_unattached(datum definition_index, const real_point3d* position, const real_vector3d* forward, real32 scale)
{
	const h1_ligh* definition = definition_index != NONE ? (const h1_ligh*)g_h1_cache_file->tag_get('ligh', definition_index) : NULL;
	if (!definition || g_h1_effects.lights.size() >= k_h1_maximum_lights)
	{
		return;
	}
	s_h1_light light = {};
	light.definition_index = definition_index;
	light.attached = false;
	light.intensity_scale = scale;
	light.position = *position;
	light.forward = *forward;
	h1_normalize(&light.forward);
	light.up = h1_perpendicular(&light.forward);
	h1_normalize(&light.up);
	light.occlusion_fraction = 0.f;
	light.id = ++g_h1_effects.next_light_id;
	g_h1_effects.lights.push_back(light);
	return;
}

// object_lights.c lights_preprocess_scene: the light's intensity, color and radius, false once an unattached light is over
static bool h1_light_update(s_h1_light* light, real32 ticks)
{
	const h1_ligh* definition = (const h1_ligh*)g_h1_cache_file->tag_get('ligh', light->definition_index);
	if (!definition)
	{
		return false;
	}

	real32 intensity;
	if (light->attached)
	{
		// the light's object function (h1_attachments_update), its change color is white
		intensity = PIN(light->intensity_scale, 0.f, 1.f);
		h1_rgb_colors_interpolate(&light->color, definition->interpolation_flags, &definition->color_lower_bound.rgb, &definition->color_upper_bound.rgb, intensity);
		if (definition->color_lower_bound.alpha > k_real_epsilon || definition->color_upper_bound.alpha > k_real_epsilon)
		{
			const real32 alpha = (1.f - intensity) * definition->color_lower_bound.alpha + intensity * definition->color_upper_bound.alpha;
			light->color.red = alpha * light->color.red + (1.f - alpha);
			light->color.green = alpha * light->color.green + (1.f - alpha);
			light->color.blue = alpha * light->color.blue + (1.f - alpha);
		}
	}
	else
	{
		light->age_ticks += ticks;
		if (light->age_ticks > definition->duration)
		{
			return false;
		}
		const real32 t = definition->duration > 0.f ? light->age_ticks / definition->duration : 0.f;
		intensity = (1.f - h1_transition_function(definition->falloff_function, t)) * light->intensity_scale;
		h1_rgb_colors_interpolate(&light->color, definition->interpolation_flags, &definition->color_lower_bound.rgb, &definition->color_upper_bound.rgb, intensity);
	}
	light->color.red = PIN(light->color.red, 0.f, 1.f);
	light->color.green = PIN(light->color.green, 0.f, 1.f);
	light->color.blue = PIN(light->color.blue, 0.f, 1.f);
	light->intensity = intensity;
	light->radius = (definition->radius_modifier.lower * (1.f - intensity) + definition->radius_modifier.upper * intensity) * definition->radius;
	return true;
}

static bool h1_light_is_dynamic(const s_h1_light* light)
{
	if (!light->attached)
	{
		return true;
	}
	const h1_ligh* definition = (const h1_ligh*)g_h1_cache_file->tag_get('ligh', light->definition_index);
	return definition && TEST_BIT(definition->flags, _h1_light_dynamic_bit);
}

// the nearest dynamic lights become the shader's lights for the frame
static void h1_lights_build_shader_constants(void)
{
	memset(g_h1_effects.light_constants, 0, sizeof(g_h1_effects.light_constants));
	const s_frame* frame = global_window_parameters_get();
	const real_point3d camera = frame->camera.point;

	std::vector<std::pair<real32, const s_h1_light*>> candidates;
	auto consider = [&](const s_h1_light* light)
	{
		if (light->radius <= 0.f || (light->color.red == 0.f && light->color.green == 0.f && light->color.blue == 0.f) || !h1_light_is_dynamic(light))
		{
			return;
		}
		const real_vector3d offset = { light->position.x - camera.x, light->position.y - camera.y, light->position.z - camera.z };
		candidates.push_back({ h1_magnitude(&offset) - light->radius, light });
	};
	for (const s_h1_light& light : g_h1_effects.lights)
	{
		consider(&light);
	}
	for (const s_h1_attachment& attachment : g_h1_effects.attachments)
	{
		if (attachment.group_tag == 'ligh')
		{
			consider(&attachment.light);
		}
	}
	std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

	for (size_t i = 0; i < candidates.size() && i < k_h1_maximum_shader_lights; i++)
	{
		const s_h1_light* light = candidates[i].second;
		real32* position = g_h1_effects.light_constants[2 * i];
		real32* color = g_h1_effects.light_constants[2 * i + 1];
		position[0] = light->position.x;
		position[1] = light->position.y;
		position[2] = light->position.z;
		position[3] = 1.f / light->radius;
		color[0] = light->color.red;
		color[1] = light->color.green;
		color[2] = light->color.blue;
	}
	return;
}

void h1_effects_set_light_constants(void)
{
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	static const real32 k_no_lights[2 * k_h1_maximum_shader_lights][4] = {};
	device->SetPixelShaderConstantF(110, h1_maps_active() ? &g_h1_effects.light_constants[0][0] : &k_no_lights[0][0], 2 * k_h1_maximum_shader_lights);
	return;
}

// the halo 2 object's markers of a halo 1 marker name (halo 2 names them with underscores)
static int16 h1_object_markers_get_third_person(datum object_index, const char* name, object_marker* markers, int16 maximum_count)
{
	if (!name || !name[0])
	{
		return 0;
	}
	char marker_name[32];
	strncpy_s(marker_name, name, _TRUNCATE);
	for (char* c = marker_name; *c; c++)
	{
		if (*c == ' ')
		{
			*c = '_';
		}
	}
	return object_get_markers_by_string_id(object_index, string_id_find_or_add(marker_name), markers, maximum_count);
}

static int16 h1_object_markers_get(datum object_index, const char* name, object_marker* markers, int16 maximum_count)
{
	if (!name || !name[0])
	{
		return 0;
	}
	// the local player's weapon is seen in first person: its attachments follow its first person model's markers
	real_matrix4x3 first_person_marker;
	if (maximum_count > 0 && h1_first_person_marker_get(object_index, name, &first_person_marker))
	{
		markers[0].node_index = 0;
		markers[0].region_index = 0;
		markers[0].matrix = first_person_marker;
		markers[0].node_matrix = first_person_marker;
		markers[0].radius = 0.f;
		return 1;
	}
	char marker_name[32];
	strncpy_s(marker_name, name, _TRUNCATE);
	for (char* c = marker_name; *c; c++)
	{
		if (*c == ' ')
		{
			*c = '_';
		}
	}
	return object_get_markers_by_string_id(object_index, string_id_find_or_add(marker_name), markers, maximum_count);
}

// effects.c effect_build_locations: each location at every marker of its name, the object's origin when there is none
static void h1_attachment_effect_build_locations(s_h1_effect* effect, datum object_index, const object_datum* object, const char* attachment_marker)
{
	const h1_effe* definition = (const h1_effe*)g_h1_cache_file->tag_get('effe', effect->definition_index);
	if (!definition)
	{
		return;
	}
	effect->locations.resize(definition->locations.count);
	for (int32 i = 0; i < definition->locations.count; i++)
	{
		const h1_effe_locations* location = g_h1_cache_file->block_get(definition->locations, i);
		std::vector<s_h1_effect_location>& instances = effect->locations[i];
		instances.clear();

		object_marker markers[k_h1_maximum_attachment_markers];
		const char* marker_name = location->marker_name[0] ? location->marker_name : attachment_marker;
		const int16 marker_count = h1_object_markers_get_third_person(object_index, marker_name, markers, k_h1_maximum_attachment_markers);
		for (int16 j = 0; j < marker_count; j++)
		{
			instances.push_back({ markers[j].matrix.position, markers[j].matrix.vectors.forward, markers[j].matrix.vectors.up, false });
		}
		if (instances.empty())
		{
			instances.push_back({ object->object.position, object->object.forward, object->object.up, false });
		}
		// the local player's weapon also has its first person model's marker
		real_matrix4x3 first_person_marker;
		if (marker_name && marker_name[0] && h1_first_person_marker_get(object_index, marker_name, &first_person_marker))
		{
			instances.push_back({ first_person_marker.position, first_person_marker.vectors.forward, first_person_marker.vectors.up, true });
			effect->first_person = true;
		}
	}
	return;
}

void h1_effect_new_on_object(datum h1_effect_index, datum object_index)
{
	h1_effect_new_on_object_ex(h1_effect_index, object_index, 1.f, 1.f, false);
	return;
}

int32 h1_effect_new_on_object_ex(datum h1_effect_index, datum object_index, real32 scale_a, real32 scale_b, bool looping)
{
	static int32 s_next_id = 0;
	const h1_effe* definition = h1_effect_index != NONE ? (const h1_effe*)g_h1_cache_file->tag_get('effe', h1_effect_index) : NULL;
	const object_datum* object = (const object_datum*)object_try_and_get(object_index);
	if (!definition || !object || definition->events.count <= 0 || g_h1_effects.effects.size() >= k_h1_maximum_effects)
	{
		return 0;
	}
	s_h1_effect effect = {};
	effect.definition_index = h1_effect_index;
	// halo 2 velocities are per second, halo 1's per tick
	effect.velocity =
	{
		object->object.translational_velocity.i / k_h1_ticks_per_second,
		object->object.translational_velocity.j / k_h1_ticks_per_second,
		object->object.translational_velocity.k / k_h1_ticks_per_second
	};
	effect.scale_a = scale_a;
	effect.scale_b = scale_b;
	effect.color = { 1.f, 1.f, 1.f };
	effect.id = ++s_next_id > 0 ? s_next_id : (s_next_id = 1);
	effect.follow_object_index = looping ? object_index : NONE;
	effect.loop = looping;
	h1_attachment_effect_build_locations(&effect, object_index, object, "");
	h1_effect_set_event(&effect, 0);
	if (!h1_effect_update(&effect, 0.f))
	{
		return 0;
	}
	const int32 id = effect.id;
	g_h1_effects.effects.push_back(std::move(effect));
	return id;
}

void h1_effect_stop(int32 id)
{
	for (s_h1_effect& effect : g_h1_effects.effects)
	{
		if (id != 0 && effect.id == id)
		{
			// effects.c effect_stop: no new events, the particles already made live on
			effect.follow_object_index = NONE;
			effect.loop = false;
			effect.stopped = true;
		}
	}
	return;
}

// objects.c object_attachments_new and effects.c effect_update of looping effects: the effects and lights a halo 1 object carries
static void h1_attachments_update(real32 dt)
{
	g_h1_effects.attachment_leftover_ticks += dt * k_h1_ticks_per_second;
	int32 ticks = (int32)g_h1_effects.attachment_leftover_ticks;
	g_h1_effects.attachment_leftover_ticks -= (real32)ticks;
	ticks = MIN(ticks, 3);

	for (s_h1_attachment& attachment : g_h1_effects.attachments)
	{
		attachment.seen = false;
	}

	object_iterator iterator;
	object_iterator_new(&iterator, _object_mask_all, 0);
	while (const object_datum* object = (const object_datum*)object_iterator_next(&iterator))
	{
		const datum h1_definition_index = h1_objects_h1_definition_get(object->definition_index);
		// every halo 1 object definition starts with the object fields, the attachments are at 0x140
		const h1_proj* definition = h1_definition_index != NONE ? (const h1_proj*)g_h1_cache_file->tag_get('obje', h1_definition_index) : NULL;
		if (!definition || definition->attachments.count <= 0)
		{
			continue;
		}
		const datum object_index = iterator.index;

		for (int16 i = 0; i < definition->attachments.count; i++)
		{
			const h1_proj_attachments* attachment_definition = g_h1_cache_file->block_get(definition->attachments, i);
			const uint32 group_tag = attachment_definition->type.group_tag;
			if ((group_tag != 'effe' && group_tag != 'ligh') || attachment_definition->type.index == NONE)
			{
				continue;
			}

			s_h1_attachment* attachment = NULL;
			for (s_h1_attachment& existing : g_h1_effects.attachments)
			{
				if (existing.object_index == object_index && existing.attachment_index == i)
				{
					attachment = &existing;
					break;
				}
			}
			if (!attachment)
			{
				if (g_h1_effects.attachments.size() >= k_h1_maximum_attachments)
				{
					continue;
				}
				s_h1_attachment created = {};
				created.object_index = object_index;
				created.attachment_index = i;
				created.group_tag = group_tag;
				if (group_tag == 'effe')
				{
					const h1_effe* effect_definition = (const h1_effe*)g_h1_cache_file->tag_get('effe', attachment_definition->type.index);
					if (!effect_definition || effect_definition->events.count <= 0)
					{
						continue;
					}
					// effect_new_looping: the object's functions scale it, halo 2 objects have none so they're 1
					created.effect.definition_index = attachment_definition->type.index;
					created.effect.velocity = { 0.f, 0.f, 0.f };
					created.effect.scale_a = 1.f;
					created.effect.scale_b = 1.f;
					created.effect.color = { 1.f, 1.f, 1.f };
					created.effect.loop = true;
					h1_effect_set_event(&created.effect, 0);
				}
				else
				{
					const h1_ligh* light_definition = (const h1_ligh*)g_h1_cache_file->tag_get('ligh', attachment_definition->type.index);
					// light_new: only dynamic lights and lights with lens flares exist
					if (!light_definition || (!TEST_BIT(light_definition->flags, _h1_light_dynamic_bit) && light_definition->lens_flare.index == NONE))
					{
						continue;
					}
					created.light.definition_index = attachment_definition->type.index;
					created.light.attached = true;
					created.light.intensity_scale = 1.f;
					created.light.occlusion_fraction = 0.f;
					created.light.id = ++g_h1_effects.next_light_id;
				}
				g_h1_effects.attachments.push_back(std::move(created));
				attachment = &g_h1_effects.attachments.back();
			}
			attachment->seen = true;

			// effects.c effect_update of looping effects and object_lights.c: the attachment's primary scale function
			const int16 function_index = attachment_definition->primary_scale > 0 ? (int16)(attachment_definition->primary_scale - 1) : (int16)NONE;
			real32 function_value;
			const bool function_active = h1_object_function_value_get(object_index, function_index, &function_value);
			if (group_tag == 'effe')
			{
				attachment->effect.scale_a = function_value;
				for (int32 tick = 0; tick < ticks; tick++)
				{
					if (!function_active)
					{
						attachment->effect.stopped = true;
						break;
					}
					h1_attachment_effect_build_locations(&attachment->effect, object_index, object, attachment_definition->marker);
					// halo 2 velocities are per second, halo 1's per tick
					attachment->effect.velocity =
					{
						object->object.translational_velocity.i / k_h1_ticks_per_second,
						object->object.translational_velocity.j / k_h1_ticks_per_second,
						object->object.translational_velocity.k / k_h1_ticks_per_second
					};
					if (attachment->effect.stopped)
					{
						attachment->effect.stopped = false;
						h1_effect_set_event(&attachment->effect, 0);
					}
					h1_effect_update(&attachment->effect, 1.f / k_h1_ticks_per_second);
				}
			}
			else
			{
				attachment->light.intensity_scale = function_value;
				real_matrix4x3 first_person_marker;
				attachment->light.first_person = h1_first_person_marker_get(object_index, attachment_definition->marker, &first_person_marker);
				object_marker marker;
				if (h1_object_markers_get(object_index, attachment_definition->marker, &marker, 1) > 0)
				{
					attachment->light.position = marker.matrix.position;
					attachment->light.forward = marker.matrix.vectors.forward;
					attachment->light.up = marker.matrix.vectors.up;
				}
				else
				{
					attachment->light.position = object->object.position;
					attachment->light.forward = object->object.forward;
					attachment->light.up = object->object.up;
				}
			}
		}
	}

	// the attachments of objects that are gone go with them
	g_h1_effects.attachments.erase(std::remove_if(g_h1_effects.attachments.begin(), g_h1_effects.attachments.end(),
		[](const s_h1_attachment& attachment) { return !attachment.seen; }), g_h1_effects.attachments.end());
	return;
}

static void h1_lights_update(real32 dt)
{
	const real32 ticks = dt * k_h1_ticks_per_second;
	for (size_t i = 0; i < g_h1_effects.lights.size();)
	{
		if (!h1_light_update(&g_h1_effects.lights[i], ticks))
		{
			g_h1_effects.lights.erase(g_h1_effects.lights.begin() + i);
			continue;
		}
		i++;
	}
	for (s_h1_attachment& attachment : g_h1_effects.attachments)
	{
		if (attachment.group_tag == 'ligh')
		{
			h1_light_update(&attachment.light, ticks);
		}
	}
	h1_lights_build_shader_constants();
	return;
}

/* decals */

struct s_h1_decal_clip_vertex
{
	real_point3d position;
	real32 x;
	real32 y;
};

// clips a polygon in decal space to one side of an axis aligned line: keeps coordinate * sign >= limit * sign
static void h1_decal_clip(std::vector<s_h1_decal_clip_vertex>* polygon, bool y_axis, real32 limit, real32 sign)
{
	std::vector<s_h1_decal_clip_vertex> result;
	const size_t count = polygon->size();
	for (size_t i = 0; i < count; i++)
	{
		const s_h1_decal_clip_vertex& a = (*polygon)[i];
		const s_h1_decal_clip_vertex& b = (*polygon)[(i + 1) % count];
		const real32 da = ((y_axis ? a.y : a.x) - limit) * sign;
		const real32 db = ((y_axis ? b.y : b.x) - limit) * sign;
		if (da >= 0.f)
		{
			result.push_back(a);
		}
		if ((da >= 0.f) != (db >= 0.f))
		{
			const real32 t = da / (da - db);
			s_h1_decal_clip_vertex v;
			v.position.x = a.position.x + (b.position.x - a.position.x) * t;
			v.position.y = a.position.y + (b.position.y - a.position.y) * t;
			v.position.z = a.position.z + (b.position.z - a.position.z) * t;
			v.x = a.x + (b.x - a.x) * t;
			v.y = a.y + (b.y - a.y) * t;
			result.push_back(v);
		}
	}
	polygon->swap(result);
	return;
}

// decals.c decal_projection_create and decal_clip_to_surface: the decal's rectangle on the structure surfaces under it, wrapping over
// edges onto surfaces up to the wrap angle
static void h1_decal_build_geometry(std::vector<s_h1_particle_vertex>* vertices, const real_point3d* center, const real_vector3d* normal,
	const real_vector3d* forward, const real_vector3d* left, const real32 extent[4], const real32 bounds[4], const real_rgb_color* color)
{
	const real32 reach_x = MAX(fabsf(extent[0]), fabsf(extent[1]));
	const real32 reach_y = MAX(fabsf(extent[2]), fabsf(extent[3]));
	const real32 reach = sqrtf(reach_x * reach_x + reach_y * reach_y);
	const real32 minimum_cosine = cosf(k_h1_decal_maximum_wrap_angle);
	const int32 triangle_count = h1_render_structure_triangle_count();

	// the triangles near the decal, and which way the structure winds: the triangle under the hit faces the surface normal
	std::vector<int32> candidates;
	real32 winding = 0.f;
	for (int32 i = 0; i < triangle_count; i++)
	{
		const real_point3d* points = h1_render_structure_triangle_get(i);
		bool near_center = true;
		for (int32 axis = 0; axis < 3 && near_center; axis++)
		{
			const real32 c = (&center->x)[axis];
			const real32 a = (&points[0].x)[axis], b = (&points[1].x)[axis], d = (&points[2].x)[axis];
			near_center = MIN(MIN(a, b), d) <= c + reach && MAX(MAX(a, b), d) >= c - reach;
		}
		if (!near_center)
		{
			continue;
		}
		candidates.push_back(i);

		if (winding == 0.f)
		{
			const real_vector3d ab = { points[1].x - points[0].x, points[1].y - points[0].y, points[1].z - points[0].z };
			const real_vector3d ac = { points[2].x - points[0].x, points[2].y - points[0].y, points[2].z - points[0].z };
			real_vector3d n = h1_cross(&ab, &ac);
			if (h1_magnitude(&n) < 1e-6f)
			{
				continue;
			}
			h1_normalize(&n);
			const real_vector3d to_center = { center->x - points[0].x, center->y - points[0].y, center->z - points[0].z };
			if (fabsf(h1_dot(&to_center, &n)) > 0.05f || fabsf(h1_dot(&n, normal)) < 0.9f)
			{
				continue;
			}
			// the hit is inside the triangle
			bool inside = true;
			for (int32 edge = 0; edge < 3 && inside; edge++)
			{
				const real_point3d& p0 = points[edge];
				const real_point3d& p1 = points[(edge + 1) % 3];
				const real_vector3d e = { p1.x - p0.x, p1.y - p0.y, p1.z - p0.z };
				const real_vector3d to_point = { center->x - p0.x, center->y - p0.y, center->z - p0.z };
				const real_vector3d c = h1_cross(&e, &to_point);
				inside = h1_dot(&c, &n) >= -1e-4f;
			}
			if (inside)
			{
				winding = h1_dot(&n, normal) > 0.f ? 1.f : -1.f;
			}
		}
	}

	std::vector<s_h1_decal_clip_vertex> polygon;
	for (int32 index : candidates)
	{
		const real_point3d* points = h1_render_structure_triangle_get(index);
		const real_vector3d ab = { points[1].x - points[0].x, points[1].y - points[0].y, points[1].z - points[0].z };
		const real_vector3d ac = { points[2].x - points[0].x, points[2].y - points[0].y, points[2].z - points[0].z };
		real_vector3d n = h1_cross(&ab, &ac);
		if (h1_magnitude(&n) < 1e-6f)
		{
			continue;
		}
		h1_normalize(&n);
		const real32 facing = winding != 0.f ? h1_dot(&n, normal) * winding : fabsf(h1_dot(&n, normal));
		if (facing < minimum_cosine)
		{
			continue;
		}
		if (winding == 0.f && h1_dot(&n, normal) < 0.f)
		{
			n = { -n.i, -n.j, -n.k };
		}
		else if (winding < 0.f)
		{
			n = { -n.i, -n.j, -n.k };
		}

		// the decal's box: the rectangle, as deep as it is wide
		polygon.clear();
		bool in_front = false, behind = false;
		for (int32 k = 0; k < 3; k++)
		{
			const real_vector3d offset = { points[k].x - center->x, points[k].y - center->y, points[k].z - center->z };
			const real32 depth = h1_dot(&offset, normal);
			in_front |= depth < reach;
			behind |= depth > -reach;
			polygon.push_back({ points[k], h1_dot(&offset, forward), h1_dot(&offset, left) });
		}
		if (!in_front || !behind)
		{
			continue;
		}
		h1_decal_clip(&polygon, false, extent[0], 1.f);
		h1_decal_clip(&polygon, false, extent[1], -1.f);
		h1_decal_clip(&polygon, true, extent[2], 1.f);
		h1_decal_clip(&polygon, true, extent[3], -1.f);
		if (polygon.size() < 3)
		{
			continue;
		}

		auto emit = [&](const s_h1_decal_clip_vertex& v)
		{
			s_h1_particle_vertex out = {};
			out.position[0] = v.position.x + n.i * k_h1_decal_offset;
			out.position[1] = v.position.y + n.j * k_h1_decal_offset;
			out.position[2] = v.position.z + n.k * k_h1_decal_offset;
			out.color[0] = color->red;
			out.color[1] = color->green;
			out.color[2] = color->blue;
			out.texcoord[0] = bounds[0] + (v.x - extent[0]) / (extent[1] - extent[0]) * (bounds[1] - bounds[0]);
			out.texcoord[1] = bounds[2] + (v.y - extent[2]) / (extent[3] - extent[2]) * (bounds[3] - bounds[2]);
			out.alpha[0] = 1.f;
			out.alpha[1] = 0.f;
			vertices->push_back(out);
		};
		for (size_t k = 1; k + 1 < polygon.size(); k++)
		{
			if (vertices->size() + 3 > k_h1_maximum_decal_vertices)
			{
				return;
			}
			emit(polygon[0]);
			emit(polygon[k]);
			emit(polygon[k + 1]);
		}
	}
	return;
}

// decals.c decal_new and decal_new_from_collision: the decal where the velocity hits the structure, then the decals chained to it
static void h1_decal_new(datum definition_index, const real_point3d* origin, const real_vector3d* velocity, real32 radius_modifier)
{
	collision_result collision;
	if (definition_index == NONE || !collision_test_vector(FLAG(_collision_test_structure_bit), origin, velocity, NONE, NONE, &collision))
	{
		return;
	}
	const real_vector3d normal = collision.fog_plane.n;

	bool reuse_previous_geometry = false;
	real_vector3d forward = {}, left = {};
	int16 sequence_index = 0;
	real32 radius = 0.f;
	std::vector<s_h1_particle_vertex> previous_vertices;
	for (int32 chain = 0; definition_index != NONE && chain < 8; chain++)
	{
		const h1_deca* definition = (const h1_deca*)g_h1_cache_file->tag_get('deca', definition_index);
		const h1_bitm* bitmap_group = definition ? (const h1_bitm*)g_h1_cache_file->tag_get('bitm', definition->map.index) : NULL;
		if (!bitmap_group || bitmap_group->bitmaps.count <= 0)
		{
			return;
		}

		if (!reuse_previous_geometry)
		{
			real_vector3d tangent, bitangent;
			real32 cosine, sine;
			if (TEST_BIT(definition->flags, _h1_decal_no_random_rotation_bit) && h1_dot(&normal, velocity) < -k_real_epsilon)
			{
				cosine = -1.f;
				sine = 0.f;
				tangent = h1_cross(&normal, velocity);
				bitangent = h1_cross(&normal, &tangent);
			}
			else
			{
				const real32 angle = h1_effects_random_range(0.f, 2.f * _pi);
				cosine = cosf(angle);
				sine = sinf(angle);
				tangent = h1_perpendicular(&normal);
				bitangent = h1_cross(&normal, &tangent);
			}
			h1_normalize(&tangent);
			h1_normalize(&bitangent);
			forward = { cosine * bitangent.i - tangent.i * sine, cosine * bitangent.j - tangent.j * sine, cosine * bitangent.k - tangent.k * sine };
			left = { tangent.i * cosine + bitangent.i * sine, tangent.j * cosine + bitangent.j * sine, tangent.k * cosine + bitangent.k * sine };

			sequence_index = (int16)h1_effects_random_integer(0, MAX(bitmap_group->sequences.count, 1));
			if (radius_modifier == 0.f)
			{
				radius_modifier = 1.f;
			}
			radius = h1_effects_random_range(definition->radius.lower, definition->radius.upper) * radius_modifier;
		}

		// decal_sprite_get_bounds: sprites keep their size relative to the largest sprite, other bitmaps cover the radius
		int16 bitmap_index = 0;
		real32 extent[4], bounds[4];
		const h1_bitm_sequences* sequence = g_h1_cache_file->block_get(bitmap_group->sequences, sequence_index);
		const h1_bitm_sequences_sprites* sprite = bitmap_group->type == _h1_bitmap_type_sprites && sequence && sequence->sprites.count > 0 ?
			g_h1_cache_file->block_get(sequence->sprites, 0) : NULL;
		const h1_bitm_bitmaps* sprite_bitmap = sprite ? g_h1_cache_file->block_get(bitmap_group->bitmaps, sprite->bitmap_index) : NULL;
		if (sprite_bitmap && definition->maximum_sprite_extent > 0.f)
		{
			bitmap_index = sprite->bitmap_index;
			const real32 aspect = TEST_BIT(definition->flags, _h1_decal_preserve_aspect_bit) ?
				((sprite->right - sprite->left) / (sprite->bottom - sprite->top)) * ((real32)sprite_bitmap->height / (real32)sprite_bitmap->width) : 1.f;
			const real32 scale = radius / definition->maximum_sprite_extent;
			const real32 width_scale = (real32)sprite_bitmap->width * scale;
			const real32 height_scale = (real32)sprite_bitmap->height * scale * aspect;
			extent[0] = -sprite->registration_point.x * width_scale;
			extent[1] = (sprite->right - sprite->registration_point.x - sprite->left) * width_scale;
			extent[2] = -sprite->registration_point.y * height_scale;
			extent[3] = (sprite->bottom - sprite->registration_point.y - sprite->top) * height_scale;
			bounds[0] = sprite->left;
			bounds[1] = sprite->right;
			bounds[2] = sprite->top;
			bounds[3] = sprite->bottom;
		}
		else
		{
			const h1_bitm_bitmaps* bitmap = g_h1_cache_file->block_get(bitmap_group->bitmaps, 0);
			const real32 aspect = TEST_BIT(definition->flags, _h1_decal_preserve_aspect_bit) && bitmap->width > 0 ? (real32)bitmap->height / (real32)bitmap->width : 1.f;
			extent[0] = -radius;
			extent[1] = radius;
			extent[2] = -aspect * radius;
			extent[3] = aspect * radius;
			bounds[0] = bounds[2] = 0.f;
			bounds[1] = bounds[3] = 1.f;
		}
		if (extent[1] - extent[0] <= 0.f || extent[3] - extent[2] <= 0.f)
		{
			return;
		}

		s_h1_decal decal;
		decal.definition_index = definition_index;
		decal.bitmap_tag_index = definition->map.index;
		decal.bitmap_index = bitmap_index;
		decal.age = 0.f;
		decal.lifetime = h1_effects_random_range(definition->lifetime.lower, definition->lifetime.upper);
		decal.decay_time = h1_effects_random_range(definition->decay_time.lower, definition->decay_time.upper);
		decal.intensity = h1_effects_random_range(definition->intensity.lower, definition->intensity.upper);
		real_rgb_color color;
		h1_rgb_colors_interpolate(&color, (definition->flags >> _h1_decal_color_interpolate_in_hsv_bit) & 3,
			&definition->color_lower_bounds, &definition->color_upper_bounds, h1_effects_random_real());
		h1_decal_build_geometry(&decal.vertices, &collision.point, &normal, &forward, &left, extent, bounds, &color);
		if (!decal.vertices.empty())
		{
			if (g_h1_effects.decals.size() >= k_h1_maximum_decals)
			{
				g_h1_effects.decals.erase(g_h1_effects.decals.begin());
			}
			g_h1_effects.decals.push_back(std::move(decal));
		}

		reuse_previous_geometry = TEST_BIT(definition->flags, _h1_decal_geometry_inherited_by_next_decal_in_chain_bit);
		definition_index = definition->next_decal_in_chain.index;
	}
	return;
}

// decals.c decal_update: decals fade out over their decay time at the end of their lifetime
static void h1_decals_update(real32 dt)
{
	for (size_t i = 0; i < g_h1_effects.decals.size();)
	{
		s_h1_decal& decal = g_h1_effects.decals[i];
		decal.age += dt;
		if (decal.lifetime > 0.f && decal.age >= decal.lifetime)
		{
			g_h1_effects.decals.erase(g_h1_effects.decals.begin() + i);
			continue;
		}
		real32 fade = 1.f;
		if (decal.lifetime > 0.f && decal.decay_time > 0.f && decal.lifetime - decal.age < decal.decay_time)
		{
			fade = (decal.lifetime - decal.age) / decal.decay_time;
		}
		for (s_h1_particle_vertex& vertex : decal.vertices)
		{
			vertex.alpha[0] = decal.intensity * fade;
		}
		i++;
	}
	return;
}

void h1_effects_render_decals(void)
{
	if (!h1_maps_active() || !g_h1_cache_file || g_h1_effects.decals.empty())
	{
		return;
	}
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	h1_render_set_camera_constants(NULL, false);
	device->SetVertexDeclaration(h1_render_vertex_declaration());
	device->SetVertexShader(h1_render_vertex_shader());
	const real32 depth_bias = -0.000002f;
	const real32 slope_bias = -1.f;
	device->SetRenderState(D3DRS_DEPTHBIAS, *(const DWORD*)&depth_bias);
	device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(const DWORD*)&slope_bias);

	for (const s_h1_decal& decal : g_h1_effects.decals)
	{
		const h1_deca* definition = (const h1_deca*)g_h1_cache_file->tag_get('deca', decal.definition_index);
		IDirect3DBaseTexture9* texture = definition ? h1_bitmap_texture_get(decal.bitmap_tag_index, decal.bitmap_index) : NULL;
		if (!texture || decal.vertices.empty())
		{
			continue;
		}
		// decal maps are clamped
		if (h1_render_particle_shader_bind(definition->framebuffer_blend_function, false, FLAG(1) | FLAG(2), texture, NULL, 0, _h1_particle_shader_mode_decal))
		{
			// the environment fog pass fogs the decals with the surface under them
			h1_fog_set_shader_constants(_h1_fog_shader_mode_none, NULL);
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, (UINT)(decal.vertices.size() / 3), decal.vertices.data(), sizeof(s_h1_particle_vertex));
			h1_render_shader_unbind();
		}
	}

	device->SetRenderState(D3DRS_DEPTHBIAS, 0);
	device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, 0);
	return;
}

/* lens flares */

// rasterizer_lights.c lens_flare_evaluate_corona_rotation_function, in turns
static real32 h1_lens_flare_corona_rotation(int16 function, const real_vector3d* direction, const real_point3d* position, const render_camera* camera,
	const real_vector3d* camera_left)
{
	real32 sine = 0.f, cosine = 1.f;
	real_vector3d offset;
	switch (function)
	{
	case 1:	// eye in light space
	{
		real_vector3d plane = h1_cross(direction, &camera->forward);
		plane = h1_cross(&plane, direction);
		sine = h1_dot(&plane, &camera->forward);
		cosine = -h1_dot(direction, &camera->forward);
		break;
	}
	case 2:	// light in eye space
		offset = { -direction->i, -direction->j, -direction->k };
		sine = h1_dot(&camera->forward, &offset);
		cosine = -h1_dot(&camera->up, &offset);
		break;
	case 3:	// eye to light in light space
	{
		real_vector3d plane = h1_cross(direction, &camera->forward);
		plane = h1_cross(&plane, direction);
		offset = { position->x - camera->point.x, position->y - camera->point.y, position->z - camera->point.z };
		sine = h1_dot(&plane, &offset);
		cosine = -h1_dot(direction, &offset);
		break;
	}
	case 4:	// eye to light in eye space
		offset = { position->x - camera->point.x, position->y - camera->point.y, position->z - camera->point.z };
		sine = h1_dot(&camera->forward, &offset);
		cosine = -h1_dot(&camera->up, &offset);
		break;
	default:
		return 0.f;
	}
	return sine != 0.f ? atan2f(sine, cosine) / (2.f * _pi) : 0.f;
}

// object_lights.c lights_preprocess_scene lens flare submit and rasterizer_lights.c rasterizer_lens_flares_draw; the occlusion test is
// a ray to the occlusion point instead of the hardware pixel count
static void h1_lens_flare_render(s_h1_light* light)
{
	const h1_ligh* definition = (const h1_ligh*)g_h1_cache_file->tag_get('ligh', light->definition_index);
	const h1_lens* lens = definition && definition->lens_flare.index != NONE ? (const h1_lens*)g_h1_cache_file->tag_get('lens', definition->lens_flare.index) : NULL;
	if (!lens || lens->reflections.count <= 0 || (light->color.red == 0.f && light->color.green == 0.f && light->color.blue == 0.f))
	{
		return;
	}

	const s_frame* frame = global_window_parameters_get();
	const render_camera* camera = &frame->camera;
	const real_vector3d camera_left = h1_cross(&camera->up, &camera->forward);
	const real_point3d corona_position = light->position;
	const real_vector3d eye_to_corona = { corona_position.x - camera->point.x, corona_position.y - camera->point.y, corona_position.z - camera->point.z };
	const real32 depth = h1_dot(&camera->forward, &eye_to_corona);
	if (depth <= 0.f || (lens->far_fade_distance != 0.f && depth >= lens->far_fade_distance))
	{
		return;
	}

	// rasterizer_lens_flares_submit_occlusion_tests
	real_point3d occlusion_point = corona_position;
	switch (lens->occlusion_offset_direction)
	{
	case 0:	// toward the viewer
		occlusion_point.x -= camera->forward.i * lens->occlusion_radius;
		occlusion_point.y -= camera->forward.j * lens->occlusion_radius;
		occlusion_point.z -= camera->forward.k * lens->occlusion_radius;
		break;
	case 1:	// marker forward
		occlusion_point.x += light->forward.i * lens->occlusion_radius * 1.41421356f;
		occlusion_point.y += light->forward.j * lens->occlusion_radius * 1.41421356f;
		occlusion_point.z += light->forward.k * lens->occlusion_radius * 1.41421356f;
		break;
	default:
		break;
	}
	h1_lens_flare_occlusion_test(light, &occlusion_point, lens->occlusion_radius);
	const real32 occlusion_fraction = light->occlusion_fraction;
	if (occlusion_fraction <= 0.f)
	{
		return;
	}

	real_vector3d corona_axis =
	{
		(camera->forward.i * depth - eye_to_corona.i) * 2.f,
		(camera->forward.j * depth - eye_to_corona.j) * 2.f,
		(camera->forward.k * depth - eye_to_corona.k) * 2.f
	};
	real32 light_brightness = lens->far_fade_distance > 0.f ?
		PIN((depth - lens->far_fade_distance) / (lens->near_fade_distance - lens->far_fade_distance), 0.f, 1.f) : 1.f;
	light_brightness *= occlusion_fraction;
	const real32 corona_rotation = h1_lens_flare_corona_rotation(lens->rotation_function, &light->forward, &corona_position, camera, &camera_left) * lens->rotation_function_scale;
	const real32 screen_rotation = atan2f(h1_dot(&camera->up, &eye_to_corona), h1_dot(&camera_left, &eye_to_corona)) * (180.f / _pi);

	real_vector3d eye_direction = eye_to_corona;
	h1_normalize(&eye_direction);
	const real32 cosine_range = lens->cosine_falloff_angle - lens->cosine_cutoff_angle;
	auto cosine_scale = [&](real32 cosine) -> real32
	{
		if (fabsf(cosine_range) < k_real_epsilon)
		{
			return cosine >= lens->cosine_cutoff_angle ? 1.f : 0.f;
		}
		return PIN((cosine - lens->cosine_cutoff_angle) / cosine_range, 0.f, 1.f);
	};
	const real32 scale_functions[4] =
	{
		1.f,
		cosine_scale(-h1_dot(&camera->forward, &light->forward)),
		cosine_scale(-h1_dot(&light->forward, &eye_direction)),
		cosine_scale(h1_dot(&camera->forward, &eye_direction)),
	};

	const real32 light_scale = PIN(light->intensity, 0.f, 1.f);
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	for (int32 i = 0; i < lens->reflections.count; i++)
	{
		const h1_lens_reflections* reflection = g_h1_cache_file->block_get(lens->reflections, i);
		const real32 brightness = (reflection->brightness.lower + (reflection->brightness.upper - reflection->brightness.lower) * light_scale) *
			scale_functions[PIN(reflection->brightness_scaled_by, 0, 3)] * light_brightness;
		if (i == 0)
		{
			light_brightness = brightness;
		}
		if (brightness <= 0.f)
		{
			continue;
		}

		real32 radius = reflection->radius.lower + (reflection->radius.upper - reflection->radius.lower) * light_scale;
		real_rgb_color color = light->color;
		const real_argb_color& tint = reflection->tint_color;
		if (tint.alpha != 0.f || tint.red != 0.f || tint.green != 0.f || tint.blue != 0.f)
		{
			// the tint factor blends the tint from white
			color.red = 1.f + (tint.red - 1.f) * tint.alpha;
			color.green = 1.f + (tint.green - 1.f) * tint.alpha;
			color.blue = 1.f + (tint.blue - 1.f) * tint.alpha;
		}

		real32 rotation;
		real32 scale_x = 1.f, scale_y = 1.f;
		if (i == 0)
		{
			rotation = corona_rotation + reflection->rotation_offset;
			scale_x = lens->horizontal_scale != 0.f ? lens->horizontal_scale : 1.f;
			scale_y = lens->vertical_scale != 0.f ? lens->vertical_scale : 1.f;
		}
		else
		{
			rotation = reflection->rotation_offset;
		}
		if (TEST_BIT(reflection->flags, _h1_lens_reflection_rotate_from_center_of_screen_bit))
		{
			rotation += screen_rotation;
		}
		if (TEST_BIT(reflection->flags, _h1_lens_reflection_radius_scaled_by_occlusion_bit))
		{
			radius = (occlusion_fraction + 1.f) * radius * 0.5f;
		}
		if (TEST_BIT(reflection->flags, _h1_lens_reflection_radius_not_scaled_by_distance_bit))
		{
			radius *= depth;
		}
		if (radius <= 0.f)
		{
			continue;
		}

		IDirect3DBaseTexture9* texture = h1_bitmap_texture_get(lens->bitmap.index, reflection->bitmap_index);
		if (!texture)
		{
			continue;
		}

		const real_point3d point =
		{
			reflection->position * corona_axis.i + corona_position.x,
			reflection->position * corona_axis.j + corona_position.y,
			reflection->position * corona_axis.k + corona_position.z
		};
		const real32 sine = sinf(rotation * (_pi / 180.f));
		const real32 cosine = cosf(rotation * (_pi / 180.f));
		static const real32 k_corners[4][2] = { { -1.f, -1.f }, { 1.f, -1.f }, { 1.f, 1.f }, { -1.f, 1.f } };
		s_h1_particle_vertex quad[4];
		for (int32 k = 0; k < 4; k++)
		{
			const real32 x = k_corners[k][0] * scale_x * radius;
			const real32 y = k_corners[k][1] * scale_y * radius;
			const real32 right = x * cosine - y * sine;
			const real32 up = x * sine + y * cosine;
			s_h1_particle_vertex* out = &quad[k];
			out->position[0] = point.x - camera_left.i * right + camera->up.i * up;
			out->position[1] = point.y - camera_left.j * right + camera->up.j * up;
			out->position[2] = point.z - camera_left.k * right + camera->up.k * up;
			out->color[0] = color.red;
			out->color[1] = color.green;
			out->color[2] = color.blue;
			out->texcoord[0] = (k_corners[k][0] + 1.f) * 0.5f;
			out->texcoord[1] = (1.f - k_corners[k][1]) * 0.5f;
			out->alpha[0] = PIN(brightness, 0.f, 1.f);
			out->alpha[1] = 0.f;
		}
		const s_h1_particle_vertex triangles[6] = { quad[0], quad[1], quad[2], quad[0], quad[2], quad[3] };
		if (h1_render_particle_shader_bind(0 /* alpha blend */, false, FLAG(1) | FLAG(2), texture, NULL, 0, _h1_particle_shader_mode_lens_flare))
		{
			device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, triangles, sizeof(s_h1_particle_vertex));
			device->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
			h1_render_shader_unbind();
		}
	}
	return;
}

// rasterizer_widget_submit_occlusion_test: a camera facing square of the occlusion radius drawn twice, depth tested and not, into
// occlusion queries; their pixel counts become the light's occlusion fraction when they finish (a frame or so later)
static void h1_lens_flare_occlusion_test(s_h1_light* light, const real_point3d* point, real32 radius)
{
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	s_h1_flare_query* query = NULL;
	for (s_h1_flare_query& candidate : g_h1_effects.flare_queries)
	{
		if (!candidate.pending)
		{
			query = &candidate;
			break;
		}
	}
	if (!query)
	{
		if (g_h1_effects.flare_queries.size() >= 256)
		{
			return;
		}
		s_h1_flare_query created = {};
		if (FAILED(device->CreateQuery(D3DQUERYTYPE_OCCLUSION, &created.visible)) || FAILED(device->CreateQuery(D3DQUERYTYPE_OCCLUSION, &created.total)))
		{
			if (created.visible) created.visible->Release();
			return;
		}
		g_h1_effects.flare_queries.push_back(created);
		query = &g_h1_effects.flare_queries.back();
	}

	const render_camera* camera = &global_window_parameters_get()->camera;
	const real_vector3d camera_left = h1_cross(&camera->up, &camera->forward);
	// at least a pixel or so across
	const real_vector3d to_point = { point->x - camera->point.x, point->y - camera->point.y, point->z - camera->point.z };
	const real32 pixel = h1_dot(&to_point, &camera->forward) * 2.f * tanf(camera->vertical_field_of_view * 0.5f) /
		(real32)MAX(camera->viewport_bounds.bottom - camera->viewport_bounds.top, 1);
	radius = MAX(radius, pixel * 1.5f);
	static const real32 k_corners[4][2] = { { -1.f, -1.f }, { 1.f, -1.f }, { 1.f, 1.f }, { -1.f, 1.f } };
	s_h1_particle_vertex quad[4] = {};
	for (int32 k = 0; k < 4; k++)
	{
		quad[k].position[0] = point->x - camera_left.i * k_corners[k][0] * radius + camera->up.i * k_corners[k][1] * radius;
		quad[k].position[1] = point->y - camera_left.j * k_corners[k][0] * radius + camera->up.j * k_corners[k][1] * radius;
		quad[k].position[2] = point->z - camera_left.k * k_corners[k][0] * radius + camera->up.k * k_corners[k][1] * radius;
	}
	const s_h1_particle_vertex triangles[6] = { quad[0], quad[1], quad[2], quad[0], quad[2], quad[3] };

	D3DVIEWPORT9 viewport;
	device->GetViewport(&viewport);
	if (light->first_person)
	{
		D3DVIEWPORT9 first_person_viewport = viewport;
		first_person_viewport.MinZ = 0.f;
		first_person_viewport.MaxZ = k_h1_first_person_depth_range;
		device->SetViewport(&first_person_viewport);
	}
	device->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
	device->SetRenderState(D3DRS_COLORWRITEENABLE1, 0);
	device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	device->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	query->visible->Issue(D3DISSUE_BEGIN);
	device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, triangles, sizeof(s_h1_particle_vertex));
	query->visible->Issue(D3DISSUE_END);
	device->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
	query->total->Issue(D3DISSUE_BEGIN);
	device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, triangles, sizeof(s_h1_particle_vertex));
	query->total->Issue(D3DISSUE_END);
	device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
	device->SetViewport(&viewport);
	query->light_id = light->id;
	query->pending = true;
	return;
}

// the occlusion queries that finished set their light's occlusion fraction
static void h1_lens_flare_occlusion_results(void)
{
	for (s_h1_flare_query& query : g_h1_effects.flare_queries)
	{
		if (!query.pending)
		{
			continue;
		}
		DWORD visible = 0, total = 0;
		if (query.visible->GetData(&visible, sizeof(visible), 0) != S_OK || query.total->GetData(&total, sizeof(total), 0) != S_OK)
		{
			continue;
		}
		query.pending = false;
		const real32 fraction = total > 0 ? PIN((real32)visible / (real32)total, 0.f, 1.f) : 0.f;
		for (s_h1_light& light : g_h1_effects.lights)
		{
			if (light.id == query.light_id) light.occlusion_fraction = fraction;
		}
		for (s_h1_attachment& attachment : g_h1_effects.attachments)
		{
			if (attachment.group_tag == 'ligh' && attachment.light.id == query.light_id) attachment.light.occlusion_fraction = fraction;
		}
	}
	return;
}

void h1_effects_render_lens_flares(void)
{
	h1_lens_flares_render();
	return;
}

static void h1_lens_flares_render(void)
{
	h1_lens_flare_occlusion_results();
	bool any = false;
	for (const s_h1_light& light : g_h1_effects.lights)
	{
		any |= light.radius >= 0.f;
	}
	for (const s_h1_attachment& attachment : g_h1_effects.attachments)
	{
		any |= attachment.group_tag == 'ligh';
	}
	if (!any)
	{
		return;
	}
	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	h1_render_set_camera_constants(NULL, false);
	device->SetVertexDeclaration(h1_render_vertex_declaration());
	device->SetVertexShader(h1_render_vertex_shader());
	for (s_h1_light& light : g_h1_effects.lights)
	{
		h1_lens_flare_render(&light);
	}
	for (s_h1_attachment& attachment : g_h1_effects.attachments)
	{
		if (attachment.group_tag == 'ligh')
		{
			h1_lens_flare_render(&attachment.light);
		}
	}
	return;
}

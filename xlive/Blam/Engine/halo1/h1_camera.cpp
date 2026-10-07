#include "stdafx.h"
#include "h1_camera.h"
#include "h1_game_state.h"

#include "h1_cache_file.h"
#include "h1_log.h"
#include "h1_map_loader.h"

#include "camera/observer.h"
#include "game/game_time.h"
#include "math/real_math.h"
#include "objects/objects.h"
#include "scenario/scenario.h"

/*
* camera_scripting.c: the scripted camera of halo 1's cutscenes. A camera point (or a point relative to an object's center)
* with its orientation and field of view, reached over a transition time; halo 1's observer moves to it smoothly, here the
* scripted camera eases from where the camera was. Halo 2's own scripted camera (camera_control) keeps the director out of
* first person; the result replaces the observer's.
*/

/* ---------- constants */

enum e_h1_camera_mode
{
	_h1_camera_mode_none = NONE,
	_h1_camera_mode_point = 0,
	_h1_camera_mode_first_person,
};

constexpr real32 k_h1_camera_default_field_of_view = 1.22173047f;	// 70 degrees

/* ---------- structures */

struct s_h1_camera_state
{
	real_point3d position;
	real_vector3d forward;
	real_vector3d up;
	real32 field_of_view;
};

struct s_h1_camera_globals
{
	bool enabled;
	int16 mode;
	int16 camera_point_index;
	real32 timer;			// seconds left of the transition
	real32 transition_time;	// its length
	real_point3d point;
	real_vector3d forward;
	real_vector3d up;
	real32 field_of_view;
	datum relative_object_index;

	bool have_current;		// what the camera showed last frame, the start of a transition
	s_h1_camera_state current;
	s_h1_camera_state start;
};

/* ---------- globals */

static s_h1_camera_globals g_h1_camera;
H1_GAME_STATE_VARIABLE(g_h1_camera);

/* ---------- prototypes */

static void h1_camera_target_get(s_h1_camera_state* out_target);
static void h1_camera_state_interpolate(const s_h1_camera_state* a, const s_h1_camera_state* b, real32 t, s_h1_camera_state* out_state);

/* ---------- public code */

void h1_camera_reset(void)
{
	g_h1_camera = {};
	g_h1_camera.mode = _h1_camera_mode_none;
	g_h1_camera.camera_point_index = NONE;
	g_h1_camera.relative_object_index = NONE;
	g_h1_camera.field_of_view = k_h1_camera_default_field_of_view;
	return;
}

void h1_camera_control(bool enabled)
{
	if (!enabled)
	{
		// halo 1's scripted camera ends with its control (a first person camera_set_first_person left on the cutscene's unit,
		// destroyed by then, would hold the view where it was): halo 2's scripted camera mode back to none
		uint8* scripted_camera = *Memory::GetAddress<uint8**>(0x4CDFA0);
		*(int16*)(scripted_camera + 2) = 0;
		*(datum*)(scripted_camera + 0x3C) = NONE;
		g_h1_camera.mode = _h1_camera_mode_none;
	}
	// halo 2's camera_control: the director leaves first person for its scripted camera
	Memory::GetAddress<void(__cdecl*)(bool)>(0x5A181)(enabled);
	h1_log("camera: camera_control %d", enabled);
	g_h1_camera.enabled = enabled;
	g_h1_camera.have_current = false;
	return;
}

void h1_camera_set(int16 camera_point_index, int16 transition_ticks, datum relative_object_index)
{
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();
	const h1_scnr_cutscene_camera_points* camera_point = g_h1_cache_file->block_get(scenario->cutscene_camera_points, camera_point_index);
	if (!camera_point)
	{
		return;
	}
	if (g_h1_camera.have_current)
	{
		g_h1_camera.start = g_h1_camera.current;
	}
	g_h1_camera.mode = _h1_camera_mode_point;
	g_h1_camera.camera_point_index = camera_point_index;
	g_h1_camera.point = camera_point->position;
	vectors3d_from_euler_angles3d(&g_h1_camera.forward, &g_h1_camera.up, &camera_point->orientation);
	g_h1_camera.field_of_view = camera_point->field_of_view != 0.f ? camera_point->field_of_view : k_h1_camera_default_field_of_view;
	g_h1_camera.relative_object_index = relative_object_index;
	// halo 1 drops the fraction of a second
	g_h1_camera.timer = (real32)(transition_ticks / 30);
	g_h1_camera.transition_time = g_h1_camera.timer;
	return;
}

void h1_camera_set_first_person(datum unit_index)
{
	if (unit_index == NONE)
	{
		return;
	}
	g_h1_camera.mode = _h1_camera_mode_first_person;
	g_h1_camera.relative_object_index = unit_index;
	// halo 2's camera_set_first_person
	Memory::GetAddress<void(__cdecl*)(datum)>(0x97A48)(unit_index);
	return;
}

bool h1_camera_scripted(void)
{
	return h1_maps_active() && g_h1_camera.enabled;
}

int16 h1_camera_time(void)
{
	return (int16)(g_h1_camera.timer * 30.f);
}

bool h1_camera_observer_override(int32 user_index, real32 dt, s_observer_result* result)
{
	if (!h1_maps_active() || !g_h1_camera.enabled || g_h1_camera.mode != _h1_camera_mode_point || user_index != 0)
	{
		return false;
	}

	s_h1_camera_state target;
	h1_camera_target_get(&target);
	if (!g_h1_camera.have_current)
	{
		// the first camera of a cutscene cuts to its point
		g_h1_camera.start = target;
		g_h1_camera.timer = 0.f;
	}

	// halo 1's observer moves along a cubic over the transition, from rest to rest
	s_h1_camera_state state = target;
	if (g_h1_camera.timer > 0.f && g_h1_camera.transition_time > 0.f)
	{
		const real32 t = 1.f - g_h1_camera.timer / g_h1_camera.transition_time;
		h1_camera_state_interpolate(&g_h1_camera.start, &target, t * t * (3.f - 2.f * t), &state);
	}
	g_h1_camera.timer = MAX(0.f, g_h1_camera.timer - game_time_get_speed() * dt);
	g_h1_camera.current = state;
	g_h1_camera.have_current = true;

	// the observer's vertical field of view keeps its ratio to the horizontal one
	const real32 ratio = result->horizontal_field_of_view > 0.f ? tanf(result->vertical_field_of_view * 0.5f) / tanf(result->horizontal_field_of_view * 0.5f) : 0.75f;
	result->position = state.position;
	result->forward = state.forward;
	result->up = state.up;
	result->horizontal_field_of_view = state.field_of_view;
	result->vertical_field_of_view = 2.f * atanf(tanf(state.field_of_view * 0.5f) * ratio);
	result->velocity = *global_zero_vector3d;
	scenario_location_from_point(&result->location, &result->position);
	return true;
}

/* ---------- private code */

// scripted_camera_update: the camera point, relative to its object's bounding sphere center
static void h1_camera_target_get(s_h1_camera_state* out_target)
{
	out_target->position = g_h1_camera.point;
	if (g_h1_camera.relative_object_index != NONE)
	{
		const object_datum* object = (const object_datum*)object_try_and_get_and_verify_type(g_h1_camera.relative_object_index, _object_mask_all);
		if (object)
		{
			out_target->position.x += object->object.center.x;
			out_target->position.y += object->object.center.y;
			out_target->position.z += object->object.center.z;
		}
	}
	out_target->forward = g_h1_camera.forward;
	out_target->up = g_h1_camera.up;
	out_target->field_of_view = g_h1_camera.field_of_view;
	return;
}

static void h1_camera_state_interpolate(const s_h1_camera_state* a, const s_h1_camera_state* b, real32 t, s_h1_camera_state* out_state)
{
	points_interpolate(&a->position, &b->position, t, &out_state->position);
	real_vector3d forward = { a->forward.i + (b->forward.i - a->forward.i) * t, a->forward.j + (b->forward.j - a->forward.j) * t, a->forward.k + (b->forward.k - a->forward.k) * t };
	real_vector3d up = { a->up.i + (b->up.i - a->up.i) * t, a->up.j + (b->up.j - a->up.j) * t, a->up.k + (b->up.k - a->up.k) * t };
	normalize3d_with_default(&forward, &b->forward);
	// up perpendicular to forward
	const real32 d = dot_product3d(&up, &forward);
	up.i -= forward.i * d;
	up.j -= forward.j * d;
	up.k -= forward.k * d;
	normalize3d_with_default(&up, &b->up);
	out_state->forward = forward;
	out_state->up = up;
	out_state->field_of_view = a->field_of_view + (b->field_of_view - a->field_of_view) * t;
	return;
}

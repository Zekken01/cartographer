#include "stdafx.h"

#include "units/unit_control.h"
#include "units/units.h"
#include "../h1_sound.h"
#include "../h1_cache_file.h"
#include "../h1_hs.h"
#include "../h1_recordings.h"
#include "../h1_weapon_logic.h"

#include <unordered_map>
#include <unordered_set>

#include "h1_ai_internal.h"
#include "h1_ai_objects.h"

/*
* units.c over carto: the AI controls halo 2's units. unit_control sets the unit's halo 1 control (in its mirror) as halo 1's does;
* each AI tick, after the actors have run, unit_update's control of the units halo 1 controls (the ones the AI actively controls,
* runs blindly or holds a persistent control on) goes to halo 2's unit_control.
*/

namespace h1_ai
{

/* ---------- constants */

// halo 2's unit control flags (its player actions')
enum : uint64
{
	k_h2_unit_control_crouch = FLAG(0),
	k_h2_unit_control_jump = FLAG(1),
	k_h2_unit_control_primary_trigger_pressed = FLAG(8),
	k_h2_unit_control_grenade_pressed = FLAG(13),
	k_h2_unit_control_primary_trigger_held = FLAG(18),
	k_h2_unit_control_grenade_held = FLAG(23),
	k_h2_unit_control_reload = FLAG(30),
	// the player's melee button
	k_h2_unit_control_melee = FLAG(2),
};

/* ---------- halo 2's unit actions */

// unit_action_start (FUN_0056528d) and its enter seat request
struct s_h2_unit_action_enter_seat
{
	int32 type;
	datum vehicle_index;
	int16 seat_index;
	int8 pad[2];
};
enum
{
	k_h2_unit_action_enter_seat = 0x1C,
};
typedef bool(__cdecl* t_h2_unit_action_start)(datum unit_index, void* request);
typedef bool(__cdecl* t_h2_unit_can_enter_seat)(datum unit_index, datum vehicle_index, int16 seat_index);
typedef void(__cdecl* t_h2_unit_exit_vehicle)(datum unit_index);
typedef void(__cdecl* t_h2_unit_exit_seat_end)(datum unit_index, int32 ticks);

/* ---------- globals */

// units halo 1 started a melee on, for their next control
static std::unordered_set<long> g_melee_units;
// each unit's animation seat and weapon labels (units.c sets its animation seat and weapon class when they change)
struct s_unit_animation_labels
{
	const char* seat_label;
	const char* weapon_label;
};
static std::unordered_map<long, s_unit_animation_labels> g_unit_animation_labels;
// breakable_surfaces.c: every surface whole (breaking isn't halo 1's here)
static uint32 g_breakable_surface_flags[0x800];

/* ---------- private prototypes */

static void unit_animation_labels_update(long unit_index, unit_datum* unit);

static void unit_running_blind(long unit_index, real_vector3d* run_vector);

/* ---------- public code */

// units.c unit_control: the control the unit's next update uses
void unit_control(long unit_index, struct unit_control_data const* control_data)
{
	unit_datum* unit = (unit_datum*)unit_get(unit_index);
	unit->unit.throttle = control_data->throttle;
	unit->unit.primary_trigger = control_data->primary_trigger;
	unit->unit.aiming_speed = control_data->aiming_speed;
	if (control_data->weapon_index != NONE)
	{
		unit->unit.desired_weapon_index = control_data->weapon_index;
	}
	if (control_data->grenade_index != NONE)
	{
		unit->unit.desired_grenade_index = (char)control_data->grenade_index;
	}
	unit->unit.desired_zoom_level = (char)control_data->zoom_level;
	unit->unit.control_flags = control_data->control_flags;
	unit->unit.desired_looking_vector = control_data->looking_vector;
	unit->unit.desired_aiming_vector = control_data->aiming_vector;
	unit->unit.desired_facing_vector = control_data->facing_vector;
	unit->unit.animation.desired_state = control_data->animation_state;
	return;
}

void unit_persistent_control(long unit_index, long control_ticks, unsigned long persistent_control_flags)
{
	unit_datum* unit = (unit_datum*)unit_get(unit_index);
	unit->unit.persistent_control_flags = persistent_control_flags;
	unit->unit.persistent_control_timer = control_ticks;
	return;
}

void unit_set_actively_controlled(long unit_index, boolean actively_controlled)
{
	unit_datum* unit = (unit_datum*)unit_get(unit_index);
	if (unit->unit.actor_index != NONE || unit->unit.swarm_actor_index != NONE || unit->unit.player_index != NONE)
	{
		actively_controlled = TRUE;
	}
	actively_controlled = !TEST_FLAG(unit->object.damage_flags, _object_dead_bit) && actively_controlled;
	SET_FLAG(unit->unit.flags, _unit_actively_controlled_bit, actively_controlled);
	SET_FLAG(unit->unit.flags, _unit_controllable_bit, actively_controlled);
	return;
}

void unit_start_running_blindly(long unit_index)
{
	unit_datum* unit = (unit_datum*)unit_get(unit_index);
	if (!TEST_FLAG(unit->unit.flags, _unit_running_blindly_bit))
	{
		real angle_range;
		real_vector3d run_vector;
		SET_FLAG(unit->unit.flags, _unit_running_blindly_bit, TRUE);
		if (unit->unit.actor_index != NONE && actor_get_running_blind_vector(unit->unit.actor_index, &run_vector))
		{
			unit->unit.run_blindly_angle = 0.0f;
			angle_range = DEGREES_TO_RADIANS(25.0f);
		}
		else
		{
			real_euler_angles2d facing_angles;
			euler_angles2d_from_vector3d(&facing_angles, &unit->object.forward);
			if (facing_angles.yaw > _pi)
			{
				facing_angles.yaw -= 2.0f * _pi;
			}
			unit->unit.run_blindly_angle = facing_angles.yaw;
			angle_range = DEGREES_TO_RADIANS(100.0f);
		}
		unit->unit.run_blindly_angle += real_seed_random_range(get_global_random_seed_address(), -angle_range, angle_range);
	}
	return;
}

void unit_stop_running_blindly(long unit_index)
{
	unit_datum* unit = (unit_datum*)unit_get(unit_index);
	SET_FLAG(unit->unit.flags, _unit_running_blindly_bit, FALSE);
	return;
}

// unit_update's control: halo 1's control of a unit it controls as halo 2's (whether halo 1 controlled it the tick before in
// controlled_last, the tick it stops it gets halo 1's uncontrolled control once)
void h1_ai_unit_control_update(long unit_index, unit_datum* unit, bool* controlled_last)
{
	// unit_update's speech
	unit_dialogue_update(unit_index);
	unit_animation_labels_update(unit_index, unit);

	// an actor's team (game teams are numbered alike)
	::unit_datum* h2_unit = (::unit_datum*)::object_try_and_get_and_verify_type(unit_index, ::_object_mask_unit);
	if (h2_unit && unit->unit.actor_index != NONE && unit->object.owner_team_index != NONE && (short)h2_unit->unit.unit_team != unit->object.owner_team_index)
	{
		h2_unit->unit.unit_team = (::e_game_team)unit->object.owner_team_index;
	}

	const bool running_blindly = TEST_FLAG(unit->unit.flags, _unit_running_blindly_bit);
	const bool actively_controlled = TEST_FLAG(unit->unit.flags, _unit_actively_controlled_bit);
	const bool controlled = unit->unit.player_index == NONE && !TEST_FLAG(unit->object.damage_flags, _object_dead_bit) &&
		(running_blindly || actively_controlled || unit->unit.persistent_control_timer > 0);
	if (!controlled && !*controlled_last)
	{
		return;
	}
	*controlled_last = controlled;
	// halo 2's unit update only takes the control of a unit it's told is actively controlled
	if (h2_unit)
	{
		SET_FLAG(h2_unit->unit.unit_flags, 1, controlled);
	}

	if (running_blindly)
	{
		unit_running_blind(unit_index, &unit->unit.desired_facing_vector);
		unit->unit.desired_aiming_vector = unit->unit.desired_facing_vector;
		unit->unit.desired_looking_vector = unit->unit.desired_facing_vector;
		unit->unit.throttle = *global_forward3d;
		unit->unit.control_flags = 0;
	}
	else if (!actively_controlled)
	{
		unit->unit.desired_looking_vector = unit->object.forward;
		unit->unit.desired_aiming_vector = unit->object.forward;
		unit->unit.desired_facing_vector = unit->object.forward;
		unit->unit.throttle = *global_zero_vector3d;
		unit->unit.control_flags = 0;
	}
	if (unit->unit.persistent_control_timer > 0)
	{
		unit->unit.control_flags = unit->unit.persistent_control_flags | unit->unit.control_flags;
		if (TEST_FLAG(unit->unit.persistent_control_flags, _unit_control_weapon_primary_trigger_bit))
		{
			SET_FLAG(unit->unit.control_flags, _unit_control_weapon_primary_trigger_bit, (unit->unit.persistent_control_timer % 7) == 0);
			unit->unit.primary_trigger = 1.f;
		}
		else
		{
			unit->unit.primary_trigger = 0.f;
		}
		if (--unit->unit.persistent_control_timer == 0)
		{
			unit->unit.persistent_control_flags = 0;
		}
	}

	::unit_control_data data;
	Memory::GetAddress<void(__cdecl*)(::unit_control_data*)>(0x138C92)(&data);
	const unsigned long flags = unit->unit.control_flags;
	uint64 h2_flags = 0;
	if (TEST_FLAG(flags, _unit_control_crouch_modifier_bit)) h2_flags |= k_h2_unit_control_crouch;
	if (TEST_FLAG(flags, _unit_control_jump_bit)) h2_flags |= k_h2_unit_control_jump;
	if (TEST_FLAG(flags, _unit_control_weapon_primary_trigger_bit)) h2_flags |= k_h2_unit_control_primary_trigger_pressed | k_h2_unit_control_primary_trigger_held;
	if (TEST_FLAG(flags, _unit_control_throw_grenade_bit)) h2_flags |= k_h2_unit_control_grenade_pressed | k_h2_unit_control_grenade_held;
	if (TEST_FLAG(flags, _unit_control_weapon_reload_bit)) h2_flags |= k_h2_unit_control_reload;
	if (g_melee_units.erase(unit_index)) h2_flags |= k_h2_unit_control_melee;
	data.control_flags = (int64)h2_flags;
	data.aiming_speed = (uint16)unit->unit.aiming_speed;
	data.throttle = *(const ::real_vector3d*)&unit->unit.throttle;
	data.primary_trigger = unit->unit.primary_trigger;
	if (unit->unit.desired_grenade_index != NONE)
	{
		data.grenade_index = (uint16)unit->unit.desired_grenade_index;
	}
	data.zoom_level = (uint16)unit->unit.desired_zoom_level;
	auto normalized = [](const real_vector3d* vector, ::real_vector3d* out)
	{
		*out = *(const ::real_vector3d*)vector;
		if (::normalize3d(out) == 0.f)
		{
			*out = *::global_forward3d;
		}
	};
	normalized(&unit->unit.desired_facing_vector, &data.facing_vector);
	normalized(&unit->unit.desired_aiming_vector, &data.aiming_vector);
	normalized(&unit->unit.desired_looking_vector, &data.looking_vector);
	::unit_control(unit_index, &data);
	return;
}

// units.c unit_melee_attack_begin: halo 2's melee, from the unit's next control
boolean unit_melee_attack_begin(long unit_index, boolean continuous, real_vector2d const* alignment_vector)
{
	const unit_datum* unit = (const unit_datum*)unit_get(unit_index);
	if (TEST_FLAG(unit->object.damage_flags, _object_dead_bit) || unit->object.parent_object_index != NONE)
	{
		return FALSE;
	}
	g_melee_units.insert(unit_index);
	return TRUE;
}

// units.c unit_enter_seat, unit_try_and_exit_seat, unit_detach_from_parent: halo 2's seat actions
boolean unit_enter_seat(long unit_index, long target_unit_index, short seat_index)
{
	if (!Memory::GetAddress<t_h2_unit_can_enter_seat>(0x139A7A)(unit_index, target_unit_index, seat_index))
	{
		return FALSE;
	}
	s_h2_unit_action_enter_seat action = { k_h2_unit_action_enter_seat, target_unit_index, seat_index, { 0, 0 } };
	Memory::GetAddress<t_h2_unit_action_start>(0x16528D)(unit_index, &action);
	return TRUE;
}

boolean unit_try_and_exit_seat(long unit_index)
{
	const ::object_datum* object = (const ::object_datum*)::object_try_and_get_and_verify_type(unit_index, ::_object_mask_unit);
	if (!object || object->object.parent_object_index == NONE)
	{
		return FALSE;
	}
	Memory::GetAddress<t_h2_unit_exit_vehicle>(0x18525F)(unit_index);
	return TRUE;
}

void unit_detach_from_parent(long unit_index)
{
	const ::object_datum* object = (const ::object_datum*)::object_try_and_get_and_verify_type(unit_index, ::_object_mask_unit);
	if (object && object->object.parent_object_index != NONE)
	{
		Memory::GetAddress<t_h2_unit_exit_seat_end>(0x165E7D)(unit_index, 0);
	}
	return;
}

// weapons.c weapon_set_current_amount, weapon_set_total_rounds: halo 1's weapon logic (h1_weapon_logic)
void weapon_set_current_amount(long weapon_index, real current_amount)
{
	h1_weapon_logic_set_current_amount(weapon_index, current_amount);
	return;
}

void weapon_set_total_rounds(long weapon_index, short* rounds_array)
{
	h1_weapon_logic_set_total_rounds(weapon_index, rounds_array);
	return;
}

// units.c unit_start_user_animation: the scripts' custom animation (halo 2 plays it, its time is the unit's animation's)
boolean unit_start_user_animation(long unit_index, long animation_graph_index, char const* animation_name, boolean interpolate)
{
	unit_datum* unit = (unit_datum*)unit_get(unit_index);
	if (!h1_hs_custom_animation(unit_index, animation_graph_index, animation_name, interpolate != FALSE))
	{
		return FALSE;
	}
	unit->unit.animation.state = _unit_state_user_animation;
	return TRUE;
}

// units.c unit_start_animation_impulse: the impulse's animation of the unit's seat and weapon class, played by halo 2 (by name,
// from the unit's own graph)
boolean unit_start_animation_impulse(long unit_index, short animation_impulse, real_vector2d* alignment_vector)
{
	unit_datum* unit = (unit_datum*)unit_get(unit_index);
	if (!unit_test_animation_impulse(unit_index, animation_impulse))
	{
		return FALSE;
	}
	struct unit_definition* unit_definition = unit_definition_get(unit->definition_index);
	struct animation_graph* animation_graph = animation_graph_definition_get(unit_definition->object.animation_graph.index);
	struct animation_graph_unit_seat* unit_seat = TAG_BLOCK_GET_ELEMENT(&animation_graph->unit_seats, unit->unit.animation.seat_index, struct animation_graph_unit_seat);
	struct animation_graph_weapon_class* weapon_class = TAG_BLOCK_GET_ELEMENT(&unit_seat->weapon_classes, unit->unit.animation.weapon_index, struct animation_graph_weapon_class);
	short interpolation_frame_count;
	const short animation_type = h1_ai_unit_animation_impulse_index(animation_impulse, &interpolation_frame_count);
	short animation_index = animation_graph_animation_index_get(&weapon_class->animations)[animation_type].animation_index;
	animation_index = animation_choose_random_permutation_internal(TRUE, unit_definition->object.animation_graph.index, animation_index);
	struct animation* animation = TAG_BLOCK_GET_ELEMENT(&animation_graph->animations, animation_index, struct animation);
	if (!animation || !h1_hs_unit_animation_play(unit_index, animation->name, interpolation_frame_count > 0))
	{
		return FALSE;
	}
	unit->unit.animation.state = _unit_state_ai_impulse;
	return TRUE;
}

// units.c unit_get_animation_frames_remaining: a user animation's ticks left
short unit_get_animation_frames_remaining(long unit_index, short* animation_state)
{
	unit_datum* unit = (unit_datum*)unit_get(unit_index);
	const short frames_remaining = h1_hs_animation_time(unit_index);
	if ((unit->unit.animation.state == _unit_state_user_animation || unit->unit.animation.state == _unit_state_ai_impulse) && frames_remaining <= 0)
	{
		unit->unit.animation.state = _unit_state_idle;
	}
	*animation_state = unit->unit.animation.state;
	return MAX(frames_remaining, 0);
}

// recorded_animations.c: the scripts' recordings (h1_recordings)
boolean recorded_animation_play(long unit_index, short animation_index)
{
	return h1_recording_play(unit_index, animation_index, false);
}

boolean recorded_animation_controlling_unit(long unit_index)
{
	return h1_recording_controlling_unit(unit_index);
}

// recorded_animation_definitions.c scenario_get_animation_by_name
short scenario_get_animation_by_name(struct scenario const* scenario, char const* name)
{
	const h1_scnr* h1_scenario = g_h1_cache_file ? g_h1_cache_file->scenario_get() : NULL;
	for (short animation_index = 0; h1_scenario && animation_index < h1_scenario->recorded_animations.count; animation_index++)
	{
		if (!_stricmp(g_h1_cache_file->block_get(h1_scenario->recorded_animations, animation_index)->name, name))
		{
			return animation_index;
		}
	}
	return NONE;
}

boolean hs_wake_by_name(char const* name)
{
	return h1_hs_wake_by_name(name);
}

byte* breakable_surface_flags_get(void)
{
	static bool s_initialized = false;
	if (!s_initialized)
	{
		csmemset(g_breakable_surface_flags, 0xFF, sizeof(g_breakable_surface_flags));
		s_initialized = true;
	}
	return (byte*)g_breakable_surface_flags;
}

// game_sound.c object_impulse_sound_new: the unit's speech, played at its head
long object_impulse_sound_new(long object_index, long sound_definition_index, short node_index, real_point3d const* position,
	real_vector3d const* forward, real scale)
{
	real_point3d world_position;
	object_get_origin(object_index, &world_position);
	const real_matrix4x3* node_matrix = object_get_node_matrix(object_index, node_index);
	if (node_matrix)
	{
		world_position = node_matrix->position;
	}
	h1_sound_impulse(sound_definition_index, (const ::real_point3d*)&world_position, scale);
	h1_hs_sound_dialog_note(sound_definition_index);
	return sound_definition_index;
}

// game_sound.c scripted_sound_new, scripted_sound_time and sound_manager.c's scripted dialog: the scripts' (h1_hs)
void scripted_sound_new(long definition_index, long source_object_index, real scale)
{
	h1_hs_sound_impulse_start(definition_index, source_object_index, scale);
	return;
}

long scripted_sound_time(long sound_index)
{
	return h1_hs_sound_impulse_time(sound_index);
}

boolean sound_scripted_dialog_is_playing(void)
{
	return h1_hs_scripted_dialog_is_playing();
}

/* ---------- private code */

// the unit's seat (its parent's seat's label, standing without one) and weapon (its current one's label, unarmed without one)
static void unit_animation_labels_update(long unit_index, unit_datum* unit)
{
	const char* seat_label = "stand";
	if (unit->object.parent_object_index != NONE && unit->unit.parent_seat_index != NONE)
	{
		const object_datum* parent = (const object_datum*)object_get(unit->object.parent_object_index);
		struct unit_definition* parent_definition = TEST_FLAG(_object_mask_unit, parent->object.type) ? unit_definition_get(parent->definition_index) : NULL;
		struct unit_seat* seat = parent_definition ? TAG_BLOCK_GET_ELEMENT(&parent_definition->unit.seats, unit->unit.parent_seat_index, struct unit_seat) : NULL;
		if (seat)
		{
			seat_label = seat->label;
		}
	}
	const long weapon_index = unit->unit.current_weapon_index != NONE ? unit->unit.weapon_object_indices[unit->unit.current_weapon_index] : NONE;
	const char* weapon_label = weapon_index != NONE ? weapon_get_label(weapon_index) : "unarmed";
	s_unit_animation_labels& labels = g_unit_animation_labels[unit_index];
	if (labels.seat_label != seat_label || labels.weapon_label != weapon_label)
	{
		labels = { seat_label, weapon_label };
		h1_ai_unit_animation_labels_set(unit_index, seat_label, weapon_label);
	}
	return;
}

// units.c unit_running_blind: the direction a unit running blindly runs in, wandering
static void unit_running_blind(long unit_index, real_vector3d* run_vector)
{
	unit_datum* unit = (unit_datum*)unit_get(unit_index);
	boolean actor_controlled = FALSE;
	if (unit->unit.actor_index == NONE || !actor_get_running_blind_vector(unit->unit.actor_index, run_vector))
	{
		*run_vector = *global_forward3d;
	}
	else
	{
		actor_controlled = TRUE;
	}

	real angular_acceleration_this_tick;
	real positive_angle_allowed = 1.f;
	real negative_angle_allowed = 1.f;
	if (actor_controlled)
	{
		const real negative_angle_bounds_dist = DEGREES_TO_RADIANS(45) - unit->unit.run_blindly_angle;
		const real positive_angle_bounds_dist = DEGREES_TO_RADIANS(45) + unit->unit.run_blindly_angle;
		negative_angle_allowed = MIN(negative_angle_allowed, negative_angle_bounds_dist / DEGREES_TO_RADIANS(13.5f));
		positive_angle_allowed = MIN(positive_angle_allowed, positive_angle_bounds_dist / DEGREES_TO_RADIANS(13.5f));
	}
	const real negative_velocity_bounds_dist = DEGREES_TO_RADIANS(12.f) - unit->unit.run_blindly_angle_delta;
	const real positive_velocity_bounds_dist = DEGREES_TO_RADIANS(12.f) + unit->unit.run_blindly_angle_delta;
	negative_angle_allowed = MIN(negative_angle_allowed, negative_velocity_bounds_dist * 15.915494f);
	positive_angle_allowed = MIN(positive_angle_allowed, positive_velocity_bounds_dist * 15.915494f);
	if (negative_angle_allowed < positive_angle_allowed)
	{
		if (negative_angle_allowed < -1.f)
		{
			angular_acceleration_this_tick = -0.020943951f;
		}
		else
		{
			angular_acceleration_this_tick = real_random_range(-0.020943951f, 0.020943951f * MIN(1.f, negative_angle_allowed));
		}
	}
	else
	{
		if (positive_angle_allowed < -1.f)
		{
			angular_acceleration_this_tick = 0.020943951f;
		}
		else
		{
			angular_acceleration_this_tick = real_random_range(-0.020943951f * MIN(1.f, positive_angle_allowed), 0.020943951f);
		}
	}
	unit->unit.run_blindly_angle_delta += angular_acceleration_this_tick;
	unit->unit.run_blindly_angle += unit->unit.run_blindly_angle_delta;
	if (unit->unit.run_blindly_angle < -_pi)
	{
		unit->unit.run_blindly_angle += 2.f * _pi;
	}
	else if (unit->unit.run_blindly_angle > _pi)
	{
		unit->unit.run_blindly_angle -= 2.f * _pi;
	}
	rotate_vector_about_axis(run_vector, global_up3d, sine(unit->unit.run_blindly_angle), cosine(unit->unit.run_blindly_angle));
	return;
}

} // namespace h1_ai

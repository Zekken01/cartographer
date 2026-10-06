#include "stdafx.h"
#include "h1_ai_internal.h"
#include "h1_ai.h"

/*
* hs.c's ai script functions as h1_hs procedures: tools/gen_ai_hs.py from halo-ce-universal (do not edit).
*/

namespace h1_ai
{

/* ---------- hs.c's argument and result layouts */

union hs_real_value
{
	real real_value;
	long long_value;
};

union hs_short_result
{
	short short_value;
	long value;
};

union hs_evaluation_argument
{
	long long_value;
	real real_value;
	short short_value;
	unsigned short unsigned_short_value;
	boolean boolean_value;
	char const *string_value;
};

struct hs_arguments_boolean
{
	boolean value;
};

struct hs_arguments_long
{
	long value;
};

struct hs_arguments_word
{
	word value;
};

struct hs_arguments_short_long
{
	short value0;
	word pad0;
	long value1;
};

struct hs_arguments_long_word
{
	long value0;
	word value1;
};

struct hs_arguments_string
{
	char const *value;
};

struct hs_arguments_long_string
{
	long value0;
	char const *value1;
};

struct hs_arguments_long_long
{
	long value0;
	long value1;
};

struct hs_arguments_long_long_string
{
	long value0;
	long value1;
	char const *value2;
};

struct hs_arguments_short_word
{
	short value0;
	word pad0;
	word value1;
};

struct hs_arguments_long_long_long
{
	long value0;
	char const *value1;
	long value2;
};

union hs_boolean_result
{
	boolean boolean;
	long value;
};

/* ---------- hs.c's evaluator macros, returning the result */

#define HS_EVALUATE_VOID_BOOLEAN(evaluator, function) \
static int32 evaluator(const int32* hs_arguments) \
{ \
	union hs_evaluation_argument *arguments = (union hs_evaluation_argument *)((long*)hs_arguments); \
	if (arguments) \
	{ \
		function(arguments[0].boolean_value); \
		return (int32)(0); \
	} \
	return 0; \
}

#define HS_EVALUATE_VOID_LONG(evaluator, function) \
static int32 evaluator(const int32* hs_arguments) \
{ \
	long *arguments = ((long*)hs_arguments); \
	if (arguments) \
	{ \
		function(arguments[0]); \
		return (int32)(0); \
	} \
	return 0; \
}

#define HS_EVALUATE_VOID_LONG_LONG(evaluator, function) \
static int32 evaluator(const int32* hs_arguments) \
{ \
	long *arguments = ((long*)hs_arguments); \
	if (arguments) \
	{ \
		function(arguments[0], arguments[1]); \
		return (int32)(0); \
	} \
	return 0; \
}

#define HS_EVALUATE_NO_ARGUMENTS(evaluator, function) \
static int32 evaluator(const int32* hs_arguments) \
{ \
	function(); \
	return (int32)(0); \
	return 0; \
}

#define HS_EVALUATE_VOID_LONG_BOOLEAN(evaluator, function) \
static int32 evaluator(const int32* hs_arguments) \
{ \
	union hs_evaluation_argument *arguments = (union hs_evaluation_argument *)((long*)hs_arguments); \
	if (arguments) \
	{ \
		function(arguments[0].long_value, arguments[1].boolean_value); \
		return (int32)(0); \
	} \
	return 0; \
}

#define HS_EVALUATE_VOID_LONG_LONG_STRING(evaluator, function) \
static int32 evaluator(const int32* hs_arguments) \
{ \
	struct hs_arguments_long_long_string *arguments = (struct hs_arguments_long_long_string *)((long*)hs_arguments); \
	if (arguments) \
	{ \
		function(arguments->value0, arguments->value1, arguments->value2); \
		return (int32)(0); \
	} \
	return 0; \
}

#define HS_EVALUATE_VOID_SHORT_SHORT(evaluator, function) \
static int32 evaluator(const int32* hs_arguments) \
{ \
	union hs_evaluation_argument *arguments = (union hs_evaluation_argument *)((long*)hs_arguments); \
	if (arguments) \
	{ \
		function(arguments[0].short_value, arguments[1].unsigned_short_value); \
		return (int32)(0); \
	} \
	return 0; \
}

#define HS_EVALUATE_VOID_LONG_UNSIGNED_SHORT(evaluator, function) \
static int32 evaluator(const int32* hs_arguments) \
{ \
	union hs_evaluation_argument *arguments = (union hs_evaluation_argument *)((long*)hs_arguments); \
	if (arguments) \
	{ \
		function(arguments[0].long_value, arguments[1].unsigned_short_value); \
		return (int32)(0); \
	} \
	return 0; \
}

#define HS_EVALUATE_VOID_FROM_ARGUMENTS_WITH_REAL(evaluator, arguments_type, real_index, expression) \
static int32 evaluator(const int32* hs_arguments) \
{ \
	arguments_type const *arguments; \
	arguments = (arguments_type const *)((long*)hs_arguments); \
	if (arguments) \
	{ \
		real real_argument = arguments[real_index].real_value; \
		expression; \
		return (int32)(0); \
	} \
	return 0; \
}

#define HS_EVALUATE_VOID_UNSIGNED_SHORT(evaluator, function) \
static int32 evaluator(const int32* hs_arguments) \
{ \
	union hs_evaluation_argument *arguments = (union hs_evaluation_argument *)((long*)hs_arguments); \
	if (arguments) \
	{ \
		function(arguments[0].unsigned_short_value); \
		return (int32)(0); \
	} \
	return 0; \
}

#define HS_EVALUATE_RETURN_BOOLEAN(evaluator, arguments_type, expression) \
static int32 evaluator(const int32* hs_arguments) \
{ \
	arguments_type const *arguments; \
	union hs_boolean_result result; \
	result.value = 0; \
	arguments = (arguments_type const *)((long*)hs_arguments); \
	if (arguments) \
	{ \
		result.boolean = expression; \
		return (int32)(result.value); \
	} \
	return 0; \
}

#define HS_EVALUATE_SHORT_FROM_LONG(evaluator, function) \
static int32 evaluator(const int32* hs_arguments) \
{ \
	long *arguments; \
	union hs_short_result result; \
	result.value = 0; \
	arguments = ((long*)hs_arguments); \
	if (arguments) \
	{ \
		result.short_value = function(arguments[0]); \
		return (int32)(result.value); \
	} \
	return 0; \
}

#define HS_EVALUATE_REAL_FROM_LONG(evaluator, function) \
static int32 evaluator(const int32* hs_arguments) \
{ \
	long *arguments = ((long*)hs_arguments); \
	if (arguments) \
	{ \
		union hs_real_value result; \
		result.real_value = function(arguments[0]); \
		return (int32)(result.long_value); \
	} \
	return 0; \
}

#define HS_EVALUATE_LONG_FROM_LONG(evaluator, function) \
static int32 evaluator(const int32* hs_arguments) \
{ \
	long *arguments = ((long*)hs_arguments); \
	if (arguments) \
		return (int32)(function(arguments[0])); \
	return 0; \
}

#define HS_EVALUATE_SHORT_FROM_UNSIGNED_SHORT(evaluator, function) \
static int32 evaluator(const int32* hs_arguments) \
{ \
	union hs_evaluation_argument *arguments; \
	union hs_short_result result; \
	result.value = 0; \
	arguments = (union hs_evaluation_argument *)((long*)hs_arguments); \
	if (arguments) \
	{ \
		result.short_value = function(arguments[0].unsigned_short_value); \
		return (int32)(result.value); \
	} \
	return 0; \
}

/* ---------- the evaluators */

HS_EVALUATE_VOID_BOOLEAN(ai_globals_ai_active_evaluate, ai_globals_ai_active)
HS_EVALUATE_VOID_BOOLEAN(ai_globals_dialogue_triggers_enabled_evaluate, ai_globals_dialogue_triggers_enabled)
HS_EVALUATE_VOID_BOOLEAN(ai_globals_grenades_enabled_evaluate, ai_globals_grenades_enabled)
HS_EVALUATE_VOID_LONG(ai_scripting_free_evaluate, ai_scripting_free)
HS_EVALUATE_VOID_LONG(ai_scripting_free_units_evaluate, ai_scripting_free_units)
HS_EVALUATE_VOID_LONG_LONG(ai_scripting_attach_unit_evaluate, ai_scripting_attach_unit)
HS_EVALUATE_VOID_LONG_LONG(ai_scripting_attach_units_evaluate, ai_scripting_attach_units)
HS_EVALUATE_VOID_LONG_LONG(ai_scripting_attach_free_evaluate, ai_scripting_attach_free)
HS_EVALUATE_VOID_LONG(ai_scripting_detach_unit_evaluate, ai_scripting_detach_unit)
HS_EVALUATE_VOID_LONG(ai_scripting_detach_units_evaluate, ai_scripting_detach_units)
HS_EVALUATE_VOID_LONG(ai_scripting_place_evaluate, ai_scripting_place)
HS_EVALUATE_VOID_LONG(ai_scripting_kill_evaluate, ai_scripting_kill)
HS_EVALUATE_VOID_LONG(ai_scripting_kill_silent_evaluate, ai_scripting_kill_silent)
HS_EVALUATE_VOID_LONG(ai_scripting_erase_evaluate, ai_scripting_erase)
HS_EVALUATE_NO_ARGUMENTS(ai_scripting_erase_all_evaluate, ai_scripting_erase_all)
HS_EVALUATE_VOID_LONG(ai_scripting_select_evaluate, ai_scripting_select)
HS_EVALUATE_NO_ARGUMENTS(ai_scripting_deselect_evaluate, ai_scripting_deselect)
HS_EVALUATE_VOID_LONG(ai_scripting_spawn_actor_evaluate, ai_scripting_spawn_actor)
HS_EVALUATE_VOID_LONG_BOOLEAN(ai_scripting_set_respawn_evaluate, ai_scripting_set_respawn)
HS_EVALUATE_VOID_LONG_BOOLEAN(ai_scripting_set_deaf_evaluate, ai_scripting_set_deaf)
HS_EVALUATE_VOID_LONG_BOOLEAN(ai_scripting_set_blind_evaluate, ai_scripting_set_blind)
HS_EVALUATE_VOID_LONG_LONG(ai_scripting_magically_see_encounter_evaluate, ai_scripting_magically_see_encounter)
HS_EVALUATE_VOID_LONG(ai_scripting_magically_see_players_evaluate, ai_scripting_magically_see_players)
HS_EVALUATE_VOID_LONG_LONG(ai_scripting_magically_see_unit_evaluate, ai_scripting_magically_see_unit)
HS_EVALUATE_VOID_LONG_LONG(ai_scripting_magically_see_units_evaluate, ai_scripting_magically_see_units)
HS_EVALUATE_VOID_LONG(ai_scripting_timer_start_evaluate, ai_scripting_timer_start)
HS_EVALUATE_VOID_LONG(ai_scripting_timer_expire_evaluate, ai_scripting_timer_expire)
HS_EVALUATE_VOID_LONG(ai_scripting_attack_evaluate, ai_scripting_attack)
HS_EVALUATE_VOID_LONG(ai_scripting_defend_evaluate, ai_scripting_defend)
HS_EVALUATE_VOID_LONG(ai_scripting_retreat_evaluate, ai_scripting_retreat)
HS_EVALUATE_VOID_LONG(ai_scripting_maneuver_evaluate, ai_scripting_maneuver)
HS_EVALUATE_VOID_LONG_BOOLEAN(ai_scripting_maneuver_enable_evaluate, ai_scripting_maneuver_enable)
HS_EVALUATE_VOID_LONG_LONG(ai_scripting_migrate_evaluate, ai_scripting_migrate)
HS_EVALUATE_VOID_LONG_LONG_STRING(ai_scripting_migrate_and_speak_evaluate, ai_scripting_migrate_and_speak)
HS_EVALUATE_VOID_LONG_LONG(ai_scripting_migrate_by_unit_evaluate, ai_scripting_migrate_by_unit)
HS_EVALUATE_VOID_SHORT_SHORT(ai_scripting_allegiance_evaluate, ai_scripting_allegiance)
HS_EVALUATE_VOID_SHORT_SHORT(ai_scripting_allegiance_remove_evaluate, ai_scripting_allegiance_remove)
HS_EVALUATE_VOID_LONG_LONG_STRING(ai_scripting_go_to_vehicle_evaluate, ai_scripting_go_to_vehicle)
HS_EVALUATE_VOID_LONG_LONG_STRING(ai_scripting_go_to_vehicle_override_evaluate, ai_scripting_go_to_vehicle_override)
HS_EVALUATE_VOID_LONG(ai_scripting_exit_vehicle_evaluate, ai_scripting_exit_vehicle)
HS_EVALUATE_VOID_LONG_BOOLEAN(ai_scripting_braindead_evaluate, ai_scripting_braindead)
HS_EVALUATE_VOID_LONG_BOOLEAN(ai_scripting_braindead_by_unit_evaluate, ai_scripting_braindead_by_unit)
HS_EVALUATE_VOID_LONG_BOOLEAN(ai_scripting_ignore_evaluate, ai_scripting_ignore)
HS_EVALUATE_VOID_LONG_BOOLEAN(ai_scripting_prefer_target_evaluate, ai_scripting_prefer_target)
HS_EVALUATE_VOID_LONG(ai_scripting_teleport_starting_location_evaluate, ai_scripting_teleport_starting_location)
HS_EVALUATE_VOID_LONG(ai_scripting_teleport_starting_location_if_unsupported_evaluate, ai_scripting_teleport_starting_location_if_unsupported)
HS_EVALUATE_VOID_LONG(ai_scripting_renew_evaluate, ai_scripting_renew)
HS_EVALUATE_VOID_LONG(ai_scripting_try_to_fight_nothing_evaluate, ai_scripting_try_to_fight_nothing)
HS_EVALUATE_VOID_LONG_LONG(ai_scripting_try_to_fight_evaluate, ai_scripting_try_to_fight)
HS_EVALUATE_VOID_LONG(ai_scripting_try_to_fight_player_evaluate, ai_scripting_try_to_fight_player)
HS_EVALUATE_VOID_LONG_UNSIGNED_SHORT(ai_scripting_command_list_evaluate, ai_scripting_command_list)
HS_EVALUATE_VOID_LONG_UNSIGNED_SHORT(ai_scripting_command_list_by_unit_evaluate, ai_scripting_command_list_by_unit)
HS_EVALUATE_VOID_LONG(ai_scripting_command_list_advance_evaluate, ai_scripting_command_list_advance)
HS_EVALUATE_VOID_LONG(ai_scripting_command_list_advance_by_unit_evaluate, ai_scripting_command_list_advance_by_unit)
HS_EVALUATE_VOID_LONG_BOOLEAN(ai_scripting_force_active_evaluate, ai_scripting_force_active)
HS_EVALUATE_VOID_LONG_BOOLEAN(ai_scripting_force_active_by_unit_evaluate, ai_scripting_force_active_by_unit)
HS_EVALUATE_VOID_LONG_UNSIGNED_SHORT(ai_scripting_set_return_state_evaluate, ai_scripting_set_return_state)
HS_EVALUATE_VOID_LONG_UNSIGNED_SHORT(ai_scripting_set_current_state_evaluate, ai_scripting_set_current_state)
HS_EVALUATE_VOID_LONG_BOOLEAN(ai_scripting_playfight_evaluate, ai_scripting_playfight)
HS_EVALUATE_NO_ARGUMENTS(ai_scripting_reconnect_evaluate, ai_scripting_reconnect)
HS_EVALUATE_VOID_LONG_LONG(ai_scripting_vehicle_encounter_evaluate, ai_scripting_vehicle_encounter)
HS_EVALUATE_VOID_FROM_ARGUMENTS_WITH_REAL(
	ai_scripting_vehicle_enterable_distance_evaluate,
	union hs_evaluation_argument,
	1,
	ai_scripting_vehicle_enterable_distance(arguments[0].long_value, real_argument))
HS_EVALUATE_VOID_LONG_UNSIGNED_SHORT(ai_scripting_vehicle_enterable_team_evaluate, ai_scripting_vehicle_enterable_team)
HS_EVALUATE_VOID_LONG_UNSIGNED_SHORT(ai_scripting_vehicle_enterable_actor_type_evaluate, ai_scripting_vehicle_enterable_actor_type)
HS_EVALUATE_VOID_LONG_LONG(ai_scripting_vehicle_enterable_actors_evaluate, ai_scripting_vehicle_enterable_actors)
HS_EVALUATE_VOID_LONG(ai_scripting_vehicle_enterable_disable_evaluate, ai_scripting_vehicle_enterable_disable)
HS_EVALUATE_VOID_LONG_LONG(ai_scripting_look_at_object_evaluate, ai_scripting_look_at_object)
HS_EVALUATE_VOID_LONG(ai_scripting_stop_looking_evaluate, ai_scripting_stop_looking)
HS_EVALUATE_VOID_LONG_BOOLEAN(ai_scripting_automatic_migration_target_evaluate, ai_scripting_automatic_migration_target)
HS_EVALUATE_VOID_LONG(ai_scripting_follow_target_disable_evaluate, ai_scripting_follow_target_disable)
HS_EVALUATE_VOID_LONG(ai_scripting_follow_target_players_evaluate, ai_scripting_follow_target_players)
HS_EVALUATE_VOID_LONG_LONG(ai_scripting_follow_target_unit_evaluate, ai_scripting_follow_target_unit)
HS_EVALUATE_VOID_LONG_LONG(ai_scripting_follow_target_ai_evaluate, ai_scripting_follow_target_ai)
HS_EVALUATE_VOID_FROM_ARGUMENTS_WITH_REAL(
	ai_scripting_follow_distance_evaluate,
	union hs_evaluation_argument,
	1,
	ai_scripting_follow_distance(arguments[0].long_value, real_argument))
HS_EVALUATE_VOID_UNSIGNED_SHORT(ai_scripting_conversation_stop_evaluate, ai_scripting_conversation_stop)
HS_EVALUATE_VOID_UNSIGNED_SHORT(ai_scripting_conversation_advance_evaluate, ai_scripting_conversation_advance)
HS_EVALUATE_VOID_LONG_LONG(ai_scripting_link_activation_evaluate, ai_scripting_link_activation)
HS_EVALUATE_VOID_LONG_BOOLEAN(ai_scripting_berserk_evaluate, ai_scripting_berserk)
HS_EVALUATE_VOID_LONG_UNSIGNED_SHORT(ai_scripting_set_team_evaluate, ai_scripting_set_team)
HS_EVALUATE_VOID_LONG_BOOLEAN(ai_scripting_allow_charge_evaluate, ai_scripting_allow_charge)
HS_EVALUATE_VOID_LONG_BOOLEAN(ai_scripting_allow_dormant_evaluate, ai_scripting_allow_dormant)
HS_EVALUATE_RETURN_BOOLEAN(ai_scripting_is_attacking_evaluate, struct hs_arguments_long, (ai_scripting_is_attacking(arguments->value)))
HS_EVALUATE_SHORT_FROM_LONG(ai_scripting_command_list_status_evaluate, ai_scripting_command_list_status)
HS_EVALUATE_SHORT_FROM_LONG(ai_scripting_going_to_vehicle_evaluate, ai_scripting_going_to_vehicle)
HS_EVALUATE_SHORT_FROM_LONG(ai_scripting_living_count_evaluate, ai_scripting_living_count)
HS_EVALUATE_REAL_FROM_LONG(ai_scripting_living_fraction_evaluate, ai_scripting_living_fraction)
HS_EVALUATE_REAL_FROM_LONG(ai_scripting_strength_evaluate, ai_scripting_strength)
HS_EVALUATE_SHORT_FROM_LONG(ai_scripting_swarm_count_evaluate, ai_scripting_swarm_count)
HS_EVALUATE_SHORT_FROM_LONG(ai_scripting_nonswarm_count_evaluate, ai_scripting_nonswarm_count)
HS_EVALUATE_LONG_FROM_LONG(object_list_from_ai_reference_evaluate, object_list_from_ai_reference)
HS_EVALUATE_SHORT_FROM_LONG(ai_scripting_status_evaluate, ai_scripting_status)
HS_EVALUATE_RETURN_BOOLEAN(ai_scripting_conversation_evaluate, struct hs_arguments_word, (ai_scripting_conversation(arguments->value)))
HS_EVALUATE_SHORT_FROM_UNSIGNED_SHORT(ai_scripting_conversation_line_evaluate, ai_scripting_conversation_line)
HS_EVALUATE_SHORT_FROM_UNSIGNED_SHORT(ai_scripting_conversation_status_evaluate, ai_scripting_conversation_status)
HS_EVALUATE_RETURN_BOOLEAN(ai_scripting_allegiance_broken_evaluate, struct hs_arguments_short_word, (ai_scripting_allegiance_broken(arguments->value0, arguments->value1)))

} // namespace h1_ai

const s_h1_ai_hs_function g_h1_ai_hs_functions[] =
{
	{ "ai", h1_ai::ai_globals_ai_active_evaluate },
	{ "ai_dialogue_triggers", h1_ai::ai_globals_dialogue_triggers_enabled_evaluate },
	{ "ai_grenades", h1_ai::ai_globals_grenades_enabled_evaluate },
	{ "ai_free", h1_ai::ai_scripting_free_evaluate },
	{ "ai_free_units", h1_ai::ai_scripting_free_units_evaluate },
	{ "ai_attach", h1_ai::ai_scripting_attach_unit_evaluate },
	{ "ai_attach_units", h1_ai::ai_scripting_attach_units_evaluate },
	{ "ai_attach_free", h1_ai::ai_scripting_attach_free_evaluate },
	{ "ai_detach", h1_ai::ai_scripting_detach_unit_evaluate },
	{ "ai_detach_units", h1_ai::ai_scripting_detach_units_evaluate },
	{ "ai_place", h1_ai::ai_scripting_place_evaluate },
	{ "ai_kill", h1_ai::ai_scripting_kill_evaluate },
	{ "ai_kill_silent", h1_ai::ai_scripting_kill_silent_evaluate },
	{ "ai_erase", h1_ai::ai_scripting_erase_evaluate },
	{ "ai_erase_all", h1_ai::ai_scripting_erase_all_evaluate },
	{ "ai_select", h1_ai::ai_scripting_select_evaluate },
	{ "ai_deselect", h1_ai::ai_scripting_deselect_evaluate },
	{ "ai_spawn_actor", h1_ai::ai_scripting_spawn_actor_evaluate },
	{ "ai_set_respawn", h1_ai::ai_scripting_set_respawn_evaluate },
	{ "ai_set_deaf", h1_ai::ai_scripting_set_deaf_evaluate },
	{ "ai_set_blind", h1_ai::ai_scripting_set_blind_evaluate },
	{ "ai_magically_see_encounter", h1_ai::ai_scripting_magically_see_encounter_evaluate },
	{ "ai_magically_see_players", h1_ai::ai_scripting_magically_see_players_evaluate },
	{ "ai_magically_see_unit", h1_ai::ai_scripting_magically_see_unit_evaluate },
	{ "ai_magically_see_units", h1_ai::ai_scripting_magically_see_units_evaluate },
	{ "ai_timer_start", h1_ai::ai_scripting_timer_start_evaluate },
	{ "ai_timer_expire", h1_ai::ai_scripting_timer_expire_evaluate },
	{ "ai_attack", h1_ai::ai_scripting_attack_evaluate },
	{ "ai_defend", h1_ai::ai_scripting_defend_evaluate },
	{ "ai_retreat", h1_ai::ai_scripting_retreat_evaluate },
	{ "ai_maneuver", h1_ai::ai_scripting_maneuver_evaluate },
	{ "ai_maneuver_enable", h1_ai::ai_scripting_maneuver_enable_evaluate },
	{ "ai_migrate", h1_ai::ai_scripting_migrate_evaluate },
	{ "ai_migrate_and_speak", h1_ai::ai_scripting_migrate_and_speak_evaluate },
	{ "ai_migrate_by_unit", h1_ai::ai_scripting_migrate_by_unit_evaluate },
	{ "ai_allegiance", h1_ai::ai_scripting_allegiance_evaluate },
	{ "ai_allegiance_remove", h1_ai::ai_scripting_allegiance_remove_evaluate },
	{ "ai_go_to_vehicle", h1_ai::ai_scripting_go_to_vehicle_evaluate },
	{ "ai_go_to_vehicle_override", h1_ai::ai_scripting_go_to_vehicle_override_evaluate },
	{ "ai_exit_vehicle", h1_ai::ai_scripting_exit_vehicle_evaluate },
	{ "ai_braindead", h1_ai::ai_scripting_braindead_evaluate },
	{ "ai_braindead_by_unit", h1_ai::ai_scripting_braindead_by_unit_evaluate },
	{ "ai_disregard", h1_ai::ai_scripting_ignore_evaluate },
	{ "ai_prefer_target", h1_ai::ai_scripting_prefer_target_evaluate },
	{ "ai_teleport_to_starting_location", h1_ai::ai_scripting_teleport_starting_location_evaluate },
	{ "ai_teleport_to_starting_location_if_unsupported", h1_ai::ai_scripting_teleport_starting_location_if_unsupported_evaluate },
	{ "ai_renew", h1_ai::ai_scripting_renew_evaluate },
	{ "ai_try_to_fight_nothing", h1_ai::ai_scripting_try_to_fight_nothing_evaluate },
	{ "ai_try_to_fight", h1_ai::ai_scripting_try_to_fight_evaluate },
	{ "ai_try_to_fight_player", h1_ai::ai_scripting_try_to_fight_player_evaluate },
	{ "ai_command_list", h1_ai::ai_scripting_command_list_evaluate },
	{ "ai_command_list_by_unit", h1_ai::ai_scripting_command_list_by_unit_evaluate },
	{ "ai_command_list_advance", h1_ai::ai_scripting_command_list_advance_evaluate },
	{ "ai_command_list_advance_by_unit", h1_ai::ai_scripting_command_list_advance_by_unit_evaluate },
	{ "ai_force_active", h1_ai::ai_scripting_force_active_evaluate },
	{ "ai_force_active_by_unit", h1_ai::ai_scripting_force_active_by_unit_evaluate },
	{ "ai_set_return_state", h1_ai::ai_scripting_set_return_state_evaluate },
	{ "ai_set_current_state", h1_ai::ai_scripting_set_current_state_evaluate },
	{ "ai_playfight", h1_ai::ai_scripting_playfight_evaluate },
	{ "ai_reconnect", h1_ai::ai_scripting_reconnect_evaluate },
	{ "ai_vehicle_encounter", h1_ai::ai_scripting_vehicle_encounter_evaluate },
	{ "ai_vehicle_enterable_distance", h1_ai::ai_scripting_vehicle_enterable_distance_evaluate },
	{ "ai_vehicle_enterable_team", h1_ai::ai_scripting_vehicle_enterable_team_evaluate },
	{ "ai_vehicle_enterable_actor_type", h1_ai::ai_scripting_vehicle_enterable_actor_type_evaluate },
	{ "ai_vehicle_enterable_actors", h1_ai::ai_scripting_vehicle_enterable_actors_evaluate },
	{ "ai_vehicle_enterable_disable", h1_ai::ai_scripting_vehicle_enterable_disable_evaluate },
	{ "ai_look_at_object", h1_ai::ai_scripting_look_at_object_evaluate },
	{ "ai_stop_looking", h1_ai::ai_scripting_stop_looking_evaluate },
	{ "ai_automatic_migration_target", h1_ai::ai_scripting_automatic_migration_target_evaluate },
	{ "ai_follow_target_disable", h1_ai::ai_scripting_follow_target_disable_evaluate },
	{ "ai_follow_target_players", h1_ai::ai_scripting_follow_target_players_evaluate },
	{ "ai_follow_target_unit", h1_ai::ai_scripting_follow_target_unit_evaluate },
	{ "ai_follow_target_ai", h1_ai::ai_scripting_follow_target_ai_evaluate },
	{ "ai_follow_distance", h1_ai::ai_scripting_follow_distance_evaluate },
	{ "ai_conversation_stop", h1_ai::ai_scripting_conversation_stop_evaluate },
	{ "ai_conversation_advance", h1_ai::ai_scripting_conversation_advance_evaluate },
	{ "ai_link_activation", h1_ai::ai_scripting_link_activation_evaluate },
	{ "ai_berserk", h1_ai::ai_scripting_berserk_evaluate },
	{ "ai_set_team", h1_ai::ai_scripting_set_team_evaluate },
	{ "ai_allow_charge", h1_ai::ai_scripting_allow_charge_evaluate },
	{ "ai_allow_dormant", h1_ai::ai_scripting_allow_dormant_evaluate },
	{ "ai_is_attacking", h1_ai::ai_scripting_is_attacking_evaluate },
	{ "ai_command_list_status", h1_ai::ai_scripting_command_list_status_evaluate },
	{ "ai_going_to_vehicle", h1_ai::ai_scripting_going_to_vehicle_evaluate },
	{ "ai_living_count", h1_ai::ai_scripting_living_count_evaluate },
	{ "ai_living_fraction", h1_ai::ai_scripting_living_fraction_evaluate },
	{ "ai_strength", h1_ai::ai_scripting_strength_evaluate },
	{ "ai_swarm_count", h1_ai::ai_scripting_swarm_count_evaluate },
	{ "ai_nonswarm_count", h1_ai::ai_scripting_nonswarm_count_evaluate },
	{ "ai_actors", h1_ai::object_list_from_ai_reference_evaluate },
	{ "ai_status", h1_ai::ai_scripting_status_evaluate },
	{ "ai_conversation", h1_ai::ai_scripting_conversation_evaluate },
	{ "ai_conversation_line", h1_ai::ai_scripting_conversation_line_evaluate },
	{ "ai_conversation_status", h1_ai::ai_scripting_conversation_status_evaluate },
	{ "ai_allegiance_broken", h1_ai::ai_scripting_allegiance_broken_evaluate },
};
const int32 g_h1_ai_hs_function_count = NUMBEROF(g_h1_ai_hs_functions);

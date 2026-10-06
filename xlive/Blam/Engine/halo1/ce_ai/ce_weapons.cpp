#include "stdafx.h"
#include "h1_ai_internal.h"

/*
* weapons.c: the functions of it the AI calls.
*/

namespace h1_ai
{

/* ---------- items/weapons.c: its declarations */

/* ---------- headers */

/* port/linux/game/pal_tags.c's */
short pal_tags_first_person_frames(long graph_index, short animation_index, short frames);

/* ---------- constants */

enum weapon_trigger_flags
{
	_weapon_trigger_released_since_last_shot_bit = 0,
	_weapon_trigger_was_down_bit,
	_weapon_trigger_toggled_bit,
	_weapon_trigger_useless_bit,
	_weapon_trigger_blurred_bit,
	_weapon_trigger_fired_before_charging_bit,
	NUMBER_OF_WEAPON_TRIGGER_DATUM_FLAGS,
};

enum
{
	MAXIMUM_NUMBER_OF_TRIGGERS_PER_WEAPON = 2,
};

/* TU-local copies: no shared header declares these tag/runtime enumerations yet.
   Names follow the HCEA database enumerations; ai.c and actors.c carry their own
   ai unit effect copies, and first_person_weapons.c an animation update result copy. */
enum trigger_distribution_function
{
	_trigger_distribution_point = 0,
	_trigger_distribution_horizontal_fan,
	NUMBER_OF_TRIGGER_DISTRIBUTION_FUNCTIONS,
};

enum trigger_firing_effect_type
{
	_trigger_firing_effect = 0,
	_trigger_overheated_effect,
	_trigger_empty_effect,
	NUMBER_OF_TRIGGER_FIRING_EFFECTS,
};

enum weapon_overcharged_action
{
	_trigger_overcharged_none = 0,
	_trigger_overcharged_explodes,
	_trigger_overcharged_fire,
	NUMBER_OF_TRIGGER_OVERCHARGED_ACTIONS,
};

enum weapon_secondary_trigger_mode
{
	_weapon_secondary_trigger_normal = 0,
	_weapon_secondary_trigger_slaved_to_primary,
	_weapon_secondary_trigger_inhibits_primary,
	_weapon_secondary_trigger_loads_alternate_ammunition,
	_weapon_secondary_trigger_loads_multiple_primary_ammunition,
	NUMBER_OF_WEAPON_SECONDARY_TRIGGER_MODES,
};

enum weapon_magazine_flags
{
	_weapon_magazine_wastes_rounds_when_reloaded_bit = 0,
	_weapon_magazine_must_be_chambered_every_shot_bit,
	NUMBER_OF_WEAPON_MAGAZINE_FLAGS,
};

enum animation_update_result
{
	_animation_running = 0,
	_animation_key_frame,
	_animation_will_restart_on_next_frame,
	_animation_restarted,
	_animation_looped,
	NUMBER_OF_ANIMATION_UPDATE_RESULTS,
};

enum
{
	_ai_unit_effect_bump = 0,
	_ai_unit_effect_shooting,
	_ai_unit_effect_death_scream,
	_ai_unit_effect_magic_sight,
	NUMBER_OF_AI_UNIT_EFFECTS,
};

/* ---------- macros */

/* ---------- structures */

struct weapon_interface_magazine_state
{
	boolean reloading;
	boolean can_fire;
	short rounds_loaded;
	short rounds_loaded_maximum;
	short rounds_remaining;
	short rounds_remaining_maximum;
};

struct weapon_interface_state
{
	real heat;
	real age;
	boolean overheated;
	byte _pad09;
	short magazine_count;
	struct weapon_interface_magazine_state magazines[2];
};

struct animation_graph_weapon_animations
{
	long unused1[4];
	struct tag_block animations;
};

struct animation_graph_first_person_weapon_animations
{
	long unused1[4];
	struct tag_block animations;
};

/* TU-local: weapon_trigger_definition.firing_effects element; no shared header declares it yet. */
struct trigger_firing_effect
{
	short shots_lower_bound;
	short shots_upper_bound;
	long unused[8];
	struct tag_reference effects[NUMBER_OF_TRIGGER_FIRING_EFFECTS];
	struct tag_reference damage_effects[NUMBER_OF_TRIGGER_FIRING_EFFECTS];
};

/* ---------- prototypes */

static struct weapon_trigger *weapon_trigger_get(
	struct weapon_datum *weapon,
	short trigger_index);
static struct weapon_magazine *weapon_magazine_get(
	struct weapon_datum *weapon,
	short magazine_index);
static real weapon_trigger_get_charged_fraction(
	long weapon_index,
	short trigger_index);

static boolean weapon_busy(
	long weapon_index);
static boolean weapon_magazine_state_change_ok(
	long weapon_index);
static long weapon_get_effect_object_index(
	long weapon_index);
static long weapon_get_owner_object_index(
	long weapon_index);
static long weapon_get_projectile_owner_object_index(
	long weapon_index);
static boolean weapon_trigger_can_fire_again(
	long weapon_index,
	short trigger_index);
static void weapon_magazine_idle(
	long weapon_index,
	short magazine_index);
static long weapon_effect_looping_new(
	long weapon_index,
	long effect_index);
static void weapon_detonate(
	long weapon_index);
static void weapon_trigger_change_state(
	long weapon_index,
	short trigger_index,
	short new_state,
	short new_state_timer);
static void weapon_trigger_start_ejection_port(
	long weapon_index,
	short trigger_index,
	boolean chamber);
static void weapon_state_key_frame(
	long weapon_index);
static boolean weapon_magazine_state_interruptable(
	short old_state,
	short new_state);
static long weapon_effect_new(
	long weapon_index,
	long effect_index,
	real effect_scale,
	real effect_error);
static void weapon_magazine_start_chamber(
	long weapon_index,
	short magazine_index);
static void weapon_magazine_finish_chamber(
	long weapon_index,
	short magazine_index);
static void weapon_trigger_fully_charged(
	long weapon_index,
	short trigger_index);
static void weapon_trigger_idle(
	long weapon_index,
	short trigger_index);
static void weapon_trigger_locked(
	long weapon_index,
	short trigger_index);
static void weapon_trigger_recover(
	long weapon_index,
	short trigger_index);
static void weapon_trigger_finish_tracking(
	long weapon_index,
	short trigger_index);
static void weapon_reset(
	long weapon_index);

static boolean weapon_state_interruptable(
	short old_state,
	short new_state);
static boolean weapon_set_state(
	long weapon_index,
	short new_state,
	boolean immediate);

static void weapon_magazine_finish_reload(
	long weapon_index,
	short magazine_index);
static void weapon_magazine_start_reload(
	long weapon_index,
	short magazine_index,
	boolean unknown);
static void weapon_state_next(
	long weapon_index);

static void projectile_distribute(
	real_vector3d *forward,
	real_vector3d *up,
	short distribution_function,
	real distribution_angle,
	short projectile_index,
	short projectile_count);
static void trigger_create_projectiles(
	long weapon_index,
	short trigger_index);
static void weapon_trigger_fire(
	long weapon_index,
	short trigger_index);
static void weapon_trigger_begin_firing(
	long weapon_index,
	short trigger_index,
	boolean force);
static void weapon_trigger_overload(
	long weapon_index,
	long trigger_index);
static void weapon_trigger_release_charge(
	long weapon_index,
	short trigger_index);
static void weapon_trigger_overcharged(
	long weapon_index,
	short trigger_index);

/* ---------- globals */

struct weapons_globals
{
	char *blurred_permutation_names[2];
	struct profile_section update_profile;
};

static struct weapons_globals data_00307140 =
{
	{"~primary-blur", "~secondary-blur"},
	{"weapon_update", NONE, TRUE}
};

/* ---------- public code */

/* ---------- private code */

// TODO: finish

/* ---------- items/weapons.c: its functions */

boolean weapon_aim(
	long weapon_index,
	short trigger_index,
	real_point3d const *origin,
	real_point3d const *target_point,
	boolean lob,
	real_vector3d *result_aim_vector,
	real *result_ticks,
	real *result_distance,
	boolean *result_linear)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);
	boolean result = FALSE;

	if (trigger_index>=0 && trigger_index<weapon_definition->weapon.triggers.count)
	{
		struct weapon_trigger *trigger = weapon_trigger_get(weapon, trigger_index);
		struct weapon_trigger_definition *trigger_definition = TAG_BLOCK_GET_ELEMENT(&weapon_definition->weapon.triggers, trigger_index, struct weapon_trigger_definition);

		projectile_aim(projectile_definition_get(trigger_definition->projectile.index), origin, target_point, NULL, NULL, NULL, NULL, lob, result_aim_vector, NULL, result_ticks, result_distance, result_linear);
		match_assert_valid_real_normal3d("c:\\halo\\SOURCE\\items\\weapons.c", 1301, result_aim_vector);

		result = TRUE;
	}

	return result;
}

real weapon_estimate_time_to_target(
	long weapon_index,
	short trigger_index,
	real distance)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);
	real result = 0.0f;

	if (trigger_index>=0 && trigger_index<weapon_definition->weapon.triggers.count)
	{
		struct weapon_trigger_definition *trigger_definition = TAG_BLOCK_GET_ELEMENT(&weapon_definition->weapon.triggers, trigger_index, struct weapon_trigger_definition);

		result = projectile_estimate_time_to_target(projectile_definition_get(trigger_definition->projectile.index), distance);
	}

	return result;
}

static struct weapon_trigger *weapon_trigger_get(
	struct weapon_datum *weapon,
	short trigger_index)
{
	struct weapon_definition const *weapon_definition = weapon_definition_get(weapon->definition_index);

	match_assert("c:\\halo\\SOURCE\\items\\weapons.c", 1639, trigger_index>=0 && trigger_index<weapon_definition->weapon.triggers.count);

	return &weapon->weapon.triggers[trigger_index];
}

char const *weapon_get_label(
	long weapon_index)
{
	char const *label = "";

	if (weapon_index!=NONE)
	{
		struct weapon_datum *weapon = weapon_get(weapon_index);
		struct weapon_definition *weapon_definition = weapon_definition_get(weapon->definition_index);

		label = weapon_definition->weapon.label;
	}

	return label;
}

} // namespace h1_ai

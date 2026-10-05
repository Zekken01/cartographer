#include "stdafx.h"
#include "h1_weapon_logic.h"

#include "h1_cache_file.h"
#include "h1_effects.h"
#include "h1_first_person_weapon.h"
#include "h1_log.h"
#include "h1_map_loader.h"
#include "h1_projectile_logic.h"
#include "h1_projectiles.h"
#include "h1_sound.h"
#include "h1_weapons.h"

#include "game/game_time.h"
#include "items/weapons.h"
#include "math/real_math.h"
#include "objects/object_placement.h"
#include "objects/damage.h"
#include "objects/object_types.h"
#include "objects/objects.h"
#include "tag_files/tag_groups.h"
#include "units/units.h"

#include <unordered_map>
#include <vector>

/* constants */

enum
{
	k_h1_ticks_per_second = 30,
	k_h1_maximum_triggers = 2,
	k_h1_maximum_magazines = 2,
	k_h1_maximum_trigger_markers = 8,
	k_h1_maximum_queued_messages = 16,
};

// weapons.h
enum
{
	_h1_weapon_state_idle = 0,
	_h1_weapon_state_primary_recoil,
	_h1_weapon_state_secondary_recoil,
	_h1_weapon_state_primary_chamber,
	_h1_weapon_state_secondary_chamber,
	_h1_weapon_state_primary_reload,
	_h1_weapon_state_secondary_reload,
	_h1_weapon_state_primary_charged,
	_h1_weapon_state_secondary_charged,
	_h1_weapon_state_ready,
	_h1_weapon_state_put_away,
};

enum
{
	_h1_trigger_idle = 0,
	_h1_trigger_overloading,
	_h1_trigger_charging,
	_h1_trigger_charged,
	_h1_trigger_recovering,
	_h1_trigger_tracking,
	_h1_trigger_spewing,
	_h1_trigger_locked,
	_h1_trigger_uninitialized,
};

enum
{
	_h1_magazine_idle = 0,
	_h1_magazine_reloading,
	_h1_magazine_unchambered,
	_h1_magazine_chambering,
};

enum
{
	_h1_weapon_control_integrated_light_bit = 0,
	_h1_weapon_control_primary_trigger_bit,
	_h1_weapon_control_secondary_trigger_bit,
	_h1_weapon_control_reload_bit,
	_h1_weapon_control_user_busy_bit,
	_h1_weapon_control_user_switching_weapons_bit,
	_h1_weapon_control_zoomed_bit,
};

// weapon_datum_flags.h
enum
{
	_h1_weapon_overheated_bit = 0,
	_h1_weapon_overheated_exit_bit,
	_h1_weapon_overheat_recoil_bit,
	_h1_weapon_needs_to_reload_bit,
};

// weapons.c weapon_trigger_flags
enum
{
	_h1_weapon_trigger_released_since_last_shot_bit = 0,
	_h1_weapon_trigger_was_down_bit,
	_h1_weapon_trigger_toggled_bit,
	_h1_weapon_trigger_useless_bit,
	_h1_weapon_trigger_blurred_bit,
	_h1_weapon_trigger_fired_before_charging_bit,
};

// weapon_definitions.h
enum
{
	_h1_weapon_definition_cannot_fire_at_maximum_age_bit = 11,
	_h1_weapon_definition_secondary_trigger_overrides_grenades_bit = 12,
};

enum
{
	_h1_trigger_definition_tracks_projectile_bit = 0,
	_h1_trigger_definition_random_firing_effects_bit,
	_h1_trigger_definition_can_fire_with_partial_ammunition_bit,
	_h1_trigger_definition_latched_bit,
	_h1_trigger_definition_toggles_bit,
	_h1_trigger_definition_uses_weapon_origin_bit,
	_h1_trigger_definition_sticks_when_dropped_bit,
	_h1_trigger_definition_ejection_port_during_chamber_animation_bit,
	_h1_trigger_definition_discharging_spews_bit,
	_h1_trigger_definition_analog_rate_of_fire_bit,
	_h1_trigger_definition_use_error_when_unzoomed_bit,
	_h1_trigger_definition_projectiles_cannot_be_aimed_bit,
	_h1_trigger_definition_projectiles_have_identical_error_bit,
};

enum
{
	_h1_trigger_distribution_point = 0,
	_h1_trigger_distribution_horizontal_fan,
};

enum
{
	_h1_trigger_overcharged_none = 0,
	_h1_trigger_overcharged_explodes,
	_h1_trigger_overcharged_fire,
};

enum
{
	_h1_weapon_secondary_trigger_normal = 0,
	_h1_weapon_secondary_trigger_slaved_to_primary,
	_h1_weapon_secondary_trigger_inhibits_primary,
	_h1_weapon_secondary_trigger_loads_alternate_ammunition,
	_h1_weapon_secondary_trigger_loads_multiple_primary_ammunition,
};

enum
{
	_h1_weapon_magazine_wastes_rounds_when_reloaded_bit = 0,
	_h1_weapon_magazine_must_be_chambered_every_shot_bit,
};

enum
{
	_h1_weapon_type_undefined = 0,
	_h1_weapon_type_shotgun,
	_h1_weapon_type_needler,
	_h1_weapon_type_plasma_pistol,
	_h1_weapon_type_plasma_rifle,
};

enum
{
	_h1_trigger_firing_effect = 0,
	_h1_trigger_overheated_effect,
	_h1_trigger_empty_effect,
};

// weapons.h first person animations used for times
enum
{
	_h1_first_person_animation_reload_while_empty = 7,
	_h1_first_person_animation_ready = 10,
	_h1_first_person_animation_shotgun_enter = 23,
};

enum
{
	_h1_shotgun_reload_type_first_round = 0,
	_h1_shotgun_reload_type_last_round,
	_h1_shotgun_reload_type_first_and_last_round,
};

// halo 2's unit control flags (unit +0x24) and weapon control flags (weapon +0x16E) read for the halo 1 controls
enum : uint32
{
	k_h2_unit_control_primary_trigger_pressed = FLAG(8),
	k_h2_unit_control_primary_trigger_held = FLAG(18),
	k_h2_unit_control_reload = FLAG(30),
	k_h2_weapon_control_busy = FLAG(5),
};

/* structures */

// weapons.h weapon_trigger
struct s_h1_weapon_logic_trigger
{
	int8 idle_ticks;
	int8 state;
	int16 state_timer;
	uint32 flags;
	uint16 firing_effects_used_flags;
	int16 firing_effect_index;
	int16 firing_effect_shots_remaining;
	int16 sequential_non_tracer_rounds;
	real32 rate_of_fire;
	real32 ejection_port_position;
	real32 illumination;
	real32 error;
	int32 charging_effect_id;
};

// weapons.h weapon_magazine: the rounds stay in halo 2's magazine
struct s_h1_weapon_logic_magazine
{
	int16 state;
	int16 state_timer;
	int16 original_time;
	int16 rounds_fractional_recharged;
};

// weapons.h _weapon_datum, the fields halo 2's weapon doesn't keep
struct s_h1_weapon_logic_state
{
	datum definition_index;
	uint32 flags;
	uint16 control_flags;
	real32 primary_trigger;
	int8 state;
	int16 state_timer;
	real32 heat;
	real32 age;
	real32 overcharged;
	int16 alternate_shots_loaded;
	datum tracked_object_index;
	s_h1_weapon_logic_trigger triggers[k_h1_maximum_triggers];
	s_h1_weapon_logic_magazine magazines[k_h1_maximum_magazines];
	// the weapon's own animation (weapon_set_state), the state ends with it
	int16 animation_index;
	int16 animation_frame;
	int32 overheated_effect_id;
	bool in_hands;
	real32 leftover_ticks;
	bool primary_pressed;
	std::vector<s_h1_first_person_weapon_message> messages;
};

struct s_h1_weapon_logic_context
{
	datum weapon_index;
	weapon_datum* weapon;
	const h1_weap* definition;
	s_h1_weapon_logic_state* state;
};

typedef bool(__cdecl* t_h2_magazine_update)(datum weapon_index, int32 magazine_index);

/* globals */

static std::unordered_map<datum, s_h1_weapon_logic_state> g_h1_weapon_logic;
static object_update_t g_h2_weapon_update = NULL;

// a player's melee: when halo 2's biped began it and whether its first person melee has begun
struct s_h1_unit_melee
{
	int32 begin_time;
	bool waiting_for_animation;
};
static std::unordered_map<datum, s_h1_unit_melee> g_h1_unit_melee;
static uint32 g_h1_weapon_random_seed = 0x1234567u;

/* prototypes */

static bool h1_weapon_update_hook(datum weapon_index);
// halo 2's biped melee begin (FUN_00567ecb): starts the melee (or the combo's next strike) the biped asks for
typedef bool(__cdecl* t_biped_melee_begin)(datum biped_index, void* melee_request);
static t_biped_melee_begin p_biped_melee_begin = NULL;
static bool __cdecl h1_biped_melee_begin_hook(datum biped_index, void* melee_request);
static bool __cdecl h1_weapon_magazine_update_hook(datum weapon_index, int32 magazine_index);
static bool h1_weapon_context_get(datum weapon_index, s_h1_weapon_logic_context* context);
static void h1_weapon_new(s_h1_weapon_logic_context* context);
static void h1_weapon_tick(s_h1_weapon_logic_context* context);
static void h1_weapon_hands_changed(s_h1_weapon_logic_context* context, bool in_hands);
static void h1_weapon_mirror_to_h2(s_h1_weapon_logic_context* context);

static const h1_weap_triggers* h1_trigger_definition_get(const s_h1_weapon_logic_context* context, int16 trigger_index);
static const h1_weap_magazines* h1_magazine_definition_get(const s_h1_weapon_logic_context* context, int16 magazine_index);
static bool h1_weapon_belongs_to_player(const s_h1_weapon_logic_context* context);
static datum h1_weapon_owner_object_index(const s_h1_weapon_logic_context* context);
static void h1_first_person_weapon_message(s_h1_weapon_logic_context* context, e_h1_first_person_weapon_message message);
static int32 h1_weapon_effect_new(s_h1_weapon_logic_context* context, const h1_tag_reference* effect, real32 scale, real32 error, bool looping);
static int16 h1_weapon_first_person_animation_time(const s_h1_weapon_logic_context* context, bool key_frame, int16 animation_type, int16 shotgun_reload_type);
static bool h1_weapon_busy(const s_h1_weapon_logic_context* context);
static bool h1_weapon_magazine_state_change_ok(const s_h1_weapon_logic_context* context);
static bool h1_weapon_set_state(s_h1_weapon_logic_context* context, int8 new_state, bool immediate);
static void h1_weapon_state_next(s_h1_weapon_logic_context* context);
static void h1_weapon_state_key_frame(s_h1_weapon_logic_context* context);
static void h1_weapon_animation_update(s_h1_weapon_logic_context* context);
static void h1_weapon_reset(s_h1_weapon_logic_context* context);
static void h1_weapon_detonate(s_h1_weapon_logic_context* context);
static void h1_weapon_magazine_idle(s_h1_weapon_logic_context* context, int16 magazine_index);
static void h1_weapon_magazine_start_reload(s_h1_weapon_logic_context* context, int16 magazine_index, bool first);
static void h1_weapon_magazine_finish_reload(s_h1_weapon_logic_context* context, int16 magazine_index);
static void h1_weapon_magazine_start_chamber(s_h1_weapon_logic_context* context, int16 magazine_index);
static void h1_weapon_trigger_change_state(s_h1_weapon_logic_context* context, int16 trigger_index, int8 new_state, int16 new_state_timer);
static void h1_weapon_trigger_idle(s_h1_weapon_logic_context* context, int16 trigger_index);
static void h1_weapon_trigger_recover(s_h1_weapon_logic_context* context, int16 trigger_index);
static void h1_weapon_trigger_fully_charged(s_h1_weapon_logic_context* context, int16 trigger_index);
static void h1_weapon_trigger_start_ejection_port(s_h1_weapon_logic_context* context, int16 trigger_index, bool chamber);
static bool h1_weapon_trigger_can_fire_again(s_h1_weapon_logic_context* context, int16 trigger_index);
static void h1_weapon_trigger_begin_firing(s_h1_weapon_logic_context* context, int16 trigger_index, bool force);
static void h1_weapon_trigger_fire(s_h1_weapon_logic_context* context, int16 trigger_index);
static void h1_weapon_trigger_overload(s_h1_weapon_logic_context* context, int16 trigger_index);
static void h1_weapon_trigger_release_charge(s_h1_weapon_logic_context* context, int16 trigger_index);
static void h1_weapon_trigger_overcharged(s_h1_weapon_logic_context* context, int16 trigger_index);
static void h1_weapon_trigger_create_projectiles(s_h1_weapon_logic_context* context, int16 trigger_index);
static void h1_weapon_damage_owner(datum owner_object_index, datum h1_damage_effect_index);

static real32 h1_weapon_random_real(void);
static int32 h1_weapon_random_range(int32 lower, int32 upper);
static void h1_random_vector_in_cone3d(const real_vector3d* axis, real32 inner_cone_angle, real32 outer_cone_angle, real_vector3d* result);
static void h1_projectile_distribute(real_vector3d* forward, const real_vector3d* up, int16 distribution_function, real32 distribution_angle,
	int16 projectile_index, int16 projectile_count);

/* public code */

void h1_weapon_logic_apply_patches(void)
{
	// the weapon part of the weapon object type: its update runs halo 1's after halo 2's for the halo 1 weapons
	object_type_definition* weapon_type = object_type_definition_get(_object_type_weapon);
	for (int32 i = 0; i < k_max_object_type_inheritence; i++)
	{
		object_type_definition* part = weapon_type->part_definitions[i];
		if (part && part->group_tag == 'weap' && part->object_update)
		{
			g_h2_weapon_update = part->object_update;
			part->object_update = h1_weapon_update_hook;
			break;
		}
	}
	// weapon_update's magazine update (FUN_00561592)
	PatchCall(Memory::GetAddress(0x161E93), h1_weapon_magazine_update_hook);
	DETOUR_ATTACH(p_biped_melee_begin, Memory::GetAddress<t_biped_melee_begin>(0x167ECB), h1_biped_melee_begin_hook);
	return;
}

// halo 2's melee on a halo 1 map: a player's biped starts no melee (no strike of halo 2's combo either) until the halo 1 first person
// melee animation of the last one is over
static bool __cdecl h1_biped_melee_begin_hook(datum biped_index, void* melee_request)
{
	const unit_datum* unit = h1_maps_active() ? (const unit_datum*)object_try_and_get_and_verify_type(biped_index, _object_mask_unit) : NULL;
	if (!unit || unit->unit.player_index == NONE)
	{
		return p_biped_melee_begin(biped_index, melee_request);
	}
	s_h1_unit_melee* melee = &g_h1_unit_melee[biped_index];
	const int32 time = (int32)game_time_get();
	const bool meleeing = h1_first_person_weapon_meleeing(biped_index);
	// (a second at most for the first person melee to begin)
	if (meleeing || time < melee->begin_time || time - melee->begin_time > (int32)(1.f / game_tick_length()))
	{
		melee->waiting_for_animation = false;
	}
	if (meleeing || melee->waiting_for_animation)
	{
		return false;
	}
	const bool result = p_biped_melee_begin(biped_index, melee_request);
	if (result)
	{
		melee->begin_time = time;
		melee->waiting_for_animation = true;
		// first_person_weapon_message_from_unit: the first person melee for every strike halo 2 begins
		const datum weapon_index = unit->unit.weapon_indices[0] != NONE ? unit_inventory_get_weapon(biped_index, unit->unit.weapon_indices[0]) : NONE;
		s_h1_weapon_logic_context context;
		if (weapon_index != NONE && h1_weapon_context_get(weapon_index, &context))
		{
			h1_first_person_weapon_message(&context, _h1_first_person_weapon_message_melee);
		}
	}
	return result;
}

void h1_weapon_logic_reset(void)
{
	g_h1_weapon_logic.clear();
	g_h1_unit_melee.clear();
	return;
}

real32 h1_weapon_logic_charged_fraction(datum weapon_index, int16 trigger_index)
{
	s_h1_weapon_logic_context context;
	if (!VALID_INDEX(trigger_index, k_h1_maximum_triggers) || !h1_weapon_context_get(weapon_index, &context) ||
		trigger_index >= context.definition->triggers.count)
	{
		return 0.f;
	}
	// weapons.c weapon_trigger_get_charged_fraction
	const s_h1_weapon_logic_trigger* trigger = &context.state->triggers[trigger_index];
	const h1_weap_triggers* trigger_definition = h1_trigger_definition_get(&context, trigger_index);
	switch (trigger->state)
	{
	case _h1_trigger_charging:
		return trigger_definition->charging_time > 0.f ?
			1.f - (trigger->state_timer * (1.f / k_h1_ticks_per_second)) / trigger_definition->charging_time : 1.f;
	case _h1_trigger_charged:
		return 1.f;
	}
	return 0.f;
}

bool h1_weapon_logic_interface_state(datum weapon_index, s_h1_weapon_interface_state* interface_state)
{
	s_h1_weapon_logic_context context;
	csmemset(interface_state, 0, sizeof(*interface_state));
	if (!h1_weapon_context_get(weapon_index, &context))
	{
		return false;
	}
	// weapons.c weapon_build_weapon_interface_state
	interface_state->heat = context.state->heat;
	interface_state->age = context.state->age;
	interface_state->overheated = TEST_BIT(context.state->flags, _h1_weapon_overheated_bit);
	interface_state->magazine_count = (int16)MIN(context.definition->magazines.count, (int32)k_h1_maximum_magazines);
	for (int16 i = 0; i < interface_state->magazine_count; i++)
	{
		const s_h1_weapon_logic_magazine* magazine = &context.state->magazines[i];
		const weapon_magazine* rounds = &context.weapon->weapon.magazines[i];
		const h1_weap_magazines* magazine_definition = h1_magazine_definition_get(&context, i);
		interface_state->magazines[i].reloading = magazine->state == _h1_magazine_reloading || magazine->state == _h1_magazine_chambering;
		interface_state->magazines[i].can_fire = magazine->state == _h1_magazine_idle;
		interface_state->magazines[i].rounds_loaded = rounds->rounds_loaded;
		interface_state->magazines[i].rounds_loaded_maximum = magazine_definition->rounds_loaded_maximum;
		interface_state->magazines[i].rounds_remaining = rounds->rounds_inventory;
		interface_state->magazines[i].rounds_remaining_maximum = magazine_definition->rounds_total_maximum;
	}
	return true;
}

bool h1_weapon_logic_overheated(datum weapon_index, bool* out_overheated_exit)
{
	auto found = g_h1_weapon_logic.find(weapon_index);
	const uint32 flags = found != g_h1_weapon_logic.end() ? found->second.flags : 0;
	if (out_overheated_exit)
	{
		*out_overheated_exit = TEST_BIT(flags, _h1_weapon_overheated_exit_bit);
	}
	return TEST_BIT(flags, _h1_weapon_overheated_bit);
}

int32 h1_weapon_logic_first_person_messages_take(datum weapon_index, s_h1_first_person_weapon_message* messages, int32 maximum_count)
{
	auto found = g_h1_weapon_logic.find(weapon_index);
	if (found == g_h1_weapon_logic.end())
	{
		return 0;
	}
	std::vector<s_h1_first_person_weapon_message>& queued = found->second.messages;
	const int32 count = MIN((int32)queued.size(), maximum_count);
	for (int32 i = 0; i < count; i++)
	{
		messages[i] = queued[i];
	}
	queued.clear();
	return count;
}

/* private code */

static bool h1_weapon_update_hook(datum weapon_index)
{
	bool result = g_h2_weapon_update(weapon_index);
	if (!h1_maps_active() || !g_h1_cache_file)
	{
		return result;
	}
	s_h1_weapon_logic_context context;
	if (!h1_weapon_context_get(weapon_index, &context))
	{
		return result;
	}

	weapon_datum* weapon = context.weapon;
	s_h1_weapon_logic_state* state = context.state;

	// units.c unit_update_weapons -> weapon_owner_update: the owner's controls, the presses kept for the next halo 1 tick
	uint16 control_flags = 0;
	real32 primary_trigger = 0.f;
	const unit_datum* unit = (const unit_datum*)object_try_and_get_and_verify_type(weapon->object.parent_object_index, _object_mask_unit);
	// only the weapon in the unit's hands gets its controls (the others ride along in the inventory)
	const bool in_hands = unit && unit->unit.weapon_indices[0] != NONE &&
		unit_inventory_get_weapon(weapon->object.parent_object_index, unit->unit.weapon_indices[0]) == weapon_index;
	// halo 2's unit swaps the weapons: weapons.c weapon_ready as the weapon comes into the hands, weapon_put_away as it leaves
	if (in_hands != state->in_hands)
	{
		h1_weapon_hands_changed(&context, in_hands);
		state->in_hands = in_hands;
	}
	if (in_hands)
	{
		const uint32 unit_control = *(const uint32*)&unit->unit.control_flags;
		if (unit_control & k_h2_unit_control_primary_trigger_pressed)
		{
			state->primary_pressed = true;
		}
		if (unit_control & (k_h2_unit_control_primary_trigger_held | k_h2_unit_control_primary_trigger_pressed))
		{
			control_flags |= FLAG(_h1_weapon_control_primary_trigger_bit);
		}
		if (unit_control & k_h2_unit_control_reload)
		{
			SET_BIT(state->flags, _h1_weapon_needs_to_reload_bit, true);
		}
		if (weapon->weapon.control_flags & k_h2_weapon_control_busy)
		{
			control_flags |= FLAG(_h1_weapon_control_user_busy_bit);
		}
		if (unit->unit.current_zoom_level >= 0)
		{
			control_flags |= FLAG(_h1_weapon_control_zoomed_bit);
		}
		primary_trigger = h1_transition_function_evaluate(4, PIN(unit->unit.primary_trigger, 0.f, 1.f));
	}

	// weapon_update once per halo 1 tick
	state->leftover_ticks += game_tick_length() * k_h1_ticks_per_second;
	while (state->leftover_ticks >= 1.f)
	{
		state->leftover_ticks -= 1.f;
		state->control_flags = control_flags;
		if (state->primary_pressed)
		{
			state->control_flags |= FLAG(_h1_weapon_control_primary_trigger_bit);
			state->primary_pressed = false;
		}
		state->primary_trigger = primary_trigger;
		// game_tick: first_person_weapons_update comes before objects_update, where the weapon's messages set its state as it sends them
		const bool first_person = in_hands && weapon->object.parent_object_index == h1_first_person_weapon_unit_get();
		if (first_person)
		{
			h1_first_person_weapon_tick();
		}
		h1_weapon_tick(&context);
		if (!object_try_and_get(weapon_index))
		{
			// detonated
			g_h1_weapon_logic.erase(weapon_index);
			return false;
		}
		if (first_person)
		{
			h1_first_person_weapon_messages();
		}
	}
	h1_weapon_mirror_to_h2(&context);

	return result || weapon->object.parent_object_index != NONE;
}

static bool __cdecl h1_weapon_magazine_update_hook(datum weapon_index, int32 magazine_index)
{
	const weapon_datum* weapon = (const weapon_datum*)object_try_and_get_and_verify_type(weapon_index, _object_mask_weapon);
	if (h1_maps_active() && weapon && h1_weapon_h1_get(weapon->definition_index) != NONE)
	{
		return false;
	}
	return Memory::GetAddress<t_h2_magazine_update>(0x161592)(weapon_index, magazine_index);
}

static bool h1_weapon_context_get(datum weapon_index, s_h1_weapon_logic_context* context)
{
	weapon_datum* weapon = (weapon_datum*)object_try_and_get_and_verify_type(weapon_index, _object_mask_weapon);
	const datum h1_weapon_index = weapon ? h1_weapon_h1_get(weapon->definition_index) : NONE;
	const h1_weap* definition = h1_weapon_index != NONE ? (const h1_weap*)g_h1_cache_file->tag_get('weap', h1_weapon_index) : NULL;
	if (!definition)
	{
		return false;
	}
	context->weapon_index = weapon_index;
	context->weapon = weapon;
	context->definition = definition;
	auto found = g_h1_weapon_logic.find(weapon_index);
	if (found == g_h1_weapon_logic.end() || found->second.definition_index != weapon->definition_index)
	{
		g_h1_weapon_logic[weapon_index] = {};
		context->state = &g_h1_weapon_logic[weapon_index];
		context->state->definition_index = weapon->definition_index;
		h1_weapon_new(context);
	}
	else
	{
		context->state = &found->second;
	}
	return true;
}

// weapons.c weapon_new (halo 2 placed the rounds)
static void h1_weapon_new(s_h1_weapon_logic_context* context)
{
	s_h1_weapon_logic_state* state = context->state;
	state->state = _h1_weapon_state_idle;
	state->tracked_object_index = NONE;
	state->animation_index = NONE;
	state->overheated_effect_id = 0;
	for (int32 i = 0; i < k_h1_maximum_triggers; i++)
	{
		state->triggers[i].charging_effect_id = 0;
		state->triggers[i].idle_ticks = 127;
	}
	return;
}

// weapons.c weapon_update
static void h1_weapon_tick(s_h1_weapon_logic_context* context)
{
	weapon_datum* weapon = context->weapon;
	const h1_weap* definition = context->definition;
	s_h1_weapon_logic_state* state = context->state;
	bool triggers_down[k_h1_maximum_triggers];

	if (state->tracked_object_index != NONE && !object_try_and_get(state->tracked_object_index))
	{
		state->tracked_object_index = NONE;
	}

	h1_weapon_animation_update(context);

	if (state->heat > 0.f)
	{
		if (state->heat >= definition->overheated_threshold && !TEST_BIT(state->flags, _h1_weapon_overheated_bit))
		{
			SET_BIT(state->flags, _h1_weapon_overheated_bit, true);
			if (definition->weapon_type == _h1_weapon_type_plasma_pistol && TEST_BIT(state->flags, _h1_weapon_overheat_recoil_bit))
			{
				SET_BIT(state->flags, _h1_weapon_overheat_recoil_bit, false);
				h1_first_person_weapon_message(context, _h1_first_person_weapon_message_overheating_super_recoil);
			}
			else
			{
				h1_first_person_weapon_message(context, _h1_first_person_weapon_message_overheating);
			}
			state->overheated_effect_id = h1_weapon_effect_new(context, &definition->overheated, 0.f, 0.f, true);
		}

		if (state->overcharged == 0.f)
		{
			real32 heat_loss = definition->heat_loss_per_second * (1.f / k_h1_ticks_per_second);
			if (definition->age_heat_recovery_penalty > 0.f)
			{
				heat_loss *= 1.f - state->age * definition->age_heat_recovery_penalty;
			}
			state->heat -= heat_loss;
			if (state->heat < 0.f)
			{
				state->heat = 0.f;
			}
			if (TEST_BIT(state->flags, _h1_weapon_overheated_bit) && !TEST_BIT(state->flags, _h1_weapon_overheated_exit_bit) &&
				heat_loss > 0.f && (state->heat - definition->heat_recovery_threshold) / heat_loss <= 1.f)
			{
				SET_BIT(state->flags, _h1_weapon_overheated_exit_bit, true);
			}
		}

		if (TEST_BIT(state->flags, _h1_weapon_overheated_bit) && state->heat < definition->heat_recovery_threshold)
		{
			state->flags &= ~(FLAG(_h1_weapon_overheated_bit) | FLAG(_h1_weapon_overheated_exit_bit));
			if (state->overheated_effect_id)
			{
				h1_effect_stop(state->overheated_effect_id);
				state->overheated_effect_id = 0;
			}
		}
	}

	state->overcharged = 0.f;
	if (state->state_timer > 0)
	{
		state->state_timer--;
	}

	if (!TEST_BIT(state->control_flags, _h1_weapon_control_user_busy_bit) && state->state_timer <= 0)
	{
		triggers_down[0] = TEST_BIT(state->control_flags, _h1_weapon_control_primary_trigger_bit);
		triggers_down[1] = TEST_BIT(definition->flags_3, _h1_weapon_definition_secondary_trigger_overrides_grenades_bit) &&
			TEST_BIT(state->control_flags, _h1_weapon_control_secondary_trigger_bit);
	}
	else
	{
		triggers_down[0] = false;
		triggers_down[1] = false;
	}

	switch (definition->secondary_trigger_mode)
	{
	case _h1_weapon_secondary_trigger_slaved_to_primary:
		if (triggers_down[1] && definition->triggers.count > 0 && state->triggers[0].rate_of_fire != 1.f)
		{
			triggers_down[1] = false;
		}
		break;
	case _h1_weapon_secondary_trigger_inhibits_primary:
		if (triggers_down[1])
		{
			triggers_down[0] = false;
		}
		break;
	}

	if (TEST_BIT(state->control_flags, _h1_weapon_control_reload_bit) && definition->magazines.count > 0)
	{
		SET_BIT(state->flags, _h1_weapon_needs_to_reload_bit, true);
	}
	if (TEST_BIT(state->flags, _h1_weapon_needs_to_reload_bit))
	{
		if (definition->magazines.count > 0)
		{
			h1_weapon_magazine_start_reload(context, 0, true);
		}
		else
		{
			SET_BIT(state->flags, _h1_weapon_needs_to_reload_bit, false);
		}
	}

	for (int16 magazine_index = 0; magazine_index < definition->magazines.count && magazine_index < k_h1_maximum_magazines; magazine_index++)
	{
		s_h1_weapon_logic_magazine* magazine = &state->magazines[magazine_index];
		weapon_magazine* rounds = &weapon->weapon.magazines[magazine_index];
		const h1_weap_magazines* magazine_definition = h1_magazine_definition_get(context, magazine_index);

		if (magazine_definition->rounds_recharged > 0 && rounds->rounds_loaded < magazine_definition->rounds_loaded_maximum)
		{
			rounds->rounds_loaded += magazine_definition->rounds_recharged / k_h1_ticks_per_second;
			magazine->rounds_fractional_recharged += magazine_definition->rounds_recharged % k_h1_ticks_per_second;
			if (magazine->rounds_fractional_recharged >= k_h1_ticks_per_second)
			{
				rounds->rounds_loaded++;
				magazine->rounds_fractional_recharged -= k_h1_ticks_per_second;
			}
			if (rounds->rounds_loaded > magazine_definition->rounds_loaded_maximum)
			{
				rounds->rounds_loaded = magazine_definition->rounds_loaded_maximum;
			}
		}

		if (magazine->state_timer)
		{
			magazine->state_timer--;
		}

		switch (magazine->state)
		{
		case _h1_magazine_reloading:
			if (magazine->state_timer - 1 <= 0)
			{
				h1_weapon_magazine_finish_reload(context, magazine_index);
			}
			break;
		case _h1_magazine_unchambered:
			h1_weapon_magazine_start_chamber(context, magazine_index);
			break;
		case _h1_magazine_chambering:
			if (!magazine->state_timer)
			{
				// weapon_magazine_finish_chamber
				h1_weapon_magazine_idle(context, magazine_index);
			}
			break;
		}
	}

	for (int16 trigger_index = 0; trigger_index < definition->triggers.count && trigger_index < k_h1_maximum_triggers; trigger_index++)
	{
		s_h1_weapon_logic_trigger* trigger = &state->triggers[trigger_index];
		const h1_weap_triggers* trigger_definition = h1_trigger_definition_get(context, trigger_index);

		if (TEST_BIT(trigger_definition->flags, _h1_trigger_definition_analog_rate_of_fire_bit) && h1_weapon_belongs_to_player(context))
		{
			triggers_down[trigger_index] = state->primary_trigger > 0.05f;
		}
		if (TEST_BIT(trigger_definition->flags, _h1_trigger_definition_sticks_when_dropped_bit) && weapon->object.parent_object_index == NONE)
		{
			triggers_down[trigger_index] = true;
		}

		if (trigger->state_timer)
		{
			trigger->state_timer--;
		}

		if (TEST_BIT(trigger_definition->flags, _h1_trigger_definition_toggles_bit))
		{
			if (!TEST_BIT(trigger->flags, _h1_weapon_trigger_was_down_bit) && triggers_down[trigger_index])
			{
				trigger->flags ^= FLAG(_h1_weapon_trigger_toggled_bit);
			}
			SET_BIT(trigger->flags, _h1_weapon_trigger_was_down_bit, triggers_down[trigger_index]);
			triggers_down[trigger_index] = TEST_BIT(trigger->flags, _h1_weapon_trigger_toggled_bit);
		}

		if (!triggers_down[trigger_index])
		{
			SET_BIT(trigger->flags, _h1_weapon_trigger_released_since_last_shot_bit, true);
		}

		if (trigger->ejection_port_position > 0.f)
		{
			trigger->ejection_port_position -= trigger_definition->ejection_port_recovery_rate;
			if (trigger->ejection_port_position <= 0.f)
			{
				trigger->ejection_port_position = 0.f;
			}
		}
		if (trigger->illumination > 0.f)
		{
			trigger->illumination -= trigger_definition->illumination_recovery_rate;
			if (trigger->illumination <= 0.f)
			{
				trigger->illumination = 0.f;
			}
		}

		switch (trigger->state)
		{
		case _h1_trigger_idle:
			if (!TEST_BIT(state->control_flags, _h1_weapon_control_user_busy_bit) && weapon->object.parent_object_index != NONE &&
				VALID_INDEX(trigger_definition->magazine_index, MIN(definition->magazines.count, k_h1_maximum_magazines)))
			{
				const weapon_magazine* rounds = &weapon->weapon.magazines[trigger_definition->magazine_index];
				if ((rounds->rounds_loaded < trigger_definition->rounds_per_shot &&
					!TEST_BIT(trigger_definition->flags, _h1_trigger_definition_can_fire_with_partial_ammunition_bit)) ||
					rounds->rounds_loaded < trigger_definition->minimum_rounds_loaded || rounds->rounds_loaded == 0)
				{
					h1_weapon_magazine_start_reload(context, trigger_definition->magazine_index, true);
				}
			}
			if (triggers_down[trigger_index] && h1_weapon_trigger_can_fire_again(context, trigger_index))
			{
				h1_weapon_trigger_begin_firing(context, trigger_index, false);
			}
			else if (trigger->idle_ticks < 127)
			{
				trigger->idle_ticks++;
			}
			break;

		case _h1_trigger_spewing:
			if (trigger->state_timer)
			{
				h1_weapon_trigger_begin_firing(context, trigger_index, true);
			}
			else
			{
				h1_weapon_trigger_recover(context, trigger_index);
			}
			break;

		case _h1_trigger_overloading:
			if (!triggers_down[trigger_index])
			{
				h1_weapon_trigger_begin_firing(context, trigger_index, true);
			}
			else if (!trigger->state_timer && state->alternate_shots_loaded < definition->maximum_alternate_shots_loaded)
			{
				h1_weapon_trigger_overload(context, trigger_index);
			}
			break;

		case _h1_trigger_charging:
			if (trigger->state_timer)
			{
				if (!triggers_down[trigger_index])
				{
					if (trigger_index == 0 && definition->triggers.count > 1 && !TEST_BIT(trigger->flags, _h1_weapon_trigger_fired_before_charging_bit))
					{
						h1_weapon_trigger_begin_firing(context, trigger_index, true);
					}
					else
					{
						h1_weapon_trigger_idle(context, trigger_index);
					}
					if (trigger->charging_effect_id)
					{
						h1_effect_stop(trigger->charging_effect_id);
						trigger->charging_effect_id = 0;
					}
				}
			}
			else
			{
				h1_weapon_trigger_fully_charged(context, trigger_index);
			}
			break;

		case _h1_trigger_charged:
			if (triggers_down[trigger_index])
			{
				state->overcharged = trigger_definition->charged_time > 0.f ?
					1.f - (trigger->state_timer * (1.f / k_h1_ticks_per_second)) / trigger_definition->charged_time : 1.f;
				if (trigger->state_timer)
				{
					if (VALID_INDEX(trigger_definition->magazine_index, MIN(definition->magazines.count, k_h1_maximum_magazines)))
					{
						const weapon_magazine* rounds = &weapon->weapon.magazines[trigger_definition->magazine_index];
						if (rounds->rounds_loaded < trigger_definition->rounds_per_shot &&
							!TEST_BIT(trigger_definition->flags, _h1_trigger_definition_can_fire_with_partial_ammunition_bit))
						{
							h1_weapon_trigger_release_charge(context, trigger_index);
						}
					}
				}
				else
				{
					h1_weapon_trigger_overcharged(context, trigger_index);
				}
			}
			else
			{
				h1_weapon_trigger_release_charge(context, trigger_index);
			}
			break;

		case _h1_trigger_recovering:
			if (!trigger->state_timer)
			{
				if (TEST_BIT(trigger_definition->flags, _h1_trigger_definition_latched_bit) && h1_weapon_belongs_to_player(context) &&
					!TEST_BIT(trigger->flags, _h1_weapon_trigger_released_since_last_shot_bit))
				{
					h1_weapon_trigger_change_state(context, trigger_index, _h1_trigger_locked, NONE);
				}
				else
				{
					h1_weapon_trigger_idle(context, trigger_index);
				}
			}
			break;

		case _h1_trigger_tracking:
			if (!triggers_down[trigger_index] || state->tracked_object_index == NONE)
			{
				// weapon_trigger_finish_tracking
				state->tracked_object_index = NONE;
				h1_weapon_trigger_recover(context, trigger_index);
			}
			break;

		case _h1_trigger_locked:
			if (!triggers_down[trigger_index])
			{
				h1_weapon_trigger_idle(context, trigger_index);
			}
			break;

		case _h1_trigger_uninitialized:
			if (!trigger->state_timer)
			{
				h1_weapon_trigger_idle(context, trigger_index);
			}
			break;
		}

		if (triggers_down[trigger_index])
		{
			trigger->rate_of_fire += trigger_definition->rate_of_fire_acceleration_rate;
			if (trigger->rate_of_fire > 1.f)
			{
				trigger->rate_of_fire = 1.f;
			}
		}
		else
		{
			trigger->rate_of_fire -= trigger_definition->rate_of_fire_deceleration_rate;
			if (trigger->rate_of_fire < 0.f)
			{
				trigger->rate_of_fire = 0.f;
			}
		}

		if (trigger->state == _h1_trigger_spewing || trigger->state == _h1_trigger_recovering || triggers_down[trigger_index])
		{
			trigger->error += trigger_definition->error_acceleration_rate;
			if (trigger->error > 1.f)
			{
				trigger->error = 1.f;
			}
		}
		else
		{
			trigger->error -= trigger_definition->error_deceleration_rate;
			if (trigger->error < 0.f)
			{
				trigger->error = 0.f;
			}
		}
	}
	return;
}

// halo 2's weapon swap readies and puts away the weapon
static void h1_weapon_hands_changed(s_h1_weapon_logic_context* context, bool in_hands)
{
	s_h1_weapon_logic_state* state = context->state;
	if (in_hands)
	{
		// the messages of its time in the inventory (its put away) are old
		state->messages.clear();
		// weapons.c weapon_ready
		h1_weapon_reset(context);
		h1_weapon_set_state(context, _h1_weapon_state_ready, true);
		h1_first_person_weapon_message(context, _h1_first_person_weapon_message_ready);
		h1_weapon_effect_new(context, &context->definition->ready_effect, 0.f, 0.f, false);
		state->state_timer = h1_weapon_first_person_animation_time(context, false, _h1_first_person_animation_ready, NONE);
	}
	else
	{
		// weapons.c weapon_put_away
		h1_weapon_set_state(context, _h1_weapon_state_put_away, true);
		state->control_flags = 0;
		h1_weapon_reset(context);
		if (state->overheated_effect_id)
		{
			h1_effect_stop(state->overheated_effect_id);
			state->overheated_effect_id = 0;
		}
		h1_first_person_weapon_message(context, _h1_first_person_weapon_message_put_away);
	}
	return;
}

// the halo 1 state halo 2 shows: the object functions, the hud's heat and ammunition, halo 2's magazines left idle
static void h1_weapon_mirror_to_h2(s_h1_weapon_logic_context* context)
{
	weapon_datum* weapon = context->weapon;
	const s_h1_weapon_logic_state* state = context->state;
	weapon->weapon.heat = state->heat;
	weapon->weapon.age = state->age;
	weapon->weapon.overcharged = state->overcharged;
	for (int32 i = 0; i < k_h1_maximum_triggers && i < k_weapon_barrel_count; i++)
	{
		weapon->weapon.barrels[i].rate_of_fire = state->triggers[i].rate_of_fire;
		weapon->weapon.barrels[i].ejection_port_position = state->triggers[i].ejection_port_position;
		weapon->weapon.barrels[i].illumination = state->triggers[i].illumination;
		weapon->weapon.barrels[i].current_error = state->triggers[i].error;
	}
	for (int32 i = 0; i < MAXIMUM_NUMBER_OF_MAGAZINES_PER_WEAPON; i++)
	{
		weapon->weapon.magazines[i].state = _magazine_idle;
		weapon->weapon.magazines[i].state_timer = 0;
		weapon->weapon.magazines[i].reload_timer = 0;
	}
	return;
}

static const h1_weap_triggers* h1_trigger_definition_get(const s_h1_weapon_logic_context* context, int16 trigger_index)
{
	return g_h1_cache_file->block_get(context->definition->triggers, trigger_index);
}

static const h1_weap_magazines* h1_magazine_definition_get(const s_h1_weapon_logic_context* context, int16 magazine_index)
{
	return g_h1_cache_file->block_get(context->definition->magazines, magazine_index);
}

// items.c _item_belongs_to_player_bit: held by a player's unit
static bool h1_weapon_belongs_to_player(const s_h1_weapon_logic_context* context)
{
	const unit_datum* unit = (const unit_datum*)object_try_and_get_and_verify_type(context->weapon->object.parent_object_index, _object_mask_unit);
	return unit && unit->unit.player_index != NONE;
}

// weapons.c weapon_get_owner_object_index (and weapon_get_projectile_owner_object_index, halo 2 has no gunners here)
static datum h1_weapon_owner_object_index(const s_h1_weapon_logic_context* context)
{
	const datum parent_index = context->weapon->object.parent_object_index;
	return parent_index != NONE && object_try_and_get_and_verify_type(parent_index, _object_mask_unit) ? parent_index : NONE;
}

// first_person_weapons.c first_person_weapon_message_from_weapon: kept for the first person weapon
static void h1_first_person_weapon_message(s_h1_weapon_logic_context* context, e_h1_first_person_weapon_message message)
{
	std::vector<s_h1_first_person_weapon_message>& messages = context->state->messages;
	if (messages.size() >= k_h1_maximum_queued_messages)
	{
		messages.erase(messages.begin());
	}
	messages.push_back({ message, context->state->magazines[0].state != _h1_magazine_idle });
	return;
}

// weapons.c weapon_effect_new and weapon_effect_looping_new: a handle for h1_effect_stop
static int32 h1_weapon_effect_new(s_h1_weapon_logic_context* context, const h1_tag_reference* effect, real32 scale, real32 error, bool looping)
{
	if (effect->index == NONE)
	{
		return 0;
	}
	if (effect->group_tag == 'snd!')
	{
		if (!looping)
		{
			// (its world origin: a held weapon's position is its parent's space)
			real_point3d origin;
			object_get_origin(context->weapon_index, &origin, false);
			h1_sound_impulse(effect->index, &origin, scale);
		}
		return 0;
	}
	return h1_effect_new_on_object_ex(effect->index, context->weapon_index, scale, error, looping);
}

// weapons.c weapon_get_first_person_animation_time: a first person animation's frames (or key frame) in halo 1 ticks
static int16 h1_weapon_first_person_animation_time(const s_h1_weapon_logic_context* context, bool key_frame, int16 animation_type, int16 shotgun_reload_type)
{
	const datum graph_index = context->definition->first_person_animations.index;
	const h1_antr* graph = graph_index != NONE ? (const h1_antr*)g_h1_cache_file->tag_get('antr', graph_index) : NULL;
	if (!graph || graph->first_person_weapons.count <= 0)
	{
		return 0;
	}
	const h1_antr_first_person_weapons* weapon_animations = g_h1_cache_file->block_get(graph->first_person_weapons, 0);
	auto animation_get = [&](int16 type) -> const h1_antr_animations*
	{
		if (type < 0 || type >= weapon_animations->animations.count)
		{
			return NULL;
		}
		const int16 animation_index = g_h1_cache_file->block_get(weapon_animations->animations, type)->animation_index;
		return VALID_INDEX(animation_index, graph->animations.count) ? g_h1_cache_file->block_get(graph->animations, animation_index) : NULL;
	};
	const h1_antr_animations* animation = animation_get(animation_type);
	if (!animation)
	{
		return 0;
	}
	int16 time = key_frame ? animation->key_frame_index : animation->frame_count;
	if (!key_frame && context->definition->weapon_type == _h1_weapon_type_shotgun &&
		(shotgun_reload_type == _h1_shotgun_reload_type_first_round || shotgun_reload_type == _h1_shotgun_reload_type_first_and_last_round))
	{
		const h1_antr_animations* enter = animation_get(_h1_first_person_animation_shotgun_enter);
		if (enter)
		{
			time = enter->frame_count;
		}
	}
	return time;
}

static bool h1_weapon_busy(const s_h1_weapon_logic_context* context)
{
	const s_h1_weapon_logic_state* state = context->state;
	return state->triggers[0].state != _h1_trigger_idle || state->triggers[1].state != _h1_trigger_idle ||
		state->magazines[0].state != _h1_magazine_idle || state->magazines[1].state != _h1_magazine_idle || state->state != _h1_weapon_state_idle;
}

static bool h1_weapon_magazine_state_change_ok(const s_h1_weapon_logic_context* context)
{
	const s_h1_weapon_logic_state* state = context->state;
	const int32 triggers = context->definition->triggers.count;
	return (triggers < 1 || state->triggers[0].state == _h1_trigger_idle) && (triggers < 2 || state->triggers[1].state == _h1_trigger_idle) &&
		state->state == _h1_weapon_state_idle;
}

// weapons.c weapon_set_state: the state lasts as long as the weapon's own animation for it
static bool h1_weapon_set_state(s_h1_weapon_logic_context* context, int8 new_state, bool immediate)
{
	s_h1_weapon_logic_state* state = context->state;
	bool interruptable = false;
	if (state->state == _h1_weapon_state_idle)
	{
		interruptable = true;
	}
	else if (state->state > _h1_weapon_state_idle && state->state <= _h1_weapon_state_secondary_recoil)
	{
		interruptable = new_state >= state->state;
	}
	if (!immediate && !interruptable)
	{
		return false;
	}

	const datum graph_index = context->definition->animation_graph.index;
	const h1_antr* graph = graph_index != NONE ? (const h1_antr*)g_h1_cache_file->tag_get('antr', graph_index) : NULL;
	if (graph && graph->weapons.count > 0)
	{
		const h1_antr_weapons* weapon_animations = g_h1_cache_file->block_get(graph->weapons, 0);
		int16 slot = NONE;
		switch (new_state)
		{
		case _h1_weapon_state_idle: slot = 0; break;
		case _h1_weapon_state_primary_recoil: slot = 9; break;
		case _h1_weapon_state_secondary_recoil: slot = 10; break;
		case _h1_weapon_state_primary_chamber: slot = 5; break;
		case _h1_weapon_state_secondary_chamber: slot = 6; break;
		case _h1_weapon_state_primary_reload:
		case _h1_weapon_state_secondary_reload: slot = 3; break;
		case _h1_weapon_state_primary_charged:
		case _h1_weapon_state_secondary_charged: slot = 8; break;
		case _h1_weapon_state_ready: slot = 1; break;
		case _h1_weapon_state_put_away: slot = 2; break;
		}
		if (slot != NONE)
		{
			int16 animation_index = slot < weapon_animations->animations.count ?
				g_h1_cache_file->block_get(weapon_animations->animations, slot)->animation_index : (int16)NONE;
			if (!VALID_INDEX(animation_index, graph->animations.count))
			{
				animation_index = NONE;
			}
			if (animation_index != NONE || new_state == _h1_weapon_state_idle)
			{
				state->animation_index = animation_index;
				state->animation_frame = 0;
				state->state = new_state;
			}
		}
	}
	return true;
}

// weapons.c weapon_state_next
static void h1_weapon_state_next(s_h1_weapon_logic_context* context)
{
	const int8 state = context->state->state;
	if (state < _h1_weapon_state_primary_charged || (state > _h1_weapon_state_secondary_charged && state != _h1_weapon_state_put_away))
	{
		h1_weapon_set_state(context, _h1_weapon_state_idle, true);
	}
	return;
}

// weapons.c weapon_state_key_frame
static void h1_weapon_state_key_frame(s_h1_weapon_logic_context* context)
{
	switch (context->state->state)
	{
	case _h1_weapon_state_primary_chamber:
		h1_weapon_trigger_start_ejection_port(context, 0, true);
		break;
	case _h1_weapon_state_secondary_chamber:
		h1_weapon_trigger_start_ejection_port(context, 1, true);
		break;
	}
	return;
}

// animations.c animation_update_internal of the weapon's animation: a frame per tick, the key frame and the last frame
static void h1_weapon_animation_update(s_h1_weapon_logic_context* context)
{
	s_h1_weapon_logic_state* state = context->state;
	if (state->animation_index == NONE)
	{
		return;
	}
	const h1_antr* graph = (const h1_antr*)g_h1_cache_file->tag_get('antr', context->definition->animation_graph.index);
	if (!graph || !VALID_INDEX(state->animation_index, graph->animations.count))
	{
		state->animation_index = NONE;
		return;
	}
	const h1_antr_animations* animation = g_h1_cache_file->block_get(graph->animations, state->animation_index);
	state->animation_frame++;
	if (state->animation_frame == animation->key_frame_index)
	{
		h1_weapon_state_key_frame(context);
	}
	else if (state->animation_frame + 1 >= animation->frame_count)
	{
		state->animation_frame = 0;
		h1_weapon_state_next(context);
	}
	return;
}

// weapons.c weapon_reset
static void h1_weapon_reset(s_h1_weapon_logic_context* context)
{
	s_h1_weapon_logic_state* state = context->state;
	for (int16 i = 0; i < context->definition->triggers.count && i < k_h1_maximum_triggers; i++)
	{
		state->triggers[i].state = _h1_trigger_uninitialized;
		state->triggers[i].state_timer = 0;
		if (state->triggers[i].charging_effect_id)
		{
			h1_effect_stop(state->triggers[i].charging_effect_id);
			state->triggers[i].charging_effect_id = 0;
		}
	}
	for (int16 i = 0; i < context->definition->magazines.count && i < k_h1_maximum_magazines; i++)
	{
		s_h1_weapon_logic_magazine* magazine = &state->magazines[i];
		if (magazine->state == _h1_magazine_reloading &&
			2 * magazine->state_timer < h1_weapon_first_person_animation_time(context, false, _h1_first_person_animation_reload_while_empty, NONE))
		{
			h1_weapon_magazine_finish_reload(context, i);
		}
		magazine->state = _h1_magazine_idle;
		magazine->state_timer = 0;
	}
	return;
}

// weapons.c weapon_detonate
static void h1_weapon_detonate(s_h1_weapon_logic_context* context)
{
	h1_weapon_effect_new(context, &context->definition->detonation, 0.f, 0.f, false);
	object_delete(context->weapon_index);
	return;
}

static void h1_weapon_magazine_idle(s_h1_weapon_logic_context* context, int16 magazine_index)
{
	context->state->magazines[magazine_index].state = _h1_magazine_idle;
	context->state->magazines[magazine_index].state_timer = 0;
	return;
}

// weapons.c weapon_magazine_start_reload
static void h1_weapon_magazine_start_reload(s_h1_weapon_logic_context* context, int16 magazine_index, bool first)
{
	s_h1_weapon_logic_state* state = context->state;
	s_h1_weapon_logic_magazine* magazine = &state->magazines[magazine_index];
	const weapon_magazine* rounds = &context->weapon->weapon.magazines[magazine_index];
	const h1_weap_magazines* magazine_definition = h1_magazine_definition_get(context, magazine_index);

	if (magazine->state != _h1_magazine_idle && magazine->state != _h1_magazine_unchambered)
	{
		return;
	}
	if (!h1_weapon_magazine_state_change_ok(context))
	{
		return;
	}
	if (rounds->rounds_inventory > 0 && rounds->rounds_loaded < magazine_definition->rounds_loaded_maximum)
	{
		int16 reload_type = NONE;
		h1_weapon_set_state(context, (int8)(_h1_weapon_state_primary_reload + magazine_index), false);
		h1_weapon_effect_new(context, &magazine_definition->reloading_effect, 0.f, 0.f, false);
		h1_first_person_weapon_message(context, rounds->rounds_loaded != 0 ?
			_h1_first_person_weapon_message_reload_while_full : _h1_first_person_weapon_message_reload_while_empty);
		if (context->definition->weapon_type == _h1_weapon_type_shotgun)
		{
			const bool last_round = magazine_definition->rounds_loaded_maximum - rounds->rounds_loaded == 1;
			if (first)
			{
				reload_type = (int16)(last_round ? _h1_shotgun_reload_type_first_and_last_round : _h1_shotgun_reload_type_first_round);
			}
			else
			{
				reload_type = last_round ? (int16)_h1_shotgun_reload_type_last_round : (int16)NONE;
			}
		}
		magazine->state = _h1_magazine_reloading;
		magazine->original_time = magazine->state_timer =
			h1_weapon_first_person_animation_time(context, false, _h1_first_person_animation_reload_while_empty, reload_type);
	}
	SET_BIT(state->flags, _h1_weapon_needs_to_reload_bit, false);
	return;
}

// weapons.c weapon_magazine_finish_reload
static void h1_weapon_magazine_finish_reload(s_h1_weapon_logic_context* context, int16 magazine_index)
{
	s_h1_weapon_logic_magazine* magazine = &context->state->magazines[magazine_index];
	weapon_magazine* rounds = &context->weapon->weapon.magazines[magazine_index];
	const h1_weap_magazines* magazine_definition = h1_magazine_definition_get(context, magazine_index);

	if (TEST_BIT(magazine_definition->flags, _h1_weapon_magazine_wastes_rounds_when_reloaded_bit))
	{
		rounds->rounds_loaded = 0;
	}
	const int16 rounds_to_load = MIN(magazine_definition->rounds_reloaded, rounds->rounds_inventory);
	int16 rounds_loaded = rounds->rounds_loaded + rounds_to_load;
	if (rounds_loaded > magazine_definition->rounds_loaded_maximum)
	{
		rounds_loaded = magazine_definition->rounds_loaded_maximum;
	}
	if (h1_weapon_belongs_to_player(context))
	{
		rounds->rounds_inventory = rounds->rounds_inventory - rounds_loaded + rounds->rounds_loaded;
	}
	rounds->rounds_loaded = rounds_loaded;
	magazine->state = _h1_magazine_unchambered;
	magazine->state_timer = 0;

	// continuous reloads (the shotgun) go on while the triggers and the swap leave the weapon alone
	if (rounds->rounds_inventory > 0 && rounds->rounds_loaded < magazine_definition->rounds_loaded_maximum &&
		!TEST_BIT(magazine_definition->flags, _h1_weapon_magazine_wastes_rounds_when_reloaded_bit) &&
		!(context->state->control_flags & (FLAG(_h1_weapon_control_primary_trigger_bit) | FLAG(_h1_weapon_control_secondary_trigger_bit) |
			FLAG(_h1_weapon_control_user_switching_weapons_bit))))
	{
		h1_weapon_magazine_start_reload(context, magazine_index, false);
	}
	return;
}

// weapons.c weapon_magazine_start_chamber
static void h1_weapon_magazine_start_chamber(s_h1_weapon_logic_context* context, int16 magazine_index)
{
	s_h1_weapon_logic_magazine* magazine = &context->state->magazines[magazine_index];
	if ((magazine->state == _h1_magazine_idle || magazine->state == _h1_magazine_unchambered) && h1_weapon_magazine_state_change_ok(context))
	{
		const h1_weap_magazines* magazine_definition = h1_magazine_definition_get(context, magazine_index);
		h1_weapon_set_state(context, (int8)(_h1_weapon_state_primary_chamber + magazine_index), false);
		h1_weapon_effect_new(context, &magazine_definition->chambering_effect, 0.f, 0.f, false);
		magazine->state = _h1_magazine_chambering;
		magazine->state_timer = (int16)(magazine_definition->chamber_time * k_h1_ticks_per_second);
	}
	return;
}

static void h1_weapon_trigger_change_state(s_h1_weapon_logic_context* context, int16 trigger_index, int8 new_state, int16 new_state_timer)
{
	context->state->triggers[trigger_index].state = new_state;
	context->state->triggers[trigger_index].state_timer = new_state_timer;
	return;
}

static void h1_weapon_trigger_idle(s_h1_weapon_logic_context* context, int16 trigger_index)
{
	h1_weapon_trigger_change_state(context, trigger_index, _h1_trigger_idle, 0);
	return;
}

static void h1_weapon_trigger_recover(s_h1_weapon_logic_context* context, int16 trigger_index)
{
	context->state->triggers[trigger_index].idle_ticks = 0;
	h1_weapon_trigger_idle(context, trigger_index);
	return;
}

// weapons.c weapon_trigger_fully_charged
static void h1_weapon_trigger_fully_charged(s_h1_weapon_logic_context* context, int16 trigger_index)
{
	const h1_weap_triggers* trigger_definition = h1_trigger_definition_get(context, trigger_index);
	h1_weapon_trigger_change_state(context, trigger_index, _h1_trigger_charged, (int16)(trigger_definition->charged_time * k_h1_ticks_per_second));
	h1_weapon_set_state(context, (int8)(_h1_weapon_state_primary_charged + trigger_index), true);
	h1_first_person_weapon_message(context, _h1_first_person_weapon_message_charged);
	return;
}

// weapons.c weapon_trigger_start_ejection_port
static void h1_weapon_trigger_start_ejection_port(s_h1_weapon_logic_context* context, int16 trigger_index, bool chamber)
{
	if (trigger_index >= context->definition->triggers.count)
	{
		return;
	}
	const h1_weap_triggers* trigger_definition = h1_trigger_definition_get(context, trigger_index);
	if (trigger_definition->ejection_port_recovery_time > 0.f &&
		TEST_BIT(trigger_definition->flags, _h1_trigger_definition_ejection_port_during_chamber_animation_bit) == chamber)
	{
		context->state->triggers[trigger_index].ejection_port_position = 1.f;
	}
	return;
}

// weapons.c weapon_trigger_can_fire_again
static bool h1_weapon_trigger_can_fire_again(s_h1_weapon_logic_context* context, int16 trigger_index)
{
	const s_h1_weapon_logic_state* state = context->state;
	const s_h1_weapon_logic_trigger* trigger = &state->triggers[trigger_index];
	const h1_weap_triggers* trigger_definition = h1_trigger_definition_get(context, trigger_index);
	const real32 fraction = TEST_BIT(trigger_definition->flags, _h1_trigger_definition_analog_rate_of_fire_bit) ? state->primary_trigger : trigger->rate_of_fire;
	// the bounds are the initial and the final rate of fire
	const real32 rate_of_fire = (trigger_definition->rounds_per_second.upper - trigger_definition->rounds_per_second.lower) * fraction +
		trigger_definition->rounds_per_second.lower;
	real32 required_ticks = rate_of_fire > 0.0001f ? k_h1_ticks_per_second / rate_of_fire : 0.f;
	if (context->definition->age_rate_of_fire_penalty > 0.f)
	{
		required_ticks = (state->age * context->definition->age_rate_of_fire_penalty + 1.f) * required_ticks;
	}
	bool result = (real32)trigger->idle_ticks + 1.f >= required_ticks;
	if (TEST_BIT(trigger_definition->flags, _h1_trigger_definition_latched_bit) && h1_weapon_belongs_to_player(context) &&
		!TEST_BIT(trigger->flags, _h1_weapon_trigger_released_since_last_shot_bit))
	{
		result = false;
	}
	return result;
}

// weapons.c weapon_trigger_begin_firing
static void h1_weapon_trigger_begin_firing(s_h1_weapon_logic_context* context, int16 trigger_index, bool force)
{
	s_h1_weapon_logic_state* state = context->state;
	s_h1_weapon_logic_trigger* trigger = &state->triggers[trigger_index];
	const h1_weap* definition = context->definition;
	const h1_weap_triggers* trigger_definition = h1_trigger_definition_get(context, trigger_index);

	bool can_fire = true;
	if (VALID_INDEX(trigger_definition->magazine_index, MIN(definition->magazines.count, k_h1_maximum_magazines)) &&
		state->magazines[trigger_definition->magazine_index].state != _h1_magazine_idle)
	{
		can_fire = false;
	}
	if (TEST_BIT(state->flags, _h1_weapon_overheated_bit))
	{
		can_fire = false;
	}
	if (!can_fire)
	{
		return;
	}

	if (!force && trigger_definition->charging_time > 0.f)
	{
		if (TEST_BIT(definition->flags_3, _h1_weapon_definition_cannot_fire_at_maximum_age_bit) && state->age >= 1.f)
		{
			h1_weapon_trigger_fire(context, trigger_index);
		}
		else
		{
			if (definition->triggers.count > 1)
			{
				trigger->charging_effect_id = h1_weapon_effect_new(context, &trigger_definition->charging_effect, 0.f, 0.f, true);
			}
			else if (trigger->rate_of_fire > 0.f)
			{
				SET_BIT(trigger->flags, _h1_weapon_trigger_fired_before_charging_bit, true);
				h1_weapon_trigger_fire(context, trigger_index);
			}
			else
			{
				SET_BIT(trigger->flags, _h1_weapon_trigger_fired_before_charging_bit, false);
			}
			h1_weapon_trigger_change_state(context, trigger_index, _h1_trigger_charging, (int16)(trigger_definition->charging_time * k_h1_ticks_per_second));
		}
	}
	else if (!force && trigger_definition->overload_time > 0.f)
	{
		h1_weapon_trigger_change_state(context, trigger_index, _h1_trigger_overloading, (int16)(trigger_definition->overload_time * k_h1_ticks_per_second));
	}
	else
	{
		h1_weapon_trigger_fire(context, trigger_index);
	}
	return;
}

// weapons.c weapon_trigger_fire
static void h1_weapon_trigger_fire(s_h1_weapon_logic_context* context, int16 trigger_index)
{
	s_h1_weapon_logic_state* state = context->state;
	s_h1_weapon_logic_trigger* trigger = &state->triggers[trigger_index];
	const h1_weap* definition = context->definition;
	const h1_weap_triggers* trigger_definition = h1_trigger_definition_get(context, trigger_index);
	const bool belongs_to_player = h1_weapon_belongs_to_player(context);
	const h1_tag_reference* effect = NULL;
	const h1_tag_reference* damage_effect = NULL;
	real32 effect_scale = 0.f;
	real32 effect_error = 0.f;
	bool fired = false;
	bool misfired = false;

	const bool loads_alternate_ammunition = trigger_index == 1 &&
		(definition->secondary_trigger_mode == _h1_weapon_secondary_trigger_loads_alternate_ammunition ||
		definition->secondary_trigger_mode == _h1_weapon_secondary_trigger_loads_multiple_primary_ammunition);

	if (VALID_INDEX(trigger_definition->magazine_index, MIN(definition->magazines.count, k_h1_maximum_magazines)))
	{
		const h1_weap_magazines* magazine_definition = h1_magazine_definition_get(context, trigger_definition->magazine_index);
		weapon_magazine* rounds = &context->weapon->weapon.magazines[trigger_definition->magazine_index];
		if (!loads_alternate_ammunition || state->alternate_shots_loaded < definition->maximum_alternate_shots_loaded)
		{
			if ((rounds->rounds_loaded >= trigger_definition->rounds_per_shot ||
				TEST_BIT(trigger_definition->flags, _h1_trigger_definition_can_fire_with_partial_ammunition_bit)) &&
				!(TEST_BIT(definition->flags_3, _h1_weapon_definition_cannot_fire_at_maximum_age_bit) && state->age >= 1.f) &&
				(rounds->rounds_loaded >= trigger_definition->minimum_rounds_loaded ||
				!TEST_BIT(trigger->flags, _h1_weapon_trigger_released_since_last_shot_bit)))
			{
				if ((rounds->rounds_loaded -= trigger_definition->rounds_per_shot) <= 0)
				{
					rounds->rounds_loaded = 0;
				}
				else if (TEST_BIT(magazine_definition->flags, _h1_weapon_magazine_must_be_chambered_every_shot_bit))
				{
					state->magazines[trigger_definition->magazine_index].state = _h1_magazine_unchambered;
					state->magazines[trigger_definition->magazine_index].state_timer = 0;
				}
				fired = true;
			}
		}
	}
	else
	{
		fired = true;
	}

	if (trigger_definition->firing_effects.count > 0)
	{
		const int16 firing_effect_count = (int16)MIN(trigger_definition->firing_effects.count, 16);
		if (trigger->firing_effect_shots_remaining <= 0)
		{
			const int16 starting_firing_effect_index = trigger->firing_effect_index;
			int16 firing_effect_index = TEST_BIT(trigger_definition->flags, _h1_trigger_definition_random_firing_effects_bit) ?
				(int16)h1_weapon_random_range(0, firing_effect_count) : starting_firing_effect_index;
			do
			{
				if (trigger->firing_effects_used_flags == (uint16)(FLAG(firing_effect_count) - 1))
				{
					trigger->firing_effects_used_flags = 0;
				}
				do
				{
					firing_effect_index++;
					if (firing_effect_index >= firing_effect_count)
					{
						firing_effect_index = 0;
					}
				} while (TEST_BIT(trigger->firing_effects_used_flags, firing_effect_index));

				const h1_weap_triggers_firing_effects* next_firing_effect = g_h1_cache_file->block_get(trigger_definition->firing_effects, firing_effect_index);
				trigger->firing_effect_index = firing_effect_index;
				SET_BIT(trigger->firing_effects_used_flags, firing_effect_index, true);
				trigger->firing_effect_shots_remaining = (int16)h1_weapon_random_range(next_firing_effect->shot_count_lower_bound,
					next_firing_effect->shot_count_upper_bound + 1);
			} while (trigger->firing_effect_shots_remaining <= 0 && firing_effect_index != starting_firing_effect_index);
		}

		trigger->firing_effect_shots_remaining--;
		const h1_weap_triggers_firing_effects* firing_effect = g_h1_cache_file->block_get(trigger_definition->firing_effects, trigger->firing_effect_index);

		if (definition->age_misfire_start > 0.f && definition->age_misfire_start < 1.f && state->age > definition->age_misfire_start)
		{
			real32 misfire_chance = ((state->age - definition->age_misfire_start) * definition->age_misfire_chance) / (1.f - definition->age_misfire_start);
			if (trigger->state == _h1_trigger_spewing)
			{
				misfire_chance *= 2.f;
			}
			if (h1_weapon_random_real() < misfire_chance)
			{
				misfired = true;
			}
		}

		if (!fired)
		{
			effect = &firing_effect->empty_effect;
			damage_effect = &firing_effect->empty_damage;
			effect_scale = 1.f;
		}
		else if (misfired)
		{
			effect = &firing_effect->misfire_effect;
			damage_effect = &firing_effect->misfire_damage;
			effect_scale = trigger->rate_of_fire;
		}
		else
		{
			effect = &firing_effect->firing_effect;
			damage_effect = &firing_effect->firing_damage;
			effect_scale = trigger->rate_of_fire;
			effect_error = definition->overheated_threshold == 0.f ? 0.f : state->heat / definition->overheated_threshold;
		}
	}

	if (fired)
	{
		context->weapon->weapon.game_time_last_fired = (int32)game_time_get();
		h1_first_person_weapon_message(context, misfired ?
			(trigger_index ? _h1_first_person_weapon_message_secondary_misfire : _h1_first_person_weapon_message_primary_misfire) :
			(trigger_index ? _h1_first_person_weapon_message_secondary_fire : _h1_first_person_weapon_message_primary_fire));
		h1_weapon_trigger_start_ejection_port(context, trigger_index, false);
		if (trigger_definition->illumination_recovery_time > 0.f)
		{
			trigger->illumination = 1.f;
		}
		state->heat += trigger_definition->heat_generated_per_round;
		if (!belongs_to_player && state->heat > definition->overheated_threshold)
		{
			state->heat = definition->overheated_threshold;
		}
		else if (state->heat > 1.f)
		{
			state->heat = 1.f;
		}
		if (belongs_to_player)
		{
			state->age += trigger_definition->age_generated_per_round;
			if (state->age > 1.f)
			{
				state->age = 1.f;
			}
		}

		h1_weapon_set_state(context, (int8)(trigger_index ? _h1_weapon_state_secondary_recoil : _h1_weapon_state_primary_recoil), false);

		if (!misfired)
		{
			if (loads_alternate_ammunition)
			{
				state->alternate_shots_loaded++;
			}
			else
			{
				h1_weapon_trigger_create_projectiles(context, trigger_index);
			}
		}

		// the firing damage on the owner: its camera impulse and shaking (the recoil)
		const datum owner_object_index = h1_weapon_owner_object_index(context);
		if (owner_object_index != NONE && damage_effect && damage_effect->index != NONE)
		{
			h1_weapon_damage_owner(owner_object_index, damage_effect->index);
		}

		if (definition->weapon_type == _h1_weapon_type_plasma_pistol && trigger_index == 1)
		{
			SET_BIT(state->flags, _h1_weapon_overheat_recoil_bit, true);
		}
	}

	if (state->heat > definition->heat_detonation_threshold && h1_weapon_random_real() < definition->heat_detonation_fraction)
	{
		h1_weapon_detonate(context);
		return;
	}

	if (!fired)
	{
		h1_weapon_trigger_change_state(context, trigger_index, _h1_trigger_locked, NONE);
	}
	else if (trigger->state != _h1_trigger_spewing || misfired)
	{
		if (TEST_BIT(trigger_definition->flags, _h1_trigger_definition_tracks_projectile_bit))
		{
			h1_weapon_trigger_change_state(context, trigger_index, _h1_trigger_tracking, NONE);
		}
		else
		{
			h1_weapon_trigger_recover(context, trigger_index);
		}
	}

	SET_BIT(trigger->flags, _h1_weapon_trigger_released_since_last_shot_bit, false);
	if (effect)
	{
		h1_weapon_effect_new(context, effect, effect_scale, effect_error, false);
	}
	return;
}

// weapons.c weapon_trigger_overload
static void h1_weapon_trigger_overload(s_h1_weapon_logic_context* context, int16 trigger_index)
{
	const h1_weap_triggers* trigger_definition = h1_trigger_definition_get(context, trigger_index);
	if (trigger_index + 1 < context->definition->triggers.count)
	{
		h1_weapon_trigger_fire(context, trigger_index + 1);
	}
	h1_weapon_trigger_change_state(context, trigger_index, _h1_trigger_overloading, (int16)(trigger_definition->overload_time * k_h1_ticks_per_second));
	return;
}

// weapons.c weapon_trigger_release_charge
static void h1_weapon_trigger_release_charge(s_h1_weapon_logic_context* context, int16 trigger_index)
{
	const h1_weap_triggers* trigger_definition = h1_trigger_definition_get(context, trigger_index);
	if (trigger_definition->spew_time > 0.f)
	{
		h1_weapon_trigger_change_state(context, trigger_index, _h1_trigger_spewing, (int16)(trigger_definition->spew_time * k_h1_ticks_per_second));
	}
	else
	{
		if (context->definition->triggers.count > 1)
		{
			h1_weapon_trigger_fire(context, 1);
		}
		h1_weapon_trigger_recover(context, trigger_index);
	}
	context->state->triggers[trigger_index].rate_of_fire = 0.f;
	return;
}

// weapons.c weapon_trigger_overcharged
static void h1_weapon_trigger_overcharged(s_h1_weapon_logic_context* context, int16 trigger_index)
{
	switch (h1_trigger_definition_get(context, trigger_index)->overcharged_action)
	{
	case _h1_trigger_overcharged_explodes:
		h1_weapon_detonate(context);
		break;
	case _h1_trigger_overcharged_fire:
		h1_weapon_trigger_release_charge(context, trigger_index);
		break;
	}
	return;
}

// weapons.c trigger_create_projectiles: halo 2 makes the projectile objects (built from the halo 1 projectiles)
static void h1_weapon_trigger_create_projectiles(s_h1_weapon_logic_context* context, int16 trigger_index)
{

	weapon_datum* weapon = context->weapon;
	s_h1_weapon_logic_state* state = context->state;
	s_h1_weapon_logic_trigger* trigger = &state->triggers[trigger_index];
	const h1_weap* definition = context->definition;
	const h1_weap_triggers* trigger_definition = h1_trigger_definition_get(context, trigger_index);
	const datum owner_object_index = h1_weapon_owner_object_index(context);
	const unit_datum* unit = owner_object_index != NONE ? (const unit_datum*)object_try_and_get_and_verify_type(owner_object_index, _object_mask_unit) : NULL;

	object_marker markers[k_h1_maximum_trigger_markers];
	int16 marker_count = object_get_markers_by_string_id(context->weapon_index, string_id_find_or_add(trigger_index == 0 ? "primary_trigger" : "secondary_trigger"),
		markers, k_h1_maximum_trigger_markers);
	if (marker_count <= 0)
	{
		markers[0].matrix.position = weapon->object.position;
		markers[0].matrix.vectors.forward = weapon->object.forward;
		marker_count = 1;
	}
	if (!TEST_BIT(trigger_definition->flags, _h1_trigger_definition_uses_weapon_origin_bit))
	{
		marker_count = 1;
	}

	for (int16 marker_index = 0; marker_index < marker_count; marker_index++)
	{
		real_point3d origin = markers[marker_index].matrix.position;
		real_vector3d forward = markers[marker_index].matrix.vectors.forward;
		real32 velocity = 0.f;
		real32 error = 0.f;
		datum target_object_index = NONE;

		if (!TEST_BIT(trigger_definition->flags, _h1_trigger_definition_projectiles_cannot_be_aimed_bit) && unit)
		{
			// units.c unit_adjust_projectile_ray: the aiming vector, the origin moved onto the camera's line (players fire from the camera)
			const bool player = unit->unit.player_index != NONE;
			forward = unit->unit.aiming_vector;
			if (player)
			{
				real_point3d camera_position;
				unit_get_camera_position(owner_object_index, &camera_position);
				const real_vector3d relative = { origin.x - camera_position.x, origin.y - camera_position.y, origin.z - camera_position.z };
				const real32 projection = relative.i * forward.i + relative.j * forward.j + relative.k * forward.k;
				origin = { camera_position.x + forward.i * projection, camera_position.y + forward.j * projection, camera_position.z + forward.k * projection };
			}
			real_vector3d object_velocity;
			object_get_velocities(owner_object_index, &object_velocity, NULL);
			velocity = forward.i * object_velocity.i + forward.j * object_velocity.j + forward.k * object_velocity.k;

			if (player)
			{
				real_vector3d right;
				real_vector3d up;
				const real_vector3d global_up = { 0.f, 0.f, 1.f };
				cross_product3d(&global_up, &forward, &right);
				if (normalize3d(&right) == 0.f)
				{
					right = { 0.f, 1.f, 0.f };
				}
				cross_product3d(&forward, &right, &up);
				normalize3d(&up);
				const real_point3d* offset = &trigger_definition->first_person_offset;
				origin.x += forward.i * offset->x + right.i * offset->y + up.i * offset->z;
				origin.y += forward.j * offset->x + right.j * offset->y + up.j * offset->z;
				origin.z += forward.k * offset->x + right.k * offset->y + up.k * offset->z;

				// aim_assist.c player_aim_projectile: halo 2's aim assist found the target this tick
				target_object_index = unit->unit.target_info.target_object;
				if (target_object_index != NONE)
				{
					const object_datum* target = (const object_datum*)object_try_and_get(target_object_index);
					if (!target)
					{
						target_object_index = NONE;
					}
				}
			}
		}

		if (TEST_BIT(trigger_definition->flags, _h1_trigger_definition_uses_weapon_origin_bit))
		{
			origin = markers[marker_index].matrix.position;
		}

		datum h1_projectile_index;
		int16 projectile_count;
		if (trigger_index == 0 && state->alternate_shots_loaded > 0 && definition->triggers.count > 1)
		{
			const h1_weap_triggers* secondary_trigger_definition = h1_trigger_definition_get(context, 1);
			int16 alternate_shots_loaded = state->alternate_shots_loaded;
			h1_projectile_index = secondary_trigger_definition->projectile.index;
			if (definition->secondary_trigger_mode == _h1_weapon_secondary_trigger_loads_multiple_primary_ammunition)
			{
				alternate_shots_loaded++;
			}
			projectile_count = trigger_definition->projectiles_per_shot * alternate_shots_loaded;
			state->alternate_shots_loaded = 0;
		}
		else
		{
			h1_projectile_index = trigger_definition->projectile.index;
			projectile_count = trigger_definition->projectiles_per_shot;
		}

		const datum projectile_definition_index = h1_projectile_index != NONE ? h1_projectile_definition_build(h1_projectile_index) : NONE;
		if (projectile_definition_index == NONE)
		{
			continue;
		}

		s_damage_owner damage_owner;
		damage_owner.owner_player_index = unit ? unit->unit.player_index : NONE;
		damage_owner.owner_object_index = owner_object_index;
		damage_owner.owner_team_index = unit ? unit->unit.unit_team : (e_game_team)NONE;
		damage_owner.pad = 0;

		real_vector3d first_projectile_forward = forward;
		for (int16 projectile_index = 0; projectile_index < projectile_count; projectile_index++)
		{
			object_placement_data data;
			object_placement_data_new(&data, projectile_definition_index, owner_object_index, &damage_owner);
			data.position = origin;
			data.forward = forward;

			bool tracer = false;
			if (trigger->rate_of_fire == 0.f || trigger->sequential_non_tracer_rounds++ >= trigger_definition->rounds_between_tracers)
			{
				tracer = true;
				trigger->sequential_non_tracer_rounds = 0;
			}

			if (error == 0.f)
			{
				const real32 fraction = TEST_BIT(trigger_definition->flags, _h1_trigger_definition_analog_rate_of_fire_bit) ? state->primary_trigger : trigger->error;
				error = (1.f - fraction) * trigger_definition->error_angle.lower + fraction * trigger_definition->error_angle.upper;
			}
			if (!TEST_BIT(trigger_definition->flags, _h1_trigger_definition_use_error_when_unzoomed_bit) ||
				!TEST_BIT(state->control_flags, _h1_weapon_control_zoomed_bit))
			{
				h1_random_vector_in_cone3d(&data.forward, trigger_definition->minimum_error, error, &data.forward);
			}
			if (projectile_index == 0)
			{
				first_projectile_forward = data.forward;
			}
			if (TEST_BIT(trigger_definition->flags, _h1_trigger_definition_projectiles_have_identical_error_bit))
			{
				data.forward = first_projectile_forward;
			}

			perpendicular3d(&data.forward, &data.up);
			normalize3d(&data.up);
			h1_projectile_distribute(&data.forward, &data.up, trigger_definition->distribution_function, trigger_definition->distribution_angle,
				projectile_index, projectile_count);
			data.translational_velocity = { data.forward.i * velocity, data.forward.j * velocity, data.forward.k * velocity };

			const datum projectile_object_index = object_new(&data);
			if (projectile_object_index == NONE)
			{
				continue;
			}
			// projectiles.c projectile_new: halo 1's projectile logic flies it (halo 2's velocities are a second's)
			h1_projectile_logic_new(projectile_object_index, velocity / k_h1_ticks_per_second, target_object_index, tracer);
		}
	}
	return;
}

// cseries random_math.c real_random and random_range
static real32 h1_weapon_random_real(void)
{
	g_h1_weapon_random_seed = g_h1_weapon_random_seed * 1664525u + 1013904223u;
	return (real32)(g_h1_weapon_random_seed >> 8) / (real32)(1u << 24);
}

static int32 h1_weapon_random_range(int32 lower, int32 upper)
{
	if (upper <= lower)
	{
		return lower;
	}
	return lower + (int32)(h1_weapon_random_real() * (real32)(upper - lower)) % (upper - lower);
}

// random_math.c seed_random_vector_in_cone3d: the axis turned about a random perpendicular by an angle between the cones
static void h1_random_vector_in_cone3d(const real_vector3d* axis, real32 inner_cone_angle, real32 outer_cone_angle, real_vector3d* result)
{
	const real_vector3d axis_copy = *axis;
	real_vector3d random_direction;
	real32 length;
	do
	{
		random_direction = { h1_weapon_random_real() * 2.f - 1.f, h1_weapon_random_real() * 2.f - 1.f, h1_weapon_random_real() * 2.f - 1.f };
		length = random_direction.i * random_direction.i + random_direction.j * random_direction.j + random_direction.k * random_direction.k;
	} while (length > 1.f || length < 0.0001f);
	*result = axis_copy;
	real_vector3d rotation_axis;
	cross_product3d(&axis_copy, &random_direction, &rotation_axis);
	if (normalize3d(&rotation_axis) > 0.f)
	{
		const real32 angle = inner_cone_angle + (outer_cone_angle - inner_cone_angle) * h1_weapon_random_real();
		rotate_vector_about_axis(result, &rotation_axis, sinf(angle), cosf(angle));
	}
	return;
}

// weapons.c projectile_distribute
static void h1_projectile_distribute(real_vector3d* forward, const real_vector3d* up, int16 distribution_function, real32 distribution_angle,
	int16 projectile_index, int16 projectile_count)
{
	real32 offset;
	if (projectile_count & 1)
	{
		if (projectile_index == 0)
		{
			offset = 0.f;
		}
		else
		{
			int16 step = projectile_index - 1;
			step = (step & 1) ? (int16)(step >> 1) : (int16)-(step >> 1);
			offset = step;
		}
	}
	else
	{
		offset = (projectile_index >> 1) - 0.5f;
		if (projectile_index & 1)
		{
			offset = -offset;
		}
	}
	const real32 angle = offset * distribution_angle;
	if (distribution_function == _h1_trigger_distribution_horizontal_fan)
	{
		rotate_vector_about_axis(forward, up, sinf(angle), cosf(angle));
	}
	return;
}

// weapons.c weapon_trigger_fire: damage_data_new with the firing damage, from the weapon, back along the aiming vector at the
// owner's center (halo 2's damage_data_new FUN_00575bac and damage owner FUN_00575c14)
static void h1_weapon_damage_owner(datum owner_object_index, datum h1_damage_effect_index)
{
	typedef void(__cdecl* t_damage_data_new)(s_damage_data* damage, datum definition_index);
	typedef void(__cdecl* t_damage_owner_from_object)(datum object_index, s_damage_owner* owner);

	const datum definition_index = h1_damage_effect_build(h1_damage_effect_index);
	const unit_datum* unit = (const unit_datum*)object_try_and_get_and_verify_type(owner_object_index, _object_mask_unit);
	if (definition_index == NONE || !unit)
	{
		return;
	}
	s_damage_data damage;
	Memory::GetAddress<t_damage_data_new>(0x175BAC)(&damage, definition_index);
	damage.flags = (e_damage_data_flags)(damage.flags | FLAG(_damage_from_weapon_bit));
	Memory::GetAddress<t_damage_owner_from_object>(0x175C14)(owner_object_index, &damage.owner);
	damage.direction = { -unit->unit.aiming_vector.i, -unit->unit.aiming_vector.j, -unit->unit.aiming_vector.k };
	damage.epicenter = unit->object.center;
	damage.origin = damage.epicenter;
	object_cause_damage(&damage, owner_object_index, NONE, NONE, NONE, NULL);
	return;
}

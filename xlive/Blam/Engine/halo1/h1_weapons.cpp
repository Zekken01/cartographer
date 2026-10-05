#include "stdafx.h"
#include "h1_weapons.h"

#include "h1_cache_file.h"
#include "h1_animations.h"
#include "h1_effects.h"
#include "h1_first_person.h"
#include "math/matrix_math.h"
#include "h1_items.h"
#include "h1_log.h"
#include "h1_map_loader.h"
#include "h1_object_tags.h"
#include "h1_objects.h"
#include "h1_projectiles.h"
#include "h1_runtime.h"
#include "h1_sound.h"
#include "h2_tag_definitions_generated.h"

#include "cache/cache_files.h"
#include "items/weapons.h"
#include "objects/objects.h"
#include "tag_files/tag_groups.h"

#include <unordered_map>

/* constants */

enum
{
	k_h2_object_type_weapon = 2,
	k_h1_maximum_weapon_triggers = 2,
};

// weapon_definitions.h halo 1 trigger flags
enum
{
	_h1_trigger_tracks_fired_projectile_bit = 0,
	_h1_trigger_random_firing_effects_bit,
	_h1_trigger_can_fire_with_partial_ammo_bit,
	_h1_trigger_does_not_repeat_automatically_bit,
	_h1_trigger_locks_in_on_off_state_bit,
	_h1_trigger_projectiles_use_weapon_origin_bit,
	_h1_trigger_sticks_when_dropped_bit,
	_h1_trigger_ejects_during_chamber_bit,
	_h1_trigger_discharging_spews_bit,
	_h1_trigger_analog_rate_of_fire_bit,
	_h1_trigger_use_error_when_unzoomed_bit,
	_h1_trigger_projectile_vector_cannot_be_adjusted_bit,
	_h1_trigger_projectiles_have_identical_error_bit,
};

// halo 2 trigger behaviors and actions
enum
{
	_h2_trigger_behavior_spew = 0,
	_h2_trigger_behavior_latch,
	_h2_trigger_behavior_latch_autofire,
	_h2_trigger_behavior_charge,
};
enum
{
	_h2_trigger_action_fire = 0,
	_h2_trigger_action_charge,
};

static const real32 k_h1_ticks_per_second = 30.f;

struct s_h1_weapon_class
{
	const char* h1_label;
	const char* h2_class;	// the halo 2 biped animations held weapons use
	const char* h2_name;
};

// halo 1 weapon labels and the halo 2 animation class and weapon of the halo 2 bipeds
static const s_h1_weapon_class k_h1_weapon_classes[] =
{
	{ "ar", "rifle", "smg" },
	{ "hp", "pistol", "hp" },
	{ "pp", "pistol", "pp" },
	{ "ne", "pistol", "ne" },
	{ "pr", "rifle", "pr" },
	{ "sg", "rifle", "sg" },
	{ "sr", "rifle", "sr" },
	{ "rl", "missile", "rl" },
	{ "ft", "support", "bs" },
	{ "b", "ball", "b" },
	{ "f", "flag", "f" },
};

struct s_h1_multiplayer_weapon
{
	const char* h2_name_part;	// the halo 2 weapon of the multiplayer weapon list
	const char* h1_label;		// becomes the halo 1 weapon with this label
};

// first match wins
static const s_h1_multiplayer_weapon k_h1_multiplayer_weapons[] =
{
	{ "brute_plasma_rifle", "pr" },
	{ "plasma_pistol", "pp" },
	{ "plasma_rifle", "pr" },
	{ "magnum", "hp" },
	{ "smg", "ar" },
	{ "battle_rifle", "hp" },
	{ "shotgun", "sg" },
	{ "sniper_rifle", "sr" },
	{ "beam_rifle", "sr" },
	{ "covenant_carbine", "pr" },
	{ "rocket_launcher", "rl" },
	{ "needler", "ne" },
	{ "brute_shot", "ft" },
};

/* structures */

struct s_h1_weapon_state
{
	uint16 fire_counts[k_h1_maximum_weapon_triggers];
	bool empty_clicked[k_h1_maximum_weapon_triggers];
	int16 magazine_state;
	bool seen;
};

/* globals */

static std::unordered_map<datum, datum> g_h1_weapons;				// halo 2 weapon: halo 1 weapon
static std::unordered_map<datum, s_h1_weapon_state> g_h1_weapon_states;	// by object

/* prototypes */

static void h1_reference_none(tag_reference* reference);
static datum h1_damage_effect_reference(const h1_tag_reference* reference);
static const s_h1_weapon_class* h1_weapon_class_get(const char* label);
static void h1_weapon_effect_at_marker(datum object_index, const char* marker_name, const h1_tag_reference* effect);

/* public code */

void h1_weapons_reset(void)
{
	g_h1_weapons.clear();
	g_h1_weapon_states.clear();
	return;
}

datum h1_weapon_h1_get(datum h2_weapon_index)
{
	auto found = g_h1_weapons.find(h2_weapon_index);
	return found != g_h1_weapons.end() ? found->second : NONE;
}

datum h1_weapon_definition_build(datum h1_weapon_index)
{
	const char* h1_name = g_h1_cache_file->tag_name_get(h1_weapon_index);
	char name[256];
	sprintf_s(name, "halo1\\%s", h1_name);
	const datum existing = h1_runtime_tag_find('weap', name);
	if (existing != NONE)
	{
		return existing;
	}

	const h1_weap* h1_weapon = (const h1_weap*)g_h1_cache_file->tag_get('weap', h1_weapon_index);
	const h1_mode* h1_model = h1_weapon ? (const h1_mode*)g_h1_cache_file->tag_get('mode', h1_weapon->model.index) : NULL;
	const h1_coll* h1_collision = h1_weapon ? (const h1_coll*)g_h1_cache_file->tag_get('coll', h1_weapon->collision_model.index) : NULL;
	if (!h1_model || h1_weapon->triggers.count <= 0)
	{
		h1_log("weapons: %s is missing its model or triggers", h1_name);
		return NONE;
	}

	// every trigger's projectile, built first so a weapon without them isn't made
	datum projectiles[k_h1_maximum_weapon_triggers] = { NONE, NONE };
	const int32 trigger_count = MIN(h1_weapon->triggers.count, (int32)k_h1_maximum_weapon_triggers);
	for (int32 i = 0; i < trigger_count; i++)
	{
		const h1_weap_triggers* trigger = g_h1_cache_file->block_get(h1_weapon->triggers, i);
		projectiles[i] = trigger->projectile.index != NONE ? h1_projectile_definition_build(trigger->projectile.index) : NONE;
	}

	s_h1_object_tags tags;
	tags.render_model = h1_object_render_model_build(h1_model, name);
	tags.collision_model = h1_collision ? h1_object_collision_model_build(h1_collision, h1_model, name) : NONE;
	tags.physics_model = NONE;
	tags.animation_graph = NONE;
	tags.disappear_distance = 80.f;
	const datum model_index = tags.render_model != NONE ? h1_object_model_build(&tags, h1_model, h1_collision, name) : NONE;
	if (model_index == NONE)
	{
		return NONE;
	}

	h2x_weap* weapon = NULL;
	const datum weapon_index = h1_runtime_tag_new('weap', name, &weapon);
	if (weapon_index == NONE)
	{
		return NONE;
	}
	tag_reference* references[] =
	{
		&weapon->model, &weapon->crate_object, &weapon->modifier_shader, &weapon->creation_effect, &weapon->material_effects, &weapon->unused,
		&weapon->collision_sound, &weapon->detonation_damage_effect, &weapon->detonating_effect, &weapon->detonation_effect, &weapon->ready_effect,
		&weapon->ready_damage_effect, &weapon->overheated, &weapon->overheated_damage_effect, &weapon->detonation, &weapon->detonation_damage_effect_2,
		&weapon->player_melee_damage, &weapon->player_melee_response, &weapon->f_1st_hit_melee_damage, &weapon->f_1st_hit_melee_response,
		&weapon->f_2nd_hit_melee_damage, &weapon->f_2nd_hit_melee_response, &weapon->f_3rd_hit_melee_damage, &weapon->f_3rd_hit_melee_response,
		&weapon->lunge_melee_damage, &weapon->lunge_melee_response, &weapon->weapon_power_on_effect, &weapon->weapon_power_off_effect,
		&weapon->pickup_sound, &weapon->zoom_in_sound, &weapon->zoom_out_sound, &weapon->new_hud_interface, &weapon->deployed_vehicle,
		&weapon->age_effect, &weapon->aged_weapon,
	};
	for (tag_reference* reference : references)
	{
		h1_reference_none(reference);
	}

	// object
	weapon->object_type = k_h2_object_type_weapon;
	weapon->bounding_radius = h1_weapon->bounding_radius;
	weapon->bounding_offset = h1_weapon->bounding_offset;
	weapon->acceleration_scale = h1_weapon->acceleration_scale;
	weapon->default_model_variant = _string_id_default;
	h1_runtime_reference_set(&weapon->model, 'hlmt', model_index);
	weapon->apply_collision_damage_scale = 1.f;
	weapon->game_acceleration = { 2.5f, 4.5f };
	weapon->game_scale = { 0.2f, 1.25f };
	weapon->absolute_acceleration = { 2.5f, 10.f };
	weapon->absolute_scale = { 0.2f, 1.25f };
	weapon->hud_text_message_index = h1_weapon->hud_text_message_index;

	// item
	weapon->flags_2 = h1_weapon->flags_2;
	weapon->old_message_index = h1_weapon->message_index;
	weapon->sort_order = h1_weapon->sort_order;
	weapon->multiplayer_on_ground_scale = h1_weapon->scale > 0.f ? h1_weapon->scale : 1.f;
	weapon->campaign_on_ground_scale = weapon->multiplayer_on_ground_scale;
	weapon->detonation_delay = h1_weapon->detonation_delay;

	// weapon
	weapon->secondary_trigger_mode = h1_weapon->secondary_trigger_mode;
	weapon->maximum_alternate_shots_loaded = h1_weapon->maximum_alternate_shots_loaded;
	weapon->ready_time = h1_weapon->ready_time;
	weapon->heat_recovery_threshold = h1_weapon->heat_recovery_threshold;
	weapon->overheated_threshold = h1_weapon->overheated_threshold;
	weapon->heat_detonation_threshold = h1_weapon->heat_detonation_threshold;
	weapon->heat_detonation_fraction = h1_weapon->heat_detonation_fraction;
	weapon->heat_loss_per_second = h1_weapon->heat_loss_per_second;
	weapon->heat_illumination = h1_weapon->heat_illumination;
	weapon->overheated_heat_loss_per_second = h1_weapon->heat_loss_per_second;
	const datum melee_damage = h1_damage_effect_reference(&h1_weapon->player_melee_damage);
	const datum melee_response = h1_damage_effect_reference(&h1_weapon->player_melee_response);
	if (melee_damage != NONE)
	{
		tag_reference* damages[] = { &weapon->player_melee_damage, &weapon->f_1st_hit_melee_damage, &weapon->f_2nd_hit_melee_damage, &weapon->f_3rd_hit_melee_damage, &weapon->lunge_melee_damage };
		for (tag_reference* reference : damages)
		{
			h1_runtime_reference_set(reference, 'jpt!', melee_damage);
		}
	}
	if (melee_response != NONE)
	{
		tag_reference* responses[] = { &weapon->player_melee_response, &weapon->f_1st_hit_melee_response, &weapon->f_2nd_hit_melee_response, &weapon->f_3rd_hit_melee_response, &weapon->lunge_melee_response };
		for (tag_reference* reference : responses)
		{
			h1_runtime_reference_set(reference, 'jpt!', melee_response);
		}
	}
	// the melee target volume of the halo 2 weapons
	weapon->damage_pyramid_angles = { 0.21838213f, 0.10579618f };
	weapon->damage_pyramid_depth = 0.6f;
	weapon->magnification_levels = h1_weapon->magnification_levels;
	weapon->magnification_range = h1_weapon->magnification_range;
	weapon->autoaim_angle = h1_weapon->autoaim_angle;
	weapon->autoaim_range = h1_weapon->autoaim_range;
	weapon->magnetism_angle = h1_weapon->magnetism_angle;
	weapon->magnetism_range = h1_weapon->magnetism_range;
	weapon->magnetism_angle_2 = h1_weapon->magnetism_angle;
	weapon->magnetism_range_2 = h1_weapon->magnetism_range;
	weapon->deviation_angle = h1_weapon->deviation_angle;
	weapon->movement_penalized = h1_weapon->movement_penalized;
	weapon->forward_movement_penalty = h1_weapon->forward_movement_penalty;
	weapon->sideways_movement_penalty = h1_weapon->sideways_movement_penalty;
	weapon->weapon_power_on_time = h1_weapon->light_power_on_time;
	weapon->weapon_power_off_time = h1_weapon->light_power_off_time;
	weapon->weapon_power_on_velocity = h1_weapon->light_power_on_time > 0.f ? 1.f / h1_weapon->light_power_on_time : 1.f;
	weapon->weapon_power_off_velocity = h1_weapon->light_power_off_time > 0.f ? 1.f / h1_weapon->light_power_off_time : 1.f;
	weapon->age_heat_recovery_penalty = h1_weapon->age_heat_recovery_penalty;
	weapon->age_rate_of_fire_penalty = h1_weapon->age_rate_of_fire_penalty;
	weapon->age_misfire_start = h1_weapon->age_misfire_start;
	weapon->age_misfire_chance = h1_weapon->age_misfire_chance;
	weapon->active_camo_ding = h1_weapon->active_camo_ding;
	weapon->active_camo_regrowth_rate = h1_weapon->active_camo_regrowth_rate;
	weapon->weapon_type = h1_weapon->weapon_type;
	const s_h1_weapon_class* weapon_class = h1_weapon_class_get(h1_weapon->label);
	weapon->weapon_class = string_id_find_or_add(weapon_class ? weapon_class->h2_class : "rifle");
	weapon->weapon_name = string_id_find_or_add(weapon_class ? weapon_class->h2_name : h1_weapon->label);
	weapon->multiplayer_weapon_type = weapon_class && strcmp(weapon_class->h1_label, "f") == 0 ? 1 : weapon_class && strcmp(weapon_class->h1_label, "b") == 0 ? 2 : 0;

	// magazines
	// energy weapons only have empty magazines, halo 2's have none
	int32 magazine_count = 0;
	for (int32 i = 0; i < h1_weapon->magazines.count; i++)
	{
		if (g_h1_cache_file->block_get(h1_weapon->magazines, i)->rounds_loaded_maximum > 0)
		{
			magazine_count = h1_weapon->magazines.count;
		}
	}
	h2x_weap_magazines* magazines = h1_runtime_block_new(&weapon->magazines, magazine_count);
	for (int32 i = 0; i < magazine_count; i++)
	{
		const h1_weap_magazines* h1_magazine = g_h1_cache_file->block_get(h1_weapon->magazines, i);
		h2x_weap_magazines* magazine = &magazines[i];
		magazine->flags = h1_magazine->flags;
		magazine->rounds_recharged = h1_magazine->rounds_recharged;
		magazine->rounds_total_initial = h1_magazine->rounds_total_initial;
		magazine->rounds_total_maximum = h1_magazine->rounds_total_maximum;
		magazine->rounds_loaded_maximum = h1_magazine->rounds_loaded_maximum;
		// halo 1 keeps one total, halo 2 counts the rounds in reserve
		magazine->rounds_inventory_maximum = (int16)MAX(h1_magazine->rounds_total_maximum - h1_magazine->rounds_loaded_maximum, 0);
		magazine->reload_time = h1_magazine->reload_time;
		magazine->rounds_reloaded = h1_magazine->rounds_reloaded;
		magazine->chamber_time = h1_magazine->chamber_time;
		h1_reference_none(&magazine->reloading_effect);
		h1_reference_none(&magazine->reloading_damage_effect);
		h1_reference_none(&magazine->chambering_effect);
		h1_reference_none(&magazine->chambering_damage_effect);
		const int32 equipment_count = h1_magazine->magazines.count;
		h2x_weap_magazines_magazine_equipment* equipment = h1_runtime_block_new(&magazine->magazine_equipment, equipment_count);
		for (int32 j = 0; j < equipment_count; j++)
		{
			const h1_weap_magazines_magazines* h1_equipment = g_h1_cache_file->block_get(h1_magazine->magazines, j);
			equipment[j].rounds = h1_equipment->rounds;
			const datum built = h1_equipment->equipment.index != NONE ? h1_equipment_definition_build(h1_equipment->equipment.index) : NONE;
			h1_runtime_reference_set(&equipment[j].equipment, built != NONE ? 'eqip' : (tag_group)NONE, built);
		}
	}

	// halo 1 triggers are halo 2 barrels; a charging first trigger fires the second when charged (the plasma pistol)
	const h1_weap_triggers* first_trigger = g_h1_cache_file->block_get(h1_weapon->triggers, 0);
	const bool charging = first_trigger->charging_time > 0.f && trigger_count > 1;
	const int32 h2_trigger_count = charging ? 1 : trigger_count;
	h2x_weap_new_triggers* triggers = h1_runtime_block_new(&weapon->new_triggers, h2_trigger_count);
	for (int32 i = 0; i < h2_trigger_count; i++)
	{
		const h1_weap_triggers* h1_trigger = g_h1_cache_file->block_get(h1_weapon->triggers, i);
		h2x_weap_new_triggers* trigger = &triggers[i];
		trigger->input = (int16)i;
		trigger->primary_barrel_index = (int16)i;
		trigger->secondary_barrel_index = NONE;
		trigger->behavior = TEST_BIT(h1_trigger->flags, _h1_trigger_does_not_repeat_automatically_bit) ? (int16)_h2_trigger_behavior_latch : (int16)_h2_trigger_behavior_spew;
		trigger->prediction = 1;
		if (charging)
		{
			trigger->behavior = (int16)_h2_trigger_behavior_latch_autofire;
			trigger->secondary_barrel_index = 1;
			trigger->autofire_time = 0.06667f;
			trigger->autofire_throw = 0.5f;
			trigger->secondary_action = (int16)_h2_trigger_action_charge;
			trigger->prediction = 2;
		}
		trigger->charging_time = h1_trigger->charging_time;
		trigger->charged_time = h1_trigger->charged_time;
		trigger->overcharged_action = h1_trigger->overcharged_action;
		trigger->charged_illumination = h1_trigger->charged_illumination;
		trigger->spew_time = h1_trigger->spew_time;
		h1_reference_none(&trigger->charging_effect);
		h1_reference_none(&trigger->charging_damage_effect);
	}

	const datum stub_effect = h1_effects_stub_effect_get();
	h2x_weap_barrels* barrels = h1_runtime_block_new(&weapon->barrels, trigger_count);
	for (int32 i = 0; i < trigger_count; i++)
	{
		const h1_weap_triggers* h1_trigger = g_h1_cache_file->block_get(h1_weapon->triggers, i);
		h2x_weap_barrels* barrel = &barrels[i];

		// tracks fired projectile, random firing effects, can fire with partial ammo, then the flags that moved
		uint32 flags = h1_trigger->flags & 7;
		if (TEST_BIT(h1_trigger->flags, _h1_trigger_projectiles_use_weapon_origin_bit)) flags |= FLAG(3);
		if (TEST_BIT(h1_trigger->flags, _h1_trigger_ejects_during_chamber_bit)) flags |= FLAG(4);
		if (TEST_BIT(h1_trigger->flags, _h1_trigger_use_error_when_unzoomed_bit)) flags |= FLAG(5);
		if (TEST_BIT(h1_trigger->flags, _h1_trigger_projectile_vector_cannot_be_adjusted_bit)) flags |= FLAG(6);
		if (TEST_BIT(h1_trigger->flags, _h1_trigger_projectiles_have_identical_error_bit)) flags |= FLAG(7);
		barrel->flags = flags;
		barrel->rounds_per_second = h1_trigger->rounds_per_second;
		barrel->acceleration_time = h1_trigger->acceleration_time;
		barrel->deceleration_time = h1_trigger->deceleration_time;
		barrel->barrel_spin_scale = 1.f;
		barrel->blurred_rate_of_fire = h1_trigger->blurred_rate_of_fire;
		const bool semi_automatic = h1_trigger->rounds_per_second.upper <= 0.f;
		barrel->shots_per_fire = semi_automatic ? short_bounds{ 1, 1 } : short_bounds{ 0, 0 };
		barrel->fire_recovery_time = semi_automatic ? (i == 0 ? 0.05f : 0.1f) : 0.f;
		barrel->soft_recovery_fraction = semi_automatic && i == 0 ? 1.f : 0.f;
		// energy weapons (the plasma pistol) keep an empty halo 1 magazine that holds no rounds: no magazine for halo 2
		const h1_weap_magazines* trigger_magazine = VALID_INDEX(h1_trigger->magazine_index, h1_weapon->magazines.count) ?
			g_h1_cache_file->block_get(h1_weapon->magazines, h1_trigger->magazine_index) : NULL;
		barrel->magazine_index = trigger_magazine && trigger_magazine->rounds_loaded_maximum > 0 ? h1_trigger->magazine_index : (int16)NONE;
		barrel->rounds_per_shot = h1_trigger->rounds_per_shot;
		barrel->minimum_rounds_loaded = h1_trigger->minimum_rounds_loaded;
		barrel->rounds_between_tracers = h1_trigger->rounds_between_tracers;
		barrel->optional_barrel_marker_name = _string_id_empty_string;
		barrel->prediction_type = h1_trigger->rounds_per_second.upper > 0.f && !TEST_BIT(h1_trigger->flags, _h1_trigger_does_not_repeat_automatically_bit) ? 1 : 2;
		barrel->firing_noise = h1_trigger->firing_noise;
		barrel->acceleration_time_2 = h1_trigger->acceleration_time_2;
		barrel->deceleration_time_2 = h1_trigger->deceleration_time_2;
		barrel->damage_error = h1_trigger->error;
		barrel->acceleration_time_3 = h1_trigger->acceleration_time_2;
		barrel->deceleration_time_3 = h1_trigger->deceleration_time_2;
		// halo 1 keeps rates per tick, halo 2 per second
		barrel->runtime_dual_error_acceleration_rate = h1_trigger->error_acceleration_rate * k_h1_ticks_per_second;
		barrel->runtime_dual_error_deceleration_rate = h1_trigger->error_deceleration_rate * k_h1_ticks_per_second;
		barrel->minimum_error = h1_trigger->minimum_error;
		barrel->error_angle = h1_trigger->error_angle;
		barrel->dual_wield_damage_scale = 1.f;
		barrel->distribution_function = h1_trigger->distribution_function;
		barrel->projectiles_per_shot = h1_trigger->projectiles_per_shot;
		barrel->distribution_angle = h1_trigger->distribution_angle;
		barrel->minimum_error_2 = h1_trigger->minimum_error;
		barrel->error_angle_2 = h1_trigger->error_angle;
		barrel->first_person_offset = h1_trigger->first_person_offset;
		h1_runtime_reference_set(&barrel->projectile, projectiles[i] != NONE ? 'proj' : (tag_group)NONE, projectiles[i]);
		h1_reference_none(&barrel->damage_effect);
		barrel->ejection_port_recovery_time = h1_trigger->ejection_port_recovery_time;
		barrel->illumination_recovery_time = h1_trigger->illumination_recovery_time;
		barrel->heat_generated_per_round = h1_trigger->heat_generated_per_round;
		barrel->age_generated_per_round = h1_trigger->age_generated_per_round;
		barrel->overload_time = h1_trigger->overload_time;
		barrel->illumination_recovery_rate = h1_trigger->illumination_recovery_rate * k_h1_ticks_per_second;
		barrel->ejection_port_recovery_rate = h1_trigger->ejection_port_recovery_rate * k_h1_ticks_per_second;
		barrel->rate_of_fire_acceleration_rate = h1_trigger->rate_of_fire_acceleration_rate * k_h1_ticks_per_second;
		barrel->rate_of_fire_deceleration_rate = h1_trigger->rate_of_fire_deceleration_rate * k_h1_ticks_per_second;
		barrel->error_acceleration_rate = h1_trigger->error_acceleration_rate * k_h1_ticks_per_second;
		barrel->error_deceleration_rate = h1_trigger->error_deceleration_rate * k_h1_ticks_per_second;

		// the halo 1 firing effects play in h1_weapons_update, halo 2 gets the stub and the halo 1 firing damage (camera shake)
		const int32 effect_count = h1_trigger->firing_effects.count;
		h2x_weap_barrels_firing_effects* effects = h1_runtime_block_new(&barrel->firing_effects, effect_count);
		for (int32 j = 0; j < effect_count; j++)
		{
			const h1_weap_triggers_firing_effects* h1_effect = g_h1_cache_file->block_get(h1_trigger->firing_effects, j);
			h2x_weap_barrels_firing_effects* effect = &effects[j];
			effect->shot_count_lower_bound = h1_effect->shot_count_lower_bound;
			effect->shot_count_upper_bound = h1_effect->shot_count_upper_bound;
			h1_runtime_reference_set(&effect->firing_effect, h1_effect->firing_effect.index != NONE && stub_effect != NONE ? 'effe' : (tag_group)NONE, h1_effect->firing_effect.index != NONE ? stub_effect : NONE);
			h1_runtime_reference_set(&effect->misfire_effect, h1_effect->misfire_effect.index != NONE && stub_effect != NONE ? 'effe' : (tag_group)NONE, h1_effect->misfire_effect.index != NONE ? stub_effect : NONE);
			h1_runtime_reference_set(&effect->empty_effect, h1_effect->empty_effect.index != NONE && stub_effect != NONE ? 'effe' : (tag_group)NONE, h1_effect->empty_effect.index != NONE ? stub_effect : NONE);
			const datum firing_damage = h1_damage_effect_reference(&h1_effect->firing_damage);
			const datum misfire_damage = h1_damage_effect_reference(&h1_effect->misfire_damage);
			const datum empty_damage = h1_damage_effect_reference(&h1_effect->empty_damage);
			h1_runtime_reference_set(&effect->firing_damage, firing_damage != NONE ? 'jpt!' : (tag_group)NONE, firing_damage);
			h1_runtime_reference_set(&effect->misfire_damage, misfire_damage != NONE ? 'jpt!' : (tag_group)NONE, misfire_damage);
			h1_runtime_reference_set(&effect->empty_damage, empty_damage != NONE ? 'jpt!' : (tag_group)NONE, empty_damage);
		}
	}

	// first person: the halo 1 first person model's nodes and markers, the halo 1 first person animations (every character)
	const h1_mode* h1_first_person_model = h1_weapon->first_person_model.index != NONE ? (const h1_mode*)g_h1_cache_file->tag_get('mode', h1_weapon->first_person_model.index) : NULL;
	if (h1_first_person_model)
	{
		char first_person_name[256];
		sprintf_s(first_person_name, "halo1\\%s", g_h1_cache_file->tag_name_get(h1_weapon->first_person_model.index));
		datum first_person_model = h1_runtime_tag_find('mode', first_person_name);
		if (first_person_model == NONE)
		{
			first_person_model = h1_object_render_model_build(h1_first_person_model, first_person_name);
		}
		char animations_name[256];
		sprintf_s(animations_name, "halo1\\%s", h1_weapon->first_person_animations.index != NONE ? g_h1_cache_file->tag_name_get(h1_weapon->first_person_animations.index) : first_person_name);
		const datum first_person_animations = h1_first_person_animation_graph_build(h1_weapon->first_person_animations.index, animations_name);
		if (first_person_model != NONE && first_person_animations != NONE)
		{
			h1_first_person_model_register(first_person_model, h1_weapon->first_person_model.index);
			h2x_weap_first_person* first_person = h1_runtime_block_new(&weapon->first_person, 2);
			for (int32 i = 0; i < 2; i++)
			{
				h1_runtime_reference_set(&first_person[i].first_person_model, 'mode', first_person_model);
				h1_runtime_reference_set(&first_person[i].first_person_animations, 'jmad', first_person_animations);
			}
		}
	}

	g_h1_weapons[weapon_index] = h1_weapon_index;
	h1_objects_bind(weapon_index, h1_weapon_index);
	h1_log("weapons: built %s (%s, %d triggers%s)", name, h1_weapon->label, trigger_count, charging ? ", charging" : "");
	return weapon_index;
}

void h1_weapons_build_multiplayer(void)
{
	const datum multiplayer_globals_index = h1_runtime_tag_find('mulg', "multiplayer\\multiplayer_globals");
	h2x_mulg* multiplayer_globals = multiplayer_globals_index != NONE ? (h2x_mulg*)tag_get('mulg', multiplayer_globals_index) : NULL;
	if (!multiplayer_globals || multiplayer_globals->runtime.count <= 0)
	{
		h1_log("weapons: no multiplayer globals");
		return;
	}
	h2x_mulg_runtime* runtime = multiplayer_globals->runtime[0];

	// the halo 1 weapons of the map by label
	std::unordered_map<std::string, datum> weapons_by_label;
	for (int32 i = 0; i < g_h1_cache_file->tag_count(); i++)
	{
		const h1_cache_file_tag_instance* instance = g_h1_cache_file->tag_instance_get_by_absolute_index(i);
		if (!instance || instance->group_tag != 'weap')
		{
			continue;
		}
		const h1_weap* h1_weapon = (const h1_weap*)g_h1_cache_file->tag_get('weap', instance->tag_index);
		if (h1_weapon && h1_weapon->label[0] && weapons_by_label.find(h1_weapon->label) == weapons_by_label.end())
		{
			weapons_by_label[h1_weapon->label] = instance->tag_index;
		}
	}

	for (int32 i = 0; i < runtime->weapons.count; i++)
	{
		h2x_mulg_runtime_weapons* entry = runtime->weapons[i];
		if (entry->weapon.index == NONE)
		{
			continue;
		}
		const char* h2_name = tag_get_name(entry->weapon.index);
		for (const s_h1_multiplayer_weapon& mapping : k_h1_multiplayer_weapons)
		{
			if (!h2_name || !strstr(h2_name, mapping.h2_name_part))
			{
				continue;
			}
			auto found = weapons_by_label.find(mapping.h1_label);
			const datum built = found != weapons_by_label.end() ? h1_weapon_definition_build(found->second) : NONE;
			if (built != NONE)
			{
				h1_runtime_reference_set(&entry->weapon, 'weap', built);
				h1_log("weapons: multiplayer weapon %d (%s) is %s", i, h2_name, g_h1_cache_file->tag_name_get(found->second));
			}
			break;
		}
	}
	return;
}

// weapons.c weapon firing effects: a halo 1 firing effect at the trigger's marker for every barrel shot halo 2 fires
void h1_weapons_update(void)
{
	if (!h1_maps_active() || !g_h1_cache_file || g_h1_weapons.empty())
	{
		return;
	}
	for (auto& entry : g_h1_weapon_states)
	{
		entry.second.seen = false;
	}

	object_iterator iterator;
	object_iterator_new(&iterator, _object_mask_weapon, 0);
	while (weapon_datum* weapon = (weapon_datum*)object_iterator_next(&iterator))
	{
		const datum h1_weapon_index = h1_weapon_h1_get(weapon->definition_index);
		const h1_weap* h1_weapon = h1_weapon_index != NONE ? (const h1_weap*)g_h1_cache_file->tag_get('weap', h1_weapon_index) : NULL;
		if (!h1_weapon)
		{
			continue;
		}
		const datum object_index = iterator.index;
		auto found = g_h1_weapon_states.find(object_index);
		if (found == g_h1_weapon_states.end())
		{
			s_h1_weapon_state created = {};
			for (int32 i = 0; i < k_h1_maximum_weapon_triggers; i++)
			{
				created.fire_counts[i] = weapon->weapon.barrels[i].fire_count;
			}
			created.magazine_state = (int16)weapon->weapon.magazines[0].state;
			found = g_h1_weapon_states.insert({ object_index, created }).first;
		}
		s_h1_weapon_state* state = &found->second;
		state->seen = true;

		for (int32 i = 0; i < k_h1_maximum_weapon_triggers && i < h1_weapon->triggers.count; i++)
		{
			const h1_weap_triggers* trigger = g_h1_cache_file->block_get(h1_weapon->triggers, i);
			const char* marker = i == 0 ? "primary trigger" : "secondary trigger";
			const weapon_barrel* barrel = &weapon->weapon.barrels[i];
			const h1_weap_triggers_firing_effects* firing_effect = trigger->firing_effects.count > 0 ?
				g_h1_cache_file->block_get(trigger->firing_effects, MIN((int32)barrel->firing_effect_index, trigger->firing_effects.count - 1) < 0 ? 0 :
					MIN((int32)barrel->firing_effect_index, trigger->firing_effects.count - 1)) : NULL;
			if (barrel->fire_count != state->fire_counts[i])
			{
				if (firing_effect)
				{
					h1_weapon_effect_at_marker(object_index, marker, &firing_effect->firing_effect);
				}
				state->fire_counts[i] = barrel->fire_count;
			}
			const bool empty_clicked = barrel->flags.test(_weapon_barrel_did_empty_click_bit);
			if (empty_clicked && !state->empty_clicked[i] && firing_effect)
			{
				h1_weapon_effect_at_marker(object_index, marker, &firing_effect->empty_effect);
			}
			state->empty_clicked[i] = empty_clicked;
		}

		// the reloading effect as a reload starts
		const int16 magazine_state = (int16)weapon->weapon.magazines[0].state;
		if (magazine_state != state->magazine_state && magazine_state != _magazine_idle && h1_weapon->magazines.count > 0)
		{
			const h1_weap_magazines* magazine = g_h1_cache_file->block_get(h1_weapon->magazines, 0);
			if (magazine_state == _magazine_chambering)
			{
				h1_weapon_effect_at_marker(object_index, "", &magazine->chambering_effect);
			}
			else if (state->magazine_state == _magazine_idle || state->magazine_state == _magazine_unchambered)
			{
				h1_weapon_effect_at_marker(object_index, "", &magazine->reloading_effect);
			}
		}
		state->magazine_state = magazine_state;
	}

	for (auto it = g_h1_weapon_states.begin(); it != g_h1_weapon_states.end();)
	{
		it = it->second.seen ? std::next(it) : g_h1_weapon_states.erase(it);
	}
	return;
}

/* private code */

static void h1_reference_none(tag_reference* reference)
{
	h1_runtime_reference_set(reference, (tag_group)NONE, NONE);
	return;
}

static datum h1_damage_effect_reference(const h1_tag_reference* reference)
{
	return reference->index != NONE && reference->group_tag == 'jpt!' ? h1_damage_effect_build(reference->index) : NONE;
}

static const s_h1_weapon_class* h1_weapon_class_get(const char* label)
{
	for (const s_h1_weapon_class& weapon_class : k_h1_weapon_classes)
	{
		if (_stricmp(weapon_class.h1_label, label) == 0)
		{
			return &weapon_class;
		}
	}
	return NULL;
}

// a halo 1 effect (or sound) at the first marker of the name, the object's origin when it has none
static void h1_weapon_effect_at_marker(datum object_index, const char* marker_name, const h1_tag_reference* effect)
{
	if (effect->index == NONE)
	{
		return;
	}
	const object_datum* object = (const object_datum*)object_try_and_get(object_index);
	if (!object)
	{
		return;
	}
	real_point3d point = object->object.position;
	real_vector3d forward = object->object.forward;
	real_matrix4x3 first_person_marker = {};
	if (marker_name && marker_name[0] && h1_first_person_marker_get(object_index, marker_name, &first_person_marker))
	{
		point = first_person_marker.position;
		forward = first_person_marker.vectors.forward;
	}
	else if (marker_name && marker_name[0])
	{
		char name[32];
		strncpy_s(name, marker_name, _TRUNCATE);
		for (char* c = name; *c; c++)
		{
			if (*c == ' ')
			{
				*c = '_';
			}
		}
		object_marker marker;
		if (object_get_markers_by_string_id(object_index, string_id_find_or_add(name), &marker, 1) > 0)
		{
			point = marker.matrix.position;
			forward = marker.matrix.vectors.forward;
		}
	}
	if (effect->group_tag == 'snd!')
	{
		h1_sound_impulse(effect->index, &point, 1.f);
	}
	else if (effect->group_tag == 'effe')
	{
		h1_effect_new_unattached(effect->index, &point, &forward);
	}
	return;
}

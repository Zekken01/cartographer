#include "stdafx.h"
#include "damage.h"

#include "game/game.h"
#include "game/game_options.h"
#include "networking/network_event.h"
#include "units/units.h"

#include "shell/shell.h"
#include "halo1/h1_map_loader.h"
#include "halo1/ce_ai/h1_ai.h"

/* typedef */
typedef void(__cdecl* t_object_apply_damage_aftermath)(datum object_index, s_damage_aftermath_data* damage_data);
t_object_apply_damage_aftermath p_object_apply_damage_aftermath;

/* public code */

void damage_apply_patches(void)
{
	// Hook on call to prevent guardian glitching
	// Used to disable it in Infection only, became a bigger problem so now we have to disable it globally.....
	PatchCall(Memory::GetAddress(0x147DB8, 0x172D55), object_cause_damage);

	DETOUR_ATTACH(p_object_apply_damage_aftermath, Memory::GetAddress<t_object_apply_damage_aftermath>(0x17A25D, 0x151ABD), object_apply_damage_aftermath);
	return;
}

void damage_initialize_for_new_map(void)
{
	// TODO: implement debug functionality
	return;
}

void __cdecl object_cause_damage(s_damage_data* damage_data, datum object_index, int16 node_index, int16 region_index, int16 material_index, real_vector3d* object_normal)
{
	bool prevent_guardian_glitch = false;

	// Only run the patch if we're in multiplayer and friendly fire is off
	if (game_is_multiplayer() && !game_options_get()->game_variant.game_engine_flags.test(_game_engine_friendly_fire_bit))
	{
		// Obtain the actor index so we can determine whether or not to disable damage
		// If object is a unit then we can grab the actor index
		const unit_datum* unit = unit_try_and_get(damage_data->owner.owner_object_index);
		const datum actor_index = unit != NULL ? unit->unit.actor_index : NONE;

		// Disable damage if all are true:
		// 1. Not coming from an actor (actor index is none)
		// 2. Does not have a valid player index or team
		if (actor_index == NONE && (damage_data->owner.owner_player_index == NONE || damage_data->owner.owner_team_index == _game_team_observer))
		{
			event(_event_verbose, "objects:damage: GUARDIAN GLITCH PREVENTED");
			prevent_guardian_glitch = true;
		}
	}

	if (!prevent_guardian_glitch)
	{
		// halo 1 maps: the AI hears of its units' damage (vitality before and after)
		const object_datum* object = h1_maps_active() ? (const object_datum*)object_try_and_get_and_verify_type(object_index, _object_mask_unit) : NULL;
		const real32 body_vitality = object ? object->object.body_vitality : 0.f;
		const real32 shield_vitality = object ? object->object.shield_vitality : 0.f;
		INVOKE(0x17AD81, 0x1525E1, object_cause_damage, damage_data, object_index, node_index, region_index, material_index, object_normal);
		if (object)
		{
			h1_ai_object_damaged(object_index, damage_data->owner.owner_object_index, &damage_data->direction, body_vitality, shield_vitality);
		}
	}
	return;
}

void __cdecl object_apply_damage_aftermath(datum object_index, s_damage_aftermath_data* aftermath_data)
{
	const s_game_variant* variant = get_game_variant();

	if (game_is_multiplayer() && variant && variant->cartographer_settings.flags.test(_cartographer_variant_explosion_physics))
	{
		SET_BIT(aftermath_data->flags, _damage_aftermath_has_explosion_physics, true);
	}

	p_object_apply_damage_aftermath(object_index, aftermath_data);
}
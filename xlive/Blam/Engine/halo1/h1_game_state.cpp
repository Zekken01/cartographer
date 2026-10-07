#include "stdafx.h"
#include "h1_game_state.h"

#include "h1_log.h"
#include "h1_map_loader.h"

#include <vector>

/* globals */

static std::vector<c_h1_game_state_entry*>& h1_game_state_entries(void)
{
	static std::vector<c_h1_game_state_entry*> entries;
	return entries;
}

// the checkpoint slots halo 1's game state has been saved into this map
static bool g_h1_game_state_saved[2] = { false, false };
// reverted, the scripts not having run a tick since
static bool g_h1_game_state_reverted = false;

/* public code */

c_h1_game_state_entry::c_h1_game_state_entry(void)
{
	h1_game_state_entries().push_back(this);
}

void h1_game_state_reset(void)
{
	g_h1_game_state_saved[0] = false;
	g_h1_game_state_saved[1] = false;
	g_h1_game_state_reverted = false;
	return;
}

bool h1_game_state_reverted(void)
{
	return g_h1_game_state_reverted;
}

void h1_game_state_scripts_ticked(void)
{
	g_h1_game_state_reverted = false;
	return;
}

/* private code */

// game_state_save (FUN_0042ffb1) writing halo 2's game state into its slot (halo 2's two alternating checkpoints): halo 1's into
// the same one
static void __cdecl h1_game_state_save_hook(int32 slot_argument)
{
	Memory::GetAddress<void(__cdecl*)(int32)>(0x8BA63)(slot_argument);
	if (h1_maps_active())
	{
		const int32 slot = (int16)slot_argument & 1;
		for (c_h1_game_state_entry* entry : h1_game_state_entries())
		{
			entry->save(slot);
		}
		g_h1_game_state_saved[slot] = true;
		h1_log("game state: saved checkpoint %d", slot);
	}
	return;
}

// game_state_revert (FUN_004305da) loading halo 2's game state from its slot: halo 1's from the same one
static bool __cdecl h1_game_state_load_hook(int32 slot)
{
	const bool loaded = Memory::GetAddress<bool(__cdecl*)(int32)>(0x8BAE2)(slot);
	h1_log("game state: loading checkpoint %d (loaded %d, halo 1's saved %d)", slot & 1, (int32)loaded, (int32)g_h1_game_state_saved[slot & 1]);
	if (loaded && h1_maps_active() && g_h1_game_state_saved[slot & 1])
	{
		for (c_h1_game_state_entry* entry : h1_game_state_entries())
		{
			entry->restore(slot & 1);
		}
		for (c_h1_game_state_entry* entry : h1_game_state_entries())
		{
			entry->restored();
		}
		g_h1_game_state_reverted = true;
		h1_log("game state: reverted to checkpoint %d", slot & 1);
	}
	return loaded;
}

void h1_game_state_apply_patches(void)
{
	PatchCall(Memory::GetAddress(0x2FFF2), h1_game_state_save_hook);
	PatchCall(Memory::GetAddress(0x306E5), h1_game_state_load_hook);
	return;
}

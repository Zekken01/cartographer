#pragma once

#include <type_traits>

/*
* game_state.c's checkpoints on halo 1 maps: halo 1's own game state (the scripts, the AI, the devices, the recordings, ... kept
* outside halo 2's game state) saved with each of halo 2's checkpoint saves and restored when halo 2 reverts to one (the
* cinematics' skip, dying, game_revert), as halo 1's game_state_revert restores its whole game state.
*/

// a part of halo 1's game state: saved into a checkpoint slot, restored from one
class c_h1_game_state_entry
{
public:
	c_h1_game_state_entry(void);
	virtual void save(int32 slot) = 0;
	virtual void restore(int32 slot) = 0;
	// after every entry is restored (fixups that need the others' state)
	virtual void restored(void) {}
};

// a variable of halo 1's game state, saved and restored by copying it
template<typename t_type>
class c_h1_game_state_variable : public c_h1_game_state_entry
{
public:
	c_h1_game_state_variable(t_type* variable) : m_variable(variable) {}
	void save(int32 slot) override { copy(m_saved[slot], *m_variable); }
	void restore(int32 slot) override { copy(*m_variable, m_saved[slot]); }

private:
	template<typename t_value>
	static void copy(t_value& destination, const t_value& source)
	{
		if constexpr (std::is_array_v<t_value>)
		{
			for (size_t i = 0; i < std::extent_v<t_value>; i++)
			{
				copy(destination[i], source[i]);
			}
		}
		else
		{
			destination = source;
		}
	}

	t_type* m_variable;
	t_type m_saved[2];
};

// a callback part of halo 1's game state
class c_h1_game_state_procedures : public c_h1_game_state_entry
{
public:
	typedef void (*t_slot_procedure)(int32 slot);
	typedef void (*t_procedure)(void);
	c_h1_game_state_procedures(t_slot_procedure save, t_slot_procedure restore, t_procedure restored = NULL) :
		m_save(save), m_restore(restore), m_restored(restored) {}
	void save(int32 slot) override { m_save(slot); }
	void restore(int32 slot) override { m_restore(slot); }
	void restored(void) override { if (m_restored) m_restored(); }

private:
	t_slot_procedure m_save;
	t_slot_procedure m_restore;
	t_procedure m_restored;
};

// the variable is halo 1 game state (at file scope, after it)
#define H1_GAME_STATE_VARIABLE(variable) static c_h1_game_state_variable<decltype(variable)> variable##_game_state(&variable)

// no checkpoint of a new map's halo 1 game state yet
void h1_game_state_reset(void);

// game_reverted: halo 1's scripts' first tick after a revert (halo 2's flag is only its own tick of the revert, before the
// scripts' next one runs); the scripts end their tick
bool h1_game_state_reverted(void);
void h1_game_state_scripts_ticked(void);

// halo 2's checkpoint save and revert call halo 1's
void h1_game_state_apply_patches(void);

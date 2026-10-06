#pragma once

/*
* Halo 1's AI on halo 1 maps (the port of halo 1's ai/ in this folder, namespace h1_ai): game.c's calls into it.
*/

// game_initialize_for_new_map: a halo 1 campaign game starts (before its objects are placed)
void h1_ai_initialize_for_new_map(void);
void h1_ai_dispose_from_old_map(void);

// game_initialize_for_new_map: after the scenario's objects are placed, the encounters created at the start
void h1_ai_place(void);

// game_tick: halo 1's AI tick (30 a second) when halo 2's game time reaches one
void h1_ai_update(void);

bool h1_ai_running(void);

// the scripts' ai_* functions (h1_hs_functions)
void h1_ai_script_place(int32 ai_index);

// hs.c's ai script functions (h1_ai_hs_functions.cpp, tools/gen_ai_hs.py): h1_hs procedures by name
struct s_h1_ai_hs_function
{
	const char* name;
	int32 (*procedure)(const int32* arguments);
};
extern const s_h1_ai_hs_function g_h1_ai_hs_functions[];
extern const int32 g_h1_ai_hs_function_count;

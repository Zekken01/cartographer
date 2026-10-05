#pragma once

/* prototypes */

void main_apply_patches(void);

bool __cdecl cinematic_sound_sync_complete(void);

void main_halt_and_display_errors(void);

void main_loop(void);

void main_quit(void);
void main_reset_map(void);
void main_revert_map(void);
void main_revert_map_scripting(void);
void main_save_and_exit(void);

void __cdecl main_switch_structure_bsp(void);
// switch_bsp: the main loop switches to the structure bsp at its next iteration
void main_switch_structure_bsp_request(int16 bsp_index);

void __cdecl main_loop_pregame(int32 a1, int32 a2);

void __cdecl main_reset_map_immediate(void);
void main_save_and_exit_campaign_immediately(void);

void main_load_core();
void main_load_core_name(const char* name);

void main_save_core();
void main_save_core_name(const char* name);

void main_loop_body(void);

void __cdecl main_loop_body_server(void);

void main_status(const char* status_type, const char* format, ...);

void main_halt_and_catch_fire(void);

void main_crash(const char* str);
void main_print_version(void);

/* globals */

extern bool debug_no_drawing;
extern bool debug_console_pauses_game;
extern bool force_crash_uploads;

#pragma once

/*
* cseries.h and the debug, profile and console pieces of halo 1's engine the AI port calls, after its math types.
*/

namespace h1_ai
{

/* ---------- colors (cseries.c) */

extern const real_argb_color* global_real_argb_white;
extern const real_argb_color* global_real_argb_grey;
extern const real_argb_color* global_real_argb_black;
extern const real_argb_color* global_real_argb_red;
extern const real_argb_color* global_real_argb_green;
extern const real_argb_color* global_real_argb_blue;
extern const real_argb_color* global_real_argb_cyan;
extern const real_argb_color* global_real_argb_yellow;
extern const real_argb_color* global_real_argb_magenta;
extern const real_argb_color* global_real_argb_pink;
extern const real_argb_color* global_real_argb_lightblue;
extern const real_argb_color* global_real_argb_orange;
extern const real_argb_color* global_real_argb_purple;
extern const real_argb_color* global_real_argb_aqua;
extern const real_argb_color* global_real_argb_darkgreen;
extern const real_argb_color* global_real_argb_salmon;
extern const real_argb_color* global_real_argb_violet;

extern const real_rgb_color* global_real_rgb_white;
extern const real_rgb_color* global_real_rgb_grey;
extern const real_rgb_color* global_real_rgb_black;
extern const real_rgb_color* global_real_rgb_red;
extern const real_rgb_color* global_real_rgb_green;
extern const real_rgb_color* global_real_rgb_blue;
extern const real_rgb_color* global_real_rgb_cyan;
extern const real_rgb_color* global_real_rgb_yellow;
extern const real_rgb_color* global_real_rgb_magenta;
extern const real_rgb_color* global_real_rgb_pink;
extern const real_rgb_color* global_real_rgb_lightblue;
extern const real_rgb_color* global_real_rgb_orange;
extern const real_rgb_color* global_real_rgb_purple;
extern const real_rgb_color* global_real_rgb_aqua;
extern const real_rgb_color* global_real_rgb_darkgreen;
extern const real_rgb_color* global_real_rgb_salmon;
extern const real_rgb_color* global_real_rgb_violet;

/* ---------- debug, profile and console: halo 1's development tools, nothing here */

#define display_assert(message, file, line, fatal) ai_assert_failed(file, line, "", message)
#define system_exit(code) ((void)0)
#define profile_enter(section) ((void)0)
#define profile_exit(section) ((void)0)
#define render_debug_line(...) ((void)0)
#define render_debug_point(...) ((void)0)
#define render_debug_sphere(...) ((void)0)
#define render_debug_circle(...) ((void)0)
#define render_debug_vector(...) ((void)0)
#define render_debug_string_at_point(...) ((void)0)

inline const char* hs_runtime_get_executing_thread_name(void) { return "ai"; }
inline boolean input_key_is_down(int32 key) { return FALSE; }
inline boolean console_is_active(void) { return FALSE; }
inline int32 fast_ftol(real value) { return (int32)value; }
#undef FLOOR
#define FLOOR(value, floor) ((value) < (floor) ? (floor) : (value))

// halo 1's comparison functions return long (the same size as int here)
typedef long (*t_ce_compare_function)(const void*, const void*);
inline void qsort(void* base, size_t count, size_t size, t_ce_compare_function compare)
{
	::qsort(base, count, size, (int(__cdecl*)(const void*, const void*))compare);
}
inline void qsort(void* base, size_t count, size_t size, int (*compare)(const void*, const void*))
{
	::qsort(base, count, size, compare);
}
inline char* strupr(const char* string)
{
	return _strupr((char*)string);
}

// profile.h, tag_groups.h and cheats.h (development tools: inert)
struct profile_section
{
	const char* name;
	int32 section_index;
	boolean active;
	int16 stack_depth;
	int32 field_C;
};

struct tag_enum_definition
{
	int32 count;
	char** names;
	void* unused;
};

struct cheat_globals
{
	boolean medusa;
};
extern cheat_globals cheat;

// input.h (no keys: halo 1's debug keys)
enum
{
	_key_g = 0x47,
	_key_i = 0x49,
	_key_n = 0x4E,
	_key_r = 0x52,
};

// scenario_definitions.h
struct scenario_object_name
{
	char name[TAG_STRING_LENGTH + 1];
	int16 runtime_object_type;
	int16 runtime_scenario_datum_index;
};
static_assert(sizeof(scenario_object_name) == 0x24);

} // namespace h1_ai

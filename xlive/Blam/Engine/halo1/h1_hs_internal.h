#pragma once

/*
* Halo 1 scripting (hs), shared by the runtime (h1_hs_runtime.cpp), the load time postprocess (h1_hs_compile.cpp) and the
* function library (h1_hs_functions.cpp). A port of halo 1's hs.c, hs_runtime.c and hs_compile.c: the scenario's compiled
* syntax tree runs as is, its threads, stacks and globals laid out as halo 1's.
*/

#include "h1_tag_definitions.h"

namespace h1_hs
{

/* ---------- constants */

enum e_hs_type : int16
{
	_hs_unparsed = 0,
	_hs_special_form,
	_hs_function_name,
	_hs_passthrough,
	_hs_type_void,
	_hs_type_boolean,
	_hs_type_real,
	_hs_type_short_integer,
	_hs_type_long_integer,
	_hs_type_string,
	_hs_type_script,
	_hs_type_trigger_volume,
	_hs_type_cutscene_flag,
	_hs_type_cutscene_camera_point,
	_hs_type_cutscene_title,
	_hs_type_cutscene_recording,
	_hs_type_device_group,
	_hs_type_ai,
	_hs_type_ai_command_list,
	_hs_type_starting_profile,
	_hs_type_conversation,
	_hs_type_navpoint,
	_hs_type_hud_message,
	_hs_type_object_list,
	_hs_type_sound,
	_hs_type_effect,
	_hs_type_damage,
	_hs_type_looping_sound,
	_hs_type_animation_graph,
	_hs_type_actor_variant,
	_hs_type_damage_effect,
	_hs_type_object_definition,
	_hs_type_enum_game_difficulty,
	_hs_type_enum_team,
	_hs_type_enum_ai_default_state,
	_hs_type_enum_actor_type,
	_hs_type_enum_hud_corner,
	_hs_type_object,
	_hs_type_unit,
	_hs_type_vehicle,
	_hs_type_weapon,
	_hs_type_device,
	_hs_type_scenery,
	_hs_type_object_name,
	_hs_type_unit_name,
	_hs_type_vehicle_name,
	_hs_type_weapon_name,
	_hs_type_device_name,
	_hs_type_scenery_name,
	k_number_of_hs_types
};

enum
{
	k_number_of_hs_object_types = 6,
	k_hs_external_global_count = 443,
	k_maximum_hs_globals = 0x400,
	k_maximum_hs_threads = 0x100,
	k_hs_thread_stack_size = 0x200,
};

enum e_hs_function_index
{
	_hs_function_begin = 0,
	_hs_function_begin_random,
	_hs_function_if,
	_hs_function_cond,
	_hs_function_set,
	_hs_function_and,
	_hs_function_or,
	_hs_function_plus,
	_hs_function_minus,
	_hs_function_times,
	_hs_function_divide,
	_hs_function_min,
	_hs_function_max,
	_hs_function_equal,
	_hs_function_not_equal,
	_hs_function_gt,
	_hs_function_lt,
	_hs_function_gte,
	_hs_function_lte,
	_hs_function_sleep,
	_hs_function_sleep_until,
	_hs_function_wake,
	_hs_function_inspect,
	_hs_function_object_to_unit,
};

enum
{
	_hs_script_startup = 0,
	_hs_script_dormant,
	_hs_script_continuous,
	_hs_script_static,
	_hs_script_stub,
};

enum
{
	_hs_syntax_node_primitive_bit = 0,
	_hs_syntax_node_script_bit,
	_hs_syntax_node_variable_bit,
};

#define HS_TYPE_VALID(type) ((type) >= _hs_type_void && (type) < k_number_of_hs_types)
#define HS_TYPE_IS_TAG_REFERENCE(type) ((type) >= _hs_type_sound && (type) <= _hs_type_object_definition)
#define HS_TYPE_IS_ENUM(type) ((type) >= _hs_type_enum_game_difficulty && (type) <= _hs_type_enum_hud_corner)
#define HS_TYPE_IS_OBJECT_NAME(type) ((type) >= _hs_type_object_name && (type) <= _hs_type_scenery_name)
#define HS_TYPE_IS_OBJECT(type) ((type) >= _hs_type_object && (type) <= _hs_type_scenery)

/* ---------- structures */

// hs_scenario_definitions.h hs_syntax_node, the scenario's script syntax data after its data array header
#pragma pack(push, 1)
struct s_hs_syntax_node
{
	int16 datum_header;
	union
	{
		int16 index;
		int16 constant_type;
		int16 function_index;
		int16 script_index;
	};
	int16 type;
	int16 flags;
	int32 next_node_index;
	int32 source_offset;
	union
	{
		int32 data;
		uint8 boolean_value;
		real32 real_value;
		int16 short_value;
	};
};
#pragma pack(pop)
static_assert(sizeof(s_hs_syntax_node) == 20);

typedef void (*hs_evaluate_procedure)(int16 function_index, int32 thread_index, bool initialize);
// a library function's work: its evaluated arguments (one 32 bit slot each), its result
typedef int32 (*hs_function_procedure)(const int32* arguments);

struct s_hs_function_definition
{
	int16 return_type;
	const char* name;
	hs_evaluate_procedure evaluate;
	int16 parameter_count;
	const int16* parameter_types;
	hs_function_procedure procedure;	// hs_evaluate_generic's, NULL until the library has one
};

struct s_hs_external_global_definition
{
	const char* name;
	int16 type;
};

/* ---------- prototypes/h1_hs_runtime.cpp */

s_hs_syntax_node* hs_syntax_get(int32 expression_index);
int32 hs_syntax_next_index(int32 expression_index);
s_hs_function_definition* hs_function_get(int16 function_index);
int16 hs_function_count(void);
int16 hs_find_function_by_name(const char* name);
int16 hs_find_script_by_name(const char* name);
int16 hs_find_global_by_name(const char* name);
int16 hs_global_get_type(int16 global_designator);
const char* hs_global_get_name(int16 global_designator);
bool hs_can_cast(int16 actual_type, int16 desired_type);
int32* hs_macro_function_evaluate(int16 function_index, int32 thread_index, bool initialize);
void hs_return(int32 thread_index, int32 value);
int32 hs_type_default(int16 type);

// the scenario's script blocks
const h1_scnr_scripts* hs_script_get(int16 script_index);
const h1_scnr_globals* hs_scenario_global_get(int16 global_index);
const char* hs_string_data(int32* out_size);

void hs_evaluate_begin(int16 function_index, int32 thread_index, bool initialize);
void hs_evaluate_begin_random(int16 function_index, int32 thread_index, bool initialize);
void hs_evaluate_if(int16 function_index, int32 thread_index, bool initialize);
void hs_evaluate_set(int16 function_index, int32 thread_index, bool initialize);
void hs_evaluate_logical(int16 function_index, int32 thread_index, bool initialize);
void hs_evaluate_arithmetic(int16 function_index, int32 thread_index, bool initialize);
void hs_evaluate_equality(int16 function_index, int32 thread_index, bool initialize);
void hs_evaluate_inequality(int16 function_index, int32 thread_index, bool initialize);
void hs_evaluate_sleep(int16 function_index, int32 thread_index, bool initialize);
void hs_evaluate_sleep_until(int16 function_index, int32 thread_index, bool initialize);
void hs_evaluate_wake(int16 function_index, int32 thread_index, bool initialize);
void hs_evaluate_inspect(int16 function_index, int32 thread_index, bool initialize);
void hs_evaluate_object_cast_up(int16 function_index, int32 thread_index, bool initialize);
void hs_evaluate_debug_string(int16 function_index, int32 thread_index, bool initialize);
// a library function: its arguments, then its procedure (the type's default without one)
void hs_evaluate_generic(int16 function_index, int32 thread_index, bool initialize);

/* ---------- prototypes/h1_hs_compile.cpp */

// hs_compile_postprocess: resolves the loaded syntax tree's functions, variables and constants by name
bool hs_compile_postprocess(void);

/* ---------- prototypes/h1_hs_functions.cpp */

// fills the function table's procedures
void hs_functions_initialize(void);
void hs_functions_initialize_for_new_map(void);
void hs_functions_dispose_from_old_map(void);
// once per tick, after the threads
void hs_functions_update(void);

// object_lists.c
int32 object_list_new(void);
void object_list_add(int32 list_index, datum object_index);
int16 object_list_count(int32 list_index);
datum object_list_get(int32 list_index, int16 index);
void object_list_add_reference(int32 list_index);
void object_list_remove_reference(int32 list_index);
void object_list_gc(void);
// ai_script.c object_list_from_ai_reference
int32 object_list_from_ai_reference(int32 ai_reference);
// object_index_from_name_index: the object a scenario object name names (NONE when it doesn't exist)
datum object_index_from_name_index(int16 name_index);
bool hs_object_type_mask_test(int16 object_type_index, datum object_index);

extern const uint16 hs_object_type_masks[k_number_of_hs_object_types];
extern const int16 hs_type_sizes[k_number_of_hs_types];
extern const char* const hs_type_names[k_number_of_hs_types];

} // namespace h1_hs

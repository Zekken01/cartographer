#include "stdafx.h"
#include "h1_hs.h"
#include "h1_hs_internal.h"

#include "h1_cache_file.h"
#include "h1_log.h"
#include "h1_map_loader.h"
#include "h1_recordings.h"
#include "h1_scenario_objects.h"

#include "game/game.h"
#include "game/game_time.h"
#include "objects/objects.h"

#include <vector>

namespace h1_hs
{

/*
* hs_runtime.c: halo 1's script threads. Each thread evaluates its script's syntax tree on its own stack, one frame per
* expression being evaluated; an evaluator runs again (initialize false) every time an expression it started returns, and a
* thread stops for the tick when one sleeps. Runtime failures (halo 1's asserts) stop the thread they happen on, logged.
*/

/* ---------- constants */

enum
{
	_hs_thread_type_script = 0,
	_hs_thread_type_global_initialize,
	_hs_thread_type_console_command,
};

enum
{
	_hs_thread_in_function_call_bit = 0,
	_hs_thread_sleeping_bit,
};

/* ---------- structures */

struct s_hs_stack_frame
{
	s_hs_stack_frame* previous;
	int32 expression_index;
	void* result;
	int16 size;
	uint8 data[2];
};
static_assert(sizeof(s_hs_stack_frame) == 16);

struct s_hs_thread
{
	int16 identifier;		// 0 when the slot is free
	uint8 type;
	uint8 flags;
	int32 script_index;
	int32 sleep_until;
	int32 previous_sleep_until;
	s_hs_stack_frame* stack;
	int32 result;
	uint8 stack_data[k_hs_thread_stack_size];
	bool failed;			// a runtime failure stopped it
};

struct s_hs_runtime_globals
{
	bool initialized;
	int16 executing_thread_index;

	std::vector<s_hs_syntax_node> syntax_nodes;
	const char* string_data;
	int32 string_data_size;
	const h1_scnr* scenario;

	s_hs_thread threads[k_maximum_hs_threads];
	int32 global_values[k_maximum_hs_globals];

	uint32 last_update_time;
};

/* ---------- globals */

#include "h1_hs_tables.inl"

const uint16 hs_object_type_masks[k_number_of_hs_object_types] = { 0xFFFF, 0x0003, 0x0002, 0x0004, 0x0380, 0x0040 };

const int16 hs_type_sizes[k_number_of_hs_types] =
{
	0, 0, 0, 0, 0, 1, 4, 2, 4, 4, 4, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2
};

const char* const hs_type_names[k_number_of_hs_types] =
{
	"unparsed", "special form", "function name", "passthrough", "void", "boolean", "real", "short", "long", "string", "script",
	"trigger_volume", "cutscene_flag", "cutscene_camera_point", "cutscene_title", "cutscene_recording", "device_group", "ai",
	"ai_command_list", "starting_profile", "conversation", "navpoint", "hud_message", "object_list", "sound", "effect", "damage",
	"looping_sound", "animation_graph", "actor_variant", "damage_effect", "object_definition", "game_difficulty", "team",
	"ai_default_state", "actor_type", "hud_corner", "object", "unit", "vehicle", "weapon", "device", "scenery", "object_name",
	"unit_name", "vehicle_name", "weapon_name", "device_name", "scenery_name",
};

static s_hs_runtime_globals g_hs;
static s_hs_syntax_node g_hs_invalid_node = { 0, { NONE }, _hs_unparsed, 0, NONE, 0, { NONE } };
static uint8 g_hs_scratch[k_hs_thread_stack_size];

/* ---------- prototypes */

static s_hs_thread* hs_thread_get(int32 thread_index);
static const char* hs_thread_format(int32 thread_index);
static void hs_thread_fail(int32 thread_index, const char* reason);
static int32 hs_thread_new(int16 type, int32 script_index);
static void hs_thread_delete(int32 thread_index);
static void hs_thread_main(int32 thread_index);
static void hs_stack_push(int32 thread_index);
static void hs_stack_pop(int32 thread_index);
static void* hs_stack_allocate(int32 thread_index, int32 size);
static void hs_evaluate(int32 thread_index, int32 expression_index, int32* destination);
static void hs_script_evaluate(int16 script_index, int32 thread_index, bool initialize);
static int32* hs_arguments_evaluate(int32 thread_index, int16 parameter_count, const int16* parameter_types, bool initialize);
static int32 hs_global_evaluate(int16 global_designator);
static int32* hs_global_value_get(int16 global_designator);
static int32 hs_cast(int32 thread_index, int16 actual_type, int16 desired_type, int32 value);
static void hs_wake(int32 thread_index);
static int32 hs_find_thread_by_script(int16 script_index);
static int32 hs_syntax_nth(int32 expression_index, int16 n);

#define HS_THREAD_ASSERT(thread_index, expression, reason) \
	(!(expression) ? (hs_thread_fail((thread_index), reason " (" #expression ")"), false) : true)

} // namespace h1_hs

using namespace h1_hs;

/* ---------- public code */

void h1_hs_initialize_for_new_map(void)
{
	h1_hs_dispose_from_old_map();
	if (!g_h1_cache_file)
	{
		return;
	}
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();
	if (!scenario || scenario->script_syntax_data.size <= 0x38)
	{
		h1_log("hs: no scripts");
		return;
	}
	g_hs.scenario = scenario;

	// the syntax data is a data array: its header (maximum count at 0x20, element size at 0x22), then its nodes
	const uint8* syntax_data = (const uint8*)g_h1_cache_file->data_get(scenario->script_syntax_data);
	const char* string_data = (const char*)g_h1_cache_file->data_get(scenario->script_string_data);
	if (!syntax_data || !string_data)
	{
		h1_log("hs: unreadable script data");
		return;
	}
	const int16 maximum_count = *(const int16*)(syntax_data + 0x20);
	const int16 element_size = *(const int16*)(syntax_data + 0x22);
	if (element_size != sizeof(s_hs_syntax_node))
	{
		h1_log("hs: unexpected syntax node size %d", element_size);
		return;
	}
	const int32 node_count = MIN((int32)maximum_count, (scenario->script_syntax_data.size - 0x38) / (int32)sizeof(s_hs_syntax_node));
	g_hs.syntax_nodes.assign((const s_hs_syntax_node*)(syntax_data + 0x38), (const s_hs_syntax_node*)(syntax_data + 0x38) + node_count);
	g_hs.string_data = string_data;
	g_hs.string_data_size = scenario->script_string_data.size;

	hs_functions_initialize();
	hs_functions_initialize_for_new_map();
	// objects_place, before the scripts start
	h1_scenario_objects_place();
	if (!hs_compile_postprocess())
	{
		h1_log("hs: the scripts didn't load, they don't run");
		h1_hs_dispose_from_old_map();
		return;
	}

	// hs_runtime_initialize_for_new_map: the globals' initial values, then a thread for every script that isn't static
	g_hs.initialized = true;
	g_hs.executing_thread_index = NONE;
	g_hs.last_update_time = game_time_get();
	csmemset(g_hs.global_values, 0, sizeof(g_hs.global_values));
	for (int16 global_index = 0; global_index < k_hs_external_global_count; global_index++)
	{
		g_hs.global_values[global_index] = hs_type_default(k_hs_external_globals[global_index].type);
	}

	const int32 internal_thread_index = hs_thread_new(_hs_thread_type_global_initialize, NONE);
	for (int16 global_index = 0; global_index < scenario->globals.count; global_index++)
	{
		const h1_scnr_globals* global = hs_scenario_global_get(global_index);
		s_hs_thread* internal_thread = hs_thread_get(internal_thread_index);
		int32* value = &g_hs.global_values[global_index + k_hs_external_global_count];
		*value = hs_type_default(global->type);
		internal_thread->script_index = NONE;
		internal_thread->stack = (s_hs_stack_frame*)internal_thread->stack_data;
		internal_thread->stack->size = 0;
		internal_thread->failed = false;
		hs_evaluate(internal_thread_index, global->initialization_expression_index, value);
		if (TEST_BIT(internal_thread->flags, _hs_thread_in_function_call_bit))
		{
			hs_thread_main(internal_thread_index);
			if (global->type == _hs_type_object_list)
			{
				object_list_add_reference(hs_global_evaluate(global_index));
			}
			if (internal_thread->sleep_until != 0)
			{
				h1_log("hs: the initialization of global %s attempted to sleep", global->name);
			}
		}
	}
	hs_thread_delete(internal_thread_index);

	int32 thread_count = 0;
	for (int16 script_index = 0; script_index < scenario->scripts.count; script_index++)
	{
		const h1_scnr_scripts* script = hs_script_get(script_index);
		if (script->script_type != _hs_script_static && script->script_type != _hs_script_stub)
		{
			if (hs_thread_new(_hs_thread_type_script, script_index) == NONE)
			{
				h1_log("hs: ran out of script threads");
				break;
			}
			thread_count++;
		}
	}
	h1_log("hs: %d syntax nodes, %d scripts (%d threads), %d globals", node_count, scenario->scripts.count, thread_count, scenario->globals.count);
	return;
}

void h1_hs_dispose_from_old_map(void)
{
	if (g_hs.initialized || !g_hs.syntax_nodes.empty())
	{
		hs_functions_dispose_from_old_map();
	}
	g_hs.initialized = false;
	g_hs.executing_thread_index = NONE;
	g_hs.syntax_nodes.clear();
	g_hs.string_data = NULL;
	g_hs.string_data_size = 0;
	g_hs.scenario = NULL;
	for (int32 i = 0; i < k_maximum_hs_threads; i++)
	{
		g_hs.threads[i].identifier = 0;
	}
	return;
}

bool h1_hs_running(void)
{
	return g_hs.initialized;
}

void h1_hs_update(void)
{
	if (!g_hs.initialized || !game_in_progress())
	{
		return;
	}

	// hs_update runs once per tick
	const uint32 time = game_time_get();
	if (time == g_hs.last_update_time)
	{
		return;
	}
	g_hs.last_update_time = time;
	h1_recordings_update();

	for (int32 thread_index = 0; g_hs.initialized && thread_index < k_maximum_hs_threads; thread_index++)
	{
		s_hs_thread* thread = &g_hs.threads[thread_index];
		if (thread->identifier != 0 && thread->sleep_until >= 0 && thread->sleep_until <= (int32)time)
		{
			hs_thread_main(thread_index);
		}
	}
	hs_functions_update();
	object_list_gc();
	return;
}

bool h1_hs_wake_by_name(const char* name)
{
	for (int32 thread_index = 0; g_hs.initialized && thread_index < k_maximum_hs_threads; thread_index++)
	{
		s_hs_thread* thread = &g_hs.threads[thread_index];
		if (thread->identifier != 0 && thread->script_index != NONE && !_stricmp(hs_script_get((int16)thread->script_index)->name, name))
		{
			hs_wake(thread_index);
			return true;
		}
	}
	return false;
}

namespace h1_hs
{

s_hs_syntax_node* hs_syntax_get(int32 expression_index)
{
	const int32 index = DATUM_INDEX_TO_ABSOLUTE_INDEX(expression_index);
	if (expression_index == NONE || index >= (int32)g_hs.syntax_nodes.size())
	{
		g_hs_invalid_node.next_node_index = NONE;
		g_hs_invalid_node.data = NONE;
		return &g_hs_invalid_node;
	}
	return &g_hs.syntax_nodes[index];
}

// data_next_index of the syntax data: the next node in use
int32 hs_syntax_next_index(int32 expression_index)
{
	int32 index = expression_index == NONE ? 0 : DATUM_INDEX_TO_ABSOLUTE_INDEX(expression_index) + 1;
	for (; index < (int32)g_hs.syntax_nodes.size(); index++)
	{
		if (g_hs.syntax_nodes[index].datum_header != 0)
		{
			return ((int32)(uint16)g_hs.syntax_nodes[index].datum_header << 16) | index;
		}
	}
	return NONE;
}

s_hs_function_definition* hs_function_get(int16 function_index)
{
	return VALID_INDEX(function_index, NUMBEROF(g_hs_functions)) ? &g_hs_functions[function_index] : NULL;
}

int16 hs_function_count(void)
{
	return (int16)NUMBEROF(g_hs_functions);
}

int16 hs_find_function_by_name(const char* name)
{
	for (int16 function_index = 0; function_index < (int16)NUMBEROF(g_hs_functions); function_index++)
	{
		if (!_stricmp(g_hs_functions[function_index].name, name))
		{
			return function_index;
		}
	}
	return NONE;
}

int16 hs_find_script_by_name(const char* name)
{
	for (int16 script_index = 0; g_hs.scenario && script_index < g_hs.scenario->scripts.count; script_index++)
	{
		if (!strcmp(name, hs_script_get(script_index)->name))
		{
			return script_index;
		}
	}
	return NONE;
}

int16 hs_find_global_by_name(const char* name)
{
	for (int16 global_index = 0; global_index < k_hs_external_global_count; global_index++)
	{
		if (!_stricmp(name, k_hs_external_globals[global_index].name))
		{
			return global_index | 0x8000;
		}
	}
	for (int16 global_index = 0; g_hs.scenario && global_index < g_hs.scenario->globals.count; global_index++)
	{
		if (!_stricmp(name, hs_scenario_global_get(global_index)->name))
		{
			return global_index & ~0x8000;
		}
	}
	return NONE;
}

int16 hs_global_get_type(int16 global_designator)
{
	if (global_designator & 0x8000)
	{
		const int16 index = global_designator & 0x7FFF;
		return VALID_INDEX(index, k_hs_external_global_count) ? k_hs_external_globals[index].type : _hs_type_void;
	}
	const h1_scnr_globals* global = hs_scenario_global_get(global_designator & 0x7FFF);
	return global ? global->type : _hs_type_void;
}

const char* hs_global_get_name(int16 global_designator)
{
	if (global_designator & 0x8000)
	{
		const int16 index = global_designator & 0x7FFF;
		return VALID_INDEX(index, k_hs_external_global_count) ? k_hs_external_globals[index].name : "<bad global>";
	}
	const h1_scnr_globals* global = hs_scenario_global_get(global_designator & 0x7FFF);
	return global ? global->name : "<bad global>";
}

const h1_scnr_scripts* hs_script_get(int16 script_index)
{
	static h1_scnr_scripts s_invalid_script = { "<bad script>", _hs_script_static, _hs_type_void, NONE };
	const h1_scnr_scripts* script = g_hs.scenario ? g_h1_cache_file->block_get(g_hs.scenario->scripts, script_index) : NULL;
	return script ? script : &s_invalid_script;
}

const h1_scnr_globals* hs_scenario_global_get(int16 global_index)
{
	return g_hs.scenario ? g_h1_cache_file->block_get(g_hs.scenario->globals, global_index) : NULL;
}

const char* hs_string_data(int32* out_size)
{
	*out_size = g_hs.string_data_size;
	return g_hs.string_data;
}

int32 hs_type_default(int16 type)
{
	switch (type)
	{
	case _hs_type_boolean:
	case _hs_type_real:
	case _hs_type_short_integer:
	case _hs_type_long_integer:
	case _hs_type_void:
		return 0;
	case _hs_type_string:
		return (int32)"";
	default:
		return NONE;
	}
}

/* ---------- typecasting */

static int32 hs_long_to_boolean(int32 n) { return n == 0; }
static int32 hs_short_to_boolean(int32 s) { return (int16)s == 0; }
static int32 hs_string_to_boolean(int32 n) { return hs_long_to_boolean((int32)strlen((const char*)n)); }
static int32 hs_data_to_void(int32 n) { return 0; }
static int32 hs_short_to_real(int32 s) { real32 r = (real32)(int16)s; return *(int32*)&r; }
static int32 hs_long_to_real(int32 l) { real32 r = (real32)l; return *(int32*)&r; }
static int32 hs_enum_to_real(int32 e) { real32 r = (real32)((int16)e + 1); return *(int32*)&r; }
static int32 hs_real_to_short(int32 r) { return (int16)*(real32*)&r; }
static int32 hs_real_to_long(int32 r) { return (int32)*(real32*)&r; }
static int32 hs_long_to_short(int32 l) { return (int16)l; }
static int32 hs_short_to_long(int32 s) { return (int16)s; }

static int32 hs_object_name_to_object_list(int32 object_name_index)
{
	int32 list_index = NONE;
	const datum object_index = object_index_from_name_index((int16)object_name_index);
	if (object_index != NONE)
	{
		list_index = object_list_new();
		object_list_add(list_index, object_index);
	}
	return list_index;
}

static int32 hs_object_to_object_list(int32 object_index)
{
	int32 list_index = NONE;
	if (object_index != NONE)
	{
		list_index = object_list_new();
		object_list_add(list_index, object_index);
	}
	return list_index;
}

typedef int32 (*hs_typecasting_procedure)(int32 value);

// hs_runtime.c typecasting_procedures[desired][actual]
static hs_typecasting_procedure hs_typecasting_procedure_get(int16 desired_type, int16 actual_type)
{
	switch (desired_type)
	{
	case _hs_type_void:
		return HS_TYPE_VALID(actual_type) ? hs_data_to_void : NULL;
	case _hs_type_boolean:
		switch (actual_type)
		{
		case _hs_type_real: return hs_long_to_boolean;
		case _hs_type_short_integer: return hs_short_to_boolean;
		case _hs_type_long_integer: return hs_long_to_boolean;
		case _hs_type_string: return hs_string_to_boolean;
		}
		return NULL;
	case _hs_type_real:
		if (HS_TYPE_IS_ENUM(actual_type)) return hs_enum_to_real;
		switch (actual_type)
		{
		case _hs_type_short_integer: return hs_short_to_real;
		case _hs_type_long_integer: return hs_long_to_real;
		}
		return NULL;
	case _hs_type_short_integer:
		switch (actual_type)
		{
		case _hs_type_real: return hs_real_to_short;
		case _hs_type_long_integer: return hs_long_to_short;
		}
		return NULL;
	case _hs_type_long_integer:
		switch (actual_type)
		{
		case _hs_type_real: return hs_real_to_long;
		case _hs_type_short_integer: return hs_short_to_long;
		}
		return NULL;
	case _hs_type_object_list:
		if (actual_type == _hs_type_ai) return object_list_from_ai_reference;
		if (HS_TYPE_IS_OBJECT(actual_type)) return hs_object_to_object_list;
		if (HS_TYPE_IS_OBJECT_NAME(actual_type)) return hs_object_name_to_object_list;
		return NULL;
	}
	return NULL;
}

static bool hs_object_type_can_cast(int16 actual_type, int16 desired_type)
{
	if (!VALID_INDEX(actual_type, k_number_of_hs_object_types) || !VALID_INDEX(desired_type, k_number_of_hs_object_types))
	{
		return false;
	}
	const uint16 actual_mask = hs_object_type_masks[actual_type];
	return (actual_mask & hs_object_type_masks[desired_type]) == actual_mask;
}

bool hs_can_cast(int16 actual_type, int16 desired_type)
{
	if (actual_type == _hs_passthrough || actual_type == desired_type)
	{
		return true;
	}
	if (HS_TYPE_IS_OBJECT(desired_type))
	{
		if (HS_TYPE_IS_OBJECT(actual_type))
		{
			return hs_object_type_can_cast(actual_type - _hs_type_object, desired_type - _hs_type_object);
		}
		if (HS_TYPE_IS_OBJECT_NAME(actual_type))
		{
			return hs_object_type_can_cast(actual_type - _hs_type_object_name, desired_type - _hs_type_object);
		}
		return false;
	}
	if (HS_TYPE_IS_OBJECT_NAME(desired_type))
	{
		return HS_TYPE_IS_OBJECT_NAME(actual_type) &&
			hs_object_type_can_cast(actual_type - _hs_type_object_name, desired_type - _hs_type_object_name);
	}
	return hs_typecasting_procedure_get(desired_type, actual_type) != NULL;
}

static int32 hs_cast(int32 thread_index, int16 actual_type, int16 desired_type, int32 value)
{
	if (!hs_can_cast(actual_type, desired_type))
	{
		hs_thread_fail(thread_index, "bad typecast.");
		return hs_type_default(desired_type);
	}
	if (actual_type != desired_type && actual_type != _hs_passthrough && !HS_TYPE_IS_OBJECT_NAME(desired_type))
	{
		if (!HS_TYPE_IS_OBJECT(desired_type))
		{
			return hs_typecasting_procedure_get(desired_type, actual_type)(value);
		}
		if (HS_TYPE_IS_OBJECT_NAME(actual_type))
		{
			return object_index_from_name_index((int16)value);
		}
	}
	return value;
}

/* ---------- evaluation */

void hs_return(int32 thread_index, int32 value)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	if (thread->failed)
	{
		return;
	}
	const s_hs_syntax_node* expression = hs_syntax_get(thread->stack->expression_index);
	const int16 return_type = TEST_BIT(expression->flags, _hs_syntax_node_script_bit) ?
		hs_script_get(expression->index)->return_type :
		(hs_function_get(expression->index) ? hs_function_get(expression->index)->return_type : (int16)_hs_type_void);
	if (!HS_THREAD_ASSERT(thread_index, thread->stack->previous, "corrupted stack."))
	{
		return;
	}
	*(int32*)thread->stack->previous->result = hs_cast(thread_index, return_type, expression->type, value);
	thread->stack = thread->stack->previous;
	return;
}

int32* hs_macro_function_evaluate(int16 function_index, int32 thread_index, bool initialize)
{
	const s_hs_function_definition* function = hs_function_get(function_index);
	return hs_arguments_evaluate(thread_index, function->parameter_count, function->parameter_types, initialize);
}

void hs_evaluate_generic(int16 function_index, int32 thread_index, bool initialize)
{
	const int32* arguments = hs_macro_function_evaluate(function_index, thread_index, initialize);
	if (arguments)
	{
		const s_hs_function_definition* function = hs_function_get(function_index);
		hs_return(thread_index, function->procedure ? function->procedure(arguments) : hs_type_default(function->return_type));
	}
	return;
}

void hs_evaluate_begin(int16 function_index, int32 thread_index, bool initialize)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	int32* expression_index = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	int32* result = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	if (initialize)
	{
		*expression_index = hs_syntax_get(hs_syntax_get(thread->stack->expression_index)->data)->next_node_index;
		*result = 0;
	}
	if (*expression_index != NONE)
	{
		hs_evaluate(thread_index, *expression_index, result);
		*expression_index = hs_syntax_get(*expression_index)->next_node_index;
	}
	else
	{
		hs_return(thread_index, *result);
	}
	return;
}

void hs_evaluate_begin_random(int16 function_index, int32 thread_index, bool initialize)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	int16* argument_count = (int16*)hs_stack_allocate(thread_index, sizeof(int16));
	uint32* evaluated = (uint32*)hs_stack_allocate(thread_index, sizeof(int32));
	int32* result = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	const int32 first_expression_index = hs_syntax_get(hs_syntax_get(thread->stack->expression_index)->data)->next_node_index;
	if (initialize)
	{
		*argument_count = 0;
		for (int32 expression_index = first_expression_index; expression_index != NONE; expression_index = hs_syntax_get(expression_index)->next_node_index)
		{
			*argument_count += 1;
		}
		if (!HS_THREAD_ASSERT(thread_index, *argument_count < 32, "too many arguments."))
		{
			return;
		}
		*evaluated = 0;
	}
	int16 index;
	const int16 random = *argument_count > 0 ? (int16)(rand() % *argument_count) : 0;
	for (index = 0; index < *argument_count; index++)
	{
		const int16 choice = (int16)((random + index) % *argument_count);
		if (!TEST_BIT(*evaluated, choice))
		{
			hs_evaluate(thread_index, hs_syntax_nth(first_expression_index, choice), result);
			*evaluated |= FLAG(choice);
			break;
		}
	}
	if (index == *argument_count)
	{
		hs_return(thread_index, *result);
	}
	return;
}

void hs_evaluate_if(int16 function_index, int32 thread_index, bool initialize)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	int32* condition = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	int32* expression_index = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	int32* result = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	const int32 condition_index = hs_syntax_get(hs_syntax_get(thread->stack->expression_index)->data)->next_node_index;
	if (initialize)
	{
		*condition = 0;
		*expression_index = NONE;
		hs_evaluate(thread_index, condition_index, condition);
	}
	else if (*expression_index == NONE)
	{
		const int32 then_index = hs_syntax_get(condition_index)->next_node_index;
		if ((uint8)*condition)
		{
			*expression_index = then_index;
		}
		else
		{
			*expression_index = hs_syntax_get(then_index)->next_node_index;
			if (*expression_index == NONE)
			{
				hs_return(thread_index, 0);
				return;
			}
		}
		hs_evaluate(thread_index, *expression_index, result);
	}
	else
	{
		hs_return(thread_index, *result);
	}
	return;
}

void hs_evaluate_set(int16 function_index, int32 thread_index, bool initialize)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	const int32 variable_expression_index = hs_syntax_get(hs_syntax_get(thread->stack->expression_index)->data)->next_node_index;
	const s_hs_syntax_node* variable = hs_syntax_get(variable_expression_index);
	const int16 designator = (int16)variable->data;
	const int16 type = hs_global_get_type(designator);
	hs_stack_allocate(thread_index, sizeof(int32));
	if (initialize)
	{
		if (type == _hs_type_object_list)
		{
			object_list_remove_reference(hs_global_evaluate(designator));
		}
		hs_evaluate(thread_index, variable->next_node_index, hs_global_value_get(designator));
	}
	else
	{
		if (type == _hs_type_object_list)
		{
			object_list_add_reference(hs_global_evaluate(designator));
		}
		hs_return(thread_index, hs_global_evaluate(designator));
	}
	return;
}

void hs_evaluate_logical(int16 function_index, int32 thread_index, bool initialize)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	int32* expression_index = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	int32* value = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	uint8* result = (uint8*)hs_stack_allocate(thread_index, sizeof(uint8));
	const bool and_ = function_index == _hs_function_and;
	if (initialize)
	{
		*expression_index = hs_syntax_get(hs_syntax_get(thread->stack->expression_index)->data)->next_node_index;
		*result = and_;
	}
	else
	{
		const bool argument = (uint8)*value != 0;
		*result = and_ ? (*result && argument) : (*result || argument);
	}
	if (*expression_index != NONE && (*result != 0) == and_)
	{
		hs_evaluate(thread_index, *expression_index, value);
		*expression_index = hs_syntax_get(*expression_index)->next_node_index;
	}
	else
	{
		hs_return(thread_index, *result);
	}
	return;
}

void hs_evaluate_arithmetic(int16 function_index, int32 thread_index, bool initialize)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	int16* argument_index = (int16*)hs_stack_allocate(thread_index, sizeof(int16));
	int32* expression_index = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	real32* value = (real32*)hs_stack_allocate(thread_index, sizeof(real32));
	real32* result = (real32*)hs_stack_allocate(thread_index, sizeof(real32));
	if (initialize)
	{
		*argument_index = 0;
		*expression_index = hs_syntax_get(hs_syntax_get(thread->stack->expression_index)->data)->next_node_index;
	}
	else
	{
		const real32 argument = *value;
		if (*argument_index == 0)
		{
			*result = argument;
		}
		else
		{
			switch (function_index)
			{
			case _hs_function_plus: *result = *result + argument; break;
			case _hs_function_minus: *result = *result - argument; break;
			case _hs_function_times: *result = *result * argument; break;
			case _hs_function_divide: *result = *result / argument; break;
			case _hs_function_min: *result = MIN(*result, argument); break;
			case _hs_function_max: *result = MAX(*result, argument); break;
			}
		}
		*argument_index += 1;
	}
	if (*expression_index != NONE)
	{
		hs_evaluate(thread_index, *expression_index, (int32*)value);
		*expression_index = hs_syntax_get(*expression_index)->next_node_index;
	}
	else
	{
		hs_return(thread_index, *(int32*)result);
	}
	return;
}

void hs_evaluate_equality(int16 function_index, int32 thread_index, bool initialize)
{
	const int16 type = hs_syntax_get(hs_syntax_get(hs_syntax_get(hs_thread_get(thread_index)->stack->expression_index)->data)->next_node_index)->type;
	const int16 parameter_types[2] = { type, type };
	const int32* arguments = hs_arguments_evaluate(thread_index, 2, parameter_types, initialize);
	if (arguments)
	{
		const int16 size = VALID_INDEX(type, k_number_of_hs_types) ? hs_type_sizes[type] : 4;
		bool equal = memcmp(arguments, arguments + 1, size) == 0;
		if (function_index == _hs_function_not_equal)
		{
			equal = !equal;
		}
		hs_return(thread_index, equal);
	}
	return;
}

void hs_evaluate_inequality(int16 function_index, int32 thread_index, bool initialize)
{
	const int16 type = hs_syntax_get(hs_syntax_get(hs_syntax_get(hs_thread_get(thread_index)->stack->expression_index)->data)->next_node_index)->type;
	const int16 parameter_types[2] = { type, type };
	const int32* arguments = hs_arguments_evaluate(thread_index, 2, parameter_types, initialize);
	if (arguments)
	{
		real32 value0;
		real32 value1;
		switch (type)
		{
		case _hs_type_real:
			value0 = ((const real32*)arguments)[0];
			value1 = ((const real32*)arguments)[1];
			break;
		case _hs_type_long_integer:
			value0 = (real32)arguments[0];
			value1 = (real32)arguments[1];
			break;
		default:
			value0 = (real32)(int16)arguments[0];
			value1 = (real32)(int16)arguments[1];
			break;
		}
		bool comparison = false;
		switch (function_index)
		{
		case _hs_function_gt: comparison = value0 > value1; break;
		case _hs_function_lt: comparison = value0 < value1; break;
		case _hs_function_gte: comparison = value0 >= value1; break;
		case _hs_function_lte: comparison = value0 <= value1; break;
		}
		hs_return(thread_index, comparison);
	}
	return;
}

void hs_evaluate_sleep(int16 function_index, int32 thread_index, bool initialize)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	int32* ticks = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	int32* script_index = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	int16* argument_index = (int16*)hs_stack_allocate(thread_index, sizeof(int16));
	const int32 ticks_index = hs_syntax_get(hs_syntax_get(thread->stack->expression_index)->data)->next_node_index;
	if (initialize)
	{
		hs_evaluate(thread_index, ticks_index, ticks);
		*argument_index = 0;
		return;
	}
	if (*argument_index == 0)
	{
		const int32 expression_index = hs_syntax_get(ticks_index)->next_node_index;
		*argument_index += 1;
		if (expression_index != NONE)
		{
			hs_evaluate(thread_index, expression_index, script_index);
			return;
		}
		*script_index = NONE;
	}

	const int16 sleep_ticks = (int16)*ticks;
	if (sleep_ticks != 0)
	{
		int32 sleep_thread_index = thread_index;
		if ((int16)*script_index != NONE)
		{
			sleep_thread_index = hs_find_thread_by_script((int16)*script_index);
		}
		if (sleep_thread_index != NONE)
		{
			s_hs_thread* sleep_thread = hs_thread_get(sleep_thread_index);
			const int32 sleep_until = sleep_ticks < 0 ? NONE - 1 : (int32)game_time_get() + sleep_ticks;
			if (sleep_thread->sleep_until != NONE)
			{
				if (sleep_thread_index != thread_index && !TEST_BIT(sleep_thread->flags, _hs_thread_sleeping_bit))
				{
					SET_BIT(sleep_thread->flags, _hs_thread_sleeping_bit, true);
					sleep_thread->previous_sleep_until = sleep_thread->sleep_until;
				}
				sleep_thread->sleep_until = sleep_until;
			}
		}
	}
	hs_return(thread_index, 0);
	return;
}

void hs_evaluate_sleep_until(int16 function_index, int32 thread_index, bool initialize)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	int32* condition = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	int32* ticks = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	int32* timeout = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	int32* start_time = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	int16* argument_index = (int16*)hs_stack_allocate(thread_index, sizeof(int16));
	const int32 condition_index = hs_syntax_get(hs_syntax_get(thread->stack->expression_index)->data)->next_node_index;
	const int32 expression_index = hs_syntax_get(condition_index)->next_node_index;
	if (initialize)
	{
		*condition = 0;
		*start_time = (int32)game_time_get();
		*argument_index = 0;
		*ticks = 30;
		*timeout = NONE;
		if (expression_index != NONE)
		{
			hs_evaluate(thread_index, expression_index, ticks);
			return;
		}
	}
	if (*argument_index == 0)
	{
		*argument_index = 1;
		if (expression_index != NONE)
		{
			const int32 timeout_expression_index = hs_syntax_get(expression_index)->next_node_index;
			if (timeout_expression_index != NONE)
			{
				hs_evaluate(thread_index, timeout_expression_index, timeout);
				return;
			}
		}
	}
	if (*argument_index == 1)
	{
		if ((uint8)*condition || (*timeout != NONE && (int32)game_time_get() >= *timeout + *start_time))
		{
			hs_return(thread_index, 0);
		}
		else
		{
			hs_evaluate(thread_index, condition_index, condition);
			thread->sleep_until = (int32)game_time_get() + MAX(1, (int16)*ticks);
			if (*timeout != NONE)
			{
				thread->sleep_until = MIN(*timeout + *start_time, thread->sleep_until);
			}
		}
	}
	return;
}

void hs_evaluate_wake(int16 function_index, int32 thread_index, bool initialize)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	const s_hs_syntax_node* script_name_node = hs_syntax_get(hs_syntax_get(hs_syntax_get(thread->stack->expression_index)->data)->next_node_index);
	if (HS_THREAD_ASSERT(thread_index, script_name_node->type == _hs_type_script, "corrupted syntax tree."))
	{
		const int32 wake_thread_index = hs_find_thread_by_script((int16)script_name_node->data);
		if (wake_thread_index != NONE)
		{
			hs_wake(wake_thread_index);
		}
		hs_return(thread_index, 0);
	}
	return;
}

void hs_evaluate_inspect(int16 function_index, int32 thread_index, bool initialize)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	int32* value = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	const int32 expression_index = hs_syntax_get(hs_syntax_get(thread->stack->expression_index)->data)->next_node_index;
	if (initialize)
	{
		hs_evaluate(thread_index, expression_index, value);
	}
	else
	{
		const int16 type = hs_syntax_get(expression_index)->type;
		switch (type)
		{
		case _hs_type_boolean: h1_log("hs: inspect %s", (uint8)*value ? "true" : "false"); break;
		case _hs_type_real: h1_log("hs: inspect %f", *(real32*)value); break;
		case _hs_type_short_integer: h1_log("hs: inspect %d", (int16)*value); break;
		case _hs_type_long_integer: h1_log("hs: inspect %ld", *value); break;
		case _hs_type_string: h1_log("hs: inspect %s", (const char*)*value); break;
		}
		hs_return(thread_index, 0);
	}
	return;
}

void hs_evaluate_object_cast_up(int16 function_index, int32 thread_index, bool initialize)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	int32* object_index = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	if (initialize)
	{
		hs_evaluate(thread_index, hs_syntax_get(hs_syntax_get(thread->stack->expression_index)->data)->next_node_index, object_index);
	}
	else if (*object_index != NONE)
	{
		const int16 object_type = function_index - _hs_function_inspect;
		hs_return(thread_index, hs_object_type_mask_test(object_type, *object_index) ? *object_index : NONE);
	}
	else
	{
		hs_return(thread_index, NONE);
	}
	return;
}

// the ai debugging functions take any number of strings, they do nothing here
void hs_evaluate_debug_string(int16 function_index, int32 thread_index, bool initialize)
{
	hs_return(thread_index, 0);
	return;
}

/* ---------- private code */

static s_hs_thread* hs_thread_get(int32 thread_index)
{
	return &g_hs.threads[thread_index & 0xFF];
}

static const char* hs_thread_format(int32 thread_index)
{
	const s_hs_thread* thread = hs_thread_get(thread_index);
	switch (thread->type)
	{
	case _hs_thread_type_script: return hs_script_get((int16)thread->script_index)->name;
	case _hs_thread_type_global_initialize: return "[global initialize]";
	default: return "[console command]";
	}
}

// halo 1 asserts here, the thread stops instead
static void hs_thread_fail(int32 thread_index, const char* reason)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	if (!thread->failed)
	{
		h1_log("hs: a problem occurred while executing the script %s: %s", hs_thread_format(thread_index), reason);
	}
	thread->failed = true;
	return;
}

static int32 hs_thread_new(int16 type, int32 script_index)
{
	for (int32 thread_index = 0; thread_index < k_maximum_hs_threads; thread_index++)
	{
		s_hs_thread* thread = &g_hs.threads[thread_index];
		if (thread->identifier == 0)
		{
			thread->identifier = 1;
			thread->stack = (s_hs_stack_frame*)thread->stack_data;
			thread->stack->previous = NULL;
			thread->stack->size = 0;
			thread->stack->expression_index = NONE;
			thread->type = (uint8)type;
			thread->script_index = script_index;
			thread->flags = 0;
			thread->failed = false;
			thread->previous_sleep_until = 0;
			thread->sleep_until = script_index != NONE && hs_script_get((int16)script_index)->script_type == _hs_script_dormant ? NONE - 1 : 0;
			return thread_index;
		}
	}
	return NONE;
}

static void hs_thread_delete(int32 thread_index)
{
	hs_thread_get(thread_index)->identifier = 0;
	return;
}

static void hs_thread_main(int32 thread_index)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	const h1_scnr_scripts* script = NULL;
	g_hs.executing_thread_index = (int16)thread_index;
	if (thread->type == _hs_thread_type_script)
	{
		script = hs_script_get((int16)thread->script_index);
	}

	thread->sleep_until = 0;
	if (thread->stack == (s_hs_stack_frame*)thread->stack_data && script)
	{
		thread->stack->size = 0;
		thread->failed = false;
		hs_evaluate(thread_index, script->root_expression_index, (int32*)hs_stack_allocate(thread_index, sizeof(int32)));
	}

	while (thread->stack != (s_hs_stack_frame*)thread->stack_data &&
		thread->sleep_until >= 0 &&
		(!game_in_progress() || thread->sleep_until <= (int32)game_time_get()) &&
		g_hs.initialized &&
		!thread->failed)
	{
		const s_hs_syntax_node* expression = hs_syntax_get(thread->stack->expression_index);
		const bool initialize = TEST_BIT(thread->flags, _hs_thread_in_function_call_bit);
		thread->stack->size = 0;
		SET_BIT(thread->flags, _hs_thread_in_function_call_bit, false);
		if (!TEST_BIT(expression->flags, _hs_syntax_node_script_bit))
		{
			const s_hs_function_definition* function = hs_function_get(expression->index);
			if (!HS_THREAD_ASSERT(thread_index, function && function->evaluate, "corrupted syntax tree."))
			{
				break;
			}
			function->evaluate(expression->index, thread_index, initialize);
		}
		else
		{
			hs_script_evaluate(expression->index, thread_index, initialize);
		}
	}

	if (thread->failed)
	{
		// a failed script stops for good
		thread->stack = (s_hs_stack_frame*)thread->stack_data;
		thread->stack->size = 0;
		SET_BIT(thread->flags, _hs_thread_in_function_call_bit, false);
		if (thread->type == _hs_thread_type_script)
		{
			thread->sleep_until = NONE;
		}
	}

	if (thread->stack == (s_hs_stack_frame*)thread->stack_data)
	{
		if (thread->type == _hs_thread_type_script)
		{
			if (script->script_type == _hs_script_startup || script->script_type == _hs_script_dormant || thread->failed)
			{
				thread->sleep_until = NONE;
			}
		}
		else if (thread->type == _hs_thread_type_console_command)
		{
			hs_thread_delete(thread_index);
		}
	}
	g_hs.executing_thread_index = NONE;
	return;
}

static void hs_script_evaluate(int16 script_index, int32 thread_index, bool initialize)
{
	const h1_scnr_scripts* script = hs_script_get(script_index);
	int32* result = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	if (initialize)
	{
		hs_evaluate(thread_index, script->root_expression_index, result);
	}
	else
	{
		hs_return(thread_index, *result);
	}
	return;
}

static void hs_stack_push(int32 thread_index)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	s_hs_stack_frame* new_frame = (s_hs_stack_frame*)((uint8*)thread->stack + thread->stack->size + sizeof(s_hs_stack_frame));
	if (!HS_THREAD_ASSERT(thread_index, (uint8*)(new_frame + 1) < thread->stack_data + k_hs_thread_stack_size, "stack overflow."))
	{
		return;
	}
	new_frame->previous = thread->stack;
	thread->stack = new_frame;
	new_frame->size = 0;
	return;
}

static void hs_stack_pop(int32 thread_index)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	if (thread->stack->previous)
	{
		thread->stack = thread->stack->previous;
	}
	return;
}

static void* hs_stack_allocate(int32 thread_index, int32 size)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	s_hs_stack_frame* frame = thread->stack;
	if (thread->failed ||
		!HS_THREAD_ASSERT(thread_index, frame->data + frame->size + size <= thread->stack_data + k_hs_thread_stack_size, "stack overflow."))
	{
		csmemset(g_hs_scratch, 0, sizeof(g_hs_scratch));
		return g_hs_scratch;
	}
	void* result = frame->data + frame->size;
	frame->size += (int16)size;
	return result;
}

static void hs_evaluate(int32 thread_index, int32 expression_index, int32* destination)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	if (thread->failed)
	{
		return;
	}
	const s_hs_syntax_node* expression = hs_syntax_get(expression_index);
	if (!HS_THREAD_ASSERT(thread_index, expression != &g_hs_invalid_node, "corrupted syntax tree."))
	{
		return;
	}
	if (TEST_BIT(expression->flags, _hs_syntax_node_primitive_bit))
	{
		if (TEST_BIT(expression->flags, _hs_syntax_node_variable_bit))
		{
			*destination = hs_cast(thread_index, hs_global_get_type((int16)expression->data), expression->type, hs_global_evaluate((int16)expression->data));
		}
		else
		{
			*destination = hs_cast(thread_index, expression->constant_type, expression->type, expression->data);
		}
	}
	else
	{
		thread->stack->result = destination;
		hs_stack_push(thread_index);
		if (thread->failed)
		{
			return;
		}
		SET_BIT(thread->flags, _hs_thread_in_function_call_bit, true);
		thread->stack->expression_index = expression_index;
	}
	return;
}

static int32* hs_arguments_evaluate(int32 thread_index, int16 parameter_count, const int16* parameter_types, bool initialize)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	int32* values = (int32*)hs_stack_allocate(thread_index, MAX(1, parameter_count) * sizeof(int32));
	int16* argument_index = (int16*)hs_stack_allocate(thread_index, sizeof(int16));
	int32* expression_index = (int32*)hs_stack_allocate(thread_index, sizeof(int32));
	if (thread->failed)
	{
		return NULL;
	}
	if (initialize)
	{
		*argument_index = 0;
		*expression_index = hs_syntax_get(hs_syntax_get(thread->stack->expression_index)->data)->next_node_index;
	}
	if (*argument_index < parameter_count)
	{
		if (!HS_THREAD_ASSERT(thread_index, *expression_index != NONE, "corrupted syntax tree."))
		{
			return NULL;
		}
		if (hs_syntax_get(*expression_index)->type != parameter_types[*argument_index])
		{
			hs_thread_fail(thread_index, "unexpected actual parameters (the script needs to be recompiled).");
			return NULL;
		}
		hs_evaluate(thread_index, *expression_index, &values[*argument_index]);
		*expression_index = hs_syntax_get(*expression_index)->next_node_index;
		*argument_index += 1;
		return NULL;
	}
	return values;
}

static int32* hs_global_value_get(int16 global_designator)
{
	// externals first, then the scenario's
	const int32 index = (global_designator & 0x8000) ? (global_designator & 0x7FFF) : (global_designator & 0x7FFF) + k_hs_external_global_count;
	static int32 s_invalid;
	return VALID_INDEX(index, k_maximum_hs_globals) ? &g_hs.global_values[index] : &s_invalid;
}

static int32 hs_global_evaluate(int16 global_designator)
{
	return *hs_global_value_get(global_designator);
}

static void hs_wake(int32 thread_index)
{
	s_hs_thread* thread = hs_thread_get(thread_index);
	if (thread->sleep_until == NONE)
	{
		return;
	}
	thread->sleep_until = 0;
	if (TEST_BIT(thread->flags, _hs_thread_sleeping_bit))
	{
		thread->sleep_until = thread->previous_sleep_until;
		SET_BIT(thread->flags, _hs_thread_sleeping_bit, false);
		return;
	}
	// a thread waiting in sleep_until wakes out of it
	if (thread->stack->expression_index != NONE && hs_syntax_get(thread->stack->expression_index)->index == _hs_function_sleep_until)
	{
		hs_stack_pop(thread_index);
		return;
	}
	if (thread->stack->previous && thread->stack->previous->expression_index != NONE &&
		hs_syntax_get(thread->stack->previous->expression_index)->index == _hs_function_sleep_until)
	{
		hs_stack_pop(thread_index);
		hs_stack_pop(thread_index);
		SET_BIT(thread->flags, _hs_thread_in_function_call_bit, false);
	}
	return;
}

static int32 hs_find_thread_by_script(int16 script_index)
{
	for (int32 thread_index = 0; thread_index < k_maximum_hs_threads; thread_index++)
	{
		const s_hs_thread* thread = &g_hs.threads[thread_index];
		if (thread->identifier != 0 && thread->script_index == script_index)
		{
			return thread_index;
		}
	}
	return NONE;
}

static int32 hs_syntax_nth(int32 expression_index, int16 n)
{
	for (int16 index = 0; index < n; index++)
	{
		expression_index = hs_syntax_get(expression_index)->next_node_index;
	}
	return expression_index;
}

} // namespace h1_hs

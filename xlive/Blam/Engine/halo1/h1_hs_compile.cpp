#include "stdafx.h"
#include "h1_hs_internal.h"

#include "h1_cache_file.h"
#include "h1_log.h"

#include <set>
#include <string>

namespace h1_hs
{

/*
* hs_compile.c hs_compile_postprocess: the scenario's syntax tree as tool compiled it, its function calls resolved by name
* against this function table, its variables against the globals, its constants that refer to the map (strings, scripts,
* scenario names, tags) parsed again from their source text.
*/

/* ---------- globals */

static const char* const k_game_difficulty_names[] = { "easy", "normal", "hard", "impossible" };
static const char* const k_team_names[] = { "default", "player", "human", "covenant", "flood", "sentinel", "unused6", "unused7", "unused8", "unused9" };
static const char* const k_ai_default_state_names[] = { "none", "sleep", "alert", "move_repeat", "move_loop", "move_loop_back_and_forth", "move_loop_random", "move_random", "guard", "guard_at_position", "search", "flee" };
static const char* const k_actor_type_names[] = { "elite", "jackal", "grunt", "hunter", "engineer", "assassin", "player", "marine", "crew", "combat_form", "infection_form", "carrier_form", "monitor", "sentinel", "none", "mounted_weapon" };
static const char* const k_hud_corner_names[] = { "top_left", "top_right", "bottom_left", "bottom_right", "center" };

struct s_hs_enum_definition
{
	int16 count;
	const char* const* values;
};

static const s_hs_enum_definition k_hs_enums[] =
{
	{ NUMBEROF(k_game_difficulty_names), k_game_difficulty_names },
	{ NUMBEROF(k_team_names), k_team_names },
	{ NUMBEROF(k_ai_default_state_names), k_ai_default_state_names },
	{ NUMBEROF(k_actor_type_names), k_actor_type_names },
	{ NUMBEROF(k_hud_corner_names), k_hud_corner_names },
};

static const uint32 k_hs_tag_reference_group_tags[] = { 'snd!', 'effe', 'jpt!', 'lsnd', 'antr', 'actv', 'jpt!', 'obje' };

static struct
{
	const char* source;
	int32 source_size;
	int32 error_count;
} g_hs_compile;

/* ---------- private code */

static void hs_compile_error(const s_hs_syntax_node* expression, const char* error)
{
	if (g_hs_compile.error_count++ < 32)
	{
		h1_log("hs: postprocess: %s (\"%s\")", error,
			VALID_INDEX(expression->source_offset, g_hs_compile.source_size) ? g_hs_compile.source + expression->source_offset : "?");
	}
	return;
}

// hs_parse_scenario_datum: the element of a scenario block with this name
template<typename t_element>
static bool hs_parse_scenario_datum(s_hs_syntax_node* expression, const h1_tag_block<t_element>& block, uint32 name_offset)
{
	const char* name = g_hs_compile.source + expression->source_offset;
	for (int32 element_index = 0; element_index < block.count; element_index++)
	{
		const t_element* element = g_h1_cache_file->block_get(block, element_index);
		if (element && !_stricmp((const char*)element + name_offset, name))
		{
			expression->data = element_index;
			return true;
		}
	}
	char error[128];
	sprintf_s(error, "this is not a valid %s name", hs_type_names[expression->type]);
	hs_compile_error(expression, error);
	return false;
}

// ai_script.c ai_index_from_string: "encounter" or "encounter/squad" or "encounter/platoon"
static bool hs_parse_ai(s_hs_syntax_node* expression)
{
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();
	const char* string = g_hs_compile.source + expression->source_offset;
	int32 reference = NONE;
	if (!_stricmp(string, "none"))
	{
		expression->data = NONE;
		return true;
	}
	char encounter_name[64];
	const char* separator = strrchr(string, '/');
	strncpy_s(encounter_name, string, separator ? MIN((size_t)(separator - string), sizeof(encounter_name) - 1) : _TRUNCATE);
	for (int32 encounter_index = 0; encounter_index < scenario->encounters.count; encounter_index++)
	{
		const h1_scnr_encounters* encounter = g_h1_cache_file->block_get(scenario->encounters, encounter_index);
		if (!encounter || _stricmp(encounter->name, encounter_name))
		{
			continue;
		}
		if (!separator)
		{
			reference = encounter_index & 0xFFFF;
			break;
		}
		for (int32 squad_index = 0; squad_index < encounter->squads.count; squad_index++)
		{
			const h1_scnr_encounters_squads* squad = g_h1_cache_file->block_get(encounter->squads, squad_index);
			if (squad && !_stricmp(squad->name, separator + 1))
			{
				reference = (1 << 30) | ((squad_index & 0xFF) << 16) | (encounter_index & 0xFFFF);
				break;
			}
		}
		for (int32 platoon_index = 0; reference == NONE && platoon_index < encounter->platoons.count; platoon_index++)
		{
			const h1_scnr_encounters_platoons* platoon = g_h1_cache_file->block_get(encounter->platoons, platoon_index);
			if (platoon && !_stricmp(platoon->name, separator + 1))
			{
				reference = (2 << 30) | ((platoon_index & 0xFF) << 16) | (encounter_index & 0xFFFF);
				break;
			}
		}
		break;
	}
	expression->data = reference;
	if (reference == NONE)
	{
		hs_compile_error(expression, "this is not a valid ai encounter or squad.");
		return false;
	}
	return true;
}

static bool hs_parse_object_name(s_hs_syntax_node* expression)
{
	const h1_scnr* scenario = g_h1_cache_file->scenario_get();
	const char* name = g_hs_compile.source + expression->source_offset;
	for (int16 name_index = 0; name_index < scenario->object_names.count; name_index++)
	{
		const h1_scnr_object_names* object_name = g_h1_cache_file->block_get(scenario->object_names, name_index);
		if (object_name && !_stricmp(object_name->name, name))
		{
			expression->data = name_index;
			return true;
		}
	}
	hs_compile_error(expression, "this is not a valid object name.");
	return false;
}

static bool hs_parse_variable(s_hs_syntax_node* expression)
{
	const int16 global_index = hs_find_global_by_name(g_hs_compile.source + expression->source_offset);
	if (global_index == NONE)
	{
		hs_compile_error(expression, "this is not a valid variable name.");
		return false;
	}
	expression->data = global_index;
	if (expression->type && !hs_can_cast(hs_global_get_type(global_index), expression->type))
	{
		hs_compile_error(expression, "the variable's type is inconsistent with its usage.");
		return false;
	}
	return true;
}

static bool hs_parse_primitive(s_hs_syntax_node* expression)
{
	if (TEST_BIT(expression->flags, _hs_syntax_node_variable_bit))
	{
		return hs_parse_variable(expression);
	}

	const h1_scnr* scenario = g_h1_cache_file->scenario_get();
	const char* source = g_hs_compile.source + expression->source_offset;
	const int16 type = expression->type;
	switch (type)
	{
	case _hs_type_string:
		expression->data = (int32)source;
		return true;
	case _hs_type_script:
	{
		const int16 script_index = hs_find_script_by_name(source);
		if (script_index == NONE)
		{
			hs_compile_error(expression, "this is not a valid script name.");
			return false;
		}
		expression->data = script_index;
		return true;
	}
	case _hs_type_trigger_volume:
		return hs_parse_scenario_datum(expression, scenario->trigger_volumes, offsetof(h1_scnr_trigger_volumes, name));
	case _hs_type_cutscene_flag:
		return hs_parse_scenario_datum(expression, scenario->cutscene_flags, 4);
	case _hs_type_cutscene_camera_point:
		return hs_parse_scenario_datum(expression, scenario->cutscene_camera_points, 4);
	case _hs_type_cutscene_title:
		return hs_parse_scenario_datum(expression, scenario->cutscene_titles, 4);
	case _hs_type_cutscene_recording:
		return hs_parse_scenario_datum(expression, scenario->recorded_animations, 0);
	case _hs_type_device_group:
		return hs_parse_scenario_datum(expression, scenario->device_groups, 0);
	case _hs_type_ai:
		return hs_parse_ai(expression);
	case _hs_type_ai_command_list:
		return hs_parse_scenario_datum(expression, scenario->command_lists, 0);
	case _hs_type_starting_profile:
		return hs_parse_scenario_datum(expression, scenario->player_starting_profile, 0);
	case _hs_type_conversation:
		return hs_parse_scenario_datum(expression, scenario->ai_conversations, 0);
	case _hs_type_navpoint:
	case _hs_type_hud_message:
		// indices into the hud globals' waypoint arrows and the scenario's hud messages, as tool compiled them
		return true;
	case _hs_type_object_list:
		return hs_parse_object_name(expression);
	}
	if (HS_TYPE_IS_TAG_REFERENCE(type))
	{
		// the tag of the scenario's script references with this name
		const uint32 group_tag = k_hs_tag_reference_group_tags[type - _hs_type_sound];
		expression->data = NONE;
		for (int32 reference_index = 0; reference_index < scenario->references.count; reference_index++)
		{
			const h1_scnr_references* reference = g_h1_cache_file->block_get(scenario->references, reference_index);
			if (!reference || reference->reference.index == NONE)
			{
				continue;
			}
			const char* name = g_h1_cache_file->tag_name_get(reference->reference.index);
			if (name && !strcmp(name, source) && g_h1_cache_file->tag_get(group_tag, reference->reference.index))
			{
				expression->data = reference->reference.index;
				break;
			}
		}
		if (expression->data == NONE)
		{
			hs_compile_error(expression, "this tag isn't one of the scenario's script references.");
		}
		return true;
	}
	if (HS_TYPE_IS_ENUM(type))
	{
		const s_hs_enum_definition* definition = &k_hs_enums[type - _hs_type_enum_game_difficulty];
		for (int16 value_index = 0; value_index < definition->count; value_index++)
		{
			if (!_stricmp(source, definition->values[value_index]))
			{
				expression->data = value_index;
				return true;
			}
		}
		hs_compile_error(expression, "this is not a valid enum value.");
		return false;
	}
	if (HS_TYPE_IS_OBJECT(type))
	{
		if (!strcmp(source, "none"))
		{
			expression->data = NONE;
			return true;
		}
		return hs_parse_object_name(expression);
	}
	if (HS_TYPE_IS_OBJECT_NAME(type))
	{
		return hs_parse_object_name(expression);
	}
	return true;
}

/* ---------- public code */

bool hs_compile_postprocess(void)
{
	g_hs_compile.source = hs_string_data(&g_hs_compile.source_size);
	g_hs_compile.error_count = 0;

	int32 node_count = 0;
	for (int32 expression_index = hs_syntax_next_index(NONE); expression_index != NONE; expression_index = hs_syntax_next_index(expression_index))
	{
		s_hs_syntax_node* expression = hs_syntax_get(expression_index);
		node_count++;
		if (!HS_TYPE_VALID(expression->type))
		{
			if (expression->type != _hs_function_name)
			{
				hs_compile_error(expression, "missing type");
			}
			continue;
		}

		int16 resolved_type;
		if (TEST_BIT(expression->flags, _hs_syntax_node_primitive_bit))
		{
			if (expression->type >= _hs_type_string || TEST_BIT(expression->flags, _hs_syntax_node_variable_bit))
			{
				if (!VALID_INDEX(expression->source_offset, g_hs_compile.source_size))
				{
					hs_compile_error(expression, "bad source offset");
					continue;
				}
				hs_parse_primitive(expression);
			}
			resolved_type = TEST_BIT(expression->flags, _hs_syntax_node_variable_bit) ?
				hs_global_get_type((int16)expression->data) :
				expression->constant_type;
		}
		else if (TEST_BIT(expression->flags, _hs_syntax_node_script_bit))
		{
			resolved_type = hs_script_get(expression->script_index)->return_type;
		}
		else
		{
			// a function call: its predicate's name
			const s_hs_syntax_node* predicate = hs_syntax_get(expression->data);
			if (predicate->type != _hs_function_name || !VALID_INDEX(predicate->source_offset, g_hs_compile.source_size))
			{
				hs_compile_error(expression, "corrupt syntax tree");
				continue;
			}
			const int16 function_index = hs_find_function_by_name(g_hs_compile.source + predicate->source_offset);
			if (function_index == NONE)
			{
				hs_compile_error(predicate, "missing function");
				expression->function_index = NONE;
				continue;
			}
			expression->function_index = function_index;
			resolved_type = hs_function_get(function_index)->return_type;
		}

		if ((!HS_TYPE_VALID(resolved_type) && resolved_type != _hs_passthrough) || !hs_can_cast(resolved_type, expression->type))
		{
			hs_compile_error(expression, "type is inconsistent with usage");
		}
	}

	// the library functions the scripts call that do nothing yet
	std::set<int16> missing;
	for (int32 expression_index = hs_syntax_next_index(NONE); expression_index != NONE; expression_index = hs_syntax_next_index(expression_index))
	{
		const s_hs_syntax_node* expression = hs_syntax_get(expression_index);
		if (HS_TYPE_VALID(expression->type) && !TEST_BIT(expression->flags, _hs_syntax_node_primitive_bit) && !TEST_BIT(expression->flags, _hs_syntax_node_script_bit))
		{
			const s_hs_function_definition* function = hs_function_get(expression->function_index);
			if (function && function->evaluate == hs_evaluate_generic && !function->procedure)
			{
				missing.insert(expression->function_index);
			}
		}
	}
	std::string names;
	for (int16 function_index : missing)
	{
		names += " ";
		names += hs_function_get(function_index)->name;
		if (names.size() > 900)
		{
			h1_log("hs: unimplemented:%s", names.c_str());
			names.clear();
		}
	}
	h1_log("hs: postprocess: %d nodes, %d errors, %d unimplemented functions used%s%s", node_count, g_hs_compile.error_count, (int32)missing.size(),
		names.empty() ? "" : ":", names.c_str());
	return true;
}

} // namespace h1_hs

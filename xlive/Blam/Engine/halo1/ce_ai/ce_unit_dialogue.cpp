#include "stdafx.h"
#include "h1_ai_internal.h"

/*
* unit_dialogue.c: the functions of it the AI calls.
*/

namespace h1_ai
{

/* ---------- units/unit_dialogue.c: its declarations */

/* ---------- headers */

/* ---------- constants */

enum unit_play_speech_type
{
	_unit_play_speech_none = 0,
	_unit_play_speech_queue,
	_unit_play_speech_immediate,
	_unit_play_speech_immediate_dequeue,
};

enum unit_dialogue_vocalization_type
{
	_vocalization_pain_body = 6,
	_vocalization_pain_body_major = 7,
	_vocalization_pain_shield = 8,
	_vocalization_pain_falling = 9,
	_vocalization_scream_fear = 10,
	_vocalization_scream_pain = 11,
	_vocalization_maimed_limb = 12,
	_vocalization_maimed_head = 13,
	_vocalization_death_quiet = 14,
	_vocalization_death_violent = 15,
	_vocalization_death_falling = 16,
	_vocalization_death_agonizing = 17,
	_vocalization_death_instant = 18,
	_vocalization_death_flying = 19,
	_vocalization_hurt_enemy_grenade = 39,
	_vocalization_resurrect = 183,
};

enum unit_dialogue_damage_category
{
	_unit_dialogue_damage_category_none = 0,
	_unit_dialogue_damage_category_falling = 1,
	_unit_dialogue_damage_category_flame = 7,
};

enum unit_dialogue_actor_combat_status
{
	_unit_dialogue_actor_combat_status_definite = 3,
};

enum unit_dialogue_ai_unit_effect
{
	_unit_dialogue_ai_unit_effect_death_scream = 2,
};

/* ---------- macros */

#define AI_BEHAVIOR(field) \
	(game_connection() == _game_connection_local && ai_debug.field)
#define NUMBER_OF_VOCALIZATION_TYPES NUMBER_OF_DIALOGUE_VOCALIZATION_TYPES

/* ---------- structures */

struct dialogue_definition
{
	short vocalization_enum_version;
	word pad;
	long unused[3];
	struct tag_reference vocalizations[NUMBER_OF_DIALOGUE_VOCALIZATION_TYPES];
	struct tag_reference unused_vocalizations[47];
};

/* ---------- prototypes */

static long unit_find_dialogue_variant(
	struct unit_definition *definition,
	short variant_number);
static void unit_dialogue_setup(
	long unit_index);
static void unit_lose_speech(
	long unit_index,
	short play_type,
	struct unit_speech_item const *speech_item);

/* ---------- globals */

extern short const dialogue_vocalization_lookup[NUMBER_OF_DIALOGUE_VOCALIZATION_TYPES];

char const *global_speech_priority_names[NUMBER_OF_UNIT_SPEECH_PRIORITIES] =
{
	"none",
	"idle",
	"pain",
	"talk",
	"communicate",
	"shout",
	"script",
	"involuntary",
	"exclaim",
	"scream",
	"death",
};

short const global_speech_override_priorities[NUMBER_OF_UNIT_SPEECH_PRIORITIES] =
{
	_unit_speech_none,
	_unit_speech_none,
	_unit_speech_idle,
	_unit_speech_idle,
	_unit_speech_pain,
	_unit_speech_pain,
	_unit_speech_shout,
	_unit_speech_shout,
	_unit_speech_involuntary,
	_unit_speech_involuntary,
	_unit_speech_death,
};

real const global_speech_queue_times[NUMBER_OF_UNIT_SPEECH_PRIORITIES] =
{
	0.0f,
	0.0f,
	0.0f,
	1.5f,
	3.0f,
	4.0f,
	REAL_MAX,
	3.0f,
	3.0f,
	3.0f,
	3.0f,
};

static long sequential_counter;

/* ---------- public code */

/* ---------- private code */

/* ---------- units/unit_dialogue.c: its functions */

char const *unit_get_speech_priority_name(
	short priority)
{
	char const *name = "<error>";

	if (priority >= _unit_speech_none && priority < NUMBER_OF_UNIT_SPEECH_PRIORITIES)
		name = global_speech_priority_names[priority];

	return name;
}

char const *unit_describe_speech(
	long unit_index,
	boolean abbreviated,
	long buffer_size,
	char *buffer)
{
	struct unit_datum *unit = unit_get(unit_index);
	char *sound_name;
	char *separator;
	char *scan;

	if (unit->unit.speech.current.priority == _unit_speech_none)
	{
		_snprintf(buffer, (short)buffer_size, "<none>");
		return buffer;
	}

	sound_name = "<none>";
	if (unit->unit.speech.current.sound_definition_index != NONE)
	{
		sound_name = (char *)tag_get_name(
			unit->unit.speech.current.sound_definition_index);
	}

	if (abbreviated)
	{
		boolean done = FALSE;

		scan = sound_name;
		while (!done)
		{
			if (scan)
			{
				scan = strchr(scan, '\\');
				if (scan)
				{
					scan++;
					sound_name = scan;
				}
				else
				{
					done = TRUE;
				}
			}
			else
			{
				done = TRUE;
			}
		}
	}
	else
	{
		separator = strrchr(sound_name, '\\');
		if (separator)
			sound_name = separator + 1;
	}

	if (unit->unit.speech.current.vocalization_type == NONE)
	{
		_snprintf(buffer, (short)buffer_size, "%s", sound_name);
	}
	else if (abbreviated)
	{
		_snprintf(
			buffer,
			(short)buffer_size,
			"%s %s",
			dialogue_get_vocalization_name(
				unit->unit.speech.current.vocalization_type,
				FALSE),
			sound_name);
	}
	else
	{
		_snprintf(
			buffer,
			(short)buffer_size,
			"%s",
			dialogue_get_vocalization_name(
				unit->unit.speech.current.vocalization_type,
				FALSE));
	}

	return buffer;
}

} // namespace h1_ai

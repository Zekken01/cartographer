#include "stdafx.h"

#include "../h1_cache_file.h"
#include "../h1_hs.h"
#include "../h1_log.h"
#include "../h1_map_loader.h"
#include "../h1_objects.h"
#include "../h1_scenario_objects.h"

#include "game/game.h"
#include "game/game_time.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "units/bipeds.h"
#include "units/units.h"
#include "../h2_tag_definitions_generated.h"

#include "h1_ai_internal.h"
#include "h1_ai_objects.h"

#include <unordered_map>
#include <vector>

namespace h1_ai
{

/*
* objects.c over carto: the AI reads halo 1 object datums. Every halo 2 object has a mirror in halo 1's layout, filled from halo 2
* at the start of each AI tick (h1_ai_objects_update) and when the AI first asks for an object made since. The fields only the AI
* writes (a unit's actor and swarm indices, its fake encounter and squad) live in the mirror and stay. object_header_data mirrors
* halo 2's object headers (the same datum indices) for halo 1's header tests.
*/

/* ---------- types */

union u_object_mirror_data
{
	object_datum object;
	unit_datum unit;
	biped_datum biped;
	vehicle_datum vehicle;
	item_datum item;
	weapon_datum weapon;
	projectile_datum projectile;
	device_datum device;
	machine_datum machine;
};

struct s_object_mirror
{
	u_object_mirror_data data;
	int32 sync_time;
	bool seen;
	// halo 1 controlled the unit the last AI tick
	bool controlled;
};

/* ---------- globals */

data_array* object_header_data = NULL;

static std::unordered_map<datum, s_object_mirror> g_object_mirrors;
static std::unordered_map<datum, int16> g_object_name_indices;
static int32 g_object_mirror_time = NONE;

/* ---------- private prototypes */

static void object_mirror_sync(datum object_index, s_object_mirror* mirror);
static void object_header_mirror_set(datum object_index, const ::object_header_datum* h2_header, s_object_mirror* mirror);
static s_object_mirror* object_mirror_get(datum object_index);
static int16 h1_object_type_get(datum h1_definition_index, int8 h2_type);
static void location_from_point(const real_point3d* point, struct location* location);
static void biped_ground_sync(datum biped_index, s_object_mirror* mirror, const real_point3d* previous_position);

/* ---------- public code */

int32 h1_ai_game_time(void)
{
	// halo 1's 30 ticks a second of halo 2's game time
	return (int32)((int64)::game_time_get() * TICKS_PER_SECOND / MAX(::game_tick_rate(), 1));
}

void h1_ai_objects_initialize_for_new_map(void)
{
	g_object_mirrors.clear();
	g_object_name_indices.clear();
	g_object_mirror_time = NONE;
	if (!object_header_data)
	{
		object_header_data = game_state_data_new("object", ::object_header_data_get()->maximum_count, sizeof(object_header_datum));
	}
	data_make_valid(object_header_data);
	return;
}

void h1_ai_objects_update(void)
{
	g_object_mirror_time = h1_ai_game_time();

	// the scenario's object names of the objects that have them
	g_object_name_indices.clear();
	const scenario* scenario_definition = global_scenario_get();
	for (int16 name_index = 0; scenario_definition && name_index < scenario_definition->object_names.count; name_index++)
	{
		const datum object_index = h1_hs_object_index_from_name_index(name_index);
		if (object_index != NONE)
		{
			g_object_name_indices[object_index] = name_index;
		}
	}

	for (auto& entry : g_object_mirrors)
	{
		entry.second.seen = false;
	}

	// halo 2's object headers, at the same absolute indices
	::data_array* h2_headers = ::object_header_data_get();
	csmemset(object_header_data->data, 0, (size_t)object_header_data->maximum_count * object_header_data->size);
	object_header_data->count = 0;
	object_header_data->actual_count = 0;
	::data_iterator iterator;
	::iterator_new(&iterator, h2_headers);
	while (const ::object_header_datum* h2_header = (const ::object_header_datum*)::iterator_next(&iterator))
	{
		const datum object_index = iterator.index;
		const int16 absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index);
		if (absolute_index >= object_header_data->maximum_count || !h2_header->datum)
		{
			continue;
		}
		s_object_mirror* mirror = object_mirror_get(object_index);
		if (!mirror || mirror->data.object.definition_index == NONE)
		{
			continue;
		}
		object_header_mirror_set(object_index, h2_header, mirror);
	}

	// mirrors of objects that are gone go with them
	for (auto it = g_object_mirrors.begin(); it != g_object_mirrors.end();)
	{
		it = it->second.seen ? std::next(it) : g_object_mirrors.erase(it);
	}
	return;
}

void h1_ai_units_control_update(void)
{
	for (auto& entry : g_object_mirrors)
	{
		s_object_mirror* mirror = &entry.second;
		if (mirror->seen && TEST_FLAG(_object_mask_unit, mirror->data.object.object.type))
		{
			h1_ai_unit_control_update(entry.first, &mirror->data.unit, &mirror->controlled);
		}
	}
	return;
}

void h1_ai_object_mirror_forget(datum object_index)
{
	g_object_mirrors.erase(object_index);
	return;
}

/* ---------- objects.c */

void* object_try_and_get_and_verify_type(long object_index, unsigned long valid_type_flags)
{
	s_object_mirror* mirror = object_index != NONE ? object_mirror_get(object_index) : NULL;
	if (!mirror || !TEST_FLAG(valid_type_flags, mirror->data.object.object.type))
	{
		return NULL;
	}
	return &mirror->data;
}

void* object_get_and_verify_type(long object_index, unsigned long valid_type_flags)
{
	void* object = object_try_and_get_and_verify_type(object_index, valid_type_flags);
	if (!object)
	{
		ai_assert_failed(__FILE__, __LINE__, "object_get_and_verify_type", NULL);
	}
	return object;
}

void object_iterator_new(struct object_iterator* iterator, unsigned long type_flags, byte flags)
{
	iterator->type_flags = type_flags;
	iterator->flags = flags;
	iterator->absolute_index = 0;
	iterator->index = NONE;
	iterator->signature = OBJECT_ITERATOR_SIGNATURE;
	return;
}

void* object_iterator_next(struct object_iterator* iterator)
{
	while (iterator->absolute_index < object_header_data->count)
	{
		const int16 absolute_index = iterator->absolute_index++;
		object_header_datum* header = (object_header_datum*)((uint8*)object_header_data->data + absolute_index * object_header_data->size);
		if (header->identifier == 0 || !header->datum || !TEST_FLAG(iterator->type_flags, header->type))
		{
			continue;
		}
		// flags bit 0: active objects only
		if (TEST_FLAG(iterator->flags, 0) && !TEST_FLAG(header->flags, _object_header_active_bit))
		{
			continue;
		}
		iterator->index = DATUM_INDEX_NEW(absolute_index, header->identifier);
		return header->datum;
	}
	iterator->index = NONE;
	return NULL;
}

real_point3d* object_get_origin(long object_index, real_point3d* origin)
{
	::object_get_origin(object_index, (::real_point3d*)origin, false);
	return origin;
}

void object_get_velocities(long object_index, real_vector3d* translational_velocity, real_vector3d* angular_velocity)
{
	// halo 2's velocities are a second's, halo 1's a tick's
	::real_vector3d linear, angular;
	::object_get_velocities(object_index, &linear, &angular);
	if (translational_velocity)
	{
		*translational_velocity = { linear.i / TICKS_PER_SECOND, linear.j / TICKS_PER_SECOND, linear.k / TICKS_PER_SECOND };
	}
	if (angular_velocity)
	{
		*angular_velocity = { angular.i / TICKS_PER_SECOND, angular.j / TICKS_PER_SECOND, angular.k / TICKS_PER_SECOND };
	}
	return;
}

real_matrix4x3* object_get_node_matrix(long object_index, short node_index)
{
	return (real_matrix4x3*)::object_get_node_matrix(object_index, node_index);
}

real_matrix4x3* object_get_node_matrices(long object_index)
{
	int32 node_count;
	return (real_matrix4x3*)::object_get_node_matrices(object_index, &node_count);
}

void object_get_orientation(long object_index, real_vector3d* forward, real_vector3d* up)
{
	const ::object_datum* object = (const ::object_datum*)::object_try_and_get_and_verify_type(object_index, -1);
	if (object && forward)
	{
		*forward = *(const real_vector3d*)&object->object.forward;
	}
	if (object && up)
	{
		*up = *(const real_vector3d*)&object->object.up;
	}
	return;
}

// halo 2 names halo 1's markers with underscores for spaces; no marker is the object's first node (the origin for "")
short object_get_marker_by_name(long object_index, char const* name, struct object_marker* markers, short maximum_marker_count)
{
	ASSERT(maximum_marker_count > 0);
	char marker_name[32] = "";
	strncpy_s(marker_name, name ? name : "", _TRUNCATE);
	for (char* c = marker_name; *c; c++)
	{
		if (*c == ' ')
		{
			*c = '_';
		}
	}
	short marker_count = 0;
	if (marker_name[0])
	{
		::object_marker h2_markers[8];
		marker_count = ::object_get_markers_by_string_id(object_index, ::string_id_find_or_add(marker_name), h2_markers, (int16)MIN(maximum_marker_count, NUMBEROF(h2_markers)));
		for (short i = 0; i < marker_count; i++)
		{
			markers[i].node_index = h2_markers[i].node_index;
			markers[i].node_matrix = *(const real_matrix4x3*)&h2_markers[i].node_matrix;
			markers[i].matrix = *(const real_matrix4x3*)&h2_markers[i].matrix;
		}
	}
	if (marker_count == 0)
	{
		markers[0].node_index = 0;
		matrix4x3_identity(&markers[0].node_matrix);
		const real_matrix4x3* node_matrix = object_get_node_matrix(object_index, 0);
		if (node_matrix)
		{
			markers[0].matrix = *node_matrix;
		}
		else
		{
			object_get_world_matrix(object_index, &markers[0].matrix);
		}
		if (name && name[0] == '\0')
		{
			marker_count = 1;
		}
	}
	return marker_count;
}

real_matrix4x3* object_get_world_matrix(long object_index, real_matrix4x3* matrix)
{
	const ::object_datum* object = (const ::object_datum*)::object_try_and_get_and_verify_type(object_index, -1);
	if (!object)
	{
		return NULL;
	}
	matrix->scale = object->object.scale;
	matrix->forward = *(const real_vector3d*)&object->object.forward;
	matrix->up = *(const real_vector3d*)&object->object.up;
	cross_product3d(&matrix->up, &matrix->forward, &matrix->left);
	matrix->position = *(const real_point3d*)&object->object.position;
	return matrix;
}

long object_get_ultimate_parent(long object_index)
{
	return ::object_get_ultimate_parent(object_index);
}

void object_activate(long object_index)
{
	::object_activate(object_index);
	return;
}

void object_deactivate(long object_index)
{
	::object_deactivate(object_index);
	return;
}

void object_set_automatic_deactivation(long object_index, boolean automatic_deactivation)
{
	// halo 2 deactivates objects on its own terms
	return;
}

void object_delete(long object_index)
{
	::object_delete(object_index);
	h1_ai_object_mirror_forget(object_index);
	return;
}

void object_delete_immediately(long object_index)
{
	object_delete(object_index);
	return;
}

long object_index_from_name_index(short name_index)
{
	return h1_hs_object_index_from_name_index(name_index);
}

void object_set_object_index_for_name_index(short name_index, long object_index)
{
	h1_hs_object_name_set(name_index, object_index);
	return;
}

void objects_garbage_collection(void)
{
	return;
}

short object_get_first_cluster(struct object_cluster_iterator* iterator, long object_index)
{
	// an object's cluster: the one its origin is in
	const object_datum* object = (const object_datum*)object_try_and_get_and_verify_type(object_index, _object_mask_all);
	csmemset(iterator, 0, sizeof(*iterator));
	return object ? object->object.location.cluster_index : (short)NONE;
}

short object_get_next_cluster(struct object_cluster_iterator* iterator, long object_index)
{
	return NONE;
}

short objects_in_sphere(unsigned long class_flags, unsigned long type_flags, struct location const* location, real_point3d const* center,
	real radius, long* object_indices, short maximum_count)
{
	// objects_in_clusters_by_indices' class flags: bit 0 the collideable objects (with collision models), bit 1 the others
	if (!class_flags)
	{
		class_flags = NONE;
	}
	short count = 0;
	object_iterator iterator;
	object_iterator_new(&iterator, type_flags ? type_flags : NONE, 0);
	while (const object_datum* object = (const object_datum*)object_iterator_next(&iterator))
	{
		if (count >= maximum_count)
		{
			break;
		}
		const bool collideable = TEST_FLAG(object->object.flags, _object_has_collision_model_bit);
		if (object->definition_index == NONE || !TEST_FLAG(class_flags, collideable ? 0 : 1))
		{
			continue;
		}
		const real_point3d* object_center = &object->object.bounding_sphere_center;
		const real reach = radius + object->object.bounding_sphere_radius;
		const real dx = object_center->x - center->x, dy = object_center->y - center->y, dz = object_center->z - center->z;
		if (dx * dx + dy * dy + dz * dz <= reach * reach)
		{
			object_indices[count++] = iterator.index;
		}
	}
	return count;
}

// objects.c's cluster partitions: the objects of a cluster, colliding (with a collision model) and not
static long cluster_object_next(long* reference_index, short cluster_index, boolean collideable)
{
	for (long absolute_index = *reference_index; absolute_index < object_header_data->count; absolute_index++)
	{
		const object_header_datum* header = (const object_header_datum*)((uint8*)object_header_data->data + absolute_index * object_header_data->size);
		if (header->identifier == 0 || !header->datum || header->cluster_index != cluster_index)
		{
			continue;
		}
		const boolean has_collision = TEST_FLAG(header->datum->object.flags, _object_has_collision_model_bit);
		if ((has_collision != 0) != (collideable != 0))
		{
			continue;
		}
		*reference_index = absolute_index + 1;
		return DATUM_INDEX_NEW(absolute_index, header->identifier);
	}
	*reference_index = object_header_data->count;
	return NONE;
}

static short g_cluster_iteration_cluster = NONE;

long cluster_get_first_collideable_object(long* reference_index, short cluster_index)
{
	*reference_index = 0;
	g_cluster_iteration_cluster = cluster_index;
	return cluster_object_next(reference_index, cluster_index, TRUE);
}

long cluster_get_next_collideable_object(long* reference_index)
{
	return cluster_object_next(reference_index, g_cluster_iteration_cluster, TRUE);
}

long cluster_get_first_noncollideable_object(long* reference_index, short cluster_index)
{
	*reference_index = 0;
	g_cluster_iteration_cluster = cluster_index;
	return cluster_object_next(reference_index, cluster_index, FALSE);
}

long cluster_get_next_noncollideable_object(long* reference_index)
{
	return cluster_object_next(reference_index, g_cluster_iteration_cluster, FALSE);
}

void object_placement_data_new(struct object_placement_data* data, long definition_index, long owner_object_index)
{
	csmemset(data, 0, sizeof(*data));
	data->definition_index = definition_index;
	data->owner_player_index = NONE;
	data->owner_object_index = owner_object_index;
	data->owner_object_definition_index = NONE;
	data->owner_team_index = NONE;
	data->forward = { 1.f, 0.f, 0.f };
	data->up = { 0.f, 0.f, 1.f };
	data->change_colors[0] = { -1.f, -1.f, -1.f };
	return;
}

long object_new(struct object_placement_data* data)
{
	// the halo 2 definition of the halo 1 tag
	const datum definition_index = h1_scenario_object_definition_get(data->definition_index);
	if (definition_index == NONE)
	{
		return NONE;
	}
	::s_damage_owner damage_owner;
	damage_owner.owner_player_index = data->owner_player_index;
	damage_owner.owner_object_index = data->owner_object_index;
	damage_owner.owner_team_index = (::e_game_team)data->owner_team_index;
	damage_owner.pad = 0;
	::object_placement_data placement;
	::object_placement_data_new(&placement, definition_index, data->owner_object_index, &damage_owner);
	placement.position = *(const ::real_point3d*)&data->position;
	placement.forward = *(const ::real_vector3d*)&data->forward;
	placement.up = *(const ::real_vector3d*)&data->up;
	placement.translational_velocity = { data->translational_velocity.i * TICKS_PER_SECOND, data->translational_velocity.j * TICKS_PER_SECOND, data->translational_velocity.k * TICKS_PER_SECOND };
	placement.angular_velocity = { data->angular_velocity.i * TICKS_PER_SECOND, data->angular_velocity.j * TICKS_PER_SECOND, data->angular_velocity.k * TICKS_PER_SECOND };
	const datum object_index = ::object_new(&placement);
	if (object_index == NONE)
	{
		return NONE;
	}
	h1_scenario_object_type_set(object_index, data->definition_index);
	s_object_mirror* mirror = object_mirror_get(object_index);
	if (mirror)
	{
		mirror->data.object.object.variant_number = data->variant_number;
	}
	return object_index;
}

void object_set_position(long object_index, real_point3d const* position, real_vector3d const* forward, real_vector3d const* up)
{
	// halo 2 needs an up whenever there is a forward
	Memory::GetAddress<void(__cdecl*)(datum, const ::real_point3d*, const ::real_vector3d*, const ::real_vector3d*, int32)>(0x136B7F)(
		object_index, (const ::real_point3d*)position, (const ::real_vector3d*)forward, (const ::real_vector3d*)(up ? up : (forward ? (const real_vector3d*)global_up3d : NULL)), 0);
	return;
}

void object_initialize_vitality(long object_index, real* custom_body_vitality, real* custom_shield_vitality)
{
	::object_datum* object = (::object_datum*)::object_try_and_get_and_verify_type(object_index, -1);
	if (!object)
	{
		return;
	}
	if (custom_body_vitality)
	{
		object->object.body_vitality = *custom_body_vitality;
	}
	if (custom_shield_vitality)
	{
		object->object.shield_vitality = *custom_shield_vitality;
	}
	return;
}

/* ---------- objects.c: the objects a search has visited (the mirrors' magic numbers) */

static uint32 g_object_marker = 0;
static bool g_object_marker_initialized = false;

void object_marker_begin(void)
{
	ASSERT(!g_object_marker_initialized);
	++g_object_marker;
	g_object_marker_initialized = true;
	return;
}

void object_marker_end(void)
{
	ASSERT(g_object_marker_initialized);
	g_object_marker_initialized = false;
	return;
}

boolean object_unmarked_function(long object_index)
{
	const object_datum* object = (const object_datum*)object_get_and_verify_type(object_index, _object_mask_all);
	return (boolean)(object->object.magic_number != g_object_marker);
}

boolean object_mark_function(long object_index)
{
	object_datum* object = (object_datum*)object_get_and_verify_type(object_index, _object_mask_all);
	if (object->object.magic_number != g_object_marker)
	{
		object->object.magic_number = g_object_marker;
		return TRUE;
	}
	return FALSE;
}

/* ---------- scenario locations */

void scenario_location_from_point(struct location* location, const real_point3d* point)
{
	location_from_point(point, location);
	return;
}

/* ---------- private code */

// the object's halo 1 header, at its absolute index (object_header_data's count covers it)
static void object_header_mirror_set(datum object_index, const ::object_header_datum* h2_header, s_object_mirror* mirror)
{
	const int16 absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index);
	if (!object_header_data || absolute_index >= object_header_data->maximum_count)
	{
		return;
	}
	object_header_datum* header = (object_header_datum*)((uint8*)object_header_data->data + absolute_index * object_header_data->size);
	const bool new_header = header->identifier == 0;
	header->identifier = h2_header->identifier;
	header->flags = 0;
	SET_FLAG(header->flags, _object_header_active_bit, h2_header->flags.test(::_object_header_active_bit));
	SET_FLAG(header->flags, _object_header_visible_bit, !h2_header->datum->object.flags.test(::_object_hidden_bit));
	SET_FLAG(header->flags, _object_header_being_deleted_bit, h2_header->flags.test(::_object_header_being_deleted_bit));
	SET_FLAG(header->flags, _object_header_connected_to_map_bit, h2_header->flags.test(::_object_header_connected_to_map_bit));
	SET_FLAG(header->flags, _object_header_child_bit, h2_header->flags.test(::_object_header_child_bit));
	header->type = (byte)mirror->data.object.object.type;
	header->cluster_index = mirror->data.object.object.location.cluster_index;
	header->data_size = sizeof(u_object_mirror_data);
	header->datum = &mirror->data.object;
	if (new_header)
	{
		object_header_data->actual_count++;
	}
	if (absolute_index + 1 > object_header_data->count)
	{
		object_header_data->count = absolute_index + 1;
	}
	return;
}

static s_object_mirror* object_mirror_get(datum object_index)
{
	const ::object_datum* object = (const ::object_datum*)::object_try_and_get_and_verify_type(object_index, -1);
	if (!object)
	{
		return NULL;
	}
	auto found = g_object_mirrors.find(object_index);
	if (found == g_object_mirrors.end())
	{
		s_object_mirror created;
		csmemset(&created, 0, sizeof(created));
		created.sync_time = NONE;
		// what the AI owns starts empty
		created.data.unit.unit.actor_index = NONE;
		created.data.unit.unit.swarm_actor_index = NONE;
		created.data.unit.unit.swarm_next_unit_index = NONE;
		created.data.unit.unit.swarm_prev_unit_index = NONE;
		created.data.unit.unit.fake_encounter_index = NONE;
		created.data.unit.unit.fake_squad_index = NONE;
		created.data.unit.unit.dialogue_index = NONE;
		created.data.biped.biped.support_surface_index = NONE;
		created.data.biped.biped.pathfinding_surface_index = NONE;
		created.data.biped.biped.last_pathfinding_surface_index = NONE;
		found = g_object_mirrors.emplace(object_index, created).first;
	}
	s_object_mirror* mirror = &found->second;
	mirror->seen = true;
	if (mirror->sync_time != g_object_mirror_time || g_object_mirror_time == NONE)
	{
		const bool first_sync = mirror->sync_time == NONE;
		object_mirror_sync(object_index, mirror);
		// units.c unit_new: a new unit picks its dialogue on its first update
		if (first_sync && TEST_FLAG(_object_mask_unit, mirror->data.object.object.type))
		{
			SET_FLAG(mirror->data.unit.unit.flags, _unit_must_set_up_dialogue_bit, TRUE);
		}
		mirror->sync_time = g_object_mirror_time;
		const ::object_header_datum* h2_header = (const ::object_header_datum*)::datum_try_and_get(::object_header_data_get(), object_index);
		// objects that aren't halo 1's (the host's) aren't the AI's
		if (h2_header && mirror->data.object.definition_index != NONE)
		{
			object_header_mirror_set(object_index, h2_header, mirror);
		}
	}
	return mirror;
}

static int16 h1_object_type_get(datum h1_definition_index, int8 h2_type)
{
	const h1_cache_file_tag_instance* instance = h1_definition_index != NONE && g_h1_cache_file ? g_h1_cache_file->tag_instance_get(h1_definition_index) : NULL;
	switch (instance ? instance->group_tag : 0)
	{
	case 'bipd': return _object_type_biped;
	case 'vehi': return _object_type_vehicle;
	case 'weap': return _object_type_weapon;
	case 'eqip': return _object_type_equipment;
	case 'garb': return _object_type_garbage;
	case 'proj': return _object_type_projectile;
	case 'scen': return _object_type_scenery;
	case 'mach': return _object_type_machine;
	case 'ctrl': return _object_type_control;
	case 'lifi': return _object_type_light_fixture;
	case 'plac': return _object_type_placeholder;
	case 'ssce': return _object_type_sound_scenery;
	}
	// halo 2's types: halo 1's up to light fixtures, sound scenery next, crates and creatures as scenery
	switch (h2_type)
	{
	case 10: return _object_type_sound_scenery;
	case 11: case 12: return _object_type_scenery;
	}
	return h2_type;
}

static void object_mirror_sync(datum object_index, s_object_mirror* mirror)
{
	const ::object_datum* h2_object = (const ::object_datum*)::object_try_and_get_and_verify_type(object_index, -1);
	const ::object_header_datum* h2_header = (const ::object_header_datum*)::datum_try_and_get(::object_header_data_get(), object_index);
	if (!h2_object || !h2_header)
	{
		return;
	}
	object_datum* object = &mirror->data.object;
	const datum h1_definition_index = h1_objects_h1_definition_get(h2_object->definition_index);
	object->definition_index = h1_definition_index;
	_object_datum* data = &object->object;
	data->type = h1_object_type_get(h1_definition_index, h2_header->type);

	const object_definition* definition = h1_definition_index != NONE ? (const object_definition*)tag_get('obje', h1_definition_index) : NULL;
	unsigned long flags = 0;
	SET_FLAG(flags, _object_invisible_bit, h2_object->object.flags.test(::_object_hidden_bit));
	SET_FLAG(flags, _object_connected_to_map_bit, h2_object->object.flags.test(::_object_connected_to_map_bit));
	SET_FLAG(flags, _object_outside_of_map_bit, h2_object->object.flags.test(::_object_outside_of_map_bit));
	SET_FLAG(flags, _object_has_collision_model_bit, definition && definition->object.collision_model.index != NONE);
	data->flags = flags;

	const real_point3d previous_position = data->position;
	data->position = *(const real_point3d*)&h2_object->object.position;
	data->forward = *(const real_vector3d*)&h2_object->object.forward;
	data->up = *(const real_vector3d*)&h2_object->object.up;
	const ::real_vector3d& linear = h2_object->object.translational_velocity;
	const ::real_vector3d& angular = h2_object->object.angular_velocity;
	data->translational_velocity = { linear.i / TICKS_PER_SECOND, linear.j / TICKS_PER_SECOND, linear.k / TICKS_PER_SECOND };
	data->angular_velocity = { angular.i / TICKS_PER_SECOND, angular.j / TICKS_PER_SECOND, angular.k / TICKS_PER_SECOND };
	location_from_point(&data->position, &data->location);
	data->bounding_sphere_center = *(const real_point3d*)&h2_object->object.center;
	data->bounding_sphere_radius = h2_object->object.radius;
	data->scale = h2_object->object.scale;
	auto name = g_object_name_indices.find(object_index);
	data->name_index = name != g_object_name_indices.end() ? name->second : (short)NONE;
	data->owner_player_index = NONE;
	data->owner_object_index = h2_object->object.damage_owner_object_index;
	data->owner_object_definition_index = NONE;
	data->maximum_body_vitality = h2_object->object.maximum_body_vitality;
	data->maximum_shield_vitality = h2_object->object.maximum_shield_vitality;
	data->body_vitality = h2_object->object.body_vitality;
	data->shield_vitality = h2_object->object.shield_vitality;
	data->current_body_damage = h2_object->object.current_body_damage;
	data->current_shield_damage = h2_object->object.current_shield_damage;
	data->recent_body_damage = h2_object->object.recent_body_damage;
	data->recent_shield_damage = h2_object->object.recent_shield_damage;
	SET_FLAG(data->damage_flags, _object_dead_bit, h2_object->object.object_damage_flags.test(::_object_is_dead_bit));
	SET_FLAG(data->damage_flags, _object_cannot_take_damage_bit, h2_object->object.object_damage_flags.test(::_object_is_immune_to_damage));
	data->next_object_index = h2_object->object.next_object_index;
	data->first_child_object_index = h2_object->object.first_child_object_index;
	data->parent_object_index = h2_object->object.parent_object_index;
	const short ai_team_index = data->owner_team_index;
	data->owner_team_index = NONE;

	if (TEST_FLAG(_object_mask_unit, data->type))
	{
		const ::unit_datum* h2_unit = (const ::unit_datum*)h2_object;
		_unit_datum* unit = &mirror->data.unit.unit;
		// an actor's unit is on its encounter's team (actor_set_team), which goes to halo 2 (h1_ai_unit_control_update)
		data->owner_team_index = unit->actor_index != NONE ? ai_team_index : (short)h2_unit->unit.unit_team;
		unit->player_index = h2_unit->unit.player_index;
		unit->desired_facing_vector = *(const real_vector3d*)&h2_unit->unit.desired_facing_vector;
		unit->desired_aiming_vector = *(const real_vector3d*)&h2_unit->unit.desired_aiming_vector;
		unit->aiming_vector = *(const real_vector3d*)&h2_unit->unit.aiming_vector;
		unit->aiming_velocity = *(const real_vector3d*)&h2_unit->unit.aiming_velocity;
		unit->desired_looking_vector = *(const real_vector3d*)&h2_unit->unit.desired_looking_vector;
		unit->looking_vector = *(const real_vector3d*)&h2_unit->unit.looking_vector;
		unit->looking_velocity = *(const real_vector3d*)&h2_unit->unit.looking_velocity;
		unit->throttle = *(const real_vector3d*)&h2_unit->unit.throttle;
		unit->primary_trigger = h2_unit->unit.primary_trigger;
		unit->aiming_speed = h2_unit->unit.aiming_speed;
		unit->parent_seat_index = h2_unit->unit.parent_seat_index;
		unit->current_weapon_index = h2_unit->unit.weapon_indices[0];
		unit->desired_weapon_index = h2_unit->unit.weapon_indices[0];
		for (int32 i = 0; i < MAXIMUM_WEAPONS_PER_UNIT && i < 4; i++)
		{
			unit->weapon_object_indices[i] = h2_unit->unit.weapon_object_indices[i];
		}
		unit->equipment_object_index = h2_unit->unit.equipment_object_index;
		unit->current_grenade_index = h2_unit->unit.current_grenade_index;
		unit->desired_grenade_index = h2_unit->unit.desired_grenade_index;
		for (int32 i = 0; i < NUMBER_OF_UNIT_GRENADE_TYPES; i++)
		{
			unit->grenade_counts[i] = h2_unit->unit.grenade_counts[i];
		}
		unit->current_zoom_level = h2_unit->unit.current_zoom_level;
		unit->desired_zoom_level = h2_unit->unit.desired_zoom_level;
		unit->last_vehicle_index = h2_unit->unit.last_vehicle_index;
		unit->game_time_at_last_vehicle_exit = h2_unit->unit.game_time_at_last_vehicle_exit;
		unit->seat_power[0] = h2_unit->unit.driver_seat_power;
		unit->seat_power[1] = h2_unit->unit.gunner_seat_power;
		unit->active_camouflage = h2_unit->unit.active_camouflage;
		unit->time_of_death = h2_unit->unit.time_of_death;
		unit->killing_spree_count = h2_unit->unit.killing_spree_count;
		SET_FLAG(unit->flags, _unit_active_camouflaged_bit, h2_unit->unit.active_camouflage > 0.f);

		if (data->type == _object_type_biped)
		{
			const ::biped_datum* h2_biped = (const ::biped_datum*)h2_object;
			_biped_datum* biped = &mirror->data.biped.biped;
			biped->crouch = h2_unit->unit.crouch;
			biped_ground_sync(object_index, mirror, &previous_position);
		}

		// a vehicle's driver and gunner: the riders in its driver and gunner seats
		unit->driver_object_index = NONE;
		unit->gunner_object_index = NONE;
		if (data->type == _object_type_vehicle)
		{
			const h2x_vehi* vehicle_definition = (const h2x_vehi*)::tag_get('vehi', h2_object->definition_index);
			for (datum child_index = h2_object->object.first_child_object_index; vehicle_definition && child_index != NONE;)
			{
				const ::unit_datum* rider = (const ::unit_datum*)::object_try_and_get_and_verify_type(child_index, FLAG(::_object_type_biped) | FLAG(::_object_type_vehicle));
				const ::object_datum* child = (const ::object_datum*)::object_try_and_get_and_verify_type(child_index, -1);
				if (rider && VALID_INDEX(rider->unit.parent_seat_index, vehicle_definition->seats.count))
				{
					const uint32 seat_flags = vehicle_definition->seats[rider->unit.parent_seat_index]->flags;
					if (TEST_BIT(seat_flags, 2) && unit->driver_object_index == NONE)
					{
						unit->driver_object_index = child_index;
					}
					if (TEST_BIT(seat_flags, 3) && unit->gunner_object_index == NONE)
					{
						unit->gunner_object_index = child_index;
					}
				}
				child_index = child ? child->object.next_object_index : NONE;
			}
		}
	}
	return;
}

// bipeds.c's support: halo 1's biped physics keeps the surface a biped stands on (none in the air, airborne_ticks counting), and
// forgets its pathfinding surface when it moves; halo 2's physics moves it, so its support is the structure under its feet
static void biped_ground_sync(datum biped_index, s_object_mirror* mirror, const real_point3d* previous_position)
{
	_biped_datum* biped = &mirror->data.biped.biped;
	const real_point3d* position = &mirror->data.object.object.position;
	struct collision_bsp* collision_bsp = global_collision_bsp_get();
	long surface_index = NONE;
	long ground_surface_index = NONE;
	real_point3d ground_point = *position;
	if (collision_bsp)
	{
		// bipeds.c biped_find_ground_surface (0.4 up, 2 down): the ground, supporting within a quarter unit below the feet
		const real_point3d origin = { position->x, position->y, position->z + 0.4f };
		const real_vector3d vector = { 0.f, 0.f, -2.f };
		struct collision_bsp_test_vector_result result;
		global_current_collision_users[global_current_collision_user_depth++] = _collision_user_bipeds;
		if (collision_bsp_test_vector(FLAG(_collision_test_front_facing_surfaces_bit), collision_bsp, 0, NULL, &origin, &vector, REAL_MAX, &result))
		{
			ground_surface_index = result.surface_index;
			ground_point = { origin.x, origin.y, origin.z + vector.k * result.t };
			if (-vector.k * result.t <= 0.65f)
			{
				surface_index = result.surface_index;
			}
		}
		--global_current_collision_user_depth;
	}
	biped->support_surface_index = surface_index;
	biped->airborne_ticks = surface_index != NONE ? 0 : (char)MIN(biped->airborne_ticks + 1, 127);
	real_vector3d moved;
	vector_from_points3d(previous_position, position, &moved);
	if (mirror->sync_time == NONE || magnitude_squared3d(&moved) > 0.0001f)
	{
		biped->pathfinding_surface_index = NONE;
		biped->pathfinding_point = ground_point;
		biped->last_pathfinding_surface_index = ground_surface_index;
	}
	return;
}

// scenario.c scenario_location_from_point: the leaf and cluster of the current structure bsp a point is in
static void location_from_point(const real_point3d* point, struct location* location)
{
	int32 leaf_index;
	location->cluster_index = (short)h1_maps_structure_bsp_leaf_get(h1_maps_structure_bsp_index(), (const ::real_point3d*)point, &leaf_index);
	location->leaf_index = leaf_index;
	return;
}

} // namespace h1_ai

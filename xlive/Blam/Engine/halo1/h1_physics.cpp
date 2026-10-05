#include "stdafx.h"
#include "h1_physics.h"

#include "h1_map_loader.h"
#include "h1_structure_bsp.h"

#include "objects/objects.h"
#include "physics/collisions.h"

/* constants */

enum
{
	k_h1_material_type_ice = 31,
};

// halo 1 friction scale on ice (physics.c)
constexpr real32 k_h1_ice_friction_scale = 0.125f;

// how far below the vehicle's origin the ground is looked for
constexpr real32 k_h1_ground_test_distance = 1.5f;

/* typedefs */

typedef bool(__cdecl* t_object_update)(datum object_index);
typedef void(__cdecl* t_object_set_velocities)(datum object_index, const real_vector3d* translational_velocity, const real_vector3d* angular_velocity);

/* globals */

static t_object_update g_vehicle_update_original = NULL;

/* prototypes */

static bool __cdecl h1_vehicle_update(datum vehicle_index);
static bool h1_object_on_ice(datum object_index);

/* public code */

void h1_physics_apply_patches(void)
{
	// the vehicle object type's update (vehicle part definition, object_update)
	const uintptr_t vehicle_update_pointer = Memory::GetAddress(0x41EAB0);
	g_vehicle_update_original = *(t_object_update*)vehicle_update_pointer;
	WritePointer(vehicle_update_pointer, h1_vehicle_update);
	return;
}

/* private code */

static bool __cdecl h1_vehicle_update(datum vehicle_index)
{
	if (!h1_maps_active())
	{
		return g_vehicle_update_original(vehicle_index);
	}

	real_vector3d velocity_before, angular_velocity_before;
	object_get_velocities(vehicle_index, &velocity_before, &angular_velocity_before);

	const bool result = g_vehicle_update_original(vehicle_index);

	if (h1_object_on_ice(vehicle_index))
	{
		real_vector3d velocity, angular_velocity;
		object_get_velocities(vehicle_index, &velocity, &angular_velocity);

		// ground friction on ice is an eighth: keep an eighth of the horizontal velocity and yaw change
		velocity.i = velocity_before.i + (velocity.i - velocity_before.i) * k_h1_ice_friction_scale;
		velocity.j = velocity_before.j + (velocity.j - velocity_before.j) * k_h1_ice_friction_scale;
		angular_velocity.k = angular_velocity_before.k + (angular_velocity.k - angular_velocity_before.k) * k_h1_ice_friction_scale;

		const t_object_set_velocities object_set_velocities = Memory::GetAddress<t_object_set_velocities>(0x135123);
		object_set_velocities(vehicle_index, &velocity, &angular_velocity);
	}
	return result;
}

static bool h1_object_on_ice(datum object_index)
{
	real_point3d origin;
	object_get_origin(object_index, &origin, false);
	origin.z += 0.5f;
	const real_vector3d down = { 0.f, 0.f, -k_h1_ground_test_distance };

	collision_result collision;
	const uint32 flags = FLAG(_collision_test_structure_bit) | FLAG(_collision_test_instanced_geometry_bit);
	if (!collision_test_vector(flags, &origin, &down, object_index, NONE, &collision))
	{
		return false;
	}
	return collision.global_material_index == h1_material_type_to_global_material(k_h1_material_type_ice);
}

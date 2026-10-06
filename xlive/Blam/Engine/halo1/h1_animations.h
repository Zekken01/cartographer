#pragma once

/*
* Halo 1 animation graphs (antr) as Halo 2 animation graphs (jmad).
*
* Every Halo 1 animation is decoded (8 byte quaternions, translations and scales per frame, the rest from its default data)
* and written with the Halo 2 uncompressed animated codec (8): a 32 byte header, per node float quaternions, translations and
* scales for every frame, then the rotation, translation and scale node flag bit vectors. Base animations carry every node,
* overlays only their animated nodes.
*/

// the halo 2 first person animation graph of a halo 1 first person weapon animation graph: the skeleton is the halo 1 graph's
// nodes (the arms and the weapon), its first person weapon animations become the halo 2 first person actions and overlays
datum h1_first_person_animation_graph_build(datum h1_animation_graph_index, const char* name);

struct h1_mode;
struct h1_phys;

// the halo 2 third person animation graph of a halo 1 model and its animation graph (NONE: only the skeleton): the halo 1 units'
// seats become modes ("stand" is "combat"), their weapon classes weapon classes, their animations actions, overlays and aiming
// blend screens
datum h1_animation_graph_build(datum h1_animation_graph_index, const h1_mode* h1_model, const char* name, const h1_phys* h1_physics = NULL);

// the marker placed at a halo 1 mass point (vehicles' wheels and hover pads, their suspension)
const char* h1_mass_point_marker_name(const char* mass_point_name, char(&buffer)[64]);

// unit_get_seat_entrance_point: where a rider of the graph starts the seat's enter animation, in the seat marker's space (the
// root of its first frame), false without one
bool h1_animation_seat_enter_root_get(datum h1_animation_graph_index, const char* seat_label, real_point3d* out_position);

// a vehicle's animation weapon class: its graph's first unit's first weapon (the ghost's "fixed", its aiming), "any" without one
string_id h1_animation_vehicle_weapon_class(datum h1_animation_graph_index);

// units.c's aiming overlay of a vehicle (its graph's first unit's first weapon's aim-still, aiming_screen_apply) on halo 2's node
// orientations, the aim's yaw (left positive) and pitch in the vehicle's frame; false without one
bool h1_animation_vehicle_aim_apply(datum h1_animation_graph_index, real32 yaw, real32 pitch, real_orientation* orientations, int32 node_count);

// a vehicle's base animations (its unit's): its idle, and its seat animations opening (the driver left: the scorpion's hatch opens)
// and closing (the driver is in)
enum e_h1_vehicle_base_animation
{
	_h1_vehicle_base_idle = 0,
	_h1_vehicle_base_opening,
	_h1_vehicle_base_closing,
};
// the animation's index in the graph (its frame count out), NONE without one
int16 h1_animation_vehicle_base_get(datum h1_animation_graph_index, e_h1_vehicle_base_animation which, int16* out_frame_count);
// a base animation's frame onto halo 2's node orientations: the nodes it animates take its frame (the others keep theirs)
void h1_animation_base_frame_apply(datum h1_animation_graph_index, int16 animation_index, int32 frame_index, real_orientation* orientations, int32 node_count);

// devices.c device_preprocess_node_orientations: a device's position (0) or power (1) animation at the frame (fractional, blended
// with the next unless not interpolated) on halo 2's node orientations; false without one
bool h1_animation_device_apply(datum h1_animation_graph_index, int16 device_animation, real32 frame, bool loops, bool interpolated,
	real_orientation* orientations, int32 node_count);

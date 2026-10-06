#include "stdafx.h"
#include "h1_ai_internal.h"

/*
* model_animations.c: the functions of it the AI calls.
*/

namespace h1_ai
{

/* ---------- models/model_animations.c: its declarations */

/* ---------- headers */

/* ---------- constants */

enum
{
	COMPRESSED_QUATERNION_COMPONENT_MAXIMUM = 32767,
	NUMBER_OF_ANIMATION_DAMAGE_TYPES = 4,
	NUMBER_OF_ANIMATION_DAMAGE_DIRECTIONS = 4,
	NUMBER_OF_DAMAGE_PARTS = 11,
};

enum
{
	animation_update_kind_render_only = 0,
	animation_update_kind_affects_game_state,
};

/* No shared header declares the update results yet; first_person_weapons.c keeps a partial copy. */
enum animation_update_result
{
	_animation_running = 0,
	_animation_key_frame,
	_animation_will_restart_on_next_frame,
	_animation_restarted,
	_animation_looped,
	NUMBER_OF_ANIMATION_UPDATE_RESULTS,
};

enum
{
	COMPRESSED_ANIMATION_NODE_HEADER_KEYFRAME_COUNT_BITS = 12,
};

/* ---------- macros */

/* ---------- structures */

struct compressed_quaternion_8byte
{
	short i;
	short j;
	short k;
	short w;
};

struct compressed_quaternion_6byte
{
	word words[3];
};

typedef char verify_compressed_quaternion_8byte_size[
	sizeof(struct compressed_quaternion_8byte) == 0x08 ? 1 : -1];
typedef char verify_compressed_quaternion_6byte_size[
	sizeof(struct compressed_quaternion_6byte) == 0x06 ? 1 : -1];

struct animation_frame_info_dx_dy
{
	real dx;
	real dy;
};

struct animation_frame_info_dx_dy_dyaw
{
	real dx;
	real dy;
	real dyaw;
};

struct animation_frame_info_dx_dy_dz_dyaw
{
	real dx;
	real dy;
	real dz;
	real dyaw;
};

typedef char verify_animation_frame_info_dx_dy_size[
	sizeof(struct animation_frame_info_dx_dy) == 0x08 ? 1 : -1];
typedef char verify_animation_frame_info_dx_dy_dyaw_size[
	sizeof(struct animation_frame_info_dx_dy_dyaw) == 0x0C ? 1 : -1];
typedef char verify_animation_frame_info_dx_dy_dz_dyaw_size[
	sizeof(struct animation_frame_info_dx_dy_dz_dyaw) == 0x10 ? 1 : -1];
struct compressed_animation_header
{
	long rotation_keyframe_frame_indices_offset;
	long default_rotations_offset;
	long rotation_keyframes_offset;
	long translation_node_headers_offset;
	long translation_keyframe_frame_indices_offset;
	long default_translations_offset;
	long translation_keyframes_offset;
	long scale_node_headers_offset;
	long scale_keyframe_frame_indices_offset;
	long default_scales_offset;
	long scale_keyframes_offset;
	unsigned long rotation_node_headers[1];
};

/* Recovered animation-graph block layouts kept TU-private to preserve VC7 header scheduling. */
struct animation_graph_node
{
	char name[TAG_STRING_LENGTH+1];
	short next_sibling_node_index;
	short first_child_node_index;
	short parent_node_index;
	word pad;
	unsigned long flags;
	real_vector3d base_vector;
	real range;
	long pad1;
};

typedef char verify_animation_graph_node_size[
	sizeof(struct animation_graph_node) == 0x40 ? 1 : -1];

/* No shared header declares this block element yet; first_person_weapons.c keeps the same copy. */
struct animation_graph_sound_reference
{
	struct tag_reference sound;
	long unused;
};

typedef char verify_animation_graph_sound_reference_size[
	sizeof(struct animation_graph_sound_reference) == 0x14 ? 1 : -1];

typedef char verify_compressed_animation_header_rotation_node_headers_offset[
	offsetof(struct compressed_animation_header, rotation_node_headers) == 0x2C ? 1 : -1];

/* ---------- prototypes */

static boolean animation_is_compressed(
	struct animation const *animation);
static short animation_keyframe_search(
	short const *keyframe_frame_indices,
	short keyframe_count,
	short target_frame_index);
static void animation_get_keyframe_rotation(
	struct animation const *animation,
	real real_frame_index,
	short adjusted_node_index,
	short node_index,
	real_quaternion *rotation);
static void animation_get_keyframe_translation(
	struct animation const *animation,
	real real_frame_index,
	short adjusted_node_index,
	short node_index,
	real_point3d *translation);
static void animation_get_keyframe_scale(
	struct animation const *animation,
	real real_frame_index,
	short adjusted_node_index,
	short node_index,
	real *scale);

/* ---------- globals */

boolean hs_model_animation_compression_enabled = TRUE;
long hs_model_animation_data_compressed_size = 0;
long hs_model_animation_data_uncompressed_size = 0;
long hs_model_animation_data_compression_savings_in_bytes = 0;
long hs_model_animation_data_compression_savings_in_bytes_at_import = 0;
real hs_model_animation_data_compression_savings_in_percent = 0.f;
long hs_model_animation_bullshit[4] = { 0 };

/* ---------- public code */

/* ---------- private code */

// binary search for the keyframe containing target_frame_index

/* ---------- models/model_animations.c: its functions */

void animation_get_x_offsets(
	struct animation const *animation,
	real *key_frame_x_offset,
	real *total_x_offset)
{
	short frame_index;
	real x_offset = 0.f;
	real key_x_offset = 0.f;
	byte const *frame_info = tag_data_address(&animation->frame_info);

	for (frame_index = 0; frame_index < animation->frame_count; frame_index++)
	{
		switch (animation->frame_info_type)
		{
		case 1:
			x_offset += ((struct animation_frame_info_dx_dy const *)frame_info)->dx;
			frame_info += sizeof(struct animation_frame_info_dx_dy);
			break;

		case 2:
			x_offset += ((struct animation_frame_info_dx_dy_dyaw const *)frame_info)->dx;
			frame_info += sizeof(struct animation_frame_info_dx_dy_dyaw);
			break;

		case 3:
			x_offset += ((struct animation_frame_info_dx_dy_dz_dyaw const *)frame_info)->dx;
			frame_info += sizeof(struct animation_frame_info_dx_dy_dz_dyaw);
			break;
		}

		if (frame_index == animation->private_key_frame_index)
		{
			key_x_offset = x_offset;
		}
	}

	if (total_x_offset)
	{
		*total_x_offset = x_offset;
	}
	if (key_frame_x_offset)
	{
		*key_frame_x_offset = key_x_offset;
	}

	return;
}

void animation_get_root_matrix(
	struct model const *model,
	struct animation const *animation,
	short frame_index,
	real_matrix4x3 *root_matrix)
{
	struct real_orientation node_orientations[MAXIMUM_NODES_PER_ANIMATION];

	animation_get_node_orientations(model, animation, frame_index, node_orientations);
	matrix4x3_from_point_and_quaternion(
		root_matrix,
		&node_orientations[0].translation,
		&node_orientations[0].rotation);

	return;
}

void animation_get_node_orientations(
	struct model const *model,
	struct animation const *animation,
	short frame_index,
	struct real_orientation *node_orientations)
{
	if (animation->type==_animation_base &&
		(!model ||
			((!animation->node_list_checksum || animation->node_list_checksum==model->node_list_checksum || !model->node_list_checksum) &&
			model->nodes.count==animation->node_count)))
	{
		boolean compressed = TEST_FLAG(animation->flags, _animation_compressed_bit) &&
			(hs_model_animation_compression_enabled || !animation->compressed_data_offset);
		byte *data = animation_get_frame_data(animation, frame_index);
		byte *default_data = animation_get_default_data(animation);
		long rotation_index = 0;
		unsigned long rotation_flags;
		long translation_index = 0;
		unsigned long translation_flags;
		long scale_index = 0;
		unsigned long scale_flags;
		short node_index;

		for (node_index = 0; node_index<animation->node_count; node_index++)
		{
			struct real_orientation *orientation = &node_orientations[node_index];

			if (!(node_index&(LONG_BITS-1)))
			{
				short long_index = node_index>>LONG_BITS_BITS;

				translation_flags = animation->nodes_with_translation_flags[long_index];
				rotation_flags = animation->nodes_with_rotation_flags[long_index];
				scale_flags = animation->nodes_with_scale_flags[long_index];
			}

			if (TEST_FLAG(rotation_flags, 0))
			{
				if (compressed)
				{
					animation_get_keyframe_rotation(animation, (real)frame_index, rotation_index++, node_index, &orientation->rotation);
				}
				else
				{
					quaternion_decompress_8byte((struct compressed_quaternion_8byte *)data, &orientation->rotation);
					data += sizeof(struct compressed_quaternion_8byte);
				}
			}
			else if (compressed)
			{
				struct compressed_animation_header const *header = (struct compressed_animation_header const *)data;

				quaternion_decompress_6byte(
					(struct compressed_quaternion_6byte *)(data+header->default_rotations_offset)+node_index,
					&orientation->rotation);
				quaternion_normalize(&orientation->rotation);
			}
			else
			{
				quaternion_decompress_8byte((struct compressed_quaternion_8byte *)default_data, &orientation->rotation);
				default_data += sizeof(struct compressed_quaternion_8byte);
			}
			rotation_flags >>= 1;

			if (TEST_FLAG(translation_flags, 0))
			{
				if (compressed)
				{
					animation_get_keyframe_translation(animation, (real)frame_index, translation_index++, node_index, &orientation->translation);
				}
				else
				{
					orientation->translation = *(real_point3d *)data;
					data += sizeof(real_point3d);
				}
			}
			else if (compressed)
			{
				struct compressed_animation_header const *header = (struct compressed_animation_header const *)data;

				orientation->translation = *((real_point3d *)(data+header->default_translations_offset)+node_index);
			}
			else
			{
				orientation->translation = *(real_point3d *)default_data;
				default_data += sizeof(real_point3d);
			}
			translation_flags >>= 1;

			if (TEST_FLAG(scale_flags, 0))
			{
				if (compressed)
				{
					animation_get_keyframe_scale(animation, (real)frame_index, scale_index++, node_index, &orientation->scale);
				}
				else
				{
					orientation->scale = *(real *)data;
					data += sizeof(real);
				}
			}
			else if (compressed)
			{
				orientation->scale = 1.0f;
			}
			else
			{
				orientation->scale = *(real *)default_data;
				default_data += sizeof(real);
			}
			scale_flags >>= 1;
		}

		match_assert(
			"c:\\halo\\SOURCE\\models\\model_animations.c",
			321,
			compressed || (byte *)data-(byte *)animation_get_frame_data(animation, frame_index)==animation->frame_size);
		match_assert(
			"c:\\halo\\SOURCE\\models\\model_animations.c",
			322,
			compressed || (byte *)default_data-(byte *)animation_get_default_data(animation)==animation->default_data.size);
	}
	else
	{
		model_get_node_orientations(model, node_orientations);
	}

	return;
}

void quaternion_decompress_8byte(
	struct compressed_quaternion_8byte const *compressed,
	real_quaternion *quaternion)
{
	real const scale = 1.f / COMPRESSED_QUATERNION_COMPONENT_MAXIMUM;

	quaternion->v.i = compressed->i * scale;
	quaternion->v.j = compressed->j * scale;
	quaternion->v.k = compressed->k * scale;
	quaternion->w = compressed->w * scale;

	return;
}

void quaternion_decompress_6byte(
	struct compressed_quaternion_6byte const *compressed,
	real_quaternion *quaternion)
{
	word word0 = compressed->words[0];
	word word1 = compressed->words[1];
	word word2 = compressed->words[2];
	short i = (short)((word0 >> 12) | (word0 & 0xFFF0));
	short j = (short)(((word1 >> 4) & 0x0FF0) | (word0 & 0x000F) | (word0 << 12));
	short k = (short)(((((word2 >> 4) & 0x0F00) | (word1 & 0x00F0)) >> 4) | (word1 << 8));
	short w = (short)(((word2 >> 8) & 0x000F) | (word2 << 4));
	real const scale = 1.f / COMPRESSED_QUATERNION_COMPONENT_MAXIMUM;

	quaternion->v.i = i * scale;
	quaternion->v.j = j * scale;
	quaternion->v.k = k * scale;
	quaternion->w = w * scale;

	return;
}

void quaternion_decompress_6byte_renormalized(
	void const *compressed,
	real_quaternion *quaternion)
{
	quaternion_decompress_6byte((struct compressed_quaternion_6byte const *)compressed, quaternion);
	quaternion_normalize(quaternion);

	return;
}

static boolean animation_is_compressed(
	struct animation const *animation)
{
	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		38,
		animation);

	return TEST_FLAG(animation->flags, _animation_compressed_bit) &&
		(hs_model_animation_compression_enabled || !animation->compressed_data_offset);
}

static short animation_keyframe_search(
	short const *keyframe_frame_indices,
	short keyframe_count,
	short target_frame_index)
{
	short low = 0;
	short high = keyframe_count-1;
	short keyframe_index;
	short infinite_loop_killer = 0;

	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		1334,
		keyframe_count>1);
	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		1335,
		keyframe_frame_indices);
	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		1336,
		keyframe_frame_indices[0]>0);
	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		1337,
		target_frame_index>=0 && target_frame_index<keyframe_frame_indices[keyframe_count-1]);

	while (TRUE)
	{
		keyframe_index = (low+high)>>1;

		match_assert(
			"c:\\halo\\SOURCE\\models\\model_animations.c",
			1343,
			keyframe_index>=0 && keyframe_index<keyframe_count);

		if (keyframe_index+1<keyframe_count && keyframe_frame_indices[keyframe_index+1]<=target_frame_index)
		{
			low = keyframe_index;
		}
		else if (keyframe_frame_indices[keyframe_index]>target_frame_index)
		{
			high = keyframe_index;
		}
		else
		{
			break;
		}

		match_assert(
			"c:\\halo\\SOURCE\\models\\model_animations.c",
			1356,
			++infinite_loop_killer<200);
	}

	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		1360,
		keyframe_index>=0 && keyframe_index<keyframe_count-1);
	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		1361,
		target_frame_index>=keyframe_frame_indices[keyframe_index] && target_frame_index<keyframe_frame_indices[keyframe_index+1]);

	return keyframe_index;
}

static void animation_get_keyframe_rotation(
	struct animation const *animation,
	real real_frame_index,
	short adjusted_node_index,
	short node_index,
	real_quaternion *rotation)
{
	byte *data = tag_data_get_pointer(&animation->data, animation->compressed_data_offset, 0);
	struct compressed_animation_header const *header = (struct compressed_animation_header const *)data;
	struct compressed_quaternion_6byte const *default_rotations = (struct compressed_quaternion_6byte const *)(data+header->default_rotations_offset);
	unsigned long node_header = header->rotation_node_headers[adjusted_node_index];
	short first_keyframe_index = (short)(node_header>>COMPRESSED_ANIMATION_NODE_HEADER_KEYFRAME_COUNT_BITS);
	short keyframe_count = (short)(node_header&(FLAG(COMPRESSED_ANIMATION_NODE_HEADER_KEYFRAME_COUNT_BITS)-1));
	word const *keyframe_frame_indices;
	struct compressed_quaternion_6byte const *keyframe_rotations;
	short frame_index;
	struct compressed_quaternion_6byte const *this_keyframe;
	struct compressed_quaternion_6byte const *next_keyframe;
	short this_keyframe_frame_index;
	short next_keyframe_frame_index;

	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		1428,
		real_frame_index>=0.0f);
	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		1430,
		real_frame_index<(real)animation->frame_count);
	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		1432,
		keyframe_count>=0);

	if (keyframe_count==0)
	{
		// this node never changes rotation
		quaternion_decompress_6byte(&default_rotations[node_index], rotation);
		quaternion_normalize(rotation);
		return;
	}

	keyframe_rotations = (struct compressed_quaternion_6byte const *)(data+header->rotation_keyframes_offset)+first_keyframe_index;
	keyframe_frame_indices = (word const *)(data+header->rotation_keyframe_frame_indices_offset)+first_keyframe_index;
	frame_index = (short)fast_ftol(floor(real_frame_index));

	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		1451,
		frame_index>=0 && frame_index<=keyframe_frame_indices[keyframe_count-1]);
	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		1452,
		keyframe_frame_indices[keyframe_count-1]==animation->frame_count-1);

	if (frame_index<keyframe_frame_indices[0])
	{
		this_keyframe_frame_index = 0;
		this_keyframe = &default_rotations[node_index];
		next_keyframe_frame_index = keyframe_frame_indices[0];
		next_keyframe = keyframe_rotations;
	}
	else if (frame_index==keyframe_frame_indices[keyframe_count-1])
	{
		this_keyframe_frame_index = keyframe_frame_indices[keyframe_count-1];
		this_keyframe = &keyframe_rotations[keyframe_count-1];
		next_keyframe_frame_index = this_keyframe_frame_index+1;
		next_keyframe = &default_rotations[node_index];
	}
	else
	{
		short keyframe_index = animation_keyframe_search((short const *)keyframe_frame_indices, keyframe_count, frame_index);

		match_assert(
			"c:\\halo\\SOURCE\\models\\model_animations.c",
			1472,
			keyframe_index>=0 && keyframe_index<keyframe_count-1);

		this_keyframe_frame_index = keyframe_frame_indices[keyframe_index];
		this_keyframe = &keyframe_rotations[keyframe_index];
		next_keyframe_frame_index = keyframe_frame_indices[keyframe_index+1];
		next_keyframe = &keyframe_rotations[keyframe_index+1];
	}

	if (real_frame_index==(real)this_keyframe_frame_index)
	{
		quaternion_decompress_6byte(this_keyframe, rotation);
		quaternion_normalize(rotation);
	}
	else
	{
		real fraction = (real_frame_index-(real)this_keyframe_frame_index)/(next_keyframe_frame_index-this_keyframe_frame_index);
		real_quaternion this_rotation;
		real_quaternion next_rotation;

		match_assert(
			"c:\\halo\\SOURCE\\models\\model_animations.c",
			1491,
			real_frame_index>=(real)this_keyframe_frame_index);
		match_assert(
			"c:\\halo\\SOURCE\\models\\model_animations.c",
			1492,
			real_frame_index< (real)next_keyframe_frame_index);

		quaternion_decompress_6byte(this_keyframe, &this_rotation);
		quaternion_decompress_6byte(next_keyframe, &next_rotation);
		quaternions_interpolate_and_normalize(&this_rotation, &next_rotation, fraction, rotation);
	}

	return;
}

static void animation_get_keyframe_translation(
	struct animation const *animation,
	real real_frame_index,
	short adjusted_node_index,
	short node_index,
	real_point3d *translation)
{
	byte *data = tag_data_get_pointer(&animation->data, animation->compressed_data_offset, 0);
	struct compressed_animation_header const *header = (struct compressed_animation_header const *)data;
	real_point3d const *default_translations = (real_point3d const *)(data+header->default_translations_offset);
	unsigned long node_header = ((unsigned long const *)(data+header->translation_node_headers_offset))[adjusted_node_index];
	short first_keyframe_index = (short)(node_header>>COMPRESSED_ANIMATION_NODE_HEADER_KEYFRAME_COUNT_BITS);
	short keyframe_count = (short)(node_header&(FLAG(COMPRESSED_ANIMATION_NODE_HEADER_KEYFRAME_COUNT_BITS)-1));

	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		1522,
		real_frame_index>=0.0f);
	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		1524,
		real_frame_index<(real)animation->frame_count);
	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		1526,
		keyframe_count>=0);

	if (keyframe_count==0)
	{
		*translation = default_translations[node_index];
	}
	else
	{
		real_point3d const *keyframe_translations = (real_point3d const *)(data+header->translation_keyframes_offset)+first_keyframe_index;
		word const *keyframe_frame_indices = (word const *)(data+header->translation_keyframe_frame_indices_offset)+first_keyframe_index;
		short frame_index = (short)fast_ftol(floor(real_frame_index));
		real_point3d const *this_keyframe;
		real_point3d const *next_keyframe;
		short this_keyframe_frame_index;
		short next_keyframe_frame_index;

		match_assert(
			"c:\\halo\\SOURCE\\models\\model_animations.c",
			1545,
			frame_index>=0 && frame_index<=keyframe_frame_indices[keyframe_count-1]);
		match_assert(
			"c:\\halo\\SOURCE\\models\\model_animations.c",
			1546,
			keyframe_frame_indices[keyframe_count-1]==animation->frame_count-1);

		if (frame_index<keyframe_frame_indices[0])
		{
			this_keyframe_frame_index = 0;
			this_keyframe = &default_translations[node_index];
			next_keyframe_frame_index = keyframe_frame_indices[0];
			next_keyframe = keyframe_translations;
		}
		else if (frame_index==keyframe_frame_indices[keyframe_count-1])
		{
			this_keyframe_frame_index = keyframe_frame_indices[keyframe_count-1];
			this_keyframe = &keyframe_translations[keyframe_count-1];
			next_keyframe_frame_index = this_keyframe_frame_index+1;
			next_keyframe = &default_translations[node_index];
		}
		else
		{
			short keyframe_index = animation_keyframe_search((short const *)keyframe_frame_indices, keyframe_count, frame_index);

			match_assert(
				"c:\\halo\\SOURCE\\models\\model_animations.c",
				1566,
				keyframe_index>=0 && keyframe_index<keyframe_count-1);

			this_keyframe_frame_index = keyframe_frame_indices[keyframe_index];
			this_keyframe = &keyframe_translations[keyframe_index];
			next_keyframe_frame_index = keyframe_frame_indices[keyframe_index+1];
			next_keyframe = &keyframe_translations[keyframe_index+1];
		}

		if (real_frame_index==(real)this_keyframe_frame_index)
		{
			*translation = *this_keyframe;
		}
		else
		{
			real fraction = (real_frame_index-(real)this_keyframe_frame_index)/(next_keyframe_frame_index-this_keyframe_frame_index);

			match_assert(
				"c:\\halo\\SOURCE\\models\\model_animations.c",
				1583,
				real_frame_index>=(real)this_keyframe_frame_index);
			match_assert(
				"c:\\halo\\SOURCE\\models\\model_animations.c",
				1584,
				real_frame_index< (real)next_keyframe_frame_index);

			points_interpolate(this_keyframe, next_keyframe, fraction, translation);
		}
	}

	return;
}

static void animation_get_keyframe_scale(
	struct animation const *animation,
	real real_frame_index,
	short adjusted_node_index,
	short node_index,
	real *scale)
{
	byte *data = tag_data_get_pointer(&animation->data, animation->compressed_data_offset, 0);
	struct compressed_animation_header const *header = (struct compressed_animation_header const *)data;
	real const *default_scales = (real const *)(data+header->default_scales_offset);
	unsigned long node_header = ((unsigned long const *)(data+header->scale_node_headers_offset))[adjusted_node_index];
	short first_keyframe_index = (short)(node_header>>COMPRESSED_ANIMATION_NODE_HEADER_KEYFRAME_COUNT_BITS);
	short keyframe_count = (short)(node_header&(FLAG(COMPRESSED_ANIMATION_NODE_HEADER_KEYFRAME_COUNT_BITS)-1));

	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		1610,
		real_frame_index>=0.0f);
	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		1612,
		real_frame_index<(real)animation->frame_count);
	match_assert(
		"c:\\halo\\SOURCE\\models\\model_animations.c",
		1614,
		keyframe_count>=0);

	if (keyframe_count==0)
	{
		*scale = default_scales[adjusted_node_index];
	}
	else
	{
		real const *keyframe_scales = (real const *)(data+header->scale_keyframes_offset)+first_keyframe_index;
		word const *keyframe_frame_indices = (word const *)(data+header->scale_keyframe_frame_indices_offset)+first_keyframe_index;
		short frame_index = (short)fast_ftol(floor(real_frame_index));
		real this_keyframe_scale;
		real next_keyframe_scale;
		short this_keyframe_frame_index;
		short next_keyframe_frame_index;

		match_assert(
			"c:\\halo\\SOURCE\\models\\model_animations.c",
			1634,
			frame_index>=0 && frame_index<=keyframe_frame_indices[keyframe_count-1]);
		match_assert(
			"c:\\halo\\SOURCE\\models\\model_animations.c",
			1635,
			keyframe_frame_indices[keyframe_count-1]==animation->frame_count-1);

		if (frame_index<keyframe_frame_indices[0])
		{
			this_keyframe_frame_index = 0;
			this_keyframe_scale = default_scales[adjusted_node_index];
			next_keyframe_frame_index = keyframe_frame_indices[0];
			next_keyframe_scale = keyframe_scales[0];
		}
		else if (frame_index==keyframe_frame_indices[keyframe_count-1])
		{
			this_keyframe_frame_index = keyframe_frame_indices[keyframe_count-1];
			this_keyframe_scale = keyframe_scales[keyframe_count-1];
			next_keyframe_frame_index = this_keyframe_frame_index+1;
			next_keyframe_scale = default_scales[adjusted_node_index];
		}
		else
		{
			short keyframe_index = animation_keyframe_search((short const *)keyframe_frame_indices, keyframe_count, frame_index);

			match_assert(
				"c:\\halo\\SOURCE\\models\\model_animations.c",
				1655,
				keyframe_index>=0 && keyframe_index<keyframe_count-1);

			this_keyframe_frame_index = keyframe_frame_indices[keyframe_index];
			this_keyframe_scale = keyframe_scales[keyframe_index];
			next_keyframe_frame_index = keyframe_frame_indices[keyframe_index+1];
			next_keyframe_scale = keyframe_scales[keyframe_index+1];
		}

		if (real_frame_index==(real)this_keyframe_frame_index)
		{
			*scale = this_keyframe_scale;
		}
		else
		{
			real fraction = (real_frame_index-(real)this_keyframe_frame_index)/(next_keyframe_frame_index-this_keyframe_frame_index);

			match_assert(
				"c:\\halo\\SOURCE\\models\\model_animations.c",
				1672,
				real_frame_index>=(real)this_keyframe_frame_index);
			match_assert(
				"c:\\halo\\SOURCE\\models\\model_animations.c",
				1673,
				real_frame_index< (real)next_keyframe_frame_index);

			scalars_interpolate(this_keyframe_scale, next_keyframe_scale, fraction, scale);
		}
	}

	return;
}

byte *animation_get_frame_data(struct animation const *animation, short frame_index)
{
	byte *frame_data;
	boolean compressed;

	compressed = TEST_FLAG(animation->flags, _animation_compressed_bit) && hs_model_animation_compression_enabled;
	frame_data = tag_data_get_pointer(&animation->data, 0, 0);

	match_assert("c:\\halo\\SOURCE\\models\\model_animation_definitions.c", 1147, frame_index>=0 && frame_index<animation->frame_count);

	if (compressed)
		frame_data += animation->compressed_data_offset;
	else
		frame_data += frame_index * animation->frame_size;

	return frame_data;
}

} // namespace h1_ai

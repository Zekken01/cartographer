#include "stdafx.h"
#include "h1_ai_internal.h"

/*
* structure_bsp_definitions.c: the functions of it the AI calls.
*/

namespace h1_ai
{

/* ---------- structures/structure_bsp_definitions.c: its declarations */

/* ---------- headers */

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

/* ---------- private code */

/* ---------- structures/structure_bsp_definitions.c: its functions */

unsigned long *structure_bsp_get_cluster_pvs(
	struct structure_bsp *structure_bsp,
	short cluster_index)
{
	match_assert("c:\\halo\\SOURCE\\structures\\structure_bsp_definitions.c", 36, cluster_index>=0 && cluster_index<structure_bsp->clusters.count);
	match_assert("c:\\halo\\SOURCE\\structures\\structure_bsp_definitions.c",
		37,
		(cluster_index+1)*BIT_VECTOR_SIZE_IN_LONGS(structure_bsp->clusters.count)<=structure_bsp->cluster_data.size);

	// Get pointer to bitvector starting at the cluster index
	return (unsigned long *)(
		(byte *)tag_data_address(&structure_bsp->cluster_data) +
		sizeof(unsigned long) * cluster_index *
		BIT_VECTOR_SIZE_IN_LONGS(structure_bsp->clusters.count));
}

byte structure_bsp_get_cluster_encoded_sound_distance(
	struct structure_bsp *structure_bsp,
	short from_cluster_index,
	short to_cluster_index)
{
	byte result;

	match_assert("c:\\halo\\SOURCE\\structures\\structure_bsp_definitions.c", 1215, from_cluster_index>=0 && from_cluster_index<structure_bsp->clusters.count);
	match_assert("c:\\halo\\SOURCE\\structures\\structure_bsp_definitions.c", 1216, to_cluster_index>=0 && to_cluster_index<structure_bsp->clusters.count);

	if (from_cluster_index!=to_cluster_index)
	{
		if (from_cluster_index>to_cluster_index)
		{
			short temp = from_cluster_index;
			from_cluster_index = to_cluster_index;
			to_cluster_index = temp;
		}

		result = *structure_bsp_get_cluster_encoded_sound_data(
			structure_bsp, 
			from_cluster_index,
			to_cluster_index);
	}
	else
	{
		result = 0;
	}

	return result;
}

byte *structure_bsp_get_cluster_encoded_sound_data(
	struct structure_bsp *structure_bsp,
	short row_index,
	short column_index)
{
	short offset = row_index * (structure_bsp->clusters.count-1)-row_index*(row_index+1)/2+column_index-1;

	match_assert("c:\\halo\\SOURCE\\structures\\structure_bsp_definitions.c", 1202, row_index<column_index);
	match_assert("c:\\halo\\SOURCE\\structures\\structure_bsp_definitions.c", 1203, offset>=0 && offset<structure_bsp->sound_cluster_data.size);

	return &((byte *)tag_data_address(&structure_bsp->sound_cluster_data))[offset];
}

} // namespace h1_ai

#include "stdafx.h"
#include "h1_ai_internal.h"

/*
* models.c: the functions of it the AI calls.
*/

namespace h1_ai
{

void model_get_node_orientations(
	struct model const *model,
	real_orientation *node_orientations)
{
	short node_index;

	for (node_index = 0; node_index<model->nodes.count; node_index++)
	{
		struct model_node *node = TAG_BLOCK_GET_ELEMENT(&model->nodes, node_index, struct model_node);

		node_orientations[node_index].rotation = node->default_rotation;
		node_orientations[node_index].translation = node->default_translation;
		node_orientations[node_index].scale = 1.f;
	}

	return;
}

} // namespace h1_ai

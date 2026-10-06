/*
BSP2D.H

header included in hcex build.
*/

#ifndef __BSP2D_H
#define __BSP2D_H
#pragma once

namespace h1_ai
{

/* ---------- headers */


/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct bsp2d_node
{
	real_plane2d plane;
	long child_indices[2];
};

struct bsp2d
{
	struct tag_block nodes;
};

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

/* ---------- public code */

long bsp2d_test_point(struct tag_block const *nodes, real_point2d const *point, long node_index);

} // namespace h1_ai

#endif // __BSP2D_H

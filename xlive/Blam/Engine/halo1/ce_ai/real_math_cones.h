#pragma once

namespace h1_ai
{


boolean pin_normal_to_cone3d(
	real_vector3d const *normal,
	real_vector3d const *direction,
	real sine,
	real cosine,
	real_vector3d *result);

} // namespace h1_ai

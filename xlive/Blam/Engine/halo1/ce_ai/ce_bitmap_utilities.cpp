#include "stdafx.h"
#include "h1_ai_internal.h"

/*
* bitmap_utilities.c: the functions of it the AI calls.
*/

namespace h1_ai
{

// bitmap_utilities.c
enum
{
	_rgb_color_interpolation_hsv_bit = 0,
	_rgb_color_interpolation_hsv_reverse_bit,
};

// bitmaps_inlines.h (the colors are checked in debug builds)
#define match_assert_valid_real_rgb_color(file, line, rgb) ((void)0)

// bitmap_utilities.h
union real_hsv_color
{
	real n[3];
	struct
	{
		real hue;
		real saturation;
		real value;
	};
};

union real_hsv_color *real_rgb_color_to_real_hsv_color(
	union real_rgb_color const *rgb,
	union real_hsv_color *hsv)
{
	real value;
	real minimum;
	real delta;
	real saturation;

	if (rgb->green > rgb->blue)
		value = rgb->green;
	else
		value = rgb->blue;
	if (rgb->red > value)
		value = rgb->red;
	else
	{
		if (rgb->green > rgb->blue)
			value = rgb->green;
		else
			value = rgb->blue;
	}

	if (rgb->green > rgb->blue)
		minimum = rgb->blue;
	else
		minimum = rgb->green;
	if (rgb->red > minimum)
	{
		if (rgb->green > rgb->blue)
			minimum = rgb->blue;
		else
			minimum = rgb->green;
	}
	else
		minimum = rgb->red;

	delta = value - minimum;

	if (!hsv)
	{
		display_assert(
			"hsv",
			"c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c",
			0x8B2,
			TRUE);
		system_exit(-1);
	}
	if ((void const *)rgb == (void const *)hsv)
	{
		display_assert(
			"rgb!=(real_rgb_color *)hsv",
			"c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c",
			0x8B3,
			TRUE);
		system_exit(-1);
	}

	hsv->value = value;
	if (value == 0.0f)
		saturation = 0.0f;
	else
		saturation = delta / value;
	hsv->saturation = saturation;

	if (saturation == 0.0f)
	{
		hsv->hue = 0.0f;
		return hsv;
	}
	if (rgb->red == value)
		hsv->hue = (rgb->green - rgb->blue) / delta;
	else if (rgb->green == value)
		hsv->hue = (rgb->blue - rgb->red) / delta + 2.0f;
	else
		hsv->hue = (rgb->red - rgb->green) / delta + 4.0f;

	hsv->hue *= 1.0f / 6.0f;
	if (hsv->hue < 0.0f)
		hsv->hue += 1.0f;
	return hsv;
}

union real_rgb_color *real_hsv_color_to_real_rgb_color(
	union real_hsv_color *hsv,
	union real_rgb_color *rgb)
{
	union real_hsv_color *source = hsv;
	real scaled_hue = source->hue * 6.0f;
	real p;
	real q;
	real t;
	long truncated_sector;
	long sector;

	if (!rgb)
	{
		display_assert(
			"rgb",
			"c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c",
			0x8DF,
			TRUE);
		system_exit(-1);
	}
	if ((void const *)rgb == (void const *)source)
	{
		display_assert(
			"rgb!=(real_rgb_color *)hsv",
			"c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c",
			0x8E1,
			TRUE);
		system_exit(-1);
	}

	if (source->saturation == 0.0f)
	{
		rgb->red = rgb->green = rgb->blue = source->value;
		return rgb;
	}

	truncated_sector = (long)scaled_hue;
	sector = (real)truncated_sector > scaled_hue ? truncated_sector - 1 : truncated_sector;
	scaled_hue -= sector;
	p = (1.0f - source->saturation) * source->value;
	q = (1.0f - scaled_hue * source->saturation) * source->value;
	t = (1.0f - (1.0f - scaled_hue) * source->saturation) * source->value;

	switch (sector)
	{
	case 0:
		rgb->red = source->value;
		rgb->green = t;
		rgb->blue = p;
		return rgb;
	case 1:
		rgb->red = q;
		rgb->green = source->value;
		rgb->blue = p;
		return rgb;
	case 2:
		rgb->red = p;
		rgb->green = source->value;
		rgb->blue = t;
		return rgb;
	case 3:
		rgb->red = p;
		rgb->green = q;
		rgb->blue = source->value;
		return rgb;
	case 4:
		rgb->red = t;
		rgb->green = p;
		rgb->blue = source->value;
		return rgb;
	case 5:
		rgb->red = source->value;
		rgb->green = p;
		rgb->blue = q;
		return rgb;
	default:
		return rgb;
	}
}

union real_rgb_color *rgb_colors_interpolate(
	union real_rgb_color *rgb_result,
	unsigned long flags,
	union real_rgb_color const *rgb_lower_bound,
	union real_rgb_color const *rgb_upper_bound,
	real interpolation_factor)
{
	real inverse_interpolation_factor = 1.f - interpolation_factor;

	match_assert_valid_real_rgb_color("c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c", 0x939, rgb_lower_bound);
	match_assert_valid_real_rgb_color("c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c", 0x93A, rgb_upper_bound);

	if (TEST_FLAG(flags, _rgb_color_interpolation_hsv_bit))
	{
		union real_hsv_color hsv_result;
		union real_hsv_color hsv_lower_bound;
		union real_hsv_color hsv_upper_bound;

		real_rgb_color_to_real_hsv_color(rgb_lower_bound, &hsv_lower_bound);
		real_rgb_color_to_real_hsv_color(rgb_upper_bound, &hsv_upper_bound);

		if ((fabs(hsv_lower_bound.hue - hsv_upper_bound.hue) > 0.5) !=
			TEST_FLAG(flags, _rgb_color_interpolation_hsv_reverse_bit))
		{
			if (hsv_lower_bound.hue < hsv_upper_bound.hue)
				hsv_lower_bound.hue += 1.f;
			else
				hsv_upper_bound.hue += 1.f;
		}

		hsv_result.hue =
			inverse_interpolation_factor * hsv_lower_bound.hue +
			interpolation_factor * hsv_upper_bound.hue;
		if (hsv_result.hue > 1.f)
			hsv_result.hue -= 1.f;
		hsv_result.saturation =
			inverse_interpolation_factor * hsv_lower_bound.saturation +
			interpolation_factor * hsv_upper_bound.saturation;
		hsv_result.value =
			inverse_interpolation_factor * hsv_lower_bound.value +
			interpolation_factor * hsv_upper_bound.value;

		real_hsv_color_to_real_rgb_color(&hsv_result, rgb_result);
	}
	else
	{
		rgb_result->red =
			inverse_interpolation_factor * rgb_lower_bound->red +
			interpolation_factor * rgb_upper_bound->red;
		rgb_result->green =
			inverse_interpolation_factor * rgb_lower_bound->green +
			interpolation_factor * rgb_upper_bound->green;
		rgb_result->blue =
			inverse_interpolation_factor * rgb_lower_bound->blue +
			interpolation_factor * rgb_upper_bound->blue;
	}

	match_assert_valid_real_rgb_color("c:\\halo\\SOURCE\\bitmaps\\bitmap_utilities.c", 0x95D, rgb_result);

	return rgb_result;
}

} // namespace h1_ai

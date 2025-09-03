/*
obs-showdraw
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

#include <obs-module.h>

class DrawingEffect {
public:
	DrawingEffect();
	~DrawingEffect();

	bool init();

	gs_effect_t *effect = nullptr;

	gs_eparam_t *texture_image = nullptr;
	gs_eparam_t *texture_image1 = nullptr;

	gs_eparam_t *float_texel_width = nullptr;
	gs_eparam_t *float_texel_height = nullptr;
	gs_eparam_t *int_kernel_size = nullptr;

	gs_eparam_t *texture_motion_map = nullptr;
	gs_eparam_t *float_strength = nullptr;
	gs_eparam_t *float_motion_threshold = nullptr;

	gs_eparam_t *float_scaling_factor = nullptr;

	gs_technique_t *tech_draw = nullptr;
	gs_technique_t *tech_extract_luminance = nullptr;
	gs_technique_t *tech_median_filtering = nullptr;
	gs_technique_t *tech_calculate_motion_map = nullptr;
	gs_technique_t *tech_motion_adaptive_filtering = nullptr;
	gs_technique_t *tech_apply_sobel = nullptr;
	gs_technique_t *tech_suppress_non_maximum = nullptr;
	gs_technique_t *tech_detect_edge = nullptr;
	gs_technique_t *tech_erosion = nullptr;
	gs_technique_t *tech_dilation = nullptr;
	gs_technique_t *tech_scaling = nullptr;
};

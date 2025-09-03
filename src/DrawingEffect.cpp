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

#include <stdexcept>

#include <plugin-support.h>

#include "DrawingEffect.hpp"
#include "bridge.hpp"

using kaitotokyo::obs::unique_bfree_t;

DrawingEffect::DrawingEffect() {}

DrawingEffect::~DrawingEffect()
{
	obs_enter_graphics();
	gs_effect_destroy(effect);
	obs_leave_graphics();
}

static gs_eparam_t *getEffectParam(gs_effect_t *effect, const char *name)
{
	gs_eparam_t *param = gs_effect_get_param_by_name(effect, name);

	if (!param) {
		obs_log(LOG_ERROR, "Effect parameter %s not found", name);
		throw std::runtime_error("Effect parameter not found");
	}

	return param;
}

static gs_technique_t *getEffectTech(gs_effect_t *effect, const char *name)
{
	gs_technique_t *tech = gs_effect_get_technique(effect, name);

	if (!tech) {
		obs_log(LOG_ERROR, "Effect technique %s not found", name);
		throw std::runtime_error("Effect technique not found");
	}

	return tech;
}

bool DrawingEffect::init()
{
	char *error_string = nullptr;

	unique_bfree_t effect_path(obs_module_file("effects/drawing.effect"));
	if (!effect_path) {
		obs_log(LOG_ERROR, "Failed to get effect path");
		return false;
	}

	effect = gs_effect_create_from_file(effect_path.get(), &error_string);
	if (!effect) {
		obs_log(LOG_ERROR, "Error loading effect: %s", error_string);
		bfree(error_string);
		return false;
	}

	try {
		texture_image = getEffectParam(effect, "image");
		texture_image1 = getEffectParam(effect, "image1");

		float_texel_width = getEffectParam(effect, "texelWidth");
		float_texel_height = getEffectParam(effect, "texelHeight");
		int_kernel_size = getEffectParam(effect, "kernelSize");

		texture_motion_map = getEffectParam(effect, "motionMap");
		float_strength = getEffectParam(effect, "strength");
		float_motion_threshold = getEffectParam(effect, "motionThreshold");

		float_scaling_factor = getEffectParam(effect, "scalingFactor");

		tech_draw = getEffectTech(effect, "Draw");
		tech_extract_luminance = getEffectTech(effect, "ExtractLuminance");
		tech_median_filtering = getEffectTech(effect, "MedianFiltering");
		tech_calculate_motion_map = getEffectTech(effect, "CalculateMotionMap");
		tech_motion_adaptive_filtering = getEffectTech(effect, "MotionAdaptiveFiltering");
		tech_apply_sobel = getEffectTech(effect, "ApplySobel");
		tech_suppress_non_maximum = getEffectTech(effect, "SuppressNonMaximum");
		tech_detect_edge = getEffectTech(effect, "DetectEdge");
		tech_erosion = getEffectTech(effect, "Erosion");
		tech_dilation = getEffectTech(effect, "Dilation");
		tech_scaling = getEffectTech(effect, "Scaling");
	} catch (const std::exception &e) {
		gs_effect_destroy(effect);
		effect = nullptr;
		obs_log(LOG_ERROR, "Error initializing effect: %s", e.what());
		return false;
	}

	return true;
}

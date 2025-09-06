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

#include "DrawingEffect.hpp"

#include "plugin-support.h"
#include <obs-module.h>

#include "obs-bridge-utils/obs-bridge-utils.hpp"

using kaito_tokyo::obs_bridge_utils::unique_bfree_t;

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

namespace kaito_tokyo {
namespace obs_showdraw {

DrawingEffect::DrawingEffect()
{
	char *error_string = nullptr;

	unique_bfree_t effect_path(obs_module_file("effects/drawing.effect"));
	if (!effect_path) {
		obs_log(LOG_ERROR, "Failed to get effect path");
		throw std::runtime_error("Failed to get effect path");
	}

	effect = gs_effect_create_from_file(effect_path.get(), &error_string);
	if (!effect) {
		obs_log(LOG_ERROR, "Error loading effect: %s", error_string);
		bfree(error_string);
		throw std::runtime_error("Error loading effect");
	}

	try {
		textureImage = getEffectParam(effect, "image");
		textureImage1 = getEffectParam(effect, "image1");

		floatTexelWidth = getEffectParam(effect, "texelWidth");
		floatTexelHeight = getEffectParam(effect, "texelHeight");
		intKernelSize = getEffectParam(effect, "kernelSize");

		textureMotionMap = getEffectParam(effect, "motionMap");
		floatStrength = getEffectParam(effect, "strength");
		floatMotionThreshold = getEffectParam(effect, "motionThreshold");

		floatHighThreshold = getEffectParam(effect, "highThreshold");
		floatLowThreshold = getEffectParam(effect, "lowThreshold");

		boolUseLog = getEffectParam(effect, "useLog");
		floatScalingFactor = getEffectParam(effect, "scalingFactor");

		techDraw = getEffectTech(effect, "Draw");
		techExtractLuminance = getEffectTech(effect, "ExtractLuminance");
		techHorizontalMedian3 = getEffectTech(effect, "HorizontalMedian3");
		techHorizontalMedian5 = getEffectTech(effect, "HorizontalMedian5");
		techHorizontalMedian7 = getEffectTech(effect, "HorizontalMedian7");
		techHorizontalMedian9 = getEffectTech(effect, "HorizontalMedian9");
		techVerticalMedian3 = getEffectTech(effect, "VerticalMedian3");
		techVerticalMedian5 = getEffectTech(effect, "VerticalMedian5");
		techVerticalMedian7 = getEffectTech(effect, "VerticalMedian7");
		techVerticalMedian9 = getEffectTech(effect, "VerticalMedian9");
		techCalculateMotionMap = getEffectTech(effect, "CalculateMotionMap");
		techMotionAdaptiveFiltering = getEffectTech(effect, "MotionAdaptiveFiltering");
		techApplySobel = getEffectTech(effect, "ApplySobel");
		techFinalizeSobelMagnitude = getEffectTech(effect, "FinalizeSobelMagnitude");
		techSuppressNonMaximum = getEffectTech(effect, "SuppressNonMaximum");
		techHysteresisClassify = getEffectTech(effect, "HysteresisClassify");
		techHysteresisPropagate = getEffectTech(effect, "HysteresisPropagate");
		techHysteresisFinalize = getEffectTech(effect, "HysteresisFinalize");
		techErosion = getEffectTech(effect, "Erosion");
		techDilation = getEffectTech(effect, "Dilation");
		techScaling = getEffectTech(effect, "Scaling");
	} catch (const std::exception &e) {
		gs_effect_destroy(effect);
		effect = nullptr;
		obs_log(LOG_ERROR, "Error initializing effect: %s", e.what());
		throw; // Re-throw the exception
	}
}

DrawingEffect::~DrawingEffect() noexcept
{
	gs_effect_destroy(effect);
}

} // namespace obs_showdraw
} // namespace kaito_tokyo

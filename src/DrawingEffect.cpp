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

using namespace kaito_tokyo::obs_bridge_utils;
using namespace kaito_tokyo::obs_showdraw;

inline gs_eparam_t *getEffectParam(const unique_gs_effect_t &effect, const char *name)
{
	gs_eparam_t *param = gs_effect_get_param_by_name(effect.get(), name);

	if (!param) {
		slog(LOG_ERROR) << "Effect parameter " << name << " not found";
		throw std::runtime_error("Effect parameter not found");
	}

	return param;
}

inline gs_technique_t *getEffectTech(const unique_gs_effect_t &effect, const char *name)
{
	gs_technique_t *tech = gs_effect_get_technique(effect.get(), name);

	if (!tech) {
		slog(LOG_ERROR) << "Effect technique " << name << " not found";
		throw std::runtime_error("Effect technique not found");
	}

	return tech;
}

namespace kaito_tokyo {
namespace obs_showdraw {

DrawingEffect::DrawingEffect(unique_gs_effect_t _effect)
	: effect(std::move(_effect)),
	  textureImage(getEffectParam(effect, "image")),
	  textureImage1(getEffectParam(effect, "image1")),
	  floatTexelWidth(getEffectParam(effect, "texelWidth")),
	  floatTexelHeight(getEffectParam(effect, "texelHeight")),
	  intKernelSize(getEffectParam(effect, "kernelSize")),
	  textureMotionMap(getEffectParam(effect, "motionMap")),
	  floatStrength(getEffectParam(effect, "strength")),
	  floatMotionThreshold(getEffectParam(effect, "motionThreshold")),
	  floatHighThreshold(getEffectParam(effect, "highThreshold")),
	  floatLowThreshold(getEffectParam(effect, "lowThreshold")),
	  boolUseLog(getEffectParam(effect, "useLog")),
	  floatScalingFactor(getEffectParam(effect, "scalingFactor")),
	  techExtractLuminance(getEffectTech(effect, "ExtractLuminance")),
	  techHorizontalMedian3(getEffectTech(effect, "HorizontalMedian3")),
	  techHorizontalMedian5(getEffectTech(effect, "HorizontalMedian5")),
	  techHorizontalMedian7(getEffectTech(effect, "HorizontalMedian7")),
	  techHorizontalMedian9(getEffectTech(effect, "HorizontalMedian9")),
	  techVerticalMedian3(getEffectTech(effect, "VerticalMedian3")),
	  techVerticalMedian5(getEffectTech(effect, "VerticalMedian5")),
	  techVerticalMedian7(getEffectTech(effect, "VerticalMedian7")),
	  techVerticalMedian9(getEffectTech(effect, "VerticalMedian9")),
	  techCalculateHorizontalMotionMap(getEffectTech(effect, "CalculateHorizontalMotionMap")),
	  techCalculateVerticalMotionMap(getEffectTech(effect, "CalculateVerticalMotionMap")),
	  techMotionAdaptiveFiltering(getEffectTech(effect, "MotionAdaptiveFiltering")),
	  techApplySobel(getEffectTech(effect, "ApplySobel")),
	  techFinalizeSobelMagnitude(getEffectTech(effect, "FinalizeSobelMagnitude")),
	  techSuppressNonMaximum(getEffectTech(effect, "SuppressNonMaximum")),
	  techHysteresisClassify(getEffectTech(effect, "HysteresisClassify")),
	  techHysteresisPropagate(getEffectTech(effect, "HysteresisPropagate")),
	  techHysteresisFinalize(getEffectTech(effect, "HysteresisFinalize")),
	  techHorizontalErosion(getEffectTech(effect, "HorizontalErosion")),
	  techVerticalErosion(getEffectTech(effect, "VerticalErosion")),
	  techHorizontalDilation(getEffectTech(effect, "HorizontalDilation")),
	  techVerticalDilation(getEffectTech(effect, "VerticalDilation")),
	  techDraw(getEffectTech(effect, "Draw"))
{
}

} // namespace obs_showdraw
} // namespace kaito_tokyo

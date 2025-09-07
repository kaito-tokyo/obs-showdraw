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

#include <obs.h>

#include <obs-bridge-utils/obs-bridge-utils.hpp>

namespace kaito_tokyo {
namespace obs_showdraw {

class DrawingEffect {
public:
	DrawingEffect(kaito_tokyo::obs_bridge_utils::unique_gs_effect_t effect);

	DrawingEffect(const DrawingEffect &) = delete;
	DrawingEffect(DrawingEffect &&) = delete;
	DrawingEffect &operator=(const DrawingEffect &) = delete;
	DrawingEffect &operator=(DrawingEffect &&) = delete;

	const kaito_tokyo::obs_bridge_utils::unique_gs_effect_t effect;

	gs_eparam_t *const textureImage;
	gs_eparam_t *const textureImage1;

	gs_eparam_t *const floatTexelWidth;
	gs_eparam_t *const floatTexelHeight;
	gs_eparam_t *const intKernelSize;

	gs_eparam_t *const textureMotionMap;
	gs_eparam_t *const floatStrength;
	gs_eparam_t *const floatMotionThreshold;

	gs_eparam_t *const boolUseLog;
	gs_eparam_t *const floatScalingFactor;

	gs_eparam_t *const floatHighThreshold;
	gs_eparam_t *const floatLowThreshold;

	gs_technique_t *const techExtractLuminance;

	gs_technique_t *const techHorizontalMedian3;
	gs_technique_t *const techHorizontalMedian5;
	gs_technique_t *const techHorizontalMedian7;
	gs_technique_t *const techHorizontalMedian9;
	gs_technique_t *const techVerticalMedian3;
	gs_technique_t *const techVerticalMedian5;
	gs_technique_t *const techVerticalMedian7;
	gs_technique_t *const techVerticalMedian9;

	gs_technique_t *const techCalculateHorizontalMotionMap;
	gs_technique_t *const techCalculateVerticalMotionMap;
	gs_technique_t *const techMotionAdaptiveFiltering;

	gs_technique_t *const techApplySobel;
	gs_technique_t *const techFinalizeSobelMagnitude;
	gs_technique_t *const techSuppressNonMaximum;
	gs_technique_t *const techHysteresisClassify;
	gs_technique_t *const techHysteresisPropagate;
	gs_technique_t *const techHysteresisFinalize;

	gs_technique_t *const techHorizontalErosion;
	gs_technique_t *const techVerticalErosion;
	gs_technique_t *const techHorizontalDilation;
	gs_technique_t *const techVerticalDilation;

	gs_technique_t *const techDraw;
};

} // namespace obs_showdraw
} // namespace kaito_tokyo

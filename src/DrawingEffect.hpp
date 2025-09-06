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

namespace kaito_tokyo {
namespace obs_showdraw {

class DrawingEffect {
public:
	DrawingEffect();
	~DrawingEffect() noexcept;

	gs_effect_t *effect = nullptr;

	gs_eparam_t *textureImage = nullptr;
	gs_eparam_t *textureImage1 = nullptr;

	gs_eparam_t *floatTexelWidth = nullptr;
	gs_eparam_t *floatTexelHeight = nullptr;
	gs_eparam_t *intKernelSize = nullptr;

	gs_eparam_t *textureMotionMap = nullptr;
	gs_eparam_t *floatStrength = nullptr;
	gs_eparam_t *floatMotionThreshold = nullptr;

	gs_eparam_t *floatHighThreshold = nullptr;
	gs_eparam_t *floatLowThreshold = nullptr;

	gs_eparam_t *floatScalingFactor = nullptr;

	gs_technique_t *techDraw = nullptr;
	gs_technique_t *techExtractLuminance = nullptr;
	gs_technique_t *techMedianFiltering = nullptr;
	gs_technique_t *techCalculateMotionMap = nullptr;
	gs_technique_t *techMotionAdaptiveFiltering = nullptr;
	gs_technique_t *techApplySobel = nullptr;
	gs_technique_t *techSuppressNonMaximum = nullptr;
	gs_technique_t *techHysteresisClassify = nullptr;
	gs_technique_t *techHysteresisPropagate = nullptr;
	gs_technique_t *techHysteresisFinalize = nullptr;
	gs_technique_t *techErosion = nullptr;
	gs_technique_t *techDilation = nullptr;
	gs_technique_t *techScaling = nullptr;
};

} // namespace obs_showdraw
} // namespace kaito_tokyo

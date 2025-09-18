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

namespace KaitoTokyo {
namespace ShowDraw {

enum class ExtractionMode {
	Default = 0,
	Passthrough = 100,
	ConvertToGrayscale = 200,
	MotionMapCalculation = 300,
	SobelMagnitude = 400,
};

struct Preset {
public:
	ExtractionMode extractionMode = ExtractionMode::Default;

	bool medianFilterEnabled = true;

	double motionAdaptiveFilteringStrength = 0.5;
	double motionAdaptiveFilteringMotionThreshold = 0.3;

	bool sobelMagnitudeFinalizationUseLog = true;
	double sobelMagnitudeFinalizationScalingFactorDb = 10.0;
};

} // namespace ShowDraw
} // namespace KaitoTokyo

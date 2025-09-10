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

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <obs.h>

namespace kaito_tokyo {
namespace obs_showdraw {

enum class ExtractionMode {
	Default = 0,
	Passthrough = 100,
	LuminanceExtraction = 200,
	MotionMapCalculation = 300,
	SobelMagnitude = 400,
	EdgeDetection = 500,
	ShowDetectedContours = 600,
};

constexpr ExtractionMode DefaultExtractionMode = ExtractionMode::SobelMagnitude;

struct Preset {
public:
	std::string presetName;

	ExtractionMode extractionMode;

	std::int64_t medianFilteringKernelSize;

	std::int64_t motionMapKernelSize;

	double motionAdaptiveFilteringStrength;
	double motionAdaptiveFilteringMotionThreshold;

	bool sobelMagnitudeFinalizationUseLog;
	double sobelMagnitudeFinalizationScalingFactorDb;

	double hysteresisHighThreshold;
	double hysteresisLowThreshold;

	std::int64_t hysteresisPropagationIterations;

	std::int64_t morphologyOpeningErosionKernelSize;
	std::int64_t morphologyOpeningDilationKernelSize;

	std::int64_t morphologyClosingDilationKernelSize;
	std::int64_t morphologyClosingErosionKernelSize;

	bool isSystem() const noexcept;
	bool isUser() const noexcept;

	obs_data_t *loadIntoObsData(obs_data_t *data) const noexcept;
	std::optional<std::string> validate() const noexcept;

	static Preset fromObsData(obs_data_t *data) noexcept;

	static void saveUserPresets(const std::vector<Preset> &presets) noexcept;
	static std::vector<Preset> loadUserPresets(const Preset &runningPreset) noexcept;

	static Preset getStrongDefault() noexcept;
};

} // namespace obs_showdraw
} // namespace kaito_tokyo

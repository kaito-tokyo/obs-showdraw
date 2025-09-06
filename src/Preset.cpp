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

#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include "plugin-support.h"
#include <obs-module.h>

#include "obs-bridge-utils/obs-bridge-utils.hpp"

#include "Preset.hpp"

using kaito_tokyo::obs_bridge_utils::unique_bfree_t;
using kaito_tokyo::obs_bridge_utils::unique_obs_data_array_t;
using kaito_tokyo::obs_bridge_utils::unique_obs_data_t;

namespace kaito_tokyo {
namespace obs_showdraw {

const char *UserPresetsJson = "UserPresets.json";
const char *UserPresetsVersion = "2025-09-03";

bool Preset::isSystem() const noexcept
{
	return presetName.size() >= 1 && presetName[0] == ' ';
}

bool Preset::isUser() const noexcept
{
	return presetName.size() >= 1 && presetName[0] != ' ';
}

obs_data_t *Preset::loadIntoObsData(obs_data_t *data) const noexcept
{
	obs_data_set_string(data, "presetName", presetName.c_str());
	obs_data_set_int(data, "extractionMode", static_cast<int>(extractionMode));
	obs_data_set_int(data, "medianFilteringKernelSize", medianFilteringKernelSize);
	obs_data_set_int(data, "motionMapKernelSize", motionMapKernelSize);
	obs_data_set_double(data, "motionAdaptiveFilteringStrength", motionAdaptiveFilteringStrength);
	obs_data_set_double(data, "motionAdaptiveFilteringMotionThreshold", motionAdaptiveFilteringMotionThreshold);
	obs_data_set_bool(data, "sobelMagnitudeFinalizationUseLog", sobelMagnitudeFinalizationUseLog);
	obs_data_set_double(data, "sobelMagnitudeFinalizationScalingFactorDb",
			    sobelMagnitudeFinalizationScalingFactorDb);
	obs_data_set_double(data, "hysteresisHighThreshold", hysteresisHighThreshold);
	obs_data_set_double(data, "hysteresisLowThreshold", hysteresisLowThreshold);
	obs_data_set_int(data, "morphologyOpeningErosionKernelSize", morphologyOpeningErosionKernelSize);
	obs_data_set_int(data, "morphologyOpeningDilationKernelSize", morphologyOpeningDilationKernelSize);
	obs_data_set_int(data, "morphologyClosingErosionKernelSize", morphologyClosingErosionKernelSize);
	obs_data_set_int(data, "morphologyClosingDilationKernelSize", morphologyClosingDilationKernelSize);
	return data;
}

std::optional<std::string> Preset::validate() const noexcept
{
	switch (extractionMode) {
	case ExtractionMode::Default:
	case ExtractionMode::Passthrough:
	case ExtractionMode::LuminanceExtraction:
	case ExtractionMode::SobelMagnitude:
	case ExtractionMode::EdgeDetection:
		break;
	default:
		return obs_module_text("configHelperInvalidExtractionMode");
	}

	switch (medianFilteringKernelSize) {
	case 1:
	case 3:
	case 5:
	case 7:
	case 9:
		break;
	default:
		return obs_module_text("configHelperInvalidMedianFilteringKernelSize");
	}

	switch (motionMapKernelSize) {
	case 1:
	case 3:
	case 5:
	case 7:
	case 9:
		break;
	default:
		return obs_module_text("configHelperInvalidMotionMapKernelSize");
	}

	if (motionAdaptiveFilteringStrength < 0.0 || motionAdaptiveFilteringStrength > 1.0) {
		return obs_module_text("configHelperInvalidMotionAdaptiveFilteringStrength");
	}

	if (motionAdaptiveFilteringMotionThreshold < 0.0 || motionAdaptiveFilteringMotionThreshold > 1.0) {
		return obs_module_text("configHelperInvalidMotionAdaptiveFilteringMotionThreshold");
	}

	if (hysteresisHighThreshold < 0.0 || hysteresisHighThreshold > 1.0) {
		return obs_module_text("configHelperInvalidHysteresisHighThreshold");
	}

	if (hysteresisLowThreshold < 0.0 || hysteresisLowThreshold > 1.0) {
		return obs_module_text("configHelperInvalidHysteresisLowThreshold");
	}

	if (morphologyOpeningErosionKernelSize < 1 || morphologyOpeningErosionKernelSize > 31 ||
	    morphologyOpeningErosionKernelSize % 2 == 0) {
		return obs_module_text("configHelperInvalidMorphologyOpeningErosionKernelSize");
	}

	if (morphologyOpeningDilationKernelSize < 1 || morphologyOpeningDilationKernelSize > 31 ||
	    morphologyOpeningDilationKernelSize % 2 == 0) {
		return obs_module_text("configHelperInvalidMorphologyOpeningDilationKernelSize");
	}

	if (morphologyClosingDilationKernelSize < 1 || morphologyClosingDilationKernelSize > 31 ||
	    morphologyClosingDilationKernelSize % 2 == 0) {
		return obs_module_text("configHelperInvalidMorphologyClosingDilationKernelSize");
	}

	if (morphologyClosingErosionKernelSize < 1 || morphologyClosingErosionKernelSize > 31 ||
	    morphologyClosingErosionKernelSize % 2 == 0) {
		return obs_module_text("configHelperInvalidMorphologyClosingErosionKernelSize");
	}

	return std::nullopt;
}

Preset Preset::fromObsData(obs_data_t *data) noexcept
{
	Preset preset;

	preset.presetName = obs_data_get_string(data, "presetName");
	preset.extractionMode = static_cast<ExtractionMode>(obs_data_get_int(data, "extractionMode"));
	preset.medianFilteringKernelSize = obs_data_get_int(data, "medianFilteringKernelSize");
	preset.motionMapKernelSize = obs_data_get_int(data, "motionMapKernelSize");
	preset.motionAdaptiveFilteringStrength = obs_data_get_double(data, "motionAdaptiveFilteringStrength");
	preset.motionAdaptiveFilteringMotionThreshold =
		obs_data_get_double(data, "motionAdaptiveFilteringMotionThreshold");
	preset.sobelMagnitudeFinalizationUseLog = obs_data_get_bool(data, "sobelMagnitudeFinalizationUseLog");
	preset.sobelMagnitudeFinalizationScalingFactorDb =
		obs_data_get_double(data, "sobelMagnitudeFinalizationScalingFactorDb");
	preset.hysteresisHighThreshold = obs_data_get_double(data, "hysteresisHighThreshold");
	preset.hysteresisLowThreshold = obs_data_get_double(data, "hysteresisLowThreshold");
	preset.morphologyOpeningErosionKernelSize = obs_data_get_int(data, "morphologyOpeningErosionKernelSize");
	preset.morphologyOpeningDilationKernelSize = obs_data_get_int(data, "morphologyOpeningDilationKernelSize");
	preset.morphologyClosingDilationKernelSize = obs_data_get_int(data, "morphologyClosingDilationKernelSize");
	preset.morphologyClosingErosionKernelSize = obs_data_get_int(data, "morphologyClosingErosionKernelSize");

	return preset;
}

void Preset::saveUserPresets(const std::vector<Preset> &presets) noexcept
{
	unique_bfree_t config_dir_path(obs_module_config_path(""));
	std::filesystem::create_directories(config_dir_path.get());

	unique_bfree_t config_path(obs_module_config_path(UserPresetsJson));

	unique_obs_data_t config_data(obs_data_create_from_json_file_safe(config_path.get(), "bak"));
	if (!config_data) {
		obs_log(LOG_ERROR, "Trying to create user presets file at %s", config_path.get());
		config_data.reset(obs_data_create());
	}

	obs_data_set_string(config_data.get(), "version", UserPresetsVersion);

	unique_obs_data_array_t settings_array(obs_data_array_create());

	std::vector<unique_obs_data_t> newPresets;
	for (size_t i = 0; i < presets.size(); i++) {
		const struct Preset &preset = presets[i];
		if (preset.presetName.length() >= 1 && preset.presetName[0] == ' ') {
			continue;
		}

		unique_obs_data_t newPreset(obs_data_create());
		preset.loadIntoObsData(newPreset.get());
		obs_data_array_push_back(settings_array.get(), newPreset.get());
		newPresets.push_back(std::move(newPreset));
	}

	obs_data_set_array(config_data.get(), "settings", settings_array.get());

	if (!obs_data_save_json_pretty_safe(config_data.get(), config_path.get(), "_", "bak")) {
		obs_log(LOG_ERROR, "Failed to save preset to %s", config_path.get());
	}
}

std::vector<Preset> Preset::loadUserPresets(const Preset &runningPreset) noexcept
{
	std::vector<Preset> presets{runningPreset, Preset::getStrongDefault()};

	unique_bfree_t configPath(obs_module_config_path(UserPresetsJson));
	unique_obs_data_t configData(obs_data_create_from_json_file_safe(configPath.get(), "bak"));

	if (!configData) {
		return presets;
	}

	unique_obs_data_array_t settingsArray(obs_data_get_array(configData.get(), "settings"));

	if (!settingsArray) {
		return presets;
	}

	for (size_t i = 0; i < obs_data_array_count(settingsArray.get()); ++i) {
		unique_obs_data_t presetData(obs_data_array_item(settingsArray.get(), i));
		if (!presetData) {
			continue;
		}

		const Preset preset = Preset::fromObsData(presetData.get());

		presets.push_back(preset);
	}

	return presets;
}

Preset Preset::getStrongDefault() noexcept
{
	Preset preset;

	preset.presetName = " strong default";
	preset.extractionMode = ExtractionMode::Default;
	preset.medianFilteringKernelSize = 3;
	preset.motionMapKernelSize = 3;
	preset.motionAdaptiveFilteringStrength = 0.5;
	preset.motionAdaptiveFilteringMotionThreshold = 0.3;
	preset.sobelMagnitudeFinalizationUseLog = true;
	preset.sobelMagnitudeFinalizationScalingFactorDb = 10.0;
	preset.hysteresisHighThreshold = 0.2;
	preset.hysteresisLowThreshold = 0.1;
	preset.hysteresisPropagationIterations = 8;
	preset.morphologyOpeningErosionKernelSize = 1;
	preset.morphologyOpeningDilationKernelSize = 1;
	preset.morphologyClosingDilationKernelSize = 7;
	preset.morphologyClosingErosionKernelSize = 5;

	return preset;
}

} // namespace obs_showdraw
} // namespace kaito_tokyo

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
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include <obs-module.h>

#include "plugin-support.h"

#include "Preset.hpp"

const char *UserPresetsJson = "UserPresets.json";
const char *UserPresetsVersion = "2025-09-03";

bool Preset::isSystem(void) const noexcept
{
	return presetName.size() >= 1 && presetName[0] == ' ';
}

bool Preset::isUser(void) const noexcept
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
	obs_data_set_int(data, "morphologyOpeningErosionKernelSize", morphologyOpeningErosionKernelSize);
	obs_data_set_int(data, "morphologyOpeningDilationKernelSize", morphologyOpeningDilationKernelSize);
	obs_data_set_int(data, "morphologyClosingErosionKernelSize", morphologyClosingErosionKernelSize);
	obs_data_set_int(data, "morphologyClosingDilationKernelSize", morphologyClosingDilationKernelSize);
	obs_data_set_double(data, "scalingFactorDb", scalingFactorDb);
	return data;
}

std::optional<std::string> Preset::validate(void) const noexcept
{
	switch (extractionMode) {
	case ExtractionMode::Default:
	case ExtractionMode::Passthrough:
	case ExtractionMode::LuminanceExtraction:
	case ExtractionMode::EdgeDetection:
	case ExtractionMode::Scaling:
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

	if (scalingFactorDb < -20.0 || scalingFactorDb > 20.0) {
		return obs_module_text("configHelperInvalidScalingFactorDB");
	}

	return std::nullopt;
}

Preset Preset::fromObsData(obs_data_t *data) noexcept
{
	return {
		obs_data_get_string(data, "presetName"),
		static_cast<ExtractionMode>(obs_data_get_int(data, "extractionMode")),
		obs_data_get_int(data, "medianFilteringKernelSize"),
		obs_data_get_int(data, "motionMapKernelSize"),
		obs_data_get_double(data, "motionAdaptiveFilteringStrength"),
		obs_data_get_double(data, "motionAdaptiveFilteringMotionThreshold"),
		obs_data_get_int(data, "morphologyOpeningErosionKernelSize"),
		obs_data_get_int(data, "morphologyOpeningDilationKernelSize"),
		obs_data_get_int(data, "morphologyClosingDilationKernelSize"),
		obs_data_get_int(data, "morphologyClosingErosionKernelSize"),
		obs_data_get_double(data, "scalingFactorDb"),
	};
}

void Preset::saveUserPresets(const std::vector<Preset> &presets) noexcept
{
	char *config_dir_path = obs_module_config_path("");
	std::filesystem::create_directories(config_dir_path);
	bfree(config_dir_path);

	char *config_path = obs_module_config_path(UserPresetsJson);

	obs_data_t *config_data = obs_data_create();
	if (!config_data) {
		obs_log(LOG_ERROR, "Trying to create user presets file at %s", config_path);
		config_data = obs_data_create();
	}

	obs_data_set_string(config_data, "version", UserPresetsVersion);

	obs_data_array_t *settings_array = obs_data_array_create();

	for (size_t i = 0; i < presets.size(); i++) {
		const struct Preset &preset = presets[i];
		if (preset.presetName.length() >= 1 && preset.presetName[0] == ' ') {
			continue;
		}

		obs_data_t *newPreset = obs_data_create();
		preset.loadIntoObsData(newPreset);
		obs_data_array_push_back(settings_array, newPreset);
		obs_data_release(newPreset);
	}

	obs_data_set_array(config_data, "settings", settings_array);

	if (!obs_data_save_json_pretty_safe(config_data, config_path, "_", "bak")) {
		obs_log(LOG_ERROR, "Failed to save preset to %s", config_path);
	}

	obs_data_array_release(settings_array);
	obs_data_release(config_data);
	bfree(config_path);
}

std::vector<Preset> Preset::loadUserPresets(const Preset &runningPreset) noexcept
{
	std::vector<Preset> presets{runningPreset, Preset::getStrongDefault()};

	char *configPath = obs_module_config_path(UserPresetsJson);
	obs_data_t *configData = obs_data_create_from_json_file_safe(configPath, "bak");
	bfree(configPath);

	if (!configData) {
		return presets;
	}

	obs_data_array_t *settingsArray = obs_data_get_array(configData, "settings");

	if (!settingsArray) {
		return presets;
	}

	for (size_t i = 0; i < obs_data_array_count(settingsArray); ++i) {
		obs_data_t *presetData = obs_data_array_item(settingsArray, i);
		if (!presetData) {
			continue;
		}

		const Preset preset = Preset::fromObsData(presetData);

		presets.push_back(preset);
		obs_data_release(presetData);
	}

	obs_data_array_release(settingsArray);
	obs_data_release(configData);

	return presets;
}

Preset Preset::getStrongDefault(void) noexcept
{
	return {
		"strong default",
		ExtractionMode::Default,
		3,
		3,
		0.5,
		0.3,
		1,
		1,
		7,
		5,
		6.0,
	};
}

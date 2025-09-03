#pragma once

#include <time.h>

#include <obs-module.h>
#include <plugin-support.h>
#include <util/dstr.h>
#include <util/platform.h>

#include "showdraw-preset.hpp"

#ifdef __cplusplus
extern "C" {
#endif

const char *USER_PRESETS_JSON = "UserPresets.json";
const char *USER_PRESETS_VERSION = "2025-09-03";

static void showdraw_conf_load_preset_into_obs_data(obs_data_t *data, const struct showdraw_preset *preset)
{
	obs_data_set_int(data, "extractionMode", preset->extraction_mode);
	obs_data_set_int(data, "medianFilteringKernelSize", preset->median_filtering_kernel_size);
	obs_data_set_int(data, "motionMapKernelSize", preset->motion_map_kernel_size);
	obs_data_set_double(data, "motionAdaptiveFilteringStrength", preset->motion_adaptive_filtering_strength);
	obs_data_set_double(data, "motionAdaptiveFilteringMotionThreshold",
			    preset->motion_adaptive_filtering_motion_threshold);
	obs_data_set_int(data, "morphologyOpeningErosionKernelSize", preset->morphology_opening_erosion_kernel_size);
	obs_data_set_int(data, "morphologyOpeningDilationKernelSize", preset->morphology_opening_dilation_kernel_size);
	obs_data_set_int(data, "morphologyClosingDilationKernelSize", preset->morphology_closing_dilation_kernel_size);
	obs_data_set_int(data, "morphologyClosingErosionKernelSize", preset->morphology_closing_erosion_kernel_size);
	obs_data_set_double(data, "scalingFactorDb", preset->scaling_factor_db);
}

static struct showdraw_preset *showdraw_conf_load_preset_from_obs_data(obs_data_t *data)
{
	const char *preset_name = obs_data_get_string(data, "presetName");

	struct showdraw_preset *preset = showdraw_preset_create(preset_name, false);
	if (!preset) {
		obs_log(LOG_ERROR, "Failed to create preset");
		return NULL;
	}

	preset->extraction_mode = obs_data_get_int(data, "extractionMode");
	preset->median_filtering_kernel_size = obs_data_get_int(data, "medianFilteringKernelSize");
	preset->motion_map_kernel_size = obs_data_get_int(data, "motionMapKernelSize");
	preset->motion_adaptive_filtering_strength = obs_data_get_double(data, "motionAdaptiveFilteringStrength");
	preset->motion_adaptive_filtering_motion_threshold =
		obs_data_get_double(data, "motionAdaptiveFilteringMotionThreshold");
	preset->morphology_opening_erosion_kernel_size = obs_data_get_int(data, "morphologyOpeningErosionKernelSize");
	preset->morphology_opening_dilation_kernel_size = obs_data_get_int(data, "morphologyOpeningDilationKernelSize");
	preset->morphology_closing_dilation_kernel_size = obs_data_get_int(data, "morphologyClosingDilationKernelSize");
	preset->morphology_closing_erosion_kernel_size = obs_data_get_int(data, "morphologyClosingErosionKernelSize");
	preset->scaling_factor_db = obs_data_get_double(data, "scalingFactorDb");

	return preset;
}

typedef char showdraw_conf_timestamp_string_t[sizeof("1990-01-01T00:00:00+00:00")];

static bool showdraw_conf_generate_local_timestamp(showdraw_conf_timestamp_string_t buf)
{
	time_t t = time(NULL);

	struct tm local_tm;

#ifdef _WIN32
	errno_t err = localtime_s(&local_tm, &t);
	if (err != 0) {
		return false;
	}
#else
	if (!localtime_r(&t, &local_tm)) {
		return false;
	}
#endif

	strftime(buf, sizeof(showdraw_conf_timestamp_string_t), "%Y-%m-%dT%H:%M:%S", &local_tm);
	strftime(buf + 19, sizeof(showdraw_conf_timestamp_string_t) - 19, "%z", &local_tm);
	buf[sizeof(showdraw_conf_timestamp_string_t) - 1] = '\0';
	buf[sizeof(showdraw_conf_timestamp_string_t) - 2] = buf[sizeof(showdraw_conf_timestamp_string_t) - 3];
	buf[sizeof(showdraw_conf_timestamp_string_t) - 3] = buf[sizeof(showdraw_conf_timestamp_string_t) - 4];
	buf[sizeof(showdraw_conf_timestamp_string_t) - 4] = ':';

	return true;
}

static void showdraw_conf_save_user_presets(struct showdraw_preset **presets, size_t count)
{
	char *config_dir_path = obs_module_config_path("");
	os_mkdirs(config_dir_path);
	bfree(config_dir_path);

	char *config_path = obs_module_config_path(USER_PRESETS_JSON);

	obs_data_t *config_data = obs_data_create();
	if (!config_data) {
		obs_log(LOG_ERROR, "Trying to create user presets file at %s", config_path);
		config_data = obs_data_create();
	}

	obs_data_set_string(config_data, "version", USER_PRESETS_VERSION);

	showdraw_conf_timestamp_string_t timestamp;
	showdraw_conf_generate_local_timestamp(timestamp);
	obs_data_set_string(config_data, "lastModified", timestamp);

	obs_data_array_t *settings_array = obs_data_array_create();

	for (size_t i = 0; i < count; i++) {
		const struct showdraw_preset *preset = presets[i];
		if (preset->preset_name.len >= 1 && preset->preset_name.array[0] == ' ') {
			continue;
		}

		obs_data_t *new_preset = obs_data_create();
		showdraw_conf_load_preset_into_obs_data(new_preset, preset);
		obs_data_set_string(new_preset, "presetName", preset->preset_name.array);
		obs_data_array_push_back(settings_array, new_preset);
		obs_data_release(new_preset);
	}

	obs_data_set_array(config_data, "settings", settings_array);

	if (!obs_data_save_json_pretty_safe(config_data, config_path, "_", "bak")) {
		obs_log(LOG_ERROR, "Failed to save preset to %s", config_path);
	}

	obs_data_array_release(settings_array);
	obs_data_release(config_data);
	bfree(config_path);
}

static const char *showdraw_conf_validate_settings(const struct showdraw_preset *preset)
{
	switch (preset->extraction_mode) {
	case EXTRACTION_MODE_DEFAULT:
	case EXTRACTION_MODE_PASSTHROUGH:
	case EXTRACTION_MODE_LUMINANCE_EXTRACTION:
	case EXTRACTION_MODE_EDGE_DETECTION:
	case EXTRACTION_MODE_SCALING:
		break;
	default:
		return obs_module_text("configHelperInvalidExtractionMode");
	}

	switch (preset->median_filtering_kernel_size) {
	case 1:
	case 3:
	case 5:
	case 7:
	case 9:
		break;
	default:
		return obs_module_text("configHelperInvalidMedianFilteringKernelSize");
	}

	switch (preset->motion_map_kernel_size) {
	case 1:
	case 3:
	case 5:
	case 7:
	case 9:
		break;
	default:
		return obs_module_text("configHelperInvalidMotionMapKernelSize");
	}

	if (preset->motion_adaptive_filtering_strength < 0.0 || preset->motion_adaptive_filtering_strength > 1.0) {
		return obs_module_text("configHelperInvalidMotionAdaptiveFilteringStrength");
	}

	if (preset->motion_adaptive_filtering_motion_threshold < 0.0 ||
	    preset->motion_adaptive_filtering_motion_threshold > 1.0) {
		return obs_module_text("configHelperInvalidMotionAdaptiveFilteringMotionThreshold");
	}

	if (preset->morphology_opening_erosion_kernel_size < 1 || preset->morphology_opening_erosion_kernel_size > 31 ||
	    preset->morphology_opening_erosion_kernel_size % 2 == 0) {
		return obs_module_text("configHelperInvalidMorphologyOpeningErosionKernelSize");
	}

	if (preset->morphology_opening_dilation_kernel_size < 1 ||
	    preset->morphology_opening_dilation_kernel_size > 31 ||
	    preset->morphology_opening_dilation_kernel_size % 2 == 0) {
		return obs_module_text("configHelperInvalidMorphologyOpeningDilationKernelSize");
	}

	if (preset->morphology_closing_dilation_kernel_size < 1 ||
	    preset->morphology_closing_dilation_kernel_size > 31 ||
	    preset->morphology_closing_dilation_kernel_size % 2 == 0) {
		return obs_module_text("configHelperInvalidMorphologyClosingDilationKernelSize");
	}

	if (preset->morphology_closing_erosion_kernel_size < 1 || preset->morphology_closing_erosion_kernel_size > 31 ||
	    preset->morphology_closing_erosion_kernel_size % 2 == 0) {
		return obs_module_text("configHelperInvalidMorphologyClosingErosionKernelSize");
	}

	if (preset->scaling_factor_db < -20.0 || preset->scaling_factor_db > 20.0) {
		return obs_module_text("configHelperInvalidScalingFactorDB");
	}

	return NULL;
}

#ifdef __cplusplus
}
#endif

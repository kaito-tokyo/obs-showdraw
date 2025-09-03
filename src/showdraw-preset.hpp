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

#include <obs-module.h>
#include <plugin-support.h>
#include <util/dstr.h>

#ifdef __cplusplus
extern "C" {
#endif

const long long EXTRACTION_MODE_DEFAULT = 0;
const long long EXTRACTION_MODE_PASSTHROUGH = 100;
const long long EXTRACTION_MODE_LUMINANCE_EXTRACTION = 200;
const long long EXTRACTION_MODE_EDGE_DETECTION = 300;
const long long EXTRACTION_MODE_SCALING = 400;

struct showdraw_preset {
	// Names which begins with whitespace are reserved for internal use
	struct dstr preset_name;

	long long extraction_mode;

	long long median_filtering_kernel_size;

	long long motion_map_kernel_size;

	double motion_adaptive_filtering_strength;
	double motion_adaptive_filtering_motion_threshold;

	long long morphology_opening_erosion_kernel_size;
	long long morphology_opening_dilation_kernel_size;

	long long morphology_closing_dilation_kernel_size;
	long long morphology_closing_erosion_kernel_size;

	double scaling_factor_db;
};

static struct showdraw_preset *showdraw_preset_create(const char *preset_name, bool is_system)
{
	if (preset_name == NULL) {
		obs_log(LOG_ERROR, "Preset name is NULL");
		return NULL;
	}

	if (is_system && preset_name[0] != ' ') {
		obs_log(LOG_ERROR, "System preset name must begin with a whitespace");
		return NULL;
	} else if (!is_system && preset_name[0] == ' ') {
		obs_log(LOG_ERROR, "User preset name must not begin with a whitespace");
		return NULL;
	}

	struct showdraw_preset *preset = (struct showdraw_preset *)bzalloc(sizeof(*preset));

	if (preset == NULL) {
		obs_log(LOG_ERROR, "Failed to allocate memory for preset");
		return NULL;
	}

	dstr_init_copy(&preset->preset_name, preset_name);

	if (preset->preset_name.array == NULL) {
		obs_log(LOG_ERROR, "Failed to allocate memory for preset name");
		bfree(preset);
		return NULL;
	}

	return preset;
}

static void showdraw_preset_copy(struct showdraw_preset *dest, const struct showdraw_preset *src)
{
	if (!dest || !src) {
		obs_log(LOG_ERROR, "Invalid preset copy");
		return;
	}

	struct dstr preset_name = dest->preset_name;
	*dest = *src;
	dest->preset_name = preset_name;
}

static bool showdraw_preset_is_system(struct showdraw_preset *preset)
{
	if (!preset) {
		obs_log(LOG_ERROR, "Preset is NULL");
		return false;
	}

	if (preset->preset_name.array == NULL) {
		obs_log(LOG_ERROR, "Preset name is NULL");
		return false;
	}

	return preset->preset_name.array[0] == ' ';
}

static void showdraw_preset_destroy(struct showdraw_preset *preset)
{
	if (!preset) {
		obs_log(LOG_DEBUG, "Preset is NULL");
		return;
	}

	dstr_free(&preset->preset_name);
	bfree(preset);
}

static struct showdraw_preset *showdraw_preset_get_strong_default(void)
{
	struct showdraw_preset *preset = showdraw_preset_create(" strong default", true);

	preset->extraction_mode = EXTRACTION_MODE_DEFAULT;
	preset->median_filtering_kernel_size = 3;
	preset->motion_map_kernel_size = 3;
	preset->motion_adaptive_filtering_strength = 0.5;
	preset->motion_adaptive_filtering_motion_threshold = 0.3;
	preset->morphology_opening_erosion_kernel_size = 1;
	preset->morphology_opening_dilation_kernel_size = 1;
	preset->morphology_closing_dilation_kernel_size = 7;
	preset->morphology_closing_erosion_kernel_size = 5;
	preset->scaling_factor_db = 6.0;

	return preset;
}

#ifdef __cplusplus
}
#endif

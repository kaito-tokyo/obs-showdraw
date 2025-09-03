#pragma once

#include <obs-module.h>
#include <util/dstr.h>

#ifdef __cplusplus
extern "C" {
#endif

const long long EXTRACTION_MODE_DEFAULT = 0;
const long long EXTRACTION_MODE_PASSTHROUGH = 100;
const long long EXTRACTION_MODE_LUMINANCE_EXTRACTION = 200;
const long long EXTRACTION_MODE_EDGE_DETECTION = 300;
const long long EXTRACTION_MODE_SCALING = 400;

struct settings {
	obs_source_t *filter;

	struct dstr preset_name;
	bool is_system;

	long long extraction_mode;

	long long median_filtering_kernel_size;

	long long motion_map_kernel_size;

	double motion_adaptive_filtering_strength;
	double motion_adaptive_filtering_motion_threshold;

	long long morphology_opening_erosion_kernel_size;
	long long morphology_opening_dilation_kernel_size;

	long long morphology_closing_dilation_kernel_size;
	long long morphology_closing_erosion_kernel_size;

	double scaling_factor;
	double scaling_factor_db;
};

#ifdef __cplusplus
}
#endif

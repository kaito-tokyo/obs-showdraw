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

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>

#include <math.h>

#include "PresetWindow.hpp"
#include "showdraw-preset.hpp"
#include "showdraw-global-state.hpp"

#define EXTRACTION_MODE_DEFAULT_VALUE EXTRACTION_MODE_SCALING

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

struct showdraw_filter_context {
	struct showdraw_global_state global_state;

	double scaling_factor;

	gs_texture_t *texture_source;
	gs_texture_t *texture_target;
	gs_texture_t *texture_motion_map;
	gs_texture_t *texture_previous_luminance;

	gs_effect_t *effect;

	gs_eparam_t *effect_texture_image;
	gs_eparam_t *effect_texture_image1;

	gs_eparam_t *effect_float_texel_width;
	gs_eparam_t *effect_float_texel_height;
	gs_eparam_t *effect_int_kernel_size;

	gs_eparam_t *effect_texture_motion_map;
	gs_eparam_t *effect_float_strength;
	gs_eparam_t *effect_float_motion_threshold;

	gs_eparam_t *effect_float_scaling_factor;

	gs_technique_t *effect_tech_draw;
	gs_technique_t *effect_tech_extract_luminance;
	gs_technique_t *effect_tech_median_filtering;
	gs_technique_t *effect_tech_calculate_motion_map;
	gs_technique_t *effect_tech_motion_adaptive_filtering;
	gs_technique_t *effect_tech_apply_sobel;
	gs_technique_t *effect_tech_suppress_non_maximum;
	gs_technique_t *effect_tech_detect_edge;
	gs_technique_t *effect_tech_erosion;
	gs_technique_t *effect_tech_dilation;
	gs_technique_t *effect_tech_scaling;
};

const char *showdraw_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("showdrawName");
}

void showdraw_update(void *data, obs_data_t *settings);

void *showdraw_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(settings);

	obs_log(LOG_INFO, "Creating showdraw source");

	struct showdraw_filter_context *context = bzalloc(sizeof(*context));

	if (context == NULL) {
		obs_log(LOG_ERROR, "Failed to allocate memory for showdraw context");
		return NULL;
	}

	context->global_state.filter = source;
	context->global_state.running_preset = showdraw_preset_create(" working", true);

	if (context->global_state.running_preset == NULL) {
		obs_log(LOG_ERROR, "Failed to create showdraw preset");
		bfree(context);
		return NULL;
	}

	showdraw_update(context, settings);

	return context;
}

void showdraw_destroy(void *data)
{
	obs_log(LOG_INFO, "Destroying showdraw source");

	struct showdraw_filter_context *context = (struct showdraw_filter_context *)data;

	showdraw_preset_destroy(context->global_state.running_preset);

	obs_enter_graphics();
	gs_texture_destroy(context->texture_source);
	gs_texture_destroy(context->texture_target);
	gs_texture_destroy(context->texture_motion_map);
	gs_texture_destroy(context->texture_previous_luminance);
	gs_effect_destroy(context->effect);
	obs_leave_graphics();

	bfree(context);
}

void showdraw_get_defaults(obs_data_t *data)
{
	struct showdraw_preset *default_preset = showdraw_preset_get_strong_default();

	obs_data_set_default_int(data, "extractionMode", default_preset->extraction_mode);

	obs_data_set_default_int(data, "medianFilteringKernelSize", default_preset->median_filtering_kernel_size);

	obs_data_set_default_int(data, "motionMapKernelSize", default_preset->motion_map_kernel_size);

	obs_data_set_default_double(data, "motionAdaptiveFilteringStrength",
				    default_preset->motion_adaptive_filtering_strength);
	obs_data_set_default_double(data, "motionAdaptiveFilteringMotionThreshold",
				    default_preset->motion_adaptive_filtering_motion_threshold);

	obs_data_set_default_int(data, "morphologyOpeningErosionKernelSize",
				 default_preset->morphology_opening_erosion_kernel_size);
	obs_data_set_default_int(data, "morphologyOpeningDilationKernelSize",
				 default_preset->morphology_opening_dilation_kernel_size);
	obs_data_set_default_int(data, "morphologyClosingDilationKernelSize",
				 default_preset->morphology_closing_dilation_kernel_size);
	obs_data_set_default_int(data, "morphologyClosingErosionKernelSize",
				 default_preset->morphology_closing_erosion_kernel_size);

	obs_data_set_default_double(data, "scalingFactorDb", default_preset->scaling_factor_db);

	showdraw_preset_destroy(default_preset);
}

bool open_preset_window(obs_properties_t *props, obs_property_t *property, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);

	struct showdraw_filter_context *context = (struct showdraw_filter_context *)data;

	showdraw_preset_window_show(&context->global_state);

	return false;
}

obs_properties_t *showdraw_get_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_button2(props, "openPresetWindow", obs_module_text("openPresetWindow"), open_preset_window,
				   data);

	obs_property_t *propExtractionMode = obs_properties_add_list(
		props, "extractionMode", obs_module_text("extractionMode"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModeDefault"),
				  EXTRACTION_MODE_DEFAULT);
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModePassthrough"),
				  EXTRACTION_MODE_PASSTHROUGH);
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModeLuminanceExtraction"),
				  EXTRACTION_MODE_LUMINANCE_EXTRACTION);
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModeEdgeDetection"),
				  EXTRACTION_MODE_EDGE_DETECTION);
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModeScaling"),
				  EXTRACTION_MODE_SCALING);

	obs_property_t *propMedianFilteringKernelSize = obs_properties_add_list(
		props, "medianFilteringKernelSize", obs_module_text("medianFilteringKernelSize"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(propMedianFilteringKernelSize, obs_module_text("medianFilteringKernelSize1"), 1);
	obs_property_list_add_int(propMedianFilteringKernelSize, obs_module_text("medianFilteringKernelSize3"), 3);
	obs_property_list_add_int(propMedianFilteringKernelSize, obs_module_text("medianFilteringKernelSize5"), 5);
	obs_property_list_add_int(propMedianFilteringKernelSize, obs_module_text("medianFilteringKernelSize7"), 7);
	obs_property_list_add_int(propMedianFilteringKernelSize, obs_module_text("medianFilteringKernelSize9"), 9);

	obs_property_t *propMMotionMapKernelSize = obs_properties_add_list(props, "motionMapKernelSize",
									   obs_module_text("motionMapKernelSize"),
									   OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(propMMotionMapKernelSize, obs_module_text("motionMapKernelSize1"), 1);
	obs_property_list_add_int(propMMotionMapKernelSize, obs_module_text("motionMapKernelSize3"), 3);
	obs_property_list_add_int(propMMotionMapKernelSize, obs_module_text("motionMapKernelSize5"), 5);
	obs_property_list_add_int(propMMotionMapKernelSize, obs_module_text("motionMapKernelSize7"), 7);
	obs_property_list_add_int(propMMotionMapKernelSize, obs_module_text("motionMapKernelSize9"), 9);

	obs_properties_add_float_slider(props, "motionAdaptiveFilteringStrength",
					obs_module_text("motionAdaptiveFilteringStrength"), 0.0, 1.0, 0.01);
	obs_properties_add_float_slider(props, "motionAdaptiveFilteringMotionThreshold",
					obs_module_text("motionAdaptiveFilteringMotionThreshold"), 0.0, 1.0, 0.01);

	obs_properties_add_int_slider(props, "morphologyOpeningErosionKernelSize",
				      obs_module_text("morphologyOpeningErosionKernelSize"), 1, 31, 2);
	obs_properties_add_int_slider(props, "morphologyOpeningDilationKernelSize",
				      obs_module_text("morphologyOpeningDilationKernelSize"), 1, 31, 2);

	obs_properties_add_int_slider(props, "morphologyClosingDilationKernelSize",
				      obs_module_text("morphologyClosingDilationKernelSize"), 1, 31, 2);
	obs_properties_add_int_slider(props, "morphologyClosingErosionKernelSize",
				      obs_module_text("morphologyClosingErosionKernelSize"), 1, 31, 2);

	obs_properties_add_float_slider(props, "scalingFactorDb", obs_module_text("scalingFactor"), -20.0, 20.0, 0.01);

	return props;
}

void showdraw_update(void *data, obs_data_t *settings)
{
	struct showdraw_filter_context *context = (struct showdraw_filter_context *)data;
	struct showdraw_preset *running_preset = context->global_state.running_preset;

	running_preset->extraction_mode = obs_data_get_int(settings, "extractionMode");

	running_preset->median_filtering_kernel_size = obs_data_get_int(settings, "medianFilteringKernelSize");

	running_preset->motion_map_kernel_size = obs_data_get_int(settings, "motionMapKernelSize");

	running_preset->motion_adaptive_filtering_strength =
		obs_data_get_double(settings, "motionAdaptiveFilteringStrength");
	running_preset->motion_adaptive_filtering_motion_threshold =
		obs_data_get_double(settings, "motionAdaptiveFilteringMotionThreshold");

	running_preset->morphology_opening_erosion_kernel_size =
		obs_data_get_int(settings, "morphologyOpeningErosionKernelSize");
	running_preset->morphology_opening_dilation_kernel_size =
		obs_data_get_int(settings, "morphologyOpeningDilationKernelSize");
	running_preset->morphology_closing_dilation_kernel_size =
		obs_data_get_int(settings, "morphologyClosingDilationKernelSize");
	running_preset->morphology_closing_erosion_kernel_size =
		obs_data_get_int(settings, "morphologyClosingErosionKernelSize");

	running_preset->scaling_factor_db = obs_data_get_double(settings, "scalingFactorDb");
	context->scaling_factor = pow(10.0, running_preset->scaling_factor_db / 10.0);
}

void swap_source_and_target_textures(struct showdraw_filter_context *context)
{
	gs_texture_t *temp = context->texture_source;
	context->texture_source = context->texture_target;
	context->texture_target = temp;
}

#define CHECK_EFFECT_PARAM(name) \
	if (!context->effect_##name) { \
		obs_log(LOG_ERROR, "Effect parameter " #name " not found"); \
		obs_source_skip_video_filter(filter); \
		return false; \
	}

#define CHECK_EFFECT_TECH(name) \
	if (!context->effect_tech_##name) { \
		obs_log(LOG_ERROR, "Effect technique " #name " not found"); \
		obs_source_skip_video_filter(filter); \
		return false; \
	}

static bool init_effect(struct showdraw_filter_context *context)
{
	obs_source_t *filter = context->global_state.filter;
	char *error_string = NULL;

	char *effect_path = obs_module_file("effects/drawing-emphasizer.effect");
	if (!effect_path) {
		obs_log(LOG_ERROR, "Failed to get effect path");
		obs_source_skip_video_filter(filter);
		return false;
	}

	context->effect = gs_effect_create_from_file(effect_path, &error_string);
	bfree(effect_path);
	if (!context->effect) {
		obs_log(LOG_ERROR, "Error loading effect: %s", error_string);
		bfree(error_string);
		obs_source_skip_video_filter(filter);
		return false;
	}

	context->effect_texture_image = gs_effect_get_param_by_name(context->effect, "image");
	CHECK_EFFECT_PARAM(texture_image);
	context->effect_texture_image1 = gs_effect_get_param_by_name(context->effect, "image1");
	CHECK_EFFECT_PARAM(texture_image1);

	context->effect_float_texel_width = gs_effect_get_param_by_name(context->effect, "texelWidth");
	CHECK_EFFECT_PARAM(float_texel_width);
	context->effect_float_texel_height = gs_effect_get_param_by_name(context->effect, "texelHeight");
	CHECK_EFFECT_PARAM(float_texel_height);
	context->effect_int_kernel_size = gs_effect_get_param_by_name(context->effect, "kernelSize");
	CHECK_EFFECT_PARAM(int_kernel_size);

	context->effect_texture_motion_map = gs_effect_get_param_by_name(context->effect, "motionMap");
	CHECK_EFFECT_PARAM(texture_motion_map);
	context->effect_float_strength = gs_effect_get_param_by_name(context->effect, "strength");
	CHECK_EFFECT_PARAM(float_strength);
	context->effect_float_motion_threshold = gs_effect_get_param_by_name(context->effect, "motionThreshold");
	CHECK_EFFECT_PARAM(float_motion_threshold);

	context->effect_float_scaling_factor = gs_effect_get_param_by_name(context->effect, "scalingFactor");
	CHECK_EFFECT_PARAM(float_scaling_factor);

	context->effect_tech_draw = gs_effect_get_technique(context->effect, "Draw");
	CHECK_EFFECT_TECH(draw);
	context->effect_tech_extract_luminance = gs_effect_get_technique(context->effect, "ExtractLuminance");
	CHECK_EFFECT_TECH(extract_luminance);
	context->effect_tech_median_filtering = gs_effect_get_technique(context->effect, "MedianFiltering");
	CHECK_EFFECT_TECH(median_filtering);
	context->effect_tech_calculate_motion_map = gs_effect_get_technique(context->effect, "CalculateMotionMap");
	CHECK_EFFECT_TECH(calculate_motion_map);
	context->effect_tech_motion_adaptive_filtering =
		gs_effect_get_technique(context->effect, "MotionAdaptiveFiltering");
	CHECK_EFFECT_TECH(motion_adaptive_filtering);
	context->effect_tech_apply_sobel = gs_effect_get_technique(context->effect, "ApplySobel");
	CHECK_EFFECT_TECH(apply_sobel);
	context->effect_tech_suppress_non_maximum = gs_effect_get_technique(context->effect, "SuppressNonMaximum");
	CHECK_EFFECT_TECH(suppress_non_maximum);
	context->effect_tech_detect_edge = gs_effect_get_technique(context->effect, "DetectEdge");
	CHECK_EFFECT_TECH(detect_edge);
	context->effect_tech_erosion = gs_effect_get_technique(context->effect, "Erosion");
	CHECK_EFFECT_TECH(erosion);
	context->effect_tech_dilation = gs_effect_get_technique(context->effect, "Dilation");
	CHECK_EFFECT_TECH(dilation);
	context->effect_tech_scaling = gs_effect_get_technique(context->effect, "Scaling");
	CHECK_EFFECT_TECH(scaling);

	return true;
}

static bool ensure_texture(obs_source_t *filter, gs_texture_t **texture, uint32_t width, uint32_t height,
			   const char *name)
{
	if (!*texture || gs_texture_get_width(*texture) != width || gs_texture_get_height(*texture) != height) {
		if (*texture) {
			gs_texture_destroy(*texture);
			*texture = NULL;
		}
		*texture = gs_texture_create(width, height, GS_BGRA, 1, NULL, GS_RENDER_TARGET);
		if (!*texture) {
			obs_log(LOG_ERROR, "Could not create %s texture", name);
			obs_source_skip_video_filter(filter);
			return false;
		}
	}
	return true;
}

static bool ensure_textures(struct showdraw_filter_context *context, uint32_t width, uint32_t height)
{
	obs_source_t *filter = context->global_state.filter;
	if (!ensure_texture(filter, &context->texture_source, width, height, "source"))
		return false;
	if (!ensure_texture(filter, &context->texture_target, width, height, "target"))
		return false;
	if (!ensure_texture(filter, &context->texture_motion_map, width, height, "motion map"))
		return false;
	if (!ensure_texture(filter, &context->texture_previous_luminance, width, height, "previous luminance"))
		return false;
	return true;
}

static void apply_effect_pass(gs_technique_t *technique, gs_texture_t *texture)
{
	size_t passes = gs_technique_begin(technique);
	for (size_t i = 0; i < passes; i++) {
		if (gs_technique_begin_pass(technique, i)) {
			gs_draw_sprite(texture, 0, 0, 0);
			gs_technique_end_pass(technique);
		}
	}
	gs_technique_end(technique);
}

static void apply_luminance_extraction_pass(struct showdraw_filter_context *context)
{
	gs_set_render_target(context->texture_target, NULL);
	gs_effect_set_texture(context->effect_texture_image, context->texture_source);
	apply_effect_pass(context->effect_tech_extract_luminance, context->texture_source);
	swap_source_and_target_textures(context);
}

static void apply_median_filtering_pass(struct showdraw_filter_context *context, const float texel_width,
					const float texel_height)
{
	gs_set_render_target(context->texture_target, NULL);

	gs_effect_set_texture(context->effect_texture_image, context->texture_source);

	gs_effect_set_float(context->effect_float_texel_width, texel_width);
	gs_effect_set_float(context->effect_float_texel_height, texel_height);
	gs_effect_set_int(context->effect_int_kernel_size,
			  (int)context->global_state.running_preset->median_filtering_kernel_size);

	apply_effect_pass(context->effect_tech_median_filtering, context->texture_source);

	swap_source_and_target_textures(context);
}

static void apply_motion_adaptive_filtering_pass(struct showdraw_filter_context *context, const float texel_width,
						 const float texel_height)
{
	struct showdraw_preset *running_preset = context->global_state.running_preset;

	gs_set_render_target(context->texture_motion_map, NULL);

	gs_effect_set_texture(context->effect_texture_image, context->texture_source);
	gs_effect_set_texture(context->effect_texture_image1, context->texture_previous_luminance);

	gs_effect_set_float(context->effect_float_texel_width, texel_width);
	gs_effect_set_float(context->effect_float_texel_height, texel_height);
	gs_effect_set_int(context->effect_int_kernel_size, (int)running_preset->motion_map_kernel_size);

	apply_effect_pass(context->effect_tech_calculate_motion_map, context->texture_source);

	gs_set_render_target(context->texture_target, NULL);
	gs_effect_set_texture(context->effect_texture_image, context->texture_source);
	gs_effect_set_texture(context->effect_texture_image1, context->texture_previous_luminance);

	gs_effect_set_float(context->effect_float_texel_width, texel_width);
	gs_effect_set_float(context->effect_float_texel_height, texel_height);

	gs_effect_set_texture(context->effect_texture_motion_map, context->texture_motion_map);
	gs_effect_set_float(context->effect_float_strength, (float)running_preset->motion_adaptive_filtering_strength);
	gs_effect_set_float(context->effect_float_motion_threshold,
			    (float)running_preset->motion_adaptive_filtering_motion_threshold);

	apply_effect_pass(context->effect_tech_motion_adaptive_filtering, context->texture_source);

	gs_copy_texture(context->texture_previous_luminance, context->texture_target);

	swap_source_and_target_textures(context);
}

static void apply_sobel_pass(struct showdraw_filter_context *context, const float texel_width,
				      const float texel_height)
{
	gs_set_render_target(context->texture_target, NULL);

	gs_effect_set_texture(context->effect_texture_image, context->texture_source);

	gs_effect_set_float(context->effect_float_texel_width, texel_width);
	gs_effect_set_float(context->effect_float_texel_height, texel_height);

	apply_effect_pass(context->effect_tech_apply_sobel, context->texture_source);

	swap_source_and_target_textures(context);
}

static void apply_supress_non_maximum_pass(struct showdraw_filter_context *context, const float texel_width,
				      const float texel_height)
{
	gs_set_render_target(context->texture_target, NULL);

	gs_effect_set_texture(context->effect_texture_image, context->texture_source);

	gs_effect_set_float(context->effect_float_texel_width, texel_width);
	gs_effect_set_float(context->effect_float_texel_height, texel_height);

	apply_effect_pass(context->effect_tech_suppress_non_maximum, context->texture_source);

	swap_source_and_target_textures(context);
}

static void apply_edge_detection_pass(struct showdraw_filter_context *context, const float texel_width,
				      const float texel_height)
{
	gs_set_render_target(context->texture_target, NULL);

	gs_effect_set_texture(context->effect_texture_image, context->texture_source);

	gs_effect_set_float(context->effect_float_texel_width, texel_width);
	gs_effect_set_float(context->effect_float_texel_height, texel_height);

	apply_effect_pass(context->effect_tech_detect_edge, context->texture_source);

	swap_source_and_target_textures(context);
}

static void apply_morphology_pass(struct showdraw_filter_context *context, const float texel_width,
				  const float texel_height, gs_technique_t *technique, int kernel_size)
{
	gs_set_render_target(context->texture_target, NULL);

	gs_effect_set_texture(context->effect_texture_image, context->texture_source);

	gs_effect_set_float(context->effect_float_texel_width, texel_width);
	gs_effect_set_float(context->effect_float_texel_height, texel_height);
	gs_effect_set_int(context->effect_int_kernel_size, kernel_size);

	apply_effect_pass(technique, context->texture_source);

	swap_source_and_target_textures(context);
}

static void apply_scaling_pass(struct showdraw_filter_context *context)
{
	gs_set_render_target(context->texture_target, NULL);

	gs_effect_set_texture(context->effect_texture_image, context->texture_source);

	gs_effect_set_float(context->effect_float_scaling_factor, (float)context->scaling_factor);

	apply_effect_pass(context->effect_tech_scaling, context->texture_source);

	swap_source_and_target_textures(context);
}

static void draw_final_image(struct showdraw_filter_context *context)
{
	gs_effect_set_texture(context->effect_texture_image, context->texture_source);

	size_t passes;
	passes = gs_technique_begin(context->effect_tech_draw);
	for (size_t i = 0; i < passes; i++) {
		if (gs_technique_begin_pass(context->effect_tech_draw, i)) {
			gs_draw_sprite(context->texture_target, 0, 0, 0);
			gs_technique_end_pass(context->effect_tech_draw);
		}
	}
	gs_technique_end(context->effect_tech_draw);
}

void showdraw_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);

	struct showdraw_filter_context *context = (struct showdraw_filter_context *)data;

	obs_source_t *filter = context->global_state.filter;
	obs_source_t *target = obs_filter_get_target(filter);

	if (!target) {
		obs_log(LOG_ERROR, "Target source not found");
		obs_source_skip_video_filter(filter);
		return;
	}

	if (!context->effect) {
		if (!init_effect(context))
			return;
	}

	const uint32_t width = obs_source_get_width(target);
	const uint32_t height = obs_source_get_height(target);

	if (width == 0 || height == 0) {
		obs_log(LOG_DEBUG, "Target source has zero width or height");
		obs_source_skip_video_filter(filter);
		return;
	}

	if (!ensure_textures(context, width, height))
		return;

	struct showdraw_preset *running_preset = context->global_state.running_preset;

	long long extractionMode = running_preset->extraction_mode == EXTRACTION_MODE_DEFAULT
					   ? EXTRACTION_MODE_DEFAULT_VALUE
					   : running_preset->extraction_mode;

	const float texel_width = 1.0f / (float)width;
	const float texel_height = 1.0f / (float)height;

	gs_texture_t *default_render_target = gs_get_render_target();

	if (!obs_source_process_filter_begin(context->global_state.filter, GS_BGRA, OBS_ALLOW_DIRECT_RENDERING)) {
		obs_log(LOG_ERROR, "Could not begin processing filter");
		obs_source_skip_video_filter(filter);
		return;
	}

	gs_set_render_target(context->texture_source, NULL);

	gs_viewport_push();
	gs_projection_push();
	gs_matrix_push();

	gs_set_viewport(0, 0, width, height);
	gs_matrix_identity();

	obs_source_process_filter_end(filter, context->effect, 0, 0);

	if (extractionMode >= EXTRACTION_MODE_LUMINANCE_EXTRACTION) {
		apply_luminance_extraction_pass(context);

		if (running_preset->median_filtering_kernel_size > 1) {
			apply_median_filtering_pass(context, texel_width, texel_height);
		}

		if (running_preset->motion_adaptive_filtering_strength > 0.0) {
			apply_motion_adaptive_filtering_pass(context, texel_width, texel_height);
		}
	}

	if (extractionMode >= EXTRACTION_MODE_EDGE_DETECTION) {
		apply_sobel_pass(context, texel_width, texel_height);

		apply_supress_non_maximum_pass(context, texel_width, texel_height);

		if (running_preset->morphology_opening_erosion_kernel_size > 1) {
			apply_morphology_pass(context, texel_width, texel_height, context->effect_tech_erosion,
					      (int)running_preset->morphology_opening_erosion_kernel_size);
		}

		if (running_preset->morphology_opening_dilation_kernel_size > 1) {
			apply_morphology_pass(context, texel_width, texel_height, context->effect_tech_dilation,
					      (int)running_preset->morphology_opening_dilation_kernel_size);
		}

		if (running_preset->morphology_closing_dilation_kernel_size > 1) {
			apply_morphology_pass(context, texel_width, texel_height, context->effect_tech_dilation,
					      (int)running_preset->morphology_closing_dilation_kernel_size);
		}

		if (running_preset->morphology_closing_erosion_kernel_size > 1) {
			apply_morphology_pass(context, texel_width, texel_height, context->effect_tech_erosion,
					      (int)running_preset->morphology_closing_erosion_kernel_size);
		}
	}

	if (extractionMode >= EXTRACTION_MODE_SCALING) {
		apply_scaling_pass(context);
	}

	gs_viewport_pop();
	gs_projection_pop();
	gs_matrix_pop();
	gs_set_render_target(default_render_target, NULL);

	draw_final_image(context);
}

struct obs_source_info showdraw_filter = {
	.id = "showdraw",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = showdraw_get_name,
	.create = showdraw_create,
	.destroy = showdraw_destroy,
	.get_defaults = showdraw_get_defaults,
	.get_properties = showdraw_get_properties,
	.update = showdraw_update,
	.video_render = showdraw_video_render,
};

bool obs_module_load(void)
{
	obs_register_source(&showdraw_filter);
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "plugin unloaded");
}

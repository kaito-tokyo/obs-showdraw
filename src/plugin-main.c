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
#include <plugin-support.h>

#include <math.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

const char *showdraw_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("showdrawName");
}

#define MAX_PREVIOUS_TEXTURES 15

#define EXTRACTION_MODE_DEFAULT 0
#define EXTRACTION_MODE_PASSTHROUGH 100
#define EXTRACTION_MODE_LUMINANCE_EXTRACTION 200
#define EXTRACTION_MODE_EDGE_DETECTION 300
#define EXTRACTION_MODE_SCALING 400

#define EXTRACTION_MODE_DEFAULT_VALUE EXTRACTION_MODE_EDGE_DETECTION

struct showdraw_filter_context {
	obs_source_t *filter;

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

	const char *effect_path;

	gs_texture_t *source_texture;
	gs_texture_t *target_texture;
	gs_texture_t *motion_map_texture;
	gs_texture_t *previous_luminance_texture;

	gs_effect_t *effect;

	gs_eparam_t *effect_image;
	gs_eparam_t *effect_image1;

	gs_eparam_t *effect_float_texel_width;
	gs_eparam_t *effect_float_texel_height;

	gs_eparam_t *effect_texture_motion_map;
	gs_eparam_t *effect_kernel_size;
	gs_eparam_t *effect_strength;
	gs_eparam_t *effect_motion_threshold;

	gs_eparam_t *effect_float_scaling_factor;

	gs_technique_t *effect_tech_draw;
	gs_technique_t *effect_tech_extract_luminance;
	gs_technique_t *effect_tech_median_filtering;
	gs_technique_t *effect_tech_calculate_motion_map;
	gs_technique_t *effect_tech_motion_adaptive_filtering;
	gs_technique_t *effect_tech_detect_edge;
	gs_technique_t *effect_tech_erosion;
	gs_technique_t *effect_tech_dilation;
	gs_technique_t *effect_tech_scaling;
};

void showdraw_update(void *data, obs_data_t *settings);

void *showdraw_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(settings);

	obs_log(LOG_INFO, "Creating showdraw source");

	struct showdraw_filter_context *context = bzalloc(sizeof(struct showdraw_filter_context));

	context->filter = source;

	context->extraction_mode = EXTRACTION_MODE_DEFAULT;

	context->median_filtering_kernel_size = 1;

	context->motion_map_kernel_size = 1;

	context->motion_adaptive_filtering_strength = 1.0;
	context->motion_adaptive_filtering_motion_threshold = 0.1;

	context->morphology_closing_erosion_kernel_size = 1;
	context->morphology_closing_dilation_kernel_size = 1;
	context->morphology_opening_dilation_kernel_size = 1;
	context->morphology_opening_erosion_kernel_size = 1;

	context->scaling_factor = 1.0;

	context->effect_path = obs_module_file("effects/drawing-emphasizer.effect");

	context->source_texture = NULL;
	context->target_texture = NULL;
	context->motion_map_texture = NULL;
	context->previous_luminance_texture = NULL;

	context->effect = NULL;

	context->effect_image = NULL;
	context->effect_image1 = NULL;

	context->effect_float_texel_width = NULL;
	context->effect_float_texel_height = NULL;

	context->effect_texture_motion_map = NULL;
	context->effect_kernel_size = NULL;
	context->effect_strength = NULL;
	context->effect_motion_threshold = NULL;

	context->effect_float_scaling_factor = NULL;

	context->effect_tech_draw = NULL;
	context->effect_tech_extract_luminance = NULL;
	context->effect_tech_median_filtering = NULL;
	context->effect_tech_calculate_motion_map = NULL;
	context->effect_tech_motion_adaptive_filtering = NULL;
	context->effect_tech_detect_edge = NULL;
	context->effect_tech_erosion = NULL;
	context->effect_tech_dilation = NULL;
	context->effect_tech_scaling = NULL;

	showdraw_update(context, settings);

	return context;
}

void showdraw_destroy(void *data)
{
	obs_log(LOG_INFO, "Destroying showdraw source");

	struct showdraw_filter_context *context = (struct showdraw_filter_context *)data;

	if (context->source_texture) {
		gs_texture_destroy(context->source_texture);
		context->source_texture = NULL;
	}

	if (context->target_texture) {
		gs_texture_destroy(context->target_texture);
		context->target_texture = NULL;
	}

	if (context->motion_map_texture) {
		gs_texture_destroy(context->motion_map_texture);
		context->motion_map_texture = NULL;
	}

	if (context->previous_luminance_texture) {
		gs_texture_destroy(context->previous_luminance_texture);
		context->previous_luminance_texture = NULL;
	}

	if (context->effect) {
		gs_effect_destroy(context->effect);
		context->effect = NULL;
	}

	bfree(context);
}

void showdraw_get_defaults(obs_data_t *data)
{
	obs_data_set_default_int(data, "extractionMode", EXTRACTION_MODE_DEFAULT);

	obs_data_set_default_int(data, "medianFilteringKernelSize", 1);

	obs_data_set_default_int(data, "motionMapKernelSize", 1);

	obs_data_set_default_double(data, "motionAdaptiveFilteringStrength", 0.0);
	obs_data_set_default_double(data, "motionAdaptiveFilteringMotionThreshold", 0.1);

	obs_data_set_default_int(data, "morphologyOpeningErosionKernelSize", 1);
	obs_data_set_default_int(data, "morphologyOpeningDilationKernelSize", 1);
	obs_data_set_default_int(data, "morphologyClosingDilationKernelSize", 1);
	obs_data_set_default_int(data, "morphologyClosingErosionKernelSize", 1);

	obs_data_set_default_double(data, "scalingFactorDb", 0.0);
	;
}

obs_properties_t *showdraw_get_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();

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

	context->extraction_mode = obs_data_get_int(settings, "extractionMode");

	context->median_filtering_kernel_size = obs_data_get_int(settings, "medianFilteringKernelSize");

	context->motion_map_kernel_size = obs_data_get_int(settings, "motionMapKernelSize");

	context->motion_adaptive_filtering_strength = obs_data_get_double(settings, "motionAdaptiveFilteringStrength");
	context->motion_adaptive_filtering_motion_threshold =
		obs_data_get_double(settings, "motionAdaptiveFilteringMotionThreshold");

	context->morphology_opening_erosion_kernel_size =
		obs_data_get_int(settings, "morphologyOpeningErosionKernelSize");
	context->morphology_opening_dilation_kernel_size =
		obs_data_get_int(settings, "morphologyOpeningDilationKernelSize");
	context->morphology_closing_dilation_kernel_size =
		obs_data_get_int(settings, "morphologyClosingDilationKernelSize");
	context->morphology_closing_erosion_kernel_size =
		obs_data_get_int(settings, "morphologyClosingErosionKernelSize");

	context->scaling_factor = pow(10.0, obs_data_get_double(settings, "scalingFactorDb") / 10.0);
}

void swap_textures(struct showdraw_filter_context *context)
{
	gs_texture_t *temp = context->source_texture;
	context->source_texture = context->target_texture;
	context->target_texture = temp;
}

void showdraw_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);

	struct showdraw_filter_context *context = (struct showdraw_filter_context *)data;

	obs_source_t *target = obs_filter_get_target(context->filter);

	if (!target) {
		obs_log(LOG_ERROR, "Target source not found");
		return;
	}

	if (!context->effect) {
		char *error_string = NULL;

		gs_effect_t *effect = gs_effect_create_from_file(context->effect_path, &error_string);
		if (!effect) {
			obs_log(LOG_ERROR, "Error loading effect: %s", error_string);
			bfree(error_string);
			return;
		}

		context->effect = effect;

		context->effect_image = gs_effect_get_param_by_name(context->effect, "image");
		context->effect_image1 = gs_effect_get_param_by_name(context->effect, "image1");

		context->effect_float_texel_width = gs_effect_get_param_by_name(context->effect, "texelWidth");
		context->effect_float_texel_height = gs_effect_get_param_by_name(context->effect, "texelHeight");

		context->effect_texture_motion_map = gs_effect_get_param_by_name(context->effect, "motionMap");
		context->effect_kernel_size = gs_effect_get_param_by_name(context->effect, "kernelSize");
		context->effect_strength = gs_effect_get_param_by_name(context->effect, "strength");
		context->effect_motion_threshold = gs_effect_get_param_by_name(context->effect, "motionThreshold");

		context->effect_float_scaling_factor = gs_effect_get_param_by_name(context->effect, "scalingFactor");

		context->effect_tech_draw = gs_effect_get_technique(context->effect, "Draw");
		context->effect_tech_extract_luminance = gs_effect_get_technique(context->effect, "ExtractLuminance");
		context->effect_tech_median_filtering = gs_effect_get_technique(context->effect, "MedianFiltering");
		context->effect_tech_calculate_motion_map =
			gs_effect_get_technique(context->effect, "CalculateMotionMap");
		context->effect_tech_motion_adaptive_filtering =
			gs_effect_get_technique(context->effect, "MotionAdaptiveFiltering");
		context->effect_tech_detect_edge = gs_effect_get_technique(context->effect, "DetectEdge");
		context->effect_tech_erosion = gs_effect_get_technique(context->effect, "Erosion");
		context->effect_tech_dilation = gs_effect_get_technique(context->effect, "Dilation");
		context->effect_tech_scaling = gs_effect_get_technique(context->effect, "Scaling");

		obs_log(LOG_INFO, "Effect loaded successfully");
	}

	const uint32_t width = obs_source_get_width(target);
	const uint32_t height = obs_source_get_height(target);

	if (width == 0 || height == 0) {
		obs_log(LOG_DEBUG, "Target source has zero width or height");
		obs_source_skip_video_filter(context->filter);
		return;
	}

	if (!context->source_texture || gs_texture_get_width(context->source_texture) != width ||
	    gs_texture_get_height(context->source_texture) != height) {
		if (context->source_texture) {
			gs_texture_destroy(context->source_texture);
		}
		context->source_texture = gs_texture_create(width, height, GS_BGRA, 1, NULL, GS_RENDER_TARGET);
		if (!context->source_texture) {
			obs_log(LOG_ERROR, "Could not create source texture");
			obs_source_skip_video_filter(context->filter);
			return;
		}
	}

	if (!context->target_texture || gs_texture_get_width(context->target_texture) != width ||
	    gs_texture_get_height(context->target_texture) != height) {
		if (context->target_texture) {
			gs_texture_destroy(context->target_texture);
		}
		context->target_texture = gs_texture_create(width, height, GS_BGRA, 1, NULL, GS_RENDER_TARGET);
		if (!context->target_texture) {
			obs_log(LOG_ERROR, "Could not create target texture");
			obs_source_skip_video_filter(context->filter);
			return;
		}
	}

	if (!context->motion_map_texture || gs_texture_get_width(context->motion_map_texture) != width ||
	    gs_texture_get_height(context->motion_map_texture) != height) {
		if (context->motion_map_texture) {
			gs_texture_destroy(context->motion_map_texture);
		}
		context->motion_map_texture = gs_texture_create(width, height, GS_BGRA, 1, NULL, GS_RENDER_TARGET);
		if (!context->motion_map_texture) {
			obs_log(LOG_ERROR, "Could not create motion map texture");
			obs_source_skip_video_filter(context->filter);
			return;
		}
	}

	if (!context->previous_luminance_texture ||
	    gs_texture_get_width(context->previous_luminance_texture) != width ||
	    gs_texture_get_height(context->previous_luminance_texture) != height) {
		if (context->previous_luminance_texture) {
			gs_texture_destroy(context->previous_luminance_texture);
		}
		context->previous_luminance_texture =
			gs_texture_create(width, height, GS_BGRA, 1, NULL, GS_RENDER_TARGET);
		if (!context->previous_luminance_texture) {
			obs_log(LOG_ERROR, "Could not create previous luminance texture");
			obs_source_skip_video_filter(context->filter);
			return;
		}
	}

	long long extractionMode = context->extraction_mode == EXTRACTION_MODE_DEFAULT ? EXTRACTION_MODE_DEFAULT_VALUE
										       : context->extraction_mode;

	const float texelWidth = 1.0f / (float)width;
	const float texelHeight = 1.0f / (float)height;

	gs_texture_t *default_render_target = gs_get_render_target();

	if (!obs_source_process_filter_begin(context->filter, GS_BGRA, OBS_ALLOW_DIRECT_RENDERING)) {
		obs_log(LOG_ERROR, "Could not begin processing filter");
		obs_source_skip_video_filter(context->filter);
		return;
	}

	gs_set_render_target(context->source_texture, NULL);

	gs_viewport_push();
	gs_projection_push();
	gs_matrix_push();

	gs_set_viewport(0, 0, width, height);
	gs_matrix_identity();

	obs_source_process_filter_end(context->filter, context->effect, 0, 0);

	size_t passes;

	if (extractionMode >= EXTRACTION_MODE_LUMINANCE_EXTRACTION) {
		gs_set_render_target(context->target_texture, NULL);

		gs_effect_set_texture(context->effect_image, context->source_texture);

		passes = gs_technique_begin(context->effect_tech_extract_luminance);
		for (size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(context->effect_tech_extract_luminance, i)) {
				gs_draw_sprite(context->source_texture, 0, 0, 0);
				gs_technique_end_pass(context->effect_tech_extract_luminance);
			}
		}
		gs_technique_end(context->effect_tech_extract_luminance);

		swap_textures(context);
	}

	if (extractionMode >= EXTRACTION_MODE_LUMINANCE_EXTRACTION && context->median_filtering_kernel_size > 1) {
		gs_set_render_target(context->target_texture, NULL);

		gs_effect_set_texture(context->effect_image, context->source_texture);

		gs_effect_set_float(context->effect_float_texel_width, texelWidth);
		gs_effect_set_float(context->effect_float_texel_height, texelHeight);
		gs_effect_set_int(context->effect_kernel_size, (int)context->median_filtering_kernel_size);

		passes = gs_technique_begin(context->effect_tech_median_filtering);
		for (size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(context->effect_tech_median_filtering, i)) {
				gs_draw_sprite(context->source_texture, 0, 0, 0);
				gs_technique_end_pass(context->effect_tech_median_filtering);
			}
		}
		gs_technique_end(context->effect_tech_median_filtering);

		swap_textures(context);
	}

	if (extractionMode >= EXTRACTION_MODE_LUMINANCE_EXTRACTION &&
	    context->motion_adaptive_filtering_strength > 0.0) {
		gs_set_render_target(context->motion_map_texture, NULL);

		gs_effect_set_texture(context->effect_image, context->source_texture);
		gs_effect_set_texture(context->effect_image1, context->previous_luminance_texture);

		gs_effect_set_float(context->effect_float_texel_width, texelWidth);
		gs_effect_set_float(context->effect_float_texel_height, texelHeight);
		gs_effect_set_int(context->effect_kernel_size, (int)context->motion_map_kernel_size);

		passes = gs_technique_begin(context->effect_tech_calculate_motion_map);
		for (size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(context->effect_tech_calculate_motion_map, i)) {
				gs_draw_sprite(context->source_texture, 0, 0, 0);
				gs_technique_end_pass(context->effect_tech_calculate_motion_map);
			}
		}
		gs_technique_end(context->effect_tech_calculate_motion_map);

		gs_set_render_target(context->target_texture, NULL);
		gs_effect_set_texture(context->effect_image, context->source_texture);
		gs_effect_set_texture(context->effect_image1, context->previous_luminance_texture);

		gs_effect_set_float(context->effect_float_texel_width, texelWidth);
		gs_effect_set_float(context->effect_float_texel_height, texelHeight);

		gs_effect_set_texture(context->effect_texture_motion_map, context->motion_map_texture);
		gs_effect_set_float(context->effect_strength, (float)context->motion_adaptive_filtering_strength);
		gs_effect_set_float(context->effect_motion_threshold,
				    (float)context->motion_adaptive_filtering_motion_threshold);

		passes = gs_technique_begin(context->effect_tech_motion_adaptive_filtering);
		for (size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(context->effect_tech_motion_adaptive_filtering, i)) {
				gs_draw_sprite(context->source_texture, 0, 0, 0);
				gs_technique_end_pass(context->effect_tech_motion_adaptive_filtering);
			}
		}
		gs_technique_end(context->effect_tech_motion_adaptive_filtering);

		swap_textures(context);

		gs_copy_texture(context->target_texture, context->previous_luminance_texture);
	}

	if (extractionMode >= EXTRACTION_MODE_EDGE_DETECTION) {
		gs_set_render_target(context->target_texture, NULL);

		gs_effect_set_texture(context->effect_image, context->source_texture);

		gs_effect_set_float(context->effect_float_texel_width, texelWidth);
		gs_effect_set_float(context->effect_float_texel_height, texelHeight);

		passes = gs_technique_begin(context->effect_tech_detect_edge);
		for (size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(context->effect_tech_detect_edge, i)) {
				gs_draw_sprite(context->source_texture, 0, 0, 0);
				gs_technique_end_pass(context->effect_tech_detect_edge);
			}
		}
		gs_technique_end(context->effect_tech_detect_edge);

		swap_textures(context);
	}

	if (extractionMode >= EXTRACTION_MODE_EDGE_DETECTION &&
	    (context->morphology_opening_erosion_kernel_size > 1 ||
	     context->morphology_opening_dilation_kernel_size > 1)) {
		gs_set_render_target(context->target_texture, NULL);

		gs_effect_set_texture(context->effect_image, context->source_texture);

		gs_effect_set_float(context->effect_float_texel_width, texelWidth);
		gs_effect_set_float(context->effect_float_texel_height, texelHeight);
		gs_effect_set_int(context->effect_kernel_size, (int)context->morphology_opening_erosion_kernel_size);

		passes = gs_technique_begin(context->effect_tech_erosion);
		for (size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(context->effect_tech_erosion, i)) {
				gs_draw_sprite(context->source_texture, 0, 0, 0);
				gs_technique_end_pass(context->effect_tech_erosion);
			}
		}
		gs_technique_end(context->effect_tech_erosion);

		swap_textures(context);

		gs_set_render_target(context->target_texture, NULL);

		gs_effect_set_texture(context->effect_image, context->source_texture);

		gs_effect_set_float(context->effect_float_texel_width, texelWidth);
		gs_effect_set_float(context->effect_float_texel_height, texelHeight);
		gs_effect_set_int(context->effect_kernel_size, (int)context->morphology_opening_dilation_kernel_size);

		passes = gs_technique_begin(context->effect_tech_dilation);
		for (size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(context->effect_tech_dilation, i)) {
				gs_draw_sprite(context->source_texture, 0, 0, 0);
				gs_technique_end_pass(context->effect_tech_dilation);
			}
		}
		gs_technique_end(context->effect_tech_dilation);

		swap_textures(context);
	}

	if (extractionMode >= EXTRACTION_MODE_EDGE_DETECTION && (context->morphology_closing_dilation_kernel_size > 1 ||
								 context->morphology_closing_erosion_kernel_size > 1)) {
		gs_set_render_target(context->target_texture, NULL);

		gs_effect_set_texture(context->effect_image, context->source_texture);

		gs_effect_set_float(context->effect_float_texel_width, texelWidth);
		gs_effect_set_float(context->effect_float_texel_height, texelHeight);
		gs_effect_set_int(context->effect_kernel_size, (int)context->morphology_closing_dilation_kernel_size);

		passes = gs_technique_begin(context->effect_tech_dilation);
		for (size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(context->effect_tech_dilation, i)) {
				gs_draw_sprite(context->source_texture, 0, 0, 0);
				gs_technique_end_pass(context->effect_tech_dilation);
			}
		}
		gs_technique_end(context->effect_tech_dilation);

		swap_textures(context);

		gs_set_render_target(context->target_texture, NULL);

		gs_effect_set_texture(context->effect_image, context->source_texture);

		gs_effect_set_float(context->effect_float_texel_width, texelWidth);
		gs_effect_set_float(context->effect_float_texel_height, texelHeight);
		gs_effect_set_int(context->effect_kernel_size, (int)context->morphology_closing_erosion_kernel_size);

		passes = gs_technique_begin(context->effect_tech_erosion);
		for (size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(context->effect_tech_erosion, i)) {
				gs_draw_sprite(context->source_texture, 0, 0, 0);
				gs_technique_end_pass(context->effect_tech_erosion);
			}
		}
		gs_technique_end(context->effect_tech_erosion);

		swap_textures(context);
	}

	if (extractionMode >= EXTRACTION_MODE_SCALING) {
		gs_set_render_target(context->target_texture, NULL);

		gs_effect_set_texture(context->effect_image, context->source_texture);

		gs_effect_set_float(context->effect_float_scaling_factor, (float)context->scaling_factor);

		passes = gs_technique_begin(context->effect_tech_scaling);
		for (size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(context->effect_tech_scaling, i)) {
				gs_draw_sprite(context->source_texture, 0, 0, 0);
				gs_technique_end_pass(context->effect_tech_scaling);
			}
		}
		gs_technique_end(context->effect_tech_scaling);

		swap_textures(context);
	}

	gs_viewport_pop();
	gs_projection_pop();
	gs_matrix_pop();
	gs_set_render_target(default_render_target, NULL);

	gs_effect_set_texture(context->effect_image, context->source_texture);

	passes = gs_technique_begin(context->effect_tech_draw);
	for (size_t i = 0; i < passes; i++) {
		if (gs_technique_begin_pass(context->effect_tech_draw, i)) {
			gs_draw_sprite(context->target_texture, 0, 0, 0);
			gs_technique_end_pass(context->effect_tech_draw);
		}
	}
	gs_technique_end(context->effect_tech_draw);
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

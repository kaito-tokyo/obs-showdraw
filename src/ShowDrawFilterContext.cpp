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

#include <new>
#include <stdexcept>

#include <obs-frontend-api.h>

#include "ShowDrawFilterContext.hpp"
#include "showdraw-preset.hpp"
#include "PresetWindow.hpp"

const char *showdraw_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("showdrawName");
}

void *showdraw_create(obs_data_t *settings, obs_source_t *source)
{
	void *data = bzalloc(sizeof(ShowDrawFilterContext));

	if (!data) {
		obs_log(LOG_ERROR, "Failed to allocate memory for showdraw context");
		return nullptr;
	}

	try {
		ShowDrawFilterContext *context = new (data) ShowDrawFilterContext(settings, source);
		return context;
	} catch (const std::exception &e) {
		obs_log(LOG_ERROR, "Failed to create showdraw context: %s", e.what());
		return nullptr;
	}
}

void showdraw_destroy(void *data)
{
	if (!data) {
		obs_log(LOG_WARNING, "showdraw_destroy called with null data");
		return;
	}

	ShowDrawFilterContext *context = static_cast<ShowDrawFilterContext *>(data);
	context->~ShowDrawFilterContext();
	bfree(context);
}

void showdraw_get_defaults(obs_data_t *data)
{
	ShowDrawFilterContext::getDefaults(data);
}

obs_properties_t *showdraw_get_properties(void *data)
{
	if (!data) {
		obs_log(LOG_WARNING, "showdraw_get_properties called with null data");
		return nullptr;
	}

	ShowDrawFilterContext *context = static_cast<ShowDrawFilterContext *>(data);
	return context->getProperties();
}

void showdraw_update(void *data, obs_data_t *settings)
{
	if (!data) {
		obs_log(LOG_WARNING, "showdraw_update called with null data");
		return;
	}

	ShowDrawFilterContext *context = static_cast<ShowDrawFilterContext *>(data);
	context->update(settings);
}

void showdraw_video_render(void *data, gs_effect_t *effect)
{
	if (!data) {
		obs_log(LOG_WARNING, "showdraw_video_render called with null data");
		return;
	}

	ShowDrawFilterContext *context = static_cast<ShowDrawFilterContext *>(data);
	context->videoRender(effect);
}

ShowDrawFilterContext::ShowDrawFilterContext(obs_data_t *settings, obs_source_t *source)
	: settings(settings),
	  source(source)
{
	obs_log(LOG_INFO, "Creating showdraw filter context");

	global_state.filter = source;
	global_state.running_preset = showdraw_preset_create(" working", true);

	if (global_state.running_preset == NULL) {
		obs_log(LOG_ERROR, "Failed to create showdraw preset");
		throw std::bad_alloc();
	}

	update(settings);
}

ShowDrawFilterContext::~ShowDrawFilterContext(void) noexcept
{
	obs_log(LOG_INFO, "Destroying showdraw filter context");

	showdraw_preset_destroy(global_state.running_preset);

	obs_enter_graphics();
	gs_texture_destroy(texture_source);
	gs_texture_destroy(texture_target);
	gs_texture_destroy(texture_motion_map);
	gs_texture_destroy(texture_previous_luminance);
	gs_effect_destroy(effect);
	obs_leave_graphics();
}

void ShowDrawFilterContext::getDefaults(obs_data_t *data) noexcept
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

obs_properties_t *ShowDrawFilterContext::getProperties(void) noexcept
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_button2(
		props, "openPresetWindow", obs_module_text("openPresetWindow"),
		[](obs_properties_t *props, obs_property_t *property, void *data) {
			UNUSED_PARAMETER(props);
			UNUSED_PARAMETER(property);
			ShowDrawFilterContext *context = static_cast<ShowDrawFilterContext *>(data);
			PresetWindow *window = new PresetWindow(&context->global_state,
								static_cast<QWidget *>(obs_frontend_get_main_window()));
			window->exec();
			return false;
		},
		this);

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

void ShowDrawFilterContext::update(obs_data_t *settings) noexcept
{
	struct showdraw_preset *running_preset = global_state.running_preset;

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
	scaling_factor = pow(10.0, running_preset->scaling_factor_db / 10.0);
}

gs_eparam_t *getEffectParam(gs_effect_t *effect, const char *name)
{
	gs_eparam_t *param = gs_effect_get_param_by_name(effect, name);

	if (!param) {
		obs_log(LOG_ERROR, "Effect parameter %s not found", name);
		throw std::runtime_error("Effect parameter not found");
	}

	return param;
}

gs_technique_t *getEffectTech(gs_effect_t *effect, const char *name)
{
	gs_technique_t *tech = gs_effect_get_technique(effect, name);

	if (!tech) {
		obs_log(LOG_ERROR, "Effect technique %s not found", name);
		throw std::runtime_error("Effect technique not found");
	}

	return tech;
}

bool ShowDrawFilterContext::initEffect(void) noexcept
{
	char *error_string = nullptr;

	char *effect_path = obs_module_file("effects/drawing-emphasizer.effect");
	if (!effect_path) {
		obs_log(LOG_ERROR, "Failed to get effect path");
		return false;
	}

	effect = gs_effect_create_from_file(effect_path, &error_string);
	bfree(effect_path);
	if (!effect) {
		obs_log(LOG_ERROR, "Error loading effect: %s", error_string);
		bfree(error_string);
		return false;
	}

	try {
		effect_texture_image = getEffectParam(effect, "image");
		effect_texture_image1 = getEffectParam(effect, "image1");

		effect_float_texel_width = getEffectParam(effect, "texelWidth");
		effect_float_texel_height = getEffectParam(effect, "texelHeight");
		effect_int_kernel_size = getEffectParam(effect, "kernelSize");

		effect_texture_motion_map = getEffectParam(effect, "motionMap");
		effect_float_strength = getEffectParam(effect, "strength");
		effect_float_motion_threshold = getEffectParam(effect, "motionThreshold");

		effect_float_scaling_factor = getEffectParam(effect, "scalingFactor");

		effect_tech_draw = getEffectTech(effect, "Draw");
		effect_tech_extract_luminance = getEffectTech(effect, "ExtractLuminance");
		effect_tech_median_filtering = getEffectTech(effect, "MedianFiltering");
		effect_tech_calculate_motion_map = getEffectTech(effect, "CalculateMotionMap");
		effect_tech_motion_adaptive_filtering = getEffectTech(effect, "MotionAdaptiveFiltering");
		effect_tech_apply_sobel = getEffectTech(effect, "ApplySobel");
		effect_tech_suppress_non_maximum = getEffectTech(effect, "SuppressNonMaximum");
		effect_tech_detect_edge = getEffectTech(effect, "DetectEdge");
		effect_tech_erosion = getEffectTech(effect, "Erosion");
		effect_tech_dilation = getEffectTech(effect, "Dilation");
		effect_tech_scaling = getEffectTech(effect, "Scaling");
	} catch (const std::exception &e) {
		gs_effect_destroy(effect);
		effect = nullptr;
		return false;
	}

	return true;
}

void ensureTexture(gs_texture_t *&texture, uint32_t width, uint32_t height)
{
	if (!texture || gs_texture_get_width(texture) != width || gs_texture_get_height(texture) != height) {
		if (texture) {
			gs_texture_destroy(texture);
		}
		texture = gs_texture_create(width, height, GS_BGRA, 1, NULL, GS_RENDER_TARGET);
		if (!texture) {
			throw std::bad_alloc();
		}
	}
}

bool ShowDrawFilterContext::ensureTextures(uint32_t width, uint32_t height) noexcept
{
	try {
		ensureTexture(texture_source, width, height);
	} catch (const std::bad_alloc &) {
		obs_log(LOG_ERROR, "Failed to create source texture");
		return false;
	}

	try {
		ensureTexture(texture_target, width, height);
	} catch (const std::bad_alloc &) {
		obs_log(LOG_ERROR, "Failed to create target texture");
		return false;
	}

	try {
		ensureTexture(texture_motion_map, width, height);
	} catch (const std::bad_alloc &) {
		obs_log(LOG_ERROR, "Failed to create motion map texture");
		return false;
	}

	try {
		ensureTexture(texture_previous_luminance, width, height);
	} catch (const std::bad_alloc &) {
		obs_log(LOG_ERROR, "Failed to create previous luminance texture");
		return false;
	}

	return true;
}

static void applyEffectPass(gs_technique_t *technique, gs_texture_t *texture) noexcept
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

void ShowDrawFilterContext::applyLuminanceExtractionPass(void) noexcept
{
	gs_set_render_target(texture_target, nullptr);

	gs_effect_set_texture(effect_texture_image, texture_source);

	applyEffectPass(effect_tech_extract_luminance, texture_source);

	std::swap(texture_source, texture_target);
}

void ShowDrawFilterContext::applyMedianFilteringPass(const float texelWidth, const float texelHeight) noexcept
{
	gs_set_render_target(texture_target, nullptr);

	gs_effect_set_texture(effect_texture_image, texture_source);

	gs_effect_set_float(effect_float_texel_width, texelWidth);
	gs_effect_set_float(effect_float_texel_height, texelHeight);
	gs_effect_set_int(effect_int_kernel_size, (int)global_state.running_preset->median_filtering_kernel_size);

	applyEffectPass(effect_tech_median_filtering, texture_source);

	std::swap(texture_source, texture_target);
}

void ShowDrawFilterContext::applyMotionAdaptiveFilteringPass(const float texelWidth, const float texelHeight) noexcept
{
	gs_set_render_target(texture_motion_map, nullptr);

	gs_effect_set_texture(effect_texture_image, texture_source);
	gs_effect_set_texture(effect_texture_image1, texture_previous_luminance);

	gs_effect_set_float(effect_float_texel_width, texelWidth);
	gs_effect_set_float(effect_float_texel_height, texelHeight);
	gs_effect_set_int(effect_int_kernel_size, (int)global_state.running_preset->motion_map_kernel_size);

	applyEffectPass(effect_tech_calculate_motion_map, texture_source);

	gs_set_render_target(texture_target, nullptr);
	gs_effect_set_texture(effect_texture_image, texture_source);
	gs_effect_set_texture(effect_texture_image1, texture_previous_luminance);

	gs_effect_set_float(effect_float_texel_width, texelWidth);
	gs_effect_set_float(effect_float_texel_height, texelHeight);

	gs_effect_set_texture(effect_texture_motion_map, texture_motion_map);
	gs_effect_set_float(effect_float_strength,
			    (float)global_state.running_preset->motion_adaptive_filtering_strength);
	gs_effect_set_float(effect_float_motion_threshold,
			    (float)global_state.running_preset->motion_adaptive_filtering_motion_threshold);

	applyEffectPass(effect_tech_motion_adaptive_filtering, texture_source);

	gs_copy_texture(texture_previous_luminance, texture_target);

	std::swap(texture_source, texture_target);
}

void ShowDrawFilterContext::applySobelPass(const float texelWidth, const float texelHeight) noexcept
{
	gs_set_render_target(texture_target, nullptr);

	gs_effect_set_texture(effect_texture_image, texture_source);

	gs_effect_set_float(effect_float_texel_width, texelWidth);
	gs_effect_set_float(effect_float_texel_height, texelHeight);

	applyEffectPass(effect_tech_apply_sobel, texture_source);

	std::swap(texture_source, texture_target);
}

void ShowDrawFilterContext::applySuppressNonMaximumPass(const float texelWidth, const float texelHeight) noexcept
{
	gs_set_render_target(texture_target, nullptr);

	gs_effect_set_texture(effect_texture_image, texture_source);

	gs_effect_set_float(effect_float_texel_width, texelWidth);
	gs_effect_set_float(effect_float_texel_height, texelHeight);

	applyEffectPass(effect_tech_suppress_non_maximum, texture_source);

	std::swap(texture_source, texture_target);
}

void ShowDrawFilterContext::applyEdgeDetectionPass(const float texelWidth, const float texelHeight) noexcept
{
	gs_set_render_target(texture_target, nullptr);

	gs_effect_set_texture(effect_texture_image, texture_source);

	gs_effect_set_float(effect_float_texel_width, texelWidth);
	gs_effect_set_float(effect_float_texel_height, texelHeight);

	applyEffectPass(effect_tech_detect_edge, texture_source);

	std::swap(texture_source, texture_target);
}

void ShowDrawFilterContext::applyMorphologyPass(const float texelWidth, const float texelHeight,
						gs_technique_t *technique, int kernelSize) noexcept
{
	gs_set_render_target(texture_target, nullptr);

	gs_effect_set_texture(effect_texture_image, texture_source);

	gs_effect_set_float(effect_float_texel_width, texelWidth);
	gs_effect_set_float(effect_float_texel_height, texelHeight);
	gs_effect_set_int(effect_int_kernel_size, kernelSize);

	applyEffectPass(technique, texture_source);

	std::swap(texture_source, texture_target);
}

void ShowDrawFilterContext::applyScalingPass(void) noexcept
{
	gs_set_render_target(texture_target, nullptr);

	gs_effect_set_texture(effect_texture_image, texture_source);

	gs_effect_set_float(effect_float_scaling_factor, (float)scaling_factor);

	applyEffectPass(effect_tech_scaling, texture_source);

	std::swap(texture_source, texture_target);
}

void ShowDrawFilterContext::drawFinalImage(void) noexcept
{
	gs_effect_set_texture(effect_texture_image, texture_source);

	size_t passes;
	passes = gs_technique_begin(effect_tech_draw);
	for (size_t i = 0; i < passes; i++) {
		if (gs_technique_begin_pass(effect_tech_draw, i)) {
			gs_draw_sprite(texture_target, 0, 0, 0);
			gs_technique_end_pass(effect_tech_draw);
		}
	}
	gs_technique_end(effect_tech_draw);
}

void ShowDrawFilterContext::videoRender(gs_effect_t *effect) noexcept
{
	UNUSED_PARAMETER(effect);

	obs_source_t *filter = global_state.filter;

	if (!filter) {
		obs_log(LOG_ERROR, "Filter source not found");
		return;
	}

	obs_source_t *target = obs_filter_get_target(filter);

	if (!target) {
		obs_log(LOG_ERROR, "Target source not found");
		obs_source_skip_video_filter(filter);
		return;
	}

	if (!effect) {
		if (!initEffect()) {
			obs_log(LOG_ERROR, "Failed to initialize effect");
			obs_source_skip_video_filter(filter);
			return;
		}
	}

	const uint32_t width = obs_source_get_width(target);
	const uint32_t height = obs_source_get_height(target);

	if (width == 0 || height == 0) {
		obs_log(LOG_DEBUG, "Target source has zero width or height");
		obs_source_skip_video_filter(filter);
		return;
	}

	if (!ensureTextures(width, height)) {
		obs_log(LOG_ERROR, "Failed to ensure textures");
		obs_source_skip_video_filter(filter);
		return;
	}

	struct showdraw_preset *running_preset = global_state.running_preset;

	long long extractionMode = running_preset->extraction_mode == EXTRACTION_MODE_DEFAULT
					   ? EXTRACTION_MODE_SCALING
					   : running_preset->extraction_mode;

	const float texelWidth = 1.0f / (float)width;
	const float texelHeight = 1.0f / (float)height;

	gs_texture_t *default_render_target = gs_get_render_target();

	if (!obs_source_process_filter_begin(filter, GS_BGRA, OBS_ALLOW_DIRECT_RENDERING)) {
		obs_log(LOG_ERROR, "Could not begin processing filter");
		obs_source_skip_video_filter(filter);
		return;
	}

	gs_set_render_target(texture_source, NULL);

	gs_viewport_push();
	gs_projection_push();
	gs_matrix_push();

	gs_set_viewport(0, 0, width, height);
	gs_matrix_identity();

	obs_source_process_filter_end(filter, effect, 0, 0);

	if (extractionMode >= EXTRACTION_MODE_LUMINANCE_EXTRACTION) {
		applyLuminanceExtractionPass();

		if (running_preset->median_filtering_kernel_size > 1) {
			applyMedianFilteringPass(texelWidth, texelHeight);
		}

		if (running_preset->motion_adaptive_filtering_strength > 0.0) {
			applyMotionAdaptiveFilteringPass(texelWidth, texelHeight);
		}
	}

	if (extractionMode >= EXTRACTION_MODE_EDGE_DETECTION) {
		applySobelPass(texelWidth, texelHeight);

		applySuppressNonMaximumPass(texelWidth, texelHeight);

		if (running_preset->morphology_opening_erosion_kernel_size > 1) {
			applyMorphologyPass(texelWidth, texelHeight, effect_tech_erosion,
					    (int)running_preset->morphology_opening_erosion_kernel_size);
		}

		if (running_preset->morphology_opening_dilation_kernel_size > 1) {
			applyMorphologyPass(texelWidth, texelHeight, effect_tech_dilation,
					    (int)running_preset->morphology_opening_dilation_kernel_size);
		}

		if (running_preset->morphology_closing_dilation_kernel_size > 1) {
			applyMorphologyPass(texelWidth, texelHeight, effect_tech_dilation,
					    (int)running_preset->morphology_closing_dilation_kernel_size);
		}

		if (running_preset->morphology_closing_erosion_kernel_size > 1) {
			applyMorphologyPass(texelWidth, texelHeight, effect_tech_erosion,
					    (int)running_preset->morphology_closing_erosion_kernel_size);
		}
	}

	if (extractionMode >= EXTRACTION_MODE_SCALING) {
		applyScalingPass();
	}

	gs_viewport_pop();
	gs_projection_pop();
	gs_matrix_pop();
	gs_set_render_target(default_render_target, NULL);

	drawFinalImage();
}

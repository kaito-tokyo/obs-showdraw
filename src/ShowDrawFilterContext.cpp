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

#include "ShowDrawFilterContext.h"

#include <stdexcept>

#include "plugin-support.h"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include "obs-bridge-utils/obs-bridge-utils.hpp"

#include "DrawingEffect.hpp"
#include "Preset.hpp"
#include "PresetWindow.hpp"

using kaito_tokyo::obs_bridge_utils::slog;

using kaito_tokyo::obs_showdraw::DrawingEffect;
using kaito_tokyo::obs_showdraw::ExtractionMode;
using kaito_tokyo::obs_showdraw::Preset;
using kaito_tokyo::obs_showdraw::PresetWindow;

const char *showdraw_get_name(void *type_data)
{
	using kaito_tokyo::obs_showdraw::ShowDrawFilterContext;

	UNUSED_PARAMETER(type_data);
	return ShowDrawFilterContext::getName();
}

void *showdraw_create(obs_data_t *settings, obs_source_t *source)
{
	using kaito_tokyo::obs_showdraw::ShowDrawFilterContext;

	try {
		auto self = std::make_shared<ShowDrawFilterContext>(settings, source);
		self->afterCreate();
		self->update(settings);
		return new std::shared_ptr<ShowDrawFilterContext>(self);
	} catch (const std::exception &e) {
		slog(LOG_ERROR) << "Failed to create showdraw context: " << e.what();
		return nullptr;
	}
}

void showdraw_destroy(void *data)
{
	using kaito_tokyo::obs_showdraw::ShowDrawFilterContext;

	if (!data) {
		slog(LOG_ERROR) << "showdraw_destroy called with null data";
		return;
	}

	auto self = static_cast<std::shared_ptr<ShowDrawFilterContext> *>(data);
	delete self;
}

void showdraw_get_defaults(obs_data_t *data)
{
	using kaito_tokyo::obs_showdraw::ShowDrawFilterContext;

	ShowDrawFilterContext::getDefaults(data);
}

obs_properties_t *showdraw_get_properties(void *data)
{
	using kaito_tokyo::obs_showdraw::ShowDrawFilterContext;

	if (!data) {
		slog(LOG_ERROR) << "showdraw_get_properties called with null data";
		return nullptr;
	}

	auto self = static_cast<std::shared_ptr<ShowDrawFilterContext> *>(data);
	return self->get()->getProperties();
}

void showdraw_update(void *data, obs_data_t *settings)
{
	using kaito_tokyo::obs_showdraw::ShowDrawFilterContext;

	if (!data) {
		slog(LOG_ERROR) << "showdraw_update called with null data";
		return;
	}

	auto self = static_cast<std::shared_ptr<ShowDrawFilterContext> *>(data);
	self->get()->update(settings);
}

void showdraw_video_render(void *data, gs_effect_t *effect)
{
	using kaito_tokyo::obs_showdraw::ShowDrawFilterContext;

	UNUSED_PARAMETER(effect);

	if (!data) {
		slog(LOG_ERROR) << "showdraw_video_render called with null data";
		return;
	}

	auto self = static_cast<std::shared_ptr<ShowDrawFilterContext> *>(data);
	self->get()->videoRender();
}

namespace kaito_tokyo {
namespace obs_showdraw {

const char *ShowDrawFilterContext::getName() noexcept
{
	return obs_module_text("pluginName");
}

ShowDrawFilterContext::ShowDrawFilterContext(obs_data_t *settings, obs_source_t *source) noexcept
	: settings(settings),
	  filter(source),
	  drawingEffect(nullptr)
{
	runningPreset.presetName = " running";
}

ShowDrawFilterContext::~ShowDrawFilterContext() noexcept
{
	slog(LOG_INFO) << "Destroying showdraw filter context";

	obs_enter_graphics();
	drawingEffect.release();
	gs_texture_destroy(texture_source);
	gs_texture_destroy(texture_target);
	gs_texture_destroy(texture_motion_map);
	gs_texture_destroy(texture_previous_luminance);
	obs_leave_graphics();
}

void ShowDrawFilterContext::afterCreate() noexcept
{
	slog(LOG_INFO) << "Creating showdraw filter context";

	future_update_check = std::async(std::launch::async, [this]() {
		UpdateChecker checker;
		latest_version = checker.fetch();
	});
}

void ShowDrawFilterContext::getDefaults(obs_data_t *data) noexcept
{
	struct Preset defaultPreset = Preset::getStrongDefault();

	obs_data_set_default_int(data, "extractionMode", (long long)defaultPreset.extractionMode);

	obs_data_set_default_int(data, "medianFilteringKernelSize", (long long)defaultPreset.medianFilteringKernelSize);

	obs_data_set_default_int(data, "motionMapKernelSize", (long long)defaultPreset.motionMapKernelSize);

	obs_data_set_default_double(data, "motionAdaptiveFilteringStrength",
				    defaultPreset.motionAdaptiveFilteringStrength);
	obs_data_set_default_double(data, "motionAdaptiveFilteringMotionThreshold",
				    defaultPreset.motionAdaptiveFilteringMotionThreshold);

	obs_data_set_default_double(data, "hysteresisHighThreshold", defaultPreset.hysteresisHighThreshold);
	obs_data_set_default_double(data, "hysteresisLowThreshold", defaultPreset.hysteresisLowThreshold);

	obs_data_set_default_int(data, "morphologyOpeningErosionKernelSize",
				 defaultPreset.morphologyOpeningErosionKernelSize);
	obs_data_set_default_int(data, "morphologyOpeningDilationKernelSize",
				 defaultPreset.morphologyOpeningDilationKernelSize);
	obs_data_set_default_int(data, "morphologyClosingDilationKernelSize",
				 defaultPreset.morphologyClosingDilationKernelSize);
	obs_data_set_default_int(data, "morphologyClosingErosionKernelSize",
				 defaultPreset.morphologyClosingErosionKernelSize);

	obs_data_set_default_double(data, "scalingFactorDb", defaultPreset.scalingFactorDb);
}

obs_properties_t *ShowDrawFilterContext::getProperties() noexcept
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_button2(
		props, "openPresetWindow", obs_module_text("openPresetWindow"),
		[](obs_properties_t *props, obs_property_t *property, void *data) {
			UNUSED_PARAMETER(props);
			UNUSED_PARAMETER(property);
			auto this_ = static_cast<ShowDrawFilterContext *>(data);
			PresetWindow *window = new PresetWindow(this_->shared_from_this(),
								static_cast<QWidget *>(obs_frontend_get_main_window()));
			window->exec();
			return true;
		},
		this);

	obs_property_t *propExtractionMode = obs_properties_add_list(
		props, "extractionMode", obs_module_text("extractionMode"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModeDefault"),
				  static_cast<long long>(ExtractionMode::Default));
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModePassthrough"),
				  static_cast<long long>(ExtractionMode::Passthrough));
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModeLuminanceExtraction"),
				  static_cast<long long>(ExtractionMode::LuminanceExtraction));
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModeEdgeDetection"),
				  static_cast<long long>(ExtractionMode::EdgeDetection));
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModeScaling"),
				  static_cast<long long>(ExtractionMode::Scaling));

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

	obs_properties_add_float_slider(props, "hysteresisHighThreshold", obs_module_text("hysteresisHighThreshold"),
					0.0, std::sqrt(20.0), 0.01);
	obs_properties_add_float_slider(props, "hysteresisLowThreshold", obs_module_text("hysteresisLowThreshold"), 0.0,
					std::sqrt(20.0), 0.01);

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
	runningPreset.extractionMode = static_cast<ExtractionMode>(obs_data_get_int(settings, "extractionMode"));

	runningPreset.medianFilteringKernelSize = obs_data_get_int(settings, "medianFilteringKernelSize");

	runningPreset.motionMapKernelSize = obs_data_get_int(settings, "motionMapKernelSize");

	runningPreset.motionAdaptiveFilteringStrength =
		obs_data_get_double(settings, "motionAdaptiveFilteringStrength");
	runningPreset.motionAdaptiveFilteringMotionThreshold =
		obs_data_get_double(settings, "motionAdaptiveFilteringMotionThreshold");

	runningPreset.hysteresisHighThreshold = obs_data_get_double(settings, "hysteresisHighThreshold");
	runningPreset.hysteresisLowThreshold = obs_data_get_double(settings, "hysteresisLowThreshold");

	runningPreset.morphologyOpeningErosionKernelSize =
		obs_data_get_int(settings, "morphologyOpeningErosionKernelSize");
	runningPreset.morphologyOpeningDilationKernelSize =
		obs_data_get_int(settings, "morphologyOpeningDilationKernelSize");
	runningPreset.morphologyClosingDilationKernelSize =
		obs_data_get_int(settings, "morphologyClosingDilationKernelSize");
	runningPreset.morphologyClosingErosionKernelSize =
		obs_data_get_int(settings, "morphologyClosingErosionKernelSize");

	runningPreset.scalingFactorDb = obs_data_get_double(settings, "scalingFactorDb");
	scaling_factor = pow(10.0, runningPreset.scalingFactorDb / 10.0);
}

void ShowDrawFilterContext::videoRender() noexcept
{
	if (!filter) {
		slog(LOG_ERROR) << "Filter source not found";
		return;
	}

	obs_source_t *target = obs_filter_get_target(filter);

	if (!target) {
		slog(LOG_ERROR) << "Target source not found";
		obs_source_skip_video_filter(filter);
		return;
	}

	if (!drawingEffect) {
		try {
			drawingEffect = std::make_unique<DrawingEffect>();
		} catch (const std::exception &e) {
			slog(LOG_ERROR) << "Failed to create drawing effect: " << e.what();
			obs_source_skip_video_filter(filter);
			return;
		}
	}

	const uint32_t width = obs_source_get_width(target);
	const uint32_t height = obs_source_get_height(target);

	if (width == 0 || height == 0) {
		slog(LOG_DEBUG) << "Target source has zero width or height";
		obs_source_skip_video_filter(filter);
		return;
	}

	if (!ensureTextures(width, height)) {
		slog(LOG_ERROR) << "Failed to ensure textures";
		obs_source_skip_video_filter(filter);
		return;
	}

	ExtractionMode extractionMode = runningPreset.extractionMode == ExtractionMode::Default
						? ExtractionMode::Scaling
						: runningPreset.extractionMode;

	const float texelWidth = 1.0f / (float)width;
	const float texelHeight = 1.0f / (float)height;

	gs_texture_t *default_render_target = gs_get_render_target();

	if (!obs_source_process_filter_begin(filter, GS_BGRA, OBS_ALLOW_DIRECT_RENDERING)) {
		slog(LOG_ERROR) << "Could not begin processing filter";
		obs_source_skip_video_filter(filter);
		return;
	}

	gs_set_render_target(texture_target, NULL);

	gs_viewport_push();
	gs_projection_push();
	gs_matrix_push();

	gs_set_viewport(0, 0, width, height);
	gs_matrix_identity();

	obs_source_process_filter_end(filter, drawingEffect->effect, 0, 0);

	std::swap(texture_source, texture_target);

	if (extractionMode >= ExtractionMode::LuminanceExtraction) {
		applyLuminanceExtractionPass();

		if (runningPreset.medianFilteringKernelSize > 1) {
			applyMedianFilteringPass(texelWidth, texelHeight);
		}

		if (runningPreset.motionAdaptiveFilteringStrength > 0.0) {
			applyMotionAdaptiveFilteringPass(texelWidth, texelHeight);
		}
	}

	if (extractionMode >= ExtractionMode::EdgeDetection) {
		applySobelPass(texelWidth, texelHeight);

		applySuppressNonMaximumPass(texelWidth, texelHeight);

		applyHysteresisClassifyPass(texelWidth, texelHeight, runningPreset.hysteresisHighThreshold,
					    runningPreset.hysteresisLowThreshold);

		for (int i = 0; i < 8; i++) {
			applyHysteresisPropagatePass(texelWidth, texelHeight);
		}

		applyHysteresisFinalizePass(texelWidth, texelHeight);

		if (runningPreset.morphologyOpeningErosionKernelSize > 1) {
			applyMorphologyPass(texelWidth, texelHeight, drawingEffect->tech_erosion,
					    (int)runningPreset.morphologyOpeningErosionKernelSize);
		}

		if (runningPreset.morphologyOpeningDilationKernelSize > 1) {
			applyMorphologyPass(texelWidth, texelHeight, drawingEffect->tech_dilation,
					    (int)runningPreset.morphologyOpeningDilationKernelSize);
		}

		if (runningPreset.morphologyClosingDilationKernelSize > 1) {
			applyMorphologyPass(texelWidth, texelHeight, drawingEffect->tech_dilation,
					    (int)runningPreset.morphologyClosingDilationKernelSize);
		}

		if (runningPreset.morphologyClosingErosionKernelSize > 1) {
			applyMorphologyPass(texelWidth, texelHeight, drawingEffect->tech_erosion,
					    (int)runningPreset.morphologyClosingErosionKernelSize);
		}
	}

	if (extractionMode >= ExtractionMode::Scaling) {
		applyScalingPass();
	}

	gs_viewport_pop();
	gs_projection_pop();
	gs_matrix_pop();
	gs_set_render_target(default_render_target, NULL);

	drawFinalImage();
}

obs_source_t *ShowDrawFilterContext::getFilter() const noexcept
{
	return filter;
}

Preset ShowDrawFilterContext::getRunningPreset() const noexcept
{
	return runningPreset;
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
		slog(LOG_ERROR) << "Failed to create source texture";
		return false;
	}

	try {
		ensureTexture(texture_target, width, height);
	} catch (const std::bad_alloc &) {
		slog(LOG_ERROR) << "Failed to create target texture";
		return false;
	}

	try {
		ensureTexture(texture_motion_map, width, height);
	} catch (const std::bad_alloc &) {
		slog(LOG_ERROR) << "Failed to create motion map texture";
		return false;
	}

	try {
		ensureTexture(texture_previous_luminance, width, height);
	} catch (const std::bad_alloc &) {
		slog(LOG_ERROR) << "Failed to create previous luminance texture";
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

void ShowDrawFilterContext::applyLuminanceExtractionPass() noexcept
{
	gs_set_render_target(texture_target, nullptr);

	gs_effect_set_texture(drawingEffect->texture_image, texture_source);

	applyEffectPass(drawingEffect->tech_extract_luminance, texture_source);

	std::swap(texture_source, texture_target);
}

void ShowDrawFilterContext::applyMedianFilteringPass(const float texelWidth, const float texelHeight) noexcept
{
	gs_set_render_target(texture_target, nullptr);

	gs_effect_set_texture(drawingEffect->texture_image, texture_source);

	gs_effect_set_float(drawingEffect->float_texel_width, texelWidth);
	gs_effect_set_float(drawingEffect->float_texel_height, texelHeight);
	gs_effect_set_int(drawingEffect->int_kernel_size, (int)runningPreset.medianFilteringKernelSize);

	applyEffectPass(drawingEffect->tech_median_filtering, texture_source);

	std::swap(texture_source, texture_target);
}

void ShowDrawFilterContext::applyMotionAdaptiveFilteringPass(const float texelWidth, const float texelHeight) noexcept
{
	gs_set_render_target(texture_target, nullptr);

	gs_effect_set_texture(drawingEffect->texture_image, texture_source);
	gs_effect_set_texture(drawingEffect->texture_image1, texture_previous_luminance);

	gs_effect_set_float(drawingEffect->float_texel_width, texelWidth);
	gs_effect_set_float(drawingEffect->float_texel_height, texelHeight);
	gs_effect_set_int(drawingEffect->int_kernel_size, (int)runningPreset.motionMapKernelSize);

	applyEffectPass(drawingEffect->tech_calculate_motion_map, texture_source);

	gs_set_render_target(texture_target, nullptr);
	gs_effect_set_texture(drawingEffect->texture_image, texture_source);
	gs_effect_set_texture(drawingEffect->texture_image1, texture_previous_luminance);

	gs_effect_set_float(drawingEffect->float_strength, (float)runningPreset.motionAdaptiveFilteringStrength);
	gs_effect_set_float(drawingEffect->float_motion_threshold,
			    (float)runningPreset.motionAdaptiveFilteringMotionThreshold);

	applyEffectPass(drawingEffect->tech_motion_adaptive_filtering, texture_source);

	gs_copy_texture(texture_previous_luminance, texture_target);

	std::swap(texture_source, texture_target);
}

void ShowDrawFilterContext::applySobelPass(const float texelWidth, const float texelHeight) noexcept
{
	gs_set_render_target(texture_target, nullptr);

	gs_effect_set_texture(drawingEffect->texture_image, texture_source);

	gs_effect_set_float(drawingEffect->float_texel_width, texelWidth);
	gs_effect_set_float(drawingEffect->float_texel_height, texelHeight);

	applyEffectPass(drawingEffect->tech_apply_sobel, texture_source);

	std::swap(texture_source, texture_target);
}

void ShowDrawFilterContext::applySuppressNonMaximumPass(const float texelWidth, const float texelHeight) noexcept
{
	gs_set_render_target(texture_target, nullptr);

	gs_effect_set_texture(drawingEffect->texture_image, texture_source);

	gs_effect_set_float(drawingEffect->float_texel_width, texelWidth);
	gs_effect_set_float(drawingEffect->float_texel_height, texelHeight);

	applyEffectPass(drawingEffect->tech_suppress_non_maximum, texture_source);

	std::swap(texture_source, texture_target);
}

void ShowDrawFilterContext::applyHysteresisClassifyPass(float texelWidth, float texelHeight, float highThreshold,
							float lowThreshold) noexcept
{
	gs_set_render_target(texture_target, nullptr);

	gs_effect_set_texture(drawingEffect->texture_image, texture_source);

	gs_effect_set_float(drawingEffect->float_texel_width, texelWidth);
	gs_effect_set_float(drawingEffect->float_texel_height, texelHeight);
	gs_effect_set_float(drawingEffect->float_high_threshold, highThreshold);
	gs_effect_set_float(drawingEffect->float_low_threshold, lowThreshold);

	applyEffectPass(drawingEffect->tech_hysteresis_classify, texture_source);

	std::swap(texture_source, texture_target);
}

void ShowDrawFilterContext::applyHysteresisPropagatePass(const float texelWidth, const float texelHeight) noexcept
{
	gs_set_render_target(texture_target, nullptr);

	gs_effect_set_texture(drawingEffect->texture_image, texture_source);

	gs_effect_set_float(drawingEffect->float_texel_width, texelWidth);
	gs_effect_set_float(drawingEffect->float_texel_height, texelHeight);

	applyEffectPass(drawingEffect->tech_hysteresis_classify, texture_source);

	std::swap(texture_source, texture_target);
}

void ShowDrawFilterContext::applyHysteresisFinalizePass(const float texelWidth, const float texelHeight) noexcept
{
	gs_set_render_target(texture_target, nullptr);

	gs_effect_set_texture(drawingEffect->texture_image, texture_source);

	gs_effect_set_float(drawingEffect->float_texel_width, texelWidth);
	gs_effect_set_float(drawingEffect->float_texel_height, texelHeight);

	applyEffectPass(drawingEffect->tech_hysteresis_classify, texture_source);

	std::swap(texture_source, texture_target);
}

void ShowDrawFilterContext::applyMorphologyPass(const float texelWidth, const float texelHeight,
						gs_technique_t *technique, int kernelSize) noexcept
{
	gs_set_render_target(texture_target, nullptr);

	gs_effect_set_texture(drawingEffect->texture_image, texture_source);

	gs_effect_set_float(drawingEffect->float_texel_width, texelWidth);
	gs_effect_set_float(drawingEffect->float_texel_height, texelHeight);
	gs_effect_set_int(drawingEffect->int_kernel_size, kernelSize);

	applyEffectPass(technique, texture_source);

	std::swap(texture_source, texture_target);
}

void ShowDrawFilterContext::applyScalingPass() noexcept
{
	gs_set_render_target(texture_target, nullptr);

	gs_effect_set_texture(drawingEffect->texture_image, texture_source);

	gs_effect_set_float(drawingEffect->float_scaling_factor, (float)scaling_factor);

	applyEffectPass(drawingEffect->tech_scaling, texture_source);

	std::swap(texture_source, texture_target);
}

void ShowDrawFilterContext::drawFinalImage() noexcept
{
	gs_effect_set_texture(drawingEffect->texture_image, texture_source);

	size_t passes;
	passes = gs_technique_begin(drawingEffect->tech_draw);
	for (size_t i = 0; i < passes; i++) {
		if (gs_technique_begin_pass(drawingEffect->tech_draw, i)) {
			gs_draw_sprite(texture_target, 0, 0, 0);
			gs_technique_end_pass(drawingEffect->tech_draw);
		}
	}
	gs_technique_end(drawingEffect->tech_draw);
}

} // namespace obs_showdraw
} // namespace kaito_tokyo

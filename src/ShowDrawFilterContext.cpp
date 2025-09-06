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
#include <utility>

#include "plugin-support.h"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include "obs-bridge-utils/obs-bridge-utils.hpp"

#include "DrawingEffect.hpp"
#include "Preset.hpp"
#include "PresetWindow.hpp"

using kaito_tokyo::obs_bridge_utils::slog;

using namespace kaito_tokyo::obs_showdraw;

const char *showdraw_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return ShowDrawFilterContext::getName();
}

void *showdraw_create(obs_data_t *settings, obs_source_t *source)
try {
	auto self = std::make_shared<ShowDrawFilterContext>(settings, source);
	self->afterCreate(settings, source);
	return new std::shared_ptr<ShowDrawFilterContext>(self);
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to create showdraw context: %s", e.what());
	return nullptr;
}

void showdraw_destroy(void *data)
try {
	if (!data) {
		slog(LOG_ERROR) << "showdraw_destroy called with null data";
		return;
	}

	auto self = static_cast<std::shared_ptr<ShowDrawFilterContext> *>(data);
	delete self;
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to destroy showdraw context: %s", e.what());
}

uint32_t showdraw_get_width(void *data)
{
	if (!data) {
		obs_log(LOG_ERROR, "showdraw_get_width called with null data");
		return 0;
	}

	auto self = static_cast<std::shared_ptr<ShowDrawFilterContext> *>(data);
	return self->get()->getWidth();
}

uint32_t showdraw_get_height(void *data)
{
	if (!data) {
		obs_log(LOG_ERROR, "showdraw_get_height called with null data");
		return 0;
	}

	auto self = static_cast<std::shared_ptr<ShowDrawFilterContext> *>(data);
	return self->get()->getHeight();
}

void showdraw_get_defaults(obs_data_t *data)
{
	ShowDrawFilterContext::getDefaults(data);
}

obs_properties_t *showdraw_get_properties(void *data)
try {
	if (!data) {
		slog(LOG_ERROR) << "showdraw_get_properties called with null data";
		return nullptr;
	}

	auto self = static_cast<std::shared_ptr<ShowDrawFilterContext> *>(data);
	return self->get()->getProperties();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to get properties: %s", e.what());
	return nullptr;
}

void showdraw_update(void *data, obs_data_t *settings)
try {
	if (!data) {
		slog(LOG_ERROR) << "showdraw_update called with null data";
		return;
	}

	auto self = static_cast<std::shared_ptr<ShowDrawFilterContext> *>(data);
	self->get()->update(settings);
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to update showdraw context: %s", e.what());
}

void showdraw_activate(void *data)
try {
	if (!data) {
		obs_log(LOG_ERROR, "showdraw_activate called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<ShowDrawFilterContext> *>(data);
	self->get()->activate();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to activate showdraw context: %s", e.what());
}

void showdraw_deactivate(void *data)
try {
	using kaito_tokyo::obs_showdraw::ShowDrawFilterContext;

	if (!data) {
		slog(LOG_ERROR) << "showdraw_deactivate called with null data";
		return;
	}

	auto self = static_cast<std::shared_ptr<ShowDrawFilterContext> *>(data);
	self->get()->deactivate();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to deactivate showdraw context: %s", e.what());
}

void showdraw_show(void *data)
try {
	using kaito_tokyo::obs_showdraw::ShowDrawFilterContext;

	if (!data) {
		slog(LOG_ERROR) << "showdraw_show called with null data";
		return;
	}

	auto self = static_cast<std::shared_ptr<ShowDrawFilterContext> *>(data);
	self->get()->show();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to show showdraw context: %s", e.what());
}

void showdraw_hide(void *data)
try {
	using kaito_tokyo::obs_showdraw::ShowDrawFilterContext;

	if (!data) {
		slog(LOG_ERROR) << "showdraw_hide called with null data";
		return;
	}

	auto self = static_cast<std::shared_ptr<ShowDrawFilterContext> *>(data);
	self->get()->hide();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to hide showdraw context: %s", e.what());
}

void showdraw_video_tick(void *data, float seconds)
try {
	if (!data) {
		slog(LOG_ERROR) << "showdraw_video_tick called with null data";
		return;
	}

	auto self = static_cast<std::shared_ptr<ShowDrawFilterContext> *>(data);
	self->get()->videoTick(seconds);
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to tick showdraw context: %s", e.what());
}

void showdraw_video_render(void *data, gs_effect_t *effect)
try {
	UNUSED_PARAMETER(effect);

	if (!data) {
		slog(LOG_ERROR) << "showdraw_video_render called with null data";
		return;
	}

	auto self = static_cast<std::shared_ptr<ShowDrawFilterContext> *>(data);
	self->get()->videoRender();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to render video in showdraw context: %s", e.what());
}

struct obs_source_frame *showdraw_filter_video(void *data, struct obs_source_frame *frame)
try {
	if (!data) {
		slog(LOG_ERROR) << "showdraw_filter_video called with null data";
		return frame;
	}

	auto self = static_cast<std::shared_ptr<ShowDrawFilterContext> *>(data);
	return self->get()->filterVideo(frame);
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to filter video in showdraw context: %s", e.what());
	return frame;
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

void ShowDrawFilterContext::afterCreate(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(source);

	slog(LOG_INFO) << "Creating showdraw filter context";

	update(settings);

	futureLatestVersion = std::async(std::launch::async, []() {
				      UpdateChecker checker;
				      return checker.fetch();
			      }).share();
}

ShowDrawFilterContext::~ShowDrawFilterContext() noexcept
{
	slog(LOG_INFO) << "Destroying showdraw filter context";

	obs_enter_graphics();
	drawingEffect.release();
	gs_texture_destroy(textureSource);
	gs_texture_destroy(textureTarget);
	gs_texture_destroy(textureMotionMap);
	gs_texture_destroy(texturePreviousLuminance);
	obs_leave_graphics();
}

uint32_t ShowDrawFilterContext::getWidth() const noexcept
{
	return width;
}

uint32_t ShowDrawFilterContext::getHeight() const noexcept
{
	return height;
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

	obs_data_set_default_bool(data, "sobelMagnitudeFinalizationUseLog",
				  defaultPreset.sobelMagnitudeFinalizationUseLog);
	obs_data_set_default_double(data, "sobelMagnitudeFinalizationScalingFactorDb",
				    defaultPreset.sobelMagnitudeFinalizationScalingFactorDb);

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
}

obs_properties_t *ShowDrawFilterContext::getProperties()
{
	obs_properties_t *props = obs_properties_create();

	const char *updateAvailableText = obs_module_text("updateCheckerPluginIsLatest");
	std::optional<LatestVersion> latestVersion = getLatestVersion();
	if (latestVersion.has_value() && latestVersion->isUpdateAvailable(PLUGIN_VERSION)) {
		updateAvailableText = obs_module_text("updateCheckerUpdateAvailable");
	}

	obs_properties_add_text(props, "isUpdateAvailable", updateAvailableText, OBS_TEXT_INFO);

	/*
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
	*/

	obs_property_t *propExtractionMode = obs_properties_add_list(
		props, "extractionMode", obs_module_text("extractionMode"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModeDefault"),
				  static_cast<long long>(ExtractionMode::Default));
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModePassthrough"),
				  static_cast<long long>(ExtractionMode::Passthrough));
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModeLuminanceExtraction"),
				  static_cast<long long>(ExtractionMode::LuminanceExtraction));
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModeSobelMagnitude"),
				  static_cast<long long>(ExtractionMode::SobelMagnitude));
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModeEdgeDetection"),
				  static_cast<long long>(ExtractionMode::EdgeDetection));

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

	obs_properties_add_bool(props, "sobelMagnitudeFinalizationUseLog",
				obs_module_text("sobelMagnitudeFinalizationUseLog"));
	obs_properties_add_float_slider(props, "sobelMagnitudeFinalizationScalingFactorDb",
					obs_module_text("sobelMagnitudeFinalizationScalingFactorDb"), -20.0, 20.0,
					0.01);

	obs_properties_add_float_slider(props, "hysteresisHighThreshold", obs_module_text("hysteresisHighThreshold"),
					0.0, 1.0, 0.01);
	obs_properties_add_float_slider(props, "hysteresisLowThreshold", obs_module_text("hysteresisLowThreshold"), 0.0,
					1.0, 0.01);

	obs_properties_add_int_slider(props, "morphologyOpeningErosionKernelSize",
				      obs_module_text("morphologyOpeningErosionKernelSize"), 1, 31, 2);
	obs_properties_add_int_slider(props, "morphologyOpeningDilationKernelSize",
				      obs_module_text("morphologyOpeningDilationKernelSize"), 1, 31, 2);

	obs_properties_add_int_slider(props, "morphologyClosingDilationKernelSize",
				      obs_module_text("morphologyClosingDilationKernelSize"), 1, 31, 2);
	obs_properties_add_int_slider(props, "morphologyClosingErosionKernelSize",
				      obs_module_text("morphologyClosingErosionKernelSize"), 1, 31, 2);

	return props;
}

void ShowDrawFilterContext::update(obs_data_t *settings)
{
	runningPreset.extractionMode = static_cast<ExtractionMode>(obs_data_get_int(settings, "extractionMode"));

	runningPreset.medianFilteringKernelSize = obs_data_get_int(settings, "medianFilteringKernelSize");

	runningPreset.motionMapKernelSize = obs_data_get_int(settings, "motionMapKernelSize");

	runningPreset.motionAdaptiveFilteringStrength =
		obs_data_get_double(settings, "motionAdaptiveFilteringStrength");
	runningPreset.motionAdaptiveFilteringMotionThreshold =
		obs_data_get_double(settings, "motionAdaptiveFilteringMotionThreshold");

	runningPreset.sobelMagnitudeFinalizationUseLog =
		obs_data_get_bool(settings, "sobelMagnitudeFinalizationUseLog");
	runningPreset.sobelMagnitudeFinalizationScalingFactorDb =
		obs_data_get_double(settings, "sobelMagnitudeFinalizationScalingFactorDb");
	sobelMagnitudeFinalizationScalingFactor =
		std::pow(10.0, runningPreset.sobelMagnitudeFinalizationScalingFactorDb / 10.0);

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
}

void ShowDrawFilterContext::activate()
{
	slog(LOG_INFO) << "Activating showdraw filter context";
}

void ShowDrawFilterContext::deactivate()
{
	slog(LOG_INFO) << "Deactivating showdraw filter context";
}

void ShowDrawFilterContext::show()
{
	slog(LOG_INFO) << "Showing showdraw filter context";
}

void ShowDrawFilterContext::hide()
{
	slog(LOG_INFO) << "Hiding showdraw filter context";
}

void ShowDrawFilterContext::videoTick(float seconds)
{
	UNUSED_PARAMETER(seconds);
}

void ShowDrawFilterContext::videoRender()
{
	if (!drawingEffect) {
		try {
			drawingEffect = std::make_unique<DrawingEffect>();
		} catch (const std::exception &e) {
			slog(LOG_ERROR) << "Failed to create drawing effect: " << e.what();
			obs_source_skip_video_filter(filter);
			return;
		}
	}

	if (width == 0 || height == 0) {
		slog(LOG_INFO) << "Target source has zero width or height";
		obs_source_skip_video_filter(filter);
		return;
	}

	ensureTextures(width, height);

	ExtractionMode extractionMode = runningPreset.extractionMode == ExtractionMode::Default
						? DefaultExtractionMode
						: runningPreset.extractionMode;

	const float texelWidth = 1.0f / (float)width;
	const float texelHeight = 1.0f / (float)height;

	gs_texture_t *default_render_target = gs_get_render_target();

	if (!obs_source_process_filter_begin(filter, GS_BGRA, OBS_ALLOW_DIRECT_RENDERING)) {
		slog(LOG_ERROR) << "Could not begin processing filter";
		obs_source_skip_video_filter(filter);
		return;
	}

	gs_set_render_target(textureTarget, NULL);

	gs_viewport_push();
	gs_projection_push();
	gs_matrix_push();

	gs_set_viewport(0, 0, width, height);
	gs_matrix_identity();

	obs_source_process_filter_end(filter, drawingEffect->effect, 0, 0);

	std::swap(textureSource, textureTarget);

	if (extractionMode >= ExtractionMode::LuminanceExtraction) {
		applyLuminanceExtractionPass();

		if (runningPreset.medianFilteringKernelSize > 1) {
			applyMedianFilteringPass(texelWidth, texelHeight);
		}

		if (runningPreset.motionAdaptiveFilteringStrength > 0.0) {
			applyMotionAdaptiveFilteringPass(texelWidth, texelHeight);
		}
	}

	if (extractionMode >= ExtractionMode::SobelMagnitude) {
		applySobelPass(texelWidth, texelHeight);

		applyFinalizeSobelMagnitudePass();
	}

	if (extractionMode >= ExtractionMode::EdgeDetection) {
		applySuppressNonMaximumPass(texelWidth, texelHeight);

		applyHysteresisClassifyPass(texelWidth, texelHeight, runningPreset.hysteresisHighThreshold,
					    runningPreset.hysteresisLowThreshold);

		for (int i = 0; i < 8; i++) {
			applyHysteresisPropagatePass(texelWidth, texelHeight);
		}

		applyHysteresisFinalizePass(texelWidth, texelHeight);

		if (runningPreset.morphologyOpeningErosionKernelSize > 1) {
			applyMorphologyPass(texelWidth, texelHeight, drawingEffect->techErosion,
					    (int)runningPreset.morphologyOpeningErosionKernelSize);
		}

		if (runningPreset.morphologyOpeningDilationKernelSize > 1) {
			applyMorphologyPass(texelWidth, texelHeight, drawingEffect->techDilation,
					    (int)runningPreset.morphologyOpeningDilationKernelSize);
		}

		if (runningPreset.morphologyClosingDilationKernelSize > 1) {
			applyMorphologyPass(texelWidth, texelHeight, drawingEffect->techDilation,
					    (int)runningPreset.morphologyClosingDilationKernelSize);
		}

		if (runningPreset.morphologyClosingErosionKernelSize > 1) {
			applyMorphologyPass(texelWidth, texelHeight, drawingEffect->techErosion,
					    (int)runningPreset.morphologyClosingErosionKernelSize);
		}
	}

	gs_viewport_pop();
	gs_projection_pop();
	gs_matrix_pop();
	gs_set_render_target(default_render_target, NULL);

	if (extractionMode == ExtractionMode::SobelMagnitude) {
		drawFinalImage(textureFinalSobelMagnitude);
	} else {
		drawFinalImage(textureSource);
	}
}

obs_source_frame *ShowDrawFilterContext::filterVideo(struct obs_source_frame *frame)
{
	width = frame->width;
	height = frame->height;
	return frame;
}

obs_source_t *ShowDrawFilterContext::getFilter() const noexcept
{
	return filter;
}

Preset ShowDrawFilterContext::getRunningPreset() const noexcept
{
	return runningPreset;
}

std::optional<LatestVersion> ShowDrawFilterContext::getLatestVersion() const
{
	if (!futureLatestVersion.valid()) {
		return std::nullopt;
	}

	if (futureLatestVersion.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
		return std::nullopt;
	}

	return futureLatestVersion.get();
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

void ShowDrawFilterContext::ensureTextures(uint32_t width, uint32_t height)
{
	ensureTexture(textureSource, width, height);
	ensureTexture(textureTarget, width, height);
	ensureTexture(textureMotionMap, width, height);
	ensureTexture(texturePreviousLuminance, width, height);
	ensureTexture(textureFinalSobelMagnitude, width, height);
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
	gs_set_render_target(textureTarget, nullptr);

	gs_effect_set_texture(drawingEffect->textureImage, textureSource);

	applyEffectPass(drawingEffect->techExtractLuminance, textureSource);

	std::swap(textureSource, textureTarget);
}

void ShowDrawFilterContext::applyMedianFilteringPass(const float texelWidth, const float texelHeight) noexcept
{
	gs_set_render_target(textureTarget, nullptr);

	gs_effect_set_texture(drawingEffect->textureImage, textureSource);

	gs_effect_set_float(drawingEffect->floatTexelWidth, texelWidth);
	gs_effect_set_float(drawingEffect->floatTexelHeight, texelHeight);
	gs_effect_set_int(drawingEffect->intKernelSize, (int)runningPreset.medianFilteringKernelSize);

	applyEffectPass(drawingEffect->techMedianFiltering, textureSource);

	std::swap(textureSource, textureTarget);
}

void ShowDrawFilterContext::applyMotionAdaptiveFilteringPass(const float texelWidth, const float texelHeight) noexcept
{
	gs_set_render_target(textureTarget, nullptr);

	gs_effect_set_texture(drawingEffect->textureImage, textureSource);
	gs_effect_set_texture(drawingEffect->textureImage1, texturePreviousLuminance);

	gs_effect_set_float(drawingEffect->floatTexelWidth, texelWidth);
	gs_effect_set_float(drawingEffect->floatTexelHeight, texelHeight);
	gs_effect_set_int(drawingEffect->intKernelSize, (int)runningPreset.motionMapKernelSize);

	applyEffectPass(drawingEffect->techCalculateMotionMap, textureSource);

	gs_set_render_target(textureTarget, nullptr);
	gs_effect_set_texture(drawingEffect->textureImage, textureSource);
	gs_effect_set_texture(drawingEffect->textureImage1, texturePreviousLuminance);

	gs_effect_set_float(drawingEffect->floatStrength, (float)runningPreset.motionAdaptiveFilteringStrength);
	gs_effect_set_float(drawingEffect->floatMotionThreshold,
			    (float)runningPreset.motionAdaptiveFilteringMotionThreshold);

	applyEffectPass(drawingEffect->techMotionAdaptiveFiltering, textureSource);

	gs_copy_texture(texturePreviousLuminance, textureTarget);

	std::swap(textureSource, textureTarget);
}

void ShowDrawFilterContext::applySobelPass(const float texelWidth, const float texelHeight) noexcept
{
	gs_set_render_target(textureTarget, nullptr);

	gs_effect_set_texture(drawingEffect->textureImage, textureSource);

	gs_effect_set_float(drawingEffect->floatTexelWidth, texelWidth);
	gs_effect_set_float(drawingEffect->floatTexelHeight, texelHeight);

	applyEffectPass(drawingEffect->techApplySobel, textureSource);

	std::swap(textureSource, textureTarget);
}

void ShowDrawFilterContext::applyFinalizeSobelMagnitudePass() noexcept
{
	gs_set_render_target(textureFinalSobelMagnitude, nullptr);

	gs_effect_set_texture(drawingEffect->textureImage, textureSource);

	gs_effect_set_bool(drawingEffect->boolUseLog, runningPreset.sobelMagnitudeFinalizationUseLog);
	gs_effect_set_float(drawingEffect->floatScalingFactor, (float)sobelMagnitudeFinalizationScalingFactor);

	applyEffectPass(drawingEffect->techFinalizeSobelMagnitude, textureSource);
}

void ShowDrawFilterContext::applySuppressNonMaximumPass(const float texelWidth, const float texelHeight) noexcept
{
	gs_set_render_target(textureTarget, nullptr);

	gs_effect_set_texture(drawingEffect->textureImage, textureSource);

	gs_effect_set_float(drawingEffect->floatTexelWidth, texelWidth);
	gs_effect_set_float(drawingEffect->floatTexelHeight, texelHeight);

	applyEffectPass(drawingEffect->techSuppressNonMaximum, textureSource);

	std::swap(textureSource, textureTarget);
}

void ShowDrawFilterContext::applyHysteresisClassifyPass(float texelWidth, float texelHeight, float highThreshold,
							float lowThreshold) noexcept
{
	gs_set_render_target(textureTarget, nullptr);

	gs_effect_set_texture(drawingEffect->textureImage, textureSource);

	gs_effect_set_float(drawingEffect->floatTexelWidth, texelWidth);
	gs_effect_set_float(drawingEffect->floatTexelHeight, texelHeight);
	gs_effect_set_float(drawingEffect->floatHighThreshold, highThreshold);
	gs_effect_set_float(drawingEffect->floatLowThreshold, lowThreshold);

	applyEffectPass(drawingEffect->techHysteresisClassify, textureSource);

	std::swap(textureSource, textureTarget);
}

void ShowDrawFilterContext::applyHysteresisPropagatePass(const float texelWidth, const float texelHeight) noexcept
{
	gs_set_render_target(textureTarget, nullptr);

	gs_effect_set_texture(drawingEffect->textureImage, textureSource);

	gs_effect_set_float(drawingEffect->floatTexelWidth, texelWidth);
	gs_effect_set_float(drawingEffect->floatTexelHeight, texelHeight);

	applyEffectPass(drawingEffect->techHysteresisPropagate, textureSource);

	std::swap(textureSource, textureTarget);
}

void ShowDrawFilterContext::applyHysteresisFinalizePass(const float texelWidth, const float texelHeight) noexcept
{
	gs_set_render_target(textureTarget, nullptr);

	gs_effect_set_texture(drawingEffect->textureImage, textureSource);

	gs_effect_set_float(drawingEffect->floatTexelWidth, texelWidth);
	gs_effect_set_float(drawingEffect->floatTexelHeight, texelHeight);

	applyEffectPass(drawingEffect->techHysteresisClassify, textureSource);

	std::swap(textureSource, textureTarget);
}

void ShowDrawFilterContext::applyMorphologyPass(const float texelWidth, const float texelHeight,
						gs_technique_t *technique, int kernelSize) noexcept
{
	gs_set_render_target(textureTarget, nullptr);

	gs_effect_set_texture(drawingEffect->textureImage, textureSource);

	gs_effect_set_float(drawingEffect->floatTexelWidth, texelWidth);
	gs_effect_set_float(drawingEffect->floatTexelHeight, texelHeight);
	gs_effect_set_int(drawingEffect->intKernelSize, kernelSize);

	applyEffectPass(technique, textureSource);

	std::swap(textureSource, textureTarget);
}

void ShowDrawFilterContext::drawFinalImage(gs_texture_t *drawingTexture) noexcept
{
	gs_effect_set_texture(drawingEffect->textureImage, drawingTexture);

	size_t passes;
	passes = gs_technique_begin(drawingEffect->techDraw);
	for (size_t i = 0; i < passes; i++) {
		if (gs_technique_begin_pass(drawingEffect->techDraw, i)) {
			gs_draw_sprite(textureTarget, 0, 0, 0);
			gs_technique_end_pass(drawingEffect->techDraw);
		}
	}
	gs_technique_end(drawingEffect->techDraw);
}

} // namespace obs_showdraw
} // namespace kaito_tokyo

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

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "plugin-support.h"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <obs-bridge-utils/obs-bridge-utils.hpp>

#include "DrawingEffect.hpp"
#include "Preset.hpp"
#include "PresetWindow.hpp"

using namespace kaito_tokyo::obs_bridge_utils;
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

	{
		graphics_context_guard guard;
		kaito_tokyo::obs_bridge_utils::gs_unique::drain();
	}
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to destroy showdraw context: %s", e.what());

	{
		graphics_context_guard guard;
		kaito_tokyo::obs_bridge_utils::gs_unique::drain();
	}
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
		obs_log(LOG_ERROR, "showdraw_video_render called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<ShowDrawFilterContext> *>(data);
	try {
		self->get()->videoRender();
	} catch (const skip_video_filter_exception &) {
		obs_source_t *filter = self->get()->getFilter();
		if (filter) {
			obs_source_skip_video_filter(filter);
		}
	}
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to render video in showdraw context: %s", e.what());
}

void showdraw_module_load()
{
	// Nothing to do
}

void showdraw_module_unload()
try {
	graphics_context_guard guard;
	kaito_tokyo::obs_bridge_utils::gs_unique::drain();
} catch (const std::exception &e) {
	obs_log(LOG_ERROR, "Failed to unload showdraw module: %s", e.what());
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

	obs_data_set_default_int(data, "extractionMode", static_cast<long long>(defaultPreset.extractionMode));

	obs_data_set_default_int(data, "medianFilteringKernelSize",
				 static_cast<long long>(defaultPreset.medianFilteringKernelSize));

	obs_data_set_default_int(data, "motionMapKernelSize",
				 static_cast<long long>(defaultPreset.motionMapKernelSize));

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
				 static_cast<long long>(defaultPreset.morphologyOpeningErosionKernelSize));
	obs_data_set_default_int(data, "morphologyOpeningDilationKernelSize",
				 static_cast<long long>(defaultPreset.morphologyOpeningDilationKernelSize));
	obs_data_set_default_int(data, "morphologyClosingDilationKernelSize",
				 static_cast<long long>(defaultPreset.morphologyClosingDilationKernelSize));
	obs_data_set_default_int(data, "morphologyClosingErosionKernelSize",
				 static_cast<long long>(defaultPreset.morphologyClosingErosionKernelSize));
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
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModeMotionMapCalculation"),
				  static_cast<long long>(ExtractionMode::MotionMapCalculation));
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
					obs_module_text("motionAdaptiveFilteringStrength"), 0.0, 1.0, 0.001);
	obs_properties_add_float_slider(props, "motionAdaptiveFilteringMotionThreshold",
					obs_module_text("motionAdaptiveFilteringMotionThreshold"), 0.0, 1.0, 0.001);

	obs_properties_add_bool(props, "sobelMagnitudeFinalizationUseLog",
				obs_module_text("sobelMagnitudeFinalizationUseLog"));
	obs_properties_add_float_slider(props, "sobelMagnitudeFinalizationScalingFactorDb",
					obs_module_text("sobelMagnitudeFinalizationScalingFactorDb"), -20.0, 20.0,
					0.01);

	obs_properties_add_float_slider(props, "hysteresisHighThreshold", obs_module_text("hysteresisHighThreshold"),
					0.0, 1.0, 0.001);
	obs_properties_add_float_slider(props, "hysteresisLowThreshold", obs_module_text("hysteresisLowThreshold"), 0.0,
					1.0, 0.0001);

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

	runningPreset.hysteresisPropagationIterations = obs_data_get_int(settings, "hysteresisPropagationIterations");

	runningPreset.morphologyOpeningErosionKernelSize =
		obs_data_get_int(settings, "morphologyOpeningErosionKernelSize");
	runningPreset.morphologyOpeningDilationKernelSize =
		obs_data_get_int(settings, "morphologyOpeningDilationKernelSize");
	runningPreset.morphologyClosingDilationKernelSize =
		obs_data_get_int(settings, "morphologyClosingDilationKernelSize");
	runningPreset.morphologyClosingErosionKernelSize =
		obs_data_get_int(settings, "morphologyClosingErosionKernelSize");

	obs_enter_graphics();
	try {
		reloadDrawingEffectInGraphics();
	} catch (const std::exception &e) {
		obs_leave_graphics();
		slog(LOG_ERROR) << "Failed to reload drawing effect: " << e.what();
		return;
	}
	obs_leave_graphics();
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
	threadPool.detach_task([self = shared_from_this(), seconds]() { self->processFrame(); });
}

void ShowDrawFilterContext::videoRender()
{
	auto _drawingEffect = std::atomic_load(&drawingEffect);
	if (!_drawingEffect) {
		slog(LOG_DEBUG) << "Drawing effect not loaded";
		throw skip_video_filter_exception();
	}

	if (width == 0 || height == 0) {
		slog(LOG_DEBUG) << "Target source has zero width or height";
		throw skip_video_filter_exception();
	}

	ensureTextures(width, height);

	ExtractionMode extractionMode = runningPreset.extractionMode == ExtractionMode::Default
						? DefaultExtractionMode
						: runningPreset.extractionMode;

	readerCannyEdge->sync();

	gs_texture_t *defaultRenderTarget = gs_get_render_target();

	if (!obs_source_process_filter_begin(filter, GS_BGRA, OBS_ALLOW_DIRECT_RENDERING)) {
		slog(LOG_ERROR) << "Could not begin processing filter";
		throw skip_video_filter_exception();
	}

	gs_set_render_target(textureSource.get(), NULL);

	gs_viewport_push();
	gs_projection_push();
	gs_matrix_push();

	gs_set_viewport(0, 0, width, height);
	gs_matrix_identity();

	obs_source_process_filter_end(filter, _drawingEffect.get()->effect.get(), 0, 0);

	if (extractionMode >= ExtractionMode::LuminanceExtraction) {
		_drawingEffect->applyLuminanceExtractionPass(width, height, textureTarget.get(), textureSource.get());
		std::swap(textureSource, textureTarget);

		if (runningPreset.medianFilteringKernelSize > 1) {
			_drawingEffect->applyMedianFilteringPass(width, height, texelWidth, texelHeight,
								 runningPreset.medianFilteringKernelSize,
								 textureTarget.get(), textureTemporary1.get(),
								 textureSource.get());
			std::swap(textureSource, textureTarget);
		}

		if (runningPreset.motionAdaptiveFilteringStrength > 0.0) {
			_drawingEffect->applyMotionAdaptiveFilteringPass(
				width, height, texelWidth, texelHeight, runningPreset.motionMapKernelSize,
				runningPreset.motionAdaptiveFilteringStrength,
				runningPreset.motionAdaptiveFilteringMotionThreshold, textureTarget.get(),
				textureMotionMap.get(), textureTemporary2.get(), textureSource.get(),
				texturePreviousLuminance.get());
			std::swap(textureSource, textureTarget);

			gs_copy_texture(texturePreviousLuminance.get(), textureSource.get());
		}
	}

	if (extractionMode >= ExtractionMode::SobelMagnitude) {
		std::shared_ptr<gs_texture_t> textureComplexSobel = textureTemporary1;

		_drawingEffect->applySobelPass(width, height, texelWidth, texelHeight, textureComplexSobel.get(),
					       textureSource.get());

		_drawingEffect->applyFinalizeSobelMagnitudePass(width, height,
								runningPreset.sobelMagnitudeFinalizationUseLog,
								sobelMagnitudeFinalizationScalingFactor,
								textureSource.get(), textureComplexSobel.get());

		if (runningPreset.morphologyOpeningErosionKernelSize > 1) {
			_drawingEffect->applyMorphologyPass(width, height, _drawingEffect->techHorizontalErosion,
							    _drawingEffect->techVerticalErosion, texelWidth,
							    texelHeight,
							    (int)runningPreset.morphologyOpeningErosionKernelSize,
							    textureTarget.get(), textureTemporary2.get(),
							    textureSource.get());
			std::swap(textureSource, textureTarget);
		}

		if (runningPreset.morphologyOpeningDilationKernelSize > 1) {
			_drawingEffect->applyMorphologyPass(width, height, _drawingEffect->techHorizontalDilation,
							    _drawingEffect->techVerticalDilation, texelWidth,
							    texelHeight,
							    (int)runningPreset.morphologyOpeningDilationKernelSize,
							    textureTarget.get(), textureTemporary2.get(),
							    textureSource.get());
			std::swap(textureSource, textureTarget);
		}

		if (runningPreset.morphologyClosingDilationKernelSize > 1) {
			_drawingEffect->applyMorphologyPass(width, height, _drawingEffect->techHorizontalDilation,
							    _drawingEffect->techVerticalDilation, texelWidth,
							    texelHeight,
							    (int)runningPreset.morphologyClosingDilationKernelSize,
							    textureTarget.get(), textureTemporary2.get(),
							    textureSource.get());
			std::swap(textureSource, textureTarget);
		}

		if (runningPreset.morphologyClosingErosionKernelSize > 1) {
			_drawingEffect->applyMorphologyPass(width, height, _drawingEffect->techHorizontalErosion,
							    _drawingEffect->techVerticalErosion, texelWidth,
							    texelHeight,
							    (int)runningPreset.morphologyClosingErosionKernelSize,
							    textureTarget.get(), textureTemporary2.get(),
							    textureSource.get());
			std::swap(textureSource, textureTarget);
		}

		gs_copy_texture(textureFinalSobelMagnitude.get(), textureSource.get());
		gs_copy_texture(textureSource.get(), textureComplexSobel.get());
	}

	if (extractionMode >= ExtractionMode::EdgeDetection) {
		_drawingEffect->applySuppressNonMaximumPass(width, height, texelWidth, texelHeight, textureTarget.get(),
							    textureSource.get());
		std::swap(textureSource, textureTarget);

		_drawingEffect->applyHysteresisClassifyPass(width, height, texelWidth, texelHeight,
							    runningPreset.hysteresisHighThreshold,
							    runningPreset.hysteresisLowThreshold, textureTarget.get(),
							    textureSource.get());
		std::swap(textureSource, textureTarget);

		for (int i = 0; i < runningPreset.hysteresisPropagationIterations; i++) {
			_drawingEffect->applyHysteresisPropagatePass(width, height, texelWidth, texelHeight,
								     textureTarget.get(), textureSource.get());
			std::swap(textureSource, textureTarget);
		}

		_drawingEffect->applyHysteresisFinalizePass(width, height, texelWidth, texelHeight, textureTarget.get(),
							    textureSource.get());
		std::swap(textureSource, textureTarget);

		gs_copy_texture(textureCannyEdge.get(), textureSource.get());
		readerCannyEdge->stage(textureCannyEdge.get());
	}

	gs_viewport_pop();
	gs_projection_pop();
	gs_matrix_pop();

	if (extractionMode == ExtractionMode::MotionMapCalculation) {
		_drawingEffect->drawFinalImage(width, height, defaultRenderTarget, textureMotionMap.get());
	} else if (extractionMode == ExtractionMode::SobelMagnitude) {
		_drawingEffect->drawFinalImage(width, height, defaultRenderTarget, textureFinalSobelMagnitude.get());
	} else {
		_drawingEffect->drawFinalImage(width, height, defaultRenderTarget, textureSource.get());
	}

	kaito_tokyo::obs_bridge_utils::gs_unique::drain();
}

obs_source_frame *ShowDrawFilterContext::filterVideo(struct obs_source_frame *frame)
{
	width = frame->width;
	height = frame->height;
	texelWidth = 1.0f / (float)frame->width;
	texelHeight = 1.0f / (float)frame->height;
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

void ShowDrawFilterContext::reloadDrawingEffectInGraphics()
try {
	unique_bfree_t effectPath(obs_module_file("effects/drawing.effect"));
	if (!effectPath) {
		throw std::runtime_error("Could not find effect file");
	}

	char *rawErrorString = nullptr;
	gs_effect_t *rawEffect = gs_effect_create_from_file(effectPath.get(), &rawErrorString);
	unique_gs_effect_t effect(rawEffect);
	unique_bfree_t errorString(rawErrorString);

	if (!effect) {
		const char *msg = errorString ? errorString.get() : "(unknown error)";
		throw std::runtime_error(std::string("gs_effect_create_from_file failed: ") + msg);
	}

	auto newDrawingEffect = std::make_shared<DrawingEffect>(std::move(effect));

	std::atomic_store(&drawingEffect, newDrawingEffect);
} catch (const std::exception &e) {
	slog(LOG_ERROR) << "Failed to load drawing effect: " << e.what();
	throw;
}

void ensureTexture(std::shared_ptr<gs_texture_t> &texture, uint32_t width, uint32_t height)
{
	if (!texture || gs_texture_get_width(texture.get()) != width ||
	    gs_texture_get_height(texture.get()) != height) {
		unique_gs_texture_t newTexture =
			make_unique_gs_texture(width, height, GS_BGRA, 1, NULL, GS_RENDER_TARGET);
		texture = std::shared_ptr<gs_texture_t>(std::move(newTexture));
	}
}

void ensureTextureReader(std::unique_ptr<AsyncTextureReader<2>> &textureReader, uint32_t width, uint32_t height)
{
	if (!textureReader || textureReader->getWidth() != width || textureReader->getHeight() != height) {
		textureReader = std::make_unique<AsyncTextureReader<2>>(width, height);
	}
}

void ShowDrawFilterContext::ensureTextures(uint32_t width, uint32_t height)
{
	ensureTexture(textureSource, width, height);
	ensureTexture(textureTarget, width, height);
	ensureTexture(textureTemporary1, width, height);
	ensureTexture(textureTemporary2, width, height);
	ensureTexture(texturePreviousLuminance, width, height);
	ensureTexture(textureMotionMap, width, height);
	ensureTexture(textureFinalSobelMagnitude, width, height);
	ensureTexture(textureCannyEdge, width, height);
	ensureTextureReader(readerCannyEdge, width, height);
}

void ShowDrawFilterContext::processFrame() noexcept try
{
	if (!readerCannyEdge) {
		slog(LOG_DEBUG) << "Texture reader not initialized yet";
		return;
	}

	cv::Mat cannyEdgeImage(height, width, CV_8UC4, readerCannyEdge->getBuffer().data(),
			       readerCannyEdge->getBufferLinesize());
	cv::Mat singleChannelImage;
	cv::extractChannel(cannyEdgeImage, singleChannelImage, 0);

	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(singleChannelImage, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
	slog(LOG_INFO) << "Found " << contours.size() << " contours";

    std::vector<std::vector<cv::Point>> found_contours;

    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area < 500) {
            continue;
        }

        std::vector<cv::Point> approx;
        double peri = cv::arcLength(contour, true);
        cv::approxPolyDP(contour, approx, 0.02 * peri, true);

        if (approx.size() == 4) {
            cv::Rect rect = cv::boundingRect(approx);
            double aspect_ratio = static_cast<double>(rect.width) / rect.height;
            const double a4_aspect_ratio = 1.414;
            const double tolerance = 0.25;

            if ((std::abs(aspect_ratio - a4_aspect_ratio) < tolerance) ||
                (std::abs(1.0 / aspect_ratio - a4_aspect_ratio) < tolerance)) {
                
                // --- ステップ5: 凸性の確認（オプション） ---
                if (cv::isContourConvex(approx)) {
                    found_contours.push_back(approx);
                }
            }
        }
    }

	std::cout << "Detected " << found_contours.size() << " A4-like contours." << std::endl;
} catch (const std::exception &e) {
	slog(LOG_ERROR) << "Failed to process frame: " << e.what();
}

} // namespace obs_showdraw
} // namespace kaito_tokyo

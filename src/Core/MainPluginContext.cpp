/*
ShowDraw
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

#include "MainPluginContext.h"

#include <obs.h>

#include "../BridgeUtils/GsUnique.hpp"
#include "../BridgeUtils/ObsLogger.hpp"

using namespace KaitoTokyo::BridgeUtils;

namespace KaitoTokyo {
namespace ShowDraw {

MainPluginContext::MainPluginContext(obs_data_t *settings, obs_source_t *_source)
	: source{_source},
	  logger("[" PLUGIN_NAME "] "),
	  mainEffect(unique_obs_module_file("effects/main.effect"))
{
	update(settings);
}

MainPluginContext::~MainPluginContext() noexcept
{
	logger.info("Destroying context");
}

void MainPluginContext::shutdown() noexcept
{
	renderingContext.reset();
	logger.info("Context shut down");
}

uint32_t MainPluginContext::getWidth() const noexcept
{
	return renderingContext ? renderingContext->width : 0;
}

uint32_t MainPluginContext::getHeight() const noexcept
{
	return renderingContext ? renderingContext->height : 0;
}

void MainPluginContext::getDefaults(obs_data_t *) {}

obs_properties_t *MainPluginContext::getProperties()
{
	obs_properties_t *props = obs_properties_create();

	const char *updateText = obs_module_text("updateCheckerPluginIsLatest");
	obs_properties_add_text(props, "update_check", updateText, OBS_TEXT_INFO);

	obs_property_t *p = obs_properties_add_list(props, "extractionMode", obs_module_text("extractionMode"),
						    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, obs_module_text("extractionModeDefault"),
				  static_cast<long long>(ExtractionMode::Default));
	obs_property_list_add_int(p, obs_module_text("extractionModePassthrough"),
				  static_cast<long long>(ExtractionMode::Passthrough));
	obs_property_list_add_int(p, obs_module_text("extractionModeConvertGrayscale"),
				  static_cast<long long>(ExtractionMode::ConvertToGrayscale));
	obs_property_list_add_int(p, obs_module_text("extractionModeMotionMapCalculation"),
				  static_cast<long long>(ExtractionMode::MotionMapCalculation));
	obs_property_list_add_int(p, obs_module_text("extractionModeSobelMagnitude"),
				  static_cast<long long>(ExtractionMode::SobelMagnitude));

	obs_properties_add_float_slider(props, "motionAdaptiveFilteringStrength",
					obs_module_text("motionAdaptiveFilteringStrength"), 0.0, 1.0, 0.001);
	obs_properties_add_float_slider(props, "motionAdaptiveFilteringMotionThreshold",
					obs_module_text("motionAdaptiveFilteringMotionThreshold"), 0.0, 1.0, 0.001);

	obs_properties_add_bool(props, "sobelMagnitudeFinalizationUseLog",
				obs_module_text("sobelMagnitudeFinalizationUseLog"));
	obs_properties_add_float_slider(props, "sobelMagnitudeFinalizationScalingFactorDb",
					obs_module_text("sobelMagnitudeFinalizationScalingFactorDb"), -20.0, 20.0,
					0.01);

	return props;
}

void MainPluginContext::update(obs_data_t *) {}

void MainPluginContext::activate()
{
	logger.info("Filter activated");
}
void MainPluginContext::deactivate()
{
	logger.info("Filter deactivated");
}
void MainPluginContext::show()
{
	logger.info("Filter shown");
}
void MainPluginContext::hide()
{
	logger.info("Filter hidden");
}

void MainPluginContext::videoTick(float) {}

void MainPluginContext::videoRender()
{
	if (renderingContext) {
		renderingContext->videoRender();
	}
}

obs_source_frame *MainPluginContext::filterVideo(obs_source_frame *frame)
try {
	if (!frame) {
		logger.error("filterVideo called with null frame");
		return nullptr;
	}

	if (frame->width == 0 || frame->height == 0) {
		renderingContext.reset();
		return frame;
	}

	if (!renderingContext || frame->width != renderingContext->width || frame->height != renderingContext->height) {
		GraphicsContextGuard guard;
		renderingContext = std::make_shared<RenderingContext>(source, logger, mainEffect, frame->width,
								      frame->height, preset);
		GsUnique::drain();
	}

	if (renderingContext) {
		return renderingContext->filterVideo(frame);
	} else {
		return frame;
	}
} catch (const std::exception &e) {
	logger.error("Exception in filterVideo: {}", e.what());
	return frame;
} catch (...) {
	logger.error("Unknown exception in filterVideo");
	return frame;
}

} // namespace ShowDraw
} // namespace KaitoTokyo

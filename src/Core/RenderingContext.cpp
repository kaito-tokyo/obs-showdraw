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

#include "RenderingContext.hpp"

#include <obs.h>

#include "../BridgeUtils/GsUnique.hpp"
#include "../BridgeUtils/ILogger.hpp"

using namespace KaitoTokyo::BridgeUtils;

namespace KaitoTokyo {
namespace ShowDraw {

RenderingContext::RenderingContext(obs_source_t *_source, const KaitoTokyo::BridgeUtils::ILogger &_logger,
				   const MainEffect &_mainEffect, std::uint32_t _width, std::uint32_t _height,
				   const Preset &_preset)
	: source(_source),
	  logger(_logger),
	  mainEffect(_mainEffect),
	  width(_width),
	  height(_height),
	  preset(_preset),
	  bgrxSource(make_unique_gs_texture(width, height, GS_BGRX, 1, nullptr, GS_RENDER_TARGET)),
	  r8SourceGrayscale(make_unique_gs_texture(width, height, GS_R8, 1, nullptr, GS_RENDER_TARGET)),
	  r8MedianFilteredGrayscale(make_unique_gs_texture(width, height, GS_R8, 1, nullptr, GS_RENDER_TARGET)),
	  r8MotionMap(make_unique_gs_texture(width, height, GS_R8, 1, nullptr, GS_RENDER_TARGET)),
	  r8MotionAdaptiveGrayscales{make_unique_gs_texture(width, height, GS_R8, 1, nullptr, GS_RENDER_TARGET),
				     make_unique_gs_texture(width, height, GS_R8, 1, nullptr, GS_RENDER_TARGET)},
	  bgrxComplexSobel(make_unique_gs_texture(width, height, GS_BGRX, 1, nullptr, GS_RENDER_TARGET)),
	  r8FinalSobelMagnitude(make_unique_gs_texture(width, height, GS_R8, 1, nullptr, GS_RENDER_TARGET)),
	  r32fIntermediate(make_unique_gs_texture(width, height, GS_R32F, 1, nullptr, GS_RENDER_TARGET))
{
}

RenderingContext::~RenderingContext() noexcept {}

void RenderingContext::videoTick(float) {}

obs_source_frame *RenderingContext::filterVideo(obs_source_frame *frame)
{
	if (frame && frame->timestamp != lastFrameTimestamp) {
		lastFrameTimestamp = frame->timestamp;
		doesNextVideoRenderReceiveNewFrame = true;
	}

	return frame;
}

void RenderingContext::videoRender()
{
	ExtractionMode extractionMode = getExtractionMode();

	bool isProcessingNewFrame = doesNextVideoRenderReceiveNewFrame.exchange(false);

	const unique_gs_texture_t *grayscaleResult;

	if (isProcessingNewFrame) {
		isProcessingNewFrame = false;
		if (extractionMode >= ExtractionMode::Passthrough) {
			mainEffect.drawSource(bgrxSource, source);
		}

		if (extractionMode >= ExtractionMode::ConvertToGrayscale) {
			mainEffect.applyConvertToGrayscale(r8SourceGrayscale, bgrxSource);
			grayscaleResult = &r8SourceGrayscale;

			if (preset.medianFilterEnabled) {
				mainEffect.applyMedianFilter(r8MedianFilteredGrayscale, r32fIntermediate,
							     *grayscaleResult);
				grayscaleResult = &r8MedianFilteredGrayscale;
			}

			if (preset.motionAdaptiveFilteringStrength > 0.0) {
				std::swap(r8MotionAdaptiveGrayscales[0], r8MotionAdaptiveGrayscales[1]);
				mainEffect.applyMotionAdaptiveFilter(
					r8MotionAdaptiveGrayscales[0], r8MotionMap, r32fIntermediate, *grayscaleResult,
					r8MotionAdaptiveGrayscales[1],
					static_cast<float>(preset.motionAdaptiveFilteringStrength),
					static_cast<float>(preset.motionAdaptiveFilteringMotionThreshold));
				grayscaleResult = &r8MotionAdaptiveGrayscales[0];
			}
		}

		if (extractionMode >= ExtractionMode::SobelMagnitude) {
			mainEffect.applySobel(bgrxComplexSobel, *grayscaleResult);
			mainEffect.applyFinalizeSobelMagnitude(r8FinalSobelMagnitude, bgrxComplexSobel,
							       preset.sobelUseLog,
							       static_cast<float>(preset.sobelScalingFactor));
		}
	}

	if (extractionMode == ExtractionMode::Passthrough) {
		mainEffect.drawTexture(bgrxSource);
	} else if (extractionMode == ExtractionMode::ConvertToGrayscale) {
		mainEffect.drawGrayscaleTexture(*grayscaleResult);
	} else if (extractionMode == ExtractionMode::MotionMapCalculation) {
		mainEffect.drawGrayscaleTexture(r8MotionMap);
	} else if (extractionMode == ExtractionMode::SobelMagnitude) {
		mainEffect.drawGrayscaleTexture(r8FinalSobelMagnitude);
	}
}

void RenderingContext::updatePreset(const Preset &newPreset)
{
	preset = newPreset;
}

} // namespace ShowDraw
} // namespace KaitoTokyo

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

#include "RenderingContext.hpp"

#include <graphics/vec4.h>
#include <obs.h>

#include <algorithm>
#include <array>
#include <vector>

#include "../BridgeUtils/GsUnique.hpp"
#include "../BridgeUtils/ILogger.hpp"

using namespace KaitoTokyo::BridgeUtils;
using namespace KaitoTokyo::ShowDraw;

namespace {

inline double getYoloxScale(std::uint32_t width, std::uint32_t height)
{
	double widthScale = static_cast<double>(ShowDrawCanvasDetectorBridge::WIDTH) / static_cast<double>(width);
	double heightScale = static_cast<double>(ShowDrawCanvasDetectorBridge::HEIGHT) / static_cast<double>(height);
	return std::min(widthScale, heightScale);
}

inline std::array<std::uint32_t, 4> getYoloxOffsetAndSize(std::uint32_t width, std::uint32_t height)
{
	double scale = getYoloxScale(width, height);

	double scaledWidth = static_cast<double>(width) * scale;
	double scaledHeight = static_cast<double>(height) * scale;

	double offsetX = (640.0 - scaledWidth) / 2.0;
	double offsetY = (640.0 - scaledHeight) / 2.0;

	return {static_cast<std::uint32_t>(offsetX), static_cast<std::uint32_t>(offsetY),
		static_cast<std::uint32_t>(scaledWidth), static_cast<std::uint32_t>(scaledHeight)};
}

} // namespace

namespace KaitoTokyo {
namespace ShowDraw {

RenderingContext::RenderingContext(obs_source_t *_source, const BridgeUtils::ILogger &_logger,
				   const MainEffect &_mainEffect, ThrottledTaskQueue &_taskQueue, std::uint32_t _width,
				   std::uint32_t _height)
	: source(_source),
	  logger(_logger),
	  mainEffect(_mainEffect),
	  taskQueue(_taskQueue),
	  width(_width),
	  height(_height),
	  bgrxSource(make_unique_gs_texture(width, height, GS_BGRX, 1, nullptr, GS_RENDER_TARGET)),
	  r8SourceGrayscale(make_unique_gs_texture(width, height, GS_R8, 1, nullptr, GS_RENDER_TARGET)),
	  r8MedianFilteredGrayscale(make_unique_gs_texture(width, height, GS_R8, 1, nullptr, GS_RENDER_TARGET)),
	  r8MotionMap(make_unique_gs_texture(width, height, GS_R8, 1, nullptr, GS_RENDER_TARGET)),
	  r8MotionAdaptiveGrayscales{make_unique_gs_texture(width, height, GS_R8, 1, nullptr, GS_RENDER_TARGET),
				     make_unique_gs_texture(width, height, GS_R8, 1, nullptr, GS_RENDER_TARGET)},
	  bgrxComplexSobel(make_unique_gs_texture(width, height, GS_BGRX, 1, nullptr, GS_RENDER_TARGET)),
	  r8FinalSobelMagnitude(make_unique_gs_texture(width, height, GS_R8, 1, nullptr, GS_RENDER_TARGET)),
	  yoloxScale(getYoloxScale(width, height)),
	  yoloxOffsetX(getYoloxOffsetAndSize(width, height)[0]),
	  yoloxOffsetY(getYoloxOffsetAndSize(width, height)[1]),
	  yoloxScaledWidth(getYoloxOffsetAndSize(width, height)[2]),
	  yoloxScaledHeight(getYoloxOffsetAndSize(width, height)[3]),
	  bgrxYoloxInput(make_unique_gs_texture(640, 640, GS_BGRX, 1, nullptr, GS_RENDER_TARGET)),
	  bgrxYoloxInputReader(640, 640, GS_BGRX),
	  r32fIntermediate(make_unique_gs_texture(width, height, GS_R32F, 1, nullptr, GS_RENDER_TARGET))
{
}

RenderingContext::~RenderingContext() noexcept {}

void RenderingContext::kickDetection()
{
	taskQueue.push([weakSelf = weak_from_this()](const ThrottledTaskQueue::CancellationToken &token) {
		try {
			if (auto self = weakSelf.lock()) {
				if (token->load()) {
					return;
				}
				self->canvasDetectorResults = self->canvasDetector.detect(
					self->bgrxYoloxInputReader.getBuffer().data(),
					self->bgrxYoloxInputReader.getWidth(), self->bgrxYoloxInputReader.getHeight());
			}
		} catch (std::exception &e) {
			blog(LOG_INFO, "RenderingContext has been destroyed, skipping segmentation");
		}
	});
}

void RenderingContext::videoTick(float)
{
	kickDetection();
}

obs_source_frame *RenderingContext::filterVideo(obs_source_frame *frame)
{
	if (frame && frame->timestamp != lastFrameTimestamp) {
		lastFrameTimestamp = frame->timestamp;
		doesNextVideoRenderReceiveNewFrame = true;
	}

	return frame;
}

void RenderingContext::videoRender(const std::shared_ptr<const Preset> &preset)
{
	ExtractionMode extractionMode = getExtractionMode(*preset);
	DetectionMode detectionMode = getDetectionMode(*preset);

	bool isProcessingNewFrame = doesNextVideoRenderReceiveNewFrame.exchange(false);

	const unique_gs_texture_t *grayscaleResult;

	if (isProcessingNewFrame) {
		try {
			bgrxYoloxInputReader.sync();
		} catch (std::exception &e) {
			logger.logException(e, "An error occurred while processing a new frame");
		}

		if (extractionMode >= ExtractionMode::Passthrough) {
			mainEffect.drawSource(bgrxSource, source);
		}

		if (extractionMode >= ExtractionMode::ConvertToGrayscale) {
			mainEffect.applyConvertToGrayscale(r8SourceGrayscale, bgrxSource);
			grayscaleResult = &r8SourceGrayscale;

			if (preset->medianFilterEnabled) {
				mainEffect.applyMedianFilter(r8MedianFilteredGrayscale, r32fIntermediate,
							     *grayscaleResult);
				grayscaleResult = &r8MedianFilteredGrayscale;
			}

			if (preset->motionAdaptiveFilteringStrength > 0.0) {
				std::swap(r8MotionAdaptiveGrayscales[0], r8MotionAdaptiveGrayscales[1]);
				mainEffect.applyMotionAdaptiveFilter(
					r8MotionAdaptiveGrayscales[0], r8MotionMap, r32fIntermediate, *grayscaleResult,
					r8MotionAdaptiveGrayscales[1],
					static_cast<float>(preset->motionAdaptiveFilteringStrength),
					static_cast<float>(preset->motionAdaptiveFilteringMotionThreshold));
				grayscaleResult = &r8MotionAdaptiveGrayscales[0];
			}
		}

		if (extractionMode >= ExtractionMode::SobelMagnitude) {
			mainEffect.applySobel(bgrxComplexSobel, *grayscaleResult);
			mainEffect.applyFinalizeSobelMagnitude(r8FinalSobelMagnitude, bgrxComplexSobel,
							       preset->sobelUseLog,
							       static_cast<float>(preset->sobelScalingFactor.linear));
		}

		if (detectionMode >= DetectionMode::DrawBoundingBoxes) {
			RenderTargetGuard renderTargetGuard;
			TransformStateGuard transformStateGuard;

			gs_set_viewport(0.0f, 0.0f, ShowDrawCanvasDetectorBridge::WIDTH,
					ShowDrawCanvasDetectorBridge::HEIGHT);
			gs_ortho(0.0f, ShowDrawCanvasDetectorBridge::WIDTH, 0.0f, ShowDrawCanvasDetectorBridge::HEIGHT,
				 -100.0f, 100.0f);
			gs_matrix_identity();
			gs_matrix_translate3f(yoloxOffsetX, yoloxOffsetY, 0.0f);

			gs_set_render_target_with_color_space(bgrxYoloxInput.get(), nullptr, GS_CS_SRGB);

			vec4 gray = {0.5f, 0.5f, 0.5f, 1.0f};
			gs_clear(GS_CLEAR_COLOR, &gray, 1.0f, 0);

			mainEffect.drawTexture(bgrxSource, yoloxScaledWidth, yoloxScaledHeight);
		}

		bgrxYoloxInputReader.stage(bgrxYoloxInput.get());
	}

	vec4 black = {0.0f, 0.0f, 0.0f, 1.0f};
	gs_clear(GS_CLEAR_COLOR, &black, 1.0f, 0);

	if (detectionMode == DetectionMode::CenterFraming) {
		float targetX = 0.0f;
		float targetY = 0.0f;
		float targetScale = 1.0f;

		if (!canvasDetectorResults.empty()) {
			float min_x = canvasDetectorResults[0].x;
			float min_y = canvasDetectorResults[0].y;
			float max_x_plus_w = canvasDetectorResults[0].x + canvasDetectorResults[0].width;
			float max_y_plus_h = canvasDetectorResults[0].y + canvasDetectorResults[0].height;

			for (size_t i = 1; i < canvasDetectorResults.size(); ++i) {
				const auto &r = canvasDetectorResults[i];
				min_x = std::min(min_x, r.x);
				min_y = std::min(min_y, r.y);
				max_x_plus_w = std::max(max_x_plus_w, r.x + r.width);
				max_y_plus_h = std::max(max_y_plus_h, r.y + r.height);
			}

			const CanvasDetectorResult result = {min_x,
								     min_y,
								     max_x_plus_w - min_x,
								     max_y_plus_h - min_y,
								     canvasDetectorResults[0].confidence};

			float x = (result.x - yoloxOffsetX) / yoloxScale;
			float y = (result.y - yoloxOffsetY) / yoloxScale;
			float w = result.width / yoloxScale;
			float h = result.height / yoloxScale;

			// Clamp the bounding box to be within [0, 0, width, height]
			const float x2 = x + w;
			const float y2 = y + h;

			x = std::max(0.0f, x);
			y = std::max(0.0f, y);

			w = std::min((float)width, x2) - x;
			h = std::min((float)height, y2) - y;

			if (w > 0 && h > 0) {
				logger.info("{} {} {} {}", x, y, w, h);

				targetX = -x;
				targetY = -y;
				const float wscale = (float)width / w;
				const float hscale = (float)height / h;
				targetScale = std::min(wscale, hscale);
			}
		}

		const float lerpFactor = 0.001f;
		currentTransformX += (targetX - currentTransformX) * lerpFactor;
		currentTransformY += (targetY - currentTransformY) * lerpFactor;
		currentTransformScale += (targetScale - currentTransformScale) * lerpFactor;

		gs_matrix_push();
		gs_matrix_translate3f(currentTransformX, currentTransformY, 0.0f);
		gs_matrix_scale3f(currentTransformScale, currentTransformScale, 1.0f);
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

	if (detectionMode == DetectionMode::DrawBoundingBoxes) {
		for (const auto &result : canvasDetectorResults) {
			const float x = (result.x - yoloxOffsetX) / yoloxScale;
			const float y = (result.y - yoloxOffsetY) / yoloxScale;
			const float w = result.width / yoloxScale;
			const float h = result.height / yoloxScale;

			mainEffect.drawRectangle(x, y, w, h, 10.0f, 0xFF00FF00);
		}
	}

	if (detectionMode == DetectionMode::CenterFraming) {
		gs_matrix_pop();
	}
}

} // namespace ShowDraw
} // namespace KaitoTokyo

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

#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

#include <obs.h>

#include "../BridgeUtils/AsyncTextureReader.hpp"
#include "../BridgeUtils/GsUnique.hpp"
#include "../BridgeUtils/ThrottledTaskQueue.hpp"

#include "MainEffect.hpp"
#include "Preset.hpp"

#include "../CoreMLShowDrawCanvasDetector/ShowDrawCanvasDetectorBridge.hpp"

namespace KaitoTokyo {
namespace ShowDraw {

class RenderingContext : public std::enable_shared_from_this<RenderingContext> {
public:
	obs_source_t *const source;
	const BridgeUtils::ILogger &logger;
	const MainEffect &mainEffect;
	BridgeUtils::ThrottledTaskQueue &taskQueue;

	const std::uint32_t width;
	const std::uint32_t height;

	const BridgeUtils::unique_gs_texture_t bgrxSource;
	const BridgeUtils::unique_gs_texture_t r8SourceGrayscale;
	const BridgeUtils::unique_gs_texture_t r8MedianFilteredGrayscale;
	const BridgeUtils::unique_gs_texture_t r8MotionMap;
	std::array<BridgeUtils::unique_gs_texture_t, 2> r8MotionAdaptiveGrayscales;

	const BridgeUtils::unique_gs_texture_t bgrxComplexSobel;
	const BridgeUtils::unique_gs_texture_t r8FinalSobelMagnitude;

	const double yoloxScale;
	const std::uint32_t yoloxOffsetX;
	const std::uint32_t yoloxOffsetY;
	const std::uint32_t yoloxScaledWidth;
	const std::uint32_t yoloxScaledHeight;
	const BridgeUtils::unique_gs_texture_t bgrxYoloxInput;
	BridgeUtils::AsyncTextureReader bgrxYoloxInputReader;

	ShowDrawCanvasDetectorBridge canvasDetector;
	std::vector<CanvasDetectorResult> canvasDetectorResults;

private:
	const BridgeUtils::unique_gs_texture_t r32fIntermediate;

private:
	std::uint64_t lastFrameTimestamp = 0;
	std::atomic<bool> doesNextVideoRenderReceiveNewFrame = false;

	float currentTransformX = 0.0f;
	float currentTransformY = 0.0f;
	float currentTransformScale = 1.0f;

public:
	RenderingContext(obs_source_t *source, const BridgeUtils::ILogger &logger,
			 const MainEffect &mainEffect, BridgeUtils::ThrottledTaskQueue &taskQueue, std::uint32_t width, std::uint32_t height);
	~RenderingContext() noexcept;

	void videoTick(float seconds);
	obs_source_frame *filterVideo(obs_source_frame *frame);
	void videoRender(const std::shared_ptr<const Preset> &preset);

private:
	static ExtractionMode getExtractionMode(const Preset &preset) noexcept
	{
		return preset.extractionMode == ExtractionMode::Default ? ExtractionMode::SobelMagnitude : preset.extractionMode;
	}

	static DetectionMode getDetectionMode(const Preset &preset) noexcept
	{
		return preset.detectionMode == DetectionMode::Default ? DetectionMode::DrawBoundingBoxes : preset.detectionMode;
	}

	void kickDetection();
};

} // namespace ShowDraw
} // namespace KaitoTokyo

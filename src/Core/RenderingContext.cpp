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
#include <vector>

#include "../BridgeUtils/GsUnique.hpp"
#include "../BridgeUtils/ILogger.hpp"

using namespace KaitoTokyo::BridgeUtils;

namespace KaitoTokyo {
namespace ShowDraw {

RenderingContext::RenderingContext(obs_source_t *_source, const KaitoTokyo::BridgeUtils::ILogger &_logger,
				   const MainEffect &_mainEffect, std::uint32_t _width, std::uint32_t _height)
	: source(_source),
	  logger(_logger),
	  mainEffect(_mainEffect),
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
	  bgrxYoloxInput(make_unique_gs_texture(640, 640, GS_BGRX, 1, nullptr, GS_RENDER_TARGET)),
	  bgrxYoloxInputReader(640, 640, GS_BGRX),
	  r32fIntermediate(make_unique_gs_texture(width, height, GS_R32F, 1, nullptr, GS_RENDER_TARGET))
{
}

RenderingContext::~RenderingContext() noexcept {}

void RenderingContext::videoTick(float)
{
	canvasDetectorResults = canvasDetector.detect(bgrxYoloxInputReader.getBuffer().data(),
						      bgrxYoloxInputReader.getWidth(),
						      bgrxYoloxInputReader.getHeight());
	logger.info("tick");
	for (auto &result : canvasDetectorResults) {
		logger.info("Detected canvas: confidence={}, x={}, y={}, width={}, height={}", result.confidence,
			    result.x, result.y, result.width, result.height);
	}
}

obs_source_frame *RenderingContext::filterVideo(obs_source_frame *frame)
{
	if (frame && frame->timestamp != lastFrameTimestamp) {
		lastFrameTimestamp = frame->timestamp;
		doesNextVideoRenderReceiveNewFrame = true;
	}

	return frame;
}

void draw_line_rectangle(float x, float y, float w, float h, uint32_t color)
{
	gs_vb_data *vbd = gs_vbdata_create();
	if (!vbd)
		return;

	vbd->num = 5;
	vbd->points = static_cast<struct vec3 *>(bmalloc(sizeof(struct vec3) * 5));

	vec3_set(&vbd->points[0], x, y, 0.0f);
	vec3_set(&vbd->points[1], x + w, y, 0.0f);
	vec3_set(&vbd->points[2], x + w, y + h, 0.0f);
	vec3_set(&vbd->points[3], x, y + h, 0.0f);
	vec3_set(&vbd->points[4], x, y, 0.0f);

	gs_vertbuffer_t *vb = gs_vertexbuffer_create(vbd, GS_DYNAMIC);

	gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_SOLID);
	gs_eparam_t *param = gs_effect_get_param_by_name(effect, "color");
	struct vec4 color_vec;
	vec4_from_rgba(&color_vec, color);
	gs_effect_set_vec4(param, &color_vec);

	gs_load_vertexbuffer(vb);

	while (gs_effect_loop(effect, "Solid")) {
		gs_draw(GS_LINESTRIP, 0, 5);
	}

	gs_load_vertexbuffer(NULL);
	gs_vertexbuffer_destroy(vb);
}

// 単色の塗りつぶし四角形を描画するヘルパー関数
static void draw_filled_rect(float x, float y, float w, float h, gs_effect_t *)
{
    gs_matrix_push();
    gs_matrix_translate3f(x, y, 0.0f);
    gs_draw_quadf(NULL, 0, w, h);
    gs_matrix_pop();
}

/**
 * @brief 太さを指定して四角形の枠線を描画します。
 *
 * @param x         X座標
 * @param y         Y座標
 * @param w         全体の幅
 * @param h         全体の高さ
 * @param thickness 線の太さ
 * @param color     線の色 (RGBA)
 */
void draw_thick_line_rectangle(float x, float y, float w, float h, float thickness, uint32_t color)
{
    // --- 色とエフェクトの準備 ---
    gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_SOLID);
    gs_eparam_t *param = gs_effect_get_param_by_name(effect, "color");
    struct vec4 color_vec;
    vec4_from_rgba(&color_vec, color);
    gs_effect_set_vec4(param, &color_vec);
    // -------------------------

    // エフェクトの描画ループを開始
    while (gs_effect_loop(effect, "Solid")) {
        // 1. 上の線
        draw_filled_rect(x, y, w, thickness, effect);
        
        // 2. 下の線
        draw_filled_rect(x, y + h - thickness, w, thickness, effect);
        
        // 3. 左の線
        draw_filled_rect(x, y + thickness, thickness, h - thickness * 2.0f, effect);
        
        // 4. 右の線
        draw_filled_rect(x + w - thickness, y + thickness, thickness, h - thickness * 2.0f, effect);
    }
}

void RenderingContext::videoRender(const std::shared_ptr<const Preset> &preset)
{
	ExtractionMode extractionMode = getExtractionMode(*preset);

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

		bgrxYoloxInputReader.stage(bgrxYoloxInput.get());

		{
			MainEffectDetail::RenderTargetGuard renderTargetGuard;
			MainEffectDetail::TransformStateGuard transformStateGuard;

			double widthScale = 640.0 / static_cast<double>(width);
			double heightScale = 640.0 / static_cast<double>(height);
			double scale = std::min(widthScale, heightScale);

			double scaledWidth = static_cast<double>(width) * scale;
			double scaledHeight = static_cast<double>(height) * scale;

			double offsetX = (640.0 - scaledWidth) / 2.0;
			double offsetY = (640.0 - scaledHeight) / 2.0;

			gs_set_viewport(0.0f, 0.0f, 640.0f, 640.0f);
			gs_ortho(0.0f, 640.0f, 0.0f, 640.0f, -100.0f, 100.0f);
			gs_matrix_identity();
			gs_matrix_translate3f(offsetX, offsetY, 0.0f);

			gs_set_render_target_with_color_space(bgrxYoloxInput.get(), nullptr, GS_CS_SRGB);

			vec4 gray = {0.5f, 0.5f, 0.5f, 1.0f};
			gs_clear(GS_CLEAR_COLOR, &gray, 1.0f, 0);

			const std::size_t passes = gs_technique_begin(mainEffect.techDraw);
			for (std::size_t i = 0; i < passes; i++) {
				if (gs_technique_begin_pass(mainEffect.techDraw, i)) {
					gs_effect_set_texture(mainEffect.textureImage, bgrxSource.get());

					gs_draw_sprite(nullptr, 0, scaledWidth, scaledHeight);
					gs_technique_end_pass(mainEffect.techDraw);
				}
			}
			gs_technique_end(mainEffect.techDraw);
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

	if (!canvasDetectorResults.empty()) {
		const float model_width = bgrxYoloxInputReader.getWidth();
		const float model_height = bgrxYoloxInputReader.getHeight();

		const float scale =
			std::min(model_width / static_cast<float>(width), model_height / static_cast<float>(height));

		const float scaled_image_width = static_cast<float>(width) * scale;
		const float scaled_image_height = static_cast<float>(height) * scale;

		const float offset_x = (model_width - scaled_image_width) / 2.0f;
		const float offset_y = (model_height - scaled_image_height) / 2.0f;

		for (const auto &result : canvasDetectorResults) {
			// Transform coordinates from model space (padded) to original image space
			const float original_x = (result.x - offset_x) / scale;
			const float original_y = (result.y - offset_y) / scale;
			const float original_w = result.width / scale;
			const float original_h = result.height / scale;

			draw_thick_line_rectangle(original_x, original_y, original_w, original_h, 10.0f, 0xFF00FF00);
		}
	}
}

} // namespace ShowDraw
} // namespace KaitoTokyo

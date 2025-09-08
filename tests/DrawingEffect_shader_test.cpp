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

#include <iostream>
#include <memory>
#include <thread>

#include <obs-module.h>
#include <opencv2/opencv.hpp>

#include <obs-bridge-utils/obs-bridge-utils.hpp>

#include "BufferedTexture.hpp"
#include "DrawingEffect.hpp"

#define TEST_LIBOBS_WITH_VIDEO
#include "obs_test_environment.hpp"

using namespace kaito_tokyo::obs_bridge_utils;
using namespace kaito_tokyo::obs_showdraw;

class DrawingEffectShaderTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		graphics_context_guard guard;

		effect = make_unique_gs_effect_from_file(CMAKE_SOURCE_DIR "/data/effects/drawing.effect");
		drawingEffect = std::make_unique<DrawingEffect>(std::move(effect));
	}

	unique_gs_effect_t effect;
	std::unique_ptr<DrawingEffect> drawingEffect;
};

TEST_F(DrawingEffectShaderTest, Draw)
{
	graphics_context_guard guard;

	unique_gs_effect_t effect =
		make_unique_gs_effect_from_file(CMAKE_SOURCE_DIR "/data/effects/drawing-test.effect");

	DrawingEffect drawingEffect(std::move(effect));

	int width = 1;
	int height = 1;

	const std::vector<uint8_t> sourcePixels(width * height * 4, 128);
	const uint8_t *sourceData = sourcePixels.data();
	auto sourceTexture = make_unique_gs_texture(width, height, GS_BGRX, 1, &sourceData, GS_RENDER_TARGET | GS_DYNAMIC);

	BufferedTexture<1> targetBufferedTexture(width, height, GS_BGRX, GS_DYNAMIC | GS_RENDER_TARGET);

	gs_texture_t *defaultRenderTarget = gs_get_render_target();
	gs_set_render_target(targetBufferedTexture.getTexture(), nullptr);

	gs_viewport_push();
	gs_projection_push();
	gs_matrix_push();
	gs_blend_state_push();

	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	gs_set_viewport(0, 0, width, height);
	gs_ortho(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f);

	vec4 zero = {0.0f, 0.0f, 0.0f, 1.0f};
	gs_clear(GS_CLEAR_COLOR, &zero, 0.0f, 0);

	gs_technique_t *tech = gs_effect_get_technique(drawingEffect.effect.get(), "Draw");

	std::size_t passes = gs_technique_begin(tech);
	for (std::size_t i = 0; i < passes; i++) {
		if (gs_technique_begin_pass(tech, i)) {
			gs_eparam_t *imageParam = gs_effect_get_param_by_name(drawingEffect.effect.get(), "image");
			gs_effect_set_texture(imageParam, sourceTexture.get());
			gs_draw_sprite(sourceTexture.get(), 0, 0, 0);
			gs_technique_end_pass(tech);
		}
	}

	gs_technique_end(tech);

	gs_blend_state_pop();
	gs_matrix_pop();
	gs_projection_pop();
	gs_viewport_pop();

	gs_set_render_target(defaultRenderTarget, nullptr);

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	targetBufferedTexture.stage();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	targetBufferedTexture.sync();

	auto &targetBuffer = targetBufferedTexture.getBuffer();

	for (std::size_t i = 0; i < targetBuffer.size(); i++) {
		std::cout << "Pixel " << i << ": " << (int)targetBuffer[i] << std::endl;
	}
}

TEST_F(DrawingEffectShaderTest, ExtractLuminance)
{
	// graphics_context_guard guard;

	// drawingEffect->applyLuminanceExtractionPass(targetBufferedTexture->getTexture(), sourceTexture.get());

	// targetBufferedTexture->stage();
	// ASSERT_TRUE(targetBufferedTexture->sync());
	// ASSERT_TRUE(targetBufferedTexture->sync());

	// cv::Mat targetImage(HEIGHT, WIDTH, CV_8UC4, (void *)targetBufferedTexture->getBuffer().data(),
	// 		    targetBufferedTexture->bufferLinesize);

	// // The luminance value for red (255, 0, 0) is calculated as:
	// // luma = 0.299 * R + 0.587 * G + 0.114 * B
	// // luma = 0.299 * 255 + 0.587 * 0 + 0.114 * 0 = 76.245
	// // The shader returns (luma, luma, luma, 1.0), which corresponds to (76, 76, 76, 255) in 8-bit BGRA.
	// cv::Mat expectedImage(HEIGHT, WIDTH, CV_8UC4, cv::Scalar(76, 76, 76, 255));

	// cv::Mat diff;
	// cv::absdiff(expectedImage, targetImage, diff);
	// cv::Scalar sum = cv::sum(diff);

	// EXPECT_LE(sum[0], 255) << "B channel differs";
	// EXPECT_LE(sum[1], 255) << "G channel differs";
	// EXPECT_LE(sum[2], 255) << "R channel differs";
	// EXPECT_EQ(sum[3], 0) << "A channel differs";
}

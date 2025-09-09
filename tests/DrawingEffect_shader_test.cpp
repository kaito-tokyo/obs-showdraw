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

#include <algorithm>
#include <iostream>
#include <memory>
#include <thread>
#include <random>

#include <obs-module.h>
#include <opencv2/opencv.hpp>

#include <obs-bridge-utils/obs-bridge-utils.hpp>

#include "AsyncTextureReader.hpp"
#include "DrawingEffect.hpp"

#define TEST_LIBOBS_WITH_VIDEO
#include "obs_test_environment.hpp"

using namespace kaito_tokyo::obs_bridge_utils;
using namespace kaito_tokyo::obs_showdraw;

class DrawingEffectShaderTest : public ::testing::Test {
protected:
	unique_gs_effect_t effect;
	std::unique_ptr<DrawingEffect> drawingEffect;

	void SetUp() override
	{
		graphics_context_guard guard;
		effect = make_unique_gs_effect_from_file(CMAKE_SOURCE_DIR "/data/effects/drawing-test.effect");
		drawingEffect = std::make_unique<DrawingEffect>(std::move(effect));
	}

	void TearDown() override
	{
		graphics_context_guard guard;
		drawingEffect.reset();
		effect.reset();
		gs_unique::drain();
	}
};

TEST_F(DrawingEffectShaderTest, Draw)
{
	graphics_context_guard guard;

	int width = 1;
	int height = 1;
	gs_color_format colorFormat = GS_BGRX;

	const std::vector<uint8_t> sourcePixels(width * height * 4, 255);
	const uint8_t *sourceData = sourcePixels.data();
	auto sourceTexture = make_unique_gs_texture(width, height, colorFormat, 1, &sourceData, 0);
	auto targetTexture = make_unique_gs_texture(width, height, colorFormat, 1, nullptr, GS_RENDER_TARGET);

	gs_viewport_push();
	gs_projection_push();
	gs_matrix_push();

	gs_set_viewport(0, 0, width, height);
	gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);
	gs_matrix_identity();

	drawingEffect->drawFinalImage(width, height, targetTexture.get(), sourceTexture.get());
	gs_flush();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	gs_matrix_pop();
	gs_projection_pop();
	gs_viewport_pop();

	unique_gs_stagesurf_t stagesurf = make_unique_gs_stagesurf(width, height, colorFormat);
	gs_stage_texture(stagesurf.get(), targetTexture.get());

	uint8_t *data = nullptr;
	uint32_t linesize = 0;
	ASSERT_TRUE(gs_stagesurface_map(stagesurf.get(), &data, &linesize));

	for (std::size_t i = 0; i < height * linesize; i++) {
		ASSERT_EQ(255, data[i]) << "at byte index " << i;
	}

	gs_stagesurface_unmap(stagesurf.get());
}

TEST_F(DrawingEffectShaderTest, ExtractLuminance)
{
	graphics_context_guard guard;

	int width = 1;
	int height = 1;
	gs_color_format colorFormat = GS_RGBA;

	// Red color in RGBA format {R, G, B, A}
	const std::vector<uint8_t> sourcePixels = {255, 0, 0, 255};
	const uint8_t *sourceData = sourcePixels.data();
	auto sourceTexture = make_unique_gs_texture(width, height, colorFormat, 1, &sourceData, 0);
	auto targetTexture = make_unique_gs_texture(width, height, colorFormat, 1, nullptr, GS_RENDER_TARGET);

	gs_viewport_push();
	gs_projection_push();
	gs_matrix_push();

	gs_set_viewport(0, 0, width, height);
	gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);
	gs_matrix_identity();

	drawingEffect->applyLuminanceExtractionPass(targetTexture.get(), sourceTexture.get());
	gs_flush();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	gs_matrix_pop();
	gs_projection_pop();
	gs_viewport_pop();

	unique_gs_stagesurf_t stagesurf = make_unique_gs_stagesurf(width, height, colorFormat);
	gs_stage_texture(stagesurf.get(), targetTexture.get());

	uint8_t *data = nullptr;
	uint32_t linesize = 0;
	ASSERT_TRUE(gs_stagesurface_map(stagesurf.get(), &data, &linesize));

	// The shader calculates luma = dot(color.rgb, float3(0.2126, 0.7152, 0.0722)).
	// Assuming the texture sampler correctly maps RGBA to RGB for the shader,
	// the input color (255, 0, 0) becomes (1.0, 0.0, 0.0) in the shader.
	// luma = 0.2126 * 1.0 + 0.7152 * 0.0 + 0.0722 * 0.0 = 0.2126.
	// The output color is (luma, luma, luma, 1.0).
	// In 8-bit format, this is 0.2126 * 255 = 54.213, which is 54.
	// The output pixel in RGBA should be (54, 54, 54, 255).
	EXPECT_EQ(54, data[0]);  // Red
	EXPECT_EQ(54, data[1]);  // Green
	EXPECT_EQ(54, data[2]);  // Blue
	EXPECT_EQ(255, data[3]); // Alpha

	gs_stagesurface_unmap(stagesurf.get());
}

TEST_F(DrawingEffectShaderTest, Median9)
{
	std::mt19937_64 rng(std::random_device{}());

	graphics_context_guard guard;

	int width = 1;
	int height = 1;
	gs_color_format colorFormat = GS_BGRX;

	const std::vector<uint8_t> sourcePixels(width * height * 4, 255);
	const uint8_t *sourceData = sourcePixels.data();
	auto sourceTexture = make_unique_gs_texture(width, height, colorFormat, 1, &sourceData, 0);
	auto targetTexture = make_unique_gs_texture(width, height, colorFormat, 1, nullptr, GS_RENDER_TARGET);

	gs_viewport_push();
	gs_projection_push();
	gs_matrix_push();

	gs_set_viewport(0, 0, width, height);
	gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);
	gs_matrix_identity();

	auto &effect = drawingEffect->effect;

	std::vector<float> sequence{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9};
	std::shuffle(sequence.begin(), sequence.end(), rng);

	gs_set_render_target(targetTexture.get(), nullptr);

	vec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
	gs_clear(GS_CLEAR_COLOR, &clearColor, 1.0f, 0);

	gs_eparam_t *floatV0 = gs_effect_get_param_by_name(effect.get(), "v0");
	gs_eparam_t *floatV1 = gs_effect_get_param_by_name(effect.get(), "v1");
	gs_eparam_t *floatV2 = gs_effect_get_param_by_name(effect.get(), "v2");
	gs_eparam_t *floatV3 = gs_effect_get_param_by_name(effect.get(), "v3");
	gs_eparam_t *floatV4 = gs_effect_get_param_by_name(effect.get(), "v4");
	gs_eparam_t *floatV5 = gs_effect_get_param_by_name(effect.get(), "v5");
	gs_eparam_t *floatV6 = gs_effect_get_param_by_name(effect.get(), "v6");
	gs_eparam_t *floatV7 = gs_effect_get_param_by_name(effect.get(), "v7");
	gs_eparam_t *floatV8 = gs_effect_get_param_by_name(effect.get(), "v8");

	gs_effect_set_float(floatV0, sequence[0]);
	gs_effect_set_float(floatV1, sequence[1]);
	gs_effect_set_float(floatV2, sequence[2]);
	gs_effect_set_float(floatV3, sequence[3]);
	gs_effect_set_float(floatV4, sequence[4]);
	gs_effect_set_float(floatV5, sequence[5]);
	gs_effect_set_float(floatV6, sequence[6]);
	gs_effect_set_float(floatV7, sequence[7]);
	gs_effect_set_float(floatV8, sequence[8]);

	while (gs_effect_loop(effect.get(), "TestMedian9")) {
		gs_effect_set_texture(drawingEffect->textureImage, sourceTexture.get());
		gs_draw_sprite(sourceTexture.get(), 0, width, height);
	}

	gs_flush();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	gs_matrix_pop();
	gs_projection_pop();
	gs_viewport_pop();

	unique_gs_stagesurf_t stagesurf = make_unique_gs_stagesurf(width, height, colorFormat);
	gs_stage_texture(stagesurf.get(), targetTexture.get());

	uint8_t *data = nullptr;
	uint32_t linesize = 0;
	ASSERT_TRUE(gs_stagesurface_map(stagesurf.get(), &data, &linesize));

	for (std::size_t i = 0; i < height * linesize; i++) {
		std::cout << "data[" << i << "] = " << static_cast<int>(data[i]) << std::endl;
	}
	ASSERT_EQ(128, data[0]) << "at byte index 0";

	gs_stagesurface_unmap(stagesurf.get());
}

TEST_F(DrawingEffectShaderTest, Median3)
{
	std::mt19937_64 rng(std::random_device{}());

	graphics_context_guard guard;

	int width = 1;
	int height = 1;
	gs_color_format colorFormat = GS_BGRX;

	const std::vector<uint8_t> sourcePixels(width * height * 4, 255);
	const uint8_t *sourceData = sourcePixels.data();
	auto sourceTexture = make_unique_gs_texture(width, height, colorFormat, 1, &sourceData, 0);
	auto targetTexture = make_unique_gs_texture(width, height, colorFormat, 1, nullptr, GS_RENDER_TARGET);

	gs_viewport_push();
	gs_projection_push();
	gs_matrix_push();

	gs_set_viewport(0, 0, width, height);
	gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);
	gs_matrix_identity();

	auto &effect = drawingEffect->effect;

	std::vector<float> sequence{0.1, 0.5, 0.9};
	std::shuffle(sequence.begin(), sequence.end(), rng);

	gs_set_render_target(targetTexture.get(), nullptr);

	vec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
	gs_clear(GS_CLEAR_COLOR, &clearColor, 1.0f, 0);

	gs_eparam_t *floatV0 = gs_effect_get_param_by_name(effect.get(), "v0");
	gs_eparam_t *floatV1 = gs_effect_get_param_by_name(effect.get(), "v1");
	gs_eparam_t *floatV2 = gs_effect_get_param_by_name(effect.get(), "v2");

	gs_effect_set_float(floatV0, sequence[0]);
	gs_effect_set_float(floatV1, sequence[1]);
	gs_effect_set_float(floatV2, sequence[2]);

	while (gs_effect_loop(effect.get(), "TestMedian3")) {
		gs_effect_set_texture(drawingEffect->textureImage, sourceTexture.get());
		gs_draw_sprite(sourceTexture.get(), 0, width, height);
	}

	gs_flush();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	gs_matrix_pop();
	gs_projection_pop();
	gs_viewport_pop();

	unique_gs_stagesurf_t stagesurf = make_unique_gs_stagesurf(width, height, colorFormat);
	gs_stage_texture(stagesurf.get(), targetTexture.get());

	uint8_t *data = nullptr;
	uint32_t linesize = 0;
	ASSERT_TRUE(gs_stagesurface_map(stagesurf.get(), &data, &linesize));

	for (std::size_t i = 0; i < height * linesize; i++) {
		std::cout << "data[" << i << "] = " << static_cast<int>(data[i]) << std::endl;
	}
	ASSERT_EQ(128, data[0]) << "at byte index 0";

	gs_stagesurface_unmap(stagesurf.get());
}

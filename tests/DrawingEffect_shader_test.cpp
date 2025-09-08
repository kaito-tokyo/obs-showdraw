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
		effect = make_unique_gs_effect_from_file(CMAKE_SOURCE_DIR "/data/effects/drawing.effect");
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

	unique_gs_effect_t effect =
		make_unique_gs_effect_from_file(CMAKE_SOURCE_DIR "/data/effects/drawing-test.effect");

	DrawingEffect drawingEffect(std::move(effect));

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

	drawingEffect.drawFinalImage(width, height, targetTexture.get(), sourceTexture.get());

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

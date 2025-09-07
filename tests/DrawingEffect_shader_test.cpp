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

#include <obs-module.h>
#include <opencv2/opencv.hpp>

#include <obs-bridge-utils/obs-bridge-utils.hpp>

#include "BufferedTexture.hpp"
#include "DrawingEffect.hpp"

#define TEST_LIBOBS_WITH_VIDEO
#include "obs_test_environment.hpp"

using namespace kaito_tokyo::obs_bridge_utils;
using namespace kaito_tokyo::obs_showdraw;

TEST(DrawingEffectShaderTest, Draw)
{
	cv::Mat loaded_image = cv::imread(CMAKE_SOURCE_DIR "/tests/fixtures/red.png", cv::IMREAD_UNCHANGED);
	ASSERT_FALSE(loaded_image.empty()) << "Failed to load source image";

	cv::Mat source_image;
	if (loaded_image.channels() == 3) {
		cv::cvtColor(loaded_image, source_image, cv::COLOR_BGR2BGRA);
	} else {
		source_image = loaded_image;
	}
	cv::resize(source_image, source_image, cv::Size(WIDTH, HEIGHT));

	graphics_context_guard guard;
	std::cout << "Loading shader from: " << CMAKE_SOURCE_DIR "/data/effects/drawing.effect" << std::endl;
	auto effect = make_unique_gs_effect_from_file(CMAKE_SOURCE_DIR "/data/effects/drawing.effect");
	ASSERT_NE(effect, nullptr);

	DrawingEffect drawingEffect(std::move(effect));

	// Create source texture
	const uint8_t *source_data[] = {source_image.data};
	kaito_tokyo::obs_bridge_utils::unique_gs_texture_t source_texture{
		gs_texture_create(WIDTH, HEIGHT, GS_BGRA, 1, source_data, 0)};
	ASSERT_NE(source_texture, nullptr);

	kaito_tokyo::obs_showdraw::BufferedTexture render_target(WIDTH, HEIGHT);

	drawingEffect.drawFinalImage(render_target.getTexture(), source_texture.get());

	render_target.stage();
	ASSERT_TRUE(render_target.sync());

	const auto &rendered_buffer = render_target.getBuffer();
	cv::Mat rendered_image(HEIGHT, WIDTH, CV_8UC4, (void *)rendered_buffer.data(),
			       render_target.bufferLinesize);

	cv::Mat diff;
	cv::absdiff(rendered_image, source_image, diff);
	cv::Scalar sum = cv::sum(diff);

	EXPECT_EQ(sum[0], 0) << "B channel differs";
	EXPECT_EQ(sum[1], 0) << "G channel differs";
	EXPECT_EQ(sum[2], 0) << "R channel differs";
	EXPECT_EQ(sum[3], 0) << "A channel differs";
}

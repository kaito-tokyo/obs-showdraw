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

TEST(DrawingEffectShaderTest, Draw)
{
    cv::Scalar redColor(0, 0, 255, 255);
	cv::Mat sourceImage(HEIGHT, WIDTH, CV_8UC4, redColor);

	graphics_context_guard guard;
	auto effect = make_unique_gs_effect_from_file(CMAKE_SOURCE_DIR "/data/effects/drawing.effect");

	DrawingEffect drawingEffect(std::move(effect));

	const uint8_t *data = sourceImage.data;
	auto sourceTexture = make_unique_gs_texture(WIDTH, HEIGHT, GS_BGRA, 1, &data, 0);

	kaito_tokyo::obs_showdraw::BufferedTexture targetBufferedTexture(WIDTH, HEIGHT);

	drawingEffect.drawFinalImage(targetBufferedTexture.getTexture(), sourceTexture.get());

	targetBufferedTexture.stage();
	ASSERT_TRUE(targetBufferedTexture.sync());
	ASSERT_TRUE(targetBufferedTexture.sync());

	cv::Mat targetImage(HEIGHT, WIDTH, CV_8UC4, (void *)targetBufferedTexture.getBuffer().data(),
			       targetBufferedTexture.bufferLinesize);

	cv::Mat diff;
	cv::absdiff(sourceImage, targetImage, diff);
	cv::Scalar sum = cv::sum(diff);

	EXPECT_EQ(sum[0], 0) << "B channel differs";
	EXPECT_EQ(sum[1], 0) << "G channel differs";
	EXPECT_EQ(sum[2], 0) << "R channel differs";
	EXPECT_EQ(sum[3], 0) << "A channel differs";
}

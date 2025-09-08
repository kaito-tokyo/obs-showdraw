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

		const uint8_t *data = sourceImage.data;
		sourceTexture = make_unique_gs_texture(WIDTH, HEIGHT, GS_BGRA, 1, &data, 0);

		targetBufferedTexture = std::make_unique<kaito_tokyo::obs_showdraw::BufferedTexture>(WIDTH, HEIGHT);
	}

	cv::Mat sourceImage = cv::Mat(HEIGHT, WIDTH, CV_8UC4, cv::Scalar(0, 0, 255, 255));

	unique_gs_effect_t effect;
	std::unique_ptr<DrawingEffect> drawingEffect;
	unique_gs_texture_t sourceTexture;
	std::unique_ptr<kaito_tokyo::obs_showdraw::BufferedTexture> targetBufferedTexture;
};

TEST_F(DrawingEffectShaderTest, Draw)
{
	graphics_context_guard guard;

	drawingEffect->drawFinalImage(targetBufferedTexture->getTexture(), sourceTexture.get());

	targetBufferedTexture->stage();
	ASSERT_TRUE(targetBufferedTexture->sync());
	ASSERT_TRUE(targetBufferedTexture->sync());

	cv::Mat targetImage(HEIGHT, WIDTH, CV_8UC4, (void *)targetBufferedTexture->getBuffer().data(),
			    targetBufferedTexture->bufferLinesize);

	cv::Mat diff;
	cv::absdiff(sourceImage, targetImage, diff);
	cv::Scalar sum = cv::sum(diff);

	EXPECT_EQ(sum[0], 0) << "B channel differs";
	EXPECT_EQ(sum[1], 0) << "G channel differs";
	EXPECT_EQ(sum[2], 0) << "R channel differs";
	EXPECT_EQ(sum[3], 0) << "A channel differs";
}

TEST_F(DrawingEffectShaderTest, ExtractLuminance)
{
	graphics_context_guard guard;

	drawingEffect->applyLuminanceExtractionPass(targetBufferedTexture->getTexture(), sourceTexture.get());

	targetBufferedTexture->stage();
	ASSERT_TRUE(targetBufferedTexture->sync());
	ASSERT_TRUE(targetBufferedTexture->sync());

	cv::Mat targetImage(HEIGHT, WIDTH, CV_8UC4, (void *)targetBufferedTexture->getBuffer().data(),
			    targetBufferedTexture->bufferLinesize);

	// The luminance value for red (255, 0, 0) is calculated as:
	// luma = 0.299 * R + 0.587 * G + 0.114 * B
	// luma = 0.299 * 255 + 0.587 * 0 + 0.114 * 0 = 76.245
	// The shader returns (luma, luma, luma, 1.0), which corresponds to (76, 76, 76, 255) in 8-bit BGRA.
	cv::Mat expectedImage(HEIGHT, WIDTH, CV_8UC4, cv::Scalar(76, 76, 76, 255));

	cv::Mat diff;
	cv::absdiff(expectedImage, targetImage, diff);
	cv::Scalar sum = cv::sum(diff);

	EXPECT_LE(sum[0], 255) << "B channel differs";
	EXPECT_LE(sum[1], 255) << "G channel differs";
	EXPECT_LE(sum[2], 255) << "R channel differs";
	EXPECT_EQ(sum[3], 0) << "A channel differs";
}

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

#include <gtest/gtest.h>
#include "obs_test_environment.hpp"

#include <obs.h>
#include <obs-module.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

#include "obs-bridge-utils/gs_unique.hpp"

#ifndef CMAKE_SOURCE_DIR
#define CMAKE_SOURCE_DIR "."
#endif

::testing::Environment *const obs_env = ::testing::AddGlobalTestEnvironment(new kaito_tokyo::obs_showdraw_testing::ObsTestEnvironment());

TEST(DrawingEffectShaderTest, Draw)
{
	cv::Mat expectedImage = cv::imread(CMAKE_SOURCE_DIR "/tests/fixtures/red.png", cv::IMREAD_UNCHANGED);
	std::cout << "size: " << expectedImage.cols << "x" << expectedImage.rows << std::endl;
	ASSERT_FALSE(expectedImage.empty()) << "Failed to load expected image";

	// // Load shader
	// char *effect_path = obs_module_file("drawing.effect");
	// if (!effect_path) {
	// 	FAIL() << "Could not find drawing.effect";
	// }
	// kaito_tokyo::obs_bridge_utils::unique_gs_effect_t effect{
	// 	gs_effect_create_from_file(effect_path, nullptr)};
	// bfree(effect_path);
	// ASSERT_NE(effect, nullptr);

	// // Create source texture
	// kaito_tokyo::obs_bridge_utils::unique_gs_texture_t source_texture{
	// 	gs_texture_create(TEXTURE_WIDTH, TEXTURE_HEIGHT, GS_RGBA, 1, nullptr, GS_DYNAMIC)};
	// ASSERT_NE(source_texture, nullptr);

	// // Create render target
	// kaito_tokyo::obs_bridge_utils::unique_gs_texture_t render_target{
	// 	gs_texture_create(TEXTURE_WIDTH, TEXTURE_HEIGHT, GS_RGBA, 1, nullptr, GS_RENDER_TARGET)};
	// ASSERT_NE(render_target, nullptr);

	// // Render
	// gs_set_render_target(render_target.get(), nullptr);
	// gs_ortho(0.0f, (float)TEXTURE_WIDTH, 0.0f, (float)TEXTURE_HEIGHT, -100.0f, 100.0f);
	// gs_clear(GS_CLEAR_COLOR, nullptr, 0.0f, 0.0f);

	// gs_technique_t *tech = gs_effect_get_technique(effect.get(), "Draw");
	// gs_effect_set_color(gs_effect_get_param_by_name(effect.get(), "color"), 0xFFFF0000);

	// gs_effect_set_texture(gs_effect_get_param_by_name(effect.get(), "image"), source_texture.get());

	// size_t passes = gs_technique_begin(tech);
	// for (size_t i = 0; i < passes; i++) {
	// 	gs_technique_begin_pass(tech, i);
	// 	gs_draw_sprite(source_texture.get(), 0, TEXTURE_WIDTH, TEXTURE_HEIGHT);
	// 	gs_technique_end_pass(tech);
	// }
	// gs_technique_end(tech);

	// // Read back rendered image
	// kaito_tokyo::obs_bridge_utils::unique_gs_stagesurf_t stagesurf{
	// 	gs_stagesurface_create(TEXTURE_WIDTH, TEXTURE_HEIGHT, GS_RGBA)};
	// ASSERT_NE(stagesurf, nullptr);
	// gs_stage_texture(stagesurf.get(), render_target.get());

	// uint8_t *data = nullptr;
	// uint32_t linesize = 0;
	// ASSERT_TRUE(gs_stagesurface_map(stagesurf.get(), &data, &linesize));
	// ASSERT_NE(data, nullptr);

	// cv::Mat rendered_image(TEXTURE_HEIGHT, TEXTURE_WIDTH, CV_8UC4, data, linesize);
	
	// // OpenCV uses BGRA, but OBS renders RGBA. So we need to convert.
	// cv::cvtColor(rendered_image, rendered_image, cv::COLOR_RGBA2BGRA);

	// gs_stagesurface_unmap(stagesurf.get());

	// Load reference image
	// char *fixture_path_str = obs_module_file(FIXTURE_PATH.c_str());
	// cv::Mat reference_image = cv::imread(fixture_path_str, cv::IMREAD_UNCHANGED);
    // bfree(fixture_path_str);
	// ASSERT_FALSE(reference_image.empty()) << "Failed to load reference image: " << FIXTURE_PATH << ". Please create a 100x100 red PNG image at tests/fixtures/red.png";

	// // Compare images
	// cv::Mat diff;
	// cv::absdiff(rendered_image, reference_image, diff);
	// cv::Scalar sum = cv::sum(diff);

	// EXPECT_EQ(sum[0], 0) << "B channel differs";
	// EXPECT_EQ(sum[1], 0) << "G channel differs";
	// EXPECT_EQ(sum[2], 0) << "R channel differs";
	// EXPECT_EQ(sum[3], 0) << "A channel differs";
}

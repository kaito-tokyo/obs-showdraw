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

#include "AsyncTextureReader.hpp"

#include <string>
#include <thread>

#include <opencv2/opencv.hpp>

#define TEST_LIBOBS_WITH_VIDEO
#include "obs_test_environment.hpp"

using namespace kaito_tokyo::obs_bridge_utils;

namespace kaito_tokyo::obs_showdraw {

TEST(AsyncTextureReader, getBytesPerPixel)
{
	EXPECT_EQ(detail::getBytesPerPixel(GS_A8), 1);
	EXPECT_EQ(detail::getBytesPerPixel(GS_R8), 1);
	EXPECT_EQ(detail::getBytesPerPixel(GS_R8G8), 2);
	EXPECT_EQ(detail::getBytesPerPixel(GS_RG16), 2);
	EXPECT_EQ(detail::getBytesPerPixel(GS_RGBA), 4);
	EXPECT_EQ(detail::getBytesPerPixel(GS_BGRX), 4);
	EXPECT_EQ(detail::getBytesPerPixel(GS_BGRA), 4);
	EXPECT_EQ(detail::getBytesPerPixel(GS_R16), 4);
	EXPECT_EQ(detail::getBytesPerPixel(GS_R16F), 4);
	EXPECT_EQ(detail::getBytesPerPixel(GS_R10G10B10A2), 8);
	EXPECT_EQ(detail::getBytesPerPixel(GS_RGBA16), 8);
	EXPECT_EQ(detail::getBytesPerPixel(GS_RGBA16F), 8);
	EXPECT_EQ(detail::getBytesPerPixel(GS_RGBA32F), 16);
	EXPECT_THROW(detail::getBytesPerPixel(GS_UNKNOWN), std::runtime_error);
}

TEST(AsyncTextureReader, sync2)
{
	obs_enter_graphics();

	constexpr std::uint32_t WIDTH = 1;
	constexpr std::uint32_t HEIGHT = 1;
	constexpr gs_color_format FORMAT = GS_BGRX;

	std::vector<uint8_t> fixturePixels{10, 20, 30, 255};
	const std::uint8_t *fixtureData = fixturePixels.data();
	gs_texture_t *fixtureTexture = gs_texture_create(WIDTH, HEIGHT, FORMAT, 1, &fixtureData, 0);

	gs_stagesurf_t *stagesurf = gs_stagesurface_create(WIDTH, HEIGHT, FORMAT);
	gs_stage_texture(stagesurf, fixtureTexture);

	uint8_t *data = nullptr;
	uint32_t linesize = 0;
	if (!gs_stagesurface_map(stagesurf, &data, &linesize)) {
		FAIL() << "gs_stagesurface_map failed";
	}

	for (std::size_t i = 0; i < fixturePixels.size(); i++) {
		std::cout << "data[" << i << "] = " << static_cast<int>(data[i]) << std::endl;
	}

	gs_stagesurface_unmap(stagesurf);

	gs_stagesurface_destroy(stagesurf);

	AsyncTextureReader<1> asyncTextureReader(WIDTH, HEIGHT, FORMAT);
	asyncTextureReader.stage(fixtureTexture);
	asyncTextureReader.sync();
	auto &buffer = asyncTextureReader.getBuffer();
	ASSERT_EQ(buffer.size(), fixturePixels.size());
	for (std::size_t i = 0; i < fixturePixels.size(); i++) {
		EXPECT_EQ(buffer[i], fixturePixels[i]) << " at index " << i;
	}

	gs_texture_destroy(fixtureTexture);

	obs_leave_graphics();
}

// TEST(BufferedTextureTest, sync)
// {
// graphics_context_guard guard;

// constexpr std::uint32_t WIDTH = 10;
// constexpr std::uint32_t HEIGHT = 10;
// constexpr gs_color_format FORMAT = GS_BGRX;

// std::vector<uint8_t> redPixel(WIDTH * HEIGHT * 4, 128);
// const std::uint8_t *redPixelData[1];
// redPixelData[0] = redPixel.data();
// auto sourceTexture = obs_bridge_utils::make_unique_gs_texture(WIDTH, HEIGHT, FORMAT, 1, redPixelData, GS_DYNAMIC);

// uint8_t *sourceTextureData;
// uint32_t sourceTextureLinesize;
// ASSERT_TRUE(gs_texture_map(sourceTexture.get(), &sourceTextureData, &sourceTextureLinesize));
// for (std::size_t i = 0; i < redPixel.size(); i++) {
// 	std::cout << "sourceTextureData[" << i << "] = " << static_cast<int>(sourceTextureData[i]) << std::endl;
// 	sourceTextureData[i] = redPixel[i];
// }
// gs_texture_unmap(sourceTexture.get());

// ASSERT_TRUE(gs_texture_map(sourceTexture.get(), &sourceTextureData, &sourceTextureLinesize));
// for (std::size_t i = 0; i < redPixel.size(); i++) {
// 	std::cout << "sourceTextureData[" << i << "] = " << static_cast<int>(sourceTextureData[i]) << std::endl;
// }
// gs_texture_unmap(sourceTexture.get());

// BufferedTexture<1> bufferedTexture(WIDTH, HEIGHT, FORMAT);
// bufferedTexture.stage(sourceTexture.get());
// bufferedTexture.sync();
// auto &buffer = bufferedTexture.getBuffer();
// ASSERT_EQ(buffer.size(), redPixel.size());
// for (std::size_t i = 0; i < redPixel.size(); i++)
// {
// 	EXPECT_EQ(buffer[i], redPixel[i]) << " at index " << i;
// }
// }

} // namespace kaito_tokyo::obs_showdraw

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

TEST(AsyncTextureReader, sync1)
{
	graphics_context_guard guard;

	constexpr std::uint32_t WIDTH = 1;
	constexpr std::uint32_t HEIGHT = 1;
	constexpr gs_color_format FORMAT = GS_BGRX;

	std::vector<uint8_t> sourcePixels{10, 20, 30, 255};
	const std::uint8_t *sourceData = sourcePixels.data();
	unique_gs_texture_t sourceTexture =
		obs_bridge_utils::make_unique_gs_texture(WIDTH, HEIGHT, FORMAT, 1, &sourceData, 0);

	AsyncTextureReader<1> asyncTextureReader(WIDTH, HEIGHT, FORMAT);
	asyncTextureReader.stage(sourceTexture.get());
	asyncTextureReader.sync();
	auto &buffer = asyncTextureReader.getBuffer();
	ASSERT_EQ(buffer.size(), sourcePixels.size());
	for (std::size_t i = 0; i < sourcePixels.size(); i++) {
		EXPECT_EQ(buffer[i], sourcePixels[i]) << " at index " << i;
	}
}

TEST(AsyncTextureReader, sync2)
{
	graphics_context_guard guard;

	constexpr std::uint32_t WIDTH = 1;
	constexpr std::uint32_t HEIGHT = 1;
	constexpr gs_color_format FORMAT = GS_BGRX;

	std::vector<uint8_t> sourcePixels{10, 20, 30, 255};
	const std::uint8_t *sourceData = sourcePixels.data();
	unique_gs_texture_t sourceTexture =
		obs_bridge_utils::make_unique_gs_texture(WIDTH, HEIGHT, FORMAT, 1, &sourceData, 0);

	AsyncTextureReader<2> asyncTextureReader(WIDTH, HEIGHT, FORMAT);
	asyncTextureReader.stage(sourceTexture.get());
	asyncTextureReader.stage(sourceTexture.get());
	asyncTextureReader.sync();
	auto &buffer = asyncTextureReader.getBuffer();
	ASSERT_EQ(buffer.size(), sourcePixels.size());
	for (std::size_t i = 0; i < sourcePixels.size(); i++) {
		EXPECT_EQ(buffer[i], sourcePixels[i]) << " at index " << i;
	}
}

} // namespace kaito_tokyo::obs_showdraw

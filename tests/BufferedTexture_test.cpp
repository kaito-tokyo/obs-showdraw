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

#include "BufferedTexture.hpp"

#define TEST_LIBOBS_ONLY
#include "obs_test_environment.hpp"

namespace kaito_tokyo::obs_showdraw {

TEST(BufferedTextureTest, getBytesPerPixel)
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

TEST(BufferedTextureTest, sync)
{
	constexpr std::uint32_t WIDTH = 16;
	constexpr std::uint32_t HEIGHT = 16;
	constexpr gs_color_format FORMAT = GS_BGRA;

	auto sourceTexture = obs_bridge_utils::make_unique_gs_texture(WIDTH, HEIGHT, FORMAT, 1, nullptr, GS_DYNAMIC);

	gs_load_texture(sourceTexture.get(), red_png_path.c_str());

	BufferedTexture<2> bufferedTexture(WIDTH, HEIGHT, FORMAT);
	bufferedTexture.stage(sourceTexture.get());
	bufferedTexture.sync();

	const auto &buffer = bufferedTexture.getBuffer();
	ASSERT_FALSE(buffer.empty());

	// TODO: Check buffer content
}

} // namespace kaito_tokyo::obs_showdraw

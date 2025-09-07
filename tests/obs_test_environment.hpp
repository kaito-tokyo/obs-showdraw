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

#pragma once

#include <gtest/gtest.h>
#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

constexpr int FPS = 30;
constexpr int WIDTH = 640;
constexpr int HEIGHT = 480;

namespace kaito_tokyo {
namespace obs_showdraw_testing {

class ObsTestEnvironment : public ::testing::Environment {
public:
	void SetUp() override
	{
		if (!obs_startup("en-US", nullptr, nullptr)) {
			FAIL() << "OBS startup failed.";
		}
	}

	void TearDown() override { obs_shutdown(); }
};

class ObsTestWithVideoEnvironment : public ::testing::Environment {
public:
	void SetUp() override
	{
		if (!obs_startup("en-US", nullptr, nullptr)) {
			FAIL() << "OBS startup failed.";
		}

		obs_video_info ovi;
		ovi.adapter = 0;
#if defined(_WIN32)
		ovi.graphics_module = "libobs-d3d11.dll";
#elif defined(__APPLE__)
		ovi.graphics_module = "libobs-opengl.dylib";
#else
		ovi.graphics_module = "libobs-opengl.so";
#endif
		ovi.output_format = VIDEO_FORMAT_BGRA;
		ovi.fps_num = FPS;
		ovi.fps_den = 1;
		ovi.base_width = WIDTH;
		ovi.base_height = HEIGHT;
		ovi.output_width = WIDTH;
		ovi.output_height = HEIGHT;
		ovi.colorspace = VIDEO_CS_709;
		ovi.range = VIDEO_RANGE_FULL;
		if (obs_reset_video(&ovi) != OBS_VIDEO_SUCCESS) {
			FAIL() << "obs_reset_video failed";
		}
	}

	void TearDown() override { obs_shutdown(); }
};

} // namespace obs_showdraw_testing
} // namespace kaito_tokyo

#if defined(TEST_LIBOBS_ONLY)
::testing::Environment *const obs_env =
	::testing::AddGlobalTestEnvironment(new kaito_tokyo::obs_showdraw_testing::ObsTestEnvironment());
#elif defined(TEST_LIBOBS_WITH_VIDEO)
::testing::Environment *const obs_env =
	::testing::AddGlobalTestEnvironment(new kaito_tokyo::obs_showdraw_testing::ObsTestWithVideoEnvironment());
#endif

#ifndef CMAKE_SOURCE_DIR
#define CMAKE_SOURCE_DIR "."
#endif

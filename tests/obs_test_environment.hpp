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

#if defined(_WIN32)
#include <windows.h>
#endif

#include <gtest/gtest.h>
#include <obs-module.h>

#include <obs-bridge-utils/obs-bridge-utils.hpp>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

constexpr int FPS = 30;
constexpr int WIDTH = 640;
constexpr int HEIGHT = 480;

using namespace kaito_tokyo::obs_bridge_utils;

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

	void TearDown() override
	{
		obs_shutdown();
		ASSERT_EQ(0, bnum_allocs()) << "Memory leak detected: " << bnum_allocs();
	}
};

class ObsTestWithVideoEnvironment : public ::testing::Environment {
public:
	void SetUp() override
	{
#if defined(_WIN32)
		const char *data_path_env = "%ProgramFiles%\\obs-studio\\data";
		char data_path[MAX_PATH];
		ExpandEnvironmentStringsA(data_path_env, data_path, MAX_PATH);
#elif defined(__APPLE__)
		const char *data_path = nullptr;
#else
		const char *data_path = nullptr;
#endif
		if (!obs_startup("en-US", data_path, nullptr)) {
			FAIL() << "OBS startup failed.";
		}

		obs_load_all_modules();

		obs_video_info ovi;
#if defined(_WIN32)
		ovi.graphics_module = "libobs-d3d11.dll";
#elif defined(__APPLE__)
		ovi.graphics_module = "libobs-opengl.dylib";
#else
		ovi.graphics_module = "libobs-opengl.so";
#endif

		ovi.fps_num = FPS;
		ovi.fps_den = 1;

		ovi.base_width = WIDTH;
		ovi.base_height = HEIGHT;

		ovi.output_width = WIDTH;
		ovi.output_height = HEIGHT;
		ovi.output_format = VIDEO_FORMAT_BGRA;

		ovi.adapter = 0;

		ovi.gpu_conversion = true;

		ovi.colorspace = VIDEO_CS_709;
		ovi.range = VIDEO_RANGE_FULL;

		ovi.scale_type = OBS_SCALE_DISABLE;

		if (obs_reset_video(&ovi) != OBS_VIDEO_SUCCESS) {
			FAIL() << "obs_reset_video failed";
		}
	}

	void TearDown() override
	{
		{
			graphics_context_guard guard;
			gs_unique::drain();
		}
		obs_shutdown();
		ASSERT_EQ(0, bnum_allocs()) << "Memory leak detected: " << bnum_allocs();
	}
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

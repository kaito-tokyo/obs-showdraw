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
#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

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
		ovi.graphics_module = "libobs-opengl.dylib";
		ovi.output_format = VIDEO_FORMAT_RGBA;
		ovi.fps_num = 30;
		ovi.fps_den = 1;
		ovi.base_width = 640;
		ovi.base_height = 480;
		ovi.output_width = 640;
		ovi.output_height = 480;
		if (obs_reset_video(&ovi) != OBS_VIDEO_SUCCESS) {
			FAIL() << "obs_reset_video failed";
		}
	}

	void TearDown() override { obs_shutdown(); }
};

} // namespace obs_showdraw_testing
} // namespace kaito_tokyo

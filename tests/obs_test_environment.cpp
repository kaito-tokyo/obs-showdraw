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

class ObsTestEnvironment : public ::testing::Environment {
public:
	void SetUp() override
	{
		// Initialize OBS
		if (!obs_startup("en-US", nullptr, nullptr)) {
			FAIL() << "OBS startup failed.";
		}
	}

	void TearDown() override
	{
		// Shutdown OBS
		obs_shutdown();
	}
};

// Register the environment
// This will be called before main() and before any tests are run.
// The return value of AddGlobalTestEnvironment is ignored.
::testing::Environment *const obs_env = ::testing::AddGlobalTestEnvironment(new ObsTestEnvironment());

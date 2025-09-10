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

#include <thread>
#include <chrono>

#include <obs-bridge-utils/obs-bridge-utils.hpp>

#include "ShowDrawFilterContext.h"
#include "UpdateChecker.hpp"

#define TEST_LIBOBS_ONLY
#include "obs_test_environment.hpp"

using kaito_tokyo::obs_bridge_utils::unique_obs_data_t;

using kaito_tokyo::obs_showdraw::ShowDrawFilterContext;
using kaito_tokyo::obs_showdraw::LatestVersion;

TEST(ShowDrawFilterContextTest, GetLatestVersion)
{
	ShowDrawFilterContext context(nullptr, nullptr);
	ASSERT_EQ(context.getLatestVersion(), std::nullopt);

	std::this_thread::sleep_for(std::chrono::seconds(1));
	auto latestVersion = context.getLatestVersion();
	ASSERT_TRUE(latestVersion.has_value());
	ASSERT_TRUE(latestVersion->isUpdateAvailable("0.0.0"));
	ASSERT_FALSE(latestVersion->isUpdateAvailable("999.999.999"));
	ASSERT_FALSE(latestVersion->toString().empty());
}

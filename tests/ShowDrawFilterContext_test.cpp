#include "gtest/gtest.h"
#include "ShowDrawFilterContext.h"
#include "UpdateChecker.hpp" // For LatestVersion

#include <thread>
#include <chrono>

#include "obs-bridge-utils/obs-bridge-utils.hpp"

using kaito_tokyo::obs_bridge_utils::unique_obs_data_t;

using kaito_tokyo::obs_showdraw::ShowDrawFilterContext;
using kaito_tokyo::obs_showdraw::LatestVersion;

TEST(ShowDrawFilterContextTest, GetLatestVersion)
{
	ShowDrawFilterContext context(nullptr, nullptr);
	context.afterCreate();
	ASSERT_EQ(context.getLatestVersion(), std::nullopt);

	std::this_thread::sleep_for(std::chrono::seconds(1));
	auto latestVersion = context.getLatestVersion();
	ASSERT_TRUE(latestVersion.has_value());
	ASSERT_TRUE(latestVersion->isUpdateAvailable("0.0.0"));
	ASSERT_FALSE(latestVersion->isUpdateAvailable("999.999.999"));
	ASSERT_FALSE(latestVersion->toString().empty());
}

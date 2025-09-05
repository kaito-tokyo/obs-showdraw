#include "gtest/gtest.h"
#include "ShowDrawFilterContext.h"
#include "UpdateChecker.hpp" // For LatestVersion

#include <thread>
#include <chrono>

using kaito_tokyo::obs_showdraw::ShowDrawFilterContext;
using kaito_tokyo::obs_showdraw::LatestVersion;

// Mock obs_data_t and obs_source_t for testing purposes
// These are minimal mocks just to allow ShowDrawFilterContext to be constructed
struct obs_data_t {};
struct obs_source_t {};

TEST(ShowDrawFilterContextTest, GetLatestVersionAsync)
{
	// Create mock objects for constructor
	obs_data_t settings;
	obs_source_t source;

	// Create an instance of ShowDrawFilterContext
	auto context = std::make_shared<ShowDrawFilterContext>(&settings, &source);

	// Call afterCreate to start the async operation
	context->afterCreate();

	// Initially, the version should not be available
	std::optional<LatestVersion> initialVersion = context->getLatestVersion();
	ASSERT_FALSE(initialVersion.has_value());

	// Wait for the async operation to complete (adjust time as needed for actual network call)
	// In a real test, you might mock UpdateChecker::fetch() to control its return value and speed.
	std::this_thread::sleep_for(std::chrono::seconds(5)); // Give it some time

	// After waiting, the version should be available
	std::optional<LatestVersion> latestVersion = context->getLatestVersion();
	ASSERT_TRUE(latestVersion.has_value());
	// You might want to assert the actual version string if UpdateChecker::fetch() is mocked
	// ASSERT_EQ(latestVersion->toString(), "expected_version_string");
}

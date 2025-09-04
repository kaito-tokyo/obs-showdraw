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

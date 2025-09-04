#include <gtest/gtest.h>

#include "Preset.hpp"

TEST(PresetTest, ValidateDefault)
{
	Preset preset = Preset::getStrongDefault();
	EXPECT_EQ(preset.validate(), std::nullopt);
}

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
#include "obs_test_environment.hpp"

#include "Preset.hpp"

using kaito_tokyo::obs_showdraw::Preset;

::testing::Environment *const obs_env =
	::testing::AddGlobalTestEnvironment(new kaito_tokyo::obs_showdraw_testing::ObsTestEnvironment());

TEST(PresetTest, ValidateDefault)
{
	Preset preset = Preset::getStrongDefault();
	EXPECT_EQ(preset.validate(), std::nullopt);
}

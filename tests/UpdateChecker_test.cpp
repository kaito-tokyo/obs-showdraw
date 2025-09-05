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

#define private public
#include "UpdateChecker.hpp"
#undef private

TEST(UpdateCheckerTest, Fetch)
{
	UpdateChecker checker;
	checker.fetch();
	EXPECT_FALSE(checker.getLatestVersion().empty());
}

TEST(UpdateCheckerTest, IsUpdateAvailable)
{
	UpdateChecker checker;

	checker.latestVersion = "1.0.0";
	EXPECT_TRUE(checker.isUpdateAvailable("0.9.0"));
	EXPECT_FALSE(checker.isUpdateAvailable("1.0.0"));
	EXPECT_FALSE(checker.isUpdateAvailable("1.1.0"));

	checker.latestVersion = "2.0.0-beta";
	EXPECT_TRUE(checker.isUpdateAvailable("1.0.0"));
	EXPECT_TRUE(checker.isUpdateAvailable("2.0.0-alpha"));
	EXPECT_FALSE(checker.isUpdateAvailable("2.0.0-beta"));
	EXPECT_FALSE(checker.isUpdateAvailable("2.0.0"));

	checker.latestVersion = "";
	EXPECT_FALSE(checker.isUpdateAvailable("1.0.0"));
}
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

#include "UpdateChecker.hpp"

#include <cpr/cpr.h>

#include <obs.h>
#include "plugin-support.h"

UpdateChecker::UpdateChecker(void) {}

void UpdateChecker::fetch(void)
{
	cpr::Response r = cpr::Get(cpr::Url{"https://obs-showdraw.kaito.tokyo/metadata/latest-version.txt"});

	if (r.status_code == 200) {
		latestVersion = r.text;
	} else {
		obs_log(LOG_INFO, "Failed to fetch latest version. Status code: %ld, Error message: %s", r.status_code,
			r.error.message.c_str());
		latestVersion = "";
	}
}

bool UpdateChecker::isUpdateAvailable(const std::string &currentVersion) const noexcept
{
	if (latestVersion.empty()) {
		obs_log(LOG_INFO, "Latest version information is not available.");
		return false;
	}

	if (latestVersion != currentVersion) {
		blog(LOG_INFO, "[obs-showdraw] A new version is available: %s (current: %s)", latestVersion.c_str(),
		     currentVersion.c_str());
	} else {
		blog(LOG_INFO, "[obs-showdraw] You are using the latest version: %s", currentVersion.c_str());
	}

	return true;
}

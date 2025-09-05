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
#include <semver.hpp>

#include <obs.h>
#include "plugin-support.h"

#include <obs-bridge-utils/obs-bridge-utils.hpp>

using kaito_tokyo::obs_bridge_utils::log;

UpdateChecker::UpdateChecker(void) {}

void UpdateChecker::fetch(void)
{
	cpr::Response r = cpr::Get(cpr::Url{"https://obs-showdraw.kaito.tokyo/metadata/latest-version.txt"});

	if (r.status_code == 200) {
		latestVersion = r.text;
	} else {
		log(LOG_WARNING) << "Failed to fetch latest version information: HTTP " << r.status_code;
		latestVersion = "";
	}
}

bool UpdateChecker::isUpdateAvailable(const std::string &currentVersion) const noexcept
{
	if (latestVersion.empty()) {
		log(LOG_INFO) << "Latest version information is not available.";
		return false;
	}

	semver::version latest, current;
	semver::parse(latestVersion, latest);
	semver::parse(currentVersion, current);

	return latest > current;
}

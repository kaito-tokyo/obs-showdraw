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

#include "plugin-support.h"
#include <obs.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#include <obs-bridge-utils/obs-bridge-utils.hpp>
#include "CprUtils.hpp"

using namespace kaito_tokyo::obs_bridge_utils;
using namespace kaito_tokyo::obs_showdraw;

namespace kaito_tokyo {
namespace obs_showdraw {

LatestVersion::LatestVersion(const std::string &version) : version(version) {}

bool LatestVersion::isUpdateAvailable(const std::string &currentVersion) const noexcept
{
	if (version.empty()) {
		slog(LOG_INFO) << "Latest version information is not available.";
		return false;
	}

	semver::version latest, current;

	if (!semver::parse(version, latest)) {
		slog(LOG_WARNING) << "Failed to parse latest version: " << version;
		return false;
	}

	if (!semver::parse(currentVersion, current)) {
		slog(LOG_WARNING) << "Failed to parse current version: " << currentVersion;
		return false;
	}

	return latest > current;
}

const std::string &LatestVersion::toString() const noexcept
{
	return version;
}

UpdateChecker::UpdateChecker() {}

std::optional<LatestVersion> UpdateChecker::fetch()
{
	MyCprSession session;
	session.SetUrl(cpr::Url{"https://obs-showdraw.kaito.tokyo/metadata/latest-version.txt"});
	cpr::Response r = session.Get();

	if (r.status_code == 200) {
		return LatestVersion(r.text);
	} else {
		obs_log(LOG_WARNING, "Failed to fetch latest version information: HTTP %ld", r.status_code);
		return std::nullopt;
	}
}

} // namespace obs_showdraw
} // namespace kaito_tokyo

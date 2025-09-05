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

#include <sstream>

#include <nlohmann/json.hpp>

#include <obs.h>
#include "plugin-support.h"

UpdateChecker::UpdateChecker(void) {}

void UpdateChecker::fetch(void)
{
	cpr::Response response = cpr::Get(cpr::Url{"https://www.example.com"});
	obs_log(LOG_INFO, "HTTP Request completed with status code: %d", response.status_code);
	obs_log(LOG_INFO, "HTTP Request completed with status code: %s", response.error.message.c_str());

	// if (response.status_code == 200) {
	// 	std::string response_body = response.text;
	// 	obs_log(LOG_INFO, "GitHub API Response: %s", response_body.c_str());

	// 	try {
	// 		auto json_response = nlohmann::json::parse(response_body);
	// 		if (json_response.contains("tag_name")) {
	// 			latestVersion = json_response["tag_name"].get<std::string>();
	// 			obs_log(LOG_INFO, "Latest version: %s", latestVersion.c_str());
	// 		} else {
	// 			obs_log(LOG_WARNING, "GitHub API Response does not contain 'tag_name'");
	// 		}
	// 	} catch (const nlohmann::json::exception &e) {
	// 		obs_log(LOG_ERROR, "JSON parsing error: %s", e.what());
	// 	}
	// } else {
	// 	obs_log(LOG_ERROR, "HTTP Request failed with status code: %d", response.status_code);
	// 	obs_log(LOG_ERROR, "Error message: %s", response.error.message.c_str());
	// }
}

void UpdateChecker::isUpdateAvailable(void) const noexcept {}

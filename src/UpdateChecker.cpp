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

#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>

#include <nlohmann/json.hpp>

#include <obs.h>
#include "plugin-support.h"

UpdateChecker::UpdateChecker(void) {}

void UpdateChecker::fetch(void)
{
	try {
		cURLpp::Cleanup myCleanup;

		std::ostringstream os;
		curlpp::Easy request;
		request.setOpt(curlpp::options::Url("https://api.github.com/repos/kaito-tokyo/obs-showdraw/releases/latest"));
		request.setOpt(curlpp::options::FollowLocation(true));
		request.setOpt(curlpp::options::UserAgent("obs-showdraw")); // GitHub API requires a User-Agent

		std::ostringstream response_stream;
		request.setOpt(curlpp::options::WriteStream(&response_stream));

		request.perform();

		std::string response_body = response_stream.str();
		obs_log(LOG_INFO, "GitHub API Response: %s", response_body.c_str());

		try {
			auto json_response = nlohmann::json::parse(response_body);
			if (json_response.contains("tag_name")) {
				latestVersion = json_response["tag_name"].get<std::string>();
				obs_log(LOG_INFO, "Latest version: %s", latestVersion.c_str());
			} else {
				obs_log(LOG_WARNING, "GitHub API Response does not contain 'tag_name'");
			}
		} catch (const nlohmann::json::exception& e) {
			obs_log(LOG_ERROR, "JSON parsing error: %s", e.what());
		}

	} catch (curlpp::LogicError &e) {
		obs_log(LOG_ERROR, "UpdateChecker LogicError: %s\n", e.what());
	} catch (curlpp::RuntimeError &e) {
		obs_log(LOG_ERROR, "UpdateChecker RuntimeError: %s\n", e.what());
	}
}

void UpdateChecker::isUpdateAvailable(void) const noexcept {}

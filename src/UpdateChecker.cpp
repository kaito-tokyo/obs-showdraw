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
#include <iostream> // For std::cout

#include <nlohmann/json.hpp>
#include <cpr/cpr.h> // Add this include

#include <obs.h>
#include "plugin-support.h"

UpdateChecker::UpdateChecker(void) {}

void UpdateChecker::fetch(void)
{
	cpr::Response r = cpr::Get(cpr::Url{"https://api.github.com/repos/kaito-tokyo/obs-showdraw/releases/latest"});

	if (r.status_code == 200) {
		try {
			auto json_response = nlohmann::json::parse(r.text);
			if (json_response.contains("tag_name")) {
				latestVersion = json_response["tag_name"].get<std::string>();
				std::cout << "Latest version: " << latestVersion << std::endl; // For debugging
			} else {
				std::cerr << "Error: 'tag_name' not found in the response." << std::endl;
			}
		} catch (const nlohmann::json::exception &e) {
			std::cerr << "JSON parsing error: " << e.what() << std::endl;
		}
	} else {
		std::cerr << "Failed to fetch latest version. Status code: " << r.status_code << std::endl;
		std::cerr << "Error message: " << r.error.message << std::endl;
	}
}

void UpdateChecker::isUpdateAvailable(void) const noexcept {}

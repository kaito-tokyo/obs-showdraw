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

#include "plugin-support.h"

#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>

#include <sstream>

UpdateChecker::UpdateChecker(void) {}

void UpdateChecker::fetch(void)
{
	try {
		cURLpp::Cleanup myCleanup;

		std::ostringstream os;
		os << curlpp::options::Url("https://api.github.com/repos/kaito-tokyo/obs-showdraw/releases/latest");

	} catch (curlpp::LogicError &e) {
		obs_log(OBS_LOG_ERROR, "UpdateChecker LogicError: %s\n", e.what());
	} catch (curlpp::RuntimeError &e) {
		obs_log(OBS_LOG_ERROR, "UpdateChecker RuntimeError: %s\n", e.what());
	}
}

void UpdateChecker::isUpdateAvailable(void) const noexcept {}

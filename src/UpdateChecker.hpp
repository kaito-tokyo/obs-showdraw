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

#pragma once

#include <optional>
#include <string>

class LatestVersion {
public:
	explicit LatestVersion(const std::string &version);
	bool isUpdateAvailable(const std::string &currentVersion) const noexcept;
	const std::string &toString() const noexcept;

private:
	std::string version;
};

/**
 * @brief Performs a synchronous check for software updates.
 *
 * @note The calling context is expected to handle any desired asynchronous
 * execution.
 */
class UpdateChecker {
public:
	UpdateChecker();
	std::optional<LatestVersion> fetch();
};

/*
obs-bridge-utils
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

#include <streambuf>
#include <ostream>
#include <string>

#include "plugin-support.h"

namespace kaito_tokyo {
namespace obs_bridge_utils {

class obs_log_streambuf : public std::streambuf {
public:
	explicit obs_log_streambuf(int level) noexcept : level(level) { buffer.reserve(1024); }

	~obs_log_streambuf() noexcept override { sync(); }

	obs_log_streambuf(const obs_log_streambuf &) = delete;
	obs_log_streambuf &operator=(const obs_log_streambuf &) = delete;
	obs_log_streambuf(obs_log_streambuf &&) = delete;
	obs_log_streambuf &operator=(obs_log_streambuf &&) = delete;

protected:
	int_type overflow(int_type c = traits_type::eof()) override
	{
		if (c != traits_type::eof()) {
			buffer += static_cast<char>(c);
			if (c == '\n') {
				sync();
			}
		}
		return c;
	}

	int sync() override
	{
		if (!buffer.empty()) {
			if (buffer.back() == '\n') {
				buffer.pop_back();
			}

			if (!buffer.empty()) {
				obs_log(level, "%s", buffer.c_str());
			}

			buffer.clear();
		}
		return 0;
	}

private:
	const int level;
	std::string buffer;
};

class obs_log_ostream : public std::ostream {
public:
	explicit obs_log_ostream(int level) noexcept : streambuf(level), std::ostream(nullptr) { rdbuf(&streambuf); }

	obs_log_ostream(const obs_log_ostream &) = delete;
	obs_log_ostream &operator=(const obs_log_ostream &) = delete;
	obs_log_ostream(obs_log_ostream &&) = delete;
	obs_log_ostream &operator=(obs_log_ostream &&) = delete;

private:
	obs_log_streambuf streambuf;
};

inline obs_log_ostream slog(int level) noexcept
{
	return obs_log_ostream(level);
}

} // namespace obs_bridge_utils
} // namespace kaito_tokyo

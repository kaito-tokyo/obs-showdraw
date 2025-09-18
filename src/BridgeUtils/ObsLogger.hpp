/*
Bridge Utils
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

#if defined(__linux__)
#define BACKWARD_HAS_DW 1
#define BACKWARD_HAS_LIBUNWIND 1
#endif

#include <backward.hpp>

#include <util/base.h>

#include "ILogger.hpp"
#include "ObsUnique.hpp"

namespace KaitoTokyo {
namespace BridgeUtils {

class ObsLogger final : public ILogger {
public:
	ObsLogger(const std::string &_prefix) : prefix(_prefix) {}

protected:
	void log(LogLevel level, std::string_view message) const noexcept override
	{
		int blogLevel;
		switch (level) {
		case LogLevel::Debug:
			blogLevel = LOG_DEBUG;
			break;
		case LogLevel::Info:
			blogLevel = LOG_INFO;
			break;
		case LogLevel::Warn:
			blogLevel = LOG_WARNING;
			break;
		case LogLevel::Error:
			blogLevel = LOG_ERROR;
			break;
		default:
			fprintf(stderr, "[LOGGER FATAL] Unknown log level: %d\n", static_cast<int>(level));
			return;
		}

		blog(blogLevel, "%.*s", static_cast<int>(message.length()), message.data());
	}

	void logException(const std::exception &e, std::string_view context) const noexcept override
	try {
		error("{}: {}", context, e.what());

		backward::StackTrace st;
		st.load_here(32);

		std::stringstream ss;
		backward::Printer p;
		p.print(st, ss);
		log(LogLevel::Error, fmt::format("{}--- Stack Trace ---\n{}", getPrefix(), ss.str()));
	} catch (const std::exception &log_ex) {
		fprintf(stderr, "[LOGGER FATAL] Failed during exception logging: %s\n", log_ex.what());
	} catch (...) {
		fprintf(stderr, "[LOGGER FATAL] Unknown error during exception logging.\n");
	}

protected:
	std::string_view getPrefix() const noexcept override { return prefix; }

private:
	const std::string prefix;
};

} // namespace BridgeUtils
} // namespace KaitoTokyo

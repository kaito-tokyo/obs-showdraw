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

#include <memory>
#include <obs.h>

namespace kaito_tokyo {
namespace obs_bridge_utils {

struct gs_effect_deleter {
	void operator()(gs_effect_t *effect) const { gs_effect_destroy(effect); }
};

using unique_gs_effect_t = std::unique_ptr<gs_effect_t, gs_effect_deleter>;

} // namespace obs_bridge_utils
} // namespace kaito_tokyo

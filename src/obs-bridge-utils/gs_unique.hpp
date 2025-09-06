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

#include <deque>
#include <memory>
#include <mutex>
#include <stdexcept>

#include <obs.h>

#include "obs-bridge-utils/unique_bfree.hpp"

namespace kaito_tokyo {
namespace obs_bridge_utils {

namespace gs_unique {

inline std::mutex &get_mutex()
{
	static std::mutex mtx;
	return mtx;
}

inline std::deque<gs_effect_t *> &get_effects_deque()
{
	static std::deque<gs_effect_t *> effects_to_delete;
	return effects_to_delete;
}

inline std::deque<gs_texture_t *> &get_textures_deque()
{
	static std::deque<gs_texture_t *> textures_to_delete;
	return textures_to_delete;
}

inline void schedule_effect_to_delete(gs_effect_t *effect)
{
	if (effect) {
		std::lock_guard lock(get_mutex());
		get_effects_deque().push_back(effect);
	}
}

inline void schedule_texture_to_delete(gs_texture_t *texture)
{
	if (texture) {
		std::lock_guard lock(get_mutex());
		get_textures_deque().push_back(texture);
	}
}

inline void drain()
{
	std::deque<gs_effect_t *> _effects_to_delete;
	std::deque<gs_texture_t *> _textures_to_delete;
	{
		std::lock_guard lock(get_mutex());
		if (!get_effects_deque().empty()) {
			_effects_to_delete = std::move(get_effects_deque());
		}
		if (!get_textures_deque().empty()) {
			_textures_to_delete = std::move(get_textures_deque());
		}
	}

	for (gs_effect_t *effect : _effects_to_delete) {
		gs_effect_destroy(effect);
	}
	for (gs_texture_t *texture : _textures_to_delete) {
		gs_texture_destroy(texture);
	}
}

} // namespace gs_unique

struct gs_effect_deleter {
	void operator()(gs_effect_t *effect) const { gs_unique::schedule_effect_to_delete(effect); }
};

using unique_gs_effect_t = std::unique_ptr<gs_effect_t, gs_effect_deleter>;

inline unique_gs_effect_t make_unique_gs_effect_from_file(const char *file)
{
	char *raw_error_string = nullptr;
	gs_effect_t *rawEffect = gs_effect_create_from_file(file, &raw_error_string);
	unique_bfree_t error_string(raw_error_string);

	if (!rawEffect) {
		throw std::runtime_error(std::string("gs_effect_create_from_file failed: ") +
					 (error_string ? error_string.get() : "(unknown error)"));
	}
	return unique_gs_effect_t(rawEffect);
}

struct gs_texture_deleter {
	void operator()(gs_texture_t *texture) const { gs_unique::schedule_texture_to_delete(texture); }
};

using unique_gs_texture_t = std::unique_ptr<gs_texture_t, gs_texture_deleter>;

inline unique_gs_texture_t make_unique_gs_texture(uint32_t width, uint32_t height, enum gs_color_format color_format,
						  uint32_t levels, const uint8_t **data, uint32_t flags)
{
	gs_texture_t *rawTexture = gs_texture_create(width, height, color_format, levels, data, flags);
	if (!rawTexture) {
		throw std::runtime_error("gs_texture_create failed");
	}
	return unique_gs_texture_t(rawTexture);
}

class graphics_context_guard {
public:
	graphics_context_guard() noexcept { obs_enter_graphics(); }
	~graphics_context_guard() noexcept { obs_leave_graphics(); }

	graphics_context_guard(const graphics_context_guard &) = delete;
	graphics_context_guard(graphics_context_guard &&) = delete;
	graphics_context_guard &operator=(const graphics_context_guard &) = delete;
	graphics_context_guard &operator=(graphics_context_guard &&) = delete;
};

} // namespace obs_bridge_utils
} // namespace kaito_tokyo

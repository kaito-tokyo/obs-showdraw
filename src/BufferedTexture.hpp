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

#include <array>
#include <cstdint>
#include <vector>

#include <obs-bridge-utils/obs-bridge-utils.hpp>

namespace kaito_tokyo {
namespace obs_showdraw {

class BufferedTexture {
public:
	BufferedTexture(std::uint32_t width, std::uint32_t height, std::uint32_t flags = GS_RENDER_TARGET,
			gs_color_format format = GS_BGRA);

	gs_texture_t *getTexture() const;
	void stage();
	bool sync();
	const std::vector<std::uint8_t> &getBuffer() const noexcept;

	const std::uint32_t width;
	const std::uint32_t height;
	const std::uint32_t bufferLinesize;

private:
	std::vector<uint8_t> buffer;
	kaito_tokyo::obs_bridge_utils::unique_gs_texture_t texture;
	std::array<kaito_tokyo::obs_bridge_utils::unique_gs_stagesurf_t, 2> stagesurfs;

	std::size_t writeIndex = 0;
};

} // namespace obs_showdraw
} // namespace kaito_tokyo

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
#include <cstring>
#include <stdexcept>
#include <vector>
#include <iostream>

#include <obs-bridge-utils/obs-bridge-utils.hpp>

namespace kaito_tokyo {
namespace obs_showdraw {

namespace detail {

inline std::uint32_t getBytesPerPixel(gs_color_format format)
{
	switch (format) {
	case GS_A8:
	case GS_R8:
		return 1;
	case GS_R8G8:
	case GS_RG16:
		return 2;
	case GS_RGBA:
	case GS_BGRX:
	case GS_BGRA:
	case GS_R16:
	case GS_R16F:
		return 4;
	case GS_R10G10B10A2:
	case GS_RGBA16:
	case GS_RGBA16F:
		return 8;
	case GS_RGBA32F:
		return 16;
	default:
		throw std::runtime_error("Unsupported color format");
	}
}

} // namespace detail

template<std::size_t BUFFER_COUNT> class BufferedTexture {
public:
	BufferedTexture(std::uint32_t width, std::uint32_t height, gs_color_format format = GS_BGRA,
			std::uint32_t flags = GS_DYNAMIC | GS_RENDER_TARGET)
		: width{width},
		  height{height},
		  bufferLinesize{width * detail::getBytesPerPixel(format)},
		  buffer(height * bufferLinesize),
		  texture{obs_bridge_utils::make_unique_gs_texture(width, height, format, 1, nullptr, flags)}
	{
		for (std::size_t i = 0; i < BUFFER_COUNT; ++i) {
			stagesurfs[i] = obs_bridge_utils::make_unique_gs_stagesurf(width, height, format);
		}
	}

	gs_texture_t *getTexture() const noexcept { return texture.get(); }

	void stage() noexcept {
		gs_stage_texture(stagesurfs[writeIndex].get(), texture.get());
	}

	void stage(gs_texture_t *sourceTexture) noexcept {
		gs_copy_texture(texture.get(), sourceTexture);
		// gs_stage_texture(stagesurfs[writeIndex].get(), texture.get());
	}

	void sync()
	{
		const std::size_t readIndex = (writeIndex + 1) % BUFFER_COUNT;
		// gs_stagesurf_t *stagesurf = stagesurfs[readIndex].get();

		std::uint8_t *data = nullptr;
		std::uint32_t linesize = 0;

		if (!gs_texture_map(texture.get(), &data, &linesize) || !data || linesize > bufferLinesize) {
			throw std::runtime_error("gs_texture_map failed");
		}

		for (std::uint32_t y = 0; y < height; y++) {
			const std::uint8_t *srcRow = data + (y * linesize);
			std::uint8_t *dstRow = buffer.data() + (y * bufferLinesize);
			std::memcpy(dstRow, srcRow, bufferLinesize);
		}

		gs_texture_unmap(texture.get());

		writeIndex = readIndex;
	}

	const std::vector<std::uint8_t> &getBuffer() const noexcept { return buffer; }

	std::uint32_t getWidth() const noexcept { return width; }
	std::uint32_t getHeight() const noexcept { return height; }

private:
	const std::uint32_t width;
	const std::uint32_t height;
	const std::uint32_t bufferLinesize;
	gs_color_format format;

	std::vector<uint8_t> buffer;
	kaito_tokyo::obs_bridge_utils::unique_gs_texture_t texture;
	std::array<kaito_tokyo::obs_bridge_utils::unique_gs_stagesurf_t, BUFFER_COUNT> stagesurfs;

	std::size_t writeIndex = 0;
};

} // namespace obs_showdraw
} // namespace kaito_tokyo

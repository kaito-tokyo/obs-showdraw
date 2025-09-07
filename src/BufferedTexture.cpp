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

#include "BufferedTexture.hpp"

#include <cstdint>
#include <cstring>

using namespace kaito_tokyo::obs_bridge_utils;

namespace {

std::uint32_t getBytesPerPixel(enum gs_color_format format)
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

} // namespace

namespace kaito_tokyo {
namespace obs_showdraw {

BufferedTexture::BufferedTexture(std::uint32_t _width, std::uint32_t _height, std::uint32_t flags, gs_color_format format)
	: width{_width},
	  height{_height},
	  bufferLinesize{_width * getBytesPerPixel(format)},
	  buffer(_height * bufferLinesize),
	  texture{make_unique_gs_texture(_width, _height, format, 1, nullptr, flags)},
	  stagesurfs{make_unique_gs_stagesurf(_width, _height, format), make_unique_gs_stagesurf(_width, _height, format)}
{
}

gs_texture_t *BufferedTexture::getTexture() const
{
	return texture.get();
}

void BufferedTexture::stage()
{
	gs_stage_texture(stagesurfs[writeIndex].get(), texture.get());
}

bool BufferedTexture::sync()
{
	const std::size_t readIndex = (writeIndex + 1) % stagesurfs.size();
	gs_stagesurf_t *stagesurf = stagesurfs[readIndex].get();

	std::uint8_t *data = nullptr;
	std::uint32_t linesize = 0;

	if (!gs_stagesurface_map(stagesurf, &data, &linesize) || !data || linesize < bufferLinesize) {
    	gs_stagesurface_unmap(stagesurf);
		return false;
	}

	for (std::uint32_t y = 0; y < height; y++) {
		const std::uint8_t *srcRow = data + y * linesize;
		std::uint8_t *dstRow = buffer.data() + (y * bufferLinesize);
		std::memcpy(dstRow, srcRow, bufferLinesize);
	}

	gs_stagesurface_unmap(stagesurf);

	writeIndex = readIndex;

	return true;
}

const std::vector<std::uint8_t> &BufferedTexture::getBuffer() const noexcept
{
	return buffer;
}

} // namespace obs_showdraw
} // namespace kaito_tokyo

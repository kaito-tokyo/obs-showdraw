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

#include <obs-bridge-utils/obs-bridge-utils.hpp>

namespace kaito_tokyo {
namespace obs_showdraw {

namespace detail {

/**
 * @brief Gets the number of bytes per pixel for a given color format.
 * @param format The OBS color format.
 * @return The number of bytes per pixel.
 * @throws std::runtime_error if the color format is unsupported.
 */
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

/**
 * @class AsyncTextureReader
 * @brief Manages a ring buffer of staging surfaces to read GPU texture data efficiently on the CPU.
 *
 * This class implements a pipeline (e.g., double or triple buffering) to copy texture data
 * from the GPU to CPU-accessible memory without stalling the render thread. The typical workflow is:
 * 1. Call stage() from the render thread to schedule a copy of a GPU texture. This is a non-blocking operation.
 * 2. Call sync() from a CPU thread to synchronize the internal buffer with the latest staged texture data.
 * 3. Access the pixel data using getBuffer().
 *
 * @tparam BUFFER_COUNT The number of buffers in the ring. 2 for double buffering, 3 for triple buffering.
 *
 * @warning This class is NOT thread-safe. Access to an instance of this class from multiple
 * threads must be synchronized externally (e.g., using a mutex).
 */
template<std::size_t BUFFER_COUNT> class AsyncTextureReader {
public:
	/**
     * @brief Constructs the AsyncTextureReader and allocates all necessary resources.
     * @param width The width of the textures to be read.
     * @param height The height of the textures to be read.
     * @param format The color format of the textures.
     */
	AsyncTextureReader(std::uint32_t width, std::uint32_t height, gs_color_format format = GS_BGRA)
		: width{width},
		  height{height},
		  bufferLinesize{(width * detail::getBytesPerPixel(format) + 3) & ~3u},
		  buffer(height * bufferLinesize)
	{
		for (std::size_t i = 0; i < BUFFER_COUNT; ++i) {
			stagesurfs[i] = obs_bridge_utils::make_unique_gs_stagesurf(width, height, format);
		}
	}

	/**
     * @brief Schedules a non-blocking copy of a GPU texture to the next available staging surface.
     *
     * This method should be called from the OBS render thread. It advances the internal write
     * pointer to the next buffer in the ring.
     * @param sourceTexture A pointer to the GPU texture to be copied.
     */
	void stage(gs_texture_t *sourceTexture) noexcept
	{
		gs_stage_texture(stagesurfs[writeIndex].get(), sourceTexture);
		writeIndex = (writeIndex + 1) % BUFFER_COUNT;
	}

	/**
     * @brief Synchronizes the internal CPU buffer with the latest fully staged texture.
     *
     * This method maps the appropriate staging surface (the one most recently staged),
     * copies its pixel data to the internal CPU buffer, and then unmaps it.
     * This can be a potentially expensive operation as it involves GPU-CPU data transfer.
     *
     * @throws std::runtime_error if mapping the staging surface fails or returns invalid data.
     */
	void sync()
	{
		const std::size_t readIndex = writeIndex;
		gs_stagesurf_t *stagesurf = stagesurfs[readIndex].get();

		std::uint8_t *data = nullptr;
		std::uint32_t linesize = 0;

		if (!gs_stagesurface_map(stagesurf, &data, &linesize)) {
			throw std::runtime_error("gs_stagesurface_map failed");
		}

		if (!data || linesize > bufferLinesize) {
			gs_stagesurface_unmap(stagesurf);
			throw std::runtime_error("gs_stagesurface_map returned invalid data");
		}

		const std::size_t bytesToCopyPerRow =
			std::min(static_cast<size_t>(linesize), static_cast<size_t>(bufferLinesize));

		// Following code won't throw exceptions.
		// We must not leave the stagesurface mapped.
		for (std::uint32_t y = 0; y < height; y++) {
			const std::uint8_t *srcRow = data + (y * linesize);
			std::uint8_t *dstRow = buffer.data() + (y * bufferLinesize);
			std::memcpy(dstRow, srcRow, bytesToCopyPerRow);
		}

		gs_stagesurface_unmap(stagesurf);
	}

	/**
     * @brief Gets read-write access to the internal CPU buffer containing the pixel data.
     * @return A reference to the pixel data buffer.
     */
	std::vector<uint8_t> &getBuffer() noexcept { return buffer; }

	/**
     * @brief Gets read-only access to the internal CPU buffer containing the pixel data.
     * @return A constant reference to the pixel data buffer.
     */
	const std::vector<uint8_t> &getBuffer() const noexcept { return buffer; }

	/**
     * @brief Gets the width of the texture.
     * @return The width in pixels.
     */
	std::uint32_t getWidth() const noexcept { return width; }

	/**
     * @brief Gets the height of the texture.
     * @return The height in pixels.
     */
	std::uint32_t getHeight() const noexcept { return height; }

	/**
     * @brief Gets the line size (stride) of the internal CPU buffer in bytes.
     * @return The number of bytes per row in the buffer.
     */
	std::uint32_t getBufferLinesize() const noexcept { return bufferLinesize; }

private:
	const std::uint32_t width;
	const std::uint32_t height;
	const std::uint32_t bufferLinesize;

	std::vector<uint8_t> buffer;
	std::array<kaito_tokyo::obs_bridge_utils::unique_gs_stagesurf_t, BUFFER_COUNT> stagesurfs;

	std::size_t writeIndex = 0;
};

} // namespace obs_showdraw
} // namespace kaito_tokyo

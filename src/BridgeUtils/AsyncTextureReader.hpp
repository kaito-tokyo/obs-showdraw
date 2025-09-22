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

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

#include <obs.h>

#include "GsUnique.hpp"

namespace KaitoTokyo {
namespace BridgeUtils {

namespace AsyncTextureReaderDetail {

inline std::uint32_t getBytesPerPixel(const gs_color_format format)
{
	switch (format) {
	case GS_A8:
	case GS_R8:
		return 1;
	case GS_R8G8:
	case GS_RG16:
	case GS_R16:
	case GS_R16F:
		return 2;
	case GS_RGBA:
	case GS_BGRX:
	case GS_BGRA:
	case GS_R10G10B10A2:
	case GS_R32F:
		return 4;
	case GS_RGBA16:
	case GS_RGBA16F:
		return 8;
	case GS_RGBA32F:
		return 16;
	default:
		throw std::runtime_error("Unsupported color format");
	}
}

struct ScopedStageSurfMap {
	gs_stagesurf_t *surf = nullptr;
	std::uint8_t *data = nullptr;
	std::uint32_t linesize = 0;

	explicit ScopedStageSurfMap(gs_stagesurf_t *surf) : surf{surf}
	{
		if (!surf) {
			throw std::invalid_argument("Target surface cannot be null.");
		}
		if (!gs_stagesurface_map(surf, &data, &linesize)) {
			surf = nullptr;
			throw std::runtime_error("gs_stagesurface_map failed");
		}
	}

	~ScopedStageSurfMap() noexcept
	{
		if (surf) {
			gs_stagesurface_unmap(surf);
		}
	}

	ScopedStageSurfMap(const ScopedStageSurfMap &) = delete;
	ScopedStageSurfMap &operator=(const ScopedStageSurfMap &) = delete;
	ScopedStageSurfMap(ScopedStageSurfMap &&) = delete;
	ScopedStageSurfMap &operator=(ScopedStageSurfMap &&) = delete;
};

} // namespace AsyncTextureReaderDetail

class AsyncTextureReader {
public:
	AsyncTextureReader(const std::uint32_t width, const std::uint32_t height,
			   const gs_color_format format = GS_BGRA)
		: width(width),
		  height(height),
		  bufferLinesize((width * AsyncTextureReaderDetail::getBytesPerPixel(format) + 3) & ~3u),
		  cpuBuffers{std::vector<std::uint8_t>(height * bufferLinesize),
			     std::vector<std::uint8_t>(height * bufferLinesize)},
		  stagesurfs{make_unique_gs_stagesurf(width, height, format),
			     make_unique_gs_stagesurf(width, height, format)}
	{
	}

	void stage(gs_texture_t *sourceTexture) noexcept
	{
		std::lock_guard<std::mutex> lock(gpuMutex);
		gs_stage_texture(stagesurfs[gpuWriteIndex].get(), sourceTexture);
		gpuWriteIndex = 1 - gpuWriteIndex;
	}

	void sync()
	{
		std::size_t gpuReadIndex;
		{
			std::lock_guard<std::mutex> lock(gpuMutex);
			gpuReadIndex = 1 - gpuWriteIndex;
		}
		gs_stagesurf_t *const stagesurf = stagesurfs[gpuReadIndex].get();

		const AsyncTextureReaderDetail::ScopedStageSurfMap mappedSurf(stagesurf);

		if (!mappedSurf.data || mappedSurf.linesize > bufferLinesize) {
			throw std::runtime_error("gs_stagesurface_map returned invalid data");
		}

		const std::size_t bytesToCopyPerRow = std::min(static_cast<std::size_t>(mappedSurf.linesize),
							       static_cast<std::size_t>(bufferLinesize));

		const std::size_t backBufferIndex = 1 - activeCpuBufferIndex.load(std::memory_order_acquire);
		auto &backBuffer = cpuBuffers[backBufferIndex];

		for (std::uint32_t y = 0; y < height; y++) {
			const std::uint8_t *srcRow = mappedSurf.data + (y * mappedSurf.linesize);
			std::uint8_t *dstRow = backBuffer.data() + (y * bufferLinesize);
			std::memcpy(dstRow, srcRow, bytesToCopyPerRow);
		}

		activeCpuBufferIndex.store(backBufferIndex, std::memory_order_release);
	}

	std::vector<std::uint8_t> &getBuffer() noexcept
	{
		return const_cast<std::vector<std::uint8_t> &>(std::as_const(*this).getBuffer());
	}

	const std::vector<std::uint8_t> &getBuffer() const noexcept
	{
		return cpuBuffers[activeCpuBufferIndex.load(std::memory_order_acquire)];
	}

	std::uint32_t getWidth() const noexcept { return width; }

	std::uint32_t getHeight() const noexcept { return height; }

	std::uint32_t getBufferLinesize() const noexcept { return bufferLinesize; }

public:
	const std::uint32_t width;
	const std::uint32_t height;
	const std::uint32_t bufferLinesize;

private:
	std::array<std::vector<std::uint8_t>, 2> cpuBuffers;
	std::atomic<std::size_t> activeCpuBufferIndex = {0};

	std::array<unique_gs_stagesurf_t, 2> stagesurfs;
	std::size_t gpuWriteIndex = 0;
	std::mutex gpuMutex;
};

} // namespace BridgeUtils
} // namespace KaitoTokyo

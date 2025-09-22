/*
ShowDraw
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

#include <cstdint>
#include <memory>
#include <vector>

namespace KaitoTokyo {
namespace ShowDraw {

struct CanvasDetectorResult {
	float confidence;
	float x;
	float y;
	float width;
	float height;
};

class ShowDrawCanvasDetectorBridge {
public:
	static const std::uint32_t WIDTH = 640;
	static const std::uint32_t HEIGHT = 640;

	ShowDrawCanvasDetectorBridge();
	~ShowDrawCanvasDetectorBridge();

	ShowDrawCanvasDetectorBridge(const ShowDrawCanvasDetectorBridge &) = delete;
	ShowDrawCanvasDetectorBridge &operator=(const ShowDrawCanvasDetectorBridge &) = delete;
	ShowDrawCanvasDetectorBridge(ShowDrawCanvasDetectorBridge &&) noexcept;
	ShowDrawCanvasDetectorBridge &operator=(ShowDrawCanvasDetectorBridge &&) noexcept;

	std::vector<CanvasDetectorResult> detect(const uint8_t *bgra_data, int width, int height);

private:
	class Impl;
	std::unique_ptr<Impl> pimpl;
};

} // namespace ShowDraw
} // namespace KaitoTokyo

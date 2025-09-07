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

#include <obs.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *showdraw_get_name(void *type_data);
void *showdraw_create(obs_data_t *settings, obs_source_t *source);
void showdraw_destroy(void *data);
uint32_t showdraw_get_width(void *data);
uint32_t showdraw_get_height(void *data);
void showdraw_get_defaults(obs_data_t *data);
obs_properties_t *showdraw_get_properties(void *data);
void showdraw_update(void *data, obs_data_t *settings);
void showdraw_activate(void *data);
void showdraw_deactivate(void *data);
void showdraw_show(void *data);
void showdraw_hide(void *data);
void showdraw_video_tick(void *data, float seconds);
void showdraw_video_render(void *data, gs_effect_t *effect);
struct obs_source_frame *showdraw_filter_video(void *data, struct obs_source_frame *frame);

void showdraw_module_load(void);
void showdraw_module_unload(void);

#ifdef __cplusplus
}

#include <array>
#include <future>
#include <memory>
#include <optional>

#include "BufferedTexture.hpp"
#include "DrawingEffect.hpp"
#include "Preset.hpp"
#include "UpdateChecker.hpp"

namespace kaito_tokyo {
namespace obs_showdraw {

class skip_video_filter_exception : public std::exception {
public:
	const char *what() const noexcept override { return "skip video filter"; }
};

class ShowDrawFilterContext : public std::enable_shared_from_this<ShowDrawFilterContext> {
public:
	static const char *getName() noexcept;

	ShowDrawFilterContext(obs_data_t *settings, obs_source_t *source) noexcept;

	void afterCreate(obs_data_t *settings, obs_source_t *source);

	~ShowDrawFilterContext() noexcept;

	std::pair<uint32_t, uint32_t> getDimensions() const noexcept;
	uint32_t getWidth() const noexcept;
	uint32_t getHeight() const noexcept;

	static void getDefaults(obs_data_t *data) noexcept;

	obs_properties_t *getProperties();
	void update(obs_data_t *settings);
	void activate();
	void deactivate();
	void show();
	void hide();

	void videoTick(float seconds);
	void videoRender();
	obs_source_frame *filterVideo(struct obs_source_frame *frame);

	obs_source_t *getFilter() const noexcept;
	Preset getRunningPreset() const noexcept;
	std::optional<LatestVersion> getLatestVersion() const;

private:
	void reloadDrawingEffectInGraphics();
	void ensureTextures(uint32_t width, uint32_t height);

	obs_data_t *settings;
	obs_source_t *filter;
	uint32_t width;
	uint32_t height;
	float texelWidth;
	float texelHeight;
	std::shared_ptr<DrawingEffect> drawingEffect;
	Preset runningPreset;

	double sobelMagnitudeFinalizationScalingFactor = 1.0;

	std::shared_ptr<gs_texture_t> textureSource = nullptr;
	std::shared_ptr<gs_texture_t> textureTarget = nullptr;
	std::shared_ptr<gs_texture_t> textureTemporary1 = nullptr;
	std::shared_ptr<gs_texture_t> textureTemporary2 = nullptr;
	std::shared_ptr<gs_texture_t> texturePreviousLuminance = nullptr;
	std::shared_ptr<gs_texture_t> textureMotionMap = nullptr;
	std::shared_ptr<gs_texture_t> textureFinalSobelMagnitude = nullptr;

	std::unique_ptr<BufferedTexture> bufferedTextureCannyEdge = nullptr;

	std::array<kaito_tokyo::obs_bridge_utils::unique_gs_stagesurf_t, 2> stagesurfCannyEdge = {};

	std::shared_future<std::optional<LatestVersion>> futureLatestVersion;
};

} // namespace obs_showdraw
} // namespace kaito_tokyo

#endif // __cplusplus

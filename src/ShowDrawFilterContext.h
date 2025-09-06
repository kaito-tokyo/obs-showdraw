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

#ifdef __cplusplus
}

#include <memory>
#include <future>
#include <optional>

#include "DrawingEffect.hpp"
#include "Preset.hpp"
#include "UpdateChecker.hpp"

namespace kaito_tokyo {
namespace obs_showdraw {

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
	void ensureTextures(uint32_t width, uint32_t height);

	void applyLuminanceExtractionPass(gs_texture_t *targetTexture, gs_texture_t *sourceTexture) noexcept;
	void applyMedianFilteringPass(const float texelWidth, const float texelHeight, gs_texture_t *targetTexture,
				      gs_texture_t *sourceTexture) noexcept;
	void applyMotionAdaptiveFilteringPass(const float texelWidth, const float texelHeight,
					      gs_texture_t *targetTexture, gs_texture_t *targetMotionMapTexture,
					      gs_texture_t *sourceTexture,
					      gs_texture_t *sourcePreviousLuminanceTexture) noexcept;
	void applySobelPass(const float texelWidth, const float texelHeight, gs_texture_t *targetTexture,
			    gs_texture_t *sourceTexture) noexcept;
	void applyFinalizeSobelMagnitudePass(gs_texture_t *targetTexture, gs_texture_t *sourceTexture) noexcept;
	void applySuppressNonMaximumPass(const float texelWidth, const float texelHeight, gs_texture_t *targetTexture,
					 gs_texture_t *sourceTexture) noexcept;
	void applyHysteresisClassifyPass(const float texelWidth, const float texelHeight, const float highThreshold,
					 const float lowThreshold, gs_texture_t *targetTexture,
					 gs_texture_t *sourceTexture) noexcept;
	void applyHysteresisPropagatePass(const float texelWidth, const float texelHeight, gs_texture_t *targetTexture,
					  gs_texture_t *sourceTexture) noexcept;
	void applyHysteresisFinalizePass(const float texelWidth, const float texelHeight, gs_texture_t *targetTexture,
					 gs_texture_t *sourceTexture) noexcept;
	void applyMorphologyPass(gs_technique_t *technique, const float texelWidth, const float texelHeight,
				 int kernelSize, gs_texture_t *targetTexture, gs_texture_t *sourceTexture) noexcept;
	void applyScalingPass(gs_texture_t *targetTexture, gs_texture_t *sourceTexture) noexcept;
	void drawFinalImage(gs_texture_t *drawingTexture) noexcept;

	obs_data_t *settings;
	obs_source_t *filter;
	uint32_t width;
	uint32_t height;
	std::unique_ptr<DrawingEffect> drawingEffect;
	Preset runningPreset;

	double sobelMagnitudeFinalizationScalingFactor = 1.0;

	gs_texture_t *textureSource;
	gs_texture_t *textureTarget;
	gs_texture_t *textureMotionMap;
	gs_texture_t *texturePreviousLuminance;
	gs_texture_t *textureComplexSobel;
	gs_texture_t *textureFinalSobelMagnitude;

	std::shared_future<std::optional<LatestVersion>> futureLatestVersion;
};

} // namespace obs_showdraw
} // namespace kaito_tokyo

#endif // __cplusplus

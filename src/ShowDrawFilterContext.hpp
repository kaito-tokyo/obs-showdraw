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

#include <obs-module.h>

#include "showdraw-global-state.hpp"

#ifdef __cplusplus
extern "C" {
#endif

const char *showdraw_get_name(void *type_data);
void *showdraw_create(obs_data_t *settings, obs_source_t *source);
void showdraw_destroy(void *data);
void showdraw_get_defaults(obs_data_t *data);
obs_properties_t *showdraw_get_properties(void *data);
void showdraw_update(void *data, obs_data_t *settings);
void showdraw_video_render(void *data, gs_effect_t *effect);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

class ShowDrawFilterContext {
public:
	static const char *getName(void) noexcept;

	ShowDrawFilterContext(obs_data_t *settings, obs_source_t *source);
	~ShowDrawFilterContext(void) noexcept;

	static void getDefaults(obs_data_t *data) noexcept;
	obs_properties_t *getProperties(void) noexcept;
	void update(obs_data_t *settings) noexcept;
	void videoRender(void) noexcept;

private:
	bool initEffect(void) noexcept;
	bool ensureTextures(uint32_t width, uint32_t height) noexcept;

	void applyLuminanceExtractionPass(void) noexcept;
	void applyMedianFilteringPass(const float texelWidth, const float texelHeight) noexcept;
	void applyMotionAdaptiveFilteringPass(const float texelWidth, const float texelHeight) noexcept;
	void applySobelPass(const float texelWidth, const float texelHeight) noexcept;
	void applySuppressNonMaximumPass(const float texelWidth, const float texelHeight) noexcept;
	void applyEdgeDetectionPass(const float texelWidth, const float texelHeight) noexcept;
	void applyMorphologyPass(const float texelWidth, const float texelHeight, gs_technique_t *technique,
				 int kernelSize) noexcept;
	void applyScalingPass(void) noexcept;
	void drawFinalImage(void) noexcept;

	obs_data_t *settings;
	obs_source_t *source;

	struct showdraw_global_state global_state;

	double scaling_factor;

	gs_texture_t *texture_source;
	gs_texture_t *texture_target;
	gs_texture_t *texture_motion_map;
	gs_texture_t *texture_previous_luminance;

	gs_effect_t *effect;

	gs_eparam_t *effect_texture_image;
	gs_eparam_t *effect_texture_image1;

	gs_eparam_t *effect_float_texel_width;
	gs_eparam_t *effect_float_texel_height;
	gs_eparam_t *effect_int_kernel_size;

	gs_eparam_t *effect_texture_motion_map;
	gs_eparam_t *effect_float_strength;
	gs_eparam_t *effect_float_motion_threshold;

	gs_eparam_t *effect_float_scaling_factor;

	gs_technique_t *effect_tech_draw;
	gs_technique_t *effect_tech_extract_luminance;
	gs_technique_t *effect_tech_median_filtering;
	gs_technique_t *effect_tech_calculate_motion_map;
	gs_technique_t *effect_tech_motion_adaptive_filtering;
	gs_technique_t *effect_tech_apply_sobel;
	gs_technique_t *effect_tech_suppress_non_maximum;
	gs_technique_t *effect_tech_detect_edge;
	gs_technique_t *effect_tech_erosion;
	gs_technique_t *effect_tech_dilation;
	gs_technique_t *effect_tech_scaling;
};

#endif

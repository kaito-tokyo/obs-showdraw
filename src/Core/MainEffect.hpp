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
#include <obs-module.h>

#include "../BridgeUtils/GsUnique.hpp"

namespace KaitoTokyo {
namespace ShowDraw {

namespace DrawingEffectDetail {

inline gs_eparam_t *getEffectParam(const KaitoTokyo::obs_bridge_utils::unique_gs_effect_t &effect, const char *name)
{
	gs_eparam_t *param = gs_effect_get_param_by_name(effect.get(), name);
	if (!param) {
		obs_log(LOG_ERROR, "Effect parameter '%s' not found", name);
		throw std::runtime_error("Effect parameter not found");
	}
	return param;
}

inline gs_technique_t *getEffectTech(const kaito_tokyo::obs_bridge_utils::unique_gs_effect_t &effect, const char *name)
{
	gs_technique_t *tech = gs_effect_get_technique(effect.get(), name);
	if (!tech) {
		obs_log(LOG_ERROR, "Effect technique '%s' not found", name);
		throw std::runtime_error("Effect technique not found");
	}
	return tech;
}

} // namespace drawing_effect_detail

struct TransformStateGuard {
	TransformStateGuard() { gs_viewport_push(); gs_projection_push(); gs_matrix_push(); }
	~TransformStateGuard() { gs_matrix_pop(); gs_projection_pop(); gs_viewport_pop(); }
};

struct RenderTargetGuard {
	gs_texture_t *previousRenderTarget;
	gs_zstencil_t *previousZStencil;
	gs_color_space previousColorSpace;

	RenderTargetGuard()
		: previousRenderTarget(gs_get_render_target()),
		  previousZStencil(gs_get_zstencil_target()),
		  previousColorSpace(gs_get_color_space())
	{}

	~RenderTargetGuard() {
		gs_set_render_target_with_color_space(previousRenderTarget, previousZStencil, previousColorSpace);
	}
};


class DrawingEffect {
public:
	const kaito_tokyo::obs_bridge_utils::unique_gs_effect_t effect;

	gs_eparam_t *const textureImage;
	gs_eparam_t *const textureImage1;
	gs_eparam_t *const floatTexelWidth;
	gs_eparam_t *const floatTexelHeight;
	gs_eparam_t *const textureMotionMap;
	gs_eparam_t *const floatStrength;
	gs_eparam_t *const floatMotionThreshold;
	gs_eparam_t *const boolUseLog;
	gs_eparam_t *const floatScalingFactor;

	gs_technique_t *const techConvertGrayscale;
	gs_technique_t *const techHorizontalMedian3;
	gs_technique_t *const techVerticalMedian3;
	gs_technique_t *const techCalculateHorizontalMotionMap3;
	gs_technique_t *const techCalculateVerticalMotionMap3;
	gs_technique_t *const techMotionAdaptiveFiltering;
	gs_technique_t *const techApplySobel;
	gs_technique_t *const techFinalizeSobelMagnitude;
	gs_technique_t *const techHorizontalErosion3;
	gs_technique_t *const techVerticalErosion3;
	gs_technique_t *const techHorizontalDilation3;
	gs_technique_t *const techVerticalDilation3;
	gs_technique_t *const techDraw;
	gs_technique_t *const techDrawGrayscale;

	explicit DrawingEffect(const kaito_tokyo::obs_bridge_utils::unique_bfree_char_t &effectPath)
		: effect(kaito_tokyo::obs_bridge_utils::make_unique_gs_effect_from_file(effectPath)),
		  textureImage(drawing_effect_detail::getEffectParam(effect, "image")),
		  textureImage1(drawing_effect_detail::getEffectParam(effect, "image1")),
		  floatTexelWidth(drawing_effect_detail::getEffectParam(effect, "texelWidth")),
		  floatTexelHeight(drawing_effect_detail::getEffectParam(effect, "texelHeight")),
		  textureMotionMap(drawing_effect_detail::getEffectParam(effect, "motionMap")),
		  floatStrength(drawing_effect_detail::getEffectParam(effect, "strength")),
		  floatMotionThreshold(drawing_effect_detail::getEffectParam(effect, "motionThreshold")),
		  boolUseLog(drawing_effect_detail::getEffectParam(effect, "useLog")),
		  floatScalingFactor(drawing_effect_detail::getEffectParam(effect, "scalingFactor")),
		  techConvertGrayscale(drawing_effect_detail::getEffectTech(effect, "ConvertGrayscale")),
		  techHorizontalMedian3(drawing_effect_detail::getEffectTech(effect, "HorizontalMedian3")),
		  techVerticalMedian3(drawing_effect_detail::getEffectTech(effect, "VerticalMedian3")),
		  techCalculateHorizontalMotionMap3(drawing_effect_detail::getEffectTech(effect, "CalculateHorizontalMotionMap3")),
		  techCalculateVerticalMotionMap3(drawing_effect_detail::getEffectTech(effect, "CalculateVerticalMotionMap3")),
		  techMotionAdaptiveFiltering(drawing_effect_detail::getEffectTech(effect, "MotionAdaptiveFiltering")),
		  techApplySobel(drawing_effect_detail::getEffectTech(effect, "ApplySobel")),
		  techFinalizeSobelMagnitude(drawing_effect_detail::getEffectTech(effect, "FinalizeSobelMagnitude")),
		  techHorizontalErosion3(drawing_effect_detail::getEffectTech(effect, "HorizontalErosion3")),
		  techVerticalErosion3(drawing_effect_detail::getEffectTech(effect, "VerticalErosion3")),
		  techHorizontalDilation3(drawing_effect_detail::getEffectTech(effect, "HorizontalDilation3")),
		  techVerticalDilation3(drawing_effect_detail::getEffectTech(effect, "VerticalDilation3")),
		  techDraw(drawing_effect_detail::getEffectTech(effect, "Draw")),
		  techDrawGrayscale(drawing_effect_detail::getEffectTech(effect, "DrawGrayscale"))
	{}

	DrawingEffect(const DrawingEffect &) = delete;
	DrawingEffect(DrawingEffect &&) = delete;
	DrawingEffect &operator=(const DrawingEffect &) = delete;
	DrawingEffect &operator=(DrawingEffect &&) = delete;

	void applyPass(gs_technique_t *tech, uint32_t width, uint32_t height, std::function<void()> setParams) const noexcept
	{
		size_t passes = gs_technique_begin(tech);
		for (size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				setParams();
				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
	}

	void applyConvertGrayscalePass(uint32_t width, uint32_t height, gs_texture_t *target, gs_texture_t *source) const noexcept
	{
		RenderTargetGuard guard;
		gs_set_render_target(target, nullptr);
		applyPass(techConvertGrayscale, width, height, [&]() {
			gs_effect_set_texture(textureImage, source);
		});
	}

	// ... (他のapply...Pass関数も同様にリファクタリング)

	void applyMorphologyPass(uint32_t width, uint32_t height, gs_technique_t *horizontalTech,
							 gs_technique_t *verticalTech, float texelWidth, float texelHeight,
							 gs_texture_t *target, gs_texture_t *intermediate,
							 gs_texture_t *source) const noexcept
	{
		{
			RenderTargetGuard guard;
			gs_set_render_target(intermediate, nullptr);
			applyPass(horizontalTech, width, height, [&]() {
				gs_effect_set_texture(textureImage, source);
				gs_effect_set_float(floatTexelWidth, texelWidth);
			});
		}
		{
			RenderTargetGuard guard;
			gs_set_render_target(target, nullptr);
			applyPass(verticalTech, width, height, [&]() {
				gs_effect_set_texture(textureImage, intermediate);
				gs_effect_set_float(floatTexelHeight, texelHeight);
			});
		}
	}

	void drawTexture(uint32_t width, uint32_t height, gs_texture_t *source) const noexcept
	{
		applyPass(techDraw, width, height, [&]() {
			gs_effect_set_texture(textureImage, source);
		});
	}

	void drawGrayscaleTexture(uint32_t width, uint32_t height, gs_texture_t *source) const noexcept
	{
		applyPass(techDrawGrayscale, width, height, [&]() {
			gs_effect_set_texture(textureImage, source);
		});
	}
};

} // namespace obs_showdraw
} // namespace kaito_tokyo

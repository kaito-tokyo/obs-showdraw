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

#include "DrawingEffect.hpp"

#include <iostream>

#include "plugin-support.h"
#include <obs-module.h>

#include <obs-bridge-utils/obs-bridge-utils.hpp>

using namespace kaito_tokyo::obs_bridge_utils;
using namespace kaito_tokyo::obs_showdraw;

namespace {

struct RenderingGuard {
	gs_texture_t *previousRenderTarget;
	gs_zstencil_t *previousZStencil;
	gs_color_space previousColorSpace;

	RenderingGuard(gs_texture_t *targetTexture, gs_blend_type srcBlendType = GS_BLEND_ONE,
		       gs_blend_type dstBlendType = GS_BLEND_ZERO, gs_zstencil_t *targetZStencil = nullptr,
		       gs_color_space targetColorSpace = GS_CS_SRGB)
		: previousRenderTarget(gs_get_render_target()),
		  previousZStencil(gs_get_zstencil_target()),
		  previousColorSpace(gs_get_color_space())
	{
		gs_set_render_target_with_color_space(targetTexture, targetZStencil, targetColorSpace);
		gs_blend_state_push();
		gs_blend_function(srcBlendType, dstBlendType);
	}

	~RenderingGuard()
	{
		gs_blend_state_pop();
		gs_set_render_target_with_color_space(previousRenderTarget, previousZStencil, previousColorSpace);
	}
};

gs_eparam_t *getEffectParam(const unique_gs_effect_t &effect, const char *name)
{
	gs_eparam_t *param = gs_effect_get_param_by_name(effect.get(), name);

	if (!param) {
		obs_log(LOG_ERROR, "Effect parameter %s not found", name);
		throw std::runtime_error("Effect parameter not found");
	}

	return param;
}

gs_technique_t *getEffectTech(const unique_gs_effect_t &effect, const char *name)
{
	gs_technique_t *tech = gs_effect_get_technique(effect.get(), name);

	if (!tech) {
		obs_log(LOG_ERROR, "Effect technique %s not found", name);
		throw std::runtime_error("Effect technique not found");
	}

	return tech;
}

} // namespace

namespace kaito_tokyo {
namespace obs_showdraw {

DrawingEffect::DrawingEffect(unique_gs_effect_t _effect)
	: effect(std::move(_effect)),
	  textureImage(getEffectParam(effect, "image")),
	  textureImage1(getEffectParam(effect, "image1")),
	  floatTexelWidth(getEffectParam(effect, "texelWidth")),
	  floatTexelHeight(getEffectParam(effect, "texelHeight")),
	  intKernelSize(getEffectParam(effect, "kernelSize")),
	  textureMotionMap(getEffectParam(effect, "motionMap")),
	  floatStrength(getEffectParam(effect, "strength")),
	  floatMotionThreshold(getEffectParam(effect, "motionThreshold")),
	  floatHighThreshold(getEffectParam(effect, "highThreshold")),
	  floatLowThreshold(getEffectParam(effect, "lowThreshold")),
	  boolUseLog(getEffectParam(effect, "useLog")),
	  floatScalingFactor(getEffectParam(effect, "scalingFactor")),
	  techConvertGrayscale(getEffectTech(effect, "ConvertGrayscale")),
	  techHorizontalMedian3(getEffectTech(effect, "HorizontalMedian3")),
	  techVerticalMedian3(getEffectTech(effect, "VerticalMedian3")),
	  techCalculateHorizontalMotionMap3(getEffectTech(effect, "CalculateHorizontalMotionMap3")),
	  techCalculateVerticalMotionMap3(getEffectTech(effect, "CalculateVerticalMotionMap3")),
	  techMotionAdaptiveFiltering(getEffectTech(effect, "MotionAdaptiveFiltering")),
	  techApplySobel(getEffectTech(effect, "ApplySobel")),
	  techFinalizeSobelMagnitude(getEffectTech(effect, "FinalizeSobelMagnitude")),
	  techHorizontalErosion3(getEffectTech(effect, "HorizontalErosion3")),
	  techVerticalErosion3(getEffectTech(effect, "VerticalErosion3")),
	  techHorizontalDilation3(getEffectTech(effect, "HorizontalDilation3")),
	  techVerticalDilation3(getEffectTech(effect, "VerticalDilation3")),
	  techDraw(getEffectTech(effect, "Draw")),
	  techDrawGrayscale(getEffectTech(effect, "DrawGrayscale"))
{
}

void DrawingEffect::applyConvertGrayscalePass(std::uint32_t width, std::uint32_t height, gs_texture_t *targetTexture,
					      gs_texture_t *sourceTexture) noexcept
{
	RenderingGuard guard(targetTexture);

	gs_technique_t *tech = techConvertGrayscale;
	std::size_t passes = gs_technique_begin(tech);
	for (std::size_t i = 0; i < passes; i++) {
		if (gs_technique_begin_pass(tech, i)) {
			gs_effect_set_texture(textureImage, sourceTexture);

			gs_draw_sprite(nullptr, 0, width, height);
			gs_technique_end_pass(tech);
		}
	}
	gs_technique_end(tech);
}

void DrawingEffect::applyMedianFilteringPass(std::uint32_t width, std::uint32_t height, float texelWidth,
					     float texelHeight, gs_texture_t *targetTexture,
					     gs_texture_t *targetIntermediateTexture,
					     gs_texture_t *sourceTexture) noexcept
{
	{
		RenderingGuard guard(targetIntermediateTexture);
		gs_technique_t *tech = techHorizontalMedian3;
		std::size_t passes = gs_technique_begin(tech);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, sourceTexture);

				gs_effect_set_float(floatTexelWidth, texelWidth);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
	}

	{
		RenderingGuard guard(targetTexture);
		gs_technique_t *tech = techVerticalMedian3;
		std::size_t passes = gs_technique_begin(tech);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, sourceTexture);

				gs_effect_set_float(floatTexelHeight, texelHeight);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
	}
}

void DrawingEffect::applyMotionAdaptiveFilteringPass(std::uint32_t width, std::uint32_t height, float texelWidth,
						     float texelHeight, float strength, float motionThreshold,
						     gs_texture_t *targetTexture, gs_texture_t *targetMotionMapTexture,
						     gs_texture_t *targetIntermediateTexture,
						     gs_texture_t *sourceTexture,
						     gs_texture_t *sourcePreviousGrayscaleTexture) noexcept
{
	{
		RenderingGuard guard(targetIntermediateTexture);
		gs_technique_t *tech = techCalculateHorizontalMotionMap3;
		std::size_t passes = gs_technique_begin(tech);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, sourceTexture);
				gs_effect_set_texture(textureImage1, sourcePreviousGrayscaleTexture);

				gs_effect_set_float(floatTexelWidth, texelWidth);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
	}

	{
		RenderingGuard guard(targetMotionMapTexture);
		gs_technique_t *tech = techCalculateVerticalMotionMap3;
		std::size_t passes = gs_technique_begin(tech);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, targetIntermediateTexture);

				gs_effect_set_float(floatTexelHeight, texelHeight);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
	}

	{
		RenderingGuard guard(targetTexture);
		gs_technique_t *tech = techMotionAdaptiveFiltering;
		std::size_t passes = gs_technique_begin(tech);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, sourceTexture);
				gs_effect_set_texture(textureImage1, sourcePreviousGrayscaleTexture);
				gs_effect_set_texture(textureMotionMap, targetMotionMapTexture);

				gs_effect_set_float(floatStrength, strength);
				gs_effect_set_float(floatMotionThreshold, motionThreshold);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
	}
}

void DrawingEffect::applySobelPass(std::uint32_t width, std::uint32_t height, float texelWidth, float texelHeight,
				   gs_texture_t *targetTexture, gs_texture_t *sourceTexture) noexcept
{
	RenderingGuard guard(targetTexture);
	gs_technique_t *tech = techApplySobel;
	std::size_t passes = gs_technique_begin(tech);
	for (std::size_t i = 0; i < passes; i++) {
		if (gs_technique_begin_pass(tech, i)) {
			gs_effect_set_texture(textureImage, sourceTexture);

			gs_effect_set_float(floatTexelWidth, texelWidth);
			gs_effect_set_float(floatTexelHeight, texelHeight);

			gs_draw_sprite(nullptr, 0, width, height);
			gs_technique_end_pass(tech);
		}
	}
}

void DrawingEffect::applyFinalizeSobelMagnitudePass(std::uint32_t width, std::uint32_t height, bool useLog,
						    float scalingFactor, gs_texture_t *targetTexture,
						    gs_texture_t *sourceTexture) noexcept
{
	RenderingGuard guard(targetTexture);
	gs_technique_t *tech = techFinalizeSobelMagnitude;
	std::size_t passes = gs_technique_begin(tech);
	for (std::size_t i = 0; i < passes; i++) {
		if (gs_technique_begin_pass(tech, i)) {
			gs_effect_set_texture(textureImage, sourceTexture);

			gs_effect_set_bool(boolUseLog, useLog);
			gs_effect_set_float(floatScalingFactor, scalingFactor);

			gs_draw_sprite(nullptr, 0, width, height);
			gs_technique_end_pass(tech);
		}
	}
}

void DrawingEffect::applyMorphologyPass(std::uint32_t width, std::uint32_t height, gs_technique_t *horizontalTechnique,
					gs_technique_t *verticalTechnique, float texelWidth, float texelHeight,
					gs_texture_t *targetTexture, gs_texture_t *targetIntermediateTexture,
					gs_texture_t *sourceTexture) noexcept
{
	{
		RenderingGuard guard(targetIntermediateTexture);
		gs_technique_t *tech = horizontalTechnique;
		std::size_t passes = gs_technique_begin(tech);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, sourceTexture);

				gs_effect_set_float(floatTexelWidth, texelWidth);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
	}

	{
		RenderingGuard guard(targetTexture);
		gs_technique_t *tech = verticalTechnique;
		std::size_t passes = gs_technique_begin(tech);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, targetIntermediateTexture);

				gs_effect_set_float(floatTexelHeight, texelHeight);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
	}
}

void DrawingEffect::drawWithBlending(std::uint32_t width, std::uint32_t height, gs_texture_t *targetTexture,
				     gs_texture_t *sourceTexture) noexcept
{
	RenderingGuard guard(targetTexture, GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	gs_technique_t *tech = techDraw;
	size_t passes = gs_technique_begin(tech);
	for (size_t i = 0; i < passes; i++) {
		if (gs_technique_begin_pass(tech, i)) {
			gs_effect_set_texture(textureImage, sourceTexture);

			gs_draw_sprite(nullptr, 0, width, height);
			gs_technique_end_pass(tech);
		}
	}
	gs_technique_end(tech);
}

void DrawingEffect::drawColoredImage(std::uint32_t width, std::uint32_t height, gs_texture_t *targetTexture,
				     gs_texture_t *sourceTexture) noexcept
{
	RenderingGuard guard(targetTexture);

	gs_technique_t *tech = techDraw;
	size_t passes = gs_technique_begin(tech);
	for (size_t i = 0; i < passes; i++) {
		if (gs_technique_begin_pass(tech, i)) {
			gs_effect_set_texture(textureImage, sourceTexture);

			gs_draw_sprite(nullptr, 0, width, height);
			gs_technique_end_pass(tech);
		}
	}
	gs_technique_end(tech);
}

void DrawingEffect::drawGrayscaleTexture(std::uint32_t width, std::uint32_t height, gs_texture_t *targetTexture,
					 gs_texture_t *sourceTexture) noexcept
{
	RenderingGuard guard(targetTexture);

	gs_technique_t *tech = techDrawGrayscale;
	size_t passes = gs_technique_begin(tech);
	for (size_t i = 0; i < passes; i++) {
		if (gs_technique_begin_pass(tech, i)) {
			gs_effect_set_texture(textureImage, sourceTexture);

			gs_draw_sprite(nullptr, 0, width, height);
			gs_technique_end_pass(tech);
		}
	}
	gs_technique_end(tech);
}

} // namespace obs_showdraw
} // namespace kaito_tokyo

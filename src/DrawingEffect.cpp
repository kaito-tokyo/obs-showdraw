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
		slog(LOG_ERROR) << "Effect parameter " << name << " not found";
		throw std::runtime_error("Effect parameter not found");
	}

	return param;
}

gs_technique_t *getEffectTech(const unique_gs_effect_t &effect, const char *name)
{
	gs_technique_t *tech = gs_effect_get_technique(effect.get(), name);

	if (!tech) {
		slog(LOG_ERROR) << "Effect technique " << name << " not found";
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
	  techExtractLuminance(getEffectTech(effect, "ExtractLuminance")),
	  techHorizontalMedian3(getEffectTech(effect, "HorizontalMedian3")),
	  techHorizontalMedian5(getEffectTech(effect, "HorizontalMedian5")),
	  techHorizontalMedian7(getEffectTech(effect, "HorizontalMedian7")),
	  techHorizontalMedian9(getEffectTech(effect, "HorizontalMedian9")),
	  techVerticalMedian3(getEffectTech(effect, "VerticalMedian3")),
	  techVerticalMedian5(getEffectTech(effect, "VerticalMedian5")),
	  techVerticalMedian7(getEffectTech(effect, "VerticalMedian7")),
	  techVerticalMedian9(getEffectTech(effect, "VerticalMedian9")),
	  techCalculateHorizontalMotionMap(getEffectTech(effect, "CalculateHorizontalMotionMap")),
	  techCalculateVerticalMotionMap(getEffectTech(effect, "CalculateVerticalMotionMap")),
	  techMotionAdaptiveFiltering(getEffectTech(effect, "MotionAdaptiveFiltering")),
	  techApplySobel(getEffectTech(effect, "ApplySobel")),
	  techFinalizeSobelMagnitude(getEffectTech(effect, "FinalizeSobelMagnitude")),
	  techSuppressNonMaximum(getEffectTech(effect, "SuppressNonMaximum")),
	  techHysteresisClassify(getEffectTech(effect, "HysteresisClassify")),
	  techHysteresisPropagate(getEffectTech(effect, "HysteresisPropagate")),
	  techHysteresisFinalize(getEffectTech(effect, "HysteresisFinalize")),
	  techHorizontalErosion(getEffectTech(effect, "HorizontalErosion")),
	  techVerticalErosion(getEffectTech(effect, "VerticalErosion")),
	  techHorizontalDilation(getEffectTech(effect, "HorizontalDilation")),
	  techVerticalDilation(getEffectTech(effect, "VerticalDilation")),
	  techDraw(getEffectTech(effect, "Draw"))
{
}

void DrawingEffect::applyLuminanceExtractionPass(std::uint32_t width, std::uint32_t height, gs_texture_t *targetTexture,
						 gs_texture_t *sourceTexture) noexcept
{
	RenderingGuard guard(targetTexture);

	gs_technique_t *tech = techExtractLuminance;
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
					     float texelHeight, int kernelSize, gs_texture_t *targetTexture,
					     gs_texture_t *targetIntermediateTexture,
					     gs_texture_t *sourceTexture) noexcept
{
	gs_technique_t *techHorizontal, *techVertical;
	switch (kernelSize) {
	case 3:
		techHorizontal = techHorizontalMedian3;
		techVertical = techVerticalMedian3;
		break;
	case 5:
		techHorizontal = techHorizontalMedian5;
		techVertical = techVerticalMedian5;
		break;
	case 7:
		techHorizontal = techHorizontalMedian7;
		techVertical = techVerticalMedian7;
		break;
	case 9:
		techHorizontal = techHorizontalMedian9;
		techVertical = techVerticalMedian9;
		break;
	default:
		obs_log(LOG_WARNING, "Invalid median filtering kernel size: %d", kernelSize);
		gs_copy_texture(targetTexture, sourceTexture);
		return;
	}

	{
		RenderingGuard guard(targetIntermediateTexture);
		gs_technique_t *tech = techHorizontal;
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
		gs_technique_t *tech = techVertical;
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
						     float texelHeight, int kernelSize, float strength,
						     float motionThreshold, gs_texture_t *targetTexture,
						     gs_texture_t *targetMotionMapTexture,
						     gs_texture_t *targetIntermediateTexture,
						     gs_texture_t *sourceTexture,
						     gs_texture_t *sourcePreviousLuminanceTexture) noexcept
{
	{
		RenderingGuard guard(targetIntermediateTexture);
		gs_technique_t *tech = techCalculateHorizontalMotionMap;
		std::size_t passes = gs_technique_begin(tech);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, sourceTexture);
				gs_effect_set_texture(textureImage1, sourcePreviousLuminanceTexture);

				gs_effect_set_float(floatTexelWidth, texelWidth);
				gs_effect_set_int(intKernelSize, kernelSize);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(tech);
			}
		}
		gs_technique_end(tech);
	}

	{
		RenderingGuard guard(targetMotionMapTexture);
		gs_technique_t *tech = techCalculateVerticalMotionMap;
		std::size_t passes = gs_technique_begin(tech);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, targetIntermediateTexture);

				gs_effect_set_float(floatTexelHeight, texelHeight);
				gs_effect_set_int(intKernelSize, kernelSize);

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
				gs_effect_set_texture(textureImage1, sourcePreviousLuminanceTexture);
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

void DrawingEffect::applySuppressNonMaximumPass(std::uint32_t width, std::uint32_t height, float texelWidth,
						float texelHeight, gs_texture_t *targetTexture,
						gs_texture_t *sourceTexture) noexcept
{
	RenderingGuard guard(targetTexture);
	gs_technique_t *tech = techSuppressNonMaximum;
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
	gs_technique_end(tech);
}

void DrawingEffect::applyHysteresisClassifyPass(std::uint32_t width, std::uint32_t height, float texelWidth,
						float texelHeight, float highThreshold, float lowThreshold,
						gs_texture_t *targetTexture, gs_texture_t *sourceTexture) noexcept
{
	RenderingGuard guard(targetTexture);
	gs_technique_t *tech = techHysteresisClassify;
	std::size_t passes = gs_technique_begin(tech);
	for (std::size_t i = 0; i < passes; i++) {
		if (gs_technique_begin_pass(tech, i)) {
			gs_effect_set_texture(textureImage, sourceTexture);

			gs_effect_set_float(floatTexelWidth, texelWidth);
			gs_effect_set_float(floatTexelHeight, texelHeight);
			gs_effect_set_float(floatHighThreshold, highThreshold);
			gs_effect_set_float(floatLowThreshold, lowThreshold);

			gs_draw_sprite(nullptr, 0, width, height);
			gs_technique_end_pass(tech);
		}
	}
	gs_technique_end(tech);
}

void DrawingEffect::applyHysteresisPropagatePass(std::uint32_t width, std::uint32_t height, float texelWidth,
						 float texelHeight, gs_texture_t *targetTexture,
						 gs_texture_t *sourceTexture) noexcept
{
	RenderingGuard guard(targetTexture);
	gs_technique_t *tech = techHysteresisPropagate;
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
	gs_technique_end(tech);
}

void DrawingEffect::applyHysteresisFinalizePass(std::uint32_t width, std::uint32_t height, float texelWidth,
						float texelHeight, gs_texture_t *targetTexture,
						gs_texture_t *sourceTexture) noexcept
{
	RenderingGuard guard(targetTexture);
	gs_technique_t *tech = techHysteresisFinalize;
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
	gs_technique_end(tech);
}

void DrawingEffect::applyMorphologyPass(std::uint32_t width, std::uint32_t height, gs_technique_t *horizontalTechnique,
					gs_technique_t *verticalTechnique, float texelWidth, float texelHeight,
					int kernelSize, gs_texture_t *targetTexture,
					gs_texture_t *targetIntermediateTexture, gs_texture_t *sourceTexture) noexcept
{
	{
		RenderingGuard guard(targetIntermediateTexture);
		gs_technique_t *tech = horizontalTechnique;
		std::size_t passes = gs_technique_begin(tech);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(tech, i)) {
				gs_effect_set_texture(textureImage, sourceTexture);

				gs_effect_set_float(floatTexelWidth, texelWidth);
				gs_effect_set_int(intKernelSize, kernelSize);

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
				gs_effect_set_int(intKernelSize, kernelSize);

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

void DrawingEffect::drawFinalImage(std::uint32_t width, std::uint32_t height, gs_texture_t *targetTexture,
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

} // namespace obs_showdraw
} // namespace kaito_tokyo

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

inline gs_eparam_t *getEffectParam(const unique_gs_effect_t &effect, const char *name)
{
	gs_eparam_t *param = gs_effect_get_param_by_name(effect.get(), name);

	if (!param) {
		slog(LOG_ERROR) << "Effect parameter " << name << " not found";
		throw std::runtime_error("Effect parameter not found");
	}

	return param;
}

inline gs_technique_t *getEffectTech(const unique_gs_effect_t &effect, const char *name)
{
	gs_technique_t *tech = gs_effect_get_technique(effect.get(), name);

	if (!tech) {
		slog(LOG_ERROR) << "Effect technique " << name << " not found";
		throw std::runtime_error("Effect technique not found");
	}

	return tech;
}

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

void DrawingEffect::applyEffectPass(gs_technique_t *technique, gs_texture_t *sourceTexture) noexcept
{
	size_t passes = gs_technique_begin(technique);
	for (size_t i = 0; i < passes; i++) {
		if (gs_technique_begin_pass(technique, i)) {
			gs_draw_sprite(sourceTexture, 0, 0, 0);
			gs_technique_end_pass(technique);
		}
	}
	gs_technique_end(technique);
}

void DrawingEffect::applyLuminanceExtractionPass(gs_texture_t *targetTexture, gs_texture_t *sourceTexture) noexcept
{
	gs_set_render_target(targetTexture, nullptr);

	gs_effect_set_texture(textureImage, sourceTexture);

	applyEffectPass(techExtractLuminance, sourceTexture);
}

void DrawingEffect::applyMedianFilteringPass(float texelWidth, float texelHeight, int kernelSize,
					     gs_texture_t *targetTexture, gs_texture_t *targetIntermediateTexture,
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

	gs_set_render_target(targetIntermediateTexture, nullptr);

	gs_effect_set_texture(textureImage, sourceTexture);

	gs_effect_set_float(floatTexelWidth, texelWidth);
	gs_effect_set_int(intKernelSize, kernelSize);

	applyEffectPass(techHorizontal, sourceTexture);

	gs_set_render_target(targetTexture, nullptr);

	gs_effect_set_texture(textureImage, targetIntermediateTexture);

	gs_effect_set_float(floatTexelHeight, texelHeight);
	gs_effect_set_int(intKernelSize, kernelSize);

	applyEffectPass(techVertical, targetIntermediateTexture);
}

void DrawingEffect::applyMotionAdaptiveFilteringPass(float texelWidth, float texelHeight, int kernelSize,
						     float strength, float motionThreshold, gs_texture_t *targetTexture,
						     gs_texture_t *targetMotionMapTexture,
						     gs_texture_t *targetIntermediateTexture,
						     gs_texture_t *sourceTexture,
						     gs_texture_t *sourcePreviousLuminanceTexture) noexcept
{
	// Calculate horizontal motion map
	gs_set_render_target(targetIntermediateTexture, nullptr);

	gs_effect_set_texture(textureImage, sourceTexture);
	gs_effect_set_texture(textureImage1, sourcePreviousLuminanceTexture);

	gs_effect_set_float(floatTexelWidth, texelWidth);
	gs_effect_set_int(intKernelSize, kernelSize);

	applyEffectPass(techCalculateHorizontalMotionMap, sourceTexture);

	// Calculate vertical motion map
	gs_set_render_target(targetMotionMapTexture, nullptr);

	gs_effect_set_texture(textureImage, targetIntermediateTexture);

	gs_effect_set_float(floatTexelHeight, texelHeight);
	gs_effect_set_int(intKernelSize, kernelSize);

	applyEffectPass(techCalculateVerticalMotionMap, targetIntermediateTexture);

	// Apply motion adaptive filtering
	gs_set_render_target(targetTexture, nullptr);

	gs_effect_set_texture(textureImage, sourceTexture);
	gs_effect_set_texture(textureImage1, sourcePreviousLuminanceTexture);
	gs_effect_set_texture(textureMotionMap, targetMotionMapTexture);

	gs_effect_set_float(floatStrength, strength);
	gs_effect_set_float(floatMotionThreshold, motionThreshold);

	applyEffectPass(techMotionAdaptiveFiltering, sourceTexture);
}

void DrawingEffect::applySobelPass(float texelWidth, float texelHeight, gs_texture_t *targetTexture,
				   gs_texture_t *sourceTexture) noexcept
{
	gs_set_render_target(targetTexture, nullptr);

	gs_effect_set_texture(textureImage, sourceTexture);

	gs_effect_set_float(floatTexelWidth, texelWidth);
	gs_effect_set_float(floatTexelHeight, texelHeight);

	applyEffectPass(techApplySobel, sourceTexture);
}

void DrawingEffect::applyFinalizeSobelMagnitudePass(bool useLog, float scalingFactor, gs_texture_t *targetTexture,
						    gs_texture_t *sourceTexture) noexcept
{
	gs_set_render_target(targetTexture, nullptr);

	gs_effect_set_texture(textureImage, sourceTexture);

	gs_effect_set_bool(boolUseLog, useLog);
	gs_effect_set_float(floatScalingFactor, scalingFactor);

	applyEffectPass(techFinalizeSobelMagnitude, sourceTexture);
}

void DrawingEffect::applySuppressNonMaximumPass(float texelWidth, float texelHeight, gs_texture_t *targetTexture,
						gs_texture_t *sourceTexture) noexcept
{
	gs_set_render_target(targetTexture, nullptr);

	gs_effect_set_texture(textureImage, sourceTexture);

	gs_effect_set_float(floatTexelWidth, texelWidth);
	gs_effect_set_float(floatTexelHeight, texelHeight);

	applyEffectPass(techSuppressNonMaximum, sourceTexture);
}

void DrawingEffect::applyHysteresisClassifyPass(float texelWidth, float texelHeight, float highThreshold,
						float lowThreshold, gs_texture_t *targetTexture,
						gs_texture_t *sourceTexture) noexcept
{
	gs_set_render_target(targetTexture, nullptr);

	gs_effect_set_texture(textureImage, sourceTexture);

	gs_effect_set_float(floatTexelWidth, texelWidth);
	gs_effect_set_float(floatTexelHeight, texelHeight);
	gs_effect_set_float(floatHighThreshold, highThreshold);
	gs_effect_set_float(floatLowThreshold, lowThreshold);

	applyEffectPass(techHysteresisClassify, sourceTexture);
}

void DrawingEffect::applyHysteresisPropagatePass(float texelWidth, float texelHeight, gs_texture_t *targetTexture,
						 gs_texture_t *sourceTexture) noexcept
{
	gs_set_render_target(targetTexture, nullptr);

	gs_effect_set_texture(textureImage, sourceTexture);

	gs_effect_set_float(floatTexelWidth, texelWidth);
	gs_effect_set_float(floatTexelHeight, texelHeight);

	applyEffectPass(techHysteresisPropagate, sourceTexture);
}

void DrawingEffect::applyHysteresisFinalizePass(float texelWidth, float texelHeight, gs_texture_t *targetTexture,
						gs_texture_t *sourceTexture) noexcept
{
	gs_set_render_target(targetTexture, nullptr);

	gs_effect_set_texture(textureImage, sourceTexture);

	gs_effect_set_float(floatTexelWidth, texelWidth);
	gs_effect_set_float(floatTexelHeight, texelHeight);

	applyEffectPass(techHysteresisFinalize, sourceTexture);
}

void DrawingEffect::applyMorphologyPass(gs_technique_t *horizontalTechnique, gs_technique_t *verticalTechnique,
					float texelWidth, float texelHeight, int kernelSize,
					gs_texture_t *targetTexture, gs_texture_t *targetIntermediateTexture,
					gs_texture_t *sourceTexture) noexcept
{
	// Horizontal pass
	gs_set_render_target(targetIntermediateTexture, nullptr);

	gs_effect_set_texture(textureImage, sourceTexture);

	gs_effect_set_float(floatTexelWidth, texelWidth);
	gs_effect_set_int(intKernelSize, kernelSize);

	applyEffectPass(horizontalTechnique, sourceTexture);

	// Vertical pass
	gs_set_render_target(targetTexture, nullptr);

	gs_effect_set_texture(textureImage, targetIntermediateTexture);

	gs_effect_set_float(floatTexelHeight, texelHeight);
	gs_effect_set_int(intKernelSize, kernelSize);

	applyEffectPass(verticalTechnique, targetIntermediateTexture);
}

void DrawingEffect::drawFinalImage(gs_texture_t *targetTexture, gs_texture_t *sourceTexture) noexcept
{
	gs_texture_t *previousRenderTarget = gs_get_render_target();

	// vec4 zero{0.0f, 0.0f, 0.0f, 0.0f};
	// gs_clear(GS_CLEAR_COLOR, &zero, 1.0f, 0);

	gs_set_render_target(targetTexture, nullptr);

	while (gs_effect_loop(effect.get(), "Draw")) {
		gs_effect_set_texture(textureImage, sourceTexture);
		gs_draw_sprite(sourceTexture, 0, 10, 10);
	}

	gs_set_render_target(previousRenderTarget, nullptr);
}

void DrawingEffect::drawFinalImage(std::uint32_t width, std::uint32_t height, gs_texture_t *targetTexture, gs_texture_t *sourceTexture) noexcept
{
	gs_texture_t *previousRenderTarget = gs_get_render_target();

	gs_set_render_target(targetTexture, nullptr);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

	vec4 zero{0.0f, 0.0f, 0.0f, 1.0f};
	gs_clear(GS_CLEAR_COLOR, &zero, 1.0f, 0);

	while (gs_effect_loop(effect.get(), "Draw")) {
		gs_effect_set_texture(textureImage, sourceTexture);
		gs_draw_sprite(nullptr, 0, width, height);
	}

	gs_blend_state_pop();

	gs_set_render_target(previousRenderTarget, nullptr);
}

} // namespace obs_showdraw
} // namespace kaito_tokyo

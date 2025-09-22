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

#include <cstddef>
#include <cstdint>

#include <obs.h>

#include "../BridgeUtils/GsUnique.hpp"
#include "../BridgeUtils/ILogger.hpp"
#include "../BridgeUtils/ObsUnique.hpp"

namespace KaitoTokyo {
namespace ShowDraw {

namespace MainEffectDetail {

inline gs_eparam_t *getEffectParam(const BridgeUtils::unique_gs_effect_t &effect, const char *name)
{
	gs_eparam_t *param = gs_effect_get_param_by_name(effect.get(), name);
	if (!param) {
		throw std::runtime_error("Effect parameter not found");
	}
	return param;
}

inline gs_technique_t *getEffectTech(const BridgeUtils::unique_gs_effect_t &effect, const char *name)
{
	gs_technique_t *tech = gs_effect_get_technique(effect.get(), name);
	if (!tech) {
		throw std::runtime_error("Effect technique not found");
	}
	return tech;
}
} // namespace MainEffectDetail

struct TransformStateGuard {
	TransformStateGuard()
	{
		gs_viewport_push();
		gs_projection_push();
		gs_matrix_push();
	}
	~TransformStateGuard()
	{
		gs_matrix_pop();
		gs_projection_pop();
		gs_viewport_pop();
	}
};

struct RenderTargetGuard {
	gs_texture_t *previousRenderTarget;
	gs_zstencil_t *previousZStencil;
	gs_color_space previousColorSpace;

	RenderTargetGuard()
		: previousRenderTarget(gs_get_render_target()),
		  previousZStencil(gs_get_zstencil_target()),
		  previousColorSpace(gs_get_color_space())
	{
	}

	~RenderTargetGuard()
	{
		gs_set_render_target_with_color_space(previousRenderTarget, previousZStencil, previousColorSpace);
	}
};

class MainEffect {
public:
	const BridgeUtils::unique_gs_effect_t effect;

	gs_eparam_t *const textureImage;
	gs_eparam_t *const textureImage1;
	gs_eparam_t *const floatTexelWidth;
	gs_eparam_t *const floatTexelHeight;
	gs_eparam_t *const textureMotionMap;
	gs_eparam_t *const floatStrength;
	gs_eparam_t *const floatMotionThreshold;
	gs_eparam_t *const boolUseLog;
	gs_eparam_t *const floatScalingFactor;

	gs_technique_t *const techDraw;
	gs_technique_t *const techDrawGrayscale;

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

	explicit MainEffect(const BridgeUtils::unique_bfree_char_t &effectPath)
		: effect(BridgeUtils::make_unique_gs_effect_from_file(effectPath)),
		  textureImage(MainEffectDetail::getEffectParam(effect, "image")),
		  textureImage1(MainEffectDetail::getEffectParam(effect, "image1")),
		  floatTexelWidth(MainEffectDetail::getEffectParam(effect, "texelWidth")),
		  floatTexelHeight(MainEffectDetail::getEffectParam(effect, "texelHeight")),
		  textureMotionMap(MainEffectDetail::getEffectParam(effect, "motionMap")),
		  floatStrength(MainEffectDetail::getEffectParam(effect, "strength")),
		  floatMotionThreshold(MainEffectDetail::getEffectParam(effect, "motionThreshold")),
		  boolUseLog(MainEffectDetail::getEffectParam(effect, "useLog")),
		  floatScalingFactor(MainEffectDetail::getEffectParam(effect, "scalingFactor")),
		  techDraw(MainEffectDetail::getEffectTech(effect, "Draw")),
		  techDrawGrayscale(MainEffectDetail::getEffectTech(effect, "DrawGrayscale")),
		  techConvertGrayscale(MainEffectDetail::getEffectTech(effect, "ConvertGrayscale")),
		  techHorizontalMedian3(MainEffectDetail::getEffectTech(effect, "HorizontalMedian3")),
		  techVerticalMedian3(MainEffectDetail::getEffectTech(effect, "VerticalMedian3")),
		  techCalculateHorizontalMotionMap3(
			  MainEffectDetail::getEffectTech(effect, "CalculateHorizontalMotionMap3")),
		  techCalculateVerticalMotionMap3(
			  MainEffectDetail::getEffectTech(effect, "CalculateVerticalMotionMap3")),
		  techMotionAdaptiveFiltering(MainEffectDetail::getEffectTech(effect, "MotionAdaptiveFiltering")),
		  techApplySobel(MainEffectDetail::getEffectTech(effect, "ApplySobel")),
		  techFinalizeSobelMagnitude(MainEffectDetail::getEffectTech(effect, "FinalizeSobelMagnitude")),
		  techHorizontalErosion3(MainEffectDetail::getEffectTech(effect, "HorizontalErosion3")),
		  techVerticalErosion3(MainEffectDetail::getEffectTech(effect, "VerticalErosion3")),
		  techHorizontalDilation3(MainEffectDetail::getEffectTech(effect, "HorizontalDilation3")),
		  techVerticalDilation3(MainEffectDetail::getEffectTech(effect, "VerticalDilation3"))
	{
	}

	MainEffect(const MainEffect &) = delete;
	MainEffect(MainEffect &&) = delete;
	MainEffect &operator=(const MainEffect &) = delete;
	MainEffect &operator=(MainEffect &&) = delete;

	bool drawSource(const BridgeUtils::unique_gs_texture_t &target, obs_source_t *source) const
	{
		const RenderTargetGuard renderTargetGuard;
		const TransformStateGuard transformStateGuard;

		const std::uint32_t width = gs_texture_get_width(target.get());
		const std::uint32_t height = gs_texture_get_height(target.get());

		gs_set_render_target_with_color_space(target.get(), nullptr, GS_CS_SRGB);

		if (!obs_source_process_filter_begin(source, GS_BGRA, OBS_ALLOW_DIRECT_RENDERING)) {
			throw std::runtime_error("Failed to begin processing filter");
		}

		gs_set_viewport(0, 0, width, height);
		gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);
		gs_matrix_identity();

		obs_source_process_filter_end(source, effect.get(), width, height);
		return true;
	}

	void drawTexture(const BridgeUtils::unique_gs_texture_t &source, std::uint32_t width,
			 std::uint32_t height) const noexcept
	{
		const std::size_t passes = gs_technique_begin(techDraw);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(techDraw, i)) {
				gs_effect_set_texture(textureImage, source.get());

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techDraw);
			}
		}
		gs_technique_end(techDraw);
	}

	void drawTexture(const BridgeUtils::unique_gs_texture_t &source) const noexcept
	{
		const std::uint32_t width = gs_texture_get_width(source.get());
		const std::uint32_t height = gs_texture_get_height(source.get());
		drawTexture(source, width, height);
	}

	void drawGrayscaleTexture(const BridgeUtils::unique_gs_texture_t &source) const noexcept
	{
		const std::uint32_t width = gs_texture_get_width(source.get());
		const std::uint32_t height = gs_texture_get_height(source.get());

		const std::size_t passes = gs_technique_begin(techDrawGrayscale);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(techDrawGrayscale, i)) {
				gs_effect_set_texture(textureImage, source.get());

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techDrawGrayscale);
			}
		}
		gs_technique_end(techDrawGrayscale);
	}

	void applyConvertToGrayscale(const BridgeUtils::unique_gs_texture_t &target,
				     const BridgeUtils::unique_gs_texture_t &source) const noexcept
	{
		const RenderTargetGuard renderTargetGuard;
		const TransformStateGuard transformStateGuard;

		std::uint32_t width = gs_texture_get_width(target.get());
		std::uint32_t height = gs_texture_get_height(target.get());

		gs_set_viewport(0, 0, width, height);
		gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
		gs_matrix_identity();

		gs_set_render_target_with_color_space(target.get(), nullptr, GS_CS_SRGB);
		const std::size_t passes = gs_technique_begin(techConvertGrayscale);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(techConvertGrayscale, i)) {
				gs_effect_set_texture(textureImage, source.get());

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techConvertGrayscale);
			}
		}
		gs_technique_end(techConvertGrayscale);
	}

	void applyMedianFilter(const BridgeUtils::unique_gs_texture_t &target,
			       const BridgeUtils::unique_gs_texture_t &intermediate,
			       const BridgeUtils::unique_gs_texture_t &source) const noexcept
	{
		const RenderTargetGuard renderTargetGuard;
		const TransformStateGuard transformStateGuard;

		const std::uint32_t width = gs_texture_get_width(target.get());
		const std::uint32_t height = gs_texture_get_height(target.get());
		const float texelWidth = 1.0f / static_cast<float>(width);
		const float texelHeight = 1.0f / static_cast<float>(height);

		gs_set_viewport(0, 0, width, height);
		gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
		gs_matrix_identity();

		// Horizontal
		gs_set_render_target_with_color_space(intermediate.get(), nullptr, GS_CS_SRGB);
		const std::size_t passesHorizontal = gs_technique_begin(techHorizontalMedian3);
		for (std::size_t i = 0; i < passesHorizontal; i++) {
			if (gs_technique_begin_pass(techHorizontalMedian3, i)) {
				gs_effect_set_texture(textureImage, source.get());

				gs_effect_set_float(floatTexelWidth, texelWidth);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techHorizontalMedian3);
			}
		}
		gs_technique_end(techHorizontalMedian3);

		// Vertical
		gs_set_render_target_with_color_space(target.get(), nullptr, GS_CS_SRGB);
		const std::size_t passesVertical = gs_technique_begin(techVerticalMedian3);
		for (std::size_t i = 0; i < passesVertical; i++) {
			if (gs_technique_begin_pass(techVerticalMedian3, i)) {
				gs_effect_set_texture(textureImage, intermediate.get());

				gs_effect_set_float(floatTexelHeight, texelHeight);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techVerticalMedian3);
			}
		}
		gs_technique_end(techVerticalMedian3);
	}

	void applyMotionAdaptiveFilter(const KaitoTokyo::BridgeUtils::unique_gs_texture_t &target,
				       const KaitoTokyo::BridgeUtils::unique_gs_texture_t &motionMap,
				       const KaitoTokyo::BridgeUtils::unique_gs_texture_t &intermediate,
				       const KaitoTokyo::BridgeUtils::unique_gs_texture_t &source,
				       const KaitoTokyo::BridgeUtils::unique_gs_texture_t &previousGrayscale,
				       float strength, float motionThreshold) const noexcept
	{
		const RenderTargetGuard renderTargetGuard;
		const TransformStateGuard transformStateGuard;

		const std::uint32_t width = gs_texture_get_width(target.get());
		const std::uint32_t height = gs_texture_get_height(target.get());
		const float texelWidth = 1.0f / static_cast<float>(width);
		const float texelHeight = 1.0f / static_cast<float>(height);

		gs_set_viewport(0, 0, width, height);
		gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
		gs_matrix_identity();

		// Horizontal motion map calculation pass
		gs_set_render_target_with_color_space(intermediate.get(), nullptr, GS_CS_SRGB);
		const std::size_t passesHorizontal = gs_technique_begin(techCalculateHorizontalMotionMap3);
		for (std::size_t i = 0; i < passesHorizontal; i++) {
			if (gs_technique_begin_pass(techCalculateHorizontalMotionMap3, i)) {
				gs_effect_set_texture(textureImage, source.get());

				gs_effect_set_texture(textureImage1, previousGrayscale.get());

				gs_effect_set_float(floatTexelWidth, texelWidth);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techCalculateHorizontalMotionMap3);
			}
		}
		gs_technique_end(techCalculateHorizontalMotionMap3);

		// Vertical motion map calculation pass
		gs_set_render_target_with_color_space(motionMap.get(), nullptr, GS_CS_SRGB);
		const std::size_t passesVertical = gs_technique_begin(techCalculateVerticalMotionMap3);
		for (std::size_t i = 0; i < passesVertical; i++) {
			if (gs_technique_begin_pass(techCalculateVerticalMotionMap3, i)) {
				gs_effect_set_texture(textureImage, intermediate.get());

				gs_effect_set_float(floatTexelHeight, texelHeight);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techCalculateVerticalMotionMap3);
			}
		}
		gs_technique_end(techCalculateVerticalMotionMap3);

		// Motion adaptive filtering pass
		gs_set_render_target_with_color_space(target.get(), nullptr, GS_CS_SRGB);
		const std::size_t passesFilter = gs_technique_begin(techMotionAdaptiveFiltering);
		for (std::size_t i = 0; i < passesFilter; i++) {
			if (gs_technique_begin_pass(techMotionAdaptiveFiltering, i)) {
				gs_effect_set_texture(textureImage, source.get());

				gs_effect_set_texture(textureImage1, previousGrayscale.get());
				gs_effect_set_texture(textureMotionMap, motionMap.get());

				gs_effect_set_float(floatStrength, strength);
				gs_effect_set_float(floatMotionThreshold, motionThreshold);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techMotionAdaptiveFiltering);
			}
		}
		gs_technique_end(techMotionAdaptiveFiltering);
	}

	void applySobel(const KaitoTokyo::BridgeUtils::unique_gs_texture_t &target,
			const KaitoTokyo::BridgeUtils::unique_gs_texture_t &source) const noexcept
	{
		const RenderTargetGuard renderTargetGuard;
		const TransformStateGuard transformStateGuard;

		const std::uint32_t width = gs_texture_get_width(target.get());
		const std::uint32_t height = gs_texture_get_height(target.get());
		const float texelWidth = 1.0f / static_cast<float>(width);
		const float texelHeight = 1.0f / static_cast<float>(height);

		gs_set_viewport(0, 0, width, height);
		gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
		gs_matrix_identity();

		gs_set_render_target_with_color_space(target.get(), nullptr, GS_CS_SRGB);
		const std::size_t passes = gs_technique_begin(techApplySobel);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(techApplySobel, i)) {
				gs_effect_set_texture(textureImage, source.get());

				gs_effect_set_float(floatTexelWidth, texelWidth);
				gs_effect_set_float(floatTexelHeight, texelHeight);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techApplySobel);
			}
		}
		gs_technique_end(techApplySobel);
	}

	void applyFinalizeSobelMagnitude(const KaitoTokyo::BridgeUtils::unique_gs_texture_t &target,
					 const KaitoTokyo::BridgeUtils::unique_gs_texture_t &source, bool useLog,
					 float scalingFactor) const noexcept
	{
		const RenderTargetGuard renderTargetGuard;
		const TransformStateGuard transformStateGuard;

		const std::uint32_t width = gs_texture_get_width(target.get());
		const std::uint32_t height = gs_texture_get_height(target.get());

		gs_set_viewport(0, 0, width, height);
		gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
		gs_matrix_identity();

		gs_set_render_target_with_color_space(target.get(), nullptr, GS_CS_SRGB);
		const std::size_t passes = gs_technique_begin(techFinalizeSobelMagnitude);
		for (std::size_t i = 0; i < passes; i++) {
			if (gs_technique_begin_pass(techFinalizeSobelMagnitude, i)) {
				gs_effect_set_texture(textureImage, source.get());

				gs_effect_set_bool(boolUseLog, useLog);
				gs_effect_set_float(floatScalingFactor, scalingFactor);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(techFinalizeSobelMagnitude);
			}
		}
		gs_technique_end(techFinalizeSobelMagnitude);
	}

	void applyMorphology(const KaitoTokyo::BridgeUtils::unique_gs_texture_t &target,
			     const KaitoTokyo::BridgeUtils::unique_gs_texture_t &intermediate,
			     const KaitoTokyo::BridgeUtils::unique_gs_texture_t &source,
			     gs_technique_t *horizontalTechnique, gs_technique_t *verticalTechnique) const noexcept
	{
		const RenderTargetGuard renderTargetGuard;
		const TransformStateGuard transformStateGuard;

		const std::uint32_t width = gs_texture_get_width(target.get());
		const std::uint32_t height = gs_texture_get_height(target.get());
		const float texelWidth = 1.0f / static_cast<float>(width);
		const float texelHeight = 1.0f / static_cast<float>(height);

		gs_set_viewport(0, 0, width, height);
		gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
		gs_matrix_identity();

		// Horizontal Pass
		gs_set_render_target_with_color_space(intermediate.get(), nullptr, GS_CS_SRGB);
		const std::size_t passesHorizontal = gs_technique_begin(horizontalTechnique);
		for (std::size_t i = 0; i < passesHorizontal; i++) {
			if (gs_technique_begin_pass(horizontalTechnique, i)) {
				gs_effect_set_texture(textureImage, source.get());
				gs_effect_set_float(floatTexelWidth, texelWidth);
				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(horizontalTechnique);
			}
		}
		gs_technique_end(horizontalTechnique);

		// Vertical Pass
		gs_set_render_target_with_color_space(target.get(), nullptr, GS_CS_SRGB);
		const std::size_t passesVertical = gs_technique_begin(verticalTechnique);
		for (std::size_t i = 0; i < passesVertical; i++) {
			if (gs_technique_begin_pass(verticalTechnique, i)) {
				gs_effect_set_texture(textureImage, intermediate.get());

				gs_effect_set_float(floatTexelHeight, texelHeight);

				gs_draw_sprite(nullptr, 0, width, height);
				gs_technique_end_pass(verticalTechnique);
			}
		}
		gs_technique_end(verticalTechnique);
	}

	void drawRectangle(float x, float y, float w, float h, float thickness, std::uint32_t color) const
	{
		gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_SOLID);
		gs_eparam_t *param = gs_effect_get_param_by_name(effect, "color");

		struct vec4 color_vec;
		vec4_from_rgba(&color_vec, color);
		gs_effect_set_vec4(param, &color_vec);

		while (gs_effect_loop(effect, "Solid")) {
			drawFilledRect(x, y, w, thickness);
			drawFilledRect(x, y + h - thickness, w, thickness);
			drawFilledRect(x, y + thickness, thickness, h - thickness * 2.0f);
			drawFilledRect(x + w - thickness, y + thickness, thickness, h - thickness * 2.0f);
		}
	}

private:
	void drawFilledRect(float x, float y, float w, float h) const
	{
		gs_matrix_push();
		gs_matrix_translate3f(x, y, 0.0f);
		gs_draw_quadf(NULL, 0, w, h);
		gs_matrix_pop();
	}
};

} // namespace ShowDraw
} // namespace KaitoTokyo

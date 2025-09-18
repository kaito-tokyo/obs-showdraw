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

#include <obs.h>
#include <obs-bridge-utils/gs_unique.hpp>

#include "DrawingEffect.hpp"
#include "Preset.hpp"

namespace kaito_tokyo {
namespace obs_showdraw {

class RenderingContext {
public:
    const uint32_t width;
    const uint32_t height;
    const float texelWidth;
    const float texelHeight;

    RenderingContext(uint32_t width, uint32_t height, obs_source_t *filter, DrawingEffect &drawingEffect, const Preset &runningPreset, double sobelMagnitudeFinalizationScalingFactor);
    ~RenderingContext() noexcept;

    void videoRender();

private:
    void renderOriginalImage();
    void renderGrayscale();
    void renderMedianFilter();
    void renderMotionAdaptiveFilter();
    void renderSobel();
    void renderMorphology();
    void drawOutput();

    obs_source_t *const filter;
    DrawingEffect &drawingEffect;
    const Preset &runningPreset;
    const double sobelMagnitudeFinalizationScalingFactor;

    // Textures
    kaito_tokyo::obs_bridge_utils::unique_gs_texture_t bgrxSource;
    kaito_tokyo::obs_bridge_utils::unique_gs_texture_t bgrxTarget;
    kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r8Source;
    kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r8Target;
    kaito_tokyo::obs_bridge_utils::unique_gs_texture_t bgrxTemporary1;
    kaito_tokyo::obs_bridge_utils::unique_gs_texture_t bgrxTemporary2;
    kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r8Temporary1;
    kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r8PreviousGrayscale;
    kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r8MotionMap;
    kaito_tokyo::obs_bridge_utils::unique_gs_texture_t bgrxComplexSobel;
    kaito_tokyo::obs_bridge_utils::unique_gs_texture_t r8FinalSobelMagnitude;
};

} // namespace obs_showdraw
} // namespace kaito_tokyo
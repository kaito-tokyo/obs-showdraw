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

#include <obs-module.h>
#include <plugin-support.h>

#include <math.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

const char *showdraw_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("showdrawName");
}

#define MAX_PREVIOUS_TEXTURES 15

struct showdraw_filter_context {
	obs_source_t *filter;

	const char *effect_path;
	const char *effect_tech;
	float sensitivity_factor;

	gs_effect_t *effect;
	gs_eparam_t *effect_image[MAX_PREVIOUS_TEXTURES];
	gs_eparam_t *effect_sensitivity_factor;
	gs_eparam_t *effect_texel_height;
	gs_eparam_t *effect_texel_width;

	gs_texrender_t *texrender;

	uint32_t height;
	gs_texture_t *previous_textures[MAX_PREVIOUS_TEXTURES];
	uint32_t width;
};

void showdraw_update(void *data, obs_data_t *settings);

void *showdraw_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(settings);

	obs_log(LOG_INFO, "Creating showdraw source");

	struct showdraw_filter_context *context = bzalloc(sizeof(struct showdraw_filter_context));

	context->filter = source;

	context->effect_path = obs_module_file("effects/drawing-emphasizer.effect");
	context->effect_tech = "Draw";
	context->sensitivity_factor = 0.0f;

	context->effect = NULL;
	for (int i = 0; i < MAX_PREVIOUS_TEXTURES; i++) {
		context->effect_image[i] = NULL;
	}
	context->effect_sensitivity_factor = NULL;
	context->effect_texel_height = NULL;
	context->effect_texel_width = NULL;

	context->texrender = NULL;

	context->height = 0;
	for (int i = 0; i < MAX_PREVIOUS_TEXTURES; i++) {
		context->previous_textures[i] = NULL;
	}
	context->width = 0;

	showdraw_update(context, settings);

	return context;
}

void showdraw_destroy(void *data)
{
	obs_log(LOG_INFO, "Destroying showdraw source");

	struct showdraw_filter_context *context = (struct showdraw_filter_context *)data;

	if (context->effect) {
		gs_effect_destroy(context->effect);
		context->effect = NULL;
	}

	bfree(context);
}

void showdraw_get_defaults(obs_data_t *data)
{
	obs_data_set_default_double(data, "sensitivityFactorDb", 0.0);
	obs_data_set_default_string(data, "effectTechnique", "Draw1");
}

obs_properties_t *showdraw_get_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_float_slider(props, "sensitivityFactorDb", obs_module_text("sensitivityFactorDb"), -50.0,
					50.0, 0.01);

	obs_property_t *propEffectTechnique = obs_properties_add_list(props, "effectTechnique", obs_module_text("effectTechnique"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(propEffectTechnique, "1", "Draw1");
	obs_property_list_add_string(propEffectTechnique, "3", "Draw3");
	obs_property_list_add_string(propEffectTechnique, "5", "Draw5");
	obs_property_list_add_string(propEffectTechnique, "7", "Draw7");
	obs_property_list_add_string(propEffectTechnique, "9", "Draw9");
	obs_property_list_add_string(propEffectTechnique, "11", "Draw11");
	obs_property_list_add_string(propEffectTechnique, "13", "Draw13");
	obs_property_list_add_string(propEffectTechnique, "15", "Draw15");

	return props;
}

void showdraw_update(void *data, obs_data_t *settings)
{
	struct showdraw_filter_context *context = (struct showdraw_filter_context *)data;

	context->sensitivity_factor = (float)pow(10.0, obs_data_get_double(settings, "sensitivityFactorDb") / 10.0);
	context->effect_tech = obs_data_get_string(settings, "effectTechnique");
}

void showdraw_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);

	struct showdraw_filter_context *context = (struct showdraw_filter_context *)data;

	obs_source_t *target = obs_filter_get_target(context->filter);

	if (!target) {
		obs_log(LOG_ERROR, "Target source not found");
		obs_source_skip_video_filter(context->filter);
		return;
	}

	uint32_t width = obs_source_get_base_width(target);
	uint32_t height = obs_source_get_base_height(target);

	if (!context->texrender) {
		context->texrender = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
		if (!context->texrender) {
			obs_log(LOG_ERROR, "Failed to create texrender");
			obs_source_skip_video_filter(context->filter);
			return;
		}
	}

	if (context->previous_textures[0] == NULL || context->width != width || context->height != height) {
		for (int i = 0; i < MAX_PREVIOUS_TEXTURES; i++) {
			context->previous_textures[i] = gs_texture_create(width, height, GS_RGBA, 1, NULL, GS_DYNAMIC);
		}

		context->width = width;
		context->height = height;
	}

	gs_texrender_reset(context->texrender);
	obs_log(LOG_INFO, "Resetting texrender %p %u %u", context->texrender, width, height);
	if (!gs_texrender_begin(context->texrender, width, height)) {
		obs_log(LOG_ERROR, "Failed to begin texrender");
		obs_source_skip_video_filter(context->filter);
		return;
	}
	struct vec4 background;
	vec4_zero(&background);
	gs_clear(GS_CLEAR_COLOR, &background, 0.0f, 0);
	gs_ortho(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f);
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	obs_source_video_render(target);
	gs_blend_state_pop();
	gs_texrender_end(context->texrender);

	gs_texture_t *target_texture = gs_texrender_get_texture(context->texrender);
	if (!target_texture) {
		obs_log(LOG_ERROR, "Target texture not found");
		obs_source_skip_video_filter(context->filter);
		return;
	}

	gs_texture_t *latest_texture = context->previous_textures[MAX_PREVIOUS_TEXTURES - 1];
	for (int i = 0; i < MAX_PREVIOUS_TEXTURES - 1; i++) {
		context->previous_textures[i + 1] = context->previous_textures[i];
	}
	gs_copy_texture(latest_texture, target_texture);
	context->previous_textures[0] = latest_texture;

	if (!context->effect) {
		char *error_string = NULL;

		gs_effect_t *effect = gs_effect_create_from_file(context->effect_path, &error_string);
		if (!effect) {
			obs_log(LOG_ERROR, "Error loading effect: %s", error_string);
			bfree(error_string);
			return;
		}

		context->effect = effect;
		context->effect_image[1] = gs_effect_get_param_by_name(context->effect, "image1");
		context->effect_image[2] = gs_effect_get_param_by_name(context->effect, "image2");
		context->effect_image[3] = gs_effect_get_param_by_name(context->effect, "image3");
		context->effect_image[4] = gs_effect_get_param_by_name(context->effect, "image4");
		context->effect_image[5] = gs_effect_get_param_by_name(context->effect, "image5");
		context->effect_image[6] = gs_effect_get_param_by_name(context->effect, "image6");
		context->effect_image[7] = gs_effect_get_param_by_name(context->effect, "image7");
		context->effect_image[8] = gs_effect_get_param_by_name(context->effect, "image8");
		context->effect_image[9] = gs_effect_get_param_by_name(context->effect, "image9");
		context->effect_image[10] = gs_effect_get_param_by_name(context->effect, "image10");
		context->effect_image[11] = gs_effect_get_param_by_name(context->effect, "image11");
		context->effect_image[12] = gs_effect_get_param_by_name(context->effect, "image12");
		context->effect_image[13] = gs_effect_get_param_by_name(context->effect, "image13");
		context->effect_image[14] = gs_effect_get_param_by_name(context->effect, "image14");
		context->effect_texel_width = gs_effect_get_param_by_name(context->effect, "texelWidth");
		context->effect_texel_height = gs_effect_get_param_by_name(context->effect, "texelHeight");
		context->effect_sensitivity_factor = gs_effect_get_param_by_name(context->effect, "sensitivityFactor");

		obs_log(LOG_INFO, "Effect loaded successfully");
	}

	if (!obs_source_process_filter_begin(context->filter, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING)) {
		obs_log(LOG_ERROR, "Could not begin filter processing");
		obs_source_skip_video_filter(context->filter);
		return;
	}

	gs_effect_set_float(context->effect_texel_width, 1.0f / (float)obs_source_get_base_width(target));
	gs_effect_set_float(context->effect_texel_height, 1.0f / (float)obs_source_get_base_width(target));
	gs_effect_set_float(context->effect_sensitivity_factor, context->sensitivity_factor);
	for (int i = 1; i < MAX_PREVIOUS_TEXTURES; i++) {
		gs_effect_set_texture(context->effect_image[i], context->previous_textures[i]);
	}

	obs_source_process_filter_tech_end(context->filter, context->effect, 0, 0, context->effect_tech);
}

struct obs_source_info showdraw_filter = {
	.id = "showdraw",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = showdraw_get_name,
	.create = showdraw_create,
	.destroy = showdraw_destroy,
	.get_defaults = showdraw_get_defaults,
	.get_properties = showdraw_get_properties,
	.update = showdraw_update,
	.video_render = showdraw_video_render,
};

bool obs_module_load(void)
{
	obs_register_source(&showdraw_filter);
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "plugin unloaded");
}

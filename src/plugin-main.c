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

#define EXTRACTION_MODE_DEFAULT 0
#define EXTRACTION_MODE_PASSTHROUGH 1
#define EXTRACTION_MODE_LUMINANCE_EXTRACTION 2

struct showdraw_filter_context {
	obs_source_t *filter;

	long long extraction_mode;

	const char *luminance_extraction_effect_path;
	const char *luminance_extraction_effect_tech;
	gs_effect_t *luminance_extraction_effect;
	gs_eparam_t *luminance_extraction_effect_texel_height;
	gs_eparam_t *luminance_extraction_effect_texel_width;

	const char *effect_path;
	const char *effect_tech;
	float sensitivity_factor;

	gs_effect_t *effect;
	gs_eparam_t *effect_sensitivity_factor;
	gs_eparam_t *effect_texel_height;
	gs_eparam_t *effect_texel_width;
};

void showdraw_update(void *data, obs_data_t *settings);

void *showdraw_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(settings);

	obs_log(LOG_INFO, "Creating showdraw source");

	struct showdraw_filter_context *context = bzalloc(sizeof(struct showdraw_filter_context));

	context->filter = source;

	context->luminance_extraction_effect_path = obs_module_file("effects/luminance-extraction.effect");
	context->luminance_extraction_effect_tech = "Draw";

	context->luminance_extraction_effect = NULL;
	context->luminance_extraction_effect_texel_height = NULL;
	context->luminance_extraction_effect_texel_width = NULL;

	context->effect_path = obs_module_file("effects/drawing-emphasizer.effect");
	context->effect_tech = "Draw";
	context->sensitivity_factor = 0.0f;

	context->effect = NULL;
	context->effect_sensitivity_factor = NULL;
	context->effect_texel_height = NULL;
	context->effect_texel_width = NULL;

	showdraw_update(context, settings);

	return context;
}

void showdraw_destroy(void *data)
{
	obs_log(LOG_INFO, "Destroying showdraw source");

	struct showdraw_filter_context *context = (struct showdraw_filter_context *)data;

	if (context->luminance_extraction_effect) {
		gs_effect_destroy(context->luminance_extraction_effect);
		context->luminance_extraction_effect = NULL;
	}

	if (context->effect) {
		gs_effect_destroy(context->effect);
		context->effect = NULL;
	}

	bfree(context);
}

void showdraw_get_defaults(obs_data_t *data)
{
	obs_data_set_default_double(data, "sensitivityFactorDb", 0.0);
	obs_data_set_default_string(data, "effectTechnique", "Draw");
	obs_data_set_default_int(data, "extractionMode", EXTRACTION_MODE_DEFAULT);
}

obs_properties_t *showdraw_get_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_float_slider(props, "sensitivityFactorDb", obs_module_text("sensitivityFactorDb"), -10.0,
					100.0, 0.001);

	obs_property_t *propEffectTechnique = obs_properties_add_list(props, "effectTechnique",
								      obs_module_text("effectTechnique"),
								      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(propEffectTechnique, "Default", "Draw");

	obs_property_t *propExtractionMode = obs_properties_add_list(
		props, "extractionMode", obs_module_text("extractionMode"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModeDefault"),
				  EXTRACTION_MODE_DEFAULT);
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModePassthrough"),
				  EXTRACTION_MODE_PASSTHROUGH);
	obs_property_list_add_int(propExtractionMode, obs_module_text("extractionModeLuminanceExtraction"),
				  EXTRACTION_MODE_LUMINANCE_EXTRACTION);

	return props;
}

void showdraw_update(void *data, obs_data_t *settings)
{
	struct showdraw_filter_context *context = (struct showdraw_filter_context *)data;

	context->sensitivity_factor = (float)pow(10.0, obs_data_get_double(settings, "sensitivityFactorDb") / 10.0);
	context->effect_tech = obs_data_get_string(settings, "effectTechnique");
	context->extraction_mode = obs_data_get_int(settings, "extractionMode");
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

	if (!context->luminance_extraction_effect) {
		char *error_string = NULL;

		gs_effect_t *luminance_extraction_effect =
			gs_effect_create_from_file(context->luminance_extraction_effect_path, &error_string);
		if (!luminance_extraction_effect) {
			obs_log(LOG_ERROR, "Error loading luminance extraction effect: %s", error_string);
			bfree(error_string);
			return;
		}

		context->luminance_extraction_effect = luminance_extraction_effect;
		context->luminance_extraction_effect_texel_width =
			gs_effect_get_param_by_name(context->luminance_extraction_effect, "texelWidth");
		context->luminance_extraction_effect_texel_height =
			gs_effect_get_param_by_name(context->luminance_extraction_effect, "texelHeight");

		obs_log(LOG_INFO, "Luminance extraction effect loaded successfully");
	}

	if (!context->effect) {
		char *error_string = NULL;

		gs_effect_t *effect = gs_effect_create_from_file(context->effect_path, &error_string);
		if (!effect) {
			obs_log(LOG_ERROR, "Error loading effect: %s", error_string);
			bfree(error_string);
			return;
		}

		context->effect = effect;
		context->effect_texel_width = gs_effect_get_param_by_name(context->effect, "texelWidth");
		context->effect_texel_height = gs_effect_get_param_by_name(context->effect, "texelHeight");
		context->effect_sensitivity_factor = gs_effect_get_param_by_name(context->effect, "sensitivityFactor");

		obs_log(LOG_INFO, "Effect loaded successfully");
	}

	long long extractionMode = context->extraction_mode == EXTRACTION_MODE_DEFAULT ? EXTRACTION_MODE_PASSTHROUGH
										       : context->extraction_mode;

	if (extractionMode <= EXTRACTION_MODE_PASSTHROUGH) {
		obs_source_skip_video_filter(context->filter);
		return;
	}

	if (!obs_source_process_filter_begin(context->filter, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING)) {
		obs_log(LOG_ERROR, "Could not begin filter processing");
		obs_source_skip_video_filter(context->filter);
		return;
	}

	gs_effect_set_float(context->luminance_extraction_effect_texel_width,
			    1.0f / (float)obs_source_get_base_width(target));
	gs_effect_set_float(context->luminance_extraction_effect_texel_height,
			    1.0f / (float)obs_source_get_base_height(target));

	obs_source_process_filter_tech_end(context->filter, context->luminance_extraction_effect, 0, 0,
					   context->effect_tech);
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

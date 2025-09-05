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
#include "plugin-support.h"

#include "ShowDrawFilterContext.h"

extern "C" {

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

struct obs_source_info showdraw_filter = {
	"showdraw",              // id
	OBS_SOURCE_TYPE_FILTER,  // type
	OBS_SOURCE_VIDEO,        // output_flags
	showdraw_get_name,       // get_name
	showdraw_create,         // create
	showdraw_destroy,        // destroy
	nullptr,                 // get_width
	nullptr,                 // get_height
	showdraw_get_defaults,   // get_defaults
	showdraw_get_properties, // get_properties
	showdraw_update,         // update
	nullptr,                 // activate
	nullptr,                 // deactivate
	nullptr,                 // show
	nullptr,                 // hide
	nullptr,                 // video_tick
	showdraw_video_render,   // video_render
				 // The rest of the fields will be zero-initialized
};

bool obs_module_load()
{
	obs_register_source(&showdraw_filter);
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload()
{
	obs_log(LOG_INFO, "plugin unloaded");
}
}

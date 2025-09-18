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

#include "MainPluginContext.h"

#include <cstdio>

#include <obs-module.h>

#include "../BridgeUtils/GsUnique.hpp"
#include "../BridgeUtils/ObsLogger.hpp"

using namespace KaitoTokyo::ShowDraw;
using namespace KaitoTokyo::BridgeUtils;

namespace {

inline const ILogger &logger()
{
	static const ObsLogger instance("[" PLUGIN_NAME "] ");
	return instance;
}

} // namespace

const char *main_plugin_context_get_name(void *)
{
	return obs_module_text("pluginName");
}

void *main_plugin_context_create(obs_data_t *settings, obs_source_t *source)
try {
	GraphicsContextGuard guard;
	auto self = std::make_shared<MainPluginContext>(logger(), settings, source);
	return new std::shared_ptr<MainPluginContext>(self);
} catch (const std::exception &e) {
	logger().logException(e, "Failed to create context");
	return nullptr;
} catch (...) {
	logger().error("Failed to create context: unknown error");
	return nullptr;
}

void main_plugin_context_destroy(void *data)
try {
	if (!data) {
		logger().error("Failed to destroy context: data is null");
		return;
	}

	auto selfPtr = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	(*selfPtr)->shutdown();
	delete selfPtr;

	GraphicsContextGuard guard;
	GsUnique::drain();
} catch (const std::exception &e) {
	logger().logException(e, "Failed to destroy context");
} catch (...) {
	logger().error("Failed to destroy context: unknown error");
}

#define GET_CONTEXT(data) static_cast<std::shared_ptr<MainPluginContext> *>(data)->get()

std::uint32_t main_plugin_context_get_width(void *data)
{
	if (data) {
		return GET_CONTEXT(data)->getWidth();
	} else {
		logger().error("Failed to get width: data is null");
		return 0;
	}
}

std::uint32_t main_plugin_context_get_height(void *data)
{
	if (data) {
		return GET_CONTEXT(data)->getHeight();
	} else {
		logger().error("Failed to get height: data is null");
		return 0;
	}
}

void main_plugin_context_get_defaults(obs_data_t *data)
{
	MainPluginContext::getDefaults(data);
}

obs_properties_t *main_plugin_context_get_properties(void *data)
{
	if (data) {
		return GET_CONTEXT(data)->getProperties();
	} else {
		logger().error("Failed to get properties: data is null");
		return obs_properties_create();
	}
}

void main_plugin_context_update(void *data, obs_data_t *settings)
{
	if (data) {
		GET_CONTEXT(data)->update(settings);
	} else {
		logger().error("Failed to update settings: data is null");
	}
}

void main_plugin_context_activate(void *data)
{
	if (data) {
		GET_CONTEXT(data)->activate();
	} else {
		logger().error("Failed to activate context: data is null");
	}
}
void main_plugin_context_deactivate(void *data)
{
	if (data) {
		GET_CONTEXT(data)->deactivate();
	} else {
		logger().error("Failed to deactivate context: data is null");
	}
}
void main_plugin_context_show(void *data)
{
	if (data) {
		GET_CONTEXT(data)->show();
	} else {
		logger().error("Failed to show context: data is null");
	}
}
void main_plugin_context_hide(void *data)
{
	if (data) {
		GET_CONTEXT(data)->hide();
	} else {
		logger().error("Failed to hide context: data is null");
	}
}
void main_plugin_context_video_tick(void *data, float s)
{
	if (data) {
		GET_CONTEXT(data)->videoTick(s);
	} else {
		logger().error("Failed to tick video: data is null");
	}
}

void main_plugin_context_video_render(void *data, gs_effect_t *)
try {
	if (data) {
		GET_CONTEXT(data)->videoRender();
	} else {
		logger().error("Failed to render video: data is null");
	}
} catch (const std::exception &e) {
	logger().logException(e, "Failed to render video");
} catch (...) {
	logger().error("Failed to render video: unknown error");
}

struct obs_source_frame *main_plugin_context_filter_video(void *data, struct obs_source_frame *frame)
try {
	if (frame) {
		return GET_CONTEXT(data)->filterVideo(frame);
	} else {
		logger().error("Failed to filter video: frame is null");
		return frame;
	}
} catch (const std::exception &e) {
	logger().logException(e, "Failed to filter video");
	return frame;
} catch (...) {
	logger().error("Failed to filter video: unknown error");
	return frame;
}

bool main_plugin_context_module_load()
{
	return true;
}

void main_plugin_context_module_unload()
try {
	GraphicsContextGuard guard;
	GsUnique::drain();
} catch (const std::exception &e) {
	logger().logException(e, "Failed to unload main plugin context");
} catch (...) {
	logger().error("Failed to unload main plugin context: unknown error");
}

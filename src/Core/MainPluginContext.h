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

#ifndef PLUGIN_NAME
#define PLUGIN_NAME "obs-showdraw"
#endif
#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "0.0.0" // Set your actual version
#endif

#ifdef __cplusplus

#include <atomic>
#include <future>
#include <memory>
#include <mutex>
#include <optional>

#include "../BridgeUtils/ILogger.hpp"
#include "../BridgeUtils/ThrottledTaskQueue.hpp"

#include "Preset.hpp"
#include "RenderingContext.hpp"
#include "MainEffect.hpp"

namespace KaitoTokyo {
namespace ShowDraw {

class skip_video_filter_exception : public std::exception {
public:
	const char *what() const noexcept override { return "skip video filter"; }
};

class MainPluginContext : public std::enable_shared_from_this<MainPluginContext> {
public:
	const KaitoTokyo::BridgeUtils::ILogger &logger;
	obs_source_t *const source;
	const MainEffect mainEffect;

	std::shared_ptr<const Preset> preset;
	std::shared_ptr<RenderingContext> renderingContext = nullptr;

	MainPluginContext(const BridgeUtils::ILogger &_logger, obs_data_t *settings, obs_source_t *source);
	~MainPluginContext() noexcept;

	void startup() noexcept;
	void shutdown() noexcept;

	uint32_t getWidth() const noexcept;
	uint32_t getHeight() const noexcept;

	static void getDefaults(obs_data_t *data);

	obs_properties_t *getProperties();
	void update(obs_data_t *settings);

	void activate();
	void deactivate();
	void show();
	void hide();

	void videoTick(float seconds);
	obs_source_frame *filterVideo(obs_source_frame *frame);
	void videoRender();

	obs_source_t *getFilter() const noexcept;
};

} // namespace ShowDraw
} // namespace KaitoTokyo

extern "C" {
#endif // __cplusplus

const char *main_plugin_context_get_name(void *type_data);
void *main_plugin_context_create(obs_data_t *settings, obs_source_t *source);
void main_plugin_context_destroy(void *data);
uint32_t main_plugin_context_get_width(void *data);
uint32_t main_plugin_context_get_height(void *data);
void main_plugin_context_get_defaults(obs_data_t *data);
obs_properties_t *main_plugin_context_get_properties(void *data);
void main_plugin_context_update(void *data, obs_data_t *settings);
void main_plugin_context_activate(void *data);
void main_plugin_context_deactivate(void *data);
void main_plugin_context_show(void *data);
void main_plugin_context_hide(void *data);
void main_plugin_context_video_tick(void *data, float seconds);
void main_plugin_context_video_render(void *data, gs_effect_t *effect);
struct obs_source_frame *main_plugin_context_filter_video(void *data, struct obs_source_frame *frame);

bool main_plugin_context_module_load(void);
void main_plugin_context_module_unload(void);

#ifdef __cplusplus
}
#endif // __cplusplus

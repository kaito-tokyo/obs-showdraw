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

#pragma once

#include <obs-module.h>

#include "settings.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void show_preset_window(struct settings *current_settings, void *parent_window_pointer);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <QDialog>
#include <QTextEdit>

namespace Ui {
class PresetWindow;
}

class PresetWindow : public QDialog {
	Q_OBJECT

public:
	explicit PresetWindow(struct settings *currentSettings, QWidget *parent = nullptr);
	~PresetWindow();

private slots:
	void onPresetSelectionChanged(int index);
	void onApplyButtonClicked();

private:
	struct settings *currentSettings;
	QTextEdit *settingsJsonTextEdit;
	struct settings selectedPreset;
};

#endif

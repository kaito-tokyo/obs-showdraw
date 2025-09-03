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

#include "showdraw-preset.hpp"
#include "showdraw-global-state.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void showdraw_preset_window_show(struct showdraw_global_state *global_state);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <vector>

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QTextEdit>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>

namespace Ui {
class PresetWindow;
}

class PresetWindow : public QDialog {
	Q_OBJECT

public:
	PresetWindow(struct showdraw_global_state *globalState, QWidget *parent = nullptr);
	~PresetWindow();

private slots:
	void onPresetSelectionChanged(int index);
	void onAddButtonClicked(void);
	void onRemoveButtonClicked(void);
	void onSettingsJsonTextEditChanged(void);
	void onApplyButtonClicked(void);

private:
	struct showdraw_global_state *globalState;
	std::vector<struct showdraw_preset *> presets;

	QComboBox *const presetSelector;
	QToolButton *const addButton;
	QToolButton *const removeButton;
	QHBoxLayout *const presetSelectorLayout;
	QTextEdit *const settingsJsonTextEdit;
	QLabel *const settingsErrorLabel;
	QPushButton *const applyButton;
	QVBoxLayout *const layout;

	void validateSettingsJsonTextEdit(void);
};

#endif

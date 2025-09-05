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

#include "Preset.hpp"

#ifdef __cplusplus
extern "C" {
#endif

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
	PresetWindow(obs_source_t *filter, const Preset &runningPreset, QWidget *parent = nullptr);
	~PresetWindow();

private slots:
	void onPresetSelectionChanged(int index);
	void onAddButtonClicked();
	void onRemoveButtonClicked();
	void onSettingsJsonTextEditChanged();
	void onApplyButtonClicked();

private:
	obs_source_t *filter;
	const Preset &runningPreset;
	std::vector<Preset> presets;

	QComboBox *const presetSelector;
	QToolButton *const addButton;
	QToolButton *const removeButton;
	QHBoxLayout *const presetSelectorLayout;
	QTextEdit *const settingsJsonTextEdit;
	QLabel *const settingsErrorLabel;
	QPushButton *const applyButton;
	QVBoxLayout *const layout;

	bool validateSettingsJsonTextEdit();
};

#endif

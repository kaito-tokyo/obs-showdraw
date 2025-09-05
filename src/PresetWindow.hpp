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

#include <memory>
#include <vector>

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QTextEdit>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>

#include <obs.h>

#include "Preset.hpp"

namespace kaito_tokyo {
namespace obs_showdraw {

class ShowDrawFilterContext;

class PresetWindow : public QDialog {
	Q_OBJECT

public:
	PresetWindow(std::shared_ptr<ShowDrawFilterContext> context, QWidget *parent);
	~PresetWindow();

private slots:
	void onPresetSelectionChanged(int index);
	void onAddButtonClicked();
	void onRemoveButtonClicked();
	void onSettingsJsonTextEditChanged();
	void onApplyButtonClicked();

private:
	std::vector<Preset> presets;
	std::shared_ptr<ShowDrawFilterContext> context;

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

} // namespace obs_showdraw
} // namespace kaito_tokyo

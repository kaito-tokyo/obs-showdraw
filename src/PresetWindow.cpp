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

#include "PresetWindow.hpp"

#include <sstream>
#include <string>

#include <QByteArray>
#include <QComboBox>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QToolButton>

#include <obs-frontend-api.h>
#include <plugin-support.h>
#include <util/platform.h>
#include <util/dstr.h>

#include "showdraw-conf.hpp"

std::vector<struct showdraw_preset *> initializePresets(struct showdraw_global_state *globalState)
{
	struct showdraw_preset *currentPreset = showdraw_preset_create(" current", true);
	showdraw_preset_copy(currentPreset, globalState->running_preset);

	std::vector<struct showdraw_preset *> presets{currentPreset, showdraw_preset_get_strong_default()};

	char *configPath = obs_module_config_path(USER_PRESETS_JSON);
	obs_data_t *configData = obs_data_create_from_json_file_safe(configPath, "bak");
	bfree(configPath);

	if (!configData) {
		return presets;
	}

	obs_data_array_t *settingsArray = obs_data_get_array(configData, "settings");

	if (!settingsArray) {
		return presets;
	}

	for (size_t i = 0; i < obs_data_array_count(settingsArray); ++i) {
		obs_data_t *presetData = obs_data_array_item(settingsArray, i);
		if (!presetData) {
			continue;
		}

		struct showdraw_preset *preset = showdraw_conf_load_preset_from_obs_data(presetData);

		if (!preset) {
			obs_log(LOG_ERROR, "Failed to create preset from JSON");
			continue;
		}

		presets.push_back(preset);
		obs_data_release(presetData);
	}

	obs_data_array_release(settingsArray);
	obs_data_release(configData);

	return presets;
}

PresetWindow::PresetWindow(struct showdraw_global_state *globalState, QWidget *parent)
	: QDialog(parent),
	  globalState(globalState),
	  presets(initializePresets(globalState)),
	  presetSelector(new QComboBox()),
	  addButton(new QToolButton()),
	  removeButton(new QToolButton()),
	  presetSelectorLayout(new QHBoxLayout()),
	  settingsJsonTextEdit(new QTextEdit()),
	  settingsErrorLabel(new QLabel()),
	  applyButton(new QPushButton(obs_module_text("presetWindowApply"))),
	  layout(new QVBoxLayout())
{
	setAttribute(Qt::WA_DeleteOnClose);

	for (struct showdraw_preset *preset : presets) {
		std::string presetName(preset->preset_name.array);
		if (presetName == " current") {
			presetSelector->addItem("-");
		} else if (presetName == " strong default") {
			presetSelector->addItem(QString::fromUtf8(obs_module_text("presetWindowStrongDefault")));
		} else {
			presetSelector->addItem(QString::fromStdString(presetName));
		}
	}
	connect(presetSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
		&PresetWindow::onPresetSelectionChanged);

	addButton->setText("+");
	connect(addButton, &QToolButton::clicked, this, &PresetWindow::onAddButtonClicked);

	removeButton->setText("-");
	removeButton->setEnabled(false);
	connect(removeButton, &QToolButton::clicked, this, &PresetWindow::onRemoveButtonClicked);

	presetSelectorLayout->addWidget(presetSelector);
	presetSelectorLayout->addWidget(addButton);
	presetSelectorLayout->addWidget(removeButton);

	settingsJsonTextEdit->setStyleSheet("QTextEdit[error=\"true\"] { border: 2px solid red; }");
	connect(settingsJsonTextEdit, &QTextEdit::textChanged, this, &PresetWindow::onSettingsJsonTextEditChanged);

	obs_data_t *settingsData = obs_data_create();
	showdraw_conf_load_preset_into_obs_data(settingsData, globalState->running_preset);
	const char *settingsJson = obs_data_get_json_pretty(settingsData);
	settingsJsonTextEdit->setPlainText(QString::fromUtf8(settingsJson));
	obs_data_release(settingsData);

	connect(applyButton, &QPushButton::clicked, this, &PresetWindow::onApplyButtonClicked);

	layout->addLayout(presetSelectorLayout);
	layout->addWidget(settingsJsonTextEdit);
	layout->addWidget(settingsErrorLabel);
	layout->addWidget(applyButton);

	setLayout(layout);
}

PresetWindow::~PresetWindow()
{
	for (struct showdraw_preset *preset : presets) {
		showdraw_preset_destroy(preset);
	}
}

void PresetWindow::onPresetSelectionChanged(int index)
{
	if (index == 0) {
		removeButton->setEnabled(false);
		return;
	}

	struct showdraw_preset *selectedPreset = presets[index];

	obs_data_t *settingsData = obs_data_create();
	showdraw_conf_load_preset_into_obs_data(settingsData, selectedPreset);
	const char *settingsJson = obs_data_get_json_pretty(settingsData);
	{
		QSignalBlocker blocker(settingsJsonTextEdit);
		settingsJsonTextEdit->setPlainText(QString::fromUtf8(settingsJson));
	}
	obs_data_release(settingsData);
	validateSettingsJsonTextEdit();

	removeButton->setEnabled(!showdraw_preset_is_system(selectedPreset));
}

void PresetWindow::onAddButtonClicked(void)
{
	size_t userPresetCount = std::count_if(presets.begin(), presets.end(), &showdraw_preset_is_user);

	std::ostringstream oss;
	oss << obs_module_text("presetWindowAddNewPrefix") << " " << (userPresetCount + 1);

	bool ok;

	QString presetName = QInputDialog::getText(this, obs_module_text("presetWindowAddTitle"),
						   obs_module_text("presetWindowAddLabel"), QLineEdit::Normal,
						   QString::fromStdString(oss.str()), &ok);

	if (!ok || presetName.isEmpty()) {
		return;
	}

	if (!validateSettingsJsonTextEdit()) {
		return;
	}

	std::string newPresetJson = settingsJsonTextEdit->toPlainText().toStdString();

	obs_data_t *newPresetData = obs_data_create_from_json(newPresetJson.c_str());
	if (!newPresetData) {
		obs_log(LOG_ERROR, "Failed to create obs_data_t from JSON");
		return;
	}

	obs_data_set_string(newPresetData, "presetName", presetName.toUtf8().constData());

	struct showdraw_preset *newPreset = showdraw_conf_load_preset_from_obs_data(newPresetData);
	obs_data_release(newPresetData);

	presetSelector->addItem(presetName);
	{
		QSignalBlocker blocker(presetSelector);
		presetSelector->setCurrentIndex(presetSelector->count() - 1);
		removeButton->setEnabled(true);
	}

	presets.push_back(newPreset);

	showdraw_conf_save_user_presets(presets.data(), presets.size());
}

void PresetWindow::onRemoveButtonClicked(void)
{
	int presetIndex = presetSelector->currentIndex();
	if (presetIndex < 0 || presetIndex >= static_cast<int>(presets.size())) {
		obs_log(LOG_ERROR, "Invalid preset index %d", presetIndex);
		return;
	}

	showdraw_preset *preset = presets[presetIndex];
	if (showdraw_preset_is_system(preset)) {
		obs_log(LOG_ERROR, "Attempted to remove system preset %s", preset->preset_name.array);
		return;
	}

	QMessageBox::StandardButton reply = QMessageBox::question(this, obs_module_text("presetWindowRemoveTitle"),
								  obs_module_text("presetWindowRemoveLabel"),
								  QMessageBox::Yes | QMessageBox::No);

	if (reply != QMessageBox::Yes) {
		return;
	}

	presets.erase(presets.begin() + presetIndex);
	presetSelector->removeItem(presetIndex);
	showdraw_preset_destroy(preset);

	showdraw_conf_save_user_presets(presets.data(), presets.size());
}

void PresetWindow::onSettingsJsonTextEditChanged(void)
{
	{
		QSignalBlocker blocker(presetSelector);
		presetSelector->setCurrentIndex(0);
		removeButton->setDisabled(true);
	}

	validateSettingsJsonTextEdit();
}

void PresetWindow::onApplyButtonClicked(void)
{
	obs_data_t *settings = obs_source_get_settings(globalState->filter);

	// showdraw_conf_load_preset_into_obs_data(settings, &selectedPreset);

	obs_source_update(globalState->filter, settings);

	obs_data_release(settings);

	close();
}

bool PresetWindow::validateSettingsJsonTextEdit(void)
{
	std::string newPresetJson = settingsJsonTextEdit->toPlainText().toStdString();

	obs_data_t *newPresetData = obs_data_create_from_json(newPresetJson.c_str());
	if (!newPresetData) {
		settingsJsonTextEdit->setStyleSheet("QTextEdit { border: 2px solid red; }");
		settingsJsonTextEdit->setToolTip(obs_module_text("presetWindowInvalidJson"));
		settingsErrorLabel->setText(obs_module_text("presetWindowInvalidJson"));
		return false;
	}

	obs_data_set_string(newPresetData, "presetName", "new preset");

	struct showdraw_preset *newPreset = showdraw_conf_load_preset_from_obs_data(newPresetData);
	obs_data_release(newPresetData);

	if (!newPreset) {
		obs_log(LOG_ERROR, "Failed to create new preset from JSON");
		return false;
	}

	const char *errorMessage = showdraw_conf_validate_settings(newPreset);
	if (errorMessage) {
		settingsJsonTextEdit->setStyleSheet("QTextEdit { border: 2px solid red; }");
		settingsJsonTextEdit->setToolTip(errorMessage);
		settingsErrorLabel->setText(errorMessage);
		return false;
	}

	settingsJsonTextEdit->setStyleSheet("");
	settingsJsonTextEdit->setToolTip(obs_module_text("presetWindowJsonOk"));
	settingsErrorLabel->setText(obs_module_text("presetWindowJsonOk"));

	showdraw_preset_destroy(newPreset);

	return true;
}

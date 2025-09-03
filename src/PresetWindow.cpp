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

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>
#include <util/platform.h>
#include <util/dstr.h>

std::vector<Preset> initializePresets(const Preset &runningPreset)
{
	std::vector<Preset> presets{runningPreset, Preset::getStrongDefault()};

	char *configPath = obs_module_config_path(UserPresetsJson);
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

		const Preset preset = Preset::fromObsData(presetData);

		presets.push_back(preset);
		obs_data_release(presetData);
	}

	obs_data_array_release(settingsArray);
	obs_data_release(configData);

	return presets;
}

PresetWindow::PresetWindow(obs_source_t *filter, const Preset &runningPreset, QWidget *parent)
	: QDialog(parent),
	  filter(filter),
	  runningPreset(runningPreset),
	  presets(initializePresets(runningPreset)),
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

	for (const Preset &preset : presets) {
		if (preset.presetName == " current") {
			presetSelector->addItem("-");
		} else if (preset.presetName == " strong default") {
			presetSelector->addItem(QString::fromUtf8(obs_module_text("presetWindowStrongDefault")));
		} else {
			presetSelector->addItem(QString::fromStdString(preset.presetName));
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
	runningPreset.loadIntoObsData(settingsData);
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
}

void PresetWindow::onPresetSelectionChanged(int index)
{
	if (index == 0) {
		removeButton->setEnabled(false);
		return;
	}

	const Preset &selectedPreset = presets[index];

	obs_data_t *settingsData = obs_data_create();
	selectedPreset.loadIntoObsData(settingsData);
	const char *settingsJson = obs_data_get_json_pretty(settingsData);
	{
		QSignalBlocker blocker(settingsJsonTextEdit);
		settingsJsonTextEdit->setPlainText(QString::fromUtf8(settingsJson));
	}
	obs_data_release(settingsData);
	validateSettingsJsonTextEdit();

	removeButton->setEnabled(selectedPreset.isUser());
}

void PresetWindow::onAddButtonClicked(void)
{
	size_t userPresetCount = std::count_if(presets.begin(), presets.end(), &Preset::isUser);

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

	const Preset newPreset = Preset::fromObsData(newPresetData);
	obs_data_release(newPresetData);

	presetSelector->addItem(presetName);
	{
		QSignalBlocker blocker(presetSelector);
		presetSelector->setCurrentIndex(presetSelector->count() - 1);
		removeButton->setEnabled(true);
	}

	presets.push_back(newPreset);

	Preset::saveUserPresets(presets);
}

void PresetWindow::onRemoveButtonClicked(void)
{
	int presetIndex = presetSelector->currentIndex();
	if (presetIndex < 0 || presetIndex >= static_cast<int>(presets.size())) {
		obs_log(LOG_ERROR, "Invalid preset index %d", presetIndex);
		return;
	}

	const Preset &preset = presets[presetIndex];
	if (preset.isSystem()) {
		obs_log(LOG_ERROR, "Attempted to remove system preset %s", preset.presetName.c_str());
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

	Preset::saveUserPresets(presets);
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
	// obs_data_t *settings = obs_source_get_settings(filter);

	// showdraw_conf_load_preset_into_obs_data(settings, &selectedPreset);

	// obs_source_update(globalState->filter, settings);

	// obs_data_release(settings);

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

	const Preset newPreset = Preset::fromObsData(newPresetData);
	obs_data_release(newPresetData);

	std::optional<std::string> errorMessage = newPreset.validate();
	if (errorMessage) {
		settingsJsonTextEdit->setStyleSheet("QTextEdit { border: 2px solid red; }");
		settingsJsonTextEdit->setToolTip(QString::fromStdString(*errorMessage));
		settingsErrorLabel->setText(QString::fromStdString(*errorMessage));
		return false;
	}

	settingsJsonTextEdit->setStyleSheet("");
	settingsJsonTextEdit->setToolTip(obs_module_text("presetWindowJsonOk"));
	settingsErrorLabel->setText(obs_module_text("presetWindowJsonOk"));

	return true;
}

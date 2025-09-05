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

#include <functional>
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

#include "plugin-support.h"
#include <util/platform.h>
#include <util/dstr.h>
#include <obs-module.h>

#include "obs-bridge-utils/obs-bridge-utils.hpp"

#include "ShowDrawFilterContext.h"

using kaito_tokyo::obs_bridge_utils::slog;
using kaito_tokyo::obs_bridge_utils::unique_obs_data_t;

using kaito_tokyo::obs_showdraw::Preset;

namespace kaito_tokyo {
namespace obs_showdraw {

PresetWindow::PresetWindow(std::shared_ptr<ShowDrawFilterContext> context, QWidget *parent = nullptr)
	: QDialog(parent),
	  presets(Preset::loadUserPresets(context->getRunningPreset())),
	  context(std::move(context)),
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

	unique_obs_data_t settingsData(obs_data_create());
	context->getRunningPreset().loadIntoObsData(settingsData.get());
	const char *settingsJson = obs_data_get_json_pretty(settingsData.get());
	settingsJsonTextEdit->setPlainText(QString::fromUtf8(settingsJson));

	connect(applyButton, &QPushButton::clicked, this, &PresetWindow::onApplyButtonClicked);

	layout->addLayout(presetSelectorLayout);
	layout->addWidget(settingsJsonTextEdit);
	layout->addWidget(settingsErrorLabel);
	layout->addWidget(applyButton);

	setLayout(layout);
}

PresetWindow::~PresetWindow() {}

void PresetWindow::onPresetSelectionChanged(int index)
{
	if (index == 0) {
		removeButton->setEnabled(false);
		return;
	}

	const Preset &selectedPreset = presets[index];

	unique_obs_data_t settingsData(obs_data_create());
	selectedPreset.loadIntoObsData(settingsData.get());
	const char *settingsJson = obs_data_get_json_pretty(settingsData.get());
	{
		QSignalBlocker blocker(settingsJsonTextEdit);
		settingsJsonTextEdit->setPlainText(QString::fromUtf8(settingsJson));
	}
	validateSettingsJsonTextEdit();

	removeButton->setEnabled(selectedPreset.isUser());
}

void PresetWindow::onAddButtonClicked()
{
	size_t userPresetCount =
		std::count_if(presets.begin(), presets.end(), std::function<bool(const Preset &)>(&Preset::isUser));

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

	unique_obs_data_t newPresetData(obs_data_create_from_json(newPresetJson.c_str()));
	if (!newPresetData) {
		obs_log(LOG_ERROR, "Failed to create obs_data_t from JSON");
		return;
	}

	obs_data_set_string(newPresetData.get(), "presetName", presetName.toUtf8().constData());

	const Preset newPreset = Preset::fromObsData(newPresetData.get());

	presetSelector->addItem(presetName);
	{
		QSignalBlocker blocker(presetSelector);
		presetSelector->setCurrentIndex(presetSelector->count() - 1);
		removeButton->setEnabled(true);
	}

	presets.push_back(newPreset);

	Preset::saveUserPresets(presets);
}

void PresetWindow::onRemoveButtonClicked()
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

void PresetWindow::onSettingsJsonTextEditChanged()
{
	{
		QSignalBlocker blocker(presetSelector);
		presetSelector->setCurrentIndex(0);
		removeButton->setDisabled(true);
	}

	validateSettingsJsonTextEdit();
}

void PresetWindow::onApplyButtonClicked()
{
	// obs_data_t *settings = obs_source_get_settings(filter);

	// showdraw_conf_load_preset_into_obs_data(settings, &selectedPreset);

	// obs_source_update(globalState->filter, settings);

	// obs_data_release(settings);

	close();
}

bool PresetWindow::validateSettingsJsonTextEdit()
{
	std::string newPresetJson = settingsJsonTextEdit->toPlainText().toStdString();

	unique_obs_data_t newPresetData(obs_data_create_from_json(newPresetJson.c_str()));
	if (!newPresetData) {
		settingsJsonTextEdit->setStyleSheet("QTextEdit { border: 2px solid red; }");
		settingsJsonTextEdit->setToolTip(obs_module_text("presetWindowInvalidJson"));
		settingsErrorLabel->setText(obs_module_text("presetWindowInvalidJson"));
		return false;
	}

	obs_data_set_string(newPresetData.get(), "presetName", "new preset");

	const Preset newPreset = Preset::fromObsData(newPresetData.get());

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

} // namespace obs_showdraw
} // namespace kaito_tokyo

#include "PresetWindow.hpp"

#include <sstream>

#include <QByteArray>
#include <QComboBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QVBoxLayout>
#include <QToolButton>

#include "settings.hpp"

#include <plugin-support.h>
#include <util/platform.h>
#include <util/dstr.h>

#define USER_PRESETS_JSON "UserPresets.json"

void show_preset_window(struct settings *current_settings, void *parent_window_pointer)
{
	QWidget *parent = static_cast<QWidget *>(parent_window_pointer);
	PresetWindow *window = new PresetWindow(current_settings, parent);
	window->exec();
}

QByteArray settingsToJson(const struct settings &settings)
{
	QJsonObject obj;

	obj["extraction_mode"] = settings.extraction_mode;
	obj["median_filtering_kernel_size"] = settings.median_filtering_kernel_size;
	obj["motion_map_kernel_size"] = settings.motion_map_kernel_size;
	obj["motion_adaptive_filtering_strength"] = settings.motion_adaptive_filtering_strength;
	obj["motion_adaptive_filtering_motion_threshold"] = settings.motion_adaptive_filtering_motion_threshold;
	obj["morphology_opening_erosion_kernel_size"] = settings.morphology_opening_erosion_kernel_size;
	obj["morphology_opening_dilation_kernel_size"] = settings.morphology_opening_dilation_kernel_size;
	obj["morphology_closing_dilation_kernel_size"] = settings.morphology_closing_dilation_kernel_size;
	obj["morphology_closing_erosion_kernel_size"] = settings.morphology_closing_erosion_kernel_size;
	obj["scaling_factor"] = pow(10.0, settings.scaling_factor_db / 10.0);
	obj["scaling_factor_db"] = settings.scaling_factor_db;

	QJsonDocument doc(obj);
	QByteArray jsonData = doc.toJson(QJsonDocument::Indented);
	return jsonData;
}

void loadSettingsIntoObsData(obs_data_t *data, const struct settings &settings)
{
	obs_data_set_int(data, "extractionMode", settings.extraction_mode);
	obs_data_set_int(data, "medianFilteringKernelSize", settings.median_filtering_kernel_size);
	obs_data_set_int(data, "motionMapKernelSize", settings.motion_map_kernel_size);
	obs_data_set_double(data, "motionAdaptiveFilteringStrength", settings.motion_adaptive_filtering_strength);
	obs_data_set_double(data, "motionAdaptiveFilteringMotionThreshold",
			    settings.motion_adaptive_filtering_motion_threshold);
	obs_data_set_int(data, "morphologyOpeningErosionKernelSize", settings.morphology_opening_erosion_kernel_size);
	obs_data_set_int(data, "morphologyOpeningDilationKernelSize", settings.morphology_opening_dilation_kernel_size);
	obs_data_set_int(data, "morphologyClosingDilationKernelSize", settings.morphology_closing_dilation_kernel_size);
	obs_data_set_int(data, "morphologyClosingErosionKernelSize", settings.morphology_closing_erosion_kernel_size);
	obs_data_set_double(data, "scalingFactorDb", settings.scaling_factor_db);
	obs_data_set_double(data, "scalingFactor", settings.scaling_factor);
}

struct settings loadSettingsFromObsData(obs_data_t *data)
{
	struct settings settings;

	const char *presetName = obs_data_get_string(data, "presetName");
	dstr_init_copy(&settings.preset_name, presetName);
	settings.is_system = false;

	settings.extraction_mode = obs_data_get_int(data, "extractionMode");
	settings.median_filtering_kernel_size = obs_data_get_int(data, "medianFilteringKernelSize");
	settings.motion_map_kernel_size = obs_data_get_int(data, "motionMapKernelSize");
	settings.motion_adaptive_filtering_strength = obs_data_get_double(data, "motionAdaptiveFilteringStrength");
	settings.motion_adaptive_filtering_motion_threshold =
		obs_data_get_double(data, "motionAdaptiveFilteringMotionThreshold");
	settings.morphology_opening_erosion_kernel_size = obs_data_get_int(data, "morphologyOpeningErosionKernelSize");
	settings.morphology_opening_dilation_kernel_size =
		obs_data_get_int(data, "morphologyOpeningDilationKernelSize");
	settings.morphology_closing_dilation_kernel_size =
		obs_data_get_int(data, "morphologyClosingDilationKernelSize");
	settings.morphology_closing_erosion_kernel_size = obs_data_get_int(data, "morphologyClosingErosionKernelSize");
	settings.scaling_factor_db = obs_data_get_double(data, "scalingFactorDb");
	settings.scaling_factor = pow(10.0, settings.scaling_factor_db / 10.0);

	return settings;
}

bool validateSettings(const struct settings &settings)
{
	switch (settings.extraction_mode) {
	case EXTRACTION_MODE_DEFAULT:
	case EXTRACTION_MODE_PASSTHROUGH:
	case EXTRACTION_MODE_LUMINANCE_EXTRACTION:
	case EXTRACTION_MODE_EDGE_DETECTION:
	case EXTRACTION_MODE_SCALING:
		break;
	default:
		return false;
	}

	switch (settings.median_filtering_kernel_size) {
	case 1:
	case 3:
	case 5:
	case 7:
	case 9:
		break;
	default:
		return false;
	}

	switch (settings.motion_map_kernel_size) {
	case 1:
	case 3:
	case 5:
	case 7:
	case 9:
		break;
	default:
		return false;
	}

	if (settings.motion_adaptive_filtering_strength < 0.0 || settings.motion_adaptive_filtering_strength > 1.0) {
		return false;
	}

	if (settings.motion_adaptive_filtering_motion_threshold < 0.0 ||
	    settings.motion_adaptive_filtering_motion_threshold > 1.0) {
		return false;
	}

	if (settings.morphology_opening_erosion_kernel_size < 1 ||
	    settings.morphology_opening_erosion_kernel_size > 31 ||
	    settings.morphology_opening_erosion_kernel_size % 2 == 1) {
		return false;
	}

	if (settings.morphology_opening_dilation_kernel_size < 1 ||
	    settings.morphology_opening_dilation_kernel_size > 31 ||
	    settings.morphology_opening_dilation_kernel_size % 2 == 1) {
		return false;
	}

	if (settings.morphology_closing_dilation_kernel_size < 1 ||
	    settings.morphology_closing_dilation_kernel_size > 31 ||
	    settings.morphology_closing_dilation_kernel_size % 2 == 1) {
		return false;
	}

	if (settings.morphology_closing_erosion_kernel_size < 1 ||
	    settings.morphology_closing_erosion_kernel_size > 31 ||
	    settings.morphology_closing_erosion_kernel_size % 2 == 1) {
		return false;
	}

	if (settings.scaling_factor_db < -20.0 || settings.scaling_factor_db > 20.0) {
		return false;
	}

	return true;
}

struct settings getDefaultHighSettings()
{
	struct settings settings;

	dstr_init_copy(&settings.preset_name, obs_module_text("presetWindowHighDefault"));
	settings.is_system = true;
	settings.extraction_mode = EXTRACTION_MODE_DEFAULT;
	settings.median_filtering_kernel_size = 3;
	settings.motion_map_kernel_size = 3;
	settings.motion_adaptive_filtering_strength = 0.5;
	settings.motion_adaptive_filtering_motion_threshold = 0.3;
	settings.morphology_opening_erosion_kernel_size = 1;
	settings.morphology_opening_dilation_kernel_size = 1;
	settings.morphology_closing_dilation_kernel_size = 7;
	settings.morphology_closing_erosion_kernel_size = 5;
	settings.scaling_factor_db = 6.0;
	settings.scaling_factor = pow(10.0, settings.scaling_factor_db / 10.0);

	return settings;
}

std::vector<struct settings> readUserPresets(const struct settings &currentSettings)
{
	std::vector<struct settings> presets{
		currentSettings,
		getDefaultHighSettings()
	};

	dstr_init_copy(&presets[0].preset_name, "-");
	presets[0].is_system = true;

	char *configPath = obs_module_config_path(USER_PRESETS_JSON);
	obs_data_t *configData = obs_data_create_from_json_file_safe(configPath, "bak");
	bfree(configPath);


	if (!configData) {
		return {};
	}

	obs_data_array_t *settingsArray = obs_data_get_array(configData, "settings");

	if (!settingsArray) {
		return {};
	}

	for (size_t i = 0; i < obs_data_array_count(settingsArray); ++i) {
		obs_data_t *presetData = obs_data_array_item(settingsArray, i);
		if (!presetData) {
			continue;
		}

		struct settings preset = loadSettingsFromObsData(presetData);
		presets.push_back(preset);
		obs_data_release(presetData);
	}

	obs_data_array_release(settingsArray);
	obs_data_release(configData);

	return presets;
}

void PresetWindow::updatePresetSelector()
{
	presetSelector->clear();

	for (const auto &preset : presets) {
		presetSelector->addItem(QString::fromUtf8(preset.preset_name.array));
	}
}

PresetWindow::PresetWindow(struct settings *currentSettings, QWidget *parent)
	: QDialog(parent),
	  currentSettings(currentSettings),
	  selectedPreset(*currentSettings),
	  presets(readUserPresets(*currentSettings))
{
	setAttribute(Qt::WA_DeleteOnClose);

	presetSelector = new QComboBox();
	updatePresetSelector();
	connect(presetSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
		&PresetWindow::onPresetSelectionChanged);

	QToolButton *addButton = new QToolButton();
	addButton->setText("+");
	connect(addButton, &QToolButton::clicked, this, &PresetWindow::onAddButtonClicked);

	removeButton = new QToolButton();
	removeButton->setText("-");
	removeButton->setEnabled(false);
	connect(removeButton, &QToolButton::clicked, this, &PresetWindow::onRemoveButtonClicked);

	QHBoxLayout *presetSelectorLayout = new QHBoxLayout();
	presetSelectorLayout->addWidget(presetSelector);
	presetSelectorLayout->addWidget(addButton);
	presetSelectorLayout->addWidget(removeButton);

	settingsJsonTextEdit = new QTextEdit();
	QByteArray settingsData = settingsToJson(*currentSettings);
	settingsJsonTextEdit->setPlainText(QString::fromUtf8(settingsData));

	QPushButton *applyButton = new QPushButton(obs_module_text("presetWindowApply"));
	connect(applyButton, &QPushButton::clicked, this, &PresetWindow::onApplyButtonClicked);

	QVBoxLayout *layout = new QVBoxLayout();
	layout->addLayout(presetSelectorLayout);
	layout->addWidget(settingsJsonTextEdit);
	layout->addWidget(applyButton);

	setLayout(layout);
}

void PresetWindow::onPresetSelectionChanged(int index)
{
	struct settings selectedPreset = presets[index];

	QByteArray settingsData = settingsToJson(selectedPreset);
	settingsJsonTextEdit->setPlainText(QString::fromUtf8(settingsData));

	removeButton->setEnabled(!selectedPreset.is_system);
}

void PresetWindow::onAddButtonClicked()
{
	char *configDirPath = obs_module_config_path("");
	os_mkdirs(configDirPath);
	bfree(configDirPath);

	char *configPath = obs_module_config_path(USER_PRESETS_JSON);

	obs_data_t *configData = obs_data_create_from_json_file_safe(configPath, "bak");
	if (!configData) {
		obs_log(LOG_ERROR, "Trying to create user presets file at %s", configPath);
		configData = obs_data_create();
	}

	obs_data_array_t *settingsArray = obs_data_get_array(configData, "settings");
	if (!settingsArray) {
		settingsArray = obs_data_array_create();
		obs_data_set_array(configData, "settings", settingsArray);
	}

	std::ostringstream defaultPresetNameStream;
	defaultPresetNameStream << obs_module_text("presetWindowAddNewPrefix") << " "
				<< (obs_data_array_count(settingsArray) + 1);

	bool ok;

	QString presetName = QInputDialog::getText(this, obs_module_text("presetWindowAddTitle"),
						   obs_module_text("presetWindowAddLabel"), QLineEdit::Normal,
						   QString::fromUtf8(defaultPresetNameStream.str().c_str()), &ok);

	if (!ok || presetName.isEmpty()) {
		return;
	}

	obs_data_t *newSettings = obs_data_create();
	loadSettingsIntoObsData(newSettings, selectedPreset);
	obs_data_set_string(newSettings, "presetName", presetName.toUtf8().constData());
	obs_data_array_push_back(settingsArray, newSettings);

	if (!obs_data_save_json_pretty_safe(configData, configPath, "_", "bak")) {
		obs_log(LOG_ERROR, "Failed to save preset to %s", configPath);
	}

	obs_data_release(newSettings);
	obs_data_array_release(settingsArray);
	obs_data_release(configData);
	bfree(configPath);
}

void PresetWindow::onRemoveButtonClicked(void)
{
	if (selectedPreset.is_system) {
		return;
	}

	int presetIndex = presetSelector->currentIndex();
	presets.erase(presets.begin() + presetIndex);
	presetSelector->removeItem(presetIndex);
}

void PresetWindow::onApplyButtonClicked()
{
	obs_data_t *settings = obs_source_get_settings(currentSettings->filter);

	loadSettingsIntoObsData(settings, selectedPreset);

	obs_source_update(currentSettings->filter, settings);

	obs_data_release(settings);

	close();
}

PresetWindow::~PresetWindow() {}

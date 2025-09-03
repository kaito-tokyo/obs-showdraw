#include "PresetWindow.hpp"

#include <QComboBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QByteArray>

#include "settings.hpp"

void show_preset_window(struct settings *current_settings, void *parent_window_pointer)
{
	QWidget *parent = static_cast<QWidget *>(parent_window_pointer);
	PresetWindow *window = new PresetWindow(current_settings, parent);
	window->exec();
}

QByteArray settingsToJson(const struct settings *settings)
{
	QJsonObject obj;

	obj["extraction_mode"] = settings->extraction_mode;
	obj["median_filtering_kernel_size"] = settings->median_filtering_kernel_size;
	obj["motion_map_kernel_size"] = settings->motion_map_kernel_size;
	obj["motion_adaptive_filtering_strength"] = settings->motion_adaptive_filtering_strength;
	obj["motion_adaptive_filtering_motion_threshold"] = settings->motion_adaptive_filtering_motion_threshold;
	obj["morphology_opening_erosion_kernel_size"] = settings->morphology_opening_erosion_kernel_size;
	obj["morphology_opening_dilation_kernel_size"] = settings->morphology_opening_dilation_kernel_size;
	obj["morphology_closing_dilation_kernel_size"] = settings->morphology_closing_dilation_kernel_size;
	obj["morphology_closing_erosion_kernel_size"] = settings->morphology_closing_erosion_kernel_size;
	obj["scaling_factor"] = pow(10.0, settings->scaling_factor_db / 10.0);
	obj["scaling_factor_db"] = settings->scaling_factor_db;

	QJsonDocument doc(obj);
	QByteArray jsonData = doc.toJson(QJsonDocument::Indented);
	return jsonData;
}

struct settings getDefaultHighSettings()
{
	struct settings settings;

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

PresetWindow::PresetWindow(struct settings *currentSettings, QWidget *parent)
	: QDialog(parent),
	  currentSettings(currentSettings)
{
	setAttribute(Qt::WA_DeleteOnClose);

	QComboBox *presetSelector = new QComboBox();
	presetSelector->addItem(obs_module_text("presetWindowCurrent"));
	presetSelector->addItem(obs_module_text("presetWindowHighDefault"));
	connect(presetSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &PresetWindow::onPresetSelectionChanged);

	QPushButton *addButton = new QPushButton("+");

	QHBoxLayout *presetSelectorLayout = new QHBoxLayout();
	presetSelectorLayout->addWidget(presetSelector);
	presetSelectorLayout->addWidget(addButton);

	settingsJsonTextEdit = new QTextEdit();
	QByteArray settingsData = settingsToJson(currentSettings);
	settingsJsonTextEdit->setPlainText(QString::fromUtf8(settingsData));
	settingsJsonTextEdit->setReadOnly(true);

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
	if (index == 0) {
		QByteArray settingsData = settingsToJson(currentSettings);
		settingsJsonTextEdit->setPlainText(QString::fromUtf8(settingsData));
		selectedPreset = *currentSettings;
	} else if (index == 1) {
		struct settings defaultSettings = getDefaultHighSettings();
		QByteArray settingsData = settingsToJson(&defaultSettings);
		settingsJsonTextEdit->setPlainText(QString::fromUtf8(settingsData));
		selectedPreset = defaultSettings;
	}
}

void PresetWindow::onApplyButtonClicked()
{
	*currentSettings = selectedPreset;
	close();
}

PresetWindow::~PresetWindow() {}

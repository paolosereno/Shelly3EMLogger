#include "settingstab.h"
#include "databasemanager.h"
#include "logger.h"
#include "logviewerdialog.h"
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QDateTime>

SettingsTab::SettingsTab(QWidget *parent)
    : QWidget(parent)
    , m_subTabWidget(nullptr)
    , m_defaultPollingIntervalSpinBox(nullptr)
    , m_autoSaveEnabledCheckBox(nullptr)
    , m_autoSaveIntervalSpinBox(nullptr)
    , m_autoSaveIntervalLabel(nullptr)
    , m_languageComboBox(nullptr)
    , m_languageInfoLabel(nullptr)
    , m_applyLanguageButton(nullptr)
    , m_temperatureAlarmEnabledCheckBox(nullptr)
    , m_temperatureThresholdSpinBox(nullptr)
    , m_phaseImbalanceAlarmEnabledCheckBox(nullptr)
    , m_phaseImbalanceThresholdSpinBox(nullptr)
    , m_powerAlarmEnabledCheckBox(nullptr)
    , m_powerThresholdSpinBox(nullptr)
    , m_powerPhaseBAlarmEnabledCheckBox(nullptr)
    , m_powerPhaseBThresholdSpinBox(nullptr)
    , m_powerPhaseCAlarmEnabledCheckBox(nullptr)
    , m_powerPhaseCThresholdSpinBox(nullptr)
    , m_lowPowerPhaseAAlarmEnabledCheckBox(nullptr)
    , m_lowPowerPhaseAThresholdSpinBox(nullptr)
    , m_lowPowerPhaseBAlarmEnabledCheckBox(nullptr)
    , m_lowPowerPhaseBThresholdSpinBox(nullptr)
    , m_lowPowerPhaseCAlarmEnabledCheckBox(nullptr)
    , m_lowPowerPhaseCThresholdSpinBox(nullptr)
    , m_themeComboBox(nullptr)
    , m_debugModeCheckBox(nullptr)
    , m_logLevelCombo(nullptr)
    , m_viewLogsButton(nullptr)
    , m_historySamplesChartTypeCombo(nullptr)
    , m_historyDailyEnergyChartTypeCombo(nullptr)
    , m_resetSettingsButton(nullptr)
    , m_dbPathLabel(nullptr)
    , m_dbSizeLabel(nullptr)
    , m_dbStatsLabel(nullptr)
    , m_compactDatabaseButton(nullptr)
    , m_backupDatabaseButton(nullptr)
    , m_clearOldDataButton(nullptr)
    , m_saveButton(nullptr)
    , m_cancelButton(nullptr)
    , m_databaseManager(nullptr)
{
    setupUi();
    loadSettings();
}

SettingsTab::~SettingsTab()
{
}

void SettingsTab::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Sub-tab widget
    m_subTabWidget = new QTabWidget(this);
    m_subTabWidget->addTab(createGeneralTab(), tr("General"));
    m_subTabWidget->addTab(createLanguageTab(), tr("Language"));
    m_subTabWidget->addTab(createAlarmsTab(), tr("Alarms"));
    m_subTabWidget->addTab(createAdvancedTab(), tr("Advanced"));

    mainLayout->addWidget(m_subTabWidget);

    // Pulsanti Save/Cancel
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_saveButton = new QPushButton(tr("Save Settings"), this);
    m_cancelButton = new QPushButton(tr("Cancel"), this);

    QFont boldFont;
    boldFont.setBold(true);
    m_saveButton->setFont(boldFont);
    m_saveButton->setMinimumWidth(120);
    m_cancelButton->setMinimumWidth(120);

    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Connessioni
    connect(m_saveButton, &QPushButton::clicked, this, &SettingsTab::onSaveClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &SettingsTab::onCancelClicked);
}

QWidget* SettingsTab::createGeneralTab()
{
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);

    // === GRUPPO POLLING ===
    QGroupBox* pollingGroup = createGroup(tr("Polling Settings"));
    QFormLayout* pollingLayout = new QFormLayout(pollingGroup);

    m_defaultPollingIntervalSpinBox = new QSpinBox(this);
    m_defaultPollingIntervalSpinBox->setRange(5, 300);
    m_defaultPollingIntervalSpinBox->setValue(60);
    m_defaultPollingIntervalSpinBox->setSuffix(tr(" seconds"));
    pollingLayout->addRow(tr("Default polling interval:"), m_defaultPollingIntervalSpinBox);

    QLabel* pollingInfoLabel = new QLabel(tr("This is the default interval used when connecting to a device."), this);
    pollingInfoLabel->setWordWrap(true);
    pollingInfoLabel->setStyleSheet("color: #888; font-size: 9pt;");
    pollingLayout->addRow("", pollingInfoLabel);

    layout->addWidget(pollingGroup);

    // === GRUPPO AUTO-SAVE ===
    QGroupBox* autoSaveGroup = createGroup(tr("Auto-Save Settings"));
    QFormLayout* autoSaveLayout = new QFormLayout(autoSaveGroup);

    m_autoSaveEnabledCheckBox = new QCheckBox(tr("Enable automatic data saving"), this);
    autoSaveLayout->addRow("", m_autoSaveEnabledCheckBox);

    m_autoSaveIntervalSpinBox = new QSpinBox(this);
    m_autoSaveIntervalSpinBox->setRange(1, 60);
    m_autoSaveIntervalSpinBox->setValue(10);
    m_autoSaveIntervalSpinBox->setSuffix(tr(" minutes"));
    m_autoSaveIntervalLabel = new QLabel(tr("Auto-save interval:"), this);
    autoSaveLayout->addRow(m_autoSaveIntervalLabel, m_autoSaveIntervalSpinBox);

    QLabel* autoSaveInfoLabel = new QLabel(
        tr("Automatically save collected data to CSV at regular intervals."), this);
    autoSaveInfoLabel->setWordWrap(true);
    autoSaveInfoLabel->setStyleSheet("color: #888; font-size: 9pt;");
    autoSaveLayout->addRow("", autoSaveInfoLabel);

    layout->addWidget(autoSaveGroup);

    // Connessione per abilitare/disabilitare intervallo
    connect(m_autoSaveEnabledCheckBox, &QCheckBox::stateChanged,
            this, &SettingsTab::onAutoSaveEnabledChanged);
    onAutoSaveEnabledChanged(m_autoSaveEnabledCheckBox->checkState());

    layout->addStretch();

    return widget;
}

QWidget* SettingsTab::createLanguageTab()
{
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);

    // === GRUPPO LANGUAGE SELECTION ===
    QGroupBox* languageGroup = createGroup(tr("Language Selection"));
    QFormLayout* languageLayout = new QFormLayout(languageGroup);

    m_languageComboBox = new QComboBox(this);
    m_languageComboBox->addItem(QIcon(), tr("English"), "en");
    m_languageComboBox->addItem(QIcon(), tr("Italiano"), "it");
    m_languageComboBox->addItem(QIcon(), tr("Français"), "fr");
    m_languageComboBox->addItem(QIcon(), tr("Deutsch"), "de");
    m_languageComboBox->addItem(QIcon(), tr("Español"), "es");

    languageLayout->addRow(tr("Application language:"), m_languageComboBox);

    m_applyLanguageButton = new QPushButton(tr("Apply Language"), this);
    languageLayout->addRow("", m_applyLanguageButton);

    m_languageInfoLabel = new QLabel(
        tr("Note: Application restart may be required for complete language change."), this);
    m_languageInfoLabel->setWordWrap(true);
    m_languageInfoLabel->setStyleSheet("color: #888; font-size: 9pt;");
    languageLayout->addRow("", m_languageInfoLabel);

    layout->addWidget(languageGroup);

    connect(m_applyLanguageButton, &QPushButton::clicked,
            this, &SettingsTab::onApplyLanguageClicked);

    layout->addStretch();

    return widget;
}

QWidget* SettingsTab::createAlarmsTab()
{
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);

    // === GRUPPO TEMPERATURE ALARM ===
    QGroupBox* tempGroup = createGroup(tr("Temperature Alarm"));
    QFormLayout* tempLayout = new QFormLayout(tempGroup);

    m_temperatureAlarmEnabledCheckBox = new QCheckBox(tr("Enable temperature alarm"), this);
    tempLayout->addRow("", m_temperatureAlarmEnabledCheckBox);

    m_temperatureThresholdSpinBox = new QDoubleSpinBox(this);
    m_temperatureThresholdSpinBox->setRange(40.0, 100.0);
    m_temperatureThresholdSpinBox->setValue(70.0);
    m_temperatureThresholdSpinBox->setSuffix(" °C");
    m_temperatureThresholdSpinBox->setDecimals(1);
    tempLayout->addRow(tr("Temperature threshold:"), m_temperatureThresholdSpinBox);

    QLabel* tempInfoLabel = new QLabel(
        tr("Alert when device temperature exceeds this threshold."), this);
    tempInfoLabel->setWordWrap(true);
    tempInfoLabel->setStyleSheet("color: #888; font-size: 9pt;");
    tempLayout->addRow("", tempInfoLabel);

    layout->addWidget(tempGroup);

    // === GRUPPO PHASE IMBALANCE ALARM ===
    QGroupBox* imbalanceGroup = createGroup(tr("Phase Imbalance Alarm"));
    QFormLayout* imbalanceLayout = new QFormLayout(imbalanceGroup);

    m_phaseImbalanceAlarmEnabledCheckBox = new QCheckBox(tr("Enable phase imbalance alarm"), this);
    imbalanceLayout->addRow("", m_phaseImbalanceAlarmEnabledCheckBox);

    m_phaseImbalanceThresholdSpinBox = new QDoubleSpinBox(this);
    m_phaseImbalanceThresholdSpinBox->setRange(5.0, 50.0);
    m_phaseImbalanceThresholdSpinBox->setValue(20.0);
    m_phaseImbalanceThresholdSpinBox->setSuffix(" %");
    m_phaseImbalanceThresholdSpinBox->setDecimals(1);
    imbalanceLayout->addRow(tr("Imbalance threshold:"), m_phaseImbalanceThresholdSpinBox);

    QLabel* imbalanceInfoLabel = new QLabel(
        tr("Alert when phase imbalance exceeds this percentage (recommended: 20%)."), this);
    imbalanceInfoLabel->setWordWrap(true);
    imbalanceInfoLabel->setStyleSheet("color: #888; font-size: 9pt;");
    imbalanceLayout->addRow("", imbalanceInfoLabel);

    layout->addWidget(imbalanceGroup);

    // === GRUPPO POWER ALARM (ALL PHASES) ===
    QGroupBox* powerGroup = createGroup(tr("Power Threshold Alarms"));
    QVBoxLayout* powerMainLayout = new QVBoxLayout(powerGroup);

    // Layout a griglia per le 3 fasi in orizzontale
    QGridLayout* powerGridLayout = new QGridLayout();
    powerGridLayout->setSpacing(10);

    // Riga 0: Checkbox (orizzontali)
    m_powerAlarmEnabledCheckBox = new QCheckBox(tr("Phase A"), this);
    m_powerPhaseBAlarmEnabledCheckBox = new QCheckBox(tr("Phase B"), this);
    m_powerPhaseCAlarmEnabledCheckBox = new QCheckBox(tr("Phase C"), this);

    powerGridLayout->addWidget(m_powerAlarmEnabledCheckBox, 0, 0);
    powerGridLayout->addWidget(m_powerPhaseBAlarmEnabledCheckBox, 0, 1);
    powerGridLayout->addWidget(m_powerPhaseCAlarmEnabledCheckBox, 0, 2);

    // Riga 1: Soglie (orizzontali)
    m_powerThresholdSpinBox = new QDoubleSpinBox(this);
    m_powerThresholdSpinBox->setRange(100.0, 10000.0);
    m_powerThresholdSpinBox->setValue(3000.0);
    m_powerThresholdSpinBox->setSuffix(" W");
    m_powerThresholdSpinBox->setDecimals(0);

    m_powerPhaseBThresholdSpinBox = new QDoubleSpinBox(this);
    m_powerPhaseBThresholdSpinBox->setRange(100.0, 10000.0);
    m_powerPhaseBThresholdSpinBox->setValue(3000.0);
    m_powerPhaseBThresholdSpinBox->setSuffix(" W");
    m_powerPhaseBThresholdSpinBox->setDecimals(0);

    m_powerPhaseCThresholdSpinBox = new QDoubleSpinBox(this);
    m_powerPhaseCThresholdSpinBox->setRange(100.0, 10000.0);
    m_powerPhaseCThresholdSpinBox->setValue(3000.0);
    m_powerPhaseCThresholdSpinBox->setSuffix(" W");
    m_powerPhaseCThresholdSpinBox->setDecimals(0);

    powerGridLayout->addWidget(m_powerThresholdSpinBox, 1, 0);
    powerGridLayout->addWidget(m_powerPhaseBThresholdSpinBox, 1, 1);
    powerGridLayout->addWidget(m_powerPhaseCThresholdSpinBox, 1, 2);

    powerMainLayout->addLayout(powerGridLayout);

    // Label informativa
    QLabel* powerInfoLabel = new QLabel(
        tr("Alert when phase power exceeds the specified threshold."), this);
    powerInfoLabel->setWordWrap(true);
    powerInfoLabel->setStyleSheet("color: #888; font-size: 9pt;");
    powerMainLayout->addWidget(powerInfoLabel);

    layout->addWidget(powerGroup);

    // === GRUPPO LOW POWER ALARMS (ALL PHASES) ===
    QGroupBox* lowPowerGroup = createGroup(tr("Low Power Alarms"));
    QVBoxLayout* lowPowerMainLayout = new QVBoxLayout(lowPowerGroup);

    // Layout a griglia per le 3 fasi in orizzontale
    QGridLayout* lowPowerGridLayout = new QGridLayout();
    lowPowerGridLayout->setSpacing(10);

    // Riga 0: Checkbox (orizzontali)
    m_lowPowerPhaseAAlarmEnabledCheckBox = new QCheckBox(tr("Phase A"), this);
    m_lowPowerPhaseBAlarmEnabledCheckBox = new QCheckBox(tr("Phase B"), this);
    m_lowPowerPhaseCAlarmEnabledCheckBox = new QCheckBox(tr("Phase C"), this);

    lowPowerGridLayout->addWidget(m_lowPowerPhaseAAlarmEnabledCheckBox, 0, 0);
    lowPowerGridLayout->addWidget(m_lowPowerPhaseBAlarmEnabledCheckBox, 0, 1);
    lowPowerGridLayout->addWidget(m_lowPowerPhaseCAlarmEnabledCheckBox, 0, 2);

    // Riga 1: Soglie (orizzontali)
    m_lowPowerPhaseAThresholdSpinBox = new QDoubleSpinBox(this);
    m_lowPowerPhaseAThresholdSpinBox->setRange(0.0, 500.0);
    m_lowPowerPhaseAThresholdSpinBox->setValue(30.0);
    m_lowPowerPhaseAThresholdSpinBox->setSuffix(" W");
    m_lowPowerPhaseAThresholdSpinBox->setDecimals(1);

    m_lowPowerPhaseBThresholdSpinBox = new QDoubleSpinBox(this);
    m_lowPowerPhaseBThresholdSpinBox->setRange(0.0, 500.0);
    m_lowPowerPhaseBThresholdSpinBox->setValue(30.0);
    m_lowPowerPhaseBThresholdSpinBox->setSuffix(" W");
    m_lowPowerPhaseBThresholdSpinBox->setDecimals(1);

    m_lowPowerPhaseCThresholdSpinBox = new QDoubleSpinBox(this);
    m_lowPowerPhaseCThresholdSpinBox->setRange(0.0, 500.0);
    m_lowPowerPhaseCThresholdSpinBox->setValue(30.0);
    m_lowPowerPhaseCThresholdSpinBox->setSuffix(" W");
    m_lowPowerPhaseCThresholdSpinBox->setDecimals(1);

    lowPowerGridLayout->addWidget(m_lowPowerPhaseAThresholdSpinBox, 1, 0);
    lowPowerGridLayout->addWidget(m_lowPowerPhaseBThresholdSpinBox, 1, 1);
    lowPowerGridLayout->addWidget(m_lowPowerPhaseCThresholdSpinBox, 1, 2);

    lowPowerMainLayout->addLayout(lowPowerGridLayout);

    // Label informativa
    QLabel* lowPowerInfoLabel = new QLabel(
        tr("Alert with sound when phase power drops below the specified threshold."), this);
    lowPowerInfoLabel->setWordWrap(true);
    lowPowerInfoLabel->setStyleSheet("color: #888; font-size: 9pt;");
    lowPowerMainLayout->addWidget(lowPowerInfoLabel);

    layout->addWidget(lowPowerGroup);

    layout->addStretch();

    return widget;
}

QWidget* SettingsTab::createAdvancedTab()
{
    QWidget* widget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(widget);

    // === LAYOUT A GRIGLIA 2x2 per ottimizzare spazio verticale ===
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setSpacing(10);

    // === ROW 0, COL 0: THEME + DEBUG ===
    QWidget* leftTopWidget = new QWidget(this);
    QVBoxLayout* leftTopLayout = new QVBoxLayout(leftTopWidget);
    leftTopLayout->setContentsMargins(0, 0, 0, 0);

    // THEME
    QGroupBox* themeGroup = createGroup(tr("Theme"));
    QFormLayout* themeLayout = new QFormLayout(themeGroup);

    m_themeComboBox = new QComboBox(this);
    m_themeComboBox->addItem(tr("Dark"), "dark");
    m_themeComboBox->addItem(tr("Light"), "light");
    m_themeComboBox->setEnabled(true);
    themeLayout->addRow(tr("Theme:"), m_themeComboBox);

    // Connect theme change
    connect(m_themeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsTab::onThemeChanged);

    leftTopLayout->addWidget(themeGroup);

    // DEBUG & LOGGING
    QGroupBox* debugGroup = createGroup(tr("Debug & Logging"));
    QVBoxLayout* debugLayout = new QVBoxLayout(debugGroup);

    m_debugModeCheckBox = new QCheckBox(tr("Enable verbose console output"), this);
    debugLayout->addWidget(m_debugModeCheckBox);

    // Log level
    QHBoxLayout* logLevelLayout = new QHBoxLayout();
    logLevelLayout->addWidget(new QLabel(tr("Log Level:"), this));
    m_logLevelCombo = new QComboBox(this);
    m_logLevelCombo->addItem(tr("Debug"), static_cast<int>(Logger::Debug));
    m_logLevelCombo->addItem(tr("Info"), static_cast<int>(Logger::Info));
    m_logLevelCombo->addItem(tr("Warning"), static_cast<int>(Logger::Warning));
    m_logLevelCombo->addItem(tr("Error"), static_cast<int>(Logger::Error));
    m_logLevelCombo->addItem(tr("Critical"), static_cast<int>(Logger::Critical));
    m_logLevelCombo->setCurrentIndex(1); // Default: Info
    logLevelLayout->addWidget(m_logLevelCombo);
    debugLayout->addLayout(logLevelLayout);

    // View logs button
    m_viewLogsButton = new QPushButton(tr("View Logs"), this);
    m_viewLogsButton->setToolTip(tr("Open log viewer dialog"));
    debugLayout->addWidget(m_viewLogsButton);

    leftTopLayout->addWidget(debugGroup);
    leftTopLayout->addStretch();

    gridLayout->addWidget(leftTopWidget, 0, 0);

    // === ROW 0, COL 1: HISTORY CHARTS ===
    QGroupBox* historyChartsGroup = createGroup(tr("History Charts"));
    QGridLayout* chartsLayout = new QGridLayout(historyChartsGroup);

    QLabel* samplesLabel = new QLabel(tr("Real-Time Samples:"), this);
    m_historySamplesChartTypeCombo = new QComboBox(this);
    m_historySamplesChartTypeCombo->addItem(tr("Line Chart"), "line");
    m_historySamplesChartTypeCombo->addItem(tr("Bar Chart"), "bar");
    chartsLayout->addWidget(samplesLabel, 0, 0);
    chartsLayout->addWidget(m_historySamplesChartTypeCombo, 0, 1);

    QLabel* dailyEnergyLabel = new QLabel(tr("Daily Energy:"), this);
    m_historyDailyEnergyChartTypeCombo = new QComboBox(this);
    m_historyDailyEnergyChartTypeCombo->addItem(tr("Line Chart"), "line");
    m_historyDailyEnergyChartTypeCombo->addItem(tr("Bar Chart"), "bar");
    chartsLayout->addWidget(dailyEnergyLabel, 1, 0);
    chartsLayout->addWidget(m_historyDailyEnergyChartTypeCombo, 1, 1);

    gridLayout->addWidget(historyChartsGroup, 0, 1);

    // === ROW 1, COL 0-1: DATABASE MAINTENANCE (span 2 columns) ===
    QGroupBox* dbMaintenanceGroup = createGroup(tr("Database Maintenance"));
    QVBoxLayout* dbMaintenanceLayout = new QVBoxLayout(dbMaintenanceGroup);

    // Info in layout orizzontale compatto
    QHBoxLayout* dbInfoLayout = new QHBoxLayout();

    m_dbSizeLabel = new QLabel(tr("Size: N/A"), this);
    m_dbSizeLabel->setStyleSheet("color: #d0d0d0; font-size: 9pt;");
    dbInfoLayout->addWidget(m_dbSizeLabel);

    dbInfoLayout->addSpacing(20);

    m_dbStatsLabel = new QLabel(tr("Records: N/A"), this);
    m_dbStatsLabel->setStyleSheet("color: #d0d0d0; font-size: 9pt;");
    dbInfoLayout->addWidget(m_dbStatsLabel);

    dbInfoLayout->addStretch();

    dbMaintenanceLayout->addLayout(dbInfoLayout);

    // Path su riga separata (più lungo)
    m_dbPathLabel = new QLabel(tr("Database: N/A"), this);
    m_dbPathLabel->setWordWrap(true);
    m_dbPathLabel->setStyleSheet("color: #888; font-size: 8pt;");
    dbMaintenanceLayout->addWidget(m_dbPathLabel);

    dbMaintenanceLayout->addSpacing(8);

    // Buttons in layout orizzontale
    QHBoxLayout* dbButtonsLayout = new QHBoxLayout();

    m_compactDatabaseButton = new QPushButton(tr("Compact"), this);
    m_compactDatabaseButton->setToolTip(tr("Optimize database (VACUUM)"));
    dbButtonsLayout->addWidget(m_compactDatabaseButton);

    m_backupDatabaseButton = new QPushButton(tr("Backup"), this);
    m_backupDatabaseButton->setToolTip(tr("Create backup copy"));
    dbButtonsLayout->addWidget(m_backupDatabaseButton);

    m_clearOldDataButton = new QPushButton(tr("Clear Old Data"), this);
    m_clearOldDataButton->setToolTip(tr("Delete old samples"));
    dbButtonsLayout->addWidget(m_clearOldDataButton);

    dbMaintenanceLayout->addLayout(dbButtonsLayout);

    gridLayout->addWidget(dbMaintenanceGroup, 1, 0, 1, 2);  // span 2 columns

    mainLayout->addLayout(gridLayout);

    // Connect database maintenance signals
    connect(m_compactDatabaseButton, &QPushButton::clicked,
            this, &SettingsTab::onCompactDatabaseClicked);
    connect(m_backupDatabaseButton, &QPushButton::clicked,
            this, &SettingsTab::onBackupDatabaseClicked);
    connect(m_clearOldDataButton, &QPushButton::clicked,
            this, &SettingsTab::onClearOldDataClicked);

    // === RESET SETTINGS (full width in basso) ===
    QGroupBox* resetGroup = createGroup(tr("Reset Settings"));
    QHBoxLayout* resetLayout = new QHBoxLayout(resetGroup);

    m_resetSettingsButton = new QPushButton(tr("Reset All Settings to Default"), this);
    m_resetSettingsButton->setStyleSheet("background-color: #c62828; color: white;");
    m_resetSettingsButton->setMaximumWidth(250);
    resetLayout->addWidget(m_resetSettingsButton);

    QLabel* resetInfoLabel = new QLabel(
        tr("Warning: This will reset all settings to their default values."), this);
    resetInfoLabel->setWordWrap(true);
    resetInfoLabel->setStyleSheet("color: #ff5252; font-size: 9pt;");
    resetLayout->addWidget(resetInfoLabel);

    resetLayout->addStretch();

    mainLayout->addWidget(resetGroup);

    connect(m_resetSettingsButton, &QPushButton::clicked,
            this, &SettingsTab::onResetSettingsClicked);

    // Connect logging signals
    connect(m_viewLogsButton, &QPushButton::clicked,
            this, &SettingsTab::onViewLogsClicked);
    connect(m_logLevelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsTab::onLogLevelChanged);

    mainLayout->addStretch();

    return widget;
}

QGroupBox* SettingsTab::createGroup(const QString& title)
{
    QGroupBox* group = new QGroupBox(title, this);
    QFont font = group->font();
    font.setBold(true);
    group->setFont(font);
    return group;
}

void SettingsTab::loadSettings()
{
    QSettings settings("ShellyLogger", "MainWindow");

    // General
    int pollingInterval = settings.value("Settings/defaultPollingInterval", 60).toInt();
    m_defaultPollingIntervalSpinBox->setValue(pollingInterval);

    bool autoSaveEnabled = settings.value("Settings/autoSaveEnabled", false).toBool();
    m_autoSaveEnabledCheckBox->setChecked(autoSaveEnabled);

    int autoSaveInterval = settings.value("Settings/autoSaveInterval", 10).toInt();
    m_autoSaveIntervalSpinBox->setValue(autoSaveInterval);

    // Language
    QString language = settings.value("Settings/language", "en").toString();
    int langIndex = m_languageComboBox->findData(language);
    if (langIndex >= 0) {
        m_languageComboBox->setCurrentIndex(langIndex);
    }

    // Alarms
    bool tempAlarmEnabled = settings.value("Alarms/temperatureEnabled", false).toBool();
    m_temperatureAlarmEnabledCheckBox->setChecked(tempAlarmEnabled);

    double tempThreshold = settings.value("Alarms/temperatureThreshold", 70.0).toDouble();
    m_temperatureThresholdSpinBox->setValue(tempThreshold);

    bool imbalanceAlarmEnabled = settings.value("Alarms/phaseImbalanceEnabled", false).toBool();
    m_phaseImbalanceAlarmEnabledCheckBox->setChecked(imbalanceAlarmEnabled);

    double imbalanceThreshold = settings.value("Alarms/phaseImbalanceThreshold", 20.0).toDouble();
    m_phaseImbalanceThresholdSpinBox->setValue(imbalanceThreshold);

    bool powerAlarmEnabled = settings.value("Alarms/powerEnabled", false).toBool();
    m_powerAlarmEnabledCheckBox->setChecked(powerAlarmEnabled);

    double powerThreshold = settings.value("Alarms/powerThreshold", 3000.0).toDouble();
    m_powerThresholdSpinBox->setValue(powerThreshold);

    bool powerPhaseBAlarmEnabled = settings.value("Alarms/powerPhaseBEnabled", false).toBool();
    m_powerPhaseBAlarmEnabledCheckBox->setChecked(powerPhaseBAlarmEnabled);

    double powerPhaseBThreshold = settings.value("Alarms/powerPhaseBThreshold", 3000.0).toDouble();
    m_powerPhaseBThresholdSpinBox->setValue(powerPhaseBThreshold);

    bool powerPhaseCAlarmEnabled = settings.value("Alarms/powerPhaseCEnabled", false).toBool();
    m_powerPhaseCAlarmEnabledCheckBox->setChecked(powerPhaseCAlarmEnabled);

    double powerPhaseCThreshold = settings.value("Alarms/powerPhaseCThreshold", 3000.0).toDouble();
    m_powerPhaseCThresholdSpinBox->setValue(powerPhaseCThreshold);

    bool lowPowerPhaseAAlarmEnabled = settings.value("Alarms/lowPowerPhaseAEnabled", false).toBool();
    m_lowPowerPhaseAAlarmEnabledCheckBox->setChecked(lowPowerPhaseAAlarmEnabled);

    double lowPowerPhaseAThreshold = settings.value("Alarms/lowPowerPhaseAThreshold", 30.0).toDouble();
    m_lowPowerPhaseAThresholdSpinBox->setValue(lowPowerPhaseAThreshold);

    bool lowPowerPhaseBAlarmEnabled = settings.value("Alarms/lowPowerPhaseBEnabled", false).toBool();
    m_lowPowerPhaseBAlarmEnabledCheckBox->setChecked(lowPowerPhaseBAlarmEnabled);

    double lowPowerPhaseBThreshold = settings.value("Alarms/lowPowerPhaseBThreshold", 30.0).toDouble();
    m_lowPowerPhaseBThresholdSpinBox->setValue(lowPowerPhaseBThreshold);

    bool lowPowerPhaseCAlarmEnabled = settings.value("Alarms/lowPowerPhaseCEnabled", false).toBool();
    m_lowPowerPhaseCAlarmEnabledCheckBox->setChecked(lowPowerPhaseCAlarmEnabled);

    double lowPowerPhaseCThreshold = settings.value("Alarms/lowPowerPhaseCThreshold", 30.0).toDouble();
    m_lowPowerPhaseCThresholdSpinBox->setValue(lowPowerPhaseCThreshold);

    // Advanced
    bool debugMode = settings.value("Advanced/debugMode", false).toBool();
    m_debugModeCheckBox->setChecked(debugMode);

    QString theme = settings.value("Advanced/theme", "dark").toString();
    int themeIndex = m_themeComboBox->findData(theme);
    if (themeIndex >= 0) {
        m_themeComboBox->setCurrentIndex(themeIndex);
    }

    // Logging
    int logLevel = settings.value("Logging/logLevel", static_cast<int>(Logger::Info)).toInt();
    int logLevelIndex = m_logLevelCombo->findData(logLevel);
    if (logLevelIndex >= 0) {
        m_logLevelCombo->setCurrentIndex(logLevelIndex);
    }
    // Apply log level immediately
    Logger::instance().setLogLevel(static_cast<Logger::LogLevel>(logLevel));

    // History Charts
    QString historySamplesChartType = settings.value("HistoryCharts/samplesChartType", "line").toString();
    int samplesChartIndex = m_historySamplesChartTypeCombo->findData(historySamplesChartType);
    if (samplesChartIndex >= 0) {
        m_historySamplesChartTypeCombo->setCurrentIndex(samplesChartIndex);
    }

    QString historyDailyEnergyChartType = settings.value("HistoryCharts/dailyEnergyChartType", "bar").toString();
    int dailyEnergyChartIndex = m_historyDailyEnergyChartTypeCombo->findData(historyDailyEnergyChartType);
    if (dailyEnergyChartIndex >= 0) {
        m_historyDailyEnergyChartTypeCombo->setCurrentIndex(dailyEnergyChartIndex);
    }

    qDebug() << "SettingsTab: Settings loaded";
}

void SettingsTab::saveSettings()
{
    QSettings settings("ShellyLogger", "MainWindow");

    // General
    settings.setValue("Settings/defaultPollingInterval", m_defaultPollingIntervalSpinBox->value());
    settings.setValue("Settings/autoSaveEnabled", m_autoSaveEnabledCheckBox->isChecked());
    settings.setValue("Settings/autoSaveInterval", m_autoSaveIntervalSpinBox->value());

    // Language
    QString languageCode = m_languageComboBox->currentData().toString();
    settings.setValue("Settings/language", languageCode);

    // Alarms
    settings.setValue("Alarms/temperatureEnabled", m_temperatureAlarmEnabledCheckBox->isChecked());
    settings.setValue("Alarms/temperatureThreshold", m_temperatureThresholdSpinBox->value());
    settings.setValue("Alarms/phaseImbalanceEnabled", m_phaseImbalanceAlarmEnabledCheckBox->isChecked());
    settings.setValue("Alarms/phaseImbalanceThreshold", m_phaseImbalanceThresholdSpinBox->value());
    settings.setValue("Alarms/powerEnabled", m_powerAlarmEnabledCheckBox->isChecked());
    settings.setValue("Alarms/powerThreshold", m_powerThresholdSpinBox->value());
    settings.setValue("Alarms/powerPhaseBEnabled", m_powerPhaseBAlarmEnabledCheckBox->isChecked());
    settings.setValue("Alarms/powerPhaseBThreshold", m_powerPhaseBThresholdSpinBox->value());
    settings.setValue("Alarms/powerPhaseCEnabled", m_powerPhaseCAlarmEnabledCheckBox->isChecked());
    settings.setValue("Alarms/powerPhaseCThreshold", m_powerPhaseCThresholdSpinBox->value());
    settings.setValue("Alarms/lowPowerPhaseAEnabled", m_lowPowerPhaseAAlarmEnabledCheckBox->isChecked());
    settings.setValue("Alarms/lowPowerPhaseAThreshold", m_lowPowerPhaseAThresholdSpinBox->value());
    settings.setValue("Alarms/lowPowerPhaseBEnabled", m_lowPowerPhaseBAlarmEnabledCheckBox->isChecked());
    settings.setValue("Alarms/lowPowerPhaseBThreshold", m_lowPowerPhaseBThresholdSpinBox->value());
    settings.setValue("Alarms/lowPowerPhaseCEnabled", m_lowPowerPhaseCAlarmEnabledCheckBox->isChecked());
    settings.setValue("Alarms/lowPowerPhaseCThreshold", m_lowPowerPhaseCThresholdSpinBox->value());

    // Advanced
    settings.setValue("Advanced/debugMode", m_debugModeCheckBox->isChecked());
    settings.setValue("Advanced/theme", m_themeComboBox->currentData().toString());

    // Logging
    settings.setValue("Logging/logLevel", m_logLevelCombo->currentData().toInt());

    // History Charts
    settings.setValue("HistoryCharts/samplesChartType", m_historySamplesChartTypeCombo->currentData().toString());
    settings.setValue("HistoryCharts/dailyEnergyChartType", m_historyDailyEnergyChartTypeCombo->currentData().toString());

    qDebug() << "SettingsTab: Settings saved";

    emit settingsSaved();
}

// Getters
int SettingsTab::getDefaultPollingInterval() const
{
    return m_defaultPollingIntervalSpinBox->value();
}

QString SettingsTab::getCurrentLanguage() const
{
    return m_languageComboBox->currentData().toString();
}

bool SettingsTab::isAutoSaveEnabled() const
{
    return m_autoSaveEnabledCheckBox->isChecked();
}

int SettingsTab::getAutoSaveInterval() const
{
    return m_autoSaveIntervalSpinBox->value();
}

bool SettingsTab::isTemperatureAlarmEnabled() const
{
    return m_temperatureAlarmEnabledCheckBox->isChecked();
}

double SettingsTab::getTemperatureAlarmThreshold() const
{
    return m_temperatureThresholdSpinBox->value();
}

bool SettingsTab::isPhaseImbalanceAlarmEnabled() const
{
    return m_phaseImbalanceAlarmEnabledCheckBox->isChecked();
}

double SettingsTab::getPhaseImbalanceAlarmThreshold() const
{
    return m_phaseImbalanceThresholdSpinBox->value();
}

bool SettingsTab::isPowerAlarmEnabled() const
{
    return m_powerAlarmEnabledCheckBox->isChecked();
}

double SettingsTab::getPowerAlarmThreshold() const
{
    return m_powerThresholdSpinBox->value();
}

bool SettingsTab::isPowerPhaseBAlarmEnabled() const
{
    return m_powerPhaseBAlarmEnabledCheckBox->isChecked();
}

double SettingsTab::getPowerPhaseBThreshold() const
{
    return m_powerPhaseBThresholdSpinBox->value();
}

bool SettingsTab::isPowerPhaseCAlarmEnabled() const
{
    return m_powerPhaseCAlarmEnabledCheckBox->isChecked();
}

double SettingsTab::getPowerPhaseCThreshold() const
{
    return m_powerPhaseCThresholdSpinBox->value();
}

bool SettingsTab::isLowPowerPhaseAAlarmEnabled() const
{
    return m_lowPowerPhaseAAlarmEnabledCheckBox->isChecked();
}

double SettingsTab::getLowPowerPhaseAThreshold() const
{
    return m_lowPowerPhaseAThresholdSpinBox->value();
}

bool SettingsTab::isLowPowerPhaseBAlarmEnabled() const
{
    return m_lowPowerPhaseBAlarmEnabledCheckBox->isChecked();
}

double SettingsTab::getLowPowerPhaseBThreshold() const
{
    return m_lowPowerPhaseBThresholdSpinBox->value();
}

bool SettingsTab::isLowPowerPhaseCAlarmEnabled() const
{
    return m_lowPowerPhaseCAlarmEnabledCheckBox->isChecked();
}

double SettingsTab::getLowPowerPhaseCThreshold() const
{
    return m_lowPowerPhaseCThresholdSpinBox->value();
}

QString SettingsTab::getHistorySamplesChartType() const
{
    return m_historySamplesChartTypeCombo->currentData().toString();
}

QString SettingsTab::getHistoryDailyEnergyChartType() const
{
    return m_historyDailyEnergyChartTypeCombo->currentData().toString();
}

QString SettingsTab::getCurrentTheme() const
{
    return m_themeComboBox->currentData().toString();
}

// Slots
void SettingsTab::onAutoSaveEnabledChanged(int state)
{
    bool enabled = (state == Qt::Checked);
    m_autoSaveIntervalSpinBox->setEnabled(enabled);
    m_autoSaveIntervalLabel->setEnabled(enabled);
}

void SettingsTab::onApplyLanguageClicked()
{
    QString languageCode = m_languageComboBox->currentData().toString();
    emit languageChanged(languageCode);

    QMessageBox::information(this, tr("Language Changed"),
                            tr("Language has been changed to: %1\n\n"
                               "Some changes may require application restart.")
                            .arg(m_languageComboBox->currentText()));
}

void SettingsTab::onThemeChanged(int index)
{
    Q_UNUSED(index);
    QString theme = m_themeComboBox->currentData().toString();
    emit themeChanged(theme);
    qDebug() << "SettingsTab: Theme changed to" << theme;
}

void SettingsTab::onSaveClicked()
{
    saveSettings();

    QMessageBox::information(this, tr("Settings Saved"),
                            tr("All settings have been saved successfully."));
}

void SettingsTab::onCancelClicked()
{
    // Ricarica le impostazioni precedenti
    loadSettings();

    QMessageBox::information(this, tr("Changes Cancelled"),
                            tr("All changes have been discarded."));
}

void SettingsTab::onResetSettingsClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Reset Settings"),
        tr("Are you sure you want to reset all settings to their default values?\n\n"
           "This action cannot be undone."),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        // Reset a valori di default
        m_defaultPollingIntervalSpinBox->setValue(60);
        m_autoSaveEnabledCheckBox->setChecked(false);
        m_autoSaveIntervalSpinBox->setValue(10);
        m_languageComboBox->setCurrentIndex(0); // English
        m_temperatureAlarmEnabledCheckBox->setChecked(false);
        m_temperatureThresholdSpinBox->setValue(70.0);
        m_phaseImbalanceAlarmEnabledCheckBox->setChecked(false);
        m_phaseImbalanceThresholdSpinBox->setValue(20.0);
        m_powerAlarmEnabledCheckBox->setChecked(false);
        m_powerThresholdSpinBox->setValue(3000.0);
        m_powerPhaseBAlarmEnabledCheckBox->setChecked(false);
        m_powerPhaseBThresholdSpinBox->setValue(3000.0);
        m_powerPhaseCAlarmEnabledCheckBox->setChecked(false);
        m_powerPhaseCThresholdSpinBox->setValue(3000.0);
        m_lowPowerPhaseAAlarmEnabledCheckBox->setChecked(false);
        m_lowPowerPhaseAThresholdSpinBox->setValue(30.0);
        m_lowPowerPhaseBAlarmEnabledCheckBox->setChecked(false);
        m_lowPowerPhaseBThresholdSpinBox->setValue(30.0);
        m_lowPowerPhaseCAlarmEnabledCheckBox->setChecked(false);
        m_lowPowerPhaseCThresholdSpinBox->setValue(30.0);
        m_debugModeCheckBox->setChecked(false);
        m_themeComboBox->setCurrentIndex(0); // Dark
        m_historySamplesChartTypeCombo->setCurrentIndex(0); // Line
        m_historyDailyEnergyChartTypeCombo->setCurrentIndex(1); // Bar (default per daily energy)

        // Salva i valori resettati
        saveSettings();

        QMessageBox::information(this, tr("Settings Reset"),
                                tr("All settings have been reset to default values."));
    }
}

// Database Manager setter
void SettingsTab::setDatabaseManager(DatabaseManager* dbManager)
{
    m_databaseManager = dbManager;
    updateDatabaseInfo();
}

// Database Maintenance Slots
void SettingsTab::onCompactDatabaseClicked()
{
    if (!m_databaseManager) {
        QMessageBox::warning(this, tr("Database Error"),
                           tr("Database manager not initialized."));
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Compact Database"),
        tr("This operation will optimize the database by reclaiming unused space.\n\n"
           "This may take a few moments. Continue?"),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        if (m_databaseManager->vacuum()) {
            updateDatabaseInfo();
            QMessageBox::information(this, tr("Success"),
                                   tr("Database compacted successfully."));
        } else {
            QMessageBox::critical(this, tr("Error"),
                                tr("Failed to compact database:\n%1")
                                .arg(m_databaseManager->lastError()));
        }
    }
}

void SettingsTab::onBackupDatabaseClicked()
{
    if (!m_databaseManager) {
        QMessageBox::warning(this, tr("Database Error"),
                           tr("Database manager not initialized."));
        return;
    }

    // Suggerisci nome backup automatico
    QString suggestedName = QString("shelly_history_backup_%1.db")
                            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

    QString backupPath = QFileDialog::getSaveFileName(
        this,
        tr("Save Database Backup"),
        suggestedName,
        tr("Database Files (*.db);;All Files (*.*)")
    );

    if (!backupPath.isEmpty()) {
        if (m_databaseManager->backupDatabase(backupPath)) {
            QFileInfo fileInfo(backupPath);
            QMessageBox::information(this, tr("Success"),
                                   tr("Database backup created successfully:\n%1\n\nSize: %2 MB")
                                   .arg(backupPath)
                                   .arg(fileInfo.size() / 1024.0 / 1024.0, 0, 'f', 2));
        } else {
            QMessageBox::critical(this, tr("Error"),
                                tr("Failed to create backup:\n%1")
                                .arg(m_databaseManager->lastError()));
        }
    }
}

void SettingsTab::onClearOldDataClicked()
{
    if (!m_databaseManager) {
        QMessageBox::warning(this, tr("Database Error"),
                           tr("Database manager not initialized."));
        return;
    }

    // Dialog per scegliere numero di giorni
    bool ok;
    int days = QInputDialog::getInt(
        this,
        tr("Clear Old Data"),
        tr("Delete all samples older than how many days?\n\n"
           "WARNING: This action cannot be undone!"),
        30,  // default
        1,   // min
        3650,  // max (10 years)
        1,   // step
        &ok
    );

    if (ok) {
        qint64 cutoffTimestamp = QDateTime::currentSecsSinceEpoch() - (days * 24 * 60 * 60);

        QMessageBox::StandardButton reply = QMessageBox::warning(
            this,
            tr("Confirm Deletion"),
            tr("Are you sure you want to delete all samples older than %1 days?\n\n"
               "This action CANNOT be undone!").arg(days),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            int deletedCount = m_databaseManager->deleteSamplesBefore(cutoffTimestamp);

            if (deletedCount >= 0) {
                updateDatabaseInfo();
                QMessageBox::information(this, tr("Success"),
                                       tr("Deleted %1 old samples successfully.").arg(deletedCount));
            } else {
                QMessageBox::critical(this, tr("Error"),
                                    tr("Failed to delete old data:\n%1")
                                    .arg(m_databaseManager->lastError()));
            }
        }
    }
}

void SettingsTab::updateDatabaseInfo()
{
    if (!m_databaseManager || !m_databaseManager->isOpen()) {
        m_dbPathLabel->setText(tr("Database: N/A"));
        m_dbSizeLabel->setText(tr("Size: N/A"));
        m_dbStatsLabel->setText(tr("Records: N/A"));
        return;
    }

    // Path
    QString dbPath = m_databaseManager->getDatabasePath();
    m_dbPathLabel->setText(tr("Database: %1").arg(dbPath));

    // Size
    qint64 sizeBytes = m_databaseManager->getDatabaseSize();
    double sizeMB = sizeBytes / 1024.0 / 1024.0;
    m_dbSizeLabel->setText(tr("Size: %1 MB").arg(sizeMB, 0, 'f', 2));

    // Stats
    qint64 sampleCount = m_databaseManager->getSampleCount();
    qint64 dailyEnergyCount = m_databaseManager->getDailyEnergyCount();
    m_dbStatsLabel->setText(tr("Records: %1 samples, %2 daily energy entries")
                            .arg(sampleCount)
                            .arg(dailyEnergyCount));
}

// Logging Slots
void SettingsTab::onViewLogsClicked()
{
    LogViewerDialog* dialog = new LogViewerDialog(this);
    dialog->exec();
    dialog->deleteLater();
}

void SettingsTab::onLogLevelChanged()
{
    int logLevel = m_logLevelCombo->currentData().toInt();
    Logger::instance().setLogLevel(static_cast<Logger::LogLevel>(logLevel));
    qDebug() << "SettingsTab: Log level changed to" << Logger::logLevelToString(static_cast<Logger::LogLevel>(logLevel));
}

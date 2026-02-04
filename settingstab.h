#ifndef SETTINGSTAB_H
#define SETTINGSTAB_H

#include <QWidget>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>

/**
 * @brief Widget per le impostazioni dell'applicazione
 *
 * Organizzato in sezioni:
 * - General: Polling interval di default, auto-save
 * - Language: Selezione lingua con cambio runtime
 * - Alarms: Soglie allarmi (temperatura, sbilanciamento, potenza)
 * - Advanced: Theme (placeholder), debug options
 */
class SettingsTab : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsTab(QWidget *parent = nullptr);
    ~SettingsTab();

    // Carica le impostazioni salvate
    void loadSettings();

    // Salva le impostazioni
    void saveSettings();

    // Getters per impostazioni
    int getDefaultPollingInterval() const;
    QString getCurrentLanguage() const;
    bool isAutoSaveEnabled() const;
    int getAutoSaveInterval() const;

    // Alarms getters
    bool isTemperatureAlarmEnabled() const;
    double getTemperatureAlarmThreshold() const;
    bool isPhaseImbalanceAlarmEnabled() const;
    double getPhaseImbalanceAlarmThreshold() const;
    bool isPowerAlarmEnabled() const;
    double getPowerAlarmThreshold() const;
    bool isPowerPhaseBAlarmEnabled() const;
    double getPowerPhaseBThreshold() const;
    bool isPowerPhaseCAlarmEnabled() const;
    double getPowerPhaseCThreshold() const;
    bool isLowPowerPhaseAAlarmEnabled() const;
    double getLowPowerPhaseAThreshold() const;
    bool isLowPowerPhaseBAlarmEnabled() const;
    double getLowPowerPhaseBThreshold() const;
    bool isLowPowerPhaseCAlarmEnabled() const;
    double getLowPowerPhaseCThreshold() const;

    // History charts getters
    QString getHistorySamplesChartType() const;
    QString getHistoryDailyEnergyChartType() const;

    // Theme getter
    QString getCurrentTheme() const;

    // Database Manager
    void setDatabaseManager(class DatabaseManager* dbManager);

private:
    // Setup UI
    void setupUi();
    QWidget* createGeneralTab();
    QWidget* createLanguageTab();
    QWidget* createAlarmsTab();
    QWidget* createAdvancedTab();

    // Helper per creare gruppi
    QGroupBox* createGroup(const QString& title);

    // Sub-tabs
    QTabWidget* m_subTabWidget;

    // === GENERAL TAB ===
    QSpinBox* m_defaultPollingIntervalSpinBox;
    QCheckBox* m_autoSaveEnabledCheckBox;
    QSpinBox* m_autoSaveIntervalSpinBox;
    QLabel* m_autoSaveIntervalLabel;

    // === LANGUAGE TAB ===
    QComboBox* m_languageComboBox;
    QLabel* m_languageInfoLabel;
    QPushButton* m_applyLanguageButton;

    // === ALARMS TAB ===
    QCheckBox* m_temperatureAlarmEnabledCheckBox;
    QDoubleSpinBox* m_temperatureThresholdSpinBox;
    QCheckBox* m_phaseImbalanceAlarmEnabledCheckBox;
    QDoubleSpinBox* m_phaseImbalanceThresholdSpinBox;
    QCheckBox* m_powerAlarmEnabledCheckBox;
    QDoubleSpinBox* m_powerThresholdSpinBox;
    QCheckBox* m_powerPhaseBAlarmEnabledCheckBox;
    QDoubleSpinBox* m_powerPhaseBThresholdSpinBox;
    QCheckBox* m_powerPhaseCAlarmEnabledCheckBox;
    QDoubleSpinBox* m_powerPhaseCThresholdSpinBox;
    QCheckBox* m_lowPowerPhaseAAlarmEnabledCheckBox;
    QDoubleSpinBox* m_lowPowerPhaseAThresholdSpinBox;
    QCheckBox* m_lowPowerPhaseBAlarmEnabledCheckBox;
    QDoubleSpinBox* m_lowPowerPhaseBThresholdSpinBox;
    QCheckBox* m_lowPowerPhaseCAlarmEnabledCheckBox;
    QDoubleSpinBox* m_lowPowerPhaseCThresholdSpinBox;

    // === ADVANCED TAB ===
    QComboBox* m_themeComboBox;
    QCheckBox* m_debugModeCheckBox;
    QComboBox* m_logLevelCombo;
    QPushButton* m_viewLogsButton;
    QComboBox* m_historySamplesChartTypeCombo;
    QComboBox* m_historyDailyEnergyChartTypeCombo;
    QPushButton* m_resetSettingsButton;

    // Database Maintenance
    QLabel* m_dbPathLabel;
    QLabel* m_dbSizeLabel;
    QLabel* m_dbStatsLabel;
    QPushButton* m_compactDatabaseButton;
    QPushButton* m_backupDatabaseButton;
    QPushButton* m_clearOldDataButton;

    // Pulsanti principali
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;

signals:
    // Emesso quando l'utente cambia lingua
    void languageChanged(const QString& languageCode);

    // Emesso quando l'utente cambia tema
    void themeChanged(const QString& theme);

    // Emesso quando le impostazioni vengono salvate
    void settingsSaved();

private slots:
    void onAutoSaveEnabledChanged(int state);
    void onApplyLanguageClicked();
    void onThemeChanged(int index);
    void onSaveClicked();
    void onCancelClicked();
    void onResetSettingsClicked();

    // Database Maintenance slots
    void onCompactDatabaseClicked();
    void onBackupDatabaseClicked();
    void onClearOldDataClicked();
    void updateDatabaseInfo();

    // Logging slots
    void onViewLogsClicked();
    void onLogLevelChanged();

private:
    class DatabaseManager* m_databaseManager;
};

#endif // SETTINGSTAB_H

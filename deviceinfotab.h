#ifndef DEVICEINFOTAB_H
#define DEVICEINFOTAB_H

#include <QWidget>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include "deviceinfo.h"

/**
 * @brief Widget per visualizzare le informazioni del dispositivo Shelly EM3
 *
 * Organizzato in sotto-tab:
 * - General: Modello, firmware, IP, MAC, uptime
 * - WiFi: SSID, signal strength, cloud status
 * - System: RAM, storage, temperature, alarms
 * - Energy: Total kWh per phase
 * - Diagnostics: Neutral current, phase imbalance
 */
class DeviceInfoTab : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceInfoTab(QWidget *parent = nullptr);
    ~DeviceInfoTab();

    // Aggiorna la visualizzazione con nuove informazioni
    void updateDeviceInfo(const DeviceInfo& deviceInfo);

private:
    // Setup UI
    void setupUi();
    QWidget* createGeneralTab();
    QWidget* createWiFiTab();
    QWidget* createSystemTab();
    QWidget* createEnergyTab();
    QWidget* createDiagnosticsTab();

    // Helper per creare label con icone/stili
    QLabel* createInfoLabel(const QString& text, bool isBold = false);
    QLabel* createValueLabel(const QString& text);
    QGroupBox* createInfoGroup(const QString& title);

    // Sub-tabs
    QTabWidget* m_subTabWidget;

    // === GENERAL TAB ===
    QLabel* m_modelLabel;
    QLabel* m_firmwareLabel;
    QLabel* m_ipLabel;
    QLabel* m_macLabel;
    QLabel* m_uptimeLabel;
    QLabel* m_lastUpdateLabel;

    // === WIFI TAB ===
    QLabel* m_wifiSsidLabel;
    QLabel* m_wifiRssiLabel;
    QLabel* m_wifiQualityLabel;
    QLabel* m_wifiBarsLabel;
    QLabel* m_wifiStatusLabel;
    QLabel* m_cloudStatusLabel;

    // === SYSTEM TAB ===
    QLabel* m_ramTotalLabel;
    QLabel* m_ramFreeLabel;
    QLabel* m_ramUsageLabel;
    QLabel* m_fsTotalLabel;
    QLabel* m_fsFreeLabel;
    QLabel* m_fsUsageLabel;
    QLabel* m_temperatureLabel;
    QLabel* m_overTempLabel;

    // === ENERGY TAB ===
    QLabel* m_energyPhaseALabel;
    QLabel* m_energyPhaseBLabel;
    QLabel* m_energyPhaseCLabel;
    QLabel* m_energyTotalLabel;
    QLabel* m_energyReturnedLabel;

    // === DIAGNOSTICS TAB ===
    QLabel* m_neutralCurrentLabel;
    QLabel* m_neutralMismatchLabel;
    QLabel* m_phaseImbalanceLabel;
    QLabel* m_updateAvailableLabel;

    // Pulsante refresh manuale
    QPushButton* m_refreshButton;

    // Ultimo DeviceInfo ricevuto
    DeviceInfo m_currentDeviceInfo;

signals:
    // Emesso quando l'utente richiede un refresh manuale
    void refreshRequested();

private slots:
    void onRefreshClicked();
};

#endif // DEVICEINFOTAB_H

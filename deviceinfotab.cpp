#include "deviceinfotab.h"
#include <QDateTime>

DeviceInfoTab::DeviceInfoTab(QWidget *parent)
    : QWidget(parent)
    , m_subTabWidget(nullptr)
    , m_modelLabel(nullptr)
    , m_firmwareLabel(nullptr)
    , m_ipLabel(nullptr)
    , m_macLabel(nullptr)
    , m_uptimeLabel(nullptr)
    , m_lastUpdateLabel(nullptr)
    , m_wifiSsidLabel(nullptr)
    , m_wifiRssiLabel(nullptr)
    , m_wifiQualityLabel(nullptr)
    , m_wifiBarsLabel(nullptr)
    , m_wifiStatusLabel(nullptr)
    , m_cloudStatusLabel(nullptr)
    , m_ramTotalLabel(nullptr)
    , m_ramFreeLabel(nullptr)
    , m_ramUsageLabel(nullptr)
    , m_fsTotalLabel(nullptr)
    , m_fsFreeLabel(nullptr)
    , m_fsUsageLabel(nullptr)
    , m_temperatureLabel(nullptr)
    , m_overTempLabel(nullptr)
    , m_energyPhaseALabel(nullptr)
    , m_energyPhaseBLabel(nullptr)
    , m_energyPhaseCLabel(nullptr)
    , m_energyTotalLabel(nullptr)
    , m_energyReturnedLabel(nullptr)
    , m_neutralCurrentLabel(nullptr)
    , m_neutralMismatchLabel(nullptr)
    , m_phaseImbalanceLabel(nullptr)
    , m_updateAvailableLabel(nullptr)
    , m_refreshButton(nullptr)
{
    setupUi();
}

DeviceInfoTab::~DeviceInfoTab()
{
}

void DeviceInfoTab::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Pulsante refresh in alto
    m_refreshButton = new QPushButton(tr("Refresh Device Info"), this);
    connect(m_refreshButton, &QPushButton::clicked, this, &DeviceInfoTab::onRefreshClicked);
    mainLayout->addWidget(m_refreshButton);

    // Crea il tab widget per i sotto-tab
    m_subTabWidget = new QTabWidget(this);
    m_subTabWidget->addTab(createGeneralTab(), tr("General"));
    m_subTabWidget->addTab(createWiFiTab(), tr("WiFi"));
    m_subTabWidget->addTab(createSystemTab(), tr("System"));
    m_subTabWidget->addTab(createEnergyTab(), tr("Energy"));
    m_subTabWidget->addTab(createDiagnosticsTab(), tr("Diagnostics"));

    mainLayout->addWidget(m_subTabWidget);

    setLayout(mainLayout);
}

QWidget* DeviceInfoTab::createGeneralTab()
{
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);

    // Group: Device Information
    QGroupBox* deviceGroup = createInfoGroup(tr("Device Information"));
    QGridLayout* deviceGrid = new QGridLayout(deviceGroup);

    deviceGrid->addWidget(createInfoLabel(tr("Model:"), true), 0, 0);
    m_modelLabel = createValueLabel("-");
    deviceGrid->addWidget(m_modelLabel, 0, 1);

    deviceGrid->addWidget(createInfoLabel(tr("Firmware:"), true), 1, 0);
    m_firmwareLabel = createValueLabel("-");
    deviceGrid->addWidget(m_firmwareLabel, 1, 1);

    deviceGrid->addWidget(createInfoLabel(tr("IP Address:"), true), 2, 0);
    m_ipLabel = createValueLabel("-");
    deviceGrid->addWidget(m_ipLabel, 2, 1);

    deviceGrid->addWidget(createInfoLabel(tr("MAC Address:"), true), 3, 0);
    m_macLabel = createValueLabel("-");
    deviceGrid->addWidget(m_macLabel, 3, 1);

    deviceGrid->addWidget(createInfoLabel(tr("Uptime:"), true), 4, 0);
    m_uptimeLabel = createValueLabel("-");
    deviceGrid->addWidget(m_uptimeLabel, 4, 1);

    deviceGrid->addWidget(createInfoLabel(tr("Last Update:"), true), 5, 0);
    m_lastUpdateLabel = createValueLabel("-");
    deviceGrid->addWidget(m_lastUpdateLabel, 5, 1);

    deviceGrid->setColumnStretch(1, 1);

    layout->addWidget(deviceGroup);
    layout->addStretch();

    return widget;
}

QWidget* DeviceInfoTab::createWiFiTab()
{
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);

    // Group: WiFi Connection
    QGroupBox* wifiGroup = createInfoGroup(tr("WiFi Connection"));
    QGridLayout* wifiGrid = new QGridLayout(wifiGroup);

    wifiGrid->addWidget(createInfoLabel(tr("Status:"), true), 0, 0);
    m_wifiStatusLabel = createValueLabel("-");
    wifiGrid->addWidget(m_wifiStatusLabel, 0, 1);

    wifiGrid->addWidget(createInfoLabel(tr("SSID:"), true), 1, 0);
    m_wifiSsidLabel = createValueLabel("-");
    wifiGrid->addWidget(m_wifiSsidLabel, 1, 1);

    wifiGrid->addWidget(createInfoLabel(tr("Signal Strength (RSSI):"), true), 2, 0);
    m_wifiRssiLabel = createValueLabel("-");
    wifiGrid->addWidget(m_wifiRssiLabel, 2, 1);

    wifiGrid->addWidget(createInfoLabel(tr("Signal Quality:"), true), 3, 0);
    m_wifiQualityLabel = createValueLabel("-");
    wifiGrid->addWidget(m_wifiQualityLabel, 3, 1);

    wifiGrid->addWidget(createInfoLabel(tr("Signal Bars:"), true), 4, 0);
    m_wifiBarsLabel = createValueLabel("-");
    wifiGrid->addWidget(m_wifiBarsLabel, 4, 1);

    wifiGrid->setColumnStretch(1, 1);

    layout->addWidget(wifiGroup);

    // Group: Cloud Connection
    QGroupBox* cloudGroup = createInfoGroup(tr("Cloud Connection"));
    QGridLayout* cloudGrid = new QGridLayout(cloudGroup);

    cloudGrid->addWidget(createInfoLabel(tr("Cloud Status:"), true), 0, 0);
    m_cloudStatusLabel = createValueLabel("-");
    cloudGrid->addWidget(m_cloudStatusLabel, 0, 1);

    cloudGrid->setColumnStretch(1, 1);

    layout->addWidget(cloudGroup);
    layout->addStretch();

    return widget;
}

QWidget* DeviceInfoTab::createSystemTab()
{
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);

    // Group: Memory
    QGroupBox* ramGroup = createInfoGroup(tr("Memory (RAM)"));
    QGridLayout* ramGrid = new QGridLayout(ramGroup);

    ramGrid->addWidget(createInfoLabel(tr("Total:"), true), 0, 0);
    m_ramTotalLabel = createValueLabel("-");
    ramGrid->addWidget(m_ramTotalLabel, 0, 1);

    ramGrid->addWidget(createInfoLabel(tr("Free:"), true), 1, 0);
    m_ramFreeLabel = createValueLabel("-");
    ramGrid->addWidget(m_ramFreeLabel, 1, 1);

    ramGrid->addWidget(createInfoLabel(tr("Usage:"), true), 2, 0);
    m_ramUsageLabel = createValueLabel("-");
    ramGrid->addWidget(m_ramUsageLabel, 2, 1);

    ramGrid->setColumnStretch(1, 1);

    layout->addWidget(ramGroup);

    // Group: Storage
    QGroupBox* fsGroup = createInfoGroup(tr("Storage (Filesystem)"));
    QGridLayout* fsGrid = new QGridLayout(fsGroup);

    fsGrid->addWidget(createInfoLabel(tr("Total:"), true), 0, 0);
    m_fsTotalLabel = createValueLabel("-");
    fsGrid->addWidget(m_fsTotalLabel, 0, 1);

    fsGrid->addWidget(createInfoLabel(tr("Free:"), true), 1, 0);
    m_fsFreeLabel = createValueLabel("-");
    fsGrid->addWidget(m_fsFreeLabel, 1, 1);

    fsGrid->addWidget(createInfoLabel(tr("Usage:"), true), 2, 0);
    m_fsUsageLabel = createValueLabel("-");
    fsGrid->addWidget(m_fsUsageLabel, 2, 1);

    fsGrid->setColumnStretch(1, 1);

    layout->addWidget(fsGroup);

    // Group: Temperature
    QGroupBox* tempGroup = createInfoGroup(tr("Temperature"));
    QGridLayout* tempGrid = new QGridLayout(tempGroup);

    tempGrid->addWidget(createInfoLabel(tr("Device Temperature:"), true), 0, 0);
    m_temperatureLabel = createValueLabel("-");
    tempGrid->addWidget(m_temperatureLabel, 0, 1);

    tempGrid->addWidget(createInfoLabel(tr("Overtemperature:"), true), 1, 0);
    m_overTempLabel = createValueLabel("-");
    tempGrid->addWidget(m_overTempLabel, 1, 1);

    tempGrid->setColumnStretch(1, 1);

    layout->addWidget(tempGroup);
    layout->addStretch();

    return widget;
}

QWidget* DeviceInfoTab::createEnergyTab()
{
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);

    // Group: Energy Totals
    QGroupBox* energyGroup = createInfoGroup(tr("Total Energy Consumed"));
    QGridLayout* energyGrid = new QGridLayout(energyGroup);

    energyGrid->addWidget(createInfoLabel(tr("Phase A:"), true), 0, 0);
    m_energyPhaseALabel = createValueLabel("-");
    energyGrid->addWidget(m_energyPhaseALabel, 0, 1);

    energyGrid->addWidget(createInfoLabel(tr("Phase B:"), true), 1, 0);
    m_energyPhaseBLabel = createValueLabel("-");
    energyGrid->addWidget(m_energyPhaseBLabel, 1, 1);

    energyGrid->addWidget(createInfoLabel(tr("Phase C:"), true), 2, 0);
    m_energyPhaseCLabel = createValueLabel("-");
    energyGrid->addWidget(m_energyPhaseCLabel, 2, 1);

    energyGrid->addWidget(createInfoLabel(tr("Total (All Phases):"), true), 3, 0);
    m_energyTotalLabel = createValueLabel("-");
    energyGrid->addWidget(m_energyTotalLabel, 3, 1);

    energyGrid->addWidget(createInfoLabel(tr("Energy Returned:"), true), 4, 0);
    m_energyReturnedLabel = createValueLabel("-");
    energyGrid->addWidget(m_energyReturnedLabel, 4, 1);

    energyGrid->setColumnStretch(1, 1);

    layout->addWidget(energyGroup);
    layout->addStretch();

    return widget;
}

QWidget* DeviceInfoTab::createDiagnosticsTab()
{
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);

    // Group: Phase Balance
    QGroupBox* phaseGroup = createInfoGroup(tr("Phase Balance"));
    QGridLayout* phaseGrid = new QGridLayout(phaseGroup);

    phaseGrid->addWidget(createInfoLabel(tr("Neutral Current:"), true), 0, 0);
    m_neutralCurrentLabel = createValueLabel("-");
    phaseGrid->addWidget(m_neutralCurrentLabel, 0, 1);

    phaseGrid->addWidget(createInfoLabel(tr("Neutral Mismatch:"), true), 1, 0);
    m_neutralMismatchLabel = createValueLabel("-");
    phaseGrid->addWidget(m_neutralMismatchLabel, 1, 1);

    phaseGrid->addWidget(createInfoLabel(tr("Phase Imbalance:"), true), 2, 0);
    m_phaseImbalanceLabel = createValueLabel("-");
    phaseGrid->addWidget(m_phaseImbalanceLabel, 2, 1);

    phaseGrid->setColumnStretch(1, 1);

    layout->addWidget(phaseGroup);

    // Group: Updates
    QGroupBox* updateGroup = createInfoGroup(tr("Firmware Updates"));
    QGridLayout* updateGrid = new QGridLayout(updateGroup);

    updateGrid->addWidget(createInfoLabel(tr("Update Available:"), true), 0, 0);
    m_updateAvailableLabel = createValueLabel("-");
    updateGrid->addWidget(m_updateAvailableLabel, 0, 1);

    updateGrid->setColumnStretch(1, 1);

    layout->addWidget(updateGroup);
    layout->addStretch();

    return widget;
}

QLabel* DeviceInfoTab::createInfoLabel(const QString& text, bool isBold)
{
    QLabel* label = new QLabel(text, this);
    if (isBold) {
        QFont font = label->font();
        font.setBold(true);
        label->setFont(font);
    }
    return label;
}

QLabel* DeviceInfoTab::createValueLabel(const QString& text)
{
    QLabel* label = new QLabel(text, this);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

QGroupBox* DeviceInfoTab::createInfoGroup(const QString& title)
{
    QGroupBox* group = new QGroupBox(title, this);
    return group;
}

void DeviceInfoTab::updateDeviceInfo(const DeviceInfo& deviceInfo)
{
    m_currentDeviceInfo = deviceInfo;

    // === GENERAL TAB ===
    m_modelLabel->setText(deviceInfo.model.isEmpty() ? "-" : deviceInfo.model);
    m_firmwareLabel->setText(deviceInfo.firmware_version.isEmpty() ? "-" : deviceInfo.firmware_version);
    m_ipLabel->setText(deviceInfo.ip_address.isEmpty() ? "-" : deviceInfo.ip_address);
    m_macLabel->setText(deviceInfo.mac_address.isEmpty() ? "-" : deviceInfo.mac_address);
    m_uptimeLabel->setText(deviceInfo.getFormattedUptime());
    m_lastUpdateLabel->setText(deviceInfo.last_update.toString("yyyy-MM-dd HH:mm:ss"));

    // === WIFI TAB ===
    m_wifiStatusLabel->setText(deviceInfo.wifi_connected ? tr("Connected") : tr("Disconnected"));
    m_wifiStatusLabel->setStyleSheet(deviceInfo.wifi_connected ? "color: green;" : "color: red;");

    m_wifiSsidLabel->setText(deviceInfo.wifi_ssid.isEmpty() ? "-" : deviceInfo.wifi_ssid);
    m_wifiRssiLabel->setText(QString("%1 dBm").arg(deviceInfo.wifi_rssi));
    m_wifiQualityLabel->setText(deviceInfo.getWiFiQuality());

    // Signal bars visualization
    int bars = deviceInfo.getWiFiBars();
    QString barsText = QString("▂▄▆█").left(bars) + QString("░").repeated(5 - bars);
    m_wifiBarsLabel->setText(QString("%1 (%2/5)").arg(barsText).arg(bars));

    // Cloud status
    QString cloudStatus;
    if (deviceInfo.cloud_enabled) {
        cloudStatus = deviceInfo.cloud_connected ? tr("Connected") : tr("Enabled but not connected");
    } else {
        cloudStatus = tr("Disabled");
    }
    m_cloudStatusLabel->setText(cloudStatus);

    // === SYSTEM TAB ===
    m_ramTotalLabel->setText(QString("%1 KB").arg(deviceInfo.ram_total));
    m_ramFreeLabel->setText(QString("%1 KB").arg(deviceInfo.ram_free));
    m_ramUsageLabel->setText(QString("%1%").arg(deviceInfo.getRamUsagePercent(), 0, 'f', 1));

    m_fsTotalLabel->setText(QString("%1 KB").arg(deviceInfo.fs_size));
    m_fsFreeLabel->setText(QString("%1 KB").arg(deviceInfo.fs_free));
    m_fsUsageLabel->setText(QString("%1%").arg(deviceInfo.getStorageUsagePercent(), 0, 'f', 1));

    m_temperatureLabel->setText(QString("%1 °C").arg(deviceInfo.temperature, 0, 'f', 1));
    m_overTempLabel->setText(deviceInfo.overtemperature ? tr("YES - WARNING!") : tr("No"));
    m_overTempLabel->setStyleSheet(deviceInfo.overtemperature ? "color: red; font-weight: bold;" : "color: green;");

    // === ENERGY TAB ===
    m_energyPhaseALabel->setText(deviceInfo.total_valid[0] ?
        QString("%1 kWh").arg(deviceInfo.total_energy_wh[0] / 1000.0, 0, 'f', 3) : tr("Invalid"));
    m_energyPhaseBLabel->setText(deviceInfo.total_valid[1] ?
        QString("%1 kWh").arg(deviceInfo.total_energy_wh[1] / 1000.0, 0, 'f', 3) : tr("Invalid"));
    m_energyPhaseCLabel->setText(deviceInfo.total_valid[2] ?
        QString("%1 kWh").arg(deviceInfo.total_energy_wh[2] / 1000.0, 0, 'f', 3) : tr("Invalid"));

    double totalEnergy = deviceInfo.total_energy_wh[0] + deviceInfo.total_energy_wh[1] + deviceInfo.total_energy_wh[2];
    m_energyTotalLabel->setText(QString("%1 kWh").arg(totalEnergy / 1000.0, 0, 'f', 3));
    m_energyReturnedLabel->setText(QString("%1 kWh").arg(deviceInfo.total_returned_wh / 1000.0, 0, 'f', 3));

    // === DIAGNOSTICS TAB ===
    m_neutralCurrentLabel->setText(QString("%1 A").arg(deviceInfo.neutral_current, 0, 'f', 2));
    m_neutralMismatchLabel->setText(deviceInfo.neutral_mismatch ? tr("YES - Check wiring") : tr("No"));
    m_neutralMismatchLabel->setStyleSheet(deviceInfo.neutral_mismatch ? "color: orange; font-weight: bold;" : "color: green;");

    double imbalance = deviceInfo.getPhaseImbalance();
    m_phaseImbalanceLabel->setText(QString("%1%").arg(imbalance, 0, 'f', 1));
    if (imbalance > 15.0) {
        m_phaseImbalanceLabel->setStyleSheet("color: orange; font-weight: bold;");
    } else {
        m_phaseImbalanceLabel->setStyleSheet("color: green;");
    }

    // Update info
    if (deviceInfo.update_available) {
        QString updateText = tr("Yes - %1 → %2").arg(deviceInfo.old_version, deviceInfo.new_version);
        m_updateAvailableLabel->setText(updateText);
        m_updateAvailableLabel->setStyleSheet("color: blue; font-weight: bold;");
    } else {
        m_updateAvailableLabel->setText(tr("No - Up to date"));
        m_updateAvailableLabel->setStyleSheet("color: green;");
    }
}

void DeviceInfoTab::onRefreshClicked()
{
    emit refreshRequested();
}

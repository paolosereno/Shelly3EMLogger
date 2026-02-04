#include "shellymanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>

ShellyManager::ShellyManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_pollTimer(new QTimer(this))
    , m_pollInterval(60)
    , m_isConnected(false)
    , m_consecutiveErrors(0)
    , m_isDeviceInfoRequest(false)
{
    // Configurazione timer per polling periodico
    m_pollTimer->setSingleShot(false);
    connect(m_pollTimer, &QTimer::timeout, this, &ShellyManager::performRequest);

    // Configurazione network manager
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &ShellyManager::handleNetworkReply);
}

ShellyManager::~ShellyManager()
{
    disconnect();
}

void ShellyManager::connectToShelly(const QString& ipAddress, int intervalSeconds)
{
    if (m_isConnected) {
        disconnect();
    }

    m_ipAddress = ipAddress;
    m_pollInterval = intervalSeconds;
    m_consecutiveErrors = 0;

    // Avvia il timer per il polling
    m_pollTimer->setInterval(m_pollInterval * 1000);
    m_pollTimer->start();

    // Effettua subito la prima richiesta
    performRequest();

    m_isConnected = true;
    emit connectionStatusChanged(true);

    qDebug() << "ShellyManager: Connesso a" << m_ipAddress
             << "con intervallo" << m_pollInterval << "secondi";
}

void ShellyManager::disconnect()
{
    if (!m_isConnected) {
        return;
    }

    m_pollTimer->stop();
    m_isConnected = false;
    m_consecutiveErrors = 0;

    emit connectionStatusChanged(false);

    qDebug() << "ShellyManager: Disconnesso da" << m_ipAddress;
}

void ShellyManager::setPollInterval(int seconds)
{
    m_pollInterval = seconds;

    if (m_isConnected) {
        m_pollTimer->setInterval(m_pollInterval * 1000);
    }

    qDebug() << "ShellyManager: Intervallo polling impostato a" << seconds << "secondi";
}

void ShellyManager::performRequest()
{
    if (!m_isConnected) {
        return;
    }

    // Costruisci l'URL per la richiesta
    QString urlString = QString("http://%1/status").arg(m_ipAddress);
    QUrl url(urlString);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Timeout di 10 secondi
    request.setTransferTimeout(10000);

    qDebug() << "ShellyManager: Richiesta HTTP GET a" << urlString;

    m_networkManager->get(request);
}

void ShellyManager::handleNetworkReply(QNetworkReply* reply)
{
    // Auto-delete del reply quando esce dallo scope
    reply->deleteLater();

    if (!m_isConnected) {
        return;
    }

    // Verifica errori di rete
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = tr("Network error: %1").arg(reply->errorString());
        qWarning() << "ShellyManager:" << errorMsg;

        m_consecutiveErrors++;

        // Disconnetti automaticamente dopo troppi errori consecutivi
        if (m_consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
            errorMsg += tr("\nToo many consecutive errors (%1). Automatic disconnection.")
                        .arg(MAX_CONSECUTIVE_ERRORS);
            disconnect();
        }

        emit errorOccurred(errorMsg);
        return;
    }

    // Leggi la risposta
    QByteArray responseData = reply->readAll();

    // Controlla se questa è una richiesta di device info
    if (m_isDeviceInfoRequest) {
        m_isDeviceInfoRequest = false;  // Reset del flag

        DeviceInfo deviceInfo;
        if (parseDeviceInfo(responseData, deviceInfo)) {
            emit deviceInfoReceived(deviceInfo);
            qDebug() << "ShellyManager: Device info received and emitted";
        } else {
            QString errorMsg = tr("Error parsing device info JSON");
            qWarning() << "ShellyManager:" << errorMsg;
            emit errorOccurred(errorMsg);
        }
        return;  // Non fare parsing dei DataPoint per device info request
    }

    // Parsing JSON per DataPoint (polling normale)
    DataPoint dataPoint;
    if (parseJsonResponse(responseData, dataPoint)) {
        // Reset contatore errori in caso di successo
        m_consecutiveErrors = 0;

        // Emetti il segnale con i nuovi dati
        emit dataReceived(dataPoint);

        qDebug() << "ShellyManager: Data received - Total Power:" << dataPoint.getTotalPower() << "W";
    } else {
        QString errorMsg = tr("Error parsing JSON response");
        qWarning() << "ShellyManager:" << errorMsg;

        m_consecutiveErrors++;

        if (m_consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
            errorMsg += tr("\nToo many consecutive errors (%1). Automatic disconnection.")
                        .arg(MAX_CONSECUTIVE_ERRORS);
            disconnect();
        }

        emit errorOccurred(errorMsg);
    }
}

bool ShellyManager::parseJsonResponse(const QByteArray& jsonData, DataPoint& outDataPoint)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "ShellyManager: Errore parsing JSON:" << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "ShellyManager: JSON non è un oggetto";
        return false;
    }

    QJsonObject root = doc.object();

    // Verifica presenza array "emeters"
    if (!root.contains("emeters") || !root["emeters"].isArray()) {
        qWarning() << "ShellyManager: Campo 'emeters' non trovato o non è un array";
        return false;
    }

    QJsonArray emeters = root["emeters"].toArray();

    // Verifica che ci siano almeno 3 fasi
    if (emeters.size() < 3) {
        qWarning() << "ShellyManager: Array 'emeters' contiene meno di 3 fasi:" << emeters.size();
        return false;
    }

    // Estrai i dati per tutte e 3 le fasi
    QJsonObject phaseA = emeters[0].toObject();
    QJsonObject phaseB = emeters[1].toObject();
    QJsonObject phaseC = emeters[2].toObject();

    // Verifica validità dei dati di ogni fase
    bool phaseAValid = phaseA.contains("is_valid") && phaseA["is_valid"].toBool();
    bool phaseBValid = phaseB.contains("is_valid") && phaseB["is_valid"].toBool();
    bool phaseCValid = phaseC.contains("is_valid") && phaseC["is_valid"].toBool();

    // Almeno una fase deve essere valida
    if (!phaseAValid && !phaseBValid && !phaseCValid) {
        qWarning() << "ShellyManager: Nessuna fase ha dati validi";
        return false;
    }

    // Timestamp comune per tutte le fasi
    outDataPoint.timestamp = QDateTime::currentDateTime();

    // Estrai i valori della Fase A
    if (phaseAValid) {
        outDataPoint.powerA = phaseA["power"].toDouble();
        outDataPoint.voltageA = phaseA["voltage"].toDouble();
        outDataPoint.currentA = phaseA["current"].toDouble();
        outDataPoint.powerFactorA = phaseA["pf"].toDouble();
    } else {
        qWarning() << "ShellyManager: Fase A non valida, impostazione valori a 0";
        outDataPoint.powerA = 0.0;
        outDataPoint.voltageA = 0.0;
        outDataPoint.currentA = 0.0;
        outDataPoint.powerFactorA = 0.0;
    }

    // Estrai i valori della Fase B
    if (phaseBValid) {
        outDataPoint.powerB = phaseB["power"].toDouble();
        outDataPoint.voltageB = phaseB["voltage"].toDouble();
        outDataPoint.currentB = phaseB["current"].toDouble();
        outDataPoint.powerFactorB = phaseB["pf"].toDouble();
    } else {
        qWarning() << "ShellyManager: Fase B non valida, impostazione valori a 0";
        outDataPoint.powerB = 0.0;
        outDataPoint.voltageB = 0.0;
        outDataPoint.currentB = 0.0;
        outDataPoint.powerFactorB = 0.0;
    }

    // Estrai i valori della Fase C
    if (phaseCValid) {
        outDataPoint.powerC = phaseC["power"].toDouble();
        outDataPoint.voltageC = phaseC["voltage"].toDouble();
        outDataPoint.currentC = phaseC["current"].toDouble();
        outDataPoint.powerFactorC = phaseC["pf"].toDouble();
    } else {
        qWarning() << "ShellyManager: Fase C non valida, impostazione valori a 0";
        outDataPoint.powerC = 0.0;
        outDataPoint.voltageC = 0.0;
        outDataPoint.currentC = 0.0;
        outDataPoint.powerFactorC = 0.0;
    }

    // Verifica che il DataPoint sia valido (almeno una fase valida)
    if (!outDataPoint.isValid()) {
        qWarning() << "ShellyManager: DataPoint risultante non valido";
        return false;
    }

    qDebug() << "ShellyManager: Parsed 3 phases - A:" << outDataPoint.powerA << "W, B:"
             << outDataPoint.powerB << "W, C:" << outDataPoint.powerC << "W";

    return true;
}

void ShellyManager::requestDeviceInfo()
{
    if (m_ipAddress.isEmpty()) {
        qWarning() << "ShellyManager: Cannot request device info - no IP address configured";
        return;
    }

    // Costruisci l'URL per la richiesta
    QString urlString = QString("http://%1/status").arg(m_ipAddress);
    QUrl url(urlString);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(10000);

    // Imposta il flag per distinguere questa richiesta
    m_isDeviceInfoRequest = true;

    qDebug() << "ShellyManager: Requesting device info from" << urlString;

    m_networkManager->get(request);
}

bool ShellyManager::parseDeviceInfo(const QByteArray& jsonData, DeviceInfo& outDeviceInfo)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "ShellyManager: Error parsing device info JSON:" << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "ShellyManager: Device info JSON is not an object";
        return false;
    }

    QJsonObject root = doc.object();

    // Timestamp dell'aggiornamento
    outDeviceInfo.last_update = QDateTime::currentDateTime();

    // === WiFi Info ===
    if (root.contains("wifi_sta")) {
        QJsonObject wifiSta = root["wifi_sta"].toObject();
        outDeviceInfo.wifi_connected = wifiSta["connected"].toBool();
        outDeviceInfo.wifi_ssid = wifiSta["ssid"].toString();
        outDeviceInfo.wifi_rssi = wifiSta["rssi"].toInt();
        outDeviceInfo.ip_address = wifiSta["ip"].toString();
    }

    // === Cloud Info ===
    if (root.contains("cloud")) {
        QJsonObject cloud = root["cloud"].toObject();
        outDeviceInfo.cloud_enabled = cloud["enabled"].toBool();
        outDeviceInfo.cloud_connected = cloud["connected"].toBool();
    }

    // === System Info ===
    if (root.contains("ram_total")) {
        outDeviceInfo.ram_total = root["ram_total"].toInt();
    }
    if (root.contains("ram_free")) {
        outDeviceInfo.ram_free = root["ram_free"].toInt();
    }
    if (root.contains("fs_size")) {
        outDeviceInfo.fs_size = root["fs_size"].toInt();
    }
    if (root.contains("fs_free")) {
        outDeviceInfo.fs_free = root["fs_free"].toInt();
    }
    if (root.contains("uptime")) {
        outDeviceInfo.uptime_seconds = root["uptime"].toInt();
    }
    if (root.contains("temperature")) {
        outDeviceInfo.temperature = root["temperature"].toDouble();
    }
    if (root.contains("overtemperature")) {
        outDeviceInfo.overtemperature = root["overtemperature"].toBool();
    }
    if (root.contains("mac")) {
        outDeviceInfo.mac_address = root["mac"].toString();
    }

    // === Update Info ===
    if (root.contains("update")) {
        QJsonObject update = root["update"].toObject();
        outDeviceInfo.update_available = update.contains("has_update") && update["has_update"].toBool();
        if (outDeviceInfo.update_available) {
            outDeviceInfo.new_version = update["new_version"].toString();
            outDeviceInfo.old_version = update["old_version"].toString();
        }
    }

    // === Energy Totals ===
    if (root.contains("emeters") && root["emeters"].isArray()) {
        QJsonArray emeters = root["emeters"].toArray();

        for (int i = 0; i < qMin(3, emeters.size()); ++i) {
            QJsonObject emeter = emeters[i].toObject();
            outDeviceInfo.total_valid[i] = emeter["is_valid"].toBool();

            if (outDeviceInfo.total_valid[i]) {
                outDeviceInfo.total_energy_wh[i] = emeter["total"].toDouble();
            } else {
                outDeviceInfo.total_energy_wh[i] = 0.0;
            }
        }

        // Total returned (se disponibile)
        if (emeters.size() > 0) {
            QJsonObject emeter0 = emeters[0].toObject();
            if (emeter0.contains("total_returned")) {
                outDeviceInfo.total_returned_wh = emeter0["total_returned"].toDouble();
            }
        }
    }

    // === Neutral Current (diagnostics) ===
    // Nota: il neutral current potrebbe non essere disponibile in tutte le versioni firmware
    // Calcolo approssimativo: somma vettoriale delle correnti delle 3 fasi
    if (root.contains("emeters") && root["emeters"].isArray()) {
        QJsonArray emeters = root["emeters"].toArray();
        if (emeters.size() >= 3) {
            double currentA = emeters[0].toObject()["current"].toDouble();
            double currentB = emeters[1].toObject()["current"].toDouble();
            double currentC = emeters[2].toObject()["current"].toDouble();

            // Approssimazione semplificata: differenza tra max e min
            double maxCurrent = qMax(qMax(currentA, currentB), currentC);
            double minCurrent = qMin(qMin(currentA, currentB), currentC);
            outDeviceInfo.neutral_current = qAbs(maxCurrent - minCurrent);

            // Flag mismatch se sbilanciamento > 15%
            double avgCurrent = (currentA + currentB + currentC) / 3.0;
            if (avgCurrent > 0.5) {  // Solo se c'è carico significativo
                double imbalance = (outDeviceInfo.neutral_current / avgCurrent) * 100.0;
                outDeviceInfo.neutral_mismatch = (imbalance > 15.0);
            }
        }
    }

    // === Device Model (fisso per Shelly EM3) ===
    outDeviceInfo.model = "SHEM-3";

    // === Firmware Version ===
    // Cerca in vari posti dove potrebbe essere
    if (root.contains("update")) {
        QJsonObject update = root["update"].toObject();
        if (update.contains("old_version")) {
            outDeviceInfo.firmware_version = update["old_version"].toString();
        }
    }
    // Fallback: usa "unknown" se non trovato
    if (outDeviceInfo.firmware_version.isEmpty()) {
        outDeviceInfo.firmware_version = "Unknown";
    }

    qDebug() << "ShellyManager: Device info parsed - Model:" << outDeviceInfo.model
             << "FW:" << outDeviceInfo.firmware_version
             << "IP:" << outDeviceInfo.ip_address
             << "Uptime:" << outDeviceInfo.getFormattedUptime();

    return true;
}

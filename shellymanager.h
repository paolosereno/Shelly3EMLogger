#ifndef SHELLYMANAGER_H
#define SHELLYMANAGER_H

#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "datapoint.h"
#include "deviceinfo.h"

/**
 * @brief Classe per gestire la comunicazione con lo Shelly EM3 via HTTP
 *
 * Responsabile di:
 * - Effettuare richieste HTTP GET all'endpoint /status
 * - Parsare la risposta JSON
 * - Estrarre i dati della Fase A
 * - Emettere segnali quando nuovi dati sono disponibili
 * - Gestire errori di rete e timeout
 */
class ShellyManager : public QObject
{
    Q_OBJECT

public:
    explicit ShellyManager(QObject *parent = nullptr);
    ~ShellyManager();

    // Gestione connessione
    void connectToShelly(const QString& ipAddress, int intervalSeconds = 60);
    void disconnect();
    bool isConnected() const { return m_isConnected; }

    // Getters
    QString currentIpAddress() const { return m_ipAddress; }
    int pollInterval() const { return m_pollInterval; }

    // Setters
    void setPollInterval(int seconds);

    // Richiesta manuale delle informazioni dispositivo
    void requestDeviceInfo();

signals:
    // Emesso quando nuovi dati sono disponibili
    void dataReceived(const DataPoint& dataPoint);

    // Emesso quando info dispositivo sono disponibili
    void deviceInfoReceived(const DeviceInfo& deviceInfo);

    // Emesso in caso di errore
    void errorOccurred(const QString& errorMessage);

    // Emesso quando lo stato di connessione cambia
    void connectionStatusChanged(bool connected);

private slots:
    // Slot per gestire il polling periodico
    void performRequest();

    // Slot per gestire la risposta HTTP
    void handleNetworkReply(QNetworkReply* reply);

private:
    // Parsing della risposta JSON
    bool parseJsonResponse(const QByteArray& jsonData, DataPoint& outDataPoint);
    bool parseDeviceInfo(const QByteArray& jsonData, DeviceInfo& outDeviceInfo);

    // Network
    QNetworkAccessManager* m_networkManager;
    QTimer* m_pollTimer;

    // Configurazione
    QString m_ipAddress;
    int m_pollInterval;  // in secondi
    bool m_isConnected;

    // Contatore errori consecutivi
    int m_consecutiveErrors;
    static const int MAX_CONSECUTIVE_ERRORS = 3;

    // Flag per distinguere richieste device info da polling dati
    bool m_isDeviceInfoRequest;
};

#endif // SHELLYMANAGER_H

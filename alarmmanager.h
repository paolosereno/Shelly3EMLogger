#ifndef ALARMMANAGER_H
#define ALARMMANAGER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include "datapoint.h"
#include "deviceinfo.h"

/**
 * @brief Gestore sistema allarmi per monitoraggio condizioni critiche
 *
 * Gestisce 3 tipi di allarmi:
 * - Temperature Alarm: Temperatura dispositivo eccessiva
 * - Phase Imbalance Alarm: Sbilanciamento eccessivo tra fasi
 * - Power Threshold Alarm: Potenza totale oltre soglia
 */
class AlarmManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Tipi di allarme supportati
     */
    enum AlarmType {
        ALARM_TEMPERATURE,
        ALARM_PHASE_IMBALANCE,
        ALARM_POWER_THRESHOLD
    };

    /**
     * @brief Struttura per rappresentare un allarme attivo
     */
    struct ActiveAlarm {
        AlarmType type;
        QString message;
        QDateTime timestamp;
        double value;           // Valore che ha triggerato l'allarme
        double threshold;       // Soglia configurata
    };

    explicit AlarmManager(QObject *parent = nullptr);
    ~AlarmManager();

    /**
     * @brief Configura soglie allarmi
     */
    void setTemperatureAlarm(bool enabled, double threshold);
    void setPhaseImbalanceAlarm(bool enabled, double threshold);
    void setPowerAlarm(bool enabled, double threshold);

    /**
     * @brief Controlla condizioni allarme su nuovo campione
     * @param dataPoint Dati campione real-time
     * @return true se almeno un allarme è stato triggerato
     */
    bool checkDataPointAlarms(const DataPoint& dataPoint);

    /**
     * @brief Controlla allarme temperatura su device info
     * @param deviceInfo Informazioni dispositivo
     * @return true se allarme temperatura attivo
     */
    bool checkTemperatureAlarm(const DeviceInfo& deviceInfo);

    /**
     * @brief Ottiene lista allarmi attivi
     */
    QVector<ActiveAlarm> getActiveAlarms() const;

    /**
     * @brief Conta allarmi attivi
     */
    int getActiveAlarmCount() const;

    /**
     * @brief Cancella tutti gli allarmi attivi
     */
    void clearActiveAlarms();

    /**
     * @brief Cancella allarme specifico
     */
    void clearAlarm(AlarmType type);

    /**
     * @brief Verifica se un allarme è attivo
     */
    bool isAlarmActive(AlarmType type) const;

signals:
    /**
     * @brief Emesso quando un nuovo allarme viene triggerato
     * @param type Tipo di allarme
     * @param message Messaggio descrittivo
     * @param value Valore che ha causato l'allarme
     * @param threshold Soglia configurata
     */
    void alarmTriggered(AlarmType type, const QString& message, double value, double threshold);

    /**
     * @brief Emesso quando un allarme viene risolto (valore torna sotto soglia)
     */
    void alarmCleared(AlarmType type);

private:
    /**
     * @brief Calcola sbilanciamento percentuale tra fasi
     * @param dataPoint Campione dati
     * @return Sbilanciamento % (0-100)
     */
    double calculatePhaseImbalance(const DataPoint& dataPoint);

    /**
     * @brief Triggera un nuovo allarme
     */
    void triggerAlarm(AlarmType type, const QString& message, double value, double threshold);

    /**
     * @brief Risolve un allarme attivo
     */
    void resolveAlarm(AlarmType type);

    // Configurazione allarmi
    bool m_temperatureAlarmEnabled;
    double m_temperatureThreshold;

    bool m_phaseImbalanceAlarmEnabled;
    double m_phaseImbalanceThreshold;

    bool m_powerAlarmEnabled;
    double m_powerThreshold;

    // Allarmi attivi
    QVector<ActiveAlarm> m_activeAlarms;
};

#endif // ALARMMANAGER_H

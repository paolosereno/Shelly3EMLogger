#include "alarmmanager.h"
#include <QDebug>
#include <QtMath>

AlarmManager::AlarmManager(QObject *parent)
    : QObject(parent)
    , m_temperatureAlarmEnabled(false)
    , m_temperatureThreshold(70.0)
    , m_phaseImbalanceAlarmEnabled(false)
    , m_phaseImbalanceThreshold(20.0)
    , m_powerAlarmEnabled(false)
    , m_powerThreshold(3000.0)
{
}

AlarmManager::~AlarmManager()
{
}

void AlarmManager::setTemperatureAlarm(bool enabled, double threshold)
{
    m_temperatureAlarmEnabled = enabled;
    m_temperatureThreshold = threshold;
    qDebug() << "AlarmManager: Temperature alarm:" << (enabled ? "ENABLED" : "DISABLED")
             << "threshold:" << threshold << "°C";
}

void AlarmManager::setPhaseImbalanceAlarm(bool enabled, double threshold)
{
    m_phaseImbalanceAlarmEnabled = enabled;
    m_phaseImbalanceThreshold = threshold;
    qDebug() << "AlarmManager: Phase imbalance alarm:" << (enabled ? "ENABLED" : "DISABLED")
             << "threshold:" << threshold << "%";
}

void AlarmManager::setPowerAlarm(bool enabled, double threshold)
{
    m_powerAlarmEnabled = enabled;
    m_powerThreshold = threshold;
    qDebug() << "AlarmManager: Power threshold alarm:" << (enabled ? "ENABLED" : "DISABLED")
             << "threshold:" << threshold << "W";
}

bool AlarmManager::checkDataPointAlarms(const DataPoint& dataPoint)
{
    bool anyAlarmTriggered = false;

    // === ALLARME SBILANCIAMENTO FASI ===
    if (m_phaseImbalanceAlarmEnabled && dataPoint.isValid()) {
        double imbalance = calculatePhaseImbalance(dataPoint);

        if (imbalance > m_phaseImbalanceThreshold) {
            if (!isAlarmActive(ALARM_PHASE_IMBALANCE)) {
                QString message = tr("Phase imbalance detected: %1% (threshold: %2%)")
                                  .arg(imbalance, 0, 'f', 1)
                                  .arg(m_phaseImbalanceThreshold, 0, 'f', 1);
                triggerAlarm(ALARM_PHASE_IMBALANCE, message, imbalance, m_phaseImbalanceThreshold);
                anyAlarmTriggered = true;
            }
        } else {
            if (isAlarmActive(ALARM_PHASE_IMBALANCE)) {
                resolveAlarm(ALARM_PHASE_IMBALANCE);
            }
        }
    }

    // === ALLARME SOGLIA POTENZA ===
    if (m_powerAlarmEnabled && dataPoint.isValid()) {
        double totalPower = dataPoint.getTotalPower();

        if (totalPower > m_powerThreshold) {
            if (!isAlarmActive(ALARM_POWER_THRESHOLD)) {
                QString message = tr("Total power exceeds threshold: %1 W (threshold: %2 W)")
                                  .arg(totalPower, 0, 'f', 1)
                                  .arg(m_powerThreshold, 0, 'f', 0);
                triggerAlarm(ALARM_POWER_THRESHOLD, message, totalPower, m_powerThreshold);
                anyAlarmTriggered = true;
            }
        } else {
            if (isAlarmActive(ALARM_POWER_THRESHOLD)) {
                resolveAlarm(ALARM_POWER_THRESHOLD);
            }
        }
    }

    return anyAlarmTriggered;
}

bool AlarmManager::checkTemperatureAlarm(const DeviceInfo& deviceInfo)
{
    if (!m_temperatureAlarmEnabled || !deviceInfo.isValid()) {
        return false;
    }

    double temperature = deviceInfo.temperature;

    if (temperature > m_temperatureThreshold) {
        if (!isAlarmActive(ALARM_TEMPERATURE)) {
            QString message = tr("Device temperature too high: %1°C (threshold: %2°C)")
                              .arg(temperature, 0, 'f', 1)
                              .arg(m_temperatureThreshold, 0, 'f', 1);
            triggerAlarm(ALARM_TEMPERATURE, message, temperature, m_temperatureThreshold);
            return true;
        }
    } else {
        if (isAlarmActive(ALARM_TEMPERATURE)) {
            resolveAlarm(ALARM_TEMPERATURE);
        }
    }

    return false;
}

double AlarmManager::calculatePhaseImbalance(const DataPoint& dataPoint)
{
    // Calcola sbilanciamento come percentuale di deviazione dalla media
    // Formula: max(|Pa - Pavg|, |Pb - Pavg|, |Pc - Pavg|) / Pavg * 100

    if (!dataPoint.isValid()) {
        return 0.0;
    }

    double powerA = dataPoint.powerA;
    double powerB = dataPoint.powerB;
    double powerC = dataPoint.powerC;

    // Media potenze
    double avgPower = (powerA + powerB + powerC) / 3.0;

    if (avgPower < 10.0) {
        // Se potenza media troppo bassa, ignora sbilanciamento (evita false positive)
        return 0.0;
    }

    // Calcola deviazione massima
    double devA = qAbs(powerA - avgPower);
    double devB = qAbs(powerB - avgPower);
    double devC = qAbs(powerC - avgPower);

    double maxDeviation = qMax(qMax(devA, devB), devC);

    // Percentuale di sbilanciamento
    double imbalancePercent = (maxDeviation / avgPower) * 100.0;

    return imbalancePercent;
}

void AlarmManager::triggerAlarm(AlarmType type, const QString& message, double value, double threshold)
{
    ActiveAlarm alarm;
    alarm.type = type;
    alarm.message = message;
    alarm.timestamp = QDateTime::currentDateTime();
    alarm.value = value;
    alarm.threshold = threshold;

    m_activeAlarms.append(alarm);

    qWarning() << "ALARM TRIGGERED:" << message;

    emit alarmTriggered(type, message, value, threshold);
}

void AlarmManager::resolveAlarm(AlarmType type)
{
    for (int i = 0; i < m_activeAlarms.size(); ++i) {
        if (m_activeAlarms[i].type == type) {
            qDebug() << "ALARM CLEARED:" << m_activeAlarms[i].message;
            m_activeAlarms.removeAt(i);
            emit alarmCleared(type);
            return;
        }
    }
}

QVector<AlarmManager::ActiveAlarm> AlarmManager::getActiveAlarms() const
{
    return m_activeAlarms;
}

int AlarmManager::getActiveAlarmCount() const
{
    return m_activeAlarms.size();
}

void AlarmManager::clearActiveAlarms()
{
    for (const ActiveAlarm& alarm : m_activeAlarms) {
        emit alarmCleared(alarm.type);
    }
    m_activeAlarms.clear();
    qDebug() << "AlarmManager: All alarms cleared";
}

void AlarmManager::clearAlarm(AlarmType type)
{
    resolveAlarm(type);
}

bool AlarmManager::isAlarmActive(AlarmType type) const
{
    for (const ActiveAlarm& alarm : m_activeAlarms) {
        if (alarm.type == type) {
            return true;
        }
    }
    return false;
}

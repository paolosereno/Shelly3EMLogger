#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QString>
#include <QSqlDatabase>
#include <QVector>
#include "datapoint.h"
#include "shellycsvimporter.h"
#include "aggregateddata.h"

/**
 * @brief Gestore del database SQLite per persistenza dati
 *
 * Gestisce:
 * - Creazione e inizializzazione database shelly_history.db
 * - Salvataggio automatico campioni in tempo reale (tabella samples)
 * - Salvataggio dati energia giornaliera da CSV import (tabella daily_energy)
 * - Query per recupero dati storici
 * - Gestione transazioni e ottimizzazioni prestazioni
 */
class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    /**
     * @brief Inizializza il database
     * @param dbPath Percorso del file database (default: shelly_history.db)
     * @return true se inizializzazione riuscita
     */
    bool initialize(const QString& dbPath = "shelly_history.db");

    /**
     * @brief Chiude la connessione al database
     */
    void close();

    /**
     * @brief Verifica se il database è aperto e pronto
     */
    bool isOpen() const;

    // ========== GESTIONE CAMPIONI TEMPO REALE ==========

    /**
     * @brief Salva un singolo campione nel database
     * @param dataPoint Campione da salvare
     * @return true se salvataggio riuscito
     */
    bool saveSample(const DataPoint& dataPoint);

    /**
     * @brief Salva multipli campioni in una transazione (più efficiente)
     * @param dataPoints Vettore di campioni da salvare
     * @return true se salvataggio riuscito
     */
    bool saveSamples(const QVector<DataPoint>& dataPoints);

    /**
     * @brief Recupera campioni in un intervallo temporale
     * @param startTime Timestamp inizio (Unix epoch seconds)
     * @param endTime Timestamp fine (Unix epoch seconds)
     * @param outDataPoints Vettore output con i dati recuperati
     * @return true se query riuscita
     */
    bool getSamples(qint64 startTime, qint64 endTime, QVector<DataPoint>& outDataPoints);

    /**
     * @brief Recupera gli ultimi N campioni
     * @param count Numero di campioni da recuperare
     * @param outDataPoints Vettore output con i dati recuperati
     * @return true se query riuscita
     */
    bool getLastSamples(int count, QVector<DataPoint>& outDataPoints);

    /**
     * @brief Conta il numero totale di campioni nel database
     */
    qint64 getSampleCount() const;

    /**
     * @brief Elimina campioni più vecchi di una certa data
     * @param beforeTimestamp Timestamp limite (Unix epoch seconds)
     * @return Numero di record eliminati (-1 se errore)
     */
    int deleteSamplesBefore(qint64 beforeTimestamp);

    // ========== GESTIONE DATI ENERGIA GIORNALIERA ==========

    /**
     * @brief Salva dati energia giornaliera da CSV import
     * @param energyData Vettore di dati giornalieri
     * @param outInserted Numero di nuovi record inseriti (opzionale)
     * @param outUpdated Numero di record aggiornati (opzionale)
     * @return true se salvataggio riuscito
     */
    bool saveDailyEnergy(const QVector<EnergyDailyData>& energyData, int* outInserted = nullptr, int* outUpdated = nullptr);

    /**
     * @brief Recupera dati energia giornaliera in un intervallo
     * @param startDate Data inizio
     * @param endDate Data fine
     * @param outEnergyData Vettore output con i dati recuperati
     * @return true se query riuscita
     */
    bool getDailyEnergy(const QDate& startDate, const QDate& endDate, QVector<EnergyDailyData>& outEnergyData);

    /**
     * @brief Conta il numero di record energia giornaliera
     */
    qint64 getDailyEnergyCount() const;

    // ========== AGGREGAZIONI TEMPORALI ==========

    /**
     * @brief Calcola aggregazioni temporali dai campioni
     * @param interval Tipo di intervallo (Hourly, Daily, Weekly, Monthly)
     * @param startTime Data/ora inizio periodo
     * @param endTime Data/ora fine periodo
     * @param outAggregatedData Vettore output con dati aggregati
     * @return true se aggregazione riuscita
     */
    bool getAggregatedData(AggregationInterval interval,
                          const QDateTime& startTime,
                          const QDateTime& endTime,
                          QVector<AggregatedData>& outAggregatedData);

    /**
     * @brief Calcola statistiche comparative tra due periodi
     * @param interval Tipo di intervallo
     * @param currentStart Inizio periodo corrente
     * @param currentEnd Fine periodo corrente
     * @param previousStart Inizio periodo precedente
     * @param previousEnd Fine periodo precedente
     * @param outComparison Struttura output con comparazione
     * @return true se calcolo riuscito
     */
    bool getComparisonStats(AggregationInterval interval,
                           const QDateTime& currentStart,
                           const QDateTime& currentEnd,
                           const QDateTime& previousStart,
                           const QDateTime& previousEnd,
                           ComparisonStats& outComparison);

    // ========== UTILITÀ ==========

    /**
     * @brief Ottimizza il database (VACUUM)
     * @return true se ottimizzazione riuscita
     */
    bool vacuum();

    /**
     * @brief Ottiene dimensione database in bytes
     */
    qint64 getDatabaseSize() const;

    /**
     * @brief Ottiene percorso file database
     */
    QString getDatabasePath() const;

    /**
     * @brief Crea backup del database
     * @param backupPath Percorso file backup (se vuoto, genera nome automatico)
     * @return true se backup riuscito
     */
    bool backupDatabase(const QString& backupPath = QString());

    /**
     * @brief Ottiene ultimo messaggio di errore
     */
    QString lastError() const;

signals:
    /**
     * @brief Emesso quando si verifica un errore database
     */
    void errorOccurred(const QString& errorMessage);

private:
    /**
     * @brief Crea le tabelle del database se non esistono
     */
    bool createTables();

    /**
     * @brief Verifica e aggiorna schema database se necessario
     */
    bool checkAndUpgradeSchema();

    /**
     * @brief Crea indici per ottimizzare le query
     */
    bool createIndexes();

    QSqlDatabase m_db;
    QString m_dbPath;
    QString m_lastError;

    static const int DB_VERSION = 1;
};

#endif // DATABASEMANAGER_H

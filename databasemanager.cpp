#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QDebug>

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    close();
}

bool DatabaseManager::initialize(const QString& dbPath)
{
    m_dbPath = dbPath;

    // Usa connection name univoco per evitare conflitti
    m_db = QSqlDatabase::addDatabase("QSQLITE", "shelly_connection");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        m_lastError = QString("Cannot open database: %1").arg(m_db.lastError().text());
        qCritical() << "DatabaseManager:" << m_lastError;
        emit errorOccurred(m_lastError);
        return false;
    }

    qDebug() << "DatabaseManager: Database opened:" << dbPath;

    // Abilita foreign keys
    QSqlQuery query(m_db);
    if (!query.exec("PRAGMA foreign_keys = ON")) {
        qWarning() << "DatabaseManager: Failed to enable foreign keys";
    }

    // Crea tabelle se non esistono
    if (!createTables()) {
        m_lastError = "Failed to create database tables";
        qCritical() << "DatabaseManager:" << m_lastError;
        emit errorOccurred(m_lastError);
        return false;
    }

    // Crea indici per performance
    if (!createIndexes()) {
        qWarning() << "DatabaseManager: Failed to create indexes (non-critical)";
    }

    // Verifica e aggiorna schema se necessario
    if (!checkAndUpgradeSchema()) {
        qWarning() << "DatabaseManager: Schema upgrade check failed (non-critical)";
    }

    qDebug() << "DatabaseManager: Initialization completed successfully";
    return true;
}

void DatabaseManager::close()
{
    if (m_db.isOpen()) {
        qDebug() << "DatabaseManager: Closing database";
        m_db.close();
    }
    QSqlDatabase::removeDatabase("shelly_connection");
}

bool DatabaseManager::isOpen() const
{
    return m_db.isOpen();
}

bool DatabaseManager::createTables()
{
    QSqlQuery query(m_db);

    // Tabella per campioni in tempo reale
    QString createSamplesTable = R"(
        CREATE TABLE IF NOT EXISTS samples (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            powerA REAL,
            voltageA REAL,
            currentA REAL,
            pfA REAL,
            powerB REAL,
            voltageB REAL,
            currentB REAL,
            pfB REAL,
            powerC REAL,
            voltageC REAL,
            currentC REAL,
            pfC REAL
        )
    )";

    if (!query.exec(createSamplesTable)) {
        m_lastError = QString("Failed to create samples table: %1").arg(query.lastError().text());
        qCritical() << "DatabaseManager:" << m_lastError;
        return false;
    }

    // Tabella per dati energia giornaliera (da CSV import)
    QString createDailyEnergyTable = R"(
        CREATE TABLE IF NOT EXISTS daily_energy (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            date TEXT NOT NULL UNIQUE,
            energyA_Wh REAL,
            energyB_Wh REAL,
            energyC_Wh REAL,
            totalEnergy_Wh REAL,
            returnedA_Wh REAL,
            returnedB_Wh REAL,
            returnedC_Wh REAL,
            totalReturned_Wh REAL
        )
    )";

    if (!query.exec(createDailyEnergyTable)) {
        m_lastError = QString("Failed to create daily_energy table: %1").arg(query.lastError().text());
        qCritical() << "DatabaseManager:" << m_lastError;
        return false;
    }

    // Tabella per metadati database (versione schema, ecc.)
    QString createMetadataTable = R"(
        CREATE TABLE IF NOT EXISTS metadata (
            key TEXT PRIMARY KEY,
            value TEXT
        )
    )";

    if (!query.exec(createMetadataTable)) {
        m_lastError = QString("Failed to create metadata table: %1").arg(query.lastError().text());
        qCritical() << "DatabaseManager:" << m_lastError;
        return false;
    }

    // Inserisci versione schema se non esiste
    query.prepare("INSERT OR IGNORE INTO metadata (key, value) VALUES ('schema_version', :version)");
    query.bindValue(":version", DB_VERSION);
    if (!query.exec()) {
        qWarning() << "DatabaseManager: Failed to insert schema version";
    }

    qDebug() << "DatabaseManager: Tables created successfully";
    return true;
}

bool DatabaseManager::createIndexes()
{
    QSqlQuery query(m_db);

    // Indice su timestamp per query veloci su intervallo temporale
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_samples_timestamp ON samples(timestamp)")) {
        qWarning() << "DatabaseManager: Failed to create index on samples.timestamp";
        return false;
    }

    // Indice su date per daily_energy
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_daily_energy_date ON daily_energy(date)")) {
        qWarning() << "DatabaseManager: Failed to create index on daily_energy.date";
        return false;
    }

    qDebug() << "DatabaseManager: Indexes created successfully";
    return true;
}

bool DatabaseManager::checkAndUpgradeSchema()
{
    QSqlQuery query(m_db);

    // Leggi versione corrente schema
    query.prepare("SELECT value FROM metadata WHERE key = 'schema_version'");
    if (!query.exec() || !query.next()) {
        qWarning() << "DatabaseManager: Cannot read schema version";
        return false;
    }

    int currentVersion = query.value(0).toInt();
    qDebug() << "DatabaseManager: Current schema version:" << currentVersion;

    // Se necessario, qui si possono aggiungere upgrade per versioni future
    if (currentVersion < DB_VERSION) {
        qDebug() << "DatabaseManager: Schema upgrade needed from" << currentVersion << "to" << DB_VERSION;
        // TODO: Implementare upgrade quando necessario
    }

    return true;
}

// ========== GESTIONE CAMPIONI TEMPO REALE ==========

bool DatabaseManager::saveSample(const DataPoint& dataPoint)
{
    if (!m_db.isOpen()) {
        m_lastError = "Database not open";
        emit errorOccurred(m_lastError);
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO samples (timestamp, powerA, voltageA, currentA, pfA,
                            powerB, voltageB, currentB, pfB,
                            powerC, voltageC, currentC, pfC)
        VALUES (:timestamp, :powerA, :voltageA, :currentA, :pfA,
                :powerB, :voltageB, :currentB, :pfB,
                :powerC, :voltageC, :currentC, :pfC)
    )");

    query.bindValue(":timestamp", dataPoint.timestamp.toSecsSinceEpoch());
    query.bindValue(":powerA", dataPoint.powerA);
    query.bindValue(":voltageA", dataPoint.voltageA);
    query.bindValue(":currentA", dataPoint.currentA);
    query.bindValue(":pfA", dataPoint.powerFactorA);
    query.bindValue(":powerB", dataPoint.powerB);
    query.bindValue(":voltageB", dataPoint.voltageB);
    query.bindValue(":currentB", dataPoint.currentB);
    query.bindValue(":pfB", dataPoint.powerFactorB);
    query.bindValue(":powerC", dataPoint.powerC);
    query.bindValue(":voltageC", dataPoint.voltageC);
    query.bindValue(":currentC", dataPoint.currentC);
    query.bindValue(":pfC", dataPoint.powerFactorC);

    if (!query.exec()) {
        m_lastError = QString("Failed to save sample: %1").arg(query.lastError().text());
        qWarning() << "DatabaseManager:" << m_lastError;
        emit errorOccurred(m_lastError);
        return false;
    }

    return true;
}

bool DatabaseManager::saveSamples(const QVector<DataPoint>& dataPoints)
{
    if (!m_db.isOpen()) {
        m_lastError = "Database not open";
        emit errorOccurred(m_lastError);
        return false;
    }

    if (dataPoints.isEmpty()) {
        return true;
    }

    // Usa transazione per performance migliori
    if (!m_db.transaction()) {
        m_lastError = "Failed to start transaction";
        qWarning() << "DatabaseManager:" << m_lastError;
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO samples (timestamp, powerA, voltageA, currentA, pfA,
                            powerB, voltageB, currentB, pfB,
                            powerC, voltageC, currentC, pfC)
        VALUES (:timestamp, :powerA, :voltageA, :currentA, :pfA,
                :powerB, :voltageB, :currentB, :pfB,
                :powerC, :voltageC, :currentC, :pfC)
    )");

    for (const DataPoint& dp : dataPoints) {
        query.bindValue(":timestamp", dp.timestamp.toSecsSinceEpoch());
        query.bindValue(":powerA", dp.powerA);
        query.bindValue(":voltageA", dp.voltageA);
        query.bindValue(":currentA", dp.currentA);
        query.bindValue(":pfA", dp.powerFactorA);
        query.bindValue(":powerB", dp.powerB);
        query.bindValue(":voltageB", dp.voltageB);
        query.bindValue(":currentB", dp.currentB);
        query.bindValue(":pfB", dp.powerFactorB);
        query.bindValue(":powerC", dp.powerC);
        query.bindValue(":voltageC", dp.voltageC);
        query.bindValue(":currentC", dp.currentC);
        query.bindValue(":pfC", dp.powerFactorC);

        if (!query.exec()) {
            m_lastError = QString("Failed to save sample in batch: %1").arg(query.lastError().text());
            qWarning() << "DatabaseManager:" << m_lastError;
            m_db.rollback();
            emit errorOccurred(m_lastError);
            return false;
        }
    }

    if (!m_db.commit()) {
        m_lastError = "Failed to commit transaction";
        qWarning() << "DatabaseManager:" << m_lastError;
        m_db.rollback();
        return false;
    }

    qDebug() << "DatabaseManager: Saved" << dataPoints.size() << "samples in batch";
    return true;
}

bool DatabaseManager::getSamples(qint64 startTime, qint64 endTime, QVector<DataPoint>& outDataPoints)
{
    if (!m_db.isOpen()) {
        m_lastError = "Database not open";
        return false;
    }

    outDataPoints.clear();

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT timestamp, powerA, voltageA, currentA, pfA,
               powerB, voltageB, currentB, pfB,
               powerC, voltageC, currentC, pfC
        FROM samples
        WHERE timestamp >= :startTime AND timestamp <= :endTime
        ORDER BY timestamp ASC
    )");

    query.bindValue(":startTime", startTime);
    query.bindValue(":endTime", endTime);

    if (!query.exec()) {
        m_lastError = QString("Failed to query samples: %1").arg(query.lastError().text());
        qWarning() << "DatabaseManager:" << m_lastError;
        return false;
    }

    while (query.next()) {
        DataPoint dp;
        dp.timestamp = QDateTime::fromSecsSinceEpoch(query.value(0).toLongLong());
        dp.powerA = query.value(1).toDouble();
        dp.voltageA = query.value(2).toDouble();
        dp.currentA = query.value(3).toDouble();
        dp.powerFactorA = query.value(4).toDouble();
        dp.powerB = query.value(5).toDouble();
        dp.voltageB = query.value(6).toDouble();
        dp.currentB = query.value(7).toDouble();
        dp.powerFactorB = query.value(8).toDouble();
        dp.powerC = query.value(9).toDouble();
        dp.voltageC = query.value(10).toDouble();
        dp.currentC = query.value(11).toDouble();
        dp.powerFactorC = query.value(12).toDouble();

        outDataPoints.append(dp);
    }

    qDebug() << "DatabaseManager: Retrieved" << outDataPoints.size() << "samples";
    return true;
}

bool DatabaseManager::getLastSamples(int count, QVector<DataPoint>& outDataPoints)
{
    if (!m_db.isOpen()) {
        m_lastError = "Database not open";
        return false;
    }

    outDataPoints.clear();

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT timestamp, powerA, voltageA, currentA, pfA,
               powerB, voltageB, currentB, pfB,
               powerC, voltageC, currentC, pfC
        FROM samples
        ORDER BY timestamp DESC
        LIMIT :count
    )");

    query.bindValue(":count", count);

    if (!query.exec()) {
        m_lastError = QString("Failed to query last samples: %1").arg(query.lastError().text());
        qWarning() << "DatabaseManager:" << m_lastError;
        return false;
    }

    while (query.next()) {
        DataPoint dp;
        dp.timestamp = QDateTime::fromSecsSinceEpoch(query.value(0).toLongLong());
        dp.powerA = query.value(1).toDouble();
        dp.voltageA = query.value(2).toDouble();
        dp.currentA = query.value(3).toDouble();
        dp.powerFactorA = query.value(4).toDouble();
        dp.powerB = query.value(5).toDouble();
        dp.voltageB = query.value(6).toDouble();
        dp.currentB = query.value(7).toDouble();
        dp.powerFactorB = query.value(8).toDouble();
        dp.powerC = query.value(9).toDouble();
        dp.voltageC = query.value(10).toDouble();
        dp.currentC = query.value(11).toDouble();
        dp.powerFactorC = query.value(12).toDouble();

        outDataPoints.prepend(dp);  // Prepend per ordine cronologico
    }

    qDebug() << "DatabaseManager: Retrieved last" << outDataPoints.size() << "samples";
    return true;
}

qint64 DatabaseManager::getSampleCount() const
{
    if (!m_db.isOpen()) {
        return 0;
    }

    QSqlQuery query(m_db);
    if (!query.exec("SELECT COUNT(*) FROM samples")) {
        qWarning() << "DatabaseManager: Failed to count samples";
        return 0;
    }

    if (query.next()) {
        return query.value(0).toLongLong();
    }

    return 0;
}

int DatabaseManager::deleteSamplesBefore(qint64 beforeTimestamp)
{
    if (!m_db.isOpen()) {
        m_lastError = "Database not open";
        return -1;
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM samples WHERE timestamp < :timestamp");
    query.bindValue(":timestamp", beforeTimestamp);

    if (!query.exec()) {
        m_lastError = QString("Failed to delete old samples: %1").arg(query.lastError().text());
        qWarning() << "DatabaseManager:" << m_lastError;
        emit errorOccurred(m_lastError);
        return -1;
    }

    int deletedCount = query.numRowsAffected();
    qDebug() << "DatabaseManager: Deleted" << deletedCount << "old samples";
    return deletedCount;
}

// ========== GESTIONE DATI ENERGIA GIORNALIERA ==========

bool DatabaseManager::saveDailyEnergy(const QVector<EnergyDailyData>& energyData, int* outInserted, int* outUpdated)
{
    if (!m_db.isOpen()) {
        m_lastError = "Database not open";
        emit errorOccurred(m_lastError);
        return false;
    }

    if (energyData.isEmpty()) {
        if (outInserted) *outInserted = 0;
        if (outUpdated) *outUpdated = 0;
        return true;
    }

    // Usa transazione per performance
    if (!m_db.transaction()) {
        m_lastError = "Failed to start transaction";
        qWarning() << "DatabaseManager:" << m_lastError;
        return false;
    }

    int insertedCount = 0;
    int updatedCount = 0;

    QSqlQuery checkQuery(m_db);
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT OR REPLACE INTO daily_energy (date, energyA_Wh, energyB_Wh, energyC_Wh, totalEnergy_Wh,
                                             returnedA_Wh, returnedB_Wh, returnedC_Wh, totalReturned_Wh)
        VALUES (:date, :energyA, :energyB, :energyC, :totalEnergy,
                :returnedA, :returnedB, :returnedC, :totalReturned)
    )");

    for (const EnergyDailyData& data : energyData) {
        // Verifica se il record esiste già
        bool exists = false;
        if (outInserted || outUpdated) {
            checkQuery.prepare("SELECT COUNT(*) FROM daily_energy WHERE date = :date");
            checkQuery.bindValue(":date", data.date.toString("yyyy-MM-dd"));
            if (checkQuery.exec() && checkQuery.next()) {
                exists = (checkQuery.value(0).toInt() > 0);
            }
        }

        query.bindValue(":date", data.date.toString("yyyy-MM-dd"));
        query.bindValue(":energyA", data.energyA_Wh);
        query.bindValue(":energyB", data.energyB_Wh);
        query.bindValue(":energyC", data.energyC_Wh);
        query.bindValue(":totalEnergy", data.totalEnergy_Wh);
        query.bindValue(":returnedA", data.returnedA_Wh);
        query.bindValue(":returnedB", data.returnedB_Wh);
        query.bindValue(":returnedC", data.returnedC_Wh);
        query.bindValue(":totalReturned", data.totalReturned_Wh);

        if (!query.exec()) {
            m_lastError = QString("Failed to save daily energy: %1").arg(query.lastError().text());
            qWarning() << "DatabaseManager:" << m_lastError;
            m_db.rollback();
            emit errorOccurred(m_lastError);
            return false;
        }

        // Conta inserimenti e aggiornamenti
        if (exists) {
            updatedCount++;
        } else {
            insertedCount++;
        }
    }

    if (!m_db.commit()) {
        m_lastError = "Failed to commit transaction";
        qWarning() << "DatabaseManager:" << m_lastError;
        m_db.rollback();
        return false;
    }

    if (outInserted) *outInserted = insertedCount;
    if (outUpdated) *outUpdated = updatedCount;

    qDebug() << "DatabaseManager: Saved" << energyData.size() << "daily energy records"
             << "(" << insertedCount << "inserted," << updatedCount << "updated)";
    return true;
}

bool DatabaseManager::getDailyEnergy(const QDate& startDate, const QDate& endDate, QVector<EnergyDailyData>& outEnergyData)
{
    if (!m_db.isOpen()) {
        m_lastError = "Database not open";
        return false;
    }

    outEnergyData.clear();

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT date, energyA_Wh, energyB_Wh, energyC_Wh, totalEnergy_Wh,
               returnedA_Wh, returnedB_Wh, returnedC_Wh, totalReturned_Wh
        FROM daily_energy
        WHERE date >= :startDate AND date <= :endDate
        ORDER BY date ASC
    )");

    query.bindValue(":startDate", startDate.toString("yyyy-MM-dd"));
    query.bindValue(":endDate", endDate.toString("yyyy-MM-dd"));

    if (!query.exec()) {
        m_lastError = QString("Failed to query daily energy: %1").arg(query.lastError().text());
        qWarning() << "DatabaseManager:" << m_lastError;
        return false;
    }

    while (query.next()) {
        EnergyDailyData data;
        data.date = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");
        data.energyA_Wh = query.value(1).toDouble();
        data.energyB_Wh = query.value(2).toDouble();
        data.energyC_Wh = query.value(3).toDouble();
        data.totalEnergy_Wh = query.value(4).toDouble();
        data.returnedA_Wh = query.value(5).toDouble();
        data.returnedB_Wh = query.value(6).toDouble();
        data.returnedC_Wh = query.value(7).toDouble();
        data.totalReturned_Wh = query.value(8).toDouble();

        outEnergyData.append(data);
    }

    qDebug() << "DatabaseManager: Retrieved" << outEnergyData.size() << "daily energy records";
    return true;
}

qint64 DatabaseManager::getDailyEnergyCount() const
{
    if (!m_db.isOpen()) {
        return 0;
    }

    QSqlQuery query(m_db);
    if (!query.exec("SELECT COUNT(*) FROM daily_energy")) {
        qWarning() << "DatabaseManager: Failed to count daily energy records";
        return 0;
    }

    if (query.next()) {
        return query.value(0).toLongLong();
    }

    return 0;
}

// ========== UTILITÀ ==========

bool DatabaseManager::vacuum()
{
    if (!m_db.isOpen()) {
        m_lastError = "Database not open";
        return false;
    }

    QSqlQuery query(m_db);
    if (!query.exec("VACUUM")) {
        m_lastError = QString("Failed to vacuum database: %1").arg(query.lastError().text());
        qWarning() << "DatabaseManager:" << m_lastError;
        return false;
    }

    qDebug() << "DatabaseManager: Database vacuumed successfully";
    return true;
}

qint64 DatabaseManager::getDatabaseSize() const
{
    QFileInfo fileInfo(m_dbPath);
    if (fileInfo.exists()) {
        return fileInfo.size();
    }
    return 0;
}

QString DatabaseManager::getDatabasePath() const
{
    return m_dbPath;
}

bool DatabaseManager::backupDatabase(const QString& backupPath)
{
    if (!m_db.isOpen()) {
        m_lastError = "Database not open";
        return false;
    }

    // Genera nome backup se non fornito
    QString targetPath = backupPath;
    if (targetPath.isEmpty()) {
        QFileInfo fileInfo(m_dbPath);
        QString baseName = fileInfo.baseName();
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        targetPath = fileInfo.dir().filePath(QString("%1_backup_%2.db").arg(baseName).arg(timestamp));
    }

    // Copia il file database
    QFile sourceFile(m_dbPath);
    if (!sourceFile.exists()) {
        m_lastError = "Source database file does not exist";
        return false;
    }

    // Rimuovi file backup esistente se presente
    if (QFile::exists(targetPath)) {
        if (!QFile::remove(targetPath)) {
            m_lastError = QString("Failed to remove existing backup file: %1").arg(targetPath);
            return false;
        }
    }

    // Esegui la copia
    if (!QFile::copy(m_dbPath, targetPath)) {
        m_lastError = QString("Failed to copy database to: %1").arg(targetPath);
        qWarning() << "DatabaseManager:" << m_lastError;
        return false;
    }

    qDebug() << "DatabaseManager: Database backup created at:" << targetPath;
    return true;
}

QString DatabaseManager::lastError() const
{
    return m_lastError;
}

// ========== AGGREGAZIONI TEMPORALI ==========

bool DatabaseManager::getAggregatedData(AggregationInterval interval,
                                       const QDateTime& startTime,
                                       const QDateTime& endTime,
                                       QVector<AggregatedData>& outAggregatedData)
{
    if (!m_db.isOpen()) {
        m_lastError = "Database not open";
        return false;
    }

    outAggregatedData.clear();

    // Determina il formato di raggruppamento SQL in base all'intervallo
    QString groupByFormat;
    QString strftimeFormat;

    switch (interval) {
        case AggregationInterval::Hourly:
            // Raggruppa per ora: "2026-01-02 14:00:00"
            strftimeFormat = "%Y-%m-%d %H:00:00";
            break;
        case AggregationInterval::Daily:
            // Raggruppa per giorno: "2026-01-02"
            strftimeFormat = "%Y-%m-%d";
            break;
        case AggregationInterval::Weekly:
            // Raggruppa per settimana (lunedì come primo giorno)
            strftimeFormat = "%Y-W%W";
            break;
        case AggregationInterval::Monthly:
            // Raggruppa per mese: "2026-01"
            strftimeFormat = "%Y-%m";
            break;
    }

    QSqlQuery query(m_db);
    QString queryStr = QString(R"(
        SELECT
            strftime('%1', datetime(timestamp, 'unixepoch')) as period,
            MIN(timestamp) as min_ts,
            MAX(timestamp) as max_ts,
            COUNT(*) as sample_count,

            -- Fase A
            AVG(powerA) as powerA_avg, MIN(powerA) as powerA_min, MAX(powerA) as powerA_max,
            AVG(voltageA) as voltageA_avg, MIN(voltageA) as voltageA_min, MAX(voltageA) as voltageA_max,
            AVG(currentA) as currentA_avg, MIN(currentA) as currentA_min, MAX(currentA) as currentA_max,
            AVG(pfA) as pfA_avg,

            -- Fase B
            AVG(powerB) as powerB_avg, MIN(powerB) as powerB_min, MAX(powerB) as powerB_max,
            AVG(voltageB) as voltageB_avg, MIN(voltageB) as voltageB_min, MAX(voltageB) as voltageB_max,
            AVG(currentB) as currentB_avg, MIN(currentB) as currentB_min, MAX(currentB) as currentB_max,
            AVG(pfB) as pfB_avg,

            -- Fase C
            AVG(powerC) as powerC_avg, MIN(powerC) as powerC_min, MAX(powerC) as powerC_max,
            AVG(voltageC) as voltageC_avg, MIN(voltageC) as voltageC_min, MAX(voltageC) as voltageC_max,
            AVG(currentC) as currentC_avg, MIN(currentC) as currentC_min, MAX(currentC) as currentC_max,
            AVG(pfC) as pfC_avg
        FROM samples
        WHERE timestamp >= :startTime AND timestamp <= :endTime
        GROUP BY period
        ORDER BY period ASC
    )").arg(strftimeFormat);

    query.prepare(queryStr);
    query.bindValue(":startTime", startTime.toSecsSinceEpoch());
    query.bindValue(":endTime", endTime.toSecsSinceEpoch());

    if (!query.exec()) {
        m_lastError = QString("Failed to aggregate data: %1").arg(query.lastError().text());
        qWarning() << "DatabaseManager:" << m_lastError;
        return false;
    }

    while (query.next()) {
        AggregatedData aggData;
        aggData.interval = interval;

        // Timestamp periodo
        qint64 minTs = query.value(1).toLongLong();
        qint64 maxTs = query.value(2).toLongLong();
        aggData.startTime = QDateTime::fromSecsSinceEpoch(minTs);
        aggData.endTime = QDateTime::fromSecsSinceEpoch(maxTs);
        aggData.sampleCount = query.value(3).toInt();

        // Fase A
        aggData.powerA_avg = query.value(4).toDouble();
        aggData.powerA_min = query.value(5).toDouble();
        aggData.powerA_max = query.value(6).toDouble();
        aggData.voltageA_avg = query.value(7).toDouble();
        aggData.voltageA_min = query.value(8).toDouble();
        aggData.voltageA_max = query.value(9).toDouble();
        aggData.currentA_avg = query.value(10).toDouble();
        aggData.currentA_min = query.value(11).toDouble();
        aggData.currentA_max = query.value(12).toDouble();
        aggData.powerFactorA_avg = query.value(13).toDouble();

        // Fase B
        aggData.powerB_avg = query.value(14).toDouble();
        aggData.powerB_min = query.value(15).toDouble();
        aggData.powerB_max = query.value(16).toDouble();
        aggData.voltageB_avg = query.value(17).toDouble();
        aggData.voltageB_min = query.value(18).toDouble();
        aggData.voltageB_max = query.value(19).toDouble();
        aggData.currentB_avg = query.value(20).toDouble();
        aggData.currentB_min = query.value(21).toDouble();
        aggData.currentB_max = query.value(22).toDouble();
        aggData.powerFactorB_avg = query.value(23).toDouble();

        // Fase C
        aggData.powerC_avg = query.value(24).toDouble();
        aggData.powerC_min = query.value(25).toDouble();
        aggData.powerC_max = query.value(26).toDouble();
        aggData.voltageC_avg = query.value(27).toDouble();
        aggData.voltageC_min = query.value(28).toDouble();
        aggData.voltageC_max = query.value(29).toDouble();
        aggData.currentC_avg = query.value(30).toDouble();
        aggData.currentC_min = query.value(31).toDouble();
        aggData.currentC_max = query.value(32).toDouble();
        aggData.powerFactorC_avg = query.value(33).toDouble();

        // Calcola potenza totale e energia
        aggData.totalPower_avg = aggData.powerA_avg + aggData.powerB_avg + aggData.powerC_avg;
        aggData.totalPower_min = aggData.powerA_min + aggData.powerB_min + aggData.powerC_min;
        aggData.totalPower_max = aggData.powerA_max + aggData.powerB_max + aggData.powerC_max;

        // Calcola energia consumata nell'intervallo
        // Energia (Wh) = Potenza media (W) × Tempo (h)
        qint64 durationSeconds = aggData.startTime.secsTo(aggData.endTime);
        double durationHours = durationSeconds / 3600.0;

        aggData.energyA_Wh = aggData.powerA_avg * durationHours;
        aggData.energyB_Wh = aggData.powerB_avg * durationHours;
        aggData.energyC_Wh = aggData.powerC_avg * durationHours;
        aggData.totalEnergy_Wh = aggData.energyA_Wh + aggData.energyB_Wh + aggData.energyC_Wh;
        aggData.calculateKWh();

        outAggregatedData.append(aggData);
    }

    qDebug() << "DatabaseManager: Aggregated" << outAggregatedData.size() << "periods";
    return true;
}

bool DatabaseManager::getComparisonStats(AggregationInterval interval,
                                        const QDateTime& currentStart,
                                        const QDateTime& currentEnd,
                                        const QDateTime& previousStart,
                                        const QDateTime& previousEnd,
                                        ComparisonStats& outComparison)
{
    QVector<AggregatedData> currentData;
    QVector<AggregatedData> previousData;

    // Ottieni dati aggregati per periodo corrente
    if (!getAggregatedData(interval, currentStart, currentEnd, currentData)) {
        m_lastError = "Failed to get current period data";
        return false;
    }

    // Ottieni dati aggregati per periodo precedente
    if (!getAggregatedData(interval, previousStart, previousEnd, previousData)) {
        m_lastError = "Failed to get previous period data";
        return false;
    }

    // Somma tutti i periodi per ottenere i totali
    auto sumAggregatedData = [](const QVector<AggregatedData>& dataVec) -> AggregatedData {
        AggregatedData total;
        if (dataVec.isEmpty()) return total;

        total.startTime = dataVec.first().startTime;
        total.endTime = dataVec.last().endTime;
        total.interval = dataVec.first().interval;

        for (const AggregatedData& data : dataVec) {
            total.energyA_Wh += data.energyA_Wh;
            total.energyB_Wh += data.energyB_Wh;
            total.energyC_Wh += data.energyC_Wh;
            total.totalEnergy_Wh += data.totalEnergy_Wh;
            total.sampleCount += data.sampleCount;
        }

        // Calcola potenza media globale
        if (!dataVec.isEmpty()) {
            double sumPowerA = 0, sumPowerB = 0, sumPowerC = 0;
            for (const AggregatedData& data : dataVec) {
                sumPowerA += data.powerA_avg;
                sumPowerB += data.powerB_avg;
                sumPowerC += data.powerC_avg;
            }
            total.powerA_avg = sumPowerA / dataVec.size();
            total.powerB_avg = sumPowerB / dataVec.size();
            total.powerC_avg = sumPowerC / dataVec.size();
            total.totalPower_avg = total.powerA_avg + total.powerB_avg + total.powerC_avg;
        }

        total.calculateKWh();
        return total;
    };

    outComparison.current = sumAggregatedData(currentData);
    outComparison.previous = sumAggregatedData(previousData);
    outComparison.calculate();

    qDebug() << "DatabaseManager: Comparison calculated - Current:" << outComparison.current.totalEnergy_kWh
             << "kWh, Previous:" << outComparison.previous.totalEnergy_kWh << "kWh";
    return true;
}

#include "shellycsvimporter.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>
#include <QStringConverter>
#include <algorithm>

ShellyCsvImporter::ShellyCsvImporter() {}

/**
 * Rileva automaticamente il formato del CSV analizzando le prime righe
 */
ShellyCsvImporter::CsvFormat ShellyCsvImporter::detectFormat(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file for format detection:" << filePath;
        return FORMAT_UNKNOWN;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);  // Qt 6: gestisce BOM UTF-8

    // Leggi prime 20 righe
    int lineCount = 0;
    while (!in.atEnd() && lineCount < 20) {
        QString line = in.readLine().trimmed();

        // Cerca pattern caratteristici formato Shelly Cloud
        if (line.contains("Fase A", Qt::CaseInsensitive) ||
            line.contains("Phase A", Qt::CaseInsensitive) ||
            line.contains("Totale", Qt::CaseInsensitive)) {
            file.close();
            return FORMAT_SHELLY_CLOUD_DAILY;
        }

        // Cerca pattern formato nativo app (futuro)
        if (line.startsWith("Timestamp,Date,Time,PowerA", Qt::CaseInsensitive)) {
            file.close();
            return FORMAT_NATIVE_REALTIME;
        }

        lineCount++;
    }

    file.close();
    return FORMAT_UNKNOWN;
}

/**
 * Entry point principale per import CSV
 */
bool ShellyCsvImporter::importShellyCsvFile(const QString& filePath,
                                             QVector<EnergyDailyData>& outData,
                                             QString& errorMessage) {
    // Verifica esistenza file
    if (!QFile::exists(filePath)) {
        errorMessage = QString("File not found: %1").arg(filePath);
        return false;
    }

    // Detect formato
    CsvFormat format = detectFormat(filePath);

    if (format == FORMAT_SHELLY_CLOUD_DAILY) {
        return parseShellyCsvMultiSection(filePath, outData, errorMessage);
    } else if (format == FORMAT_NATIVE_REALTIME) {
        errorMessage = "Native realtime CSV format not supported for import. "
                      "Use database history instead.";
        return false;
    } else {
        errorMessage = "Unknown or unsupported CSV format";
        return false;
    }
}

/**
 * Parser specifico per formato Shelly Cloud multi-sezione
 */
bool ShellyCsvImporter::parseShellyCsvMultiSection(const QString& filePath,
                                                    QVector<EnergyDailyData>& outData,
                                                    QString& errorMessage) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorMessage = QString("Cannot open file: %1").arg(filePath);
        return false;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    // Map per raggruppare dati per data
    // Key: data, Value: struttura con tutti i valori per quella data
    QMap<QDate, EnergyDailyData> dataMap;

    Section currentSection = SECTION_UNKNOWN;
    int lineNumber = 0;
    int dataLinesRead = 0;
    int dataLinesSkipped = 0;

    qDebug() << "Starting CSV import from:" << QFileInfo(filePath).fileName();

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        lineNumber++;

        // Salta righe vuote
        if (line.isEmpty()) {
            continue;
        }

        // Detect cambio sezione
        Section newSection = detectSection(line);
        if (newSection != SECTION_UNKNOWN) {
            currentSection = newSection;
            qDebug() << "  Section detected at line" << lineNumber << ":" << line;
            // Salta la prossima riga (header "Ore, Wh")
            if (!in.atEnd()) {
                in.readLine();
                lineNumber++;
            }
            continue;
        }

        // Salta header "Ore, Wh" o "Hours, Wh"
        if (line.startsWith("Ore,", Qt::CaseInsensitive) ||
            line.startsWith("Hours,", Qt::CaseInsensitive)) {
            continue;
        }

        // Parse data line
        QDate date;
        double value;
        if (!parseDataLine(line, date, value)) {
            dataLinesSkipped++;
            if (dataLinesSkipped <= 5) {  // Mostra solo primi 5 errori
                qWarning() << "Failed to parse line" << lineNumber << ":" << line;
            }
            continue;
        }

        // Aggiungi al map (crea entry se non esiste)
        if (!dataMap.contains(date)) {
            EnergyDailyData data;
            data.date = date;
            dataMap[date] = data;
        }

        // Assegna valore alla colonna corretta in base alla sezione
        switch (currentSection) {
            case SECTION_PHASE_A:
                dataMap[date].energyA_Wh = value;
                break;
            case SECTION_PHASE_B:
                dataMap[date].energyB_Wh = value;
                break;
            case SECTION_PHASE_C:
                dataMap[date].energyC_Wh = value;
                break;
            case SECTION_TOTAL:
                dataMap[date].totalEnergy_Wh = value;
                break;
            case SECTION_RETURNED_A:
                dataMap[date].returnedA_Wh = value;
                break;
            case SECTION_RETURNED_B:
                dataMap[date].returnedB_Wh = value;
                break;
            case SECTION_RETURNED_C:
                dataMap[date].returnedC_Wh = value;
                break;
            case SECTION_RETURNED_TOTAL:
                dataMap[date].totalReturned_Wh = value;
                break;
            default:
                qWarning() << "Unknown section at line" << lineNumber;
                break;
        }

        dataLinesRead++;
    }

    file.close();

    if (dataLinesSkipped > 5) {
        qWarning() << "... and" << (dataLinesSkipped - 5) << "more lines skipped";
    }

    if (dataMap.isEmpty()) {
        errorMessage = "No valid data found in CSV file";
        return false;
    }

    // Converti map in vector ordinato per data
    outData.clear();
    outData.reserve(dataMap.size());
    for (const EnergyDailyData& data : dataMap.values()) {
        if (data.isValid()) {
            outData.append(data);
        }
    }

    // Ordina per data crescente
    std::sort(outData.begin(), outData.end(),
              [](const EnergyDailyData& a, const EnergyDailyData& b) {
                  return a.date < b.date;
              });

    qDebug() << "CSV import completed:";
    qDebug() << "  - Total lines read:" << lineNumber;
    qDebug() << "  - Data lines parsed:" << dataLinesRead;
    qDebug() << "  - Lines skipped:" << dataLinesSkipped;
    qDebug() << "  - Daily records extracted:" << outData.size();

    return true;
}

/**
 * Rileva sezione corrente da header line
 */
ShellyCsvImporter::Section ShellyCsvImporter::detectSection(const QString& line) {
    QString lower = line.toLower();

    // Fase A consumata
    if (lower.contains("fase a") && !lower.contains("restituita")) {
        return SECTION_PHASE_A;
    }
    // Fase B consumata
    if (lower.contains("fase b") && !lower.contains("restituita")) {
        return SECTION_PHASE_B;
    }
    // Fase C consumata
    if (lower.contains("fase c") && !lower.contains("restituita")) {
        return SECTION_PHASE_C;
    }
    // Totale consumato
    if (lower.contains("totale") && !lower.contains("restituito")) {
        return SECTION_TOTAL;
    }
    // Fase A restituita
    if (lower.contains("fase a") && lower.contains("restituita")) {
        return SECTION_RETURNED_A;
    }
    // Fase B restituita
    if (lower.contains("fase b") && lower.contains("restituita")) {
        return SECTION_RETURNED_B;
    }
    // Fase C restituita
    if (lower.contains("fase c") && lower.contains("restituita")) {
        return SECTION_RETURNED_C;
    }
    // Totale restituito
    if (lower.contains("totale") && lower.contains("restituito")) {
        return SECTION_RETURNED_TOTAL;
    }

    return SECTION_UNKNOWN;
}

/**
 * Parse singola riga dati: " 29/12/2025 00:00 , 5696.99 "
 */
bool ShellyCsvImporter::parseDataLine(const QString& line,
                                       QDate& outDate,
                                       double& outValue) {
    // Split per virgola
    QStringList parts = line.split(',');
    if (parts.size() != 2) {
        return false;
    }

    // Parse data (prima colonna)
    QString dateStr = parts[0].trimmed();
    outDate = parseShellyDate(dateStr);
    if (!outDate.isValid()) {
        return false;
    }

    // Parse valore (seconda colonna)
    QString valueStr = parts[1].trimmed();
    bool ok;
    outValue = valueStr.toDouble(&ok);
    if (!ok) {
        return false;
    }

    return true;
}

/**
 * Parse formato data Shelly: "DD/MM/YYYY HH:MM"
 * Esempi: "01/12/2025 00:00", "29/12/2024 00:00"
 */
QDate ShellyCsvImporter::parseShellyDate(const QString& dateStr) {
    // Formato completo: DD/MM/YYYY HH:MM
    QDateTime dt = QDateTime::fromString(dateStr, "dd/MM/yyyy HH:mm");
    if (dt.isValid()) {
        return dt.date();
    }

    // Prova formato senza ora: DD/MM/YYYY
    QDate date = QDate::fromString(dateStr.left(10), "dd/MM/yyyy");
    if (date.isValid()) {
        return date;
    }

    // Prova formato con separatore diverso: DD-MM-YYYY
    date = QDate::fromString(dateStr.left(10), "dd-MM-yyyy");
    return date;
}

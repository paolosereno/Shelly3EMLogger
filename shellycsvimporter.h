#ifndef SHELLYCSVIMPORTER_H
#define SHELLYCSVIMPORTER_H

#include <QString>
#include <QVector>
#include <QDateTime>
#include <QMap>

/**
 * @brief Struttura per dati energia giornaliera
 *
 * Contiene i dati aggregati per un singolo giorno, estratti dai CSV
 * esportati dall'app Shelly Cloud.
 */
struct EnergyDailyData {
    QDate date;              // Data (YYYY-MM-DD)

    // Energia consumata (Wh)
    double energyA_Wh;       // Fase A consumata
    double energyB_Wh;       // Fase B consumata
    double energyC_Wh;       // Fase C consumata
    double totalEnergy_Wh;   // Totale consumata

    // Energia restituita (Wh) - per impianti con produzione (es: fotovoltaico)
    double returnedA_Wh;     // Fase A restituita
    double returnedB_Wh;     // Fase B restituita
    double returnedC_Wh;     // Fase C restituita
    double totalReturned_Wh; // Totale restituita

    // Costruttore di default
    EnergyDailyData()
        : energyA_Wh(0), energyB_Wh(0), energyC_Wh(0), totalEnergy_Wh(0),
          returnedA_Wh(0), returnedB_Wh(0), returnedC_Wh(0), totalReturned_Wh(0) {}

    // Validazione
    bool isValid() const { return date.isValid(); }
};

/**
 * @brief Classe per import CSV esportati da Shelly Cloud
 *
 * Supporta il parsing di file CSV multi-sezione esportati dall'app mobile
 * Shelly Cloud, con dati energia giornaliera aggregata per fase.
 *
 * Formato CSV Shelly Cloud:
 * - Sezioni multiple separate da header testuali
 * - Granularità giornaliera (1 campione/giorno)
 * - Formato data: "DD/MM/YYYY HH:MM"
 * - Separatore: ", " (virgola + spazio)
 */
class ShellyCsvImporter {
public:
    enum CsvFormat {
        FORMAT_UNKNOWN,
        FORMAT_SHELLY_CLOUD_DAILY,  // Formato em3.csv (multi-sezione)
        FORMAT_NATIVE_REALTIME      // Formato nativo app (futuro)
    };

    ShellyCsvImporter();

    /**
     * @brief Rileva automaticamente il formato del CSV
     * @param filePath Percorso del file CSV
     * @return Formato rilevato
     */
    CsvFormat detectFormat(const QString& filePath);

    /**
     * @brief Importa file CSV Shelly Cloud
     * @param filePath Percorso del file CSV
     * @param outData Vector di output con dati energia giornaliera
     * @param errorMessage Messaggio di errore in caso di fallimento
     * @return true se import riuscito, false altrimenti
     */
    bool importShellyCsvFile(const QString& filePath,
                             QVector<EnergyDailyData>& outData,
                             QString& errorMessage);

private:
    /**
     * @brief Parser specifico per formato Shelly Cloud multi-sezione
     */
    bool parseShellyCsvMultiSection(const QString& filePath,
                                     QVector<EnergyDailyData>& outData,
                                     QString& errorMessage);

    // Enum per sezioni del CSV
    enum Section {
        SECTION_UNKNOWN,
        SECTION_PHASE_A,          // "Fase A"
        SECTION_PHASE_B,          // "Fase B"
        SECTION_PHASE_C,          // "Fase C"
        SECTION_TOTAL,            // "Totale"
        SECTION_RETURNED_A,       // "Fase A restituita"
        SECTION_RETURNED_B,       // "Fase B restituita"
        SECTION_RETURNED_C,       // "Fase C restituita"
        SECTION_RETURNED_TOTAL    // "Totale restituito"
    };

    /**
     * @brief Rileva sezione corrente da header line
     * @param line Riga del CSV
     * @return Sezione identificata
     */
    Section detectSection(const QString& line);

    /**
     * @brief Parse riga dati: " 29/12/2025 00:00 , 5696.99 "
     * @param line Riga da parsare
     * @param outDate Data estratta
     * @param outValue Valore energia estratto (Wh)
     * @return true se parsing riuscito
     */
    bool parseDataLine(const QString& line, QDate& outDate, double& outValue);

    /**
     * @brief Parse formato data Shelly: "DD/MM/YYYY HH:MM"
     * @param dateStr Stringa data
     * @return QDate parsato
     */
    QDate parseShellyDate(const QString& dateStr);
};

#endif // SHELLYCSVIMPORTER_H

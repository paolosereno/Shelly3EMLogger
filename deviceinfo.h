#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <QString>
#include <QDateTime>

/**
 * @brief Struttura per memorizzare informazioni complete dal dispositivo Shelly EM3
 *
 * Contiene tutti i dati ricevuti dall'endpoint /status del dispositivo:
 * - Informazioni generali (modello, firmware, uptime)
 * - Stato WiFi (SSID, segnale, cloud)
 * - Risorse di sistema (RAM, storage, temperatura)
 * - Totali energia per fase
 * - Diagnostica (neutral current, allarmi)
 */
struct DeviceInfo {
    // === GENERAL INFO ===
    QString model;              // Es: "SHEM-3"
    QString firmware_version;   // Es: "1.14.0"
    QString ip_address;         // Es: "192.168.1.100"
    QString mac_address;        // Es: "AA:BB:CC:DD:EE:FF"
    int uptime_seconds;         // Secondi dall'ultimo avvio
    QDateTime last_update;      // Timestamp ultimo aggiornamento

    // === WIFI INFO ===
    QString wifi_ssid;          // Nome rete WiFi
    int wifi_rssi;              // Potenza segnale in dBm (es: -45)
    bool wifi_connected;        // Connesso al WiFi
    bool cloud_connected;       // Connesso al cloud Shelly
    bool cloud_enabled;         // Cloud abilitato

    // === SYSTEM RESOURCES ===
    int ram_total;              // RAM totale in KB
    int ram_free;               // RAM libera in KB
    int fs_size;                // Filesystem size in KB
    int fs_free;                // Filesystem free in KB
    double temperature;         // Temperatura dispositivo in °C
    bool overtemperature;       // Flag sovratemperatura

    // === ENERGY TOTALS ===
    double total_energy_wh[3];  // Energia totale per fase in Wh [A, B, C]
    double total_returned_wh;   // Energia restituita alla rete in Wh
    bool total_valid[3];        // Validità dati energia per fase

    // === DIAGNOSTICS ===
    double neutral_current;     // Corrente sul neutro in A
    bool neutral_mismatch;      // Flag mismatch neutro

    // === UPDATE INFO ===
    bool update_available;      // Aggiornamento firmware disponibile
    QString new_version;        // Nuova versione disponibile
    QString old_version;        // Versione corrente

    /**
     * @brief Costruttore di default - inizializza con valori vuoti/zero
     */
    DeviceInfo()
        : uptime_seconds(0)
        , wifi_rssi(0)
        , wifi_connected(false)
        , cloud_connected(false)
        , cloud_enabled(false)
        , ram_total(0)
        , ram_free(0)
        , fs_size(0)
        , fs_free(0)
        , temperature(0.0)
        , overtemperature(false)
        , total_returned_wh(0.0)
        , neutral_current(0.0)
        , neutral_mismatch(false)
        , update_available(false)
    {
        total_energy_wh[0] = total_energy_wh[1] = total_energy_wh[2] = 0.0;
        total_valid[0] = total_valid[1] = total_valid[2] = false;
    }

    /**
     * @brief Verifica se i dati sono validi (aggiornati di recente)
     */
    bool isValid() const {
        return !last_update.isNull();
    }

    /**
     * @brief Formatta l'uptime in formato leggibile
     * @return Stringa formato "Xd Yh Zm" (es: "5d 3h 25m")
     */
    QString getFormattedUptime() const {
        int days = uptime_seconds / 86400;
        int hours = (uptime_seconds % 86400) / 3600;
        int minutes = (uptime_seconds % 3600) / 60;

        return QString("%1d %2h %3m").arg(days).arg(hours).arg(minutes);
    }

    /**
     * @brief Percentuale RAM utilizzata
     */
    double getRamUsagePercent() const {
        if (ram_total <= 0) return 0.0;
        return ((ram_total - ram_free) * 100.0) / ram_total;
    }

    /**
     * @brief Percentuale storage utilizzato
     */
    double getStorageUsagePercent() const {
        if (fs_size <= 0) return 0.0;
        return ((fs_size - fs_free) * 100.0) / fs_size;
    }

    /**
     * @brief Qualità segnale WiFi in formato testuale
     * @return "Excellent", "Good", "Fair", "Poor", o "No Signal"
     */
    QString getWiFiQuality() const {
        if (wifi_rssi >= -50) return "Excellent";
        else if (wifi_rssi >= -60) return "Good";
        else if (wifi_rssi >= -70) return "Fair";
        else if (wifi_rssi >= -80) return "Poor";
        else return "No Signal";
    }

    /**
     * @brief Numero di "barre" del segnale WiFi (0-5)
     */
    int getWiFiBars() const {
        if (wifi_rssi >= -50) return 5;
        else if (wifi_rssi >= -60) return 4;
        else if (wifi_rssi >= -70) return 3;
        else if (wifi_rssi >= -80) return 2;
        else if (wifi_rssi >= -90) return 1;
        else return 0;
    }

    /**
     * @brief Calcola lo sbilanciamento del carico tra le fasi (percentuale)
     * @return Valore 0-100 che indica quanto sono sbilanciate le fasi
     */
    double getPhaseImbalance() const {
        if (!total_valid[0] || !total_valid[1] || !total_valid[2]) {
            return 0.0;
        }

        double max_energy = qMax(qMax(total_energy_wh[0], total_energy_wh[1]), total_energy_wh[2]);
        double min_energy = qMin(qMin(total_energy_wh[0], total_energy_wh[1]), total_energy_wh[2]);
        double avg_energy = (total_energy_wh[0] + total_energy_wh[1] + total_energy_wh[2]) / 3.0;

        if (avg_energy < 0.1) return 0.0;  // Evita divisione per zero

        return ((max_energy - min_energy) / avg_energy) * 100.0;
    }
};

#endif // DEVICEINFO_H

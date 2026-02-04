#ifndef AGGREGATEDDATA_H
#define AGGREGATEDDATA_H

#include <QDateTime>
#include <QString>

/**
 * @brief Intervalli temporali per aggregazione dati
 */
enum class AggregationInterval {
    Hourly,      // Aggregazione oraria
    Daily,       // Aggregazione giornaliera
    Weekly,      // Aggregazione settimanale
    Monthly      // Aggregazione mensile
};

/**
 * @brief Struttura dati aggregati per un intervallo temporale
 *
 * Contiene statistiche aggregate (min, max, avg, total) per tutte e 3 le fasi
 * in un determinato intervallo di tempo.
 */
struct AggregatedData {
    QDateTime startTime;        // Inizio intervallo
    QDateTime endTime;          // Fine intervallo
    AggregationInterval interval;  // Tipo di intervallo

    // === FASE A ===
    double powerA_avg;          // Potenza media (W)
    double powerA_min;          // Potenza minima (W)
    double powerA_max;          // Potenza massima (W)
    double energyA_Wh;          // Energia totale (Wh)

    double voltageA_avg;        // Tensione media (V)
    double voltageA_min;        // Tensione minima (V)
    double voltageA_max;        // Tensione massima (V)

    double currentA_avg;        // Corrente media (A)
    double currentA_min;        // Corrente minima (A)
    double currentA_max;        // Corrente massima (A)

    double powerFactorA_avg;    // Fattore di potenza medio

    // === FASE B ===
    double powerB_avg;
    double powerB_min;
    double powerB_max;
    double energyB_Wh;

    double voltageB_avg;
    double voltageB_min;
    double voltageB_max;

    double currentB_avg;
    double currentB_min;
    double currentB_max;

    double powerFactorB_avg;

    // === FASE C ===
    double powerC_avg;
    double powerC_min;
    double powerC_max;
    double energyC_Wh;

    double voltageC_avg;
    double voltageC_min;
    double voltageC_max;

    double currentC_avg;
    double currentC_min;
    double currentC_max;

    double powerFactorC_avg;

    // === TOTALI ===
    double totalPower_avg;      // Potenza totale media (W)
    double totalPower_min;      // Potenza totale minima (W)
    double totalPower_max;      // Potenza totale massima (W)
    double totalEnergy_Wh;      // Energia totale (Wh)
    double totalEnergy_kWh;     // Energia totale (kWh)

    int sampleCount;            // Numero di campioni aggregati

    // Costruttore di default
    AggregatedData()
        : interval(AggregationInterval::Hourly)
        , powerA_avg(0), powerA_min(0), powerA_max(0), energyA_Wh(0)
        , voltageA_avg(0), voltageA_min(0), voltageA_max(0)
        , currentA_avg(0), currentA_min(0), currentA_max(0)
        , powerFactorA_avg(0)
        , powerB_avg(0), powerB_min(0), powerB_max(0), energyB_Wh(0)
        , voltageB_avg(0), voltageB_min(0), voltageB_max(0)
        , currentB_avg(0), currentB_min(0), currentB_max(0)
        , powerFactorB_avg(0)
        , powerC_avg(0), powerC_min(0), powerC_max(0), energyC_Wh(0)
        , voltageC_avg(0), voltageC_min(0), voltageC_max(0)
        , currentC_avg(0), currentC_min(0), currentC_max(0)
        , powerFactorC_avg(0)
        , totalPower_avg(0), totalPower_min(0), totalPower_max(0)
        , totalEnergy_Wh(0), totalEnergy_kWh(0)
        , sampleCount(0)
    {}

    // Helper: Calcola kWh da Wh
    void calculateKWh() {
        totalEnergy_kWh = totalEnergy_Wh / 1000.0;
    }

    // Helper: Ottieni descrizione intervallo
    QString getIntervalDescription() const {
        switch (interval) {
            case AggregationInterval::Hourly:
                return QString("Hourly: %1").arg(startTime.toString("yyyy-MM-dd HH:00"));
            case AggregationInterval::Daily:
                return QString("Daily: %1").arg(startTime.toString("yyyy-MM-dd"));
            case AggregationInterval::Weekly:
                return QString("Weekly: %1 - %2")
                    .arg(startTime.toString("yyyy-MM-dd"))
                    .arg(endTime.toString("yyyy-MM-dd"));
            case AggregationInterval::Monthly:
                return QString("Monthly: %1").arg(startTime.toString("yyyy-MM"));
            default:
                return "Unknown";
        }
    }

    // Helper: Verifica validità
    bool isValid() const {
        return startTime.isValid() && sampleCount > 0;
    }
};

/**
 * @brief Struttura per statistiche comparative tra periodi
 */
struct ComparisonStats {
    AggregatedData current;       // Periodo corrente
    AggregatedData previous;      // Periodo precedente

    // Variazioni percentuali
    double energyChange_percent;  // Variazione energia totale (%)
    double powerChange_percent;   // Variazione potenza media (%)
    double costChange_percent;    // Variazione costo (%)

    // Trend
    enum Trend {
        Increasing,    // In aumento
        Decreasing,    // In diminuzione
        Stable         // Stabile (variazione < 5%)
    };
    Trend energyTrend;
    Trend powerTrend;

    // Costruttore di default
    ComparisonStats()
        : energyChange_percent(0)
        , powerChange_percent(0)
        , costChange_percent(0)
        , energyTrend(Stable)
        , powerTrend(Stable)
    {}

    // Helper: Calcola variazioni
    void calculate() {
        if (previous.isValid() && previous.totalEnergy_Wh > 0) {
            energyChange_percent = ((current.totalEnergy_Wh - previous.totalEnergy_Wh)
                                   / previous.totalEnergy_Wh) * 100.0;
            powerChange_percent = ((current.totalPower_avg - previous.totalPower_avg)
                                  / previous.totalPower_avg) * 100.0;

            // Determina trend (> 5% = significativo)
            energyTrend = (energyChange_percent > 5.0) ? Increasing :
                         (energyChange_percent < -5.0) ? Decreasing : Stable;
            powerTrend = (powerChange_percent > 5.0) ? Increasing :
                        (powerChange_percent < -5.0) ? Decreasing : Stable;
        }
    }

    // Helper: Ottieni descrizione trend
    QString getTrendDescription(Trend trend) const {
        switch (trend) {
            case Increasing: return "↑ Increasing";
            case Decreasing: return "↓ Decreasing";
            case Stable: return "→ Stable";
            default: return "Unknown";
        }
    }
};

#endif // AGGREGATEDDATA_H

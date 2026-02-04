#ifndef DATAPOINT_H
#define DATAPOINT_H

#include <QDateTime>

/**
 * @brief Struttura per memorizzare un singolo punto dati acquisito dallo Shelly EM3
 *
 * Contiene timestamp e tutti i parametri elettrici di tutte e 3 le fasi (A, B, C)
 */
struct DataPoint {
    QDateTime timestamp;    // Data e ora della rilevazione

    // Fase A (Phase A)
    double powerA;          // Potenza in Watt
    double voltageA;        // Tensione in Volt
    double currentA;        // Corrente in Ampere
    double powerFactorA;    // Fattore di potenza (0-1)

    // Fase B (Phase B)
    double powerB;          // Potenza in Watt
    double voltageB;        // Tensione in Volt
    double currentB;        // Corrente in Ampere
    double powerFactorB;    // Fattore di potenza (0-1)

    // Fase C (Phase C)
    double powerC;          // Potenza in Watt
    double voltageC;        // Tensione in Volt
    double currentC;        // Corrente in Ampere
    double powerFactorC;    // Fattore di potenza (0-1)

    // Costruttore di default
    DataPoint()
        : powerA(0.0), voltageA(0.0), currentA(0.0), powerFactorA(0.0)
        , powerB(0.0), voltageB(0.0), currentB(0.0), powerFactorB(0.0)
        , powerC(0.0), voltageC(0.0), currentC(0.0), powerFactorC(0.0)
    {}

    // Costruttore con parametri per tutte le fasi
    DataPoint(const QDateTime& ts,
              double pA, double vA, double cA, double pfA,
              double pB, double vB, double cB, double pfB,
              double pC, double vC, double cC, double pfC)
        : timestamp(ts)
        , powerA(pA), voltageA(vA), currentA(cA), powerFactorA(pfA)
        , powerB(pB), voltageB(vB), currentB(cB), powerFactorB(pfB)
        , powerC(pC), voltageC(vC), currentC(cC), powerFactorC(pfC)
    {}

    // Verifica se il punto dati è valido (almeno una fase deve essere valida)
    bool isValid() const {
        return timestamp.isValid() &&
               (isPhaseAValid() || isPhaseBValid() || isPhaseCValid());
    }

    // Verifica validità singola fase
    bool isPhaseAValid() const {
        return powerA >= 0 && voltageA > 0 && currentA >= 0 &&
               powerFactorA >= 0 && powerFactorA <= 1;
    }

    bool isPhaseBValid() const {
        return powerB >= 0 && voltageB > 0 && currentB >= 0 &&
               powerFactorB >= 0 && powerFactorB <= 1;
    }

    bool isPhaseCValid() const {
        return powerC >= 0 && voltageC > 0 && currentC >= 0 &&
               powerFactorC >= 0 && powerFactorC <= 1;
    }

    // Calcola la potenza totale (somma delle 3 fasi)
    double getTotalPower() const {
        return powerA + powerB + powerC;
    }
};

#endif // DATAPOINT_H

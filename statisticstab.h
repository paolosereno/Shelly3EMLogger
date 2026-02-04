#ifndef STATISTICSTAB_H
#define STATISTICSTAB_H

#include <QWidget>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTableWidget>
#include "qcustomplot.h"
#include "databasemanager.h"
#include "aggregateddata.h"

/**
 * @brief Tab per visualizzazione statistiche e aggregazioni temporali
 *
 * Funzionalità:
 * - Selezione intervallo temporale (Hourly, Daily, Weekly, Monthly)
 * - Selezione range date (start/end)
 * - Visualizzazione dati aggregati in tabella
 * - Grafici energia consumata per periodo
 * - Confronto tra periodi (es. questo mese vs mese scorso)
 * - Calcolo costi energia con prezzo configurabile
 * - Analisi trend
 */
class StatisticsTab : public QWidget
{
    Q_OBJECT

public:
    explicit StatisticsTab(DatabaseManager* dbManager, QWidget *parent = nullptr);
    ~StatisticsTab();

    /**
     * @brief Restituisce il grafico energia corrente per export
     */
    QCustomPlot* getCurrentChart() const;

public slots:
    /**
     * @brief Aggiorna la visualizzazione con nuovi dati
     */
    void refresh();

private slots:
    void onIntervalChanged(int index);
    void onQueryClicked();
    void onCompareClicked();
    void onExportStatsClicked();
    void onQuickRangeClicked();

private:
    void setupUI();
    void setupConnections();
    void setupChart();

    /**
     * @brief Aggiorna la tabella con i dati aggregati
     */
    void updateTable(const QVector<AggregatedData>& data);

    /**
     * @brief Aggiorna il grafico con i dati aggregati
     */
    void updateChart(const QVector<AggregatedData>& data);

    /**
     * @brief Mostra confronto tra due periodi
     */
    void showComparison(const ComparisonStats& comparison);

    /**
     * @brief Formatta valore energia con unità appropriata
     */
    QString formatEnergy(double wh) const;

    /**
     * @brief Formatta valore potenza con unità appropriata
     */
    QString formatPower(double w) const;

    /**
     * @brief Calcola costo basato su energia e prezzo
     */
    double calculateCost(double energyWh, double pricePerKWh) const;

    /**
     * @brief Esporta statistiche in CSV
     */
    bool exportStatsToCsv(const QString& filename);

    // UI Controls - Selezione parametri
    QComboBox* m_intervalCombo;
    QDateTimeEdit* m_startDateEdit;
    QDateTimeEdit* m_endDateEdit;
    QPushButton* m_queryButton;
    QPushButton* m_compareButton;
    QPushButton* m_exportButton;

    // Quick range buttons
    QPushButton* m_todayButton;
    QPushButton* m_last7DaysButton;
    QPushButton* m_last30DaysButton;
    QPushButton* m_thisMonthButton;
    QPushButton* m_lastMonthButton;

    // Tabella dati aggregati
    QTableWidget* m_dataTable;

    // Grafico energia
    QCustomPlot* m_energyChart;

    // Labels statistiche generali
    QLabel* m_totalEnergyLabel;
    QLabel* m_avgPowerLabel;
    QLabel* m_peakPowerLabel;
    QLabel* m_totalCostLabel;

    // Labels confronto periodi
    QGroupBox* m_comparisonGroup;
    QLabel* m_comparisonEnergyLabel;
    QLabel* m_comparisonPowerLabel;
    QLabel* m_comparisonCostLabel;
    QLabel* m_comparisonTrendLabel;

    // Configurazione costo energia
    QLabel* m_priceLabel;
    QDoubleSpinBox* m_priceSpinBox;  // Prezzo €/kWh

    // Backend
    DatabaseManager* m_dbManager;

    // Dati correnti
    QVector<AggregatedData> m_currentData;
    AggregationInterval m_currentInterval;

    // Costanti
    static constexpr double DEFAULT_PRICE_PER_KWH = 0.25;  // €0.25/kWh
};

#endif // STATISTICSTAB_H

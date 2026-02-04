#ifndef HISTORYVIEWERTAB_H
#define HISTORYVIEWERTAB_H

#include <QWidget>
#include <QDateTimeEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QTabWidget>
#include "databasemanager.h"
#include "datapoint.h"
#include "shellycsvimporter.h"
#include "qcustomplot.h"

/**
 * @brief Tab per visualizzazione e interrogazione dati storici dal database
 *
 * Funzionalità:
 * - Selezione range temporale (date picker)
 * - Query campioni real-time o dati energia giornaliera
 * - Visualizzazione tabellare con paginazione
 * - Statistiche aggregate (min/max/avg/total)
 * - Export risultati query in CSV
 * - Filtri per fase (A/B/C/All)
 */
class HistoryViewerTab : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryViewerTab(DatabaseManager* dbManager, class SettingsTab* settingsTab, QWidget *parent = nullptr);
    ~HistoryViewerTab();

signals:
    /**
     * @brief Richiesta export dati correntemente visualizzati
     */
    void exportRequested(const QString& filename);

private slots:
    /**
     * @brief Esegue query per campioni real-time
     */
    void onQuerySamplesClicked();

    /**
     * @brief Esegue query per dati energia giornaliera
     */
    void onQueryDailyEnergyClicked();

    /**
     * @brief Esporta dati visualizzati in CSV
     */
    void onExportClicked();

    /**
     * @brief Pulisce la tabella e le statistiche
     */
    void onClearClicked();

    /**
     * @brief Aggiorna statistiche quando cambia selezione fase
     */
    void onPhaseFilterChanged(int index);

private:
    /**
     * @brief Inizializza l'interfaccia utente
     */
    void setupUI();

    /**
     * @brief Popola tabella con campioni real-time
     */
    void displaySamples(const QVector<DataPoint>& samples);

    /**
     * @brief Popola tabella con dati energia giornaliera
     */
    void displayDailyEnergy(const QVector<EnergyDailyData>& energyData);

    /**
     * @brief Calcola e visualizza statistiche per campioni
     */
    void updateSamplesStatistics(const QVector<DataPoint>& samples);

    /**
     * @brief Calcola e visualizza statistiche per energia giornaliera
     */
    void updateDailyEnergyStatistics(const QVector<EnergyDailyData>& energyData);

    /**
     * @brief Esporta campioni in CSV
     */
    bool exportSamplesToCsv(const QString& filename, const QVector<DataPoint>& samples);

    /**
     * @brief Esporta dati energia in CSV
     */
    bool exportDailyEnergyToCsv(const QString& filename, const QVector<EnergyDailyData>& energyData);

    /**
     * @brief Renderizza grafico per campioni real-time
     */
    void renderSamplesChart(const QVector<DataPoint>& samples);

    /**
     * @brief Renderizza grafico per dati energia giornaliera
     */
    void renderDailyEnergyChart(const QVector<EnergyDailyData>& energyData);

    /**
     * @brief Pulisce il grafico
     */
    void clearChart();

    // Database manager (reference, non owned)
    DatabaseManager* m_dbManager;

    // Settings tab (reference, non owned)
    class SettingsTab* m_settingsTab;

    // UI Controls - Query
    QDateTimeEdit* m_startDateEdit;
    QDateTimeEdit* m_endDateEdit;
    QPushButton* m_querySamplesButton;
    QPushButton* m_queryDailyEnergyButton;
    QPushButton* m_exportButton;
    QPushButton* m_clearButton;
    QComboBox* m_phaseFilterCombo;

    // UI Display
    QTabWidget* m_displayTabs;  // Tab per switchare tra tabella e grafico
    QTableWidget* m_dataTable;
    QCustomPlot* m_chartWidget;

    // UI Statistics
    QLabel* m_recordCountLabel;
    QLabel* m_dateRangeLabel;
    QLabel* m_powerStatsLabel;
    QLabel* m_energyStatsLabel;

    // Data cache per export
    QVector<DataPoint> m_cachedSamples;
    QVector<EnergyDailyData> m_cachedDailyEnergy;
    enum DataType { TYPE_NONE, TYPE_SAMPLES, TYPE_DAILY_ENERGY };
    DataType m_currentDataType;

    // Configurazione
    static constexpr int MAX_DISPLAY_ROWS = 1000;  // Limite righe visualizzate
};

#endif // HISTORYVIEWERTAB_H

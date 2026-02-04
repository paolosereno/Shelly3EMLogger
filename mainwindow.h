#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QStatusBar>
#include <QVector>
#include <QTabWidget>
#include "qcustomplot.h"
#include "shellymanager.h"
#include "datapoint.h"
#include "deviceinfotab.h"
#include "settingstab.h"
#include "shellycsvimporter.h"
#include "databasemanager.h"
#include "historyviewertab.h"
#include "alarmmanager.h"
#include "statisticstab.h"

/**
 * @brief Finestra principale dell'applicazione Shelly Logger
 *
 * Gestisce:
 * - Interfaccia utente con controlli e visualizzazione dati
 * - 3 Grafici QCustomPlot (uno per fase) in tabs separati
 * - Cursori interattivi per ispezione dati
 * - Export CSV
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Slots per i pulsanti
    void onConnectClicked();
    void onDisconnectClicked();
    void onExportCsvClicked();
    void onClearDataClicked();
    void onRemoveCursorsClicked();
    void onImportShellyCsvClicked();
    void onExportChartClicked();

    // Slots per dati da ShellyManager
    void onDataReceived(const DataPoint& dataPoint);
    void onDeviceInfoReceived(const DeviceInfo& deviceInfo);
    void onErrorOccurred(const QString& errorMessage);
    void onConnectionStatusChanged(bool connected);

    // Slots per interazione con il grafico
    void onPlotMouseMove(QMouseEvent* event);
    void onPlotMousePress(QMouseEvent* event);
    void onPlotMouseDoubleClick(QMouseEvent* event);

    // Slot per cambio tab
    void onTabChanged(int index);

    // Slot per refresh device info
    void onDeviceInfoRefreshRequested();

    // Slot per Settings
    void onSettingsSaved();
    void onLanguageChanged(const QString& languageCode);
    void onThemeChanged(const QString& theme);

    // Slot per Allarmi
    void onAlarmTriggered(AlarmManager::AlarmType type, const QString& message, double value, double threshold);
    void onAlarmCleared(AlarmManager::AlarmType type);

    // Slot per Help menu
    void about();
    void showLogViewer();

private:
    // Inizializzazione UI
    void setupUI();
    void setupPlot(QCustomPlot* plot);
    void setupConnections();

    // Aggiornamento UI
    void updateCurrentValuesDisplay(const DataPoint& dp);
    void updateStatisticsDisplay();
    void updateStatusBar();
    void updatePlot();

    // Gestione cursori
    void setupCursors(QCustomPlot* plot, QCPItemTracer** tracer, QCPItemText** tracerLabel);
    void updateTracerPosition(double x, QCustomPlot* plot, QCPItemTracer* tracer, QCPItemText* tracerLabel);
    void addFixedCursor(double x);
    void removeAllCursors();
    void updateCursorInfoDisplay();
    DataPoint findNearestDataPoint(double x);

    // Ottieni il grafico attivo corrente
    QCustomPlot* getCurrentPlot();
    int getCurrentPhase(); // 0=A, 1=B, 2=C
    int getCurrentParameter(); // 0=Power, 1=Voltage, 2=Current
    QString getCurrentParameterName(); // "Power", "Voltage", "Current"
    QString getCurrentParameterUnit(); // "W", "V", "A"

    // Export
    bool exportToCsv(const QString& filename);
    bool exportChartToPng(const QString& filename);
    bool exportChartToPdf(const QString& filename);

    // Calcolo statistiche
    void calculateStatistics(double& min, double& max, double& avg);

    // Caricamento/salvataggio impostazioni
    void loadSettings();
    void saveSettings();

    // Gestione tipo grafico (line/bar)
    void updateChartType();

    // Gestione tema
    void applyTheme(const QString& theme);
    void applyThemeToPlot(QCustomPlot* plot, const QString& theme);

    // Widget UI - Controlli
    QLineEdit* m_ipLineEdit;
    QSpinBox* m_intervalSpinBox;
    QPushButton* m_connectButton;
    QPushButton* m_disconnectButton;
    QPushButton* m_exportButton;
    QPushButton* m_clearButton;
    QPushButton* m_removeCursorsButton;
    QPushButton* m_importCsvButton;
    QPushButton* m_exportChartButton;

    // Widget UI - Display valori correnti
    QGroupBox* m_currentDataGroup;
    QLabel* m_powerLabel;
    QLabel* m_voltageLabel;
    QLabel* m_currentLabel;
    QLabel* m_powerFactorLabel;

    // Widget UI - Statistiche
    QLabel* m_minLabel;
    QLabel* m_maxLabel;
    QLabel* m_avgLabel;

    // Widget UI - Info cursori
    QGroupBox* m_cursorsGroup;
    QLabel* m_cursor1Label;
    QLabel* m_cursor2Label;
    QLabel* m_deltaLabel;

    // Widget UI - Total Power
    QLabel* m_totalPowerLabel;

    // Tab widget principale (fasi + Device Info)
    QTabWidget* m_mainTabWidget;

    // Tab widget fasi (Phase A/B/C)
    QTabWidget* m_tabWidget;

    // Sotto-tab widget (parametri) per ogni fase
    QTabWidget* m_paramTabWidgetA;
    QTabWidget* m_paramTabWidgetB;
    QTabWidget* m_paramTabWidgetC;

    // Device Info Tab
    DeviceInfoTab* m_deviceInfoTab;

    // Settings Tab
    SettingsTab* m_settingsTab;

    // History Viewer Tab
    HistoryViewerTab* m_historyViewerTab;

    // Statistics Tab
    StatisticsTab* m_statisticsTab;

    // 9 grafici: 3 fasi × 3 parametri (Power, Voltage, Current)
    QCustomPlot* m_plotPhaseA_Power;
    QCustomPlot* m_plotPhaseA_Voltage;
    QCustomPlot* m_plotPhaseA_Current;

    QCustomPlot* m_plotPhaseB_Power;
    QCustomPlot* m_plotPhaseB_Voltage;
    QCustomPlot* m_plotPhaseB_Current;

    QCustomPlot* m_plotPhaseC_Power;
    QCustomPlot* m_plotPhaseC_Voltage;
    QCustomPlot* m_plotPhaseC_Current;

    // Cursori interattivi per ogni grafico (9 × 2 = 18 cursori)
    QCPItemTracer* m_tracerA_Power;
    QCPItemText* m_tracerLabelA_Power;
    QCPItemTracer* m_tracerA_Voltage;
    QCPItemText* m_tracerLabelA_Voltage;
    QCPItemTracer* m_tracerA_Current;
    QCPItemText* m_tracerLabelA_Current;

    QCPItemTracer* m_tracerB_Power;
    QCPItemText* m_tracerLabelB_Power;
    QCPItemTracer* m_tracerB_Voltage;
    QCPItemText* m_tracerLabelB_Voltage;
    QCPItemTracer* m_tracerB_Current;
    QCPItemText* m_tracerLabelB_Current;

    QCPItemTracer* m_tracerC_Power;
    QCPItemText* m_tracerLabelC_Power;
    QCPItemTracer* m_tracerC_Voltage;
    QCPItemText* m_tracerLabelC_Voltage;
    QCPItemTracer* m_tracerC_Current;
    QCPItemText* m_tracerLabelC_Current;

    struct FixedCursor {
        QCPItemStraightLine* line;        // Linea verticale
        QCPItemTracer* tracer;            // Marker sul punto
        DataPoint dataPoint;              // Dati associati
    };
    QVector<FixedCursor> m_fixedCursors;  // Max 2 cursori fissi
    static const int MAX_FIXED_CURSORS = 2;

    // Backend
    ShellyManager* m_shellyManager;
    DatabaseManager* m_databaseManager;
    AlarmManager* m_alarmManager;

    // Dati acquisiti
    QVector<DataPoint> m_dataPoints;
    DataPoint m_lastDataPoint;

    // Configurazione
    static const int MAX_DATA_POINTS = 10000;      // Limite punti in memoria
    static const int PLOT_TIME_WINDOW = 7200;     // 2 ore in secondi

    // Sistema di allarme sonoro per bassa potenza (una flag per ogni fase)
    bool m_lowPowerAlarmActiveA;
    bool m_lowPowerAlarmActiveB;
    bool m_lowPowerAlarmActiveC;

    // Sistema di allarme per soglia potenza (una flag per ogni fase)
    bool m_powerAlarmActiveA;
    bool m_powerAlarmActiveB;
    bool m_powerAlarmActiveC;
};

#endif // MAINWINDOW_H

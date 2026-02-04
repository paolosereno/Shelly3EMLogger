#include "statisticstab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QHeaderView>
#include <QDateTime>

StatisticsTab::StatisticsTab(DatabaseManager* dbManager, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
    , m_currentInterval(AggregationInterval::Daily)
{
    setupUI();
    setupConnections();
    setupChart();

    // Imposta date predefinite: ultimi 30 giorni
    m_endDateEdit->setDateTime(QDateTime::currentDateTime());
    m_startDateEdit->setDateTime(QDateTime::currentDateTime().addDays(-30));
}

StatisticsTab::~StatisticsTab()
{
}

QCustomPlot* StatisticsTab::getCurrentChart() const
{
    return m_energyChart;
}

void StatisticsTab::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // ========== Gruppo Selezione Parametri ==========
    QGroupBox* selectionGroup = new QGroupBox(tr("Time Range Selection"));
    QGridLayout* selectionLayout = new QGridLayout(selectionGroup);

    // Intervallo aggregazione
    selectionLayout->addWidget(new QLabel(tr("Aggregation:")), 0, 0);
    m_intervalCombo = new QComboBox();
    m_intervalCombo->addItem(tr("Hourly"), static_cast<int>(AggregationInterval::Hourly));
    m_intervalCombo->addItem(tr("Daily"), static_cast<int>(AggregationInterval::Daily));
    m_intervalCombo->addItem(tr("Weekly"), static_cast<int>(AggregationInterval::Weekly));
    m_intervalCombo->addItem(tr("Monthly"), static_cast<int>(AggregationInterval::Monthly));
    m_intervalCombo->setCurrentIndex(1); // Daily default
    selectionLayout->addWidget(m_intervalCombo, 0, 1);

    // Date range
    selectionLayout->addWidget(new QLabel(tr("Start Date:")), 1, 0);
    m_startDateEdit = new QDateTimeEdit();
    m_startDateEdit->setCalendarPopup(true);
    m_startDateEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    selectionLayout->addWidget(m_startDateEdit, 1, 1);

    selectionLayout->addWidget(new QLabel(tr("End Date:")), 2, 0);
    m_endDateEdit = new QDateTimeEdit();
    m_endDateEdit->setCalendarPopup(true);
    m_endDateEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    selectionLayout->addWidget(m_endDateEdit, 2, 1);

    // Pulsanti azione
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_queryButton = new QPushButton(tr("Query Data"));
    m_queryButton->setIcon(QIcon(":/icons/search.png"));
    m_compareButton = new QPushButton(tr("Compare Periods"));
    m_compareButton->setIcon(QIcon(":/icons/compare.png"));
    m_exportButton = new QPushButton(tr("Export CSV"));
    m_exportButton->setIcon(QIcon(":/icons/export.png"));

    buttonLayout->addWidget(m_queryButton);
    buttonLayout->addWidget(m_compareButton);
    buttonLayout->addWidget(m_exportButton);
    buttonLayout->addStretch();

    selectionLayout->addLayout(buttonLayout, 3, 0, 1, 2);

    // Quick range buttons
    QGroupBox* quickRangeGroup = new QGroupBox(tr("Quick Ranges"));
    QHBoxLayout* quickLayout = new QHBoxLayout(quickRangeGroup);

    m_todayButton = new QPushButton(tr("Today"));
    m_last7DaysButton = new QPushButton(tr("Last 7 Days"));
    m_last30DaysButton = new QPushButton(tr("Last 30 Days"));
    m_thisMonthButton = new QPushButton(tr("This Month"));
    m_lastMonthButton = new QPushButton(tr("Last Month"));

    quickLayout->addWidget(m_todayButton);
    quickLayout->addWidget(m_last7DaysButton);
    quickLayout->addWidget(m_last30DaysButton);
    quickLayout->addWidget(m_thisMonthButton);
    quickLayout->addWidget(m_lastMonthButton);

    selectionLayout->addWidget(quickRangeGroup, 4, 0, 1, 2);

    mainLayout->addWidget(selectionGroup);

    // ========== Gruppo Statistiche Generali ==========
    QGroupBox* statsGroup = new QGroupBox(tr("Summary Statistics"));
    QGridLayout* statsLayout = new QGridLayout(statsGroup);

    statsLayout->addWidget(new QLabel(tr("<b>Total Energy:</b>")), 0, 0);
    m_totalEnergyLabel = new QLabel("0.0 kWh");
    m_totalEnergyLabel->setStyleSheet("font-size: 14pt; color: #2196F3;");
    statsLayout->addWidget(m_totalEnergyLabel, 0, 1);

    statsLayout->addWidget(new QLabel(tr("<b>Average Power:</b>")), 0, 2);
    m_avgPowerLabel = new QLabel("0.0 W");
    m_avgPowerLabel->setStyleSheet("font-size: 14pt; color: #4CAF50;");
    statsLayout->addWidget(m_avgPowerLabel, 0, 3);

    statsLayout->addWidget(new QLabel(tr("<b>Peak Power:</b>")), 1, 0);
    m_peakPowerLabel = new QLabel("0.0 W");
    m_peakPowerLabel->setStyleSheet("font-size: 14pt; color: #FF9800;");
    statsLayout->addWidget(m_peakPowerLabel, 1, 1);

    // Configurazione prezzo energia
    statsLayout->addWidget(new QLabel(tr("<b>Price (€/kWh):</b>")), 1, 2);
    m_priceSpinBox = new QDoubleSpinBox();
    m_priceSpinBox->setRange(0.0, 10.0);
    m_priceSpinBox->setSingleStep(0.01);
    m_priceSpinBox->setValue(DEFAULT_PRICE_PER_KWH);
    m_priceSpinBox->setPrefix("€ ");
    m_priceSpinBox->setDecimals(3);
    statsLayout->addWidget(m_priceSpinBox, 1, 3);

    statsLayout->addWidget(new QLabel(tr("<b>Total Cost:</b>")), 2, 0);
    m_totalCostLabel = new QLabel("€ 0.00");
    m_totalCostLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #F44336;");
    statsLayout->addWidget(m_totalCostLabel, 2, 1);

    mainLayout->addWidget(statsGroup);

    // ========== Gruppo Confronto Periodi ==========
    m_comparisonGroup = new QGroupBox(tr("Period Comparison"));
    m_comparisonGroup->setVisible(false); // Nascosto finché non si fa un confronto
    QGridLayout* compLayout = new QGridLayout(m_comparisonGroup);

    compLayout->addWidget(new QLabel(tr("<b>Energy Change:</b>")), 0, 0);
    m_comparisonEnergyLabel = new QLabel("--");
    compLayout->addWidget(m_comparisonEnergyLabel, 0, 1);

    compLayout->addWidget(new QLabel(tr("<b>Power Change:</b>")), 0, 2);
    m_comparisonPowerLabel = new QLabel("--");
    compLayout->addWidget(m_comparisonPowerLabel, 0, 3);

    compLayout->addWidget(new QLabel(tr("<b>Cost Change:</b>")), 1, 0);
    m_comparisonCostLabel = new QLabel("--");
    compLayout->addWidget(m_comparisonCostLabel, 1, 1);

    compLayout->addWidget(new QLabel(tr("<b>Trend:</b>")), 1, 2);
    m_comparisonTrendLabel = new QLabel("--");
    compLayout->addWidget(m_comparisonTrendLabel, 1, 3);

    mainLayout->addWidget(m_comparisonGroup);

    // ========== Grafico Energia ==========
    QGroupBox* chartGroup = new QGroupBox(tr("Energy Consumption Chart"));
    QVBoxLayout* chartLayout = new QVBoxLayout(chartGroup);

    m_energyChart = new QCustomPlot();
    m_energyChart->setMinimumHeight(300);
    chartLayout->addWidget(m_energyChart);

    mainLayout->addWidget(chartGroup);

    // ========== Tabella Dati Dettagliati ==========
    QGroupBox* tableGroup = new QGroupBox(tr("Detailed Data"));
    QVBoxLayout* tableLayout = new QVBoxLayout(tableGroup);

    m_dataTable = new QTableWidget();
    m_dataTable->setColumnCount(8);
    m_dataTable->setHorizontalHeaderLabels({
        tr("Period"),
        tr("Total Energy (kWh)"),
        tr("Avg Power (W)"),
        tr("Peak Power (W)"),
        tr("Phase A (kWh)"),
        tr("Phase B (kWh)"),
        tr("Phase C (kWh)"),
        tr("Samples")
    });
    m_dataTable->horizontalHeader()->setStretchLastSection(true);
    m_dataTable->setAlternatingRowColors(true);
    m_dataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_dataTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    tableLayout->addWidget(m_dataTable);

    mainLayout->addWidget(tableGroup);
}

void StatisticsTab::setupConnections()
{
    connect(m_intervalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StatisticsTab::onIntervalChanged);
    connect(m_queryButton, &QPushButton::clicked, this, &StatisticsTab::onQueryClicked);
    connect(m_compareButton, &QPushButton::clicked, this, &StatisticsTab::onCompareClicked);
    connect(m_exportButton, &QPushButton::clicked, this, &StatisticsTab::onExportStatsClicked);

    // Quick range buttons
    connect(m_todayButton, &QPushButton::clicked, this, &StatisticsTab::onQuickRangeClicked);
    connect(m_last7DaysButton, &QPushButton::clicked, this, &StatisticsTab::onQuickRangeClicked);
    connect(m_last30DaysButton, &QPushButton::clicked, this, &StatisticsTab::onQuickRangeClicked);
    connect(m_thisMonthButton, &QPushButton::clicked, this, &StatisticsTab::onQuickRangeClicked);
    connect(m_lastMonthButton, &QPushButton::clicked, this, &StatisticsTab::onQuickRangeClicked);

    // Aggiorna costo quando cambia il prezzo
    connect(m_priceSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this]() {
        // Ricalcola costo totale
        if (!m_currentData.isEmpty()) {
            double totalEnergy = 0;
            for (const auto& data : m_currentData) {
                totalEnergy += data.totalEnergy_Wh;
            }
            double cost = calculateCost(totalEnergy, m_priceSpinBox->value());
            m_totalCostLabel->setText(QString("€ %1").arg(cost, 0, 'f', 2));
        }
    });
}

void StatisticsTab::setupChart()
{
    // === TEMA SCURO (stesso degli altri grafici) ===
    // Background principale (area esterna)
    m_energyChart->setBackground(QBrush(QColor("#2b2b2b")));

    // Background area grafico (dove vengono disegnati i dati)
    m_energyChart->axisRect()->setBackground(QBrush(QColor("#1e1e1e")));

    // Aggiungi grafico
    m_energyChart->addGraph();
    m_energyChart->graph(0)->setPen(QPen(QColor(33, 150, 243), 2));
    m_energyChart->graph(0)->setBrush(QBrush(QColor(33, 150, 243, 50)));

    // Etichette assi
    m_energyChart->xAxis->setLabel(tr("Time Period"));
    m_energyChart->yAxis->setLabel(tr("Energy (kWh)"));

    // Colori assi e label
    m_energyChart->xAxis->setBasePen(QPen(QColor("#b0b0b0"), 1));
    m_energyChart->yAxis->setBasePen(QPen(QColor("#b0b0b0"), 1));
    m_energyChart->xAxis->setTickPen(QPen(QColor("#b0b0b0"), 1));
    m_energyChart->yAxis->setTickPen(QPen(QColor("#b0b0b0"), 1));
    m_energyChart->xAxis->setSubTickPen(QPen(QColor("#808080"), 1));
    m_energyChart->yAxis->setSubTickPen(QPen(QColor("#808080"), 1));
    m_energyChart->xAxis->setTickLabelColor(QColor("#d0d0d0"));
    m_energyChart->yAxis->setTickLabelColor(QColor("#d0d0d0"));
    m_energyChart->xAxis->setLabelColor(QColor("#e0e0e0"));
    m_energyChart->yAxis->setLabelColor(QColor("#e0e0e0"));

    // Griglia con linee scure ma visibili
    m_energyChart->xAxis->grid()->setVisible(true);
    m_energyChart->yAxis->grid()->setVisible(true);
    m_energyChart->xAxis->grid()->setPen(QPen(QColor("#3a3a3a"), 1, Qt::SolidLine));
    m_energyChart->yAxis->grid()->setPen(QPen(QColor("#3a3a3a"), 1, Qt::SolidLine));
    m_energyChart->xAxis->grid()->setSubGridVisible(true);
    m_energyChart->yAxis->grid()->setSubGridVisible(true);
    m_energyChart->xAxis->grid()->setSubGridPen(QPen(QColor("#2f2f2f"), 1, Qt::DotLine));
    m_energyChart->yAxis->grid()->setSubGridPen(QPen(QColor("#2f2f2f"), 1, Qt::DotLine));

    m_energyChart->xAxis->setTickLabelRotation(45);
    m_energyChart->xAxis->setTicker(QSharedPointer<QCPAxisTickerText>(new QCPAxisTickerText()));

    m_energyChart->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
}

void StatisticsTab::refresh()
{
    // Chiamato quando si passa a questo tab
    // Potremmo ricaricare i dati se necessario
}

void StatisticsTab::onIntervalChanged(int index)
{
    m_currentInterval = static_cast<AggregationInterval>(m_intervalCombo->currentData().toInt());
}

void StatisticsTab::onQueryClicked()
{
    if (!m_dbManager || !m_dbManager->isOpen()) {
        QMessageBox::warning(this, tr("Database Error"),
                           tr("Database is not open. Cannot query data."));
        return;
    }

    QDateTime startTime = m_startDateEdit->dateTime();
    QDateTime endTime = m_endDateEdit->dateTime();

    if (startTime >= endTime) {
        QMessageBox::warning(this, tr("Invalid Range"),
                           tr("Start date must be before end date."));
        return;
    }

    m_currentData.clear();

    bool success = m_dbManager->getAggregatedData(m_currentInterval, startTime, endTime, m_currentData);

    if (!success) {
        QMessageBox::critical(this, tr("Query Error"),
                            tr("Failed to query aggregated data: %1").arg(m_dbManager->lastError()));
        return;
    }

    if (m_currentData.isEmpty()) {
        QMessageBox::information(this, tr("No Data"),
                               tr("No data found in the selected time range."));
        return;
    }

    // Aggiorna visualizzazioni
    updateTable(m_currentData);
    updateChart(m_currentData);

    // Calcola e mostra statistiche generali
    double totalEnergy = 0;
    double totalPowerSum = 0;
    double peakPower = 0;
    int totalSamples = 0;

    for (const auto& data : m_currentData) {
        totalEnergy += data.totalEnergy_Wh;
        totalPowerSum += data.totalPower_avg * data.sampleCount;
        totalSamples += data.sampleCount;
        if (data.totalPower_max > peakPower) {
            peakPower = data.totalPower_max;
        }
    }

    double avgPower = (totalSamples > 0) ? (totalPowerSum / totalSamples) : 0;
    double totalEnergyKWh = totalEnergy / 1000.0;
    double totalCost = calculateCost(totalEnergy, m_priceSpinBox->value());

    m_totalEnergyLabel->setText(formatEnergy(totalEnergy));
    m_avgPowerLabel->setText(formatPower(avgPower));
    m_peakPowerLabel->setText(formatPower(peakPower));
    m_totalCostLabel->setText(QString("€ %1").arg(totalCost, 0, 'f', 2));

    // Nascondi confronto
    m_comparisonGroup->setVisible(false);
}

void StatisticsTab::onCompareClicked()
{
    if (!m_dbManager || !m_dbManager->isOpen()) {
        QMessageBox::warning(this, tr("Database Error"),
                           tr("Database is not open. Cannot compare data."));
        return;
    }

    QDateTime currentStart = m_startDateEdit->dateTime();
    QDateTime currentEnd = m_endDateEdit->dateTime();

    if (currentStart >= currentEnd) {
        QMessageBox::warning(this, tr("Invalid Range"),
                           tr("Start date must be before end date."));
        return;
    }

    // Calcola periodo precedente della stessa durata
    qint64 durationSecs = currentStart.secsTo(currentEnd);
    QDateTime previousEnd = currentStart.addSecs(-1); // 1 secondo prima dell'inizio corrente
    QDateTime previousStart = previousEnd.addSecs(-durationSecs);

    ComparisonStats comparison;
    bool success = m_dbManager->getComparisonStats(m_currentInterval,
                                                   currentStart, currentEnd,
                                                   previousStart, previousEnd,
                                                   comparison);

    if (!success) {
        QMessageBox::critical(this, tr("Comparison Error"),
                            tr("Failed to compare periods: %1").arg(m_dbManager->lastError()));
        return;
    }

    if (!comparison.current.isValid() || !comparison.previous.isValid()) {
        QMessageBox::information(this, tr("No Data"),
                               tr("Insufficient data for comparison."));
        return;
    }

    showComparison(comparison);
}

void StatisticsTab::onExportStatsClicked()
{
    if (m_currentData.isEmpty()) {
        QMessageBox::warning(this, tr("No Data"),
                           tr("No data to export. Please query data first."));
        return;
    }

    QString filename = QFileDialog::getSaveFileName(this,
        tr("Export Statistics"),
        QString("statistics_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        tr("CSV Files (*.csv)"));

    if (filename.isEmpty()) {
        return;
    }

    if (exportStatsToCsv(filename)) {
        QMessageBox::information(this, tr("Export Successful"),
                               tr("Statistics exported to %1").arg(filename));
    } else {
        QMessageBox::critical(this, tr("Export Failed"),
                            tr("Failed to export statistics."));
    }
}

void StatisticsTab::onQuickRangeClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    QDateTime now = QDateTime::currentDateTime();
    QDateTime start, end;

    if (button == m_todayButton) {
        start = QDateTime(now.date(), QTime(0, 0, 0));
        end = now;
    } else if (button == m_last7DaysButton) {
        start = now.addDays(-7);
        end = now;
    } else if (button == m_last30DaysButton) {
        start = now.addDays(-30);
        end = now;
    } else if (button == m_thisMonthButton) {
        start = QDateTime(QDate(now.date().year(), now.date().month(), 1), QTime(0, 0, 0));
        end = now;
    } else if (button == m_lastMonthButton) {
        QDate firstDayThisMonth(now.date().year(), now.date().month(), 1);
        QDate firstDayLastMonth = firstDayThisMonth.addMonths(-1);
        start = QDateTime(firstDayLastMonth, QTime(0, 0, 0));
        end = QDateTime(firstDayThisMonth, QTime(0, 0, 0)).addSecs(-1);
    }

    m_startDateEdit->setDateTime(start);
    m_endDateEdit->setDateTime(end);
}

void StatisticsTab::updateTable(const QVector<AggregatedData>& data)
{
    m_dataTable->setRowCount(data.size());

    for (int i = 0; i < data.size(); ++i) {
        const AggregatedData& aggData = data[i];

        m_dataTable->setItem(i, 0, new QTableWidgetItem(aggData.getIntervalDescription()));
        m_dataTable->setItem(i, 1, new QTableWidgetItem(QString::number(aggData.totalEnergy_kWh, 'f', 3)));
        m_dataTable->setItem(i, 2, new QTableWidgetItem(QString::number(aggData.totalPower_avg, 'f', 1)));
        m_dataTable->setItem(i, 3, new QTableWidgetItem(QString::number(aggData.totalPower_max, 'f', 1)));
        m_dataTable->setItem(i, 4, new QTableWidgetItem(QString::number(aggData.energyA_Wh / 1000.0, 'f', 3)));
        m_dataTable->setItem(i, 5, new QTableWidgetItem(QString::number(aggData.energyB_Wh / 1000.0, 'f', 3)));
        m_dataTable->setItem(i, 6, new QTableWidgetItem(QString::number(aggData.energyC_Wh / 1000.0, 'f', 3)));
        m_dataTable->setItem(i, 7, new QTableWidgetItem(QString::number(aggData.sampleCount)));

        // Allinea numeri a destra
        for (int col = 1; col < 8; ++col) {
            m_dataTable->item(i, col)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        }
    }

    m_dataTable->resizeColumnsToContents();
}

void StatisticsTab::updateChart(const QVector<AggregatedData>& data)
{
    QVector<double> xData, yData;
    QSharedPointer<QCPAxisTickerText> textTicker = qSharedPointerCast<QCPAxisTickerText>(m_energyChart->xAxis->ticker());
    textTicker->clear();

    for (int i = 0; i < data.size(); ++i) {
        const AggregatedData& aggData = data[i];
        xData.append(i);
        yData.append(aggData.totalEnergy_kWh);

        // Etichetta asse X basata sull'intervallo
        QString label;
        switch (aggData.interval) {
            case AggregationInterval::Hourly:
                label = aggData.startTime.toString("MM-dd HH:00");
                break;
            case AggregationInterval::Daily:
                label = aggData.startTime.toString("MM-dd");
                break;
            case AggregationInterval::Weekly:
                label = QString("W%1").arg(aggData.startTime.date().weekNumber());
                break;
            case AggregationInterval::Monthly:
                label = aggData.startTime.toString("yyyy-MM");
                break;
        }
        textTicker->addTick(i, label);
    }

    m_energyChart->graph(0)->setData(xData, yData);
    m_energyChart->rescaleAxes();
    m_energyChart->replot();
}

void StatisticsTab::showComparison(const ComparisonStats& comparison)
{
    // Calcola costi
    double currentCost = calculateCost(comparison.current.totalEnergy_Wh, m_priceSpinBox->value());
    double previousCost = calculateCost(comparison.previous.totalEnergy_Wh, m_priceSpinBox->value());
    double costChange = ((currentCost - previousCost) / previousCost) * 100.0;

    // Formatta con colori
    auto formatChange = [](double percent) -> QString {
        QString color = (percent > 0) ? "#F44336" : "#4CAF50"; // Rosso se aumenta, verde se diminuisce
        QString arrow = (percent > 0) ? "↑" : "↓";
        return QString("<span style='color: %1; font-weight: bold;'>%2 %3%</span>")
            .arg(color)
            .arg(arrow)
            .arg(qAbs(percent), 0, 'f', 1);
    };

    m_comparisonEnergyLabel->setText(formatChange(comparison.energyChange_percent));
    m_comparisonPowerLabel->setText(formatChange(comparison.powerChange_percent));
    m_comparisonCostLabel->setText(formatChange(costChange));
    m_comparisonTrendLabel->setText(comparison.getTrendDescription(comparison.energyTrend));

    m_comparisonGroup->setVisible(true);
}

QString StatisticsTab::formatEnergy(double wh) const
{
    if (wh >= 1000000.0) {
        return QString("%1 MWh").arg(wh / 1000000.0, 0, 'f', 2);
    } else if (wh >= 1000.0) {
        return QString("%1 kWh").arg(wh / 1000.0, 0, 'f', 2);
    } else {
        return QString("%1 Wh").arg(wh, 0, 'f', 1);
    }
}

QString StatisticsTab::formatPower(double w) const
{
    if (w >= 1000000.0) {
        return QString("%1 MW").arg(w / 1000000.0, 0, 'f', 2);
    } else if (w >= 1000.0) {
        return QString("%1 kW").arg(w / 1000.0, 0, 'f', 2);
    } else {
        return QString("%1 W").arg(w, 0, 'f', 1);
    }
}

double StatisticsTab::calculateCost(double energyWh, double pricePerKWh) const
{
    return (energyWh / 1000.0) * pricePerKWh;
}

bool StatisticsTab::exportStatsToCsv(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);

    // Header
    out << "Period,Total Energy (kWh),Avg Power (W),Peak Power (W),"
        << "Phase A (kWh),Phase B (kWh),Phase C (kWh),Samples\n";

    // Data rows
    for (const auto& data : m_currentData) {
        out << data.getIntervalDescription() << ","
            << data.totalEnergy_kWh << ","
            << data.totalPower_avg << ","
            << data.totalPower_max << ","
            << (data.energyA_Wh / 1000.0) << ","
            << (data.energyB_Wh / 1000.0) << ","
            << (data.energyC_Wh / 1000.0) << ","
            << data.sampleCount << "\n";
    }

    file.close();
    return true;
}

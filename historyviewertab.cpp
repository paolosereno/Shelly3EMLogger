#include "historyviewertab.h"
#include "settingstab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QDebug>
#include <QTextStream>
#include <QSettings>

HistoryViewerTab::HistoryViewerTab(DatabaseManager* dbManager, SettingsTab* settingsTab, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
    , m_settingsTab(settingsTab)
    , m_currentDataType(TYPE_NONE)
{
    setupUI();
}

HistoryViewerTab::~HistoryViewerTab()
{
}

void HistoryViewerTab::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // === GRUPPO QUERY ===
    QGroupBox* queryGroup = new QGroupBox(tr("Query Parameters"), this);
    QVBoxLayout* queryLayout = new QVBoxLayout(queryGroup);

    // Date range selector
    QHBoxLayout* dateLayout = new QHBoxLayout();
    dateLayout->addWidget(new QLabel(tr("From:"), this));

    m_startDateEdit = new QDateTimeEdit(QDateTime::currentDateTime().addDays(-7), this);
    m_startDateEdit->setCalendarPopup(true);
    m_startDateEdit->setDisplayFormat("dd/MM/yyyy HH:mm");
    dateLayout->addWidget(m_startDateEdit);

    dateLayout->addWidget(new QLabel(tr("To:"), this));

    m_endDateEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    m_endDateEdit->setCalendarPopup(true);
    m_endDateEdit->setDisplayFormat("dd/MM/yyyy HH:mm");
    dateLayout->addWidget(m_endDateEdit);

    dateLayout->addStretch();
    queryLayout->addLayout(dateLayout);

    // Phase filter
    QHBoxLayout* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel(tr("Phase Filter:"), this));

    m_phaseFilterCombo = new QComboBox(this);
    m_phaseFilterCombo->addItem(tr("All Phases"));
    m_phaseFilterCombo->addItem(tr("Phase A"));
    m_phaseFilterCombo->addItem(tr("Phase B"));
    m_phaseFilterCombo->addItem(tr("Phase C"));
    filterLayout->addWidget(m_phaseFilterCombo);

    filterLayout->addStretch();
    queryLayout->addLayout(filterLayout);

    // Query buttons
    QHBoxLayout* queryButtonLayout = new QHBoxLayout();

    m_querySamplesButton = new QPushButton(tr("Query Real-Time Samples"), this);
    m_queryDailyEnergyButton = new QPushButton(tr("Query Daily Energy"), this);
    m_exportButton = new QPushButton(tr("Export to CSV"), this);
    m_clearButton = new QPushButton(tr("Clear"), this);

    m_exportButton->setEnabled(false);

    queryButtonLayout->addWidget(m_querySamplesButton);
    queryButtonLayout->addWidget(m_queryDailyEnergyButton);
    queryButtonLayout->addWidget(m_exportButton);
    queryButtonLayout->addWidget(m_clearButton);
    queryButtonLayout->addStretch();

    queryLayout->addLayout(queryButtonLayout);

    mainLayout->addWidget(queryGroup, 0);  // 0 = non si espande

    // === GRUPPO STATISTICHE ===
    QGroupBox* statsGroup = new QGroupBox(tr("Statistics"), this);
    QGridLayout* statsLayout = new QGridLayout(statsGroup);

    m_recordCountLabel = new QLabel(tr("Records: ---"), this);
    m_dateRangeLabel = new QLabel(tr("Date Range: ---"), this);
    m_powerStatsLabel = new QLabel(tr("Power Stats: ---"), this);
    m_energyStatsLabel = new QLabel(tr("Energy Stats: ---"), this);

    QFont boldFont;
    boldFont.setBold(true);
    m_recordCountLabel->setFont(boldFont);

    statsLayout->addWidget(m_recordCountLabel, 0, 0);
    statsLayout->addWidget(m_dateRangeLabel, 0, 1);
    statsLayout->addWidget(m_powerStatsLabel, 1, 0);
    statsLayout->addWidget(m_energyStatsLabel, 1, 1);

    mainLayout->addWidget(statsGroup, 0);  // 0 = non si espande

    // === TAB WIDGET PER VISUALIZZAZIONE (Tabella + Grafico) ===
    m_displayTabs = new QTabWidget(this);

    // Tab Tabella
    m_dataTable = new QTableWidget(this);
    m_dataTable->setAlternatingRowColors(true);
    m_dataTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_dataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_dataTable->setSortingEnabled(true);
    m_dataTable->horizontalHeader()->setStretchLastSection(true);
    m_dataTable->verticalHeader()->setDefaultSectionSize(25); // Altezza righe più compatta

    // Abilita scrollbar sempre visibili quando necessario
    m_dataTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_dataTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_dataTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_dataTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Tab Grafico
    m_chartWidget = new QCustomPlot(this);
    m_chartWidget->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    m_chartWidget->axisRect()->setupFullAxesBox();
    m_chartWidget->xAxis->setLabel(tr("Time"));
    m_chartWidget->yAxis->setLabel(tr("Value"));

    // === TEMA SCURO (come Charts tab) ===
    // Background principale
    m_chartWidget->setBackground(QBrush(QColor("#2b2b2b")));
    // Background area grafico
    m_chartWidget->axisRect()->setBackground(QBrush(QColor("#1e1e1e")));

    // Colori assi e label
    m_chartWidget->xAxis->setBasePen(QPen(QColor("#b0b0b0"), 1));
    m_chartWidget->yAxis->setBasePen(QPen(QColor("#b0b0b0"), 1));
    m_chartWidget->xAxis->setTickPen(QPen(QColor("#b0b0b0"), 1));
    m_chartWidget->yAxis->setTickPen(QPen(QColor("#b0b0b0"), 1));
    m_chartWidget->xAxis->setSubTickPen(QPen(QColor("#808080"), 1));
    m_chartWidget->yAxis->setSubTickPen(QPen(QColor("#808080"), 1));
    m_chartWidget->xAxis->setTickLabelColor(QColor("#d0d0d0"));
    m_chartWidget->yAxis->setTickLabelColor(QColor("#d0d0d0"));
    m_chartWidget->xAxis->setLabelColor(QColor("#e0e0e0"));
    m_chartWidget->yAxis->setLabelColor(QColor("#e0e0e0"));

    // Griglia con linee scure ma visibili
    m_chartWidget->xAxis->grid()->setVisible(true);
    m_chartWidget->yAxis->grid()->setVisible(true);
    m_chartWidget->xAxis->grid()->setPen(QPen(QColor("#3a3a3a"), 1, Qt::SolidLine));
    m_chartWidget->yAxis->grid()->setPen(QPen(QColor("#3a3a3a"), 1, Qt::SolidLine));
    m_chartWidget->xAxis->grid()->setSubGridVisible(true);
    m_chartWidget->yAxis->grid()->setSubGridVisible(true);
    m_chartWidget->xAxis->grid()->setSubGridPen(QPen(QColor("#2f2f2f"), 1, Qt::DotLine));
    m_chartWidget->yAxis->grid()->setSubGridPen(QPen(QColor("#2f2f2f"), 1, Qt::DotLine));

    // Aggiungi tabs
    m_displayTabs->addTab(m_dataTable, tr("Table"));
    m_displayTabs->addTab(m_chartWidget, tr("Chart"));

    mainLayout->addWidget(m_displayTabs, 1);  // stretch factor = 1 per espandersi

    // Connessioni
    connect(m_querySamplesButton, &QPushButton::clicked, this, &HistoryViewerTab::onQuerySamplesClicked);
    connect(m_queryDailyEnergyButton, &QPushButton::clicked, this, &HistoryViewerTab::onQueryDailyEnergyClicked);
    connect(m_exportButton, &QPushButton::clicked, this, &HistoryViewerTab::onExportClicked);
    connect(m_clearButton, &QPushButton::clicked, this, &HistoryViewerTab::onClearClicked);
    connect(m_phaseFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HistoryViewerTab::onPhaseFilterChanged);
}

void HistoryViewerTab::onQuerySamplesClicked()
{
    if (!m_dbManager || !m_dbManager->isOpen()) {
        QMessageBox::warning(this, tr("Database Error"), tr("Database not available"));
        return;
    }

    QDateTime startTime = m_startDateEdit->dateTime();
    QDateTime endTime = m_endDateEdit->dateTime();

    if (startTime >= endTime) {
        QMessageBox::warning(this, tr("Invalid Range"), tr("Start time must be before end time"));
        return;
    }

    qint64 startSecs = startTime.toSecsSinceEpoch();
    qint64 endSecs = endTime.toSecsSinceEpoch();

    QVector<DataPoint> samples;
    if (!m_dbManager->getSamples(startSecs, endSecs, samples)) {
        QMessageBox::critical(this, tr("Query Failed"),
                            tr("Failed to query samples:\n%1").arg(m_dbManager->lastError()));
        return;
    }

    if (samples.isEmpty()) {
        QMessageBox::information(this, tr("No Data"), tr("No samples found in the specified range"));
        onClearClicked();
        return;
    }

    // Cache per export
    m_cachedSamples = samples;
    m_currentDataType = TYPE_SAMPLES;

    displaySamples(samples);
    updateSamplesStatistics(samples);
    renderSamplesChart(samples);

    m_exportButton->setEnabled(true);

    qDebug() << "HistoryViewerTab: Queried" << samples.size() << "samples";
}

void HistoryViewerTab::onQueryDailyEnergyClicked()
{
    if (!m_dbManager || !m_dbManager->isOpen()) {
        QMessageBox::warning(this, tr("Database Error"), tr("Database not available"));
        return;
    }

    QDate startDate = m_startDateEdit->date();
    QDate endDate = m_endDateEdit->date();

    if (startDate >= endDate) {
        QMessageBox::warning(this, tr("Invalid Range"), tr("Start date must be before end date"));
        return;
    }

    QVector<EnergyDailyData> energyData;
    if (!m_dbManager->getDailyEnergy(startDate, endDate, energyData)) {
        QMessageBox::critical(this, tr("Query Failed"),
                            tr("Failed to query daily energy:\n%1").arg(m_dbManager->lastError()));
        return;
    }

    if (energyData.isEmpty()) {
        QMessageBox::information(this, tr("No Data"), tr("No daily energy data found in the specified range"));
        onClearClicked();
        return;
    }

    // Cache per export
    m_cachedDailyEnergy = energyData;
    m_currentDataType = TYPE_DAILY_ENERGY;

    displayDailyEnergy(energyData);
    updateDailyEnergyStatistics(energyData);
    renderDailyEnergyChart(energyData);

    m_exportButton->setEnabled(true);

    qDebug() << "HistoryViewerTab: Queried" << energyData.size() << "daily energy records";
}

void HistoryViewerTab::onExportClicked()
{
    QString defaultFileName;
    if (m_currentDataType == TYPE_SAMPLES) {
        defaultFileName = QString("history_samples_%1.csv")
                          .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmmss"));
    } else if (m_currentDataType == TYPE_DAILY_ENERGY) {
        defaultFileName = QString("history_daily_energy_%1.csv")
                          .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmmss"));
    } else {
        return;
    }

    QString filename = QFileDialog::getSaveFileName(this,
                                                     tr("Export History Data"),
                                                     defaultFileName,
                                                     tr("CSV Files (*.csv);;All Files (*)"));

    if (filename.isEmpty()) {
        return;
    }

    bool success = false;
    if (m_currentDataType == TYPE_SAMPLES) {
        success = exportSamplesToCsv(filename, m_cachedSamples);
    } else if (m_currentDataType == TYPE_DAILY_ENERGY) {
        success = exportDailyEnergyToCsv(filename, m_cachedDailyEnergy);
    }

    if (success) {
        QMessageBox::information(this, tr("Export Successful"),
                               tr("Data exported successfully to:\n%1").arg(filename));
    } else {
        QMessageBox::critical(this, tr("Export Failed"),
                            tr("Failed to export data to file"));
    }
}

void HistoryViewerTab::onClearClicked()
{
    m_dataTable->clear();
    m_dataTable->setRowCount(0);
    m_dataTable->setColumnCount(0);

    clearChart();

    m_cachedSamples.clear();
    m_cachedDailyEnergy.clear();
    m_currentDataType = TYPE_NONE;

    m_recordCountLabel->setText(tr("Records: ---"));
    m_dateRangeLabel->setText(tr("Date Range: ---"));
    m_powerStatsLabel->setText(tr("Power Stats: ---"));
    m_energyStatsLabel->setText(tr("Energy Stats: ---"));

    m_exportButton->setEnabled(false);
}

void HistoryViewerTab::onPhaseFilterChanged(int index)
{
    Q_UNUSED(index);

    // Ricalcola statistiche e aggiorna grafico con nuovo filtro
    if (m_currentDataType == TYPE_SAMPLES && !m_cachedSamples.isEmpty()) {
        updateSamplesStatistics(m_cachedSamples);
        renderSamplesChart(m_cachedSamples);
    } else if (m_currentDataType == TYPE_DAILY_ENERGY && !m_cachedDailyEnergy.isEmpty()) {
        updateDailyEnergyStatistics(m_cachedDailyEnergy);
        renderDailyEnergyChart(m_cachedDailyEnergy);
    }
}

void HistoryViewerTab::displaySamples(const QVector<DataPoint>& samples)
{
    m_dataTable->clear();
    m_dataTable->setColumnCount(14);

    QStringList headers;
    headers << tr("Timestamp") << tr("Date") << tr("Time")
            << tr("Power A (W)") << tr("Voltage A (V)") << tr("Current A (A)") << tr("PF A")
            << tr("Power B (W)") << tr("Voltage B (V)") << tr("Current B (A)") << tr("PF B")
            << tr("Power C (W)") << tr("Voltage C (V)") << tr("Current C (A)") << tr("PF C");

    m_dataTable->setHorizontalHeaderLabels(headers);

    // Limita numero di righe visualizzate
    int rowCount = qMin(samples.size(), MAX_DISPLAY_ROWS);
    m_dataTable->setRowCount(rowCount);

    for (int i = 0; i < rowCount; ++i) {
        const DataPoint& dp = samples[i];

        m_dataTable->setItem(i, 0, new QTableWidgetItem(dp.timestamp.toString("yyyy-MM-dd HH:mm:ss")));
        m_dataTable->setItem(i, 1, new QTableWidgetItem(dp.timestamp.toString("yyyy-MM-dd")));
        m_dataTable->setItem(i, 2, new QTableWidgetItem(dp.timestamp.toString("HH:mm:ss")));

        m_dataTable->setItem(i, 3, new QTableWidgetItem(QString::number(dp.powerA, 'f', 2)));
        m_dataTable->setItem(i, 4, new QTableWidgetItem(QString::number(dp.voltageA, 'f', 2)));
        m_dataTable->setItem(i, 5, new QTableWidgetItem(QString::number(dp.currentA, 'f', 3)));
        m_dataTable->setItem(i, 6, new QTableWidgetItem(QString::number(dp.powerFactorA, 'f', 3)));

        m_dataTable->setItem(i, 7, new QTableWidgetItem(QString::number(dp.powerB, 'f', 2)));
        m_dataTable->setItem(i, 8, new QTableWidgetItem(QString::number(dp.voltageB, 'f', 2)));
        m_dataTable->setItem(i, 9, new QTableWidgetItem(QString::number(dp.currentB, 'f', 3)));
        m_dataTable->setItem(i, 10, new QTableWidgetItem(QString::number(dp.powerFactorB, 'f', 3)));

        m_dataTable->setItem(i, 11, new QTableWidgetItem(QString::number(dp.powerC, 'f', 2)));
        m_dataTable->setItem(i, 12, new QTableWidgetItem(QString::number(dp.voltageC, 'f', 2)));
        m_dataTable->setItem(i, 13, new QTableWidgetItem(QString::number(dp.currentC, 'f', 3)));
        m_dataTable->setItem(i, 14, new QTableWidgetItem(QString::number(dp.powerFactorC, 'f', 3)));
    }

    m_dataTable->resizeColumnsToContents();

    if (samples.size() > MAX_DISPLAY_ROWS) {
        QMessageBox::information(this, tr("Display Limit"),
                               tr("Showing first %1 of %2 records.\nAll data is available for export.")
                               .arg(MAX_DISPLAY_ROWS).arg(samples.size()));
    }
}

void HistoryViewerTab::displayDailyEnergy(const QVector<EnergyDailyData>& energyData)
{
    m_dataTable->clear();
    m_dataTable->setColumnCount(9);

    QStringList headers;
    headers << tr("Date")
            << tr("Energy A (Wh)") << tr("Energy B (Wh)") << tr("Energy C (Wh)") << tr("Total Energy (Wh)")
            << tr("Returned A (Wh)") << tr("Returned B (Wh)") << tr("Returned C (Wh)") << tr("Total Returned (Wh)");

    m_dataTable->setHorizontalHeaderLabels(headers);

    m_dataTable->setRowCount(energyData.size());

    for (int i = 0; i < energyData.size(); ++i) {
        const EnergyDailyData& data = energyData[i];

        m_dataTable->setItem(i, 0, new QTableWidgetItem(data.date.toString("dd/MM/yyyy")));
        m_dataTable->setItem(i, 1, new QTableWidgetItem(QString::number(data.energyA_Wh, 'f', 2)));
        m_dataTable->setItem(i, 2, new QTableWidgetItem(QString::number(data.energyB_Wh, 'f', 2)));
        m_dataTable->setItem(i, 3, new QTableWidgetItem(QString::number(data.energyC_Wh, 'f', 2)));
        m_dataTable->setItem(i, 4, new QTableWidgetItem(QString::number(data.totalEnergy_Wh, 'f', 2)));
        m_dataTable->setItem(i, 5, new QTableWidgetItem(QString::number(data.returnedA_Wh, 'f', 2)));
        m_dataTable->setItem(i, 6, new QTableWidgetItem(QString::number(data.returnedB_Wh, 'f', 2)));
        m_dataTable->setItem(i, 7, new QTableWidgetItem(QString::number(data.returnedC_Wh, 'f', 2)));
        m_dataTable->setItem(i, 8, new QTableWidgetItem(QString::number(data.totalReturned_Wh, 'f', 2)));
    }

    m_dataTable->resizeColumnsToContents();
}

void HistoryViewerTab::updateSamplesStatistics(const QVector<DataPoint>& samples)
{
    if (samples.isEmpty()) {
        return;
    }

    m_recordCountLabel->setText(tr("Records: %1").arg(samples.size()));

    QDateTime firstTime = samples.first().timestamp;
    QDateTime lastTime = samples.last().timestamp;
    m_dateRangeLabel->setText(tr("Date Range: %1 to %2")
                             .arg(firstTime.toString("dd/MM/yyyy HH:mm"))
                             .arg(lastTime.toString("dd/MM/yyyy HH:mm")));

    // Calcola statistiche potenza in base al filtro fase
    int phaseFilter = m_phaseFilterCombo->currentIndex(); // 0=All, 1=A, 2=B, 3=C

    double minPower = std::numeric_limits<double>::max();
    double maxPower = std::numeric_limits<double>::lowest();
    double sumPower = 0.0;

    for (const DataPoint& dp : samples) {
        double power = 0.0;

        if (phaseFilter == 0) {
            // All phases - total power
            power = dp.powerA + dp.powerB + dp.powerC;
        } else if (phaseFilter == 1) {
            power = dp.powerA;
        } else if (phaseFilter == 2) {
            power = dp.powerB;
        } else if (phaseFilter == 3) {
            power = dp.powerC;
        }

        if (power < minPower) minPower = power;
        if (power > maxPower) maxPower = power;
        sumPower += power;
    }

    double avgPower = sumPower / samples.size();

    QString phaseLabel;
    if (phaseFilter == 0) phaseLabel = tr("Total");
    else if (phaseFilter == 1) phaseLabel = tr("Phase A");
    else if (phaseFilter == 2) phaseLabel = tr("Phase B");
    else phaseLabel = tr("Phase C");

    m_powerStatsLabel->setText(tr("%1 Power - Min: %2 W | Max: %3 W | Avg: %4 W")
                               .arg(phaseLabel)
                               .arg(minPower, 0, 'f', 0)
                               .arg(maxPower, 0, 'f', 0)
                               .arg(avgPower, 0, 'f', 0));

    m_energyStatsLabel->setText(tr("---"));
}

void HistoryViewerTab::updateDailyEnergyStatistics(const QVector<EnergyDailyData>& energyData)
{
    if (energyData.isEmpty()) {
        return;
    }

    m_recordCountLabel->setText(tr("Records: %1").arg(energyData.size()));

    QDate firstDate = energyData.first().date;
    QDate lastDate = energyData.last().date;
    m_dateRangeLabel->setText(tr("Date Range: %1 to %2")
                             .arg(firstDate.toString("dd/MM/yyyy"))
                             .arg(lastDate.toString("dd/MM/yyyy")));

    // Calcola totali energia
    int phaseFilter = m_phaseFilterCombo->currentIndex(); // 0=All, 1=A, 2=B, 3=C

    double totalConsumed = 0.0;
    double totalReturned = 0.0;

    for (const EnergyDailyData& data : energyData) {
        if (phaseFilter == 0) {
            totalConsumed += data.totalEnergy_Wh;
            totalReturned += data.totalReturned_Wh;
        } else if (phaseFilter == 1) {
            totalConsumed += data.energyA_Wh;
            totalReturned += data.returnedA_Wh;
        } else if (phaseFilter == 2) {
            totalConsumed += data.energyB_Wh;
            totalReturned += data.returnedB_Wh;
        } else if (phaseFilter == 3) {
            totalConsumed += data.energyC_Wh;
            totalReturned += data.returnedC_Wh;
        }
    }

    QString phaseLabel;
    if (phaseFilter == 0) phaseLabel = tr("Total");
    else if (phaseFilter == 1) phaseLabel = tr("Phase A");
    else if (phaseFilter == 2) phaseLabel = tr("Phase B");
    else phaseLabel = tr("Phase C");

    m_powerStatsLabel->setText(tr("---"));

    m_energyStatsLabel->setText(tr("%1 Energy - Consumed: %2 kWh | Returned: %3 kWh | Net: %4 kWh")
                               .arg(phaseLabel)
                               .arg(totalConsumed / 1000.0, 0, 'f', 2)
                               .arg(totalReturned / 1000.0, 0, 'f', 2)
                               .arg((totalConsumed - totalReturned) / 1000.0, 0, 'f', 2));
}

bool HistoryViewerTab::exportSamplesToCsv(const QString& filename, const QVector<DataPoint>& samples)
{
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "HistoryViewerTab: Unable to open file for writing:" << filename;
        return false;
    }

    QTextStream out(&file);

    // Header
    out << "Timestamp,Date,Time,"
        << "PowerA(W),VoltageA(V),CurrentA(A),PFA,"
        << "PowerB(W),VoltageB(V),CurrentB(A),PFB,"
        << "PowerC(W),VoltageC(V),CurrentC(A),PFC,"
        << "TotalPower(W)\n";

    // Data
    for (const DataPoint& dp : samples) {
        out << dp.timestamp.toString("yyyy-MM-dd HH:mm:ss") << ","
            << dp.timestamp.toString("yyyy-MM-dd") << ","
            << dp.timestamp.toString("HH:mm:ss") << ","
            << QString::number(dp.powerA, 'f', 2) << ","
            << QString::number(dp.voltageA, 'f', 2) << ","
            << QString::number(dp.currentA, 'f', 3) << ","
            << QString::number(dp.powerFactorA, 'f', 3) << ","
            << QString::number(dp.powerB, 'f', 2) << ","
            << QString::number(dp.voltageB, 'f', 2) << ","
            << QString::number(dp.currentB, 'f', 3) << ","
            << QString::number(dp.powerFactorB, 'f', 3) << ","
            << QString::number(dp.powerC, 'f', 2) << ","
            << QString::number(dp.voltageC, 'f', 2) << ","
            << QString::number(dp.currentC, 'f', 3) << ","
            << QString::number(dp.powerFactorC, 'f', 3) << ","
            << QString::number(dp.getTotalPower(), 'f', 2) << "\n";
    }

    file.close();

    qDebug() << "HistoryViewerTab: Exported" << samples.size() << "samples to" << filename;
    return true;
}

bool HistoryViewerTab::exportDailyEnergyToCsv(const QString& filename, const QVector<EnergyDailyData>& energyData)
{
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "HistoryViewerTab: Unable to open file for writing:" << filename;
        return false;
    }

    QTextStream out(&file);

    // Header
    out << "Date,"
        << "EnergyA(Wh),EnergyB(Wh),EnergyC(Wh),TotalEnergy(Wh),"
        << "ReturnedA(Wh),ReturnedB(Wh),ReturnedC(Wh),TotalReturned(Wh)\n";

    // Data
    for (const EnergyDailyData& data : energyData) {
        out << data.date.toString("yyyy-MM-dd") << ","
            << QString::number(data.energyA_Wh, 'f', 2) << ","
            << QString::number(data.energyB_Wh, 'f', 2) << ","
            << QString::number(data.energyC_Wh, 'f', 2) << ","
            << QString::number(data.totalEnergy_Wh, 'f', 2) << ","
            << QString::number(data.returnedA_Wh, 'f', 2) << ","
            << QString::number(data.returnedB_Wh, 'f', 2) << ","
            << QString::number(data.returnedC_Wh, 'f', 2) << ","
            << QString::number(data.totalReturned_Wh, 'f', 2) << "\n";
    }

    file.close();

    qDebug() << "HistoryViewerTab: Exported" << energyData.size() << "daily energy records to" << filename;
    return true;
}

void HistoryViewerTab::renderSamplesChart(const QVector<DataPoint>& samples)
{
    if (samples.isEmpty()) {
        clearChart();
        return;
    }

    m_chartWidget->clearGraphs();
    m_chartWidget->clearPlottables();  // Rimuove anche QCPBars

    // Leggi impostazioni tipo grafico
    QString chartType = "line";  // Default
    if (m_settingsTab) {
        chartType = m_settingsTab->getHistorySamplesChartType();
    }
    bool useBarChart = (chartType == "bar");

    // Ottieni filtro fase
    int phaseFilter = m_phaseFilterCombo->currentIndex(); // 0=All, 1=A, 2=B, 3=C

    // Prepara dati per asse X (timestamp)
    QVector<double> timeData;
    timeData.reserve(samples.size());

    for (const DataPoint& dp : samples) {
        timeData.append(dp.timestamp.toSecsSinceEpoch());
    }

    // Crea grafici per potenza
    if (phaseFilter == 0) {
        // Mostra tutte le fasi
        QVector<double> powerA, powerB, powerC;
        powerA.reserve(samples.size());
        powerB.reserve(samples.size());
        powerC.reserve(samples.size());

        for (const DataPoint& dp : samples) {
            powerA.append(dp.powerA);
            powerB.append(dp.powerB);
            powerC.append(dp.powerC);
        }

        if (useBarChart) {
            // GRAFICI A BARRE AFFIANCATE
            QCPBars* barsA = new QCPBars(m_chartWidget->xAxis, m_chartWidget->yAxis);
            QCPBars* barsB = new QCPBars(m_chartWidget->xAxis, m_chartWidget->yAxis);
            QCPBars* barsC = new QCPBars(m_chartWidget->xAxis, m_chartWidget->yAxis);

            barsA->setName(tr("Phase A"));
            barsB->setName(tr("Phase B"));
            barsC->setName(tr("Phase C"));

            barsA->setPen(QPen(QColor("#4fc3f7")));
            barsA->setBrush(QColor("#4fc3f7"));
            barsB->setPen(QPen(QColor("#ffb74d")));
            barsB->setBrush(QColor("#ffb74d"));
            barsC->setPen(QPen(QColor("#81c784")));
            barsC->setBrush(QColor("#81c784"));

            // Larghezza barre proporzionale all'intervallo tra campioni
            double barWidth = 30;  // 30 secondi di default
            if (samples.size() > 1) {
                double avgInterval = (samples.last().timestamp.toSecsSinceEpoch() -
                                     samples.first().timestamp.toSecsSinceEpoch()) / samples.size();
                barWidth = avgInterval * 0.25;  // 25% dell'intervallo medio
            }
            barsA->setWidth(barWidth);
            barsB->setWidth(barWidth);
            barsC->setWidth(barWidth);

            // Posiziona barre affiancate
            barsB->moveAbove(barsA);
            barsC->moveAbove(barsB);

            barsA->setData(timeData, powerA);
            barsB->setData(timeData, powerB);
            barsC->setData(timeData, powerC);
        } else {
            // GRAFICI A LINEE
            m_chartWidget->addGraph();
            m_chartWidget->graph(0)->setData(timeData, powerA);
            m_chartWidget->graph(0)->setName(tr("Phase A"));
            m_chartWidget->graph(0)->setPen(QPen(QColor("#4fc3f7"), 2));
            m_chartWidget->graph(0)->setBrush(QBrush(QColor(79, 195, 247, 50)));  // Semi-trasparente

            m_chartWidget->addGraph();
            m_chartWidget->graph(1)->setData(timeData, powerB);
            m_chartWidget->graph(1)->setName(tr("Phase B"));
            m_chartWidget->graph(1)->setPen(QPen(QColor("#ffb74d"), 2));
            m_chartWidget->graph(1)->setBrush(QBrush(QColor(255, 183, 77, 50)));  // Semi-trasparente

            m_chartWidget->addGraph();
            m_chartWidget->graph(2)->setData(timeData, powerC);
            m_chartWidget->graph(2)->setName(tr("Phase C"));
            m_chartWidget->graph(2)->setPen(QPen(QColor("#81c784"), 2));
            m_chartWidget->graph(2)->setBrush(QBrush(QColor(129, 199, 132, 50)));  // Semi-trasparente
        }

        m_chartWidget->yAxis->setLabel(tr("Power (W)"));
    } else {
        // Mostra singola fase
        QVector<double> power;
        power.reserve(samples.size());

        for (const DataPoint& dp : samples) {
            if (phaseFilter == 1) {
                power.append(dp.powerA);
            } else if (phaseFilter == 2) {
                power.append(dp.powerB);
            } else {
                power.append(dp.powerC);
            }
        }

        QString phaseName;
        QColor phaseColor;
        if (phaseFilter == 1) {
            phaseName = tr("Phase A");
            phaseColor = QColor("#4fc3f7");  // Celeste
        } else if (phaseFilter == 2) {
            phaseName = tr("Phase B");
            phaseColor = QColor("#ffb74d");  // Arancione
        } else {
            phaseName = tr("Phase C");
            phaseColor = QColor("#81c784");  // Verde
        }

        if (useBarChart) {
            // GRAFICO A BARRE SINGOLO
            QCPBars* bars = new QCPBars(m_chartWidget->xAxis, m_chartWidget->yAxis);
            bars->setName(phaseName);
            bars->setPen(QPen(phaseColor));
            bars->setBrush(phaseColor);

            // Larghezza barre proporzionale all'intervallo tra campioni
            double barWidth = 30;  // 30 secondi di default
            if (samples.size() > 1) {
                double avgInterval = (samples.last().timestamp.toSecsSinceEpoch() -
                                     samples.first().timestamp.toSecsSinceEpoch()) / samples.size();
                barWidth = avgInterval * 0.6;  // 60% dell'intervallo medio
            }
            bars->setWidth(barWidth);
            bars->setData(timeData, power);
        } else {
            // GRAFICO A LINEE
            m_chartWidget->addGraph();
            m_chartWidget->graph(0)->setData(timeData, power);
            m_chartWidget->graph(0)->setName(phaseName);
            m_chartWidget->graph(0)->setPen(QPen(phaseColor, 2));
            // Area colorata semi-trasparente sotto la linea
            QColor brushColor = phaseColor;
            brushColor.setAlpha(50);
            m_chartWidget->graph(0)->setBrush(QBrush(brushColor));
        }

        m_chartWidget->yAxis->setLabel(tr("Power (W) - %1").arg(phaseName));
    }

    // Configura assi
    QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
    dateTicker->setDateTimeFormat("dd/MM\nHH:mm");
    m_chartWidget->xAxis->setTicker(dateTicker);
    m_chartWidget->xAxis->setLabel(tr("Time"));

    // Abilita legenda
    m_chartWidget->legend->setVisible(phaseFilter == 0);
    m_chartWidget->legend->setFont(QFont("Helvetica", 9));
    m_chartWidget->legend->setBrush(QBrush(QColor(255, 255, 255, 200)));
    m_chartWidget->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);

    // Auto-scale
    m_chartWidget->rescaleAxes();
    m_chartWidget->replot();

    qDebug() << "HistoryViewerTab: Rendered samples chart with" << samples.size() << "points";
}

void HistoryViewerTab::renderDailyEnergyChart(const QVector<EnergyDailyData>& energyData)
{
    if (energyData.isEmpty()) {
        clearChart();
        return;
    }

    m_chartWidget->clearGraphs();
    m_chartWidget->clearPlottables();  // Rimuove anche QCPBars

    // Leggi impostazioni tipo grafico
    QString chartType = "bar";  // Default per daily energy
    if (m_settingsTab) {
        chartType = m_settingsTab->getHistoryDailyEnergyChartType();
    }
    bool useBarChart = (chartType == "bar");

    // Ottieni filtro fase
    int phaseFilter = m_phaseFilterCombo->currentIndex(); // 0=All, 1=A, 2=B, 3=C

    // Prepara dati per asse X (date)
    QVector<double> dateData;
    dateData.reserve(energyData.size());

    for (const EnergyDailyData& data : energyData) {
        // Converti QDate in QDateTime (mezzanotte del giorno)
        dateData.append(data.date.startOfDay().toSecsSinceEpoch());
    }

    // Crea grafici per energia consumata
    if (phaseFilter == 0) {
        // Mostra tutte le fasi
        QVector<double> energyA, energyB, energyC;
        energyA.reserve(energyData.size());
        energyB.reserve(energyData.size());
        energyC.reserve(energyData.size());

        for (const EnergyDailyData& data : energyData) {
            energyA.append(data.energyA_Wh / 1000.0);  // Converti in kWh
            energyB.append(data.energyB_Wh / 1000.0);
            energyC.append(data.energyC_Wh / 1000.0);
        }

        if (useBarChart) {
            // GRAFICI A BARRE AFFIANCATE
            QCPBars* barsA = new QCPBars(m_chartWidget->xAxis, m_chartWidget->yAxis);
            QCPBars* barsB = new QCPBars(m_chartWidget->xAxis, m_chartWidget->yAxis);
            QCPBars* barsC = new QCPBars(m_chartWidget->xAxis, m_chartWidget->yAxis);

            barsA->setName(tr("Phase A"));
            barsB->setName(tr("Phase B"));
            barsC->setName(tr("Phase C"));

            barsA->setPen(QPen(QColor("#4fc3f7")));
            barsA->setBrush(QColor("#4fc3f7"));
            barsB->setPen(QPen(QColor("#ffb74d")));
            barsB->setBrush(QColor("#ffb74d"));
            barsC->setPen(QPen(QColor("#81c784")));
            barsC->setBrush(QColor("#81c784"));

            // Larghezza barre
            double barWidth = 86400 * 0.25;  // 25% di un giorno in secondi
            barsA->setWidth(barWidth);
            barsB->setWidth(barWidth);
            barsC->setWidth(barWidth);

            // Posiziona barre affiancate
            barsB->moveAbove(barsA);  // Barra B sopra A (affiancata)
            barsC->moveAbove(barsB);  // Barra C sopra B (affiancata)

            barsA->setData(dateData, energyA);
            barsB->setData(dateData, energyB);
            barsC->setData(dateData, energyC);
        } else {
            // GRAFICI A LINEE
            m_chartWidget->addGraph();
            m_chartWidget->graph(0)->setData(dateData, energyA);
            m_chartWidget->graph(0)->setName(tr("Phase A"));
            m_chartWidget->graph(0)->setPen(QPen(QColor("#4fc3f7"), 2));
            m_chartWidget->graph(0)->setLineStyle(QCPGraph::lsLine);
            m_chartWidget->graph(0)->setBrush(QBrush(QColor(79, 195, 247, 50)));  // Semi-trasparente

            m_chartWidget->addGraph();
            m_chartWidget->graph(1)->setData(dateData, energyB);
            m_chartWidget->graph(1)->setName(tr("Phase B"));
            m_chartWidget->graph(1)->setPen(QPen(QColor("#ffb74d"), 2));
            m_chartWidget->graph(1)->setLineStyle(QCPGraph::lsLine);
            m_chartWidget->graph(1)->setBrush(QBrush(QColor(255, 183, 77, 50)));  // Semi-trasparente

            m_chartWidget->addGraph();
            m_chartWidget->graph(2)->setData(dateData, energyC);
            m_chartWidget->graph(2)->setName(tr("Phase C"));
            m_chartWidget->graph(2)->setPen(QPen(QColor("#81c784"), 2));
            m_chartWidget->graph(2)->setLineStyle(QCPGraph::lsLine);
            m_chartWidget->graph(2)->setBrush(QBrush(QColor(129, 199, 132, 50)));  // Semi-trasparente
        }

        m_chartWidget->yAxis->setLabel(tr("Daily Energy (kWh)"));
    } else {
        // Mostra singola fase
        QVector<double> energy;
        energy.reserve(energyData.size());

        for (const EnergyDailyData& data : energyData) {
            if (phaseFilter == 1) {
                energy.append(data.energyA_Wh / 1000.0);
            } else if (phaseFilter == 2) {
                energy.append(data.energyB_Wh / 1000.0);
            } else {
                energy.append(data.energyC_Wh / 1000.0);
            }
        }

        QString phaseName;
        QColor phaseColor;
        if (phaseFilter == 1) {
            phaseName = tr("Phase A");
            phaseColor = QColor("#4fc3f7");  // Celeste
        } else if (phaseFilter == 2) {
            phaseName = tr("Phase B");
            phaseColor = QColor("#ffb74d");  // Arancione
        } else {
            phaseName = tr("Phase C");
            phaseColor = QColor("#81c784");  // Verde
        }

        if (useBarChart) {
            // GRAFICO A BARRE SINGOLO
            QCPBars* bars = new QCPBars(m_chartWidget->xAxis, m_chartWidget->yAxis);
            bars->setName(phaseName);
            bars->setPen(QPen(phaseColor));
            bars->setBrush(phaseColor);
            bars->setWidth(86400 * 0.6);  // 60% di un giorno in secondi
            bars->setData(dateData, energy);
        } else {
            // GRAFICO A LINEE
            m_chartWidget->addGraph();
            m_chartWidget->graph(0)->setData(dateData, energy);
            m_chartWidget->graph(0)->setLineStyle(QCPGraph::lsLine);
            m_chartWidget->graph(0)->setName(phaseName);
            m_chartWidget->graph(0)->setPen(QPen(phaseColor, 2));
            // Area colorata semi-trasparente sotto la linea
            QColor brushColor = phaseColor;
            brushColor.setAlpha(50);
            m_chartWidget->graph(0)->setBrush(QBrush(brushColor));
        }

        m_chartWidget->yAxis->setLabel(tr("Daily Energy (kWh) - %1").arg(phaseName));
    }

    // Configura assi
    QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
    dateTicker->setDateTimeFormat("dd/MM/yyyy");
    m_chartWidget->xAxis->setTicker(dateTicker);
    m_chartWidget->xAxis->setLabel(tr("Date"));

    // Abilita legenda
    m_chartWidget->legend->setVisible(phaseFilter == 0);
    m_chartWidget->legend->setFont(QFont("Helvetica", 9));
    m_chartWidget->legend->setBrush(QBrush(QColor(255, 255, 255, 200)));
    m_chartWidget->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);

    // Auto-scale
    m_chartWidget->rescaleAxes();
    m_chartWidget->replot();

    qDebug() << "HistoryViewerTab: Rendered daily energy chart with" << energyData.size() << "points";
}

void HistoryViewerTab::clearChart()
{
    m_chartWidget->clearGraphs();
    m_chartWidget->clearPlottables();
    m_chartWidget->replot();
}

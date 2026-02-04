#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QTextStream>
#include <QDebug>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolTip>
#include <QInputDialog>
#include "logviewerdialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_mainTabWidget(nullptr)
    , m_tabWidget(nullptr)
    , m_paramTabWidgetA(nullptr)
    , m_paramTabWidgetB(nullptr)
    , m_paramTabWidgetC(nullptr)
    , m_deviceInfoTab(nullptr)
    , m_settingsTab(nullptr)
    , m_historyViewerTab(nullptr)
    , m_statisticsTab(nullptr)
    , m_plotPhaseA_Power(nullptr)
    , m_plotPhaseA_Voltage(nullptr)
    , m_plotPhaseA_Current(nullptr)
    , m_plotPhaseB_Power(nullptr)
    , m_plotPhaseB_Voltage(nullptr)
    , m_plotPhaseB_Current(nullptr)
    , m_plotPhaseC_Power(nullptr)
    , m_plotPhaseC_Voltage(nullptr)
    , m_plotPhaseC_Current(nullptr)
    , m_shellyManager(new ShellyManager(this))
    , m_databaseManager(new DatabaseManager(this))
    , m_alarmManager(new AlarmManager(this))
    , m_lowPowerAlarmActiveA(false)
    , m_lowPowerAlarmActiveB(false)
    , m_lowPowerAlarmActiveC(false)
    , m_powerAlarmActiveA(false)
    , m_powerAlarmActiveB(false)
    , m_powerAlarmActiveC(false)
{
    // Inizializza database
    if (!m_databaseManager->initialize("shelly_history.db")) {
        QMessageBox::warning(this, tr("Database Error"),
                           tr("Failed to initialize database:\n%1\n\nData will not be persisted.")
                           .arg(m_databaseManager->lastError()));
    } else {
        qDebug() << "MainWindow: Database initialized - Samples:" << m_databaseManager->getSampleCount()
                 << "Daily records:" << m_databaseManager->getDailyEnergyCount();
    }

    setupUI();
    setupConnections();

    // Imposta dimensioni finestra e limiti
    setMinimumSize(900, 550);
    setWindowTitle(tr("Shelly 3EM - 3 Phase Monitor"));

    // Imposta icona finestra
    setWindowIcon(QIcon(":/resources/icons/icon.png"));

    // Carica impostazioni (inclusa geometria finestra)
    loadSettings();

    // Applica il tipo di grafico salvato nelle impostazioni
    updateChartType();

    // Applica il tema salvato nelle impostazioni
    QString theme = m_settingsTab->getCurrentTheme();
    applyTheme(theme);
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::setupUI()
{
    // Widget centrale
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // === GRUPPO CONNESSIONE ===
    QGroupBox* connectionGroup = new QGroupBox(tr("Connection"), this);
    QHBoxLayout* connLayout = new QHBoxLayout(connectionGroup);

    QLabel* ipLabel = new QLabel(tr("IP:"), this);
    m_ipLineEdit = new QLineEdit("192.168.1.100", this);
    m_ipLineEdit->setMaximumWidth(150);

    m_connectButton = new QPushButton(tr("Connect"), this);
    m_disconnectButton = new QPushButton(tr("Disconnect"), this);
    m_disconnectButton->setEnabled(false);

    QLabel* intervalLabel = new QLabel(tr("Interval:"), this);
    m_intervalSpinBox = new QSpinBox(this);
    m_intervalSpinBox->setRange(5, 300);
    m_intervalSpinBox->setValue(60);
    m_intervalSpinBox->setSuffix(tr(" seconds"));
    m_intervalSpinBox->setMaximumWidth(120);

    connLayout->addWidget(ipLabel);
    connLayout->addWidget(m_ipLineEdit);
    connLayout->addWidget(m_connectButton);
    connLayout->addWidget(m_disconnectButton);
    connLayout->addSpacing(20);
    connLayout->addWidget(intervalLabel);
    connLayout->addWidget(m_intervalSpinBox);
    connLayout->addStretch();

    mainLayout->addWidget(connectionGroup, 0);  // 0 = non si espande

    // === GRUPPO DATI ATTUALI ===
    m_currentDataGroup = new QGroupBox(tr("Current Data"), this);
    QGridLayout* currentLayout = new QGridLayout(m_currentDataGroup);

    m_powerLabel = new QLabel(tr("Power: --- W"), this);
    m_voltageLabel = new QLabel(tr("Voltage: --- V"), this);
    m_currentLabel = new QLabel(tr("Current: --- A"), this);
    m_powerFactorLabel = new QLabel(tr("PF: ---"), this);
    m_totalPowerLabel = new QLabel(tr("Total Power: --- W"), this);

    m_minLabel = new QLabel(tr("Min: --- W"), this);
    m_maxLabel = new QLabel(tr("Max: --- W"), this);
    m_avgLabel = new QLabel(tr("Avg: --- W"), this);

    QFont boldFont;
    boldFont.setBold(true);
    m_powerLabel->setFont(boldFont);
    m_totalPowerLabel->setFont(boldFont);

    currentLayout->addWidget(m_powerLabel, 0, 0);
    currentLayout->addWidget(m_voltageLabel, 0, 1);
    currentLayout->addWidget(m_currentLabel, 1, 0);
    currentLayout->addWidget(m_powerFactorLabel, 1, 1);
    currentLayout->addWidget(m_minLabel, 0, 2);
    currentLayout->addWidget(m_maxLabel, 0, 3);
    currentLayout->addWidget(m_avgLabel, 1, 2);
    currentLayout->addWidget(m_totalPowerLabel, 1, 3);

    mainLayout->addWidget(m_currentDataGroup, 0);  // 0 = non si espande

    // === TAB PRINCIPALE: Charts + Device Info ===
    m_mainTabWidget = new QTabWidget(this);

    // === TAB 1: GRAFICI CON TAB E SOTTO-TAB ===
    QWidget* chartsWidget = new QWidget(this);
    QVBoxLayout* chartsLayout = new QVBoxLayout(chartsWidget);

    // Crea QTabWidget per le fasi con tab a destra
    m_tabWidget = new QTabWidget(chartsWidget);
    m_tabWidget->setTabPosition(QTabWidget::East);  // Tab Phase A/B/C a destra

    // === FASE A ===
    m_paramTabWidgetA = new QTabWidget(this);
    m_plotPhaseA_Power = new QCustomPlot(this);
    m_plotPhaseA_Voltage = new QCustomPlot(this);
    m_plotPhaseA_Current = new QCustomPlot(this);
    m_paramTabWidgetA->addTab(m_plotPhaseA_Power, tr("Power"));
    m_paramTabWidgetA->addTab(m_plotPhaseA_Voltage, tr("Voltage"));
    m_paramTabWidgetA->addTab(m_plotPhaseA_Current, tr("Current"));

    // === FASE B ===
    m_paramTabWidgetB = new QTabWidget(this);
    m_plotPhaseB_Power = new QCustomPlot(this);
    m_plotPhaseB_Voltage = new QCustomPlot(this);
    m_plotPhaseB_Current = new QCustomPlot(this);
    m_paramTabWidgetB->addTab(m_plotPhaseB_Power, tr("Power"));
    m_paramTabWidgetB->addTab(m_plotPhaseB_Voltage, tr("Voltage"));
    m_paramTabWidgetB->addTab(m_plotPhaseB_Current, tr("Current"));

    // === FASE C ===
    m_paramTabWidgetC = new QTabWidget(this);
    m_plotPhaseC_Power = new QCustomPlot(this);
    m_plotPhaseC_Voltage = new QCustomPlot(this);
    m_plotPhaseC_Current = new QCustomPlot(this);
    m_paramTabWidgetC->addTab(m_plotPhaseC_Power, tr("Power"));
    m_paramTabWidgetC->addTab(m_plotPhaseC_Voltage, tr("Voltage"));
    m_paramTabWidgetC->addTab(m_plotPhaseC_Current, tr("Current"));

    // Aggiungi sotto-tab ai tab principali
    m_tabWidget->addTab(m_paramTabWidgetA, tr("Phase A"));
    m_tabWidget->addTab(m_paramTabWidgetB, tr("Phase B"));
    m_tabWidget->addTab(m_paramTabWidgetC, tr("Phase C"));

    chartsLayout->addWidget(m_tabWidget);

    // === TAB 2: DEVICE INFO ===
    m_deviceInfoTab = new DeviceInfoTab(this);

    // === TAB 3: SETTINGS ===
    m_settingsTab = new SettingsTab(this);
    m_settingsTab->setDatabaseManager(m_databaseManager);

    // === TAB 4: HISTORY VIEWER ===
    m_historyViewerTab = new HistoryViewerTab(m_databaseManager, m_settingsTab, this);

    // === TAB 5: STATISTICS ===
    m_statisticsTab = new StatisticsTab(m_databaseManager, this);

    // Aggiungi i tab al widget principale
    m_mainTabWidget->addTab(chartsWidget, tr("Charts"));
    m_mainTabWidget->addTab(m_deviceInfoTab, tr("Device Info"));
    m_mainTabWidget->addTab(m_settingsTab, tr("Settings"));
    m_mainTabWidget->addTab(m_historyViewerTab, tr("History"));
    m_mainTabWidget->addTab(m_statisticsTab, tr("Statistics"));

    mainLayout->addWidget(m_mainTabWidget, 1);  // 1 = si espande per riempire lo spazio

    // Setup dei 9 grafici (3 fasi × 3 parametri)
    setupPlot(m_plotPhaseA_Power);
    setupPlot(m_plotPhaseA_Voltage);
    setupPlot(m_plotPhaseA_Current);
    setupPlot(m_plotPhaseB_Power);
    setupPlot(m_plotPhaseB_Voltage);
    setupPlot(m_plotPhaseB_Current);
    setupPlot(m_plotPhaseC_Power);
    setupPlot(m_plotPhaseC_Voltage);
    setupPlot(m_plotPhaseC_Current);

    // Setup cursori per ogni grafico
    setupCursors(m_plotPhaseA_Power, &m_tracerA_Power, &m_tracerLabelA_Power);
    setupCursors(m_plotPhaseA_Voltage, &m_tracerA_Voltage, &m_tracerLabelA_Voltage);
    setupCursors(m_plotPhaseA_Current, &m_tracerA_Current, &m_tracerLabelA_Current);
    setupCursors(m_plotPhaseB_Power, &m_tracerB_Power, &m_tracerLabelB_Power);
    setupCursors(m_plotPhaseB_Voltage, &m_tracerB_Voltage, &m_tracerLabelB_Voltage);
    setupCursors(m_plotPhaseB_Current, &m_tracerB_Current, &m_tracerLabelB_Current);
    setupCursors(m_plotPhaseC_Power, &m_tracerC_Power, &m_tracerLabelC_Power);
    setupCursors(m_plotPhaseC_Voltage, &m_tracerC_Voltage, &m_tracerLabelC_Voltage);
    setupCursors(m_plotPhaseC_Current, &m_tracerC_Current, &m_tracerLabelC_Current);

    // === GRUPPO CURSORI ATTIVI ===
    m_cursorsGroup = new QGroupBox(tr("Active Cursors"), this);
    QVBoxLayout* cursorsLayout = new QVBoxLayout(m_cursorsGroup);

    m_cursor1Label = new QLabel(tr("Cursor 1: None"), this);
    m_cursor2Label = new QLabel(tr("Cursor 2: None"), this);
    m_deltaLabel = new QLabel(tr("Delta: ---"), this);
    m_removeCursorsButton = new QPushButton(tr("Remove Cursors"), this);
    m_removeCursorsButton->setEnabled(false);

    cursorsLayout->addWidget(m_cursor1Label);
    cursorsLayout->addWidget(m_cursor2Label);
    cursorsLayout->addWidget(m_deltaLabel);
    cursorsLayout->addWidget(m_removeCursorsButton);

    mainLayout->addWidget(m_cursorsGroup, 0);  // 0 = non si espande

    // === PULSANTI AZIONE ===
    QHBoxLayout* actionLayout = new QHBoxLayout();

    m_exportButton = new QPushButton(tr("Export CSV"), this);
    m_clearButton = new QPushButton(tr("Clear Data"), this);
    m_importCsvButton = new QPushButton(tr("Import Shelly CSV"), this);
    m_exportChartButton = new QPushButton(tr("Export Chart"), this);

    actionLayout->addWidget(m_exportButton);
    actionLayout->addWidget(m_clearButton);
    actionLayout->addWidget(m_importCsvButton);
    actionLayout->addWidget(m_exportChartButton);
    actionLayout->addStretch();

    mainLayout->addLayout(actionLayout);

    // === STATUS BAR ===
    statusBar()->showMessage(tr("Disconnected"));

    // === MENU BAR ===

    // File menu
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    QAction* preferencesAction = new QAction(tr("&Preferences"), this);
    preferencesAction->setStatusTip(tr("Open application preferences"));
    preferencesAction->setShortcut(QKeySequence::Preferences);  // Ctrl+, on most platforms
    connect(preferencesAction, &QAction::triggered, [this]() {
        // Attiva il tab Settings
        m_mainTabWidget->setCurrentWidget(m_settingsTab);
    });
    fileMenu->addAction(preferencesAction);

    fileMenu->addSeparator();

    QAction* exitAction = new QAction(tr("E&xit"), this);
    exitAction->setStatusTip(tr("Exit the application"));
    exitAction->setShortcut(QKeySequence::Quit);  // Ctrl+Q on most platforms
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);

    // Help menu
    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));

    // View Logs action
    QAction* viewLogsAction = new QAction(tr("View &Logs"), this);
    viewLogsAction->setStatusTip(tr("View application logs"));
    viewLogsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    connect(viewLogsAction, &QAction::triggered, this, &MainWindow::showLogViewer);
    helpMenu->addAction(viewLogsAction);

    // About Shelly Logger action
    QAction* aboutAction = new QAction(tr("&About Shelly Logger"), this);
    aboutAction->setStatusTip(tr("About this application"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::about);
    helpMenu->addAction(aboutAction);

    // About Qt action
    QAction* aboutQtAction = new QAction(tr("About &Qt"), this);
    aboutQtAction->setStatusTip(tr("About Qt Framework"));
    connect(aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
    helpMenu->addAction(aboutQtAction);
}

void MainWindow::setupPlot(QCustomPlot* plot)
{
    // Determina quale fase e parametro è questo grafico
    QString phaseName;
    QColor lineColor;
    QString parameterName;
    QString yAxisLabel;

    // Identificazione fase
    if (plot == m_plotPhaseA_Power || plot == m_plotPhaseA_Voltage || plot == m_plotPhaseA_Current) {
        phaseName = tr("Phase A");
        lineColor = QColor("#4fc3f7");  // Celeste chiaro
    } else if (plot == m_plotPhaseB_Power || plot == m_plotPhaseB_Voltage || plot == m_plotPhaseB_Current) {
        phaseName = tr("Phase B");
        lineColor = QColor("#ffb74d");  // Arancione
    } else {
        phaseName = tr("Phase C");
        lineColor = QColor("#81c784");  // Verde
    }

    // Identificazione parametro
    if (plot == m_plotPhaseA_Power || plot == m_plotPhaseB_Power || plot == m_plotPhaseC_Power) {
        parameterName = tr("Power");
        yAxisLabel = tr("Power (W)");
    } else if (plot == m_plotPhaseA_Voltage || plot == m_plotPhaseB_Voltage || plot == m_plotPhaseC_Voltage) {
        parameterName = tr("Voltage");
        yAxisLabel = tr("Voltage (V)");
    } else {
        parameterName = tr("Current");
        yAxisLabel = tr("Current (A)");
    }

    // Configurazione grafico
    plot->setInteraction(QCP::iRangeDrag, true);
    plot->setInteraction(QCP::iRangeZoom, true);
    plot->axisRect()->setRangeDrag(Qt::Horizontal | Qt::Vertical);
    plot->axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical);

    // Abilita mouse tracking per tooltip
    plot->setMouseTracking(true);

    // === TEMA COLORI (Grigio Scuro Moderno) ===
    // Background principale (area esterna)
    plot->setBackground(QBrush(QColor("#2b2b2b")));

    // Background area grafico (dove vengono disegnati i dati)
    plot->axisRect()->setBackground(QBrush(QColor("#1e1e1e")));

    // Aggiungi il grafico principale con colore specifico per fase
    plot->addGraph();
    plot->graph(0)->setPen(QPen(lineColor, 2.5));
    // Area colorata semi-trasparente sotto la linea (come Statistics tab)
    QColor brushColor = lineColor;
    brushColor.setAlpha(50);  // Semi-trasparente
    plot->graph(0)->setBrush(QBrush(brushColor));
    plot->graph(0)->setName(phaseName + " - " + parameterName);

    // Configurazione assi
    QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
    dateTicker->setDateTimeFormat("HH:mm:ss");
    plot->xAxis->setTicker(dateTicker);
    plot->xAxis->setLabel(tr("Time"));
    plot->yAxis->setLabel(yAxisLabel);

    // Colori assi e label
    plot->xAxis->setBasePen(QPen(QColor("#b0b0b0"), 1));
    plot->yAxis->setBasePen(QPen(QColor("#b0b0b0"), 1));
    plot->xAxis->setTickPen(QPen(QColor("#b0b0b0"), 1));
    plot->yAxis->setTickPen(QPen(QColor("#b0b0b0"), 1));
    plot->xAxis->setSubTickPen(QPen(QColor("#808080"), 1));
    plot->yAxis->setSubTickPen(QPen(QColor("#808080"), 1));
    plot->xAxis->setTickLabelColor(QColor("#d0d0d0"));
    plot->yAxis->setTickLabelColor(QColor("#d0d0d0"));
    plot->xAxis->setLabelColor(QColor("#e0e0e0"));
    plot->yAxis->setLabelColor(QColor("#e0e0e0"));

    // Titolo con colore chiaro
    plot->plotLayout()->insertRow(0);
    QCPTextElement* title = new QCPTextElement(plot, phaseName + " - " + parameterName, QFont("sans", 12, QFont::Bold));
    title->setTextColor(QColor("#e0e0e0"));
    plot->plotLayout()->addElement(0, 0, title);

    // Griglia con linee scure ma visibili
    plot->xAxis->grid()->setVisible(true);
    plot->yAxis->grid()->setVisible(true);
    plot->xAxis->grid()->setPen(QPen(QColor("#3a3a3a"), 1, Qt::SolidLine));
    plot->yAxis->grid()->setPen(QPen(QColor("#3a3a3a"), 1, Qt::SolidLine));
    plot->xAxis->grid()->setSubGridVisible(true);
    plot->yAxis->grid()->setSubGridVisible(true);
    plot->xAxis->grid()->setSubGridPen(QPen(QColor("#2f2f2f"), 1, Qt::DotLine));
    plot->yAxis->grid()->setSubGridPen(QPen(QColor("#2f2f2f"), 1, Qt::DotLine));

    // Leggenda con sfondo scuro
    plot->legend->setVisible(true);
    plot->legend->setBrush(QBrush(QColor("#2b2b2b")));
    plot->legend->setBorderPen(QPen(QColor("#606060")));
    plot->legend->setTextColor(QColor("#d0d0d0"));
    plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);

    plot->replot();
}

void MainWindow::setupCursors(QCustomPlot* plot, QCPItemTracer** tracer, QCPItemText** tracerLabel)
{
    // Tracer mobile (segue il mouse) - giallo vivace per tema scuro
    *tracer = new QCPItemTracer(plot);
    (*tracer)->setGraph(plot->graph(0));
    (*tracer)->setInterpolating(true);
    (*tracer)->setStyle(QCPItemTracer::tsCircle);
    (*tracer)->setPen(QPen(QColor("#ffd54f"), 2));  // Giallo ambra
    (*tracer)->setBrush(QColor("#ffd54f"));
    (*tracer)->setSize(8);
    (*tracer)->setVisible(false);

    // Label per il tracer - tema scuro
    *tracerLabel = new QCPItemText(plot);
    (*tracerLabel)->setPositionAlignment(Qt::AlignTop | Qt::AlignHCenter);
    (*tracerLabel)->position->setType(QCPItemPosition::ptPlotCoords);
    (*tracerLabel)->setPen(QPen(QColor("#606060")));
    (*tracerLabel)->setBrush(QBrush(QColor(40, 40, 40, 230)));  // Sfondo scuro semi-trasparente
    (*tracerLabel)->setColor(QColor("#e0e0e0"));  // Testo chiaro
    (*tracerLabel)->setPadding(QMargins(8, 6, 8, 6));
    (*tracerLabel)->setFont(QFont("sans", 9));
    (*tracerLabel)->setVisible(false);
}

void MainWindow::setupConnections()
{
    // Pulsanti
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_exportButton, &QPushButton::clicked, this, &MainWindow::onExportCsvClicked);
    connect(m_clearButton, &QPushButton::clicked, this, &MainWindow::onClearDataClicked);
    connect(m_removeCursorsButton, &QPushButton::clicked, this, &MainWindow::onRemoveCursorsClicked);
    connect(m_importCsvButton, &QPushButton::clicked, this, &MainWindow::onImportShellyCsvClicked);
    connect(m_exportChartButton, &QPushButton::clicked, this, &MainWindow::onExportChartClicked);

    // ShellyManager
    connect(m_shellyManager, &ShellyManager::dataReceived, this, &MainWindow::onDataReceived);
    connect(m_shellyManager, &ShellyManager::deviceInfoReceived, this, &MainWindow::onDeviceInfoReceived);
    connect(m_shellyManager, &ShellyManager::errorOccurred, this, &MainWindow::onErrorOccurred);
    connect(m_shellyManager, &ShellyManager::connectionStatusChanged, this, &MainWindow::onConnectionStatusChanged);

    // Device Info Tab
    connect(m_deviceInfoTab, &DeviceInfoTab::refreshRequested, this, &MainWindow::onDeviceInfoRefreshRequested);

    // Settings Tab
    connect(m_settingsTab, &SettingsTab::settingsSaved, this, &MainWindow::onSettingsSaved);
    connect(m_settingsTab, &SettingsTab::languageChanged, this, &MainWindow::onLanguageChanged);
    connect(m_settingsTab, &SettingsTab::themeChanged, this, &MainWindow::onThemeChanged);

    // AlarmManager
    connect(m_alarmManager, &AlarmManager::alarmTriggered, this, &MainWindow::onAlarmTriggered);
    connect(m_alarmManager, &AlarmManager::alarmCleared, this, &MainWindow::onAlarmCleared);

    // Eventi mouse sui 9 grafici
    // Fase A
    connect(m_plotPhaseA_Power, &QCustomPlot::mouseMove, this, &MainWindow::onPlotMouseMove);
    connect(m_plotPhaseA_Power, &QCustomPlot::mousePress, this, &MainWindow::onPlotMousePress);
    connect(m_plotPhaseA_Power, &QCustomPlot::mouseDoubleClick, this, &MainWindow::onPlotMouseDoubleClick);
    connect(m_plotPhaseA_Voltage, &QCustomPlot::mouseMove, this, &MainWindow::onPlotMouseMove);
    connect(m_plotPhaseA_Voltage, &QCustomPlot::mousePress, this, &MainWindow::onPlotMousePress);
    connect(m_plotPhaseA_Voltage, &QCustomPlot::mouseDoubleClick, this, &MainWindow::onPlotMouseDoubleClick);
    connect(m_plotPhaseA_Current, &QCustomPlot::mouseMove, this, &MainWindow::onPlotMouseMove);
    connect(m_plotPhaseA_Current, &QCustomPlot::mousePress, this, &MainWindow::onPlotMousePress);
    connect(m_plotPhaseA_Current, &QCustomPlot::mouseDoubleClick, this, &MainWindow::onPlotMouseDoubleClick);

    // Fase B
    connect(m_plotPhaseB_Power, &QCustomPlot::mouseMove, this, &MainWindow::onPlotMouseMove);
    connect(m_plotPhaseB_Power, &QCustomPlot::mousePress, this, &MainWindow::onPlotMousePress);
    connect(m_plotPhaseB_Power, &QCustomPlot::mouseDoubleClick, this, &MainWindow::onPlotMouseDoubleClick);
    connect(m_plotPhaseB_Voltage, &QCustomPlot::mouseMove, this, &MainWindow::onPlotMouseMove);
    connect(m_plotPhaseB_Voltage, &QCustomPlot::mousePress, this, &MainWindow::onPlotMousePress);
    connect(m_plotPhaseB_Voltage, &QCustomPlot::mouseDoubleClick, this, &MainWindow::onPlotMouseDoubleClick);
    connect(m_plotPhaseB_Current, &QCustomPlot::mouseMove, this, &MainWindow::onPlotMouseMove);
    connect(m_plotPhaseB_Current, &QCustomPlot::mousePress, this, &MainWindow::onPlotMousePress);
    connect(m_plotPhaseB_Current, &QCustomPlot::mouseDoubleClick, this, &MainWindow::onPlotMouseDoubleClick);

    // Fase C
    connect(m_plotPhaseC_Power, &QCustomPlot::mouseMove, this, &MainWindow::onPlotMouseMove);
    connect(m_plotPhaseC_Power, &QCustomPlot::mousePress, this, &MainWindow::onPlotMousePress);
    connect(m_plotPhaseC_Power, &QCustomPlot::mouseDoubleClick, this, &MainWindow::onPlotMouseDoubleClick);
    connect(m_plotPhaseC_Voltage, &QCustomPlot::mouseMove, this, &MainWindow::onPlotMouseMove);
    connect(m_plotPhaseC_Voltage, &QCustomPlot::mousePress, this, &MainWindow::onPlotMousePress);
    connect(m_plotPhaseC_Voltage, &QCustomPlot::mouseDoubleClick, this, &MainWindow::onPlotMouseDoubleClick);
    connect(m_plotPhaseC_Current, &QCustomPlot::mouseMove, this, &MainWindow::onPlotMouseMove);
    connect(m_plotPhaseC_Current, &QCustomPlot::mousePress, this, &MainWindow::onPlotMousePress);
    connect(m_plotPhaseC_Current, &QCustomPlot::mouseDoubleClick, this, &MainWindow::onPlotMouseDoubleClick);

    // Tab change (fasi e parametri)
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(m_paramTabWidgetA, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(m_paramTabWidgetB, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(m_paramTabWidgetC, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    // Main tab change - show/hide cursors group, current data group, and action buttons
    connect(m_mainTabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        // Show cursors group only for Charts tab (index 0)
        m_cursorsGroup->setVisible(index == 0);
        // Show current data group only for Charts tab (index 0)
        m_currentDataGroup->setVisible(index == 0);
        // Show export CSV button only for Charts tab (index 0)
        m_exportButton->setVisible(index == 0);
        // Show export chart button only for Charts (0) and Statistics (4) tabs
        m_exportChartButton->setVisible(index == 0 || index == 4);
        // Show clear data button only for Charts tab (index 0)
        m_clearButton->setVisible(index == 0);
    });

    // Cambio intervallo polling
    connect(m_intervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int value) {
                if (m_shellyManager->isConnected()) {
                    m_shellyManager->setPollInterval(value);
                }
            });
}

// ========== SLOTS PULSANTI ==========

void MainWindow::onConnectClicked()
{
    QString ip = m_ipLineEdit->text().trimmed();

    // Validazione IP basilare
    if (ip.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please enter a valid IP address"));
        return;
    }

    int interval = m_intervalSpinBox->value();

    m_shellyManager->connectToShelly(ip, interval);
}

void MainWindow::onDisconnectClicked()
{
    m_shellyManager->disconnect();
}

void MainWindow::onExportCsvClicked()
{
    if (m_dataPoints.isEmpty()) {
        QMessageBox::information(this, tr("Export CSV"), tr("No data to export"));
        return;
    }

    QString defaultFileName = QString("phase_a_consumption_%1.csv")
                              .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmmss"));

    QString filename = QFileDialog::getSaveFileName(this,
                                                     tr("Export CSV Data"),
                                                     defaultFileName,
                                                     tr("CSV Files (*.csv);;All Files (*)"));

    if (filename.isEmpty()) {
        return;
    }

    if (exportToCsv(filename)) {
        QMessageBox::information(this, tr("Export CSV"),
                                tr("Data exported successfully to:\n%1").arg(filename));
    } else {
        QMessageBox::critical(this, tr("Export Error"),
                             tr("Unable to write file:\n%1").arg(filename));
    }
}

void MainWindow::onClearDataClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                              tr("Clear Data"),
                                                              tr("Are you sure you want to clear all acquired data?"),
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_dataPoints.clear();
        // Pulisci tutti e 9 i grafici
        m_plotPhaseA_Power->graph(0)->data()->clear();
        m_plotPhaseA_Voltage->graph(0)->data()->clear();
        m_plotPhaseA_Current->graph(0)->data()->clear();
        m_plotPhaseB_Power->graph(0)->data()->clear();
        m_plotPhaseB_Voltage->graph(0)->data()->clear();
        m_plotPhaseB_Current->graph(0)->data()->clear();
        m_plotPhaseC_Power->graph(0)->data()->clear();
        m_plotPhaseC_Voltage->graph(0)->data()->clear();
        m_plotPhaseC_Current->graph(0)->data()->clear();

        removeAllCursors();
        updateStatisticsDisplay();
        updateStatusBar();

        // Replot tutti i grafici
        m_plotPhaseA_Power->replot();
        m_plotPhaseA_Voltage->replot();
        m_plotPhaseA_Current->replot();
        m_plotPhaseB_Power->replot();
        m_plotPhaseB_Voltage->replot();
        m_plotPhaseB_Current->replot();
        m_plotPhaseC_Power->replot();
        m_plotPhaseC_Voltage->replot();
        m_plotPhaseC_Current->replot();

        qDebug() << "MainWindow: Data cleared";
    }
}

void MainWindow::onRemoveCursorsClicked()
{
    removeAllCursors();
}

void MainWindow::onImportShellyCsvClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this,
                                                     tr("Import Shelly Cloud CSV"),
                                                     QString(),
                                                     tr("CSV Files (*.csv);;All Files (*)"));

    if (filePath.isEmpty()) {
        return;  // User cancelled
    }

    ShellyCsvImporter importer;
    QVector<EnergyDailyData> energyData;
    QString errorMessage;

    if (!importer.importShellyCsvFile(filePath, energyData, errorMessage)) {
        QMessageBox::critical(this, tr("Import Failed"), errorMessage);
        return;
    }

    // Salva nel database
    int insertedCount = 0;
    int updatedCount = 0;
    if (m_databaseManager->isOpen()) {
        if (!m_databaseManager->saveDailyEnergy(energyData, &insertedCount, &updatedCount)) {
            QMessageBox::warning(this, tr("Database Save Failed"),
                               tr("CSV data imported successfully but failed to save to database:\n%1")
                               .arg(m_databaseManager->lastError()));
        }
    }

    // Display summary
    QString summaryText = tr("Successfully processed %1 daily energy records:\n").arg(energyData.size());
    summaryText += tr("  • %1 new records inserted\n").arg(insertedCount);
    summaryText += tr("  • %1 existing records updated\n\n").arg(updatedCount);

    if (!energyData.isEmpty()) {
        summaryText += tr("Date range: %1 to %2\n\n")
                       .arg(energyData.first().date.toString("dd/MM/yyyy"))
                       .arg(energyData.last().date.toString("dd/MM/yyyy"));

        // Show first 3 records as sample
        summaryText += tr("Sample data (first 3 records):\n");
        int sampleCount = qMin(3, energyData.size());
        for (int i = 0; i < sampleCount; ++i) {
            const EnergyDailyData& data = energyData[i];
            summaryText += tr("  %1: A=%2 Wh, B=%3 Wh, C=%4 Wh, Total=%5 Wh\n")
                           .arg(data.date.toString("dd/MM/yyyy"))
                           .arg(data.energyA_Wh, 0, 'f', 2)
                           .arg(data.energyB_Wh, 0, 'f', 2)
                           .arg(data.energyC_Wh, 0, 'f', 2)
                           .arg(data.totalEnergy_Wh, 0, 'f', 2);
        }

        if (m_databaseManager->isOpen()) {
            summaryText += tr("\nData saved to database (total daily records: %1)")
                           .arg(m_databaseManager->getDailyEnergyCount());
        }
    }

    // Aggiungi suggerimento per visualizzare i dati
    summaryText += tr("\n\nTip: To view the imported data, go to the 'History' tab and click 'Query Daily Energy'.");

    QMessageBox::information(this, tr("Import Successful"), summaryText);

    qDebug() << "MainWindow: CSV import completed - Records:" << energyData.size();
}

void MainWindow::onExportChartClicked()
{
    // Ottieni il grafico corrente
    QCustomPlot* plot = getCurrentPlot();
    if (!plot) {
        QMessageBox::warning(this, tr("Export Chart"), tr("No chart available to export"));
        return;
    }

    // Mostra dialog per scegliere formato
    QStringList formats;
    formats << "PNG Image (*.png)" << "PDF Document (*.pdf)";

    bool ok;
    QString selectedFormat = QInputDialog::getItem(this,
                                                     tr("Export Chart"),
                                                     tr("Select export format:"),
                                                     formats,
                                                     0,  // default index
                                                     false,  // not editable
                                                     &ok);

    if (!ok || selectedFormat.isEmpty()) {
        return;  // User cancelled
    }

    // Genera nome file di default
    QString defaultFileName;
    int mainTabIndex = m_mainTabWidget->currentIndex();

    if (mainTabIndex == 4) {
        // Statistics tab
        defaultFileName = QString("chart_Statistics_%1")
                          .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmmss"));
    } else {
        // Charts tab
        QString phaseName = getCurrentPhase() == 0 ? "PhaseA" : (getCurrentPhase() == 1 ? "PhaseB" : "PhaseC");
        QString paramName = getCurrentParameterName();
        defaultFileName = QString("chart_%1_%2_%3")
                          .arg(phaseName)
                          .arg(paramName)
                          .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmmss"));
    }

    QString filter;
    QString extension;
    if (selectedFormat.contains("PNG")) {
        filter = tr("PNG Images (*.png);;All Files (*)");
        extension = ".png";
        defaultFileName += extension;
    } else {
        filter = tr("PDF Documents (*.pdf);;All Files (*)");
        extension = ".pdf";
        defaultFileName += extension;
    }

    QString filename = QFileDialog::getSaveFileName(this,
                                                     tr("Export Chart"),
                                                     defaultFileName,
                                                     filter);

    if (filename.isEmpty()) {
        return;  // User cancelled
    }

    // Assicurati che il file abbia l'estensione corretta
    if (!filename.endsWith(extension, Qt::CaseInsensitive)) {
        filename += extension;
    }

    bool success = false;
    if (selectedFormat.contains("PNG")) {
        success = exportChartToPng(filename);
    } else {
        success = exportChartToPdf(filename);
    }

    if (success) {
        QMessageBox::information(this, tr("Export Chart"),
                                tr("Chart exported successfully to:\n%1").arg(filename));
        statusBar()->showMessage(tr("Chart exported to %1").arg(filename), 5000);
    } else {
        QMessageBox::critical(this, tr("Export Error"),
                             tr("Failed to export chart to:\n%1").arg(filename));
    }
}

// ========== SLOTS SHELLYMANAGER ==========

void MainWindow::onDataReceived(const DataPoint& dataPoint)
{
    // Aggiungi ai dati memorizzati
    m_dataPoints.append(dataPoint);
    m_lastDataPoint = dataPoint;

    // Salva nel database
    if (m_databaseManager->isOpen()) {
        if (!m_databaseManager->saveSample(dataPoint)) {
            qWarning() << "MainWindow: Failed to save sample to database";
        }
    }

    // Limita il numero di punti in memoria
    while (m_dataPoints.size() > MAX_DATA_POINTS) {
        m_dataPoints.removeFirst();
    }

    // Controlla allarmi
    m_alarmManager->checkDataPointAlarms(dataPoint);

    // === CONTROLLO POTENZA FASI A, B, C - ALLARMI BASSA POTENZA ===
    // Leggi le impostazioni dalla tab Settings
    bool lowPowerAlarmEnabledA = m_settingsTab->isLowPowerPhaseAAlarmEnabled();
    double lowPowerThresholdA = m_settingsTab->getLowPowerPhaseAThreshold();
    bool lowPowerAlarmEnabledB = m_settingsTab->isLowPowerPhaseBAlarmEnabled();
    double lowPowerThresholdB = m_settingsTab->getLowPowerPhaseBThreshold();
    bool lowPowerAlarmEnabledC = m_settingsTab->isLowPowerPhaseCAlarmEnabled();
    double lowPowerThresholdC = m_settingsTab->getLowPowerPhaseCThreshold();

    // === FASE A ===
    if (lowPowerAlarmEnabledA && dataPoint.powerA < lowPowerThresholdA) {
        if (!m_lowPowerAlarmActiveA) {
            m_lowPowerAlarmActiveA = true;
            qWarning() << "MainWindow: LOW POWER ALARM - Phase A power (" << dataPoint.powerA
                      << "W) is below threshold (" << lowPowerThresholdA << "W)";

            // Emetti suono di allarme (3 beep consecutivi)
            for (int i = 0; i < 3; ++i) {
                QApplication::beep();
            }

            // Mostra notifica visiva
            statusBar()->showMessage(
                tr("LOW POWER ALERT - Phase A: %1 W (threshold: %2 W)")
                .arg(dataPoint.powerA, 0, 'f', 2)
                .arg(lowPowerThresholdA),
                10000  // 10 secondi
            );
        }
    } else {
        if (m_lowPowerAlarmActiveA) {
            m_lowPowerAlarmActiveA = false;
            qDebug() << "MainWindow: Low power alarm cleared - Phase A power:" << dataPoint.powerA << "W";
            statusBar()->showMessage(
                tr("Phase A power restored: %1 W").arg(dataPoint.powerA, 0, 'f', 2),
                5000
            );
        }
    }

    // === FASE B ===
    if (lowPowerAlarmEnabledB && dataPoint.powerB < lowPowerThresholdB) {
        if (!m_lowPowerAlarmActiveB) {
            m_lowPowerAlarmActiveB = true;
            qWarning() << "MainWindow: LOW POWER ALARM - Phase B power (" << dataPoint.powerB
                      << "W) is below threshold (" << lowPowerThresholdB << "W)";

            // Emetti suono di allarme (3 beep consecutivi)
            for (int i = 0; i < 3; ++i) {
                QApplication::beep();
            }

            // Mostra notifica visiva
            statusBar()->showMessage(
                tr("LOW POWER ALERT - Phase B: %1 W (threshold: %2 W)")
                .arg(dataPoint.powerB, 0, 'f', 2)
                .arg(lowPowerThresholdB),
                10000  // 10 secondi
            );
        }
    } else {
        if (m_lowPowerAlarmActiveB) {
            m_lowPowerAlarmActiveB = false;
            qDebug() << "MainWindow: Low power alarm cleared - Phase B power:" << dataPoint.powerB << "W";
            statusBar()->showMessage(
                tr("Phase B power restored: %1 W").arg(dataPoint.powerB, 0, 'f', 2),
                5000
            );
        }
    }

    // === FASE C ===
    if (lowPowerAlarmEnabledC && dataPoint.powerC < lowPowerThresholdC) {
        if (!m_lowPowerAlarmActiveC) {
            m_lowPowerAlarmActiveC = true;
            qWarning() << "MainWindow: LOW POWER ALARM - Phase C power (" << dataPoint.powerC
                      << "W) is below threshold (" << lowPowerThresholdC << "W)";

            // Emetti suono di allarme (3 beep consecutivi)
            for (int i = 0; i < 3; ++i) {
                QApplication::beep();
            }

            // Mostra notifica visiva
            statusBar()->showMessage(
                tr("LOW POWER ALERT - Phase C: %1 W (threshold: %2 W)")
                .arg(dataPoint.powerC, 0, 'f', 2)
                .arg(lowPowerThresholdC),
                10000  // 10 secondi
            );
        }
    } else {
        if (m_lowPowerAlarmActiveC) {
            m_lowPowerAlarmActiveC = false;
            qDebug() << "MainWindow: Low power alarm cleared - Phase C power:" << dataPoint.powerC << "W";
            statusBar()->showMessage(
                tr("Phase C power restored: %1 W").arg(dataPoint.powerC, 0, 'f', 2),
                5000
            );
        }
    }

    // === CONTROLLO POTENZA FASI A, B, C - ALLARMI ALTA POTENZA (POWER THRESHOLD) ===
    // Leggi le impostazioni dalla tab Settings
    bool powerAlarmEnabledA = m_settingsTab->isPowerAlarmEnabled();
    double powerThresholdA = m_settingsTab->getPowerAlarmThreshold();
    bool powerAlarmEnabledB = m_settingsTab->isPowerPhaseBAlarmEnabled();
    double powerThresholdB = m_settingsTab->getPowerPhaseBThreshold();
    bool powerAlarmEnabledC = m_settingsTab->isPowerPhaseCAlarmEnabled();
    double powerThresholdC = m_settingsTab->getPowerPhaseCThreshold();

    // === FASE A ===
    if (powerAlarmEnabledA && dataPoint.powerA > powerThresholdA) {
        if (!m_powerAlarmActiveA) {
            m_powerAlarmActiveA = true;
            qWarning() << "MainWindow: POWER THRESHOLD ALARM - Phase A power (" << dataPoint.powerA
                      << "W) exceeds threshold (" << powerThresholdA << "W)";

            // Mostra notifica visiva
            statusBar()->showMessage(
                tr("POWER THRESHOLD ALERT - Phase A: %1 W (threshold: %2 W)")
                .arg(dataPoint.powerA, 0, 'f', 2)
                .arg(powerThresholdA),
                10000  // 10 secondi
            );
        }
    } else {
        if (m_powerAlarmActiveA) {
            m_powerAlarmActiveA = false;
            qDebug() << "MainWindow: Power threshold alarm cleared - Phase A power:" << dataPoint.powerA << "W";
            statusBar()->showMessage(
                tr("Phase A power normalized: %1 W").arg(dataPoint.powerA, 0, 'f', 2),
                5000
            );
        }
    }

    // === FASE B ===
    if (powerAlarmEnabledB && dataPoint.powerB > powerThresholdB) {
        if (!m_powerAlarmActiveB) {
            m_powerAlarmActiveB = true;
            qWarning() << "MainWindow: POWER THRESHOLD ALARM - Phase B power (" << dataPoint.powerB
                      << "W) exceeds threshold (" << powerThresholdB << "W)";

            // Mostra notifica visiva
            statusBar()->showMessage(
                tr("POWER THRESHOLD ALERT - Phase B: %1 W (threshold: %2 W)")
                .arg(dataPoint.powerB, 0, 'f', 2)
                .arg(powerThresholdB),
                10000  // 10 secondi
            );
        }
    } else {
        if (m_powerAlarmActiveB) {
            m_powerAlarmActiveB = false;
            qDebug() << "MainWindow: Power threshold alarm cleared - Phase B power:" << dataPoint.powerB << "W";
            statusBar()->showMessage(
                tr("Phase B power normalized: %1 W").arg(dataPoint.powerB, 0, 'f', 2),
                5000
            );
        }
    }

    // === FASE C ===
    if (powerAlarmEnabledC && dataPoint.powerC > powerThresholdC) {
        if (!m_powerAlarmActiveC) {
            m_powerAlarmActiveC = true;
            qWarning() << "MainWindow: POWER THRESHOLD ALARM - Phase C power (" << dataPoint.powerC
                      << "W) exceeds threshold (" << powerThresholdC << "W)";

            // Mostra notifica visiva
            statusBar()->showMessage(
                tr("POWER THRESHOLD ALERT - Phase C: %1 W (threshold: %2 W)")
                .arg(dataPoint.powerC, 0, 'f', 2)
                .arg(powerThresholdC),
                10000  // 10 secondi
            );
        }
    } else {
        if (m_powerAlarmActiveC) {
            m_powerAlarmActiveC = false;
            qDebug() << "MainWindow: Power threshold alarm cleared - Phase C power:" << dataPoint.powerC << "W";
            statusBar()->showMessage(
                tr("Phase C power normalized: %1 W").arg(dataPoint.powerC, 0, 'f', 2),
                5000
            );
        }
    }

    // Aggiorna UI
    updateCurrentValuesDisplay(dataPoint);
    updateStatisticsDisplay();
    updatePlot();
    updateStatusBar();

    qDebug() << "MainWindow: Data received - Total Power:" << dataPoint.getTotalPower()
             << "W (A:" << dataPoint.powerA << "W, B:" << dataPoint.powerB << "W, C:" << dataPoint.powerC << "W)";
}

void MainWindow::onErrorOccurred(const QString& errorMessage)
{
    QMessageBox::warning(this, tr("Communication Error"), errorMessage);
    statusBar()->showMessage(tr("Error: %1").arg(errorMessage), 5000);
}

void MainWindow::onConnectionStatusChanged(bool connected)
{
    m_connectButton->setEnabled(!connected);
    m_disconnectButton->setEnabled(connected);
    m_ipLineEdit->setEnabled(!connected);

    if (connected) {
        statusBar()->showMessage(tr("Connected to %1").arg(m_ipLineEdit->text()));
        // Richiedi immediatamente le device info alla connessione
        m_shellyManager->requestDeviceInfo();
    } else {
        statusBar()->showMessage(tr("Disconnected"));
    }
}

void MainWindow::onDeviceInfoReceived(const DeviceInfo& deviceInfo)
{
    // Aggiorna il Device Info Tab
    m_deviceInfoTab->updateDeviceInfo(deviceInfo);

    // Controlla allarme temperatura
    m_alarmManager->checkTemperatureAlarm(deviceInfo);

    qDebug() << "MainWindow: Device info received - Model:" << deviceInfo.model
             << "FW:" << deviceInfo.firmware_version
             << "IP:" << deviceInfo.ip_address;
}

void MainWindow::onDeviceInfoRefreshRequested()
{
    if (m_shellyManager->isConnected()) {
        m_shellyManager->requestDeviceInfo();
        statusBar()->showMessage(tr("Refreshing device info..."), 2000);
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Not connected to device"));
    }
}

// ========== SLOTS MOUSE GRAFICO ==========

void MainWindow::onPlotMouseMove(QMouseEvent* event)
{
    if (m_dataPoints.isEmpty()) {
        return;
    }

    // Identifica quale grafico ha generato l'evento
    QCustomPlot* plot = qobject_cast<QCustomPlot*>(sender());
    if (!plot) {
        return;
    }

    QCPItemTracer* tracer = nullptr;
    QCPItemText* tracerLabel = nullptr;

    // Identifica il tracer corretto in base al grafico
    if (plot == m_plotPhaseA_Power) {
        tracer = m_tracerA_Power;
        tracerLabel = m_tracerLabelA_Power;
    } else if (plot == m_plotPhaseA_Voltage) {
        tracer = m_tracerA_Voltage;
        tracerLabel = m_tracerLabelA_Voltage;
    } else if (plot == m_plotPhaseA_Current) {
        tracer = m_tracerA_Current;
        tracerLabel = m_tracerLabelA_Current;
    } else if (plot == m_plotPhaseB_Power) {
        tracer = m_tracerB_Power;
        tracerLabel = m_tracerLabelB_Power;
    } else if (plot == m_plotPhaseB_Voltage) {
        tracer = m_tracerB_Voltage;
        tracerLabel = m_tracerLabelB_Voltage;
    } else if (plot == m_plotPhaseB_Current) {
        tracer = m_tracerB_Current;
        tracerLabel = m_tracerLabelB_Current;
    } else if (plot == m_plotPhaseC_Power) {
        tracer = m_tracerC_Power;
        tracerLabel = m_tracerLabelC_Power;
    } else if (plot == m_plotPhaseC_Voltage) {
        tracer = m_tracerC_Voltage;
        tracerLabel = m_tracerLabelC_Voltage;
    } else if (plot == m_plotPhaseC_Current) {
        tracer = m_tracerC_Current;
        tracerLabel = m_tracerLabelC_Current;
    }

    double x = plot->xAxis->pixelToCoord(event->pos().x());

    // Trova il punto dati più vicino
    DataPoint nearest = findNearestDataPoint(x);

    if (!nearest.isValid()) {
        // Nascondi tracer e tooltip se non ci sono dati validi
        if (tracer) {
            tracer->setVisible(false);
        }
        if (tracerLabel) {
            tracerLabel->setVisible(false);
        }
        QToolTip::hideText();
        plot->replot(QCustomPlot::rpQueuedReplot);
        return;
    }

    // Determina fase e parametro
    int phase = -1;
    int parameter = -1;

    if (plot == m_plotPhaseA_Power) { phase = 0; parameter = 0; }
    else if (plot == m_plotPhaseA_Voltage) { phase = 0; parameter = 1; }
    else if (plot == m_plotPhaseA_Current) { phase = 0; parameter = 2; }
    else if (plot == m_plotPhaseB_Power) { phase = 1; parameter = 0; }
    else if (plot == m_plotPhaseB_Voltage) { phase = 1; parameter = 1; }
    else if (plot == m_plotPhaseB_Current) { phase = 1; parameter = 2; }
    else if (plot == m_plotPhaseC_Power) { phase = 2; parameter = 0; }
    else if (plot == m_plotPhaseC_Voltage) { phase = 2; parameter = 1; }
    else if (plot == m_plotPhaseC_Current) { phase = 2; parameter = 2; }

    double power, voltage, current, pf;
    if (phase == 0) {
        power = nearest.powerA;
        voltage = nearest.voltageA;
        current = nearest.currentA;
        pf = nearest.powerFactorA;
    } else if (phase == 1) {
        power = nearest.powerB;
        voltage = nearest.voltageB;
        current = nearest.currentB;
        pf = nearest.powerFactorB;
    } else {
        power = nearest.powerC;
        voltage = nearest.voltageC;
        current = nearest.currentC;
        pf = nearest.powerFactorC;
    }

    // Determina il valore Y corretto in base al tipo di parametro
    double yValue;
    if (parameter == 0) {
        yValue = power;
    } else if (parameter == 1) {
        yValue = voltage;
    } else {
        yValue = current;
    }

    // Posiziona e mostra il marker giallo (tracer)
    if (tracer) {
        tracer->position->setCoords(nearest.timestamp.toSecsSinceEpoch(), yValue);
        tracer->setVisible(true);
    }

    // Nascondi la label del tracer (usiamo solo il tooltip nativo)
    if (tracerLabel) {
        tracerLabel->setVisible(false);
    }

    // Formatta tooltip
    QString tooltipText = QString("<b>%1</b><br>"
                                 "Power: %2 W<br>"
                                 "Voltage: %3 V<br>"
                                 "Current: %4 A<br>"
                                 "PF: %5")
                         .arg(nearest.timestamp.toString("dd/MM/yyyy HH:mm:ss"))
                         .arg(power, 0, 'f', 2)
                         .arg(voltage, 0, 'f', 2)
                         .arg(current, 0, 'f', 3)
                         .arg(pf, 0, 'f', 3);

    // Mostra tooltip nella posizione globale del mouse (Qt 6 usa globalPosition())
    QToolTip::showText(event->globalPosition().toPoint(), tooltipText, plot);

    // Aggiorna il grafico per mostrare il marker
    plot->replot(QCustomPlot::rpQueuedReplot);
}

void MainWindow::onPlotMousePress(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && !m_dataPoints.isEmpty()) {
        QCustomPlot* plot = getCurrentPlot();
        double x = plot->xAxis->pixelToCoord(event->pos().x());
        addFixedCursor(x);
    }
}

void MainWindow::onPlotMouseDoubleClick(QMouseEvent* event)
{
    Q_UNUSED(event);
    // Reset zoom sul grafico corrente
    QCustomPlot* plot = getCurrentPlot();
    plot->rescaleAxes();
    plot->replot();
}

void MainWindow::onTabChanged(int index)
{
    // Aggiorna i dati visualizzati quando si cambia tab
    if (!m_lastDataPoint.isValid()) {
        return;
    }

    updateCurrentValuesDisplay(m_lastDataPoint);
    updateStatisticsDisplay();

    qDebug() << "MainWindow: Tab changed to phase" << index;
}

// ========== AGGIORNAMENTO UI ==========

void MainWindow::updateCurrentValuesDisplay(const DataPoint& dp)
{
    // Mostra i dati della fase corrente selezionata
    int phase = getCurrentPhase();
    double power, voltage, current, pf;

    if (phase == 0) {  // Phase A
        power = dp.powerA;
        voltage = dp.voltageA;
        current = dp.currentA;
        pf = dp.powerFactorA;
    } else if (phase == 1) {  // Phase B
        power = dp.powerB;
        voltage = dp.voltageB;
        current = dp.currentB;
        pf = dp.powerFactorB;
    } else {  // Phase C
        power = dp.powerC;
        voltage = dp.voltageC;
        current = dp.currentC;
        pf = dp.powerFactorC;
    }

    m_powerLabel->setText(tr("Power: %1 W").arg(power, 0, 'f', 2));
    m_voltageLabel->setText(tr("Voltage: %1 V").arg(voltage, 0, 'f', 2));
    m_currentLabel->setText(tr("Current: %1 A").arg(current, 0, 'f', 3));
    m_powerFactorLabel->setText(tr("PF: %1").arg(pf, 0, 'f', 3));
    m_totalPowerLabel->setText(tr("Total Power: %1 W").arg(dp.getTotalPower(), 0, 'f', 2));
}

void MainWindow::updateStatisticsDisplay()
{
    if (m_dataPoints.isEmpty()) {
        m_minLabel->setText(tr("Min: --- W"));
        m_maxLabel->setText(tr("Max: --- W"));
        m_avgLabel->setText(tr("Avg: --- W"));
        return;
    }

    double min, max, avg;
    calculateStatistics(min, max, avg);

    m_minLabel->setText(tr("Min: %1 W").arg(min, 0, 'f', 0));
    m_maxLabel->setText(tr("Max: %1 W").arg(max, 0, 'f', 0));
    m_avgLabel->setText(tr("Avg: %1 W").arg(avg, 0, 'f', 0));
}

void MainWindow::updateStatusBar()
{
    if (!m_shellyManager->isConnected()) {
        statusBar()->showMessage(tr("Disconnected"));
        return;
    }

    QString status = tr("Connected to %1 | Samples: %2")
                     .arg(m_shellyManager->currentIpAddress())
                     .arg(m_dataPoints.size());

    if (!m_dataPoints.isEmpty()) {
        status += tr(" | Last: %1")
                  .arg(m_lastDataPoint.timestamp.toString("HH:mm:ss"));
    }

    statusBar()->showMessage(status);
}

void MainWindow::updatePlot()
{
    if (m_dataPoints.isEmpty()) {
        return;
    }

    // Prepara dati per i 9 grafici (3 fasi × 3 parametri)
    QVector<double> timeData;
    QVector<double> powerDataA, voltageDataA, currentDataA;
    QVector<double> powerDataB, voltageDataB, currentDataB;
    QVector<double> powerDataC, voltageDataC, currentDataC;

    timeData.reserve(m_dataPoints.size());
    powerDataA.reserve(m_dataPoints.size());
    voltageDataA.reserve(m_dataPoints.size());
    currentDataA.reserve(m_dataPoints.size());
    powerDataB.reserve(m_dataPoints.size());
    voltageDataB.reserve(m_dataPoints.size());
    currentDataB.reserve(m_dataPoints.size());
    powerDataC.reserve(m_dataPoints.size());
    voltageDataC.reserve(m_dataPoints.size());
    currentDataC.reserve(m_dataPoints.size());

    // Filtra i dati da visualizzare (mantieni solo finestra temporale)
    QDateTime now = QDateTime::currentDateTime();
    double currentTime = now.toSecsSinceEpoch();

    for (const DataPoint& dp : m_dataPoints) {
        double t = dp.timestamp.toSecsSinceEpoch();

        // Mostra solo gli ultimi PLOT_TIME_WINDOW secondi
        if (currentTime - t <= PLOT_TIME_WINDOW) {
            timeData.append(t);
            // Fase A
            powerDataA.append(dp.powerA);
            voltageDataA.append(dp.voltageA);
            currentDataA.append(dp.currentA);
            // Fase B
            powerDataB.append(dp.powerB);
            voltageDataB.append(dp.voltageB);
            currentDataB.append(dp.currentB);
            // Fase C
            powerDataC.append(dp.powerC);
            voltageDataC.append(dp.voltageC);
            currentDataC.append(dp.currentC);
        }
    }

    // Aggiorna i 9 grafici
    m_plotPhaseA_Power->graph(0)->setData(timeData, powerDataA);
    m_plotPhaseA_Voltage->graph(0)->setData(timeData, voltageDataA);
    m_plotPhaseA_Current->graph(0)->setData(timeData, currentDataA);
    m_plotPhaseB_Power->graph(0)->setData(timeData, powerDataB);
    m_plotPhaseB_Voltage->graph(0)->setData(timeData, voltageDataB);
    m_plotPhaseB_Current->graph(0)->setData(timeData, currentDataB);
    m_plotPhaseC_Power->graph(0)->setData(timeData, powerDataC);
    m_plotPhaseC_Voltage->graph(0)->setData(timeData, voltageDataC);
    m_plotPhaseC_Current->graph(0)->setData(timeData, currentDataC);

    // Auto-scale con margine per ogni grafico
    auto applyMargin = [](QCustomPlot* plot) {
        plot->graph(0)->rescaleAxes();
        QCPRange yRange = plot->yAxis->range();
        double margin = (yRange.upper - yRange.lower) * 0.1;
        if (margin > 0) {
            plot->yAxis->setRange(yRange.lower - margin, yRange.upper + margin);
        }
    };

    applyMargin(m_plotPhaseA_Power);
    applyMargin(m_plotPhaseA_Voltage);
    applyMargin(m_plotPhaseA_Current);
    applyMargin(m_plotPhaseB_Power);
    applyMargin(m_plotPhaseB_Voltage);
    applyMargin(m_plotPhaseB_Current);
    applyMargin(m_plotPhaseC_Power);
    applyMargin(m_plotPhaseC_Voltage);
    applyMargin(m_plotPhaseC_Current);

    // Replot tutti i grafici
    m_plotPhaseA_Power->replot(QCustomPlot::rpQueuedReplot);
    m_plotPhaseA_Voltage->replot(QCustomPlot::rpQueuedReplot);
    m_plotPhaseA_Current->replot(QCustomPlot::rpQueuedReplot);
    m_plotPhaseB_Power->replot(QCustomPlot::rpQueuedReplot);
    m_plotPhaseB_Voltage->replot(QCustomPlot::rpQueuedReplot);
    m_plotPhaseB_Current->replot(QCustomPlot::rpQueuedReplot);
    m_plotPhaseC_Power->replot(QCustomPlot::rpQueuedReplot);
    m_plotPhaseC_Voltage->replot(QCustomPlot::rpQueuedReplot);
    m_plotPhaseC_Current->replot(QCustomPlot::rpQueuedReplot);
}

// ========== GESTIONE CURSORI ==========

void MainWindow::updateTracerPosition(double x, QCustomPlot* plot, QCPItemTracer* tracer, QCPItemText* tracerLabel)
{
    DataPoint nearest = findNearestDataPoint(x);

    if (!nearest.isValid()) {
        tracer->setVisible(false);
        tracerLabel->setVisible(false);
        plot->replot(QCustomPlot::rpQueuedReplot);
        return;
    }

    // Determina quale fase e parametro mostrare
    int phase = -1;
    int parameter = -1;

    // Identifica fase e parametro dal plot
    if (plot == m_plotPhaseA_Power) { phase = 0; parameter = 0; }
    else if (plot == m_plotPhaseA_Voltage) { phase = 0; parameter = 1; }
    else if (plot == m_plotPhaseA_Current) { phase = 0; parameter = 2; }
    else if (plot == m_plotPhaseB_Power) { phase = 1; parameter = 0; }
    else if (plot == m_plotPhaseB_Voltage) { phase = 1; parameter = 1; }
    else if (plot == m_plotPhaseB_Current) { phase = 1; parameter = 2; }
    else if (plot == m_plotPhaseC_Power) { phase = 2; parameter = 0; }
    else if (plot == m_plotPhaseC_Voltage) { phase = 2; parameter = 1; }
    else if (plot == m_plotPhaseC_Current) { phase = 2; parameter = 2; }

    double power, voltage, current, pf;
    if (phase == 0) {
        power = nearest.powerA;
        voltage = nearest.voltageA;
        current = nearest.currentA;
        pf = nearest.powerFactorA;
    } else if (phase == 1) {
        power = nearest.powerB;
        voltage = nearest.voltageB;
        current = nearest.currentB;
        pf = nearest.powerFactorB;
    } else {
        power = nearest.powerC;
        voltage = nearest.voltageC;
        current = nearest.currentC;
        pf = nearest.powerFactorC;
    }

    // Determina il valore Y in base al parametro
    double yValue;
    if (parameter == 0) {
        yValue = power;
    } else if (parameter == 1) {
        yValue = voltage;
    } else {
        yValue = current;
    }

    // Posiziona il tracer
    tracer->position->setCoords(nearest.timestamp.toSecsSinceEpoch(), yValue);
    tracer->setVisible(true);

    // Aggiorna la label
    QString labelText = QString("%1\n%2 W | %3 V\n%4 A | PF: %5")
                        .arg(nearest.timestamp.toString("dd/MM/yyyy HH:mm:ss"))
                        .arg(power, 0, 'f', 2)
                        .arg(voltage, 0, 'f', 2)
                        .arg(current, 0, 'f', 3)
                        .arg(pf, 0, 'f', 3);

    tracerLabel->setText(labelText);
    tracerLabel->position->setCoords(nearest.timestamp.toSecsSinceEpoch(), yValue);
    tracerLabel->setVisible(true);

    plot->replot(QCustomPlot::rpQueuedReplot);
}

void MainWindow::addFixedCursor(double x)
{
    QCustomPlot* plot = getCurrentPlot();
    int phase = getCurrentPhase();
    int parameter = getCurrentParameter();

    if (m_fixedCursors.size() >= MAX_FIXED_CURSORS) {
        // Rimuovi il primo cursore se raggiungi il massimo
        QCPItemStraightLine* line = m_fixedCursors[0].line;
        QCPItemTracer* tracer = m_fixedCursors[0].tracer;
        plot->removeItem(line);
        plot->removeItem(tracer);
        m_fixedCursors.removeFirst();
    }

    DataPoint nearest = findNearestDataPoint(x);

    if (!nearest.isValid()) {
        return;
    }

    // Determina il valore in base a fase e parametro
    double power, voltage, current;
    if (phase == 0) {
        power = nearest.powerA;
        voltage = nearest.voltageA;
        current = nearest.currentA;
    } else if (phase == 1) {
        power = nearest.powerB;
        voltage = nearest.voltageB;
        current = nearest.currentB;
    } else {
        power = nearest.powerC;
        voltage = nearest.voltageC;
        current = nearest.currentC;
    }

    // Valore Y basato sul parametro corrente
    double yValue;
    if (parameter == 0) {
        yValue = power;
    } else if (parameter == 1) {
        yValue = voltage;
    } else {
        yValue = current;
    }

    // Crea nuovo cursore fisso
    FixedCursor cursor;
    cursor.dataPoint = nearest;

    // Linea verticale
    cursor.line = new QCPItemStraightLine(plot);
    cursor.line->point1->setCoords(nearest.timestamp.toSecsSinceEpoch(), 0);
    cursor.line->point2->setCoords(nearest.timestamp.toSecsSinceEpoch(), 1);

    // Colore in base all'indice - colori vivaci per tema scuro
    QColor color = (m_fixedCursors.isEmpty()) ? QColor("#ff5252") : QColor("#69f0ae");  // Rosso e verde vivaci
    QPen pen(color, 2, Qt::DashLine);
    cursor.line->setPen(pen);

    // Marker sul punto
    cursor.tracer = new QCPItemTracer(plot);
    cursor.tracer->setGraph(plot->graph(0));
    cursor.tracer->position->setCoords(nearest.timestamp.toSecsSinceEpoch(), yValue);
    cursor.tracer->setStyle(QCPItemTracer::tsCircle);
    cursor.tracer->setPen(QPen(color, 2));
    cursor.tracer->setBrush(color);
    cursor.tracer->setSize(10);

    m_fixedCursors.append(cursor);

    updateCursorInfoDisplay();
    m_removeCursorsButton->setEnabled(true);

    plot->replot();
}

void MainWindow::removeAllCursors()
{
    QCustomPlot* plot = getCurrentPlot();

    for (const FixedCursor& cursor : m_fixedCursors) {
        plot->removeItem(cursor.line);
        plot->removeItem(cursor.tracer);
    }

    m_fixedCursors.clear();

    updateCursorInfoDisplay();
    m_removeCursorsButton->setEnabled(false);

    plot->replot();
}

void MainWindow::updateCursorInfoDisplay()
{
    if (m_fixedCursors.isEmpty()) {
        m_cursor1Label->setText(tr("Cursor 1: None"));
        m_cursor2Label->setText(tr("Cursor 2: None"));
        m_deltaLabel->setText(tr("Delta: ---"));
        return;
    }

    int phase = getCurrentPhase();

    // Cursore 1 (rosso)
    const DataPoint& dp1 = m_fixedCursors[0].dataPoint;
    double power1, voltage1, current1, pf1;

    if (phase == 0) {
        power1 = dp1.powerA; voltage1 = dp1.voltageA; current1 = dp1.currentA; pf1 = dp1.powerFactorA;
    } else if (phase == 1) {
        power1 = dp1.powerB; voltage1 = dp1.voltageB; current1 = dp1.currentB; pf1 = dp1.powerFactorB;
    } else {
        power1 = dp1.powerC; voltage1 = dp1.voltageC; current1 = dp1.currentC; pf1 = dp1.powerFactorC;
    }

    m_cursor1Label->setText(tr("Cursor 1 (Red): %1 | %2 W | %3 V | %4 A | PF: %5")
                            .arg(dp1.timestamp.toString("dd/MM/yyyy HH:mm:ss"))
                            .arg(power1, 0, 'f', 2)
                            .arg(voltage1, 0, 'f', 2)
                            .arg(current1, 0, 'f', 3)
                            .arg(pf1, 0, 'f', 3));

    if (m_fixedCursors.size() < 2) {
        m_cursor2Label->setText(tr("Cursor 2: None"));
        m_deltaLabel->setText(tr("Delta: ---"));
        return;
    }

    // Cursore 2 (verde)
    const DataPoint& dp2 = m_fixedCursors[1].dataPoint;
    double power2, voltage2, current2, pf2;

    if (phase == 0) {
        power2 = dp2.powerA; voltage2 = dp2.voltageA; current2 = dp2.currentA; pf2 = dp2.powerFactorA;
    } else if (phase == 1) {
        power2 = dp2.powerB; voltage2 = dp2.voltageB; current2 = dp2.currentB; pf2 = dp2.powerFactorB;
    } else {
        power2 = dp2.powerC; voltage2 = dp2.voltageC; current2 = dp2.currentC; pf2 = dp2.powerFactorC;
    }

    m_cursor2Label->setText(tr("Cursor 2 (Green): %1 | %2 W | %3 V | %4 A | PF: %5")
                            .arg(dp2.timestamp.toString("dd/MM/yyyy HH:mm:ss"))
                            .arg(power2, 0, 'f', 2)
                            .arg(voltage2, 0, 'f', 2)
                            .arg(current2, 0, 'f', 3)
                            .arg(pf2, 0, 'f', 3));

    // Calcola delta
    double deltaPower = power2 - power1;
    qint64 deltaTime = dp1.timestamp.secsTo(dp2.timestamp);

    int minutes = std::abs(deltaTime) / 60;
    int seconds = std::abs(deltaTime) % 60;

    QString deltaText = tr("Delta: ΔP = %1%2 W | Δt = %3m %4s")
                        .arg(deltaPower > 0 ? "+" : "")
                        .arg(deltaPower, 0, 'f', 2)
                        .arg(minutes)
                        .arg(seconds);

    m_deltaLabel->setText(deltaText);
}

DataPoint MainWindow::findNearestDataPoint(double x)
{
    if (m_dataPoints.isEmpty()) {
        return DataPoint();
    }

    // Trova il punto più vicino al timestamp x
    double minDistance = std::numeric_limits<double>::max();
    DataPoint nearest;

    for (const DataPoint& dp : m_dataPoints) {
        double t = dp.timestamp.toSecsSinceEpoch();
        double distance = std::abs(t - x);

        if (distance < minDistance) {
            minDistance = distance;
            nearest = dp;
        }
    }

    return nearest;
}

// ========== EXPORT CSV ==========

bool MainWindow::exportToCsv(const QString& filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "MainWindow: Unable to open file for writing:" << filename;
        return false;
    }

    QTextStream out(&file);

    // Header CSV - formato esteso con tutte le 3 fasi
    out << "Timestamp,Date,Time,"
        << "PowerA(W),VoltageA(V),CurrentA(A),PFA,"
        << "PowerB(W),VoltageB(V),CurrentB(A),PFB,"
        << "PowerC(W),VoltageC(V),CurrentC(A),PFC,"
        << "TotalPower(W)\n";

    // Dati
    for (const DataPoint& dp : m_dataPoints) {
        out << dp.timestamp.toString("yyyy-MM-dd HH:mm:ss") << ","
            << dp.timestamp.toString("yyyy-MM-dd") << ","
            << dp.timestamp.toString("HH:mm:ss") << ","
            // Fase A
            << QString::number(dp.powerA, 'f', 2) << ","
            << QString::number(dp.voltageA, 'f', 2) << ","
            << QString::number(dp.currentA, 'f', 3) << ","
            << QString::number(dp.powerFactorA, 'f', 3) << ","
            // Fase B
            << QString::number(dp.powerB, 'f', 2) << ","
            << QString::number(dp.voltageB, 'f', 2) << ","
            << QString::number(dp.currentB, 'f', 3) << ","
            << QString::number(dp.powerFactorB, 'f', 3) << ","
            // Fase C
            << QString::number(dp.powerC, 'f', 2) << ","
            << QString::number(dp.voltageC, 'f', 2) << ","
            << QString::number(dp.currentC, 'f', 3) << ","
            << QString::number(dp.powerFactorC, 'f', 3) << ","
            // Totale
            << QString::number(dp.getTotalPower(), 'f', 2) << "\n";
    }

    file.close();

    qDebug() << "MainWindow: Exported" << m_dataPoints.size() << "data points to" << filename;

    return true;
}

bool MainWindow::exportChartToPng(const QString& filename)
{
    QCustomPlot* plot = getCurrentPlot();
    if (!plot) {
        qWarning() << "MainWindow: No current plot available for PNG export";
        return false;
    }

    // Export con risoluzione alta: 1920x1080 (Full HD)
    bool success = plot->savePng(filename, 1920, 1080, 1.0, -1);

    if (success) {
        qDebug() << "MainWindow: Chart exported to PNG:" << filename;
    } else {
        qWarning() << "MainWindow: Failed to export chart to PNG:" << filename;
    }

    return success;
}

bool MainWindow::exportChartToPdf(const QString& filename)
{
    QCustomPlot* plot = getCurrentPlot();
    if (!plot) {
        qWarning() << "MainWindow: No current plot available for PDF export";
        return false;
    }

    // Export PDF con dimensioni A4 landscape (297x210 mm)
    bool success = plot->savePdf(filename, 297, 210);

    if (success) {
        qDebug() << "MainWindow: Chart exported to PDF:" << filename;
    } else {
        qWarning() << "MainWindow: Failed to export chart to PDF:" << filename;
    }

    return success;
}

// ========== STATISTICHE ==========

void MainWindow::calculateStatistics(double& min, double& max, double& avg)
{
    if (m_dataPoints.isEmpty()) {
        min = max = avg = 0.0;
        return;
    }

    int phase = getCurrentPhase();
    min = std::numeric_limits<double>::max();
    max = std::numeric_limits<double>::lowest();
    double sum = 0.0;

    for (const DataPoint& dp : m_dataPoints) {
        double power;
        if (phase == 0) {
            power = dp.powerA;
        } else if (phase == 1) {
            power = dp.powerB;
        } else {
            power = dp.powerC;
        }

        if (power < min) min = power;
        if (power > max) max = power;
        sum += power;
    }

    avg = sum / m_dataPoints.size();
}

// ========== HELPER FUNCTIONS ==========

QCustomPlot* MainWindow::getCurrentPlot()
{
    // Check which main tab is active
    int mainTabIndex = m_mainTabWidget->currentIndex();

    // If Statistics tab is active (index 4), return the statistics chart
    if (mainTabIndex == 4 && m_statisticsTab) {
        return m_statisticsTab->getCurrentChart();
    }

    // If not in Charts tab (index 0), return nullptr
    if (mainTabIndex != 0) {
        return nullptr;
    }

    // Original logic for Charts tab
    int phaseIndex = m_tabWidget->currentIndex();
    QTabWidget* paramTab = nullptr;

    if (phaseIndex == 0) {
        paramTab = m_paramTabWidgetA;
    } else if (phaseIndex == 1) {
        paramTab = m_paramTabWidgetB;
    } else {
        paramTab = m_paramTabWidgetC;
    }

    int paramIndex = paramTab->currentIndex();

    if (phaseIndex == 0) {  // Fase A
        if (paramIndex == 0) return m_plotPhaseA_Power;
        else if (paramIndex == 1) return m_plotPhaseA_Voltage;
        else return m_plotPhaseA_Current;
    } else if (phaseIndex == 1) {  // Fase B
        if (paramIndex == 0) return m_plotPhaseB_Power;
        else if (paramIndex == 1) return m_plotPhaseB_Voltage;
        else return m_plotPhaseB_Current;
    } else {  // Fase C
        if (paramIndex == 0) return m_plotPhaseC_Power;
        else if (paramIndex == 1) return m_plotPhaseC_Voltage;
        else return m_plotPhaseC_Current;
    }
}

int MainWindow::getCurrentPhase()
{
    return m_tabWidget->currentIndex();  // 0=A, 1=B, 2=C
}

int MainWindow::getCurrentParameter()
{
    int phaseIndex = m_tabWidget->currentIndex();
    QTabWidget* paramTab = nullptr;

    if (phaseIndex == 0) {
        paramTab = m_paramTabWidgetA;
    } else if (phaseIndex == 1) {
        paramTab = m_paramTabWidgetB;
    } else {
        paramTab = m_paramTabWidgetC;
    }

    return paramTab->currentIndex();  // 0=Power, 1=Voltage, 2=Current
}

QString MainWindow::getCurrentParameterName()
{
    int param = getCurrentParameter();
    if (param == 0) return tr("Power");
    else if (param == 1) return tr("Voltage");
    else return tr("Current");
}

QString MainWindow::getCurrentParameterUnit()
{
    int param = getCurrentParameter();
    if (param == 0) return "W";
    else if (param == 1) return "V";
    else return "A";
}

// ========== IMPOSTAZIONI ==========

void MainWindow::loadSettings()
{
    QSettings settings("ShellyLogger", "MainWindow");

    // Carica IP e intervallo polling
    QString lastIp = settings.value("lastIp", "192.168.1.100").toString();
    int lastInterval = settings.value("lastInterval", 60).toInt();

    m_ipLineEdit->setText(lastIp);
    m_intervalSpinBox->setValue(lastInterval);

    // Ripristina geometria finestra (posizione e dimensioni)
    if (settings.contains("windowGeometry")) {
        restoreGeometry(settings.value("windowGeometry").toByteArray());
        qDebug() << "MainWindow: Window geometry restored";
    } else {
        // Prima esecuzione: dimensioni di default
        resize(1200, 700);
        qDebug() << "MainWindow: Using default window size (1200x700)";
    }

    // Ripristina stato finestra (massimizzato/normale)
    if (settings.contains("windowState")) {
        restoreState(settings.value("windowState").toByteArray());
    }

    qDebug() << "MainWindow: Settings loaded - IP:" << lastIp << "Interval:" << lastInterval;
}

void MainWindow::saveSettings()
{
    QSettings settings("ShellyLogger", "MainWindow");

    settings.setValue("lastIp", m_ipLineEdit->text());
    settings.setValue("lastInterval", m_intervalSpinBox->value());

    // Salva geometria finestra (posizione e dimensioni)
    settings.setValue("windowGeometry", saveGeometry());

    // Salva stato finestra (massimizzato/normale)
    settings.setValue("windowState", saveState());

    qDebug() << "MainWindow: Settings saved (including window geometry)";
}

// ========== SLOTS SETTINGS TAB ==========

void MainWindow::onSettingsSaved()
{
    qDebug() << "MainWindow: Settings tab saved notification received";
    statusBar()->showMessage(tr("Settings saved successfully"), 3000);

    // Applica le nuove impostazioni
    int defaultInterval = m_settingsTab->getDefaultPollingInterval();
    m_intervalSpinBox->setValue(defaultInterval);

    // Configura allarmi
    m_alarmManager->setTemperatureAlarm(
        m_settingsTab->isTemperatureAlarmEnabled(),
        m_settingsTab->getTemperatureAlarmThreshold()
    );
    m_alarmManager->setPhaseImbalanceAlarm(
        m_settingsTab->isPhaseImbalanceAlarmEnabled(),
        m_settingsTab->getPhaseImbalanceAlarmThreshold()
    );
    m_alarmManager->setPowerAlarm(
        m_settingsTab->isPowerAlarmEnabled(),
        m_settingsTab->getPowerAlarmThreshold()
    );

    // Aggiorna tipo grafico (line/bar) per i grafici real-time
    updateChartType();

    qDebug() << "MainWindow: Applied default polling interval:" << defaultInterval;
    qDebug() << "MainWindow: Applied alarm settings";
    qDebug() << "MainWindow: Applied chart type setting";
}

void MainWindow::onLanguageChanged(const QString& languageCode)
{
    qDebug() << "MainWindow: Language change requested to:" << languageCode;
    statusBar()->showMessage(tr("Language changed. Some changes may require application restart."), 5000);

    // TODO: Implementare cambio lingua runtime con QTranslator
    // Per ora mostriamo solo un messaggio
    Q_UNUSED(languageCode);
}

void MainWindow::updateChartType()
{
    // Leggi impostazione tipo grafico
    QString chartType = "line";  // Default
    if (m_settingsTab) {
        chartType = m_settingsTab->getHistorySamplesChartType();
    }

    qDebug() << "MainWindow: Updating real-time charts type to:" << chartType;

    // I grafici real-time sono sempre line charts (non bar charts)
    // perché i dati arrivano in streaming continuo.
    // Bar charts hanno senso solo per dati discreti/campionati.
    // Tuttavia, se l'utente seleziona "Line Chart", usiamo linee normali.
    // Se seleziona "Bar Chart", usiamo impulse style per simulare barre verticali.

    bool useImpulseStyle = (chartType.toLower() == "bar");

    // Array con tutti i 9 grafici
    QCustomPlot* plots[] = {
        m_plotPhaseA_Power, m_plotPhaseA_Voltage, m_plotPhaseA_Current,
        m_plotPhaseB_Power, m_plotPhaseB_Voltage, m_plotPhaseB_Current,
        m_plotPhaseC_Power, m_plotPhaseC_Voltage, m_plotPhaseC_Current
    };

    for (QCustomPlot* plot : plots) {
        if (plot && plot->graphCount() > 0) {
            if (useImpulseStyle) {
                // Stile impulse: barre verticali dalla baseline
                plot->graph(0)->setLineStyle(QCPGraph::lsImpulse);
            } else {
                // Stile linea normale
                plot->graph(0)->setLineStyle(QCPGraph::lsLine);
            }
        }
    }

    // Aggiorna tutti i grafici con il nuovo stile
    if (!m_dataPoints.isEmpty()) {
        updatePlot();
    }

    qDebug() << "MainWindow: Chart type updated to" << (useImpulseStyle ? "impulse (bar-style)" : "line");
}

void MainWindow::applyTheme(const QString& theme)
{
    qDebug() << "MainWindow: Applying theme:" << theme;

    // Array con tutti i 9 grafici
    QCustomPlot* plots[] = {
        m_plotPhaseA_Power, m_plotPhaseA_Voltage, m_plotPhaseA_Current,
        m_plotPhaseB_Power, m_plotPhaseB_Voltage, m_plotPhaseB_Current,
        m_plotPhaseC_Power, m_plotPhaseC_Voltage, m_plotPhaseC_Current
    };

    for (QCustomPlot* plot : plots) {
        if (plot) {
            applyThemeToPlot(plot, theme);
        }
    }

    qDebug() << "MainWindow: Theme applied successfully";
    statusBar()->showMessage(tr("Theme changed to: %1").arg(theme == "dark" ? tr("Dark") : tr("Light")), 3000);
}

void MainWindow::applyThemeToPlot(QCustomPlot* plot, const QString& theme)
{
    // Determina colore linea basato sulla fase
    QColor lineColor;
    if (plot == m_plotPhaseA_Power || plot == m_plotPhaseA_Voltage || plot == m_plotPhaseA_Current) {
        lineColor = (theme == "dark") ? QColor("#4fc3f7") : QColor("#0277bd");  // Celeste scuro per light
    } else if (plot == m_plotPhaseB_Power || plot == m_plotPhaseB_Voltage || plot == m_plotPhaseB_Current) {
        lineColor = (theme == "dark") ? QColor("#ffb74d") : QColor("#f57c00");  // Arancione scuro per light
    } else {
        lineColor = (theme == "dark") ? QColor("#81c784") : QColor("#388e3c");  // Verde scuro per light
    }

    if (theme == "dark") {
        // === TEMA SCURO ===
        plot->setBackground(QBrush(QColor("#2b2b2b")));
        plot->axisRect()->setBackground(QBrush(QColor("#1e1e1e")));

        // Assi
        plot->xAxis->setBasePen(QPen(QColor("#b0b0b0"), 1));
        plot->yAxis->setBasePen(QPen(QColor("#b0b0b0"), 1));
        plot->xAxis->setTickPen(QPen(QColor("#b0b0b0"), 1));
        plot->yAxis->setTickPen(QPen(QColor("#b0b0b0"), 1));
        plot->xAxis->setSubTickPen(QPen(QColor("#808080"), 1));
        plot->yAxis->setSubTickPen(QPen(QColor("#808080"), 1));
        plot->xAxis->setTickLabelColor(QColor("#d0d0d0"));
        plot->yAxis->setTickLabelColor(QColor("#d0d0d0"));
        plot->xAxis->setLabelColor(QColor("#e0e0e0"));
        plot->yAxis->setLabelColor(QColor("#e0e0e0"));

        // Griglia
        plot->xAxis->grid()->setPen(QPen(QColor("#3a3a3a"), 1, Qt::SolidLine));
        plot->yAxis->grid()->setPen(QPen(QColor("#3a3a3a"), 1, Qt::SolidLine));
        plot->xAxis->grid()->setSubGridPen(QPen(QColor("#2f2f2f"), 1, Qt::DotLine));
        plot->yAxis->grid()->setSubGridPen(QPen(QColor("#2f2f2f"), 1, Qt::DotLine));

        // Leggenda
        plot->legend->setBrush(QBrush(QColor("#2b2b2b")));
        plot->legend->setBorderPen(QPen(QColor("#606060")));
        plot->legend->setTextColor(QColor("#d0d0d0"));

        // Titolo
        if (plot->plotLayout()->elementCount() > 0 && plot->plotLayout()->rowCount() > 0) {
            QCPTextElement* title = qobject_cast<QCPTextElement*>(plot->plotLayout()->element(0, 0));
            if (title) {
                title->setTextColor(QColor("#e0e0e0"));
            }
        }

    } else {
        // === TEMA CHIARO ===
        plot->setBackground(QBrush(QColor("#f5f5f5")));
        plot->axisRect()->setBackground(QBrush(QColor("#ffffff")));

        // Assi
        plot->xAxis->setBasePen(QPen(QColor("#424242"), 1));
        plot->yAxis->setBasePen(QPen(QColor("#424242"), 1));
        plot->xAxis->setTickPen(QPen(QColor("#424242"), 1));
        plot->yAxis->setTickPen(QPen(QColor("#424242"), 1));
        plot->xAxis->setSubTickPen(QPen(QColor("#757575"), 1));
        plot->yAxis->setSubTickPen(QPen(QColor("#757575"), 1));
        plot->xAxis->setTickLabelColor(QColor("#212121"));
        plot->yAxis->setTickLabelColor(QColor("#212121"));
        plot->xAxis->setLabelColor(QColor("#000000"));
        plot->yAxis->setLabelColor(QColor("#000000"));

        // Griglia
        plot->xAxis->grid()->setPen(QPen(QColor("#e0e0e0"), 1, Qt::SolidLine));
        plot->yAxis->grid()->setPen(QPen(QColor("#e0e0e0"), 1, Qt::SolidLine));
        plot->xAxis->grid()->setSubGridPen(QPen(QColor("#eeeeee"), 1, Qt::DotLine));
        plot->yAxis->grid()->setSubGridPen(QPen(QColor("#eeeeee"), 1, Qt::DotLine));

        // Leggenda
        plot->legend->setBrush(QBrush(QColor("#fafafa")));
        plot->legend->setBorderPen(QPen(QColor("#bdbdbd")));
        plot->legend->setTextColor(QColor("#212121"));

        // Titolo
        if (plot->plotLayout()->elementCount() > 0 && plot->plotLayout()->rowCount() > 0) {
            QCPTextElement* title = qobject_cast<QCPTextElement*>(plot->plotLayout()->element(0, 0));
            if (title) {
                title->setTextColor(QColor("#000000"));
            }
        }
    }

    // Aggiorna colore linea grafico
    if (plot->graphCount() > 0) {
        plot->graph(0)->setPen(QPen(lineColor, 2.5));
    }

    // Replot per applicare i cambiamenti
    plot->replot();
}

void MainWindow::onThemeChanged(const QString& theme)
{
    applyTheme(theme);
    qDebug() << "MainWindow: Theme changed to" << theme;
}

// ========== SLOTS ALLARMI ==========

void MainWindow::onAlarmTriggered(AlarmManager::AlarmType type, const QString& message, double value, double threshold)
{
    qWarning() << "ALARM TRIGGERED:" << message;

    // Mostra notifica visiva all'utente
    QString title;
    switch (type) {
        case AlarmManager::ALARM_TEMPERATURE:
            title = tr("Temperature Alarm");
            break;
        case AlarmManager::ALARM_PHASE_IMBALANCE:
            title = tr("Phase Imbalance Alarm");
            break;
        case AlarmManager::ALARM_POWER_THRESHOLD:
            title = tr("Power Threshold Alarm");
            break;
    }

    // Mostra message box non-bloccante
    QMessageBox* msgBox = new QMessageBox(this);
    msgBox->setIcon(QMessageBox::Warning);
    msgBox->setWindowTitle(title);
    msgBox->setText(message);
    msgBox->setInformativeText(tr("Current value: %1\nThreshold: %2")
                               .arg(value, 0, 'f', 2)
                               .arg(threshold, 0, 'f', 2));
    msgBox->setStandardButtons(QMessageBox::Ok);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->setModal(false);
    msgBox->show();

    // Aggiorna status bar
    statusBar()->showMessage(tr("ALARM: %1").arg(message), 10000);
}

void MainWindow::onAlarmCleared(AlarmManager::AlarmType type)
{
    QString typeStr;
    switch (type) {
        case AlarmManager::ALARM_TEMPERATURE:
            typeStr = tr("Temperature");
            break;
        case AlarmManager::ALARM_PHASE_IMBALANCE:
            typeStr = tr("Phase Imbalance");
            break;
        case AlarmManager::ALARM_POWER_THRESHOLD:
            typeStr = tr("Power Threshold");
            break;
    }

    qDebug() << "ALARM CLEARED:" << typeStr;
    statusBar()->showMessage(tr("Alarm cleared: %1").arg(typeStr), 5000);
}

// ========== HELP MENU ==========

void MainWindow::about()
{
    QString aboutText = tr(
        "<h2>Shelly 3EM - 3 Phase Monitor</h2>"
        "<p><b>Version:</b> %1</p>"
        "<p>A comprehensive monitoring application for Shelly 3EM energy meters, "
        "providing real-time data acquisition, visualization, and analysis for three-phase electrical systems.</p>"
        "<p><b>Features:</b></p>"
        "<ul>"
        "<li>Real-time monitoring of power, voltage, and current for all three phases</li>"
        "<li>Interactive charts with zoom, pan, and cursor tools</li>"
        "<li>Historical data storage and analysis</li>"
        "<li>Configurable alarms for temperature, phase imbalance, and power thresholds</li>"
        "<li>CSV import/export for data portability</li>"
        "<li>Database maintenance and optimization tools</li>"
        "</ul>"
        "<p><b>Built with:</b></p>"
        "<ul>"
        "<li>Qt %2</li>"
        "<li>QCustomPlot 2.1.1+ (GPL v3) by Emanuel Eichhammer - <a href='https://www.qcustomplot.com'>qcustomplot.com</a></li>"
        "<li>SQLite (Public Domain)</li>"
        "</ul>"
        "<p>Copyright © 2026 Paolo Sereno. Released under MIT License.</p>"
        "<p><i>QCustomPlot library is licensed under GPL v3.</i></p>"
    ).arg(QCoreApplication::applicationVersion()).arg(qVersion());

    QMessageBox::about(this, tr("About Shelly Logger"), aboutText);
}

void MainWindow::showLogViewer()
{
    LogViewerDialog* dialog = new LogViewerDialog(this);
    dialog->exec();
    dialog->deleteLater();
}

# Architettura Software - Shelly Logger

## Panoramica Generale

**Shelly Logger** è un'applicazione desktop Qt/C++ per il monitoraggio e logging dei consumi energetici di dispositivi Shelly EM3 (Energy Meter 3-Phase). L'applicazione utilizza un'architettura **MVC-like** con separazione delle responsabilità e comunicazione tramite segnali/slot Qt.

---

## 🏗️ Pattern Architetturale

L'applicazione segue un pattern **Model-View-Controller** adattato al framework Qt:

```
┌─────────────────────────────────────────────────────────────┐
│                     MainWindow (View)                        │
│  - Interfaccia grafica (9 grafici + 5 tab)                  │
│  - Gestione eventi utente                                    │
│  - Visualizzazione dati real-time e storici                  │
└──────────────────┬──────────────────────────────────────────┘
                   │ signals/slots
         ┌─────────┴──────────┬──────────────┬─────────────┐
         ▼                    ▼              ▼             ▼
┌────────────────┐   ┌───────────────┐  ┌──────────┐  ┌────────────┐
│ ShellyManager  │   │DatabaseManager│  │AlarmMgr  │  │SettingsTab │
│   (Network)    │   │   (Storage)   │  │ (Logic)  │  │   (Config) │
└────────────────┘   └───────────────┘  └──────────┘  └────────────┘
         │                    │
         ▼                    ▼
    [Shelly EM3]         [SQLite DB]
```

---

## 📦 Componenti Principali

### 1️⃣ **MainWindow** - Controllore & Vista Principale
**File:** `mainwindow.h/cpp`

**Responsabilità:**
- Coordinamento generale dell'applicazione
- Gestione interfaccia utente con **5 tab principali**:
  - **Charts**: 9 grafici interattivi (3 fasi × 3 parametri)
  - **Device Info**: Diagnostica dispositivo con 5 sotto-tab
  - **Settings**: Configurazione con 4 sotto-tab
  - **History**: Visualizzazione dati storici
  - **Statistics**: Aggregazioni e analisi trend
- Orchestrazione componenti (ShellyManager, DatabaseManager, AlarmManager)
- Gestione cursori interattivi e export dati

**Dipendenze Principali:**
```cpp
ShellyManager*      m_shellyManager;       // Comunicazione HTTP
DatabaseManager*    m_databaseManager;     // Persistenza dati
AlarmManager*       m_alarmManager;        // Sistema allarmi
DeviceInfoTab*      m_deviceInfoTab;       // Tab diagnostica
SettingsTab*        m_settingsTab;         // Tab configurazione
HistoryViewerTab*   m_historyViewerTab;    // Tab storico
StatisticsTab*      m_statisticsTab;       // Tab statistiche
```

**Pattern Utilizzati:**
- **Observer** (Qt Signals/Slots): Reagisce a eventi da componenti
- **Facade**: Espone interfaccia semplificata ai sotto-componenti
- **Strategy**: Diversi tipi di grafici (line/bar) configurabili

---

### 2️⃣ **ShellyManager** - Gestore Comunicazione HTTP
**File:** `shellymanager.h/cpp`

**Responsabilità:**
- Comunicazione con dispositivo Shelly EM3 via HTTP GET `/status`
- Polling periodico configurabile (5-300 secondi)
- Parsing risposta JSON con dati 3-phase
- Gestione errori e timeout di rete
- Disconnessione automatica dopo 3 errori consecutivi

**API Pubblica:**
```cpp
void connectToShelly(QString ip, int intervalSeconds);
void disconnect();
void requestDeviceInfo();  // Richiesta manuale info dispositivo
void setPollInterval(int seconds);

signals:
    void dataReceived(const DataPoint& dp);
    void deviceInfoReceived(const DeviceInfo& di);
    void errorOccurred(const QString& error);
    void connectionStatusChanged(bool connected);
```

**Tecnologie:**
- `QNetworkAccessManager`: HTTP client
- `QTimer`: Polling periodico
- `QJsonDocument`: Parsing JSON

---

### 3️⃣ **DatabaseManager** - Persistenza Dati SQLite
**File:** `databasemanager.h/cpp`

**Responsabilità:**
- Gestione database SQLite `shelly_history.db`
- **Auto-save continuo**: Ogni campione salvato automaticamente
- 2 tabelle principali:
  - `samples`: Campioni real-time (timestamp + 12 campi elettrici)
  - `daily_energy`: Energia giornaliera da import CSV
- Query temporali e aggregazioni
- Manutenzione: VACUUM, backup, pulizia dati vecchi

**API Principali:**
```cpp
bool saveSample(const DataPoint& dp);
bool getSamples(qint64 start, qint64 end, QVector<DataPoint>& out);
bool getAggregatedData(AggregationInterval interval, ...);
bool getComparisonStats(..., ComparisonStats& out);
bool vacuum();
bool backupDatabase(QString path);
int deleteSamplesBefore(qint64 timestamp);
```

**Schema Database:**
```sql
-- Tabella samples
CREATE TABLE samples (
    timestamp INTEGER PRIMARY KEY,
    powerA REAL, voltageA REAL, currentA REAL, powerFactorA REAL,
    powerB REAL, voltageB REAL, currentB REAL, powerFactorB REAL,
    powerC REAL, voltageC REAL, currentC REAL, powerFactorC REAL
);

-- Tabella daily_energy (da CSV import)
CREATE TABLE daily_energy (
    date TEXT PRIMARY KEY,
    energyA_kWh REAL, energyB_kWh REAL, energyC_kWh REAL
);
```

**Pattern:**
- **Repository**: Astrae accesso dati
- **Transaction Script**: Batch insert per performance

---

### 4️⃣ **AlarmManager** - Sistema Allarmi
**File:** `alarmmanager.h/cpp`

**Responsabilità:**
- Monitoraggio condizioni critiche
- 3 tipi di allarmi:
  - **Temperature**: Sovratemperatura dispositivo (40-100°C)
  - **Phase Imbalance**: Sbilanciamento fasi (5-50%)
  - **Power Threshold**: Potenza oltre soglia (3-phase, high/low)
- Notifiche visual tramite status bar
- Auto-clear quando valori tornano normali

**API:**
```cpp
void setTemperatureAlarm(bool enabled, double threshold);
void setPhaseImbalanceAlarm(bool enabled, double threshold);
void setPowerAlarm(bool enabled, double threshold);

bool checkDataPointAlarms(const DataPoint& dp);
bool checkTemperatureAlarm(const DeviceInfo& di);

signals:
    void alarmTriggered(AlarmType type, QString msg, double val, double thresh);
    void alarmCleared(AlarmType type);
```

---

### 5️⃣ **DeviceInfoTab** - Diagnostica Dispositivo
**File:** `deviceinfotab.h/cpp`

**Responsabilità:**
- Visualizzazione 30+ parametri dispositivo
- 5 sotto-tab tematici:
  - **General**: Modello, firmware, IP, MAC, uptime
  - **WiFi**: SSID, RSSI, connessione cloud
  - **System**: RAM, storage, temperatura
  - **Energy**: kWh totali per fase
  - **Diagnostics**: Corrente neutro, aggiornamenti FW
- Indicatori colorati (verde/rosso/arancione)
- Auto-refresh al connect + refresh manuale

---

### 6️⃣ **SettingsTab** - Configurazione
**File:** `settingstab.h/cpp`

**Responsabilità:**
- Configurazione completa con 4 sotto-tab:
  - **General**: Polling interval, auto-save
  - **Language**: Selezione lingua (EN/IT/FR/DE/ES)
  - **Alarms**: Soglie temperatura, sbilanciamento, potenza 3-phase
  - **Advanced**: Tema, debug, tipo grafici, manutenzione DB
- Persistenza con `QSettings` (Windows Registry)
- Validazione input e conferme modifiche

**Database Maintenance Features:**
- Info database (path, size MB, record count)
- Compact (VACUUM)
- Backup automatico con timestamp
- Clear old data con conferma doppia

---

### 7️⃣ **HistoryViewerTab** - Visualizzazione Storico
**File:** `historyviewertab.h/cpp`

**Responsabilità:**
- Query dati storici con filtri:
  - Range data/ora
  - Filtro fase (A/B/C/All)
  - Tipo query (Real-time samples / Daily energy)
- Dual view: **Table** + **Chart**
- Grafici interattivi (zoom/pan) con tipo configurabile (line/bar)
- Export CSV risultati query
- Import CSV Shelly Cloud con parser multi-sezione

---

### 8️⃣ **StatisticsTab** - Aggregazioni & Trend
**File:** `statisticstab.h/cpp`

**Responsabilità:**
- Aggregazioni temporali (Hourly/Daily/Weekly/Monthly)
- Statistiche: energia totale, potenza avg/peak, costo
- Comparazione periodi con trend analysis
- Indicatori colorati (↑ rosso, ↓ verde)
- Export CSV + PNG/PDF grafici
- Calcolo costo energia (€/kWh configurabile)

---

## 🗂️ Strutture Dati Principali

### DataPoint - Campione Real-Time
**File:** `datapoint.h`

```cpp
struct DataPoint {
    QDateTime timestamp;

    // Fase A
    double powerA, voltageA, currentA, powerFactorA;

    // Fase B
    double powerB, voltageB, currentB, powerFactorB;

    // Fase C
    double powerC, voltageC, currentC, powerFactorC;

    // Helper methods
    bool isValid() const;
    bool isPhaseAValid() const;
    double getTotalPower() const;
};
```

### DeviceInfo - Informazioni Dispositivo
**File:** `deviceinfo.h`

```cpp
struct DeviceInfo {
    // General
    QString model, firmware_version, ip_address, mac_address;
    int uptime_seconds;

    // WiFi
    QString wifi_ssid;
    int wifi_rssi;
    bool wifi_connected, cloud_connected;

    // System
    int ram_total, ram_free, fs_size, fs_free;
    double temperature;

    // Energy
    double total_energy_wh[3];  // Per fase [A,B,C]

    // Helpers
    QString getFormattedUptime() const;
    double getRamUsagePercent() const;
    QString getWiFiQuality() const;
    int getWiFiBars() const;
    double getPhaseImbalance() const;
};
```

### AggregatedData - Statistiche Aggregate
**File:** `aggregateddata.h`

```cpp
struct AggregatedData {
    QDateTime startTime, endTime;
    AggregationInterval interval;  // Hourly/Daily/Weekly/Monthly

    // Per ogni fase: avg/min/max power/voltage/current + energy
    double powerA_avg, powerA_min, powerA_max, energyA_Wh;
    // ... (idem per B e C)

    // Totali
    double totalPower_avg, totalEnergy_kWh;
    int sampleCount;
};

struct ComparisonStats {
    AggregatedData current, previous;
    double energyChange_percent, powerChange_percent;

    enum Trend { Increasing, Decreasing, Stable };
    Trend energyTrend, powerTrend;
};
```

---

## 🔄 Flusso Dati Principali

### 1. Acquisizione Real-Time
```
┌──────────────┐  HTTP GET /status   ┌──────────────┐
│ Shelly EM3   │ ←────────────────── │ShellyManager │
└──────────────┘                     └──────┬───────┘
                                            │
                                   emit dataReceived(dp)
                                            │
        ┌───────────────────────────────────┼────────────────┐
        │                                   │                │
        ▼                                   ▼                ▼
┌──────────────┐                   ┌──────────────┐  ┌──────────────┐
│ MainWindow   │                   │DatabaseManager│  │ AlarmManager │
│ - Update UI  │                   │- saveSample() │  │- checkAlarms()│
│ - Plot graph │                   └──────────────┘  └──────────────┘
└──────────────┘
```

### 2. Query Storico
```
┌──────────────┐  SELECT * FROM    ┌──────────────┐
│HistoryTab    │ ─────────────────▶│DatabaseManager│
│(User Query)  │ ◀───────────────── │              │
└──────────────┘  QVector<DataPoint>└──────────────┘
       │
       ├─▶ Update Table View
       └─▶ Update Chart View (QCustomPlot)
```

### 3. Aggregazioni Statistiche
```
┌──────────────┐   getAggregatedData()   ┌──────────────┐
│StatisticsTab │ ──────────────────────▶ │DatabaseManager│
│              │                          │ SQL GROUP BY  │
│              │ ◀────────────────────── │ + strftime()  │
└──────┬───────┘  QVector<AggregatedData>└──────────────┘
       │
       ├─▶ Calculate Trends (±5% threshold)
       ├─▶ Compare Periods (current vs previous)
       ├─▶ Update Summary Panel
       └─▶ Update Energy Chart
```

---

## 🎨 Pattern di Design Utilizzati

### 1. **Observer Pattern** (Qt Signals/Slots)
Comunicazione disaccoppiata tra componenti:
```cpp
// ShellyManager emette segnali
connect(m_shellyManager, &ShellyManager::dataReceived,
        this, &MainWindow::onDataReceived);

// MainWindow reagisce
void MainWindow::onDataReceived(const DataPoint& dp) {
    m_databaseManager->saveSample(dp);  // Auto-save
    m_alarmManager->checkDataPointAlarms(dp);
    updatePlot();
}
```

### 2. **Repository Pattern**
DatabaseManager astrae completamente l'accesso ai dati:
```cpp
// Client non conosce SQL/SQLite
bool ok = m_databaseManager->getSamples(startTime, endTime, dataPoints);
```

### 3. **Strategy Pattern**
Tipo di grafico configurabile runtime:
```cpp
// Settings: Line Chart vs Bar Chart
QString chartType = settings.value("chartType", "Line").toString();
if (chartType == "Bar") {
    graph->setLineStyle(QCPGraph::lsNone);
    graph->setScatterStyle(QCPScatterStyle::ssBar);
} else {
    graph->setLineStyle(QCPGraph::lsLine);
}
```

### 4. **Facade Pattern**
MainWindow fornisce interfaccia semplificata:
```cpp
void MainWindow::onConnectClicked() {
    QString ip = m_ipLineEdit->text();
    int interval = m_intervalSpinBox->value();

    // Dietro le quinte: network setup, timer start, DB init...
    m_shellyManager->connectToShelly(ip, interval);
}
```

### 5. **Template Method**
Setup grafici con comportamento comune:
```cpp
void MainWindow::setupPlot(QCustomPlot* plot) {
    // Template: azioni comuni a tutti i 9 grafici
    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    plot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    applyThemeToPlot(plot, currentTheme);
    setupCursors(plot, tracer, label);  // Hook points
}
```

---

## 🔌 Dipendenze Esterne

### Qt Framework (6.9.1)
- **QtCore**: Container, threading, I/O
- **QtGui**: Grafica base, eventi
- **QtWidgets**: Componenti UI (QTabWidget, QPushButton, etc.)
- **QtNetwork**: `QNetworkAccessManager` per HTTP
- **QtSql**: `QSqlDatabase` per SQLite

### QCustomPlot (2.1.1+)
Libreria GPL v3 per grafici scientifici interattivi:
- 9 grafici real-time con cursori
- Grafici storico e aggregati
- Export PNG/PDF

### SQLite
Database embedded per persistenza dati (via Qt SQL).

---

## 📊 Gestione Memoria e Performance

### Limite Dati in RAM
```cpp
static const int MAX_DATA_POINTS = 10000;  // Limite in memoria
```
Dopo 10.000 campioni, i dati più vecchi vengono rimossi dal buffer (ma rimangono nel DB).

### Batch Insert Database
```cpp
// Transazione per multipli insert
db.transaction();
for (const auto& dp : dataPoints) {
    query.bindValue(":timestamp", dp.timestamp.toSecsSinceEpoch());
    // ... bind altri campi
    query.exec();
}
db.commit();
```

### Aggregazioni SQL Efficienti
```sql
SELECT
    strftime('%Y-%m-%d %H:00:00', timestamp, 'unixepoch') as period,
    AVG(powerA), MIN(powerA), MAX(powerA),
    AVG(powerA) * (strftime('%s', MAX(timestamp)) - strftime('%s', MIN(timestamp))) / 3600.0 as energyA_Wh
FROM samples
WHERE timestamp BETWEEN ? AND ?
GROUP BY period
ORDER BY period;
```

---

## 🌐 Internazionalizzazione (i18n)

Supporto multilingua tramite Qt Linguist:

```cpp
// In codice
QString msg = tr("Connection established");

// Translation files
translations/shelly_logger_it.ts  → Italian
translations/shelly_logger_en.ts  → English
translations/shelly_logger_fr.ts  → French
translations/shelly_logger_de.ts  → German
translations/shelly_logger_es.ts  → Spanish
```

Runtime language switch via SettingsTab (alcuni testi richiedono restart).

---

## 🎯 Principi di Design Seguiti

### Single Responsibility Principle (SRP)
Ogni classe ha una responsabilità chiara:
- `ShellyManager` → Solo comunicazione HTTP
- `DatabaseManager` → Solo persistenza
- `AlarmManager` → Solo logica allarmi

### Dependency Injection
MainWindow riceve dipendenze costruite:
```cpp
MainWindow::MainWindow(QWidget *parent) {
    m_shellyManager = new ShellyManager(this);      // DI via parent
    m_databaseManager = new DatabaseManager(this);
    m_alarmManager = new AlarmManager(this);
}
```

### Open/Closed Principle
Facilmente estendibile senza modificare codice esistente:
- Nuovi tipi di allarmi: Aggiungi enum in `AlarmManager::AlarmType`
- Nuovi aggregation intervals: Aggiungi enum in `AggregationInterval`

---

## 🧪 Gestione Errori

### Network Errors
```cpp
int m_consecutiveErrors = 0;
static const int MAX_CONSECUTIVE_ERRORS = 3;

void ShellyManager::handleNetworkReply(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        m_consecutiveErrors++;
        if (m_consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
            disconnect();  // Auto-disconnect
        }
        emit errorOccurred(reply->errorString());
    } else {
        m_consecutiveErrors = 0;
    }
}
```

### Database Errors
```cpp
QString m_lastError;

bool DatabaseManager::saveSample(const DataPoint& dp) {
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        emit errorOccurred(m_lastError);
        return false;
    }
    return true;
}
```

---

## 🚀 Estensibilità Futura

### Hook Points Previsti

1. **Log Viewer Dialog**: Struttura presente, implementazione futura
```cpp
void MainWindow::showLogViewer() {
    // TODO: Implement log viewer dialog
    QMessageBox::information(this, tr("Log Viewer"),
                            tr("Log viewer coming soon..."));
}
```

2. **Multi-Device Support**: DatabaseManager preparato per multi-device
```sql
-- Schema futuro
CREATE TABLE devices (
    id INTEGER PRIMARY KEY,
    ip TEXT,
    name TEXT
);

ALTER TABLE samples ADD COLUMN device_id INTEGER;
```

3. **Desktop Notifications**: AlarmManager pronto per notifiche native Windows

---

## 📝 Conclusioni

**Shelly Logger** è un'applicazione ben strutturata che dimostra:

✅ **Separazione delle responsabilità** (MVC-like pattern)
✅ **Architettura modulare** facilmente testabile ed estendibile
✅ **Gestione robusta errori** (auto-disconnect, validation, confirmations)
✅ **Performance ottimizzate** (batch insert, SQL aggregations, buffer limit)
✅ **UX professionale** (dark theme, interactive cursors, i18n, persistence)
✅ **Codice manutenibile** (documentazione Doxygen, naming consistente)

L'architettura permette facilmente di:
- Aggiungere nuovi tipi di dispositivi
- Implementare nuovi algoritmi di aggregazione
- Estendere il sistema di allarmi
- Aggiungere nuovi formati di export

---

**Versione Documento:** 1.0
**Data:** 2026-02-04
**Autore:** Claude Code (Anthropic)

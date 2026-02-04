# Evolution Notes - Grafici Multi-Parametro

## Proposta: Grafici Tensione e Corrente oltre alla Potenza

### Motivazione

Ha senso inserire anche i grafici della **tensione** (e volendo anche della **corrente**) oltre a quelli della potenza. Ecco perché:

## Vantaggi di aggiungere grafici Tensione/Corrente:

### 1. Diagnosi problemi elettrici
- **Tensione instabile**: Fluttuazioni o cali di tensione indicano problemi nella rete elettrica
- **Sbilanciamento fasi**: Se una fase ha tensione significativamente diversa dalle altre, c'è un problema
- **Blackout/brownout**: Crolli improvvisi di tensione sono visibili immediatamente

### 2. Correlazione dati
- **Potenza = Tensione × Corrente × PF**: Vedere i 3 grafici insieme aiuta a capire perché la potenza varia
- Esempio: potenza cala ma tensione è stabile → è la corrente che scende (carico ridotto)
- Esempio: potenza cala e tensione pure → problema di alimentazione

### 3. Analisi efficienza
- **Power Factor basso**: Se vedi alta corrente ma bassa potenza con tensione normale → PF scarso
- **Corrente di spunto**: Picchi di corrente all'avvio di motori/elettrodomestici

### 4. Conformità normativa
- La tensione dovrebbe essere 230V ±10% (207-253V) in Europa
- Monitorare la tensione aiuta a verificare la qualità dell'alimentazione

## Opzioni di implementazione:

### Opzione A: Sotto-tab per ogni fase

```
Power Chart
├─ Phase A
│  ├─ [Power] [Voltage] [Current]  ← 3 sotto-tab
│  └─ [Grafico del parametro selezionato]
├─ Phase B
└─ Phase C
```

**Vantaggi:**
- UI pulita e organizzata
- Ogni parametro ha il suo spazio dedicato
- Facile da implementare
- Chiara separazione dei dati

**Svantaggi:**
- Serve switchare tab per correlare i dati
- Non si vede la relazione temporale tra parametri

### Opzione B: Grafico multi-linea (consigliata per diagnostica)

```
Power Chart
├─ Phase A
│  └─ [Grafico con 3 linee: Power (W), Voltage (V), Current (A)]
│     Assi Y doppi: sinistra Watt, destra V/A
├─ Phase B
└─ Phase C
```

**Implementazione tecnica:**
- Asse Y sinistro: Potenza (W) - scala 0-3000W
- Asse Y destro: Tensione (V) e Corrente (A) - scala 0-250V/A
- 3 linee con colori distinti:
  - Power: Celeste (#4fc3f7) - linea spessa
  - Voltage: Giallo (#ffd54f) - linea media
  - Current: Rosso (#ff5252) - linea media
- Leggenda sempre visibile

**Vantaggi:**
- Vista d'insieme immediata
- Correlazione temporale visibile a colpo d'occhio
- Ideale per diagnostica in tempo reale
- Identificazione rapida di anomalie

**Svantaggi:**
- Grafico più complesso da leggere
- Necessita di assi Y multipli (QCustomPlot lo supporta)
- Può essere "affollato" con 3 linee + cursori

### Opzione C: Tab separati per tipo (best per confronto fasi)

```
Power Chart
├─ [Power] [Voltage] [Current]  ← 3 tab principali
│  └─ Ogni tab mostra tutte e 3 le fasi sovrapposte
│     Esempio tab "Voltage":
│       - Fase A: Rosso
│       - Fase B: Verde
│       - Fase C: Blu
```

**Vantaggi:**
- Confronto immediato tra fasi per lo stesso parametro
- Utile per vedere sbilanciamenti di carico
- Identificazione rapida di quale fase ha problemi
- Ogni grafico ha scala ottimizzata per quel parametro

**Svantaggi:**
- Perde la vista per-fase
- Difficile correlare Power-Voltage-Current della stessa fase
- Architettura UI completamente diversa dall'attuale

## Raccomandazione

### Prima scelta: Opzione A (Sotto-tab per ogni fase)

**Motivazione:**
- Mantiene l'architettura attuale (tab per fase)
- Aggiunta incrementale non invasiva
- UI rimane pulita e comprensibile
- Facile navigazione: seleziona fase → seleziona parametro
- Implementazione relativamente semplice

**Implementazione suggerita:**
```cpp
QTabWidget* phaseTabWidget = new QTabWidget();

// Per ogni fase (A, B, C)
QWidget* phaseAWidget = new QWidget();
QTabWidget* parameterTabWidget = new QTabWidget();
parameterTabWidget->addTab(powerPlotA, tr("Power"));
parameterTabWidget->addTab(voltagePlotA, tr("Voltage"));
parameterTabWidget->addTab(currentPlotA, tr("Current"));

phaseTabWidget->addTab(phaseAWidget, tr("Phase A"));
// ... repeat for B and C
```

### Seconda scelta: Opzione B (Grafico multi-linea)

**Quando preferirla:**
- L'utente è un tecnico/elettricista esperto
- Serve diagnostica avanzata in tempo reale
- L'analisi della correlazione P-V-I è prioritaria

**Implementazione suggerita:**
```cpp
// Setup dual Y axis
customPlot->yAxis->setLabel(tr("Power (W)"));
customPlot->yAxis2->setLabel(tr("Voltage (V) / Current (A)"));
customPlot->yAxis2->setVisible(true);

// Power graph on left axis
customPlot->graph(0)->setValueAxis(customPlot->yAxis);
// Voltage and Current on right axis
customPlot->graph(1)->setValueAxis(customPlot->yAxis2);
customPlot->graph(2)->setValueAxis(customPlot->yAxis2);
```

### Non consigliata: Opzione C

Troppo diversa dall'architettura attuale. Da considerare solo se l'obiettivo primario diventa il confronto tra fasi piuttosto che l'analisi della singola fase.

## Considerazioni aggiuntive

### Export CSV
Rimane invariato - già include tutti i parametri di tutte le fasi.

### Current Data panel
Dovrebbe mostrare tutti i parametri della fase/tab attivo, quindi già compatibile con tutte le opzioni.

### Cursori
- Opzione A: cursori indipendenti per ogni sotto-tab (power/voltage/current)
- Opzione B: cursori mostrano tutti e 3 i valori contemporaneamente (ideale!)
- Opzione C: cursori sincronizzati tra fasi

### Performance
- Opzione A: 9 grafici totali (3 fasi × 3 parametri) - solo quello visibile fa replot
- Opzione B: 3 grafici multi-linea - più dati per replot ma meno istanze QCustomPlot
- Opzione C: 3 grafici multi-fase - simile a B

## Decisione finale suggerita

**Implementare Opzione A** per i seguenti motivi:

1. **Gradualità**: estende l'architettura esistente senza stravolgerla
2. **Usabilità**: intuitiva anche per utenti non tecnici
3. **Flessibilità**: l'utente sceglie cosa monitorare
4. **Manutenibilità**: codice modulare e riutilizzabile
5. **Scalabilità**: facile aggiungere in futuro un 4° tab "All" con vista multi-linea (Opzione B)

**Roadmap consigliata:**
1. Implementare Opzione A come MVP
2. Raccogliere feedback utente
3. Se necessario, aggiungere 4° sotto-tab "Overview" con grafico multi-linea (Opzione B) per utenti avanzati
4. Best of both worlds: semplicità + potenza diagnostica

---

**Data**: 29 Dicembre 2024
**Status**: Proposta da valutare prima dell'implementazione

---

## Architettura UI Completa con Device Info

### Visione d'insieme dell'interfaccia finale

L'applicazione completa integra:
1. **Monitoring in tempo reale** (grafici multi-fase, multi-parametro)
2. **Device Info** (informazioni sistema Shelly EM3)
3. **Energy Statistics** (analisi consumi e costi)
4. **Settings** (configurazione applicazione)

### Layout gerarchico completo

```
┌────────────────────────────────────────────────────────────────────────┐
│ Shelly EM3 Monitor                                   [─] [□] [×]       │
├────────────────────────────────────────────────────────────────────────┤
│ [Monitoring] [Device Info] [Energy Stats] [Settings]  ← Tab principali │
├────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│ ╔═══════════════ TAB: MONITORING ═════════════════════════════════╗   │
│ ║                                                                   ║   │
│ ║ ┌─ Connection ─────────────────────────────────────────────┐    ║   │
│ ║ │ IP: [192.168.1.100] [Connect] [Disconnect] Int: [60]s   │    ║   │
│ ║ └──────────────────────────────────────────────────────────┘    ║   │
│ ║                                                                   ║   │
│ ║ ┌─ Current Data (Phase A) ─────────────┬─ Totals ──────────┐    ║   │
│ ║ │ Power: 1250W   Voltage: 230.2V      │ Total: 3351.50 W  │    ║   │
│ ║ │ Current: 5.4A  PF: 0.998            │ Energy: 1234 kWh  │    ║   │
│ ║ │ Min/Max/Avg: 890/1580/1215 W        │ Today: 12.5 kWh   │    ║   │
│ ║ └──────────────────────────────────────┴────────────────────┘    ║   │
│ ║                                                                   ║   │
│ ║ ┌─ Power Chart ─────────────────────────────────────────────┐    ║   │
│ ║ │ ┌──────────────────────────────────────────────────────┐ │    ║   │
│ ║ │ │ [Phase A] [Phase B] [Phase C]  ← Tab fasi           │ │    ║   │
│ ║ │ ├──────────────────────────────────────────────────────┤ │    ║   │
│ ║ │ │ ┌────────────────────────────────────────────────┐  │ │    ║   │
│ ║ │ │ │ [Power] [Voltage] [Current] ← Sotto-tab param │  │ │    ║   │
│ ║ │ │ ├────────────────────────────────────────────────┤  │ │    ║   │
│ ║ │ │ │ Phase A - Power Consumption                   │  │ │    ║   │
│ ║ │ │ │                                                │  │ │    ║   │
│ ║ │ │ │  1600W ┤        /\                            │  │ │    ║   │
│ ║ │ │ │  1200W ┤    /\  /  \  /\                      │  │ │    ║   │
│ ║ │ │ │   800W ┼\/\/  \/    \/  \                     │  │ │    ║   │
│ ║ │ │ │        └────────────────────────────────────→ │  │ │    ║   │
│ ║ │ │ │         14:30    15:00    15:30    16:00     │  │ │    ║   │
│ ║ │ │ └────────────────────────────────────────────────┘  │ │    ║   │
│ ║ │ └──────────────────────────────────────────────────────┘ │    ║   │
│ ║ └───────────────────────────────────────────────────────────┘    ║   │
│ ║                                                                   ║   │
│ ║ ┌─ Active Cursors (Phase A - Power) ──────────────────────┐     ║   │
│ ║ │ Cursor 1: 15:23 | 1250W  Cursor 2: 15:45 | 980W        │     ║   │
│ ║ │ Delta: -270W | 22min  [Remove Cursors]                  │     ║   │
│ ║ └──────────────────────────────────────────────────────────┘     ║   │
│ ║                                                                   ║   │
│ ║ [Export CSV] [Clear Data] [Device Info...]                      ║   │
│ ╚═══════════════════════════════════════════════════════════════════╝   │
│                                                                         │
│ ╔═══════════════ TAB: DEVICE INFO ════════════════════════════════╗   │
│ ║                                                                   ║   │
│ ║ [General] [WiFi] [System] [Energy] [Diagnostics] [Phase Compare]║   │
│ ║ ┌────────────────────────────────────────────────────────────┐  ║   │
│ ║ │ ┌─ General ────────────────┬─ WiFi ──────────────────────┐ │  ║   │
│ ║ │ │ Model: Shelly EM3        │ SSID: Casa_5GHz            │ │  ║   │
│ ║ │ │ FW: v1.14.0              │ Signal: -45dBm (●●●●○)     │ │  ║   │
│ ║ │ │ IP: 192.168.1.100        │ Cloud: ✅ Connected        │ │  ║   │
│ ║ │ │ Uptime: 5d 3h 25m        │                             │ │  ║   │
│ ║ │ └──────────────────────────┴─────────────────────────────┘ │  ║   │
│ ║ │                                                             │  ║   │
│ ║ │ ┌─ System Resources ──────────────────────────────────────┐ │  ║   │
│ ║ │ │ RAM:  [████████░░] 28.5/50 KB (57%)                    │ │  ║   │
│ ║ │ │ Disk: [████████████] 180/233 KB (77%)                  │ │  ║   │
│ ║ │ │ Temp: 45.2°C ✅ Normal                                 │ │  ║   │
│ ║ │ └─────────────────────────────────────────────────────────┘ │  ║   │
│ ║ │                                                             │  ║   │
│ ║ │ ┌─ Phase Status ──────────────────────────────────────────┐ │  ║   │
│ ║ │ │ ✅ Phase A: 230.2V, 5.4A, PF 0.998                      │ │  ║   │
│ ║ │ │ ✅ Phase B: 229.8V, 4.2A, PF 0.995                      │ │  ║   │
│ ║ │ │ ✅ Phase C: 230.5V, 6.1A, PF 0.992                      │ │  ║   │
│ ║ │ │ ⚠️ Neutral: Imbalance detected (2.1A)                   │ │  ║   │
│ ║ │ └─────────────────────────────────────────────────────────┘ │  ║   │
│ ║ │                                                             │  ║   │
│ ║ │ ┌─ Energy Totals ─────────────────────────────────────────┐ │  ║   │
│ ║ │ │ Phase A: 456.2 kWh   Total consumed: 1234.56 kWh      │ │  ║   │
│ ║ │ │ Phase B: 389.8 kWh   Returned: 0.00 kWh               │ │  ║   │
│ ║ │ │ Phase C: 388.5 kWh   Today: 12.5 kWh                  │ │  ║   │
│ ║ │ └─────────────────────────────────────────────────────────┘ │  ║   │
│ ║ │                                                             │  ║   │
│ ║ │ [Refresh] [Reboot Device] [Update Firmware]                │  ║   │
│ ║ └─────────────────────────────────────────────────────────────┘  ║   │
│ ╚═══════════════════════════════════════════════════════════════════╝   │
│                                                                         │
│ ╔═══════════════ TAB: ENERGY STATS ═══════════════════════════════╗   │
│ ║                                                                   ║   │
│ ║ [Today] [Week] [Month] [Year] [Custom Range]                    ║   │
│ ║                                                                   ║   │
│ ║ ┌─ Consumption by Phase ──────────────────────────────────────┐  ║   │
│ ║ │                                                               │  ║   │
│ ║ │ Phase A: ████████████████ 456.2 kWh (37%)                   │  ║   │
│ ║ │ Phase B: █████████████░░  389.8 kWh (32%)                   │  ║   │
│ ║ │ Phase C: █████████████░░  388.5 kWh (31%)                   │  ║   │
│ ║ │                                                               │  ║   │
│ ║ │ Imbalance: 5% ✅ (Ideal < 20%)                              │  ║   │
│ ║ └───────────────────────────────────────────────────────────────┘  ║   │
│ ║                                                                   ║   │
│ ║ ┌─ Cost Analysis (€/kWh: [0.25] ) ───────────────────────────┐  ║   │
│ ║ │ Total cost: €308.64                                          │  ║   │
│ ║ │ Average daily: €12.36                                        │  ║   │
│ ║ │ Peak hour: 18:00-19:00 (€2.45)                              │  ║   │
│ ║ └──────────────────────────────────────────────────────────────┘  ║   │
│ ║                                                                   ║   │
│ ║ [Export Report] [Print]                                          ║   │
│ ╚═══════════════════════════════════════════════════════════════════╝   │
│                                                                         │
├─────────────────────────────────────────────────────────────────────────┤
│ ✅ Connected | Samples: 450 | Last: 16:45:32 | Temp: 45°C | RSSI: -45dBm│
└─────────────────────────────────────────────────────────────────────────┘
```

### Gerarchia di navigazione

**Livello 1 - Tab Principali:**
1. **Monitoring** - Monitoraggio real-time
2. **Device Info** - Informazioni dispositivo
3. **Energy Stats** - Statistiche consumi
4. **Settings** - Configurazione app

**Livello 2 - All'interno di Monitoring:**
- Tab fasi: **Phase A | Phase B | Phase C**

**Livello 3 - All'interno di ogni fase:**
- Sotto-tab parametri: **Power | Voltage | Current**

**Livello 2 - All'interno di Device Info:**
- Sotto-tab: **General | WiFi | System | Energy | Diagnostics | Phase Compare**

**Totale visualizzazioni grafiche:** 3 fasi × 3 parametri = **9 grafici**

### Componenti chiave per tab

#### TAB: Monitoring (principale)

**Pannello Connection:**
- Input IP, pulsanti Connect/Disconnect
- SpinBox intervallo polling

**Pannello Current Data:**
- Dati in tempo reale della fase/parametro selezionato
- Si aggiorna dinamicamente in base a tab attivo
- Sezione "Totals" con potenza totale e energia giornaliera

**Power Chart:**
- QTabWidget livello 2 per fasi (A/B/C)
- QTabWidget livello 3 per parametri (Power/Voltage/Current)
- 9 istanze QCustomPlot totali
- Solo il grafico visibile fa `replot()` in tempo reale

**Active Cursors:**
- Mostra cursori del grafico attualmente visibile
- Titolo dinamico: "Active Cursors (Phase X - Parameter)"
- Calcolo delta tra cursori

**Azioni:**
- Export CSV (tutte le fasi, tutti i parametri)
- Clear Data
- Device Info... (apre tab Device Info)

#### TAB: Device Info

**Implementazione dati da `/status` API:**

Struttura C++ suggerita:
```cpp
struct DeviceInfo {
    // General
    QString model;
    QString firmware_version;
    QString ip_address;
    QString mac_address;
    int uptime_seconds;

    // WiFi
    QString wifi_ssid;
    int wifi_rssi;
    bool wifi_connected;
    bool cloud_connected;

    // System
    int ram_total;
    int ram_free;
    int fs_size;
    int fs_free;
    double temperature;
    bool overtemperature;

    // Energy totals
    double total_energy_wh[3];      // Per fase
    double total_returned_wh;

    // Neutral
    double neutral_current;
    bool neutral_mismatch;

    // Update
    bool update_available;
    QString new_version;
    QString old_version;
};
```

**Sotto-tab General:**
- Modello, firmware, IP, MAC
- Uptime formattato (giorni/ore/minuti)
- WiFi SSID e signal strength con indicatore grafico
- Cloud status

**Sotto-tab WiFi:**
- SSID, signal strength (dBm + barre visive)
- IP address
- Qualità connessione: Eccellente/Buono/Discreto/Scarso
- Cloud enabled/connected

**Sotto-tab System:**
- RAM: barra progressiva + percentuale
- Storage: barra progressiva + percentuale
- Temperatura con indicatore stato (✅ ⚠️ ❌)
- Filesystem status

**Sotto-tab Energy:**
- Energia totale per fase (kWh)
- Energia restituita alla rete
- Consumo giornaliero
- Statistiche periodo

**Sotto-tab Diagnostics:**
- Validità dati per fase
- Sbilanciamento neutro
- Allarmi attivi
- Log errori recenti

**Sotto-tab Phase Compare:**
```
┌─ Phase Load Comparison ──────────────────┐
│                                           │
│ Phase A: ████████████ 1250W (37%)        │
│ Phase B: ██████████░░  980W (29%)        │
│ Phase C: ███████████░ 1120W (34%)        │
│                                           │
│ Imbalance: 12% ✅ (Ideal < 20%)         │
│                                           │
│ Neutral current: 2.1A ⚠️                 │
│ (Mismatch detected)                      │
└───────────────────────────────────────────┘
```

**Pulsanti azioni:**
- Refresh (aggiorna info da `/status`)
- Reboot Device (se API lo supporta)
- Update Firmware (se disponibile)

#### TAB: Energy Stats

**Range temporali:**
- Today / Week / Month / Year / Custom Range

**Consumption by Phase:**
- Barre orizzontali comparative
- Percentuali per fase
- Calcolo sbilanciamento

**Cost Analysis:**
- Input €/kWh configurabile
- Costo totale periodo
- Media giornaliera
- Ora di picco consumo

**Export:**
- Report PDF/CSV
- Stampa

#### TAB: Settings

**Sezioni:**
- Language selection (en, it, fr, de, es)
- Theme (dark/light - se implementato)
- Default polling interval
- Cost per kWh
- Alarms thresholds (temperature, imbalance, etc.)
- Auto-save preferences

### Status Bar (sempre visibile)

**Informazioni integrate:**
- Connection status: "✅ Connected to 192.168.1.100" o "❌ Disconnected"
- Sample count: "Samples: 450"
- Last update: "Last: 16:45:32"
- Device temperature: "Temp: 45°C"
- WiFi signal: "RSSI: -45dBm"

### Considerazioni implementative

**Performance:**
- 9 grafici QCustomPlot, ma solo quello visibile fa `replot()` real-time
- Gli altri 8 si aggiornano quando l'utente switcha tab
- Device Info: refresh manuale o timer separato (es: ogni 30s)

**Architettura classi suggerita:**
```cpp
MainWindow
├── MonitoringTab (QWidget)
│   ├── PhaseTabWidget (QTabWidget)
│   │   ├── PhaseWidget (per A, B, C)
│   │   │   ├── ParameterTabWidget (QTabWidget)
│   │   │   │   ├── PlotWidget (Power)
│   │   │   │   ├── PlotWidget (Voltage)
│   │   │   │   └── PlotWidget (Current)
│   │   └── CurrentDataPanel
│   └── CursorsPanel
├── DeviceInfoTab (QWidget)
│   ├── GeneralInfoWidget
│   ├── WiFiInfoWidget
│   ├── SystemInfoWidget
│   ├── EnergyInfoWidget
│   ├── DiagnosticsWidget
│   └── PhaseCompareWidget
├── EnergyStatsTab (QWidget)
│   ├── ConsumptionChartWidget
│   └── CostAnalysisWidget
└── SettingsTab (QWidget)
```

**Gestione dati:**
- `ShellyManager` gestisce chiamate HTTP a `/status`
- Parsing JSON completo (non solo `emeters[]`)
- Signal `deviceInfoUpdated(DeviceInfo)` per aggiornare Device Info tab
- Signal `dataPointReceived(DataPoint)` per aggiornare grafici Monitoring

**Scalabilità:**
- Architettura modulare permette aggiunta facile di nuovi tab
- Ogni widget è riutilizzabile e testabile indipendentemente
- Settings persistenti con QSettings

### Vantaggi architettura proposta

1. **Organizzazione gerarchica** - Nulla è nascosto ma tutto è organizzato logicamente
2. **Non sovraffollamento** - Ogni schermata mostra solo info rilevanti
3. **Navigazione intuitiva** - Tab logic familiare a tutti gli utenti
4. **Separazione concerns** - Monitoring vs Info vs Stats vs Config
5. **Scalabilità** - Facile aggiungere nuovi tab/features
6. **Performance** - Solo i widget visibili sono attivi
7. **Completezza** - Tutte le info disponibili da Shelly EM3 sono esposte

### Roadmap implementazione suggerita

**Fase 1: MVP esteso (Monitoring multi-parametro)**
1. Implementare tab fasi (A/B/C) ✅ (già in prompt.md)
2. Aggiungere sotto-tab parametri (Power/Voltage/Current)
3. Estendere DataPoint per tutti i parametri
4. Update ShellyManager per leggere `emeters[0-2]` completi
5. 9 grafici QCustomPlot con sistema cursori indipendenti

**Fase 2: Device Info**
1. Estendere parsing JSON `/status` per campi sistema
2. Implementare DeviceInfo struct e DeviceInfoTab
3. Sotto-tab: General, WiFi, System, Energy, Diagnostics, Phase Compare
4. Pulsante "Refresh" manuale
5. Indicatori visivi stato (✅ ⚠️ ❌)

**Fase 3: Energy Stats**
1. Database SQLite per storico lungo termine
2. Query per statistiche temporali (oggi/settimana/mese)
3. Grafici comparativi consumi
4. Calcolo costi con tariffe configurabili
5. Export report

**Fase 4: Polish**
1. Settings tab completo
2. Alarms system
3. Auto-refresh configurabile
4. Temi dark/light
5. Localizzazione completa (5 lingue)

---

**Data aggiornamento**: 29 Dicembre 2024
**Status**: Architettura completa proposta - Da implementare in fasi successive

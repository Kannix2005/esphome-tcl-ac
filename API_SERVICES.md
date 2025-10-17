# TCL AC - API Services Documentation

## 🎉 Neue Features (Option A - Direkte API-Calls)

Alle erweiterten Features sind über **ESPHome API Services** verfügbar. Diese kannst du direkt in Home Assistant nutzen!

---

## 📋 Verfügbare Services

### 1. **ECO/TURBO/QUIET/HEALTH Modi**

#### ECO Mode aktivieren/deaktivieren
```yaml
service: esphome.tclac_set_eco_mode
data:
  enabled: true  # oder false
```

#### TURBO Mode aktivieren/deaktivieren
```yaml
service: esphome.tclac_set_turbo_mode
data:
  enabled: true  # oder false
```

#### QUIET Mode aktivieren/deaktivieren
```yaml
service: esphome.tclac_set_quiet_mode
data:
  enabled: true  # oder false
```

#### HEALTH Mode aktivieren/deaktivieren
```yaml
service: esphome.tclac_set_health_mode
data:
  enabled: true  # oder false
```

---

### 2. **Display & Beeper Control**

#### Display ein/aus schalten
```yaml
service: esphome.tclac_set_display
data:
  state: true  # oder false (false = Display aus, spart Strom)
```

#### Beeper ein/aus schalten
```yaml
service: esphome.tclac_set_beeper
data:
  state: true  # oder false (false = stumm)
```

---

### 3. **Vertikale Lamellen (Fixierte Positionen)**

#### Vertikale Airflow-Position setzen
```yaml
service: esphome.tclac_set_vertical_airflow
data:
  position: 5  # 0=LAST, 1=MAX_UP, 2=UP, 3=CENTER, 4=DOWN, 5=MAX_DOWN
```

**Optionen:**
- `0` = LAST (letzte Position)
- `1` = MAX_UP (ganz nach oben)
- `2` = UP (nach oben)
- `3` = CENTER (Mitte)
- `4` = DOWN (nach unten)
- `5` = MAX_DOWN (ganz nach unten) ⭐ **STANDARD**

---

### 4. **Horizontale Lamellen (Fixierte Positionen)**

#### Horizontale Airflow-Position setzen
```yaml
service: esphome.tclac_set_horizontal_airflow
data:
  position: 5  # 0=LAST, 1=MAX_LEFT, 2=LEFT, 3=CENTER, 4=RIGHT, 5=MAX_RIGHT
```

**Optionen:**
- `0` = LAST (letzte Position)
- `1` = MAX_LEFT (ganz nach links)
- `2` = LEFT (nach links)
- `3` = CENTER (Mitte)
- `4` = RIGHT (nach rechts)
- `5` = MAX_RIGHT (ganz nach rechts) ⭐ **STANDARD**

---

### 5. **Vertikale Swing-Richtung**

#### Vertikale Swing-Richtung setzen
```yaml
service: esphome.tclac_set_vertical_swing_direction
data:
  direction: 1  # 0=OFF, 1=UP_DOWN, 2=UPSIDE, 3=DOWNSIDE
```

**Optionen:**
- `0` = OFF (kein Schwenken)
- `1` = UP_DOWN (voller Bereich, auf/ab)
- `2` = UPSIDE (nur obere Hälfte)
- `3` = DOWNSIDE (nur untere Hälfte)

---

### 6. **Horizontale Swing-Richtung**

#### Horizontale Swing-Richtung setzen
```yaml
service: esphome.tclac_set_horizontal_swing_direction
data:
  direction: 1  # 0=OFF, 1=LEFT_RIGHT, 2=LEFTSIDE, 3=CENTER, 4=RIGHTSIDE
```

**Optionen:**
- `0` = OFF (kein Schwenken)
- `1` = LEFT_RIGHT (voller Bereich, links/rechts)
- `2` = LEFTSIDE (nur linke Seite)
- `3` = CENTER (nur Mitte)
- `4` = RIGHTSIDE (nur rechte Seite)

---

## 🏠 Home Assistant Automationen - Beispiele

### Beispiel 1: ECO-Mode bei Abwesenheit
```yaml
automation:
  - alias: "AC ECO-Mode aktivieren wenn niemand zuhause"
    trigger:
      - platform: state
        entity_id: group.family
        to: 'not_home'
        for: "00:15:00"
    action:
      - service: esphome.tclac_set_eco_mode
        data:
          enabled: true
```

### Beispiel 2: TURBO-Mode bei hoher Temperatur
```yaml
automation:
  - alias: "AC TURBO bei >28°C"
    trigger:
      - platform: numeric_state
        entity_id: sensor.wohnzimmer_temperatur
        above: 28
    action:
      - service: esphome.tclac_set_turbo_mode
        data:
          enabled: true
```

### Beispiel 3: QUIET-Mode nachts
```yaml
automation:
  - alias: "AC QUIET-Mode nachts"
    trigger:
      - platform: time
        at: "22:00:00"
    action:
      - service: esphome.tclac_set_quiet_mode
        data:
          enabled: true
      - service: esphome.tclac_set_beeper
        data:
          state: false  # Beeper auch ausschalten
  
  - alias: "AC QUIET-Mode aus morgens"
    trigger:
      - platform: time
        at: "07:00:00"
    action:
      - service: esphome.tclac_set_quiet_mode
        data:
          enabled: false
      - service: esphome.tclac_set_beeper
        data:
          state: true  # Beeper wieder einschalten
```

### Beispiel 4: Lamellen nach unten richten
```yaml
automation:
  - alias: "AC Lamellen nach unten beim Einschalten"
    trigger:
      - platform: state
        entity_id: climate.klimaanlage_wohnzimmer
        from: 'off'
        to: 'cool'
    action:
      - service: esphome.tclac_set_vertical_airflow
        data:
          position: 5  # MAX_DOWN
      - service: esphome.tclac_set_horizontal_airflow
        data:
          position: 5  # MAX_RIGHT
```

### Beispiel 5: Display nachts ausschalten
```yaml
automation:
  - alias: "AC Display nachts aus"
    trigger:
      - platform: time
        at: "23:00:00"
    action:
      - service: esphome.tclac_set_display
        data:
          state: false
  
  - alias: "AC Display morgens ein"
    trigger:
      - platform: time
        at: "07:00:00"
    action:
      - service: esphome.tclac_set_display
        data:
          state: true
```

---

## 🎮 Lovelace UI - Dashboard-Karten

### Script-Buttons für schnellen Zugriff
```yaml
script:
  ac_eco_on:
    alias: "AC ECO"
    icon: mdi:leaf
    sequence:
      - service: esphome.tclac_set_eco_mode
        data:
          enabled: true
  
  ac_turbo_on:
    alias: "AC TURBO"
    icon: mdi:fan-speed-3
    sequence:
      - service: esphome.tclac_set_turbo_mode
        data:
          enabled: true
  
  ac_quiet_on:
    alias: "AC LEISE"
    icon: mdi:volume-off
    sequence:
      - service: esphome.tclac_set_quiet_mode
        data:
          enabled: true
  
  ac_lamellen_unten:
    alias: "Lamellen unten"
    icon: mdi:arrow-down-bold
    sequence:
      - service: esphome.tclac_set_vertical_airflow
        data:
          position: 5
      - service: esphome.tclac_set_horizontal_airflow
        data:
          position: 5
```

### Dashboard-Karte
```yaml
type: entities
title: Klimaanlage Steuerung
entities:
  - entity: climate.klimaanlage_wohnzimmer
  - type: divider
  - type: button
    name: ECO Mode
    icon: mdi:leaf
    tap_action:
      action: call-service
      service: script.ac_eco_on
  - type: button
    name: TURBO Mode
    icon: mdi:fan-speed-3
    tap_action:
      action: call-service
      service: script.ac_turbo_on
  - type: button
    name: QUIET Mode
    icon: mdi:volume-off
    tap_action:
      action: call-service
      service: script.ac_quiet_on
  - type: button
    name: Lamellen unten
    icon: mdi:arrow-down-bold
    tap_action:
      action: call-service
      service: script.ac_lamellen_unten
```

---

## 🔍 Debugging - Logs ansehen

```bash
python -m esphome logs tclac-fixed.yaml --device tclac.local
```

Bei VERBOSE-Logging siehst du:
- `[tcl_ac:xxx]: Setting ECO mode: ON`
- `[tcl_ac:xxx]: Setting vertical airflow direction: 5`
- `[tcl_ac:xxx]: Created complete SET packet with TCLAC protocol`

---

## ✅ Feature-Übersicht

| Feature | Status | Service Name | Parameter |
|---------|--------|-------------|-----------|
| ECO Mode | ✅ | `tclac_set_eco_mode` | `enabled: bool` |
| TURBO Mode | ✅ | `tclac_set_turbo_mode` | `enabled: bool` |
| QUIET Mode | ✅ | `tclac_set_quiet_mode` | `enabled: bool` |
| HEALTH Mode | ✅ | `tclac_set_health_mode` | `enabled: bool` |
| Display Control | ✅ | `tclac_set_display` | `state: bool` |
| Beeper Control | ✅ | `tclac_set_beeper` | `state: bool` |
| Vertical Airflow | ✅ | `tclac_set_vertical_airflow` | `position: 0-5` |
| Horizontal Airflow | ✅ | `tclac_set_horizontal_airflow` | `position: 0-5` |
| Vertical Swing Direction | ✅ | `tclac_set_vertical_swing_direction` | `direction: 0-3` |
| Horizontal Swing Direction | ✅ | `tclac_set_horizontal_swing_direction` | `direction: 0-4` |

---

## 🚀 Nächste Schritte

1. **In Home Assistant testen:**
   - Developer Tools → Services
   - Service auswählen (z.B. `esphome.tclac_set_eco_mode`)
   - Parameter eingeben (z.B. `enabled: true`)
   - "CALL SERVICE" klicken

2. **Automationen erstellen:**
   - Nutze die Beispiele oben
   - Passe sie an deine Bedürfnisse an

3. **Dashboard-Karten hinzufügen:**
   - Erstelle Scripts für schnellen Zugriff
   - Füge Buttons zu deinem Dashboard hinzu

---

## 📝 Hinweise

- **Alle Services sind sofort verfügbar** nach dem OTA-Update
- **Keine zusätzliche Konfiguration nötig** - die Services sind automatisch registriert
- **Kombination möglich** - du kannst mehrere Services nacheinander aufrufen
- **force_mode ist aktiviert** - Befehle werden sofort gesendet

**Firmware Version:** 2025.10.1  
**Compiled:** October 17, 2025, 09:36:32  
**Firmware Size:** 1.073.142 bytes (1,07 MB)

---

## 🐛 Probleme?

Falls ein Service nicht funktioniert:
1. Logs checken: `python -m esphome logs tclac-fixed.yaml --device tclac.local`
2. AC aus- und einschalten (vom Strom trennen)
3. ESPHome neu starten über Home Assistant

Bei Fragen: GitHub Issues im Repository `Kannix2005/esphome-tcl-ac`

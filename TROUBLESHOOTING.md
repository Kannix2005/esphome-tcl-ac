# Troubleshooting: Component not appearing in logs

## Problem
ESPHome compiles and connects successfully, but the TCL AC component doesn't appear in the `dump_config()` output. You only see WiFi, Logger, UART, etc., but no "TCL AC Climate" section.

## Symptoms
```
[C][wifi:679]: WiFi:
[C][logger:261]: Logger:
[C][uart.idf:180]: UART Bus 0:
[C][api:222]: Server:
# Missing: [C][tcl_ac:xxx]: TCL AC Climate:
```

## Causes

### 1. Component Not Compiled
**Check**: Look for compilation errors in ESPHome build output

**Solution**: 
```bash
# Clean build
esphome clean yourconfig.yaml
esphome compile yourconfig.yaml
```

Look for errors like:
- `error: 'TclAcClimate' does not name a type`
- `fatal error: tcl_ac.h: No such file or directory`
- Python errors during code generation

### 2. Wrong Platform in YAML
**Check**: Your YAML shows `platform: ESP8266` but logs show ESP32 (GPIO43/44 are ESP32-S3 pins!)

**Current config**:
```yaml
esphome:
  name: tcl-ac
  platform: ESP8266  # ← WRONG!
  board: d1_mini
```

**Your actual hardware** (from logs):
```
[C][uart.idf:181]:   TX Pin: GPIO43  # ← ESP32-S3 pins!
[C][uart.idf:182]:   RX Pin: GPIO44
```

**Solution**: Update your YAML:
```yaml
esphome:
  name: tclac
  platform: ESP32
  board: esp32-s3-devkitc-1  # or your specific board

uart:
  tx_pin: GPIO43  # Your current pins
  rx_pin: GPIO44
  baud_rate: 9600
  parity: EVEN
```

### 3. Component Not Refreshing
**Check**: Old cached component version

**Solution**: Force refresh:
```yaml
external_components:
  - source: github://Kannix2005/esphome-tcl-ac
    components: [ tcl_ac ]
    refresh: 0s  # ← Force fetch latest
```

Then clean build:
```bash
esphome clean yourconfig.yaml
esphome compile yourconfig.yaml
```

### 4. Missing UART Connection
**Check**: If UART device isn't properly initialized, component may fail silently

**Solution**: Add UART debugging temporarily:
```yaml
uart:
  tx_pin: GPIO43
  rx_pin: GPIO44
  baud_rate: 9600
  parity: EVEN
  debug:
    direction: BOTH  # See all UART traffic
```

### 5. Logger Baud Rate Conflict
**Current config**:
```yaml
logger:
  baud_rate: 0  # Disables serial logging
```

**Issue**: With `baud_rate: 0`, you can't see serial logs directly. You MUST use network logging.

**Solution**: 
- Use `esphome logs yourconfig.yaml` over WiFi
- OR temporarily enable serial logging for debugging:
```yaml
logger:
  level: DEBUG
  baud_rate: 115200  # Temporary for debugging
```

Then change UART pins to avoid conflict:
```yaml
uart:
  tx_pin: GPIO17  # Different pins during debug
  rx_pin: GPIO16
```

## Step-by-Step Fix

### 1. Update your YAML to match your hardware:

```yaml
esphome:
  name: tclac
  platform: ESP32  # ← CHANGED!
  board: esp32-s3-devkitc-1  # or esp32dev

wifi:
  ssid: "YourWiFi"
  password: "YourPassword"

# Enable verbose logging
logger:
  level: DEBUG
  logs:
    tcl_ac: VERBOSE  # ← Add this to see component logs
    climate: DEBUG
    uart: INFO

api:
ota:

# UART with your current pins
uart:
  tx_pin: GPIO43
  rx_pin: GPIO44
  baud_rate: 9600
  parity: EVEN
  data_bits: 8
  stop_bits: 1

# Force refresh component
external_components:
  - source: github://Kannix2005/esphome-tcl-ac
    components: [ tcl_ac ]
    refresh: 0s

# Climate with ID for easier debugging
climate:
  - platform: tcl_ac
    id: my_ac
    name: "TCL Air Conditioner"
    beeper: true
    display: false
```

### 2. Clean build and flash:

```bash
# Clean old build
esphome clean yourconfig.yaml

# Compile (watch for errors)
esphome compile yourconfig.yaml

# Flash
esphome upload yourconfig.yaml
```

### 3. Watch logs over WiFi:

```bash
esphome logs yourconfig.yaml
```

You should now see:
```
[C][tcl_ac:xxx]: TCL AC Climate:
[C][tcl_ac:xxx]:   Beeper: ON
[C][tcl_ac:xxx]:   Display: OFF
```

### 4. Check Home Assistant:

After successful flash, the component should appear in HA as:
- **Entity**: `climate.tcl_air_conditioner`
- **Device**: "tclac"

## Expected Output After Fix

```
[14:20:13.540][C][uart.idf:191]:   Baud Rate: 9600 baud
[14:20:13.540][C][uart.idf:191]:   Data Bits: 8
[14:20:13.540][C][uart.idf:191]:   Parity: EVEN
[14:20:13.540][C][uart.idf:191]:   Stop bits: 1
[14:20:13.xxx][C][tcl_ac:072]: TCL AC Climate:  # ← Should appear!
[14:20:13.xxx][C][tcl_ac:073]:   Beeper: ON
[14:20:13.xxx][C][tcl_ac:074]:   Display: OFF
[14:20:13.xxx][C][tcl_ac:075]:   Vertical Direction: 5
[14:20:13.xxx][C][tcl_ac:076]:   Horizontal Direction: 5
[14:20:13.542][C][api:222]: Server:
```

## Still Not Working?

### Enable maximum debugging:

```yaml
logger:
  level: VERBOSE
  logs:
    tcl_ac: VERBOSE
    climate: VERBOSE
    uart: VERBOSE
    component: VERBOSE
    
uart:
  # ... your config ...
  debug:
    direction: BOTH
    dummy_receiver: false
```

### Check compilation output:

Look for these lines during `esphome compile`:
```
INFO Reading configuration...
INFO Generating C++ source...
INFO Compiling app...
[tcl_ac] Compiling .pioenvs/xxx/src/esphome/components/tcl_ac/tcl_ac.cpp.o
# ← Should see component being compiled!
```

If you DON'T see `tcl_ac.cpp.o` being compiled, the component isn't being included!

### Verify component was downloaded:

```bash
# Component cache location
ls ~/.esphome/external_components/
# Should see a folder with hash name containing tcl_ac/
```

## Common Mistakes

1. ❌ **Wrong platform**: ESP8266 config but ESP32 hardware
2. ❌ **No refresh**: Old cached component version
3. ❌ **Logger conflict**: baud_rate 0 prevents seeing logs
4. ❌ **Pin conflict**: UART pins clash with other components
5. ❌ **Network issue**: Can't download component from GitHub

## Quick Test

Use the provided `examples/debug.yaml`:
```bash
cp examples/debug.yaml test-tcl-ac.yaml
# Edit WiFi credentials
esphome run test-tcl-ac.yaml
```

This has all debugging enabled and correct ESP32 configuration.

## Need Help?

If component still doesn't appear:
1. Post full `esphome compile` output
2. Post full runtime logs
3. Share your complete YAML config
4. Mention your hardware (ESP32 model, board type)

GitHub Issues: https://github.com/Kannix2005/esphome-tcl-ac/issues

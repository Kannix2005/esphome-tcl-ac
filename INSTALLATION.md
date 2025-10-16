# Installation Guide - ESPHome TCL AC Component

This guide will help you install and configure the TCL AC component for ESPHome.

## Table of Contents
1. [Hardware Setup](#hardware-setup)
2. [ESPHome Installation](#esphome-installation)
3. [Component Configuration](#component-configuration)
4. [Testing](#testing)
5. [Troubleshooting](#troubleshooting)

---

## 1. Hardware Setup

### Required Components

- **ESP8266** (D1 Mini, NodeMCU, etc.) or **ESP32** board
- **TCL Air Conditioner** with Realtek RTL8710C WiFi module
- Dupont wires for UART connection
- (Optional) Logic level converter if your ESP uses 5V logic

### UART Connection

The TCL AC uses a Realtek RTL8710C WiFi module internally. You need to tap into the UART connection:

```
┌─────────────────┐         ┌─────────────────┐
│   ESP8266/32    │         │  TCL AC (RTL)   │
│                 │         │                 │
│  TX (GPIO1)  ───┼────────►│  RX (GPIO13)    │
│  RX (GPIO3)  ◄──┼─────────│  TX (GPIO14)    │
│  GND         ───┼─────────│  GND            │
│                 │         │                 │
└─────────────────┘         └─────────────────┘
```

**Pin Mappings:**

| ESP8266 (D1 Mini) | ESP32 | TCL AC (RTL8710C) |
|-------------------|-------|-------------------|
| GPIO1 (TX)        | Any GPIO | GPIO13 (RX) |
| GPIO3 (RX)        | Any GPIO | GPIO14 (TX) |
| GND               | GND   | GND             |

**Important Notes:**
- ⚠️ Both devices use **3.3V logic** - no level shifter needed
- ⚠️ Do **NOT** connect VCC/3.3V - power them separately
- ⚠️ Make sure to connect **common ground**
- 📝 TX connects to RX and vice versa

### Physical Installation

1. **Power off the AC completely** (unplug from wall)
2. Open the AC cover to access the WiFi module
3. Locate the RTL8710C module (small PCB with WiFi antenna)
4. Identify GPIO13 (RX) and GPIO14 (TX) pins
5. Solder or use a connector to tap the UART lines
6. Route wires to your ESP board location
7. Keep ESP board away from high voltage components

### Testing UART Connection

Before proceeding, verify UART is working:

1. Power on AC and ESP
2. In ESPHome logs, you should see UART traffic
3. Use this test configuration:

```yaml
uart:
  tx_pin: GPIO1
  rx_pin: GPIO3
  baud_rate: 9600
  parity: EVEN
  debug:
    direction: BOTH
```

If you see hex data like `BB 00 04 ...`, UART is working!

---

## 2. ESPHome Installation

### Option A: ESPHome Dashboard (Recommended)

1. **Install ESPHome** (if not already installed):
   ```bash
   pip install esphome
   ```

2. **Start ESPHome Dashboard**:
   ```bash
   esphome dashboard config/
   ```

3. **Access Dashboard** at `http://localhost:6052`

### Option B: Home Assistant Add-on

1. Go to **Settings** → **Add-ons**
2. Install **ESPHome** add-on
3. Start the add-on and open the web UI

---

## 3. Component Configuration

### Step 1: Create New Device

In ESPHome Dashboard, click **"+ NEW DEVICE"**:
- Name: `tcl-ac`
- Device Type: Choose your board (e.g., ESP8266 / D1 Mini)

### Step 2: Edit Configuration

Replace the generated YAML with one of the examples:

#### Basic Configuration

```yaml
esphome:
  name: tcl-ac
  platform: ESP8266
  board: d1_mini

wifi:
  ssid: "YOUR_WIFI_SSID"
  password: "YOUR_WIFI_PASSWORD"
  
  ap:
    ssid: "TCL-AC-Fallback"
    password: "12345678"

logger:
  level: INFO
  baud_rate: 0  # Important: Disable serial logging!

api:
ota:

uart:
  tx_pin: GPIO1
  rx_pin: GPIO3
  baud_rate: 9600
  parity: EVEN
  data_bits: 8
  stop_bits: 1

external_components:
  - source: github://yourusername/esphome-tcl-ac
    components: [ tcl_ac ]

climate:
  - platform: tcl_ac
    name: "Air Conditioner"
```

**Important Settings:**
- `logger.baud_rate: 0` - **Must be 0** to free up UART pins
- `uart.parity: EVEN` - TCL protocol uses **even parity**
- `uart.baud_rate: 9600` - Fixed baud rate for TCL AC

### Step 3: Customize Options (Optional)

```yaml
climate:
  - platform: tcl_ac
    name: "Living Room AC"
    
    # Beeper on/off (default: true)
    # Recommended: true (98% of packets in analysis had beeper ON)
    beeper: true
    
    # Display always on/off (default: false)
    # false = saves power (87% of packets had display OFF)
    display: false
    
    # Vertical direction (default: "down")
    # Options: max_up, up, center, down, max_down, swing
    vertical_direction: "down"
    
    # Horizontal direction (default: "max_right")
    # Options: max_left, left, center, right, max_right, swing
    horizontal_direction: "max_right"
```

### Step 4: Install to Device

1. Click **"INSTALL"** in ESPHome Dashboard
2. Choose installation method:
   - **Wirelessly** (if ESP already has ESPHome)
   - **Plug into this computer** (first install via USB)
   - **Manual download** (download .bin and flash with esptool)

3. Wait for compilation and upload

---

## 4. Testing

### Verify Installation

1. **Check ESPHome logs**:
   ```
   INFO tcl_ac: TCL AC Climate component initialized
   INFO tcl_ac: Beeper: ON
   INFO tcl_ac: Display: OFF
   ```

2. **Check Home Assistant**:
   - Go to **Settings** → **Devices & Services**
   - You should see **ESPHome** integration
   - Find your device (e.g., "tcl-ac")
   - Entity should be: `climate.air_conditioner`

3. **Test basic control**:
   - Set mode to **Cool**
   - Set temperature to **22°C**
   - Check if AC responds

### First Test Sequence

1. **Turn ON** (mode: Cool, 22°C)
   - AC should power on and start cooling
   - ESPHome logs: `Sent SET packet to AC`

2. **Change temperature** to 24°C
   - AC should acknowledge
   - Target temp should update

3. **Change fan mode** to Medium
   - Fan speed should increase

4. **Try preset**: Eco
   - AC should switch to Auto mode with ECO flag

5. **Enable swing**: Vertical
   - Louvers should start moving

6. **Turn OFF**
   - AC should power off
   - ESPHome logs: `Sent POWER OFF packet to AC`

---

## 5. Troubleshooting

### AC doesn't respond to commands

**Check UART connection:**
```yaml
uart:
  tx_pin: GPIO1
  rx_pin: GPIO3
  baud_rate: 9600
  parity: EVEN
  debug:
    direction: BOTH  # Add this temporarily
```

Look for outgoing packets in logs like:
```
[UART] >>> BB 00 01 03 20 03 01 24 01 ...
```

**Solutions:**
- Verify TX/RX are not swapped
- Check GND connection
- Verify parity is EVEN (not NONE or ODD)
- Check for loose wires

### Can't see logs in ESPHome

**Reason**: You set `logger.baud_rate: 0` to free UART pins.

**Solution**: View logs over WiFi:
```bash
esphome logs tcl-ac.yaml
```

Or use Home Assistant ESPHome add-on logs viewer.

### "Checksum mismatch" errors

**Reason**: UART noise or incorrect parity.

**Solutions:**
1. Check parity is EVEN
2. Add short wires (reduce noise)
3. Add capacitor between RX/GND (100nF)
4. Keep ESP away from AC's high-voltage components

### AC turns on but doesn't follow commands

**Possible causes:**
- Wrong packet format for your AC model
- AC firmware version difference

**Debugging:**
1. Enable debug logs: `logger.level: DEBUG`
2. Capture incoming packets from AC
3. Compare with expected format
4. Report issue on GitHub with logs

### Temperature shows as "Unknown"

**Reason**: AC doesn't always send temperature response packets.

**Solutions:**
- Wait 30 seconds (polling every 10s)
- Some AC models don't report current temp
- Target temp should always work

### WiFi keeps disconnecting

**Solutions:**
1. Check WiFi signal strength (add `wifi_signal` sensor)
2. Use static IP instead of DHCP
3. Increase TX power:
   ```yaml
   wifi:
     output_power: 20dB
   ```

### Home Assistant doesn't discover device

**Check API encryption:**
```yaml
api:
  # Remove encryption for testing
  # encryption:
  #   key: "..."
```

**Or regenerate API key:**
1. Delete device from HA
2. Generate new encryption key in ESPHome
3. Re-add device

### ESP keeps rebooting

**Reason**: Insufficient power supply.

**Solutions:**
- Use quality USB cable
- Use 2A+ power supply
- Add 100μF capacitor across VCC/GND
- Don't power ESP from AC's 3.3V (use separate PSU)

---

## Advanced Configuration

### Multiple ACs

```yaml
uart:
  - id: uart_ac1
    tx_pin: GPIO1
    rx_pin: GPIO3
    baud_rate: 9600
    parity: EVEN

  - id: uart_ac2
    tx_pin: GPIO15
    rx_pin: GPIO13
    baud_rate: 9600
    parity: EVEN

climate:
  - platform: tcl_ac
    uart_id: uart_ac1
    name: "Living Room AC"
  
  - platform: tcl_ac
    uart_id: uart_ac2
    name: "Bedroom AC"
```

### Automations

**Turn on at specific time:**
```yaml
time:
  - platform: homeassistant
    on_time:
      - hours: 7
        minutes: 0
        then:
          - climate.control:
              id: my_ac
              mode: COOL
              target_temperature: 22°C
```

**Eco mode based on electricity price:**
```yaml
sensor:
  - platform: homeassistant
    id: electricity_price
    entity_id: sensor.electricity_price
    on_value:
      then:
        - if:
            condition:
              lambda: 'return x > 0.30;'
            then:
              - climate.control:
                  id: my_ac
                  preset: ECO
```

---

## Support

- **GitHub Issues**: [yourusername/esphome-tcl-ac/issues]
- **ESPHome Discord**: #custom-components channel
- **Documentation**: See README.md for full details

---

## Next Steps

1. ✅ Hardware connected and tested
2. ✅ ESPHome installed and configured
3. ✅ Device discovered in Home Assistant
4. 📱 Create dashboard cards for easy control
5. 🤖 Set up automations (schedules, presence detection, etc.)
6. 📊 Monitor energy usage (if you have power monitoring)

Enjoy your smart TCL AC! 🎉

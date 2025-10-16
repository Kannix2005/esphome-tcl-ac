# Quick Start Guide

Get your TCL AC working with ESPHome in 5 minutes!

## Prerequisites

- ESP8266 or ESP32 board (e.g., D1 Mini, NodeMCU)
- TCL AC with Realtek RTL8710C WiFi module
- 3 wires for UART connection
- ESPHome installed

## Hardware Connection

```
ESP TX (GPIO1)  →  AC RX (GPIO13)
ESP RX (GPIO3)  ←  AC TX (GPIO14)
ESP GND         -  AC GND
```

⚠️ **Power them separately!** Don't connect VCC.

## Software Setup

### 1. Create ESPHome Configuration

Create `tcl-ac.yaml`:

```yaml
esphome:
  name: tcl-ac
  platform: ESP8266
  board: d1_mini

wifi:
  ssid: "YourWiFi"
  password: "YourPassword"

logger:
  baud_rate: 0  # Important!

api:
ota:

uart:
  tx_pin: GPIO1
  rx_pin: GPIO3
  baud_rate: 9600
  parity: EVEN

external_components:
  - source: github://yourusername/esphome-tcl-ac
    components: [ tcl_ac ]

climate:
  - platform: tcl_ac
    name: "Air Conditioner"
```

### 2. Flash to ESP

```bash
esphome run tcl-ac.yaml
```

Choose "Plug into this computer" for first flash.

### 3. Done!

Your AC should now appear in Home Assistant as a climate entity.

## Test It

1. Open Home Assistant
2. Find `climate.air_conditioner`
3. Set mode to **Cool**
4. Set temperature to **22°C**
5. AC should turn on!

## Troubleshooting

### AC doesn't respond?
- Check UART wiring (TX→RX, RX→TX)
- Verify parity is EVEN
- Check common ground

### Can't see logs?
View logs over WiFi:
```bash
esphome logs tcl-ac.yaml
```

### More help?
See [INSTALLATION.md](INSTALLATION.md) for detailed guide.

## Next Steps

- Customize settings (beeper, display, directions)
- Add automations (schedules, presence detection)
- Create dashboard cards
- Check [examples/](examples/) for advanced configs

Enjoy your smart AC! 🎉

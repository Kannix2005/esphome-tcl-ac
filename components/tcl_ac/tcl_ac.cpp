#include "tcl_ac.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace tcl_ac {

static const char *const TAG = "tcl_ac";

void TclAcClimate::setup() {
  // Initialize with defaults
  this->mode = climate::CLIMATE_MODE_OFF;
  this->target_temperature = 22.0f;
  this->current_temperature = NAN;
  this->fan_mode = climate::CLIMATE_FAN_LOW;  // Most common in log (83%)
  this->preset = climate::CLIMATE_PRESET_NONE;
  this->swing_mode = climate::CLIMATE_SWING_OFF;
  
  ESP_LOGCONFIG(TAG, "TCL AC Climate component initialized");
}

void TclAcClimate::loop() {
  // Read incoming UART data
  while (this->available()) {
    uint8_t byte;
    this->read_byte(&byte);
    this->rx_buffer_.push_back(byte);
    
    // Check if we have a complete packet (minimum 7 bytes)
    if (this->rx_buffer_.size() >= 7) {
      // Check for valid header (BB 00 04 = AC to MCU)
      if (this->rx_buffer_[0] == 0xBB && this->rx_buffer_[1] == 0x00 && this->rx_buffer_[2] == 0x04) {
        uint8_t cmd = this->rx_buffer_[3];
        uint8_t length = this->rx_buffer_[4];
        size_t expected_size = 5 + length + 1; // header(3) + cmd(1) + len(1) + data + checksum(1)
        
        if (this->rx_buffer_.size() >= expected_size) {
          // Validate checksum
          uint8_t calculated = this->calculate_checksum_(this->rx_buffer_.data(), expected_size - 1);
          uint8_t received = this->rx_buffer_[expected_size - 1];
          
          if (calculated == received) {
            // Process packet based on command
            if (cmd == CMD_SHORT_STATUS) {
              this->parse_status_packet_(this->rx_buffer_.data() + 5, length);
            } else if (cmd == CMD_TEMP_RESPONSE) {
              this->parse_temp_response_(this->rx_buffer_.data() + 5, length);
            }
          } else {
            ESP_LOGW(TAG, "Checksum mismatch: expected 0x%02X, got 0x%02X", calculated, received);
          }
          
          // Clear buffer after processing
          this->rx_buffer_.clear();
        }
      } else {
        // Invalid header, shift buffer
        this->rx_buffer_.erase(this->rx_buffer_.begin());
      }
    }
  }
  
  // Poll AC every 10 seconds for status updates
  uint32_t now = millis();
  if (now - this->last_poll_ > 10000) {
    this->send_poll_packet_();
    this->last_poll_ = now;
  }
}

void TclAcClimate::dump_config() {
  ESP_LOGCONFIG(TAG, "TCL AC Climate:");
  ESP_LOGCONFIG(TAG, "  Beeper: %s", this->beeper_enabled_ ? "ON" : "OFF");
  ESP_LOGCONFIG(TAG, "  Display: %s", this->display_enabled_ ? "ON" : "OFF");
  ESP_LOGCONFIG(TAG, "  Vertical Direction: %d", this->vertical_direction_);
  ESP_LOGCONFIG(TAG, "  Horizontal Direction: %d", this->horizontal_direction_);
  this->check_uart_settings(9600, 1, uart::UART_CONFIG_PARITY_EVEN, 8);
}

climate::ClimateTraits TclAcClimate::traits() {
  auto traits = climate::ClimateTraits();
  
  // Supported modes (VALIDATED from log)
  traits.set_supported_modes({
    climate::CLIMATE_MODE_OFF,
    climate::CLIMATE_MODE_COOL,      // MODE_COOLING - 44x in log
    climate::CLIMATE_MODE_HEAT,      // MODE_HEATING - 6x in log
    climate::CLIMATE_MODE_DRY,       // MODE_DRY - 1x in log
    climate::CLIMATE_MODE_FAN_ONLY,  // MODE_FAN - theoretical
    climate::CLIMATE_MODE_AUTO,      // MODE_AUTO - 1x with ECO in log
  });
  
  // Fan modes (mapped to fan speeds)
  traits.set_supported_fan_modes({
    climate::CLIMATE_FAN_AUTO,     // FAN_SPEED_AUTO
    climate::CLIMATE_FAN_LOW,      // FAN_SPEED_LOW (83% in log)
    climate::CLIMATE_FAN_MEDIUM,   // FAN_SPEED_MEDIUM
    climate::CLIMATE_FAN_HIGH,     // FAN_SPEED_HIGH/MAX
  });
  
  // Presets (special modes)
  traits.set_supported_presets({
    climate::CLIMATE_PRESET_NONE,
    climate::CLIMATE_PRESET_ECO,       // ECO mode (Byte 7 Bit 7)
    climate::CLIMATE_PRESET_BOOST,     // TURBO mode (Byte 8 Bit 6)
    climate::CLIMATE_PRESET_SLEEP,     // SLEEP mode (Byte 19)
    climate::CLIMATE_PRESET_COMFORT,   // QUIET mode (Byte 8 Bit 7)
  });
  
  // Swing modes (combined vertical + horizontal)
  traits.set_supported_swing_modes({
    climate::CLIMATE_SWING_OFF,
    climate::CLIMATE_SWING_VERTICAL,
    climate::CLIMATE_SWING_HORIZONTAL,
    climate::CLIMATE_SWING_BOTH,
  });
  
  // Temperature range (from log: 18°C - 30°C observed)
  traits.set_visual_min_temperature(16.0f);
  traits.set_visual_max_temperature(32.0f);
  traits.set_visual_temperature_step(1.0f);
  traits.set_supports_current_temperature(true);
  
  return traits;
}

void TclAcClimate::control(const climate::ClimateCall &call) {
  // Handle mode change
  if (call.get_mode().has_value()) {
    this->mode = *call.get_mode();
  }
  
  // Handle temperature change
  if (call.get_target_temperature().has_value()) {
    this->target_temperature = *call.get_target_temperature();
  }
  
  // Handle fan mode change
  if (call.get_fan_mode().has_value()) {
    this->fan_mode = *call.get_fan_mode();
  }
  
  // Handle preset change
  if (call.get_preset().has_value()) {
    climate::ClimatePreset preset = *call.get_preset();
    
    // Reset all preset flags
    this->eco_mode_ = false;
    this->turbo_mode_ = false;
    this->quiet_mode_ = false;
    
    // Set the appropriate flag
    switch (preset) {
      case climate::CLIMATE_PRESET_ECO:
        this->eco_mode_ = true;
        // ECO only works with AUTO mode (observed in log)
        if (this->mode != climate::CLIMATE_MODE_OFF) {
          this->mode = climate::CLIMATE_MODE_AUTO;
        }
        break;
      case climate::CLIMATE_PRESET_BOOST:
        this->turbo_mode_ = true;
        break;
      case climate::CLIMATE_PRESET_COMFORT:
        this->quiet_mode_ = true;
        break;
      case climate::CLIMATE_PRESET_SLEEP:
        // Sleep mode is handled separately in packet creation
        break;
      default:
        break;
    }
    
    this->preset = preset;
  }
  
  // Handle swing mode change
  if (call.get_swing_mode().has_value()) {
    this->swing_mode = *call.get_swing_mode();
  }
  
  // Publish updated state
  this->publish_state();
  
  // Send control packet to AC
  if (this->mode != climate::CLIMATE_MODE_OFF) {
    uint8_t packet[SET_PACKET_SIZE];
    this->create_set_packet_(packet);
    this->send_packet_(packet, SET_PACKET_SIZE);
    ESP_LOGD(TAG, "Sent SET packet to AC");
  } else {
    // Send power off packet (simplified set packet with specific flags)
    uint8_t packet[SET_PACKET_SIZE];
    memset(packet, 0, SET_PACKET_SIZE);
    packet[0] = HEADER_MCU_TO_AC_0;
    packet[1] = HEADER_MCU_TO_AC_1;
    packet[2] = HEADER_MCU_TO_AC_2;
    packet[3] = CMD_SET_PARAMS;
    packet[4] = 0x20;  // 32 data bytes
    packet[5] = 0x03;
    packet[6] = 0x01;
    packet[7] = 0x00;  // Mode byte = 0x00 indicates power off (observed as 0x20 in one packet)
    packet[SET_PACKET_SIZE - 1] = this->calculate_checksum_(packet, SET_PACKET_SIZE - 1);
    this->send_packet_(packet, SET_PACKET_SIZE);
    ESP_LOGD(TAG, "Sent POWER OFF packet to AC");
  }
}

void TclAcClimate::create_set_packet_(uint8_t *packet) {
  memset(packet, 0, SET_PACKET_SIZE);
  
  // Header
  packet[0] = HEADER_MCU_TO_AC_0;
  packet[1] = HEADER_MCU_TO_AC_1;
  packet[2] = HEADER_MCU_TO_AC_2;
  
  // Command and length
  packet[3] = CMD_SET_PARAMS;
  packet[4] = 0x20;  // 32 data bytes
  
  // Data payload (starting at offset 5)
  packet[5] = 0x03;
  packet[6] = 0x01;
  
  // Byte 7 (offset 7): Mode byte + FLAGS
  packet[7] = MODE_BASE;  // Start with base 0x04
  
  // Add mode-specific flags (observed patterns from log)
  if (this->mode == climate::CLIMATE_MODE_HEAT) {
    // Heating typically has display ON (6/6 heating packets in log)
    if (this->display_enabled_) {
      packet[7] |= FLAG_DISPLAY_ON;
    }
  } else if (this->mode == climate::CLIMATE_MODE_AUTO) {
    // Auto mode can have ECO
    if (this->eco_mode_) {
      packet[7] |= FLAG_ECO_MODE;
    }
  }
  
  // Add display flag if enabled
  if (this->display_enabled_) {
    packet[7] |= FLAG_DISPLAY_ON;
  }
  
  // Add beeper flag (ON by default - 98% of packets)
  if (this->beeper_enabled_) {
    packet[7] |= FLAG_BEEPER_ON;
  }
  
  // Byte 8 (offset 8): Fan Speed + FLAGS
  packet[8] = this->get_fan_speed_();  // Bits 0-2
  
  // Add speed modifier flags
  if (this->quiet_mode_) {
    packet[8] |= FLAG_QUIET_MODE;
  } else if (this->turbo_mode_) {
    packet[8] |= FLAG_TURBO_MODE;
  }
  
  if (this->health_mode_) {
    packet[8] |= FLAG_HEALTH_MODE;
  }
  
  // Byte 9 (offset 9): Room temperature (hypothesis: 0x56 = 22°C)
  packet[9] = 0x56;
  
  // Byte 10 (offset 10): Swing enable flags
  packet[10] = 0x00;
  
  // Byte 11 (offset 11): Horizontal swing enable
  packet[11] = 0x00;
  
  // Byte 12 (offset 12): Temperature unit (0x00 = Celsius)
  packet[12] = 0x00;
  
  // Byte 13 (offset 13): Constant
  packet[13] = 0x01;
  
  // Byte 19 (offset 19): Sleep mode
  if (this->preset == climate::CLIMATE_PRESET_SLEEP) {
    packet[19] = SLEEP_MODE_1;  // Default to mode 1
  } else {
    packet[19] = SLEEP_MODE_OFF;
  }
  
  // Byte 30 (offset 30): Observed as 0x20
  packet[30] = 0x20;
  
  // Byte 31 (offset 31): Target temperature
  packet[31] = this->celsius_to_raw_(this->target_temperature);
  
  // Byte 32 (offset 32): Vertical direction
  uint8_t v_pos = this->vertical_direction_;
  uint8_t v_swing = VERTICAL_SWING_OFF;
  
  if (this->swing_mode == climate::CLIMATE_SWING_VERTICAL || 
      this->swing_mode == climate::CLIMATE_SWING_BOTH) {
    v_swing = VERTICAL_SWING_FULL;
  }
  
  if (v_pos == 255) {  // Special value from config
    v_pos = VERTICAL_POS_MAX_DOWN;  // Default
    v_swing = VERTICAL_SWING_FULL;
  }
  
  packet[32] = (v_pos & 0x07) | ((v_swing & 0x03) << 3);
  
  // Byte 33 (offset 33): Horizontal direction
  uint8_t h_pos = this->horizontal_direction_;
  uint8_t h_swing = HORIZONTAL_SWING_OFF;
  
  if (this->swing_mode == climate::CLIMATE_SWING_HORIZONTAL || 
      this->swing_mode == climate::CLIMATE_SWING_BOTH) {
    h_swing = HORIZONTAL_SWING_FULL;
  }
  
  if (h_pos == 255) {  // Special value from config
    h_pos = HORIZONTAL_POS_MAX_RIGHT;  // Default
    h_swing = HORIZONTAL_SWING_FULL;
  }
  
  packet[33] = (h_pos & 0x07) | ((h_swing & 0x07) << 3);
  
  // Calculate and set checksum
  packet[SET_PACKET_SIZE - 1] = this->calculate_checksum_(packet, SET_PACKET_SIZE - 1);
}

void TclAcClimate::send_packet_(const uint8_t *packet, size_t length) {
  // Log packet for debugging
  ESP_LOGV(TAG, "Sending packet (%d bytes):", length);
  for (size_t i = 0; i < length; i++) {
    ESP_LOGV(TAG, "  [%02d] 0x%02X", i, packet[i]);
  }
  
  // Send via UART
  this->write_array(packet, length);
  this->flush();
  this->last_transmit_ = millis();
}

void TclAcClimate::send_poll_packet_() {
  uint8_t packet[POLL_PACKET_SIZE] = {
    HEADER_MCU_TO_AC_0,
    HEADER_MCU_TO_AC_1,
    HEADER_MCU_TO_AC_2,
    CMD_POLL,
    0x01,  // Length
    0x00,  // Data
    0x00   // Checksum (will be calculated)
  };
  
  packet[POLL_PACKET_SIZE - 1] = this->calculate_checksum_(packet, POLL_PACKET_SIZE - 1);
  this->send_packet_(packet, POLL_PACKET_SIZE);
  ESP_LOGV(TAG, "Sent POLL packet");
}

uint8_t TclAcClimate::calculate_checksum_(const uint8_t *data, size_t length) {
  // XOR checksum - VALIDATED from log analysis
  uint8_t checksum = 0;
  for (size_t i = 0; i < length; i++) {
    checksum ^= data[i];
  }
  return checksum;
}

void TclAcClimate::parse_status_packet_(const uint8_t *data, size_t length) {
  if (length < 32) {
    ESP_LOGW(TAG, "Status packet too short: %d bytes", length);
    return;
  }
  
  // Parse status data (same structure as set packet)
  // Byte 2 (data[2]): Mode flags
  uint8_t mode_byte = data[2];
  bool display_on = (mode_byte & FLAG_DISPLAY_ON) != 0;
  bool eco_on = (mode_byte & FLAG_ECO_MODE) != 0;
  
  // Byte 3 (data[3]): Speed flags
  uint8_t speed_byte = data[3];
  bool turbo_on = (speed_byte & FLAG_TURBO_MODE) != 0;
  bool quiet_on = (speed_byte & FLAG_QUIET_MODE) != 0;
  
  // Update state
  this->eco_mode_ = eco_on;
  this->turbo_mode_ = turbo_on;
  this->quiet_mode_ = quiet_on;
  
  // Byte 26 (data[26]): Target temperature
  if (data[26] > 0) {
    this->target_temperature = this->raw_to_celsius_(data[26]);
  }
  
  ESP_LOGD(TAG, "Status update - Temp: %.1f°C, ECO: %d, Turbo: %d, Quiet: %d", 
           this->target_temperature, eco_on, turbo_on, quiet_on);
  
  this->publish_state();
}

void TclAcClimate::parse_temp_response_(const uint8_t *data, size_t length) {
  if (length < 4) {
    ESP_LOGW(TAG, "Temp response too short: %d bytes", length);
    return;
  }
  
  // Byte 0: Current temperature (raw - 7 = Celsius)
  // Byte 2: Target temperature (raw - 12 = Celsius)
  uint8_t current_raw = data[0];
  
  if (current_raw > 7) {
    this->current_temperature = this->raw_to_celsius_(current_raw);
    ESP_LOGD(TAG, "Current temperature: %.1f°C (raw: 0x%02X)", this->current_temperature, current_raw);
    this->publish_state();
  }
}

uint8_t TclAcClimate::get_fan_speed_() {
  // Map ESPHome fan modes to TCL fan speeds (validated from log)
  switch (this->fan_mode.value_or(climate::CLIMATE_FAN_LOW)) {
    case climate::CLIMATE_FAN_AUTO:
      return FAN_SPEED_AUTO;
    case climate::CLIMATE_FAN_LOW:
      return FAN_SPEED_LOW;  // Most common (83% in log)
    case climate::CLIMATE_FAN_MEDIUM:
      return FAN_SPEED_MEDIUM;
    case climate::CLIMATE_FAN_HIGH:
      return FAN_SPEED_MAX;  // Use MAX for "high"
    default:
      return FAN_SPEED_LOW;
  }
}

uint8_t TclAcClimate::celsius_to_raw_(float temp) {
  // Formula validated from log: raw = celsius + 12
  int raw = (int)(temp + 12.5f);  // +0.5 for rounding
  if (raw < 0) raw = 0;
  if (raw > 255) raw = 255;
  return (uint8_t)raw;
}

float TclAcClimate::raw_to_celsius_(uint8_t raw) {
  // Formula validated from log: celsius = raw - 12 (for target temp)
  // For current temp in temp_response: celsius = raw - 7
  return (float)raw - 12.0f;
}

}  // namespace tcl_ac
}  // namespace esphome

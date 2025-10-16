#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace tcl_ac {

// Protocol Constants - VALIDATED from UART log analysis (both.txt)

// Packet Headers - CRITICAL: Direction matters!
// MCU → AC: BB 00 01
static const uint8_t HEADER_MCU_TO_AC_0 = 0xBB;
static const uint8_t HEADER_MCU_TO_AC_1 = 0x00;
static const uint8_t HEADER_MCU_TO_AC_2 = 0x01;
// AC → MCU: BB 01 00 (bytes 1-2 swapped!)
static const uint8_t HEADER_AC_TO_MCU_0 = 0xBB;
static const uint8_t HEADER_AC_TO_MCU_1 = 0x01;
static const uint8_t HEADER_AC_TO_MCU_2 = 0x00;

// Commands
static const uint8_t CMD_SET_PARAMS = 0x03;  // MCU → AC: Set parameters (AC responds with same cmd)
static const uint8_t CMD_POLL = 0x04;        // MCU → AC: Poll status
static const uint8_t CMD_TEMP_RESPONSE = 0x05;
static const uint8_t CMD_STATUS_ECHO = 0x06;  // AC → MCU: Status echo (received only)
static const uint8_t CMD_SHORT_STATUS = 0x09;
static const uint8_t CMD_POWER = 0x0A;
static const uint8_t CMD_TIME = 0x0B;

// Packet sizes
static const uint8_t SET_PACKET_SIZE = 38;
static const uint8_t POLL_PACKET_SIZE = 7;

// Fan Speed (Byte 8 Bits 0-2) - VALIDATED: 44x Speed1, 2x Speed3, 1x Speed7
static const uint8_t FAN_SPEED_AUTO = 0;
static const uint8_t FAN_SPEED_LOW = 1;        // 83% in log - DEFAULT
static const uint8_t FAN_SPEED_MEDIUM_LOW = 2;
static const uint8_t FAN_SPEED_MEDIUM = 3;
static const uint8_t FAN_SPEED_MEDIUM_HIGH = 4;
static const uint8_t FAN_SPEED_HIGH = 5;
static const uint8_t FAN_SPEED_VERY_HIGH = 6;
static const uint8_t FAN_SPEED_MAX = 7;

// Byte 7 Flags (Mode + Flags) - VALIDATED
static const uint8_t FLAG_ECO_MODE = 0x80;    // Bit 7 (1x observed)
static const uint8_t FLAG_DISPLAY_ON = 0x40;   // Bit 6 (7x observed)
static const uint8_t FLAG_BEEPER_ON = 0x20;    // Bit 5 (52/53 observed - DEFAULT ON)
static const uint8_t MODE_BASE = 0x04;         // Bits 0-4 base value

// Byte 8 Flags (Speed + Flags) - VALIDATED
static const uint8_t FLAG_QUIET_MODE = 0x80;   // Bit 7 (1x observed)
static const uint8_t FLAG_TURBO_MODE = 0x40;   // Bit 6 (3x observed)
static const uint8_t FLAG_HEALTH_MODE = 0x20;  // Bit 5 (position identified)

// Sleep Mode (Byte 19) - VALIDATED: 48x OFF, 1x Mode1, 2x Mode2
static const uint8_t SLEEP_MODE_OFF = 0;
static const uint8_t SLEEP_MODE_1 = 1;
static const uint8_t SLEEP_MODE_2 = 2;

// Direction Positions - VALIDATED from log
static const uint8_t VERTICAL_POS_LAST = 0;
static const uint8_t VERTICAL_POS_MAX_UP = 1;
static const uint8_t VERTICAL_POS_UP = 2;
static const uint8_t VERTICAL_POS_CENTER = 3;
static const uint8_t VERTICAL_POS_DOWN = 4;
static const uint8_t VERTICAL_POS_MAX_DOWN = 5;  // DEFAULT - 75% in log

static const uint8_t HORIZONTAL_POS_LAST = 0;
static const uint8_t HORIZONTAL_POS_MAX_LEFT = 1;
static const uint8_t HORIZONTAL_POS_LEFT = 2;
static const uint8_t HORIZONTAL_POS_CENTER = 3;
static const uint8_t HORIZONTAL_POS_RIGHT = 4;
static const uint8_t HORIZONTAL_POS_MAX_RIGHT = 5;  // DEFAULT - 60% in log

// Swing Modes
static const uint8_t VERTICAL_SWING_OFF = 0;
static const uint8_t VERTICAL_SWING_SMALL = 1;
static const uint8_t VERTICAL_SWING_MEDIUM = 2;
static const uint8_t VERTICAL_SWING_FULL = 3;

static const uint8_t HORIZONTAL_SWING_OFF = 0;
static const uint8_t HORIZONTAL_SWING_SMALL = 1;
static const uint8_t HORIZONTAL_SWING_MEDIUM = 2;
static const uint8_t HORIZONTAL_SWING_FULL = 4;

class TclAcClimate : public climate::Climate, public uart::UARTDevice, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  // Configuration setters (called from Python code generation)
  void set_beeper_enabled(bool enabled) { beeper_enabled_ = enabled; }
  void set_display_enabled(bool enabled) { display_enabled_ = enabled; }
  void set_vertical_direction(uint8_t direction) { vertical_direction_ = direction; }
  void set_horizontal_direction(uint8_t direction) { horizontal_direction_ = direction; }

  // Climate traits (capabilities)
  climate::ClimateTraits traits() override;

 protected:
  // Climate control implementation
  void control(const climate::ClimateCall &call) override;

  // Packet creation and communication
  void create_set_packet_(uint8_t *packet);
  void send_packet_(const uint8_t *packet, size_t length);
  void send_poll_packet_();
  uint8_t calculate_checksum_(const uint8_t *data, size_t length);
  
  // Packet parsing
  void parse_status_packet_(const uint8_t *data, size_t length);
  void parse_temp_response_(const uint8_t *data, size_t length);
  
  // Helper functions
  uint8_t get_fan_speed_();
  uint8_t celsius_to_raw_(float temp);
  float raw_to_celsius_(uint8_t raw);

  // Configuration
  bool beeper_enabled_{true};      // DEFAULT: ON (98% in log)
  bool display_enabled_{false};    // DEFAULT: OFF (87% in log)
  uint8_t vertical_direction_{VERTICAL_POS_MAX_DOWN};    // DEFAULT (75% in log)
  uint8_t horizontal_direction_{HORIZONTAL_POS_MAX_RIGHT}; // DEFAULT (60% in log)

  // State tracking
  uint32_t last_transmit_{0};
  uint32_t last_poll_{0};
  bool eco_mode_{false};
  bool turbo_mode_{false};
  bool quiet_mode_{false};
  bool health_mode_{false};
  
  // UART buffer
  std::vector<uint8_t> rx_buffer_;
};

}  // namespace tcl_ac
}  // namespace esphome

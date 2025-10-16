"""
ESPHome Custom Component for TCL AC (Realtek RTL8710C UART Protocol)
Based on validated UART log analysis from real TCL AC unit.
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@Kannix2005"]

# Component namespace
tcl_ac_ns = cg.esphome_ns.namespace("tcl_ac")
TclAcClimate = tcl_ac_ns.class_("TclAcClimate", climate.Climate, cg.Component, uart.UARTDevice)

# Configuration keys
CONF_BEEPER = "beeper"
CONF_DISPLAY = "display"
CONF_VERTICAL_DIRECTION = "vertical_direction"
CONF_HORIZONTAL_DIRECTION = "horizontal_direction"

# Validation schema
CONFIG_SCHEMA = (
    climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(TclAcClimate),
            cv.Optional(CONF_BEEPER, default=True): cv.boolean,
            cv.Optional(CONF_DISPLAY, default=False): cv.boolean,
            cv.Optional(CONF_VERTICAL_DIRECTION, default="max_down"): cv.one_of(
                "max_up", "up", "center", "down", "max_down", "swing", lower=True
            ),
            cv.Optional(CONF_HORIZONTAL_DIRECTION, default="max_right"): cv.one_of(
                "max_left", "left", "center", "right", "max_right", "swing", lower=True
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    """Generate C++ code for the component."""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    await uart.register_uart_device(var, config)

    # Set configuration options
    cg.add(var.set_beeper_enabled(config[CONF_BEEPER]))
    cg.add(var.set_display_enabled(config[CONF_DISPLAY]))
    
    # Vertical direction mapping
    vertical_map = {
        "max_up": 1,
        "up": 2,
        "center": 3,
        "down": 4,
        "max_down": 5,
        "swing": 255,
    }
    cg.add(var.set_vertical_direction(vertical_map[config[CONF_VERTICAL_DIRECTION]]))
    
    # Horizontal direction mapping
    horizontal_map = {
        "max_left": 1,
        "left": 2,
        "center": 3,
        "right": 4,
        "max_right": 5,
        "swing": 255,
    }
    cg.add(var.set_horizontal_direction(horizontal_map[config[CONF_HORIZONTAL_DIRECTION]]))

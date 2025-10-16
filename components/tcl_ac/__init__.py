"""
ESPHome Custom Component for TCL AC (Realtek RTL8710C UART Protocol)
Based on validated UART log analysis from real TCL AC unit.
"""

import esphome.codegen as cg
from esphome.components import climate, uart

CODEOWNERS = ["@Kannix2005"]

# Component namespace
tcl_ac_ns = cg.esphome_ns.namespace("tcl_ac")
TclAcClimate = tcl_ac_ns.class_("TclAcClimate", climate.Climate, cg.Component, uart.UARTDevice)

# Configuration keys (exported for climate.py)
CONF_BEEPER = "beeper"
CONF_DISPLAY = "display"
CONF_VERTICAL_DIRECTION = "vertical_direction"
CONF_HORIZONTAL_DIRECTION = "horizontal_direction"

"""Climate component for TCL AC."""
import esphome.codegen as cg
from esphome.components import climate
from . import TclAcClimate, tcl_ac_ns

CODEOWNERS = ["@stefan"]

CONFIG_SCHEMA = climate.CLIMATE_SCHEMA

async def to_code(config):
    """Generate code for climate component."""
    pass  # Handled by main __init__.py

import esphome.codegen as cg
import esphome.config_validation as cv

# Ensure this component is always compiled alongside ethernet
AUTO_LOAD = ["ethernet"]

CONFIG_SCHEMA = cv.Schema({})

async def to_code(config):
    # No build flags required; link-time wrapper enabled in CMakeLists
    pass



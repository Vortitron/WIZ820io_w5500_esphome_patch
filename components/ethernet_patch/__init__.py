import esphome.codegen as cg
import esphome.config_validation as cv

# Ensure this component is always compiled alongside ethernet
AUTO_LOAD = ["ethernet"]

CONFIG_SCHEMA = cv.Schema({})

async def to_code(config):
    # Compile our C++ file
    cg.add_source_files("ethernet_patch.cpp")
    # Remap all call sites to our patched function (we undef inside our TU to avoid recursion)
    cg.add_build_flag("-Desp_eth_mac_new_w5500=esp_eth_mac_new_w5500_patched")



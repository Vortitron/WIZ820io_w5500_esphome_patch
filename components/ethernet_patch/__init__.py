import os
import esphome.codegen as cg
import esphome.config_validation as cv

AUTO_LOAD = ["ethernet"]

CONFIG_SCHEMA = cv.Schema({})

async def to_code(config):
    # Keep only the symbol remap; our C++ TU undefines it locally before calling IDF
    cg.add_build_flag("-Desp_eth_mac_new_w5500=esp_eth_mac_new_w5500_patched")
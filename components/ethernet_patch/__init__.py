import os
import esphome.codegen as cg
import esphome.config_validation as cv

AUTO_LOAD = ["ethernet"]

CONFIG_SCHEMA = cv.Schema({})

async def to_code(config):
    # Intercept all calls to esp_eth_mac_new_w5500 at link time
    cg.add_build_flag("-Wl,--wrap=esp_eth_mac_new_w5500")
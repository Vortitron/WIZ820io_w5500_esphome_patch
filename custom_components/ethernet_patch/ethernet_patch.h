#ifndef ETHERNET_PATCH_H
#define ETHERNET_PATCH_H

#include "esphome/components/ethernet/ethernet_component.h"
#include "esp_eth.h"

extern "C" {
esp_eth_mac_t *esp_eth_mac_new_w5500_patched(const eth_w5500_config_t *config, const eth_mac_config_t *mac_config);
}

#endif

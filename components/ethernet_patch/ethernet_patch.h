#ifndef ETHERNET_PATCH_H
#define ETHERNET_PATCH_H

#include "esp_eth.h"
#include "esp_log.h"
#include "driver/spi_master.h"

#ifdef __cplusplus
extern "C" {
#endif

// W5500 register definitions
#define W5500_VERSIONR 0x0039
#define W5500_ACCESS_MODE_READ 0x00
#define W5500_RWB_OFFSET 2
#define W5500_ADDR_OFFSET 16

// Function to create patched W5500 MAC
esp_eth_mac_t *esp_eth_mac_new_w5500_patched(const eth_w5500_config_t *config, const eth_mac_config_t *mac_config);

#ifdef __cplusplus
}
#endif

#endif

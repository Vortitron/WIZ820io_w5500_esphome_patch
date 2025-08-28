#include "ethernet_patch.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "ethernet_patch";

// Hook into the ESP-IDF W5500 functions
extern "C" {
    // Declare the original ESP-IDF function as weak
    esp_eth_mac_t *esp_eth_mac_new_w5500_original(const eth_w5500_config_t *config, const eth_mac_config_t *mac_config) __attribute__((weak));
}

// Our patched version that will replace the original
extern "C" esp_eth_mac_t *esp_eth_mac_new_w5500(const eth_w5500_config_t *config, const eth_mac_config_t *mac_config) {
    ESP_LOGI(TAG, "W5500 MAC creation hooked - applying 0x82 version patch");

    if (!config || !mac_config) {
        ESP_LOGE(TAG, "Invalid configuration parameters");
        return NULL;
    }
    
    // Create a patched configuration with custom SPI driver
    eth_w5500_config_t patched_config = *config;
    
    // Define our custom SPI functions as static variables to avoid lambda issues
    static void* (*custom_init)(const void*);
    static esp_err_t (*custom_deinit)(void*);
    static esp_err_t (*custom_read)(void*, uint32_t, uint32_t, void*, uint32_t);
    static esp_err_t (*custom_write)(void*, uint32_t, uint32_t, const void*, uint32_t);
    
    // SPI context structure
    typedef struct {
        spi_device_handle_t handle;
        bool version_patched;
    } patch_spi_ctx_t;
    
    // Initialize function pointers
    custom_init = [](const void *cfg) -> void* {
        const eth_w5500_config_t *w5500_cfg = (const eth_w5500_config_t*)cfg;
        
        patch_spi_ctx_t *ctx = (patch_spi_ctx_t*)calloc(1, sizeof(patch_spi_ctx_t));
        if (!ctx) {
            ESP_LOGE(TAG, "Failed to allocate SPI context");
            return NULL;
        }
        
        // Configure SPI device
        spi_device_interface_config_t spi_cfg = *(w5500_cfg->spi_devcfg);
        if (spi_cfg.command_bits == 0 && spi_cfg.address_bits == 0) {
            spi_cfg.command_bits = 16;  // W5500 address phase
            spi_cfg.address_bits = 8;   // W5500 control phase
        }
        
        esp_err_t ret = spi_bus_add_device(w5500_cfg->spi_host_id, &spi_cfg, &ctx->handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
            free(ctx);
            return NULL;
        }
        
        ctx->version_patched = false;
        ESP_LOGI(TAG, "Custom W5500 SPI driver initialized");
        return ctx;
    };
    
    custom_deinit = [](void *ctx_ptr) -> esp_err_t {
        patch_spi_ctx_t *ctx = (patch_spi_ctx_t*)ctx_ptr;
        if (ctx) {
            if (ctx->version_patched) {
                ESP_LOGI(TAG, "W5500 version 0x82 was successfully patched to 0x04");
            }
            spi_bus_remove_device(ctx->handle);
            free(ctx);
        }
        return ESP_OK;
    };
    
    custom_read = [](void *ctx_ptr, uint32_t cmd, uint32_t addr, void *data, uint32_t len) -> esp_err_t {
        patch_spi_ctx_t *ctx = (patch_spi_ctx_t*)ctx_ptr;
        
        spi_transaction_t trans = {
            .flags = len <= 4 ? SPI_TRANS_USE_RXDATA : 0,
            .cmd = cmd,
            .addr = addr,
            .length = 8 * len,
            .rx_buffer = data
        };
        
        esp_err_t ret = spi_device_polling_transmit(ctx->handle, &trans);
        
        if (ret == ESP_OK && (trans.flags & SPI_TRANS_USE_RXDATA) && len <= 4) {
            memcpy(data, trans.rx_data, len);
        }
        
        // Check for version register read and patch it
        if (ret == ESP_OK && len == 1) {
            uint32_t reg_addr = (cmd >> 16) & 0xFFFF;  // Extract register address
            if (reg_addr == 0x0039) {  // W5500 VERSIONR register
                uint8_t *version = (uint8_t*)data;
                if (*version == 0x82) {
                    ESP_LOGI(TAG, "Patching W5500 version: 0x82 -> 0x04");
                    *version = 0x04;
                    ctx->version_patched = true;
                }
            }
        }
        
        return ret;
    };
    
    custom_write = [](void *ctx_ptr, uint32_t cmd, uint32_t addr, const void *data, uint32_t len) -> esp_err_t {
        patch_spi_ctx_t *ctx = (patch_spi_ctx_t*)ctx_ptr;
        
        spi_transaction_t trans = {
            .cmd = cmd,
            .addr = addr,
            .length = 8 * len,
            .tx_buffer = data
        };
        
        return spi_device_polling_transmit(ctx->handle, &trans);
    };
    
    // Apply the custom SPI driver to our config
    patched_config.custom_spi_driver.init = custom_init;
    patched_config.custom_spi_driver.deinit = custom_deinit;
    patched_config.custom_spi_driver.read = custom_read;
    patched_config.custom_spi_driver.write = custom_write;
    patched_config.custom_spi_driver.config = config;
    
    // Try to call the original function if available
    if (esp_eth_mac_new_w5500_original) {
        ESP_LOGI(TAG, "Calling original ESP-IDF W5500 function with patched config");
        return esp_eth_mac_new_w5500_original(&patched_config, mac_config);
    } else {
        ESP_LOGE(TAG, "Original ESP-IDF W5500 function not found");
        ESP_LOGE(TAG, "Make sure esp_eth component is properly linked");
        return NULL;
    }
}

// Alternative function for direct use
extern "C" esp_eth_mac_t *esp_eth_mac_new_w5500_patched(const eth_w5500_config_t *config, const eth_mac_config_t *mac_config) {
    return esp_eth_mac_new_w5500(config, mac_config);
}
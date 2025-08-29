#define ESP_ETH_MAC_NEW_W5500_NO_REMAP
#include "ethernet_patch.h"
#include <string.h>
#include <stdlib.h>

// Avoid global -D remap inside this TU when we need to call the real IDF API
#ifdef esp_eth_mac_new_w5500
#undef esp_eth_mac_new_w5500
#endif
extern "C" esp_eth_mac_t *esp_eth_mac_new_w5500(const eth_w5500_config_t *config, const eth_mac_config_t *mac_config);

static const char *TAG = "ethernet_patch";

// SPI context structure used by our custom driver
typedef struct {
    spi_device_handle_t handle;
    bool version_patched;
} patch_spi_ctx_t;

static void *patch_spi_init(const void *cfg) {
    const eth_w5500_config_t *w5500_cfg = (const eth_w5500_config_t*)cfg;

    patch_spi_ctx_t *ctx = (patch_spi_ctx_t*)calloc(1, sizeof(patch_spi_ctx_t));
    if (!ctx) {
        ESP_LOGE(TAG, "Failed to allocate SPI context");
        return NULL;
    }

    spi_device_interface_config_t spi_cfg = *(w5500_cfg->spi_devcfg);
    if (spi_cfg.command_bits == 0 && spi_cfg.address_bits == 0) {
        spi_cfg.command_bits = 16; // W5500 address phase
        spi_cfg.address_bits = 8;  // W5500 control phase
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
}

static esp_err_t patch_spi_deinit(void *ctx_ptr) {
    patch_spi_ctx_t *ctx = (patch_spi_ctx_t*)ctx_ptr;
    if (ctx) {
        if (ctx->version_patched) {
            ESP_LOGI(TAG, "W5500 version 0x82 was patched to 0x04");
        }
        spi_bus_remove_device(ctx->handle);
        free(ctx);
    }
    return ESP_OK;
}

static esp_err_t patch_spi_read(void *ctx_ptr, uint32_t cmd, uint32_t addr, void *data, uint32_t len) {
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

    // Patch version register reads
    if (ret == ESP_OK && len == 1) {
        uint32_t reg_addr = (cmd & 0xFFFF); // Extract register address
        if (reg_addr == 0x0039) { // VERSIONR
            uint8_t *version = (uint8_t*)data;
            if (*version == 0x82) {
                ESP_LOGW(TAG, "Patching W5500 VERSIONR: 0x82 -> 0x04");
                *version = 0x04;
                ctx->version_patched = true;
            }
        }
    }

    return ret;
}

static esp_err_t patch_spi_write(void *ctx_ptr, uint32_t cmd, uint32_t addr, const void *data, uint32_t len) {
    patch_spi_ctx_t *ctx = (patch_spi_ctx_t*)ctx_ptr;

    spi_transaction_t trans = {
        .cmd = cmd,
        .addr = addr,
        .length = 8 * len,
        .tx_buffer = data
    };

    return spi_device_polling_transmit(ctx->handle, &trans);
}

// Call site remap target. Other translation units will be forced to call this
extern "C" esp_eth_mac_t *esp_eth_mac_new_w5500_patched(const eth_w5500_config_t *config, const eth_mac_config_t *mac_config) {
    if (!config || !mac_config) {
        ESP_LOGE(TAG, "Invalid configuration parameters");
        return NULL;
    }

    ESP_LOGI(TAG, "Using patched W5500 MAC factory (accept VERSIONR 0x82)");

    eth_w5500_config_t cfg = *config;
    cfg.custom_spi_driver.init = patch_spi_init;
    cfg.custom_spi_driver.deinit = patch_spi_deinit;
    cfg.custom_spi_driver.read = patch_spi_read;
    cfg.custom_spi_driver.write = patch_spi_write;
    cfg.custom_spi_driver.config = &cfg;

    // Call the real IDF function here (our file disables the remap macro)
    return esp_eth_mac_new_w5500(&cfg, mac_config);
}

// Linker wrap to intercept calls to IDF's esp_eth_mac_new_w5500
extern "C" esp_eth_mac_t *__real_esp_eth_mac_new_w5500(const eth_w5500_config_t *config, const eth_mac_config_t *mac_config);
extern "C" esp_eth_mac_t *__wrap_esp_eth_mac_new_w5500(const eth_w5500_config_t *config, const eth_mac_config_t *mac_config) {
    if (!config || !mac_config) {
        ESP_LOGE(TAG, "Invalid configuration parameters (wrap)");
        return NULL;
    }

    ESP_LOGI(TAG, "Wrapping esp_eth_mac_new_w5500 to accept VERSIONR 0x82");

    eth_w5500_config_t cfg = *config;
    cfg.custom_spi_driver.init = patch_spi_init;
    cfg.custom_spi_driver.deinit = patch_spi_deinit;
    cfg.custom_spi_driver.read = patch_spi_read;
    cfg.custom_spi_driver.write = patch_spi_write;
    cfg.custom_spi_driver.config = &cfg;

    return __real_esp_eth_mac_new_w5500(&cfg, mac_config);
}

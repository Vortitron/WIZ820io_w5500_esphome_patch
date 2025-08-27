#include "ethernet_patch.h"
#include "esp_log.h"

static const char *TAG = "ethernet_patch";

extern "C" esp_eth_mac_t *esp_eth_mac_new_w5500_patched(const eth_w5500_config_t *config, const eth_mac_config_t *mac_config) {
    // Original function pointer (dynamically resolved)
    static esp_eth_mac_t *(*original_new_w5500)(const eth_w5500_config_t *, const eth_mac_config_t *) =
        (esp_eth_mac_t *(*)(const eth_w5500_config_t *, const eth_mac_config_t *))esp_eth_mac_new_w5500;

    if (!config || !mac_config) {
        ESP_LOGE(TAG, "Invalid configuration");
        return nullptr;
    }

    // Call the original function
    esp_eth_mac_t *mac = original_new_w5500(config, mac_config);
    if (!mac) {
        ESP_LOGW(TAG, "Original W5500 MAC creation failed, attempting to bypass version check");
        // Manually initialize with a dummy check bypass (simplified)
        w5500_t *w5500 = (w5500_t *)config->priv;
        uint8_t version = w5500_read_byte(w5500, 0x0039); // VERSIONR
        ESP_LOGI(TAG, "Detected W5500 version: 0x%02x, accepting 0x82", version);
        if (version == 0x82) {
            // Force success by reattempting with a patched config
            config->priv = w5500; // Ensure priv is intact
            mac = original_new_w5500(config, mac_config);
            if (mac) {
                ESP_LOGI(TAG, "Bypassed version check, MAC initialized");
            } else {
                ESP_LOGE(TAG, "Patch failed, MAC still null");
            }
        }
    }
    return mac;
}

# W5500 Ethernet Patch for ESPHome

This external component patches the ESP-IDF W5500 Ethernet driver to accept W5500 modules that return version ID `0x82` instead of the expected `0x04`.

## Problem

Some W5500 Ethernet modules (particularly certain clones or hardware revisions) return a version register value of `0x82` instead of `0x04`. The standard ESP-IDF W5500 driver rejects these modules during initialisation, causing ethernet connectivity to fail.

## Solution

This patch intercepts the W5500 MAC creation process and provides a custom SPI driver that:

1. Allows the module to initialise normally
2. Intercepts reads from the version register (0x0039)
3. Patches version `0x82` to `0x04` when read by the driver
4. Maintains full functionality for all other operations

## Usage

Add this external component to your ESPHome configuration:

```yaml
external_components:
  - source: github://Vortitron/WIZ820io_w5500_esphome_patch
    components: [ethernet_patch]

ethernet:
  type: W5500
  clk_pin: GPIO18
  mosi_pin: GPIO23
  miso_pin: GPIO19
  cs_pin: GPIO5
  interrupt_pin: GPIO4
  reset_pin: GPIO2
```

## How It Works

The patch uses ESP-IDF's custom SPI driver feature for the W5500 component. When ESPHome calls `esp_eth_mac_new_w5500()`, our patched version:

1. Creates a custom SPI driver context
2. Wraps all SPI read/write operations
3. Specifically watches for version register reads
4. Patches the version value from `0x82` to `0x04` when detected
5. Passes through all other operations unchanged

## Compatibility

- ✅ ESP32 family microcontrollers
- ✅ ESPHome 2023.x and later
- ✅ W5500 modules with version ID 0x04 (standard)
- ✅ W5500 modules with version ID 0x82 (patched)

## Logging

When the patch is active, you'll see log messages like:

```
[I][ethernet_patch:xxx] W5500 MAC creation hooked - applying 0x82 version patch
[I][ethernet_patch:xxx] Custom W5500 SPI driver initialized
[I][ethernet_patch:xxx] Patching W5500 version: 0x82 -> 0x04
```

This confirms the patch is working and your 0x82 module is being accepted.

idf_component_register(
    SRCS "ethernet_patch.cpp"
    INCLUDE_DIRS "."
    REQUIRES "esp_eth" "driver" "esp_common" "freertos"
    PRIV_REQUIRES "spi_flash"
)

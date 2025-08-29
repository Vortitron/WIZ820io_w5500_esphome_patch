idf_component_register(
    SRCS "ethernet_patch.cpp"
    INCLUDE_DIRS "."
    REQUIRES "esp_eth" "driver" "esp_common" "freertos"
    PRIV_REQUIRES "spi_flash"
)

# Ensure our wrapper function is used if supported by the linker
target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=esp_eth_mac_new_w5500")

# Also add to global link options to guarantee wrap applies at final link
idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=esp_eth_mac_new_w5500" APPEND)

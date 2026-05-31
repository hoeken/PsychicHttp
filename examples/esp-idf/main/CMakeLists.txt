# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES
    PsychicHttp
    bblanchon__arduinojson
    esp_event
    esp_netif
    esp_timer
    esp_wifi
    espressif__mdns
    joltwallet__littlefs
    nvs_flash)

target_compile_definitions(${COMPONENT_TARGET} PRIVATE -DPSYCHIC_HTTP_SUPPRESS_HTTPS_WARNING)

littlefs_create_partition_image(littlefs ${project_dir}/data FLASH_IN_PROJECT)

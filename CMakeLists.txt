set(COMPONENT_SRCDIRS
    "src"
)

set(COMPONENT_ADD_INCLUDEDIRS
    "src"
)

# Detect Arduino framework automatically: if the arduino component is present
# in the project's build (i.e. the user has it in their EXTRA_COMPONENT_DIRS or
# managed_components), add it as a dep and the ARDUINO define flows through.
# No manual flag needed — pure IDF projects just don't include the component.
idf_build_get_property(build_components BUILD_COMPONENTS)

if("arduino" IN_LIST build_components)
    set(COMPONENT_REQUIRES
        "arduino"
        "esp_rom"
        "esp_https_server"
        "esp_http_server"
        "mbedtls"
        "arduinojson"
    )
else()
    set(COMPONENT_REQUIRES
        "esp_rom"
        "esp_https_server"
        "esp_http_server"
        "esp_netif"
        "mbedtls"
        "arduinojson"
    )
endif()

register_component()

target_compile_definitions(${COMPONENT_TARGET} PUBLIC -DESP32)
target_compile_options(${COMPONENT_TARGET} PRIVATE -fno-rtti)

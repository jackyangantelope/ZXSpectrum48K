function(mac_link_for_loader target)
    set(_override_dir "${CMAKE_CURRENT_BINARY_DIR}/loader_linker")
    file(MAKE_DIRECTORY "${_override_dir}")
    file(WRITE "${_override_dir}/pico_flash_region.ld"
        "FLASH(rx) : ORIGIN = 0x10080000, LENGTH = 0x00F80000\n")
    pico_add_linker_script_override_path(${target} "${_override_dir}")
    target_compile_definitions(${target} PRIVATE
        JACK_APP_BASE=0x10080000 PICO_FLASH_SIZE_BYTES=0x01000000)
    set_target_properties(${target} PROPERTIES
        PICOTOOL_EXTRA_UF2_ARGS "--platform;rp2350")
endfunction()

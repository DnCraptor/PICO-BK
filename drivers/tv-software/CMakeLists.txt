add_library(tv-software INTERFACE)

target_sources(tv-software INTERFACE ${CMAKE_CURRENT_LIST_DIR}/tv-software.c)

target_link_libraries(tv-software INTERFACE
        hardware_pio
        hardware_clocks
        hardware_dma
        hardware_irq
        vga-nextgen          # provides get_graphics_buffer(), vsync_ptr, enum graphics_mode_t
)

target_include_directories(tv-software INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
)


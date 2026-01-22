# Library definitions for pico-miniboy

# System Config Library
add_library(system_config STATIC
    system_config/system_config.c
)
target_include_directories(system_config PUBLIC
    system_config
)
target_link_libraries(system_config PUBLIC
    pico_stdlib
    hardware_clocks
    hardware_vreg
)

# Display Library
add_library(display STATIC
    display/display_driver.c
    display/transport_pio.c
    display/dma_mem.c
)
pico_generate_pio_header(display ${CMAKE_CURRENT_LIST_DIR}/display/spi.pio)
target_include_directories(display PUBLIC
    display
)
target_link_libraries(display PUBLIC
    pico_stdlib
    hardware_spi
    hardware_dma
    hardware_gpio
    hardware_pio
    system_config
)

# Graphics Library
add_library(graphics STATIC
    graphics/framebuffer.c
    graphics/render_service.c
    graphics/font.c
)
target_include_directories(graphics PUBLIC
    graphics
)
target_link_libraries(graphics PUBLIC
    pico_stdlib
    pico_multicore
    display
)

# Profiler Library
add_library(profiler STATIC
    profiler/profiler.c
)
target_include_directories(profiler PUBLIC
    profiler
)
target_link_libraries(profiler PUBLIC
    pico_stdlib
    graphics
)

# Core Engine Library (MiniBoy Engine)
add_library(miniboy_core STATIC
    core/miniboy_engine.c
)
target_include_directories(miniboy_core PUBLIC
    core
)
target_link_libraries(miniboy_core PUBLIC
    pico_stdlib
    profiler
    graphics
    display
    system_config
)

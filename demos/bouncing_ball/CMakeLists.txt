# Bouncing Ball Demo

add_executable(bouncing_ball main.c)

pico_set_program_name(bouncing_ball "bouncing_ball")
pico_set_program_version(bouncing_ball "0.1")

# Enable USB stdio
pico_enable_stdio_uart(bouncing_ball 0)
pico_enable_stdio_usb(bouncing_ball 1)

# Link libraries
target_link_libraries(bouncing_ball
    pico_stdlib
    system_config
    display
    graphics
    profiler
)

pico_add_extra_outputs(bouncing_ball)

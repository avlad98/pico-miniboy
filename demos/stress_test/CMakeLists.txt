# Stress Test Demo

add_executable(stress_test main.c)

pico_set_program_name(stress_test "stress_test")
pico_set_program_version(stress_test "0.1")

# Enable USB stdio
pico_enable_stdio_uart(stress_test 0)
pico_enable_stdio_usb(stress_test 1)

# Link libraries
target_link_libraries(stress_test
    pico_stdlib
    miniboy_core
)

pico_add_extra_outputs(stress_test)

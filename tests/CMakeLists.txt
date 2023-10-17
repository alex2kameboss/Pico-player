project(sd_test C CXX ASM)
add_executable(sd_test sd_test.c)
target_link_libraries(sd_test fatfs pico_stdlib)

pico_sdk_init()

# enable uart@115200
pico_enable_stdio_usb(sd_test 1)
pico_enable_stdio_uart(sd_test 0)

pico_add_extra_outputs(sd_test)

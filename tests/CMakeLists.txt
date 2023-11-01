project(sd_test C CXX ASM)
add_executable(sd_test sd_test.cpp)
target_link_libraries(sd_test fatfs pico_stdlib hardware_spi)
target_include_directories(sd_test PRIVATE ../libs/include)

# enable uart@115200
pico_enable_stdio_usb(sd_test 1)
pico_enable_stdio_uart(sd_test 0)

pico_add_extra_outputs(sd_test)

project(sd_test C CXX ASM)
add_executable(sd_test sd_test.cpp)
target_link_libraries(sd_test fatfs pico_stdlib hardware_spi)

# enable uart@115200
pico_enable_stdio_usb(sd_test 1)
pico_enable_stdio_uart(sd_test 0)

pico_add_extra_outputs(sd_test)


project(mp3_test C CXX ASM)
add_executable(mp3_test mp3_test.cpp)
target_link_libraries(mp3_test fatfs pico_stdlib pico_audio_i2s)

# enable uart@115200
pico_enable_stdio_usb(mp3_test 1)
pico_enable_stdio_uart(mp3_test 0)

pico_add_extra_outputs(mp3_test)


add_compile_definitions(USE_AUDIO_I2S PICO_AUDIO_I2S_MONO_INPUT)
project(i2s_test C CXX ASM)
add_executable(i2s_test i2s_test.c)
target_link_libraries(i2s_test pico_stdlib pico_audio_i2s)

# enable uart@115200
pico_enable_stdio_usb(i2s_test 1)
pico_enable_stdio_uart(i2s_test 0)

pico_add_extra_outputs(i2s_test)
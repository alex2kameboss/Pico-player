# FATFS lib
project(fatfs C CXX ASM)
add_library(fatfs STATIC src/fatfs/ff.c src/fatfs/sdmm.c)
target_include_directories(fatfs PRIVATE include/fatfs)
target_link_libraries(fatfs pico_stdlib hardware_spi)

# mp3_decoder
project(mp3_decoder CXX)
add_library(mp3_decoder STATIC src/mp3_decoder/mp3_decoder.cpp)
target_include_directories(mp3_decoder PRIVATE include/mp3_decoder)
target_link_libraries(mp3_decoder pico_stdlib)

# messages
project(messages CXX)
add_library(messages STATIC src/messages/messages.cpp src/messages/utils.cpp)
target_include_directories(messages PRIVATE include/messages)
target_include_directories(messages PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
target_compile_definitions(messages PRIVATE NO_SYS=0)
target_link_libraries(messages pico_cyw43_arch_lwip_sys_freertos FreeRTOS-Kernel FreeRTOS-Kernel-Heap4)

# arduino libmad
add_subdirectory(externals/arduino-libmad-0.7.1)

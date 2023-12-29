cmake_minimum_required(VERSION 3.13)

file(MAKE_DIRECTORY "build")

# Pull in SDK (must be before project)
set(PICO_BOARD pico_w)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

# Pull in FreeRTOS
include($ENV{FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(app C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_executable(app main.cpp)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}
                                PRIVATE ${CMAKE_CURRENT_LIST_DIR}/libs/include)
 
# pull in common dependencies
target_link_libraries(app 
    pico_cyw43_arch_lwip_sys_freertos
    fatfs 
    pico_stdlib 
    pico_lwip_iperf
    FreeRTOS-Kernel 
    FreeRTOS-Kernel-Heap4 
    pico_audio_i2s 
    mp3_decoder
    messages)

target_compile_definitions(app PRIVATE
    WIFI_SSID=\"eror\"
    WIFI_PASSWORD=\"AnaAre1Mere\"
    NO_SYS=0)

# enable uart@115200
pico_enable_stdio_usb(app 1)
pico_enable_stdio_uart(app 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(app)

include_directories(libs/include)
include_directories(libs/externals/arduino-libmad-0.7.1/src)

# Include Libs
add_subdirectory(libs)

# Include tests
add_subdirectory(tests)

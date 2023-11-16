# FATFS lib
project(fatfs C CXX ASM)
add_library(fatfs STATIC src/fatfs/ff.c src/fatfs/sdmm.c)
target_include_directories(fatfs PRIVATE include/fatfs)
target_link_libraries(fatfs pico_stdlib hardware_spi)
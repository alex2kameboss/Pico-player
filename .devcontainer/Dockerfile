FROM ubuntu:22.04

RUN apt update
RUN apt install -y cmake git
RUN apt install -y gcc-arm-none-eabi
RUN apt install -y python3-dev build-essential

RUN mkdir freertos-pico
WORKDIR /freertos-pico

RUN git clone https://github.com/RaspberryPi/pico-sdk --recurse-submodules
RUN git clone -b smp https://github.com/FreeRTOS/FreeRTOS-Kernel --recurse-submodules
ENV PICO_SDK_PATH /freertos-pico/pico-sdk
ENV FREERTOS_KERNEL_PATH /freertos-pico/FreeRTOS-Kernel

RUN git clone https://github.com/raspberrypi/openocd.git --branch rp2040-v0.12.0 --depth=1 --no-single-branch
RUN apt install -y automake autoconf texinfo libtool libftdi-dev libusb-1.0-0-dev gdb-multiarch pkg-config
WORKDIR /freertos-pico/openocd
RUN ./bootstrap
RUN ./configure --enable-picoprobe
RUN make -j4
RUN make install

WORKDIR /workspace

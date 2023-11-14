FROM ubuntu:22.04

RUN apt update
RUN apt install -y cmake git
RUN apt install -y gcc-arm-none-eabi
RUN apt install -y python3-dev build-essential

RUN mkdir freertos-pico
WORKDIR freertos-pico

RUN git clone https://github.com/RaspberryPi/pico-sdk --recurse-submodules
RUN git clone -b smp https://github.com/FreeRTOS/FreeRTOS-Kernel --recurse-submodules
RUN git clone https://github.com/raspberrypi/pico-extras.git --recurse-submodules
ENV PICO_SDK_PATH /freertos-pico/pico-sdk
ENV FREERTOS_KERNEL_PATH /freertos-pico/FreeRTOS-Kernel
ENV PICO_EXTRAS_PATH /freertos-pico/pico-extras
WORKDIR /workspace

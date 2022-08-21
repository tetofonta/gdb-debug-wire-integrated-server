cmake_minimum_required(VERSION 3.22)
project(avrtest C CXX ASM)

set(F_CPU 16000000)
set(MCU atmega328p)

set(CMAKE_BUILD_TYPE Debug)
set(ADDITIONAL_OPTIMIZATIONS "-flto -fdata-sections -mrelax -fpack-struct -fshort-enums -fno-inline-small-functions -fno-strict-aliasing -funsigned-char -funsigned-bitfields -ffunction-sections")
set(WARNINGS "-Wall -Wstrict-prototypes -Wno-array-bounds ")
include(cmake/env.cmake)

include_directories(./includes)

add_library(rtt STATIC src/rtt/rtt.c)
target_link_options(rtt PUBLIC -Wdiscarded-qualifiers)

avrtarget(avrtest /dev/ttyACM0 src/main.c)
target_link_libraries(avrtest-elf rtt)

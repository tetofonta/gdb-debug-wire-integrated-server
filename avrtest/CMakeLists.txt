cmake_minimum_required(VERSION 3.22)
project(avrtest C CXX ASM)

set(F_CPU 16000000)
set(MCU atmega328p)

set(OPTIMIZATION g)
set(ADDITIONAL_OPTIMIZATIONS "-flto -fdata-sections -mrelax -fpack-struct -fshort-enums -fno-inline-small-functions -fno-strict-aliasing -funsigned-char -funsigned-bitfields -ffunction-sections")
set(WARNINGS "-Wall -Wstrict-prototypes -Wno-array-bounds ")
include(cmake/env.cmake)

avrtarget(avrtest main.c)

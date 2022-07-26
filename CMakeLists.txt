cmake_minimum_required(VERSION 3.22)
project(Ardwino C CXX ASM)

set(MCU atmega16u2)
set(ARCH AVR8)
set(F_CPU 16000000)
set(F_USB 16000000)
set(OPTIMIZATION z)

set(CMAKE_C_FLAGS "")

set(LUFA_ROOT_PATH ${CMAKE_SOURCE_DIR}/lufa/LUFA)

set(LUFA_SRC_USB_COMMON
        ${LUFA_ROOT_PATH}/Drivers/USB/Core/${ARCH}/USBController_${ARCH}.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Core/${ARCH}/USBInterrupt_${ARCH}.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Core/ConfigDescriptors.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Core/Events.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Core/USBTask.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Class/Common/HIDParser.c
        )
set(LUFA_SRC_USB_HOST
        ${LUFA_ROOT_PATH}/Drivers/USB/Core/${ARCH}/Host_${ARCH}.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Core/${ARCH}/Pipe_${ARCH}.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Core/${ARCH}/PipeStream_${ARCH}.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Core/HostStandardReq.c
        )
set(LUFA_SRC_USB_DEVICE
        ${LUFA_ROOT_PATH}/Drivers/USB/Core/${ARCH}/Device_${ARCH}.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Core/${ARCH}/Endpoint_${ARCH}.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Core/${ARCH}/EndpointStream_${ARCH}.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Core/DeviceStandardReq.c
        )
set(LUFA_SRC_USBCLASS_DEVICE
        ${LUFA_ROOT_PATH}/Drivers/USB/Class/Device/AudioClassDevice.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Class/Device/CCIDClassDevice.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Class/Device/CDCClassDevice.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Class/Device/HIDClassDevice.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Class/Device/MassStorageClassDevice.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Class/Device/MIDIClassDevice.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Class/Device/PrinterClassDevice.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Class/Device/RNDISClassDevice.c
        )
set(LUFA_SRC_USBCLASS_HOST
        ${LUFA_ROOT_PATH}/Drivers/USB/Class/Host/AndroidAccessoryClassHost.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Class/Host/AudioClassHost.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Class/Host/CDCClassHost.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Class/Host/HIDClassHost.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Class/Host/MassStorageClassHost.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Class/Host/MIDIClassHost.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Class/Host/PrinterClassHost.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Class/Host/RNDISClassHost.c
        ${LUFA_ROOT_PATH}/Drivers/USB/Class/Host/StillImageClassHost.c
        )
set(LUFA_SRC_USB ${LUFA_SRC_USB_COMMON} ${LUFA_SRC_USB_DEVICE} src/include/gdb/rtt.h src/include/gdb/pstr.h)
list(SORT LUFA_SRC_USB)

set(LUFA_SRC_USBCLASS ${LUFA_SRC_USBCLASS_DEVICE} ${LUFA_SRC_USBCLASS_HOST})
set(LUFA_SRC_TEMPERATURE ${LUFA_ROOT_PATH}/Drivers/Board/Temperature.c)
set(LUFA_SRC_SERIAL ${LUFA_ROOT_PATH}/Drivers/Peripheral/${ARCH}/Serial_${ARCH}.c)
set(LUFA_SRC_TWI ${LUFA_ROOT_PATH}/Drivers/Peripheral/${ARCH}/TWI_${ARCH}.c)

if (${ARCH} EQUAL "UC3")
    set(LUFA_SRC_PLATFORM
            ${LUFA_ROOT_PATH}/Platform/UC3/Exception.S
            ${LUFA_ROOT_PATH}/Platform/UC3/InterruptManagement.c)
else ()
    set(LUFA_SRC_PLATFORM "")
endif ()


set(LUFA_SRC
        ${LUFA_SRC_USB}
        ${LUFA_SRC_USBCLASS}
        ${LUFA_SRC_TEMPERATURE}
        ${LUFA_SRC_SERIAL}
        ${LUFA_SRC_TWI}
        ${LUFA_SRC_PLATFORM}
        )

include_directories(${LUFA_ROOT_PATH} src/Config src/include)

add_compile_definitions(F_CPU=${F_CPU})
add_compile_definitions(F_USB=${F_USB})
add_compile_definitions(DMBS_ARCH_${ARCH})
add_compile_definitions(USE_LUFA_CONFIG_HEADER)

set(C_STANDARD gnu99)
set(CPP_STANDARD gnu++11)
set(DEBUG_FORMAT dwarf-2)
set(DEBUG_LEVEL 2)

set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_ASM_COMPILER avr-gcc)
set(CMAKE_CXX_COMPILER avr-g++)
set(USE_LTO "-flto")

set(CMAKE_C_FLAGS "-pipe ${USE_LTO} -fdata-sections -mrelax -g${DEBUG_FORMAT} -Wno-array-bounds -fno-jump-tables -fpack-struct -mmcu=${MCU} -fshort-enums -fno-inline-small-functions -g${DEBUG_LEVEL} -Wall -fno-strict-aliasing -funsigned-char -funsigned-bitfields -ffunction-sections -O${OPTIMIZATION} -Wstrict-prototypes")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions -fno-threadsafe-statics")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp")
set(CMAKE_LINK_LIBRARY_FLAG "-lm ${USE_LTO} -Wl,--cref -Wl,--gc-sections -Wl,--relax -mmcu=${MCU} -Oz")

add_library(lufa STATIC ${LUFA_SRC_USB})
add_library(cdc STATIC src/usb/usb_cdc.c src/usb/Descriptors.c)
target_link_libraries(cdc lufa)

add_library(dw_devices STATIC src/dw/devices.c)
add_library(gdb_commands STATIC
        src/gdb/commands/gdb_cmd_query.c
        src/gdb/commands/gdb_cmd_v.c
        src/gdb/commands/gdb_cmd_read_registers.c
        src/gdb/commands/gdb_cmd_write_registers.c
        src/gdb/commands/gdb_cmd_end.c
        src/gdb/commands/gdb_cmd_read_memory.c
        src/gdb/commands/gdb_cmd_breakpoint.c
        src/gdb/commands/gdb_cmd_write_memory.c)
add_library(gdb STATIC src/gdb/gdb.c src/gdb/utils.c src/gdb/rtt.c src/gdb/consts.c)
target_link_libraries(gdb gdb_commands)
add_library(stk500 STATIC src/stk500/isp_pgm.c src/stk500/isp.c src/stk500/utils.c)
add_executable(integrated_server_elf
        src/main.c
        src/usb/usb_events.c
        src/dw/debug_wire_ll.c
        src/dw/phisical/open_drain_serial.c
        src/panic.c
        src/dw/debug_wire.c
        src/user_button/user_button.c src/avr.c)
target_link_libraries(integrated_server_elf lufa cdc dw_devices gdb stk500)

add_custom_target(integrated_server
        COMMAND avr-objdump -d \"${CMAKE_BINARY_DIR}/integrated_server_elf\" > \"${CMAKE_BINARY_DIR}/integrated-server-asm.txt\";
        COMMAND avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature \"${CMAKE_BINARY_DIR}/integrated_server_elf\" \"${CMAKE_BINARY_DIR}/integrated_server.flash.hex\";
        COMMAND avr-size --mcu="${MCU}" -C \"${CMAKE_BINARY_DIR}/integrated_server_elf\";
        DEPENDS integrated_server_elf
        )

add_custom_target(upload COMMAND avrdude -c ft232r -p ${MCU} -U \"flash:w:${CMAKE_BINARY_DIR}/integrated_server.flash.hex\" USES_TERMINAL DEPENDS integrated_server)
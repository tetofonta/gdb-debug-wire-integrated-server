if(NOT DEFINED F_CPU)
    message(FATAL_ERROR "No frequency specified")
endif()

if(NOT DEFINED MCU)
    message(FATAL_ERROR "No mcu specified")
endif()

if(DCMAKE_BUILD_TYPE EQUAL Debug)
    set(OPTIMIZATION g)
else()
    set(OPTIMIZATION z)
endif()

set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_ASM_COMPILER avr-gcc)
set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_OBJCOPY avr-objcopy)
set(CMAKE_SIZE avr-size)

set(C_STANDARD gnu17)
set(CPP_STANDARD gnu++17)
set(DEBUG_FORMAT dwarf-2)
set(DEBUG_LEVEL 2)

set(CMAKE_C_FLAGS "-pipe -g${DEBUG_FORMAT} -g${DEBUG_LEVEL} -mmcu=${MCU} -O${OPTIMIZATION} ${ADDITIONAL_OPTIMIZATIONS} ${WARNINGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions -fno-threadsafe-statics")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp")
set(CMAKE_LINK_LIBRARY_FLAG "-lm -Wl,--cref -Wl,--gc-sections -Wl,--relax -mmcu=${MCU} -O${OPTIMIZATION} ${ADDITIONAL_OPTIMIZATIONS}")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_compile_definitions(F_CPU=${F_CPU})
add_compile_definitions(F_USB=${F_USB})
add_compile_definitions(__AVR__MCU__=${MCU})
add_compile_definitions(BUILD_TYPE=${CMAKE_BUILD_TYPE})

function(avrtarget targetName port)
    add_executable("${targetName}-elf" ${ARGN})
    target_link_options("${targetName}-elf" PUBLIC -Wl,--verbose -Xlinker -Map=${CMAKE_BINARY_DIR}/${targetName}.map -T ${CMAKE_SOURCE_DIR}/cmake/linker/${MCU}.ld)
    add_custom_target(
        "${targetName}-bin" 
        COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom -R .fuse -R .lock -R .signature '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${targetName}-elf' '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${targetName}.hex';
        COMMAND ${CMAKE_OBJCOPY} -O ihex -j .eeprom '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${targetName}-elf' '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${targetName}.eeprom.hex';
        COMMAND ${CMAKE_OBJCOPY} -O ihex -j .fuse '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${targetName}-elf' '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${targetName}.fuse.hex';
        COMMAND ${CMAKE_OBJCOPY} -O ihex -j .lock '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${targetName}-elf' '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${targetName}.lock.hex';
        COMMAND ${CMAKE_SIZE} --mcu=${MCU} -C '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${targetName}-elf';
        DEPENDS "${targetName}-elf"
    )
    add_custom_target(
        "${targetName}-upload"
        COMMAND python ../../host_software/flash.py --port ${port} --flash '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${targetName}' --mcu ${MCU}
        USES_TERMINAL
        DEPENDS "${targetName}-bin"
    )
endfunction()
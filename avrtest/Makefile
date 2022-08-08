elf: main.c
	avr-gcc main.c -o main.elf -ggdb -Os -DF_CPU=16000000 -mmcu=atmega328p -Wl,--verbose -Xlinker -Map=output.map # -T linker.ld
	avr-size -A main.elf

fw_image: main.elf
	avr-objcopy -O binary -R .eeprom main.elf main.flash.bin

all: elf fw_image
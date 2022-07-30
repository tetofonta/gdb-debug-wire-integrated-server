#!/usr/bin/env sh
set -e
ELF_FILE="$1"
OUT_DIR=$(dirname "$ELF_FILE")
IMAGE_BASE_NAME="${CARGO_PKG_NAME}-${CARGO_PKG_VERSION}-${RUSTUP_TOOLCHAIN}"

avr-size --mcu="${MCU}" -C "${ELF_FILE}"
avr-objdump -d "${ELF_FILE}" > "${OUT_DIR}/asm.txt"

avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature "${ELF_FILE}" "${OUT_DIR}/${IMAGE_BASE_NAME}.flash.hex"
avr-objcopy -O binary -R .eeprom -R .fuse -R .lock -R .signature "${ELF_FILE}" "${OUT_DIR}/${IMAGE_BASE_NAME}.flash.bin"
avr-objcopy -O ihex -j .eeprom "${ELF_FILE}" "${OUT_DIR}/${IMAGE_BASE_NAME}.eep.hex"
avr-objcopy -O ihex -j .fuse "${ELF_FILE}" "${OUT_DIR}/${IMAGE_BASE_NAME}.fuses.hex"

echo "Upload? [Y/n]"
read -r DO_UPLOAD

case $DO_UPLOAD in
  [nN] ) exit;;
esac

ARG_LIST="-c ft232r -p ${MCU} -U flash:w:${OUT_DIR}/${IMAGE_BASE_NAME}.flash.hex"
if [ ! "$(wc -c "${OUT_DIR}/${IMAGE_BASE_NAME}.eep.hex" | cut -d ' ' -f1)" = "0" ]; then
  ARG_LIST="${ARG_LIST} -U eeprom:w:${OUT_DIR}/${IMAGE_BASE_NAME}.eep.hex"
fi

#if [ ! "$(wc -c "${OUT_DIR}/${IMAGE_BASE_NAME}.fuses.hex" | cut -d ' ' -f1)" = "0" ]; then
#  ARG_LIST="${ARG_LIST} -U \"eeprom:w:${OUT_DIR}/${IMAGE_BASE_NAME}.eep.hex\""
#  srec_cat "${OUT_DIR}/${IMAGE_BASE_NAME}.fuses.hex" -Intel -crop 0x00 0x01 -offset  0x00 -O "${OUT_DIR}/${IMAGE_BASE_NAME}.fuses.l.hex" -Intel
#  srec_cat "${OUT_DIR}/${IMAGE_BASE_NAME}.fuses.hex" -Intel -crop 0x01 0x02 -offset -0x01 -O "${OUT_DIR}/${IMAGE_BASE_NAME}.fuses.h.hex" -Intel
#  srec_cat "${OUT_DIR}/${IMAGE_BASE_NAME}.fuses.hex" -Intel -crop 0x02 0x03 -offset -0x02 -O "${OUT_DIR}/${IMAGE_BASE_NAME}.fuses.e.hex" -Intel
#  ARG_LIST="${ARG_LIST} -U hfuse:w:${OUT_DIR}/${IMAGE_BASE_NAME}.fuses.h.hex:i -U lfuse:w:${OUT_DIR}/${IMAGE_BASE_NAME}.fuses.l.hex:i -U efuse:w:${OUT_DIR}/${IMAGE_BASE_NAME}.fuses.e.hex:i -u "
#fi

# shellcheck disable=SC2090
avrdude $ARG_LIST
avr-size --mcu="${MCU}" -C "${ELF_FILE}"
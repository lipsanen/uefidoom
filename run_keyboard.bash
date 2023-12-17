#!/bin/bash

mkdir -p root/efi/boot/
mv -f build_efi/efi_keyboard.efi root/efi/boot/bootx64.efi

qemu-system-x86_64 \
  -bios OVMF.fd \
  -drive format=raw,file=fat:rw:root \
  -net none

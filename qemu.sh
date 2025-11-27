#!/bin/bash

# VARSファイルのコピーを作成（初回のみ）
if [ ! -f OVMF_VARS.fd ]; then
    cp /usr/share/OVMF/OVMF_VARS_4M.fd OVMF_VARS.fd
fi

qemu-system-x86_64 \
    -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
    -drive if=pflash,format=raw,file=OVMF_VARS.fd \
    -hda disk.img
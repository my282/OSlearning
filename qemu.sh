OVMF_PATH="/usr/share/ovmf/OVMF.fd"

echo "Starting QEMU..."
mcopy -i disk.img ./kernel/kernel.elf ::kernel.elf
qemu-system-x86_64 \
  -s \
  -S \
  -bios "$OVMF_PATH" \
  -drive format=raw,file=disk.img,index=0,if=ide\
  -monitor stdio 


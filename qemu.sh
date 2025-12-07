OVMF_PATH="/usr/share/ovmf/OVMF.fd"

echo "Starting QEMU..."

qemu-system-x86_64 \
  -bios "$OVMF_PATH" \
  -drive format=raw,file=disk.img,index=0,if=ide\
  -monitor stdio



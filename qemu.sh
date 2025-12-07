OVMF_PATH="/usr/share/ovmf/OVMF.fd"
IMAGE_DIR="./dist"
if [ -f "$IMAGE_DIR/memmap" ]; then
  rm "$IMAGE_DIR/memmap"
fi

echo "Starting QEMU..."

qemu-system-x86_64 \
  -bios "$OVMF_PATH" \
  -drive format=raw,file=fat:rw:"$IMAGE_DIR" \
  -monitor stdio



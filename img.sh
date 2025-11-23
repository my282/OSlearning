qemu-img create -f raw disk.img 200M
mkfs.fat -n 'famOS' -s 2 -f 2 -R 32 -F 32 disk.img
mkdir -p mnt
sudo mount -o loop disk.img mnt
sudo mkdir -p mnt/BOOT/BOOTX64.EFI
sudo umount mnt
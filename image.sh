# Create an empty zeroed out 64MiB image file.
dd if=/dev/zero bs=1M count=0 seek=64 of=d.img
 
# Create a GPT partition table.
parted -s d.img mklabel gpt
 
# Create an ESP partition that spans the whole disk.
parted -s d.img mkpart ESP fat32 2048s 100%
parted -s d.img set 1 esp on
 
# Install the Limine BIOS stages onto the image.
limine-install d.img
 
# Mount the loopback device.
USED_LOOPBACK=$(sudo losetup -Pf --show d.img)
 
# Format the ESP partition as FAT32.
sudo mkfs.fat -F 32 ${USED_LOOPBACK}p1
 
# Mount the partition itself.
mkdir -p img_mount
sudo mount ${USED_LOOPBACK}p1 img_mount
 
# Copy the relevant files over.
sudo mkdir -p img_mount/EFI/BOOT
sudo cp -v d.elf limine.cfg /usr/local/share/limine/limine.sys img_mount/
sudo cp -v /usr/local/share/limine/BOOTX64.EFI img_mount/EFI/BOOT/
 
# Sync system cache and unmount partition and loopback device.
sync
sudo umount img_mount
sudo losetup -d ${USED_LOOPBACK}

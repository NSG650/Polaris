if [[ "$OSTYPE" == "darwin"* ]]; then
   echo "==> Making an image on macOS..."	

   # Image creation
   echo ""
   echo "==> Creating a .dmg file (limitation from hdiutil), FAT32, 64MB..."
   hdiutil create -layout GPTSPUD -size 64m -fs FAT32 -volname d d.dmg
   echo ""
   echo "==> Renaming .dmg to .img..."
   mv d.dmg d.img

   # Installing bootloader
   echo ""
   echo "==> Installing the bootloader..."
   limine-install d.img

   # Mount
   echo ""
   echo "==> Mounting the image..."
   hdiutil mount d.img

   # Copying necessary files
   echo ""
   echo "==> Copying necessary files..."
   mkdir -p /Volumes/D/EFI/BOOT/
   cp -v d.elf limine.cfg /usr/local/share/limine/limine.sys /Volumes/D/
   cp -v /usr/local/share/limine/BOOTX64.EFI /Volumes/D/EFI/BOOT/

   # Sync the system cache and unmount
   echo ""
   echo "==> Finishing up..."
   sync
   hdiutil unmount /Volumes/D
else
   echo "==> Making an image on $OSTYPE..."
   # Create an empty zeroed out 64MiB image file.
   echo ""
   echo "==> Creating an empty 64 MiB image..."
   dd if=/dev/zero bs=1M count=0 seek=64 of=d.img

   # Create a GPT partition table.
   echo ""
   echo "==> Creating a GPT partition table..."
   parted -s d.img mklabel gpt

   # Create an ESP partition that spans the whole disk.
   echo ""
   echo "==> Creating an ESP partition..."
   parted -s d.img mkpart ESP fat32 2048s 100%
   parted -s d.img set 1 esp on

   # Install the Limine BIOS stages onto the image.
   echo ""
   echo "==> Installing Limine..."
   limine-install d.img

   # Mount the loopback device.
   echo ""
   echo "==> Mounting and formatting the image (might request your password)..."
   USED_LOOPBACK=$(sudo losetup -Pf --show d.img)

   # Format the ESP partition as FAT32.
   sudo mkfs.fat -F 32 ${USED_LOOPBACK}p1

   # Mount the partition itself.
   mkdir -p img_mount
   sudo mount ${USED_LOOPBACK}p1 img_mount

   # Copy the relevant files over.
   echo ""
   echo "==> Copying necessary files..."
   sudo mkdir -p img_mount/EFI/BOOT/
   sudo cp -v d.elf limine.cfg img_mount/
   {
      sudo cp -v /usr/local/share/limine/limine.sys img_mount/
      sudo cp -v /usr/local/share/limine/BOOTX64.EFI img_mount/EFI/BOOT/
   } 2> /dev/null

   # Alternative path used as default in limine AUR package.
   if [[ $? != 0 ]]; then
      sudo cp -v /usr/share/limine/limine.sys img_mount/
      sudo cp -v /usr/share/limine/BOOTX64.EFI img_mount/EFI/BOOT/
   fi

   # Sync system cache and unmount partition and loopback device.
   echo ""
   echo "==> Finishing up..."
   sync
   sudo umount img_mount
   sudo losetup -d ${USED_LOOPBACK}
fi
echo "-----------"
echo "==> Done!"
echo "==> Now, you can launch D using the following command."
echo "> qemu-system-x86_64 -hda d.img -m 512M"
echo "==> Good luck!"

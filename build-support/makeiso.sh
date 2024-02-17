#!/bin/sh

set -ex

rm -rf sysroot
./jinx sysroot
./jinx host-build limine

( cd sysroot && tar -cvf ../ramdisk.tar * )

rm -rf iso_root
mkdir -v iso_root
cp sysroot/usr/share/polaris/polaris.elf iso_root/polaris.elf
cp build-support/limine.cfg iso_root/
cp ramdisk.tar iso_root/

cp host-pkgs/limine/usr/local/share/limine/limine-bios.sys iso_root/
cp host-pkgs/limine/usr/local/share/limine/limine-bios-cd.bin iso_root/
cp host-pkgs/limine/usr/local/share/limine/limine-uefi-cd.bin iso_root/
mkdir -pv iso_root/EFI/BOOT
cp host-pkgs/limine/usr/local/share/limine/BOOT*.EFI iso_root/EFI/BOOT/

xorriso -as mkisofs -b limine-bios-cd.bin -no-emul-boot -boot-load-size 4 \
	-boot-info-table --efi-boot limine-uefi-cd.bin -efi-boot-part \
	--efi-boot-image --protective-msdos-label iso_root -o polaris.iso

host-pkgs/limine/usr/local/bin/limine bios-install polaris.iso

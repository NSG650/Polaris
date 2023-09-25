CONFIG_ARCH ?= x86_64
CONFIG_TARGET ?= pc-new

NAME := polaris
ISO_IMAGE := $(NAME).iso
KERNEL_ELF = $(NAME).elf
PROGRAM_ELF = $(wildcard user/*.elf)
DRIVER_ELF = $(wildcard drivers/*.ko)

.PHONY: all
all: $(ISO_IMAGE)

.PHONY: limine
limine:
	make -C limine

.PHONY: kernel
kernel:
	$(MAKE) -C kernel/arch/$(CONFIG_ARCH)-$(CONFIG_TARGET)

.PHONY: drivers
drivers:
	$(MAKE) -C drivers console.ko

.PHONY: user
user:
	$(MAKE) -C user

$(ISO_IMAGE): limine kernel user drivers
	rm -rf build
	mkdir -p build
	cp $(PROGRAM_ELF) root/bin
	cp $(DRIVER_ELF) root/lib/modules
	$(MAKE) -C root
	cp kernel/arch/$(CONFIG_ARCH)-$(CONFIG_TARGET)/$(KERNEL_ELF) ramdisk.tar \
		limine.cfg limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin build/
	mkdir -p build/EFI/BOOT
	cp -v limine/BOOTX64.EFI build/EFI/BOOT/
	cp -v limine/BOOTIA32.EFI build/EFI/BOOT/
	xorriso -as mkisofs -b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		build -o $(ISO_IMAGE)
	limine/limine bios-install $(ISO_IMAGE)
	rm -rf build

.PHONY: clean
clean:
	rm -rf build *.iso *.img *.tar *.tar.gz
	$(MAKE) -C kernel/arch/$(CONFIG_ARCH)-$(CONFIG_TARGET) clean

.PHONY: distclean
distclean: clean
	$(MAKE) -C kernel/arch/$(CONFIG_ARCH)-$(CONFIG_TARGET) distclean

.PHONY: format
format:
	clang-format -i $(shell find . \( -iname *.h -o -iname *.c \) -not -ipath "*./limine*" \
		-not -iname "limine.h" -not -ipath "*./kernel/arch/x86_64-pc/fw/lai/*" \
		-not -ipath "*./kernel/arch/x86_64-pc/include/debug/zydis/*" \
		-not -ipath "*./kernel/arch/x86_64-pc/include/debug/Zycore/*" \
		-not -ipath "*./sources*" -not -ipath "*./host-pkgs*" \
		-not -ipath "*./pkgs*" -not -ipath "*./builds*" -not -ipath "*./host-builds*" \
		-not -ipath "*./kernel/fb/terminal*" -not -ipath "*./.jinx-cache*")

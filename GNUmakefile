CONFIG_ARCH ?= x86_64

NAME := polaris
ISO_IMAGE := $(NAME).iso
KERNEL_ELF = $(NAME).elf
PROGRAM_ELF = program64.elf

.PHONY: all
all: $(ISO_IMAGE)

.PHONY: limine
limine:
	make -C limine

.PHONY: kernel
kernel:
	$(MAKE) -C kernel

.PHONY: user
user:
	$(MAKE) -C user

$(ISO_IMAGE): limine kernel user
	rm -rf build
	mkdir -p build
	$(MAKE) -C root
	cp user/$(PROGRAM_ELF) root/bin
	cp kernel/$(KERNEL_ELF) ramdisk.tar \
		limine.cfg limine/limine.sys limine/limine-cd.bin limine/limine-cd-efi.bin build/
	xorriso -as mkisofs -b limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-cd-efi.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		build -o $(ISO_IMAGE)
	limine/limine-deploy $(ISO_IMAGE)
	rm -rf build

.PHONY: clean
clean:
	rm -rf build *.iso *.img
	$(MAKE) -C kernel clean

.PHONY: distclean
distclean: clean
	$(MAKE) -C kernel distclean

.PHONY: format
format:
	clang-format -i $(shell find . \( -iname *.h -o -iname *.c \) -not -ipath "*./limine*" -not -iname "limine.h" -not -ipath "*./kernel/arch/x86_64/fw/lai/*")

include ./.config

ifndef CONFIG_ARCH
$(info warning: CONFIG_ARCH is not set (defaulting to x86_64))
CONFIG_ARCH := x86_64
endif

NAME := Polaris
ISO_IMAGE := $(NAME).iso
KERNEL_ELF = $(NAME)-$(CONFIG_ARCH).elf
PROGRAM_ELF = program64.elf


.PHONY: all

all: $(ISO_IMAGE)

limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v2.0-branch-binary --depth=1
	make -C limine

.PHONY: kernel
kernel:
	$(MAKE) -C kernel

.PHONY: user
user:
	$(MAKE) -C user

$(ISO_IMAGE): kernel user limine
ifeq ($(CONFIG_ARCH),x86_64)
	rm -rf build
	mkdir -p build
	cp kernel/$(KERNEL_ELF) user/$(PROGRAM_ELF) \
		limine.cfg limine/limine.sys limine/limine-cd.bin limine/limine-eltorito-efi.bin build/
	xorriso -as mkisofs -b limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-eltorito-efi.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		build -o $(ISO_IMAGE)
	chmod +x limine/limine-install
	limine/limine-install $(ISO_IMAGE)
	rm -rf build
endif

.PHONY: clean
clean:
	rm -f *.iso *.img
	$(MAKE) -C kernel clean

.PHONY: distclean
distclean: clean
	rm -rf limine
	$(MAKE) -C kernel distclean

.PHONY: format
format:
	clang-format -i $(shell find . -name *.h -o -name *.c)

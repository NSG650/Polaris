CONFIG_ARCH ?= x86_64
CONFIG_TARGET ?= pc

NAME := polaris
ISO_IMAGE := $(NAME).iso
KERNEL_ELF = $(NAME).elf

.PHONY: all
all: kernel

.PHONY: kernel
kernel:
	$(MAKE) -C arch/$(CONFIG_ARCH)-$(CONFIG_TARGET)

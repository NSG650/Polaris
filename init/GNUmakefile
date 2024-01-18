CONFIG_ARCH ?= x86_64

CINITFILES := $(wildcard init/*.c)
ASMFILES := $(wildcard $(CONFIG_ARCH)/*.asm)
INITOBJ := $(CINITFILES:.c=.o) $(ASMFILES:.asm=.o)

ifeq ($(CONFIG_ARCH),x86_64)
	AS := nasm
	CC := x86_64-polaris-gcc
	LD := x86_64-polaris-ld
	LDFLAGS := -T$(CONFIG_ARCH)/linker.ld
	ASFLAGS := -felf64
	CFLAGS := -ffreestanding -mno-red-zone -Os -pipe -Wall -Wextra -std=gnu11
endif

.PHONY: all
all: init

init: $(INITOBJ)
	$(LD) $(LDFLAGS) $^ -o init.elf

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) -f elf64 $< -o $@

.PHONY: clean
clean:
	rm -rf *.elf $(INITOBJ) $(TESTOBJ)

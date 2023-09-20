CONFIG_ARCH ?= x86_64

CHELLOFILES := $(wildcard $(CONFIG_ARCH)/hello/*.c)
CBEEPFILES := $(wildcard $(CONFIG_ARCH)/beep/*.c)

ifeq ($(CONFIG_ARCH),x86_64)
	AS := nasm
	CC := x86_64-elf-gcc
	ASFLAGS := -felf64 -static
	CFLAGS := -c -mcmodel=large -fno-asynchronous-unwind-tables -ffreestanding -masm=intel -Isdk
endif

hello.ko: $(CHELLOFILES)
	$(CC) $(CFLAGS) $^ -o hello.ko

beep.ko: $(CBEEPFILES)
	$(CC) $(CFLAGS) $^ -o beep.ko


CONFIG_ARCH ?= x86_64

CHELLOFILES := $(wildcard $(CONFIG_ARCH)/hello/*.c)

ifeq ($(CONFIG_ARCH),x86_64)
	AS := nasm
	CC := x86_64-elf-gcc
	ASFLAGS := -felf64 -static
	CFLAGS := -c -mcmodel=large -fno-asynchronous-unwind-tables
endif

hello.ko: $(CHELLOFILES)
	$(CC) $(CFLAGS) $^ -o hello.ko

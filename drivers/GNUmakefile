CONFIG_ARCH ?= x86_64
CCONSOLEFILES := $(wildcard console/*.c)

ifeq ($(CONFIG_ARCH),x86_64)
	AS := nasm
	CC := x86_64-polaris-gcc
	ASFLAGS := -felf64 -static
	CFLAGS := -c -fsanitize=undefined -mcmodel=large -fno-asynchronous-unwind-tables -ffreestanding -masm=intel -Isdk -Isdk/x86_64
endif

.PHONY: all
all:
	$(CC) $(CFLAGS) $(CCONSOLEFILES) -o console.ko

.PHONY: clean
clean:
	rm -rf *.ko

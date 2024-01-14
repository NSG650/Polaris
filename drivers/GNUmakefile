CONFIG_ARCH ?= x86_64
CCONSOLEFILES := $(wildcard console/*.c)
CKEYBOARDFILES := $(wildcard x86_64/keyboard/*.c)
CSERIALFILES := $(wildcard x86_64/serial/*.c)

ifeq ($(CONFIG_ARCH),x86_64)
	AS := nasm
	CC := x86_64-polaris-gcc
	ASFLAGS := -felf64 -static
	CFLAGS := -c -mcmodel=large -fno-asynchronous-unwind-tables -ffreestanding -masm=intel -Isdk
endif

.PHONY: all
all:
	$(CC) $(CFLAGS) $(CCONSOLEFILES) -o console.ko
	$(CC) $(CFLAGS) $(CKEYBOARDFILES) -o keyboard.ko
	$(CC) $(CFLAGS) $(CSERIALFILES) -o serial.ko

.PHONY: clean
clean:
	rm -rf *.ko

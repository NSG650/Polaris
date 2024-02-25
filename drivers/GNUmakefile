CONFIG_ARCH ?= x86_64
CCONSOLEFILES := $(wildcard console/*.c)
CNVMEFILES := $(wildcard x86_64/nvme/*.c)
CVIRTIOGPUFILES := $(wildcard x86_64/virtiogpu/*.c)
CFAT32FILES := $(wildcard fs/fat32/*.c)
CSERIALFILES := $(wildcard x86_64/serial/*.c)
CTARFILES := $(wildcard fs/tarfs/*.c)

ifeq ($(CONFIG_ARCH),x86_64)
	AS := nasm
	CC := x86_64-polaris-gcc
	ASFLAGS := -felf64 -static
	CFLAGS := -c -fsanitize=undefined -mgeneral-regs-only -mno-red-zone -mcmodel=large -fno-asynchronous-unwind-tables -ffreestanding -masm=intel -I../kernel/include -I../kernel/arch/x86_64-pc/include -I../kernel/arch/x86_64-pc/fw/lai/include
endif

.PHONY: all
all:
	$(CC) $(CFLAGS) $(CCONSOLEFILES) -o console.ko
	$(CC) $(CFLAGS) $(CNVMEFILES) -o nvme.ko
	$(CC) $(CFLAGS) $(CVIRTIOGPUFILES) -o virtiogpu.ko
	$(CC) $(CFLAGS) $(CFAT32FILES) -o fat32.ko
	$(CC) $(CFLAGS) $(CSERIALFILES) -o serial.ko
	$(CC) $(CFLAGS) $(CTARFILES) -o tarfs.ko
	
.PHONY: clean
clean:
	rm -rf *.ko

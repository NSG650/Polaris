CONFIG_ARCH ?= x86_64

ifeq ($(CONFIG_ARCH),x86_64)
	AS := nasm
	CC := x86_64-polaris-gcc
	LD := x86_64-polaris-ld
	ASFLAGS := -felf64 -static
	CFLAGS := -c -fsanitize=undefined -mgeneral-regs-only -mno-red-zone -mcmodel=large -fno-asynchronous-unwind-tables -ffreestanding -masm=intel -I../kernel/include -I../kernel/arch/x86_64-pc/include -I../kernel/arch/x86_64-pc/fw/lai/include
	
	CNVMEFILES := $(wildcard x86_64/nvme/*.c)
	CVIRTIOGPUFILES := $(wildcard x86_64/virtiogpu/*.c)
	CSERIALFILES := $(wildcard x86_64/serial/*.c)
endif

.PHONY: all
all:
	$(CC) $(CFLAGS) $(CNVMEFILES) -o nvme.ko
	$(CC) $(CFLAGS) $(CVIRTIOGPUFILES) -o virtiogpu.ko
	$(CC) $(CFLAGS) $(CSERIALFILES) -o serial.ko

ifeq ($(CONFIG_ARCH),x86_64)
	# PS2 devices
	$(CC) $(CFLAGS) x86_64/ps2/ps2.c -o ps2.o
	$(CC) $(CFLAGS) x86_64/ps2/mouse.c -o mouse.o
	$(CC) $(CFLAGS) x86_64/ps2/keyboard.c -o keyboard.o
	$(LD) -r ps2.o mouse.o keyboard.o -o ps2.ko
	rm ps2.o mouse.o keyboard.o
endif

	$(CC) $(CFLAGS) x86_64/i8254x/main.c -o i8254x.ko
	$(CC) $(CFLAGS) fs/fat32/main.c -o fat32.ko

.PHONY: clean
clean:
	rm -rf *.ko

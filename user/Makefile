include ../.config

ifndef CONFIG_ARCH
$(info warning: CONFIG_ARCH is not set (defaulting to x86_64))
CONFIG_ARCH := x86_64
endif

CFILES	:= $(wildcard */*.c)
ASMFILES := $(wildcard $(CONFIG_ARCH)/*.asm)
SFILES	:= $(wildcard $(CONFIG_ARCH)/*.S)
OBJ     := $(CFILES:.c=.o) $(ASMFILES:.asm=.o) $(SFILES:.S=.o)

ifeq ($(CONFIG_ARCH),x86_64)
	AS := nasm
	CC := x86_64-elf-gcc
	LD := x86_64-elf-ld
	LDFLAGS := -T$(CONFIG_ARCH)/linker.ld
	ASFLAGS := -felf64 -O3 -static
	CFLAGS := -ffreestanding -mno-red-zone -fno-pic -no-pie -static
endif

.PHONY: all

all: $(OBJ)
	$(LD) $(LDFLAGS) $^ -o program64.elf

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) -f elf64 $< -o $@

%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf *.elf $(OBJ)

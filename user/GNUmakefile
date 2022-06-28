CONFIG_ARCH ?= x86_64

CFILES := $(wildcard */*.c)
ASMFILES := $(wildcard $(CONFIG_ARCH)/*.asm)
OBJ := $(CFILES:.c=.o) $(ASMFILES:.asm=.o)

ifeq ($(CONFIG_ARCH),x86_64)
	AS := nasm
	CC := x86_64-elf-gcc
	LD := x86_64-elf-ld
	LDFLAGS := -T$(CONFIG_ARCH)/linker.ld
	ASFLAGS := -felf64 -O3 -static
	CFLAGS := -ffreestanding -mno-red-zone -fno-pic -fno-pie -static
endif

.PHONY: all
all: $(OBJ)
	$(LD) $(LDFLAGS) $^ -o program64.elf

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) -f elf64 $< -o $@

.PHONY: clean
clean:
	rm -rf *.elf $(OBJ)

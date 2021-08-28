# Name of the final executable
KERNEL := d.elf

# The compiler we are using
CC := x86_64-elf-gcc
AS := nasm

# Compiler flags
CFLAGS :=                             \
	-Wall -Wextra -g -I stivale/      \
	-I kernel/klibc/liballoc/include/ \
	-I kernel/acpi/lai/include/

# Assembler flags
ASFLAGS := -g

# Internal flags that shouldn't be changed
INTERNALLDFLAGS :=          \
	-fpie -Wl,-static       \
	-nostdlib               \
	-T kernel/linker.ld     \
	-z max-page-size=0x1000 \
	-lgcc

INTERNALCFLAGS :=        \
	-std=gnu11           \
	-ffreestanding       \
	-fno-stack-protector \
	-fpie -mno-red-zone	 \
	-masm=intel

# C files and objects
CFILES := $(wildcard kernel/*/*.c kernel/acpi/lai/*/*.c \
			kernel/klibc/liballoc/liballoc.c)
ASMFILES := $(wildcard kernel/*/*.asm)
OBJ := $(CFILES:.c=.o) $(ASMFILES:.asm=.o)

.PHONY: all clean

all: $(KERNEL)

$(KERNEL): $(OBJ)
	$(CC) $(INTERNALLDFLAGS) $(OBJ) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) -f elf64 $< -o $@

clean:
	rm -rf $(KERNEL) $(OBJ)
	rm -rf d.hdd img_mount

image:
	@./image.sh

run:
	qemu-system-x86_64 -hda d.hdd -serial stdio -m 512M

debug:
	qemu-system-x86_64 -hda d.hdd -M q35,smm=off -d int -no-reboot -s -m 512M

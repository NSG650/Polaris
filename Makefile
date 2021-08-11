# Name of the final executable

KERNEL := d.elf

# The compiler we are using

CC = x86_64-elf-gcc
AS = nasm

# Compiler flags

CFLAGS = -Wall -Wextra -g -I stivale/

# Internal flags that shouldn't be changed

INTERNALLDFLAGS :=          \
	-fno-pic -fpie          \
	-Wl,-static             \
	-nostdlib               \
	-T kernel/linker.ld     \
	-z max-page-size=0x1000 \
	-lgcc

INTERNALCFLAGS :=        \
	-std=gnu11           \
	-ffreestanding       \
	-fno-stack-protector \
	-fno-pic -fpie       \
	-mno-red-zone	     \
	-masm=intel

# C files and objects

CFILES := $(wildcard kernel/*/*.c)
ASMFILES := $(wildcard kernel/*/*.asm)
OBJ := $(CFILES:.c=.o) $(ASMFILES:.asm=.o)

.PHONY: all clean

all: $(KERNEL)

$(KERNEL): $(OBJ)
	$(CC) $(INTERNALLDFLAGS) $(OBJ) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) -f elf64 $< -o $@


clean:
	rm -rf $(KERNEL) $(OBJ)
	rm -rf d.img

image:
	./image.sh

run:
	qemu-system-x86_64 -hda d.img -serial stdio -m 512M

debug:
	qemu-system-x86_64 -hda d.img -M q35,smm=off -d int -no-reboot -s -m 512M

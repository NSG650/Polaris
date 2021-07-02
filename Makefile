#name of the final executable

KERNEL := d.elf

#the compiler we are using

CC = x86_64-elf-gcc
AS = nasm

#compiler flags

CFLAGS = -Wall -Wextra -O2 -pipe -I stivale/

# internal flags that shouldnt be changed

INTERNALLDFLAGS :=     \
	-fno-pic -fpie \
	-Wl,-static \
	-static-pie    \
	-nostdlib      \
	-T kernel/linker.ld    \
	-z max-page-size=0x1000

# internal flags that shouldnt be changed

INTERNALCFLAGS  :=           \
	-I.                  \
	-std=gnu11           \
	-ffreestanding       \
	-fno-stack-protector \
	-fno-pic -fpie       \
	-mgeneral-regs-only  \
	-mno-red-zone	     \
	-masm=intel

#c files and object

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
	$(AS) -felf64 $< -o $@


clean:
	rm -rf $(KERNEL) $(OBJ)
	rm -rf d.img

image:
	./image.sh

run:
	qemu-system-x86_64 -hda d.img

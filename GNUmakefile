# This is a Makefile meant for GNU Make. Assert that
ifneq (,)
This makefile requires GNU Make.
endif

# Name of the final executable
override KERNEL := polaris.elf

# The compiler we are using
CC := x86_64-elf-gcc
AS := nasm

# Compiler flags
CFLAGS :=                             \
	-Wall -Wextra -g -Og -I stivale/  \
	-I kernel/klibc/liballoc/include/ \
	-I kernel/acpi/lai/include/       \
	-pipe -DKVERSION=\"git-$(shell git log -1 --pretty=format:%h)\"

# Assembler flags
ASFLAGS := -g -MD -MP

# Internal flags that shouldn't be changed
override INTERNALLDFLAGS :=  \
	-T kernel/linker.ld      \
	-nostdlib                \
	-Wl,-static,-pie	     \
	-Wl,--no-dynamic-linker	 \
	-z max-page-size=4096    \
	-z text -lgcc

override INTERNALCFLAGS := \
	-std=gnu11             \
	-ffreestanding         \
	-fno-stack-protector   \
	-fpie -MMD -MP         \
	-mno-red-zone -masm=intel

override CFILES := $(wildcard kernel/*/*.c kernel/acpi/lai/*/*.c kernel/klibc/*/*.c)
override ASMFILES := $(wildcard kernel/*/*.asm)
override OBJECTS := $(CFILES:.c=.o) $(ASMFILES:.asm=.o)
override DEPENDS := $(CFILES:.c=.d) $(ASMFILES:.asm=.o.d)

.PHONY: all clean

all: $(KERNEL)

$(KERNEL): $(OBJECTS)
	$(CC) $(INTERNALLDFLAGS) $^ -o $@
	python3 gensym.py $(KERNEL)

-include $(DEPENDS)

%.o: %.c GNUmakefile
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@

%.o: %.asm GNUmakefile
	$(AS) $(ASFLAGS) -f elf64 $< -o $@

clean:
	$(RM) $(OBJECTS) $(DEPENDS) $(KERNEL)
	$(RM) -r *.hdd img_mount
	$(RM) -r *.gz

image: $(KERNEL)
	@./image.sh

run: image
	qemu-system-x86_64 -hda polaris.hdd -serial stdio -m 512M

debug:
	qemu-system-x86_64 -hda polaris.hdd -M q35,smm=off -d int -no-reboot -s -m 512M

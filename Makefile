# Name of the final executable
KERNEL := polaris.elf

# The compiler we are using
CC := x86_64-elf-gcc
AS := nasm

# Compiler flags
CFLAGS :=                             \
	-Wall -Wextra -g -I stivale/      \
	-I kernel/klibc/liballoc/include/ \
	-I kernel/acpi/lai/include/ -MMD  \
	-D KVERSION=\"git-$(shell git log -1 --pretty=format:%h)\" \
	-MP -pipe

# Assembler flags
ASFLAGS := -g -MD -MP

# Internal flags that shouldn't be changed
INTERNALLDFLAGS :=           \
	-T kernel/linker.ld      \
	-nostdlib                \
	-Wl,-static,-pie         \
	-Wl,--no-dynamic-linker  \
	-z max-page-size=4096    \
	-z text -lgcc

INTERNALCFLAGS :=        \
	-std=gnu11           \
	-ffreestanding       \
	-fno-stack-protector \
	-fpie -mno-red-zone	 \
	-masm=intel

CFILES := $(wildcard kernel/*/*.c kernel/acpi/lai/*/*.c kernel/klibc/*/*.c)
ASMFILES := $(wildcard kernel/*/*.asm)
OBJECTS := $(CFILES:.c=.o) $(ASMFILES:.asm=.o)
DEPENDS := $(CFILES:.c=.d) $(ASMFILES:.asm=.o.d)

.PHONY: all clean

all: $(KERNEL)

$(KERNEL): $(OBJECTS)
	$(CC) $(INTERNALLDFLAGS) $^ -o $@

-include $(DEPENDS)

%.o: %.c Makefile
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@

%.o: %.asm Makefile
	$(AS) $(ASFLAGS) -f elf64 $< -o $@

clean:
	$(RM) $(OBJECTS) $(DEPENDS) $(KERNEL)
	$(RM) -r *.hdd img_mount
	$(RM) -r *.gz

image:
	@./image.sh

run:
	qemu-system-x86_64 -hda polaris.hdd -serial stdio -m 512M

debug:
	qemu-system-x86_64 -hda polaris.hdd -M q35,smm=off -d int -no-reboot -s -m 512M

CONFIG_ARCH ?= x86_64

CINITFILES := $(wildcard init/*.c)
CKILLFILES := $(wildcard kill/*.c)
CSIMPLEFILES := $(wildcard simple/*.c)
ASMFILES := $(wildcard $(CONFIG_ARCH)/*.asm)
INITOBJ := $(CINITFILES:.c=.o) $(ASMFILES:.asm=.o)
KILLOBJ := $(CKILLFILES:.c=.o) $(ASMFILES:.asm=.o)
SIMPLEOBJ := $(CSIMPLEFILES:.c=.o) $(ASMFILES:.asm=.o)

ifeq ($(CONFIG_ARCH),x86_64)
	AS := nasm
	CC := x86_64-elf-gcc
	LD := x86_64-elf-ld
	LDFLAGS := -T$(CONFIG_ARCH)/linker.ld
	ASFLAGS := -felf64 -static
	CFLAGS := -Os -ffreestanding -mno-red-zone -fno-pic -fno-pie -static
endif

.PHONY: all
all: init kill simple

init: $(INITOBJ)
	$(LD) $(LDFLAGS) $^ -o init.elf

kill: $(KILLOBJ)
	$(LD) $(LDFLAGS) $^ -o kill.elf

simple: $(SIMPLEOBJ)
	$(LD) $(LDFLAGS) $^ -o simple.elf

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) -f elf64 $< -o $@

.PHONY: clean
clean:
	rm -rf *.elf $(INITOBJ) $(TESTOBJ)

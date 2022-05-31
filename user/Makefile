include ../.config

ifndef CONFIG_ARCH
$(info warning: CONFIG_ARCH is not set (defaulting to x86_64))
CONFIG_ARCH := x86_64
endif

ASMFILES := $(wildcard $(CONFIG_ARCH)/*.asm)
SFILES	:= $(wildcard $(CONFIG_ARCH)/*.S)
OBJ     := $(ASMFILES:.asm=.o) $(SFILES:.S=.o)


ifeq ($(CONFIG_ARCH),x86_64)
	AS := nasm
	LD := x86_64-elf-ld
	LDFLAGS := -T$(CONFIG_ARCH)/linker.ld
	ASFLAGS := -felf64 -O3 -static
endif

.PHONY: all

all: $(OBJ)
	$(LD) $(LDFLAGS) $^ -o program64.elf

%.o: %.asm
	$(AS) $(ASFLAGS) -f elf64 $< -o $@

%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf *.elf $(OBJ)

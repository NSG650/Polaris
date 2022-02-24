include ../.config

ifndef CONFIG_ARCH
$(info warning: CONFIG_ARCH is not set (defaulting to x86_64))
CONFIG_ARCH := x86_64
endif

KERNEL := Polaris

CARCHFILES  := $(wildcard arch/$(CONFIG_ARCH)/*/*.c)
CFILES		:= $(wildcard */*.c)
ASMFILES	:= $(wildcard arch/$(CONFIG_ARCH)/*/*.asm)
SFILES		:= $(wildcard arch/$(CONFIG_ARCH)/*/*.S)
OBJ         := $(CARCHFILES:.c=.o) $(CFILES:.c=.o) $(ASMFILES:.asm=.o) $(SFILES:.S=.o)
HEADER_DEPS := $(CARCHFILES:.c=.d) $(CFILES:.c=.d) $(ASMFILES:.asm=.d) $(SFILES:.S=.d)

ifeq ($(CONFIG_ARCH),x86_64)
	CC := x86_64-elf-gcc
	AS := nasm
	LD := x86_64-elf-gcc
	CFLAGS  := -Wall -Wextra -O0 -g -pipe -I include/ -I arch/$(CONFIG_ARCH)/include/ -I arch/$(CONFIG_ARCH)/fw/lai/include
	LDFLAGS :=
	ASFLAGS := -g -MD -MP
	CARCHFILES += $(wildcard arch/$(CONFIG_ARCH)/fw/lai/*/*.c)
	OBJ         := $(CARCHFILES:.c=.o) $(CFILES:.c=.o) $(ASMFILES:.asm=.o) $(SFILES:.S=.o)
	HEADER_DEPS := $(CARCHFILES:.c=.d) $(CFILES:.c=.d) $(ASMFILES:.asm=.d) $(SFILES:.S=.d)
	INTERNALLDFLAGS :=         			\
		-T arch/$(CONFIG_ARCH)/linker.ld	\
		-nostdlib              			\
		-mcmodel=kernel		   		\
		-zmax-page-size=0x1000 			\
		-static                			\
		-pie                   			\
		-ztext

	INTERNALCFLAGS :=            \
		-I.                  \
		-std=gnu11           \
		-ffreestanding       \
		-fno-stack-protector \
		-fpie                \
		-masm=intel	     \
		-mno-80387           \
		-mno-mmx             \
		-mno-3dnow           \
		-mno-sse             \
		-mno-sse2            \
		-mno-red-zone        \
		-MMD
endif

.PHONY: all

all: $(OBJ)
	$(LD) $(INTERNALLDFLAGS) $^ -o $(KERNEL)-$(CONFIG_ARCH).elf

-include $(HEADER_DEPS)

%.o: %.asm
	$(AS) $(ASFLAGS) -f elf64 $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@

%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf *.elf $(OBJ) $(HEADER_DEPS)

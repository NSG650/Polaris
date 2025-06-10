CONFIG_ARCH ?= x86_64
CONFIG_TARGET ?= pc

# This is the name that our final kernel executable will have.
override KERNEL := polaris.elf

# User controllable CFLAGS.
GIT_VERSION := $(shell git --no-pager describe --tags --always --dirty)
CFLAGS ?= -Wvla -Wshadow -g -Og -pipe

# User controllable linker flags. We set none by default.
LDFLAGS ?=

# User controllable assembler flags.
ASFLAGS ?= -g

# Internal C flags that should not be changed by the user.
override INTERNALCFLAGS :=          \
	-I.                             \
	-I ../../include/               \
	-I include/ 					\
	-std=gnu11                      \
	-fms-extensions                 \
	-ffreestanding                  \
	-fno-stack-protector            \
	-fno-stack-check                \
	-fpie                           \
	-MMD

# Internal linker flags that should not be changed by the user.
override INTERNALLDFLAGS :=     \
	-nostdlib                   \
	-Wl,-static,-pie            \
	-Wl,--gc-sections           \
	-Wl,--no-dynamic-linker	    \
	-Wl,-z,max-page-size=0x1000 \
	-Wl,-Tlinker.ld

# Internal assembler flags that should not be changed by the user.
override INTERNALASFLAGS := \
	-MD                     \
	-MP                     \
	-f elf64

CFILES := $(shell find ../../ -path ../../arch -prune -type f -o -name '*.c')
CFILES += $(shell find ../../arch/x86_64-pc -path . -prune -type f -o -name '*.c')
# CFILES += ../../locks/spinlock.c ../../debug/debug.c ../../klibc/string.c ../../klibc/kargs.c ../../klibc/vec.c ../../fb/fb.c ../../fb/terminal/flanterm.c ../../fb/terminal/backends/fb.c
ASMFILES := $(shell find ../../arch/x86_64-pc -type f -name '*.asm')
OBJ := $(CFILES:.c=.o) $(ASMFILES:.asm=.o)
HEADER_DEPS := $(CFILES:.c=.d) $(ASMFILES:.asm=.d)

CC := x86_64-polaris-gcc
AS := nasm

override INTERNALCFLAGS += 				\
	-m64                   				\
	-march=x86-64          				\
	-masm=intel            				\
	-mgeneral-regs-only    				\
	-I fw/lai/include      				\
	-I include/debug       				\
	-fsanitize=undefined   				\
	-Wall -Wextra 		   				\
	-Wno-error					\
	-Wno-builtin-declaration-mismatch	\
	-mno-red-zone

override INTERNALLDFLAGS += \
	-Wl,-m,elf_x86_64

.PHONY: all
all: $(KERNEL)

# Don't try to download if it already exists.
limine.h:
	[ -s include/$@ ] || curl \
		https://raw.githubusercontent.com/limine-bootloader/limine/trunk/limine.h -o include/$@

# Link rules for the final kernel executable.
$(KERNEL): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) $(INTERNALLDFLAGS) -o $@

../../fb/terminal/backends/fb.o: override INTERNALCFLAGS += \
	-DFLANTERM_FB_DISABLE_BUMP_ALLOC

../../sched/sched.o: override INTERNALCFLAGS += \
	-DGIT_VERSION=\"$(GIT_VERSION)\"

../../debug/printf.o: override INTERNALCFLAGS += \
	-DPRINTF_INCLUDE_CONFIG_H=1

# Compilation rules for *.c files.
-include $(HEADER_DEPS)
%.o: %.c limine.h
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@

# Compilation rules for *.asm files.
%.o: %.asm
	$(AS) $(ASFLAGS) $(INTERNALASFLAGS) $< -o $@

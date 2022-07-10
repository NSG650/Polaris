# Target processor architecture.
CONFIG_ARCH ?= x86_64

# This is the name that our final kernel executable will have.
override KERNEL := polaris.elf

# User controllable CFLAGS.
CFLAGS ?= -Wall -Wextra -g -Og -pipe

# User controllable linker flags. We set none by default.
LDFLAGS ?=

# User controllable assembler flags.
ASFLAGS ?= -g

# Internal C flags that should not be changed by the user.
override INTERNALCFLAGS :=                \
	-I.                                   \
	-I include/                           \
	-I arch/$(CONFIG_ARCH)/include/       \
	-I arch/$(CONFIG_ARCH)/fw/lai/include \
	-std=gnu11                            \
	-ffreestanding                        \
	-fno-stack-protector                  \
	-fno-stack-check                      \
	-fpie                                 \
	-MMD

# Internal linker flags that should not be changed by the user.
override INTERNALLDFLAGS :=     \
	-nostdlib                   \
	-Wl,-static,-pie            \
	-Wl,--no-dynamic-linker	    \
	-Wl,-z,max-page-size=0x1000 \
	-Wl,-T,arch/$(CONFIG_ARCH)/linker.ld

# Internal assembler flags that should not be changed by the user.
override INTERNALASFLAGS := \
	-MD                     \
	-MP                     \
	-f elf64

CARCHFILES := $(wildcard arch/$(CONFIG_ARCH)/*/*.c)
CFILES := $(wildcard */*.c)
ASMFILES := $(wildcard arch/$(CONFIG_ARCH)/*/*.asm)
OBJ := $(CARCHFILES:.c=.o) $(CFILES:.c=.o) $(ASMFILES:.asm=.o)
HEADER_DEPS := $(CARCHFILES:.c=.d) $(CFILES:.c=.d) $(ASMFILES:.asm=.d)

ifeq ($(CONFIG_ARCH),x86_64)
	CC := x86_64-elf-gcc
	AS := nasm
	CARCHFILES += $(wildcard arch/$(CONFIG_ARCH)/fw/lai/*/*.c)
	OBJ := $(CARCHFILES:.c=.o) $(CFILES:.c=.o) $(ASMFILES:.asm=.o)
	HEADER_DEPS := $(CARCHFILES:.c=.d) $(CFILES:.c=.d) $(ASMFILES:.asm=.d)

	override INTERNALCFLAGS += \
		-m64                   \
		-march=x86-64          \
		-mabi=sysv             \
		-masm=intel            \
		-mno-80387             \
		-mno-mmx               \
		-mno-3dnow             \
		-mno-sse               \
		-mno-sse2              \
		-mno-red-zone
	override INTERALLDFLAGS += \
		-Wl,-m,elf_x86_64
endif

.PHONY: all
all: $(KERNEL)

limine.h:
	curl https://raw.githubusercontent.com/limine-bootloader/limine/trunk/limine.h -o arch/$(CONFIG_ARCH)/include/$@

# Link rules for the final kernel executable.
$(KERNEL): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) $(INTERNALLDFLAGS) -o $@

# Compilation rules for *.c files.
-include $(HEADER_DEPS)
%.o: %.c limine.h
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@

# Compilation rules for *.asm files.
%.o: %.asm
	$(AS) $(ASFLAGS) $(INTERNALASFLAGS) $< -o $@

# Remove object files and the final executable.
.PHONY: clean
clean:
	rm -rf *.elf $(OBJ) $(HEADER_DEPS)

.PHONY: distclean
distclean: clean
	rm -f arch/$(CONFIG_ARCH)/include/limine.h

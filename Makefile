#name of the final executable

KERNEL := d.elf

#the compiler we are using

CC = x86_64-elf-gcc

#compiler flags

CFLAGS = -Wall -Wextra -O2 -pipe -I stivale/

# internal flags that shouldnt be changed

INTERNALLDFLAGS :=     \
	-fno-pic -fpie \
	-Wl,-static,-pie,--no-dynamic-linker,-ztext \
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
	-mno-red-zone

#c files and object

CFILES := $(wildcard kernel/kernel/*.c)
OBJ := $(CFILES:.c=.o)

.PHONY: all clean

all: $(KERNEL)

$(KERNEL): $(OBJ)
	$(CC) $(INTERNALLDFLAGS) $(OBJ) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@

clean:
	rm -rf $(KERNEL) $(OBJ)
	rm -rf d.img

image:
	./image.sh

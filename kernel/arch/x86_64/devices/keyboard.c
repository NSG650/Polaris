#include <io/ports.h>
#include <sys/isr.h>
#include <asm/asm.h>
#include <debug/debug.h>
#include <sys/apic.h>
#include <devices/keyboard.h>
#include <fb/fb.h>

// Hello old friend

bool keyboard_typed = 0;

static char ktoc(uint8_t key){
    char c = 0;
    uint8_t dict[2][94] = {
        {57,40,51,12,52,53,11,2 ,3, 4, 5, 6, 7, 8, 9, 10,39,13,26,43,27,41,30,48,46,32, 18, 33, 34, 35, 23, 36, 37, 38, 50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44},
        {32,39,44,45,46,47,48,49,50,51,52,53,54,55,56,57,59,61,91,92,93,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122}
    };
    for(int i = 0; i < 94; i++){
        if(dict[0][i]==key){
            c = (char)dict[1][i];
        }
    }
    return c;
}

static char ktocSHIFT(uint8_t key){
    char c = 0;
    uint8_t dict[2][94] = {
        {41, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,26, 27, 43, 39,40,51,52,53,30,48,46,32,18,33,34,35,23,36,37,38,50,49,24,25,16,19,31,20,22,47,17,45,21,44,57},
        {126,33,64,35,36,37,94,38,42,40,41,95,43,123,125,124,58,34,60,62,63,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,32}
    };
    for(int i = 0; i < 94; i++){
        if(dict[0][i]==key){
            c = (char)dict[1][i];
        }
    }
    return c;
}


uint8_t keyboard_read(void) {
    while ((inb(0x64) & 1) == 0);
    return inb(0x60);
}

void keyboard_write(uint16_t port, uint8_t value) {
    while ((inb(0x64) & 2) != 0);
    outb(port, value);
}

uint8_t keyboard_read_config(void) {
    keyboard_write(0x64, 0x20);
    return keyboard_read();
}

void keyboard_write_config(uint8_t value) {
    keyboard_write(0x64, 0x60);
    keyboard_write(0x60, value);
}

char keyboard_getchar(void) {
	while (!keyboard_typed);
	char c = keyboard_read();
	keyboard_typed = 0;
	return ktoc(c);
}

void keyboard_gets(char *string, size_t count) {
	uint8_t key = 0;
	size_t c = 0;
	while (key != 0x1c && c != count) {
		while (!keyboard_typed);
		key = keyboard_read();
		if (key == 0xE && c) {
			string[c--] = '\0';
			framebuff.tex_x--;
			framebuffer_putchar(' ');
			framebuff.tex_x--;
			continue;
		}
		if (key == 0x2A) {
			for (;;) {
				while (!keyboard_typed);
				key = keyboard_read();
				if (key == 0xAA)
					break;
				if (key == 0xE && c) {
					string[c--] = '\0';
					framebuff.tex_x--;
					framebuffer_putchar(' ');
					framebuff.tex_x--;
					continue;
				}
				if(ktocSHIFT(key) == 0){
                    continue;
                }
				char a = string[c++] = ktocSHIFT(key);
				framebuffer_putchar(a);
			}
		}
		if(ktoc(key) == 0){
            continue;
        }
        char a = string[c++] = ktoc(key);
		framebuffer_putchar(a);
	}
	framebuffer_putchar('\n');
	string[c] = '\0';
}

void keyboard_handle(registers_t *reg) {
	(void)reg;
	keyboard_typed = 1;
	apic_eoi();
}

void keyboard_init(void) {
	// Disable primary and secondary PS/2 ports
    keyboard_write(0x64, 0xad);
    keyboard_write(0x64, 0xa7);

    uint8_t keyboard_config = keyboard_read_config();
    // Enable keyboard interrupt and keyboard scancode translation
    keyboard_config |= (1 << 0) | (1 << 6);
    // Enable mouse interrupt if any
    if ((keyboard_config & (1 << 5)) != 0) {
        keyboard_config |= (1 << 1);
    }
    keyboard_write_config(keyboard_config);

    // Enable keyboard port
    keyboard_write(0x64, 0xae);

	isr_register_handler(49, keyboard_handle);
	ioapic_redirect_irq(1, 49);
	inb(0x60);
}

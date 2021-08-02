#include "../video/video.h"
#include "../klibc/string.h"
#include "panic.h"
#include "../klibc/printf.h"
#include "../serial/serial.h"

void panic(char message[], char file[]) {
	vid_reset();
	clear_screen(0xB80000);
    kprintbgc("*** PANIC: ", 0xFFFFFF, 0xB80000);
	kprintbgc(message, 0xFFFFFF, 0xB80000);
    kprintbgc("\n\nSystem Halted", 0xFFFFFF, 0xB80000);
    char x[1024];
    sprintf(x, "\n*** PANIC: %s\nFile: %s\n", message, file);
    write_serial(x);
    for(;;)
        __asm__("cli\nhlt");
}

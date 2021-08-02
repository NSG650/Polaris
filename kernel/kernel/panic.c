#include "../video/video.h"
#include "../klibc/string.h"
#include "panic.h"
#include "../klibc/printf.h"
#include "../serial/serial.h"

void panic(char message[], char file[], char assert, uint32_t line) {
    char x[1024];
	vid_reset();
    if(assert) {
        clear_screen(0x00B800);
        sprintf(x, "*** ASSERTION FAILURE: %s\nFile: %s\nLine: %u", message, file, line);
        kprintbgc(x, 0xFFFFFF, 0x00B800);
    }
    else {
        clear_screen(0xB80000);
        sprintf(x, "*** PANIC: %s\nFile: %s\nLine: %u", message, file, line);
        kprintbgc(x, 0xFFFFFF, 0xB80000);
    }
    write_serial(x);
    for(;;)
        __asm__("cli\nhlt");
}

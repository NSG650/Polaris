#include "../video/video.h"
#include "../klibc/string.h"
#include "panic.h"

void panic(char message[]) {
	vid_reset();
	clear_screen(0xB80000);
    kprintbgc("*** PANIC: ", 0xFFFFFF, 0xB80000);
	kprintbgc(message, 0xFFFFFF, 0xB80000);
    kprintbgc("\n\nSystem Halted", 0xFFFFFF, 0xB80000);
    for(;;)
        __asm__("hlt");
}

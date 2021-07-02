#include "../video/video.h"
#include "../klibc/string.h"
#include "die.h"

char* errmsg[] = {
	"SYSTEM_SERVICE_EXCEPTION_NOT_HANDLED",
    "ASSERTION_FAILURE"
};

void die(int code) {
	dieex(code, 0, 0, 0, 0);
}

void dieex(int code, int code0, int code1, int code2, int code3) {
	vidreset();
	clear_screen(0xB80000);
	char msg[15];
	char msg0[15];
	char msg1[15];
	char msg2[15];
	char msg3[15];
	hex_to_ascii_upper(code, msg);
    hex_to_ascii_upper(code0, msg0);
	hex_to_ascii_upper(code1, msg1);
	hex_to_ascii_upper(code2, msg2);
	hex_to_ascii_upper(code3, msg3);
    if(!code0) {
        strcpy(msg0, "NONE");
    }
    if(!code1) {
        strcpy(msg1, "NONE");
    }
    if(!code2) {
        strcpy(msg2, "NONE");
    }
    if(!code3) {
        strcpy(msg3, "NONE");
    }
	kprintbgc("*** STOP: ", 0xFFFFFF, 0xB80000);
	kprintbgc(msg, 0xFFFFFF, 0xB80000);
	kprintbgc(" (", 0xFFFFFF, 0xB80000);
	kprintbgc(msg0, 0xFFFFFF, 0xB80000);
	kprintbgc(", ", 0xFFFFFF, 0xB80000);
	kprintbgc(msg1, 0xFFFFFF, 0xB80000);
	kprintbgc(", ", 0xFFFFFF, 0xB80000);
	kprintbgc(msg2, 0xFFFFFF, 0xB80000);
	kprintbgc(", ", 0xFFFFFF, 0xB80000);
	kprintbgc(msg3, 0xFFFFFF, 0xB80000);
	kprintbgc(")\n", 0xFFFFFF, 0xB80000);
	if(code > sizeof(errmsg)) {
		kprintbgc("\n\nSystem Halted", 0xFFFFFF, 0xB80000);
		for(;;)
			__asm__("cli\nhlt");
	}
	kprintbgc(errmsg[code - 1], 0xFFFFFF, 0xB80000);
	kprintbgc("\n\nSystem Halted", 0xFFFFFF, 0xB80000);
	for(;;)
		__asm__("cli\nhlt");
}

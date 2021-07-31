#include "printf.h"
#include "../video/video.h"
#include "../serial/serial.h"
#include "debug.h"

extern uint64_t global_tick;
char v_out_debug = 1;

void enable_v_out_debug(char yes) {
    v_out_debug = yes;
}

void kprintf(char* format, ...) {
    va_list va;
    va_start(va, format);
    char buffer[1024];
    printf("[%d] ", global_tick);
    sprintf(buffer, "[%d] ", global_tick);
    write_serial(buffer);
    memset(buffer, 0, 1024);
    printf(format, va);
    sprintf(buffer, format, va);
    write_serial(buffer);
    va_end(va);
}



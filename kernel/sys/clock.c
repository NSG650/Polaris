#include "clock.h"
#include "../cpu/ports.h"
#include "../klibc/printf.h"

static int is_updating(void) {
    port_byte_out(0x70, 0x0A);
    return port_byte_in(0x71) & 0x80;
}

static unsigned char read(int reg) {
    while (is_updating())
        ;
    port_byte_out(0x70, reg);

    return port_byte_in(0x71);
}

unsigned char clock_get_seconds(void) {
    unsigned char seconds = read(0);
    unsigned char second = (seconds & 0x0F) + ((seconds / 16) * 10);
    return second;
}

unsigned char clock_get_minutes(void) {
    unsigned char minutes = read(0x2);
    unsigned char minute = (minutes & 0x0F) + ((minutes / 16) * 10);
    return minute;
}

unsigned char clock_get_hours(void) {
    unsigned char hours = read(0x4);
    unsigned char hour = ((hours & 0x0F) + (((hours & 0x70) / 16) * 10)) | (hours & 0x80);
    return hour;
}

uint64_t get_unix_timestamp(void) {
   uint32_t y, m, d;
   uint64_t t;
   y = read(0x9) + 2000; // do you really think this project will exist in 2100?
   m = read(0x8);
   d = read(0x7);
   if(m <= 2) {
       m += 12;
       y -= 1;
   }
    //Convert years to days
    t = (365 * y) + (y / 4) - (y / 100) + (y / 400);
    //Convert months to days
    t += (30 * m) + (3 * (m + 1) / 5) + d;
    //Unix time starts on January 1st, 1970
    t -= 719561;
    //Convert days to seconds
    t *= 86400;
    //Add hours, minutes and seconds
    t += (3600 * clock_get_hours()) + (60 * clock_get_minutes()) + clock_get_seconds();

    return t;
}

#ifndef PANIC_H
#define PANIC_H

void panic(char message[]);

#define ASSERT(b) ((b) ? (void)0 : panic("Assertion failure");

#endif

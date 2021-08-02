#ifndef PANIC_H
#define PANIC_H

void panic(char message[], char file[]);

#define PANIC(b) (panic(b, __FILE__));
#define ASSERT(b) ((b) ? (void)0 : panic("Assertion failure", __FILE__);

#endif

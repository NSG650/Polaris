#ifndef DIE_H
#define DIE_H

void die(int code);
void dieex(int code, int code0, int code1, int code2, int code3);

#define ASSERT(b) ((b) ? (void)0 : dieex(0x2, __LINE__, NULL, NULL, NULL);

#endif

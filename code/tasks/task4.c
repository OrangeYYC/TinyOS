#include "../kernel/defs.h"
extern void PrintAtPos(char *message, int color, int x, int y);

void _start() {/*
    while (1) {
        u16 code = 'D' | (7 << 8);
        u32 pos  = 80 * 16 * 2;
        __asm__ __volatile__ (
            "movw   %%ax, %%gs:(%%ebx)\n"
            :: "a"(code), "b"(pos)
        );
    }*/

    while (1)
        PrintAtPos("      MRSU (TASK D)      ", F_Cyan | B_Cyan | L_Light, 16, 30);
}
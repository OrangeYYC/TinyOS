#include "lib.h"

// 写显存辅助函数
static void WriteToVedio(u32 code, u32 pos) {
    __asm__ __volatile__ (
        "movw   %%ax, %%gs:(%%ebx)\n"
        :: "a"(code), "b"(pos)
    );
}

// 字符串定位输出函数
void PrintAtPos(char *message, int color, int x, int y) {
    for (char *c = message; *c; c++) {
        u32 code = ((*c) & 0xff) | color;
        u32 pos  = (80 * x + y++) * 2;
        WriteToVedio(code, pos);
    }
}
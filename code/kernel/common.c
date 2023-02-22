//  common.c         by OrangeYYC
//  TinyOS 全局共享的函数与变量的定义

#include "defs.h"

/* ========================== 全局共享变量 ========================== */
u32         RAMSize            = 0;    // 系统内存大小
u32         MemoryEntryCount   = 0;    // 描述符数量
ARD         ARDs[10]           = {};   // 描述符结构体
u8          gdtPtr[6]          = {};   // 全局描述符表指针
Descriptor  gdt[GDT_SIZE]      = {};   // 全局描述符表
u8          idtPtr[6]          = {};   // 中断向量表指针
Gate        idt[IDT_SIZE]      = {};   // 中断向量表
PCB         process[MAX_TASKS] = {};   // 进程表
TSS         tss                = {};   // 任务状态段
u32         readyPid           = -1;   // 就绪 pid

/* ========================== 信息显示函数 ========================== */
int dispX = 0;                                  // 当前光标所在行
int dispY = 0;                                  // 当前光标所在列

// 写显存辅助函数
void WriteToVedio(u32 code, u32 pos) {
    __asm__ __volatile__ (
        "movw   %%ax, %%gs:(%%ebx)\n"
        :: "a"(code), "b"(pos)
    );
}

// 字符串按行输出函数
void Print(char *message, int color) {
    for (char *c = message; *c; c++) {
        if ((*c) == '\n') {
            dispX++;
            dispY = 0;
            continue;
        }
        u32 code = ((*c) & 0xff) | color;
        u32 pos  = (80 * dispX + dispY++) * 2;
        WriteToVedio(code, pos);
    }
}

// 字符串定位输出函数
void PrintAtPos(char *message, int color, int x, int y) {
    for (char *c = message; *c; c++) {
        u32 code = ((*c) & 0xff) | color;
        u32 pos  = (80 * x + y++) * 2;
        WriteToVedio(code, pos);
    }
}

// 数字输出函数
void PrintNumber(u32 value, int color) {
    char buffer[20];
	char *p = buffer;
    for (int i = 28; i >= 0; i-= 4) {
        char ch = (value >> i) & 0xf;
        if (ch >= 0){
            ch += '0';
            if (ch > '9')
                ch += 7;
            *p++ = ch;
        }
    }
	*p = 0;
    Print(buffer, color);
}

/* ========================== 端口输入输出函数 ========================== */
// 用于向端口写入信息的函数
void OutByte(u16 port, u8 value) {
    __asm__ __volatile__ (
        "outb %%al, %%dx\n"
        "nop\n"                     // 等待端口写入完成 进行两次 nop
        "nop\n"
        :: "d"(port), "a"(value)
    );
}

// 用于从端口读入信息的函数
u8 InByte(u16 port) {
    u8 rv;
    __asm__ __volatile__ (
        "inb %%dx, %%al\n"
        : "=a"(rv)
        : "d"(port)
    );
    return rv;
}

// 用于从端口读入信息的函数
u16 InWord(u16 port) {
    u16 rv;
    __asm__ __volatile__ (
        "in %%dx, %%ax\n"
        : "=a"(rv)
        : "d"(port)
    );
    return rv;
}

/* ========================== 描述符设置函数 ========================== */
// 设置全局/局部描述符的函数
void SetDesEntry(Descriptor *des, u32 base, u32 limit, u16 attr) {
	des->limitLow       = limit & 0xffff;
    des->baseLow        = base  & 0xffff;
    des->baseMid        = (base >> 16) & 0xff;
    des->attr1          = attr & 0xff;
    des->limitHighAttr2 = ((attr & 0xf000) >> 8) | ((limit & 0xf0000) >> 16);
    des->baseHigh       = (base >> 24) & 0xff;
}

// 根据选择子(16位)、偏移(32位)、参数数量(8位)、属性(8位) 初始化中断门
void SetIdtEntry(Gate *pGate, u16 selector, u32 offset, u8 dcount, u8 attr) {
    pGate->offsetLow  = offset & 0xffff;
    pGate->selector   = selector;
    pGate->dcount     = dcount & 0x1f;
    pGate->attr       = attr & 0xff;
    pGate->offsetHigh = (offset >> 16) & 0xffff; 
}

/* ========================== 磁盘读写函数 ========================== */
// 磁盘读写函数
// sector: 读取的扇区编号, buffer: 读取到的内存地址
void ReadDisk(u32 sector, u32 buffer) {
    // 设置读取参数
    OutByte(0x1f1, 0);
    OutByte(0x1f2, 1);
    OutByte(0x1f3, (u8)sector);
    OutByte(0x1f4, (u8)(sector >> 8));
    OutByte(0x1f5, (u8)(sector >> 16));
    // 使用 LBA 模式读取，选择主盘
    OutByte(0x1f6, 0xE0 | (u8)((sector >> 24) & 0xF));
    OutByte(0x1f7, 0x20);
    // 循环读取数据
    u16 *data = (u16 *)buffer;
    while ((InByte(0x1f7) & 0x88) != 0x8);      // 等待数据准备就绪
    for (int i = 0; i < 512 / 2; i++)
        data[i] = InWord(0x1f0);
}
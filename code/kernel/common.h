//  common.h         by OrangeYYC
//  TinyOS 全局共享的函数与变量的定义

#ifndef	_COMMON_H_
#define	_COMMON_H_

#include "defs.h"

// 全局可用的功能函数
extern void WriteToVedio (u32 code, u32 pos);
extern void Print        (char *message, int color);
extern void PrintAtPos   (char *message, int color, int x, int y);
extern void PrintNumber  (u32 value, int color);
extern void OutByte      (u16 port, u8 value);
extern   u8 InByte       (u16 port);
extern  u16 InWord       (u16 port);
extern void SetDesEntry  (Descriptor *des, u32 base, u32 limit, u16 attr);
extern void SetIdtEntry  (Gate *pGate, u16 selector, u32 offset, u8 dcount, u8 attr);
extern void ReadDisk     (u32 sector, u32 buffer);

// 数据段中定义的变量
extern u32         RAMSize;             // 系统内存大小
extern u32         MemoryEntryCount;    // 描述符数量
extern ARD         ARDs[10];            // 描述符结构体
extern u8          gdtPtr[6];           // 全局描述符表指针
extern Descriptor  gdt[GDT_SIZE];       // 全局描述符表
extern u8          idtPtr[6];           // 中断向量表指针
extern Gate        idt[IDT_SIZE];       // 中断向量表
extern PCB         process[MAX_TASKS];  // 进程表
extern TSS         tss;                 // 任务状态段
extern u32         readyPid;            // 就绪 pid

#endif
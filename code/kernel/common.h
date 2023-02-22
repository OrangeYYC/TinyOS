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

#endif
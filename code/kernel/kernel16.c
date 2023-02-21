/* TinyOS 内核 —— 实模式执行程序
内存空间: 0x200000 4M 内存空间
    0x000000 -> 0x000400  BIOS 加载的中断向量表
    0x000400 -> 0x000500  BIOS 参数的相关区域
    0x000500 -> 0x007c00  TinyOS 引导程序的栈空间
    0x007c00 -> 0x007e00  TinyOS 引导程序代码段和数据段
    0x007e00 -> 0x090000  空闲空间
    0x090000 -> 0x09e000  TinyOS 内核程序代码段和数据段
    0x09e000 -> 0x09efff  TinyOS 内核的栈空间
    0x100000 -> 0x1FFFFF  空闲空间
运行模式: 16 位实模式
段寄存器: CS = DS = ES = SS = 0x9000
功能：设置全局描述符表，并跳入保护模式
*/

/* ========================== 初始化代码段 ========================== */
asm (
    ".code16gcc\n"
    "movw   %cs, %ax\n"         // 设置 cs = ds = es = ss = 0x9000
    "movw   %ax, %ds\n"
    "movw   %ax, %es\n"
    "movw   %ax, %ss\n"
    "movw   $0xefff, %sp\n"     // 设置 sp = 0x9000:0xefff 向低地址处增长
    "call   Kernel16Main\n"     // 调用内核主函数
);

#include "defs.h"
 
static void CheckMemory();
static void SetGdt();
static void SwitchProtectMode();

/* ========================== 内核主程序 ========================== */
static void Kernel16Main() {
    // 首先检查可用内存
    CheckMemory();
    // 设置全局描述符表
    SetGdt();
    // 进入保护模式
    SwitchProtectMode();
}

/* ========================== 检查系统内存 ========================== */
extern u32                    MemoryEntryCount;    // 描述符数量
extern AddressRangeDescriptor ARDs[10];            // 描述符结构体

// 检查内存函数
static void CheckMemory() {
    u32 flag = 0, addr = (u32)ARDs;
    while (1) {
        __asm__ __volatile__ (
            "int    $0x15\n"
            : "+b"(flag)
            : "a"((u32)0xe820), "c"((u32)20) , "d"((u32)0x534d4150), "D"(addr)
        );
        if (!flag)
            break;
        addr += 20;
        MemoryEntryCount += 1;
    }
}

/* ========================== 设置全局描述符表 ========================== */
extern u8          gdtPtr[6];      // 0-15: 限界，16-47: 基地址
extern Descriptor  gdt[GDT_SIZE];  // 全局描述符表

// 描述符初始化函数
// 根据基地址(32位)、限界(20位)、属性(12位) 设置段描述符
static void SetGdtEntry(Descriptor *des, u32 base, u32 limit, u16 attr) {
	des->limitLow       = limit & 0xffff;
    des->baseLow        = base  & 0xffff;
    des->baseMid        = (base >> 16) & 0xff;
    des->attr1          = attr & 0xff;
    des->limitHighAttr2 = ((attr & 0xf000) >> 8) | ((limit & 0xf0000) >> 16);
    des->baseHigh       = (base >> 24) & 0xff;
}

// 全局描述符表初始化函数
static void SetGdt() {
    SetGdtEntry(&gdt[INDEX_DUMMY], 0, 0, 0);
    // flat 代码段 DPL0 (内核级)
    SetGdtEntry(&gdt[INDEX_FLAT_C], 0x90000, 0xfffff, DA_CR + DA_32 + DA_LIMIT_4K);
    // flat 数据段 DPL0 (内核级)
    SetGdtEntry(&gdt[INDEX_FLAT_RW], 0x90000, 0xfffff, DA_DRW + DA_32 + DA_LIMIT_4K);
    // 显存段 DPL3 (用户级)
    SetGdtEntry(&gdt[INDEX_VIDEO], 0xb8000, 0xffff, DA_DRW + DA_DPL3);
    u16* pGdtLimit = (u16*)(&gdtPtr[0]);
	u32* pGdtBase  = (u32*)(&gdtPtr[2]);
	*pGdtLimit = GDT_SIZE * sizeof(Descriptor) - 1;
	*pGdtBase  = (u32)(&gdt) + 0x90000;
}

/* ========================== 转到保护模式 ========================== */
extern void Kernel32Main();         // 保护模式入口函数

// 用于转入保护模式的函数
static void SwitchProtectMode() {
    __asm__ __volatile__ (
        "cli\n"                     // 关中断
        "lgdt   %0\n"               // 加载全局描述符表
        "inb    $0x92, %%al\n"      // 打开地址线
        "orb    $0x2, %%al\n"
        "outb   %%al, $0x92\n"
        "movl   %%cr0, %%eax\n"     // 修改控制寄存器
        "orl    $0x1, %%eax\n"
        "movl   %%eax, %%cr0\n"
        "ljmp   $0x8, $%1\n"        // 长跳转进入保护模式
        :: "m"(gdtPtr), "m"(Kernel32Main)
    );
}
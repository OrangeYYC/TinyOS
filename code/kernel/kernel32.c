//  kernel32.c         by OrangeYYC
//  TinyOS 保护模式 32 位内核程序部分

/* TinyOS 内核 —— 保护模式执行程序
内存空间: 0x200000 4M 内存空间
    0x000000 -> 0x000400  BIOS 加载的中断向量表
    0x000400 -> 0x000500  BIOS 参数的相关区域
    0x000500 -> 0x007c00  TinyOS 引导程序的栈空间
    0x007c00 -> 0x007e00  TinyOS 引导程序代码段和数据段
    0x007e00 -> 0x090000 空闲空间(计划分给内核)
        0x007e00 -> 0x007fff 内核程序的栈空间
        0x008000 -> 0x00Bfff 内核程序的代码与数据(IDT GDT TSS 均在这个部分)
        0x00C000 -> 0x09efff 页目录和页表的区域
            0x0010000 -> 0x001ffff 内核程序的页表 
                0x010000 -> 0x0100ff 内核程序的页目录表
                0x011000 -> 0x018fff 内核程序的页表
            0x0020000 -> 0x002ffff 1号任务的页表
            ...
            0x0090000 -> 0x009e000 8号任务的页表 
        0x00C000
    0x100000 -> 0x1FFFFF  空间空间(计划划分给用户)
        0x100000 -> 0x10ffff 用户进程 1
        0x110000 -> 0x11ffff 用户进程 2
        ...
        0x170000 -> 0x17ffff 用户进程 8
        0x180000 -> 0x18ffff 加载时的备用存储空间

运行模式: 32 位保护模式
段寄存器: CS = DS = ES = SS = 0
功能：开启分页管理 设置中断向量表
*/

#include "common.h"

/* ========================== 显示系统内存 ========================== */
// 显示系统内存函数
static void CheckMemory() {
    Print("[KERNEL] Testing System Memory\n", F_Cyan | L_Light);
    Print("           Offset      Size       State\n", F_White);
    for (int i = 0; i < MemoryEntryCount; i++) {
        PrintNumber(ARDs[i].BaseAddrLow, F_White);
        Print("-", F_White);
        PrintNumber(ARDs[i].BaseAddrLow + ARDs[i].LengthLow - 1, F_White);
        Print("  ", F_White);
        PrintNumber(ARDs[i].LengthLow, F_White);
        if (ARDs[i].Type == 1)
            Print("    Free", F_Green | L_Light);
        else
            Print("    Reserved", F_White);
        Print("\n", F_White);
        if (ARDs[i].BaseAddrLow + ARDs[i].LengthLow > RAMSize)
            RAMSize = ARDs[i].BaseAddrLow + ARDs[i].LengthLow;
    }
    Print("RAM Size: ", F_White);
    PrintNumber(RAMSize, F_White | L_Light);
    Print("\n", F_White);
}

/* ========================== 启动分页机制 ========================== */
// 启动分页机制主函数
static void SetupPaging() {
    Print("[KERNEL] Starting Memory Paging\n", F_Cyan | L_Light);
    // 计算需要的页目录的个数
    u32 PDECount = RAMSize / 0x400000 + (RAMSize % 0x400000 > 0 ? 1 : 0);
    u32 *PDE     = (u32*) PAGE_DIR_BASE;
    u32 *PTE     = (u32*) PAGE_TABLE_BASE;
    // 设置 线性地址 = 虚拟地址 的页表
    for (u32 i = 0; i < PDECount; i++)
        PDE[i] = (PAGE_TABLE_BASE + (i<<12)) | PAGE_P | PAGE_U | PAGE_W;
    for (u32 i = 0; i < PDECount * 1024; i++)
        PTE[i] = (i<<12) | PAGE_P | PAGE_U | PAGE_W;
    // 设置 cr3 寄存器为页表基地址，设置 cr0 寄存器开启分页机制
    __asm__ __volatile__ (
        "movl %%eax, %%cr3\n"
        "movl %%cr0, %%eax\n"
        "orl  $0x80000000, %%eax\n"
        "movl %%eax, %%cr0\n"
        ::"a"(PAGE_DIR_BASE)
    );
}

/* ========================== 装载TSS函数 ========================== */
static void SetupTSS() {
    // TSS 所有的元素设置为 0
    for (int i = 0; i < sizeof(TSS); i++)
        ((char *)(&tss))[i] = 0;
    tss.ss0 = SELECTOR_FLAT_RW;     // Ring0 段寄存器为 flatRW
    tss.iobase = sizeof(TSS);
    SetDesEntry(&gdt[INDEX_TSS], (u32)&tss, sizeof(TSS) - 1, DA_386TSS);
    __asm__ __volatile__ (
        "ltr %%ax"
        :: "a"(SELECTOR_TSS)
    );
}

/* ========================== 内核主函数 ========================== */
// 导入重要功能
extern void SetupIdt();
extern void SetupProcess();
extern void restart();
extern void choose();

// 内核主功能函数
void Kernel32Main() {
    // 初始化寄存器
    __asm__ __volatile__ (
        "movw   $0x10, %%ax\n"
        "movw   %%ax, %%ds\n"
        "movw   %%ax, %%es\n"
        "movw   %%ax, %%fs\n"
        "movw   %%ax, %%ss\n"
        "movw   $0x1B, %%ax\n"
        "movw   %%ax, %%gs\n"
        "movl   $0x7fff, %%esp\n"
        ::: "eax"
    );
    // 显示当前模式信息
    Print("[KERNEL] In Protect Mode Now\n", F_Brown | L_Light);
    // 检查系统内存
    CheckMemory(); 
    // 开启分页机制                     
    SetupPaging();
    // 初始化 8259A 并建立中断向量表
    SetupIdt();
    // 设置 TSS
    SetupTSS();
    // 初始化进程表
    SetupProcess();
    Print("[KERNEL] All Done! Start to do tasks ...\n", F_Brown | L_Light);
    // 选择并执行任务
    choose();
    restart();
}
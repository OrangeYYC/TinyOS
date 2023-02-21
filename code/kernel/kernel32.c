/* TinyOS 内核 —— 保护模式执行程序
内存空间: 0x200000 4M 内存空间
    0x000000 -> 0x000400  BIOS 加载的中断向量表
    0x000400 -> 0x000500  BIOS 参数的相关区域
    0x000500 -> 0x007c00  TinyOS 引导程序的栈空间
    0x007c00 -> 0x007e00  TinyOS 引导程序代码段和数据段
    0x007e00 -> 0x090000  空闲空间
    0x090000 -> 0x09e000  TinyOS 内核程序代码段和数据段
        包含 GDT、IDT
    0x09e000 -> 0x09efff  TinyOS 内核的栈空间
    0x100000 -> 0x1FFFFF  空闲空间
        0x100000 -> 0x101000 页目录表
        0x101000 -> 连续空间 页表
运行模式: 32 位保护模式
段寄存器: CS = DS = ES = SS = 0x9000
功能：开启分页管理 设置中断向量表
*/

#include "defs.h"

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

/* ========================== 获取系统内存 ========================== */
extern u32                    RAMSize          ;
extern u32                    MemoryEntryCount ;
extern AddressRangeDescriptor ARDs[10]         ;

// 获取系统内存函数
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
        PDE[i] = (PAGE_TABLE_PBASE + (i<<12)) | PAGE_P | PAGE_U | PAGE_W;
    for (u32 i = 0; i < PDECount * 1024; i++)
        PTE[i] = (i<<12) | PAGE_P | PAGE_U | PAGE_W;
    // 设置 cr3 寄存器为页表基地址，设置 cr0 寄存器开启分页机制
    __asm__ __volatile__ (
        "movl %%eax, %%cr3\n"
        "movl %%cr0, %%eax\n"
        "orl  $0x80000000, %%eax\n"
        "movl %%eax, %%cr0\n"
        ::"a"(PAGE_DIR_PBASE)
    );
}

/* ========================== 设置 8259A ========================== */
// 用于向端口写入信息的函数
static void OutByte(u16 port, u8 value) {
    __asm__ __volatile__ (
        "outb %%al, %%dx\n"
        "nop\n"                     // 等待端口写入完成 进行两次 nop
        "nop\n"
        :: "d"(port), "a"(value)
    );
}

// 设置 8259A 的主函数
static void Init8259A() {
    Print("[KERNEL] Init 8259A\n", F_Cyan | L_Light);
    // 写入 ICW1 设置为边缘触发模式，使用 8 字节中断向量的级联模式
    OutByte(INT_M_CTL, 0x11);
    OutByte(INT_S_CTL, 0x11);
    // 写入 ICW2 设置中断向量编号为 0x20 起始
    OutByte(INT_M_CTLMASK, INT_VECTOR_IRQ0);
    OutByte(INT_S_CTLMASK, INT_VECTOR_IRQ8);
    // 写入 ICW3 设置 IR2 级联从片
    OutByte(INT_M_CTLMASK, 0x4);
    OutByte(INT_S_CTLMASK, 0x2);
    // 写入 ICW4 设置为 80x86 模式
    OutByte(INT_M_CTLMASK, 0x1);
    OutByte(INT_S_CTLMASK, 0x1);
    // 写入 OCW 选择屏蔽中断
    OutByte(INT_M_CTLMASK, 0xfe);       // 此处只打开了时钟中断
    OutByte(INT_S_CTLMASK, 0xff);
}

/* ========================== 初始化中断向量表 ========================== */
extern u8   idtPtr[6];             // 中断向量表指针
extern Gate idt[IDT_SIZE];         // 中断向量表格

void SimpleIntWarpper();    // 一般中断处理函数
void ClockIntWarpper();     // 时钟中断处理函数

// 根据选择子(16位)、偏移(32位)、参数数量(8位)、属性(8位) 初始化中断门
static void SetIdtEntry(Gate *pGate, u16 selector, u32 offset, u8 dcount, u8 attr) {
    pGate->offsetLow  = offset & 0xffff;
    pGate->selector   = selector;
    pGate->dcount     = dcount & 0x1f;
    pGate->attr       = attr & 0xff;
    pGate->offsetHigh = (offset >> 16) & 0xffff; 
}

// 设置中断向量表的函数
static void SetupIdt() {
    Print("[KERNEL] Setup IDT\n", F_Cyan | L_Light);
    // 初始化 idt 所有中断统一用 SimpleIntWarpper，时钟中断使用 ClockIntWarpper
    for (int i = 0; i < IDT_SIZE; i++) 
        SetIdtEntry(&idt[i], SELECTOR_FLAT_C, (u32)SimpleIntWarpper, 0, DA_386IGate);
    SetIdtEntry(&idt[0x20], SELECTOR_FLAT_C, (u32)ClockIntWarpper, 0, DA_386IGate);
    // 加载 idt
    u16* pIdtLimit = (u16*)(&idtPtr[0]);
	u32* pIdtBase  = (u32*)(&idtPtr[2]);
	*pIdtLimit = IDT_SIZE * sizeof(Gate) - 1;
	*pIdtBase  = (u32)(&idt) + 0x90000;
    __asm__ __volatile__ (
        "cli\n"
        "lidt %0\n"
        ::"m"(idtPtr)
    );
}

/* ========================== 进程管理函数 ========================== */
int                        reEnter = -1;            // 重入判断标志
int                        readyPid;                // 就绪 pid
extern Descriptor          gdt[GDT_SIZE];           // 全局描述符表
extern ProcessControlBlock process[MAX_TASKS];      // 进程表
extern Task                tasks[MAX_TASKS];        // 任务结构体
extern int                 taskCount;               // 任务数量
extern TSS                 tss;                     // TSS 结构

// 设置描述符的函数
static void SetDesEntry(Descriptor *des, u32 base, u32 limit, u16 attr) {
	des->limitLow       = limit & 0xffff;
    des->baseLow        = base  & 0xffff;
    des->baseMid        = (base >> 16) & 0xff;
    des->attr1          = attr & 0xff;
    des->limitHighAttr2 = ((attr & 0xf000) >> 8) | ((limit & 0xf0000) >> 16);
    des->baseHigh       = (base >> 24) & 0xff;
}

// 设置进程的函数
static void SetupProcess() {
    // 初始化重入标志
    reEnter = -1;

    // 初始化进程
    for (int i = 0; i < taskCount; i++) {
        ProcessControlBlock *pcb = &process[i];
        // 设置基本信息
        pcb->pid = i;
        pcb->priority = tasks[i].priority;
        pcb->tick = tasks[i].priority;
        // 填充 GDT 表中的 LDT 描述符
        SetDesEntry(&gdt[INDEX_LDT_FIRST + i], (u32)pcb->ldts + 0x90000, LDT_SIZE * sizeof(Descriptor) - 1, DA_LDT);
        // 初始化局部描述符表
        pcb->ldtSelector = SELECTOR_LDT_FIRST + 0x8 * i;
        SetDesEntry(&pcb->ldts[0], 0x90000, 0xfffff, DA_C | DA_DPL3 | DA_32);   // 用户级的平坦代码段
        SetDesEntry(&pcb->ldts[1], 0x90000, 0xfffff, DA_DRW | DA_DPL3);         // 用户级的平坦数据段
        // 初始化段寄存器
        pcb->regs.cs = (0x0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | SA_RPL3;
        pcb->regs.ds = (0x8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | SA_RPL3;
        pcb->regs.es = (0x8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | SA_RPL3;
        pcb->regs.fs = (0x8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | SA_RPL3;
        pcb->regs.ss = (0x8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | SA_RPL3;
        pcb->regs.gs = (SELECTOR_VIDEO & SA_RPL_MASK) | SA_RPL3;
        // 初始化任务
        pcb->regs.eip = (u32)tasks[i].eip;
        // 初始化栈空间
        pcb->regs.esp = (u32)tasks[i].stackBase + tasks[i].stackSize;
        // 初始化标志寄存器
        pcb->regs.eflags = 0x1202;
    }
}

// 选择函数
static void choose() {
    int maxTick = 0, maxId = -1;
    // 寻找最大 tick 程序
    for (int i = 0; i < taskCount; i++) {
        if (process[i].tick > maxTick) {
            maxTick = process[i].tick;
            maxId   = i;
        }
    }
    // 若全部为 0 则重置进度并再次选择
    if (maxId == -1) {
        for (int i = 0; i < taskCount; i++)
            process[i].tick = tasks[i].priority;
        choose();
    } else
        readyPid = maxId;
}

// 重启函数
static void restart() {
    // 当下次中断发生的时候，返回到的内核栈为对应 process 的 stack frame
    tss.esp0 = sizeof(StackFrame) + (u32)&process[readyPid];
    __asm__ __volatile__ (
        "movl   %0, %%esp\n"        // 转移堆栈到 stack frame
        "lldt   %1\n"               // 加载局部描述符表
        "pop    %%gs\n"             // 恢复寄存器
        "pop    %%fs\n"
        "pop    %%es\n"
        "pop    %%ds\n"
        "popal\n"
        "sti\n"                     // 开中断
        "iretl\n"                   // 返回对应的进程，进入 Ring3
        :: "r"(&process[readyPid]), "m"(process[readyPid].ldtSelector)
    );

}

/* ========================== 装载TSS函数 ========================== */
extern TSS tss;     // TSS 结构体

static void SetupTSS() {
    // TSS 所有的元素设置为 0
    for (int i = 0; i < sizeof(TSS); i++)
        ((char *)(&tss))[i] = 0;
    tss.ss0 = SELECTOR_FLAT_RW;     // Ring0 段寄存器为 flatRW
    tss.iobase = sizeof(TSS);
    SetDesEntry(&gdt[INDEX_TSS], (u32)&tss + 0x90000, sizeof(TSS) - 1, DA_386TSS);
    __asm__ __volatile__ (
        "ltr %%ax"
        :: "a"(SELECTOR_TSS)
    );
}

/* ========================== 内核主函数 ========================== */
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
        "movl   $0xefff, %%esp\n"
        ::: "eax"
    );
    // 显示当前模式信息
    Print("[KERNEL] In Protect Mode Now\n", F_Brown | L_Light);
    // 检查系统内存
    CheckMemory(); 
    // 开启分页机制                     
    SetupPaging();
    // 初始化 8259A
    Init8259A();
    // 建立中断向量表
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

/* ========================== 中断处理函数 ========================== */
int flag = 1;

// 时钟中断处理函数
static void ClockIntHandler() {
    reEnter = -1;
    // 显示标记，与主逻辑无关
    flag = 1 - flag;
    if (flag)
        PrintAtPos("TIMER", F_Cyan | B_Cyan | L_Light, 0, 75);
    else
        PrintAtPos("TIMER", F_Brown | B_Brown | L_Light, 0, 75);
    // 调度进程
    if (process[readyPid].tick > 0)
        process[readyPid].tick--;
    else
        choose();
    restart();
}

// 时钟中断处理函数包装
asm (
    "ClockIntWarpper:\n"
    "cli\n"                     // 关中断
    "pushal\n"                  // 保存寄存器的值
    "push %ds\n"
    "push %es\n"
    "push %fs\n"
    "push %gs\n"

    "movw %ss, %dx\n"           // 修改选择子
    "movw %dx, %ds\n"
    "movw %dx, %es\n"

    "movb $0x20, %al\n"         // 响应下一个中断
	"outb %al, $0x20\n"

    "incl reEnter\n"            // 判断中断是否是重入
    "cmpl $0, reEnter\n"
    "jne  ReEnter\n"

    // 正常的中断处理
    "movl $0xefff, %esp\n"      // 切换到内核栈空间
    "call ClockIntHandler\n"    // 调用中断处理函数，函数完成中断返回
	
    // 重入中断时的处理
    "ReEnter:\n"
    "decl reEnter\n"
    "pop %gs\n"                 // 还原寄存器的值
    "pop %fs\n"
    "pop %es\n"
    "pop %ds\n"
    "popal\n"
    "sti\n"

    "iretl\n"                   // 返回
);

// 普通中断处理函数 不做任何处理 直接停机
asm (
    "SimpleIntWarpper:\n"
    "hlt\n"
);

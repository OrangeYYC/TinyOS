//  process.h         by OrangeYYC
//  TinyOS 实现进程的相关功能在本文件中实现

#include "common.h"
#include "elf.h"

/* ========================== 任务的基本信息 ========================== */
const int taskCount = 4;                                // 要加载的任务数量
u32 priority[MAX_TASKS] = { 800, 500, 250, 100 };       // 每个任务的优先级别

/* ========================== 进程初始化设置函数 ========================== */
// 装入进程的函数
static void ReadProcessToMemory(const int pid) {
    // 将进程 elf 文件从硬盘读取到缓冲区
    u32 startSector = pid * PROCESS_TOTAL_SECTOR + PROCESS_START_SECTOR;
    u32 tempAddr    = PROCESS_LOAD_BUFFER;
    for (int i = 0; i < PROCESS_TOTAL_SECTOR; i++)
        ReadDisk(startSector + i, tempAddr + i * DISK_SECTOR_SIZE);
    // 解析 elf 文件头并找到需要装入的段 (此处默认任务的第一个段为待装入段)
    Elf32_Ehdr *header  = (Elf32_Ehdr *)PROCESS_LOAD_BUFFER;
    Elf32_Phdr *pHeader = (Elf32_Phdr *)(PROCESS_LOAD_BUFFER + header->e_phoff);
    u32 size  = pHeader->p_memsz;                           // 进程大小
    u32 vaddr = pid * PROCESS_PSIZE + PROCESS_PSTART;       // 进程待装入的物理地址
    u32 faddr = pHeader->p_offset + PROCESS_LOAD_BUFFER;    // 待装入段的文件偏移
    // 复制文件到对应位置
    u8 *f = (u8 *)faddr;
    u8 *v = (u8 *)vaddr;
    for (int i = 0; i < size; i++)
        v[i] = f[i];
}

// 设置进程的页表
void SetProcessPageTable(int pid) {
    // 计算页目录和页表的位置
    u32 PDECount = RAMSize / 0x400000 + (RAMSize % 0x400000 > 0 ? 1 : 0);
    u32 *PDE     = (u32*)(PROCESS_PDE_START + pid * PROCESS_PTE_SIZE);
    u32 *PTE     = (u32*)(PROCESS_PTE_START + pid * PROCESS_PTE_SIZE);
    // 对于内核部分设置 线性地址 = 虚拟地址 的页表
    // 对于用户部分设置 线性地址 = 虚拟地址 + PROCESS_PSIZE * pid
    for (u32 i = 0; i < PDECount; i++)
        PDE[i] = ((u32)PTE + (i<<12)) | PAGE_P | PAGE_U | PAGE_W;
    for (u32 i = 0; i < PDECount * 1024; i++)
        if (i < 256)
            PTE[i] = (i<<12) | PAGE_P | PAGE_U | PAGE_W;
        else
            PTE[i] = ((i<<12) + PROCESS_PSIZE * pid) | PAGE_P | PAGE_U | PAGE_W;
    process[pid].pageDirBase = (u32)PDE;
}

// 设置进程的函数
void SetupProcess() {
    // 若请求的进程数量大于最大进程数，显示错误信息
    if (taskCount > MAX_TASKS) {
        Print("[KERNEL] Error: Too many tasks!", F_Red | L_Light);
        while (1) ;
    }

    // 初始化请求的进程
    for (int i = 0; i < taskCount; i++) {
        PCB *pcb = &process[i];
        // 设置基本信息
        pcb->pid = i;
        pcb->priority = priority[i];
        pcb->tick = priority[i];
        // 填充 GDT 表中的 LDT 描述符
        SetDesEntry(&gdt[INDEX_LDT_FIRST + i], (u32)pcb->ldts, LDT_SIZE * sizeof(Descriptor) - 1, DA_LDT);
        // 初始化局部描述符表
        pcb->ldtSelector = SELECTOR_LDT_FIRST + 0x8 * i;
        SetDesEntry(&pcb->ldts[0], 0, 0x10f, DA_C | DA_DPL3 | DA_32 | DA_LIMIT_4K);   // 用户级的平坦代码段
        SetDesEntry(&pcb->ldts[1], 0, 0x10f, DA_DRW | DA_DPL3 | DA_32 | DA_LIMIT_4K); // 用户级的平坦数据段
        // 初始化段寄存器
        pcb->regs.cs = (0x0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | SA_RPL3;
        pcb->regs.ds = (0x8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | SA_RPL3;
        pcb->regs.es = (0x8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | SA_RPL3;
        pcb->regs.fs = (0x8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | SA_RPL3;
        pcb->regs.ss = (0x8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | SA_RPL3;
        pcb->regs.gs = (SELECTOR_VIDEO & SA_RPL_MASK) | SA_RPL3;
        // 初始化任务，每个进程都从其虚拟地址 0x100000 开始执行
        pcb->regs.eip = 0x100000;
        // 初始化栈空间，每个进程的栈顶为其虚拟地址 0x10ffff 处
        pcb->regs.esp = 0x10ffff;
        // 初始化标志寄存器
        pcb->regs.eflags = 0x1202;
        // 将进程从硬盘装入内存
        ReadProcessToMemory(i);
        // 设置进程的页表
        SetProcessPageTable(i);
    }
}

/* ========================== 进程调度函数 ========================== */
// 进程选择函数
// 使用优先级调度算法选择优先级最高的进程执行，将待调度的 pid 保存在 readyRid 中
void choose() {
    // 当前运行有任务且任务 tick 不为 0 则继续执行
    if (readyPid != -1 && process[readyPid].tick > 0) {
        process[readyPid].tick -= 1;
        return;
    }
    int maxTick = 0, maxId = -1;
    // 寻找最大 tick 程序
    for (int i = 0; i < taskCount; i++) {
        if (process[i].tick > maxTick) {
            maxTick = process[i].tick;
            maxId   = i;
        }
    }
    // 若全部为 0 则重置 tick 为优先级并再次选择
    if (maxId == -1) {
        readyPid = -1;
        for (int i = 0; i < taskCount; i++)
            process[i].tick = priority[i];
        choose();
    } else
        readyPid = maxId;
}

// 重启函数
// 转入执行 readPid 进程，执行特权级转换和代码跳转
void restart() {
    // 当下次中断发生的时候，返回到的内核栈为对应 process 的 stack frame
    tss.esp0 = sizeof(StackFrame) + (u32)&process[readyPid];
    // 进行页表的切换
    __asm__ __volatile__ (
        "movl %%eax, %%cr3\n"
        ::"a"(process[readyPid].pageDirBase)
    );
    // 执行中断返回
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
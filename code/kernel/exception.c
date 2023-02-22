//  exception.h         by OrangeYYC
//  TinyOS 中断处理的相关功能在本文件中实现

#include "common.h"

/* ========================== 保护模式下的中断和异常处理函数 ========================== */
void DefaultInt();                  // 默认中断处理函数入口
void ClockInt();                    // 时钟中断处理函数入口
void DivideError();                 // 除法错误处理函数入口
void SingleStepException();         // 调试异常处理函数入口
void Nmi();                         // 非屏蔽中断处理函数入口
void BreakpointException();         // 调试端点处理函数入口
void Overflow();                    // 溢出处理函数入口
void BoundsCheck();                 // 越界处理函数入口
void InvalOpcode();                 // 无效操作码处理函数入口
void CoprNotAvailable();            // 设备不可用处理函数入口
void DoubleFault();                 // 双重错误处理函数入口
void CoprsegOverrun();              // 协处理器段越界处理函数入口
void InvalTss();                    // 无效 TSS 处理函数入口
void SegmentNotPresent();           // 段不存在处理函数入口
void StackException();              // 堆栈段错误处理函数入口
void GeneralProtection();           // 常规保护错误处理函数入口
void PageFault();                   // 页错误处理函数入口
void CoprError();                   // 浮点错误处理函数入口

// 异常处理函数
// 所有异常会经入口汇总到这里，本函数向用户显示异常信息，发生位置和相应的错误代码
static void ExceptionHandler(int id,int err,int eip,int cs,int eflags) {
    char * ErrorMessages[] = {
        "#DE Divide Error",
        "#DB RESERVED",
        "--- NMI Interrupt",
        "#BP Breakpoint",
        "#OF Overflow",
        "#BR BOUND Range Exceeded",
        "#UD Invalid Opcode (Undefined Opcode)",
        "#NM Device Not Available (No Math Coprocessor)",
        "#DF Double Fault",
        "--- Coprocessor Segment Overrun (reserved)",
        "#TS Invalid TSS",
        "#NP Segment Not Present",
        "#SS Stack-Segment Fault",
        "#GP General Protection",
        "#PF Page Fault",
        "--- (Intel reserved. Do not use.)",
        "#MF x87 FPU Floating-Point Error (Math Fault)",
        "#AC Alignment Check",
        "#MC Machine Check",
        "#XF SIMD Floating-Point Exception"
	};
    Print("Error Detected! ", F_Red | L_Light);  Print(ErrorMessages[id],  F_Red | L_Light);
    Print("\n    eip: ",      F_Red | L_Light);  PrintNumber(eip, F_White | L_Light);
    Print("\n     cs: ",      F_Red | L_Light);  PrintNumber( cs, F_White | L_Light);
    Print("\n   code: ",      F_Red | L_Light);  PrintNumber(err, F_White | L_Light);
}

// 异常处理函数的入口定义
asm (
"DivideError:\n"
	"push	$0xffffffff\n"
	"push	$0\n"
	"jmp	exception\n"
"SingleStepException:\n"
	"push	$0xffffffff\n"
	"push	$1\n"
	"jmp	exception\n"
"Nmi:"
	"push	$0xffffffff\n"
	"push	$2\n"
	"jmp	exception\n"
"BreakpointException:\n"
	"push	$0xffffffff\n"
	"push	$3\n"
	"jmp	exception\n"
"Overflow:\n"
	"push	$0xffffffff\n"
	"push	$4\n"
	"jmp	exception\n"
"BoundsCheck:"
	"push	$0xffffffff\n"
	"push	$5\n"
	"jmp	exception\n"
"InvalOpcode:"
	"push	$0xffffffff\n"
	"push	$6\n"
	"jmp	exception\n"
"CoprNotAvailable:\n"
	"push	$0xffffffff\n"
	"push	$7\n"
	"jmp	exception\n"
"DoubleFault:\n"
	"push	$8\n"
	"jmp	exception\n"
"CoprsegOverrun:\n"
	"push	$0xffffffff\n"
	"push	$9\n"
	"jmp	exception\n"
"InvalTss:\n"
	"push	$10\n"
	"jmp	exception\n"
"SegmentNotPresent:\n"
	"push	$11\n"
	"jmp	exception\n"
"StackException:\n"
	"push	$12\n"
	"jmp	exception\n"
"GeneralProtection:\n"
	"push	$13\n"
	"jmp	exception\n"
"PageFault:\n"
	"push	$14\n"
	"jmp	exception\n"
"CoprError:\n"
	"push	$0xffffffff\n"
	"push	$16\n"
	"jmp	exception\n"
"exception:\n"
	"call	ExceptionHandler\n"
	"addl	$0x8, %esp\n"
	"hlt\n"
);

/* ========================== 8259A外部中断处理函数 ========================== */
static int flag    =  1;    // 时钟中断处理函数的计数标记，奇数次为1，偶数次为0
static int reEnter = -1;    // 时钟中断处理函数的重入标记，在处理函数中为0，不在为-1

void DefaultInt();          // 除时钟中断外的其余处理函数
void ClockInt();            // 时钟中断处理函数入口

extern void restart();      // 导入重启进程的函数
extern void choose();       // 导入进程调度函数

// 时钟中断处理函数
static void ClockIntHandler() {
    // 设置重入标记
    reEnter = -1;
    // 显示时钟中断标记，与主逻辑无关,用于确认时钟正常工作
    flag = 1 - flag;
    if (flag)
        PrintAtPos("TIMER", F_Cyan | B_Cyan | L_Light, 0, 75);
    else
        PrintAtPos("TIMER", F_Brown | B_Brown | L_Light, 0, 75);
    // 调度进程
    choose();
    restart();
}

// 外部中断处理函数的入口定义
asm (
"ClockInt:\n"
    "cli\n"                     // 保存寄存器的值
    "pushal\n"
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
    "movl $0x7fff, %esp\n"      // 切换到内核栈空间
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

"DefaultInt:\n"
    "hlt\n"
);

/* ========================== 设置 8259A ========================== */
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

/* ========================== 中断设置函数 ========================== */
extern u8   idtPtr[6];             // 中断向量表指针
extern Gate idt[IDT_SIZE];         // 中断向量表格

// 设置中断向量表的函数
void SetupIdt() {
    Print("[KERNEL] Setup IDT\n", F_Cyan | L_Light);
    // 初始化 8259A 芯片
    Init8259A();

    // 初始化 idt 所有中断统一用 DefaultInt
    for (int i = 0; i < IDT_SIZE; i++) 
        SetIdtEntry(&idt[i], SELECTOR_FLAT_C, (u32)DefaultInt, 0, DA_386IGate);

    // 设置内中断的中断向量
    SetIdtEntry(&idt[INT_VECTOR_DIVIDE],       SELECTOR_FLAT_C, 
                (u32)DivideError, 0,           DA_386IGate);
    SetIdtEntry(&idt[INT_VECTOR_DEBUG],        SELECTOR_FLAT_C, 
                (u32)SingleStepException, 0,   DA_386IGate);
    SetIdtEntry(&idt[INT_VECTOR_NMI],          SELECTOR_FLAT_C, 
                (u32)Nmi, 0,                   DA_386IGate);
    SetIdtEntry(&idt[INT_VECTOR_BREAKPOINT],   SELECTOR_FLAT_C, 
                (u32)BreakpointException, 0,   DA_386IGate);
    SetIdtEntry(&idt[INT_VECTOR_OVERFLOW],     SELECTOR_FLAT_C, 
                (u32)Overflow, 0,              DA_386IGate);
    SetIdtEntry(&idt[INT_VECTOR_BOUNDS],       SELECTOR_FLAT_C, 
                (u32)BoundsCheck, 0,           DA_386IGate);
    SetIdtEntry(&idt[INT_VECTOR_INVAL_OP],     SELECTOR_FLAT_C, 
                (u32)InvalOpcode, 0,           DA_386IGate);
    SetIdtEntry(&idt[INT_VECTOR_COPROC_NOT],   SELECTOR_FLAT_C, 
                (u32)CoprNotAvailable, 0,      DA_386IGate);
    SetIdtEntry(&idt[INT_VECTOR_DOUBLE_FAULT], SELECTOR_FLAT_C, 
                (u32)DoubleFault, 0,           DA_386IGate);
    SetIdtEntry(&idt[INT_VECTOR_COPROC_SEG],   SELECTOR_FLAT_C, 
                (u32)CoprsegOverrun, 0,        DA_386IGate);
    SetIdtEntry(&idt[INT_VECTOR_INVAL_TSS],    SELECTOR_FLAT_C, 
                (u32)InvalTss, 0,              DA_386IGate);
    SetIdtEntry(&idt[INT_VECTOR_SEG_NOT],      SELECTOR_FLAT_C, 
                (u32)SegmentNotPresent, 0,     DA_386IGate);
    SetIdtEntry(&idt[INT_VECTOR_STACK_FAULT],  SELECTOR_FLAT_C, 
                (u32)StackException, 0,        DA_386IGate);
    SetIdtEntry(&idt[INT_VECTOR_PROTECTION],   SELECTOR_FLAT_C, 
                (u32)GeneralProtection, 0,     DA_386IGate);
    SetIdtEntry(&idt[INT_VECTOR_PAGE_FAULT],   SELECTOR_FLAT_C, 
                (u32)PageFault, 0,             DA_386IGate);
    SetIdtEntry(&idt[INT_VECTOR_COPROC_ERR],   SELECTOR_FLAT_C, 
                (u32)CoprError, 0,             DA_386IGate);

    // 设置时钟中断的中断向量
    SetIdtEntry(&idt[0x20],                    SELECTOR_FLAT_C, 
                (u32)ClockInt, 0,              DA_386IGate);
    
    // 加载 idt
    u16* pIdtLimit = (u16*)(&idtPtr[0]);
	u32* pIdtBase  = (u32*)(&idtPtr[2]);
	*pIdtLimit = IDT_SIZE * sizeof(Gate) - 1;
	*pIdtBase  = (u32)(&idt);
    __asm__ __volatile__ (
        "cli\n"
        "lidt %0\n"
        ::"m"(idtPtr)
    );
}
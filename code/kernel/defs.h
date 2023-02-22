//  defs.h         by OrangeYYC
//  TinyOS 使用的重要参数、常量和类型定义

#ifndef	_DEFS_H_
#define	_DEFS_H_

/* ========================== 重要参数定义 ========================== */
// 最大进程数量 受制于页表数量 可以最多容纳 8 进程
#define MAX_TASKS 		 		8
// 进程的局部描述符大小
#define LDT_SIZE 		 		2
// 全局描述符表大小
#define GDT_SIZE 		 		(6+MAX_TASKS)
// 中断向量表大小
#define IDT_SIZE 		 		256
// 内核加载的偏移地址
#define KERNEL_BASE 	 		0x8000
// 内核程序页目录基地址
#define PAGE_DIR_BASE    		0x010000
// 内核程序页表基地址
#define PAGE_TABLE_BASE  		0x011000
// 软盘中进程的开始扇区
#define PROCESS_START_SECTOR 	40
// 软盘中进程的占用扇区数目
#define PROCESS_TOTAL_SECTOR 	10
// 作为装入缓存的物理地址
#define PROCESS_LOAD_BUFFER  	0x180000
// 用户进程占用物理内存大小
#define PROCESS_PSIZE			0x10000
// 用户进程占物理内存的起始位置
#define PROCESS_PSTART			0x100000
// 用户进程页目录起始地址
#define PROCESS_PDE_START		0x20000
// 用户进程页表起始地址
#define PROCESS_PTE_START   	0x21000
// 用户进程页表大小
#define PROCESS_PTE_SIZE		0x10000
// 硬盘扇区大小
#define DISK_SECTOR_SIZE 		0x200

/* ========================== 类型定义 ========================== */
typedef unsigned int    u32;
typedef unsigned short  u16;
typedef unsigned char    u8;

/* ========================== 结构定义 ========================== */
// 描述符结构 用于全局描述符表和局部描述符表
typedef struct s_descriptor {
	u16	limitLow;
	u16	baseLow;
	u8	baseMid;
	u8	attr1;
	u8	limitHighAttr2;
	u8	baseHigh;
} Descriptor;

// 门描述符结构 用于中断向量表
typedef struct s_gate {
	u16	offsetLow;
	u16	selector;
	u8	dcount;		
	u8	attr;		
	u16	offsetHigh;	
} Gate;

// 地址描述结构 用于获取系统可用的物理内存大小
typedef struct s_ard {
	u32		BaseAddrLow;
	u32		BaseAddrHigh;
	u32		LengthLow;
	u32		LengthHigh;
	u32		Type;
} ARD;

// 进程栈帧结构 用于在进程切换的过程中保存寄存器环境
typedef struct s_stackFrame {
	u32		gs;				// 任务 gs		   <---- 使用 push gs 指令保存
	u32		fs;				// 任务 fs		   <---- 使用 push fs 指令保存
	u32		es;				// 任务 es		   <---- 使用 push es 指令保存
	u32		ds;				// 任务 ds		   <---- 使用 push ds 指令保存
	u32		edi;			// 任务 edi
	u32		esi;			// 任务 esi
	u32		ebp;			// 任务 ebp
	u32		espKernel;		// 内核在进程切换时的 esp 指向栈帧的首地址
	u32		ebx;			// 任务 ebx
	u32		edx;			// 任务 edx
	u32		ecx;			// 任务 ecx
	u32		eax;			// 任务 eax	 	   <---- 此处向上的内容由内核中断处理程序使用 pushal 指令保存寄存器
	u32		eip;			// 任务 eip
	u32		cs;				// 任务 cs
	u32		eflags;			// 任务 eflags
	u32		esp;			// 任务 esp(Ring3)
	u32		ss;				// 任务 ss(Ring3)  <---- 此处向上的内容由中断硬件机制保存		
} StackFrame;

// 进程控制块结构 用于描述进程
typedef struct s_pcb {
	StackFrame 	regs;				// 进程栈帧
	u16			ldtSelector;		// 进程的局部描述符表在全局描述符表中的选择子
	Descriptor  ldts[LDT_SIZE];		// 进程的局部描述符表
	u32			pid;				// 进程编号
	u32			tick;				// 进程的等待执行计数
	u32			priority;			// 进程优先级
	u32 		pageDirBase;		// 进程页目录的地址
} PCB;

// 任务状态段结构 用于在优先级转换的过程中重置信息
typedef struct s_tss {
	u32	backlink;
	u32	esp0;						// 任务在内核中的栈帧的高地址
	u32	ss0;						// 内核的堆栈段选择子
	u32	esp1;
	u32	ss1;
	u32	esp2;
	u32	ss2;
	u32	cr3;
	u32	eip;
	u32	flags;
	u32	eax;
	u32	ecx;
	u32	edx;
	u32	ebx;
	u32	esp;
	u32	ebp;
	u32	esi;
	u32	edi;
	u32	es;
	u32	cs;
	u32	ss;
	u32	ds;
	u32	fs;
	u32	gs;
	u32	ldt;
	u16	trap;
	u16	iobase;
} TSS;

/* ========================== 常量定义 ========================== */
// 全局描述符表中的表项序号
// 0: 空描述符 DPL0
#define	INDEX_DUMMY		    0
#define	SELECTOR_DUMMY		0
// 1: 内核级平坦代码段 DPL0
#define	INDEX_FLAT_C		1
#define	SELECTOR_FLAT_C		0x08
// 2: 内核级平坦数据段 DPL0
#define	INDEX_FLAT_RW		2
#define	SELECTOR_FLAT_RW	0x10
// 3: 用户级显存段 DPL3
#define	INDEX_VIDEO		    3
#define	SELECTOR_VIDEO		(0x18 + 3)
// 4: 内核级任务状态段 DPL0 
#define INDEX_TSS			4
#define SELECTOR_TSS		0x20
// >=5: 用户级进程的局部描述符表选择子 DPL3
#define INDEX_LDT_FIRST		5
#define SELECTOR_LDT_FIRST  (0x28 + 3)

// 全局描述符属性定义
#define	DA_32			0x4000
#define	DA_LIMIT_4K		0x8000
#define	DA_DPL0			0x00
#define	DA_DPL1			0x20
#define	DA_DPL2			0x40
#define	DA_DPL3			0x60
#define	DA_DR			0x90
#define	DA_DRW			0x92
#define	DA_DRWA			0x93
#define	DA_C			0x98
#define	DA_CR			0x9A
#define	DA_CCO			0x9C
#define	DA_CCOR			0x9E
#define	DA_LDT			0x82
#define	DA_TaskGate		0x85
#define	DA_386TSS		0x89
#define	DA_386CGate		0x8C
#define	DA_386IGate		0x8E
#define	DA_386TGate		0x8F
// 选择子属性定义
#define SA_RPL_MASK		0xfffc
#define SA_RPL0			0
#define SA_RPL1			1
#define SA_RPL2			2
#define SA_RPL3			3
#define SA_TI_MASK		0xfffb
#define SA_TIG			0
#define SA_TIL			4

// 显存输出的前景颜色和背景颜色定义
#define F_Black			0
#define F_Blue			(1 << 8)
#define F_Green			(2 << 8)
#define	F_Cyan			(3 << 8)
#define F_Red			(4 << 8)
#define F_Pink			(5 << 8)
#define	F_Brown			(6 << 8)
#define	F_White			(7 << 8)
#define B_Black			0
#define B_Blue			(1 << 12)
#define B_Green			(2 << 12)
#define	B_Cyan			(3 << 12)
#define B_Red			(4 << 12)
#define B_Pink			(5 << 12)
#define	B_Brown			(6 << 12)
#define	B_White			(7 << 12)
#define L_Light			(1 << 11)
#define L_Dark			0
#define Flicker			(1 << 15)

// 页表属性定义
#define PAGE_P           1
#define PAGE_R           0              
#define PAGE_W           2
#define PAGE_S           0
#define PAGE_U           4

// 中断控制器相关常量
#define INT_M_CTL       0x20        // 主中断控制器输入输出端口
#define INT_M_CTLMASK   0x21        // 主中断控制器掩码端口
#define INT_S_CTL       0xa0        // 从中断控制器输入输出端口
#define INT_S_CTLMASK   0xa1        // 从中断控制器掩码端口
#define INT_VECTOR_IRQ0 0x20        // 主中断处理器中断向量号
#define INT_VECTOR_IRQ8 0x28        // 从中断处理器中断向量号

// 异常定义
#define	INT_VECTOR_DIVIDE		0x0
#define	INT_VECTOR_DEBUG		0x1
#define	INT_VECTOR_NMI			0x2
#define	INT_VECTOR_BREAKPOINT	0x3
#define	INT_VECTOR_OVERFLOW		0x4
#define	INT_VECTOR_BOUNDS		0x5
#define	INT_VECTOR_INVAL_OP		0x6
#define	INT_VECTOR_COPROC_NOT	0x7
#define	INT_VECTOR_DOUBLE_FAULT	0x8
#define	INT_VECTOR_COPROC_SEG	0x9
#define	INT_VECTOR_INVAL_TSS	0xA
#define	INT_VECTOR_SEG_NOT		0xB
#define	INT_VECTOR_STACK_FAULT	0xC
#define	INT_VECTOR_PROTECTION	0xD
#define	INT_VECTOR_PAGE_FAULT	0xE
#define	INT_VECTOR_COPROC_ERR	0x10

#endif
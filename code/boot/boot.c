/* TinyOS 引导程序
内存空间: 0x200000 4M 内存空间
    0x000000 -> 0x000400  BIOS 加载的中断向量表
    0x000400 -> 0x000500  BIOS 参数的相关区域
    0x000500 -> 0x007c00  TinyOS 引导程序的栈空间
    0x007c00 -> 0x007e00  TinyOS 引导程序代码段和数据段
    0x007e00 -> 0x09efff  空闲空间
        内核将被加载到 0x090000 -> 0x09e000
    0x100000 -> 0x1FFFFF  空间空间
运行模式: 16 位实模式
段寄存器: CS = DS = ES = SS = 0
功能：加载内核进入内存
*/

/* ========================== 初始化代码段 ========================== */
asm (
    ".code16gcc\n"
    "movw   %cs, %ax\n"         // 设置 cs = ds = es = ss = 0
    "movw   %ax, %ds\n"
    "movw   %ax, %es\n"
    "movw   %ax, %ss\n"
    "movw   $0x7c00, %sp\n"     // 设置 sp = 0x7c00 向低地址处增长
    "call   BootMain\n"         // 调用引导程序主函数
);

typedef unsigned int    u32;
typedef unsigned short  u16;
typedef unsigned char    u8;
static void ReadKernel(u32 base, u32 offset, u32 id);

/* ========================== 引导程序 ========================== */
// 引导程序主功能函数
static void BootMain() {
    // 首先使用 10 号中断清除屏幕
    __asm__ __volatile__ (
        "int    $0x10\n"
        :: "a"((u16)0x600), "b"((u16)0x700), "c"((u16)0), "d"((u16)0x184f)
    );
    // 将软盘驱动器复位
    __asm__ __volatile__ (
        "int    $0x13\n"
        :: "a"((u16)0), "d"((u16)0)
    );
    // 将内核加载到 0x90000 处
    for (int i = 0; i < 0x70; i++)
        ReadKernel(0x9000, i * 0x200, i + 1);
    // 交权给操作系统内核
    __asm__ __volatile__ (
        "ljmp $0x9000, $0x0\n"
    );
}

/* ========================== 装载内核程序 ========================== */
// 读入内核函数
// base: 装载基地址，offset: 装载偏移, id: 扇区编号
static void ReadKernel(u32 base, u32 offset, u32 id) {
    // 首先计算柱面号，起始扇区号，磁头号
    u32 sector   = id % 18 + 1;
    u32 cylinder = (id / 18) >> 1;
    u32 head     = (id / 18) & 1;
    // 使用 13 号中断读磁盘
    __asm__ __volatile__ (
        "GoOnReading:\n"
        "movw   %0, %%es\n"
        "movw   $0x0201, %%ax\n"
        "int    $0x13\n"
        "jc     GoOnReading\n"
        :: "r"(base),                         // es = 存储地址
           "b"(offset),                       // bx = 存储偏移
           "c"(sector | (cylinder << 8)),     // ch = 柱面号, cl = 起始扇区号
           "d"(head << 8)                     // dh = 磁头号，dl = 驱动器号
    );
}
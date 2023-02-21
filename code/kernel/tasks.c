#include "defs.h"

/* ========================== 各个进程的基本定义 ========================== */
// 进程数量
const int taskCount = 4;                              

// 进程入口地址(进程的主函数)
void TaskA();
void TaskB();
void TaskC();
void TaskD();

// 任务列表，用于生成进程控制块 { 主函数, 堆栈基地址, 堆栈大小, 优先级别 }
Task tasks[MAX_TASKS] = {    
    { TaskA, 0x100000, 0x4000, 800 },                    // 进程 A
    { TaskB, 0x104000, 0x4000, 500 },                    // 进程 B
    { TaskC, 0x108000, 0x4000, 250 },                    // 进程 C
    { TaskD, 0x10C000, 0x4000, 100 }                     // 进程 D
};

// 引入公共函数
extern void PrintAtPos(char *message, int color, int x, int y);

/* ========================== 用户任务 A ========================== */
void TaskA() {
    while (1)
        PrintAtPos("  VERY (TASK A)  ", F_Brown | B_Brown | L_Light, 16, 30);
}
/* ========================== 用户任务 B ========================== */
void TaskB() {
    while (1)
        PrintAtPos("  LOVE (TASK B)  ", F_Red | B_Red | L_Light, 16, 30);
}
/* ========================== 用户任务 C ========================== */
void TaskC() {
    while (1)
        PrintAtPos("  HUST (TASK C)  ", F_Pink | B_Pink | L_Light, 16, 30);
}
/* ========================== 用户任务 D ========================== */
void TaskD() {
    while (1)
        PrintAtPos("  MRSU (TASK D)  ", F_Cyan | B_Cyan | L_Light, 16, 30);
}
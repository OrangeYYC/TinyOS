#ifndef _TASK_H_
#define _TASK_H_

typedef unsigned int    u32;
typedef unsigned short  u16;
typedef unsigned char    u8;

void PrintAtPos(char *message, int color, int x, int y);

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

#endif
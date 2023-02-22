# 构建所需要的工具
ASM		    = nasm
CC          = gcc
LD          = ld

# 构建所需要的参数
ASMELF      = -f elf
CCFLAG      = -std=gnu99 -O0 -c -nostdlib -m32 -fno-pie -march=i386 -ffreestanding -fno-builtin
LDFLAG      = -s -m elf_i386 --nmagic --script
BOOT_LD     = code/boot/boot.ld
KERNEL_LD   = code/kernel/kernel.ld
TASK_LD     = code/tasks/task.ld
KERNEL_OBJS = build/kernel16.o build/kernel32.o build/kernelData.o build/process.o build/common.o build/exception.o

.PHONY : everything all write writeboot writekernel start tasks

# 使用 all 构建所有程序 写入软盘 启动模拟器
all : everything tasks start
# 使用 everything 构建程序
everything : build/boot.bin build/kernel.bin
# 使用 tasks 构建测试任务
tasks : build/task1 build/task2 build/task3 build/task4
# 使用 start 启动模拟器
start: 
	bochs -f bochsrc

# TinyOS 启动扇区		启动扇区位于软盘的0号扇区
build/boot.bin : code/boot/boot.c
	$(CC) $(CCFLAG) -o build/boot.o $<
	$(LD) $(LDFLAG) $(BOOT_LD) -o $@ build/boot.o
	dd if=build/boot.bin of=bin/TinyOS.img bs=512 count=1 conv=notrunc

# TinyOS 内核			 内核位于软盘的1至32扇区
build/kernel.bin : $(KERNEL_OBJS)
	$(LD) $(LDFLAG) $(KERNEL_LD) -o $@ $(KERNEL_OBJS)
	dd if=build/kernel.bin of=bin/TinyOS.img bs=512 seek=1 count=32 conv=notrunc
build/kernelData.o : code/kernel/data.c code/kernel/defs.h
	$(CC) $(CCFLAG) -o $@ $<
build/kernel16.o : code/kernel/kernel16.c code/kernel/defs.h 
	$(CC) $(CCFLAG) -o $@ $<
build/kernel32.o : code/kernel/kernel32.c code/kernel/defs.h
	$(CC) $(CCFLAG) -o $@ $<
build/common.o : code/kernel/common.c code/kernel/defs.h
	$(CC) $(CCFLAG) -o $@ $<
build/process.o : code/kernel/process.c code/kernel/defs.h code/kernel/common.h
	$(CC) $(CCFLAG) -o $@ $<
build/exception.o : code/kernel/exception.c code/kernel/defs.h code/kernel/common.h
	$(CC) $(CCFLAG) -o $@ $<

# 4 个不同的任务
build/task1 : build/task1.o
	$(LD) $(LDFLAG) $(TASK_LD) -o $@ build/task1.o build/display.o
	dd if=build/task1 of=bin/TinyOS.img bs=512 seek=40 count=10 conv=notrunc
build/task2 : build/task2.o build/display.o
	$(LD) $(LDFLAG) $(TASK_LD) -o $@ build/task2.o build/display.o
	dd if=build/task2 of=bin/TinyOS.img bs=512 seek=50 count=10 conv=notrunc
build/task3 : build/task3.o build/display.o
	$(LD) $(LDFLAG) $(TASK_LD) -o $@ build/task3.o build/display.o
	dd if=build/task3 of=bin/TinyOS.img bs=512 seek=60 count=10 conv=notrunc
build/task4 : build/task4.o build/display.o
	$(LD) $(LDFLAG) $(TASK_LD) -o $@ build/task4.o build/display.o
	dd if=build/task4 of=bin/TinyOS.img bs=512 seek=70 count=10 conv=notrunc

build/task1.o : code/tasks/task1.c code/kernel/defs.h
	$(CC) $(CCFLAG) -o $@ $<
build/task2.o : code/tasks/task2.c code/kernel/defs.h
	$(CC) $(CCFLAG) -o $@ $<
build/task3.o : code/tasks/task3.c code/kernel/defs.h
	$(CC) $(CCFLAG) -o $@ $<
build/task4.o : code/tasks/task4.c code/kernel/defs.h
	$(CC) $(CCFLAG) -o $@ $<
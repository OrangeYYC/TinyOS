# 构建所需要的工具
ASM		    = nasm
CC          = gcc
LD          = ld

# 构建所需要的参数
ASMELF      = -f elf
CCFLAG      = -std=gnu99 -c -O0 -nostdlib -m32 -fno-pie -march=i386 -ffreestanding
LDFLAG      = -s -m elf_i386 --nmagic --script
BOOT_LD     = code/boot/boot.ld
KERNEL_LD   = code/kernel/kernel.ld
KERNEL_OBJS = build/kernel16.o build/kernel32.o build/kernelData.o build/tasks.o

.PHONY : everything all write writeboot writekernel start

# 使用 all 构建所有程序 写入软盘 启动模拟器
all : everything start
# 使用 everything 构建程序
everything : build/boot.bin build/kernel.bin
# 使用 start 启动模拟器
start: 
	bochs -f bochsrc

# TinyOS 启动扇区
build/boot.bin : code/boot/boot.c
	$(CC) $(CCFLAG) -o build/boot.o $<
	$(LD) $(LDFLAG) $(BOOT_LD) -o $@ build/boot.o
	dd if=build/boot.bin of=bin/tinyos.img bs=512 count=1 conv=notrunc

# TinyOS 内核
build/kernel.bin : $(KERNEL_OBJS)
	$(LD) $(LDFLAG) $(KERNEL_LD) -o $@ $(KERNEL_OBJS)
	dd if=build/kernel.bin of=bin/tinyos.img bs=512 seek=1 count=32 conv=notrunc
build/kernelData.o : code/kernel/data.c code/kernel/defs.h
	$(CC) $(CCFLAG) -o $@ $<
build/kernel16.o : code/kernel/kernel16.c code/kernel/defs.h 
	$(CC) $(CCFLAG) -o $@ $<
build/kernel32.o : code/kernel/kernel32.c code/kernel/defs.h
	$(CC) $(CCFLAG) -o $@ $<
build/tasks.o : code/kernel/tasks.c code/kernel/defs.h
	$(CC) $(CCFLAG) -o $@ $<
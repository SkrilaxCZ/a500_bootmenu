#
# Makefile for bootmenu
#

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy

CFLAGS := -Os -Wall -Wno-return-type -Wno-main -fno-builtin -mthumb -ffunction-sections
AFLAGS := -D__ASSEMBLY__ -fno-builtin -mthumb -fPIC -ffunction-sections
LDFLAGS := -static -nostdlib --gc-sections 
O ?= .
OBJS := $(O)/start.o $(O)/bl_0_03_14.o $(O)/bootmenu.o $(O)/fastboot.o 

all: $(O)/bootloader_v7.bin

$(O)/start.o:
	$(CC) $(AFLAGS) -c start.S -o $@

$(O)/bl_0_03_14.o:
	$(CC) $(CFLAGS) -c bl_0_03_14.c -o $@

$(O)/bootmenu.o:
	$(CC) $(CFLAGS) -c bootmenu.c -o $@

$(O)/fastboot.o:
	$(CC) $(CFLAGS) -c fastboot.c -o $@

$(O)/bootmenu.elf: $(OBJS)
	$(LD) $(LDFLAGS) -T ld-script -o $(O)/bootmenu.elf $(OBJS)

$(O)/bootmenu.bin: $(O)/bootmenu.elf
	$(OBJCOPY) -O binary $(O)/bootmenu.elf -R .note -R .comment -R .bss -S $@

$(O)/bootloader_v7.bin: $(O)/bootmenu.bin
	cp -f bootloader.bin $@
	dd if=$(O)/bootmenu.bin of=$@ bs=1 seek=577536 conv=notrunc
	
clean:
	rm -f $(OBJS)
	rm -f $(O)/bootmenu.elf
	rm -f $(O)/bootmenu.bin
	rm -f $(O)/bootloader_v7.bin

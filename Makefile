#
# Makefile for bootmenu
#

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy

CFLAGS := -Os -Wall -Wno-return-type -fno-builtin -mthumb -fPIC -ffunction-sections 
LDFLAGS := -static -nostdlib --gc-sections 
O ?= .
OBJS := $(O)/bootmenu.o $(O)/bl_0_03_14.o

all: $(O)/bootloader_v6.bin

$(O)/bootmenu.o:
	$(CC) $(CFLAGS) -c bootmenu.c -o $(O)/bootmenu.o

$(O)/bl_0_03_14.o:
	$(CC) $(CFLAGS) -c bl_0_03_14.c -o $(O)/bl_0_03_14.o
	
$(O)/bootmenu.elf: $(OBJS)
	$(LD) $(LDFLAGS) -T ld-script -o $(O)/bootmenu.elf $(OBJS)

$(O)/bootmenu.bin: $(O)/bootmenu.elf
	$(OBJCOPY) -O binary $(O)/bootmenu.elf -R .note -R .comment -R .bss -S $@

$(O)/bootloader_v6.bin: $(O)/bootmenu.bin
	cp -f bootloader.bin $@
	dd if=$(O)/bootmenu.bin of=$@ bs=1 seek=577536 conv=notrunc
	
clean:
	rm $(OBJS)
	rm $(O)/bootmenu.elf
	rm $(O)/bootmenu.bin
	rm $(O)/bootloader_v6.bin

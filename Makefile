#
# Makefile for bootmenu
#

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy

HOST_CC := gcc
HOST_CFLAGS :=

CFLAGS := -Os -Wall -Wno-return-type -Wno-main -fno-builtin -mthumb -ffunction-sections -Iinclude
AFLAGS := -D__ASSEMBLY__ -fno-builtin -mthumb -fPIC -ffunction-sections
LDFLAGS := -static -nostdlib --gc-sections 
O ?= .
OBJS := $(O)/start.o $(O)/bl_0_03_14.o $(O)/bootmenu.o $(O)/fastboot.o 

all: $(O)/bootloader_v8.bin $(O)/bootloader_v8.blob

$(O)/%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(O)/%.o : %.S
	$(CC) $(AFLAGS) -c $< -o $@

$(O)/bootmenu.elf: $(OBJS)
	$(LD) $(LDFLAGS) -T ld-script -o $(O)/bootmenu.elf $(OBJS)

$(O)/bootmenu.bin: $(O)/bootmenu.elf
	$(OBJCOPY) -O binary $(O)/bootmenu.elf -R .note -R .comment -R .bss -S $@

$(O)/bootloader_v8.bin: $(O)/bootmenu.bin
	cp -f bootloader.bin $@
	dd if=$(O)/bootmenu.bin of=$@ bs=1 seek=577536 conv=notrunc
	
$(O)/blobmaker:
	$(HOST_CC)  $(HOST_CFLAGS) blobmaker.c -o $@
	
$(O)/bootloader_v8.blob: $(O)/blobmaker $(O)/bootloader_v8.bin
	$(O)/blobmaker $(O)/bootloader_v8.bin $@
	
	
.PHONY: clean

clean:
	rm -f $(OBJS)
	rm -f $(O)/blobmaker
	rm -f $(O)/bootmenu.elf
	rm -f $(O)/bootmenu.bin
	rm -f $(O)/bootloader_v8.bin
	rm -f $(O)/bootloader_v8.blob

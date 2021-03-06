#
# Makefile for bootmenu
#

CROSS_COMPILE ?= arm-elf-
LINUX_COMPILE ?= arm-linux-gnueabihf-
ANDROID_COMPILE ?= arm-linux-androideabi-

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy

HOST_CC := gcc
HOST_CFLAGS := -O2

LINUX_CC := $(LINUX_COMPILE)gcc
LINUX_CFLAGS :=

ANDROID_CC := $(ANDROID_COMPILE)gcc
ANDROID_CFLAGS :=

LIBGCC := -L $(shell dirname `$(CC) $(CFLAGS) -print-libgcc-file-name`) -lgcc

EXTRA_CFLAGS ?= 
EXTRA_AFLAGS ?=

O ?= .

ARM_CFLAGS := -Os -Wall -Wno-return-type -Wno-main -fno-builtin -fno-stack-protector -mthumb-interwork -march=armv7-a -mtune=cortex-a9 -mfloat-abi=softfp -mfpu=vfp3 -ffunction-sections -Iinclude -I$(O) $(EXTRA_CFLAGS)
THUMB_CFLAGS := $(ARM_CFLAGS) -mthumb
AFLAGS := -D__ASSEMBLY__ -fno-builtin -march=armv7-a -mtune=cortex-a9 -mfloat-abi=softfp -mfpu=vfp3 -ffunction-sections $(EXTRA_AFLAGS)
LDFLAGS := -static $(LIBGCC) -nostdlib --gc-sections 

LIB_OBJS := $(O)/lib/_ashldi3.o $(O)/lib/_ashrdi3.o  $(O)/lib/_div0.o $(O)/lib/_divsi3.o $(O)/lib/_lshrdi3.o $(O)/lib/_modsi3.o  $(O)/lib/_udivsi3.o $(O)/lib/_umodsi3.o $(O)/lib/mystdlib.o
BL_OBJS := $(O)/bl_0_03_14.o $(O)/framebuffer.o $(O)/jpeg.o $(O)/bootmenu.go $(O)/bootimg.o $(O)/fastboot.o $(O)/ext2fs.o
ARM_OBJS := $(O)/debug.ao
OBJS := $(O)/start.o $(LIB_OBJS) $(BL_OBJS) $(ARM_OBJS)

BOOTLOADER := bootloader_v10

# Attempt to create a output directory.
$(shell [ -d ${O} ] || mkdir -p ${O})

# Verify if it was successful.
OUTPUT_DIR := $(shell cd $(O) && /bin/pwd)
$(if $(OUTPUT_DIR),,$(error output directory "$(O)" does not exist))

# Attempt to create a lib subdirectory
$(shell [ -d ${O}/lib ] || mkdir -p ${O}/lib)

# Verify if it was successful.
OUTPUT_DIR := $(shell cd $(O)/lib && /bin/pwd)
$(if $(OUTPUT_DIR),,$(error output directory "$(O)/lib" does not exist))

# Revision
GIT_SHORT_REV := $(shell git rev-parse --short HEAD)
GIT_SHORT_REV := $(GIT_SHORT_REV)$(shell git diff-index --quiet HEAD || echo -dirty)

$(shell echo "#define BOOTLOADER_GIT_REV \"g${GIT_SHORT_REV}\"" > ${O}/generated.h)

# Targets
ALL_DEPS := $(O)/$(BOOTLOADER).bin $(O)/$(BOOTLOADER).blob

ifeq ($(NO_BOOTLOADERCTL),)
	ALL_DEPS += $(O)/bootloaderctl-linux $(O)/bootloaderctl-android $(O)/bootloaderctl-android-static
endif

all: $(ALL_DEPS)

$(O)/%.o: %.c
	$(CC) $(THUMB_CFLAGS) -c $< -o $@

$(O)/%.ao: %.c
	$(CC) $(ARM_CFLAGS) -c $< -o $@

$(O)/%.go: %.c $(O)/generated.h
	$(CC) $(THUMB_CFLAGS) -c $< -o $@

$(O)/%.o: %.S
	$(CC) $(AFLAGS) -c $< -o $@

$(O)/bootmenu.elf: $(OBJS)
	$(LD) $(LDFLAGS) -T ld-script -o $@ $(OBJS)

$(O)/bootmenu.bin: $(O)/bootmenu.elf
	$(OBJCOPY) -O binary $< -R .note -R .comment --set-section-flags .bss=alloc,load,contents -S $@

$(O)/$(BOOTLOADER).bin: $(O)/bootmenu.bin
	cp -f bootloader.bin $@
	dd if=$< of=$@ bs=1 seek=577536 conv=notrunc
	dd if=font.jpg of=$@ bs=1 seek=622592 conv=notrunc
	dd if=bootlogo.jpg of=$@ bs=1 seek=643072 conv=notrunc
	dd if=/dev/zero of=$@ bs=1 seek=622336 count=256 conv=notrunc

$(O)/blobmaker: blobmaker.c
	$(HOST_CC) $(HOST_CFLAGS) $< -o $@

$(O)/$(BOOTLOADER).blob: $(O)/blobmaker $(O)/$(BOOTLOADER).bin
	$(O)/blobmaker $(O)/$(BOOTLOADER).bin $@

$(O)/bootloaderctl-linux: bootloaderctl.c
	$(LINUX_CC) $(LINUX_CFLAGS) -Iinclude $< -O2 -o $@

$(O)/bootloaderctl-android: bootloaderctl.c
	$(ANDROID_CC) $(ANDROID_CFLAGS) -Iinclude -DANDROID $< -O2 -o $@

$(O)/bootloaderctl-android-static: bootloaderctl.c
	$(ANDROID_CC) $(ANDROID_CFLAGS) -Iinclude -DANDROID -static $< -O2 -o $@

.PHONY: prep

#Clean
clean:
	rm -f $(OBJS)
	rm -f $(O)/generated.h
	rm -f $(O)/blobmaker
	rm -f $(O)/bootloaderctl-linux
	rm -f $(O)/bootloaderctl-android
	rm -f $(O)/bootloaderctl-android-static
	rm -f $(O)/bootmenu.elf
	rm -f $(O)/bootmenu.bin
	rm -f $(O)/$(BOOTLOADER).bin
	rm -f $(O)/$(BOOTLOADER).blob

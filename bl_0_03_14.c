/*
 * This file maps functions and variables that can be found in Acer 0.03.14-ICS bootloader,
 * and the patched version.
 *
 * Copyright 2012 Skrilax_CZ
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * Please note: Entry point of the application occurs when the bootloader
 * is about to check for bootmode (MSC commands).
 * 
 * Initial key state is already stored. Application is entered every time,
 * and can override boot mode.
 * 
 * This file contains only functions that are linked to the bootloader binary.
 */

#include "bl_0_03_14.h"

/* ===========================================================================
 * Thumb Mode functions
 * ===========================================================================
 */

#define NAKED __attribute__((naked))

/* This is a HACK, use linker script */
#define ASM_THUMB_B(address) __asm__("B  " #address "\n")
#define ASM_ARM_B(address)   __asm__("LDR R12, =" #address "\n BX R12")

/* 
 * Misc partition commands
 */

int  NAKED msc_cmd_factory_reset()                                { ASM_THUMB_B(0x10C214); }
int  NAKED msc_cmd_boot_fastboot()                                { ASM_THUMB_B(0x10C1B4); }
int  NAKED msc_cmd_boot_recovery()                                { ASM_THUMB_B(0x10C1E4); }
int  NAKED msc_get_debug_mode()                                   { ASM_THUMB_B(0x10BDC4); }
int  NAKED msc_get_boot_partition()                               { ASM_THUMB_B(0x1102C0); }
void NAKED msc_set_debug_mode(unsigned char debug_mode)           { ASM_THUMB_B(0x10C088); }
void NAKED msc_set_boot_partition(unsigned char boot_partition)   { ASM_THUMB_B(0x10BE14); }
void NAKED msc_set_fastboot_mode()                                { ASM_THUMB_B(0x10BFC4); }
void NAKED msc_cmd_clear()                                        { ASM_THUMB_B(0x10C138); }

/*
 * GPIO
 */

int NAKED get_gpio(int row, int column)                  { ASM_THUMB_B(0x10C2FC); }

/*
 * Display functions 
 */

void NAKED println_display(const char* fmt, ...)         { ASM_THUMB_B(0x10ECB0); }
void NAKED println_display_error(const char* fmt, ...)   { ASM_THUMB_B(0x10ED1C); }
void NAKED print_bootlogo()                              { ASM_THUMB_B(0x10CEDC); }
void NAKED clear_screen()                                { ASM_THUMB_B(0x10EDE0); }

/*
 * Miscellaneuos
 */

void NAKED format_partition(const char* partition)       { ASM_THUMB_B(0x11E534); }
int  NAKED is_wifi_only()                                { ASM_THUMB_B(0x10C5C0); }

/*
 * Booting
 */

const char* NAKED android_load_image(char** image_bytes, int* image_ep, const char* partition)          { ASM_THUMB_B(0x10C898); }
void        NAKED android_boot_image(char* image_bytes, int image_ep, int magic_boot_argument)          { ASM_THUMB_B(0x10CB40); }

/* ===========================================================================
 * ARM Mode functions
 * ===========================================================================
 */

/* 
 * Standard library:
 * 
 * You can use your own of course, but these are found in the bootloader
 */

int   NAKED strncmp(const char *str1, const char *str2, int n)                         { ASM_ARM_B(0x17B1B8); }
char* NAKED strncpy(char *destination, const char *source, int num)                    { ASM_ARM_B(0x17B0C8); }
int   NAKED strlen(const char *str)                                                    { ASM_ARM_B(0x17B0F8); }
int   NAKED snprintf(char *str, int size, const char *format, ...)                     { ASM_ARM_B(0x17C1DC); }

void* NAKED malloc(int size)                                                           { ASM_ARM_B(0x179998); }
int   NAKED memcmp(const void *ptr1, const void *ptr2, int num)                        { ASM_ARM_B(0x17B150); }
void* NAKED memcpy(void *destination, const void *source, int num)                     { ASM_ARM_B(0x17B188); }
void* NAKED memset(void *ptr, int value, int num)                                      { ASM_ARM_B(0x17B1F0); }
void  NAKED free(void *ptr)                                                            { ASM_ARM_B(0x1799F0); }

void  NAKED printf(const char *format, ...)                                            { ASM_ARM_B(0x17C30C); }

void  NAKED sleep(int ms)                                                              { ASM_ARM_B(0x179F44); }

/* ===========================================================================
 * Functions using magic argument (need to be reverse engineered properly)
 * ===========================================================================
 */
void NAKED reboot(void* magic)
{
	/* magic is on R0 */
	__asm__
	(
		"PUSH    {LR}\n"
		"LDR     R1, =0xFFFFF9F8\n"
		"LDR     R0, [R0,R1]\n"
		"LDR     R0, [R0]\n"
		"BL      0x11124C\n"
		"POP     {PC}\n"
	);
	
}

int  NAKED check_bootloader_update(void* magic)
{
	/* magic is on R0 */
	__asm__
	(
		"PUSH    {LR}\n"
		"SUB     SP, SP, #4\n"
		"LDR     R1, =0xFFFFF9F8\n"
		"LDR     R0, [R0,R1]\n"
		"LDR     R0, [R0]\n"
		"ADD     R1, SP\n" /* unused argument */
		"BL      0x110DD8\n"
		"ADD     SP, SP, #4\n"
		"POP     {PC}\n"
	);
}


int  NAKED fastboot_send(void* fb_magic_handler, const char *command, int command_length)    
{ 
	__asm__
	(
		"PUSH    {LR}\n"
		"SUB     SP, SP, #4\n"
		"MOV.W   R3, #0x3E8\n"
		"STR     R3, [SP]\n"
		"MOV     R3, #0\n"
		"LDR     R0, [R0]\n"
		"BL      0x11A734\n"
		"ADD     SP, SP, #4\n"
		"POP     {PC}\n"
	);
}


/* ===========================================================================
 * Variables
 * ===========================================================================
 */

/* Bootloader ID */
const char* bootloader_id = (void*)0x18EBD4;

/* Bootloader version */
const char* bootloader_version = (void*)0x18EBF8;

/* Loaded command from MSC partition */
volatile struct msc_command* msc_cmd = (void*)0x23ECD8;
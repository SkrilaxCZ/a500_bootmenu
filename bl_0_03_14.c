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

#ifndef __thumb__
#error Thumb mode must be used.
#endif

/* ===========================================================================
 * Thumb Mode functions
 * ===========================================================================
 */

#define NAKED __attribute__((naked))
#define ASM_THUMB_BL(address) __asm__("PUSH {LR}\n BL  " #address "\n POP {PC}\n")
#define ASM_ARM_BL(address)   __asm__("PUSH {LR}\n BLX " #address "\n POP {PC}\n")

/* 
 * Misc partition commands
 */

int  NAKED msc_cmd_factory_reset()                                { ASM_THUMB_BL(0x10C214); }
int  NAKED msc_cmd_boot_fastboot()                                { ASM_THUMB_BL(0x10C1B4); }
int  NAKED msc_cmd_boot_recovery()                                { ASM_THUMB_BL(0x10C1E4); }
int  NAKED msc_get_debug_mode()                                   { ASM_THUMB_BL(0x10BDC4); }
int  NAKED msc_get_boot_partition()                               { ASM_THUMB_BL(0x1102C0); }
void NAKED msc_set_debug_mode(unsigned char debug_mode)           { ASM_THUMB_BL(0x10C088); }
void NAKED msc_set_boot_partition(unsigned char boot_partition)   { ASM_THUMB_BL(0x10BE14); }
void NAKED msc_write_cmd_fastboot_mode()                          { ASM_THUMB_BL(0x10BFC4); }
void NAKED msc_cmd_clear()                                        { ASM_THUMB_BL(0x10C138); }

/*
 * Stored input key state at boot time:
 * 
 * Volume UP: boot to boot menu (overrides fastboot)
 * Volume DOWN: boot to recovory (handled by the BL binary - don't do anything)
 *
 */

int NAKED boot_key_volume_down_pressed()                 { ASM_THUMB_BL(0x10BDEC); }
int NAKED boot_key_volume_up_pressed()                   { ASM_THUMB_BL(0x10BE00); }

/*
 * Current volume key state
 */

int NAKED key_volume_down_pressed()                      { ASM_THUMB_BL(0x10C2FC); }
int NAKED key_volume_up_pressed()                        { ASM_THUMB_BL(0x10C34C); }

/*
 * Display functions 
 */

void NAKED println_display(const char* fmt, ...)         { ASM_THUMB_BL(0x10ECB0); }
void NAKED println_display_error(const char* fmt, ...)   { ASM_THUMB_BL(0x10ED1C); }
void NAKED print_bootlogo()                              { ASM_THUMB_BL(0x10CEDC); }
void NAKED clear_screen()                                { ASM_THUMB_BL(0x10EDE0); }

/*
 * Miscellaneuos
 */

void NAKED set_boot_normal()                             { ASM_THUMB_BL(0x110398); }
void NAKED set_boot_recovery()                           { ASM_THUMB_BL(0x110378); }
void NAKED set_boot_fastboot_mode()                      { ASM_THUMB_BL(0x110388); }
void NAKED format_partition(const char* partition)       { ASM_THUMB_BL(0x11E534); }
int  NAKED is_wifi_only()                                { ASM_THUMB_BL(0x10C5C0); }

/* ===========================================================================
 * ARM Mode functions
 * ===========================================================================
 */

/* 
 * Standard library:
 * 
 * You can use your own of course, but these are found in the bootloader
 */

int   NAKED strncmp(const char *str1, const char *str2, int n)                         { ASM_ARM_BL(0x17B1B8); }
char* NAKED strncpy(char *destination, const char *source, int num)                    { ASM_ARM_BL(0x17B0C8); }
int   NAKED strlen(const char *str)                                                    { ASM_ARM_BL(0x17B0F8); }
int   NAKED snprintf(char *str, int size, const char *format, ...)                     { ASM_ARM_BL(0x17C1DC); }

void* NAKED malloc(int size)                                                           { ASM_ARM_BL(0x179998); }
int   NAKED memcmp(const void *ptr1, const void *ptr2, int num)                        { ASM_ARM_BL(0x17B150); }
void* NAKED memcpy(void *destination, const void *source, int num)                     { ASM_ARM_BL(0x17B188); }
void* NAKED memset(void *ptr, int value, int num)                                      { ASM_ARM_BL(0x17B1F0); }

void  NAKED printf(const char *format, ...)                                            { ASM_ARM_BL(0x17C30C); }

/* ===========================================================================
 * Variables
 * ===========================================================================
 */

/* Bootloader ID */
const char* bootloader_id = (void*)0x18EBD4;

/* Bootloader version */
const char* bootloader_version = (void*)0x18EBF8;

/* Loaded command from MSC partition */
struct msc_command* msc_cmd = (void*)0x23ECD8;
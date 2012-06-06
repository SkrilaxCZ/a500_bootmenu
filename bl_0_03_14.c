/*
 * This file maps functions and variables that can be found in Acer 0.03.14-ICS bootloader,
 * and the patched version.
 *
 * Copyright (C) 2012 Skrilax_CZ
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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
void NAKED framebuffer_unknown_call()                    { ASM_THUMB_B(0x10ED84); }

/*
 * Partitions
 */

int  NAKED open_partition(const char* partition, int open_type, int* partition_handle)                     { ASM_THUMB_B(0x11E648); }
int  NAKED read_partition(int partition_handle, void* buffer, int buffer_length, int* processed_bytes)     { ASM_THUMB_B(0x11E1A4); }
int  NAKED write_partition(int partition_handle, void* buffer, int data_size, int* processed_bytes)        { ASM_THUMB_B(0x11E1E0); }
int  NAKED close_partition(int partition_handle)                                                           { ASM_THUMB_B(0x11E28C); }
int  NAKED format_partition(const char* partition)                                                         { ASM_THUMB_B(0x11E534); }

/*
 * Miscellaneuos
 */

int      NAKED is_wifi_only()                                                                              { ASM_THUMB_B(0x10C5C0); }
long int NAKED strtol(const char * str, char ** endptr, int base)                                          { ASM_THUMB_B(0x1798A8); }

/*
 * Fastboot
 */
int  NAKED fastboot_load_handle(int* fastboot_handle)                                                      { ASM_THUMB_B(0x11A840); }
void NAKED fastboot_unload_handle(int fastboot_handle)                                                     { ASM_THUMB_B(0x11A820); }

/*
 * Booting
 */

int  NAKED android_load_image(char** bootimg_data, int* bootimg_size, const char* partition)               { ASM_THUMB_B(0x10C898); }
void NAKED android_boot_image(const char* bootimg_data, int bootimg_size, int boot_handle)                 { ASM_THUMB_B(0x10CB40); }

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
int   NAKED vsnprintf(char *str, int size, const char *format, va_list ap)             { ASM_ARM_B(0x17C230); }

void* NAKED malloc(int size)                                                           { ASM_ARM_B(0x179998); }
int   NAKED memcmp(const void *ptr1, const void *ptr2, int num)                        { ASM_ARM_B(0x17B150); }
void* NAKED memcpy(void *destination, const void *source, int num)                     { ASM_ARM_B(0x17B188); }
void* NAKED memset(void *ptr, int value, int num)                                      { ASM_ARM_B(0x17B1F0); }
void  NAKED free(void *ptr)                                                            { ASM_ARM_B(0x1799F0); }

void  NAKED printf(const char *format, ...)                                            { ASM_ARM_B(0x17C30C); }

void  NAKED sleep(int ms)                                                              { ASM_ARM_B(0x179F44); }

/* ===========================================================================
 * Functions using magic argument (need to be reverse engineered more)
 * ===========================================================================
 */
void NAKED reboot(void* global_handle)
{
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

int  NAKED check_bootloader_update(void* global_handle)
{
	/* magic is on R0 */
	__asm__
	(
		"PUSH    {LR}\n"
		"SUB     SP, SP, #4\n"
		"LDR     R1, =0xFFFFF9F8\n"
		"LDR     R0, [R0,R1]\n"
		"LDR     R0, [R0]\n"
		"MOV     R1, SP\n" /* unused argument */
		"BL      0x110DD8\n"
		"ADD     SP, SP, #4\n"
		"POP     {PC}\n"
	);
}

void NAKED get_serial_no(unsigned int* serial_no)
{
	__asm__
	(
		"PUSH    {LR}\n"
		"MOV     R2, R0\n"
		"MOV     R1, #8\n"
		"LDR     R0, =0x24BB40\n"
		"BL      0x153E94\n"
		"POP     {PC}\n"
	);
}

/* 
 * Fastboot related
 */

void NAKED fastboot_init_unk0(void* global_handle)
{
	__asm__
	(
		"PUSH    {LR}\n"
		"LDR     R1, =0xFFFFFA24\n"
		"LDR     R0, [R0,R1]\n"
		"LDR     R0, [R0]\n"
		"BL      0x139C1C\n"
		"POP     {PC}\n"
	);
}

void NAKED fastboot_init_unk1()
{
	__asm__
	(
		"PUSH    {LR}\n"
		"LDR     R0, =0x23ED30\n"
		"LDR     R0, [R0]\n"
		"BL      0x15F6CC\n"
		"POP     {PC}\n"
	);
}

int  NAKED fastboot_send(int fastboot_handle, const char *command, int command_length)    
{ 
	__asm__
	(
		"PUSH    {LR}\n"
		"SUB     SP, SP, #4\n"
		"MOV.W   R3, #0x3E8\n"
		"STR     R3, [SP]\n"
		"MOV     R3, #0\n"
		"BL      0x11A734\n"
		"ADD     SP, SP, #4\n"
		"POP     {PC}\n"
	);
}

int  NAKED fastboot_recv0(int fastboot_handle, char* cmd_buffer, int buffer_length, int* cmd_length)    
{ 
	__asm__
	(
		"PUSH    {R4,LR}\n"
		"SUB     SP, SP, #4\n"
		"MOV     R4, #0\n"
		"STR     R4, [SP]\n"
		"BL      0x11A780\n"
		"ADD     SP, SP, #4\n"
		"POP     {R4,PC}\n"
	);
}

int  NAKED fastboot_recv5(int fastboot_handle, char* cmd_buffer, int buffer_length, int* cmd_length)    
{ 
	__asm__
	(
		"PUSH    {R4,LR}\n"
		"SUB     SP, SP, #8\n"
		"MOV.W   R4, #0\n"
		"STR     R4, [SP]\n"
		"MOV     R4, #0x3E8\n"
		"STR     R4, [SP, #4]\n"
		"BL      0x11A6DC\n"
		"ADD     SP, SP, #8\n"
		"POP     {R4,PC}\n"
	);
}


/* ===========================================================================
 * Variables
 * ===========================================================================
 */

/* Bootloader ID */
const char* bootloader_id = (const char*)0x18EBD4;

/* Bootloader version */
const char* bootloader_version = (const char*)0x18EBF8;

/* Framebuffer */
uint8_t** framebuffer_ptr = (uint8_t**)0x23EDA8;
uint32_t* framebuffer_size_ptr = (uint32_t*)0x23EDA4;

/* Fastboot unknown */
int* fastboot_unk_handle_var = (int*)0x23EDAC;
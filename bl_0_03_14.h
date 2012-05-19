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

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

/* ===========================================================================
 * Structures
 * ===========================================================================
 */

struct msc_command
{
	/* Boot commands:
	 * FastbootMode - fastboot
	 * FOTA - recovery 
	 */
	char boot_command[0x0C];
	
	/* Debug mode:
	 * 00 - off
	 * 01 - on
	 */
	unsigned char debug_mode;
	
	/* Primary boot partition:
	 * 00 - LNX
	 * 01 - AKB
	 */
	unsigned char boot_mode;
	
	/* Not used */
	unsigned char unused[0x06];
};

/* ===========================================================================
 * Thumb Mode functions
 * ===========================================================================
 */

/* 
 * Misc partition commands
 */

/* Wheter to do factory reset */
int msc_cmd_factory_reset();

/* Wheter to boot fastboot */
int msc_cmd_boot_fastboot();

/* Whether to boot to recovery */
int msc_cmd_boot_recovery();

/* Check debug mode */
int msc_get_debug_mode();

/* V5 bootloader: Check boot partition (00 - LNX, 01 - AKB) */
int msc_get_boot_partition();

/* Set debug mode */
void msc_set_debug_mode(unsigned char debug_mode);

/* V5 bootloader: Set boot partition (00 - LNX, 01 - AKB) */
void msc_set_boot_partition(unsigned char boot_partition);

/* Write fastboot mode reboot command */
void msc_write_cmd_fastboot_mode();

/* Remove msc command for boot mode */
void msc_cmd_clear();

/*
 * Stored input key state at boot time:
 * 
 * Lock state: ignored
 * Volume UP: boot to boot menu (overrides fastboot)
 * Volume DOWN: boot to recovory (handled by the BL binary - don't do anything)
 *
 */

/* Volume DOWN was pressed at boot time */
int boot_key_volume_down_pressed();

/* Volume UP was pressed at boot time */
int boot_key_volume_up_pressed();

/*
 * Current volume key state
 */

/* Volume DOWN key pressed */
int key_volume_down_pressed();

/* Volume UP key pressed */
int key_volume_up_pressed();

/*
 * Display functions 
 */

/* Print on the display */
void println_display(const char* fmt, ...);

/* Print error on the display (red color) */
void println_display_error(const char* fmt, ...);

/* Print bootlogo */
void print_bootlogo();

/* Clear screen */
void clear_screen();

/*
 * Miscellaneuos
 */

/* Force normal boot */
void set_boot_normal();

/* Force boot to recovery */
void set_boot_recovery();

/* Force boot to fastboot */
void set_boot_fastboot_mode();

/* Format partition - use LNX, SOS, CAC as partition ID */
void format_partition(const char* partition);

/* Check for wifi only tablet (00 - wifi only, 01 - 3G modem */
int is_wifi_only();

/* Reboot (uses magic argument )*/
void reboot(void* magic);

/* ===========================================================================
 * ARM Mode functions
 * ===========================================================================
 */

/* 
 * Standard library:
 * 
 * You can use your own of course, but these are found in the bootloader
 */

int strncmp(const char *str1, const char *str2, int n);
char* strncpy(char *destination, const char *source, int num);
int strlen(const char *str);
int snprintf(char *str, int size, const char *format, ...);

void* malloc(int size);
int memcmp(const void *ptr1, const void *ptr2, int num);
void* memcpy(void *destination, const void *source, int num);
void* memset(void *ptr, int value, int num);

void printf(const char *format, ...);

/* ===========================================================================
 * Variables
 * ===========================================================================
 */

/* Bootloader ID */
extern const char* bootloader_id;

/* Bootloader version */
extern const char* bootloader_version;

/* MSC command */
extern volatile struct msc_command* msc_cmd;

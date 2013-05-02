/*
 * Acer bootloader boot menu application main header.
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

#ifndef BOOTMENU_H
#define BOOTMENU_H

#define MSC_CMD_RECOVERY        "FOTA"
#define MSC_CMD_FCTRY_RESET     "FactoryReset"
#define MSC_CMD_FASTBOOT        "FastbootMode"
#define MSC_CMD_BOOTMENU        "BootmenuMode"

/* MSC command */
struct msc_command
{
	/* Boot command:
	 * see MSC_CMD_ defines
	 */
	char boot_command[0x0C];

	/* Debug mode:
	 * 00 - off
	 * 01 - on
	 */
	unsigned char debug_mode;

	/* Boot image:
	 * 00 - LNX
	 * 01 - AKB
	 * 02 - first item parsed from boot file
	 * 03 - second item parsed from boot file
	 */
	unsigned char boot_image;

	/* Erase cache on boot */
	unsigned char erase_cache;

	/* Path to the boot file in BL format:
	 * Example: UBN:/boot/menu.lst
	 */
	unsigned char boot_file[256];
};

/* How to boot */
enum boot_mode
{
	/* Normal */
	BM_NORMAL,

	/* Recovery */
	BM_RECOVERY,

	/* Factory Reset */
	BM_FCTRY_RESET,

	/* Fastboot */
	BM_FASTBOOT,

	/* Bootmenu */
	BM_BOOTMENU
};

/* Menu item */
struct boot_menu_item
{
	const char* title;
	int id;
};

/* Interactive boot selection item */
struct boot_selection_item
{
	char partition[10];
	char path_android[256];
	char path_zImage[256];
	char path_ramdisk[256];
	char cmdline[512];
};

/* GPIO key descriptors */
struct gpio_key
{
	int row;
	int column;
	int active_low;
};

/* Key type */
enum key_type
{
	KEY_NONE = -1,
	KEY_VOLUME_UP = 0,
	KEY_VOLUME_DOWN = 1,
	KEY_ROTATION_LOCK = 2,
	KEY_POWER = 3,
};

/*
 * Globals
 */

/* Bootloader ID */
extern const char* bootloader_id;

/* MSC command */
extern struct msc_command msc_cmd;

/* Current boot modes */
extern enum boot_mode this_boot_mode;
extern enum boot_mode msc_boot_mode;

/*
 * Key handling
 */

/* Is key active */
int get_key_active(enum key_type key);

/* Wait for key event */
enum key_type wait_for_key_event(void);

/* Wait for key event with timeout */
enum key_type wait_for_key_event_timed(int* timeout);

/* Show menu */
int show_menu(struct boot_menu_item* items, int num_items, int initial_selection, const char* message, const char* error, int timeout);

/*
 * Misc partition
 */

/* Read MSC command */
void msc_cmd_read(void);

/* Write MSC command */
void msc_cmd_write(void);

/*
 * Booting
 */

/* Load boot images */
int load_boot_images(struct boot_selection_item* boot_items, struct boot_menu_item* menu_items, int max_items);

/* Show interactive boot selection */
void boot_interactively(int initial_selection, const char* message, const char* error, int boot_handle, char* error_message, int error_message_size);

/* Set default boot image interactivey*/
void set_default_boot_image(int initial_selection);

/* Boot android image from partition */
void boot_android_image_from_partition(const char* partition, int boot_handle);

/* Boots normally (shows interactive boot screen) */
void boot_normal(struct boot_selection_item* item, const char* status, int boot_handle);

/* Boot from partition and update screen */
void boot_partition(const char* partition, const char* status, int boot_handle);

/* Boots recovery */
void boot_recovery(int boot_handle);

/* Bootmenu new frame */
void bootmenu_new_frame(void);

/* Bootmenu basic frame */
void bootmenu_basic_frame(void);

/* Error */
void bootmenu_error(void);

#endif //!BOOTMENU_H

/* 
 * Acer bootloader boot menu application main header.
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

#ifndef BOOTMENU_H
#define BOOTMENU_H

#define MSC_CMD_RECOVERY        "FOTA"
#define MSC_CMD_FCTRY_RESET     "FactoryReset"
#define MSC_CMD_FASTBOOT        "FastbootMode"
#define MSC_CMD_BOOTMENU        "BootmenuMode"

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
	
	/* Boot partition:
	 * 00 - LNX
	 * 01 - AKB
	 */
	unsigned char boot_partition;
	
	/* Not used */
	unsigned char unused[0x06];
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

/* GPIO key descriptors */
struct gpio_key
{
	int row;
	int column;
	int active_low;
};

/* */
enum key_type
{
	KEY_VOLUME_UP = 0,
	KEY_VOLUME_DOWN = 1,
	KEY_ROTATION_LOCK = 2,
	KEY_POWER = 3
};

/* Is key active */
int get_key_active(enum key_type key);

/* Get debug mode */
int get_debug_mode(void);

/* Boot android */
void boot_android(const char* partition, int boot_magic_value);

/* Bootmenu new frame */
void bootmenu_new_frame(void);

/* Bootmenu basic frame */
void bootmenu_basic_frame(void);

/* Error */
void bootmenu_error(void);

#endif //!BOOTMENU_H

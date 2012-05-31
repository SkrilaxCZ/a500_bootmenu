/* 
 * Acer bootloader boot menu application fastboot file.
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

#include "bl_0_03_14.h" 
#include "bootmenu.h"

#define FASTBOOT_VERSION "0.4"
#define FASTBOOT_SECURE  "no"
#define FASTBOOT_MID     "001"

#define FASTBOOT_CMD_RESP_OK    "OKAY"
#define FASTBOOT_CMD_RESP_INFO  "INFO"
#define FASTBOOT_CMD_RESP_FAIL  "FAIL"

/* Storage */

char serial_no_buf[0x20];

/* Command handlers */

typedef const char*(*fb_get_var_handler)(void);

struct fb_get_var_list_item
{
	const char* var_name;
	fb_get_var_handler var_handler;
};

typedef int(*fb_oem_cmd_handler)(void* fb_magic_handler);

struct fb_oem_cmd_list_item
{
	const char* cmd_name;
	fb_oem_cmd_handler cmd_handler;
};

/* ===========================================================================
 * Fastboot Get Var
 * ===========================================================================
 */

/* Bootloader version */
const char* fb_get_var_bootloader_version(void)
{
	return bootloader_version;
}

/* Baseband version */
const char* fb_get_var_baseband_version(void)
{
	return NULL;
}

/* Protocol version */
const char* fb_get_var_version(void)
{
	return FASTBOOT_VERSION;
}

/* Secure */
const char* fb_get_var_secure(void)
{
	return FASTBOOT_SECURE;
}

/* Mid */
const char* fb_get_var_mid(void)
{
	return FASTBOOT_MID;
}

/* Get serial number */
const char* fb_get_var_serialno(void)
{
	unsigned int serial_no[2];
	
	get_serial_no(serial_no);
	
	if (serial_no[0] == 0 && serial_no[1] == 0)
		return "";
	
	snprintf(serial_no_buf, ARRAY_SIZE(serial_no_buf), "%08x%08x", serial_no[1], serial_no[0]);
	
	return serial_no_buf;
}

/* Wifi only */
const char* fb_get_var_wifi_only(void)
{
	if (is_wifi_only())
		return "yes";
	else
		return "no";
}

/* Boot partition */
const char* fb_get_var_boot_partition(void)
{
	if (msc_cmd->boot_partition == 0)
		return "b1";
	else
		return "b2";
}

/* Debug mode */
const char* fb_get_var_debug_mode(void)
{
	if (msc_cmd->debug_mode == 0)
		return "OFF";
	else
		return "ON";
}

/* Product */
const char* fb_get_var_product(void)
{
	if (is_wifi_only())
		return "a500_ww_gen1";
	else
		return "a501_ww_gen1";
}

/* List of fastboot variables */
struct fb_get_var_list_item fastboot_variable_table[] = 
{
	{
		.var_name = "version-bootloader",
		.var_handler = &fb_get_var_bootloader_version,
	},
	{
		.var_name = "version-baseband",
		.var_handler = &fb_get_var_baseband_version,
	},
	{
		.var_name = "version",
		.var_handler = &fb_get_var_version,
	},
	{
		.var_name = "secure",
		.var_handler = &fb_get_var_secure,
	},
	{
		.var_name = "mid",
		.var_handler = &fb_get_var_mid,
	},
	{
		.var_name = "serialno",
		.var_handler = &fb_get_var_serialno,
	},
	{
		.var_name = "wifi-only",
		.var_handler = &fb_get_var_wifi_only,
	},
	{
		.var_name = "bootmode",
		.var_handler = &fb_get_var_boot_partition,
	},
	{
		.var_name = "debugmode",
		.var_handler = &fb_get_var_debug_mode,
	},
	{
		.var_name = "product",
		.var_handler = &fb_get_var_product,
	}
};

/* 
 * Fastboot get variable function 
 * Return NULL for unknown variable
 */
const char* fastboot_get_var(const char* cmd)
{
	int i;
	
	for (i = 0; i < ARRAY_SIZE(fastboot_variable_table); i++)
	{
		if (!strncmp(cmd, fastboot_variable_table[i].var_name, strlen(fastboot_variable_table[i].var_name)))
			return fastboot_variable_table[i].var_handler();
	}
	
	return NULL;
}

/* ===========================================================================
 * Fastboot OEM command
 * ===========================================================================
 */

/* This is taken from the BL, which reacts positively on return value 0 and 5 */
inline int fb_status_ok(int status) { return status != 0 && status != 5; }

/* Debug ON */
int fb_oem_cmd_debug_on(void* fb_magic_handler)
{
	int fb_status;
	const char* info_reply = FASTBOOT_CMD_RESP_INFO "Debug set to ON";
	
	/* FIXME: Fix MSC command handling in Acer BL */
	msc_set_debug_mode(1);
	msc_cmd->debug_mode = 1;
	
	fb_status = fastboot_send(fb_magic_handler, info_reply, strlen(info_reply));
	return fb_status_ok(fb_status);
}

/* Debug OFF */
int fb_oem_cmd_debug_off(void* fb_magic_handler)
{
	int fb_status;
	const char* info_reply = FASTBOOT_CMD_RESP_INFO "Debug set to OFF";
	
	/* FIXME: Fix MSC command handling in Acer BL */
	msc_set_debug_mode(0);
	msc_cmd->debug_mode = 0;
	
	fb_status = fastboot_send(fb_magic_handler, info_reply, strlen(info_reply));
	return fb_status_ok(fb_status);
}

/* Set primary boot partition */
int fb_oem_cmd_setboot_primary(void* fb_magic_handler)
{
	int fb_status;
	const char* info_reply = FASTBOOT_CMD_RESP_INFO "Set to boot primary kernel image.";
	
	/* FIXME: Fix MSC command handling in Acer BL */
	msc_set_boot_partition(0);
	msc_cmd->boot_partition = 0;
	
	fb_status = fastboot_send(fb_magic_handler, info_reply, strlen(info_reply));
	return fb_status_ok(fb_status);
}

/* Set secondary boot partition */
int fb_oem_cmd_setboot_secondary(void* fb_magic_handler)
{
	int fb_status;
	const char* info_reply = FASTBOOT_CMD_RESP_INFO "Set to boot secondary kernel image.";
	
	/* FIXME: Fix MSC command handling in Acer BL */
	msc_set_boot_partition(1);
	msc_cmd->boot_partition = 1;
	
	fb_status = fastboot_send(fb_magic_handler, info_reply, strlen(info_reply));
	return fb_status_ok(fb_status);
}

/* Lock */
int fb_oem_cmd_oem_lock(void* fb_magic_handler)
{
	int fb_status;
	const char* info_reply = FASTBOOT_CMD_RESP_INFO "Seriously, are you kidding me?"; /* Tsk :D */
	
	fb_status = fastboot_send(fb_magic_handler, info_reply, strlen(info_reply));
	return fb_status_ok(fb_status);
}

/* Unlock */
int fb_oem_cmd_oem_unlock(void* fb_magic_handler)
{
	int fb_status;
	const char* info_reply = FASTBOOT_CMD_RESP_INFO "Already unlocked.";
	
	fb_status = fastboot_send(fb_magic_handler, info_reply, strlen(info_reply));
	return fb_status_ok(fb_status);
}

/* List of fastboot oem commands */
struct fb_oem_cmd_list_item fastboot_oem_command_table[] = 
{
	{
		.cmd_name = "debug on",
		.cmd_handler = &fb_oem_cmd_debug_on,
	},
	{
		.cmd_name = "debug off",
		.cmd_handler = &fb_oem_cmd_debug_off,
	},
	{
		.cmd_name = "setboot b1",
		.cmd_handler = &fb_oem_cmd_setboot_primary,
	},
	{
		.cmd_name = "setboot b2",
		.cmd_handler = &fb_oem_cmd_setboot_secondary,
	},
	{
		.cmd_name = "lock",
		.cmd_handler = &fb_oem_cmd_oem_lock,
	},
	{
		.cmd_name = "unlock",
		.cmd_handler = &fb_oem_cmd_oem_unlock,
	},
};

/* 
 * Fastboot oem command function.
 * Return 00 - OK, 01 - ERROR, don't pass OKAY to PC side
 */
int fastboot_oem_command(const char* cmd, void* fb_magic_handler)
{
	int i;
	
	for (i = 0; i < ARRAY_SIZE(fastboot_oem_command_table); i++)
	{
		if (!strncmp(cmd, fastboot_oem_command_table[i].cmd_name, strlen(fastboot_oem_command_table[i].cmd_name)))
			return fastboot_oem_command_table[i].cmd_handler(fb_magic_handler);
	}
	
	return 1;
}

/* ===========================================================================
 * Fastboot continue
 * ===========================================================================
 */

void fastboot_continue(int boot_magic_value)
{
	/* Display */
	bootmenu_basic_frame();
	
	/* Normal or recovery */
	if (msc_boot_mode == BM_RECOVERY)
		boot_recovery(boot_magic_value);
	else
		boot_normal(msc_cmd->boot_partition, boot_magic_value);
	
	/* Return -> fastboot error */
}

/* ===========================================================================
 * Fastboot download
 * ===========================================================================
 */

void fastboot_download(char* image_bytes, int image_ep, int magic_boot_argument)
{
	/* Display */
	bootmenu_basic_frame();
	
	/* Write message */
	println_display("Booting downloaded kernel image");
	
	/* Boot it */
	android_boot_image(image_bytes, image_ep, magic_boot_argument);
	
	/* Return -> fastboot error */
}

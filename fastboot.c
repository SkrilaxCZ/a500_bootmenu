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
	return "";
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

/* Product */
const char* fb_get_var_product(void)
{
	return "Picasso";
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
		return "no";
	else
		return "yes";
}

/* List of fastboot variables */
struct fb_get_var_list_item fastboot_variable_table[] = {
	
	{
		.var_name = "version-bootloader",
		.var_handler = &fb_get_var_bootloader_version,
	},
	{
		.var_name = "version-baseband",
		.var_handler = &fb_get_var_bootloader_version,
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

/* Debug ON */
int fb_oem_cmd_debug_on(void* fb_magic_handler)
{
	int fb_status;
	const char* info_reply = FASTBOOT_CMD_RESP_INFO "Debug set to ON";
	
	msc_set_debug_mode(1);
	
	fb_status = fastboot_send(fb_magic_handler, info_reply, strlen(info_reply));
	return fb_status != 0 && fb_status != 5;
}

/* Debug OFF */
int fb_oem_cmd_debug_off(void* fb_magic_handler)
{
	int fb_status;
	const char* info_reply = FASTBOOT_CMD_RESP_INFO "Debug set to OFF";
	
	msc_set_debug_mode(0);
	
	fb_status = fastboot_send(fb_magic_handler, info_reply, strlen(info_reply));
	return fb_status != 0 && fb_status != 5;
}

/* Set primary boot partition */
int fb_oem_cmd_setboot_primary(void* fb_magic_handler)
{
	int fb_status;
	const char* info_reply = FASTBOOT_CMD_RESP_INFO "Set to boot primary kernel image.";
	
	msc_set_boot_partition(0);
	
	fb_status = fastboot_send(fb_magic_handler, info_reply, strlen(info_reply));
	return fb_status != 0 && fb_status != 5;
}

/* Set secondary boot partition */
int fb_oem_cmd_setboot_secondary(void* fb_magic_handler)
{
	int fb_status;
	const char* info_reply = FASTBOOT_CMD_RESP_INFO "Set to boot secondary kernel image.";
	
	msc_set_boot_partition(1);
	
	fb_status = fastboot_send(fb_magic_handler, info_reply, strlen(info_reply));
	return fb_status != 0 && fb_status != 5;
}

/* Lock */
int fb_oem_cmd_oem_lock(void* fb_magic_handler)
{
	int fb_status;
	const char* info_reply = FASTBOOT_CMD_RESP_INFO "Seriously, are you kidding me?"; /* Tsk :D */
	
	fb_status = fastboot_send(fb_magic_handler, info_reply, strlen(info_reply));
	return fb_status != 0 && fb_status != 5;
}

/* Unlock */
int fb_oem_cmd_oem_unlock(void* fb_magic_handler)
{
	int fb_status;
	const char* info_reply = FASTBOOT_CMD_RESP_INFO "Already unlocked.";
	
	fb_status = fastboot_send(fb_magic_handler, info_reply, strlen(info_reply));
	return fb_status != 0 && fb_status != 5;
}

/* List of fastboot oem commands */
struct fb_oem_cmd_list_item fastboot_oem_command_table[] = {
	
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
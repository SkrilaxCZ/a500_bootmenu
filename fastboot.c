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

/* Right now, only getvar is customizable */
typedef const char*(*fb_get_var_handler)(void);

struct fb_get_var_list_item
{
	const char* var_name;
	fb_get_var_handler var_handler;
};

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

/* Fastboot get variable function */
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
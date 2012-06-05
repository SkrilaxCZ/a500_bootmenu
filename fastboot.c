/* 
 * Acer bootloader boot menu application fastboot file.
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

#include <stddef.h>
#include <bl_0_03_14.h> 
#include <bootmenu.h>
#include <byteorder.h>

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

typedef int(*fb_oem_cmd_handler)(void* fb_handle);

struct fb_oem_cmd_list_item
{
	const char* cmd_name;
	fb_oem_cmd_handler cmd_handler;
};

/* ===========================================================================
 * Fastboot GUI
 * ===========================================================================
 */

/*
 * Clear screen & Print ID
 */
void fastboot_new_frame(void)
{
	char buffer[0x80];
	
	/* clear screen */
	clear_screen();
	
	/* print bootlogo */
	print_bootlogo();
	
	/* print id */
	snprintf(buffer, 0x80, "%s: %s\n", full_bootloader_version, "Fastboot Mode"); 
	
	println_display(buffer);
}

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
	uint32_t serial_no[2];
	
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
	if (msc_cmd.boot_partition == 0)
		return "b1";
	else
		return "b2";
}

/* Debug mode */
const char* fb_get_var_debug_mode(void)
{
	if (msc_cmd.debug_mode == 0)
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

/* SBK */
int fb_oem_cmd_sbk(void* fb_handle)
{
	int fb_status;
	uint32_t serial_no[2];
	uint32_t sbk[4];
	const char* bad_serial_reply = FASTBOOT_CMD_RESP_INFO "Invalid serial number.";
	const char* sbk_reply = FASTBOOT_CMD_RESP_INFO "SBK is displayed on the tablet. Press POWER key to continue.";
	enum key_type key;
	
	int i, j, s_dig, s_char, cnt_a, cnt_b, mult;
	
	get_serial_no(serial_no);
	
	if (serial_no[0] == 0 && serial_no[1] == 0)
	{
		fb_status = fastboot_send(fb_handle, bad_serial_reply, strlen(bad_serial_reply));
		return fb_status_ok(fb_status);
	}
	
	/* Calculate SBK */
	memset(sbk, 0, sizeof(sbk));
	
	for (i = 0; i < 2; i++)
	{
		cnt_a = 0;
		cnt_b = 0;
		mult = 1;
		
		for (j = 0; j < 4; j++)
		{
			s_dig = (serial_no[i] >> (4 * j)) & 0xF;
			
			if (s_dig >= 0xA)
				s_char = s_dig - 0xA + 'A';
			else
				s_char = s_dig + '0';
			
			cnt_b += mult * s_char;
			
			s_dig = (serial_no[i] >> (4 * (4 + j))) & 0xF;
			
			if (s_dig >= 0xA)
				s_char = s_dig - 0xA + 'A';
			else
				s_char = s_dig + '0';
			
			cnt_a += mult * s_char;
			
			mult *= 100;
		}
		
		sbk[2*i] = cnt_a;
		sbk[2*i + 1] = cnt_b;
	}
	
	for (i = 0; i < 4; i++)
		sbk[i] ^= sbk[3 - i];
	
	fastboot_new_frame();
	
	println_display("Displaying your SBK to be used with nvflash.");
	println_display("Press POWER to continue.\n");
	println_display("SBK: 0x%08x 0x%08x 0x%08x 0x%08x", ___swab32(sbk[0]), ___swab32(sbk[1]), ___swab32(sbk[2]), ___swab32(sbk[3]));

	fb_status = fastboot_send(fb_handle, sbk_reply, strlen(sbk_reply));
	
	do
	{
		key = wait_for_key_event();
	} while (key != KEY_POWER);
	
	fastboot_new_frame();
	return fb_status;
}

/* Debug ON */
int fb_oem_cmd_debug_on(void* fb_handle)
{
	int fb_status;
	const char* info_reply = FASTBOOT_CMD_RESP_INFO "Debug set to ON";
	
	msc_cmd.debug_mode = 1;
	msc_cmd_write();
	
	fb_status = fastboot_send(fb_handle, info_reply, strlen(info_reply));
	return fb_status_ok(fb_status);
}

/* Debug OFF */
int fb_oem_cmd_debug_off(void* fb_handle)
{
	int fb_status;
	const char* info_reply = FASTBOOT_CMD_RESP_INFO "Debug set to OFF";

	msc_cmd.debug_mode = 0;
	msc_cmd_write();
	
	fb_status = fastboot_send(fb_handle, info_reply, strlen(info_reply));
	return fb_status_ok(fb_status);
}

/* Set primary boot partition */
int fb_oem_cmd_setboot_primary(void* fb_handle)
{
	int fb_status;
	const char* info_reply = FASTBOOT_CMD_RESP_INFO "Set to boot primary kernel image.";
	
	msc_cmd.boot_partition = 0;
	msc_cmd_write();
	
	fb_status = fastboot_send(fb_handle, info_reply, strlen(info_reply));
	return fb_status_ok(fb_status);
}

/* Set secondary boot partition */
int fb_oem_cmd_setboot_secondary(void* fb_handle)
{
	int fb_status;
	const char* info_reply = FASTBOOT_CMD_RESP_INFO "Set to boot secondary kernel image.";
	
	msc_cmd.boot_partition = 1;
	msc_cmd_write();
	
	fb_status = fastboot_send(fb_handle, info_reply, strlen(info_reply));
	return fb_status_ok(fb_status);
}

/* Lock */
int fb_oem_cmd_oem_lock(void* fb_handle)
{
	int fb_status;
	const char* info_reply = FASTBOOT_CMD_RESP_INFO "Seriously, are you kidding me?"; /* Tsk :D */
	
	fb_status = fastboot_send(fb_handle, info_reply, strlen(info_reply));
	return fb_status_ok(fb_status);
}

/* Unlock */
int fb_oem_cmd_oem_unlock(void* fb_handle)
{
	int fb_status;
	const char* info_reply = FASTBOOT_CMD_RESP_INFO "Already unlocked.";
	
	fb_status = fastboot_send(fb_handle, info_reply, strlen(info_reply));
	return fb_status_ok(fb_status);
}

/* List of fastboot oem commands */
struct fb_oem_cmd_list_item fastboot_oem_command_table[] = 
{
	{
		.cmd_name = "sbk",
		.cmd_handler = &fb_oem_cmd_sbk,
	},
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
int fastboot_oem_command(void* fb_handle, const char* cmd)
{
	int i;
	
	for (i = 0; i < ARRAY_SIZE(fastboot_oem_command_table); i++)
	{
		if (!strncmp(cmd, fastboot_oem_command_table[i].cmd_name, strlen(fastboot_oem_command_table[i].cmd_name)))
			return fastboot_oem_command_table[i].cmd_handler(fb_handle);
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
		boot_normal(msc_cmd.boot_partition, boot_magic_value);
	
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

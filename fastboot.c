/*
 * Acer bootloader boot menu application fastboot handler
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
#include <framebuffer.h>
#include <bootmenu.h>
#include <byteorder.h>
#include <fastboot.h>
#include <ext2fs.h>
#include <stdlib.h>
#include <debug.h>

#define FASTBOOT_VERSION               "0.4"
#define FASTBOOT_SECURE                "no"
#define FASTBOOT_MID                   "001"

#define FASTBOOT_RESP_OK               "OKAY"
#define FASTBOOT_RESP_INFO             "INFO"
#define FASTBOOT_RESP_FAIL             "FAIL"

/* 700 MiB */
#define FASTBOOT_DOWNLOAD_MAX_SIZE     734003200
#define FASTBOOT_DOWNLOAD_CHUNK_SIZE   1048576

#define FASTBOOT_CMD_DOWNLOAD          "download:"
#define FASTBOOT_CMD_FLASH             "flash:"
#define FASTBOOT_CMD_ERASE             "erase:"
#define FASTBOOT_CMD_GETVAR            "getvar:"
#define FASTBOOT_CMD_OEM               "oem "
#define FASTBOOT_CMD_BOOT              "boot"
#define FASTBOOT_CMD_CONTINUE          "continue"
#define FASTBOOT_CMD_REBOOT            "reboot"
#define FASTBOOT_CMD_REBOOT_BOOTLOADER "reboot-bootloader"

#define ANDROID_VERSION_PROP_NAME      "ro.build.version.release"

/* Command handlers */

typedef void(*fastboot_get_var_handler)(char* reply_buffer, int reply_buffer_size);

struct fastboot_get_var_list_item
{
	const char* var_name;
	fastboot_get_var_handler var_handler;
};

typedef int(*fastboot_oem_cmd_handler)(int fastboot_handle, const char* args);

struct fastboot_oem_cmd_list_item
{
	const char* cmd_name;
	fastboot_oem_cmd_handler cmd_handler;
};

struct fastboot_partition_id
{
	const char* fastboot_id;
	const char* partition_id;
};

/* This is taken from the BL, which reacts positively on return value 0 and 5 */
inline int fastboot_status_ok(int status) { return status == 0 || status == 5; }
inline int fastboot_cmd_status(int status) { return status != 0 && status != 5; }

/* ===========================================================================
 * Fastboot Get Var
 * ===========================================================================
 */

/* Full Bootloader version */
void fastboot_get_var_bootloader_id(char* reply_buffer, int reply_buffer_size)
{
	strncpy(reply_buffer, bootloader_id, reply_buffer_size);
}

/* Bootloader version */
void fastboot_get_var_bootloader_version(char* reply_buffer, int reply_buffer_size)
{
	strncpy(reply_buffer, bootloader_version, reply_buffer_size);
}

/* Baseband version */
void fastboot_get_var_baseband_version(char* reply_buffer, int reply_buffer_size)
{
	reply_buffer[0] = '\0';
}

/* Android version */
void fastboot_get_var_android_version(char* reply_buffer, int reply_buffer_size)
{
	int sz, len;
	char *data, *ptr, *end;

	/* Mount system partition */
	if (ext2fs_mount("APP"))
		return;

	/* Open build.prop file */
	sz = ext2fs_open("/build.prop");
	if (sz == -1)
		goto fail;

	/* It's definitely in 1st kB */
	if (sz > 1024);
		sz = 1024;

	data = malloc(sz);
	if (!data)
		goto fail2;

	if (ext2fs_read(data, sz) != sz)
		goto fail3;

	ptr = data;
	len = strlen(ANDROID_VERSION_PROP_NAME "=");
	end = strchr(ptr, '\n');

	while(1)
	{
		if (!strncmp(ptr, ANDROID_VERSION_PROP_NAME "=", len))
		{
			ptr += len;
			len = end - ptr;
			if (len > reply_buffer_size)
				len = reply_buffer_size;

			strncpy(reply_buffer, ptr, len);
			break;
		}

		if (!end)
			break;

		ptr = end + 1;
		end = strchr(ptr, '\n');
	}

fail3:
	free(data);
fail2:
	ext2fs_close();
fail:
	ext2fs_unmount();
}

/* Protocol version */
void fastboot_get_var_version(char* reply_buffer, int reply_buffer_size)
{
	strncpy(reply_buffer, FASTBOOT_VERSION, reply_buffer_size);
}

/* Secure */
void fastboot_get_var_secure(char* reply_buffer, int reply_buffer_size)
{
	strncpy(reply_buffer, FASTBOOT_SECURE, reply_buffer_size);
}

/* Mid */
void fastboot_get_var_mid(char* reply_buffer, int reply_buffer_size)
{
	strncpy(reply_buffer, FASTBOOT_MID, reply_buffer_size);
}

/* Get serial number */
void fastboot_get_var_serialno(char* reply_buffer, int reply_buffer_size)
{
	uint32_t serial_no[2];

	get_serial_no(serial_no);

	if (serial_no[0] == 0 && serial_no[1] == 0)
		reply_buffer[0] = '\0';
	else
		snprintf(reply_buffer, reply_buffer_size, "%08x%08x", serial_no[1], serial_no[0]);
}

/* Wifi only */
void fastboot_get_var_wifi_only(char* reply_buffer, int reply_buffer_size)
{
	const char* repl;

	if (is_wifi_only())
		repl = "yes";
	else
		repl = "no";

	strncpy(reply_buffer, repl, reply_buffer_size);
}

/* Boot partition (id) */
void fastboot_get_var_boot_image_id(char* reply_buffer, int reply_buffer_size)
{
	snprintf(reply_buffer, reply_buffer_size, "%d", msc_cmd.boot_image);
}

/* Boot partition (name) */
void fastboot_get_var_boot_image_name(char* reply_buffer, int reply_buffer_size)
{
	struct boot_selection_item boot_items[20];
	struct boot_menu_item menu_items[20];
	int num_items;
	const char* repl;

	num_items = load_boot_images(boot_items, menu_items, ARRAY_SIZE(boot_items));

	if (msc_cmd.boot_image >= num_items)
		repl = menu_items[0].title;
	else
		repl = menu_items[msc_cmd.boot_image].title;

	strncpy(reply_buffer, repl, reply_buffer_size);
}

/* Debug mode */
void fastboot_get_var_debug_mode(char* reply_buffer, int reply_buffer_size)
{
	const char* repl;

	if (msc_cmd.debug_mode == 0)
		repl = "OFF";
	else
		repl = "ON";

	strncpy(reply_buffer, repl, reply_buffer_size);
}

/* Product */
void fastboot_get_var_product(char* reply_buffer, int reply_buffer_size)
{
	const char* repl;

	if (is_wifi_only())
		repl = "a500_ww_gen1";
	else
		repl = "a501_ww_gen1";

	strncpy(reply_buffer, repl, reply_buffer_size);
}

/* List of fastboot variables */
struct fastboot_get_var_list_item fastboot_variable_table[] =
{
	{
		.var_name = "id-bootloader",
		.var_handler = &fastboot_get_var_bootloader_id,
	},
	{
		.var_name = "version-bootloader",
		.var_handler = &fastboot_get_var_bootloader_version,
	},
	{
		.var_name = "version-baseband",
		.var_handler = &fastboot_get_var_baseband_version,
	},
	{
		.var_name = "version-android",
		.var_handler = &fastboot_get_var_android_version,
	},
	{
		.var_name = "version",
		.var_handler = &fastboot_get_var_version,
	},
	{
		.var_name = "secure",
		.var_handler = &fastboot_get_var_secure,
	},
	{
		.var_name = "mid",
		.var_handler = &fastboot_get_var_mid,
	},
	{
		.var_name = "serialno",
		.var_handler = &fastboot_get_var_serialno,
	},
	{
		.var_name = "wifi-only",
		.var_handler = &fastboot_get_var_wifi_only,
	},
	{
		.var_name = "boot_image_id",
		.var_handler = fastboot_get_var_boot_image_id,
	},
	{
		.var_name = "boot_image_name",
		.var_handler = fastboot_get_var_boot_image_name,
	},
	{
		.var_name = "debugmode",
		.var_handler = &fastboot_get_var_debug_mode,
	},
	{
		.var_name = "product",
		.var_handler = &fastboot_get_var_product,
	}
};

/* ===========================================================================
 * Fastboot OEM command
 * ===========================================================================
 */

/* SBK */
int fastboot_oem_cmd_sbk(int fastboot_handle, const char* args)
{
	int fastboot_status;
	uint32_t serial_no[2];
	uint32_t sbk[4];
	const char* bad_serial_reply = FASTBOOT_RESP_INFO "Invalid serial number.";
	const char* sbk_reply = FASTBOOT_RESP_INFO "SBK is displayed on the tablet. Press POWER key to continue.";
	enum key_type key;

	int i, j, s_dig, s_char, cnt_a, cnt_b, mult;

	get_serial_no(serial_no);

	if (serial_no[0] == 0 && serial_no[1] == 0)
	{
		fastboot_status = fastboot_send(fastboot_handle, bad_serial_reply, strlen(bad_serial_reply));
		return fastboot_cmd_status(fastboot_status);
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

	fb_clear();

	printf("FASTBOOT: SBK: 0x%08x 0x%08x 0x%08x 0x%08x\n", ___swab32(sbk[0]), ___swab32(sbk[1]), ___swab32(sbk[2]), ___swab32(sbk[3]));

	fb_printf("Displaying your SBK to be used with nvflash.\n");
	fb_printf("Press POWER to continue.\n\n");
	fb_printf("SBK: 0x%08x 0x%08x 0x%08x 0x%08x\n", ___swab32(sbk[0]), ___swab32(sbk[1]), ___swab32(sbk[2]), ___swab32(sbk[3]));

	fb_refresh();
	fastboot_status = fastboot_send(fastboot_handle, sbk_reply, strlen(sbk_reply));

	do
	{
		key = wait_for_key_event();
	} while (key != KEY_POWER);

	fb_clear();
	fb_refresh();

	return fastboot_cmd_status(fastboot_status);
}

/* Boot image */
int fastboot_oem_cmd_set_boot_image(int fastboot_handle, const char* args)
{
	int fastboot_status;
	const char* info_reply_ok = FASTBOOT_RESP_INFO "Boot partition set to: %d";
	const char* info_reply_bad = FASTBOOT_RESP_INFO "Invalid boot partition";
	char ok_reply_buffer[128];
	long int boot_image;

	if (args == NULL)
	{
		fastboot_status = fastboot_send(fastboot_handle, info_reply_bad, strlen(info_reply_bad));
		return fastboot_cmd_status(fastboot_status);
	}

	boot_image = strtol(args, NULL, 10);

	if (boot_image < 0 || boot_image > 0xFF)
	{
		fastboot_status = fastboot_send(fastboot_handle, info_reply_bad, strlen(info_reply_bad));
		return fastboot_cmd_status(fastboot_status);
	}

	msc_cmd.boot_image = (unsigned char)boot_image;
	msc_cmd_write();

	snprintf(ok_reply_buffer, ARRAY_SIZE(ok_reply_buffer), info_reply_ok, msc_cmd.boot_image);
	fastboot_status = fastboot_send(fastboot_handle, ok_reply_buffer, strlen(ok_reply_buffer));
	return fastboot_cmd_status(fastboot_status);
}

/* Debug ON/OFF */
int fastboot_oem_cmd_debug_mode(int fastboot_handle, const char* args)
{
	int fastboot_status;
	const char* info_reply_on = FASTBOOT_RESP_INFO "Debug set to ON";
	const char* info_reply_off = FASTBOOT_RESP_INFO "Debug set to OFF";
	const char* info_reply_bad = FASTBOOT_RESP_INFO "Invalid debug mode";
	const char* reply;

	if (!strncmp(args, "on", strlen("on")))
	{
		msc_cmd.debug_mode = 1;
		msc_cmd_write();

		reply = info_reply_on;
	}
	else if (!strncmp(args, "off", strlen("off")))
	{
		msc_cmd.debug_mode = 0;
		msc_cmd_write();

		reply = info_reply_off;
	}
	else
		reply = info_reply_bad;

	fastboot_status = fastboot_send(fastboot_handle, reply, strlen(reply));
	return fastboot_cmd_status(fastboot_status);
}

/* Lock (cough cough) */
int fastboot_oem_cmd_oem_lock(int fastboot_handle, const char* args)
{
	int fastboot_status;
	const char* info_reply = FASTBOOT_RESP_INFO "Seriously, are you kidding me?"; /* Tsk :D */

	fastboot_status = fastboot_send(fastboot_handle, info_reply, strlen(info_reply));
	return fastboot_cmd_status(fastboot_status);
}

/* Unlock */
int fastboot_oem_cmd_oem_unlock(int fastboot_handle, const char* args)
{
	int fastboot_status;
	const char* info_reply = FASTBOOT_RESP_INFO "Already unlocked.";

	fastboot_status = fastboot_send(fastboot_handle, info_reply, strlen(info_reply));
	return fastboot_cmd_status(fastboot_status);
}

/* Some debugging functions */
#ifdef BOOTLOADER_ENABLE_DEBUG

/* Dump */
int fastboot_oem_cmd_oem_bldebug_dump(int fastboot_handle, const char* args)
{
	int fastboot_status, size, rem;
	long int address, length;
	unsigned int ilength, value, i, c;
	unsigned int* paddress;
	char reply[0x100];
	char* reply_ptr;
	char* endp;
	const char* header_a = FASTBOOT_RESP_INFO "            00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f";
	const char* header_b = FASTBOOT_RESP_INFO " --------   -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --";

	if (!strncmp(args, "0x", strlen("0x")) || !strncmp(args, "0X", strlen("0X")))
		args += 2;

	address = strtol(args, &endp, 16);

	if (address < 0)
		snprintf(reply, ARRAY_SIZE(reply), FASTBOOT_RESP_INFO "Illegal address %d!", (int) address);
	else if (address % 4)
		snprintf(reply, ARRAY_SIZE(reply), FASTBOOT_RESP_INFO "Illegal address 0x%08X - unaligned access!", (int) address);
	else
	{
		while (*endp == ' ')
			endp++;

		if (*endp == '\0')
			snprintf(reply, ARRAY_SIZE(reply), FASTBOOT_RESP_INFO "Missing length!");
		else
		{
			if (!strncmp(endp, "0x", strlen("0x")) || !strncmp(endp, "0X", strlen("0X")))
				endp += 2;

			length = strtol(endp, NULL, 16);

			paddress = (unsigned int*) address;
			ilength = ((unsigned int) length);

			if (ilength % 4)
				ilength = (ilength >> 2) + 1;
			else
				ilength >>= 2;

			i = 0;
			c = 0;
			rem = 0x100;
			reply_ptr = reply;

			snprintf(reply_ptr, rem, FASTBOOT_RESP_INFO " %08X  ", (unsigned int)paddress);
			size = strlen(reply_ptr);
			rem -= size;
			reply_ptr += size;

			fastboot_status = fastboot_send(fastboot_handle, header_a, strlen(header_a));
			if (fastboot_cmd_status(fastboot_status))
				return 1;

			fastboot_status = fastboot_send(fastboot_handle, header_b, strlen(header_b));
			if (fastboot_cmd_status(fastboot_status))
				return 1;

			while (i < ilength)
			{
				/* little endian */
				value = *paddress;
				paddress++;

				snprintf(reply_ptr, rem, " %02X", value & 0xFF);
				size = strlen(reply_ptr);
				rem -= size;
				reply_ptr += size;

				snprintf(reply_ptr, rem, " %02X", (value >> 8) & 0xFF);
				size = strlen(reply_ptr);
				rem -= size;
				reply_ptr += size;

				snprintf(reply_ptr, rem, " %02X", (value >> 16) & 0xFF);
				size = strlen(reply_ptr);
				rem -= size;
				reply_ptr += size;

				snprintf(reply_ptr, rem, " %02X", (value >> 24) & 0xFF);
				size = strlen(reply_ptr);
				rem -= size;
				reply_ptr += size;

				i++;
				c += 4;

				if (c == 16)
				{
					fastboot_status = fastboot_send(fastboot_handle, reply, strlen(reply));

					if (fastboot_cmd_status(fastboot_status))
						return 1;

					c = 0;
					rem = 0x100;
					reply_ptr = reply;

					snprintf(reply_ptr, rem, FASTBOOT_RESP_INFO " %08X  ", (unsigned int)paddress);
					size = strlen(reply_ptr);
					rem -= size;
					reply_ptr += size;
				}
			}

			if (c != 0)
			{
				fastboot_status = fastboot_send(fastboot_handle, reply, strlen(reply));
				return fastboot_cmd_status(fastboot_status);
			}

			return 0;
		}
	}

	fastboot_status = fastboot_send(fastboot_handle, reply, strlen(reply));
	return fastboot_cmd_status(fastboot_status);
}

/* Get */
int fastboot_oem_cmd_oem_bldebug_get(int fastboot_handle, const char* args)
{
	int fastboot_status;
	long int address;
	unsigned int value;
	char reply[0x100];

	if (!strncmp(args, "0x", strlen("0x")) || !strncmp(args, "0X", strlen("0X")))
		args += 2;

	address = strtol(args, NULL, 16);

	if (address < 0)
		snprintf(reply, ARRAY_SIZE(reply), FASTBOOT_RESP_INFO "Illegal address %d!", (int) address);
	else if (address % 4)
		snprintf(reply, ARRAY_SIZE(reply), FASTBOOT_RESP_INFO "Illegal address 0x%08X - unaligned access!", (int) address);
	else
	{
		value = *((unsigned int*)address);
		snprintf(reply, ARRAY_SIZE(reply), FASTBOOT_RESP_INFO "0x%08X = 0x%08X", (unsigned int) address, value);
	}

	fastboot_status = fastboot_send(fastboot_handle, reply, strlen(reply));
	return fastboot_cmd_status(fastboot_status);
}

/* Set */
int fastboot_oem_cmd_oem_bldebug_set(int fastboot_handle, const char* args)
{
	int fastboot_status;
	long int address, value;
	char reply[0x100];
	char* endp;

	if (!strncmp(args, "0x", strlen("0x")) || !strncmp(args, "0X", strlen("0X")))
		args += 2;

	address = strtol(args, &endp, 16);

	if (address < 0)
		snprintf(reply, ARRAY_SIZE(reply), FASTBOOT_RESP_INFO "Illegal address %d!", (int) address);
	else if (address % 4)
		snprintf(reply, ARRAY_SIZE(reply), FASTBOOT_RESP_INFO "Illegal address 0x%08X - unaligned access!", (int) address);
	else
	{
		while (*endp == ' ')
			endp++;

		if (*endp == '\0')
			snprintf(reply, ARRAY_SIZE(reply), FASTBOOT_RESP_INFO "Missing value!");
		else
		{
			if (!strncmp(endp, "0x", strlen("0x")) || !strncmp(endp, "0X", strlen("0X")))
				endp += 2;

			value = strtol(endp, NULL, 16);

			*((unsigned int*)address) = (unsigned int) value;
			snprintf(reply, ARRAY_SIZE(reply), FASTBOOT_RESP_INFO "0x%08X = 0x%08X", (unsigned int) address, (unsigned int) value);
		}
	}

	fastboot_status = fastboot_send(fastboot_handle, reply, strlen(reply));
	return fastboot_cmd_status(fastboot_status);
}

/* dmesg */
int fastboot_oem_cmd_oem_bldebug_dmesg(int fastboot_handle, const char* args)
{
	int fastboot_status, rotated, finished, sz, rem;
	const char* print_begin = FASTBOOT_RESP_INFO "===== DMESG START =====";
	const char* print_end = FASTBOOT_RESP_INFO "===== DMESG END =====";
	char* ptr;
	char* sline;
	char* pline;
	char line[0x100];

	fastboot_status = fastboot_send(fastboot_handle, print_begin, strlen(print_begin));
	if (fastboot_cmd_status(fastboot_status))
		return 1;

	ptr = debug_start_ptr;
	rotated = 0;
	finished = 0;

	snprintf(line, ARRAY_SIZE(line), FASTBOOT_RESP_INFO);
	sline = &(line[strlen(line)]);
	sz = ARRAY_SIZE(line) - strlen(FASTBOOT_RESP_INFO) - 1;

	while (1)
	{
		pline = sline;
		rem = sz;

		while (1)
		{
			if (rem <= 0)
				break;

			if (*ptr == '\n')
			{
				ptr++;
				break;
			}

			if (*ptr == '\0')
			{
				if (!rotated && (((unsigned int)debug_start_ptr) > ((unsigned int)debug_end_ptr)) )
				{
					ptr = debug_text;
					rotated = 1;
					continue;
				}

				finished = 1;
				break;
			}

			*pline = *ptr;
			pline++;
			ptr++;
			rem--;
		}

		if (finished && pline == sline)
			break;

		*pline = '\0';

		fastboot_status = fastboot_send(fastboot_handle, line, strlen(line));
		if (fastboot_cmd_status(fastboot_status))
			return 1;

		if (finished)
			break;
	}

	fastboot_status = fastboot_send(fastboot_handle, print_end, strlen(print_end));
	return fastboot_cmd_status(fastboot_status);
}

#endif

/* List of fastboot oem commands */
struct fastboot_oem_cmd_list_item fastboot_oem_command_table[] =
{
	{
		.cmd_name = "sbk",
		.cmd_handler = &fastboot_oem_cmd_sbk,
	},
	{
		.cmd_name = "setboot",
		.cmd_handler = &fastboot_oem_cmd_set_boot_image,
	},
	{
		.cmd_name = "debug",
		.cmd_handler = &fastboot_oem_cmd_debug_mode,
	},
	{
		.cmd_name = "lock",
		.cmd_handler = &fastboot_oem_cmd_oem_lock,
	},
	{
		.cmd_name = "unlock",
		.cmd_handler = &fastboot_oem_cmd_oem_unlock,
	},
#ifdef BOOTLOADER_ENABLE_DEBUG
	{
		.cmd_name = "bldebug dump",
		.cmd_handler = &fastboot_oem_cmd_oem_bldebug_dump,
	},
	{
		.cmd_name = "bldebug get",
		.cmd_handler = &fastboot_oem_cmd_oem_bldebug_get,
	},
	{
		.cmd_name = "bldebug set",
		.cmd_handler = &fastboot_oem_cmd_oem_bldebug_set,
	},
	{
		.cmd_name = "bldebug dmesg",
		.cmd_handler = &fastboot_oem_cmd_oem_bldebug_dmesg,
	}
#endif
};

/* ===========================================================================
 * Fastboot get partition
 * ===========================================================================
 */

struct fastboot_partition_id fastboot_partitions[] =
{
	{
		.fastboot_id = "bootloader",
		.partition_id = "EBT",
	},
	{
		.fastboot_id = "boot",
		.partition_id = "LNX",
	},
	{
		.fastboot_id = "recovery",
		.partition_id = "SOS",
	},
	{
		.fastboot_id = "system",
		.partition_id = "APP",
	},
	{
		.fastboot_id = "cache",
		.partition_id = "CAC",
	},
	{
		.fastboot_id = "flex",
		.partition_id = "FLX",
	},
	{
		.fastboot_id = "linux",
		.partition_id = "UBN",
	},
	{
		.fastboot_id = "secboot",
		.partition_id = "AKB",
	},
	{
		.fastboot_id = "userdata",
		.partition_id = "UDA",
	},
};

const char* fastboot_get_partition(const char* partition)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fastboot_partitions); i++)
	{
		if (!strncmp(partition, fastboot_partitions[i].fastboot_id, strlen(fastboot_partitions[i].fastboot_id)))
			return fastboot_partitions[i].partition_id;
	}

	return NULL;
}

/* ===========================================================================
 * Fastboot main
 * ===========================================================================
 */
void fastboot_main(void* global_handle, int boot_handle, char* error_msg, int error_msg_size)
{
	int fastboot_handle, fastboot_init, fastboot_error;
	char cmd_buffer[0x100];
	char reply_buffer[0x100];
	char* cmd_pointer = cmd_buffer;
	char* reply_pointer = reply_buffer;
	const char* partition = NULL;
	char *downloaded_data, *downloaded_data_ptr;
	uint32_t download_size, download_chunk_size, download_left, received_bytes, processed_bytes;
	uint64_t partition_size;
	int fastboot_status, cmd_status;
	int pt_handle, bootloader_flash;
	int cmdlen, mycmdlen;
	int i;

	/* Unindetified initialization functions */
	fastboot_init_unk0(global_handle);
	fastboot_init_unk1();

	/* Set status */
	fb_set_status("Fastboot Mode");
	fb_clear();
	fb_refresh();

	fastboot_status = 0;
	fastboot_init = 0;
	downloaded_data = NULL;
	download_size = 0;

	/* Fastboot loop */
	while (1)
	{
		if (!fastboot_init)
		{
			*fastboot_unk_handle_var = 0;

			/* Load fastboot handle */
			if (fastboot_load_handle(&fastboot_handle))
				continue;

			fastboot_status = 0;
			fastboot_init = 1;
			printf("FASTBOOT: Initialized.\n");
		}

		/* Set error state, if it persists till the end then it's error */
		fastboot_error = 1;

		/* Receive command */
		memset(cmd_buffer, 0, ARRAY_SIZE(cmd_buffer));

		if (fastboot_status == 0)
			fastboot_status = fastboot_recv0(fastboot_handle, cmd_buffer, ARRAY_SIZE(cmd_buffer), &received_bytes);
		else /* fastboot_status == 5 */
			fastboot_status = fastboot_recv5(fastboot_handle, cmd_buffer, ARRAY_SIZE(cmd_buffer), &received_bytes);

		/* Reset framebuffer */
		fb_clear();
		fb_refresh();

		if (fastboot_status_ok(fastboot_status))
		{
			/* Parse the command */
			printf("FASTBOOT: Received command \"%s\".\n", cmd_buffer);

			if (!strncmp(cmd_buffer, FASTBOOT_CMD_DOWNLOAD, strlen(FASTBOOT_CMD_DOWNLOAD)))
			{
				/* Download data */
				cmd_pointer = cmd_buffer + strlen(FASTBOOT_CMD_DOWNLOAD);
				download_size = strtol(cmd_pointer, NULL, 16);

				/*
				 * Allright, we have 1 GB RAM, that should provide enough space to download even the biggest partitions
				 * Allowed maximum size 700 MiB should be enough.
				 */
				if (download_size > FASTBOOT_DOWNLOAD_MAX_SIZE)
				{
					fb_printf("Downloads over 700 MiB are not supported.\n\n");
					printf("FASTBOOT: Failed downloading %d MiB\n.", download_size / (1024 * 1024));
					fastboot_status = 6;
					goto error;
				}

				printf("FASTBOOT: Downloading %d MiB\n.", download_size / (1024 * 1024));
				downloaded_data = malloc(download_size);
				if (downloaded_data == NULL)
				{
					fb_printf("Failed allocating data array.\n\n");
					fastboot_status = 6;
					goto error;
				}

				/* Acknowledge it */
				snprintf(reply_buffer, ARRAY_SIZE(reply_buffer), "DATA%08X", download_size);

				fastboot_status = fastboot_send(fastboot_handle, reply_buffer, strlen(reply_buffer));
				if (!fastboot_status_ok(fastboot_status))
					goto error;

				/* Download it */
				download_left = download_size;
				downloaded_data_ptr = downloaded_data;

				fb_printf("Downloading data...\n\n", partition);
				fb_refresh();

				while (download_left > 0)
				{
					if (download_left > FASTBOOT_DOWNLOAD_CHUNK_SIZE)
						download_chunk_size = FASTBOOT_DOWNLOAD_CHUNK_SIZE;
					else
						download_chunk_size = download_left;

					fastboot_status = fastboot_recv5(fastboot_handle, downloaded_data_ptr, download_chunk_size, &received_bytes);
					if (!fastboot_status_ok(fastboot_status))
					{
						free(downloaded_data);
						download_size = 0;
						goto error;
					}

					if (received_bytes != download_chunk_size)
					{
						fastboot_status = 0x12003;
						free(downloaded_data);
						download_size = 0;
						goto error;
					}

					downloaded_data_ptr += download_chunk_size;
					download_left -= download_chunk_size;
				}

				/* Downloading finished */
				fastboot_status = fastboot_send(fastboot_handle, FASTBOOT_RESP_OK, strlen(FASTBOOT_RESP_OK));
				if (fastboot_status_ok(fastboot_status))
					fastboot_error = 0;
				else
					free(downloaded_data);

			}
			else if (!strncmp(cmd_buffer, FASTBOOT_CMD_FLASH, strlen(FASTBOOT_CMD_FLASH)))
			{
				/* Flash parititon with downloaded data */
				cmd_pointer = cmd_buffer + strlen(FASTBOOT_CMD_FLASH);

				if (downloaded_data == NULL || download_size == 0)
				{
					/* Don't refresh here, let it do the error report */
					fb_printf("No downloaded data to be flashed.\n\n");
					fastboot_status = 0x30003;
					goto error;
				}

				partition = fastboot_get_partition(cmd_pointer);

				if (partition == NULL)
				{
					free(downloaded_data);
					fastboot_status = 0x30003;
					goto error;
				}

				if (!strncmp(partition, "EBT", strlen("EBT")))
				{
					partition = "CAC";
					bootloader_flash = 1;
				}
				else
					bootloader_flash = 0;

				/* Check size */
				if (get_partition_size(partition, &partition_size))
				{
					fb_printf("Partition %s not found.\n", partition);
					free(downloaded_data);
					fastboot_status = 0x30003;
					goto error;
				}
				else if (partition_size < download_size)
				{
					fb_printf("Not enough space in %s partition.\n", partition);
					free(downloaded_data);
					fastboot_status = 0x30003;
					goto error;
				}

				if (!bootloader_flash)
				{
					fb_printf("Flashing %s partition...\n\n", partition);
					fb_refresh();
				}

				fastboot_status = open_partition(partition, PARTITION_OPEN_WRITE, &pt_handle);

				if (fastboot_status != 0)
				{
					free(downloaded_data);
					download_size = 0;
					goto error;
				}

				/* Flash the downloaded data */
				download_left = download_size;
				downloaded_data_ptr = downloaded_data;

				while (download_left > 0)
				{
					if (download_left > FASTBOOT_DOWNLOAD_CHUNK_SIZE)
						download_chunk_size = FASTBOOT_DOWNLOAD_CHUNK_SIZE;
					else
						download_chunk_size = download_left;

					write_partition(pt_handle, downloaded_data_ptr, download_chunk_size, &processed_bytes);

					if (processed_bytes != download_chunk_size)
					{
						free(downloaded_data);
						download_size = 0;
						close_partition(pt_handle);
						goto error;
					}

					downloaded_data_ptr += download_chunk_size;
					download_left -= download_chunk_size;
				}

				close_partition(pt_handle);

				fastboot_status = fastboot_send(fastboot_handle, FASTBOOT_RESP_OK, strlen(FASTBOOT_RESP_OK));

				if (fastboot_status_ok(fastboot_status))
				{
					if (bootloader_flash)
					{
						/* Set MSC to reboot to bootloader and to erase cache afterwards */
						memcpy(msc_cmd.boot_command, MSC_CMD_FASTBOOT, strlen(MSC_CMD_FASTBOOT));
						msc_cmd.erase_cache = 1;
						msc_cmd_write();

						/* Flash the update */
						check_bootloader_update(global_handle);

						/* We returned => bad flash, format CAC */
						format_partition("CAC");

						/* Error */
						fb_printf("Bad bootloader.blob file.\n");
						fb_refresh();

						/* Unset MSC to reboot to bootloader */
						memset(msc_cmd.boot_command, 0, ARRAY_SIZE(msc_cmd.boot_command));
						msc_cmd_write();

						fastboot_status = 0;
						fastboot_error = 0;
					}
					else
					{
						fb_printf("Done.\n");
						fb_refresh();
						fastboot_status = 0;
						fastboot_error = 0;
					}
				}

				/* Clean up downloaded data */
				free(downloaded_data);
				download_size = 0;
			}
			else if (!strncmp(cmd_buffer, FASTBOOT_CMD_BOOT, strlen(FASTBOOT_CMD_BOOT)))
			{
				/* Boot downloaded data */
				if (downloaded_data == NULL || download_size == 0)
				{
					/* Don't refresh here, let it do the error report */
					fb_printf("No downloaded data to be booted.\n\n");
					fastboot_status = 0x30003;
					goto error;
				}

				/* Send ok */
				fastboot_status = fastboot_send(fastboot_handle, FASTBOOT_RESP_OK, strlen(FASTBOOT_RESP_OK));

				if (fastboot_status_ok(fastboot_status))
				{
					/* Exit fastboot */
					fastboot_unload_handle(fastboot_handle);
					fastboot_handle = 0;

					/* Send reboot */
					fb_set_status("Booting downloaded kernel image");
					fb_refresh();

					printf("FASTBOOT: Booting downloaded kernel image\n");
					android_boot_image(downloaded_data, download_size, boot_handle);

					/* It returned */
					bootmenu_error();
				}

			}
			else if (!strncmp(cmd_buffer, FASTBOOT_CMD_ERASE, strlen(FASTBOOT_CMD_ERASE)))
			{
				/* Erase parititon */
				cmd_pointer = cmd_buffer + strlen(FASTBOOT_CMD_ERASE);

				partition = fastboot_get_partition(cmd_pointer);

				if (!strncmp(partition, "EBT", strlen("EBT")))
				{
					/* Don't refresh here, let it do the error report */
					fb_printf("Erasing bootloader is not supported.\n\n");
					fastboot_status = 0x30003;
					goto error;
				}

				if (partition == NULL)
				{
					fastboot_status = 0x30003;
					goto error;
				}

				fb_printf("Erasing %s partition...\n\n", partition);
				fb_refresh();

				fastboot_status = format_partition(partition);

				if (fastboot_status == 0)
				{
					fastboot_status = fastboot_send(fastboot_handle, FASTBOOT_RESP_OK, strlen(FASTBOOT_RESP_OK));

					if (fastboot_status_ok(fastboot_status))
					{
						fb_printf("Done.\n");
						fb_refresh();
						fastboot_status = 0;
						fastboot_error = 0;
					}
				}

			}
			else if (!strncmp(cmd_buffer, FASTBOOT_CMD_GETVAR, strlen(FASTBOOT_CMD_GETVAR)))
			{
				/* Get variable */
				cmd_pointer = cmd_buffer + strlen(FASTBOOT_CMD_GETVAR);

				/* Prepare default reply */
				strncpy(reply_buffer, FASTBOOT_RESP_OK, strlen(FASTBOOT_RESP_OK));
				reply_pointer = reply_buffer + strlen(FASTBOOT_RESP_OK);

				/* Append reply */
				for (i = 0; i < ARRAY_SIZE(fastboot_variable_table); i++)
				{
					if (!strncmp(cmd_pointer, fastboot_variable_table[i].var_name, strlen(fastboot_variable_table[i].var_name)))
					{
						fastboot_variable_table[i].var_handler(reply_pointer, ARRAY_SIZE(reply_buffer) - strlen(FASTBOOT_RESP_OK));
						break;
					}
				}

				fastboot_status = fastboot_send(fastboot_handle, reply_buffer, strlen(reply_buffer));

				/* Continue if ok, error otherwise */
				if (fastboot_status_ok(fastboot_status))
				{
					fastboot_status = 0;
					fastboot_error = 0;
				}
			}
			else if (!strncmp(cmd_buffer, FASTBOOT_CMD_OEM, strlen(FASTBOOT_CMD_OEM)))
			{
				/* OEM command - load the command handle */
				cmd_pointer = cmd_buffer + strlen(FASTBOOT_CMD_OEM);
				cmd_status = 1;

				mycmdlen = strlen(cmd_pointer);

				for (i = 0; i < ARRAY_SIZE(fastboot_oem_command_table); i++)
				{
					cmdlen = strlen(fastboot_oem_command_table[i].cmd_name);

					if (!strncmp(cmd_pointer, fastboot_oem_command_table[i].cmd_name, cmdlen))
					{
						/* Make sure it's a match */
						if (mycmdlen == cmdlen)
						{
							cmd_status = fastboot_oem_command_table[i].cmd_handler(fastboot_handle, "");
							break;
						}
						else if (mycmdlen > cmdlen && cmd_pointer[cmdlen] == ' ')
						{
							cmd_status = fastboot_oem_command_table[i].cmd_handler(fastboot_handle, cmd_pointer + cmdlen + 1);
							break;
						}
					}
				}

				/* Continue if ok, error otherwise */
				if (!cmd_status)
				{
					fastboot_status = fastboot_send(fastboot_handle, FASTBOOT_RESP_OK, strlen(FASTBOOT_RESP_OK));

					if (fastboot_status_ok(fastboot_status))
					{
						fastboot_status = 0;
						fastboot_error = 0;
					}
				}
				else
					fastboot_status = 2;
			}
			else if (!strncmp(cmd_buffer, FASTBOOT_CMD_REBOOT, strlen(FASTBOOT_CMD_REBOOT)))
			{
				if (!strncmp(cmd_buffer, FASTBOOT_CMD_REBOOT_BOOTLOADER, strlen(FASTBOOT_CMD_REBOOT_BOOTLOADER)))
				{
					memcpy(msc_cmd.boot_command, MSC_CMD_FASTBOOT, strlen(MSC_CMD_FASTBOOT));
					msc_cmd_write();
				}

				fastboot_status = fastboot_send(fastboot_handle, FASTBOOT_RESP_OK, strlen(FASTBOOT_RESP_OK));

				if (fastboot_status_ok(fastboot_status))
				{
					/* Exit fastboot */
					fastboot_unload_handle(fastboot_handle);
					fastboot_handle = 0;

					/* Send reboot */
					reboot(global_handle);

					/* Reboot returned */
					bootmenu_error();
				}
			}
			else if (!strncmp(cmd_buffer, FASTBOOT_CMD_CONTINUE, strlen(FASTBOOT_CMD_CONTINUE)))
			{
				fastboot_status = fastboot_send(fastboot_handle, FASTBOOT_RESP_OK, strlen(FASTBOOT_RESP_OK));

				if (fastboot_status_ok(fastboot_status))
				{
					/* Exit fastboot */
					fastboot_unload_handle(fastboot_handle);
					fastboot_handle = 0;

					/* Boot */
					if (msc_boot_mode == BM_RECOVERY)
						boot_recovery(boot_handle);
					else
						boot_interactively(msc_cmd.boot_image, NULL, NULL, boot_handle, NULL, 0);

					/* Booting returned - back to bootmenu */
					strncpy(error_msg, "Booting kernel image from fastboot failed.", error_msg_size);
					return;
				}
			}
			else
				fastboot_status = 0x30001;
		}

		/* Error cleared, go for another command */
		if (!fastboot_error)
		{
			printf("FASTBOOT: Command processed successfully\n");
			continue;
		}

error:
		/* If we got here, we get command error in fastboot_status */
		printf("FASTBOOT: Error processing command! (%08x)\n", fastboot_status);

		snprintf(reply_buffer, ARRAY_SIZE(reply_buffer), FASTBOOT_RESP_FAIL "(%08x)", fastboot_status);
		fastboot_send(fastboot_handle, reply_buffer, strlen(reply_buffer));

		fb_color_printf("ERROR: Failed to process command %s.\nError(0x%x)\n", NULL, &error_text_color, cmd_buffer, fastboot_status);
		fb_refresh();

		/* Unload fastboot handle */
		fastboot_unload_handle(fastboot_handle);
		fastboot_init = 0;
		printf("FASTBOOT: Uninitialized.\n");
	}
}

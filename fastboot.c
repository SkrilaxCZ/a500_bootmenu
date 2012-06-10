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

/* Command handlers */

typedef void(*fastboot_get_var_handler)(char* reply_buffer, int reply_buffer_size);

struct fastboot_get_var_list_item
{
	const char* var_name;
	fastboot_get_var_handler var_handler;
};

typedef int(*fastboot_oem_cmd_handler)(int fastboot_handle);

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

/* Boot partition */
void fastboot_get_var_boot_partition(char* reply_buffer, int reply_buffer_size)
{
	const char* repl;
	
	if (msc_cmd.boot_partition == 0)
		repl = "b1";
	else
		repl = "b2";
	
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
		.var_name = "bootmode",
		.var_handler = &fastboot_get_var_boot_partition,
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
int fastboot_oem_cmd_sbk(int fastboot_handle)
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
		return fastboot_status_ok(fastboot_status) == 0;
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
	
	return fastboot_status_ok(fastboot_status) == 0;
}

/* Debug ON */
int fastboot_oem_cmd_debug_on(int fastboot_handle)
{
	int fastboot_status;
	const char* info_reply = FASTBOOT_RESP_INFO "Debug set to ON";
	
	msc_cmd.debug_mode = 1;
	msc_cmd_write();
	
	fastboot_status = fastboot_send(fastboot_handle, info_reply, strlen(info_reply));
	return fastboot_status_ok(fastboot_status) == 0;
}

/* Debug OFF */
int fastboot_oem_cmd_debug_off(int fastboot_handle)
{
	int fastboot_status;
	const char* info_reply = FASTBOOT_RESP_INFO "Debug set to OFF";

	msc_cmd.debug_mode = 0;
	msc_cmd_write();
	
	fastboot_status = fastboot_send(fastboot_handle, info_reply, strlen(info_reply));
	return fastboot_status_ok(fastboot_status) == 0;
}

/* Set primary boot partition */
int fastboot_oem_cmd_setboot_primary(int fastboot_handle)
{
	int fastboot_status;
	const char* info_reply = FASTBOOT_RESP_INFO "Set to boot primary kernel image.";
	
	msc_cmd.boot_partition = 0;
	msc_cmd_write();
	
	fastboot_status = fastboot_send(fastboot_handle, info_reply, strlen(info_reply));
	return fastboot_status_ok(fastboot_status) == 0;
}

/* Set secondary boot partition */
int fastboot_oem_cmd_setboot_secondary(int fastboot_handle)
{
	int fastboot_status;
	const char* info_reply = FASTBOOT_RESP_INFO "Set to boot secondary kernel image.";
	
	msc_cmd.boot_partition = 1;
	msc_cmd_write();
	
	fastboot_status = fastboot_send(fastboot_handle, info_reply, strlen(info_reply));
	return fastboot_status_ok(fastboot_status) == 0;
}

/* Lock */
int fastboot_oem_cmd_oem_lock(int fastboot_handle)
{
	int fastboot_status;
	const char* info_reply = FASTBOOT_RESP_INFO "Seriously, are you kidding me?"; /* Tsk :D */
	
	fastboot_status = fastboot_send(fastboot_handle, info_reply, strlen(info_reply));
	return fastboot_status_ok(fastboot_status) == 0;
}

/* Unlock */
int fastboot_oem_cmd_oem_unlock(int fastboot_handle)
{
	int fastboot_status;
	const char* info_reply = FASTBOOT_RESP_INFO "Already unlocked.";
	
	fastboot_status = fastboot_send(fastboot_handle, info_reply, strlen(info_reply));
	return fastboot_status_ok(fastboot_status) == 0;
}

/* List of fastboot oem commands */
struct fastboot_oem_cmd_list_item fastboot_oem_command_table[] = 
{
	{
		.cmd_name = "sbk",
		.cmd_handler = &fastboot_oem_cmd_sbk,
	},
	{
		.cmd_name = "debug on",
		.cmd_handler = &fastboot_oem_cmd_debug_on,
	},
	{
		.cmd_name = "debug off",
		.cmd_handler = &fastboot_oem_cmd_debug_off,
	},
	{
		.cmd_name = "setboot b1",
		.cmd_handler = &fastboot_oem_cmd_setboot_primary,
	},
	{
		.cmd_name = "setboot b2",
		.cmd_handler = &fastboot_oem_cmd_setboot_secondary,
	},
	{
		.cmd_name = "lock",
		.cmd_handler = &fastboot_oem_cmd_oem_lock,
	},
	{
		.cmd_name = "unlock",
		.cmd_handler = &fastboot_oem_cmd_oem_unlock,
	},
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
					fastboot_status = 6;
					goto error;
				}
				
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
						/* Set MSC to reboot to bootloader */
						memcpy(msc_cmd.boot_command, MSC_CMD_FASTBOOT, strlen(MSC_CMD_FASTBOOT));
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
						fb_printf("Done.");
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
						fb_printf("Done.");
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
				
				for (i = 0; i < ARRAY_SIZE(fastboot_oem_command_table); i++)
				{
					if (!strncmp(cmd_pointer, fastboot_oem_command_table[i].cmd_name, strlen(fastboot_oem_command_table[i].cmd_name)))
					{
						cmd_status = fastboot_oem_command_table[i].cmd_handler(fastboot_handle);
						break;
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
						boot_normal(msc_cmd.boot_partition, boot_handle);
					
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
			continue;
		
error:
		/* If we got here, we get command error in fastboot_status */
		snprintf(reply_buffer, ARRAY_SIZE(reply_buffer), FASTBOOT_RESP_FAIL "(%08x)", fastboot_status);
		fastboot_send(fastboot_handle, reply_buffer, strlen(reply_buffer));
		
		fb_printf("%sERROR: Failed to process command %s.\nError(0x%x)\n", fb_text_color_code2(error_text_color), cmd_buffer, fastboot_status);
		fb_refresh();
		
		/* Unload fastboot handle */
		fastboot_unload_handle(fastboot_handle);
		fastboot_init = 0;
	}
}

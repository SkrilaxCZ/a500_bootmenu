/*
 * Acer bootloader boot menu application userspace util.
 *
 * Copyright (C) 2013 Skrilax_CZ
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

#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>
#include "bootmenu.h"

#ifdef ANDROID
#define MISC_PARTITION "/dev/block/mmcblk0p5"
#else
#define MISC_PARTITION "/dev/mmcblk0p5"
#endif

struct option long_options[] =
{
	/* Get options */
	{ "get-command", no_argument, 0, 'c'},
	{ "get-settings", no_argument, 0, 's' },
	{ "get-boot-image", no_argument, 0, 'i' },
	{ "get-next-boot-image", no_argument, 0, 'n' },
	{ "get-boot-file", no_argument, 0, 'f' },
	/* Set options */
	{ "set-command", required_argument, 0, 'C'},
	{ "set-settings", required_argument, 0, 'S' },
	{ "set-boot-image", required_argument, 0, 'I' },
	{ "set-next-boot-image", required_argument, 0, 'N' },
	{ "set-boot-file", required_argument, 0, 'F' },
	/* End */
	{ 0, 0, 0, 0 }
};

const char* desc_options[] =
{
	/* Get options */
	"Gets the current boot command.",
	"Gets the current bootloader settings.",
	"Gets the current default boot image.",
	"Gets the next boot image.",
	"Gets the current boot file (path in bootloader format).",
	/* Set options */
	"Sets the boot command with the one specified in the argument.",
	"Sets the bootloader settings:\n"
		"\tDEBUG_OFF:       Debug mode off\n"
		"\tDEBUG_ON:        Debug mode on\n"
		"\tNOEXT4_OFF:      EXT4 boot allowed\n"
		"\tNOEXT4_ON:       EXT4 boot forbidden\n"
		"\tSHOW_FB_REC_ON:  Show fastboot / recovery on selection screen\n"
		"\tSHOW_FB_REC_OFF: Hide fastboot / recovery on selection screen\n",
	"Sets the default boot image",
	"Sets the next boot image",
	"Sets the default boot file (path in bootloader format).",
	/* End */
	NULL
};

const char* allowed_partitions[] =
{
	"LNX", /* mmcblk0p1 */
	"SOS", /* mmcblk0p2 */
	"APP", /* mmcblk0p3 */
	"CAC", /* mmcblk0p4 */
	"FLX", /* mmcblk0p6 */
	"UBN", /* custom */
	"AKB", /* mmcblk0p7 */
	"UDA", /* mmcblk0p8 */
	NULL
};

int read_msc_command(struct msc_command* cmd)
{
	FILE* f = fopen(MISC_PARTITION, "r");
	if (f == NULL)
		return 1;

	if (fread(cmd, 1, sizeof(struct msc_command), f) != sizeof(struct msc_command))
		return 1;

	fclose(f);
	return 0;
}

int write_msc_command(struct msc_command* cmd)
{
	FILE* f = fopen(MISC_PARTITION, "w");
	if (f == NULL)
		return 1;

	if (fwrite(cmd, 1, sizeof(struct msc_command), f) != sizeof(struct msc_command))
		return 1;

	fclose(f);
	return 0;
}

void error(const char* reason)
{
	int i = 0;

	if (reason != NULL)
		fprintf(stderr, "ERROR: %s\n", reason);

	fprintf(stderr, "Usage: \n\n");

	while (1)
	{
		fprintf(stderr, "--%s: %s\n", long_options[i].name, desc_options[i]);
		i++;

		if (long_options[i].name == NULL || desc_options[i] == NULL)
			break;
	}
}

#define LOAD_MSC(cmd, dirty) if (dirty == -1) { if (read_msc_command(&cmd)) { error("Failed reading MSC command!"); return 1; } dirty = 0; }

int main(int argc, char** argv)
{
	int option_index = 0;
	int get_option_possible = 1;
	int partition_valid;
	struct msc_command cmd;
	int dirty = -1;
	char array[0x100];
	char* ptr;

	int c, i, l, mode, image;

	if (argc == 1)
	{
		error("No option specified!");
		return 1;
	}
	else if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		error(NULL);
		return 0;
	}

	while (1)
	{
		c = getopt_long(argc, argv, "csifC:S:I:F:", long_options, &option_index);

		if (c == -1)
			break;

		switch (c)
		{
			case 'c':
				if (!get_option_possible)
				{
					error("Get options cannot be combined!");
					return 1;
				}

				get_option_possible = 0;
				LOAD_MSC(cmd, dirty);

				strncpy(array, cmd.boot_command, sizeof(cmd.boot_command));
				array[sizeof(cmd.boot_command)] = '\0';
				printf("%s\n", array);
				break;

			case 's':
				if (!get_option_possible)
				{
					error("Get options cannot be combined!");
					return 1;
				}

				get_option_possible = 0;
				LOAD_MSC(cmd, dirty);

				if (cmd.settings & MSC_SETTINGS_DEBUG_MODE)
					puts("DEBUG_ON|");
				else
					puts("DEBUG_OFF|");

					if (cmd.settings & MSC_SETTINGS_FORBID_EXT)
					puts("NOEXT4_ON|");
				else
					puts("NOEXT4_OFF|");

				if (cmd.settings & MSC_SETTINGS_SHOW_FB_REC)
					puts("SHOW_FB_REC_ON\n");
				else
					puts("SHOW_FB_REC_OFF\n");

				break;

			case 'i':
				if (!get_option_possible)
				{
					error("Get options cannot be combined!");
					return 1;
				}

				get_option_possible = 0;
				LOAD_MSC(cmd, dirty);
				if (cmd.boot_image == 0xFF)
					puts("last\n");
				else
					printf("%u\n", (unsigned int)cmd.boot_image);
				break;

			case 'n':
				if (!get_option_possible)
				{
					error("Get options cannot be combined!");
					return 1;
				}

				get_option_possible = 0;
				LOAD_MSC(cmd, dirty);
				if (cmd.next_boot_image == 0xFF)
					puts("unset\n");
				else
					printf("%u\n", (unsigned int)cmd.next_boot_image);
				break;

			case 'f':
				if (!get_option_possible)
				{
					error("Get options cannot be combined!");
					return 1;
				}

				get_option_possible = 0;
				LOAD_MSC(cmd, dirty);
				strncpy(array, cmd.boot_file, sizeof(cmd.boot_file));
				array[sizeof(cmd.boot_file)] = '\0';
				printf("%s\n", array);
				break;

			case 'C':
				get_option_possible = 0;
				LOAD_MSC(cmd, dirty);

				l = strlen(optarg);
				if (l > sizeof(cmd.boot_command))
				{
					error("Boot command too long!");
					return 1;
				}

				strncpy(cmd.boot_command, optarg, sizeof(cmd.boot_command));
				dirty = 1;
				break;

			case 'S':
				get_option_possible = 0;
				LOAD_MSC(cmd, dirty);

				if (!strcmp(optarg, "DEBUG_ON"))
					mode |= MSC_SETTINGS_DEBUG_MODE;
				else if (!strcmp(optarg, "DEBUG_OFF"))
					mode &= ~MSC_SETTINGS_DEBUG_MODE;
				else if (!strcmp(optarg, "NOEXT4_ON"))
					mode |= MSC_SETTINGS_FORBID_EXT;
				else if (!strcmp(optarg, "NOEXT4_OFF"))
					mode &= ~MSC_SETTINGS_FORBID_EXT;
				else if (!strcmp(optarg, "SHOW_FB_REC_ON"))
					mode |= MSC_SETTINGS_SHOW_FB_REC;
				else if (!strcmp(optarg, "SHOW_FB_REC_OFF"))
					mode &= ~MSC_SETTINGS_SHOW_FB_REC;
				else
				{
					error("Invalid bootloader settings!");
					return 1;
				}

				cmd.settings = mode;
				dirty = 1;
				break;

			case 'I':
				get_option_possible = 0;
				LOAD_MSC(cmd, dirty);

				if (sscanf(optarg, "%u", &image) != 1)
				{
					if (!strcmp(optarg, "last"))
						image = 0xFF;
					else
					{
						error("Invalid boot image!");
						return 1;
					}
				}
				else
				{
					if (image > 0xFF)
					{
						error("Invalid boot image!");
						return 1;
					}
				}

				cmd.boot_image = (unsigned char) image;
				dirty = 1;
				break;

			case 'N':
				get_option_possible = 0;
				LOAD_MSC(cmd, dirty);

				if (sscanf(optarg, "%u", &image) != 1)
				{
					if (!strcmp(optarg, "unset"))
						image = 0xFF;
					else
					{
						error("Invalid boot image!");
						return 1;
					}
				}
				else
				{
					if (image > 0xFF)
					{
						error("Invalid boot image!");
						return 1;
					}
				}

				cmd.next_boot_image = (unsigned char) image;
				dirty = 1;
				break;

			case 'F':
				get_option_possible = 0;
				LOAD_MSC(cmd, dirty);

				/* Check partition */
				ptr = strchr(optarg, ':');
				l = ((int)(ptr - optarg));

				/* Failure reading the partition */
				if (ptr == NULL || l > 4)
				{
					error("Invalid boot file parition");
					return 1;
				}

				/* Terminate it and verify it */
				*ptr = '\0';
				partition_valid = 0;
				i = 0;

				while (allowed_partitions[i])
				{
					if (!strcmp(allowed_partitions[i], optarg))
					{
						partition_valid = 1;
						break;
					}

					i++;
				}

				/* Restore it and if we have valid partition, then write */
				*ptr = ':';

				if (!partition_valid)
				{
					error("Invalid boot file parition");
					return 1;
				}

				strncpy(cmd.boot_file, optarg, sizeof(cmd.boot_file));
				dirty = 1;
				break;
		}
	}

	/* If something changed, write it */
	if (dirty == 1)
		write_msc_command(&cmd);

	return 0;
}

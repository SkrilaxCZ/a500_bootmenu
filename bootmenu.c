/*
 * Acer bootloader boot menu application main file.
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
#include <stdlib.h>
#include <bl_0_03_14.h>
#include <bootmenu.h>
#include <fastboot.h>
#include <framebuffer.h>
#include <ext2fs.h>

#define MENU_ID_CONTINUE      0
#define MENU_ID_REBOOT        1
#define MENU_ID_FASTBOOT      2
#define MENU_ID_BOOT          3
#define MENU_ID_SECBOOT       4
#define MENU_ID_RECOVERY      5
#define MENU_ID_SETBOOT				6
#define MENU_ID_TOGGLE_DEBUG	7
#define MENU_ID_WIPE_CACHE		8

/* Bootloader ID */
const char* bootloader_id = "Skrilax_CZ's bootloader V9";
const char* bootloader_short_id = "V9";

/* Boot menu items */
struct boot_menu_item boot_menu_items[] =
{
	{ "Continue", MENU_ID_CONTINUE },
	{ "Reboot", MENU_ID_REBOOT },
	{ "Fastboot Mode", MENU_ID_FASTBOOT },
	{ "Boot Primary Kernel Image", MENU_ID_BOOT },
	{ "Boot Secondary Kernel Image", MENU_ID_SECBOOT },
	{ "Boot Recovery", MENU_ID_RECOVERY },
	{ "Set Default Kernel Image", MENU_ID_SETBOOT },
	{ "" /* set in main */, MENU_ID_TOGGLE_DEBUG },
	{ "Wipe Cache", MENU_ID_WIPE_CACHE },
};

/* Menu file items */
#define MENU_SUSPEND_AKB_PARTITION "suspend=1"

#define MENU_TITLE_PROP            "title"
#define MENU_ANDROID_IMAGE_PROP    "android"
#define MENU_ZIMAGE_PROP           "zImage"
#define MENU_RAMDISK_PROP          "ramdisk"
#define MENU_CMDLINE_PROP          "cmdline"

/* Device keys */
struct gpio_key device_keys[] =
{
	/* Volume UP */
	{
		.row = 16,
		.column = 4,
		.active_low = 1,
	},
	/* Volume DOWN */
	{
		.row = 16,
		.column = 5,
		.active_low = 1,
	},
	/* Rotation Lock */
	{
		.row = 16,
		.column = 2,
		.active_low = 1,
	},
	/* Power */
	{
		.row = 8,
		.column = 3,
		.active_low = 0,
	},
};

/* MSC partition command */
struct msc_command msc_cmd;

/* How to boot */
enum boot_mode this_boot_mode = BM_NORMAL;

/* How to boot from msc command */
enum boot_mode msc_boot_mode = BM_NORMAL;

/*
 * Bootmenu frame
 */
void bootmenu_frame(int set_status)
{
	/* Centered */
	char buffer[TEXT_LINE_CHARS+1];
	const char* hint = "Use volume keys to highlight, power to select.";
	int i;
	int l = strlen(hint);

	/* clear screen */
	fb_clear();

	/* set status */
	if (set_status)
		fb_set_status("Bootmenu Mode");

	/* Draw hint */
	for (i = 0; i < (TEXT_LINE_CHARS - l) / 2; i++)
		buffer[i] = ' ';

	strncpy(&(buffer[(TEXT_LINE_CHARS - l) / 2]), hint, l);
	buffer[ARRAY_SIZE(buffer) - 1] = '\0';
	fb_printf(buffer);
	fb_printf("\n\n\n");
}

/*
 * Basic frame (normal mode)
 */
void bootmenu_basic_frame(void)
{
	/* clear screen */
	fb_clear();

	/* clear status */
	fb_set_status("");
}

/*
 * Bootmenu error
 */
void bootmenu_error(void)
{
	bootmenu_basic_frame();
	fb_color_printf("Unrecoverable bootloader error, please reboot the device manually.", NULL, &error_text_color);

	while (1)
		sleep(1000);
}

/*
 * Is key active
 */
int get_key_active(enum key_type key)
{
	int gpio_state = get_gpio(device_keys[key].row, device_keys[key].column);

	if (device_keys[key].active_low)
		return gpio_state == 0;
	else
		return gpio_state == 1;
}

/*
 * Wait for key event
 */
enum key_type wait_for_key_event(void)
{
	/* Wait for key event, debounce them first */
	while (get_key_active(KEY_VOLUME_UP))
		sleep(30);

	while (get_key_active(KEY_VOLUME_DOWN))
		sleep(30);

	while (get_key_active(KEY_POWER))
		sleep(30);

	while (1)
	{
		if (get_key_active(KEY_VOLUME_DOWN))
			return KEY_VOLUME_DOWN;

		if (get_key_active(KEY_VOLUME_UP))
			return KEY_VOLUME_UP;

		if (get_key_active(KEY_POWER))
		{
			/* Power key - act on releasing it */
			while (get_key_active(KEY_POWER))
				sleep(30);

			return KEY_POWER;
		}

		sleep(30);
	}
}

/*
 * Wait for key event with timeout
 */
enum key_type wait_for_key_event_timed(int* timeout)
{
	/* Wait for key event, debounce them first */
	while (get_key_active(KEY_VOLUME_UP))
		sleep(30);

	while (get_key_active(KEY_VOLUME_DOWN))
		sleep(30);

	while (get_key_active(KEY_POWER))
		sleep(30);

	while (1)
	{
		if (get_key_active(KEY_VOLUME_DOWN))
			return KEY_VOLUME_DOWN;

		if (get_key_active(KEY_VOLUME_UP))
			return KEY_VOLUME_UP;

		if (get_key_active(KEY_POWER))
		{
			/* Power key - act on releasing it */
			while (get_key_active(KEY_POWER))
				sleep(30);

			return KEY_POWER;
		}

		sleep(30);
		if (*timeout > 0)
			(*timeout)--;
		else if (*timeout == 0)
			return KEY_NONE;
	}
}

/*
 * Read MSC command
 */
void msc_cmd_read(void)
{
	struct msc_command my_cmd;
	int msc_pt_handle;
	uint32_t processed_bytes;

	msc_pt_handle = 0;

	if (open_partition("MSC", PARTITION_OPEN_READ, &msc_pt_handle))
		goto finish;

	if (read_partition(msc_pt_handle, &my_cmd, sizeof(my_cmd), &processed_bytes))
		goto finish;

	if (processed_bytes != sizeof(my_cmd))
		goto finish;

	memcpy(&msc_cmd, &my_cmd, sizeof(my_cmd));

finish:
	close_partition(msc_pt_handle);
	return;
}

/*
 * Write MSC Command
 */
void msc_cmd_write(void)
{
	int msc_pt_handle;
	uint32_t processed_bytes;

	msc_pt_handle = 0;

	if (open_partition("MSC", PARTITION_OPEN_WRITE, &msc_pt_handle))
		goto finish;

	write_partition(msc_pt_handle, &msc_cmd, sizeof(msc_cmd), &processed_bytes);

finish:
	close_partition(msc_pt_handle);
	return;
}

/*
 * Show menu
 */
int show_menu(struct boot_menu_item* items, int num_items, int initial_selection, const char* message, const char* error, int timeout)
{
	int selected_item = initial_selection;
	int my_timeout = timeout;
	int i, l;
	struct color* b;
	struct font_color* fc;

	/* Key press: -1 nothing, 0 Vol DOWN, 1 Vol UP, 2 Power */
	enum key_type key_press = KEY_NONE;

	/* Line builder - two color codes used */
	char line_builder[TEXT_LINE_CHARS + 8 + 1];

	if (selected_item >= num_items)
		selected_item = 0;

	while(1)
	{
		/* New frame (no status) */
		bootmenu_frame(0);

		/* Print error if we're stuck in bootmenu */
		if (error)
			fb_color_printf("%s\n\n", NULL, &error_text_color, error);

		if (message)
			fb_color_printf("%s\n\n", NULL, &text_color, message);

		/* Print options */
		for (i = 0; i < num_items; i++)
		{
			memset(line_builder, 0x20, ARRAY_SIZE(line_builder));
			line_builder[ARRAY_SIZE(line_builder) - 1] = '\0';
			line_builder[ARRAY_SIZE(line_builder) - 2] = '\n';

			if (i == selected_item)
			{
				b = &highlight_color;
				fc = &highlight_text_color;
			}
			else
			{
				b = NULL;
				fc = &text_color;
			}

			snprintf(line_builder, ARRAY_SIZE(line_builder), items[i].title);

			l = strlen(line_builder);
			if (l == ARRAY_SIZE(line_builder) - 1)
				line_builder[ARRAY_SIZE(line_builder) - 2] = '\n';
			else if (l < ARRAY_SIZE(line_builder) - 1)
				line_builder[l] = ' ';

			fb_color_printf(line_builder, b, fc);
		}

		/* Draw framebuffer */
		fb_refresh();

		my_timeout = timeout;
		if (my_timeout == 0)
			key_press = wait_for_key_event();
		else
			key_press = wait_for_key_event_timed(&my_timeout);

		/* Timeout - return (it won't return if timeout was 0) */
		if (key_press == KEY_NONE)
			return items[selected_item].id;

		/* Volume DOWN */
		if (key_press == KEY_VOLUME_DOWN)
		{
			selected_item++;

			if (selected_item >= num_items)
				selected_item = 0;

			continue;
		}

		/* Volume UP */
		if (key_press == KEY_VOLUME_UP)
		{
			selected_item--;

			if (selected_item < 0)
				selected_item = num_items - 1;

			continue;
		}

		if (key_press == KEY_POWER)
			return items[selected_item].id;
	}
}

/*
 * Add custom elements to the cmdline
 * Use snprintf to limit the size
 */
void configure_custom_cmdline(char* cmdline, int size)
{
	const char* debug_cmd;
	int len;

	if (msc_cmd.debug_mode)
		debug_cmd = "console=ttyS0,115200n8 debug_uartport=lsport ";
	else
		debug_cmd = "console=none debug_uartport=hsport ";

	snprintf(cmdline, size, debug_cmd);
	len = strlen(cmdline);
	cmdline += len;
	size -= len;

	snprintf(cmdline, size, "skrilax.bootloader=%s ", bootloader_short_id);
}

/*
 * Load boot images
 */
int load_boot_images(struct boot_selection_item* boot_items, struct boot_menu_item* menu_items, int max_items)
{
	int i, len, ret;
	int have_akb = 0;
	int num_items = 0;
	uint64_t partition_size;
	char boot_file_partition[8];
	char section[64];
	char line[256];
	struct boot_selection_item boot_current;
	struct boot_menu_item menu_current;
	char *ptr, *ptr2;

	if (max_items < 2)
		return 0;

	/* Always add primary boot (secondary only if the partition exists) */
	strncpy(boot_items[num_items].partition, "LNX", ARRAY_SIZE(boot_items[0].partition));
	boot_items[num_items].partition[ARRAY_SIZE(boot_items[0].partition) - 1] = '\0';
	boot_items[num_items].path_android[0] = '\0';
	boot_items[num_items].path_ramdisk[0] = '\0';
	boot_items[num_items].path_zImage[0] = '\0';
	boot_items[num_items].cmdline[0] = '\0';

	menu_items[num_items].title = "Primary (LNX)";
	menu_items[num_items].id = num_items;
	num_items++;

	/* Read from PT if AKB exists, if it does, add it */
	if (get_partition_size("AKB", &partition_size) == 0)
	{
		strncpy(boot_items[num_items].partition, "AKB", ARRAY_SIZE(boot_items[0].partition));
		boot_items[num_items].partition[ARRAY_SIZE(boot_items[1].partition) - 1] = '\0';
		boot_items[num_items].path_android[0] = '\0';
		boot_items[num_items].path_ramdisk[0] = '\0';
		boot_items[num_items].path_zImage[0] = '\0';
		boot_items[num_items].cmdline[0] = '\0';

		menu_items[num_items].title = "Secondary (AKB)";
		menu_items[num_items].id = num_items;
		num_items++;
		have_akb = 1;
	}

	/* Read menu file */

	/* Make sure it's null terminated */
	msc_cmd.boot_file[ARRAY_SIZE(msc_cmd.boot_file) - 1] = '\0';
	ptr = strchr(msc_cmd.boot_file, ':');
	len = ((int)(ptr - msc_cmd.boot_file));

	/* Failure reading the path */
	if (ptr == NULL || len > 4)
		return num_items;

	/* Copy partition */
	strncpy(boot_file_partition, msc_cmd.boot_file, len);
	boot_file_partition[len] = '\0';
	ptr++;

	/* Mount partition */
	if (ext2fs_mount(boot_file_partition))
		return num_items;

	/* Open the file */
	if (ext2fs_open(ptr) < 0)
		goto fail_open;

	/* Read it line by line */
	section[0] = '\0';
	boot_current.title[0] = '\0';

	while(1)
	{
		ret = ext2fs_gets(line, ARRAY_SIZE(line));

		if (!ret)
			break;

		ptr = line;

		/* Check space */
		while (*ptr == ' ' || *ptr == '\t')
			ptr++;

		/* Check comment */
		if (*ptr == ';')
			continue;

		/* Check section */
		if (*ptr == '[')
		{
			ptr2 = strchr(ptr, ']');

			if (ptr2)
			{
				ptr++;
				*ptr2 = '\0';

				/* Push the item (and it's not LNX or AKB) */
				if (section[0] != '\0' && strcmp(section, "LNX") && strcmp(section, "AKB"))
				{
					memcpy(&boot_items[num_items], &boot_current, sizeof(struct boot_selection_item));
					memcpy(&menu_items[num_items], &menu_current, sizeof(struct boot_menu_item));
					menu_items[num_items].title = boot_items[num_items].title;
					menu_items[num_items].id = num_items;
					num_items++;
				}

				/* Set section */
				strncpy(section, ptr, ARRAY_SIZE(section));
				section[ARRAY_SIZE(section) - 1] = '\0';

				/* Reset */
				strncpy(boot_current.title, section, ARRAY_SIZE(boot_current.title));
				boot_current.title[ARRAY_SIZE(boot_current.title) - 1] = '\0';
				boot_current.partition[0] = '\0';
				boot_current.path_android[0] = '\0';
				boot_current.path_ramdisk[0] = '\0';
				boot_current.path_zImage[0] = '\0';
				boot_current.cmdline[0] = '\0';

				menu_current.title = boot_current.title;
				continue;
			}
		}

		/* Check if we have section */
		if (section[0] == '\0')
			continue;

		/* Remove trailing whitespace */
		ptr2 = &ptr[strlen(ptr) - 1];

		while (ptr2 >= ptr && (*ptr2 == ' ' || *ptr2 == '\t' || *ptr2 == '\r' || *ptr2 == '\n'))
		{
			*ptr2 = '\0';
			ptr2--;
		}

		/* Handle AKB and LNX section specially */
		if (!strcmp(section, "LNX"))
		{
			/* Title only */
			if (!strncmp(ptr, MENU_TITLE_PROP "=", strlen(MENU_TITLE_PROP "=")))
			{
				ptr2 = &ptr[strlen(MENU_TITLE_PROP "=")];
				strncpy(boot_items[0].title, ptr2, ARRAY_SIZE(boot_items[0].title));
				boot_items[0].title[ARRAY_SIZE(boot_items[0].title) - 1] = '\0';
				menu_items[0].title = boot_items[0].title;
			}

			continue;
		}

		if (!strcmp(section, "AKB"))
		{
			if (have_akb)
			{
				/* Title only */
				if (!strncmp(ptr, MENU_TITLE_PROP "=", strlen(MENU_TITLE_PROP "=")))
				{
					ptr2 = &ptr[strlen(MENU_TITLE_PROP "=")];
					strncpy(boot_items[1].title, ptr2, ARRAY_SIZE(boot_items[1].title));
					boot_items[1].title[ARRAY_SIZE(boot_items[1].title) - 1] = '\0';
					menu_items[1].title = boot_items[1].title;
				}
				else if (!strcmp(ptr, MENU_SUSPEND_AKB_PARTITION))
				{
					have_akb = 0;

					if (num_items == 2)
						num_items = 1;
					else
					{
						/* Move all subsequent items one line ahead */
						for (i = 2; i < num_items; i++)
						{
							memcpy(&boot_items[i - 1], &boot_items[i], sizeof(struct boot_selection_item));
							memcpy(&menu_items[i - 1], &menu_items[i], sizeof(struct boot_menu_item));
							menu_items[i-1].title = boot_items[i-1].title;
							menu_items[i-1].id--;
						}

						num_items--;
					}
				}
			}
			continue;
		}

		/* Now, common items */
		if (!strncmp(ptr, MENU_TITLE_PROP "=", strlen(MENU_TITLE_PROP "=")))
		{
			ptr2 = &ptr[strlen(MENU_TITLE_PROP "=")];
			strncpy(boot_current.title, ptr2, ARRAY_SIZE(boot_current.title));
			boot_current.title[ARRAY_SIZE(boot_current.title) - 1] = '\0';
		}
		else if (!strncmp(ptr, MENU_ANDROID_IMAGE_PROP "=", strlen(MENU_ANDROID_IMAGE_PROP "=")))
		{
			ptr2 = &ptr[strlen(MENU_ANDROID_IMAGE_PROP "=")];
			strncpy(boot_current.path_android, ptr2, ARRAY_SIZE(boot_current.path_android));
			boot_current.path_android[ARRAY_SIZE(boot_current.path_android) - 1] = '\0';
		}
		else if (!strncmp(ptr, MENU_ZIMAGE_PROP "=", strlen(MENU_ZIMAGE_PROP "=")))
		{
			ptr2 = &ptr[strlen(MENU_ZIMAGE_PROP "=")];
			strncpy(boot_current.path_zImage, ptr2, ARRAY_SIZE(boot_current.path_zImage));
			boot_current.path_zImage[ARRAY_SIZE(boot_current.path_zImage) - 1] = '\0';
		}
		else if (!strncmp(ptr, MENU_RAMDISK_PROP "=", strlen(MENU_RAMDISK_PROP "=")))
		{
			ptr2 = &ptr[strlen(MENU_RAMDISK_PROP "=")];
			strncpy(boot_current.path_ramdisk, ptr2, ARRAY_SIZE(boot_current.path_ramdisk));
			boot_current.path_ramdisk[ARRAY_SIZE(boot_current.path_ramdisk) - 1] = '\0';
		}
		else if (!strncmp(ptr, MENU_CMDLINE_PROP "=", strlen(MENU_CMDLINE_PROP "=")))
		{
			ptr2 = &ptr[strlen(MENU_CMDLINE_PROP "=")];
			strncpy(boot_current.cmdline, ptr2, ARRAY_SIZE(boot_current.cmdline));
			boot_current.cmdline[ARRAY_SIZE(boot_current.cmdline) - 1] = '\0';
		}
	}

	/* Push it if we have something */
	if (section[0] != '\0' && strcmp(section, "LNX") && strcmp(section, "AKB"))
	{
		memcpy(&boot_items[num_items], &boot_current, sizeof(struct boot_selection_item));
		memcpy(&menu_items[num_items], &menu_current, sizeof(struct boot_menu_item));
		menu_items[num_items].title = boot_items[num_items].title;
		menu_items[num_items].id = num_items;
		num_items++;
	}

	ext2fs_close();

fail_open:
	ext2fs_unmount();
	return num_items;
}

/*
 * Show interactive boot selection
 */
void boot_interactively(int initial_selection, const char* message, const char* error, uint32_t ram_base, char* error_message, int error_message_size)
{
	struct boot_selection_item boot_items[20];
	struct boot_menu_item menu_items[20];
	int num_items, selected_item;
	const char* boot_status;
	char my_message[256];

	num_items = load_boot_images(boot_items, menu_items, ARRAY_SIZE(boot_items));

	if (num_items == 0)
	{
		if (error_message)
		{
			strncpy(error_message, "ERROR: No images to boot.", error_message_size);
			error_message[error_message_size - 1] = '\0';
		}
		return;
	}

	/* If we have only LNX, boot right away */
	if (num_items == 1)
	{
		boot_status = "Booting primary kernel image";
		if (error_message)
		{
			strncpy(error_message, "ERROR: Invalid primary (LNX) kernel image.", error_message_size);
			error_message[error_message_size - 1] = '\0';
		}

		boot_normal(&boot_items[0], boot_status, ram_base);
		return;
	}

	/* Set message */
	if (message)
		snprintf(my_message, ARRAY_SIZE(my_message), "%s\nSelect boot image:", message);
	else
		snprintf(my_message, ARRAY_SIZE(my_message), "Select boot image:");

	/* show menu with cca 5 second timeout */
	selected_item = show_menu(menu_items, num_items, initial_selection, my_message, error, 150);

	if (!strcmp(boot_items[selected_item].partition, "LNX"))
	{
		boot_status = "Booting primary kernel image";
		if (error_message)
		{
			strncpy(error_message, "ERROR: Invalid primary (LNX) kernel image.", error_message_size);
			error_message[error_message_size - 1] = '\0';
		}
	}
	else if (!strcmp(boot_items[selected_item].partition, "AKB"))
	{
		boot_status = "Booting secondary kernel image";
		if (error_message)
		{
			strncpy(error_message, "ERROR: Invalid secondary (AKB) kernel image.", error_message_size);
			error_message[error_message_size - 1] = '\0';
		}
	}
	else
	{
		boot_status = "Booting kernel image from filesystem";
		if (error_message)
		{
			strncpy(error_message, "ERROR: Invalid filesystem kernel image.", error_message_size);
			error_message[error_message_size - 1] = '\0';
		}
	}

	boot_normal(&boot_items[selected_item], boot_status, ram_base);
}

/*
 * Boots Android image (returns on ERROR)
 */
void boot_android_image_from_partition(const char* partition, uint32_t ram_base)
{
	char* bootimg_data = NULL;
	uint32_t bootimg_size = 0;

	if (!android_load_image(&bootimg_data, &bootimg_size, partition))
		return;

	if (!*(bootimg_data))
		return;

	android_boot_image(bootimg_data, bootimg_size, ram_base);
}

/*
 * Boots normally (returns on ERROR)
 */
void boot_normal(struct boot_selection_item* item, const char* status, uint32_t ram_base)
{
	int len;
	char boot_file_partition[8];
	char* ptr;
	char* bootimg;

	/* Normal mode frame */
	bootmenu_basic_frame();

	fb_set_status(status);

	if (item->partition[0] != '\0')
	{
		fb_printf("Booting android image from %s partition ...\n", item->partition);
		fb_refresh();

		boot_android_image_from_partition(item->partition, ram_base);
	}
	else if (item->path_android[0] != '\0')
	{
		ptr = strchr(item->path_android, ':');
		len = ((int)(ptr - item->path_android));

		fb_printf("Loading android image from filesystem ...", item->partition);
		fb_refresh();

		/* Failure reading the path */
		if (ptr == NULL || len > 4)
		{
			fb_printf(" FAIL\n", item->partition);
			fb_refresh();
			sleep(2000);
			return;
		}

		strncpy(boot_file_partition, msc_cmd.boot_file, len);
		boot_file_partition[len] = '\0';
		ptr++;

		/* Mount partition */
		if (ext2fs_mount(boot_file_partition))
		{
			fb_printf(" FAIL\n", item->partition);
			fb_refresh();
			sleep(2000);
			return;
		}

		/* Open the file */
		len = ext2fs_open(ptr);
		if (len < 0)
		{
			ext2fs_unmount();
			fb_printf(" FAIL\n", item->partition);
			fb_refresh();
			sleep(2000);
			return;
		}

		bootimg = malloc(len);
		ext2fs_read(bootimg, len);
		ext2fs_close();
		ext2fs_unmount();

		fb_printf(" OK\n", item->partition);
		fb_printf("Booting ...", item->partition);
		fb_refresh();
		android_boot_image(bootimg, len, ram_base);
	}
}

/*
 * Boots from specified partition (used for boots for failures in ext4)
 */
 void boot_partition(const char* partition, const char* status, uint32_t ram_base)
{
	/* Normal mode frame */
	bootmenu_basic_frame();

	fb_set_status(status);
	fb_printf("Booting android image from %s partition ...\n", partition);
	fb_refresh();

	boot_android_image_from_partition(partition, ram_base);
}

/*
 * Boots to recovery
 */
void boot_recovery(uint32_t ram_base)
{
	/* Normal mode frame */
	bootmenu_basic_frame();

	fb_set_status("Booting recovery kernel image");
	fb_refresh();

	boot_android_image_from_partition("SOS", ram_base);
}

/*
 * Select default boot partition
 */
void set_default_boot_image(int initial_selection)
{
	struct boot_selection_item boot_items[20];
	struct boot_menu_item menu_items[20];
	int num_items = 0;
	int selected_item;

	num_items = load_boot_images(boot_items, menu_items, ARRAY_SIZE(boot_items));

	/* show the menu */
	selected_item = show_menu(menu_items, num_items, initial_selection, "Select default boot image:", NULL, 0);
	msc_cmd.boot_image = selected_item;
	msc_cmd_write();
}

/*
 * Entry point of bootmenu
 * Magic is Acer BL data structure, used as parameter for some functions.
 *
 * - keystate at boot is loaded
 * - boot partition (primary or secondary) is loaded
 * - can continue boot, and force fastboot or recovery mode
 *
 * ram_base
 */
void main(void* global_handle, uint32_t ram_base)
{
	/* Debug mode status */
	const char* debug_mode_str;

	/* Print error, from which partition booting failed */
	char error_message[TEXT_LINE_CHARS + 1];
	const char* error_message_ptr;
	int menu_selection;

	error_message[0] = '\0';

	/* Write to the log */
	printf("BOOTMENU: %s\n", bootloader_id);

	/* Initialize framebuffer */
	fb_init();

	/* Set title */
	fb_set_title(bootloader_id);

	/* Print it */
	fb_refresh();

	/* Ensure we have bootloader update */
	check_bootloader_update(global_handle);

	/* Read msc command */
	msc_cmd_read();

	/* Check if we should wipe cache */
	if (msc_cmd.erase_cache)
	{
		/* Erase cache */
		format_partition("CAC");

		/* Clear the flag */
		msc_cmd.erase_cache = 0;
	}

	/* First, check MSC command */
	if (!strncmp((const char*)msc_cmd.boot_command, MSC_CMD_RECOVERY, strlen(MSC_CMD_RECOVERY)))
		this_boot_mode = BM_RECOVERY;
	else if (!strncmp((const char*)msc_cmd.boot_command, MSC_CMD_FCTRY_RESET, strlen(MSC_CMD_FCTRY_RESET)))
		this_boot_mode = BM_FCTRY_RESET;
	else if (!strncmp((const char*)msc_cmd.boot_command, MSC_CMD_FASTBOOT, strlen(MSC_CMD_FASTBOOT)))
		this_boot_mode = BM_FASTBOOT;
	else if (!strncmp((const char*)msc_cmd.boot_command, MSC_CMD_BOOTMENU, strlen(MSC_CMD_BOOTMENU)))
		this_boot_mode = BM_BOOTMENU;
	else
		this_boot_mode = BM_NORMAL;

	msc_boot_mode = this_boot_mode;

	/* Evaluate key status */
	if (get_key_active(KEY_VOLUME_UP))
		this_boot_mode = BM_BOOTMENU;
	else if (get_key_active(KEY_VOLUME_DOWN))
		this_boot_mode = BM_RECOVERY;

	/* Clear boot command from msc */
	memset(msc_cmd.boot_command, 0, ARRAY_SIZE(msc_cmd.boot_command));
	msc_cmd_write();

	/* Set debug mode */
	if (msc_cmd.debug_mode == 0)
	{
		debug_mode_str = "Debug Mode: OFF";
		boot_menu_items[7].title = "Set Debug Mode ON";
	}
	else
	{
		debug_mode_str = "Debug Mode: ON";
		boot_menu_items[7].title = "Set Debug Mode OFF";
	}

	/* Evaluate boot mode */
	if (this_boot_mode == BM_NORMAL)
	{
		if (error_message[0] == '\0')
			error_message_ptr = NULL;
		else
			error_message_ptr = error_message;

		boot_interactively(msc_cmd.boot_image, debug_mode_str, error_message_ptr, ram_base, error_message, ARRAY_SIZE(error_message));
	}
	else if (this_boot_mode == BM_RECOVERY)
	{
		boot_recovery(ram_base);
		snprintf(error_message, ARRAY_SIZE(error_message), "ERROR: Invalid recovery (SOS) kernel image.");
	}
	else if (this_boot_mode == BM_FCTRY_RESET)
	{
		fb_set_status("Factory reset\n");

		/* Erase userdata */
		fb_printf("Erasing UDA partition...\n\n");
		fb_refresh();
		format_partition("UDA");

		/* Erase cache */
		fb_printf("Erasing CAC partition...\n\n");
		fb_refresh();
		format_partition("CAC");

		/* Finished */
		fb_printf("Done.\n");
		fb_refresh();

		/* Sleep */
		sleep(5000);

		/* Reboot */
		reboot(global_handle);

		/* Reboot returned */
		bootmenu_error();
	}
	else if (this_boot_mode == BM_FASTBOOT)
	{
		/* Load fastboot */
		fastboot_main(global_handle, ram_base, error_message, ARRAY_SIZE(error_message));

		/* Fastboot returned - show bootmenu */
	}

	/* Allright - now we're in bootmenu */

	/* Boot menu */
	while (1)
	{
		/* New frame */
		bootmenu_frame(1);

		if (error_message[0] == '\0')
			error_message_ptr = NULL;
		else
			error_message_ptr = error_message;

		/* Check menu selection*/
		menu_selection = show_menu(boot_menu_items, ARRAY_SIZE(boot_menu_items), 0, debug_mode_str, error_message_ptr, 0);

		switch(menu_selection)
		{
			case MENU_ID_CONTINUE: /* Continue */
				if (error_message[0] == '\0')
					error_message_ptr = NULL;
				else
					error_message_ptr = error_message;

				boot_interactively(msc_cmd.boot_image, debug_mode_str, error_message_ptr, ram_base, error_message, ARRAY_SIZE(error_message));
				break;

			case MENU_ID_REBOOT: /* Reboot */
				reboot(global_handle);

				/* Reboot returned */
				bootmenu_error();
				break;

			case MENU_ID_FASTBOOT: /* Fastboot mode */
				fastboot_main(global_handle, ram_base, error_message, ARRAY_SIZE(error_message));

				/* Returned? Continue bootmenu */
				break;

			case MENU_ID_BOOT: /* Primary kernel image */
				boot_partition("LNX", "Booting primary kernel image", ram_base);
				snprintf(error_message, ARRAY_SIZE(error_message), "ERROR: Invalid primary (LNX) kernel image.");
				break;

			case MENU_ID_SECBOOT: /* Secondary kernel image */
				boot_partition("AKB", "Booting secondary kernel image", ram_base);
				snprintf(error_message, ARRAY_SIZE(error_message), "ERROR: Invalid secondary (AKB) kernel image.");
				break;

			case MENU_ID_RECOVERY: /* Recovery kernel image */
				boot_recovery(ram_base);
				snprintf(error_message, ARRAY_SIZE(error_message), "ERROR: Invalid recovery (SOS) kernel image.");
				break;

			case MENU_ID_SETBOOT: /* Sets default boot image */
				set_default_boot_image(msc_cmd.boot_image);
				break;

			case MENU_ID_TOGGLE_DEBUG: /* Toggle debug mode */
				msc_cmd.debug_mode = !msc_cmd.debug_mode;
				msc_cmd_write();
				printf("BOOTMENU: Debug mode %d\n", msc_cmd.debug_mode);

				if (msc_cmd.debug_mode == 0)
				{
					debug_mode_str = "Debug Mode: OFF";
					boot_menu_items[7].title = "Set Debug Mode ON";
				}
				else
				{
					debug_mode_str = "Debug Mode: ON";
					boot_menu_items[7].title = "Set Debug Mode OFF";
				}

				break;

			case MENU_ID_WIPE_CACHE: /* Wipe cache */
				bootmenu_basic_frame();
				fb_set_status("Bootmenu Mode");
				fb_printf("Erasing CAC partition...\n\n");
				fb_refresh();

				format_partition("CAC");

				fb_printf("Done.\n");
				fb_refresh();
				sleep(2000);

				break;
		}

	}
}

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
#include <bl_0_03_14.h>
#include <bootmenu.h>
#include <fastboot.h>
#include <framebuffer.h>

/* Bootloader ID */
const char* bootloader_id = "Skrilax_CZ's bootloader V9";

/* Boot menu items */
const char* boot_menu_items[] =
{
	"Reboot",
	"Fastboot Mode",
	"Boot Primary Kernel Image",
	"Boot Secondary Kernel Image",
	"Boot Recovery",
	"Set Boot %s Kernel Image",
	"Set Debug Mode %s",
	"Wipe Cache",
};

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
enum key_type wait_for_key_event()
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
 * Read MSC command
 */
void msc_cmd_read()
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
void msc_cmd_write()
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
 * Boot Android (returns on ERROR)
 */
void boot_android_image(const char* partition, int boot_handle)
{
	char* bootimg_data = NULL;
	uint32_t bootimg_size = 0;
	
	if (!android_load_image(&bootimg_data, &bootimg_size, partition))
		return;
	
	if (!*(bootimg_data))
		return;
	
	android_boot_image(bootimg_data, bootimg_size, boot_handle);
}

/*
 * Boots normally (returns on ERROR)
 */
void boot_normal(int boot_partition, int boot_handle)
{
	/* Normal mode frame */
	bootmenu_basic_frame();
	
	if (boot_partition == 0)
	{
		fb_set_status("Booting primary kernel image");
		fb_refresh();
		
		boot_android_image("LNX", boot_handle);
	}
	else
	{
		fb_set_status("Booting secondary kernel image");
		fb_refresh();
		
		boot_android_image("AKB", boot_handle);
	}
}

/*
 * Boots to recovery (returns on ERROR)
 */
void boot_recovery(int boot_handle)
{
	/* Normal mode frame */
	bootmenu_basic_frame();
	
	fb_set_status("Booting recovery kernel image");
	fb_refresh();
	boot_android_image("SOS", boot_handle);
}

/*
 * Bootmenu frame
 */
void bootmenu_frame(void)
{
	/* Centered */
	char buffer[TEXT_LINE_CHARS+1];
	const char* hint = "Use volume keys to highlight, power to select.";
	int i;
	int l = strlen(hint);
	
	/* clear screen */
	fb_clear();
	
	/* set status */
	fb_set_status("Bootmenu Mode");
	
	/* Draw hint */
	for (i = 0; i < (TEXT_LINE_CHARS - l) / 2; i++)
		buffer[i] = ' ';

	strncpy(&(buffer[(TEXT_LINE_CHARS - l) / 2]), hint, l);
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
 * Entry point of bootmenu 
 * Magic is Acer BL data structure, used as parameter for some functions.
 * 
 * - keystate at boot is loaded
 * - boot partition (primary or secondary) is loaded
 * - can continue boot, and force fastboot or recovery mode
 * 
 * boot_handle - pass to boot partition
 */
void main(void* global_handle, int boot_handle)
{
	/* Selected option in boot menu */
	int selected_option = 0;
	
	/* Key press: -1 nothing, 0 Vol DOWN, 1 Vol UP, 2 Power */
	enum key_type key_press = KEY_NONE;

	/* Which kernel image is booted */
	const char* boot_partition_str;
	const char* other_boot_partition_str;
	const char* boot_partition_attempt;
	
	/* Debug mode status */
	const char* debug_mode_str;
	const char* other_debug_mode_str;
	
	/* Print error, from which partition booting failed */
	char error_message[TEXT_LINE_CHARS + 1];
	
	/* Line builder - two color codes used */
	char line_builder[TEXT_LINE_CHARS + 8 + 1];
	
	int i, l;
	struct color* b;
	struct font_color* fc;
	
	error_message[0] = '\0';
		
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
	
	/* Evaluate boot mode */
	if (this_boot_mode == BM_NORMAL)
	{		
		if (msc_cmd.boot_partition == 0)
			boot_partition_attempt = "primary (LNX)";
		else
			boot_partition_attempt = "secondary (AKB)";
		
		boot_normal(msc_cmd.boot_partition, boot_handle);
		snprintf(error_message, ARRAY_SIZE(error_message), "ERROR: Invalid %s kernel image.", boot_partition_attempt);
	}
	else if (this_boot_mode == BM_RECOVERY)
	{
		boot_recovery(boot_handle);
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
		fastboot_main(global_handle, boot_handle, error_message, ARRAY_SIZE(error_message));
		
		/* Fastboot returned - show bootmenu */
	}
	
	/* Allright - now we're in bootmenu */
	
	/* Boot menu */
	while (1)
	{ 
		/* New frame */
		bootmenu_frame();
		
		/* Print current boot mode */
		if (msc_cmd.boot_partition == 0)
		{
			boot_partition_str = "Primary";
			other_boot_partition_str = "Secondary";
		}
		else
		{
			boot_partition_str = "Secondary";
			other_boot_partition_str = "Primary";
		}
		
		fb_printf("Current boot mode: %s kernel image\n", boot_partition_str);
		
		if (msc_cmd.debug_mode == 0)
		{
			debug_mode_str = "OFF";
			other_debug_mode_str = "ON";
		}
		else
		{
			debug_mode_str = "ON";
			other_debug_mode_str = "OFF";
		}
		
		fb_printf("Debug mode: %s\n\n", debug_mode_str);
		
		/* Print error if we're stuck in bootmenu */
		if (error_message[0] != '\0')
			fb_color_printf("%s\n\n", NULL, &error_text_color, error_message);
		else
			fb_printf("\n");
		
		/* Print options */
		for (i = 0; i < ARRAY_SIZE(boot_menu_items); i++)
		{
			memset(line_builder, 0x20, ARRAY_SIZE(line_builder));
			line_builder[ARRAY_SIZE(line_builder) - 1] = '\0';
			line_builder[ARRAY_SIZE(line_builder) - 2] = '\n';
			
			if (i == selected_option)
			{
				b = &highlight_color;
				fc = &highlight_text_color;
			}
			else
			{
				b = NULL;
				fc = &text_color;
			}
			
			if (i == 5)
				snprintf(line_builder, ARRAY_SIZE(line_builder), boot_menu_items[i], other_boot_partition_str);
			else if (i == 6)
				snprintf(line_builder, ARRAY_SIZE(line_builder), boot_menu_items[i], other_debug_mode_str);
			else
				snprintf(line_builder, ARRAY_SIZE(line_builder), boot_menu_items[i]);
			
			l = strlen(line_builder);
			if (l == ARRAY_SIZE(line_builder) - 1)
				line_builder[ARRAY_SIZE(line_builder) - 2] = '\n';
			else if (l < ARRAY_SIZE(line_builder) - 1)
				line_builder[l] = ' ';
			
			fb_color_printf(line_builder, b, fc);
		}
		
		/* Draw framebuffer */
		fb_refresh();
		
		key_press = wait_for_key_event();
		
		if (key_press == KEY_NONE)
			continue;
		
		/* Volume DOWN */
		if (key_press == KEY_VOLUME_DOWN)
		{
			selected_option++;
			
			if (selected_option >= ARRAY_SIZE(boot_menu_items))
				selected_option = 0;
			
			continue;
		}
		
		/* Volume UP */
		if (key_press == KEY_VOLUME_UP)
		{
			selected_option--;
			
			if (selected_option < 0)
				selected_option = ARRAY_SIZE(boot_menu_items) - 1;
			
			continue;
		}
		
		/* Power */
		if (key_press == KEY_POWER)
		{
			switch(selected_option)
			{
				case 0: /* Reboot */
					reboot(global_handle);
					
					/* Reboot returned */
					bootmenu_error();
					
				case 1: /* Fastboot mode */
					fastboot_main(global_handle, boot_handle, error_message, ARRAY_SIZE(error_message));
					
					/* Returned? Continue bootmenu */
					break;
				
				case 2: /* Primary kernel image */
					boot_normal(0, boot_handle);
					snprintf(error_message, ARRAY_SIZE(error_message), "ERROR: Invalid primary (LNX) kernel image.");
					break;
					
				case 3: /* Secondary kernel image */
					boot_normal(1, boot_handle);
					snprintf(error_message, ARRAY_SIZE(error_message), "ERROR: Invalid secondary (AKB) kernel image.");
					break;
					
				case 4: /* Recovery kernel image */
					boot_recovery(boot_handle);
					snprintf(error_message, ARRAY_SIZE(error_message), "ERROR: Invalid recovery (SOS) kernel image.");
					break;
					
				case 5: /* Toggle boot kernel image */
					msc_cmd.boot_partition = !msc_cmd.boot_partition;
					msc_cmd_write();
					selected_option = 0;
					break;
					
				case 6: /* Toggle debug mode */
					msc_cmd.debug_mode = !msc_cmd.debug_mode;
					msc_cmd_write();
					selected_option = 0;
					break;
					
				case 7: /* Wipe cache */
					bootmenu_basic_frame();
					fb_set_status("Bootmenu Mode");
					fb_printf("Erasing CAC partition...\n\n");
					fb_refresh();
					
					format_partition("CAC");
					
					fb_printf("Done.\n");
					fb_refresh();
					sleep(2000);
					
					selected_option = 0;
					break;
			}
		}
		
	}
}

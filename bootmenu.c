/* 
 * Acer bootloader boot menu application main file.
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

/* Boot menu items - '%c' is either ">" when selected or " " when not selected */
const char* boot_menu_items[] =
{
	"%c Reboot\n",
	"%c Fastboot Mode\n",
	"%c Boot Primary Kernel Image\n",
	"%c Boot Secondary Kernel Image\n",
	"%c Boot Recovery\n",
	"%c Set Boot %s Kernel Image\n",
	"%c Set Debug Mode %s\n",
	"%c Wipe Cache\n",
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

/* How to boot */
enum boot_mode this_boot_mode = BM_NORMAL;

/* How to boot from msc command */
enum boot_mode msc_boot_mode = BM_NORMAL;

/* Full bootloader version */
char full_bootloader_version[0x80];

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
		sleep(100);
	
	while (get_key_active(KEY_VOLUME_DOWN))
		sleep(100);
	
	while (get_key_active(KEY_POWER))
		sleep(100);
		
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
				sleep(100);
			
			return KEY_POWER;
		}
		
		sleep(100);
	}
}

/* 
 * Get debug mode
 */
int get_debug_mode(void)
{
	return msc_cmd->debug_mode;
}

/*
 * Boot Android (returns on ERROR)
 */
void boot_android_image(const char* partition, int boot_magic_value)
{
	char* kernel_code = NULL;
	int kernel_ep = 0;
	
	if (!android_load_image(&kernel_code, &kernel_ep, partition))
		return;
	
	if (!*(kernel_code))
		return;
	
	android_boot_image(kernel_code, kernel_ep, boot_magic_value);
}

/*
 * Boots normally (returns on ERROR)
 */
void boot_normal(int boot_partition, int boot_magic_value)
{
	if (boot_partition == 0)
	{
		println_display("Booting primary kernel image");
		boot_android_image("LNX", boot_magic_value);
	}
	else
	{
		println_display("Booting secondary kernel image");
		boot_android_image("AKB", boot_magic_value);
	}
}

/*
 * Boots to recovery (returns on ERROR)
 */
void boot_recovery(int boot_magic_value)
{
	println_display("Booting recovery kernel image");
	boot_android_image("SOS", boot_magic_value);
}

/*
 * Clear screen & Print ID
 */
void bootmenu_new_frame(void)
{
	char buffer[0x80];
	
	/* clear screen */
	clear_screen();
	
	/* print bootlogo */
	print_bootlogo();
	
	/* print id */
	snprintf(buffer, 0x80, "%s: %s", full_bootloader_version, "Bootmenu Mode"); 
	
	println_display(buffer);
	println_display("Use volume keys to highlight, power to select.\n\n");
}

/*
 * Restore frame for stock BL
 */
void bootmenu_basic_frame(void)
{
	/* clear screen */
	clear_screen();
	
	/* print bootlogo */
	print_bootlogo();
	
	/* Print booloader ID */
	println_display(full_bootloader_version);
}

/*
 * Bootmenu error
 */
void error(void)
{
	bootmenu_basic_frame();
	println_display_error("\nUnrecoverable bootloader error, please reboot the device manually.");
	
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
 * boot magic_boot_argument - pass to boot partition
 */
void main(void* magic, int magic_boot_argument)
{
	/* Selected option in boot menu */
	int selected_option = 0;
	
	/* Key press: -1 nothing, 0 Vol DOWN, 1 Vol UP, 2 Power */
	enum key_type key_press = KEY_NONE;

	/* Which kernel image is booted */
	const char* boot_partition_str;
	const char* other_boot_partition_str;
	int boot_partition;
	
	/* Debug mode status */
	const char* debug_mode_str;
	const char* other_debug_mode_str;
	int debug_mode;
	
	/* Print error, from which partition booting failed */
	const char* boot_partition_attempt = NULL;
	
	int i;
	char c;
	
	/* Fill full bootloader version */
	snprintf(full_bootloader_version, 0x80, bootloader_id, bootloader_version);
	
	/* Ensure we have bootloader update */
	check_bootloader_update(magic);
	
	/* First, check MSC command */
	if (!strncmp((const char*)msc_cmd->boot_command, MSC_CMD_RECOVERY, strlen(MSC_CMD_RECOVERY)))
		this_boot_mode = BM_RECOVERY;
	else if (!strncmp((const char*)msc_cmd->boot_command, MSC_CMD_FCTRY_RESET, strlen(MSC_CMD_FCTRY_RESET)))
		this_boot_mode = BM_FCTRY_RESET;
	else if (!strncmp((const char*)msc_cmd->boot_command, MSC_CMD_FASTBOOT, strlen(MSC_CMD_FASTBOOT)))
		this_boot_mode = BM_FASTBOOT;
	else if (!strncmp((const char*)msc_cmd->boot_command, MSC_CMD_BOOTMENU, strlen(MSC_CMD_BOOTMENU)))
		this_boot_mode = BM_BOOTMENU;
	else
		this_boot_mode = BM_NORMAL;
	
	msc_boot_mode = this_boot_mode;
	
	/* Evaluate key status */
	if (get_key_active(KEY_VOLUME_UP))
		this_boot_mode = BM_BOOTMENU;
	else if (get_key_active(KEY_VOLUME_DOWN))
		this_boot_mode = BM_RECOVERY;
	
	/* Get boot partiiton */
	boot_partition = msc_cmd->boot_partition;
	
	/* Get debug mode */
	debug_mode = msc_cmd->debug_mode;
	
	/* Clear msc command */
	msc_cmd_clear();
	
	/* Evaluate boot mode */
	if (this_boot_mode == BM_NORMAL)
	{
		if (boot_partition == 0)
			boot_partition_attempt = "primary (LNX)";
		else
			boot_partition_attempt = "secondary (AKB)";
		
		boot_normal(boot_partition, magic_boot_argument);
	}
	else if (this_boot_mode == BM_RECOVERY)
	{
		boot_partition_attempt = "recovery (SOS)";
		boot_recovery(magic_boot_argument);
	}
	else if (this_boot_mode == BM_FCTRY_RESET)
	{
		println_display("Factory reset\n");
		
		/* Erase userdata */
		println_display("Erasing UDA partition...\n");
		format_partition("UDA");
		 
		/* Erase cache */
		println_display("Erasing CAC partition...\n");
		format_partition("CAC");
		
		/* Finished */
		println_display("Done.");
		
		/* Sleep */
		sleep(5000);
		
		/* Reboot */
		reboot(magic);
		
		/* Reboot returned */
		error();
	}
	else if (this_boot_mode == BM_FASTBOOT)
	{
		/* Return -> jump to fastboot */
		return;
	}
	
	/* Allright - now we're in bootmenu */
	
	/* Boot menu */
	while (1)
	{ 
		/* New frame */
		bootmenu_new_frame();
		
		/* Print current boot mode */
		if (boot_partition == 0)
		{
			boot_partition_str = "Primary";
			other_boot_partition_str = "Secondary";
		}
		else
		{
			boot_partition_str = "Secondary";
			other_boot_partition_str = "Primary";
		}
		
		println_display("Current boot mode: %s kernel image", boot_partition_str);
		
		if (debug_mode == 0)
		{
			debug_mode_str = "OFF";
			other_debug_mode_str = "ON";
		}
		else
		{
			debug_mode_str = "ON";
			other_debug_mode_str = "OFF";
		}
		
		println_display("Debug mode: %s\n\n", debug_mode_str);
		
		/* Print error if we're stuck in bootmenu */
		if (boot_partition_attempt)
			println_display_error("ERROR: Invalid %s kernel image.\n\n", boot_partition_attempt);
		
		println_display("");
		
		/* Print options */
		for (i = 0; i < ARRAY_SIZE(boot_menu_items); i++)
		{
			if (i == selected_option)
				c = '>';
			else
				c = ' ';
			
			if (i == 5)
				println_display(boot_menu_items[i], c, other_boot_partition_str);
			else if (i == 6)
				println_display(boot_menu_items[i], c, other_debug_mode_str);
			else
				println_display(boot_menu_items[i], c);
		}
		
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
					reboot(magic);
					
					/* Reboot returned */
					error();
					
				case 1: /* Fastboot mode */
					return;
				
				case 2: /* Primary kernel image */
					boot_partition_attempt = "primary (LNX)";
					bootmenu_basic_frame();
					boot_normal(0, magic_boot_argument);
					break;
					
				case 3: /* Secondary kernel image */
					boot_partition_attempt = "secondary (AKB)";
					bootmenu_basic_frame();
					boot_normal(1, magic_boot_argument);
					break;
					
				case 4: /* Recovery kernel image */
					boot_partition_attempt = "recovery (SOS)";
					bootmenu_basic_frame();
					boot_recovery(magic_boot_argument);
					break;
					
				case 5: /* Toggle boot kernel image */
					boot_partition = !boot_partition;
					msc_set_boot_partition(boot_partition);
					selected_option = 0;
					break;
					
				case 6: /* Toggle debug mode */
					debug_mode = !debug_mode;
					msc_set_debug_mode(debug_mode);
					selected_option = 0;
					break;
					
				case 7: /* Wipe cache */
					bootmenu_new_frame();
					println_display("Erasing CAC partition...");
					format_partition("CAC");
					selected_option = 0;
					break;
			}
		}
		
	}
}

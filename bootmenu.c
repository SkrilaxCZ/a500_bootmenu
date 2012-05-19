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

/* Boot menu items - '%c' is either ">" when selected or " " when not selected */
const char* boot_menu_items[] =
{
	"%c Reboot\n",
	"%c Fastboot Mode\n",
	"%c Boot Recovery\n",
	"%c Set Boot %s Kernel Image\n",
	"%c Set Debug Mode %s\n",
	"%c Wipe Cache\n",
};

/* Boot menu application main function */

/*
 * Clear screen & Print ID
 */
void new_frame()
{
	char buffer[0x80];
	char* ptr;
		
	/* clear screen */
	clear_screen();
	
	/* print bootlogo */
	print_bootlogo();
	
	/* print id */
	snprintf(buffer, 0x100, bootloader_id, bootloader_version);
	ptr = &(buffer[strlen(buffer)]);
	strncpy(ptr, ": Bootmenu Mode\n", strlen(": Bootmenu Mode")); 
	
	println_display(buffer);
	println_display("Use volume keys to highlight, power to select.\n\n");	
}

/*
 * Restore frame for stock BL
 */
void restore_frame()
{
	/* clear screen */
	clear_screen();
	
	/* print bootlogo */
	print_bootlogo();
	
	/* Print booloader ID */
	println_display(bootloader_id, bootloader_version);
}

/*
 * Bootmenu error
 */
void error()
{
	new_frame();
	println_display_error("Unrecoverable bootloader error, please reboot the device manually.");
	
	while (1) { }
}

/* 
 * Entry point of bootmenu 
 * Magic is Acer BL data structure, used as parameter for some functions.
 * 
 * - keystate at boot is loaded
 * - boot partition (primary or secondary) is loaded
 * - can continue boot, and force fastboot or recovery mode
 */
void main(void* magic)
{
	/* Selected option in boot menu */
	int selected_option = 0;
	
	/* Key press: -1 nothing, 0 Vol DOWN, 1 Vol UP, 2 Power */
	int key_press = -1;

	/* Which kernel image is booted */
	const char* boot_partition_str;
	const char* other_boot_partition_str;
	int boot_partition;
	
	/* Debug mode status */
	const char* debug_mode_str;
	const char* other_debug_mode_str;
	int debug_mode;
	
	int i;
	char c;
		
	/* Boot to bootmenu = volume key up during boot,
	 * or in msc command "BootmenuMode"
	 */
	if (!boot_key_volume_up_pressed())
	{
		if (strncmp((const char*)msc_cmd->boot_command, "BootmenuMode", strlen("BootmenuMode")))
			return; /* continue booting normally */
	}
	
	/* Reset it */
	set_boot_normal();
	
	/* Clear msc command */
	msc_cmd_clear();
	
	/* Get boot partiiton */
	boot_partition = msc_cmd->boot_mode;
	
	/* Get debug mode */
	debug_mode = msc_cmd->debug_mode;
	
	/* Boot menu */
	while (1)
	{ 
		/* New frame */
		new_frame();
		
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
		
		println_display("Debug mode: %s\n\n\n\n", debug_mode_str);
		
		/* Print options */
		for (i = 0; i < ARRAY_SIZE(boot_menu_items); i++)
		{
			if (i == selected_option)
				c = '>';
			else
				c = ' ';
			
			if (i == 3)
				println_display(boot_menu_items[i], c, other_boot_partition_str);
			else if (i == 4)
				println_display(boot_menu_items[i], c, other_debug_mode_str);
			else
				println_display(boot_menu_items[i], c);
		}
		
		/* Now wait for key event, debounce them first */
		while (key_volume_up_pressed()) { }
		while (key_volume_down_pressed()) { }
		while (key_power_pressed()) { }
		
		key_press = -1;
		
		while (1)
		{
			if (key_volume_down_pressed())
			{
				key_press = 0;
				break;
			}
			
			if (key_volume_up_pressed())
			{
				key_press = 1;
				break;
			}
			
			if (key_power_pressed())
			{
				/* Power key - act on releasing it */
				while (key_power_pressed()) { }
				
				key_press = 2;
				break;
			}
		}
		
		if (key_press < 0)
			continue;
		
		/* Volume DOWN */
		if (key_press == 0)
		{
			selected_option++;
			
			if (selected_option >= ARRAY_SIZE(boot_menu_items))
				selected_option = 0;
			
			continue;
		}
		
		/* Volume UP */
		if (key_press == 1)
		{
			selected_option--;
			
			if (selected_option < 0)
				selected_option = ARRAY_SIZE(boot_menu_items) - 1;
			
			continue;
		}
		
		/* Power */
		if (key_press == 2)
		{
			switch(selected_option)
			{
				case 0: /* Reboot */
					reboot(magic);
					
					/* Reboot returned*/
					error();
					return;
					
				case 1: /* Fastboot mode */
					set_boot_fastboot_mode();
					return;
					
				case 2: /* Recovery mode */
					set_boot_recovery();
					restore_frame();
					return;
					
				case 3: /* Toggle boot kernel image */
					boot_partition = !boot_partition;
					msc_set_boot_partition(boot_partition);
					selected_option = 0;
					break;
					
				case 4: /* Toggle debug mode */
					debug_mode = !debug_mode;
					msc_set_debug_mode(debug_mode);
					selected_option = 0;
					break;
					
				case 5: /* Wipe cache */
					new_frame();
					println_display("Erasing CAC partition...");
					format_partition("CAC");
					selected_option = 0;
					break;
			}
		}
		
	}
}

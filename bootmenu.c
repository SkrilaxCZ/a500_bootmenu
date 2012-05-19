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
	"%c Continue\n",
	"%c Fastboot Mode\n",
	"%c Boot Recovery\n",
	"%c Set Boot Primary Kernel Image\n",
	"%c Set Boot Secondary Kernel Image\n",
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
	println_display("Use Volume DOWN to list, Volume UP to select.\n\n");	
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
 * Entry point of bootmenu: 
 * 
 * - keystate at boot is loaded
 * - boot partition (primary or secondary) is loaded
 * - can continue boot, and force fastboot or recovery mode
 */
void main()
{
	/* Selected option in boot menu */
	int selected_option = 0;
	
	/* Key press: -1 nothing, 0 Vol DOWN, 1 Vol UP */
	int key_press = -1;

	/* Which kernel image is booted */
	const char* boot_mode;
	int boot_partition;
	
	int i;
	char c;
		
	/* Boot to bootmenu = volume key up during boot */
	if (!boot_key_volume_up_pressed())
		return; /* continue booting normally */
	
	/* Reset it */
	set_boot_normal();
	
	/* Clear msc command */
	msc_cmd_clear();
	
	/* Get boot partiiton */
	boot_partition = msc_get_boot_partition();
	
	/* Boot menu */
	while (1)
	{ 
		/* New frame */
		new_frame();
		
		/* Print current boot mode */
		if (boot_partition == 0)
			boot_mode = "Primary";
		else
			boot_mode = "Secondary";
		
		println_display("Current boot mode: %s kernel image\n\n\n\n", boot_mode);
		
		/* Print options */
		for (i = 0; i < ARRAY_SIZE(boot_menu_items); i++)
		{
			if (i == selected_option)
				c = '>';
			else
				c = ' ';
			
			println_display(boot_menu_items[i], c);
		}
		
		/* Now wait for key event, debounce them first */
		while (key_volume_up_pressed()) { }
		
		while (key_volume_down_pressed()) { }
		
		key_press = -1;
		
		while (1)
		{
			if (key_volume_up_pressed())
			{
				key_press = 1;
				break;
			}
			
			if (key_volume_down_pressed())
			{
				key_press = 0;
				break;
			}
		}
		
		if (key_press < 0)
			continue;
		
		/* We have keypress */
		if (key_press == 0)
		{
			selected_option++;
			
			if (selected_option >= ARRAY_SIZE(boot_menu_items))
				selected_option = 0;
			
			continue;
		}
		
		if (key_press == 1)
		{
			switch(selected_option)
			{
				case 0: /* Continue */
					restore_frame();
					return;
					
				case 1: /* Fastboot mode */
					set_boot_fastboot_mode();
					return;
					
				case 2: /* Recovery mode */
					set_boot_recovery();
					restore_frame();
					return;
					
				case 3: /* Set boot primary kernel image */
					msc_set_boot_partition(0);
					boot_partition = 0;
					selected_option = 0;
					break;
					
				case 4: /* Set boot secondary kernel image */
					msc_set_boot_partition(1);
					boot_partition = 1;
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

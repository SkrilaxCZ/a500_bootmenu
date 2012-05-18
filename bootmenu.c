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

/* Boot menu application main function */

/* 
 * Entry point of bootmenu: 
 * 
 * - keystate at boot is loaded
 * - boot partition (primary or secondary) is loaded
 * - can continue boot, and force fastboot or recovery mode
 * 
 * Modify the BL state for your own purpose, then either
 * return 00 to continue booting or 01 to force reboot.
 */
int main()
{	
	/* Test primitive code */
	clear_screen();
	print_bootlogo();
	
	printf_display("Hello world!\n");
	while (1)
	{ }
	
	return 0;
}

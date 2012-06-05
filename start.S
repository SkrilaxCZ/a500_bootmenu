/* 
 * Acer bootloader boot menu application start file.
 * 
 * Copyright 2012 (C) Skrilax_CZ
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

.text
.thumb

.globl _main
.globl _fastboot_get_var
.globl _fastboot_oem_command
.globl _fastboot_continue
.globl _fastboot_download

#Jumptable
.align(2)
_main:
	b     t_main 

.align(2)
	
_fastboot_get_var:
	b     t_fastboot_get_var
	
.align(2)
	
_fastboot_oem_command:
	b     t_fastboot_oem_command

.align(2)

_fastboot_continue:
	b     t_fastboot_continue

.align(2)

_fastboot_download:
	b     t_fastboot_download
	
#Trampoline
.align(4)
t_main:
	push  {lr}
	bl    main
	pop   {pc}

t_fastboot_get_var:
	push  {lr}
	bl    fastboot_get_var
	pop   {pc}
	
t_fastboot_oem_command:
	push  {lr}
	bl    fastboot_oem_command
	pop   {pc}
	
t_fastboot_continue:
	push  {lr}
	bl    fastboot_continue
	pop   {pc}
	
t_fastboot_download:
	push  {lr}
	bl    fastboot_download
	pop   {pc}


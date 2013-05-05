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
.syntax unified

.global _main
.global _fb_compat_println
.global _fb_compat_println_error
.global _configure_custom_cmdline
.global _debug_write

#Jumptable
.align(2)
_main:
	b.w    main

.align(2)
_fb_compat_println:
	b.w    fb_compat_println

.align(2)
_fb_compat_println_error:
	b.w    fb_compat_println_error

.align(2)
_configure_custom_cmdline:
	b.w    configure_custom_cmdline

.arm

.align(8)
_debug_write:
	b      debug_write

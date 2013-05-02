/*
 * This file maps functions and variables that can be found in Acer 0.03.14-ICS bootloader,
 * and the patched version.
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
 */

#ifndef DEBUG_H
#define DEBUG_H

/* debug text */
extern char debug_text[16384];

/* start pointer */
extern char* debug_start_ptr;

/* end pointer */
extern char* debug_end_ptr;

/* write debug info */
void debug_write(const char* text);

#endif //!DEBUG_H
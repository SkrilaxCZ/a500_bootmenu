/* 
 * Acer bootloader gui defines taken from Android Recovery.
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

#ifndef SKIN_H
#define SKIN_H

/* Required font properties */

#define FONT_HEIGHT            18
#define FONT_WIDTH             10

#define FIRST_CHAR             32
#define NUM_CHARS              96

/* Images */

#define CUSTOM_COLORS_ID       "CLRS"
#define CUSTOM_COLORS_OFFSET   0x19FF00

#define FONT_OFFSET            0x1A0000
#define FONT_SIZE_LIMIT        0x5000

#define BOOTLOGO_OFFSET        0x1A5000
#define BOOTLOGO_SIZE_LIMIT    0x32000

#endif //!SKIN_H
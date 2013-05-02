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

#include "framebuffer.h"

/* Required font properties */
struct font_data
{
	int font_height;
	int font_width;
	int font_outline;
	int font_kerning;
};

/* Bootloader skin structure */
struct bootloader_skin
{
	/* Magic ID */
	char              magic[4];
	
	/* Colors */
	struct font_color title_color;
	struct font_color text_color;
	struct font_color error_text_color;
	struct color      highlight_color;
	struct font_color highlight_text_color;
	
	/* Outlining */
	int               font_outline;
	int               font_kerning;
};

/* Default parameters for outlining */
#define DEFAULT_FONT_OUTLINE             0
#define DEFAULT_FONT_KERNING             0

/* Font data, maximum used in initial malloc */
#define MAXIMUM_FONT_DATA_SIZE           0x80000

#define FIRST_CHAR                       32
#define NUM_CHARS                        96

/* Images */

#define CUSTOM_COLORS_ID                 "CLR2"
#define CUSTOM_COLORS_OFFSET             0x19FF00

#define FONT_OFFSET                      0x1A0000
#define FONT_SIZE_LIMIT                  0x5000

#define BOOTLOGO_OFFSET                  0x1A5000
#define BOOTLOGO_SIZE_LIMIT              0x32000

inline int outlined_font_height(struct font_data* f) 
{ 
	return f->font_height + 2*f->font_outline; 
}

inline int outlined_font_width(struct font_data* f)  
{
	return f->font_width + 2*f->font_outline;
}

inline int font_data_size(struct font_data* f)
{ 
	return outlined_font_width(f) * outlined_font_height(f) * NUM_CHARS * sizeof(struct color); 
}

#endif //!SKIN_H

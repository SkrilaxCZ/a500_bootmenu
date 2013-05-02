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

/* This is compiled in ARM mode unlike the rest.
 * It is a circular buffer to which vprintf stores.
 */

#include <stddef.h>
#include <stdarg.h>
#include <bl_0_03_14.h>
#include <framebuffer.h>

/* 16 kB buffer of debug text */
char debug_text[16384];

/* buffer end pointer */
char* debug_text_end = &(debug_text[16383]);

/* whether we are rotating */
int debug_rotating = 0;

/* start pointer */
char* debug_start_ptr = debug_text;

/* end pointer */
char* debug_end_ptr = debug_text;

/* store debug info */
void debug_write(const char* text)
{
	int len, space;

	len = strlen(text);
	space = ((unsigned int)debug_text_end - (unsigned int)debug_end_ptr);

	if (len > space)
	{
		memcpy(debug_end_ptr, text, space);

		text += space;
		len -= space;
		debug_end_ptr = debug_text;

		if (len > ARRAY_SIZE(debug_text) - 1)
			len = ARRAY_SIZE(debug_text) - 1;

		memcpy(debug_end_ptr, text, len);
		debug_end_ptr += len;
		debug_rotating = 1;
	}
	else
	{
		memcpy(debug_end_ptr, text, len);
		debug_end_ptr += len;
	}

	*debug_end_ptr = '\0';

	if (debug_rotating)
	{
		if (debug_end_ptr == debug_text_end)
			debug_start_ptr = debug_text;
		else
			debug_start_ptr = debug_end_ptr + 1;
	}
}

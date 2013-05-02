/*
 * Acer bootloader boot menu application jpeg handler
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

#ifndef JPEG_H
#define JPEG_H

/* Load jpeg to RGBX */
int jpeg_load_rgbx(uint8_t* output_data, int output_data_size, int* width, int* height,
                   int* image_size, const uint8_t* jpeg_data, int jpeg_data_size);

#endif //!JPEG_H
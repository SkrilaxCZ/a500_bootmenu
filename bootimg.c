 /*
	* This file creates android image in the RAM
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

#include "stdlib.h"
#include "bl_0_03_14.h"
#include "bootimg.h"

#define PAGE_SIZE 2048

int create_android_image(struct boot_img_hdr** bootimg, int* bootimg_size, const char* kernel, int kernel_size, const char* ramdisk, int ramdisk_size)
{
	struct boot_img_hdr hdr;
	char* bootimg_data;
	int padsize;

	/* Check */
	if (!kernel || kernel_size == 0)
		return 1;

	if (!ramdisk)
		ramdisk_size = 0;

	/* Init */
	memset(&hdr, 0, sizeof(hdr));

	/* Magic */
	memcpy(&hdr.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);

	/* Ignore load addresses (bootloader will only care about the second ramdisk address, but we don't use that) */
	hdr.page_size    = PAGE_SIZE;
	hdr.kernel_size  = kernel_size;
	hdr.ramdisk_size = ramdisk_size;

	/* Total size */
	*bootimg_size = ((sizeof(struct boot_img_hdr) + hdr.page_size - 1) / hdr.page_size +
	                (kernel_size + hdr.page_size - 1) / hdr.page_size +
	                (ramdisk_size + hdr.page_size - 1) / hdr.page_size) * hdr.page_size;

	*bootimg = malloc(*bootimg_size);
	bootimg_data = (char*)(*bootimg);

	/* Header */
	memcpy(bootimg_data, (char*)&hdr, sizeof(struct boot_img_hdr));
	bootimg_data += sizeof(struct boot_img_hdr);

	padsize = hdr.page_size - (sizeof(struct boot_img_hdr) & (hdr.page_size - 1));
	if (padsize > 0)
	{
		memset(bootimg_data, 0, padsize);
		bootimg_data += padsize;
	}

	/* Kernel */
	memcpy(bootimg_data, kernel, kernel_size);
	bootimg_data += kernel_size;

	padsize = hdr.page_size - (kernel_size & (hdr.page_size - 1));
	if (padsize > 0)
	{
		memset(bootimg_data, 0, padsize);
		bootimg_data += padsize;
	}

	/* Ramdisk if present */
	if (ramdisk_size > 0)
	{
		memcpy(bootimg_data, ramdisk, ramdisk_size);
		bootimg_data += ramdisk_size;

		padsize = hdr.page_size - (ramdisk_size & (hdr.page_size - 1));
		if (padsize > 0)
		{
			memset(bootimg_data, 0, padsize);
			bootimg_data += padsize;
		}
	}

	return 0;
}

 /*
	* This file generates update.blob from the bootloader binary
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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

/* Copy buffer */
char copy_buffer[4*1024*1024];

/* Blob ID */
char blob_id[] = "MSM-RADIO-UPDATE";

int main(int argc, char** argv)
{
	FILE *bin, *blob;
	uint32_t bin_size, total_size, tmp;
	int i;

	if (argc != 3)
	{
		printf("Usage: %s bootloader.bin bootloader.blob\n", argv[0]);
		return 1;
	}

	/* Stat the bootloader.bin file */
	bin = fopen(argv[1], "r");

	if (bin == NULL)
	{
		fprintf(stderr, "Could not open bootloader bin file.\n");
		return 1;
	}

	/* Stat the bootloader.blob file */
	blob = fopen(argv[2], "w+");

	if (blob == NULL)
	{
		fprintf(stderr, "Could not open bootloader blob file.\n");
		fclose(bin);
		return 1;
	}

	/* Get bootloader.bin file size */
	fseek(bin, 0, SEEK_END);
	bin_size = ftell(bin);
	total_size = bin_size + 0x204C;
	fseek(bin, 0, SEEK_SET);

	/* Write blob header */
	fwrite(blob_id, 1, strlen(blob_id), blob);

	tmp = 0;
	fwrite(&tmp, 1, sizeof(uint32_t), blob);

	fwrite(&total_size, 1, sizeof(uint32_t), blob);

	tmp = 0x203C;
	fwrite(&tmp, 1, sizeof(uint32_t), blob);

	tmp = 1;
	fwrite(&tmp, 1, sizeof(uint32_t), blob);

	/* A bunch of zeroes */
	tmp = 0;

	for (i = 0; i < 0x2010; i += 4)
		fwrite(&tmp, 1, sizeof(uint32_t), blob);

	/* EBT header */
	tmp = 0;

	fwrite(&tmp, 1, sizeof(uint32_t), blob);
	fwrite(&tmp, 1, sizeof(uint32_t), blob);
	fwrite(&tmp, 1, sizeof(uint32_t), blob);

	tmp = 0x544245; /* "EBT" */
	fwrite(&tmp, 1, sizeof(uint32_t), blob);

	tmp = 0x204C;
	fwrite(&tmp, 1, sizeof(uint32_t), blob);

	fwrite(&bin_size, 1, sizeof(uint32_t), blob);

	tmp = 1;
	fwrite(&tmp, 1, sizeof(uint32_t), blob);

	/* EBT binary */
	if (fread(&copy_buffer, 1, ARRAY_SIZE(copy_buffer), bin) != bin_size)
	{
		fprintf(stderr, "Failed writing the update blob file.\n");
		fclose(bin);
		fclose(blob);
		unlink(argv[2]);
		return 1;
	}

	fwrite(&copy_buffer, 1, bin_size, blob);
	fclose(bin);
	fclose(blob);
	return 0;
}
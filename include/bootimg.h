/*
**
** Copyright 2007, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef BOOTIMG_H
#define BOOTIMG_H

typedef struct boot_img_hdr boot_img_hdr;

#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512

struct boot_img_hdr
{
	unsigned char magic[BOOT_MAGIC_SIZE];

	unsigned int kernel_size;  /* size in bytes */
	unsigned int kernel_addr;  /* physical load addr */

	unsigned int ramdisk_size; /* size in bytes */
	unsigned int ramdisk_addr; /* physical load addr */

	unsigned int second_size;  /* size in bytes */
	unsigned int second_addr;  /* physical load addr */

	unsigned int tags_addr;    /* physical addr for kernel tags */
	unsigned int page_size;    /* flash page size we assume */
	unsigned int unused[2];    /* future expansion: should be 0 */

	char name[BOOT_NAME_SIZE]; /* asciiz product name */

	char cmdline[BOOT_ARGS_SIZE];

	unsigned int id[8]; /* timestamp / checksum / sha1 / etc */
};

/*
** +-----------------+
** | boot header     | 1 page
** +-----------------+
** | kernel          | n pages
** +-----------------+
** | ramdisk         | m pages
** +-----------------+
** | second stage    | o pages
** +-----------------+
**
** n = (kernel_size + page_size - 1) / page_size
** m = (ramdisk_size + page_size - 1) / page_size
** o = (second_size + page_size - 1) / page_size
**
** 0. all entities are page_size aligned in flash
** 1. kernel and ramdisk are required (size != 0)
** 2. second is optional (second_size == 0 -> no second)
** 3. load each element (kernel, ramdisk, second) at
**    the specified physical address (kernel_addr, etc)
** 4. prepare tags at tag_addr.  kernel_args[] is
**    appended to the kernel commandline in the tags.
** 5. r0 = 0, r1 = MACHINE_TYPE, r2 = tags_addr
** 6. if second_size != 0: jump to second_addr
**    else: jump to kernel_addr
*/

/* The NVABoot kernel loading code is so awkward so it's easier to generate Android.mk on the fly and pass that as a structure */
int create_android_image(struct boot_img_hdr** bootimg, int* bootimg_size, const char* kernel, int kernel_size, const char* ramdisk, int ramdisk_size);

#endif //!BOOTIMG_H

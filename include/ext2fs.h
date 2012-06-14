/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000, 2001  Free Software Foundation, Inc.
 *
 *  (C) Copyright 2003 Sysgo Real-Time Solutions, AG <www.elinos.com>
 *  Pavel Bartusek <pba@sysgo.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* An implementation for the Ext2FS filesystem ported from GRUB.
 * Some parts of this code (mainly the structures and defines) are
 * from the original ext2 fs code, as found in the linux kernel.
 */

#ifndef EXT2FS_H
#define EXT2FS_H

#define SECTOR_SIZE        0x200
#define SECTOR_BITS        9

extern int ext2fs_ls(const char* dirname);
extern int ext2fs_open(const char* filename);
extern int ext2fs_read(char* buf, uint32_t len);
extern int ext2fs_close(void);
extern int ext2fs_mount(const char* partition);
extern int ext2fs_unmount(void);

#endif //!EXT2FS_H
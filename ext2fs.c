/*
 * (C) Copyright 2004
 *  esd gmbh <www.esd-electronics.com>
 *  Reinhard Arlt <reinhard.arlt@esd-electronics.com>
 *
 *  based on code from grub2 fs/ext2.c and fs/fshelp.c by
 *
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004  Free Software Foundation, Inc.
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

#include <bl_0_03_14.h>
#include <stdlib.h>
#include <stddef.h>
#include <ext2fs.h>
#include <byteorder.h>

/* Magic value used to identify an ext2 filesystem.  */
#define	EXT2_MAGIC             0xEF53
/* Amount of indirect blocks in an inode.  */
#define INDIRECT_BLOCKS        12
/* Maximum lenght of a pathname.  */
#define EXT2_PATH_MAX          4096
/* Maximum nesting of symlinks, used to prevent a loop.  */
#define	EXT2_MAX_SYMLINKCNT    8

/* Filetype used in directory entry.  */
#define	FILETYPE_UNKNOWN       0
#define	FILETYPE_REG           1
#define	FILETYPE_DIRECTORY     2
#define	FILETYPE_SYMLINK       7

/* Filetype information as used in inodes.  */
#define FILETYPE_INO_MASK      0170000
#define FILETYPE_INO_REG       0100000
#define FILETYPE_INO_DIRECTORY 0040000
#define FILETYPE_INO_SYMLINK   0120000

/* Bits used as offset in sector */
#define DISK_SECTOR_BITS       9

/* Log2 size of ext2 block in 512 blocks.  */
#define LOG2_EXT2_BLOCK_SIZE(data) (__le32_to_cpu (data->sblock.log2_block_size) + 1)

/* Log2 size of ext2 block in bytes.  */
#define LOG2_BLOCK_SIZE(data)  (__le32_to_cpu (data->sblock.log2_block_size) + 10)

/* The size of an ext2 block in bytes.  */
#define EXT2_BLOCK_SIZE(data)  (1 << LOG2_BLOCK_SIZE(data))

/* ext4 extension flag */
#define EXT4_EXTENTS_FLAG      0x80000

/* ext4 extension magic */
#define EXT4_EXT_MAGIC         0xf30a

/* The ext2 superblock.  */
struct ext2_sblock
{
	uint32_t total_inodes;
	uint32_t total_blocks;
	uint32_t reserved_blocks;
	uint32_t free_blocks;
	uint32_t free_inodes;
	uint32_t first_data_block;
	uint32_t log2_block_size;
	uint32_t log2_fragment_size;
	uint32_t blocks_per_group;
	uint32_t fragments_per_group;
	uint32_t inodes_per_group;
	uint32_t mtime;
	uint32_t utime;
	uint16_t mnt_count;
	uint16_t max_mnt_count;
	uint16_t magic;
	uint16_t fs_state;
	uint16_t error_handling;
	uint16_t minor_revision_level;
	uint32_t lastcheck;
	uint32_t checkinterval;
	uint32_t creator_os;
	uint32_t revision_level;
	uint16_t uid_reserved;
	uint16_t gid_reserved;
	uint32_t first_inode;
	uint16_t inode_size;
	uint16_t block_group_number;
	uint32_t feature_compatibility;
	uint32_t feature_incompat;
	uint32_t feature_ro_compat;
	uint32_t unique_id[4];
	char volume_name[16];
	char last_mounted_on[64];
	uint32_t compression_info;
};

/* The ext2 blockgroup.  */
struct ext2_block_group
{
	uint32_t block_id;
	uint32_t inode_id;
	uint32_t inode_table_id;
	uint16_t free_blocks;
	uint16_t free_inodes;
	uint16_t used_dir_cnt;
	uint32_t reserved[3];
};

/* The ext2 inode.  */
struct ext2_inode
{
	uint16_t mode;
	uint16_t uid;
	uint32_t size;
	uint32_t atime;
	uint32_t ctime;
	uint32_t mtime;
	uint32_t dtime;
	uint16_t gid;
	uint16_t nlinks;
	uint32_t blockcnt;	/* Blocks of 512 bytes!! */
	uint32_t flags;
	uint32_t osd1;
	union
	{
		struct datablocks
		{
			uint32_t dir_blocks[INDIRECT_BLOCKS];
			uint32_t indir_block;
			uint32_t double_indir_block;
			uint32_t triple_indir_block;
		} blocks;
		char symlink[60];
	} b;
	uint32_t version;
	uint32_t acl;
	uint32_t dir_acl;
	uint32_t fragment_addr;
	uint32_t osd2[3];
};

/* The header of an ext2 directory entry.  */
struct ext2_dirent
{
	uint32_t inode;
	uint16_t direntlen;
	uint8_t namelen;
	uint8_t filetype;
};

struct ext2fs_node
{
	struct ext2_data* data;
	struct ext2_inode inode;
	int ino;
	int inode_read;
};

/* Information about a "mounted" ext2 filesystem.  */
struct ext2_data
{
	struct ext2_sblock sblock;
	struct ext2_inode *inode;
	struct ext2fs_node diropen;
};

/* ext4 extensions */
struct ext4_extent_header
{
	uint16_t magic;
	uint16_t entries;
	uint16_t max;
	uint16_t depth;
	uint32_t generation;
};

struct ext4_extent
{
	uint32_t block;
	uint16_t len;
	uint16_t start_hi;
	uint32_t start;
};

struct ext4_extent_idx
{
	uint32_t block;
	uint32_t leaf;
	uint16_t leaf_hi;
	uint16_t unused;
};

typedef struct ext2fs_node* ext2fs_node_t;
typedef struct ext4_extent_header* ext4_extent_header_t;

static struct ext2_data* ext2fs_root = NULL;
static ext2fs_node_t ext2fs_file = NULL;
static int ext2fs_pos = 0;
static int symlinknest = 0;
static unsigned int inode_size;
static int ext_pt_handle = -1;
static uint64_t pt_size;
static char gets_buffer[1024];
static char* gets_buffer_ptr = gets_buffer;

static int ext2fs_devread(uint64_t sector, int byte_offset, int byte_len, char *buf)
{
	uint32_t processed_bytes;

	if ((sector < 0) || (sector * SECTOR_SIZE) + (byte_offset + byte_len - 1) >= pt_size)
	{
		printf(" ** ext2fs_devread() read outside partition sector %d\n", sector);
		return 1;
	}

	/*
	 * Set position
	 */
	if (set_partition_position(ext_pt_handle, (sector * SECTOR_SIZE) + byte_offset, PARTITION_SETPOS_ABSOLUTE))
		return 1;

	/*
	 * Read it
	 */
	if (read_partition(ext_pt_handle, buf, byte_len, &processed_bytes) || (processed_bytes != (uint32_t)byte_len))
		return 1;

	return 0;
}

static int ext2fs_blockgroup(struct ext2_data* data, int group, struct ext2_block_group* blkgrp)
{
	uint64_t blkno;
	uint32_t blkoff;
	uint32_t desc_per_blk;

	desc_per_blk = EXT2_BLOCK_SIZE(data) / sizeof(struct ext2_block_group);
	blkno = __le32_to_cpu(data->sblock.first_data_block) + 1 + (group / desc_per_blk);
	blkoff = (group % desc_per_blk) * sizeof(struct ext2_block_group);
	return ext2fs_devread(blkno << LOG2_EXT2_BLOCK_SIZE(data), blkoff, sizeof(struct ext2_block_group), (char*)blkgrp);
}

static int ext2fs_read_inode(struct ext2_data* data, int ino, struct ext2_inode* inode)
{
	struct ext2_block_group blkgrp;
	struct ext2_sblock* sblock = &data->sblock;
	int inodes_per_block;
	int status;

	uint64_t blkno;
	uint32_t blkoff;

	/* It is easier to calculate if the first inode is 0.  */
	ino--;
	status = ext2fs_blockgroup(data, ino / __le32_to_cpu(sblock->inodes_per_group), &blkgrp);

	if (status)
		return 1;

	inodes_per_block = EXT2_BLOCK_SIZE(data) / inode_size;

	blkno = __le32_to_cpu(blkgrp.inode_table_id) + (ino % __le32_to_cpu(sblock->inodes_per_group)) / inodes_per_block;
	blkoff = (ino % inodes_per_block) * inode_size;

	/* Read the inode.  */
	status = ext2fs_devread(blkno << LOG2_EXT2_BLOCK_SIZE(data), blkoff, sizeof(struct ext2_inode), (char*) inode);
	if (status)
		return 1;

	return 0;
}

static void ext2fs_free_node(ext2fs_node_t node, ext2fs_node_t currroot)
{
	if ((node != &ext2fs_root->diropen) && (node != currroot))
		free(node);
}

static ext4_extent_header_t ext4_find_leaf(struct ext2_data* data, char* buf, ext4_extent_header_t ext_block, uint32_t fileblock)
{
	struct ext4_extent_idx* index;

	while (1)
	{
		int i;
		uint64_t block;

		index = (struct ext4_extent_idx*)(ext_block + 1);

		if (__le16_to_cpu(ext_block->magic) != EXT4_EXT_MAGIC)
			return NULL;

		if (ext_block->depth == 0)
			return ext_block;

		for (i = 0; i < __le16_to_cpu(ext_block->entries); i++)
		{
			if (fileblock < __le32_to_cpu(index[i].block))
				break;
		}

		if (--i < 0)
			return NULL;

		block = __le16_to_cpu(index[i].leaf_hi);
		block = (block << 32) + __le32_to_cpu(index[i].leaf);
		block = block << LOG2_EXT2_BLOCK_SIZE(data);
		if (ext2fs_devread(block, 0, EXT2_BLOCK_SIZE(data), buf))
			return NULL;

		ext_block = (ext4_extent_header_t)buf;
	}
}

static uint64_t ext2fs_read_block(ext2fs_node_t node, uint64_t fileblock)
{
	struct ext2_data* data = node->data;
	struct ext2_inode* inode = &node->inode;
	uint64_t blknr;
	int blksz = EXT2_BLOCK_SIZE(data);
	int log2_blksz = LOG2_EXT2_BLOCK_SIZE(data);
	int status;

	/* Ext4 extension */
	if (__le32_to_cpu(inode->flags) & EXT4_EXTENTS_FLAG)
	{
		char buf[EXT2_BLOCK_SIZE(data)];
		ext4_extent_header_t leaf;
		struct ext4_extent* ext;
		int i;

		leaf = ext4_find_leaf(data, buf, (ext4_extent_header_t)inode->b.blocks.dir_blocks, fileblock);
		if (!leaf)
			return -1;

		ext = (struct ext4_extent*)(leaf + 1);
		for (i = 0; i < __le16_to_cpu(leaf->entries); i++)
		{
			if (fileblock < __le32_to_cpu(ext[i].block))
				break;
		}

		if (--i >= 0)
		{
			fileblock -= __le32_to_cpu(ext[i].block);
			if (fileblock >= __le16_to_cpu(ext[i].len))
				return 0;
			else
			{
				uint64_t start;

				start = __le16_to_cpu(ext[i].start_hi);
				start = (start << 32) + __le32_to_cpu(ext[i].start);

				return fileblock + start;
			}
		}
		else
			return -1;
	}

	/* Direct blocks.  */
	if (fileblock < INDIRECT_BLOCKS)
		blknr = __le32_to_cpu(inode->b.blocks.dir_blocks[fileblock]);
	/* Indirect.  */
	else if (fileblock < (INDIRECT_BLOCKS + (blksz / 4)))
	{
		uint32_t indir[blksz / 4];

		status = ext2fs_devread(__le32_to_cpu(inode->b.blocks.indir_block) << log2_blksz, 0, blksz, (char*)indir);
		if (status)
		{
			printf("** ext2fs read block (indir 1) failed. **\n");
			return -1;
		}
		blknr = __le32_to_cpu(indir[fileblock - INDIRECT_BLOCKS]);
	}
	/* Double indirect.  */
	else if (fileblock < (INDIRECT_BLOCKS + (blksz / 4 * (blksz / 4 + 1))))
	{
		uint32_t perblock = blksz / 4;
		uint32_t rblock = fileblock - (INDIRECT_BLOCKS  + blksz / 4);
		uint32_t indir[blksz / 4];

		status = ext2fs_devread(__le32_to_cpu(inode->b.blocks.double_indir_block) << log2_blksz, 0, blksz, (char*)indir);
		if (status)
		{
			printf("** ext2fs read block (indir 2 1) failed. **\n");
			return -1;
		}

		status = ext2fs_devread(__le32_to_cpu(indir[rblock / perblock]) << log2_blksz, 0, blksz, (char*)indir);
		if (status)
		{
			printf("** ext2fs read block (indir 2 2) failed. **\n");
			return -1;
		}

		blknr = __le32_to_cpu(indir[rblock % perblock]);
	}
	/* Tripple indirect.  */
	else if (fileblock < INDIRECT_BLOCKS + blksz / 4 * (blksz / 4 + 1) + (blksz / 4) * (blksz / 4) * (blksz / 4 + 1))
	{
		uint32_t perblock = blksz / 4;
		uint32_t rblock = fileblock - (INDIRECT_BLOCKS + blksz / 4 * (blksz / 4 + 1));
		uint32_t indir[blksz / 4];

		status = ext2fs_devread(__le32_to_cpu(inode->b.blocks.triple_indir_block) << log2_blksz, 0, blksz, (char*)indir);
		if (status)
		{
			printf("** ext2fs read block (indir 3 1) failed. **\n");
			return -1;
		}

		status = ext2fs_devread(__le32_to_cpu(indir[rblock / perblock]) << log2_blksz, 0, blksz, (char*)indir);
		if (status)
		{
			printf("** ext2fs read block (indir 3 2) failed. **\n");
			return -1;
		}

		status = ext2fs_devread(__le32_to_cpu(indir[rblock / perblock]) << log2_blksz, 0, blksz, (char*)indir);
		if (status)
		{
			printf("** ext2fs read block (indir 3 3) failed. **\n");
			return -1;
		}

		blknr = __le32_to_cpu(indir[rblock % perblock]);
	}
	else
	{
		printf("** ext2fs doesn't support quadruple indirect blocks. **\n");
		return -1;
	}

	return blknr;
}

int ext2fs_read_file(ext2fs_node_t node, int pos, unsigned int len, char* buf)
{
	uint64_t i;
	int blockcnt;
	int log2blocksize = LOG2_EXT2_BLOCK_SIZE(node->data);
	int blocksize = 1 << (log2blocksize + DISK_SECTOR_BITS);
	unsigned int filesize = __le32_to_cpu(node->inode.size);

	/* Adjust len so it we can't read past the end of the file. */
	if (pos + len > filesize)
		len = filesize - pos;

	if (len == 0)
		return 0;

	blockcnt = ((len + pos) + blocksize - 1) / blocksize;

	for (i = pos / blocksize; i < blockcnt; i++)
	{
		uint64_t blknr;
		uint32_t blockoff = pos % blocksize;
		uint32_t blockend = blocksize;
		uint32_t skipfirst = 0;

		blknr = ext2fs_read_block(node, i);
		if (blknr < 0)
			return -1;

		blknr = blknr << log2blocksize;

		/* Last block.  */
		if (i == blockcnt - 1)
		{
			blockend = (len + pos) % blocksize;

			/* The last portion is exactly blocksize.  */
			if (!blockend)
				blockend = blocksize;
		}

		/* First block.  */
		if (i == pos / blocksize)
		{
			skipfirst = blockoff;
			blockend -= skipfirst;
		}

		/* If the block number is 0 this block is not stored on disk but
		   is zero filled instead.  */
		if (blknr)
		{
			int status;

			status = ext2fs_devread(blknr, skipfirst, blockend, buf);
			if (status)
				return -1;
		}
		else
			memset(buf, 0, blocksize - skipfirst);

		buf += blocksize - skipfirst;
	}
	return len;
}

static int ext2fs_iterate_dir(ext2fs_node_t dir, char* name, ext2fs_node_t* fnode, int* ftype)
{
	unsigned int fpos = 0;
	int status;
	struct ext2fs_node* diro = (struct ext2fs_node*) dir;

	if(!diro->inode_read)
	{
		status = ext2fs_read_inode(diro->data, diro->ino, &diro->inode);
		if (status)
			return 1;
	}

	/* Search the file.  */
	while(fpos < __le32_to_cpu(diro->inode.size))
	{
		struct ext2_dirent dirent;

		status = ext2fs_read_file(diro, fpos, sizeof(struct ext2_dirent), (char*) &dirent);
		if (status < 1)
			return 1;


		if (dirent.direntlen == 0)
			return 1;

		if (dirent.namelen != 0)
		{
			char filename[dirent.namelen + 1];
			ext2fs_node_t fdiro;
			int type = FILETYPE_UNKNOWN;

			status = ext2fs_read_file(diro, fpos + sizeof(struct ext2_dirent), dirent.namelen, filename);
			if (status < 1)
				return 1;

			fdiro = malloc(sizeof(struct ext2fs_node));
			if (!fdiro)
				return 1;

			fdiro->data = diro->data;
			fdiro->ino = __le32_to_cpu(dirent.inode);

			filename[dirent.namelen] = '\0';

			if (dirent.filetype != FILETYPE_UNKNOWN)
			{
				fdiro->inode_read = 0;

				if (dirent.filetype == FILETYPE_DIRECTORY)
					type = FILETYPE_DIRECTORY;
				else if (dirent.filetype == FILETYPE_SYMLINK)
					type = FILETYPE_SYMLINK;
				else if (dirent.filetype == FILETYPE_REG)
					type = FILETYPE_REG;
			}
			else
			{
				/* The filetype can not be read from the dirent, get it from inode */
				status = ext2fs_read_inode(diro->data,  __le32_to_cpu(dirent.inode), &fdiro->inode);
				if (status)
				{
					free(fdiro);
					return 1;
				}
				fdiro->inode_read = 1;

				if ((__le16_to_cpu(fdiro->inode.mode) & FILETYPE_INO_MASK) == FILETYPE_INO_DIRECTORY)
					type = FILETYPE_DIRECTORY;
				else if ((__le16_to_cpu(fdiro->inode.mode) & FILETYPE_INO_MASK) == FILETYPE_INO_SYMLINK)
					type = FILETYPE_SYMLINK;
				else if ((__le16_to_cpu(fdiro->inode.mode) & FILETYPE_INO_MASK) == FILETYPE_INO_REG)
					type = FILETYPE_REG;
			}

			if ((name != NULL) && (fnode != NULL) && (ftype != NULL))
			{
				if (strcmp(filename, name) == 0)
				{
					*ftype = type;
					*fnode = fdiro;
					return 0;
				}
			}
			else
			{
				if (fdiro->inode_read == 0)
				{
					status = ext2fs_read_inode(diro->data,  __le32_to_cpu(dirent.inode), &fdiro->inode);
					if (status)
					{
						free(fdiro);
						return 1;
					}
					fdiro->inode_read = 1;
				}
			}
			free(fdiro);
		}
		fpos += __le16_to_cpu(dirent.direntlen);
	}
	return 1;
}

static char* ext2fs_read_symlink(ext2fs_node_t node)
{
	char* symlink;
	struct ext2fs_node* diro = node;
	int status;

	if (!diro->inode_read)
	{
		status = ext2fs_read_inode(diro->data, diro->ino, &diro->inode);

		if (status)
			return NULL;
	}
	symlink = malloc(__le32_to_cpu(diro->inode.size) + 1);
	if (!symlink)
		return NULL;

	/* If the filesize of the symlink is bigger than
	   60 the symlink is stored in a separate block,
	   otherwise it is stored in the inode.  */
	if (__le32_to_cpu(diro->inode.size) <= 60)
	{
		strncpy(symlink, diro->inode.b.symlink, __le32_to_cpu(diro->inode.size));
		symlink[__le32_to_cpu(diro->inode.size) - 1] = '\0';
	}
	else
	{
		status = ext2fs_read_file(diro, 0, __le32_to_cpu(diro->inode.size), symlink);
		if (status)
		{
			free(symlink);
			return NULL;
		}
	}
	symlink[__le32_to_cpu(diro->inode.size)] = '\0';
	return symlink;
}


int ext2fs_find_file1(const char* currpath, ext2fs_node_t currroot, ext2fs_node_t* currfound, int* foundtype)
{
	char fpath[strlen(currpath) + 1];
	char* name = fpath;
	char* next;
	int status;
	int type = FILETYPE_DIRECTORY;
	ext2fs_node_t currnode = currroot;
	ext2fs_node_t oldnode = currroot;

	strncpy(fpath, currpath, ARRAY_SIZE(fpath));
	fpath[ARRAY_SIZE(fpath) - 1] ='\0';

	/* Remove all leading slashes.  */
	while (*name == '/')
		name++;

	if (!*name)
	{
		*currfound = currnode;
		return 1;
	}

	for (;;)
	{
		int found;

		/* Extract the actual part from the pathname.  */
		next = strchr(name, '/');
		if (next)
		{
			/* Remove all leading slashes.  */
			while (*next == '/')
				*(next++) = '\0';
		}

		/* At this point it is expected that the current node is a directory, check if this is true.  */
		if (type != FILETYPE_DIRECTORY)
		{
			ext2fs_free_node(currnode, currroot);
			return 1;
		}

		oldnode = currnode;

		/* Iterate over the directory.  */
		found = ext2fs_iterate_dir(currnode, name, &currnode, &type);
		if (found == 1)
			return 1;

		if (found == -1)
			break;

		/* Read in the symlink and follow it.  */
		if (type == FILETYPE_SYMLINK)
		{
			char* symlink;

			/* Test if the symlink does not loop.  */
			if (++symlinknest == 8)
			{
				ext2fs_free_node(currnode, currroot);
				ext2fs_free_node(oldnode, currroot);
				return 1;
			}

			symlink = ext2fs_read_symlink(currnode);
			ext2fs_free_node(currnode, currroot);

			if (!symlink)
			{
				ext2fs_free_node(oldnode, currroot);
				return 1;
			}

			/* The symlink is an absolute path, go back to the root inode.  */
			if (symlink[0] == '/')
			{
				ext2fs_free_node(oldnode, currroot);
				oldnode = &ext2fs_root->diropen;
			}

			/* Lookup the node the symlink points to.  */
			status = ext2fs_find_file1(symlink, oldnode, &currnode, &type);

			free(symlink);

			if (status)
			{
				ext2fs_free_node(oldnode, currroot);
				return 1;
			}
		}

		ext2fs_free_node(oldnode, currroot);

		/* Found the node!  */
		if (!next || *next == '\0')
		{
			*currfound = currnode;
			*foundtype = type;
			return 0;
		}
		name = next;
	}

	return -1;
}


int ext2fs_find_file(const char* path, ext2fs_node_t rootnode, ext2fs_node_t* foundnode, int expecttype)
{
	int status;
	int foundtype = FILETYPE_DIRECTORY;

	symlinknest = 0;
	if (!path)
		return 1;

	status = ext2fs_find_file1(path, rootnode, foundnode, &foundtype);
	if (status)
		return 1;

	/* Check if the node that was found was of the expected type.  */
	if ((expecttype == FILETYPE_REG) && (foundtype != expecttype))
		return 1;
	else if ((expecttype == FILETYPE_DIRECTORY)  && (foundtype != expecttype))
		return 1;

	return 0;
}


int ext2fs_ls(const char* dirname)
{
	ext2fs_node_t dirnode;
	int status;

	if (ext2fs_root == NULL)
		return 1;

	status = ext2fs_find_file(dirname, &ext2fs_root->diropen, &dirnode, FILETYPE_DIRECTORY);

	if (status)
	{
		printf("** Can not find directory. **\n");
		return 1;
	}

	ext2fs_iterate_dir(dirnode, NULL, NULL, NULL);
	ext2fs_free_node(dirnode, &ext2fs_root->diropen);
	return 0;
}

int ext2fs_open(const char* filename)
{
	ext2fs_node_t fdiro = NULL;
	int status;
	int len;

	if (ext2fs_root == NULL)
		return -1;

	ext2fs_file = NULL;
	status = ext2fs_find_file(filename, &ext2fs_root->diropen, &fdiro, FILETYPE_REG);
	if (status)
		goto fail;

	if (!fdiro->inode_read)
	{
		status = ext2fs_read_inode(fdiro->data, fdiro->ino, &fdiro->inode);
		if (status)
			goto fail;
	}

	len = __le32_to_cpu(fdiro->inode.size);
	ext2fs_file = fdiro;
	ext2fs_pos = 0;
	gets_buffer[0] = '\0';
	gets_buffer_ptr = gets_buffer;
	return len;

fail:
	ext2fs_free_node(fdiro, &ext2fs_root->diropen);
	return -1;
}

int ext2fs_close(void)
{
	if ((ext2fs_file != NULL) && (ext2fs_root != NULL))
	{
		ext2fs_free_node(ext2fs_file, &ext2fs_root->diropen);
		ext2fs_file = NULL;
	}

	if (ext2fs_root != NULL)
	{
		free(ext2fs_root);
		ext2fs_root = NULL;
	}

	return 0;
}

int ext2fs_read(char* buf, unsigned int len)
{
	int status;

	if (ext2fs_root == NULL)
		return 1;

	if (ext2fs_file == NULL)
		return 1;

	status = ext2fs_read_file(ext2fs_file, ext2fs_pos, len, buf);
	ext2fs_pos += status;
	return status;
}

int ext2fs_seek(int pos)
{
	if (ext2fs_root == NULL)
		return 1;

	if (ext2fs_file == NULL)
		return 1;

	if (pos < 0 && pos > __le32_to_cpu(ext2fs_file->inode.size))
		return 1;

	ext2fs_pos = pos;
	return 0;
}

int ext2fs_gets(char* buf, int bufsize)
{
	char* chr = strchr(gets_buffer_ptr, '\n');
	int l, ret, acc;

	acc = 0;
	buf[0] = '\0';

	while (!chr)
	{
		l = strlen(gets_buffer_ptr);

		if (l >= (bufsize - 1))
		{
			strncpy(&(buf[acc]), gets_buffer_ptr, (bufsize - 1));
			buf[acc + (bufsize - 1)] = '\0';
			gets_buffer_ptr += (bufsize - 1);
			return acc + (bufsize - 1);
		}
		else if (l != 0)
		{
			strncpy(&(buf[acc]), gets_buffer_ptr, l);
			buf[acc + l] = '\0';
			bufsize -= l;
			acc += l;
		}

		ret = ext2fs_read(gets_buffer, ARRAY_SIZE(gets_buffer) - 1);

		if (ret > 0)
		{
			gets_buffer[ret] = '\0';
			gets_buffer_ptr = gets_buffer;
		}
		else
		{
			/* EOF, return */
			gets_buffer[0] = '\0';
			gets_buffer_ptr = gets_buffer;
			return acc;
		}

		chr = strchr(gets_buffer_ptr, '\n');
	}

	/* i now is pointing to some '\n' */
	l = ((int)(chr - gets_buffer_ptr)) / sizeof(char);
	if (l + 1 >= (bufsize - 1))
	{
		strncpy(&(buf[acc]), gets_buffer_ptr, (bufsize - 1));
		buf[acc + (bufsize - 1)] = '\0';
		gets_buffer_ptr += (bufsize - 1);
		return acc + (bufsize - 1);
	}

	strncpy(&(buf[acc]), gets_buffer_ptr, l + 1);
	buf[acc + (l + 1)] = '\0';
	gets_buffer_ptr += l + 1;
	return acc + (l + 1);
}

int ext2fs_mount(const char* partition)
{
	struct ext2_data* data;
	int status;

	/* Open the partition */
	if (ext_pt_handle != -1)
		return 1;

	data = malloc(sizeof(struct ext2_data));
	if (!data)
		return 1;

	status = open_partition(partition, PARTITION_OPEN_READ, &ext_pt_handle);
	if (status)
		goto fail;

	/* Get partition size */
	if (get_partition_size(partition, &pt_size))
		goto fail;

	/* Read the superblock.  */
	status = ext2fs_devread(1 * 2, 0, sizeof(struct ext2_sblock), (char*) &data->sblock);
	if (status)
		goto fail;

	/* Make sure this is an ext2 filesystem.  */
	if (__le16_to_cpu(data->sblock.magic) != EXT2_MAGIC)
		goto fail;

	if (__le32_to_cpu(data->sblock.revision_level == 0))
		inode_size = 128;
	else
		inode_size = __le16_to_cpu(data->sblock.inode_size);

	data->diropen.data = data;
	data->diropen.ino = 2;
	data->diropen.inode_read = 1;
	data->inode = &data->diropen.inode;

	status = ext2fs_read_inode(data, 2, data->inode);
	if (status)
		goto fail;

	ext2fs_root = data;
	return 0;

fail:
	printf("Failed to mount ext2 filesystem...\n");
	free(data);
	ext2fs_root = NULL;
	return 1;
}

int ext2fs_unmount(void)
{
	ext2fs_close();

	if (ext_pt_handle != -1)
	{
		close_partition(ext_pt_handle);
		ext_pt_handle = -1;
		return 0;
	}

	return 1;
}

int ext2fs_loadfile(char** data, int* size, const char* path)
{
	char partition[8];
	char* ptr;
	int len;

	ptr = strchr(path, ':');
	len = ((int)(ptr - path));

	if (ptr == NULL || len > 4)
		return 1;

	strncpy(partition, path, len);
	partition[len] = '\0';
	ptr++;

	if (ext2fs_mount(partition))
		return 1;

	len = ext2fs_open(ptr);
	if (len <= 0)
		goto fail;

	*data = malloc(len);
	*size = len;

	ext2fs_read(*data, len);
	ext2fs_close();
	ext2fs_unmount();
	return 0;

fail:
	ext2fs_unmount();
	return 1;
}

/*
 * Acer bootloader, boot menu application jpeg loader, based on GRUB jpeg loader
 * Modifications by Skrilax_CZ 
 * 
 * GRUB  --  GRand Unified Bootloader
 * Copyright (C) 2008  Free Software Foundation, Inc.
 *
 * GRUB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GRUB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stddef.h>
#include <bl_0_03_14.h>
#include <jpeg.h>
#include <byteorder.h>

#define JPEG_ESC_CHAR	    0xFF

#define JPEG_SAMPLING_1x1 0x11

#define JPEG_MARKER_SOI   0xd8
#define JPEG_MARKER_EOI   0xd9
#define JPEG_MARKER_DHT   0xc4
#define JPEG_MARKER_DQT   0xdb
#define JPEG_MARKER_SOF0  0xc0
#define JPEG_MARKER_SOS   0xda

#define SHIFT_BITS        8
#define CONST(x)          ((int) ((x) * (1L << SHIFT_BITS) + 0.5))

#define JPEG_UNIT_SIZE		8

static const uint8_t jpeg_zigzag_order[] = 
{
	0, 1, 8, 16, 9, 2, 3, 10,
	17, 24, 32, 25, 18, 11, 4, 5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13, 6, 7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63
};

typedef int jpeg_data_unit_t[64];

struct jpeg_data
{
	const uint8_t* jpeg_data;
	const uint8_t* ptr;
	int jpeg_data_size;
	
	uint8_t* output_data;
	int output_data_size;
	
	int image_width;
	int image_height;
	
	uint8_t *huff_value[4];
	int huff_offset[4][16];
	int huff_maxval[4][16];
	
	uint8_t quan_table[2][64];
	int comp_index[3][3];
	
	jpeg_data_unit_t ydu[4];
	jpeg_data_unit_t crdu;
	jpeg_data_unit_t cbdu;
	
	int vs, hs;
	int dc_value[3];
	int bit_mask, bit_save;
};

static int jpeg_get_offset(struct jpeg_data* data)
{
	return (int)(data->ptr) - (int)(data->jpeg_data);
}

static uint8_t jpeg_get_byte(struct jpeg_data* data)
{
	uint8_t r;
	
	if (jpeg_get_offset(data) >= data->jpeg_data_size)
		return 0xFF;
	
	r = *(data->ptr);
	data->ptr++;
	
	return r;
}

static uint16_t jpeg_get_word(struct jpeg_data* data)
{
	uint16_t r;
	
	if (jpeg_get_offset(data) >= data->jpeg_data_size - 1)
		return 0xFFFF;
	
	/* NOTE: casting data->ptr to uint16_t directly caused lockup for unknown reason */
	
	r = *(data->ptr);
	data->ptr++;
	
	r += (uint16_t)(*(data->ptr) << 8);
	data->ptr++;
	
	return __be16_to_cpu(r);
}

static int jpeg_get_bit(struct jpeg_data* data)
{
	int ret;
	
	if (data->bit_mask == 0)
	{
		data->bit_save = jpeg_get_byte(data);
		if (data->bit_save == JPEG_ESC_CHAR)
		{
			if (jpeg_get_byte(data) != 0)
				return 0;
		}
		
		data->bit_mask = 0x80;
	}
	
	ret = ((data->bit_save & data->bit_mask) != 0);
	data->bit_mask >>= 1;
	return ret;
}

static int jpeg_get_number(struct jpeg_data* data, int num)
{
	int value, i, msb;
	
	if (num == 0)
		return 0;
	
	msb = value = jpeg_get_bit(data);
	
	for (i = 1; i < num; i++)
		value = (value << 1) + (jpeg_get_bit(data) != 0);
	
	if (!msb)
		value += 1 - (1 << num);
	
	return value;
}

static int jpeg_get_huff_code(struct jpeg_data* data, int id)
{
	int code;
	unsigned i;
	
	code = 0;
	for (i = 0; i < ARRAY_SIZE(data->huff_maxval[id]); i++)
	{
		code <<= 1;
		if (jpeg_get_bit (data))
			code++;
		
		if (code < data->huff_maxval[id][i])
			return data->huff_value[id][code + data->huff_offset[id][i]];
	}
	
	return 0;
}

static int jpeg_decode_huff_table(struct jpeg_data* data)
{
	int id, ac, n, base, ofs;
	uint32_t next_marker;
	uint8_t count[16];
	unsigned i;
	
	next_marker = jpeg_get_offset(data);
	next_marker += jpeg_get_word(data);
	
	while (jpeg_get_offset(data) + sizeof(count) + 1 <= next_marker)
	{
		id = jpeg_get_byte(data);
		ac = (id >> 4) & 1;
		id &= 0xF;
		
		if (id > 1)
			return 1;
		
		if (jpeg_get_offset(data) + sizeof(count) >= data->jpeg_data_size)
			return 1;
		
		memcpy(count, data->ptr, sizeof(count));
		data->ptr += sizeof(count);
		
		n = 0;
		for (i = 0; i < ARRAY_SIZE(count); i++)
			n += count[i];
		
		id += ac * 2;
		data->huff_value[id] = malloc(n);
		
		if (data->huff_value[id] == NULL)
			return 1;
		
		if (jpeg_get_offset(data) + n >= data->jpeg_data_size)
			return 1;
		
		memcpy(data->huff_value[id], data->ptr, n);
		data->ptr += n;
		
		base = 0;
		ofs = 0;
		for (i = 0; i < ARRAY_SIZE(count); i++)
		{
			base += count[i];
			ofs += count[i];
			
			data->huff_maxval[id][i] = base;
			data->huff_offset[id][i] = ofs - base;
			
			base <<= 1;
		}
	}
	
	if (jpeg_get_offset(data) != next_marker)
		return 1;
	
	return 0;
}

static int jpeg_decode_quan_table(struct jpeg_data* data)
{
	int id;
	uint32_t next_marker;
	
	next_marker = jpeg_get_offset(data);
	next_marker += jpeg_get_word(data);
	
	while (jpeg_get_offset(data) + sizeof(data->quan_table[id]) + 1 <= next_marker)
	{		
		id = jpeg_get_byte(data);
		if (id >= 0x10)    /* Upper 4-bit is precision.  */
			return 1;
		
		if (id > 1)
			return 1;
		
		if (jpeg_get_offset(data) + sizeof(data->quan_table[id]) >= data->jpeg_data_size)
			return 1;
		
		memcpy(data->quan_table[id], data->ptr, sizeof(data->quan_table[id]));
		data->ptr += sizeof(data->quan_table[id]);
	}
	
	if (jpeg_get_offset(data) != next_marker)
		return 1;

	return 0;
}

static int jpeg_decode_sof(struct jpeg_data* data)
{
	int i, cc;
	uint32_t next_marker;
	
	next_marker = jpeg_get_offset(data);
	next_marker += jpeg_get_word(data);
	
	if (jpeg_get_byte(data) != 8)
		return 1;
	
	data->image_height = jpeg_get_word(data);
	data->image_width = jpeg_get_word(data);
	
	if ((!data->image_height) || (!data->image_width))
		return 1;
	
	cc = jpeg_get_byte(data);
	if (cc != 3)
		return 1;
	
	for (i = 0; i < cc; i++)
	{
		int id, ss;
		
		id = jpeg_get_byte(data) - 1;
		if ((id < 0) || (id >= 3))
			return 1;
		
		ss = jpeg_get_byte(data);	/* Sampling factor.  */
		
		if (!id)
		{
			data->vs = ss & 0xF;	/* Vertical sampling.  */
			data->hs = ss >> 4;	/* Horizontal sampling.  */
			
			if ((data->vs > 2) || (data->hs > 2))
				return 1;
		}
		else if (ss != JPEG_SAMPLING_1x1)
			return 1;
		
		data->comp_index[id][0] = jpeg_get_byte(data);
	}
	
	if (jpeg_get_offset(data) != next_marker)
		return 1;
	
	return 0;
}

static void jpeg_idct_transform(jpeg_data_unit_t du)
{
	int *pd;
	int i;
	int t0, t1, t2, t3, t4, t5, t6, t7;
	int v0, v1, v2, v3, v4;
	
	pd = du;
	for (i = 0; i < JPEG_UNIT_SIZE; i++, pd++)
	{
		if ((pd[JPEG_UNIT_SIZE * 1] | pd[JPEG_UNIT_SIZE * 2] |
			pd[JPEG_UNIT_SIZE * 3] | pd[JPEG_UNIT_SIZE * 4] |
			pd[JPEG_UNIT_SIZE * 5] | pd[JPEG_UNIT_SIZE * 6] |
			pd[JPEG_UNIT_SIZE * 7]) == 0)
		{
			pd[JPEG_UNIT_SIZE * 0] <<= SHIFT_BITS;
			
			pd[JPEG_UNIT_SIZE * 1] = pd[JPEG_UNIT_SIZE * 2]
			= pd[JPEG_UNIT_SIZE * 3] = pd[JPEG_UNIT_SIZE * 4]
			= pd[JPEG_UNIT_SIZE * 5] = pd[JPEG_UNIT_SIZE * 6]
			= pd[JPEG_UNIT_SIZE * 7] = pd[JPEG_UNIT_SIZE * 0];
			
			continue;
		}
		
		t0 = pd[JPEG_UNIT_SIZE * 0];
		t1 = pd[JPEG_UNIT_SIZE * 2];
		t2 = pd[JPEG_UNIT_SIZE * 4];
		t3 = pd[JPEG_UNIT_SIZE * 6];
		
		v4 = (t1 + t3) * CONST(0.541196100);
		
		v0 = ((t0 + t2) << SHIFT_BITS);
		v1 = ((t0 - t2) << SHIFT_BITS);
		v2 = v4 - t3 * CONST(1.847759065);
		v3 = v4 + t1 * CONST(0.765366865);
		
		t0 = v0 + v3;
		t3 = v0 - v3;
		t1 = v1 + v2;
		t2 = v1 - v2;
		
		t4 = pd[JPEG_UNIT_SIZE * 7];
		t5 = pd[JPEG_UNIT_SIZE * 5];
		t6 = pd[JPEG_UNIT_SIZE * 3];
		t7 = pd[JPEG_UNIT_SIZE * 1];
		
		v0 = t4 + t7;
		v1 = t5 + t6;
		v2 = t4 + t6;
		v3 = t5 + t7;
		
		v4 = (v2 + v3) * CONST(1.175875602);
		
		v0 *= CONST(0.899976223);
		v1 *= CONST(2.562915447);
		v2 = v2 * CONST(1.961570560) - v4;
		v3 = v3 * CONST(0.390180644) - v4;
		
		t4 = t4 * CONST(0.298631336) - v0 - v2;
		t5 = t5 * CONST(2.053119869) - v1 - v3;
		t6 = t6 * CONST(3.072711026) - v1 - v2;
		t7 = t7 * CONST(1.501321110) - v0 - v3;
		
		pd[JPEG_UNIT_SIZE * 0] = t0 + t7;
		pd[JPEG_UNIT_SIZE * 7] = t0 - t7;
		pd[JPEG_UNIT_SIZE * 1] = t1 + t6;
		pd[JPEG_UNIT_SIZE * 6] = t1 - t6;
		pd[JPEG_UNIT_SIZE * 2] = t2 + t5;
		pd[JPEG_UNIT_SIZE * 5] = t2 - t5;
		pd[JPEG_UNIT_SIZE * 3] = t3 + t4;
		pd[JPEG_UNIT_SIZE * 4] = t3 - t4;
	}
	
	pd = du;
	for (i = 0; i < JPEG_UNIT_SIZE; i++, pd += JPEG_UNIT_SIZE)
	{
		if ((pd[1] | pd[2] | pd[3] | pd[4] | pd[5] | pd[6] | pd[7]) == 0)
		{
			pd[0] >>= (SHIFT_BITS + 3);
			pd[1] = pd[2] = pd[3] = pd[4] = pd[5] = pd[6] = pd[7] = pd[0];
			continue;
		}
		
		v4 = (pd[2] + pd[6]) * CONST(0.541196100);
		
		v0 = (pd[0] + pd[4]) << SHIFT_BITS;
		v1 = (pd[0] - pd[4]) << SHIFT_BITS;
		v2 = v4 - pd[6] * CONST(1.847759065);
		v3 = v4 + pd[2] * CONST(0.765366865);
		
		t0 = v0 + v3;
		t3 = v0 - v3;
		t1 = v1 + v2;
		t2 = v1 - v2;
		
		t4 = pd[7];
		t5 = pd[5];
		t6 = pd[3];
		t7 = pd[1];
		
		v0 = t4 + t7;
		v1 = t5 + t6;
		v2 = t4 + t6;
		v3 = t5 + t7;
		
		v4 = (v2 + v3) * CONST(1.175875602);
		
		v0 *= CONST(0.899976223);
		v1 *= CONST(2.562915447);
		v2 = v2 * CONST(1.961570560) - v4;
		v3 = v3 * CONST(0.390180644) - v4;
		
		t4 = t4 * CONST(0.298631336) - v0 - v2;
		t5 = t5 * CONST(2.053119869) - v1 - v3;
		t6 = t6 * CONST(3.072711026) - v1 - v2;
		t7 = t7 * CONST(1.501321110) - v0 - v3;
		
		pd[0] = (t0 + t7) >> (SHIFT_BITS * 2 + 3);
		pd[7] = (t0 - t7) >> (SHIFT_BITS * 2 + 3);
		pd[1] = (t1 + t6) >> (SHIFT_BITS * 2 + 3);
		pd[6] = (t1 - t6) >> (SHIFT_BITS * 2 + 3);
		pd[2] = (t2 + t5) >> (SHIFT_BITS * 2 + 3);
		pd[5] = (t2 - t5) >> (SHIFT_BITS * 2 + 3);
		pd[3] = (t3 + t4) >> (SHIFT_BITS * 2 + 3);
		pd[4] = (t3 - t4) >> (SHIFT_BITS * 2 + 3);
	}
	
	for (i = 0; i < JPEG_UNIT_SIZE * JPEG_UNIT_SIZE; i++)
	{
		du[i] += 128;
		
		if (du[i] < 0)
			du[i] = 0;
		
		if (du[i] > 255)
			du[i] = 255;
	}
}

static void jpeg_decode_du(struct jpeg_data* data, int id, jpeg_data_unit_t du)
{
	int h1, h2, qt;
	unsigned pos;
	
	memset(du, 0, sizeof(jpeg_data_unit_t));
	
	qt = data->comp_index[id][0];
	h1 = data->comp_index[id][1];
	h2 = data->comp_index[id][2];
	
	data->dc_value[id] += jpeg_get_number(data, jpeg_get_huff_code (data, h1));
	
	du[0] = data->dc_value[id] * (int)data->quan_table[qt][0];
	pos = 1;
	while (pos < ARRAY_SIZE(data->quan_table[qt]))
	{
		int num, val;
		
		num = jpeg_get_huff_code(data, h2);
		if (!num)
			break;
		
		val = jpeg_get_number(data, num & 0xF);
		num >>= 4;
		pos += num;
		du[jpeg_zigzag_order[pos]] = val * (int) data->quan_table[qt][pos];
		pos++;
	}
	
	jpeg_idct_transform(du);
}

static void jpeg_ycrcb_to_rgbx(int yy, int cr, int cb, uint8_t* rgb)
{
	int dd;
	
	cr -= 128;
	cb -= 128;
	
	/* Red  */
	dd = yy + ((cr * CONST(1.402)) >> SHIFT_BITS);
	if (dd < 0)
		dd = 0;
	if (dd > 255)
		dd = 255;
	*(rgb++) = dd;
	
	/* Green  */
	dd = yy - ((cb * CONST(0.34414) + cr * CONST (0.71414)) >> SHIFT_BITS);
	if (dd < 0)
		dd = 0;
	if (dd > 255)
		dd = 255;
	*(rgb++) = dd;
	
	/* Blue  */
	dd = yy + ((cb * CONST(1.772)) >> SHIFT_BITS);
	if (dd < 0)
		dd = 0;
	if (dd > 255)
		dd = 255;
	*(rgb++) = dd;
	
	/* Unused */
	*(rgb++) = 0;
}

static int jpeg_decode_sos(struct jpeg_data* data)
{
	int i, cc, r1, c1, nr1, nc1, vb, hb;
	uint8_t *ptr1;
	uint32_t data_offset;
	
	data_offset = jpeg_get_offset(data);
	data_offset += jpeg_get_word(data);
	
	cc = jpeg_get_byte(data);
	
	if (cc != 3)
		return 1;
	
	for (i = 0; i < cc; i++)
	{
		int id, ht;
		
		id = jpeg_get_byte(data) - 1;
		if ((id < 0) || (id >= 3))
			return 1;
		
		ht = jpeg_get_byte(data);
		data->comp_index[id][1] = (ht >> 4);
		data->comp_index[id][2] = (ht & 0xF) + 2;
	}
	
	jpeg_get_byte(data);	/* Skip 3 unused bytes.  */
	jpeg_get_word(data);
	
	if (jpeg_get_offset(data) != data_offset)
		return 1;
	
	if (data->output_data_size < (data->image_width) * (data->image_height) * 4)
		return 1;
	
	data->bit_mask = 0x0;
	
	vb = data->vs * 8;
	hb = data->hs * 8;
	nr1 = (data->image_height + vb - 1) / vb;
	nc1 = (data->image_width + hb - 1) / hb;
	
	ptr1 = data->output_data;
	
	for (r1 = 0; r1 < nr1; r1++, ptr1 += (vb * data->image_width - hb * nc1) * 4)
	{
		for (c1 = 0; c1 < nc1; c1++, ptr1 += hb * 4)
		{
			int r2, c2, nr2, nc2;
			uint8_t* ptr2;
			
			for (r2 = 0; r2 < data->vs; r2++)
				for (c2 = 0; c2 < data->hs; c2++)
					jpeg_decode_du(data, 0, data->ydu[r2 * 2 + c2]);
				
			jpeg_decode_du(data, 1, data->cbdu);
			jpeg_decode_du(data, 2, data->crdu);
			
			nr2 = (r1 == nr1 - 1) ? (data->image_height - r1 * vb) : vb;
			nc2 = (c1 == nc1 - 1) ? (data->image_width - c1 * hb) : hb;
			
			ptr2 = ptr1;
			for (r2 = 0; r2 < nr2; r2++, ptr2 += (data->image_width - nc2) * 4)
			{
				for (c2 = 0; c2 < nc2; c2++, ptr2 += 4)
				{
					int i0, yy, cr, cb;
					
					i0 = (r2 / data->vs) * 8 + (c2 / data->hs);
					cr = data->crdu[i0];
					cb = data->cbdu[i0];
					yy = data->ydu[(r2 / 8) * 2 + (c2 / 8)][(r2 % 8) * 8 + (c2 % 8)];
					
					jpeg_ycrcb_to_rgbx(yy, cr, cb, ptr2);
				}
			}
		}
	}
	
	return 0;
}

static uint8_t jpeg_get_marker(struct jpeg_data* data)
{
	uint8_t r;
	
	r = jpeg_get_byte(data);
	
	if (r != JPEG_ESC_CHAR)
		return 0;
	
	return jpeg_get_byte(data);
}

static int jpeg_decode_jpeg(struct jpeg_data* data)
{
	int error;
	
	if (jpeg_get_marker(data) != JPEG_MARKER_SOI)	/* Start Of Image.  */
		return 1;
	
	error = 0;
	while (error == 0)
	{
		uint8_t marker;
		
		marker = jpeg_get_marker(data);
		
		switch (marker)
		{
			case JPEG_MARKER_DHT:	/* Define Huffman Table.  */
				error = jpeg_decode_huff_table(data);
				break;
				
			case JPEG_MARKER_DQT:	/* Define Quantization Table.  */
				error = jpeg_decode_quan_table(data);
				break;
				
			case JPEG_MARKER_SOF0:	/* Start Of Frame 0.  */
				error = jpeg_decode_sof(data);
				break;
				
			case JPEG_MARKER_SOS:	/* Start Of Scan.  */
				error = jpeg_decode_sos(data);
				break;
				
			case JPEG_MARKER_EOI:	/* End Of Image.  */
				return error;
				
			default:		/* Skip unrecognized marker.  */
			{
				uint16_t sz;
				
				sz = jpeg_get_word(data);
				data->ptr += sz - 2;
			}
		}
	}
	return error;
}

/* Load jpeg to rggb buffer */
int jpeg_load_rgbx(uint8_t* output_data, int output_data_size, int* width, int* height, 
                   int* image_size, const uint8_t* jpeg_data, int jpeg_data_size)
{
	struct jpeg_data *data;
	int i, res;
	
	data = malloc(sizeof(struct jpeg_data));
	if (data == NULL)
		return 1;
	
	memset(data, 0, sizeof(struct jpeg_data));
	
	data->jpeg_data = jpeg_data;
	data->ptr = jpeg_data;
	data->jpeg_data_size = jpeg_data_size;
	data->output_data = output_data;
	data->output_data_size = output_data_size;
	res = 1;
	
	if (!jpeg_decode_jpeg(data))
	{
		*width = data->image_width;
		*height = data->image_height;
		*image_size = data->image_width * data->image_height * 4;
		res = 0;
	}
	
	for (i = 0; i < 4; i++)
		if (data->huff_value[i])
			free(data->huff_value[i]);
		
	free(data);
	return res;
}

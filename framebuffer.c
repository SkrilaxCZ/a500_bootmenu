/* 
 * Acer bootloader boot menu application framebuffer
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

#include <stddef.h>
#include <stdarg.h>
#include <bl_0_03_14.h>
#include <framebuffer.h>
#include <jpeg.h>

#define SCREEN_WIDTH        1280
#define SCREEN_HEIGHT       800

#define TITLE_LINES         2
#define LINE_WIDTH          (TEXT_LINE_CHARS + 1)
#define BUFFER_LINE_WIDTH   (LINE_WIDTH + 20)

#define TITLE_Y_OFFSET      (FONT_HEIGHT)

#define TEXT_Y_OFFSET       (4*FONT_HEIGHT)
#define TEXT_X_OFFSET       ((SCREEN_WIDTH/2) - (FONT_WIDTH*(TEXT_LINE_CHARS/2)))

#include <gui.h>

/* Title color */
struct color title_color =
{
	.R = 0xFF,
	.G = 0xFF,
	.B = 0xFF,
	.X = 0x00,
};

/* Text color */
struct color text_color =
{
	.R = 0xFF,
	.G = 0xFF,
	.B = 0xFF,
	.X = 0x00,
};

/* Highlight color */
struct color highlight_color =
{
	.R = 0x3F,
	.G = 0x3F,
	.B = 0x3F,
	.X = 0x00,
};

/* Framebuffer data */
uint8_t* framebuffer;
uint32_t framebuffer_size;

uint8_t* builder;

/* Background*/
uint8_t* background;

/* Font data */
uint8_t* font_data;

/* Title */
char title[BUFFER_LINE_WIDTH];

/* Status Message */
char status[BUFFER_LINE_WIDTH];

/* Text */
char text[TEXT_LINES][BUFFER_LINE_WIDTH];

/* Cursor */
int text_cur_x, text_cur_y;

/* Conversions  */
uint32_t clr2int(struct color clr)
{
	return (clr.R << 24) + (clr.G << 16) + (clr.B << 8) + clr.X;
}

struct color int2clr (uint32_t u)
{
	struct color clr;
	
	clr.R = ((u & 0xFF000000) >> 24);
	clr.G = ((u & 0x00FF0000) >> 16);
	clr.B = ((u & 0x0000FF00) >> 8);
	clr.X = (u & 0x000000FF);
	
	return clr;
}

/* 
 * Framebuffer error
 */
static void fb_error(const char* msg)
{
	/* Use stock BL functions to print error */
	clear_screen();
	println_display_error(msg);
	
	while (1) 
		sleep(1000);
}

/* 
 * Draw text on location
 */
static void fb_draw_string(uint32_t x, uint32_t y, const char* s, struct color* b, struct color* c)
{
	char off;
	int cc, bkg;
	uint32_t i, j, pixel;
	uint16_t p;
	struct color* changing;
	
	if (font_data == NULL)
		return;
	
	/* Check if we're in the screen */
	if (y + FONT_HEIGHT >= SCREEN_HEIGHT)
		return;
	
	bkg = ((b->R) || (b->G) || (b->B));
	
	while ((off = *s++))
	{
		/* Out of bounds */
		if (x + FONT_WIDTH >= SCREEN_WIDTH)
			return;
		
		/* Check color code */
		cc = 0;
		
		if (off == 0x1B)
		{
			changing = c;
			cc = 1;
		}
		else if (off == 0x1C)
		{
			changing = b;
			bkg = 1;
			cc = 1;
		}
		else if (off == 0x1D)
		{
			b->R = 0;
			b->G = 0;
			b->B = 0;
			bkg = 0;
			
			continue;
		}
		
		if (cc)
		{
			/* R */
			off = *s++;
			if (!off)
				break;
			
			changing->R = off;
			
			/* G */
			off = *s++;
			if (!off)
				break;
			
			changing->G = off;
			
			/* B */
			off = *s++;
			if (!off)
				break;
			
			changing->B = off;
			
			continue;
		}
		
		/* Allright - check if we can render it */ 
		off -= FIRST_CHAR;
		
		if (off < 0 || off >= NUM_CHARS)
			continue;
		
		for (i = 0; i < FONT_HEIGHT; i++)
		{
			for (j = 0; j < FONT_WIDTH; j++)
			{
				/* Get the pixel in font */
				p = font_data[(i * NUM_CHARS * FONT_WIDTH) + (off * FONT_WIDTH) + j];
				
				/* Get the pixel in the frame */
				pixel = sizeof(struct color) * ((y + i) * SCREEN_WIDTH + (x + j));
				
				if (bkg)
				{
					builder[pixel  ] = (uint8_t)(((b->R * (255 - p)) + (c->R * p)) / 255);
					builder[pixel+1] = (uint8_t)(((b->G * (255 - p)) + (c->G * p)) / 255);
					builder[pixel+2] = (uint8_t)(((b->B * (255 - p)) + (c->B * p)) / 255);
				}
				else
				{
					builder[pixel  ] = (uint8_t)(((builder[pixel  ] * (255 - p)) + (c->R * p)) / 255);
					builder[pixel+1] = (uint8_t)(((builder[pixel+1] * (255 - p)) + (c->G * p)) / 255);
					builder[pixel+2] = (uint8_t)(((builder[pixel+2] * (255 - p)) + (c->B * p)) / 255);
				}
				
			}
		}
		
		x += FONT_WIDTH;
	}
}

/*
 * Init framebuffer
 */
void fb_init()
{
	const uint8_t *in;
	uint8_t *out;
	uint8_t *jpg_out_data;
	int font_size;
	int jpg_height, jpg_width, jpg_image_size; 
	int i;
	uint32_t gray;
	
	/* Init framebuffer */
	framebuffer = *framebuffer_ptr;
	framebuffer_size = *framebuffer_size_ptr;
	
	/* Init font */
	font_size = FONT_WIDTH * FONT_HEIGHT * NUM_CHARS;
	jpg_out_data = malloc(font_size * sizeof(struct color));
	
	if (jpg_out_data != NULL)
	{
		/* The font is in RGB format */
		if (!jpeg_load_rgbx(jpg_out_data, font_size * sizeof(struct color), &jpg_width, &jpg_height, 
		                    &jpg_image_size, (const uint8_t*) FONT_OFFSET, FONT_SIZE_LIMIT))
		{
			font_data = malloc(font_size);
			
			if ((jpg_width != FONT_WIDTH * NUM_CHARS) || (jpg_height != FONT_HEIGHT))
				fb_error("Font jpg image size is not valid (must be 960x80).");
			
			/* Convert it to grayscale and store it */
			in = jpg_out_data;
			out = font_data;
			
			for (i = 0; i < font_size; i++)
			{
				gray = 299*in[0] + 587*in[1] + 114*in[2];
				out[i] = (char)(gray / 1000);
				in += 4;
			}
		}
		else
			fb_error("Failed to load jpg image.");
		
		free(jpg_out_data);
		jpg_out_data = NULL;
	}
	else
		fb_error("Failed to initialize font.");
	
	/* Init background */
	background = malloc(SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(struct color));
	
	if (background != NULL)
	{
		if (!jpeg_load_rgbx(background, SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(struct color), &jpg_width, &jpg_height, 
		                    &jpg_image_size, (const uint8_t*) BOOTLOGO_OFFSET, BOOTLOGO_SIZE_LIMIT))
		{
			if ((jpg_width != SCREEN_WIDTH) || (jpg_height != SCREEN_HEIGHT))
			{
				/* Discard - invalid size */
				free(background);
				background = NULL;
			}
		}
		else
		{
			free(background);
			background = NULL;
		}
		
	}
	
	text_cur_x = 0;
	text_cur_y = 0;
	
	/* Reset texts */
	title[0] = '\0';
	status[0] = '\0';
	text[0][0] = '\0';
	
	/* Clear framebuffer */
	builder = framebuffer + (SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(struct color));
	memset(framebuffer, 0, framebuffer_size);
}

/*
 * Set title
 */
void fb_set_title(const char* title_msg)
{
	strncpy(title, title_msg, ARRAY_SIZE(title));
}

/*
 * Set status
 */
void fb_set_status(const char* status_msg)
{
	strncpy(status, status_msg, ARRAY_SIZE(status));
}

/*
 * Draw text
 */
void fb_printf(const char* fmt, ...)
{
	char* ptr;
	char buffer[256];
	
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, ARRAY_SIZE(buffer), fmt, args);
	va_end(args);
	
	ptr = buffer;
	
	while (*ptr)
	{
		/* Color code */
		
		if (*ptr == '\n')
		{
			if (text_cur_y >= TEXT_LINES)
				break;
			
			text[text_cur_y][text_cur_x] = '\0';
			
			text_cur_y++;
			text_cur_x = 0;
			ptr++;
			continue;
		}
		
		if (text_cur_x >= (BUFFER_LINE_WIDTH - 1))
		{
			ptr++;
			continue;
		}
		
		/* Color code */
		if (*ptr == 0x1B)
		{
			text[text_cur_y][text_cur_x++] = *ptr++;
			
			/* R */
			if (*ptr && text_cur_x < (BUFFER_LINE_WIDTH - 1))
				text[text_cur_y][text_cur_x++] = *ptr++;
			
			/* G */
			if (*ptr && text_cur_x < (BUFFER_LINE_WIDTH - 1))
				text[text_cur_y][text_cur_x++] = *ptr++;
			
			/* B */
			if (*ptr && text_cur_x < (BUFFER_LINE_WIDTH - 1))
				text[text_cur_y][text_cur_x++] = *ptr++;
			
			continue;
		}
		
		/* Character */
		text[text_cur_y][text_cur_x++] = *ptr;
		ptr++;
	}
	
	/* Done - set null terminator */
	text[text_cur_y][text_cur_x] = '\0';
}

/*
 * Compatibility functions
 */
void fb_compat_println(const char* fmt, ...)
{
	char buffer[256];
	
	/* Parse arguments */
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, ARRAY_SIZE(buffer), fmt, args);
	va_end(args);
	
	/* Send it */
	fb_printf("%s%s\n", fb_text_color_code(text_color.R, text_color.G, text_color.B), buffer);
	fb_refresh();
}

void fb_compat_println_error(const char* fmt, ...)
{
	char buffer[256];
	
	/* Parse arguments */
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, ARRAY_SIZE(buffer), fmt, args);
	va_end(args);
	
	/* Send it */
	fb_printf("%s%s\n", fb_text_color_code(0xFF, 0xFF, 0x01), buffer);
	fb_refresh();
}

/*
 * Text color code
 */
static char text_color_buf[5];

const char* fb_text_color_code(uint8_t r, uint8_t g, uint8_t b)
{
	text_color_buf[0] = (char)0x1B;
	text_color_buf[1] = (char)r;
	text_color_buf[2] = (char)g;
	text_color_buf[3] = (char)b;
	text_color_buf[4] = '\0';
	
	return text_color_buf;
}

/*
 * Text color code
 */
static char background_color_buf[5];

const char* fb_background_color_code(uint8_t r, uint8_t g, uint8_t b)
{
	if (r || b || g)
	{
		background_color_buf[0] = (char)0x1C;
		background_color_buf[1] = (char)r;
		background_color_buf[2] = (char)g;
		background_color_buf[3] = (char)b;
		background_color_buf[4] = '\0';
	}
	else
	{
		background_color_buf[0] = (char)0x1D;
		background_color_buf[1] = '\0';
	}
	
	return background_color_buf;
}

/*
 * Clear framebuffer
 */
void fb_clear()
{
	/* Reset text */
	text_cur_x = 0;
	text_cur_y = 0;
	
	text[0][0] = '\0';
}

/*
 * Refresh screen
 */
void fb_refresh()
{
	int i, l;
	struct color b;
	struct color c;
		
	/* Clear framebuffer */
	if (background != NULL)
		memcpy(builder, background, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(struct color));
	else
		memset(builder, 0x00, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(struct color));
		
	/* Draw title */
	c = title_color;
	b.R = 0;
	b.G = 0;
	b.B = 0;
	
	l = strlen(title);
	fb_draw_string((SCREEN_WIDTH/2) - (l*FONT_WIDTH/2), TITLE_Y_OFFSET, title, &b, &c);
	
	c = title_color;
	b.R = 0;
	b.G = 0;
	b.B = 0;
	
	l = strlen(status);
	fb_draw_string((SCREEN_WIDTH/2) - (l*FONT_WIDTH/2), TITLE_Y_OFFSET + FONT_HEIGHT, status, &b, &c);
	
	/* Draw text */
	c = text_color;
	b.R = 0;
	b.G = 0;
	b.B = 0;
	
	for (i = 0; i <= text_cur_y; i++)
		fb_draw_string(TEXT_X_OFFSET, TEXT_Y_OFFSET + i * FONT_HEIGHT, text[i], &b, &c);
		
	/* Push and refresh framebuffer */
	memcpy(framebuffer, builder, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(struct color));
	framebuffer_unknown_call();
}
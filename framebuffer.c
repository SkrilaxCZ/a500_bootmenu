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
static void fb_draw_string(uint32_t x, uint32_t y, const char* s, struct color* c)
{
	char off;
	uint32_t i, j, pixel;
	uint16_t p;
	
	if (font_data == NULL)
		return;
	
	/* Check if we're in the screen */
	if (y + FONT_HEIGHT >= SCREEN_HEIGHT)
		return;
	
	while ((off = *s++))
	{
		/* Out of bounds */
		if (x + FONT_WIDTH >= SCREEN_WIDTH)
			return;
		
		/* Check color code */
		if (off == 0x1B)
		{
			/* R */
			off = *s++;
			if (!off)
				break;
			
			c->R = off;
			
			/* G */
			off = *s++;
			if (!off)
				break;
			
			c->G = off;
			
			/* B */
			off = *s++;
			if (!off)
				break;
			
			c->B = off;
			
			/* Next char */
			off = *s++;
			if (!off)
				break;
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
				
				builder[pixel  ] = (uint8_t)(((builder[pixel  ] * (255 - p)) + (c->R * p)) / 255);
				builder[pixel+1] = (uint8_t)(((builder[pixel+1] * (255 - p)) + (c->G * p)) / 255);
				builder[pixel+2] = (uint8_t)(((builder[pixel+2] * (255 - p)) + (c->B * p)) / 255);
				
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
	struct color c;
	
	/* Clear framebuffer */
	if (background != NULL)
		memcpy(builder, background, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(struct color));
	else
		memset(builder, 0x00, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(struct color));
		
	/* Draw title */
	c = title_color;
	l = strlen(title);
	fb_draw_string((SCREEN_WIDTH/2) - (l*FONT_WIDTH/2), TITLE_Y_OFFSET, title, &c);
	
	c = title_color;
	l = strlen(status);
	fb_draw_string((SCREEN_WIDTH/2) - (l*FONT_WIDTH/2), TITLE_Y_OFFSET + FONT_HEIGHT, status, &c);
	
	/* Draw text */
	c = text_color;
	
	for (i = 0; i <= text_cur_y; i++)
		fb_draw_string(TEXT_X_OFFSET, TEXT_Y_OFFSET + i * FONT_HEIGHT, text[i], &c);
		
	/* Push and refresh framebuffer */
	memcpy(framebuffer, builder, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(struct color));
	framebuffer_unknown_call();
}
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

#include <stdarg.h>
#include "mystdlib.h"
#include "bl_0_03_14.h"
#include "framebuffer.h"
#include "jpeg.h"

#define SCREEN_WIDTH            1280
#define SCREEN_HEIGHT           800

#define TITLE_LINES             2
#define LINE_WIDTH              (TEXT_LINE_CHARS + 1)
#define BUFFER_LINE_WIDTH       (LINE_WIDTH + 20)

#include <skin.h>

/* Font data */
struct font_data font =
{
	.font_height = -1,
	.font_width = -1,
	.font_outline = DEFAULT_FONT_OUTLINE,
	.font_kerning = DEFAULT_FONT_KERNING,
};

/* Text offsets */
#define TITLE_Y_OFFSET          (outlined_font_height(&font))

#define TEXT_Y_OFFSET           (4*outlined_font_height(&font))
#define TEXT_X_OFFSET           ((SCREEN_WIDTH/2) - ((outlined_font_width(&font) + font.font_kerning) * (TEXT_LINE_CHARS/2)))

/* Skin */
struct bootloader_skin skin;

/* Title color */
struct font_color title_color =
{
	.color =
	{
		.R = 0x98,
		.G = 0xCD,
		.B = 0x57,
		.X = 0x00,
	},
	.outline =
	{
		.R = 0x00,
		.G = 0x00,
		.B = 0x00,
		.X = 0x00,
	}
};

/* Text color */
struct font_color text_color =
{
	.color =
	{
		.R = 0x98,
		.G = 0xCD,
		.B = 0x57,
		.X = 0x00,
	},
	.outline =
	{
		.R = 0x00,
		.G = 0x00,
		.B = 0x00,
		.X = 0x00,
	}
};

/* Error text color */
struct font_color error_text_color =
{
	.color =
	{
		.R = 0x98,
		.G = 0xCD,
		.B = 0x57,
		.X = 0x00,
	},
	.outline =
	{
		.R = 0x00,
		.G = 0x00,
		.B = 0x00,
		.X = 0x00,
	}
};

/* Highlighting color */
struct color highlight_color =
{
	.R = 0x98,
	.G = 0xCD,
	.B = 0x57,
	.X = 0x00,
};

/* Highlighted text color */
struct font_color highlight_text_color =
{
	.color =
	{
		.R = 0x00,
		.G = 0x00,
		.B = 0x00,
		.X = 0x00,
	},
	.outline =
	{
		.R = 0x00,
		.G = 0x00,
		.B = 0x00,
		.X = 0x00,
	}
};

/* Framebuffer data */
uint8_t* framebuffer;
uint32_t framebuffer_size;

uint8_t* builder;

/* Background*/
uint8_t* background;

/* Font data */
uint8_t* font_data;

/* Font Outline data */
uint8_t* font_outline_data;

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
static void fb_draw_string(uint32_t x, uint32_t y, const char* s, struct color* b, struct color* c, struct color* o)
{
	char off;
	int cc, bkg;
	uint32_t i, j, pixel;
	uint16_t p, op;
	struct color* changing;

	if (font_data == NULL)
		return;

	/* Check if we're in the screen */
	if (y + outlined_font_height(&font) >= SCREEN_HEIGHT)
		return;

	bkg = ((b->R) || (b->G) || (b->B));

	while ((off = *s++))
	{
		/* Out of bounds */
		if (x + outlined_font_width(&font) >= SCREEN_WIDTH)
			return;

		/* Check color code */
		cc = 0;

		if (off == 0x1B)
		{
			changing = c;
			cc = 1;
		}
		else if (off == 0x2B)
		{
			changing = o;
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

		if (bkg)
		{
			for (i = 0; i < outlined_font_height(&font); i++)
			{
				for (j = 0; j < outlined_font_width(&font); j++)
				{
					/* Get the pixel in font */
					p = font_data[(i * NUM_CHARS * outlined_font_width(&font)) + (off * outlined_font_width(&font)) + j];

					/* Get the pixel in the frame */
					pixel = sizeof(struct color) * ((y + i) * SCREEN_WIDTH + (x + j));

					builder[pixel  ] = (uint8_t)(((b->R * (255 - p)) + (c->R * p)) / 255);
					builder[pixel+1] = (uint8_t)(((b->G * (255 - p)) + (c->G * p)) / 255);
					builder[pixel+2] = (uint8_t)(((b->B * (255 - p)) + (c->B * p)) / 255);
				}
			}
		}
		else if (off)
		{
			for (i = 0; i < outlined_font_height(&font); i++)
			{
				for (j = 0; j < outlined_font_width(&font); j++)
				{
					/* Get the pixel in font */
					pixel = (i * NUM_CHARS * outlined_font_width(&font)) + (off * outlined_font_width(&font)) + j;
					p = font_data[pixel];
					op = font_outline_data[pixel];

					/* Get the pixel in the frame */
					pixel = sizeof(struct color) * ((y + i) * SCREEN_WIDTH + (x + j));

					builder[pixel  ] = (uint8_t)(((builder[pixel  ] * (255 - p - op)) + (c->R * p) + (o->R * op)) / 255);
					builder[pixel+1] = (uint8_t)(((builder[pixel+1] * (255 - p - op)) + (c->G * p) + (o->G * op)) / 255);
					builder[pixel+2] = (uint8_t)(((builder[pixel+2] * (255 - p - op)) + (c->B * p) + (o->B * op)) / 255);
				}
			}
		}

		x += outlined_font_width(&font) + font.font_kerning;
	}
}

/*
 * Init framebuffer
 */
void fb_init()
{
	uint8_t *jpg_out_data, gray;
	int font_size;
	int jpg_height, jpg_width, jpg_image_size;
	int off, i, j, pixel, outline_i, outline_j;

	/* Init framebuffer */
	framebuffer = *framebuffer_ptr;
	framebuffer_size = *framebuffer_size_ptr;

	builder = malloc(SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(struct color));
	if (!builder)
	{
		fb_error("Failed to initialize framebuffer.");
		return;
	}

	/* Clear builder */
	memset(builder, 0, SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(struct color));

	/* Read skin */
	memcpy(&skin, (const uint8_t*)(CUSTOM_COLORS_OFFSET), sizeof(struct bootloader_skin));

	if (!memcmp(skin.magic, CUSTOM_COLORS_ID, 4))
	{
		/* Copy colors */
		title_color = skin.title_color;
		text_color = skin.text_color;
		error_text_color = skin.error_text_color;
		highlight_color = skin.highlight_color;
		highlight_text_color = skin.highlight_text_color;

		font.font_outline = skin.font_outline;
		font.font_kerning = skin.font_kerning;
	}

	/* Init font */
	jpg_out_data = malloc(MAXIMUM_FONT_DATA_SIZE);

	if (jpg_out_data)
	{
		/* The font is in RGB format */
		if (!jpeg_load_rgbx(jpg_out_data, MAXIMUM_FONT_DATA_SIZE, &jpg_width, &jpg_height,
		                    &jpg_image_size, (const uint8_t*) FONT_OFFSET, FONT_SIZE_LIMIT))
		{
			font.font_height = jpg_height;
			font.font_width = jpg_width / NUM_CHARS;

			font_size = font_data_size(&font);

			font_data = malloc(font_size);
			font_outline_data = malloc(font_size);

			/* Clear font and outline data */

			for (i = 0; i < font_size; i++)
			{
				font_data[i] = 0;
				font_outline_data[i] = 0;
			}

			/* Convert it to grayscale and store it */

			for (off = 0; off < NUM_CHARS; off++)
			{
				for (i = 0; i < font.font_height; i++)
				{
					for (j = 0; j < font.font_width; j++)
					{
						/* Load pixel from JPG */
						pixel = ((i * NUM_CHARS * font.font_width) + (off * font.font_width) + j) * sizeof(struct color);
						gray = (char)((299*jpg_out_data[pixel] + 587*jpg_out_data[pixel+1] + 114*jpg_out_data[pixel+2]) / 1000);

						/* Save pixel to font_data */
						pixel = ((i + font.font_outline) * NUM_CHARS * outlined_font_width(&font)) + (off * outlined_font_width(&font)) + j + font.font_outline;
						font_data[pixel] = gray;

						/* Outlining */
						if (font.font_outline)
						{
							for (outline_i = i; outline_i <= i + font.font_outline * 2; outline_i++)
							{
								for (outline_j = j; outline_j <= j + font.font_outline * 2; outline_j++)
								{
									pixel = (outline_i * NUM_CHARS * outlined_font_width(&font)) + (off * outlined_font_width(&font)) + outline_j;
									if (gray > font_outline_data[pixel])
										font_outline_data[pixel] = gray;
								}
							}
						}
					}
				}
			}

			/* Normalize outline data */
			if (font.font_outline)
			{
				for (i = 0; i < font_size; i++)
				{
					if (font_data[i] + font_outline_data[i] > 255)
						font_outline_data[i] = 255 - font_data[i];
				}
			}
		}
		else
			fb_error("Failed to load jpg image.");

		free(jpg_out_data);
		jpg_out_data = NULL;
	}
	else
	{
		fb_error("Failed to initialize font.");
		return;
	}

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
}

/*
 * Set title
 */
void fb_set_title(const char* title_msg)
{
	strncpy(title, title_msg, ARRAY_SIZE(title));
	title[ARRAY_SIZE(title) - 1] = '\0';
}

/*
 * Set status
 */
void fb_set_status(const char* status_msg)
{
	strncpy(status, status_msg, ARRAY_SIZE(status));
	status[ARRAY_SIZE(status) - 1] = '\0';
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

	/* We're out of the buffer, so don't print anything */
	if (text_cur_y >= TEXT_LINES)
		return;

	while (*ptr)
	{
		/* Line feed */
		if (*ptr == '\n')
		{
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

void fb_color_printf(const char* fmt, const struct color* bkg, const struct font_color* clr, ...)
{
	char buffer[256];

	va_list args;
	va_start(args, clr);
	vsnprintf(buffer, ARRAY_SIZE(buffer), fmt, args);
	va_end(args);

	fb_printf("%s%s%s%s",
	          fb_background_color_code2(bkg),
	          fb_text_color_code2(&clr->color),
	          fb_text_outline_color_code2(&clr->outline),
	          buffer);
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
	fb_color_printf("%s\n", NULL, &text_color, buffer);
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
	fb_color_printf("%s\n", NULL, &error_text_color, buffer);
	fb_refresh();
}

/*
 * Text color code
 */
static char text_color_buf[5];

const char* fb_text_color_code(uint8_t r, uint8_t g, uint8_t b)
{
	text_color_buf[0] = (char)0x1B;
	text_color_buf[1] = (char)(r ? r : 0x01);
	text_color_buf[2] = (char)(g ? g : 0x01);
	text_color_buf[3] = (char)(b ? b : 0x01);
	text_color_buf[4] = '\0';

	return text_color_buf;
}

const char* fb_text_color_code2(const struct color* c)
{
	if (!c)
		return "";

	text_color_buf[0] = (char)0x1B;
	text_color_buf[1] = (char)(c->R ? c->R : 0x01);
	text_color_buf[2] = (char)(c->G ? c->G : 0x01);
	text_color_buf[3] = (char)(c->B ? c->B : 0x01);
	text_color_buf[4] = '\0';

	return text_color_buf;
}

/*
 * Text outline color code
 */
static char text_outline_color_buf[5];

const char* fb_text_outline_color_code(uint8_t r, uint8_t g, uint8_t b)
{
	text_outline_color_buf[0] = (char)0x2B;
	text_outline_color_buf[1] = (char)(r ? r : 0x01);
	text_outline_color_buf[2] = (char)(g ? g : 0x01);
	text_outline_color_buf[3] = (char)(b ? b : 0x01);
	text_outline_color_buf[4] = '\0';

	return text_outline_color_buf;
}

const char* fb_text_outline_color_code2(const struct color* c)
{
	if (!c)
		return "";

	text_outline_color_buf[0] = (char)0x2B;
	text_outline_color_buf[1] = (char)(c->R ? c->R : 0x01);
	text_outline_color_buf[2] = (char)(c->G ? c->G : 0x01);
	text_outline_color_buf[3] = (char)(c->B ? c->B : 0x01);
	text_outline_color_buf[4] = '\0';

	return text_outline_color_buf;
}

/*
 * Background color code
 */
static char background_color_buf[5];

const char* fb_background_color_code(uint8_t r, uint8_t g, uint8_t b)
{
	if (r || g || b)
	{
		background_color_buf[0] = (char)0x1C;
		background_color_buf[1] = (char)(r ? r : 0x01);
		background_color_buf[2] = (char)(g ? g : 0x01);
		background_color_buf[3] = (char)(b ? b : 0x01);
		background_color_buf[4] = '\0';
	}
	else
	{
		background_color_buf[0] = (char)0x1D;
		background_color_buf[1] = '\0';
	}

	return background_color_buf;
}

const char* fb_background_color_code2(const struct color* c)
{
	if (c && (c->R || c->G || c->B))
	{
		background_color_buf[0] = (char)0x1C;
		background_color_buf[1] = (char)(c->R ? c->R : 0x01);
		background_color_buf[2] = (char)(c->G ? c->G : 0x01);
		background_color_buf[3] = (char)(c->B ? c->B : 0x01);
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
	struct color o;

	/* Clear framebuffer */
	if (background != NULL)
		memcpy(builder, background, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(struct color));
	else
		memset(builder, 0x00, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(struct color));

	/* Draw title */
	c = title_color.color;
	o = title_color.outline;
	b.R = 0;
	b.G = 0;
	b.B = 0;

	l = strlen(title);
	fb_draw_string((SCREEN_WIDTH/2) - (l*(outlined_font_width(&font) + font.font_kerning)/2), TITLE_Y_OFFSET, title, &b, &c, &o);

	c = title_color.color;
	o = title_color.outline;
	b.R = 0;
	b.G = 0;
	b.B = 0;

	l = strlen(status);
	fb_draw_string((SCREEN_WIDTH/2) - (l*(outlined_font_width(&font) + font.font_kerning)/2), TITLE_Y_OFFSET + outlined_font_height(&font), status, &b, &c, &o);

	/* Draw text */
	c = text_color.color;
	o = text_color.outline;
	b.R = 0;
	b.G = 0;
	b.B = 0;

	for (i = 0; i <= text_cur_y && i < TEXT_LINES; i++)
		fb_draw_string(TEXT_X_OFFSET, TEXT_Y_OFFSET + i * outlined_font_height(&font), text[i], &b, &c, &o);

	/* Push and refresh the framebuffer */
	memcpy(framebuffer, builder, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(struct color));
	framebuffer_unknown_call();
}

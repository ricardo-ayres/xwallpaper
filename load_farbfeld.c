/*
 * Copyright (c) 2021 Ricardo B. Ayres <ricardo.bosqueiro.ayres@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <pixman.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "functions.h"

#define FF_MAGIC "farbfeld"

pixman_image_t *
badread(const char *msg)
{
	debug(msg);
	return NULL;
}

pixman_image_t *
load_farbfeld(FILE *fp)
{
	pixman_image_t *img;
	uint8_t *buf;
	uint8_t a, r, g, b;
	uint32_t *pixels, *pixel, width, height;
	size_t len;

	/* the farbfeld header is 16 bytes long */
	buf = xmalloc(8*sizeof(uint8_t));
	if ((fread(buf, sizeof(uint8_t), 8, fp) != 8)
	   || (memcmp(FF_MAGIC, (uint8_t *) buf, 8) != 0))
		return badread("invalid farbfeld magic.\n");

	if (fread(buf, sizeof(uint8_t), 8, fp) != 8)
		return badread("could not read file.\n");

	width = ntohl(((uint32_t *) buf)[0]);
	height = ntohl(((uint32_t *) (buf))[1]);

	/* now comes the pixels */
	SAFE_MUL3(len, height, width, sizeof(*pixel));
	pixel = pixels = xmalloc(len);

	/* here we naively convert to 8-bit per channel argb while reading */
	while ((feof(fp) == 0)
		    && (fread(buf, sizeof(uint8_t), 8, fp) == 8)) {
		r = buf[0];
		g = buf[2];
		b = buf[4];
		a = buf[6];
		*pixel = (a<<24) | (r<<16) | (g<<8) | (b<<0);
		pixel++;
	}
	free(buf);

	img = pixman_image_create_bits(PIXMAN_a8r8g8b8, width, height, pixels,
	    width * sizeof(uint32_t));
	if (img == NULL) {
		errx(1, "failed to create pixman image");
		free(pixels);
	}

	return img;
}

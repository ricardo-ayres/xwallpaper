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

/* read and convert 16-bit channel to 8-bit channel */
static uint8_t
getffchannel(uint16_t n16)
{
	return UINT8_MAX * ntohs(n16) / UINT16_MAX; 
}

pixman_image_t *
load_farbfeld(FILE *fp)
{
	pixman_image_t *img;
	uint8_t *buf, a, r, g, b;
	uint16_t *c;
	uint32_t *pixels, *pixel, width, height, pcount;
	size_t len;

	/* the farbfeld header is 16 bytes long */
	/* the magic number is in the first 8 bytes */
	buf = xmalloc(8*sizeof(uint8_t));
	if (fread(buf, sizeof(uint8_t), 8, fp) != 8)
		errx(1, "failed to read farbfeld magic");
	if (memcmp(FF_MAGIC, (uint8_t *) buf, 8) != 0) {
		debug("invalid farbfeld magic\n");
		return NULL;
	}
	/* and the picture dimensions are the next 8 bytes */
	if (fread(buf, sizeof(uint8_t), 8, fp) != 8)
		errx(1, "failed to read farbfeld dimensions");
	width = ntohl(((uint32_t *) buf)[0]);
	height = ntohl(((uint32_t *) buf)[1]);
	pcount = width * height;
	debug("farbfeld dimensions: %lu x %lu\n", width, height);

	/* now comes the pixels */
	SAFE_MUL3(len, height, width, sizeof(*pixel));
	pixel = pixels = xmalloc(len);

	/* build each pixel and write to pixels buffer */
	while (fread(buf, sizeof(uint16_t), 4, fp) == 4) {
		c = (uint16_t *) buf;
		r = getffchannel(c[0]);
		g = getffchannel(c[1]);
		b = getffchannel(c[2]);
		a = getffchannel(c[3]);
		uint8_t c8[] = {a, r, g, b};
		*pixel = htonl(*((uint32_t *) c8));
		pixel++;
		pcount--;
	}
	/* check if loop exited on error */
	if (!feof(fp) || ferror(fp) || pcount > 0)
		errx(1, "read error with %d pixels remaining", pcount);

	free(buf);

	img = pixman_image_create_bits(PIXMAN_a8r8g8b8, width, height, pixels,
	    width * sizeof(uint32_t));
	if (img == NULL) {
		errx(1, "failed to create pixman image");
		free(pixels);
	}

	return img;
}

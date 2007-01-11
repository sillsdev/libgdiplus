/*
 * Copyright (C) 2007 Novell, Inc (http://www.novell.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *	Sebastien Pouliot  <sebastien@ximian.com>
 */

#include "icocodec.h"

/* Codecinfo related data*/
static ImageCodecInfo ico_codec;
static const WCHAR ico_codecname[] = {'B', 'u', 'i','l', 't', '-','i', 'n', ' ', 'I', 'C', 'O', 0}; /* Built-in ICO */
static const WCHAR ico_extension[] = {'*','.','I', 'C', 'O', 0}; /* *.ICO */
static const WCHAR ico_mimetype[] = {'i', 'm', 'a','g', 'e', '/', 'x', '-', 'i', 'c', 'o', 'n', 0}; /* image/x-icon */
static const WCHAR ico_format[] = {'I', 'C', 'O', 0}; /* ICO */
static const BYTE ico_sig_pattern[] = { 0x00, 0x00, 0x01, 0x00 };
static const BYTE ico_sig_mask[] = { 0xFF, 0xFF, 0xFF, 0xFF };


ImageCodecInfo*
gdip_getcodecinfo_ico ()
{
	ico_codec.Clsid = (CLSID) { 0x557cf407, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e } };
	ico_codec.FormatID = gdip_ico_image_format_guid;
	ico_codec.CodecName = (const WCHAR*) ico_codecname;            
	ico_codec.DllName = NULL;
	ico_codec.FormatDescription = (const WCHAR*) ico_format;
	ico_codec.FilenameExtension = (const WCHAR*) ico_extension;
	ico_codec.MimeType = (const WCHAR*) ico_mimetype;
	ico_codec.Flags = Decoder | SupportBitmap | Builtin;
	ico_codec.Version = 1;
	ico_codec.SigCount = 1;
	ico_codec.SigSize = 4;
	ico_codec.SigPattern = ico_sig_pattern;
	ico_codec.SigMask = ico_sig_mask;
	return &ico_codec; 
}

static BYTE
get_ico_data (BYTE *data, int x, int y, int bpp, int line_length)
{
	BYTE result = 0;
	BYTE *line_data = data + y * line_length;

	switch (bpp) {
	case 1:
		result = (line_data [x >> 3] >> (7 - (x & 7))) & 0x01;
		break;
	case 4:
		result = line_data [x >> 1];
		if ((x & 1) == 1)
			result &= 0x0F;
		else
			result >>= 4;
		break;
	case 8:
		result = line_data [x];
		break;
	}
	return result;
}

static GpStatus
gdip_read_ico_image_from_file_stream (void *pointer, GpImage **image, bool useFile)
{
	GpStatus status = InvalidParameter;
	GpBitmap *result = NULL;
	guchar *pixels = NULL;
	WORD w, count;
	void *p = &w;
	BYTE *b = (BYTE*)&w;
	ICONDIRENTRY entry;
	int i, pos;
	BOOL upsidedown = TRUE;
	BOOL os2format = FALSE;
	BITMAPINFOHEADER bih;
	int palette_entries;
	ARGB *colors = NULL;
	int x, y;
	int line_xor_length, xor_size;
	int line_and_length, and_size;
	BYTE *xor_data = NULL, *and_data = NULL;

	/* WORD ICONDIR.idReserved / reversed, MUST be 0 */
	if (gdip_read_ico_data (pointer, p, sizeof (WORD), useFile) != sizeof (WORD))
		goto error;
	if (w != 0)
		goto error;

	/* WORD ICONDIR.idType / resource type, MUST be 1 for icons */
	if (gdip_read_ico_data (pointer, p, sizeof (WORD), useFile) != sizeof (WORD))
		goto error;
	if (w != 1)
		goto error;

	/* WORD ICONDIR.idCount / number of icons, must be greater than 0 */
	if (gdip_read_ico_data (pointer, p, sizeof (WORD), useFile) != sizeof (WORD))
		goto error;
	count = (b[1] << 8 | b[0]); 
	if (count < 1)
		goto error;

	pos = 6;
	/*
	 * NOTE: it looks like (from unit tests) that we get the last icon 
	 * (e.g. it can return the 16 pixel version, instead of the 32 or 48 pixels available in the same file)
	 */ 
	for (i = 0; i < count; i++) {
		if (gdip_read_ico_data (pointer, (void*)&entry, sizeof (ICONDIRENTRY), useFile) != sizeof (ICONDIRENTRY))
			goto error;
		pos += sizeof (ICONDIRENTRY);
	}

	while (pos < entry.dwImageOffset) {
		if (gdip_read_ico_data (pointer, p, sizeof (WORD), useFile) != sizeof (WORD))
			goto error;
		pos += sizeof (WORD);
	}

	/* BITMAPINFOHEADER */
	status = gdip_read_BITMAPINFOHEADER (pointer, &bih, useFile, &os2format, &upsidedown);
	if (status != Ok)
		goto error;
	
	result = gdip_bitmap_new_with_frame (NULL, TRUE);
	result->type = imageBitmap;
	result->image_format = ICON;
	result->active_bitmap->pixel_format = Format32bppArgb; /* icons are always promoted to 32 bbp */
	result->active_bitmap->width = entry.bWidth;
	result->active_bitmap->height = entry.bHeight;
	result->active_bitmap->stride = result->active_bitmap->width * 4;
	/* Ensure pixman_bits_t alignment */
	result->active_bitmap->stride += (sizeof(pixman_bits_t) - 1);
	result->active_bitmap->stride &= ~(sizeof(pixman_bits_t) - 1);
	result->active_bitmap->dpi_horz = 96.0f;
	result->active_bitmap->dpi_vert = 96.0f;

	switch (bih.biBitCount) {
	case 1:
	case 4:
	case 8:
		/* support 2, 16 and 256 colors icons, no compression */
		if (bih.biCompression == 0) {
			palette_entries = 1 << bih.biBitCount;
			break;
		}
		 /* fall through */
	default:
		status = InvalidParameter;
		goto error;
	}

	/*
	 * Strangely, even if we're supplying a 32bits ARGB image, 
	 * the icon's palette is also supplied with the image.
	 */
	result->active_bitmap->palette = GdipAlloc (sizeof(ColorPalette) + sizeof(ARGB) * palette_entries);
	if (result->active_bitmap->palette == NULL) {
		status = OutOfMemory;
		goto error;
	}
	result->active_bitmap->palette->Flags = 0;
	result->active_bitmap->palette->Count = palette_entries;

	for (i = 0; i < palette_entries; i++) {
		/* colors are stored as R, G, B and reserved (always 0) */
		BYTE color[4]; 
		void *p = &color;

		if (gdip_read_ico_data (pointer, p, 4, useFile) < 4) {
			status = InvalidParameter;
			goto error;
		}

		set_pixel_bgra (result->active_bitmap->palette->Entries, i * 4,
			(color[0] & 0xFF),		/* B */
			(color[1] & 0xFF),		/* G */
			(color[2] & 0xFF),		/* R */
			0xFF);				/* Alpha */
	}

	/*
	 * Let's build the 32bpp ARGB bitmap from the icon's XOR and AND bitmaps
	 * notes:
	 * - XORBitmap can be a 1, 4 or 8 bpp bitmap
	 * - ANDBitmap is *always* a monochrome (1bpp) bitmap
	 * - in every case each line is padded to 32 bits boundary
	 */
	pixels = GdipAlloc (result->active_bitmap->stride * result->active_bitmap->height);
	if (pixels == NULL) {
		status = OutOfMemory;
		goto error;
	}

	result->active_bitmap->scan0 = pixels;
	result->active_bitmap->reserved = GBD_OWN_SCAN0;
	result->active_bitmap->image_flags = ImageFlagsReadOnly | ImageFlagsHasRealPixelSize | ImageFlagsColorSpaceRGB | ImageFlagsHasAlpha;

	line_xor_length = (((bih.biBitCount * entry.bWidth + 31) & ~31) >> 3);
	xor_size = line_xor_length * entry.bHeight;
	xor_data = (BYTE*) GdipAlloc (xor_size);
	if (!xor_data) {
		status = OutOfMemory;
		goto error;
	}
	if (gdip_read_ico_data (pointer, xor_data, xor_size, useFile) < xor_size) {
		status = InvalidParameter;
		goto error;
	}

	line_and_length = (((entry.bWidth + 31) & ~31) >> 3);
	and_size = line_and_length * entry.bHeight;
	and_data = (BYTE*) GdipAlloc (and_size);
	if (!and_data) {
		status = OutOfMemory;
		goto error;
	}
	if (gdip_read_ico_data (pointer, and_data, and_size, useFile) < and_size) {
		status = InvalidParameter;
		goto error;
	}

	colors = result->active_bitmap->palette->Entries;
	for (y = 0; y < entry.bHeight; y++) {
		for (x = 0; x < entry.bWidth; x++) {
			ARGB color = colors [get_ico_data (xor_data, x, y, bih.biBitCount, line_xor_length)];
			if (get_ico_data (and_data, x, y, 1, line_and_length) == 1)
				color = color & 0x00FFFFFF;
			/* image is reversed (y) */
			GdipBitmapSetPixel (result, x, entry.bHeight - y - 1, color);
		}
	}

	GdipFree (xor_data);
	GdipFree (and_data);

	*image = result;
	return Ok;

error:
	if (result->active_bitmap->palette)
		GdipFree (result->active_bitmap->palette);
	if (xor_data)
		GdipFree (xor_data);
	if (and_data)
		GdipFree (and_data);
	return status;
}

GpStatus 
gdip_load_ico_image_from_file (FILE *fp, GpImage **image)
{
	return gdip_read_ico_image_from_file_stream ((void*)fp, image, TRUE);
}

GpStatus 
gdip_load_ico_image_from_stream_delegate (dstream_t *loader, GpImage **image)
{
	return gdip_read_ico_image_from_file_stream ((void *)loader, image, FALSE);
}
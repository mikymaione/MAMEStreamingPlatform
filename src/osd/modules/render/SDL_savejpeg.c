/**
 * Copyright (C) 2004 Pluto, Inc., a Florida Corporation
 *
 * www.plutohome.com
 *
 * Phone: +1 (877) 758-8648
 * This program is distributed according to the terms of the Pluto Public
 * License, available at:
 * http://plutohome.com/index.php?section=public_license
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the Pluto
 * Public License for more details.
 */

 /**
  * Adapted from JpegWrapper.cpp by Tyler Montbriand, 2005
  * http://svn.plutohome.com/pluto/trunk/src/Orbiter/SDL/JpegWrapper.cpp
  */

#include <stdio.h>

#include "jpeglib.h"

#include "SDL_savejpeg.h"

#define SUCCESS 0
#define ERROR -1
#define RGB_PIXELSIZE 4

int SDL_SaveJPG_RW(SDL_Surface* pSurface, SDL_RWops* dst, int freedst, int iQuality)
{
	//unsigned long jpg_length;
	int bDeleteBuffer = 0;

	if (!dst)
	{
		SDL_SetError("Argument 2 to SDL_SavePNG_RW can't be NULL, expecting SDL_RWops*\n");

		return (ERROR);
	}
	if (!pSurface)
	{
		SDL_SetError("Argument 1 to SDL_SavePNG_RW can't be NULL, expecting SDL_Surface*\n");

		if (freedst)
			SDL_RWclose(dst);

		return (ERROR);
	}

	Uint32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#elif SDL_BYTEORDER == SDL_LIL_ENDIAN
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#else
#error "No endian defined!"
#endif

	SDL_Surface* nsurface = SDL_CreateRGBSurface(
		SDL_SWSURFACE,
		pSurface->w, pSurface->h, 24,
		rmask, gmask, bmask, amask);

	if (nsurface == NULL)
	{
		SDL_SetError("Couldn't flatten surface to 24bpp");
		return (-1);
	}

	SDL_BlitSurface(pSurface, NULL, nsurface, NULL);
	pSurface = nsurface;
	bDeleteBuffer = 1;

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);

	jpeg_create_compress(&cinfo);

	cinfo.image_width = pSurface->w;
	cinfo.image_height = pSurface->h;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	if (SDL_MUSTLOCK(pSurface))
		SDL_LockSurface(pSurface);

	unsigned char* image_buffer = pSurface->pixels;

	if (iQuality < 0)
		iQuality = JPEG_QUALITY_DEFAULT;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, iQuality, TRUE /* limit to baseline-JPEG values */);
	//jpeg_mem_dest(&cinfo, &dst, &jpg_length);
	jpeg_start_compress(&cinfo, TRUE);

	JSAMPROW row_pointer[1];
	int row_stride = pSurface->pitch;

	while (cinfo.next_scanline < cinfo.image_height)
	{
		row_pointer[0] = &image_buffer[cinfo.next_scanline * row_stride];
		(void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	if (SDL_MUSTLOCK(pSurface))
		SDL_UnlockSurface(pSurface);

	if (bDeleteBuffer)
		SDL_FreeSurface(pSurface);

	/* After finish_compress, we can close the output file. */
	if (freedst)
		SDL_RWclose(dst);

	return (SUCCESS);
}

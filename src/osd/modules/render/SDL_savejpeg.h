/*
 * Copyright (C) 2004 Pluto, Inc., a Florida Corporation
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
 * Adapted from JpegWrapper.h by Tyler Montbriand, 2005
 * http://svn.plutohome.com/pluto/trunk/src/Orbiter/SDL/JpegWrapper.h
 */
#ifndef __SDL_SAVEJPEG_H__
#define __SDL_SAVEJPEG_H__

#include <SDL_video.h>

#ifdef __cplusplus
extern "C" { /* This helps CPP projects that include this header */
#endif

#define JPEG_QUALITY_MIN 0
#define JPEG_QUALITY_MAX 100
#define JPEG_QUALITY_DEFAULT 70

/*
 * Save an SDL_Surface as a JPG file.
 *
 * Returns 0 success or -1 on failure, the error message is then retrievable
 * via SDL_GetError().
 */
#define SDL_SaveJPG(surface, file) \
	SDL_SaveJPG_RW(surface, SDL_RWFromFile(file, "wb"), 1)

/*
 * Save an SDL_Surface as a JPG file, using writable RWops.
 * 
 * surface - the SDL_Surface structure containing the image to be saved
 * dst - a data stream to save to
 * freedst - non-zero to close the stream after being written
 *
 * Returns 0 success or -1 on failure, the error message is then retrievable
 * via SDL_GetError().
 */
extern int SDL_SaveJPG_RW(SDL_Surface *surface, SDL_RWops *rw, int freedst, int iQuality);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*__SDL_SAVEJPEG_H__*/
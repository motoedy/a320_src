/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

#ifndef	LOADIMAGE_H
#define LOADIMAGE_H

// Blah.

s_screen * loadscreen(char *filename, char *packfile, unsigned char* pal, int format);
s_bitmap * loadbitmap(char *filename, char *packfile, int format);
int loadimagepalette(char *filename, char *packfile, unsigned char* pal);
#endif



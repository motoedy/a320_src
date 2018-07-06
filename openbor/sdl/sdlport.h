/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

#ifndef SDLPORT_H
#define SDLPORT_H

#include <SDL/SDL.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "globals.h"

#if GP2X || LINUX || DINGOO
#define stricmp  strcasecmp
#define strnicmp strncasecmp
#endif

//#define MEMTEST 1

void initSDL();
void borExit(int reset);
void openborMain(void);

extern char packfile[128];

#endif

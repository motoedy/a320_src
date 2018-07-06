/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

#ifndef __GFXTYPES_H__
#define __GFXTYPES_H__

#ifndef PSP
typedef   signed char s8;
typedef unsigned char u8;
typedef   signed short s16;
typedef unsigned short u16;
typedef   signed long s32;
typedef unsigned long u32;
#else
#include "psptypes.h"
#endif

#ifndef bool
#define bool s8
#endif

#if _WIN32_ || XBOX
#define inline _inline
#endif

extern u32 GfxColorMask;
extern u32 GfxLowPixelMask;
extern u32 GfxQColorMask;
extern u32 GfxQLowpixelMask;
extern u32 qRGB_COLOR_MASK[2];
extern u32 RGB_LOW_BITS_MASK;
extern u32 GfxColorDepth;
extern u32 GfxRedBlueMask;
extern u32 GfxGreenMask;
extern u32 GfxRedShift;
extern u32 GfxGreenShift;
extern u32 GfxBlueShift;

#endif
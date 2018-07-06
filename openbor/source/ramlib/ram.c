/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

/*
 * This library is used for calculating how much memory is available/used.
 * Certain platforms offer physical memory statistics, we obviously wrap
 * around those functions.  For platforms where we can't retrieve this
 * information we then calculate the estimated sizes based on a few key
 * variables and symbols.  These estimated values should tolerable.......
 */

/////////////////////////////////////////////////////////////////////////////
// Libraries

#ifdef XBOX
#include <xtl.h>
#elif WIN
#include <windows.h>
#elif OSX
#include <sys/sysctl.h>
#elif LINUX
#include <sys/sysinfo.h>
#elif PSP
#include "kernel/kernel.h"
#elif GP2X
#include "gp2xport.h"
#endif

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "ram.h"

/////////////////////////////////////////////////////////////////////////////
// Globals

static unsigned long systemRam = 0x00000000;
static unsigned long elfOffset = 0x00000000;
static unsigned long stackSize = 0x00000000;


/////////////////////////////////////////////////////////////////////////////
// Symbols

#ifndef OSX
#ifndef _WIN32_
#ifndef XBOX
#if (__GNUC__ > 3)
extern unsigned long _end;
extern unsigned long _start;
#else
extern unsigned long end;
extern unsigned long start;
#define _end end
#define _start start
#endif
#endif
#endif
#endif

/////////////////////////////////////////////////////////////////////////////
//  Functions

unsigned long getFreeRam()
{
#if WIN || XBOX
	MEMORYSTATUS stat;
	memset(&stat, 0, sizeof(MEMORYSTATUS));
	stat.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&stat);
	return stat.dwAvailPhys - stackSize;
#elif OSX
	unsigned int physmem;
	size_t len;
	len = sizeof(physmem);
	sysctlbyname("hw.physmem", &physmem, &len, NULL, 0);
	return physmem - stackSize;
#elif LINUX
	struct sysinfo info;
	sysinfo(&info);
	return info.freeram - stackSize;
#else
    struct mallinfo mi = mallinfo();
#ifdef _INCLUDE_MALLOC_H_
	// Standard ANSI C Implementation
    return systemRam - (mi.arena + stackSize);
#else
    return systemRam - (mi.usmblks + stackSize);
#endif
#endif
}

void setSystemRam()
{
	char string[128] = {""};
#if DC
	// 16 MBytes - Memory Map:
	systemRam = 0x8d000000 - 0x8c000000;
	elfOffset = 0x8c000000;
#elif PSP
	// 24 MBytes - Memory Map:
	systemRam = 0x0A000000 - 0x08800000;
	elfOffset = 0x08800000;
	if (getHardwareModel() == 1) systemRam += 32 * 1024 * 1024;
#elif GP2X
	// 32 MBytes - Memory Map:
	systemRam = 0x02000000 - 0x00000000;
	elfOffset = 0x00000000;
	if (gp2x_init() == 2) systemRam += 32 * 1024 * 1024;
#else
	elfOffset = 0x00000000;
	stackSize = 0x00000000;
	systemRam = getFreeRam();
#endif
#ifndef OSX
#ifndef _WIN32_
#ifndef XBOX
	stackSize = (int)&_end - (int)&_start + ((int)&_start - elfOffset);
#endif
#endif
#endif
	getRamStatus(string);
	printf("%s\n\n", string);
}

unsigned long getSystemRam()
{
	return systemRam;
}

unsigned long getUsedRam()
{
	return (systemRam - getFreeRam());
}

void getRamStatus()
{
	char status[128] = {""};
	sprintf(status,
		    "Total Ram: %lu, Free Ram: %lu, Used Ram: %lu",
			getSystemRam(),
			getFreeRam(),
			getUsedRam());
}

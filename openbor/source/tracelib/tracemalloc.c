/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

/////////////////////////////////////////////////////////////////////////////

#define I_AM_TRACEMALLOC

#ifdef PSP
#include <pspsysmem.h>
#endif

#include <stdio.h>
#include <malloc.h>
#include "ram.h"
#include "globals.h"
#include "tracemalloc.h"

/////////////////////////////////////////////////////////////////////////////

static int *tracehead = NULL;
unsigned long tracemalloc_total = 0;

#ifdef GP2X
	#define TRACE_BYTES 20
	#define TRACE_SIZE 4
#else
	#define TRACE_BYTES 16
	#define TRACE_SIZE 3
#endif

/////////////////////////////////////////////////////////////////////////////

static void tracemalloc_dump_collect(int *p, int *len, int *nalloc)
{
	int name = p[2];
    *len = 0;
    *nalloc = 0;
    for(; p; p = (int*)(p[1]))
    {
		if(p[2] == name && p[TRACE_SIZE] > 0)
		{
			(*len) += p[TRACE_SIZE];
			(*nalloc) += 1;
			p[TRACE_SIZE] = -p[TRACE_SIZE];
		}
	}
}

int tracemalloc_dump(void)
{
	int totalbytes = 0;
	int *p, *pp;
	for(p = tracehead; p; p = (int*)(p[1]))
	{
		if(p[TRACE_SIZE] > 0)
		{
			const char *name = (const char*)(p[2]);
			int len = 0;
			int nalloc = 0;
			tracemalloc_dump_collect(p, &len, &nalloc);
			printf("%s: %d bytes in %d allocs\n", name, len, nalloc);
			totalbytes += len;
		}
	}
	for(p = tracehead; p; pp = p, p = (int*)(p[1]), free(pp))
		if(p[TRACE_SIZE] < 0) p[TRACE_SIZE] = -p[TRACE_SIZE];
    if(totalbytes)
	{
		printf("Total Leaked Bytes %d\n", totalbytes);
		return 1;
	}
	return 0;
}

void *tracemalloc(const char *name, int len)
{
	int *p;
	p = malloc(TRACE_BYTES + len);

#ifdef GP2X
	int uRam = 0;
	if(!p)
	{
		p = UpperMalloc(TRACE_BYTES + len);
		if(!p)
		{
			writeToLogFile("name: %s Requested: %d Bytes, Remaining, %lu Bytes\n", name, len, getFreeRam());
			return NULL;
		}
		uRam = 1;
	}
#else
	if(!p)
	{
		writeToLogFile("name: %s Requested: %d Bytes, Remaining, %lu Bytes\n", name, len, getFreeRam());
		return NULL;
	}
#endif

	if(tracehead) tracehead[0] = (int)p;
	p[0] = 0;
	p[1] = (int)tracehead;
	p[2] = (int)name;

#ifdef GP2X
	p[3] = uRam;
#endif

	p[TRACE_SIZE] = len;
	tracehead = p;
	tracemalloc_total += TRACE_BYTES + len;
	return (void*)(p + (TRACE_SIZE + 1));
}

void tracefree(void *vp)
{
	int *p = NULL;
	int *p_from_prev = NULL;
	int *p_from_next = NULL;
	p = ((int*)vp) - (TRACE_SIZE + 1);
	tracemalloc_total -= TRACE_BYTES + p[TRACE_SIZE];
	if(p == tracehead) p_from_prev = (void*)(&tracehead);
	else               p_from_prev = (int*)(p[0] + 4);
	p_from_next = (int*)(p[1]);
	if(p_from_prev) *p_from_prev = p[1];
	if(p_from_next) *p_from_next = p[0];

#ifdef GP2X
	if(p[3]) UpperFree(p);
	else free(p);
#else
	free(p);
#endif
}

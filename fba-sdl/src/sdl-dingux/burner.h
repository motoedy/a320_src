/*
 * FinalBurn Alpha for Dingux/OpenDingux
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _BURNER_H_
#define _BURNER_H_

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>

#include "tchar.h"

// Macro to make quoted strings
#define MAKE_STRING_2(s) #s
#define MAKE_STRING(s) MAKE_STRING_2(s)

#define BZIP_MAX (20)								// Maximum zip files to search through
#define DIRS_MAX (20)								// Maximum number of directories to search

#include "title.h"
#include "burn.h"
#include "png.h"
#include "burner_sdl.h"

// ---------------------------------------------------------------------------
// OS dependent functionality
typedef struct tagIMAGE {
	unsigned int	width;
	unsigned int	height;
	unsigned int	rowbytes;
	unsigned int	imgbytes;
	unsigned char**	rowptr;
	unsigned char*	bmpbits;
	unsigned int	flags;
} IMAGE;

// ---------------------------------------------------------------------------
// OS independent functionality

#include "interface.h"

#define IMG_FREE		(1 << 0)

// Macros for parsing text
#define SKIP_WS(s) while (_istspace(*s)) { s++; }			// Skip whitespace
#define FIND_WS(s) while (*s && !_istspace(*s)) { s++; }	// Find whitespace
#define FIND_QT(s) while (*s && *s != _T('\"')) { s++; }	// Find quote

// dat.cpp
#define DAT_ARCADE_ONLY		0
#define DAT_MEGADRIVE_ONLY	1
#define DAT_PCENGINE_ONLY	2
#define DAT_TG16_ONLY		3
#define DAT_SGX_ONLY		4
char* DecorateGameName(UINT32 nBurnDrv);
INT32 write_datfile(INT32 bType, FILE* fDat);
INT32 create_datfile(TCHAR* szFilename, INT32 bType);

// state.cpp
INT32 BurnStateLoadEmbed(FILE* fp, INT32 nOffset, INT32 bAll, INT32 (*pLoadGame)());
INT32 BurnStateLoad(TCHAR* szName, INT32 bAll, INT32 (*pLoadGame)());
INT32 BurnStateSaveEmbed(FILE* fp, INT32 nOffset, INT32 bAll);
INT32 BurnStateSave(TCHAR* szName, INT32 bAll);
extern int nSavestateSlot;
int StatedAuto(int bSave);
int StatedLoad(int nSlot);
int StatedSave(int nSlot);

// zipfn.cpp
struct ZipEntry { char* szName;	UINT32 nLen; UINT32 nCrc; };

INT32 ZipOpen(char* szZip);
INT32 ZipClose();
INT32 ZipGetList(struct ZipEntry** pList, INT32* pnListCount);
INT32 ZipLoadFile(UINT8* Dest, INT32 nLen, INT32* pnWrote, INT32 nEntry);
INT32 __cdecl ZipLoadOneFile(char* arcName, const char* fileName, void** Dest, INT32* pnWrote);

// bzip.cpp
#define BZIP_STATUS_OK		(0)
#define BZIP_STATUS_BADDATA	(1)
#define BZIP_STATUS_ERROR	(2)

INT32 BzipOpen(bool);
INT32 BzipClose();
INT32 BzipInit();
INT32 BzipExit();
INT32 BzipStatus();

// paths.cpp
extern char szAppHomePath[MAX_PATH];
extern char szAppSavePath[MAX_PATH];
extern char szAppConfigPath[MAX_PATH];
extern char szAppSamplesPath[MAX_PATH]; // for burn/snd/samples.cpp
void BurnPathsInit();

#endif // _BURNER_H_
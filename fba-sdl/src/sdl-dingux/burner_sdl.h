#include "SDL/SDL.h"

#ifndef RECT
typedef struct tagRECT {
	int left;
	int top;
	int right;
	int bottom;
} RECT,*PRECT,*LPRECT;
typedef const RECT *LPCRECT;
#endif

#ifndef MAX_PATH
#define MAX_PATH 511
#endif

#ifndef DWORD
typedef unsigned int DWORD;
#endif

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef __cdecl
#define	__cdecl
#endif

// fba_player.cpp
extern char szAppBurnVer[16];

// main.cpp
extern bool bRunPause;
TCHAR* ANSIToTCHAR(const char* pszInString, TCHAR* pszOutString, int nOutSize);
char* TCHARToANSI(const TCHAR* pszInString, char* pszOutString, int nOutSize);

// config.cpp
typedef struct
{
	int option_sound_enable;
	int option_rescale;
	int option_rotate;
	int option_samplerate;
	int option_showfps;
	int option_frameskip;
	int option_68kcore;
	int option_z80core;
	int option_sense;
	int option_useswap;
	char option_frontend[MAX_PATH];
	int option_create_lists;
} CFG_OPTIONS;

typedef struct
{
	int up;
	int down;
	int left;
	int right;
	int fire1;
	int fire2;
	int fire3;
	int fire4;
	int fire5;
	int fire6;
	int coin1;
	int coin2;
	int start1;
	int start2;
	int pause;
	int quit;
	int qsave;
	int qload;
} CFG_KEYMAP;

extern CFG_OPTIONS config_options;
extern CFG_KEYMAP config_keymap;

int ConfigAppLoad();
int ConfigAppSave();
int ConfigGameLoad();
int ConfigGameSave();

// drv.cpp
extern int bDrvOkay; // 1 if the Driver has been initted okay, and it's okay to use the BurnDrv functions
extern char szAppRomPaths[DIRS_MAX][MAX_PATH];
int DrvInit(int nDrvNum, bool bRestore);
int DrvInitCallback(); // Used when Burn library needs to load a game. DrvInit(nBurnSelect, false)
int DrvExit();
int ProgressUpdateBurner(double dProgress, const TCHAR* pszText, bool bAbs);
int AppError(TCHAR* szText, int bWarning);

// run.cpp
extern bool bShowFPS;
extern bool bPauseOn;
int RunReset();
int RunOneFrame(bool bDraw, int fps);

// input.cpp
extern int nAnalogSpeed;
int InpInit();
int InpExit();
void InpDIP();

// stringset.cpp
class StringSet {
public:
	TCHAR* szText;
	int nLen;
	// printf function to add text to the Bzip string
	int __cdecl Add(TCHAR* szFormat, ...);
	int Reset();
	StringSet();
	~StringSet();
};


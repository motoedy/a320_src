// Driver Init module
#include "burner.h"
#include "snd.h"

int bDrvOkay = 0;						// 1 if the Driver has been initted okay, and it's okay to use the BurnDrv functions

char szAppRomPaths[DIRS_MAX][MAX_PATH] = {{"/usr/local/share/roms/"},{"roms/"}, };

static bool bSaveRAM = false;

static int DoLibInit()					// Do Init of Burn library driver
{
	int nRet;

	BzipOpen(false);

	//ProgressCreate();

	nRet = BurnDrvInit();

	BzipClose();

	if (nRet) {
		return 1;
	} else {
		return 0;
	}
}

// Catch calls to BurnLoadRom() once the emulation has started;
// Intialise the zip module before forwarding the call, and exit cleanly.
static int DrvLoadRom(unsigned char* Dest, int* pnWrote, int i)
{
	int nRet;

	BzipOpen(false);

	if ((nRet = BurnExtLoadRom(Dest, pnWrote, i)) != 0) {
		char szText[256] = "";
		char* pszFilename;

		BurnDrvGetRomName(&pszFilename, i, 0);
		sprintf(szText, "Error loading %s, requested by %s.\nThe emulation will likely suffer problems.", pszFilename, BurnDrvGetTextA(0));
	}

	BzipClose();

	BurnExtLoadRom = DrvLoadRom;

	//ScrnTitle();

	return nRet;
}

int DrvInit(int nDrvNum, bool bRestore)
{
	DrvExit();						// Make sure exitted
	nBurnDrvSelect[0] = nDrvNum;		// Set the driver number

	// Define nMaxPlayers early; GameInpInit() needs it (normally defined in DoLibInit()).
	nMaxPlayers = BurnDrvGetMaxPlayers();
//	GameInpInit();					// Init game input

	ConfigGameLoad();
//	InputMake(true);

//	GameInpDefault();
	SndInit();
	//SndOpen();

	if (DoLibInit()) {				// Init the Burn library's driver
		char szTemp[512];

		BurnDrvExit();				// Exit the driver

		_stprintf (szTemp, _T("There was an error starting '%s'.\n"), BurnDrvGetText(DRV_FULLNAME));
		return 1;
	}

	//SndInit();
	SndOpen();

	BurnExtLoadRom = DrvLoadRom;

	bDrvOkay = 1;					// Okay to use all BurnDrv functions

	bSaveRAM = false;
	nBurnLayer = 0xFF;				// show all layers

	// Reset the speed throttling code, so we don't 'jump' after the load
	RunReset();
	return 0;
}

int DrvInitCallback()
{
	return DrvInit(nBurnDrvSelect[0], false);
}

int DrvExit()
{
	if (bDrvOkay) {
//		VidExit();

		if (nBurnDrvSelect[0] < nBurnDrvCount) {
			if (bSaveRAM) {

				bSaveRAM = false;
			}

			ConfigGameSave();

//			GameInpExit();				// Exit game input
			BurnDrvExit();				// Exit the driver
		}
	}

	BurnExtLoadRom = NULL;

	bDrvOkay = 0;					// Stop using the BurnDrv functions

//	if (bAudOkay) {
//		// Write silence into the sound buffer on exit, and for drivers which don't use pBurnSoundOut
//		memset(nAudNextSound, 0, nAudSegLen << 2);
//	}
	SndExit();

	nBurnDrvSelect[0] = ~0U;			// no driver selected

	return 0;
}

int ProgressUpdateBurner(double dProgress, const TCHAR* pszText, bool bAbs)
{
	printf(".");
	return 0;
}

int AppError(TCHAR* szText, int bWarning)
{
	return 0;
}


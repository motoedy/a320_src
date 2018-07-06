/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

#include "sdlport.h"
#include "packfile.h"
#include "ram.h"
#include "video.h"
#include "menu.h"

#ifdef OSX
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef SDL
#define appExit exit
#undef exit
#endif

#ifdef WIN
#undef main
#endif

char packfile[128] = {"bor.pak"};

void borExit(int reset)
{
	tracemalloc_dump();

#ifdef GP2X
	gp2x_end();
	chdir("/usr/gp2x");
	execl("/usr/gp2x/gp2xmenu", "/usr/gp2x/gp2xmenu", NULL);
#else
	SDL_Delay(1000);
#endif

	appExit(0);
}

int main(int argc, char *argv[])
{
#ifdef OSX
	char resourcePath[PATH_MAX];
	CFBundleRef mainBundle;
	CFURLRef resourcesDirectoryURL;
	mainBundle = CFBundleGetMainBundle();
	resourcesDirectoryURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
	if(!CFURLGetFileSystemRepresentation(resourcesDirectoryURL, true, (UInt8 *) resourcePath, PATH_MAX))
	{
		borExit(0);
	}
	CFRelease(resourcesDirectoryURL);
	chdir(resourcePath);
#endif
	

	//Debug
	fprintf(stderr,"Dentro do Main\n");
	setSystemRam();
fprintf(stderr,"setSystemRam\n");
	initSDL();
fprintf(stderr,"initSDL\n");
	packfile_mode(0);
fprintf(stderr,"packfile_mode\n");

	dirExists("Paks", 1);
fprintf(stderr,"Paks\n");
	dirExists("Saves", 1);
fprintf(stderr,"Saves\n");
	dirExists("Logs", 1);
fprintf(stderr,"Logs\n");
	dirExists("ScreenShots", 1);
fprintf(stderr,"ScreenShots\n");

	Menu();
fprintf(stderr,"Menu\n");
	openborMain();
fprintf(stderr,"openBorMain\n");
	borExit(0);
fprintf(stderr,"Exit\n");
	return 0;
}

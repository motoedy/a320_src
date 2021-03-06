/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include "utils.h"
#include "stristr.h"
#include "openbor.h"
#include "packfile.h"

#if PSP || WIN || LINUX || GP2X || DINGOO
#include <dirent.h>
#endif

#if GP2X || LINUX || DINGOO
#include <sys/stat.h>
#endif

#ifdef DOS
#include <direct.h>
#include "dosport.h"
#include "savepcx.h"
#endif

#ifdef SDL
#include <unistd.h>
#include "sdlport.h"
#include "savepcx.h"
#endif

#ifdef DC
#include "dcport.h"
#endif

#ifdef XBOX
#include "xboxport.h"
#include "savepcx.h"
#endif

#ifdef PSP
#include "image.h"
#endif

#if PSP || GP2X || LINUX || DINGOO
#define MKDIR(x) mkdir(x, 0777)
#else
#define MKDIR(x) mkdir(x)
#endif

#ifdef XBOX
#define CHECK_LOGFILE(type)  type ? fileExists("d:\\Logs\\OpenBorLog.txt") : fileExists("d:\\Logs\\ScriptLog.txt")
#define OPEN_LOGFILE(type)   type ? fopen("d:\\Logs\\OpenBorLog.txt", "wt") : fopen("d:\\Logs\\ScriptLog.txt", "wt")
#define APPEND_LOGFILE(type) type ? fopen("d:\\Logs\\OpenBorLog.txt", "at") : fopen("d:\\Logs\\ScriptLog.txt", "at")
#define READ_LOGFILE(type)   type ? fopen("d:\\Logs\\OpenBorLog.txt", "rt") : fopen("d:\\Logs\\ScriptLog.txt", "rt")
#define COPY_ROOT_PATH(buf, name) strncpy(buf, "d:\\", 3); strncat(buf, name, strlen(name)); strncat(buf, "\\", 1)
#define COPY_PAKS_PATH(buf, name) strncpy(buf, "d:\\Paks\\", 8); strncat(buf, name, strlen(name))
#else
#define CHECK_LOGFILE(type)  type ? fileExists("./Logs/OpenBorLog.txt") : fileExists("./Logs/ScriptLog.txt")
#define OPEN_LOGFILE(type)   type ? fopen("./Logs/OpenBorLog.txt", "wt") : fopen("./Logs/ScriptLog.txt", "wt")
#define APPEND_LOGFILE(type) type ? fopen("./Logs/OpenBorLog.txt", "at") : fopen("./Logs/ScriptLog.txt", "at")
#define READ_LOGFILE(type)   type ? fopen("./Logs/OpenBorLog.txt", "rt") : fopen("./Logs/ScriptLog.txt", "rt")
#define COPY_ROOT_PATH(buf, name) strncpy(buf, "./", 2); strncat(buf, name, strlen(name)); strncat(buf, "/", 1);
#define COPY_PAKS_PATH(buf, name) strncpy(buf, "./Paks/", 7); strncat(buf, name, strlen(name));
#endif

char debug_msg[2048];
unsigned long debug_time = 0xFFFFFFFF;

void getBasePath(char *newName, char *name, int type)
{
#ifndef DC
	char buf[128] = {""};
	switch(type)
	{
		case 0:
			COPY_ROOT_PATH(buf, name);
			break;
		case 1:
			COPY_PAKS_PATH(buf, name);
			break;
	}
	memcpy(newName, buf, sizeof(buf));
#else
	memcpy(newName, name, 128);
#endif
}



#ifndef DC
int dirExists(char *dname, int create)
{
	char realName[128] = {""};
#ifdef XBOX
	getBasePath(realName, dname, 0);
	return CreateDirectory(realName, NULL);
#else
	DIR	*fd1 = NULL;
	int  fd2 = -1;
	strncpy(realName, dname, 128);
	fd1 = opendir(realName);
	if(fd1 != NULL)
	{
		closedir(fd1);
		return 1;
	}
	if(create)
	{
		fd2 = MKDIR(realName);
		if(fd2 < 0) return 0;
#ifdef MAC
		chmod(realName, 0777);
#endif
	}
#endif
	return 0;
}

int fileExists(char *fnam)
{
	FILE *handle = NULL;
	if((handle=fopen(fnam,"rb")) == NULL) return 0;
	fclose(handle);
	return 1;
}

char* readFromLogFile(int which)
{
	long  size;
    int disCcWarns;
	FILE* handle = NULL;
	char* buffer = NULL;
	handle = READ_LOGFILE((which ? OPENBOR_LOG : SCRIPT_LOG));
	if(handle == NULL) return NULL;
	fseek(handle, 0, SEEK_END);
	size = ftell(handle);
	rewind(handle);
	// allocate memory to contain the whole file:
	buffer = (char*)tracemalloc("readFromLogFile()", sizeof(char)*size);
	if(buffer == NULL) return NULL;
	disCcWarns = fread(buffer, 1, size, handle);
	fclose(handle);
	return buffer;
}
#endif

void writeToLogFile(const char *msg, ...)
{
#ifndef DC
	static int removeLog = 1;
    int disCcWarns;
	FILE *handle = NULL;
	char buf[1024] = "";
	va_list arglist;
    if(!savedata.uselog) return;
	va_start(arglist, msg);
	vsprintf(buf, msg, arglist);
	va_end(arglist);

	if(CHECK_LOGFILE(OPENBOR_LOG))
	{
		if(removeLog)
		{
			handle = OPEN_LOGFILE(OPENBOR_LOG);
			removeLog = 0;
		}
		else handle = APPEND_LOGFILE(OPENBOR_LOG);
	}
	else
	{
		handle = OPEN_LOGFILE(OPENBOR_LOG);
		removeLog = 0;
	}
	if(handle == NULL) return;
	disCcWarns = fwrite(buf, 1, strlen(buf), handle);
	fclose(handle);
#endif
}

void writeToScriptLog(const char *msg)
{
#ifndef DC
    int disCcWarns;
	static int removeLog = 1;
	FILE *handle = NULL;
    if(!savedata.uselog) return;
	if(CHECK_LOGFILE(SCRIPT_LOG))
	{
		if(removeLog)
		{
			handle = OPEN_LOGFILE(SCRIPT_LOG);
			removeLog = 0;
		}
		else handle = APPEND_LOGFILE(SCRIPT_LOG);
	}
	else
	{
		handle = OPEN_LOGFILE(SCRIPT_LOG);
		removeLog = 0;
	}
	if(handle == NULL) return;
	disCcWarns = fwrite(msg, 1, strlen(msg), handle);
	fclose(handle);
#endif
}

void debug_printf(char *format, ...){
    va_list arglist;

    va_start(arglist, format);
    vsprintf(debug_msg, format, arglist);
    va_end(arglist);

    debug_time = 0xFFFFFFFF;
}

void getPakName(char name[256], int type){

	int i,x,y;
	char mod[256] = {""};

	strncpy(mod,packfile,strlen(packfile)-4);

	switch(type){
		case 0:
			strncat(mod,".sav",4);
			break;
		case 1:
			strncat(mod,".hi",3);
			break;
		case 2:
            strncat(mod,".scr",4);
            break;
		case 3:
            strncat(mod,".inp",4);
            break;
		case 4:
            strncat(mod,".cfg",4);
            break;
		default:
			// Loose extension!
			break;
	}

	x=0;
	for(i=0; i<(int)strlen(mod); i++){
		if((mod[i] == '/') || (mod[i] == '\\')) x = i;
	}
	y=0;
	for(i=0; i<(int)strlen(mod); i++){
		// For packfiles without '/'
		if(x == 0){
			name[y] = mod[i];
			y++;
		}
		// For packfiles with '/'
		if(x != 0 && i > x){
			name[y] = mod[i];
			y++;
		}
	}
}

void screenshot(s_screen *vscreen, unsigned char *pal, int ingame){
#ifndef DC
	int	 shotnum = 0;
	char shotname[128] = {""};
	char modname[128]  = {""};

    getPakName(modname,99);
#ifdef PSP
	if(dirExists("ms0:/PICTURE/", 1) && dirExists("ms0:/PICTURE/Beats Of Rage/", 1)){
#endif
		do{
#ifdef PSP
			sprintf(shotname, "ms0:/PICTURE/Beats Of Rage/%s - ScreenShot - %02u.png", modname,shotnum);
#endif

#ifdef DOS
			sprintf(shotname, "./SShots/s%04u.pcx", shotnum);
#endif

#ifdef XBOX
			sprintf(shotname, "d:\\ScreenShots\\%s - %04u.pcx", modname,shotnum);
#elif SDL
			sprintf(shotname, "./ScreenShots/%s - %04u.pcx", modname,shotnum);
#endif
			++shotnum;
		}while(fileExists(shotname) && shotnum<100);

#ifdef PSP
		if(shotnum<10000) saveImage(shotname);
#else
		if(shotnum<10000) savepcx(shotname, vscreen, pal);
#endif
		if(ingame) debug_printf("Saved %s", shotname);
#ifdef PSP
	}
#endif
#endif
}

#ifdef XBOX
int findmods(void)
{
	int i = 0;
	HANDLE hFind;
	WIN32_FIND_DATAA oFindData;
	hFind = FindFirstFile("d:\\Paks\\*", &oFindData);
	if(hFind == INVALID_HANDLE_VALUE) return 1;
	do
	{
		if(stristr(oFindData.cFileName, ".pak") && stricmp(oFindData.cFileName, "menu.pak") != 0)
		{
			strncpy(paklist[i].filename, oFindData.cFileName, 128-strlen(oFindData.cFileName));
			i++;
		}
	}
	while(FindNextFile(hFind, &oFindData));
	FindClose(hFind);
	return i;
}
#endif

unsigned readlsb32(const unsigned char *src)
{
    return
        ((((unsigned)(src[0])) & 0xFF) <<  0) |
		((((unsigned)(src[1])) & 0xFF) <<  8) |
		((((unsigned)(src[2])) & 0xFF) << 16) |
		((((unsigned)(src[3])) & 0xFF) << 24);
}

// Optimized search in an arranged string table, return the index
int searchList(const char* list[], const char* value, int length)
{
    int i;
	int a = 0;
	int b = length / 2;
	int c = length - 1;
	int v = value[0];
	
	// We must convert uppercase values to lowercase,
	// since this is how every command is written in
	// our source.  Refer to an ASCII Chart
	if(v >= 0x41 && v <= 0x5A) v += 0x20;

	// Index value equals middle value,
	// Lets search starting from center.
	if(v == list[b][0])
	{
		if(stricmp(list[b], value) == 0) return b;

		// Search Down the List.
		if(v == list[b-1][0])
		{
			for(i=b-1 ; i>=0; i--)
			{
				if(stricmp(list[i], value) == 0) return i;
				if(v != list[i-1][0]) break;
			}
		}

		// Search Up the List.
		if(v == list[b+1][0])
		{
            for(i=b+1; i<length; i++)
			{
				if(stricmp(list[i], value) == 0) return i;
				if(v != list[i+1][0]) break;
			}
		}

		// No match, return failure.
		goto searchListFailed;
	}

	// Define the starting point.
	if(v >= list[b+1][0]) a = b+1;
	else if(v <= list[b-1][0]) c = b-1;
	else goto searchListFailed;

	// Search Up from starting point.
	for(i=a; i<=c; i++)
	{
		if(v == list[i][0])
		{
            if(stricmp(list[i], value) == 0) return i;
			if(v != list[i+1][0]) break;
		}
	}

searchListFailed:

	// The search failed!
	// On five reasons for failure!
	// 1. Is the list in alphabetical order?  
	// 2. Is the first letter lowercase in list?
	// 3. Does the value exist in the list?
	// 4. Is it a typo?
	// 5. Is it a text file error?
	return -1;
}

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>


#if !defined(__WIN32__)
#include <dirent.h>
#include <unistd.h>
#else
#include <windows.h>
#endif


#if defined(ARM_ARCH)
#include <fcntl.h>
#include <sys/ioctl.h>

#if !defined(GIZMONDO) && !defined(POCKETPC) && !defined(IPHONE)
  #include <sys/mman.h>
  #include <sys/soundcard.h>
  #include <linux/fb.h>
  #include <dlfcn.h>
#endif

#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#endif

#include "common.h"
#include "gui.h"
#include "sio.h"
#include "gp2x_cpu.h"
int psx4all_emulating=0;
s8 iphonefile[256];
s8 svsfilename[260];
s8* packfile = NULL;
s8 gamepath[256]={ '.', '/', 0, 0 };
s32 LoadCdBios;
PsxConfig Config;
FILE* emuLog;
s32 Log=0;
u32 gpuDisp;
s32 cdOpenCase = 0;
s32 ShowPic=0;
s32 StatesC = 0;
void gp2x_sound_frame(void *blah, void *buff, int samples) {}

#define PSX4ALL_MENU_START_POS		50

#define PSX4ALL_MENU_DEFAULT_STATE	0
#define PSX4ALL_MENU_GPU_STATE		1
#define PSX4ALL_MENU_SPU_STATE		2
#define PSX4ALL_MENU_GAMESTATE_STATE	3

/* EXTERNS FOR MENU SYSTEM	*/
/* NEEDS A BETTER PLACE		*/
extern int iSoundMuted;
extern int linesInterlace_user;
extern bool enableFrameLimit;
extern bool enableAbbeyHack;
extern bool displayFrameInfo;
extern bool displayGpuStats;
extern bool displayVideoMemory;
extern bool activeNullGPU;
extern int  skipCount;
extern int  skipRate;
int  skipValue = 0;
#ifdef IPHONE
static int  skipCountTablePhone[4] 	= { 0,2,3,4 };
static int  skipRateTablePhone[4] 	= { 1,3,4,5 };
#endif
int  skipCountTable[9] =  { 0,1,3,2,4,7,10,15,17 };
int  skipRateTable[9] 	= { 1,2,5,3,5,8,11,16,18 };

extern bool hardframeskip_line;
extern bool hardframeskip_poly;
extern bool hardframeskip_sprite;
extern bool hardframeskip_image;
extern bool hardframeskip_blit;
extern bool use_wall_clock_time;
#define MAX_CLOCK_VALUES 10
static int clock_values[MAX_CLOCK_VALUES] = { 133, 222, 266, 300, 336, 360, 380 ,400,420 };
int psp_cpu_clock = 336;
static void ChangeWorkingDirectory(char *exe)
{
#ifndef __WIN32__
	s8 *s = strrchr(exe, '/');
	if (s != NULL) {
		*s = '\0';
		chdir(exe);
		*s = '/';
	}
#endif
}

struct dir_item {
	s8	*name;
	s32 type; 
};

void sort_dir(struct dir_item *list, int num_items, int sepdir) {
	s32 i;
	struct dir_item temp;

	for(i=0; i<(num_items-1); i++) {
		if(strcmp(list[i].name, list[i+1].name)>0) {
			temp=list[i];
			list[i]=list[i+1];
			list[i+1]=temp;
			i=0;
		}
	}
	if(sepdir) {
		for(i=0; i<(num_items-1); i++) {
			if((list[i].type!=0)&&(list[i+1].type==0)) {
				temp=list[i];
				list[i]=list[i+1];
				list[i+1]=temp;
				i=0;
			}
		}
	}
}

static s8 filereq_fullgamepath[257];
static struct dir_item filereq_dir_items[1024] = {{0,0},};

char *FileReq(char *dir, const char *ext)
{
	static s8 *cwd=NULL;
	static s32 cursor_pos=1;
	static s32 first_visible;
	static s32 num_items=0;
	DIR *dirstream;
	struct dirent *direntry;
	s8 *path;
	struct stat item;
	static s32 row;
	s32 pathlength;
	s8 tmp_string[32];
	s8 *selected;
	u32 keys;
#define MENU_Y 90
#define MENU_LS MENU_Y+10
#define MENU_HEIGHT 12

	if(dir!=NULL) cwd=dir;
	if(cwd==NULL)
	{
		sprintf(filereq_fullgamepath, "%s/", gamepath);
		cwd=filereq_fullgamepath;
	}


	for(;;)
	{
		BACKSCREEN;
		keys = gp2x_joystick_read();

		gp2x_video_RGB_clearscreen16();

		gp2x_printf(NULL, 0, 10,	"psx4all V.by flpstrquerendoumdingoo!!!");
		gp2x_printf(NULL, 0, 20,	"CREDITS: UNAI - ZODTTD - HLIDE - CHUI - TINNUS");
		gp2x_printf(NULL, 0, 30,        "Dingoo/MIPS port by Ulrich Hecht");
		gp2x_printf(NULL, 0, 40,	"CHOOSE A GAME OR PRESS L TO EXIT");

		if( keys & GP2X_L )
		{
			for(int i=0; i<num_items; i++) if (filereq_dir_items[i].name)  { free(filereq_dir_items[i].name); filereq_dir_items[i].name = NULL; }
			num_items=0;
			gp2x_timer_delay(100);
			return NULL;
		}

		if(num_items==0) {
				s32 i2;
				dirstream=opendir(cwd);
				if(dirstream==NULL) {
					gp2x_printf(NULL, 0, 20, "error opening directory");
					return NULL;
				}
				while((direntry=readdir(dirstream))) {
					if( (ext == NULL &&
						 ((NULL == strstr(direntry->d_name, ".")) ||
						 (strlen(direntry->d_name) > 1 && 0 == strnicmp(direntry->d_name, "..", 2))								  ||
						 (strlen(direntry->d_name) > 2 && 0 == strnicmp(direntry->d_name+(strlen(direntry->d_name)-2), ".z", 2))  ||
						 (strlen(direntry->d_name) > 4 && 0 == strnicmp(direntry->d_name+(strlen(direntry->d_name)-4), ".iso", 4)) ||
						 (strlen(direntry->d_name) > 4 && 0 == strnicmp(direntry->d_name+(strlen(direntry->d_name)-4), ".bin", 4)) ||
						 (strlen(direntry->d_name) > 4 && 0 == strnicmp(direntry->d_name+(strlen(direntry->d_name)-4), ".img", 4)) ||
						 (strlen(direntry->d_name) > 4 && 0 == strnicmp(direntry->d_name+(strlen(direntry->d_name)-4), ".znx", 4)) ||
						 (strlen(direntry->d_name) > 4 && 0 == strnicmp(direntry->d_name+(strlen(direntry->d_name)-4), ".cbn", 4)) ||
						 (strlen(direntry->d_name) > 4 && 0 == strnicmp(direntry->d_name+(strlen(direntry->d_name)-4), ".pbp", 4)) ||
						(strlen(direntry->d_name) > 4 && 0 == strnicmp(direntry->d_name+(strlen(direntry->d_name)-4), ".cue", 4)) ||
 						 (strlen(direntry->d_name) > 4 && 0 == strnicmp(direntry->d_name+(strlen(direntry->d_name)-4), ".mdf", 4))) ) ||						
(ext != NULL && (strlen(direntry->d_name) > 4 && 0 == strnicmp(direntry->d_name+(strlen(direntry->d_name)-strlen(ext)), ext, strlen(ext)))) )
					{
						 filereq_dir_items[num_items].name=(s8 *)malloc(strlen(direntry->d_name)+1);
						strcpy(filereq_dir_items[num_items].name, direntry->d_name);
						num_items++;
						if(num_items>1024) break;
					}
				}
				closedir(dirstream);
				for(i2=0; i2<num_items; i2++) {
					path=(s8 *)malloc(strlen(cwd)+strlen(filereq_dir_items[i2].name)+2);
					sprintf(path, "%s/%s", cwd, filereq_dir_items[i2].name);
					if(!stat(path, &item)) {
						if(S_ISDIR(item.st_mode)) {
							filereq_dir_items[i2].type=0;
						}
						else
						{
							s32 len = strlen(filereq_dir_items[i2].name);

							filereq_dir_items[i2].type = 2;
							if (len >= 4)
							{
								if (!strnicmp(filereq_dir_items[i2].name+(len-2), ".Z", 2))
								{
									filereq_dir_items[i2].type = 1;
								}
								if (!strnicmp(filereq_dir_items[i2].name+(len-4), ".bin", 4))
								{
									filereq_dir_items[i2].type = 1;
								}
								if (!strnicmp(filereq_dir_items[i2].name+(len-4), ".ZNX", 4))
								{
									filereq_dir_items[i2].type = 1;
								}
							}
						}
					} else {
						filereq_dir_items[i2].type=0;
					}
					free(path);
				}
			sort_dir(filereq_dir_items, num_items, 1);
			cursor_pos=0;
			first_visible=0;
		}

		gp2x_printf(NULL, 80, MENU_Y, cwd);

		if(keys & GP2X_DOWN) { 
				if(cursor_pos<(num_items-1)) cursor_pos++;
				if((cursor_pos-first_visible)>=MENU_HEIGHT) first_visible++;
		}
		else if(keys & GP2X_UP) { 
				if(cursor_pos>0) cursor_pos--;
				if(cursor_pos<first_visible) first_visible--;
		}
		else if(keys & GP2X_LEFT) { 
				if(cursor_pos>=10) cursor_pos-=10;
				else cursor_pos=0;
				if(cursor_pos<first_visible) first_visible=cursor_pos;
		}
		else if(keys & GP2X_RIGHT) {
				if(cursor_pos<(num_items-11)) cursor_pos+=10;
				else cursor_pos=num_items-1;
				if((cursor_pos-first_visible)>=MENU_HEIGHT)
					first_visible=cursor_pos-(MENU_HEIGHT-1);
		}
		else if(keys & GP2X_B) 
		{
		
				path=(s8 *)malloc(strlen(cwd)
				+strlen(filereq_dir_items[cursor_pos].name)
				+2);
			sprintf(path, "%s/%s", cwd, filereq_dir_items[cursor_pos].name);
			for(int i=0; i<num_items; i++) if (filereq_dir_items[i].name)  { free(filereq_dir_items[i].name); filereq_dir_items[i].name = NULL; }
			num_items=0;
			if(filereq_dir_items[cursor_pos].type==0) {
				pathlength=strlen(path);
				if(	path[pathlength-1]=='.' &&
					path[pathlength-2]=='/') 
				{
					path[pathlength-2]='\0';
					cwd=path;
				}
				else if(path[pathlength-1]=='.'
						&& path[pathlength-2]=='.'
						&& path[pathlength-3]=='/' ) 
				{
					if( pathlength > 4 )
					{
						s8* p = strrchr(path, '/');	
						p[0] = '\0';
						p = strrchr(path, '/');			
						p[0] = '\0';
						p = strrchr(path, '/');			
						p[1] = '\0';					
						cwd=path;
					}
				}
				else
				{
					if( path[0] == '/' &&
						path[1] == '/' )
					{
						cwd = path + 1; 
					}
					else
					{
						cwd = path;
					}
				}
			}
			else
			{
				BACKSCREEN;
				gp2x_video_RGB_clearscreen16();
				gp2x_printf(NULL, 10, 120, "ARE YOU SURE YOU WANT TO SELECT...");
				gp2x_printf(NULL, 10, 130, "%s", path);
				gp2x_printf(NULL, 10, 140, "PRESS START FOR YES OR SELECT FOR NO");
				FRONTSCREEN;
				gp2x_video_flip();
				for(;;)
				{
					u32 keys = gp2x_joystick_read();
					if( keys & GP2X_SELECT )
					{
						return NULL;
					}
					if( keys & GP2X_START )
					{
						char* p = strrchr(path, '/');
						if( p != NULL )
						{
							sprintf(CdromLabel, "%10.10s", p + 1);
						}
						return path;
					}

					gp2x_timer_delay(100);
				}
			}
		}

		row=0;
		while(row<num_items && row<MENU_HEIGHT) {
			if(row==(cursor_pos-first_visible)) {
				gp2x_printf(NULL, 80, MENU_LS+(10*row), "------>");

				selected=filereq_dir_items[row+first_visible].name;
			}

			if(filereq_dir_items[row+first_visible].type==0)
			{
        		gp2x_printf(NULL, 80, MENU_LS+(10*row), "DIR ");
			}
			snprintf(tmp_string, 30, "%s", filereq_dir_items[row+first_visible].name);
			gp2x_printf(NULL, 80+(10*6), MENU_LS+(10*row), tmp_string);
			row++;
		}
		while(row<MENU_HEIGHT) {
			row++;
		}

		FRONTSCREEN;
		gp2x_video_flip();
		gp2x_timer_delay(75);

		if(keys & (GP2X_A|GP2X_B|GP2X_X|GP2X_Y|GP2X_L|GP2X_R|GP2X_PUSH|
			GP2X_LEFT|GP2X_RIGHT|GP2X_UP|GP2X_DOWN) )
		{
			gp2x_timer_delay(50);
		}
	}

	return NULL;
}

s32 SelectGame()
{
	u32 keys;
	s32 loadst = 0;

	s32 menu_state = 0;
	s32 menu_pos = 0;
	s8* newpackfile = NULL;

	gp2x_video_RGB_clearscreen16();
	gp2x_video_flip();

#if 1
	for(;;)
	{
		BACKSCREEN;
		gp2x_video_RGB_clearscreen16();

		gp2x_printf(NULL, 0, 10, "psx4all V.by flpstrquerendoumdingoo!!!");
		gp2x_printf(NULL, 0, 20, "CREDITS: UNAI - ZODTTD - HLIDE - CHUI - TINNUS");
		gp2x_printf(NULL, 0, 30, "Dingoo/MIPS port by Ulrich Hecht");
		gp2x_printf(NULL, 0, 40, " psx4all------------Edition 6");

		keys = gp2x_joystick_read();

		gp2x_printf(NULL, 10, (menu_pos * 10) + PSX4ALL_MENU_START_POS, "----> ");

		if( keys & GP2X_UP )
		{
			if( menu_pos > 0 ) menu_pos--;
		}

		switch(menu_state)
		{
		case PSX4ALL_MENU_DEFAULT_STATE:
			if( keys & GP2X_DOWN )
			{
				if( menu_pos < 3 ) menu_pos++;
			}
			break;
		case PSX4ALL_MENU_GPU_STATE:
			if( keys & GP2X_DOWN )
			{
				if( menu_pos < 16 ) menu_pos++;
			}
			break;
		case PSX4ALL_MENU_SPU_STATE:
			if( keys & GP2X_DOWN )
			{
				if( menu_pos < 1 ) menu_pos++;
			}
			break;
		case PSX4ALL_MENU_GAMESTATE_STATE:
			if( keys & GP2X_DOWN )
			{
				if( menu_pos < 4 ) menu_pos++;
			}
			break;
		}

		switch(menu_state)
		{
		case PSX4ALL_MENU_DEFAULT_STATE:
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 0,	"GRAPHICS OPTIONS");
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 10,	"SOUND OPTIONS");
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 20,	"FILE OPTIONS");
			if (psx4all_emulating) gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 30, "RESUME EMULATION");
			else gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 30,	"QUIT");
			break;
		case PSX4ALL_MENU_GPU_STATE:
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS +  0,
				"Show FPS                 %s",
				(displayFrameInfo == false ? "OFF" : "ON"));
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 10,
				"Show GPU Stats           %s",
				(displayGpuStats == false ? "OFF" : "ON"));
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 20,
				"Display Video Memory     %s",
				(displayVideoMemory == false ? "OFF" : "ON"));
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 30,
				"Set NULL GPU             %s",
				(activeNullGPU == false ? "OFF" : "ON"));
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 40,
				"Interlace Count          %d",
				linesInterlace_user );
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 50,
				"Frame Limit              %s",
				(enableFrameLimit == false ? "OFF" : "ON"));
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 60,
				"Frame Skip               %d/%d",
				skipCount, skipRate);
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 70,
				"Abe's Oddysee Fix        %s",
				(enableAbbeyHack == false ? "OFF" : "ON"));
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 80,
				"Cycle Multiplier         %d",
				PsxCycleMult);
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 90,
				"Frameskip: Line          %s",
				hardframeskip_line ? "ON" : "OFF");
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 100,
				"Frameskip: Poly          %s",
				hardframeskip_poly ? "ON" : "OFF");
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 110,
				"Frameskip: Sprite        %s",
				hardframeskip_sprite ? "ON" : "OFF");
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 120,
				"Frameskip: Image         %s",
				hardframeskip_image ? "ON" : "OFF");
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 130,
				"Frameskip: Blit          %s",
				hardframeskip_blit ? "ON" : "OFF");
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 140,
				"Wall Clock Timing        %s",
				use_wall_clock_time ? "ON" : "OFF");
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 150,
				"Clock                    %d",
				psp_cpu_clock);
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 160, "<-Back");
			break;
		case PSX4ALL_MENU_SPU_STATE:
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 0,	"SOUND IS %s", (iSoundMuted == 0 ? "ON" : "OFF"));
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 10,	"BACK");
			break;
		case PSX4ALL_MENU_GAMESTATE_STATE:
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 0,	"SAVE GAME STATE" );
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 10,	"LOAD GAME STATE");
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 20,	"LOAD A GAME");
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 30,	"LOAD A GAME WITHOUT BIOS");
			gp2x_printf(NULL, 80, PSX4ALL_MENU_START_POS + 40,	"BACK");
			break;
		}
	
		switch(menu_state)
		{
		case PSX4ALL_MENU_DEFAULT_STATE:
			if( keys & GP2X_B )
			{
				switch(menu_pos)
				{
				case 0:
					menu_state = PSX4ALL_MENU_GPU_STATE;
					menu_pos = 0;
					break;
				case 1:
					menu_state = PSX4ALL_MENU_SPU_STATE;
					menu_pos = 0;
					break;
				case 2:
					menu_state = PSX4ALL_MENU_GAMESTATE_STATE;
					menu_pos = 0;
					break;
				case 3:
					gp2x_video_RGB_clearscreen16();
					FRONTSCREEN;
					return 0;
				default:
					break;
				}
			}
			if (keys & GP2X_L && psx4all_emulating) {
				gp2x_video_RGB_clearscreen16();
				FRONTSCREEN;
				return 0;
			}
			break;
		case PSX4ALL_MENU_GPU_STATE:
				switch(menu_pos)
				{
				case 0:
					if( keys & GP2X_B || keys & GP2X_LEFT || keys & GP2X_RIGHT )
					{
						displayFrameInfo = !displayFrameInfo;
					}
					break;
				case 1:
					if( keys & GP2X_B || keys & GP2X_LEFT || keys & GP2X_RIGHT )
					{
						displayGpuStats = !displayGpuStats;
					}
					break;
				case 2:
					if( keys & GP2X_B )
					{
						displayVideoMemory = !displayVideoMemory;
					}
					break;
				case 3:
					if( keys & GP2X_B )
					{
						activeNullGPU = !activeNullGPU;
					}
					break;
				case 4:
					if( keys & GP2X_RIGHT )
					{
						switch( linesInterlace_user )
						{
						case 0:
							linesInterlace_user = 1;
							break;
						case 1:
							linesInterlace_user = 2;
							break;
						case 2:
							linesInterlace_user = 3; 								break;
						case 3:
							linesInterlace_user = 4;
							break;
						case 4:
							linesInterlace_user = 5;
							break;
						case 5:
							linesInterlace_user = 6;
							break;
						case 6:
							linesInterlace_user = 7;
							break;
						case 7:
							linesInterlace_user = 0;
							break;
						}
					}
					break;
				case 5:
					if( keys & GP2X_B )
					{
						enableFrameLimit = !enableFrameLimit;
					}
					break;
				case 6:
					if( keys & GP2X_LEFT )
					{
						if( skipValue > 0 )
						{
							skipValue--;
							skipCount = skipCountTable[skipValue];
							skipRate = skipRateTable[skipValue];
						}
					}
					if( keys & GP2X_RIGHT )
					{
						if( skipValue < 8 )
						{
							skipValue++;
							skipCount = skipCountTable[skipValue];
							skipRate = skipRateTable[skipValue];
						}
					}
					break;
				case 7:
					if( keys & GP2X_B )
					{
						enableAbbeyHack = !enableAbbeyHack;
					}
					break;
				case 8:
					if (keys & GP2X_LEFT && PsxCycleMult > 1) PsxCycleMult--;
					if (keys & GP2X_RIGHT && PsxCycleMult < 18) PsxCycleMult++;
					break;
				case 9:
					if( keys & GP2X_B )
					{
						hardframeskip_line = !hardframeskip_line;
					}
					break;
				case 10:
					if( keys & GP2X_B )
					{
						hardframeskip_poly = !hardframeskip_poly;
					}
					break;
				case 11:
					if( keys & GP2X_B )
					{
						hardframeskip_sprite = !hardframeskip_sprite;
					}
					break;
				case 12:
					if( keys & GP2X_B )
					{
						hardframeskip_image = !hardframeskip_image;
					}
					break;
				case 13:
					if( keys & GP2X_B )
					{
						hardframeskip_blit = !hardframeskip_blit;
					}
					break;
				case 14:
					if( keys & GP2X_B )
					{
						use_wall_clock_time = !use_wall_clock_time;
					}
					break;

				case 15:
					if ( keys & GP2X_RIGHT  ) 
					{
					switch ( psp_cpu_clock )
						{
					case 180: 
						psp_cpu_clock = 200;
						break;
					case 200:
						psp_cpu_clock = 300;
						break;
					case 300: 
						psp_cpu_clock = 336;
						break;
					case 336:
						psp_cpu_clock = 380;
						break;
					case 380: 
						psp_cpu_clock = 390;
						break; 
					case 390:
						psp_cpu_clock = 400;
						break;
					case 400:
						psp_cpu_clock = 410;
						break;
					case 410:
						psp_cpu_clock = 420;
						break;
					case 420:
						psp_cpu_clock = 430;
						break;
					case 430:
						psp_cpu_clock = 180;
						break;
					default:
						psp_cpu_clock = 336;
						break;
						}
					}
					
					else {		
												
					if ( keys & GP2X_LEFT )
					{
 					switch ( psp_cpu_clock ) 
						{
					case 180: 
						psp_cpu_clock = 430;
						break;
					case 200: 
						psp_cpu_clock = 180;
						break;
					case 300: 
						psp_cpu_clock = 200;
						break;
					case 336:
						psp_cpu_clock = 300;
						break;
					case 380:
						psp_cpu_clock = 336;
						break;
					case 390:
						psp_cpu_clock = 380;
						break;
					case 400:
						psp_cpu_clock = 390;
						break;
					case 410:
						psp_cpu_clock = 400;
						break;
					case 420:
						psp_cpu_clock = 410;
						break;
					case 430:
						psp_cpu_clock = 420;
						break;
					default:
						psp_cpu_clock = 336;
						break;
						}
					}
					     }
					break;
				case 16:
					if( keys & GP2X_B )
					{
						menu_state = PSX4ALL_MENU_DEFAULT_STATE;
						menu_pos = 0;
					}
					break;
				default:
					break;
				}
				if (keys & GP2X_L) {
					menu_state = PSX4ALL_MENU_DEFAULT_STATE;
					menu_pos = 0;
				}
				break;
		case PSX4ALL_MENU_SPU_STATE:
			switch(menu_pos)
			{
				case 0:
					if( keys & GP2X_B || keys & GP2X_LEFT || keys & GP2X_RIGHT )
					{
#ifndef NOSOUND
						iSoundMuted = !iSoundMuted;
#endif
					}
					break;
				case 1:
					if( keys & GP2X_B )
					{
						menu_state = PSX4ALL_MENU_DEFAULT_STATE;
						menu_pos = 0;
					}
					break;
			}
			if (keys & GP2X_L) {
				menu_state = PSX4ALL_MENU_DEFAULT_STATE;
				menu_pos = 0;
			}
			break;
		case PSX4ALL_MENU_GAMESTATE_STATE:
			switch(menu_pos)
			{
				case 0:
					if( keys & GP2X_B )
					{
#ifndef IPHONE
						if( 1 == psx4all_emulating )
						{
							s32 ret;
							char buffer[360];
							char filename[260];

							struct stat s;
							for(int count = 1;; count++) {
								sprintf(filename, "%s-%04d.svs", packfile, count);
								if (stat(filename, &s)) break;
							}
							
							BACKSCREEN;
							gp2x_printf(NULL, 80, 130, "Saving...");
							FRONTSCREEN;
							gp2x_video_flip();
							GPU_freeze(2, NULL);
							ret = SaveState(filename);
							if (ret == 0)
								 sprintf(buffer, "Saved!");
							else sprintf(buffer, "Error Saving!");

							BACKSCREEN;
							gp2x_printf(NULL, 80, 140, "%s", buffer);
							FRONTSCREEN;
							gp2x_video_flip();
							gp2x_timer_delay(1000);
						}
#endif
					}
					break;
				case 1:
					if( keys & GP2X_B )
					{
#ifndef IPHONE
						gp2x_timer_delay(500);
						newpackfile = FileReq(NULL, ".svs");
#endif
					}
					break;
				case 2:
					if( keys & GP2X_B )
					{
#ifndef IPHONE
						Config.HLE = 0;
						gp2x_timer_delay(500);
						newpackfile = FileReq(NULL, NULL);
#endif
					}
					break;
				case 3:
					if( keys & GP2X_B )
					{
#ifndef IPHONE
						Config.HLE = 1;
						gp2x_timer_delay(500);
						newpackfile = FileReq(NULL, NULL);
#endif
					}
					break;
				case 4:
					if( keys & GP2X_B )
					{
						menu_state = PSX4ALL_MENU_DEFAULT_STATE;
						menu_pos = 0;
					}
					break;
			}
			if (keys & GP2X_L) {
				menu_state = PSX4ALL_MENU_DEFAULT_STATE;
				menu_pos = 0;
			}
			break;
		}

		if( newpackfile != NULL )
		{
			break;
		}
		FRONTSCREEN;
		gp2x_video_flip();
		gp2x_timer_delay(100);

		if(keys & (GP2X_A|GP2X_B|GP2X_X|GP2X_Y|GP2X_L|GP2X_R|GP2X_PUSH|
			GP2X_LEFT|GP2X_RIGHT|GP2X_UP|GP2X_DOWN) )
		{
			gp2x_timer_delay(50);
		}
	}
#else
	Config.HLE = 1;
	newpackfile = "Cotton Jap.cbn";
#endif
	DEBUGF("loaded %s", newpackfile);
	packfile = newpackfile;

	BACKSCREEN;
	gp2x_video_RGB_clearscreen16();

	keys = gp2x_joystick_read();

	LoadCdBios = 0;

	if( (!strcasecmp(packfile + (strlen(packfile)-4), ".svs")) )
	{
		char *pos;
		loadst = 1;
		sprintf(svsfilename, "%s", packfile);
		pos = strrchr(packfile, '-');
		if (pos) *pos = '\0';
	}
	else
	{
		loadst = 0;
	}

	if( loadst > 0 )
	{
		gp2x_printf(NULL, 120, 100, "LOADING SAVE STATE");
	}
	else
	{
		gp2x_printf(NULL, 120, 100, "LOADING BIOS");
	}

	FRONTSCREEN;
	gp2x_video_flip();

	if( 1 == psx4all_emulating )
	{
		psxShutdown();
		CloseComponents();
	}

	if (SysInit() == -1)
	{
		gp2x_deinit();
		return 0;
	}

	if (InitComponents() == -1)
	{
		gp2x_deinit();
		return 0;
	}

	SysReset();
	CheckCdrom();

	if( Config.HLE )
	{
		LoadCdBios = 0;
	 	if( LoadCdrom() == -1 )
		{
			BACKSCREEN;
			gp2x_printf(NULL, 120, 120, "LOAD FAILED");
			FRONTSCREEN;
			gp2x_video_flip();
			gp2x_timer_delay(2000);
			gp2x_video_RGB_clearscreen16();
			return 0;
		}
	}
	else
	{
		BACKSCREEN;
		gp2x_printf(NULL, 120, 120, "LOADED!");
		FRONTSCREEN;
		gp2x_video_flip();
		gp2x_timer_delay(100);
	}

	if (loadst) {
		if( LoadState(svsfilename) == -1 )
		{
			BACKSCREEN;
			gp2x_printf(NULL, 120, 120, "LOAD SAVE FAILED");
			FRONTSCREEN;
			gp2x_video_flip();
			gp2x_timer_delay(2000);
			gp2x_video_RGB_clearscreen16();
			return 0;
		}
	}

	return 1;
}

#if defined(IPHONE)
extern "C" int iphone_main(char* filename)
#else
int main(int argc, char *argv[])
#endif
{
#ifndef IPHONE
#if defined(ARM_ARCH)
	ChangeWorkingDirectory(argv[0]);
	getcwd(gamepath, 256);
#else
#if defined(__WIN32__)
	if(argc == 1)
		strncpy(gamepath,"E:\\ps1",256);
	else
		strncpy(gamepath,argv[1],256);
#else
	strncpy(gamepath,ROM_PREFIX,256);
#endif
#endif
#endif

#ifdef PSP
	sprintf(gamepath,"");
#endif

#ifdef IPHONE
	sprintf(gamepath,"");
#endif

	memset(&Config, 0, sizeof(PsxConfig));
	Config.PsxAuto = 1;
	Config.Cdda = 1;
	Config.Xa = 0;
#ifdef DYNAREC
	Config.Cpu = 0;
#else
	Config.Cpu = 1;
#endif

#ifdef WITH_HLE
	Config.HLE = 0;
#else
	Config.HLE = 0;
#endif

	Config.Mdec = 0;
	Config.PsxOut = 0;
	Config.PsxType = 0;
	Config.QKeys = 0;
	Config.RCntFix = 0;
	Config.Sio = 0;
	Config.SpuIrq = 1;
	Config.VSyncWA = 0;

#if defined(PSP) || defined(SDL) || defined(IPHONE)
    	sprintf(Config.BiosDir, "%s/Media/ROMs/PSX/", appworkdir);
  	sprintf(Config.Bios, "scph1001.bin");
	sprintf(Config.Mcd1, "mcd001.mcr");
	sprintf(Config.Mcd2, "mcd002.mcr");
#else
	sprintf(Config.BiosDir, "%s", gamepath);
	sprintf(Config.Bios, "/scph1001.bin");
	sprintf(Config.Mcd1, "%s/mcd001.mcr", gamepath);
	sprintf(Config.Mcd2, "%s/mcd002.mcr", gamepath);
	
#endif
	gp2x_init(1000, 16, 11025, 16, 1, 60, 1);
#ifndef GP2X
	gp2x_video_flip_single();
#endif

#ifdef IPHONE
	u32 loadsvs = 0;
	linesInterlace_user = preferences.interlace;
	skipCount = skipCountTablePhone[preferences.frameSkip];
	skipRate = skipRateTablePhone[preferences.frameSkip];   
	iSoundMuted = preferences.muted;
	Config.Cdda = preferences.muted;
	Config.Xa = preferences.muted;
#ifdef WITH_HLE
	Config.HLE = !preferences.bios;
#else
	Config.HLE = 0;
#endif
	if( (!strcasecmp(filename + (strlen(filename)-4), ".svs")) )
	{
		u32 pos;
		loadsvs = 1;
		sprintf(svsfilename, "%s", filename);
		sprintf(iphonefile, "%s", filename);
		pos = strlen(iphonefile)-18;
		iphonefile[pos] = '\0';
		packfile = iphonefile;
	}
	else
	{
		loadsvs = 0;
		sprintf(iphonefile, "%s", filename);
		packfile = iphonefile;
	}

	gp2x_video_RGB_clearscreen16();
	LoadCdBios = 0;

	if (SysInit() == -1)
	{
		gp2x_deinit();
		return 0;
	}

	if (InitComponents() == -1)
	{
		gp2x_deinit();
		return 0;
	}

	SysReset();
	CheckCdrom();

	if( Config.HLE )
	{
		LoadCdBios = 0;
		if( LoadCdrom() == -1 )
		{
			gp2x_printf(NULL, 120, 120, "LOAD FAILED");
			gp2x_video_flip();
			gp2x_timer_delay(2000);
			gp2x_video_RGB_clearscreen16();
			return 0;
		}
	}
	
	if (loadsvs) {
		if( LoadState(svsfilename) == -1 )
		{
			gp2x_printf(NULL, 120, 120, "LOAD SAVE FAILED");
			gp2x_video_flip();
			gp2x_timer_delay(2000);
			gp2x_video_RGB_clearscreen16();

			psxShutdown();
			CloseComponents();
			
			gp2x_deinit();
			pthread_exit(NULL);
		}
	}

	psx4all_emulating=1;
	psx4all_prof_start(PROFILER_TOTAL);
	psxCpu->Execute();
	psx4all_prof_end(PROFILER_TOTAL);
	psx4all_emulating=0;

	psx4all_prof_show();
#else
	if( 0 != SelectGame() )
	{
		psx4all_emulating=1;
		psx4all_prof_start(PROFILER_TOTAL);
		psxCpu->Execute();
		psx4all_prof_end(PROFILER_TOTAL);
		psx4all_emulating=0;

		psx4all_prof_show();
	}
#endif
	gp2x_deinit();

	return 0;
}

int SysInit() {
#ifdef GTE_DUMP
	gteLog = fopen("gteLog.txt","wb");
	setvbuf(gteLog, NULL, _IONBF, 0);
#endif

#ifdef EMU_LOG
#ifndef LOG_STDOUT
	emuLog = fopen("emuLog.txt","wb");
#else
	emuLog = stdout;
#endif
	setvbuf(emuLog, NULL, _IONBF, 0);

	SysMessage("\n--- SysInit ---\n");
	fflush(emuLog);
#endif

	psxInit();

	LoadMcds(Config.Mcd1, Config.Mcd2);

	return 0;
}

void SysReset() {
	psxReset();
}

void SysClose() {
	psxShutdown();

	if (emuLog != NULL) fclose(emuLog);
}

static char __sysmsg[1024];

void SysPrintf(const char *fmt, ...) {
	va_list list;

	va_start(list, fmt);
	vsprintf(__sysmsg, fmt, list);
	va_end(list);

	__sysmsg[1023]=0;

	if (Config.PsxOut) printf ("%s", __sysmsg);
#if defined(__WIN32__) && !defined(GIZMONDO)
  OutputDebugString(__sysmsg);
#endif

	gp2x_printf(NULL, 0, 100, __sysmsg);
	gp2x_video_flip_single();

#ifdef EMU_LOG
	fprintf(emuLog, "%s", __sysmsg);
#endif
}

void SysMessage(const char* fmt, ...) {
#ifdef EMU_LOG

	va_list list;

	va_start(list, fmt);
	vsprintf(__sysmsg, fmt, list);
	va_end(list);

	__sysmsg[127]=0;

	if (Config.PsxOut) printf ("%s", __sysmsg);
	fprintf(emuLog, "%s\r\n", __sysmsg);
	fflush(emuLog);
#endif
}

void SysRunGui(){ }

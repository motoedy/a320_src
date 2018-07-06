#include <sys/cachectl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sched.h>
#include <dirent.h>
#include "minimal.h"
#include "dingoo_frontend_list.h"
#include "dingoo_frontend_menu.h"
#include "dingoo_frontend_splash.h"

#define CACHE_FLUSH dingoo_flush_video();
#define COMPATCORES 1

int game_num_avail=0;
static int last_game_selected=0;
char playemu[16] = "mame\0";
char playgame[16] = "builtinn\0";

int dingoo_freq=336;		/* default dingoo Mhz */
int dingoo_video_depth=8;	/* MAME video depth */
int dingoo_video_aspect=0;	/* Normal, no scaling or rotation */
int dingoo_video_sync=0;	/* No vsync */
int dingoo_brightness=70;	/* Screen Brightness */
int dingoo_sound = 6;		/* 16000hz, Mono */
int dingoo_clock_cpu=100;	/* Emulated Main CPU clock percentage */
int dingoo_clock_sound=100;	/* Emulated Sound CPU clock percentage */
int dingoo_cpu_cores=0;		/* Compatible cores */
int dingoo_volume=1;		/* Dingoo Volume = Max */
int dingoo_cheat=0;		/* Enable MAME cheat modes */
int dingoo_profilemode=0;	/* Disable profile mode */

int master_volume = 50;		/* Needed by minimal.cpp */

static void load_bmp_8bpp(unsigned short *out, unsigned char *in) 
{
	int i,x,y;
	unsigned char r,g,b,c;
	in+=14; /* Skip HEADER */
	in+=40; /* Skip INFOHD */
	/* Set Palette */
	for (i=0;i<256;i++) {
		b=*in++;
		g=*in++;
		r=*in++;
		c=*in++;
		dingoo_video_color8(i,r,g,b);
	}
	/* Set Bitmap */	
	for (y=239;y!=-1;y--) {
		for (x=0;x<320;x++) {
			*out++=dingoo_palette[in[x+y*320]];
		}
	}
}

static void dingoo_intro_screen(void) {
	char name[256];
	FILE *f;

	sprintf(name,"skins/dingoosplash.bmp");
	f=fopen(name,"rb");
	if (f) {
		fread(dingoosplash_bmp,1,77878,f);
		fclose(f);
	}
	load_bmp_8bpp(dingoo_screen15,dingoosplash_bmp);
	CACHE_FLUSH
	sprintf(name,"skins/dingoomenu.bmp");
	f=fopen(name,"rb");
	if (f) {
		fread(dingoomenu_bmp,1,77878,f);
		fclose(f);
	}
	sleep(1);
}

static void game_list_init_nocache(void)
{
	int i;
	FILE *f;
	DIR *d=opendir("roms");
	char game[32];
	if (d)
	{
		struct dirent *actual=readdir(d);
		while(actual)
		{
			/*printf ("ROM: %s %d %d\n", actual->d_name, actual->d_reclen, actual->d_type);*/
			for (i=0;i<NUMGAMES;i++)
			{
				if (drivers[i].available==0)
				{
					sprintf(game,"%s.zip",drivers[i].name);
					if (strcmp(actual->d_name,game)==0)
					{
						drivers[i].available=1;
						game_num_avail++;
						break;
					}
				}
			}
			actual=readdir(d);
		}
		closedir(d);
	}
	
	if (game_num_avail)
	{
		remove("frontend/mame.lst");
		sync();
		f=fopen("frontend/mame.lst","w");
		if (f)
		{
			for (i=0;i<NUMGAMES;i++)
			{
				fputc(drivers[i].available,f);
			}
			fclose(f);
			sync();
		}
	}
	else
	{
		printf("No games found!\n");
	}
}

static void game_list_init_cache(void)
{
	FILE *f;
	int i;
	struct stat buf;

	f=fopen("frontend/mame.lst","r");
	if (f)
	{
		stat("frontend/mame.lst", &buf);
		if (buf.st_size == NUMGAMES)
		{
			for (i=0;i<NUMGAMES;i++)
			{
				drivers[i].available=fgetc(f);
				if (drivers[i].available)
					game_num_avail++;
			}
			fclose(f);
			return; /* Success */
		}
	}

	/* Create a new cache file */
	game_list_init_nocache();
}

static void game_list_init(int argc)
{
	if (argc==1)
		game_list_init_nocache();
	else
		game_list_init_cache();
}

static void game_list_view(int *pos) {

	int i;
	int view_pos;
	int aux_pos=0;
	int screen_y = 45;
	int screen_x = 38;

	/* Draw background image */
	load_bmp_8bpp(dingoo_screen15,dingoomenu_bmp);

	/* Check Limits */
	if (*pos<0)
		*pos=game_num_avail-1;
	if (*pos>(game_num_avail-1))
		*pos=0;
					   
	/* Set View Pos */
	if (*pos<10) {
		view_pos=0;
	} else {
		if (*pos>game_num_avail-11) {
			view_pos=game_num_avail-21;
			view_pos=(view_pos<0?0:view_pos);
		} else {
			view_pos=*pos-10;
		}
	}

	/* Show List */
	for (i=0;i<NUMGAMES;i++) {
		if (drivers[i].available==1) {
			if (aux_pos>=view_pos && aux_pos<=view_pos+20) {
				dingoo_gamelist_text_out( screen_x, screen_y, drivers[i].description);
				if (aux_pos==*pos) {
					dingoo_gamelist_text_out( screen_x-10, screen_y,">" );
					dingoo_gamelist_text_out( screen_x-13, screen_y-1,"-" );
				}
				screen_y+=8;
			}
			aux_pos++;
		}
	}
}

static void game_list_select (int index, char *game, char *emu) {
	int i;
	int aux_pos=0;
	for (i=0;i<NUMGAMES;i++)
	{
		if (drivers[i].available==1)
		{
			if(aux_pos==index)
			{
				strcpy(game,drivers[i].name);
				strcpy(emu,drivers[i].exe);
				//dingoo_cpu_cores=drivers[i].cores;
				break;
			}
			aux_pos++;
		}
	}
}

static char *game_list_description (int index)
{
	int i;
	int aux_pos=0;
	for (i=0;i<NUMGAMES;i++) {
		if (drivers[i].available==1) {
			if(aux_pos==index) {
				return(drivers[i].description);
			}
			aux_pos++;
		   }
	}
	return ((char *)0);
}

static int show_options(char *game)
{
	unsigned long ExKey=0;
	int selected_option=0;
	int x_Pos = 41;
	int y_Pos = 38;
	int options_count = 11;
	char text[256];
	FILE *f;
	int i=0;

	/* Read game configuration */
	sprintf(text,"frontend/%s.cfg",game);
	f=fopen(text,"r");
	if (f) {
		fscanf(f,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",&dingoo_freq,&dingoo_video_depth,&dingoo_video_aspect,&dingoo_video_sync,
		&dingoo_brightness,&dingoo_sound,&dingoo_clock_cpu,&dingoo_clock_sound,&dingoo_cpu_cores,&dingoo_volume,&i,&dingoo_cheat);
		fclose(f);
	}
	/* Convert from old settings */
	if (dingoo_brightness < 10)
	{
		dingoo_brightness = 70;
	}
#ifdef COMPATCORES
	if (dingoo_cpu_cores != 0)
	{
		dingoo_cpu_cores = 0;
	}
#endif

	while(1)
	{
		/* Draw background image */
		load_bmp_8bpp(dingoo_screen15,dingoomenu_bmp);

		/* Draw the options */
		strncpy (text,game_list_description(last_game_selected),33);
		text[32]='\0';
		dingoo_gamelist_text_out(x_Pos,y_Pos+10,text);

		/* (0) Dingoo Clock */
		dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+30, "Dingoo Clock  %d MHz", dingoo_freq);

		/* (1) Video Depth */
		switch (dingoo_video_depth)
		{
			case -1: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+40,"Video Depth   Auto"); break;
			case 8:  dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+40,"Video Depth   8 bit"); break;
			case 16: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+40,"Video Depth   16 bit"); break;
		}

		/* (2) Video Aspect */
		switch (dingoo_video_aspect)
		{
			case 0: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+50,"Video Aspect  Normal"); break;
			case 1: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+50,"Video Aspect  Scale Horizontal"); break;
			case 2: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+50,"Video Aspect  Scale Best"); break;
			case 3: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+50,"Video Aspect  Scale Fast"); break;
			case 4: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+50,"Video Aspect  Scale Halfsize"); break;
			case 5: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+50,"Video Aspect  Rotate Normal"); break;
			case 6: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+50,"Video Aspect  Rotate Scale Horiz"); break;
			case 7: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+50,"Video Aspect  Rotate Best"); break;
			case 8: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+50,"Video Aspect  Rotate Fast"); break;
			case 9: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+50,"Video Aspect  Rotate Halfsize"); break;
		}
		
		/* (3) Video Sync */
		switch (dingoo_video_sync)
		{
			case 0: dingoo_gamelist_text_out(x_Pos,y_Pos+60, "Video Sync    Normal"); break;
			case 1: dingoo_gamelist_text_out(x_Pos,y_Pos+60, "Video Sync    DblBuf"); break;
			case -1: dingoo_gamelist_text_out(x_Pos,y_Pos+60,"Video Sync    OFF"); break;
		}
		
		/* (4) Screeen Brightness */
		dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+70,"Brightness    %d%%",dingoo_brightness);

		/* (5) Sound - Max is 48000hz, multiples of this is best */
		switch(dingoo_sound)
		{
			case 0: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+80,"Sound         %s","OFF"); break;
			case 1: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+80,"Sound         %s","ON 8000hz fast"); break;
			case 2: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+80,"Sound         %s","ON 16000hz fast"); break;
			case 3: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+80,"Sound         %s","ON 24000hz fast"); break;
			case 4: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+80,"Sound         %s","ON 32000hz fast"); break;
			case 5: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+80,"Sound         %s","ON 8000hz mono"); break;
			case 6: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+80,"Sound         %s","ON 16000hz mono"); break;
			case 7: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+80,"Sound         %s","ON 24000hz mono"); break;
			case 8: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+80,"Sound         %s","ON 32000hz mono"); break;
			case 9: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+80,"Sound         %s","ON 8000hz stereo"); break;
			case 10: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+80,"Sound         %s","ON 16000hz stereo"); break;
			case 11: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+80,"Sound         %s","ON 24000hz stereo"); break;
			case 12: dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+80,"Sound         %s","ON 32000hz stereo"); break;
		}

		/* (6) CPU Clock */
		dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+90,"CPU Clock     %d%%",dingoo_clock_cpu);

		/* (7) Audio Clock */
		dingoo_gamelist_text_out_fmt(x_Pos,y_Pos+100,"Audio Clock   %d%%",dingoo_clock_sound);

		/* (8) CPU cores */
#ifdef COMPATCORES
		dingoo_gamelist_text_out(x_Pos,y_Pos+110, "CPU Cores     Normal");
#else
		switch (dingoo_cpu_cores)
		{
			case 0: dingoo_gamelist_text_out(x_Pos,y_Pos+110, "CPU Cores     Normal"); break;
			case 1: dingoo_gamelist_text_out(x_Pos,y_Pos+110, "CPU Cores     Fast M68000"); break;
			case 2: dingoo_gamelist_text_out(x_Pos,y_Pos+110, "CPU Cores     Fast Z80"); break;
			case 3: dingoo_gamelist_text_out(x_Pos,y_Pos+110, "CPU Cores     Fast M68000/Z80"); break;
		}
#endif

		/* (9) Volume */
		if (dingoo_sound == 0)
		{
			dingoo_gamelist_text_out(x_Pos,y_Pos+120,"Volume        OFF");
		}
		else
		{
			switch (dingoo_volume)
			{
				case 1: dingoo_gamelist_text_out(x_Pos,y_Pos+120,"Volume        Max"); break;
				case 2: dingoo_gamelist_text_out(x_Pos,y_Pos+120,"Volume        Medium"); break;
				case 3: dingoo_gamelist_text_out(x_Pos,y_Pos+120,"Volume        Low"); break;
				case 4: dingoo_gamelist_text_out(x_Pos,y_Pos+120,"Volume        Quiet"); break;
			}
		}

		/* (10) Cheats */
		if (dingoo_cheat)
			dingoo_gamelist_text_out(x_Pos,y_Pos+130,"Cheats        ON");
		else
			dingoo_gamelist_text_out(x_Pos,y_Pos+130,"Cheats        OFF");
	
		dingoo_gamelist_text_out(x_Pos,y_Pos+160,"Press A to confirm, B to return\0");

		/* Show currently selected item */
		dingoo_gamelist_text_out(x_Pos-16,y_Pos+(selected_option*10)+30," >");

		CACHE_FLUSH
		while(dingoo_joystick_read(1)) { dingoo_timer_delay(150); }
		while(!(ExKey=dingoo_joystick_read(1))) { sched_yield(); }
		if(ExKey & DINGOO_DOWN){
			selected_option++;
			selected_option = selected_option % options_count;
		}
		else if(ExKey & DINGOO_UP){
			selected_option--;
			if(selected_option<0)
				selected_option = options_count - 1;
		}
		else if(ExKey & DINGOO_RIGHT || ExKey & DINGOO_LEFT)
		{
			switch(selected_option) {
			case 0:
				/* Dingoo Clock */
				if(ExKey & DINGOO_RIGHT){
					switch (dingoo_freq) {
						case 300: dingoo_freq=312;break;
						case 312: dingoo_freq=324;break;
						case 324: dingoo_freq=336;break;
						case 336: dingoo_freq=348;break;
						case 348: dingoo_freq=360;break;
						case 360: dingoo_freq=372;break;
						case 372: dingoo_freq=384;break;
						case 384: dingoo_freq=396;break;
						case 396: dingoo_freq=408;break;
						case 408: dingoo_freq=420;break;
						case 420: dingoo_freq=300;break;
					}
				} else {
					switch (dingoo_freq) {
						case 420: dingoo_freq=408;break;
						case 408: dingoo_freq=396;break;
						case 396: dingoo_freq=384;break;
						case 384: dingoo_freq=372;break;
						case 372: dingoo_freq=360;break;
						case 360: dingoo_freq=348;break;
						case 348: dingoo_freq=336;break;
						case 336: dingoo_freq=324;break;
						case 324: dingoo_freq=312;break;
						case 312: dingoo_freq=300;break;
						case 300: dingoo_freq=420;break;
					}
				}
				break;
			case 1:
				switch (dingoo_video_depth)
				{
					case -1: dingoo_video_depth=8; break;
					case 8: dingoo_video_depth=16; break;
					case 16: dingoo_video_depth=-1; break;
				}
				break;
			case 2:
				if(ExKey & DINGOO_RIGHT)
				{
					dingoo_video_aspect++;
					if (dingoo_video_aspect>9)
						dingoo_video_aspect=0;
				}
				else
				{
					dingoo_video_aspect--;
					if (dingoo_video_aspect<0)
						dingoo_video_aspect=9;
				}
				break;
			case 3:
				dingoo_video_sync=dingoo_video_sync+1;
				if (dingoo_video_sync>1)
					dingoo_video_sync=-1;
				break;
			case 4:
				/* "Brightness" */
				if(ExKey & DINGOO_RIGHT)
				{
					dingoo_brightness+=10;
					if (dingoo_brightness > 100)
						dingoo_brightness = 100;
				}
				else {
					dingoo_brightness-=10;
					if (dingoo_brightness < 0)
						dingoo_brightness = 0;
				}
				break;
			case 5:
				if(ExKey & DINGOO_RIGHT)
				{
					dingoo_sound ++;
					if (dingoo_sound>12)
						dingoo_sound=0;
				}
				else
				{
					dingoo_sound--;
					if (dingoo_sound<0)
						dingoo_sound=12;
				}
				break;
			case 6:
				/* "CPU Clock" */
				if(ExKey & DINGOO_RIGHT)
				{
					dingoo_clock_cpu += 10; /* Add 10% */
					if (dingoo_clock_cpu > 200) /* 200% is the max */
						dingoo_clock_cpu = 200;
				}
				else
				{
					dingoo_clock_cpu -= 10; /* Subtract 10% */
					if (dingoo_clock_cpu < 10) /* 10% is the min */
						dingoo_clock_cpu = 10;
				}
				break;
			case 7:
				/* "Audio Clock" */
				if(ExKey & DINGOO_RIGHT)
				{
					dingoo_clock_sound += 10; /* Add 10% */
					if (dingoo_clock_sound > 200) /* 200% is the max */
						dingoo_clock_sound = 200;
				}
				else{
					dingoo_clock_sound -= 10; /* Subtract 10% */
					if (dingoo_clock_sound < 10) /* 10% is the min */
						dingoo_clock_sound = 10;
				}
				break;
			case 8:
#ifdef COMPATCORES
				dingoo_cpu_cores=0;
#else
				dingoo_cpu_cores=(dingoo_cpu_cores+1)%4;
#endif
				break;
			case 9:
				/* Volume */
				if(ExKey & DINGOO_RIGHT)
				{
					dingoo_volume++;
					if (dingoo_volume > 4)
						dingoo_volume = 1;
				}
				else {
					dingoo_volume--;
					if (dingoo_volume < 1)
						dingoo_volume = 4;
				}
				break;
			case 10:
				dingoo_cheat=!dingoo_cheat;
				break;
			}
		}

		if ((ExKey & DINGOO_A) || (ExKey & DINGOO_B) || (ExKey & DINGOO_START)) 
		{
			if (ExKey & DINGOO_START)
			{
				/* Do not write game config in profile mode */
				dingoo_profilemode = 1;
			}
			else
			{
				/* Write game configuration */
				sprintf(text,"frontend/%s.cfg",game);
				f=fopen(text,"w");
				if (f) {
					fprintf(f,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",dingoo_freq,dingoo_video_depth,dingoo_video_aspect,dingoo_video_sync,
					dingoo_brightness,dingoo_sound,dingoo_clock_cpu,dingoo_clock_sound,dingoo_cpu_cores,dingoo_volume,i,dingoo_cheat);
					fclose(f);
					sync();
				}
			}

			/* Selected game will be run */
			return 1;
		}
		else if ((ExKey & DINGOO_X) || (ExKey & DINGOO_Y) || (ExKey & DINGOO_SELECT))
		{
			/* Return To Menu */
			return 0;
		}
	}
}

static void dingoo_exit(void)
{
	remove("frontend/mame.lst");
	sync();
	dingoo_deinit();
      	exit(0);
}

static void select_game(char *emu, char *game)
{

	unsigned long ExKey;

	/* No Selected game */
	strcpy(game,"builtinn");

	/* Wait until user selects a game */
	while(1)
	{
		game_list_view(&last_game_selected);
		CACHE_FLUSH

		if( dingoo_joystick_read(1)) dingoo_timer_delay(150); 
		while(!(ExKey=dingoo_joystick_read(1))) { sched_yield(); }

		if (ExKey & DINGOO_UP) last_game_selected--;
		if (ExKey & DINGOO_DOWN) last_game_selected++;
		if (ExKey & DINGOO_LEFT) last_game_selected-=21;
		if (ExKey & DINGOO_RIGHT) last_game_selected+=21;
		if ((ExKey & DINGOO_SEL_L) && (ExKey & DINGOO_SEL_R)) dingoo_exit();

		if ((ExKey & DINGOO_A) || (ExKey & DINGOO_B) || (ExKey & DINGOO_START))
		{
			/* Select the game */
			game_list_select(last_game_selected, game, emu);

			/* Emulation Options */
			if(show_options(game))
			{
				break;
			}
		}
	}
}

void execute_game (char *playemu, char *playgame)
{
	char *args[255];
	char str[8][64];
	int n=0;
	int i=0;
	
	/* executable */
	args[n]=playemu; n++;

	/* playgame */
	args[n]=playgame; n++;

	/* dingoo_freq */
	args[n]="-clock"; n++;
	sprintf(str[i],"%d",dingoo_freq);
	args[n]=str[i]; i++; n++;

	/*
	 * This is NOT the Dingoo's video depth!
	 * This is MAME's internal video depth.
	 * The Dingoo LCD can only display 16-bit so MAME's 8-bit video
	 * is mapped to Dingoo's 16-bit when blitting (see blit.cpp).
	 */
	if (dingoo_video_depth==8)
	{
		args[n]="-depth"; n++;
		args[n]="8"; n++;
	}
	if (dingoo_video_depth==16)
	{
		args[n]="-depth"; n++;
		args[n]="16"; n++;
	}

	/* dingoo_video_aspect */
	if ((dingoo_video_aspect==1) || (dingoo_video_aspect==6))
	{
		args[n]="-horizscale"; n++;
		args[n]="-nodirty"; n++;
	}
	if ((dingoo_video_aspect==2) || (dingoo_video_aspect==7))
	{
		args[n]="-bestscale"; n++;
		args[n]="-nodirty"; n++;
	}
	if ((dingoo_video_aspect==3) || (dingoo_video_aspect==8))
	{
		args[n]="-fastscale"; n++;
		args[n]="-nodirty"; n++;
	}
	if ((dingoo_video_aspect==4) || (dingoo_video_aspect==9))
	{
		args[n]="-halfscale"; n++;
		args[n]="-nodirty"; n++;
	}
	if ((dingoo_video_aspect>=5) && (dingoo_video_aspect<=9))
	{
		args[n]="-rotatecontrols"; n++;
		args[n]="-ror"; n++;
	}
	
	/* dingoo_video_sync */
	if (dingoo_video_sync==1)
	{
		args[n]="-nodirty"; n++;
		args[n]="-dblbuf"; n++;
	}
	else if (dingoo_video_sync==-1)
	{
		args[n]="-nothrottle"; n++;
	}
	
	/* dingoo_brightness */
	if (dingoo_brightness)
	{
		args[n]="--brightness"; n++;
		sprintf(str[i],"%d",dingoo_brightness);
		args[n]=str[i]; i++; n++;
	}

	/* dingoo_sound */
	if (dingoo_sound==0)
	{
		args[n]="-soundcard"; n++;
		args[n]="0"; n++;
	}
	if ((dingoo_sound==1) || (dingoo_sound==5) || (dingoo_sound==9))
	{
		args[n]="-samplerate"; n++;
		args[n]="8000"; n++;
	}
	if ((dingoo_sound==2) || (dingoo_sound==6) || (dingoo_sound==10))
	{
		args[n]="-samplerate"; n++;
		args[n]="16000"; n++;
	}
	if ((dingoo_sound==3) || (dingoo_sound==7) || (dingoo_sound==11))
	{
		args[n]="-samplerate"; n++;
		args[n]="24000"; n++;
	}
	if ((dingoo_sound==4) || (dingoo_sound==8) || (dingoo_sound==12))
	{
		args[n]="-samplerate"; n++;
		args[n]="32000"; n++;
	}
	if ((dingoo_sound>=1) && (dingoo_sound<=4))
	{
		args[n]="-fastsound"; n++;
	}
	if (dingoo_sound>=9)
	{
		args[n]="-stereo"; n++;
	}

	/* dingoo_clock_cpu */
	if (dingoo_clock_cpu!=100)
	{
		args[n]="-uclock"; n++;
		sprintf(str[i],"%d",100-dingoo_clock_cpu);
		args[n]=str[i]; i++; n++;
	}

	/* dingoo_clock_sound */
	if (dingoo_clock_cpu!=100)
	{
		args[n]="-uclocks"; n++;
		sprintf(str[i],"%d",100-dingoo_clock_sound);
		args[n]=str[i]; i++; n++;
	}
	
	/* dingoo_cpu_cores */
	if ((dingoo_cpu_cores==1) || (dingoo_cpu_cores==3))
	{
		args[n]="-fast_68000"; n++;
	}
	if ((dingoo_cpu_cores==2) || (dingoo_cpu_cores==3))
	{
		args[n]="-fast_z80"; n++;
	}

	switch (dingoo_volume)
	{
		case 1: break; /* nothing, default to maximum volume */
		case 2: args[n]="-volume"; n++; args[n]="-4"; n++; break;
		case 3: args[n]="-volume"; n++; args[n]="-8"; n++; break;
		case 4: args[n]="-volume"; n++; args[n]="-10"; n++; break;
	}
	
	if (dingoo_cheat)
	{
		args[n]="-cheat"; n++;
	}

	if (dingoo_profilemode)
	{
		args[n]="-profilemode"; n++;
	}

	args[n]=NULL;
	
	for (i=0; i<n; i++)
	{
		printf("%s ",args[i]);
	}
	printf("\n");
	
	dingoo_deinit();
	execv(args[0], args); 
}


int main (int argc, char **argv)
{
	FILE *f;

	/* Dingoo Initialization */
	dingoo_init(16000,16,0, DINGOO_DEFAULT_MHZ, DINGOO_DEFAULT_BRIGHT);

	/* Show intro screen */
	dingoo_intro_screen();

	/* Initialize list of available games */
	game_list_init(argc);
	if (game_num_avail==0)
	{
		dingoo_gamelist_text_out(35, 110, "ERROR: NO AVAILABLE GAMES FOUND");
		CACHE_FLUSH
		dingoo_joystick_press();
		dingoo_exit();
	}

	/* Read default configuration */
	f=fopen("frontend/mame.cfg","r");
	if (f) {
		fscanf(f,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",&dingoo_freq,&dingoo_video_depth,&dingoo_video_aspect,&dingoo_video_sync,
		&dingoo_brightness,&dingoo_sound,&dingoo_clock_cpu,&dingoo_clock_sound,&dingoo_cpu_cores,&dingoo_volume,&last_game_selected,&dingoo_cheat);
		fclose(f);
	}

	/* Convert from old settings */
	if (dingoo_brightness < 10)
	{
		dingoo_brightness = 70;
	}
	
	/* Select Game */
	select_game(playemu,playgame); 

	/* Write default configuration */
	f=fopen("frontend/mame.cfg","w");
	if (f) {
		fprintf(f,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",dingoo_freq,dingoo_video_depth,dingoo_video_aspect,dingoo_video_sync,
		dingoo_brightness,dingoo_sound,dingoo_clock_cpu,dingoo_clock_sound,dingoo_cpu_cores,dingoo_volume,last_game_selected,dingoo_cheat);
		fclose(f);
		sync();
	}
	
	/* Execute Game */
	execute_game (playemu,playgame);
	
	exit (0);
}

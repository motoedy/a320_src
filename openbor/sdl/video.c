/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

#include <SDL/SDL_image.h>
#include <SDL/SDL_framerate.h>
#include "types.h"
#include "video.h"
#include "vga.h"
#include "screen.h"
#include "sdlport.h"
#include "openbor.h"
#include "gfxtypes.h"
#include "gfx.h"

extern int videoMode;

#if GP2X || OSX || DINGOO
#define SKIP_CODE
#endif

#ifndef SKIP_CODE
#include "../resources/OpenBOR_Icon_32x32.h"
#endif

FPSmanager framerate_manager;
static SDL_Surface *screen = NULL;
static SDL_Surface *bscreen = NULL;
static SDL_Surface *bscreen2 = NULL;
#ifdef DINGOO
static SDL_Surface *screen_dingoo = NULL;
#endif
static SDL_Color colors[256];
static int bytes_per_pixel = 1;


u8 pDeltaBuffer[480 * 2592];

void initSDL()
{
	#ifndef DINGOO
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
	#else
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK) < 0)
	#endif
	{
		printf("SDL Failed to Init!!!!\n");
		//DEBUG
		borExit(0);
	}
	SDL_ShowCursor(SDL_DISABLE);
	atexit(SDL_Quit);
#ifndef SKIP_CODE
	SDL_WM_SetCaption("OpenBOR",NULL);
	SDL_WM_SetIcon((SDL_Surface*)IMG_ReadXPMFromArray(OpenBOR_Icon_32x32), NULL);
#endif
	SDL_initFramerate(&framerate_manager);
    	SDL_setFramerate(&framerate_manager, 200);
}

static unsigned masks[4][4] = {{0,0,0,0},{0x1F,0x07E0,0xF800,0},{0xFF,0xFF00,0xFF0000,0},{0xFF,0xFF00,0xFF0000,0}};

int video_set_mode(s_videomodes videomodes)
{
    bytes_per_pixel = videomodes.pixel;

	if(videomodes.hRes==0 && videomodes.vRes==0)
	{
		Term_Gfx();
		return 0;
	}

	if(savedata.screen[videoMode][0])
    	{
        if(screen) SDL_FreeSurface(screen);
        if(bscreen) SDL_FreeSurface(bscreen);
        if(bscreen2) SDL_FreeSurface(bscreen2);

	#ifdef DINGOO
	screen_dingoo = SDL_SetVideoMode(videomodes.hRes*savedata.screen[videoMode][0],videomodes.vRes*savedata.screen[videoMode][0],16,SDL_SWSURFACE);
	screen = SDL_AllocSurface(SDL_SWSURFACE,videomodes.hRes*savedata.screen[videoMode][0],videomodes.vRes*savedata.screen[videoMode][0], 8*bytes_per_pixel,0,0,0,0);
	#else
        screen = SDL_SetVideoMode(videomodes.hRes*savedata.screen[videoMode][0],videomodes.vRes*savedata.screen[videoMode][0],16,savedata.fullscreen?(SDL_SWSURFACE|SDL_DOUBLEBUF|SDL_FULLSCREEN):(SDL_SWSURFACE|SDL_DOUBLEBUF));
	#endif
		SDL_ShowCursor(SDL_DISABLE);
        bscreen = SDL_AllocSurface(SDL_SWSURFACE, videomodes.hRes, videomodes.vRes, 8*bytes_per_pixel, masks[bytes_per_pixel-1][0], masks[bytes_per_pixel-1][1], masks[bytes_per_pixel-1][2], masks[bytes_per_pixel-1][3]); // 24bit mask
        bscreen2 = SDL_AllocSurface(SDL_SWSURFACE, videomodes.hRes+4, videomodes.vRes+8, 16, masks[1][2], masks[1][1], masks[1][0], masks[1][3]);
        Init_Gfx(565, 16);
		memset(pDeltaBuffer, 0x00, 1244160);
        if(bscreen==NULL || bscreen2==NULL) return 0;
    }
    else
    {
        if(bscreen2) {SDL_FreeSurface(bscreen2); bscreen2=NULL;}
        if(bytes_per_pixel>1)
        {
            if(bscreen) SDL_FreeSurface(bscreen);
            bscreen = SDL_AllocSurface(SDL_SWSURFACE, videomodes.hRes, videomodes.vRes, 8*bytes_per_pixel, masks[bytes_per_pixel-1][0], masks[bytes_per_pixel-1][1], masks[bytes_per_pixel-1][2], masks[bytes_per_pixel-1][3]); // 24bit mask
            if(!bscreen) return 0;
        }
        if(screen) SDL_FreeSurface(screen);
	
	#ifdef DINGOO
	screen_dingoo = SDL_SetVideoMode(videomodes.hRes,videomodes.vRes,16,SDL_SWSURFACE);
	screen = SDL_AllocSurface(SDL_SWSURFACE,videomodes.hRes,videomodes.vRes, 8*bytes_per_pixel,0,0,0,0);
	#else
        screen = SDL_SetVideoMode(videomodes.hRes,videomodes.vRes,8*bytes_per_pixel,savedata.fullscreen?(SDL_SWSURFACE|SDL_DOUBLEBUF|SDL_FULLSCREEN):(SDL_SWSURFACE|SDL_DOUBLEBUF));
	#endif
	SDL_ShowCursor(SDL_DISABLE);
    }

    if(screen==NULL) return 0;

	video_clearscreen();
    return 1;
}

void video_fullscreen_flip()
{
	#ifdef DINGOO
	// do nothing
	#else

	savedata.fullscreen ^= 1;
	if(screen) SDL_FreeSurface(screen);
	if(savedata.screen[videoMode][0])
		screen = SDL_SetVideoMode(screen->w,screen->h,16,savedata.fullscreen?(SDL_SWSURFACE|SDL_DOUBLEBUF|SDL_FULLSCREEN):(SDL_SWSURFACE|SDL_DOUBLEBUF));
	else
		screen = SDL_SetVideoMode(screen->w,screen->h,8*bytes_per_pixel,savedata.fullscreen?(SDL_SWSURFACE|SDL_DOUBLEBUF|SDL_FULLSCREEN):(SDL_SWSURFACE|SDL_DOUBLEBUF));
	SDL_ShowCursor(SDL_DISABLE);
	SDL_SetColors(screen,colors,0,256);
    if(bscreen) SDL_SetColors(bscreen,colors,0,256);
	#endif
}

//16bit, scale 2x 4x 8x ...
void _stretchblit(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect)
{
    SDL_Rect rect;
    int dst_x, dst_y, dst_w, dst_h, dst_row, src_row;
    int i;
    Uint16* psrc, *pdst;

    if(!srcrect)
    {
        rect.x = rect.y = 0;
        rect.w = src->w;
        rect.h = src->h;
        srcrect = &rect;
    }
    dst_w = savedata.screen[videoMode][0] * srcrect->w;
    dst_h = savedata.screen[videoMode][0] * srcrect->h;
    if(!dstrect)
    {
        dst_x = dst_y = 0;
        if(dst_w>dst->w) dst_w = dst->w;
        if(dst_h>dst->h) dst_h = dst->h;
    }
    else
    {
        dst_x = dstrect->x;
        dst_y = dstrect->y;
        if(dst_w>dstrect->w) dst_w = dstrect->w;
        if(dst_h>dstrect->h) dst_h = dstrect->h;
    }
    psrc = (Uint16*)src->pixels + srcrect->x + srcrect->y * src->pitch/2;
    pdst = (Uint16*)dst->pixels + dst_x + dst_y*dst->pitch/2;
    dst_row = dst->pitch/2;
    src_row = src->pitch/2;
    while(dst_h>0)
    {
        for(i=0; i<dst_w; i++)
        {
            *(pdst + i) = *(psrc+(i/savedata.screen[videoMode][0]));
        }

        for(i=1, pdst += dst_row; i<savedata.screen[videoMode][0] && dst_h; i++, dst_h--, pdst += dst_row)
        {
            memcpy(pdst, pdst-dst_row, dst_w<<1);
        }
        dst_h--;
        psrc += src_row;
    }
}

int video_copy_screen(s_screen* src)
{
    unsigned char *sp;
    char *dp;
    int width, height, linew, slinew;
    int h;
    SDL_Surface* ds = NULL;
	SDL_Rect rectdes, rectsrc;

    width = screen->w;
    if(width > src->width) width = src->width;
    height = screen->h;
    if(height > src->height) height = src->height;
    if(!width || !height) return 0;
    h = height;

    if(bscreen)
	{
        rectdes.x = rectdes.y = 0;
        rectdes.w = width*savedata.screen[videoMode][0]; rectdes.h = height*savedata.screen[videoMode][0];
        if(bscreen2) {rectsrc.x = 2; rectsrc.y = 4;}
        else         {rectsrc.x = 0; rectsrc.y = 0;}
        rectsrc.w = width; rectsrc.h = height;
        if(SDL_MUSTLOCK(bscreen)) SDL_LockSurface(bscreen);
    }

    // Copy to linear video ram
    if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);

    sp = (unsigned char*)src->data;
    ds = (bscreen?bscreen:screen);
    dp = ds->pixels;

    linew = width*bytes_per_pixel;
    slinew = src->width*bytes_per_pixel;

    do{
        memcpy(dp, sp, linew);
        sp += slinew;
        dp += ds->pitch;
    }while(--h);

    if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);

    if(bscreen)
    {
        if(SDL_MUSTLOCK(bscreen)) SDL_UnlockSurface(bscreen);
        if(bscreen2) SDL_BlitSurface(bscreen, NULL, bscreen2, &rectsrc);
        else         SDL_BlitSurface(bscreen, NULL, screen, &rectsrc);
        if(bscreen2)
        {
            if(bscreen2 && SDL_MUSTLOCK(bscreen2)) SDL_LockSurface(bscreen2);
            if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);

            if(savedata.screen[videoMode][0]!=2) _stretchblit(bscreen2, &rectsrc, screen, &rectdes);
            else (*GfxBlitters[(int)savedata.screen[videoMode][1]])((u8*)bscreen2->pixels+bscreen2->pitch*4+4, bscreen2->pitch, pDeltaBuffer+bscreen2->pitch, (u8*)screen->pixels, screen->pitch, screen->w>>1, screen->h>>1);

            if(SDL_MUSTLOCK(bscreen2)) SDL_UnlockSurface(bscreen2);
            if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
        }
    }

    #ifdef DINGOO
    SDL_BlitSurface(screen,NULL,screen_dingoo,NULL);
    SDL_Flip(screen_dingoo);
    #else
        SDL_Flip(screen);
    #endif

#if WIN || LINUX
	SDL_framerateDelay(&framerate_manager);
#endif

    return 1;
}

void video_clearscreen()
{
    if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
    memset(screen->pixels, 0, screen->pitch*screen->h);
    if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
    if(bscreen)
    {
        if(SDL_MUSTLOCK(bscreen)) SDL_LockSurface(bscreen);
        memset(bscreen->pixels, 0, bscreen->pitch*bscreen->h);
        if(SDL_MUSTLOCK(bscreen)) SDL_UnlockSurface(bscreen);
    }
}

void vga_vwait(void)
{
#ifdef GP2X
	gp2x_video_wait_vsync();
#else
	static int prevtick = 0;
	int now = SDL_GetTicks();
	int wait = 1000/60 - (now - prevtick);
	if (wait>0)
    {
		SDL_Delay(wait);
    }
    else SDL_Delay(1);
	prevtick = now;
#endif
}

void vga_setpalette(unsigned char* palette)
{
    int i;
    if(bytes_per_pixel>1) return;
    for(i=0;i<256;i++){
        colors[i].r=palette[0];
        colors[i].g=palette[1];
        colors[i].b=palette[2];
        palette+=3;
    }
    SDL_SetColors(screen,colors,0,256);
    if(bscreen) SDL_SetColors(bscreen,colors,0,256);
}

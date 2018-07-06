/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

#ifndef GAMELIB_TYPES_H
#define GAMELIB_TYPES_H

#define	ANYNUMBER		 2

#if DOS || SDL
#pragma pack(push,4)
#endif

#define PIXEL_8          0
#define PIXEL_x8         1
#define PIXEL_16         2
#define PIXEL_32         4


#define BLEND_SCREEN     0
#define BLEND_MULTIPLY   1
#define BLEND_OVERLAY    2
#define BLEND_HARDLIGHT  3
#define BLEND_DODGE      4
#define BLEND_HALF       5

#define MAX_BLENDINGS    6
/*
#define _copy24bitp(pd, ps) (pd)[0] = (ps)[0]; (pd)[1] = (ps)[1]; (pd)[2] = (ps)[2];
#define _copy24bit(pd, v) (pd)[0] = ((unsigned char*)(&(v)))[0]; (pd)[1] = ((unsigned char*)(&(v)))[1]; (pd)[2] = ((unsigned char*)(&(v)))[2];
*/
extern int pixelformat;
extern int screenformat;
// in bitmap.c
extern int pixelbytes[(int)5];

#define PAL_BYTES (screenformat==PIXEL_8?768:(pixelbytes[(int)screenformat]*256))

typedef struct{
	int	width;
	int	height;
	char pixelformat;
	unsigned char* palette;
	unsigned char data[ANYNUMBER];
}s_screen;


typedef struct{
	int	width;
	int	height;
	int	planar;
	int	banked;		// Still unused
	unsigned char *	data;
}s_vram;


typedef struct{
	int	width;
	int	height;
	char pixelformat;
	unsigned char* palette;
    unsigned char data[ANYNUMBER];
}s_bitmap;


typedef struct{
	int	centerx;
	int	centery;
	int	width;
	int	height;
    char pixelformat;
    unsigned char* palette; 
	int data[ANYNUMBER];
}s_sprite;

struct sprite_list{
	char *filename;
	s_sprite *sprite;
	struct sprite_list *next;
};
typedef struct sprite_list s_sprite_list;
s_sprite_list *sprite_list;

typedef struct{
    char *filename;
	int  ofsx;
    int	 ofsy;
	int  centerx;
	int  centery;
	s_sprite *sprite;
}s_sprite_map;
s_sprite_map *sprite_map;

void set_blendtables(unsigned char* tables[]); // set global blend tables for 8bit mode


typedef unsigned char (*transpixelfunc)(unsigned char* table, unsigned char src, unsigned char dest);
typedef unsigned short (*blend16fp)(unsigned short, unsigned short);
typedef unsigned (*blend32fp)(unsigned, unsigned);

extern blend16fp blendfunctions16[MAX_BLENDINGS];
extern blend32fp blendfunctions32[MAX_BLENDINGS];
extern unsigned char* blendtables[MAX_BLENDINGS];

unsigned short colour16(unsigned char r, unsigned char g, unsigned char b);
unsigned colour32(unsigned char r, unsigned char g, unsigned char b);

void u8revcpy(unsigned char*pa, const unsigned char*pb, unsigned len);
void u8revpcpy(unsigned char*pa, const unsigned char*pb, unsigned char*pp, unsigned len);
void u8pcpy(unsigned char*pa, const unsigned char*pb, unsigned char* pp, unsigned len);

void u16revpcpy(unsigned short* pdest, const unsigned char* psrc, unsigned short* pp, unsigned len);
void u16pcpy(unsigned short* pdest, const unsigned char* psrc, unsigned short* pp, unsigned len);

void u32revpcpy(unsigned* pdest, const unsigned char* psrc, unsigned* pp, unsigned len);
void u32pcpy(unsigned* pdest, const unsigned char* psrc, unsigned* pp, unsigned len);

typedef struct
{
    unsigned char* table;
    void* fp;
    unsigned fillcolor;
    int flag:1;
    int alpha:8;
    int remap:8;
    int flipx:1;
    int flipy:1;
    int transbg:1;
    int fliprotate:1; // entity only, whether the flip is affected by the entity's facing(not the sprite's flip )
    int rotate:11; // 360 degrees
    int scalex;
    int scaley;
    int shiftx;
    int centerx;   // shift centerx
    int centery;   //shift centery
}s_drawmethod;

typedef struct  
{
	short hRes;        // Horizontal Resolution
	short vRes;		 // Vertical Resolution
	short hShift;	     // Offset for X-Axis Text
	short vShift;	     // Offset for Y-Axis Text
	short dOffset;	 // Offset for Debug Text
	short shiftpos[4];
	char filter;
	char mode;
	char pixel;
	float hScale;    // Multiplier for X-Axis
	float vScale;    // Multiplier for Y-Axis

}s_videomodes;

#if DOS || SDL
#pragma pack(pop)
#endif

#endif




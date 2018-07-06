/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

#ifndef	JOYSTICKS_H
#define	JOYSTICKS_H


#define JOY_TYPE_DEFAULT   0
#define JOY_TYPE_SONY      1
#define JOY_TYPE_MICROSOFT 2
#define JOY_TYPE_GAMEPARK  3
#define JOY_AXIS_X         0
#define JOY_AXIS_Y         1
#define JOY_MAX_INPUTS     32
#define	JOY_LIST_FIRST     500
#define JOY_LIST_TOTAL     4
#define	JOY_LIST_LAST      JOY_LIST_FIRST + JOY_MAX_INPUTS * JOY_LIST_TOTAL
#define JOY_NAME_SIZE      1 + 1 + JOY_MAX_INPUTS * JOY_LIST_TOTAL


/* Real-Time Joystick Data */
typedef struct{
	const char *Name;
	const char *KeyName[JOY_MAX_INPUTS + 1];
	int Type;
	int NumButtons;
	unsigned Data;
	unsigned Hats;
	unsigned Buttons;
}s_joysticks;
s_joysticks joysticks[JOY_LIST_TOTAL];


extern const char *JoystickKeyName[JOY_NAME_SIZE];
extern const char *SonyKeyName[JOY_NAME_SIZE];
extern const char *MicrosoftKeyName[JOY_NAME_SIZE];
extern const char *GameparkKeyName[JOY_NAME_SIZE];
extern const int JoystickBits[JOY_MAX_INPUTS + 1];


#endif

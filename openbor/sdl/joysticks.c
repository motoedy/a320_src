/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

#include "joysticks.h"

const char *JoystickKeyName[JOY_NAME_SIZE] = {
	"...",
#define JOYSTICK_NAMES(x) \
	x" Up",             \
	x" Right",          \
	x" Down",           \
	x" Left",           \
	x" Button 1",       \
	x" Button 2",       \
	x" Button 3",       \
	x" Button 4",       \
	x" Button 5",       \
	x" Button 6",       \
	x" Button 7",       \
	x" Button 8",       \
	x" Button 9",       \
	x" Button 10",      \
	x" Button 11",      \
	x" Button 12",      \
	x" Button 13",      \
	x" Button 14",      \
	x" Button 15",      \
	x" Button 16",      \
	x" Button 17",      \
	x" Button 18",      \
	x" Button 19",      \
	x" Button 20",      \
	x" Button 21",      \
	x" Button 22",      \
	x" Button 23",      \
	x" Button 24",      \
	x" Button 25",      \
	x" Button 26",      \
	x" Button 27",      \
	x" Button 28",
	JOYSTICK_NAMES("P1")
	JOYSTICK_NAMES("P2")
	JOYSTICK_NAMES("P3")
	JOYSTICK_NAMES("P4")
	"undefined"
};

const char *SonyKeyName[JOY_NAME_SIZE] = {
	"...",
#define SONY_NAMES(x)   \
	x" Up",             \
	x" Right",          \
	x" Down",           \
	x" Left",           \
	x" Select",         \
	x" L3",             \
	x" R3",             \
	x" Start",          \
	x" L2",             \
	x" R2",             \
	x" L1",             \
	x" R1",             \
	x" /\\",            \
	x" O",              \
	x" X",              \
	x" []",             \
	x" PS",             \
	x" unknown 14",     \
	x" unknown 15",     \
	x" unknown 16",     \
	x" unknown 17",     \
	x" unknown 18",     \
	x" unknown 19",     \
	x" unknown 20",     \
	x" unknown 21",     \
	x" unknown 22",     \
	x" unknown 23",     \
	x" unknown 24",     \
	x" unknown 25",     \
	x" unknown 26",     \
	x" unknown 27",     \
	x" unknown 28",
	SONY_NAMES("P1")
	SONY_NAMES("P2")
	SONY_NAMES("P3")
	SONY_NAMES("P4")
	"undefined"
};

const char *MicrosoftKeyName[JOY_NAME_SIZE] = {
	"...",
#define MICROSOFT_NAMES(x) \
	x" Up",             \
	x" Right",          \
	x" Down",           \
	x" Left",           \
	x" A",              \
	x" B",              \
	x" X",              \
	x" Y",              \
	x" Left Back",      \
	x" Right Back",     \
	x" Back",           \
	x" Start",          \
	x" L-Thumb",        \
	x" R-Thumb",        \
	x" unknown 11",     \
	x" unknown 12",     \
	x" unknown 13",     \
	x" unknown 14",     \
	x" unknown 15",     \
	x" unknown 16",     \
	x" unknown 17",     \
	x" unknown 18",     \
	x" unknown 19",     \
	x" unknown 20",     \
	x" unknown 21",     \
	x" unknown 22",     \
	x" unknown 23",     \
	x" unknown 24",     \
	x" unknown 25",     \
	x" unknown 26",     \
	x" unknown 27",     \
	x" unknown 28",
	MICROSOFT_NAMES("P1")
	MICROSOFT_NAMES("P2")
	MICROSOFT_NAMES("P3")
	MICROSOFT_NAMES("P4")
	"undefined"
};

const char *GameparkKeyName[JOY_NAME_SIZE] = {
	"...",
#define GAMEPARK_NAMES(x) \
	x" Up",             \
	x" Right",          \
	x" Down",           \
	x" Left",           \
	x" Start",          \
	x" Select",         \
	x" L-Trigger",      \
	x" R-Trigger",      \
	x" A",              \
	x" B",              \
	x" X",              \
	x" Y",              \
	x" Volume Up",      \
	x" Volume Down",    \
	x" Click",          \
	x" unknown 16",     \
	x" unknown 17",     \
	x" unknown 18",     \
	x" unknown 19",     \
	x" unknown 20",     \
	x" unknown 21",     \
	x" unknown 22",     \
	x" unknown 23",     \
	x" unknown 24",     \
	x" unknown 25",     \
	x" unknown 26",     \
	x" unknown 27",     \
	x" unknown 28",
	GAMEPARK_NAMES("P1")
	GAMEPARK_NAMES("P2")
	GAMEPARK_NAMES("P3")
	GAMEPARK_NAMES("P4")
	"undefined"
};

const int JoystickBits[JOY_MAX_INPUTS + 1] = {
	0x00000000, // No Buttons Pressed
	0x00000001, // Hat Up
	0x00000002,	// Hat Right
	0x00000004, // Hat Down
	0x00000008,	// Hat Left
	0x00000010,	// Button 1
	0x00000020,	// Button 2
	0x00000040,	// Button 3
	0x00000080,	// Button 4
	0x00000100,	// Button 5
	0x00000200,	// Button 6
	0x00000400,	// Button 7
	0x00000800,	// Button 8
	0x00001000, // Button 9
	0x00002000,	// Button 10
	0x00004000,	// Button 11
	0x00008000,	// Button 12
	0x00010000,	// Button 13
	0x00020000,	// Button 14
	0x00040000,	// Button 15
	0x00080000,	// Button 16
	0x00100000,	// Button 17
	0x00200000,	// Button 18
	0x00400000,	// Button 19
	0x00800000,	// Button 20
	0x01000000,	// Button 21
	0x02000000,	// Button 22
	0x04000000,	// Button 23
	0x08000000,	// Button 24
	0x10000000,	// Button 25
	0x20000000,	// Button 26
	0x40000000,	// Button 27
	0x80000000 	// Button 28
};

/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

// Generic control stuff (keyboard+joystick)
#include "video.h"
#include "globals.h"
#include "control.h"
#include "stristr.h"
#include "sblaster.h"
#include "joysticks.h"


SDL_Joystick *joystick[JOY_LIST_TOTAL]; // SDL struct for joysticks
static int usejoy;						// To be or Not to be used?
static int numjoy;						// Number of Joy(s) found
static int lastkey;						// Last keyboard key Pressed


/*
Here is where we aquiring all joystick events
and map them to BOR's layout.  Currently support
up to 4 controllers.
*/
void getPads()
{
	int i, x, y;
	SDL_Event ev;
	while(SDL_PollEvent(&ev))
	{
		switch(ev.type)
		{
			case SDL_KEYDOWN:
				lastkey = ev.key.keysym.sym;
				//if (lastkey==SDLK_F11) video_fullscreen_flip();
				if (lastkey!=SDLK_F10) break;
		
			case SDL_QUIT:
				exit(0);
				break;

			case SDL_JOYBUTTONUP:
				for(i=0; i<JOY_LIST_TOTAL; i++)
				{
					if(ev.jbutton.which == i)
					{
						if(joysticks[i].Type == JOY_TYPE_SONY)
						{
							if(ev.jbutton.button <= 3 && (joysticks[i].Buttons & JoystickBits[ev.jbutton.button + 5]))
								joysticks[i].Buttons &= ~(JoystickBits[ev.jbutton.button + 5]);
							else if(ev.jbutton.button >= 4 && ev.jbutton.button <= 7)
								joysticks[i].Hats &= ~(JoystickBits[ev.jbutton.button - 3]);
							else if(ev.jbutton.button >= 8 && ev.jbutton.button <= 16 && (joysticks[i].Buttons & JoystickBits[ev.jbutton.button + 1])) 
								joysticks[i].Buttons &= ~(JoystickBits[ev.jbutton.button + 1]);
						}
						else if(joysticks[i].Type == JOY_TYPE_GAMEPARK)
						{
							if(ev.jbutton.button == 0 || ev.jbutton.button == 7 || ev.jbutton.button == 1) joysticks[i].Hats &= ~(JoystickBits[1]);
							if(ev.jbutton.button == 6 || ev.jbutton.button == 5 || ev.jbutton.button == 7) joysticks[i].Hats &= ~(JoystickBits[2]);
							if(ev.jbutton.button == 4 || ev.jbutton.button == 3 || ev.jbutton.button == 5) joysticks[i].Hats &= ~(JoystickBits[3]);
							if(ev.jbutton.button == 2 || ev.jbutton.button == 1 || ev.jbutton.button == 3) joysticks[i].Hats &= ~(JoystickBits[4]);
							if(ev.jbutton.button >= 8 && ev.jbutton.button <= 18) joysticks[i].Buttons &= ~(JoystickBits[ev.jbutton.button - 3]);
						}
						else 
							joysticks[i].Buttons &= ~(JoystickBits[ev.jbutton.button + 5]);

						joysticks[i].Data = joysticks[i].Hats | joysticks[i].Buttons;
					}
				}
				break;

			case SDL_JOYBUTTONDOWN:
				for(i=0; i<JOY_LIST_TOTAL; i++)
				{
					if(ev.jbutton.which == i)
					{
						if(joysticks[i].Type == JOY_TYPE_SONY)
						{
							if(ev.jbutton.button <= 3)
								joysticks[i].Buttons |= JoystickBits[ev.jbutton.button + 5];
							else if(ev.jbutton.button >= 4 && ev.jbutton.button <= 7) 
								joysticks[i].Hats |= JoystickBits[ev.jbutton.button - 3];
							else if(ev.jbutton.button >= 8 && ev.jbutton.button <= 16)
								joysticks[i].Buttons |= JoystickBits[ev.jbutton.button + 1];
						}
						else if(joysticks[i].Type == JOY_TYPE_GAMEPARK)
						{
							if(ev.jbutton.button == 0 || ev.jbutton.button == 7 || ev.jbutton.button == 1) joysticks[i].Hats |= JoystickBits[1];
							if(ev.jbutton.button == 6 || ev.jbutton.button == 5 || ev.jbutton.button == 7) joysticks[i].Hats |= JoystickBits[2];
							if(ev.jbutton.button == 4 || ev.jbutton.button == 3 || ev.jbutton.button == 5) joysticks[i].Hats |= JoystickBits[3];
							if(ev.jbutton.button == 2 || ev.jbutton.button == 1 || ev.jbutton.button == 3) joysticks[i].Hats |= JoystickBits[4];
							if(ev.jbutton.button >= 8 && ev.jbutton.button <= 18) joysticks[i].Buttons |= JoystickBits[ev.jbutton.button - 3];
							if(joysticks[i].Buttons & JoystickBits[13] && joysticks[i].Buttons & JoystickBits[14]) joysticks[i].Buttons |= JoystickBits[15];
							if(joysticks[i].Buttons & JoystickBits[13]) SB_updatevolume(1);
							if(joysticks[i].Buttons & JoystickBits[14]) SB_updatevolume(-1);
						}
						else
							joysticks[i].Buttons |= JoystickBits[ev.jbutton.button + 5];

						joysticks[i].Data = joysticks[i].Hats | joysticks[i].Buttons;
					}
				}
				break;

			case SDL_JOYHATMOTION:
				for(i=0; i<JOY_LIST_TOTAL; i++)
				{
					if(ev.jhat.which == i)
					{
						joysticks[i].Hats = ev.jhat.value;
						joysticks[i].Data = joysticks[i].Hats;
					}
				}
				break;
		
			case SDL_JOYAXISMOTION:
				for(i=0; i<JOY_LIST_TOTAL; i++)
				{
					if(ev.jaxis.which == i && joysticks[i].Type != JOY_TYPE_SONY)
					{
						x = SDL_JoystickGetAxis(joystick[i], JOY_AXIS_X);
						y = SDL_JoystickGetAxis(joystick[i], JOY_AXIS_Y);
						if(x < -7000 && !(joysticks[i].Hats & JoystickBits[4]))     joysticks[i].Hats |= JoystickBits[4];
						else if(x > -7000 && (joysticks[i].Hats & JoystickBits[4])) joysticks[i].Hats &= ~(JoystickBits[4]);
						if(x > +7000 && !(joysticks[i].Hats & JoystickBits[2]))     joysticks[i].Hats |= JoystickBits[2];
						else if(x < +7000 && (joysticks[i].Hats & JoystickBits[2])) joysticks[i].Hats &= ~(JoystickBits[2]);
						if(y < -7000 && !(joysticks[i].Hats & JoystickBits[1]))     joysticks[i].Hats |= JoystickBits[1];
						else if(y > -7000 && (joysticks[i].Hats & JoystickBits[1])) joysticks[i].Hats &= ~(JoystickBits[1]);
						if(y > +7000 && !(joysticks[i].Hats & JoystickBits[3]))     joysticks[i].Hats |= JoystickBits[3];
						else if(y < +7000 && (joysticks[i].Hats & JoystickBits[3])) joysticks[i].Hats &= ~(JoystickBits[3]);

						joysticks[i].Data = joysticks[i].Hats;
					}
				}
				break;
		}
	}	
}


/*
Convert binary masked data to indexes
*/
static int flag_to_index(unsigned long flag)
{
	int index = 0;
	unsigned long bit = 1;
	while(!((bit<<index)&flag) && index<31) ++index;
	return index;
}


/*
Search for usb joysticks. Set 
types, defaults and keynames.
*/
void joystick_scan(int scan)
{
	int i, j, k;
	if(!scan) return;
	numjoy = SDL_NumJoysticks();
	if(!numjoy && scan != 2)
	{
		printf("No Joystick(s) Found!\n");
		return;
	}
	for(i=0, k=0; i<numjoy; i++, k+=JOY_MAX_INPUTS)
	{
		joystick[i] = SDL_JoystickOpen(i);
		joysticks[i].NumButtons = SDL_JoystickNumButtons(joystick[i]);
		joysticks[i].Name = SDL_JoystickName(i);
		if(stristr(joysticks[i].Name, "Sony"))
		{	
			joysticks[i].Type = JOY_TYPE_SONY;
			for(j=0; j<JOY_MAX_INPUTS+1; j++)
			{
				if(j) joysticks[i].KeyName[j] = SonyKeyName[j + k];
				else joysticks[i].KeyName[j] = SonyKeyName[j];
			}
		}
		else if(stristr(joysticks[i].Name, "Microsoft"))
		{	
			joysticks[i].Type = JOY_TYPE_MICROSOFT;
			for(j=0; j<JOY_MAX_INPUTS+1; j++)
			{
				if(j) joysticks[i].KeyName[j] = MicrosoftKeyName[j + k];
				else joysticks[i].KeyName[j] = MicrosoftKeyName[j];
			}
		}
		else if(stristr(joysticks[i].Name, "GP2X"))
		{	
			joysticks[i].Type = JOY_TYPE_GAMEPARK;
			for(j=0; j<JOY_MAX_INPUTS+1; j++)
			{
				if(j) joysticks[i].KeyName[j] = GameparkKeyName[j + k];
				else joysticks[i].KeyName[j] = GameparkKeyName[j];
			}
		}
		if(scan != 2)
		{
			if(numjoy == 1) printf("%s - %d Buttons\n", joysticks[i].Name, joysticks[i].NumButtons);
			else if(numjoy > 1)
			{
				if(!i) printf("%d. %s - %d Buttons\n", i + 1, joysticks[i].Name, joysticks[i].NumButtons);
				else printf("\t\t\t\t%d. %s - %d Buttons\n", i + 1, joysticks[i].Name, joysticks[i].NumButtons);
			}
		}
	}
}


/*
Reset All data back to Zero and
destroy all SDL Joystick data.
*/
void control_exit()
{
	int i;
	usejoy = 0;
	for(i=0; i<numjoy; i++) SDL_JoystickClose(joystick[i]);
	memset(joysticks, 0, sizeof(s_joysticks) * JOY_LIST_TOTAL);
}


/*
Create default values for joysticks if enabled.
Then scan for joysticks and update their data.
*/
void control_init(int joy_enable)
{
	int i, j, k;
#ifdef GP2X
	usejoy = joy_enable ? joy_enable : 1;
#else
	usejoy = joy_enable;
#endif
	memset(joysticks, 0, sizeof(s_joysticks) * JOY_LIST_TOTAL);
	for(i=0, k=0; i<JOY_LIST_TOTAL; i++, k+=JOY_MAX_INPUTS)
	{
		for(j=0; j<JOY_MAX_INPUTS+1; j++)
		{
			if(j) joysticks[i].KeyName[j] = JoystickKeyName[j + k];
			else joysticks[i].KeyName[j] = JoystickKeyName[j];
		}
	}
	joystick_scan(usejoy);
}


/*
Set global variable, which is used for
enabling and disabling all joysticks.
*/
int control_usejoy(int enable)
{
	usejoy = enable;
	return 0;
}


/*
Only used in openbor.c to get current status
of joystick usage.
*/
int control_getjoyenabled()
{
	return usejoy;
}


void control_setkey(s_playercontrols * pcontrols, unsigned int flag, int key)
{
	if(!pcontrols) return;
	pcontrols->settings[flag_to_index(flag)] = key;
	pcontrols->keyflags = pcontrols->newkeyflags = 0;
}


int keyboard_getlastkey()
{
	int i, ret = lastkey;
	lastkey = 0;
	for(i=0; i<JOY_LIST_TOTAL; i++) joysticks[i].Buttons = 0;
	return ret;
}


// Scan input for newly-pressed keys.
// Return value:
// 0  = no key was pressed
// >0 = key code for pressed key
// <0 = error
int control_scankey()
{
	static unsigned ready = 0;
	unsigned k = 0, j = 0;
	
    k = keyboard_getlastkey();

		 if(joysticks[0].Data) j = 1 + 0 * JOY_MAX_INPUTS + flag_to_index(joysticks[0].Data);
	else if(joysticks[1].Data) j = 1 + 1 * JOY_MAX_INPUTS + flag_to_index(joysticks[1].Data);
	else if(joysticks[2].Data) j = 1 + 2 * JOY_MAX_INPUTS + flag_to_index(joysticks[2].Data);
	else if(joysticks[3].Data) j = 1 + 3 * JOY_MAX_INPUTS + flag_to_index(joysticks[3].Data);

	if(ready && (k || j)) 
	{
		ready = 0;
		if(k) return k;
		if(j) return JOY_LIST_FIRST+j;
		else return -1;
	}
	ready = (!k || !j);
	return 0;
}


char *control_getkeyname(unsigned int keycode)
{

	int i;
	for(i=0; i<JOY_LIST_TOTAL; i++)
	{
		if((keycode >= (JOY_LIST_FIRST + 1 + (i * JOY_MAX_INPUTS))) && (keycode <= JOY_LIST_FIRST + JOY_MAX_INPUTS + (i * JOY_MAX_INPUTS)))
			return (char*)joysticks[i].KeyName[keycode - (JOY_LIST_FIRST + (i * JOY_MAX_INPUTS))];
	}

	if(keycode > SDLK_FIRST && keycode < SDLK_LAST) 
#ifdef DINGOO
	{
		
		if(keycode == SDLK_UP) return "Up";
		else if(keycode== SDLK_DOWN) return "Down";
		else if(keycode== SDLK_LEFT) return "Left";
		else if(keycode== SDLK_RIGHT) return "Right";
		else if(keycode== SDLK_LCTRL) return "A";
		else if(keycode== SDLK_LALT) return "B";
		else if(keycode== SDLK_SPACE)  return "X";
		else if(keycode== SDLK_LSHIFT)  return "Y";
		else if(keycode== SDLK_TAB)  return "L";
		else if(keycode== SDLK_BACKSPACE)  return "R";
		else if(keycode== SDLK_RETURN)  return "Start";
		else if(keycode== SDLK_ESCAPE)  return "Select";
		else return "...";
	}
		
#else
		return SDL_GetKeyName(keycode);
#endif
	else 
		return "...";
}


void control_update(s_playercontrols ** playercontrols, int numplayers)
{
	unsigned k;
	unsigned i;
	int player;
	int t;
	s_playercontrols * pcontrols;
	Uint8* keystate;
	
	keystate = SDL_GetKeyState(NULL);
	
	getPads();	
	for(player=0; player<numplayers; player++){

		pcontrols = playercontrols[player];

		k = 0;

		for(i=0;i<32;i++) 
		{
			t = pcontrols->settings[i];
			if(t >= SDLK_FIRST && t < SDLK_LAST){
				if(keystate[t]) k |= (1<<i);
			}
		}

		if(usejoy)
		{
			for(i=0; i<32; i++)
			{
				t = pcontrols->settings[i];
				if(t >= JOY_LIST_FIRST && t <= JOY_LIST_LAST)
				{
					int portnum = (t-JOY_LIST_FIRST-1) / JOY_MAX_INPUTS;
					int shiftby = (t-JOY_LIST_FIRST-1) % JOY_MAX_INPUTS;
					if(portnum >= 0 && portnum <= 3)
					{
						if((joysticks[portnum].Data >> shiftby) & 1) k |= (1<<i);
					}
				}
			}
		}
		pcontrols->kb_break = 0;
		pcontrols->newkeyflags = k & (~pcontrols->keyflags);
		pcontrols->keyflags = k;
	}
}

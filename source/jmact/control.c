/*
 * control.c
 * MACT library -to- JonoF's Build Port Control System Glue
 * 
 * by Jonathon Fowler
 *
 * Since we don't have the source to the MACT library I've had to
 * concoct some magic to glue its idea of controllers into that of
 * my Build port.
 *
 */
//-------------------------------------------------------------------------
/*
Duke Nukem Copyright (C) 1996, 2003 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#include "types.h"
#include "keyboard.h"
#include "mouse.h"
#include "control.h"

#include "osd.h"

boolean  CONTROL_RudderEnabled;
boolean  CONTROL_MousePresent;
boolean  CONTROL_JoysPresent[ MaxJoys ];
boolean  CONTROL_MouseEnabled;
boolean  CONTROL_JoystickEnabled;
byte     CONTROL_JoystickPort;
uint32   CONTROL_ButtonState1;
uint32   CONTROL_ButtonHeldState1;
uint32   CONTROL_ButtonState2;
uint32   CONTROL_ButtonHeldState2;

/*
 * Joystick:
 * Button0 = Button A
 * Button1 = Button B
 * Button2 = Button C
 * Button3 = Button D
 * Button4 = Hat Up
 * Button5 = Hat Right
 * Button6 = Hat Down
 * Button7 = Hat Left
 * AAxis0  = Joystick X
 * AAxis1  = Joystick Y
 * AAxis2  = Rudder
 * AAxis3  = Throttle
 */

static struct {
	boolean toggle;
	kb_scancode key1, key2;
} buttons[MAXGAMEBUTTONS];

static fixed axisvalue[6];		// analog_*

#define MAXMOUSEAXES 2
#define MAXMOUSEBUTTONS 6
static int32 mouseaxismapping[MAXMOUSEAXES];	// x, y
static int32 mouseaxisscaling[MAXMOUSEAXES] = { 65536, 65536 };
static int32 mousebuttonages[MAXMOUSEBUTTONS], mousebuttonclicks=0;
static int32 mousebuttonfunctions[MAXMOUSEBUTTONS], mousedblbuttonfunctions[MAXMOUSEBUTTONS];
static int32 mousedigitalfunctions[MAXMOUSEAXES][2], mousedigitaldirection[MAXMOUSEAXES];
static int32 mousesensitivity = 0x8000;

#define MAXJOYAXES 8
#define MAXJOYBUTTONS (32+4)
static int32 joyaxismapping[MAXJOYAXES];	// x, y, rudder, throttle, extras
static int32 joyaxisscaling[MAXJOYAXES] = { 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536 };
static int32 joybuttonages[MAXJOYBUTTONS], joybuttonclicks=0;
static int32 joybuttonfunctions[MAXJOYBUTTONS], joydblbuttonfunctions[MAXJOYBUTTONS];
static int32 joydigitalfunctions[MAXJOYAXES][2], joydigitaldirection[MAXJOYAXES];
static int32 joyhatstatus = 0;

static int32 (*timefunc)(void) = (void*)0;
static int32 timetics = 0, timedbldelay = 0;

static int32 DoubleClickDelay = 500;		// milliseconds
#define WasMouseDoubleClicked(t,b)	((t-mousebuttonages[b-1]) <= timedbldelay)
#define WasJoyDoubleClicked(t,b)	((t-joybuttonages[b-1]) <= timedbldelay)


void CONTROL_MapKey( int32 which, kb_scancode key1, kb_scancode key2 )
{
	if (which < 0 || which >= MAXGAMEBUTTONS) return;
	buttons[which].key1 = key1;
	buttons[which].key2 = key2;
}


void CONTROL_MapButton
        (
        int32 whichfunction,
        int32 whichbutton,
        boolean doubleclicked,
	controldevice device
        )
{
	if (whichfunction < 0 || whichfunction >= MAXGAMEBUTTONS) return;
	if (whichbutton < 0) return;
	switch (device) {
		case controldevice_mouse:
			if (whichbutton >= MAXMOUSEBUTTONS) break;
			if (doubleclicked) mousedblbuttonfunctions[whichbutton] = 1+whichfunction;
			else mousebuttonfunctions[whichbutton] = 1+whichfunction;
			break;
		case controldevice_joystick:
		case controldevice_gamepad:
			if (whichbutton >= MAXJOYBUTTONS) break;
			if (doubleclicked) joydblbuttonfunctions[whichbutton] = 1+whichfunction;
			else joybuttonfunctions[whichbutton] = 1+whichfunction;
			break;
		default: break;
	}
}


void CONTROL_DefineFlag( int32 which, boolean toggle )
{
	if (which < 0 || which >= MAXGAMEBUTTONS) return;
	buttons[which].toggle = toggle;
}


boolean CONTROL_FlagActive( int32 which )
{
	if (which < 0 || which >= MAXGAMEBUTTONS) return 0;
	return BUTTON(which);
}


void CONTROL_ClearAssignments( void )
{
	int i;

	for (i=0; i<MAXGAMEBUTTONS; i++) {
		buttons[i].key1 = 0;
		buttons[i].key2 = 0;
		buttons[i].toggle = 0;
	}

	for (i=0; i<MAXMOUSEAXES; i++) {
		mouseaxismapping[i] = 0;
		mouseaxisscaling[i] = 65535;
		mousedigitalfunctions[i][0] = 0;
		mousedigitalfunctions[i][1] = 0;
		mousedigitaldirection[i] = 0;
	}

	for (i=0; i<MAXMOUSEBUTTONS; i++) {
		mousebuttonages[i] = 0;
		mousebuttonfunctions[i] = 0;
		mousedblbuttonfunctions[i] = 0;
	}

	for (i=0; i<MAXJOYAXES; i++) {
		joyaxismapping[i] = 0;
		joyaxisscaling[i] = 65535;
		joydigitalfunctions[i][0] = 0;
		joydigitalfunctions[i][1] = 0;
		joydigitaldirection[i] = 0;
	}

	for (i=0; i<MAXJOYBUTTONS; i++) {
		joybuttonages[i] = 0;
		joybuttonfunctions[i] = 0;
		joydblbuttonfunctions[i] = 0;
	}
}


void CONTROL_GetUserInput( UserInput *info )
{
	if (!info) return;

	info->button0 = 0;
	info->button1 = 0;
	info->dir = dir_None;
}


void CONTROL_GetInput( ControlInfo *info )
{
	int i,j;
	int32 mx,my;
	
	if (!info) return;

	if (CONTROL_MouseEnabled && CONTROL_MousePresent) {
		MOUSE_GetDelta(&mx,&my);
		if (mouseaxismapping[0])
			axisvalue[mouseaxismapping[0]-1] += (((mx*mousesensitivity)>>8) * mouseaxisscaling[0]) >> 16;
		if (mouseaxismapping[1])
			axisvalue[mouseaxismapping[1]-1] += (((my*mousesensitivity)>>6) * mouseaxisscaling[1]) >> 16;

		// now for the digital axis support
		if (mx < 3 && mx > -3) mx=0;	// a little bit of filtering
		if (my < 3 && my > -3) my=0;

		if (mx < 0) mx = -1;
		else if (mx > 0) mx = 1;
		if (my < 0) my = -1;
		else if (my > 0) my = 1;
		
		if (mx != mousedigitaldirection[0]) {
			if (!mx || (mx && mousedigitaldirection[0])) {
				// stopped moving or moved the other way
				if (mousedigitaldirection[0]>0) i=1;
				else i=0;
				CONTROL_MouseEvent(0x80000000ul|i, 0);
			}
			
			if (mx) {
				// activate direction
				if (mx>0) i=1;
				else i=0;
				CONTROL_MouseEvent(0x80000000ul|i, 1);
			}

			mousedigitaldirection[0] = mx;
		}

		if (my != mousedigitaldirection[1]) {
			if (!my || (my && mousedigitaldirection[1])) {
				// stopped moving or moved the other way
				if (mousedigitaldirection[1]>0) i=1;
				else i=0;
				CONTROL_MouseEvent(0x80000002ul|i, 0);
			}
			
			if (my) {
				// activate direction
				if (my>0) i=1;
				else i=0;
				CONTROL_MouseEvent(0x80000002ul|i, 1);
			}

			mousedigitaldirection[1] = my;
		}
	}

	if (CONTROL_JoystickEnabled && CONTROL_JoysPresent[0]) {
		for (j=0; j<MAXJOYAXES && j<joynumaxes; j++) {
			if (joyaxismapping[j])
				axisvalue[joyaxismapping[j]-1] += ((joyaxis[j]>>5) * joyaxisscaling[j]) >> 16;

			mx = 0;
			if (joyaxis[j] < 0) mx = -1;
			else if (joyaxis[j] > 0) mx = 1;

			if (mx != joydigitaldirection[j]) {
				if (!mx || (mx && joydigitaldirection[j])) {
					// stopped moving, or moved the other way
					if (joydigitaldirection[j]>0) i=1;
					else i=0;
					CONTROL_JoyEvent(0x80000000ul|(j<<1)|i, 0);
				}

				if (mx) {
					// activate direction
					if (mx > 0) i=1;
					else i=0;
					CONTROL_JoyEvent(0x80000000ul|(j<<1)|i, 1);
				}

				joydigitaldirection[j] = mx;
			}
		}

		// joystick hats. ugh. we use the first hat.
		mx = 0;
		if (joynumhats > 0) {
			if (joyhat[0] != -1) {
				static int hatstate[] = { 1, 1|2, 2, 2|4, 4, 4|8, 8, 8|1 };
				int val;
				
				// thanks SDL for this much more sensible method
				val = ((joyhat[0] + 4500 / 2) % 36000) / 4500;
				if (val < 8) mx = hatstate[val];
			}
			for (i=0; i<4; i++) {
				j = 1<<i;
				if ((joyhatstatus & j) && !(mx & j))
					CONTROL_JoyEvent(1+MAXJOYBUTTONS-4+i, 0);	// hat direction released
				else if (!(joyhatstatus & j) && (mx & j))
					CONTROL_JoyEvent(1+MAXJOYBUTTONS-4+i, 1);	// hat direction pressed
			}
		}
		joyhatstatus = mx;
	}

	info->dx = axisvalue[analog_strafing];
	info->dy = axisvalue[analog_elevation];
	info->dz = axisvalue[analog_moving];
	info->dyaw = axisvalue[analog_turning];
	info->dpitch = axisvalue[analog_lookingupanddown];
	info->droll = axisvalue[analog_rolling];
	
	for (i=0;i<6;i++) axisvalue[i] = 0;
}


void CONTROL_ClearButton( int32 whichbutton )
{
	uint32 mask;
	
	if (whichbutton < 0 || whichbutton >= MAXGAMEBUTTONS) return;
	if (whichbutton>31) {
		mask = 1ul<<(whichbutton-32);
		CONTROL_ButtonState2 &= ~(0xfffffffful & mask);
	} else {
		mask = 1ul<<whichbutton;
		CONTROL_ButtonState1 &= ~(0xfffffffful & mask);
	}
}


void CONTROL_ClearUserInput( UserInput *info )
{
	if (!info) return;

	info->button0 = 0;
	info->button1 = 0;
	info->dir = dir_None;
}


void CONTROL_WaitRelease( void )
{
}


void CONTROL_Ack( void )
{
}


void CONTROL_CenterJoystick
   (
   void ( *CenterCenter )( void ),
   void ( *UpperLeft )( void ),
   void ( *LowerRight )( void ),
   void ( *CenterThrottle )( void ),
   void ( *CenterRudder )( void )
   )
{
}


int32 CONTROL_GetMouseSensitivity( void )
{
	return mousesensitivity;
}


void CONTROL_SetMouseSensitivity( int32 newsensitivity )
{
	mousesensitivity = newsensitivity;
}


void CONTROL_Startup
   (
   controltype which,
   int32 ( *TimeFunction )( void ),
   int32 ticspersecond
   )
{
	int i;

	timefunc = TimeFunction;
	timetics = ticspersecond;
	timedbldelay = DoubleClickDelay/(1000/timetics);
	
	CONTROL_RudderEnabled = 0;
	CONTROL_MousePresent = 0;
	for (i=0; i<MaxJoys; i++) CONTROL_JoysPresent[i] = 0;
	CONTROL_MouseEnabled = 0;
	CONTROL_JoystickEnabled = 0;
	CONTROL_JoystickPort = 0;
	CONTROL_ButtonState1 = 0;
	CONTROL_ButtonHeldState1 = 0;
	CONTROL_ButtonState2 = 0;
	CONTROL_ButtonHeldState2 = 0;
//	CONTROL_ClearAssignments();

	initinput();
	setkeypresscallback((KeyPressCallback)KB_KeyEvent);		// JBF 20030910: little stricter on the typing
	setmousepresscallback((MousePressCallback)CONTROL_MouseEvent);
	setjoypresscallback((JoyPressCallback)CONTROL_JoyEvent);

	CONTROL_MousePresent = MOUSE_Init();
	CONTROL_MouseEnabled = (CONTROL_MousePresent && which == controltype_keyboardandmouse);
	CONTROL_JoysPresent[0] = ((inputdevices&4) == 4);
	CONTROL_JoystickEnabled = (CONTROL_JoysPresent[0] && 
			(which == controltype_keyboardandjoystick ||
			 which == controltype_keyboardandgamepad ||
			 which == controltype_keyboardandflightstick ||
			 which == controltype_keyboardandthrustmaster));
}


void CONTROL_Shutdown( void )
{
	uninitinput();
}


void CONTROL_SetDoubleClickDelay(int32 delay)
{
	if (delay < 0) delay = 0;
	DoubleClickDelay = delay;
	timedbldelay = DoubleClickDelay/(1000/timetics);
}

int32 CONTROL_GetDoubleClickDelay(void)
{
	return DoubleClickDelay;
}


void CONTROL_MapAnalogAxis
   (
   int32 whichaxis,
   int32 whichanalog,
   controldevice device
   )
{
	if (whichaxis < 0) return;
	switch (device) {
		case controldevice_mouse:
			if (whichaxis >= MAXMOUSEAXES) break;
			mouseaxismapping[whichaxis] = whichanalog+1;
			break;
		case controldevice_joystick:
		case controldevice_gamepad:
			if (whichaxis >= MAXJOYAXES) break;
			joyaxismapping[whichaxis] = whichanalog+1;
			break;
		default: break;
	}
}


void CONTROL_MapDigitalAxis
   (
   int32 whichaxis,
   int32 whichfunction,
   int32 direction,
   controldevice device
   )
{
	if (whichaxis < 0) return;
	if (direction < 0) return;
	switch (device) {
		case controldevice_mouse:
			if (whichaxis >= MAXMOUSEAXES) break;
			if (direction >= 2) break;
			mousedigitalfunctions[whichaxis][direction] = whichfunction+1;
			break;
		case controldevice_joystick:
		case controldevice_gamepad:
			if (whichaxis >= MAXJOYAXES) break;
			if (direction >= 2) break;
			joydigitalfunctions[whichaxis][direction] = whichfunction+1;
			break;
		default: break;
	}
}


// 65535 = 1
void CONTROL_SetAnalogAxisScale
   (
   int32 whichaxis,
   int32 axisscale,
   controldevice device
   )
{
	if (whichaxis < 0) return;
	switch (device) {
		case controldevice_mouse:
			if (whichaxis >= MAXMOUSEAXES) break;
			mouseaxisscaling[whichaxis] = axisscale;
			break;
		case controldevice_joystick:
		case controldevice_gamepad:
			if (whichaxis >= MAXJOYAXES) break;
			joyaxisscaling[whichaxis] = axisscale;
			break;
		default: break;
	}
}


void CONTROL_PrintAxes( void )
{
}


void CONTROL_KeyEvent(kb_scancode scancode, boolean keypressed)
{
	int i;
	uint32 mask;

	for (i=0; i<MAXGAMEBUTTONS; i++) {
		if (buttons[i].key1 == scancode || buttons[i].key2 == scancode) {
			if (!buttons[i].toggle) {
				if (i>31) {
					mask = 1ul<<(i-32);
					if (keypressed)
						CONTROL_ButtonState2 |= mask;
					else
						CONTROL_ButtonState2 &= ~mask;
				} else {
					mask = 1ul<<i;
					if (keypressed)
						CONTROL_ButtonState1 |= mask;
					else
						CONTROL_ButtonState1 &= ~mask;
				}
			} else {
				if (!keypressed) continue;
				if (i>31) {
					mask = 1ul<<(i-32);
					if (CONTROL_ButtonState2 & mask)
						CONTROL_ButtonState2 &= ~mask;
					else
						CONTROL_ButtonState2 |= mask;
				} else {
					mask = 1ul<<i;
					if (CONTROL_ButtonState1 & mask)
						CONTROL_ButtonState1 &= ~mask;
					else
						CONTROL_ButtonState1 |= mask;
				}
			}
		}
	}
}

void CONTROL_MouseEvent(int32 button, boolean keypressed)
{
	int i=0;
	uint32 mask,bmask;
	int32 b,c;
	int32 thetime;

	if (!CONTROL_MouseEnabled) return;

	thetime = (timefunc)?timefunc():0;
	b = 0;

	if (button & 0x80000000ul) {
		// digital axis
		button &= ~0x80000000ul;
		if (mousedigitalfunctions[button>>1][button&1] > 0) b |= 4;
		else return;	// no action on the axis
	} else {
		// button
		button--;

		bmask = 1<<button;
		if (mousebuttonfunctions[button] > 0) b |= 1;
		if (mousedblbuttonfunctions[button] > 0) {
			// check and see if this was a double-click
			if (keypressed) {
				if ((mousebuttonclicks & bmask)) {
					if (WasMouseDoubleClicked(thetime,(1+button)))
						// was the second click
						b |= 2;
					else
						mousebuttonclicks &= ~bmask;
				}
				mousebuttonages[button] = thetime;
			} else {
				// was a release
				if (!(mousebuttonclicks & bmask)) {
					mousebuttonclicks |= bmask;
				} else {
					mousebuttonclicks &= ~bmask;
					b |= 2;
				}
			}
		}		
	}

	for (c=1; c<=4; c<<=1) {
		if (!(b&c)) { continue; }
		if (c==1) i = mousebuttonfunctions[button] - 1;
		else if (c==2) i = mousedblbuttonfunctions[button] - 1;
		else if (c==4) i = mousedigitalfunctions[button>>1][button&1] - 1;

		if (!buttons[i].toggle) {
			if (i>31) {
				mask = 1ul<<(i-32);
				if (keypressed)
					CONTROL_ButtonState2 |= mask;
				else
					CONTROL_ButtonState2 &= ~mask;
			} else {
				mask = 1ul<<i;
				if (keypressed)
					CONTROL_ButtonState1 |= mask;
				else
					CONTROL_ButtonState1 &= ~mask;
			}
		} else {
			if (keypressed) {
				if (i>31) {
					mask = 1ul<<(i-32);
					if (CONTROL_ButtonState2 & mask)
						CONTROL_ButtonState2 &= ~mask;
					else
						CONTROL_ButtonState2 |= mask;
				} else {
					mask = 1ul<<i;
					if (CONTROL_ButtonState1 & mask)
						CONTROL_ButtonState1 &= ~mask;
					else
						CONTROL_ButtonState1 |= mask;
				}
			}
		}
	}
}

void CONTROL_JoyEvent(int32 button, boolean keypressed)
{
	int i=0;
	uint32 mask,bmask;
	int32 b,c;
	int32 thetime;

	if (!CONTROL_JoystickEnabled) return;

	thetime = (timefunc)?timefunc():0;
	b = 0;

	if (button & 0x80000000ul) {
		// digital axis
		button &= ~0x80000000ul;
		if (joydigitalfunctions[button>>1][button&1] > 0) b |= 4;
		else return;	// no action on the axis
	} else {
		// button
		button--;

		bmask = 1<<button;
		if (joybuttonfunctions[button] > 0) b |= 1;
		if (joydblbuttonfunctions[button] > 0) {
			// check and see if this was a double-click
			if (keypressed) {
				if ((joybuttonclicks & bmask)) {
					if (WasJoyDoubleClicked(thetime,(1+button)))
						// was the second click
						b |= 2;
					else
						joybuttonclicks &= ~bmask;
				}
				joybuttonages[button] = thetime;
			} else {
				// was a release
				if (!(joybuttonclicks & bmask)) {
					joybuttonclicks |= bmask;
				} else {
					joybuttonclicks &= ~bmask;
					b |= 2;
				}
			}
		}		
	}

	for (c=1; c<=4; c<<=1) {
		if (!(b&c)) { continue; }
		if (c==1) i = joybuttonfunctions[button] - 1;
		else if (c==2) i = joydblbuttonfunctions[button] - 1;
		else if (c==4) i = joydigitalfunctions[button>>1][button&1] - 1;

		if (!buttons[i].toggle) {
			if (i>31) {
				mask = 1ul<<(i-32);
				if (keypressed)
					CONTROL_ButtonState2 |= mask;
				else
					CONTROL_ButtonState2 &= ~mask;
			} else {
				mask = 1ul<<i;
				if (keypressed)
					CONTROL_ButtonState1 |= mask;
				else
					CONTROL_ButtonState1 &= ~mask;
			}
		} else {
			if (keypressed) {
				if (i>31) {
					mask = 1ul<<(i-32);
					if (CONTROL_ButtonState2 & mask)
						CONTROL_ButtonState2 &= ~mask;
					else
						CONTROL_ButtonState2 |= mask;
				} else {
					mask = 1ul<<i;
					if (CONTROL_ButtonState1 & mask)
						CONTROL_ButtonState1 &= ~mask;
					else
						CONTROL_ButtonState1 |= mask;
				}
			}
		}
	}
}



//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

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

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jonof@edgenetwk.com)
*/
//-------------------------------------------------------------------------

/*
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
*/

#include "duke3d.h"
#include "scriplib.h"
#include "osd.h"

#include "baselayer.h"

// we load this in to get default button and key assignments
// as well as setting up function mappings

#define __SETUP__	// JBF 20031211
#include "_functio.h"

//
// Sound variables
//
int32 FXDevice;
int32 MusicDevice;
int32 FXVolume;
int32 MusicVolume;
int32 SoundToggle;
int32 MusicToggle;
int32 VoiceToggle;
int32 AmbienceToggle;
//fx_blaster_config BlasterConfig;
int32 NumVoices;
int32 NumChannels;
int32 NumBits;
int32 MixRate;
//int32 MidiPort;
int32 ReverseStereo;

int32 ControllerType;
int32 MouseAiming;
int32 RunMode;
int32 AutoAim;	// JBF 20031125

// JBF 20031211: Store the input settings because
// (currently) jmact can't regurgitate them
byte KeyboardKeys[NUMGAMEFUNCTIONS][2];
int32 MouseFunctions[MAXMOUSEBUTTONS][2];
int32 MouseDigitalFunctions[MAXMOUSEAXES][2];
int32 MouseAnalogueAxes[MAXMOUSEAXES];
int32 MouseAnalogueScale[MAXMOUSEAXES];
int32 JoystickFunctions[MAXJOYBUTTONS][2];
int32 JoystickDigitalFunctions[MAXJOYAXES][2];
int32 JoystickAnalogueAxes[MAXJOYAXES];
int32 JoystickAnalogueScale[MAXJOYAXES];
int32 JoystickAnalogueDead[MAXJOYAXES];
int32 JoystickAnalogueSaturate[MAXJOYAXES];

//
// Screen variables
//

int32 ScreenMode = 1;
int32 ScreenWidth = 640;
int32 ScreenHeight = 480;
int32 ScreenBPP = 8;

static char setupfilename[256]={SETUPFILENAME};
static int32 scripthandle = -1;
static int32 setupread=0;


/*
===================
=
= CONFIG_FunctionNameToNum
=
===================
*/

int32 CONFIG_FunctionNameToNum( char * func )
   {
   int32 i;

   for (i=0;i<NUMGAMEFUNCTIONS;i++)
      {
      if (!Bstrcasecmp(func,gamefunctions[i]))
         {
         return i;
         }
      }
   return -1;
   }

/*
===================
=
= CONFIG_FunctionNumToName
=
===================
*/

char * CONFIG_FunctionNumToName( int32 func )
   {
   if ((unsigned)func >= (unsigned)NUMGAMEFUNCTIONS)
      {
      return NULL;
      }
   else
      {
      return gamefunctions[func];
      }
   }

/*
===================
=
= CONFIG_AnalogNameToNum
=
===================
*/


int32 CONFIG_AnalogNameToNum( char * func )
   {

   if (!Bstrcasecmp(func,"analog_turning"))
      {
      return analog_turning;
      }
   if (!Bstrcasecmp(func,"analog_strafing"))
      {
      return analog_strafing;
      }
   if (!Bstrcasecmp(func,"analog_moving"))
      {
      return analog_moving;
      }
   if (!Bstrcasecmp(func,"analog_lookingupanddown"))
      {
      return analog_lookingupanddown;
      }

   return -1;
   }


char * CONFIG_AnalogNumToName( int32 func )
   {
   switch (func) {
	case analog_turning:
		return "analog_turning";
	case analog_strafing:
		return "analog_strafing";
	case analog_moving:
   		return "analog_moving";
	case analog_lookingupanddown:
		return "analog_lookingupanddown";
   }

   return NULL;
   }


/*
===================
=
= CONFIG_SetDefaults
=
===================
*/

void CONFIG_SetDefaults( void )
   {
   // JBF 20031211
   int32 i,f;
   byte k1,k2;

   FXDevice = -1;
   MusicDevice = -1;
   NumVoices = 4;
   NumChannels = 2;
   NumBits = 8;
   MixRate = 11025;
   SoundToggle = 1;
   MusicToggle = 1;
   VoiceToggle = 1;
   AmbienceToggle = 1;
   FXVolume = 192;
   MusicVolume = 128;
   ReverseStereo = 0;
   myaimmode = ps[0].aim_mode = 0;
   MouseAiming = 0;
   AutoAim = 1;
   ControllerType = 1;
   ud.mouseflip = 0;
   ud.runkey_mode = 0;
   ud.statusbarscale = 100;
   ud.screen_size = 8;
   ud.screen_tilting = 1;
   ud.shadows = 1;
   ud.detail = 1;
   ud.lockout = 0;
   ud.pwlockout[0] = '\0';
   ud.crosshair = 0;
   ud.m_marker = 1;
   ud.m_ffire = 1;
   ud.levelstats = 0;
   Bstrcpy(ud.rtsname, "DUKE.RTS");
   Bstrcpy(myname, "Duke");

   Bstrcpy(ud.ridecule[0], "An inspiration for birth control.");
   Bstrcpy(ud.ridecule[1], "You're gonna die for that!");
   Bstrcpy(ud.ridecule[2], "It hurts to be you.");
   Bstrcpy(ud.ridecule[3], "Lucky Son of a Bitch.");
   Bstrcpy(ud.ridecule[4], "Hmmm....Payback time.");
   Bstrcpy(ud.ridecule[5], "You bottom dwelling scum sucker.");
   Bstrcpy(ud.ridecule[6], "Damn, you're ugly.");
   Bstrcpy(ud.ridecule[7], "Ha ha ha...Wasted!");
   Bstrcpy(ud.ridecule[8], "You suck!");
   Bstrcpy(ud.ridecule[9], "AARRRGHHHHH!!!");
   
   // JBF 20031211
   memset(KeyboardKeys, 0, sizeof(KeyboardKeys));
   for (i=0; i < (int32)(sizeof(keydefaults)/sizeof(keydefaults[0]))/3; i++) {
      f = CONFIG_FunctionNameToNum( keydefaults[3*i+0] );
      if (f == -1) continue;
      k1 = KB_StringToScanCode( keydefaults[3*i+1] );
      k2 = KB_StringToScanCode( keydefaults[3*i+2] );

      KeyboardKeys[f][0] = k1;
      KeyboardKeys[f][1] = k2;
      CONTROL_MapKey( f, k1, k2 );
   }

   memset(MouseFunctions, -1, sizeof(MouseFunctions));
   for (i=0; i<MAXMOUSEBUTTONS; i++) {
      f = CONFIG_FunctionNameToNum( mousedefaults[i] );
      if (f != -1)
         CONTROL_MapButton( f, i, false, controldevice_mouse );
      MouseFunctions[i][0] = f;

      if (i<4) continue;

      f = CONFIG_FunctionNameToNum( mouseclickeddefaults[i] );
      if (f != -1)
         CONTROL_MapButton( f, i, true, controldevice_mouse );
      MouseFunctions[i][1] = f;
   }

   memset(MouseDigitalFunctions, -1, sizeof(MouseDigitalFunctions));
   for (i=0; i<MAXMOUSEAXES; i++) {
	MouseAnalogueScale[i] = 65536;

	f = CONFIG_FunctionNameToNum( mousedigitaldefaults[i*2] );
	if (f != -1)
		CONTROL_MapDigitalAxis( i, f, 0,controldevice_mouse );
	MouseDigitalFunctions[i][0] = f;
	
	f = CONFIG_FunctionNameToNum( mousedigitaldefaults[i*2+1] );
	if (f != -1)
		CONTROL_MapDigitalAxis( i, f, 1,controldevice_mouse );
	MouseDigitalFunctions[i][1] = f;

	f = CONFIG_AnalogNameToNum( mouseanalogdefaults[i] );
	if (f != -1)
		CONTROL_MapAnalogAxis(i,f,controldevice_mouse);
	MouseAnalogueAxes[i] = f;
   }

   memset(JoystickFunctions, -1, sizeof(JoystickFunctions));
   for (i=0; i<MAXJOYBUTTONS; i++) {
      f = CONFIG_FunctionNameToNum( joystickdefaults[i] );
      if (f != -1)
         CONTROL_MapButton( f, i, false, controldevice_joystick );
      JoystickFunctions[i][0] = f;

      f = CONFIG_FunctionNameToNum( joystickclickeddefaults[i] );
      if (f != -1)
         CONTROL_MapButton( f, i, true, controldevice_joystick );
      JoystickFunctions[i][1] = f;
   }

   memset(JoystickDigitalFunctions, -1, sizeof(JoystickDigitalFunctions));
   for (i=0; i<MAXJOYAXES; i++) {
	JoystickAnalogueScale[i] = 65536;
	JoystickAnalogueDead[i] = 1000;
	JoystickAnalogueSaturate[i] = 9500;

	f = CONFIG_FunctionNameToNum( joystickdigitaldefaults[i*2] );
	if (f != -1)
		CONTROL_MapDigitalAxis( i, f, 0,controldevice_joystick );
	JoystickDigitalFunctions[i][0] = f;
	
	f = CONFIG_FunctionNameToNum( joystickdigitaldefaults[i*2+1] );
	if (f != -1)
		CONTROL_MapDigitalAxis( i, f, 1,controldevice_joystick );
	JoystickDigitalFunctions[i][1] = f;

	f = CONFIG_AnalogNameToNum( joystickanalogdefaults[i] );
	if (f != -1)
		CONTROL_MapAnalogAxis(i,f,controldevice_joystick);
	JoystickAnalogueAxes[i] = f;
   }
}
/*
===================
=
= CONFIG_ReadKeys
=
===================
*/

void CONFIG_ReadKeys( void )
   {
   int32 i;
   int32 numkeyentries;
   int32 function;
   char keyname1[80];
   char keyname2[80];
   kb_scancode key1,key2;

   numkeyentries = SCRIPT_NumberEntries( scripthandle, "KeyDefinitions" );

   for (i=0;i<numkeyentries;i++)
      {
      function = CONFIG_FunctionNameToNum(SCRIPT_Entry( scripthandle, "KeyDefinitions", i ));
      if (function != -1)
         {
         Bmemset(keyname1,0,sizeof(keyname1));
         Bmemset(keyname2,0,sizeof(keyname2));
         SCRIPT_GetDoubleString
            (
            scripthandle,
            "KeyDefinitions",
            SCRIPT_Entry( scripthandle,"KeyDefinitions", i ),
            keyname1,
            keyname2
            );
         key1 = 0;
         key2 = 0;
         if (keyname1[0])
            {
            key1 = (byte) KB_StringToScanCode( keyname1 );
            }
         if (keyname2[0])
            {
            key2 = (byte) KB_StringToScanCode( keyname2 );
            }
         if (function == gamefunc_Show_Console)
            OSD_CaptureKey(key1);
         else
            CONTROL_MapKey( function, key1, key2 );
         KeyboardKeys[function][0] = key1;
         KeyboardKeys[function][1] = key2;
         }
      }
   }


/*
===================
=
= CONFIG_SetupMouse
=
===================
*/

void CONFIG_SetupMouse( int32 scripthandle )
   {
   int32 i;
   char str[80],*p;
   char temp[80];
   int32 function, scale;

   for (i=0;i<MAXMOUSEBUTTONS;i++)
      {
      Bsprintf(str,"MouseButton%ld",i);
      //Bmemset(temp,0,sizeof(temp));
	  Bstrcpy(temp,(p=CONFIG_FunctionNumToName(MouseFunctions[i][0]))?p:"");
      SCRIPT_GetString( scripthandle,"Controls", str,temp);
	  function = CONFIG_FunctionNameToNum(temp);
      //if (function != -1)
         CONTROL_MapButton( function, i, false, controldevice_mouse );
      MouseFunctions[i][0] = function;
	  
      Bsprintf(str,"MouseButtonClicked%ld",i);
      //Bmemset(temp,0,sizeof(temp));
	  Bstrcpy(temp,(p=CONFIG_FunctionNumToName(MouseFunctions[i][1]))?p:"");
      SCRIPT_GetString( scripthandle,"Controls", str,temp);
      function = CONFIG_FunctionNameToNum(temp);
      //if (function != -1)
         CONTROL_MapButton( function, i, true, controldevice_mouse );
      MouseFunctions[i][1] = function;
      }
   // map over the axes
   for (i=0;i<MAXMOUSEAXES;i++)
      {
      Bsprintf(str,"MouseAnalogAxes%ld",i);
      //Bmemset(temp,0,sizeof(temp));
	  Bstrcpy(temp,(p=CONFIG_AnalogNumToName(MouseAnalogueAxes[i]))?p:"");
      SCRIPT_GetString(scripthandle, "Controls", str,temp);
      function = CONFIG_AnalogNameToNum(temp);
      //if (function != -1)
         {
         CONTROL_MapAnalogAxis(i,function,controldevice_mouse);
         }
      MouseAnalogueAxes[i] = function;
	  
      Bsprintf(str,"MouseDigitalAxes%ld_0",i);
      //Bmemset(temp,0,sizeof(temp));
	  Bstrcpy(temp,(p=CONFIG_FunctionNumToName(MouseDigitalFunctions[i][0]))?p:"");
      SCRIPT_GetString(scripthandle, "Controls", str,temp);
      function = CONFIG_FunctionNameToNum(temp);
      //if (function != -1)
         CONTROL_MapDigitalAxis( i, function, 0,controldevice_mouse );
      MouseDigitalFunctions[i][0] = function;
	  
      Bsprintf(str,"MouseDigitalAxes%ld_1",i);
      //Bmemset(temp,0,sizeof(temp));
	  Bstrcpy(temp,(p=CONFIG_FunctionNumToName(MouseDigitalFunctions[i][1]))?p:"");
      SCRIPT_GetString(scripthandle, "Controls", str,temp);
      function = CONFIG_FunctionNameToNum(temp);
      //if (function != -1)
         CONTROL_MapDigitalAxis( i, function, 1,controldevice_mouse );
      MouseDigitalFunctions[i][1] = function;

      Bsprintf(str,"MouseAnalogScale%ld",i);
	  scale = MouseAnalogueScale[i];
      SCRIPT_GetNumber(scripthandle, "Controls", str,&scale);
      CONTROL_SetAnalogAxisScale( i, scale, controldevice_mouse );
      MouseAnalogueScale[i] = scale;
      }

   function = 32768;
   SCRIPT_GetNumber( scripthandle, "Controls","MouseSensitivity",&function);
   CONTROL_SetMouseSensitivity(function);
   }

/*
===================
=
= CONFIG_SetupJoystick
=
===================
*/

void CONFIG_SetupJoystick( int32 scripthandle )
   {
   int32 i;
   char str[80],*p;
   char temp[80];
   int32 function, scale;

   for (i=0;i<MAXJOYBUTTONS;i++)
      {
      Bsprintf(str,"JoystickButton%ld",i);
      //Bmemset(temp,0,sizeof(temp));
	  Bstrcpy(temp,(p=CONFIG_FunctionNumToName(JoystickFunctions[i][0]))?p:"");
      SCRIPT_GetString( scripthandle,"Controls", str,temp);
      function = CONFIG_FunctionNameToNum(temp);
      if (function != -1)
         CONTROL_MapButton( function, i, false, controldevice_joystick );
      JoystickFunctions[i][0] = function;
	  
      Bsprintf(str,"JoystickButtonClicked%ld",i);
      //Bmemset(temp,0,sizeof(temp));
	  Bstrcpy(temp,(p=CONFIG_FunctionNumToName(JoystickFunctions[i][1]))?p:"");
      SCRIPT_GetString( scripthandle,"Controls", str,temp);
      function = CONFIG_FunctionNameToNum(temp);
      if (function != -1)
         CONTROL_MapButton( function, i, true, controldevice_joystick );
      JoystickFunctions[i][1] = function;
     }
   // map over the axes
   for (i=0;i<MAXJOYAXES;i++)
      {
      Bsprintf(str,"JoystickAnalogAxes%ld",i);
      //Bmemset(temp,0,sizeof(temp));
	  Bstrcpy(temp,(p=CONFIG_AnalogNumToName(JoystickAnalogueAxes[i]))?p:"");
      SCRIPT_GetString(scripthandle, "Controls", str,temp);
      function = CONFIG_AnalogNameToNum(temp);
      if (function != -1)
         {
         CONTROL_MapAnalogAxis(i,function, controldevice_joystick);
         }
	  JoystickAnalogueAxes[i] = function;
	  
      Bsprintf(str,"JoystickDigitalAxes%ld_0",i);
      //Bmemset(temp,0,sizeof(temp));
	  Bstrcpy(temp,(p=CONFIG_FunctionNumToName(JoystickDigitalFunctions[i][0]))?p:"");
      SCRIPT_GetString(scripthandle, "Controls", str,temp);
      function = CONFIG_FunctionNameToNum(temp);
      if (function != -1)
         CONTROL_MapDigitalAxis( i, function, 0, controldevice_joystick );
	  JoystickDigitalFunctions[i][0] = function;
	  
      Bsprintf(str,"JoystickDigitalAxes%ld_1",i);
      //Bmemset(temp,0,sizeof(temp));
	  Bstrcpy(temp,(p=CONFIG_FunctionNumToName(JoystickDigitalFunctions[i][1]))?p:"");
      SCRIPT_GetString(scripthandle, "Controls", str,temp);
      function = CONFIG_FunctionNameToNum(temp);
      if (function != -1)
         CONTROL_MapDigitalAxis( i, function, 1, controldevice_joystick );
	  JoystickDigitalFunctions[i][1] = function;
	  
      Bsprintf(str,"JoystickAnalogScale%ld",i);
	  scale = JoystickAnalogueScale[i];
      SCRIPT_GetNumber(scripthandle, "Controls", str,&scale);
      CONTROL_SetAnalogAxisScale( i, scale, controldevice_joystick );
      JoystickAnalogueScale[i] = scale;

	  Bsprintf(str,"JoystickAnalogDead%ld",i);
	  scale = JoystickAnalogueDead[i];
      SCRIPT_GetNumber(scripthandle, "Controls", str,&scale);
      JoystickAnalogueDead[i] = scale;

	  Bsprintf(str,"JoystickAnalogSaturate%ld",i);
	  scale = JoystickAnalogueSaturate[i];
      SCRIPT_GetNumber(scripthandle, "Controls", str,&scale);
      JoystickAnalogueSaturate[i] = scale;
	  }
   }

void readsavenames(void)
{
    long dummy;
    short i;
    char fn[13];
    BFILE *fil;

    Bstrcpy(fn,"game_.sav");

    for (i=0;i<10;i++)
    {
        fn[4] = i+'0';
        if ((fil = Bfopen(fn,"rb")) == NULL ) continue;
        if (dfread(&dummy,4,1,fil) != 1) { Bfclose(fil); continue; }

        if(dummy != BYTEVERSION) return;
        if (dfread(&dummy,4,1,fil) != 1) { Bfclose(fil); continue; }
		if (dfread(&ud.savegame[i][0],19,1,fil) != 1) { ud.savegame[i][0] = 0; }
        Bfclose(fil);
    }
}

/*
===================
=
= CONFIG_ReadSetup
=
===================
*/

void CONFIG_ReadSetup( void )
{
   int32 dummy;
   char commmacro[] = "CommbatMacro# ";
/*
   if (!SafeFileExists(setupfilename))
      {
      Error("ReadSetup: %s does not exist\n"
            "           Please run SETUP.EXE\n",setupfilename);
      }
*/
   CONTROL_ClearAssignments();	// JBF 20031211: Moved from above CONFIG_ReadKeys

   CONFIG_SetDefaults();

   if (SafeFileExists(setupfilename))	// JBF 20031211
	   scripthandle = SCRIPT_Load( setupfilename );

   for(dummy = 0;dummy < 10;dummy++)
   {
       commmacro[13] = dummy+'0';
       SCRIPT_GetString( scripthandle, "Comm Setup",commmacro,&ud.ridecule[dummy][0]);
   }

   SCRIPT_GetString( scripthandle, "Comm Setup","PlayerName",&myname[0]);

   SCRIPT_GetString( scripthandle, "Comm Setup","RTSName",&ud.rtsname[0]);

   SCRIPT_GetNumber( scripthandle, "Screen Setup", "Shadows",&ud.shadows);
   SCRIPT_GetString( scripthandle, "Screen Setup","Password",&ud.pwlockout[0]);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "Detail",&ud.detail);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "Tilt",&ud.screen_tilting);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "Messages",&ud.fta_on);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "ScreenWidth",&ScreenWidth);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "ScreenHeight",&ScreenHeight);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "ScreenMode",&ScreenMode);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "ScreenGamma",&ud.brightness);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "ScreenSize",&ud.screen_size);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "Out",&ud.lockout);

   SCRIPT_GetNumber( scripthandle, "Screen Setup", "ScreenBPP", &ScreenBPP);
   if (ScreenBPP < 8) ScreenBPP = 8;

#ifdef RENDERTYPEWIN
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "MaxRefreshFreq", (int32*)&maxrefreshfreq);
#endif

   SCRIPT_GetNumber( scripthandle, "Screen Setup", "GLTextureMode", &gltexfiltermode);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "GLAnisotropy", &glanisotropy);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "GLUseTextureCompr", &glusetexcompr);

   SCRIPT_GetNumber( scripthandle, "Misc", "Executions",&ud.executions);
   ud.executions++;
   SCRIPT_GetNumber( scripthandle, "Misc", "RunMode",&RunMode);
   SCRIPT_GetNumber( scripthandle, "Misc", "Crosshairs",&ud.crosshair);
   SCRIPT_GetNumber( scripthandle, "Misc", "StatusBarScale",&ud.statusbarscale);
   SCRIPT_GetNumber( scripthandle, "Misc", "ShowLevelStats",&ud.levelstats);
   if(ud.wchoice[0][0] == 0 && ud.wchoice[0][1] == 0)
   {
       ud.wchoice[0][0] = 3;
       ud.wchoice[0][1] = 4;
       ud.wchoice[0][2] = 5;
       ud.wchoice[0][3] = 7;
       ud.wchoice[0][4] = 8;
       ud.wchoice[0][5] = 6;
       ud.wchoice[0][6] = 0;
       ud.wchoice[0][7] = 2;
       ud.wchoice[0][8] = 9;
       ud.wchoice[0][9] = 1;

       for(dummy=0;dummy<10;dummy++)
       {
           Bsprintf(buf,"WeaponChoice%ld",dummy);
           SCRIPT_GetNumber( scripthandle, "Misc", buf, &ud.wchoice[0][dummy]);
       }
    }

   SCRIPT_GetNumber( scripthandle, "Sound Setup", "FXDevice",&FXDevice);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "MusicDevice",&MusicDevice);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "FXVolume",&FXVolume);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "MusicVolume",&MusicVolume);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "SoundToggle",&SoundToggle);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "MusicToggle",&MusicToggle);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "VoiceToggle",&VoiceToggle);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "AmbienceToggle",&AmbienceToggle);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "NumVoices",&NumVoices);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "NumChannels",&NumChannels);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "NumBits",&NumBits);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "MixRate",&MixRate);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "ReverseStereo",&ReverseStereo);

   SCRIPT_GetNumber( scripthandle, "Controls","ControllerType",&ControllerType);
   SCRIPT_GetNumber( scripthandle, "Controls","MouseAimingFlipped",&ud.mouseflip);	// mouse aiming inverted
   SCRIPT_GetNumber( scripthandle, "Controls","MouseAiming",&MouseAiming);		// 1=momentary/0=toggle
   //SCRIPT_GetNumber( scripthandle, "Controls","GameMouseAiming",(int32 *)&ps[0].aim_mode);	// dupe of below (?)
   ps[0].aim_mode = MouseAiming;
   SCRIPT_GetNumber( scripthandle, "Controls","AimingFlag",(int32 *)&myaimmode);	// (if toggle mode) gives state
   SCRIPT_GetNumber( scripthandle, "Controls","RunKeyBehaviour",&ud.runkey_mode);	// JBF 20031125
   SCRIPT_GetNumber( scripthandle, "Controls","AutoAim",&AutoAim);			// JBF 20031125
   ps[0].auto_aim = AutoAim;

   CONFIG_ReadKeys();

   CONFIG_SetupMouse(scripthandle);
   CONFIG_SetupJoystick(scripthandle);

   setupread = 1;
   }

/*
===================
=
= CONFIG_WriteSetup
=
===================
*/

void CONFIG_WriteSetup( void )
   {
   int32 dummy;

   //if (!setupread) return;
   if (scripthandle < 0)
	   scripthandle = SCRIPT_Init(setupfilename);

   SCRIPT_PutNumber( scripthandle, "Screen Setup", "Shadows",ud.shadows,false,false);
   SCRIPT_PutString( scripthandle, "Screen Setup", "Password",ud.pwlockout);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "Detail",ud.detail,false,false);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "Tilt",ud.screen_tilting,false,false);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "Messages",ud.fta_on,false,false);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "Out",ud.lockout,false,false);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "ScreenWidth",ScreenWidth,false,false);	// JBF 20031206
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "ScreenHeight",ScreenHeight,false,false);	// JBF 20031206
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "ScreenMode",ScreenMode,false,false);	// JBF 20031206
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "ScreenBPP",ScreenBPP,false,false);	// JBF 20040523
#ifdef RENDERTYPEWIN
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "MaxRefreshFreq",maxrefreshfreq,false,false);
#endif
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "GLTextureMode",gltexfiltermode,false,false);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "GLAnisotropy",glanisotropy,false,false);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "GLUseTextureCompr",glusetexcompr,false,false);
   SCRIPT_PutNumber( scripthandle, "Sound Setup", "FXVolume",FXVolume,false,false);
   SCRIPT_PutNumber( scripthandle, "Sound Setup", "MusicVolume",MusicVolume,false,false);
   SCRIPT_PutNumber( scripthandle, "Sound Setup", "SoundToggle",SoundToggle,false,false);
   SCRIPT_PutNumber( scripthandle, "Sound Setup", "VoiceToggle",VoiceToggle,false,false);
   SCRIPT_PutNumber( scripthandle, "Sound Setup", "AmbienceToggle",AmbienceToggle,false,false);
   SCRIPT_PutNumber( scripthandle, "Sound Setup", "MusicToggle",MusicToggle,false,false);
   SCRIPT_PutNumber( scripthandle, "Sound Setup", "ReverseStereo",ReverseStereo,false,false);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "ScreenSize",ud.screen_size,false,false);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "ScreenGamma",ud.brightness,false,false);
   SCRIPT_PutNumber( scripthandle, "Misc", "Executions",ud.executions,false,false);
   SCRIPT_PutNumber( scripthandle, "Misc", "RunMode",RunMode,false,false);
   SCRIPT_PutNumber( scripthandle, "Misc", "Crosshairs",ud.crosshair,false,false);
   SCRIPT_PutNumber( scripthandle, "Misc", "ShowLevelStats",ud.levelstats,false,false);
   SCRIPT_PutNumber( scripthandle, "Misc", "StatusBarScale",ud.statusbarscale,false,false);
   SCRIPT_PutNumber( scripthandle, "Controls", "MouseAimingFlipped",ud.mouseflip,false,false);
   SCRIPT_PutNumber( scripthandle, "Controls","MouseAiming",MouseAiming,false,false);
   //SCRIPT_PutNumber( scripthandle, "Controls","GameMouseAiming",(int32) ps[myconnectindex].aim_mode,false,false);
   SCRIPT_PutNumber( scripthandle, "Controls","AimingFlag",(long) myaimmode,false,false);
   SCRIPT_PutNumber( scripthandle, "Controls","RunKeyBehaviour",ud.runkey_mode,false,false);
   SCRIPT_PutNumber( scripthandle, "Controls","AutoAim",AutoAim,false,false);

   // JBF 20031211
   for(dummy=0;dummy<NUMGAMEFUNCTIONS;dummy++) {
       SCRIPT_PutDoubleString( scripthandle, "KeyDefinitions", CONFIG_FunctionNumToName(dummy),
		KB_ScanCodeToString(KeyboardKeys[dummy][0]), KB_ScanCodeToString(KeyboardKeys[dummy][1]));
   }

   for(dummy=0;dummy<10;dummy++)
   {
       Bsprintf(buf,"WeaponChoice%ld",dummy);
       SCRIPT_PutNumber( scripthandle, "Misc",buf,ud.wchoice[myconnectindex][dummy],false,false);
   }

	for (dummy=0;dummy<MAXMOUSEBUTTONS;dummy++) {
		Bsprintf(buf,"MouseButton%ld",dummy);
		SCRIPT_PutString( scripthandle,"Controls", buf, CONFIG_FunctionNumToName( MouseFunctions[dummy][0] ));

		if (dummy >= (MAXMOUSEBUTTONS-2)) continue;
		
		Bsprintf(buf,"MouseButtonClicked%ld",dummy);
		SCRIPT_PutString( scripthandle,"Controls", buf, CONFIG_FunctionNumToName( MouseFunctions[dummy][1] ));
	}
	for (dummy=0;dummy<MAXMOUSEAXES;dummy++) {
		Bsprintf(buf,"MouseAnalogAxes%ld",dummy);
		SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_AnalogNumToName( MouseAnalogueAxes[dummy] ));

		Bsprintf(buf,"MouseDigitalAxes%ld_0",dummy);
		SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(MouseDigitalFunctions[dummy][0]));

		Bsprintf(buf,"MouseDigitalAxes%ld_1",dummy);
		SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(MouseDigitalFunctions[dummy][1]));
		
		Bsprintf(buf,"MouseAnalogScale%ld",dummy);
		SCRIPT_PutNumber(scripthandle, "Controls", buf, MouseAnalogueScale[dummy], false, false);
	}
         dummy = CONTROL_GetMouseSensitivity();
         SCRIPT_PutNumber( scripthandle, "Controls","MouseSensitivity",dummy,false,false);

	for (dummy=0;dummy<MAXJOYBUTTONS;dummy++) {
		Bsprintf(buf,"JoystickButton%ld",dummy);
		SCRIPT_PutString( scripthandle,"Controls", buf, CONFIG_FunctionNumToName( JoystickFunctions[dummy][0] ));

		Bsprintf(buf,"JoystickButtonClicked%ld",dummy);
		SCRIPT_PutString( scripthandle,"Controls", buf, CONFIG_FunctionNumToName( JoystickFunctions[dummy][1] ));
	}
	for (dummy=0;dummy<MAXJOYAXES;dummy++) {
		Bsprintf(buf,"JoystickAnalogAxes%ld",dummy);
		SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_AnalogNumToName( JoystickAnalogueAxes[dummy] ));

		Bsprintf(buf,"JoystickDigitalAxes%ld_0",dummy);
		SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(JoystickDigitalFunctions[dummy][0]));

		Bsprintf(buf,"JoystickDigitalAxes%ld_1",dummy);
		SCRIPT_PutString(scripthandle, "Controls", buf, CONFIG_FunctionNumToName(JoystickDigitalFunctions[dummy][1]));
		
		Bsprintf(buf,"JoystickAnalogScale%ld",dummy);
		SCRIPT_PutNumber(scripthandle, "Controls", buf, JoystickAnalogueScale[dummy], false, false);

		Bsprintf(buf,"JoystickAnalogDead%ld",dummy);
		SCRIPT_PutNumber(scripthandle, "Controls", buf, JoystickAnalogueDead[dummy], false, false);

		Bsprintf(buf,"JoystickAnalogSaturate%ld",dummy);
		SCRIPT_PutNumber(scripthandle, "Controls", buf, JoystickAnalogueSaturate[dummy], false, false);
	}

   SCRIPT_PutString( scripthandle, "Comm Setup","PlayerName",&myname[0]);

   SCRIPT_Save (scripthandle, setupfilename);
   SCRIPT_Free (scripthandle);
   }


/*
 * vim:ts=4:sw=4:
 */


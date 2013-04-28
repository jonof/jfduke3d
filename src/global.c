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
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//-------------------------------------------------------------------------

#include "duke3d.h"

char *mymembuf;
char MusicPtr[72000*2];

short global_random;
short neartagsector, neartagwall, neartagsprite;

int neartaghitdist,lockclock,max_player_health,max_armour_amount,max_ammo_amount[MAX_WEAPONS];
// JBF: gc modified to default to Atomic ed. default when using 1.3d CONs
int gc=176;

// int temp_data[MAXSPRITES][6];
struct weaponhit hittype[MAXSPRITES];
short spriteq[1024],spriteqloc,spriteqamount=64;
struct animwalltype animwall[MAXANIMWALLS];
short numanimwalls;
int *animateptr[MAXANIMATES], animategoal[MAXANIMATES], animatevel[MAXANIMATES], animatecnt;
// int oanimateval[MAXANIMATES];
short animatesect[MAXANIMATES];
int msx[2048],msy[2048];
short cyclers[MAXCYCLERS][6],numcyclers;

char fta_quotes[NUMOFFIRSTTIMEACTIVE][64];

char tempbuf[2048];
unsigned char packbuf[576];

char buf[1024];

short camsprite;
short mirrorwall[64], mirrorsector[64], mirrorcnt;

int current_menu;

char betaname[80];

char level_names[44][33],level_file_names[44][128];
int partime[44],designertime[44];
char volume_names[4][33] = { "L.A. MELTDOWN", "LUNAR APOCALYPSE", "SHRAPNEL CITY", "" };
char skill_names[5][33] = { "PIECE OF CAKE", "LET'S ROCK", "COME GET SOME", "DAMN I'M GOOD", "" };

int soundsiz[NUM_SOUNDS];

short soundps[NUM_SOUNDS],soundpe[NUM_SOUNDS],soundvo[NUM_SOUNDS];
char soundm[NUM_SOUNDS],soundpr[NUM_SOUNDS];
char sounds[NUM_SOUNDS][14];

short title_zoom;

//fx_device device;

SAMPLE Sound[ NUM_SOUNDS ];
SOUNDOWNER SoundOwner[NUM_SOUNDS][4];

char loadfromgrouponly=0,earthquaketime;
int numplayersprites;

int fricxv,fricyv;
struct player_orig po[MAXPLAYERS];
struct player_struct ps[MAXPLAYERS];
struct user_defs ud;

char pus, pub;
char syncstat, syncval[MAXPLAYERS][MOVEFIFOSIZ];
int syncvalhead[MAXPLAYERS], syncvaltail, syncvaltottail;

input sync[MAXPLAYERS], loc;
input recsync[RECSYNCBUFSIZ];
int avgfvel, avgsvel, avgavel, avghorz, avgbits;


input inputfifo[MOVEFIFOSIZ][MAXPLAYERS];
input recsync[RECSYNCBUFSIZ];

int movefifosendplc;

  //Multiplayer syncing variables
short screenpeek;
int movefifoend[MAXPLAYERS];


    //Game recording variables

char playerreadyflag[MAXPLAYERS],ready2send;
char playerquitflag[MAXPLAYERS];
int vel, svel, angvel, horiz, ototalclock, respawnactortime=768, respawnitemtime=768, groupfile;

int script[MAXSCRIPTSIZE+16];
int *scriptptr,*insptr;
int *labelcode,labelcnt;
char *label,*labeltype;
char *textptr,error,warning,killit_flag;
int *actorscrptr[MAXTILES],*parsing_actor;
char *music_pointer;
char actortype[MAXTILES];


char display_mirror,typebuf[41];
int typebuflen;

char music_fn[4][11][13];
unsigned char music_select;
char env_music_fn[4][13];
char rtsplaying;


short weaponsandammosprites[15] = {
        RPGSPRITE,
        CHAINGUNSPRITE,
        DEVISTATORAMMO,
        RPGAMMO,
        RPGAMMO,
        JETPACK,
        SHIELD,
        FIRSTAID,
        STEROIDS,
        RPGAMMO,
        RPGAMMO,
        RPGSPRITE,
        RPGAMMO,
        FREEZESPRITE,
        FREEZEAMMO
    };

int impact_damage;

        //GLOBAL.C - replace the end "my's" with this
int myx, omyx, myxvel, myy, omyy, myyvel, myz, omyz, myzvel;
short myhoriz, omyhoriz, myhorizoff, omyhorizoff;
short myang, omyang, mycursectnum, myjumpingcounter,frags[MAXPLAYERS][MAXPLAYERS];

char myjumpingtoggle, myonground, myhardlanding, myreturntocenter;
signed char multiwho, multipos, multiwhat, multiflag;

int fakemovefifoplc,movefifoplc;
int myxbak[MOVEFIFOSIZ], myybak[MOVEFIFOSIZ], myzbak[MOVEFIFOSIZ];
int myhorizbak[MOVEFIFOSIZ],dukefriction = 0xcc00, show_shareware;

short myangbak[MOVEFIFOSIZ];
char myname[32],camerashitable,freezerhurtowner=0,lasermode=0;
char networkmode = -1, movesperpacket = 1,gamequit = 0,everyothertime;
int numfreezebounces=3,rpgblastradius,pipebombblastradius,tripbombblastradius,shrinkerblastradius,morterblastradius,bouncemineblastradius,seenineblastradius;
STATUSBARTYPE sbar;

int myminlag[MAXPLAYERS], mymaxlag, otherminlag, bufferjitter = 1;
short numclouds,clouds[128],cloudx[128],cloudy[128];
int cloudtotalclock = 0,totalmemory = 0;
int numinterpolations = 0, startofdynamicinterpolations = 0;
int oldipos[MAXINTERPOLATIONS];
int bakipos[MAXINTERPOLATIONS];
int *curipos[MAXINTERPOLATIONS];

int nextvoxid = 0;


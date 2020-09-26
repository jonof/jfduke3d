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

#ifdef __cplusplus
extern "C" {
#endif

#include "build.h"
#include "cache1d.h"
#include "pragmas.h"
#include "mmulti.h"

#include "baselayer.h"

#include "version.h"
#include "grpscan.h"

extern int conversion,shareware,gametype;
extern const char *gameeditionname;

#define GAMEDUKE 0
#define GAMENAM 1

#define VOLUMEALL (shareware==0)
#define PLUTOPAK (conversion==14)
#define VOLUMEONE (shareware==1)
#define NAM (gametype==GAMENAM)

// #define TEN
// #define BETA

// #define AUSTRALIA

#define MAXSLEEPDIST  16384
#define SLEEPTIME 24*64

#define BYTEVERSION_13	27
#define BYTEVERSION_14	116
#define BYTEVERSION_15	117
#define BYTEVERSION_JF	132	// increase by 3, because atomic GRP adds 1, and Shareware adds 2

#define BYTEVERSION (BYTEVERSION_JF+(PLUTOPAK?1:(VOLUMEONE<<1)))	// JBF 20040116: different data files give different versions

#define NUMPAGES 1

#define AUTO_AIM_ANGLE          48
#define RECSYNCBUFSIZ 2520   //2520 is the (LCM of 1-8)*3
#define MOVEFIFOSIZ 256

#define FOURSLEIGHT (1<<8)

#include "types.h"
#include "file_lib.h"
#include "develop.h"
#include "gamedefs.h"
#include "keyboard.h"
#include "util_lib.h"
#include "mathutil.h"
#include "function.h"
#include "fx_man.h"
#include "config.h"
#include "sounds.h"
#include "control.h"
#include "_rts.h"
#include "rts.h"
#include "soundefs.h"

#include "music.h"

#include "names.h"
#include "funct.h"


#define TICRATE (120)
#define TICSPERFRAME (TICRATE/26)

#define MAXCACHE1DSIZE (16*1048576)

// #define GC (TICSPERFRAME*44)

#define NUM_SOUNDS 450

/*
#pragma aux sgn =\
        "add ebx, ebx",\
        "sbb eax, eax",\
        "cmp eax, ebx",\
        "adc eax, 0",\
        parm [ebx]\
*/

#define    ALT_IS_PRESSED ( KB_KeyPressed( sc_RightAlt ) || KB_KeyPressed( sc_LeftAlt ) )
#define    SHIFTS_IS_PRESSED ( KB_KeyPressed( sc_RightShift ) || KB_KeyPressed( sc_LeftShift ) )
#define    RANDOMSCRAP EGS(s->sectnum,s->x+(TRAND&255)-128,s->y+(TRAND&255)-128,s->z-(8<<8)-(TRAND&8191),SCRAP6+(TRAND&15),-8,48,48,TRAND&2047,(TRAND&63)+64,-512-(TRAND&2047),i,5)

#define    BLACK 0
#define    DARKBLUE 1
#define    DARKGREEN 2
#define    DARKCYAN 3
#define    DARKRED 4
#define    DARKPURPLE 5
#define    BROWN 6
#define    LIGHTGRAY 7

#define    DARKGRAY 8
#define    BLUE 9
#define    GREEN 10
#define    CYAN 11
#define    RED 12
#define    PURPLE 13
#define    YELLOW 14
#define    WHITE 15

#define    PHEIGHT (38<<8)

// #define P(X) printf("%ld\n",X);

#define WAIT(X) ototalclock=totalclock+(X);while(totalclock<ototalclock)getpackets()


#define MODE_MENU       1
#define MODE_DEMO       2
#define MODE_GAME       4
#define MODE_EOL        8
#define MODE_TYPE       16
#define MODE_RESTART    32
#define MODE_SENDTOWHOM 64
#define MODE_END        128


#define MAXANIMWALLS 512
#define MAXINTERPOLATIONS 2048
#define NUMOFFIRSTTIMEACTIVE 192

#define MAXCYCLERS 256
#define MAXLABELS 4096
#define MAXLABELLEN 64
#define MAXSCRIPTSIZE 32768
#define MAXANIMATES 64

#define SP  sprite[i].yvel
#define SX  sprite[i].x
#define SY  sprite[i].y
#define SZ  sprite[i].z
#define SS  sprite[i].shade
#define PN  sprite[i].picnum
#define SA  sprite[i].ang
#define SV  sprite[i].xvel
#define ZV  sprite[i].zvel
#define RX  sprite[i].xrepeat
#define RY  sprite[i].yrepeat
#define OW  sprite[i].owner
#define CS  sprite[i].cstat
#define SH  sprite[i].extra
#define CX  sprite[i].xoffset
#define CY  sprite[i].yoffset
#define CD  sprite[i].clipdist
#define PL  sprite[i].pal
#define SLT  sprite[i].lotag
#define SHT  sprite[i].hitag
#define SECT sprite[i].sectnum

#define face_player 1
#define geth 2
#define getv 4
#define random_angle 8
#define face_player_slow 16
#define spin 32
#define face_player_smart 64
#define fleeenemy 128
#define jumptoplayer 257
#define seekplayer 512
#define furthestdir 1024
#define dodgebullet 4096

#define TRAND krand()


#define MAX_WEAPONS  12

#define KNEE_WEAPON          0
#define PISTOL_WEAPON        1
#define SHOTGUN_WEAPON       2
#define CHAINGUN_WEAPON      3
#define RPG_WEAPON           4
#define HANDBOMB_WEAPON      5
#define SHRINKER_WEAPON      6
#define DEVISTATOR_WEAPON    7
#define TRIPBOMB_WEAPON      8
#define FREEZE_WEAPON        9
#define HANDREMOTE_WEAPON    10
#define GROW_WEAPON          11

#define T1  hittype[i].temp_data[0]
#define T2  hittype[i].temp_data[1]
#define T3  hittype[i].temp_data[2]
#define T4  hittype[i].temp_data[3]
#define T5  hittype[i].temp_data[4]
#define T6  hittype[i].temp_data[5]

#define ESCESCAPE if(KB_KeyPressed( sc_Escape ) ) gameexit(" ");

#define IFWITHIN(B,E) if((PN)>=(B) && (PN)<=(E))
#define KILLIT(KX) {deletesprite(KX);goto BOLT;}


#define IFMOVING if(ssp(i,CLIPMASK0))
#define IFHIT j=ifhitbyweapon(i);if(j >= 0)
#define IFHITSECT j=ifhitsectors(s->sectnum);if(j >= 0)

#define AFLAMABLE(X) (X==BOX||X==TREE1||X==TREE2||X==TIRE||X==CONE)


#define IFSKILL1 if(player_skill<1)
#define IFSKILL2 if(player_skill<2)
#define IFSKILL3 if(player_skill<3)
#define IFSKILL4 if(player_skill<4)

#define rnd(X) ((TRAND>>8)>=(255-(X)))

typedef struct
{
    short i;
    int voice;
} SOUNDOWNER;

#define __USRHOOKS_H

enum USRHOOKS_Errors
   {
   USRHOOKS_Warning = -2,
   USRHOOKS_Error   = -1,
   USRHOOKS_Ok      = 0
   };

typedef struct
{
    signed char avel, horz;
    short fvel, svel;
    unsigned int bits;
} input;

#define sync dsync	// JBF 20040604: sync is a function on some platforms
extern input inputfifo[MOVEFIFOSIZ][MAXPLAYERS], sync[MAXPLAYERS];
extern input recsync[RECSYNCBUFSIZ];

extern int movefifosendplc;

typedef struct
{
    char *ptr;
    unsigned char lock;
    int  length, num;
	 int numall;	// total number of this sound played in any way
} SAMPLE;

struct animwalltype
{
        short wallnum;
        int tag;
};
extern struct animwalltype animwall[MAXANIMWALLS];
extern short numanimwalls,probey,lastprobey;

extern int typebuflen;
extern char typebuf[41];
extern int msx[2048],msy[2048];
extern short cyclers[MAXCYCLERS][6],numcyclers;
extern char myname[32];

struct user_defs
{
    unsigned char god,warp_on,cashman,eog,showallmap;
    unsigned char show_help,scrollmode,clipping;
    char user_name[MAXPLAYERS][32];
    char ridecule[10][40];
    char savegame[10][22];
    char pwlockout[128],rtsname[128];
    unsigned char overhead_on,last_overhead,showweapons;

    short pause_on,from_bonus;
    short camerasprite,last_camsprite;
    short last_level,secretlevel;

    int const_visibility,uw_framerate;
    int camera_time,folfvel,folavel,folx,foly,fola;
    int reccnt;

    int32 runkey_mode,statusbarscale,mouseaiming,weaponswitch;

    int32 entered_name,screen_tilting,shadows,fta_on,executions,auto_run;
    int32 coords,tickrate,levelstats,m_coop,coop,screen_size,lockout,crosshair;
    int32 wchoice[MAXPLAYERS][MAX_WEAPONS],playerai;

    int32 respawn_monsters,respawn_items,respawn_inventory,recstat,monsters_off,brightness;
    int32 m_respawn_items,m_respawn_monsters,m_respawn_inventory,m_recstat,m_monsters_off,detail;
    int32 m_ffire,ffire,m_player_skill,m_level_number,m_volume_number,multimode;
    int32 player_skill,level_number,volume_number,m_marker,marker,mouseflip;

};

struct player_orig
{
    int ox,oy,oz;
    short oa,os;
};


extern int numplayersprites;

void add_ammo( short, short, short, short );


extern int fricxv,fricyv;

struct player_struct
{
    int zoom,exitx,exity,loogiex[64],loogiey[64],numloogs,loogcnt;
    int posx, posy, posz, horiz, ohoriz, ohorizoff, invdisptime;
    int bobposx,bobposy,oposx,oposy,oposz,pyoff,opyoff;
    int posxv,posyv,poszv,last_pissed_time,truefz,truecz;
    int player_par,visibility;
    int bobcounter,weapon_sway;
    int pals_time,randomflamex,crack_time;

    int32 aim_mode,auto_aim,weaponswitch;

    short ang,oang,angvel,cursectnum,look_ang,last_extra,subweapon;
    short ammo_amount[MAX_WEAPONS],wackedbyactor,frag,fraggedself;

    short curr_weapon, last_weapon, tipincs, horizoff, wantweaponfire;
    short holoduke_amount,newowner,hurt_delay,hbomb_hold_delay;
    short jumping_counter,airleft,knee_incs,access_incs;
    short fta,ftq,access_wallnum,access_spritenum;
    short kickback_pic,got_access,weapon_ang,firstaid_amount;
    short somethingonplayer,on_crane,i,one_parallax_sectnum;
    short over_shoulder_on,random_club_frame,fist_incs;
    short one_eighty_count,cheat_phase;
    short dummyplayersprite,extra_extra8,quick_kick;
    short heat_amount,actorsqu,timebeforeexit,customexitsound;

    short weaprecs[16],weapreccnt;
    unsigned int interface_toggle_flag;

    short orotscrnang,rotscrnang,dead_flag,show_empty_weapon;	// JBF 20031220: added orotscrnang
    short scuba_amount,jetpack_amount,steroids_amount,shield_amount;
    short holoduke_on,pycount,weapon_pos,frag_ps;
    short transporter_hold,last_full_weapon,footprintshade,boot_amount;

    int scream_voice;

    unsigned char gm;
    unsigned char on_warping_sector,footprintcount;
    unsigned char hbomb_on,jumping_toggle,rapid_fire_hold,on_ground;
    char name[32];
    unsigned char inven_icon,buttonpalette;

    unsigned char jetpack_on,spritebridge,lastrandomspot;
    unsigned char scuba_on,footprintpal,heat_on;

    unsigned char  holster_weapon;
    unsigned char falling_counter;
    unsigned char  gotweapon[MAX_WEAPONS],refresh_inventory;
    unsigned char *palette;

    unsigned char toggle_key_flag,knuckle_incs; // ,select_dir;
    unsigned char walking_snd_toggle, palookup, hard_landing;
    unsigned char /*fire_flag,*/pals[3];
    unsigned char return_to_center;

    int max_secret_rooms,secret_rooms,max_actors_killed,actors_killed;
};

extern unsigned char tempbuf[2048];
extern unsigned char packbuf[576];
extern char buf[1024];  //use this for string prep

extern int gc,max_player_health,max_armour_amount,max_ammo_amount[MAX_WEAPONS];

extern int impact_damage,respawnactortime,respawnitemtime;

#define MOVFIFOSIZ 256

extern short spriteq[1024],spriteqloc,spriteqamount;
extern struct player_struct ps[MAXPLAYERS];
extern struct player_orig po[MAXPLAYERS];
extern struct user_defs ud;
extern short int global_random;
extern int scaredfallz;

extern char fta_quotes[NUMOFFIRSTTIMEACTIVE][64];
extern unsigned char scantoasc[128],ready2send;
extern unsigned char scantoascwithshift[128];

//extern fx_device device;
extern SAMPLE Sound[ NUM_SOUNDS ];
extern int32 VoiceToggle,AmbienceToggle;
extern SOUNDOWNER SoundOwner[NUM_SOUNDS][4];

extern unsigned char playerreadyflag[MAXPLAYERS],playerquitflag[MAXPLAYERS];
extern char sounds[NUM_SOUNDS][14];

	// JBF 20040531: adding 16 extra to the script so we have some leeway
	// to (hopefully) safely abort when hitting the limit
extern int script[MAXSCRIPTSIZE+16],*scriptptr,*insptr;
extern int *labelcode,labelcnt;
extern char *label,*labeltype;
extern char *textptr;
extern int error,warning,killit_flag;
extern int *actorscrptr[MAXTILES],*parsing_actor;
extern char actortype[MAXTILES];
extern char *music_pointer;

extern char ipath[80],opath[80];

extern char music_fn[4][11][13];
extern unsigned char music_select;
extern char env_music_fn[4][13];
extern short camsprite;

// extern char gotz;
extern char inspace(short sectnum);


struct weaponhit
{
    unsigned char cgg;
    short picnum,ang,extra,owner,movflag;
    short tempang,actorstayput,dispicnum;
    short timetosleep;
    int floorz,ceilingz,lastvx,lastvy,bposx,bposy,bposz;
    int temp_data[6];
};

extern struct weaponhit hittype[MAXSPRITES];

extern input loc;
extern input recsync[RECSYNCBUFSIZ];
extern int avgfvel, avgsvel, avgavel, avghorz, avgbits;

extern int numplayers, myconnectindex;	// JBF 20040716: was short until now
extern int connecthead, connectpoint2[MAXPLAYERS];   //Player linked list variables (indeces, not connection numbers)
extern short screenpeek;

extern int current_menu;
extern int tempwallptr,animatecnt;
extern int lockclock;
extern unsigned char display_mirror,loadfromgrouponly,rtsplaying;

extern int movefifoend[MAXPLAYERS], groupfile;
extern int ototalclock;

extern int *animateptr[MAXANIMATES], animategoal[MAXANIMATES];
extern int animatevel[MAXANIMATES];
// extern int oanimateval[MAXANIMATES];
extern short neartagsector, neartagwall, neartagsprite;
extern int neartaghitdist;
extern short animatesect[MAXANIMATES];
extern int movefifoplc, vel,svel,angvel,horiz;

extern short mirrorwall[64], mirrorsector[64], mirrorcnt;

#define NUMKEYS 19

#include "funct.h"

extern unsigned char screencapt;
extern short soundps[NUM_SOUNDS],soundpe[NUM_SOUNDS],soundvo[NUM_SOUNDS];
extern unsigned char soundpr[NUM_SOUNDS],soundm[NUM_SOUNDS];
extern int soundsiz[NUM_SOUNDS];
extern char level_names[44][33];
extern int partime[44],designertime[44];
extern char volume_names[4][33];
extern char skill_names[5][33];
extern char level_file_names[44][128];

extern int32 SoundToggle,MusicToggle;
extern short last_threehundred,lastsavedpos;
extern unsigned char restorepalette;

extern short buttonstat;
extern int cachecount;
extern char boardfilename[BMAX_PATH];
extern unsigned char waterpal[768],slimepal[768],titlepal[768],drealms[768],endingpal[768];
extern char betaname[80];
extern unsigned char cachedebug,earthquaketime;
extern unsigned char lumplockbyte[11];

extern char duke3dgrp[BMAX_PATH+1];

    //DUKE3D.H - replace the end "my's" with this
extern int myx, omyx, myxvel, myy, omyy, myyvel, myz, omyz, myzvel;
extern short myhoriz, omyhoriz, myhorizoff, omyhorizoff, globalskillsound;
extern short myang, omyang, mycursectnum, myjumpingcounter;
extern unsigned char myjumpingtoggle, myonground, myhardlanding,myreturntocenter;
extern int fakemovefifoplc;
extern int myxbak[MOVEFIFOSIZ], myybak[MOVEFIFOSIZ], myzbak[MOVEFIFOSIZ];
extern int myhorizbak[MOVEFIFOSIZ];
extern short myangbak[MOVEFIFOSIZ];

extern short weaponsandammosprites[15];


//DUKE3D.H:
typedef struct
{
        short frag[MAXPLAYERS], got_access, last_extra, shield_amount, curr_weapon;
        short ammo_amount[MAX_WEAPONS], holoduke_on;
        unsigned char gotweapon[MAX_WEAPONS], inven_icon, jetpack_on, heat_on;
        short firstaid_amount, steroids_amount, holoduke_amount, jetpack_amount;
        short heat_amount, scuba_amount, boot_amount;
        short last_weapon, weapon_pos, kickback_pic;

} STATUSBARTYPE;

extern STATUSBARTYPE sbar;
extern short frags[MAXPLAYERS][MAXPLAYERS];
extern int cameradist, cameraclock, dukefriction,show_shareware;
extern unsigned char movesperpacket;
extern unsigned char gamequit;

extern unsigned char pus,pub,camerashitable,freezerhurtowner,lasermode;
extern unsigned char syncstat, syncval[MAXPLAYERS][MOVEFIFOSIZ];
extern signed char multiwho, multipos, multiwhat, multiflag;
extern int syncvalhead[MAXPLAYERS], syncvaltail, syncvaltottail;
extern int numfreezebounces,rpgblastradius,pipebombblastradius,tripbombblastradius,shrinkerblastradius,morterblastradius,bouncemineblastradius,seenineblastradius;
extern unsigned char stereo,playerswhenstarted,everyothertime;
extern int myminlag[MAXPLAYERS], mymaxlag, otherminlag, bufferjitter;

extern int numinterpolations, startofdynamicinterpolations;
extern int oldipos[MAXINTERPOLATIONS];
extern int bakipos[MAXINTERPOLATIONS];
extern int *curipos[MAXINTERPOLATIONS];

extern short numclouds,clouds[128],cloudx[128],cloudy[128];
extern int cloudtotalclock,totalmemory;

extern int stereomode, stereowidth, stereopixelwidth;

extern int myaimmode, myaimstat, omyaimstat;

#define IFISGLMODE if (POLYMOST_RENDERMODE_POLYGL())
#define IFISSOFTMODE if (!POLYMOST_RENDERMODE_POLYGL())

void onvideomodechange(int newmode);

#define TILE_SAVESHOT (MAXTILES-1)
#define TILE_LOADSHOT (MAXTILES-3)
#define TILE_TILT     (MAXTILES-2)
#define TILE_ANIM     (MAXTILES-4)
#define TILE_VIEWSCR  (MAXTILES-5)

extern unsigned char useprecache;

#define NAM_GRENADE_LIFETIME	120
#define NAM_GRENADE_LIFETIME_VAR	30

struct startwin_settings {
    int fullscreen;
    int xdim3d, ydim3d, bpp3d;
    int forcesetup;
    int usemouse, usejoy;
    int samplerate, bitspersample, channels;

    struct grpfile *selectedgrp;

    int numplayers;
    char *joinhost;
};

#ifdef __cplusplus
}
#endif


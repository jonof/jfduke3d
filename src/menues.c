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
#include "mouse.h"
#include "animlib.h"
#include "osd.h"
#include <sys/stat.h>
#include <assert.h>


struct savehead {
    char name[19];
    int32 numplr,volnum,levnum,plrskl;
    char boardfn[BMAX_PATH];
};

extern short inputloc;
extern int recfilep;
//extern char vgacompatible;
short probey=0,lastprobey=0,last_menu,globalskillsound=-1;
short sh,onbar,deletespot;
short last_zero,last_fifty,last_threehundred = 0;
UserInput uinfo;

static char fileselect = 1, menunamecnt, menuname[256][64], curpath[80], menupath[80];

static CACHE1D_FIND_REC *finddirs=NULL, *findfiles=NULL, *finddirshigh=NULL, *findfileshigh=NULL;
static int numdirs=0, numfiles=0;
static int currentlist=0;

static int function, whichkey;
static int changesmade, newvidmode, curvidmode;
#define MAXVIDSETS 16
static struct vidset {
    unsigned char bpp;
    unsigned char fs;
} vidsets[MAXVIDSETS];
static int curvidset, newvidset, numvidsets;

static const char *mousebuttonnames[] = { "Left", "Right", "Middle", "Thumb", "Wheel Down", "Wheel Up" };


void cmenu(short cm)
{
    current_menu = cm;

    if( (cm >= 1000 && cm <= 1009) )
        return;

    if( cm == 0 )
        probey = last_zero;
    else if(cm == 50)
        probey = last_fifty;
    else if(cm >= 300 && cm < 400)
        probey = last_threehundred;
    else if(cm == 110)
        probey = 1;
    else probey = 0;
    lastprobey = -1;

    uinfo.dir = dir_None;
    uinfo.button0 = uinfo.button1 = FALSE;
}

void getangplayers(short snum)
{
    short i,a;

    for(i=connecthead;i>=0;i=connectpoint2[i])
    {
        if(i != snum)
        {
            a = ps[snum].ang+getangle(ps[i].posx-ps[snum].posx,ps[i].posy-ps[snum].posy);
            a = a-1024;
            rotatesprite(
                (320<<15) + (((sintable[(a+512)&2047])>>7)<<15),
                (320<<15) - (((sintable[a&2047])>>8)<<15),
                klabs(sintable[((a>>1)+768)&2047]<<2),0,APLAYER,0,ps[i].palookup,0,0,0,xdim-1,ydim-1);
        }
    }
}

void readsavenames(void)
{
    int bv, dummy;
    short i;
    char fn[13];
    int fil;

    strcpy(fn,"game_.sav");

    for (i=0;i<10;i++)
    {
        fn[4] = i+'0';
        ud.savegame[i][0] = 0;
        if ((fil = kopen4load(fn,0)) == -1) continue;
        do
        {
            if (kdfread(&bv,sizeof(int),1,fil) != 1) break;
            if (bv != BYTEVERSION) break;
            if (kdfread(&dummy,sizeof(int),1,fil) != 1) break;
            if (kdfread(&ud.savegame[i][0],19,1,fil) != 1) ud.savegame[i][0] = 0;
        } while(0);
        kclose(fil);
    }
}

int loadpheader(char spot,struct savehead *saveh)
{
    int i;
    char fn[13];
    int fil;
    int bv;

    strcpy(fn, "game0.sav");
    fn[4] = spot+'0';

    if ((fil = kopen4load(fn,0)) == -1) return(-1);

    walock[TILE_LOADSHOT] = 255;

    if (kdfread(&bv,4,1,fil) != 1) goto corrupt;
    if(bv != BYTEVERSION) {
        FTA(114,&ps[myconnectindex]);
        kclose(fil);
        return 1;
    }

    if (kdfread(&saveh->numplr,sizeof(int32),1,fil) != 1) goto corrupt;

    if (kdfread(saveh->name,19,1,fil) != 1) goto corrupt;
    if (kdfread(&saveh->volnum,sizeof(int32),1,fil) != 1) goto corrupt;
    if (kdfread(&saveh->levnum,sizeof(int32),1,fil) != 1) goto corrupt;
    if (kdfread(&saveh->plrskl,sizeof(int32),1,fil) != 1) goto corrupt;
    if (kdfread(saveh->boardfn,BMAX_PATH,1,fil) != 1) goto corrupt;

    if (waloff[TILE_LOADSHOT] == 0) allocache((void **)&waloff[TILE_LOADSHOT],320*200,&walock[TILE_LOADSHOT]);
    tilesizx[TILE_LOADSHOT] = 200; tilesizy[TILE_LOADSHOT] = 320;
    if (kdfread((char *)waloff[TILE_LOADSHOT],320,200,fil) != 200) goto corrupt;
#if USE_POLYMOST && USE_OPENGL
    invalidatetile(TILE_LOADSHOT,0,255);
#endif

    kclose(fil);

    return(0);
corrupt:
    kclose(fil);
    return 1;
}


int loadplayer(signed char spot)
{
    short k;
    char fn[13];
    char mpfn[13];
    char *fnptr;
    int fil, bv, i, j, x;
    int32 nump;
    int ptrbuf[MAXTILES];
    
    assert(MAXTILES > MAXANIMATES);
    
    strcpy(fn, "game0.sav");
    strcpy(mpfn, "gameA_00.sav");

    if(spot < 0)
    {
        multiflag = 1;
        multiwhat = 0;
        multipos = -spot-1;
        return -1;
    }

    if( multiflag == 2 && multiwho != myconnectindex )
    {
        fnptr = mpfn;
        mpfn[4] = spot + 'A';

        if(ud.multimode > 9)
        {
            mpfn[6] = (multiwho/10) + '0';
            mpfn[7] = (multiwho%10) + '0';
        }
        else mpfn[7] = multiwho + '0';
    }
    else
    {
        fnptr = fn;
        fn[4] = spot + '0';
    }

    if ((fil = kopen4load(fnptr,0)) == -1) return(-1);

    ready2send = 0;

    if (kdfread(&bv,4,1,fil) != 1) return -1;
    if(bv != BYTEVERSION)
    {
        FTA(114,&ps[myconnectindex]);
        kclose(fil);
        ototalclock = totalclock;
        ready2send = 1;
        return 1;
    }

    if (kdfread(&nump,sizeof(nump),1,fil) != 1) return -1;
    if(nump != numplayers)
    {
        kclose(fil);
        ototalclock = totalclock;
        ready2send = 1;
        FTA(124,&ps[myconnectindex]);
        return 1;
    }

    if(numplayers > 1)
    {
        pub = NUMPAGES;
        pus = NUMPAGES;
        vscrn();
        drawbackground();
        menutext(160,100,0,0,"LOADING...");
        nextpage();
    }

    waitforeverybody();

    FX_StopAllSounds();
    clearsoundlocks();
    stopmusic();

    if(numplayers > 1) {
        if (kdfread(&buf,19,1,fil) != 1) goto corrupt;
    } else {
        if (kdfread(&ud.savegame[spot][0],19,1,fil) != 1) goto corrupt;
    }

    //     music_changed = (music_select != (ud.volume_number*11) + ud.level_number);

    if (kdfread(&ud.volume_number,sizeof(ud.volume_number),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.level_number,sizeof(ud.level_number),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.player_skill,sizeof(ud.player_skill),1,fil) != 1) goto corrupt;
    if (kdfread(&boardfilename[0],BMAX_PATH,1,fil) != 1) goto corrupt;

    ud.m_level_number = ud.level_number;
    ud.m_volume_number = ud.volume_number;
    ud.m_player_skill = ud.player_skill;

    //Fake read because lseek won't work with compression
    walock[TILE_LOADSHOT] = 1;
    if (waloff[TILE_LOADSHOT] == 0) allocache((void **)&waloff[TILE_LOADSHOT],320*200,&walock[TILE_LOADSHOT]);
    tilesizx[TILE_LOADSHOT] = 200; tilesizy[TILE_LOADSHOT] = 320;
    if (kdfread((char *)waloff[TILE_LOADSHOT],320,200,fil) != 200) goto corrupt;
#if USE_POLYMOST && USE_OPENGL
    invalidatetile(TILE_LOADSHOT,0,255);
#endif

    if (kdfread(&numwalls,2,1,fil) != 1) goto corrupt;
    if (kdfread(&wall[0],sizeof(walltype),MAXWALLS,fil) != MAXWALLS) goto corrupt;
    if (kdfread(&numsectors,2,1,fil) != 1) goto corrupt;
    if (kdfread(&sector[0],sizeof(sectortype),MAXSECTORS,fil) != MAXSECTORS) goto corrupt;
    if (kdfread(&sprite[0],sizeof(spritetype),MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&spriteext[0],sizeof(spriteexttype),MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&headspritesect[0],2,MAXSECTORS+1,fil) != MAXSECTORS+1) goto corrupt;
    if (kdfread(&prevspritesect[0],2,MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&nextspritesect[0],2,MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&headspritestat[0],2,MAXSTATUS+1,fil) != MAXSTATUS+1) goto corrupt;
    if (kdfread(&prevspritestat[0],2,MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&nextspritestat[0],2,MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&numcyclers,sizeof(numcyclers),1,fil) != 1) goto corrupt;
    if (kdfread(&cyclers[0][0],12,MAXCYCLERS,fil) != MAXCYCLERS) goto corrupt;
    if (kdfread(ps,sizeof(ps),1,fil) != 1) goto corrupt;
    if (kdfread(po,sizeof(po),1,fil) != 1) goto corrupt;
    if (kdfread(&numanimwalls,sizeof(numanimwalls),1,fil) != 1) goto corrupt;
    if (kdfread(&animwall,sizeof(animwall),1,fil) != 1) goto corrupt;
    if (kdfread(&msx[0],sizeof(int),sizeof(msx)/sizeof(int),fil) != sizeof(msx)/sizeof(int)) goto corrupt;
    if (kdfread(&msy[0],sizeof(int),sizeof(msy)/sizeof(int),fil) != sizeof(msy)/sizeof(int)) goto corrupt;
    if (kdfread((short *)&spriteqloc,sizeof(short),1,fil) != 1) goto corrupt;
    if (kdfread((short *)&spriteqamount,sizeof(short),1,fil) != 1) goto corrupt;
    if (kdfread((short *)&spriteq[0],sizeof(short),spriteqamount,fil) != spriteqamount) goto corrupt;
    if (kdfread(&mirrorcnt,sizeof(short),1,fil) != 1) goto corrupt;
    if (kdfread(&mirrorwall[0],sizeof(short),64,fil) != 64) goto corrupt;
    if (kdfread(&mirrorsector[0],sizeof(short),64,fil) != 64) goto corrupt;
    if (kdfread(&show2dsector[0],sizeof(char),MAXSECTORS>>3,fil) != (MAXSECTORS>>3)) goto corrupt;
    if (kdfread(&actortype[0],sizeof(char),MAXTILES,fil) != MAXTILES) goto corrupt;

    if (kdfread(&numclouds,sizeof(numclouds),1,fil) != 1) goto corrupt;
    if (kdfread(&clouds[0],sizeof(short)<<7,1,fil) != 1) goto corrupt;
    if (kdfread(&cloudx[0],sizeof(short)<<7,1,fil) != 1) goto corrupt;
    if (kdfread(&cloudy[0],sizeof(short)<<7,1,fil) != 1) goto corrupt;

    if (kdfread(&script[0],4,MAXSCRIPTSIZE,fil) != MAXSCRIPTSIZE) goto corrupt;

    if (kdfread(&ptrbuf[0],4,MAXTILES,fil) != MAXTILES) goto corrupt;
    for(i=0;i<MAXTILES;i++)
        if(ptrbuf[i])
        {
            actorscrptr[i] = (int *)((intptr_t)&script[0] + ptrbuf[i]);
        }

    if (kdfread(&hittype[0],sizeof(struct weaponhit),MAXSPRITES,fil) != MAXSPRITES) goto corrupt;

    if (kdfread(&lockclock,sizeof(lockclock),1,fil) != 1) goto corrupt;
    if (kdfread(&pskybits,sizeof(pskybits),1,fil) != 1) goto corrupt;
    if (kdfread(&pskyoff[0],sizeof(pskyoff[0]),MAXPSKYTILES,fil) != MAXPSKYTILES) goto corrupt;

    if (kdfread(&animatecnt,sizeof(animatecnt),1,fil) != 1) goto corrupt;
    if (kdfread(&animatesect[0],2,MAXANIMATES,fil) != MAXANIMATES) goto corrupt;
    if (kdfread(&ptrbuf[0],4,MAXANIMATES,fil) != MAXANIMATES) goto corrupt;
    for(i = animatecnt-1;i>=0;i--) animateptr[i] = (int *)((intptr_t)&sector[0] + ptrbuf[i]);
    if (kdfread(&animategoal[0],4,MAXANIMATES,fil) != MAXANIMATES) goto corrupt;
    if (kdfread(&animatevel[0],4,MAXANIMATES,fil) != MAXANIMATES) goto corrupt;

    if (kdfread(&earthquaketime,sizeof(earthquaketime),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.from_bonus,sizeof(ud.from_bonus),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.secretlevel,sizeof(ud.secretlevel),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.respawn_monsters,sizeof(ud.respawn_monsters),1,fil) != 1) goto corrupt;
    ud.m_respawn_monsters = ud.respawn_monsters;
    if (kdfread(&ud.respawn_items,sizeof(ud.respawn_items),1,fil) != 1) goto corrupt;
    ud.m_respawn_items = ud.respawn_items;
    if (kdfread(&ud.respawn_inventory,sizeof(ud.respawn_inventory),1,fil) != 1) goto corrupt;
    ud.m_respawn_inventory = ud.respawn_inventory;

    if (kdfread(&ud.god,sizeof(ud.god),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.auto_run,sizeof(ud.auto_run),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.crosshair,sizeof(ud.crosshair),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.monsters_off,sizeof(ud.monsters_off),1,fil) != 1) goto corrupt;
    ud.m_monsters_off = ud.monsters_off;
    if (kdfread(&ud.last_level,sizeof(ud.last_level),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.eog,sizeof(ud.eog),1,fil) != 1) goto corrupt;

    if (kdfread(&ud.coop,sizeof(ud.coop),1,fil) != 1) goto corrupt;
    ud.m_coop = ud.coop;
    if (kdfread(&ud.marker,sizeof(ud.marker),1,fil) != 1) goto corrupt;
    ud.m_marker = ud.marker;
    if (kdfread(&ud.ffire,sizeof(ud.ffire),1,fil) != 1) goto corrupt;
    ud.m_ffire = ud.ffire;

    if (kdfread(&camsprite,sizeof(camsprite),1,fil) != 1) goto corrupt;
    if (kdfread(&connecthead,sizeof(connecthead),1,fil) != 1) goto corrupt;
    if (kdfread(connectpoint2,sizeof(connectpoint2),1,fil) != 1) goto corrupt;
    if (kdfread(&numplayersprites,sizeof(numplayersprites),1,fil) != 1) goto corrupt;
    if (kdfread((short *)&frags[0][0],sizeof(frags),1,fil) != 1) goto corrupt;

    if (kdfread(&randomseed,sizeof(randomseed),1,fil) != 1) goto corrupt;
    if (kdfread(&global_random,sizeof(global_random),1,fil) != 1) goto corrupt;
    if (kdfread(&parallaxyscale,sizeof(parallaxyscale),1,fil) != 1) goto corrupt;

     kclose(fil);

     if(ps[myconnectindex].over_shoulder_on != 0)
     {
         cameradist = 0;
         cameraclock = 0;
         ps[myconnectindex].over_shoulder_on = 1;
     }

     screenpeek = myconnectindex;

     clearbufbyte(gotpic,sizeof(gotpic),0L);
     clearsoundlocks();
         cacheit();

     music_select = (ud.volume_number*11) + ud.level_number;
     playmusic(&music_fn[0][music_select][0]);

     ps[myconnectindex].gm = MODE_GAME;
         ud.recstat = 0;

     if(ps[myconnectindex].jetpack_on)
         spritesound(DUKE_JETPACK_IDLE,ps[myconnectindex].i);

     restorepalette = 1;
     setpal(&ps[myconnectindex]);
     vscrn();

     FX_SetReverb(0);

     if(ud.lockout == 0)
     {
         for(x=0;x<numanimwalls;x++)
             if( wall[animwall[x].wallnum].extra >= 0 )
                 wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
     }
     else
     {
         for(x=0;x<numanimwalls;x++)
             switch(wall[animwall[x].wallnum].picnum)
         {
             case FEMPIC1:
                 wall[animwall[x].wallnum].picnum = BLANKSCREEN;
                 break;
             case FEMPIC2:
             case FEMPIC3:
                 wall[animwall[x].wallnum].picnum = SCREENBREAK6;
                 break;
         }
     }

     numinterpolations = 0;
     startofdynamicinterpolations = 0;

     k = headspritestat[3];
     while(k >= 0)
     {
        switch(sprite[k].lotag)
        {
            case 31:
                setinterpolation(&sector[sprite[k].sectnum].floorz);
                break;
            case 32:
                setinterpolation(&sector[sprite[k].sectnum].ceilingz);
                break;
            case 25:
                setinterpolation(&sector[sprite[k].sectnum].floorz);
                setinterpolation(&sector[sprite[k].sectnum].ceilingz);
                break;
            case 17:
                setinterpolation(&sector[sprite[k].sectnum].floorz);
                setinterpolation(&sector[sprite[k].sectnum].ceilingz);
                break;
            case 0:
            case 5:
            case 6:
            case 11:
            case 14:
            case 15:
            case 16:
            case 26:
            case 30:
                setsectinterpolate(k);
                break;
        }

        k = nextspritestat[k];
     }

     for(i=numinterpolations-1;i>=0;i--) bakipos[i] = *curipos[i];
     for(i = animatecnt-1;i>=0;i--)
         setinterpolation(animateptr[i]);

     show_shareware = 0;
     everyothertime = 0;

     clearbufbyte(playerquitflag,MAXPLAYERS,0x01010101);

     resetmys();

     ready2send = 1;

     flushpackets();
     clearfifo();
     waitforeverybody();

     resettimevars();

     return(0);
corrupt:
     Bsprintf(buf,"Save game file \"%s\" is corrupt.",fnptr);
     gameexit(buf);
     return -1;
}

int saveplayer(signed char spot)
{
    int i, j;
    char fn[13];
    char mpfn[13];
    char *fnptr;
    FILE *fil;
    int bv = BYTEVERSION;
    int ptrbuf[MAXTILES];

    assert(MAXTILES > MAXANIMATES);
    
    strcpy(fn, "game0.sav");
    strcpy(mpfn, "gameA_00.sav");

    if(spot < 0)
    {
        multiflag = 1;
        multiwhat = 1;
        multipos = -spot-1;
        return -1;
    }

    waitforeverybody();

    if( multiflag == 2 && multiwho != myconnectindex )
    {
        fnptr = mpfn;
        mpfn[4] = spot + 'A';

        if(ud.multimode > 9)
        {
            mpfn[6] = (multiwho/10) + '0';
            mpfn[7] = multiwho + '0';
        }
        else mpfn[7] = multiwho + '0';
    }
    else
    {
        fnptr = fn;
        fn[4] = spot + '0';
    }

    if ((fil = fopen(fnptr,"wb")) == 0) return(-1);

    ready2send = 0;

    dfwrite(&bv,4,1,fil);
    dfwrite(&ud.multimode,sizeof(ud.multimode),1,fil);

    dfwrite(&ud.savegame[spot][0],19,1,fil);
    dfwrite(&ud.volume_number,sizeof(ud.volume_number),1,fil);
    dfwrite(&ud.level_number,sizeof(ud.level_number),1,fil);
    dfwrite(&ud.player_skill,sizeof(ud.player_skill),1,fil);
    dfwrite(&boardfilename[0],BMAX_PATH,1,fil);

    if (!waloff[TILE_SAVESHOT]) {
        walock[TILE_SAVESHOT] = 254;
        allocache((void **)&waloff[TILE_SAVESHOT],200*320,&walock[TILE_SAVESHOT]);
        clearbuf((void*)waloff[TILE_SAVESHOT],(200*320)/4,0);
        walock[TILE_SAVESHOT] = 1;
    }
    dfwrite((char *)waloff[TILE_SAVESHOT],320,200,fil);

    dfwrite(&numwalls,2,1,fil);
    dfwrite(&wall[0],sizeof(walltype),MAXWALLS,fil);
    dfwrite(&numsectors,2,1,fil);
    dfwrite(&sector[0],sizeof(sectortype),MAXSECTORS,fil);
    dfwrite(&sprite[0],sizeof(spritetype),MAXSPRITES,fil);
    dfwrite(&spriteext[0],sizeof(spriteexttype),MAXSPRITES,fil);
    dfwrite(&headspritesect[0],2,MAXSECTORS+1,fil);
    dfwrite(&prevspritesect[0],2,MAXSPRITES,fil);
    dfwrite(&nextspritesect[0],2,MAXSPRITES,fil);
    dfwrite(&headspritestat[0],2,MAXSTATUS+1,fil);
    dfwrite(&prevspritestat[0],2,MAXSPRITES,fil);
    dfwrite(&nextspritestat[0],2,MAXSPRITES,fil);
    dfwrite(&numcyclers,sizeof(numcyclers),1,fil);
    dfwrite(&cyclers[0][0],12,MAXCYCLERS,fil);
    dfwrite(ps,sizeof(ps),1,fil);
    dfwrite(po,sizeof(po),1,fil);
    dfwrite(&numanimwalls,sizeof(numanimwalls),1,fil);
    dfwrite(&animwall,sizeof(animwall),1,fil);
    dfwrite(&msx[0],sizeof(int),sizeof(msx)/sizeof(int),fil);
    dfwrite(&msy[0],sizeof(int),sizeof(msy)/sizeof(int),fil);
    dfwrite(&spriteqloc,sizeof(short),1,fil);
    dfwrite(&spriteqamount,sizeof(short),1,fil);
    dfwrite(&spriteq[0],sizeof(short),spriteqamount,fil);
    dfwrite(&mirrorcnt,sizeof(short),1,fil);
    dfwrite(&mirrorwall[0],sizeof(short),64,fil);
    dfwrite(&mirrorsector[0],sizeof(short),64,fil);
    dfwrite(&show2dsector[0],sizeof(char),MAXSECTORS>>3,fil);
    dfwrite(&actortype[0],sizeof(char),MAXTILES,fil);

    dfwrite(&numclouds,sizeof(numclouds),1,fil);
    dfwrite(&clouds[0],sizeof(short)<<7,1,fil);
    dfwrite(&cloudx[0],sizeof(short)<<7,1,fil);
    dfwrite(&cloudy[0],sizeof(short)<<7,1,fil);

    dfwrite(&script[0],4,MAXSCRIPTSIZE,fil);

    memset(ptrbuf, 0, sizeof(ptrbuf));
    for(i=0;i<MAXTILES;i++)
        if(actorscrptr[i])
        {
            ptrbuf[i] = (int)((intptr_t)actorscrptr[i] - (intptr_t)&script[0]);
        }
    dfwrite(&ptrbuf[0],4,MAXTILES,fil);

    dfwrite(&hittype[0],sizeof(struct weaponhit),MAXSPRITES,fil);

    dfwrite(&lockclock,sizeof(lockclock),1,fil);
    dfwrite(&pskybits,sizeof(pskybits),1,fil);
    dfwrite(&pskyoff[0],sizeof(pskyoff[0]),MAXPSKYTILES,fil);
    dfwrite(&animatecnt,sizeof(animatecnt),1,fil);
    dfwrite(&animatesect[0],2,MAXANIMATES,fil);
    for(i = animatecnt-1;i>=0;i--) ptrbuf[i] = (int)((intptr_t)animateptr[i] - (intptr_t)&sector[0]);
    dfwrite(&ptrbuf[0],4,MAXANIMATES,fil);
    dfwrite(&animategoal[0],4,MAXANIMATES,fil);
    dfwrite(&animatevel[0],4,MAXANIMATES,fil);

    dfwrite(&earthquaketime,sizeof(earthquaketime),1,fil);
    dfwrite(&ud.from_bonus,sizeof(ud.from_bonus),1,fil);
    dfwrite(&ud.secretlevel,sizeof(ud.secretlevel),1,fil);
    dfwrite(&ud.respawn_monsters,sizeof(ud.respawn_monsters),1,fil);
    dfwrite(&ud.respawn_items,sizeof(ud.respawn_items),1,fil);
    dfwrite(&ud.respawn_inventory,sizeof(ud.respawn_inventory),1,fil);
    dfwrite(&ud.god,sizeof(ud.god),1,fil);
    dfwrite(&ud.auto_run,sizeof(ud.auto_run),1,fil);
    dfwrite(&ud.crosshair,sizeof(ud.crosshair),1,fil);
    dfwrite(&ud.monsters_off,sizeof(ud.monsters_off),1,fil);
    dfwrite(&ud.last_level,sizeof(ud.last_level),1,fil);
    dfwrite(&ud.eog,sizeof(ud.eog),1,fil);
    dfwrite(&ud.coop,sizeof(ud.coop),1,fil);
    dfwrite(&ud.marker,sizeof(ud.marker),1,fil);
    dfwrite(&ud.ffire,sizeof(ud.ffire),1,fil);
    dfwrite(&camsprite,sizeof(camsprite),1,fil);
    dfwrite(&connecthead,sizeof(connecthead),1,fil);
    dfwrite(connectpoint2,sizeof(connectpoint2),1,fil);
    dfwrite(&numplayersprites,sizeof(numplayersprites),1,fil);
    dfwrite((short *)&frags[0][0],sizeof(frags),1,fil);

    dfwrite(&randomseed,sizeof(randomseed),1,fil);
    dfwrite(&global_random,sizeof(global_random),1,fil);
    dfwrite(&parallaxyscale,sizeof(parallaxyscale),1,fil);

    fclose(fil);

    if(ud.multimode < 2)
    {
    strcpy(fta_quotes[122],"GAME SAVED");
    FTA(122,&ps[myconnectindex]);
    }

    ready2send = 1;

    waitforeverybody();

    ototalclock = totalclock;

    return(0);
}

static int probe_(int type,int x,int y,int i,int n)
{
    short centre;

    if( x == (320>>1) )
        centre = 320>>2;
    else centre = 0;

    if (uinfo.dir == dir_North)
    {
        sound(KICK_HIT);
        probey--;
        if(probey < 0) probey = n-1;
    }
    else if (uinfo.dir == dir_South)
    {
        sound(KICK_HIT);
        probey++;
    }

    if(probey >= n)
        probey = 0;

    if (x|y) {
        if(centre)
        {
            rotatesprite(((320>>1)+(centre>>1)+70)<<16,(y+(probey*i)-4)<<16,65536L>>type,0,SPINNINGNUKEICON+6-((6+(totalclock>>3))%7),sh,0,10,0,0,xdim-1,ydim-1);
            rotatesprite(((320>>1)-(centre>>1)-70)<<16,(y+(probey*i)-4)<<16,65536L>>type,0,SPINNINGNUKEICON+((totalclock>>3)%7),sh,0,10,0,0,xdim-1,ydim-1);
        }
        else
            rotatesprite((x<<16)-((tilesizx[BIGFNTCURSOR]-4)<<(16-type)),(y+(probey*i)-(4>>type))<<16,65536L>>type,0,SPINNINGNUKEICON+(((totalclock>>3))%7),sh,0,10,0,0,xdim-1,ydim-1);
    }

    if (uinfo.button0)
    {
        if(current_menu != 110)
            sound(PISTOL_BODYHIT);
        KB_ClearKeyDown(sc_Space);
        KB_ClearKeyDown(sc_Enter);
        KB_ClearKeyDown(sc_kpad_Enter);
        return(probey);
    }
    else if(uinfo.button1)
    {
        onbar = 0;
        sound(EXITMENUSOUND);
        KB_ClearKeyDown(sc_Escape);
        return(-1);
    }
    else
    {
        if(onbar == 0) return(-probey-2);
        if (uinfo.dir == dir_West)
            return(probey);
        else if (uinfo.dir == dir_East)
            return(probey);
        else return(-probey-2);
    }
}
int probe(int x,int y,int i,int n) { return probe_(0,x,y,i,n); }
int probesm(int x,int y,int i,int n) { return probe_(1,x,y,i,n); }

int menutext(int x,int y,short s,short p,const char *t)
{
    short i, ac, centre;

    y -= 12;

    i = centre = 0;

    if( x == (320>>1) )
    {
        while( *(t+i) )
        {
            if(*(t+i) == ' ')
            {
                centre += 5;
                i++;
                continue;
            }
            ac = 0;
            if(*(t+i) >= '0' && *(t+i) <= '9')
                ac = *(t+i) - '0' + BIGALPHANUM-10;
            else if(*(t+i) >= 'a' && *(t+i) <= 'z')
                ac = toupper(*(t+i)) - 'A' + BIGALPHANUM;
            else if(*(t+i) >= 'A' && *(t+i) <= 'Z')
                ac = *(t+i) - 'A' + BIGALPHANUM;
            else switch(*(t+i))
            {
                case '-':
                    ac = BIGALPHANUM-11;
                    break;
                case '.':
                    ac = BIGPERIOD;
                    break;
                case '\'':
                    ac = BIGAPPOS;
                    break;
                case ',':
                    ac = BIGCOMMA;
                    break;
                case '!':
                    ac = BIGX;
                    break;
                case '?':
                    ac = BIGQ;
                    break;
                case ';':
                    ac = BIGSEMI;
                    break;
                case ':':
                    ac = BIGSEMI;
                    break;
                default:
                    centre += 5;
                    i++;
                    continue;
            }

            centre += tilesizx[ac]-1;
            i++;
        }
    }

    if(centre)
        x = (320-centre-10)>>1;

    while(*t)
    {
        if(*t == ' ') {x+=5;t++;continue;}
        ac = 0;
        if(*t >= '0' && *t <= '9')
            ac = *t - '0' + BIGALPHANUM-10;
        else if(*t >= 'a' && *t <= 'z')
            ac = toupper(*t) - 'A' + BIGALPHANUM;
        else if(*t >= 'A' && *t <= 'Z')
            ac = *t - 'A' + BIGALPHANUM;
        else switch(*t)
        {
            case '-':
                ac = BIGALPHANUM-11;
                break;
            case '.':
                ac = BIGPERIOD;
                break;
            case ',':
                ac = BIGCOMMA;
                break;
            case '!':
                ac = BIGX;
                break;
            case '\'':
                ac = BIGAPPOS;
                break;
            case '?':
                ac = BIGQ;
                break;
            case ';':
                ac = BIGSEMI;
                break;
            case ':':
                ac = BIGCOLIN;
                break;
            default:
                x += 5;
                t++;
                continue;
        }

        rotatesprite(x<<16,y<<16,65536L,0,ac,s,p,10+16,0,0,xdim-1,ydim-1);

        x += tilesizx[ac];
        t++;
    }
    return (x);
}

int menutextc(int x,int y,short s,short p,const char *t)
{
    short i, ac, centre;

    s += 8;
    y -= 12;

    i = centre = 0;

//    if( x == (320>>1) )
    {
        while( *(t+i) )
        {
            if(*(t+i) == ' ')
            {
                centre += 5;
                i++;
                continue;
            }
            ac = 0;
            if(*(t+i) >= '0' && *(t+i) <= '9')
                ac = *(t+i) - '0' + BIGALPHANUM+26+26;
            if(*(t+i) >= 'a' && *(t+i) <= 'z')
                ac = *(t+i) - 'a' + BIGALPHANUM+26;
            if(*(t+i) >= 'A' && *(t+i) <= 'Z')
                ac = *(t+i) - 'A' + BIGALPHANUM;

            else switch(*t)
            {
                case '-':
                    ac = BIGALPHANUM-11;
                    break;
                case '.':
                    ac = BIGPERIOD;
                    break;
                case ',':
                    ac = BIGCOMMA;
                    break;
                case '!':
                    ac = BIGX;
                    break;
                case '?':
                    ac = BIGQ;
                    break;
                case ';':
                    ac = BIGSEMI;
                    break;
                case ':':
                    ac = BIGCOLIN;
                    break;
            }

            centre += tilesizx[ac]-1;
            i++;
        }
    }

    x -= centre>>1;

    while(*t)
    {
        if(*t == ' ') {x+=5;t++;continue;}
        ac = 0;
        if(*t >= '0' && *t <= '9')
            ac = *t - '0' + BIGALPHANUM+26+26;
        if(*t >= 'a' && *t <= 'z')
            ac = *t - 'a' + BIGALPHANUM+26;
        if(*t >= 'A' && *t <= 'Z')
            ac = *t - 'A' + BIGALPHANUM;
        switch(*t)
        {
            case '-':
                ac = BIGALPHANUM-11;
                break;
            case '.':
                ac = BIGPERIOD;
                break;
            case ',':
                ac = BIGCOMMA;
                break;
            case '!':
                ac = BIGX;
                break;
            case '?':
                ac = BIGQ;
                break;
            case ';':
                ac = BIGSEMI;
                break;
            case ':':
                ac = BIGCOLIN;
                break;
        }

        rotatesprite(x<<16,y<<16,65536L,0,ac,s,p,10+16,0,0,xdim-1,ydim-1);

        x += tilesizx[ac];
        t++;
    }
    return (x);
}


static void bar_(int type, int x,int y,short *p,short dainc,char damodify,short s, short pa)
{
    short xloc;
    char rev;

    if(dainc < 0) { dainc = -dainc; rev = 1; }
    else rev = 0;
    y-=2;

    if(damodify)
    {
        if(rev == 0)
        {
            if (uinfo.dir == dir_West)
            {
                *p -= dainc;
                if(*p < 0)
                    *p = 0;
                sound(KICK_HIT);
            }
            else if (uinfo.dir == dir_East)
            {
                *p += dainc;
                if(*p > 63)
                    *p = 63;
                sound(KICK_HIT);
            }
        }
        else
        {
            if (uinfo.dir == dir_West)
            {
                *p += dainc;
                if(*p > 64)
                    *p = 64;
                sound(KICK_HIT);
            }
            else if (uinfo.dir == dir_East)
            {
                *p -= dainc;
                if(*p < 0)
                    *p = 0;
                sound(KICK_HIT);
            }
        }
    }

    xloc = *p;

    rotatesprite( (x<<16)+(22<<(16-type)),(y<<16)-(3<<(16-type)),65536L>>type,0,SLIDEBAR,s,pa,10,0,0,xdim-1,ydim-1);
    if(rev == 0)
        rotatesprite( (x<<16)+((xloc+1)<<(16-type)),(y<<16)+(1<<(16-type)),65536L>>type,0,SLIDEBAR+1,s,pa,10,0,0,xdim-1,ydim-1);
    else
        rotatesprite( (x<<16)+((65-xloc)<<(16-type)),(y<<16)+(1<<(16-type)),65536L>>type,0,SLIDEBAR+1,s,pa,10,0,0,xdim-1,ydim-1);
}

void bar(int x,int y,short *p,short dainc,char damodify,short s, short pa) { bar_(0,x,y,p,dainc,damodify,s,pa); }
void barsm(int x,int y,short *p,short dainc,char damodify,short s, short pa) { bar_(1,x,y,p,dainc,damodify,s,pa); }

#define SHX(X) 0
// ((x==X)*(-sh))
#define PHX(X) 0
// ((x==X)?1:2)
#define MWIN(X) rotatesprite( 320<<15,200<<15,X,0,MENUSCREEN,-16,0,10+64,0,0,xdim-1,ydim-1)
#define MWINXY(X,OX,OY) rotatesprite( ( 320+(OX) )<<15, ( 200+(OY) )<<15,X,0,MENUSCREEN,-16,0,10+64,0,0,xdim-1,ydim-1)


static struct savehead savehead;
//static int32 volnum,levnum,plrskl,numplr;
//static char brdfn[BMAX_PATH];
short lastsavedpos = -1;

void dispnames(void)
{
    short x, c = 160;

    c += 64;
    for(x = 0;x <= 108;x += 12)
    rotatesprite((c+91-64)<<16,(x+56)<<16,65536L,0,TEXTBOX,24,0,10,0,0,xdim-1,ydim-1);

    rotatesprite(22<<16,97<<16,65536L,0,WINDOWBORDER2,24,0,10,0,0,xdim-1,ydim-1);
    rotatesprite(180<<16,97<<16,65536L,1024,WINDOWBORDER2,24,0,10,0,0,xdim-1,ydim-1);
    rotatesprite(99<<16,50<<16,65536L,512,WINDOWBORDER1,24,0,10,0,0,xdim-1,ydim-1);
    rotatesprite(103<<16,144<<16,65536L,1024+512,WINDOWBORDER1,24,0,10,0,0,xdim-1,ydim-1);

    minitext(c,48,ud.savegame[0],2,10+16);
    minitext(c,48+12,ud.savegame[1],2,10+16);
    minitext(c,48+12+12,ud.savegame[2],2,10+16);
    minitext(c,48+12+12+12,ud.savegame[3],2,10+16);
    minitext(c,48+12+12+12+12,ud.savegame[4],2,10+16);
    minitext(c,48+12+12+12+12+12,ud.savegame[5],2,10+16);
    minitext(c,48+12+12+12+12+12+12,ud.savegame[6],2,10+16);
    minitext(c,48+12+12+12+12+12+12+12,ud.savegame[7],2,10+16);
    minitext(c,48+12+12+12+12+12+12+12+12,ud.savegame[8],2,10+16);
    minitext(c,48+12+12+12+12+12+12+12+12+12,ud.savegame[9],2,10+16);

}

void clearfilenames(void)
{
    klistfree(finddirs);
    klistfree(findfiles);
    finddirs = findfiles = NULL;
    numfiles = numdirs = 0;
}

int getfilenames(char *path, char kind[])
{
    CACHE1D_FIND_REC *r;
    
    clearfilenames();
    finddirs = klistpath(path,"*",CACHE1D_FIND_DIR);
    findfiles = klistpath(path,kind,CACHE1D_FIND_FILE);
    for (r = finddirs; r; r=r->next) numdirs++;
    for (r = findfiles; r; r=r->next) numfiles++;
    
    finddirshigh = finddirs;
    findfileshigh = findfiles;
    currentlist = 0;
    if (findfileshigh) currentlist = 1;
    
    return(0);
}

int quittimer = 0;

void menus(void)
{
    CACHE1D_FIND_REC *dir;
    short c,x,i,s;
    int l,m;
    char *p = NULL;

    getpackets();

    if( (ps[myconnectindex].gm&MODE_MENU) == 0 )
    {
        walock[TILE_LOADSHOT] = 1;
        return;
    }

    CONTROL_GetUserInput(&uinfo);
    CONTROL_ClearUserInput(&uinfo);

    ps[myconnectindex].gm &= (0xff-MODE_TYPE);
    ps[myconnectindex].fta = 0;

    x = 0;

    sh = 4-(sintable[(totalclock<<4)&2047]>>11);

    if(!(current_menu >= 1000 && current_menu <= 2999 && current_menu >= 300 && current_menu <= 369))
        vscrn();

    switch(current_menu)
    {
        case 25000:
            gametext(160,90,"SELECT A SAVE SPOT BEFORE",0,2+8+16);
            gametext(160,90+9,"YOU QUICK RESTORE.",0,2+8+16);

            x = probe(186,124,0,0);
            if(x >= -1)
            {
                if(ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
                ps[myconnectindex].gm &= ~MODE_MENU;
            }
            break;

        case 20000:
            x = probe(326,190,0,0);
            gametext(160,50-8,"YOU ARE PLAYING THE SHAREWARE",0,2+8+16);
            gametext(160,59-8,"VERSION OF DUKE NUKEM 3D.  WHILE",0,2+8+16);
            gametext(160,68-8,"THIS VERSION IS REALLY COOL, YOU",0,2+8+16);
            gametext(160,77-8,"ARE MISSING OVER 75%% OF THE TOTAL",0,2+8+16);
            gametext(160,86-8,"GAME, ALONG WITH OTHER GREAT EXTRAS",0,2+8+16);
            gametext(160,95-8,"AND GAMES, WHICH YOU'LL GET WHEN",0,2+8+16);
            gametext(160,104-8,"YOU ORDER THE COMPLETE VERSION AND",0,2+8+16);
            gametext(160,113-8,"GET THE FINAL TWO EPISODES.",0,2+8+16);

            gametext(160,113+8,"PLEASE READ THE 'HOW TO ORDER' ITEM",0,2+8+16);
            gametext(160,122+8,"ON THE MAIN MENU IF YOU WISH TO",0,2+8+16);
            gametext(160,131+8,"UPGRADE TO THE FULL REGISTERED",0,2+8+16);
            gametext(160,140+8,"VERSION OF DUKE NUKEM 3D.",0,2+8+16);
            gametext(160,149+16,"PRESS ANY KEY...",0,2+8+16);

            if( x >= -1 ) cmenu(100);
            break;

        case 15001:
        case 15000:

            gametext(160,90,"LOAD last game:",0,2+8+16);

            sprintf(buf,"\"%s\"",ud.savegame[lastsavedpos]);
            gametext(160,99,buf,0,2+8+16);

            gametext(160,99+9,"(Y/N)",0,2+8+16);

            if (uinfo.button1 || KB_KeyPressed(sc_N))
            {
                KB_ClearKeyDown(sc_N);

                if(sprite[ps[myconnectindex].i].extra <= 0)
                {
                    if (enterlevel(MODE_GAME)) backtomenu();
                    return;
                }

                ps[myconnectindex].gm &= ~MODE_MENU;
                if(ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
            }

            if (uinfo.button0 || KB_KeyPressed(sc_Y))
            {
                KB_FlushKeyboardQueue();
                KB_ClearKeysDown();
                FX_StopAllSounds();

                if(ud.multimode > 1)
                {
                    loadplayer(-1-lastsavedpos);
                    ps[myconnectindex].gm = MODE_GAME;
                }
                else
                {
                    c = loadplayer(lastsavedpos);
                    if(c == 0)
                        ps[myconnectindex].gm = MODE_GAME;
                }
            }

            probe(186,124+9,0,0);

            break;

        case 10000:
        case 10001:
            if (NAM) { cmenu(0); break; }

            c = 60;
            rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,"ADULT MODE");

            x = probe(60,50+16,16,2);
            if(x == -1) { cmenu(200); probey = 4; break; }

            menutext(c,50+16,SHX(-2),PHX(-2),"ADULT MODE");
            menutext(c,50+16+16,SHX(-3),PHX(-3),"ENTER PASSWORD");

            if(ud.lockout) menutext(c+160+40,50+16,0,0,"OFF");
            else menutext(c+160+40,50+16,0,0,"ON");

            if(current_menu == 10001)
            {
                gametext(160,50+16+16+16+16-12,"ENTER PASSWORD",0,2+8+16);
                x = strget((320>>1),50+16+16+16+16,buf,19, STRGET_PASSWORD);

                if( x )
                {
                    if(ud.pwlockout[0] == 0 || ud.lockout == 0 )
                        strcpy(&ud.pwlockout[0],buf);
                    else if( strcmp(buf,&ud.pwlockout[0]) == 0 )
                    {
                        ud.lockout = 0;
                        buf[0] = 0;

                        for(x=0;x<numanimwalls;x++)
                            if( wall[animwall[x].wallnum].picnum != W_SCREENBREAK &&
                                wall[animwall[x].wallnum].picnum != W_SCREENBREAK+1 &&
                                wall[animwall[x].wallnum].picnum != W_SCREENBREAK+2 )
                                    if( wall[animwall[x].wallnum].extra >= 0 )
                                        wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;

                    }
                    current_menu = 10000;
                    KB_ClearKeyDown(sc_Enter);
                    KB_ClearKeyDown(sc_kpad_Enter);
                    KB_FlushKeyboardQueue();
                }
            }
            else
            {
                if(x == 0)
                {
                    if( ud.lockout == 1 )
                    {
                        if(ud.pwlockout[0] == 0)
                        {
                            ud.lockout = 0;
                            for(x=0;x<numanimwalls;x++)
                            if( wall[animwall[x].wallnum].picnum != W_SCREENBREAK &&
                                wall[animwall[x].wallnum].picnum != W_SCREENBREAK+1 &&
                                wall[animwall[x].wallnum].picnum != W_SCREENBREAK+2 )
                                    if( wall[animwall[x].wallnum].extra >= 0 )
                                        wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
                        }
                        else
                        {
                            buf[0] = 0;
                            current_menu = 10001;
                            inputloc = 0;
                            KB_FlushKeyboardQueue();
                        }
                    }
                    else
                    {
                        ud.lockout = 1;

                        for(x=0;x<numanimwalls;x++)
                            switch(wall[animwall[x].wallnum].picnum)
                            {
                                case FEMPIC1:
                                    wall[animwall[x].wallnum].picnum = BLANKSCREEN;
                                    break;
                                case FEMPIC2:
                                case FEMPIC3:
                                    wall[animwall[x].wallnum].picnum = SCREENBREAK6;
                                    break;
                            }
                    }
                }

                else if(x == 1)
                {
                    current_menu = 10001;
                    inputloc = 0;
                    KB_FlushKeyboardQueue();
                }
            }

            break;

        case 1000:
        case 1001:
        case 1002:
        case 1003:
        case 1004:
        case 1005:
        case 1006:
        case 1007:
        case 1008:
        case 1009:

            rotatesprite(160<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64,0,0,xdim-1,ydim-1);
            rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,"LOAD GAME");
            rotatesprite(101<<16,97<<16,65536>>1,512,TILE_LOADSHOT,-32,0,4+10+64,0,0,xdim-1,ydim-1);

            dispnames();

            sprintf(buf,"PLAYERS: %-2d                      ",savehead.numplr);
            gametext(160,156,buf,0,2+8+16);

            sprintf(buf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+savehead.volnum,1+savehead.levnum,savehead.plrskl);
            gametext(160,168,buf,0,2+8+16);

            if (savehead.volnum == 0 && savehead.levnum == 7)
                gametext(160,180,savehead.boardfn,0,2+8+16);
            
            gametext(160,90,"LOAD game:",0,2+8+16);
            sprintf(buf,"\"%s\"",ud.savegame[current_menu-1000]);
            gametext(160,99,buf,0,2+8+16);
            gametext(160,99+9,"(Y/N)",0,2+8+16);

            if (uinfo.button0 || KB_KeyPressed(sc_Y))
            {
                lastsavedpos = current_menu-1000;

                KB_FlushKeyboardQueue();
                KB_ClearKeysDown();
                if(ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }

                if(ud.multimode > 1)
                {
                    if( ps[myconnectindex].gm&MODE_GAME )
                    {
                        loadplayer(-1-lastsavedpos);
                        ps[myconnectindex].gm = MODE_GAME;
                    }
                    else
                    {
                        tempbuf[0] = 126;
                        tempbuf[1] = lastsavedpos;
                        tempbuf[2] = myconnectindex;
                        for(x=connecthead;x>=0;x=connectpoint2[x])
                        {
                             if (x != myconnectindex) sendpacket(x,tempbuf,3);
                             if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
                        }
                        getpackets();

                        loadplayer(lastsavedpos);

                        multiflag = 0;
                    }
                }
                else
                {
                    c = loadplayer(lastsavedpos);
                    if(c == 0)
                        ps[myconnectindex].gm = MODE_GAME;
                }

                break;
            }
            if (uinfo.button1 || KB_KeyPressed(sc_N))
            {
                KB_ClearKeyDown(sc_N);
                cmenu(300);
                break;
            }

            probe(186,124+9,0,0);

            break;

        case 1500:

            if (uinfo.button0 || KB_KeyPressed(sc_Y))
            {
                KB_FlushKeyboardQueue();
                cmenu(100);
            }
            if (uinfo.button1 || KB_KeyPressed(sc_N))
            {
                KB_ClearKeyDown(sc_N);
                if(ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
                ps[myconnectindex].gm &= ~MODE_MENU;
                sound(EXITMENUSOUND);
                break;
            }
            probe(186,124,0,0);
            gametext(160,90,"ABORT this game?",0,2+8+16);
            gametext(160,90+9,"(Y/N)",0,2+8+16);

            break;

        case 2000:
        case 2001:
        case 2002:
        case 2003:
        case 2004:
        case 2005:
        case 2006:
        case 2007:
        case 2008:
        case 2009:

            rotatesprite(160<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64,0,0,xdim-1,ydim-1);
            rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,"SAVE GAME");

            rotatesprite(101<<16,97<<16,65536L>>1,512,TILE_LOADSHOT,-32,0,4+10+64,0,0,xdim-1,ydim-1);
            sprintf(buf,"PLAYERS: %-2d                      ",ud.multimode);
            gametext(160,156,buf,0,2+8+16);

            sprintf(buf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill);
            gametext(160,168,buf,0,2+8+16);

            if (ud.volume_number == 0 && ud.level_number == 7)
                gametext(160,180,boardfilename,0,2+8+16);

            dispnames();

            gametext(160,90,"OVERWRITE previous SAVED game?",0,2+8+16);
            gametext(160,90+9,"(Y/N)",0,2+8+16);

            if(uinfo.button0 || KB_KeyPressed(sc_Y))
            {
                inputloc = strlen(&ud.savegame[current_menu-2000][0]);

                cmenu(current_menu-2000+360);

                KB_FlushKeyboardQueue();
                break;
            }
            if(uinfo.button1 || KB_KeyPressed(sc_N))
            {
                KB_ClearKeyDown(sc_N);
                cmenu(351);
                sound(EXITMENUSOUND);
                break;
            }

            probe(186,124,0,0);

            break;

        case 990:
        case 991:
        case 992:
        case 993:
        case 994:
        case 995:
        case 996:
        case 997:
        case 998:
            c = 160;
            if (!VOLUMEALL || !PLUTOPAK) {
                //rotatesprite(c<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64,0,0,xdim-1,ydim-1);
                rotatesprite(c<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
                menutext(c,24,0,0,current_menu == 998 ? "PORT CREDITS" : "CREDITS");

                l = 8;
            } else {
                l = 3;
            }

            probe(0,0,0,0);

            if(uinfo.button1)
            {
                cmenu(0);
                break;
            }

            if(uinfo.dir == dir_West || uinfo.dir == dir_North || KB_KeyPressed( sc_PgUp ))
            {
                KB_ClearKeyDown(sc_PgUp);

                sound(KICK_HIT);
                current_menu--;
                if(current_menu < 990) current_menu = 990+l;
            }
            else if(uinfo.dir == dir_East || uinfo.dir == dir_South || KB_KeyPressed( sc_PgDn ))
            {
                KB_ClearKeyDown(sc_PgDn);

                sound(KICK_HIT);
                current_menu++;
                if(current_menu > 990+l) current_menu = 990;
            }

            if (!VOLUMEALL || !PLUTOPAK) {
            switch (current_menu) {
                case 990:
                    gametext(c,40,                      "ORIGINAL CONCEPT",0,2+8+16);
                    gametext(c,40+9,                    "TODD REPLOGLE",0,2+8+16);
                    gametext(c,40+9+9,                  "ALLEN H. BLUM III",0,2+8+16);
                        
                    gametext(c,40+9+9+9+9,              "PRODUCED & DIRECTED BY",0,2+8+16);
                    gametext(c,40+9+9+9+9+9,            "GREG MALONE",0,2+8+16);

                    gametext(c,40+9+9+9+9+9+9+9,        "EXECUTIVE PRODUCER",0,2+8+16);
                    gametext(c,40+9+9+9+9+9+9+9+9,      "GEORGE BROUSSARD",0,2+8+16);

                    gametext(c,40+9+9+9+9+9+9+9+9+9+9,  "BUILD ENGINE",0,2+8+16);
                    gametext(c,40+9+9+9+9+9+9+9+9+9+9+9,"KEN SILVERMAN",0,2+8+16);
                    break;
                case 991:
                    gametext(c,40,                      "GAME PROGRAMMING",0,2+8+16);
                    gametext(c,40+9,                    "TODD REPLOGLE",0,2+8+16);
                        
                    gametext(c,40+9+9+9,                "3D ENGINE/TOOLS/NET",0,2+8+16);
                    gametext(c,40+9+9+9+9,              "KEN SILVERMAN",0,2+8+16);

                    gametext(c,40+9+9+9+9+9+9,          "NETWORK LAYER/SETUP PROGRAM",0,2+8+16);
                    gametext(c,40+9+9+9+9+9+9+9,        "MARK DOCHTERMANN",0,2+8+16);
                    break;
                case 992:
                    gametext(c,40,                      "MAP DESIGN",0,2+8+16);
                    gametext(c,40+9,                    "ALLEN H BLUM III",0,2+8+16);
                    gametext(c,40+9+9,                  "RICHARD GRAY",0,2+8+16);
                        
                    gametext(c,40+9+9+9+9,              "3D MODELING",0,2+8+16);
                    gametext(c,40+9+9+9+9+9,            "CHUCK JONES",0,2+8+16);
                    gametext(c,40+9+9+9+9+9+9,          "SAPPHIRE CORPORATION",0,2+8+16);

                    gametext(c,40+9+9+9+9+9+9+9+9,      "ARTWORK",0,2+8+16);
                    gametext(c,40+9+9+9+9+9+9+9+9+9,    "DIRK JONES, STEPHEN HORNBACK",0,2+8+16);
                    gametext(c,40+9+9+9+9+9+9+9+9+9+9,  "JAMES STOREY, DAVID DEMARET",0,2+8+16);
                    gametext(c,40+9+9+9+9+9+9+9+9+9+9+9,"DOUGLAS R WOOD",0,2+8+16);
                    break;
                case 993:
                    gametext(c,40,                      "SOUND ENGINE",0,2+8+16);
                    gametext(c,40+9,                    "JIM DOSE",0,2+8+16);
                        
                    gametext(c,40+9+9+9,                "SOUND & MUSIC DEVELOPMENT",0,2+8+16);
                    gametext(c,40+9+9+9+9,              "ROBERT PRINCE",0,2+8+16);
                    gametext(c,40+9+9+9+9+9,            "LEE JACKSON",0,2+8+16);

                    gametext(c,40+9+9+9+9+9+9+9,        "VOICE TALENT",0,2+8+16);
                    gametext(c,40+9+9+9+9+9+9+9+9,      "LANI MINELLA - VOICE PRODUCER",0,2+8+16);
                    gametext(c,40+9+9+9+9+9+9+9+9+9,    "JON ST. JOHN AS \"DUKE NUKEM\"",0,2+8+16);
                    break;
                case 994:
                    gametext(c,60,                      "GRAPHIC DESIGN",0,2+8+16);
                    gametext(c,60+9,                    "PACKAGING, MANUAL, ADS",0,2+8+16);
                    gametext(c,60+9+9,                  "ROBERT M. ATKINS",0,2+8+16);
                    gametext(c,60+9+9+9,                "MICHAEL HADWIN",0,2+8+16);
                    
                    gametext(c,60+9+9+9+9+9,            "SPECIAL THANKS TO",0,2+8+16);
                    gametext(c,60+9+9+9+9+9+9,          "STEVEN BLACKBURN, TOM HALL",0,2+8+16);
                    gametext(c,60+9+9+9+9+9+9+9,        "SCOTT MILLER, JOE SIEGLER",0,2+8+16);
                    gametext(c,60+9+9+9+9+9+9+9+9,      "TERRY NAGY, COLLEEN COMPTON",0,2+8+16);
                    gametext(c,60+9+9+9+9+9+9+9+9+9,    "HASH INC., FORMGEN, INC.",0,2+8+16);
                    break;
                case 995:
                    gametext(c,49,                      "THE 3D REALMS BETA TESTERS",0,2+8+16);

                    gametext(c,49+9+9,                  "NATHAN ANDERSON, WAYNE BENNER",0,2+8+16);
                    gametext(c,49+9+9+9,                "GLENN BRENSINGER, ROB BROWN",0,2+8+16);
                    gametext(c,49+9+9+9+9,              "ERIK HARRIS, KEN HECKBERT",0,2+8+16);
                    gametext(c,49+9+9+9+9+9,            "TERRY HERRIN, GREG HIVELY",0,2+8+16);
                    gametext(c,49+9+9+9+9+9+9,          "HANK LEUKART, ERIC BAKER",0,2+8+16);
                    gametext(c,49+9+9+9+9+9+9+9,        "JEFF RAUSCH, KELLY ROGERS",0,2+8+16);
                    gametext(c,49+9+9+9+9+9+9+9+9,      "MIKE DUNCAN, DOUG HOWELL",0,2+8+16);
                    gametext(c,49+9+9+9+9+9+9+9+9+9,    "BILL BLAIR",0,2+8+16);
                    break;
                case 996:
                    gametext(c,32,                      "COMPANY PRODUCT SUPPORT",0,2+8+16);

                    gametext(c,32+9+9,                  "THE FOLLOWING COMPANIES WERE COOL",0,2+8+16);
                    gametext(c,32+9+9+9,                "ENOUGH TO GIVE US LOTS OF STUFF",0,2+8+16);
                    gametext(c,32+9+9+9+9,              "DURING THE MAKING OF DUKE NUKEM 3D.",0,2+8+16);

                    gametext(c,32+9+9+9+9+9+9,          "ALTEC LANSING MULTIMEDIA",0,2+8+16);
                    gametext(c,32+9+9+9+9+9+9+9,        "FOR TONS OF SPEAKERS AND THE",0,2+8+16);
                    gametext(c,32+9+9+9+9+9+9+9+9,      "THX-LICENSED SOUND SYSTEM",0,2+8+16);
                    gametext(c,32+9+9+9+9+9+9+9+9+9,    "FOR INFO CALL 1-800-548-0620",0,2+8+16);
                    
                    gametext(c,32+9+9+9+9+9+9+9+9+9+9+9,"CREATIVE LABS, INC.",0,2+8+16);

                    gametext(c,32+9+9+9+9+9+9+9+9+9+9+9+9+9,"THANKS FOR THE HARDWARE, GUYS.",0,2+8+16);
                    break;
                case 997:
                    gametext(c,50,                      "DUKE NUKEM IS A TRADEMARK OF",0,2+8+16);
                    gametext(c,50+9,                    "3D REALMS ENTERTAINMENT",0,2+8+16);
                        
                    gametext(c,50+9+9+9,                "DUKE NUKEM",0,2+8+16);
                    gametext(c,50+9+9+9+9,              "(C) 1996 3D REALMS ENTERTAINMENT",0,2+8+16);

                    if (VOLUMEONE) {
                    gametext(c,106,                     "PLEASE READ LICENSE.DOC FOR SHAREWARE",0,2+8+16);
                    gametext(c,106+9,                   "DISTRIBUTION GRANTS AND RESTRICTIONS",0,2+8+16);
                    }

                    gametext(c,VOLUMEONE?134:115,       "MADE IN DALLAS, TEXAS USA",0,2+8+16);
                    break;
                case 998:
                    l = 10;
                    goto cheat_for_port_credits;
            }
            break;
            }

            // Plutonium pak menus
            switch(current_menu)
            {
                case 990:
                case 991:
                case 992:
                   rotatesprite(160<<16,200<<15,65536L,0,2504+current_menu-990,0,0,10+64,0,0,xdim-1,ydim-1);
                   break;
                case 993:   // JBF 20031220
                   rotatesprite(160<<16,200<<15,65536L,0,MENUSCREEN,0,0,10+64,0,0,xdim-1,ydim-1);
                   menutext(160,28,0,0,"PORT CREDITS");

                   l = 0;
cheat_for_port_credits:
                   gametext(160,40-l,"GAME AND ENGINE PORT",0,2+8+16);
                   p = "Jonathon \"JonoF\" Fowler";
                   minitext(160-(Bstrlen(p)<<1), 40+10-l, p, 12, 10+16+128);
                   
                   gametext(160,60-l,"\"POLYMOST\" 3D RENDERER",0,2+8+16);
                   gametext(160,60+8-l,"NETWORKING, OTHER CODE",0,2+8+16);
                   p = "Ken \"Awesoken\" Silverman";
                   minitext(160-(Bstrlen(p)<<1), 60+8+10-l, p, 12, 10+16+128);

                   p = "Icon and startup graphics by Lachlan \"NetNessie\" McDonald";
                   minitext(160-(Bstrlen(p)<<1), 92-l, p, 12, 10+16+128);

                   {
                        const char *scroller[] = {
                            "This program is free software; you can redistribute it",
                            "and/or modify it under the terms of the GNU General",
                            "Public License as published by the Free Software",
                            "Foundation; either version 2 of the License, or (at your",
                            "option) any later version.",
                            "",
                            "This program is distributed in the hope that it will be",
                            "useful but WITHOUT ANY WARRANTY; without even the implied",
                            "warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR",
                            "PURPOSE. See the GNU General Public License (GPL.TXT) for",
                            "more details.",
                            "",
                            "",
                            "",
                            "",
                            "Thanks to these people for their input and contributions:",
                            "",
                            "Michael Burgwin",
                            "Richard \"TerminX\" Gobeille, ",
                            "Par \"Parkar\" Karlsson", // "Pär \"Parkar\" Karlsson",
                            "Matthieu Klein",
                            "Ben \"ProAsm\" Smit,",
                            "",
                            "and all those who submitted bug reports and ",
                            "supported the project financially!",
                            "",
                            "",
                            "--x--",
                            "",
                            "",
                            "",
                            ""
                        };
                        const int numlines = sizeof(scroller)/sizeof(char *);
                        for (m=0,i=(totalclock/104)%numlines; m<4; m++,i++) {
                            if (i==numlines) i=0;
                            minitext(160-(Bstrlen(scroller[i])<<1), 100+10+(m*7)-l, scroller[i], 8, 10+16+128);
                        }
                   }

                   for (i=0;i<2;i++) {
                       switch (i) {
                           case 0: p = "Visit http://www.jonof.id.au/jfduke3d for"; break;
                           case 1: p = "the source code, latest news, and updates of this port."; break;
                       }
                       minitext(160-(Bstrlen(p)<<1), 135+10+(i*7)-l, p, 8, 10+16+128);
                   }
                   break;
            }
            break;

        case 0:
            c = (320>>1);
            rotatesprite(c<<16,28<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10,0,0,xdim-1,ydim-1);
            if (PLUTOPAK)   // JBF 20030804
                rotatesprite((c+100)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,(sintable[(totalclock<<4)&2047]>>11),0,2+8,0,0,xdim-1,ydim-1);
            x = probe(c,67,16,6);
            if(x >= 0)
            {
                if( ud.multimode > 1 && x == 0 && ud.recstat != 2)
                {
                    if( movesperpacket == 4 && myconnectindex != connecthead )
                        break;

                    last_zero = 0;
                    cmenu( 600 );
                }
                else
                {
                    last_zero = x;
                    switch(x)
                    {
                        case 0:
                            cmenu(100);
                            break;

                        case 1: cmenu(200);break;
                        case 2:
                            if(movesperpacket == 4 && connecthead != myconnectindex)
                                break;
                            cmenu(300);
                            break;
                        case 3: KB_FlushKeyboardQueue();cmenu(400);break;
                        case 4: cmenu(990);break;
                        case 5: cmenu(500);break;
                    }
                }
            }

            if(KB_KeyPressed(sc_Q)) cmenu(500);

            if(x == -1)
            {
                ps[myconnectindex].gm &= ~MODE_MENU;
                if(ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
            }

            if(movesperpacket == 4)
            {
                if( myconnectindex == connecthead )
                    menutext(c,67,SHX(-2),PHX(-2),"NEW GAME");
                else
                    menutext(c,67,SHX(-2),1,"NEW GAME");
            }
            else
                menutext(c,67,SHX(-2),PHX(-2),"NEW GAME");

            menutext(c,67+16,SHX(-3),PHX(-3),"OPTIONS");

            if(movesperpacket == 4 && connecthead != myconnectindex)
                menutext(c,67+16+16,SHX(-4),1,"LOAD GAME");
            else menutext(c,67+16+16,SHX(-4),PHX(-4),"LOAD GAME");

            menutext(c,67+16+16+16,SHX(-5),PHX(-5), VOLUMEALL ? "HELP" : "HOW TO ORDER");

            menutext(c,67+16+16+16+16,SHX(-6),PHX(-6),"CREDITS");

            menutext(c,67+16+16+16+16+16,SHX(-7),PHX(-7),"QUIT");
            break;

        case 50:
            c = (320>>1);
            rotatesprite(c<<16,32<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10,0,0,xdim-1,ydim-1);
            if (PLUTOPAK)
                rotatesprite((c+100)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,(sintable[(totalclock<<4)&2047]>>11),0,2+8,0,0,xdim-1,ydim-1);
            x = probe(c,67,16,7);
            switch(x)
            {
                case 0:
                    if(movesperpacket == 4 && myconnectindex != connecthead)
                        break;
                    if(ud.multimode < 2 || ud.recstat == 2)
                        cmenu(1500);
                    else
                    {
                        cmenu(600);
                        last_fifty = 0;
                    }
                    break;
                case 1:
                    if(movesperpacket == 4 && connecthead != myconnectindex)
                        break;
                    if(ud.recstat != 2)
                    {
                        last_fifty = 1;
                        cmenu(350);
                        setview(0,0,xdim-1,ydim-1);
                    }
                    break;
                case 2:
                    if(movesperpacket == 4 && connecthead != myconnectindex)
                        break;
                    last_fifty = 2;
                    cmenu(300);
                    break;
                case 3:
                    last_fifty = 3;
                    cmenu(200);
                    break;
                case 4:
                    last_fifty = 4;
                    KB_FlushKeyboardQueue();
                    cmenu(400);
                    break;
                case 5:
                    if(numplayers < 2)
                    {
                        last_fifty = 5;
                        cmenu(501);
                    }
                    break;
                case 6:
                    last_fifty = 6;
                    cmenu(500);
                    break;
                case -1:
                    ps[myconnectindex].gm &= ~MODE_MENU;
                    if(ud.multimode < 2 && ud.recstat != 2)
                    {
                        ready2send = 1;
                        totalclock = ototalclock;
                    }
                    break;
            }

            if( KB_KeyPressed(sc_Q) )
                cmenu(500);

            if(movesperpacket == 4 && connecthead != myconnectindex)
            {
                menutext(c,67                  ,SHX(-2),1,"NEW GAME");
                menutext(c,67+16               ,SHX(-3),1,"SAVE GAME");
                menutext(c,67+16+16            ,SHX(-4),1,"LOAD GAME");
            }
            else
            {
                menutext(c,67                  ,SHX(-2),PHX(-2),"NEW GAME");
                menutext(c,67+16               ,SHX(-3),PHX(-3),"SAVE GAME");
                menutext(c,67+16+16            ,SHX(-4),PHX(-4),"LOAD GAME");
            }

            menutext(c,67+16+16+16         ,SHX(-5),PHX(-5),"OPTIONS");
if (!VOLUMEALL) {
            menutext(c,67+16+16+16+16      ,SHX(-6),PHX(-6),"HOW TO ORDER");
} else {
            menutext(c,67+16+16+16+16      ,SHX(-6),PHX(-6)," HELP");
}
            if(numplayers > 1)
                menutext(c,67+16+16+16+16+16   ,SHX(-7),1,"QUIT TO TITLE");
            else menutext(c,67+16+16+16+16+16   ,SHX(-7),PHX(-7),"QUIT TO TITLE");
            menutext(c,67+16+16+16+16+16+16,SHX(-8),PHX(-8),"QUIT GAME");
            break;

        case 100:
            rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,"SELECT AN EPISODE");
//            if(boardfilename[0])
if (PLUTOPAK)
                x = probe(160,60,20,5);
//            else x = probe(160,60,20,4);
//            if(boardfilename[0])
else
                x = probe(160,60,20,VOLUMEONE?3:4);
//            else x = probe(160,60,20,3);
            if(x >= 0)
            {
if (VOLUMEONE) {
                if(x > 0)
                    cmenu(20000);
                else
                {
                    ud.m_volume_number = x;
                    ud.m_level_number = 0;
                    cmenu(110);
                }
}

if (!VOLUMEONE) {
                if(!PLUTOPAK && x == 3 /*&& boardfilename[0]*/)
                {
                    //ud.m_volume_number = 0;
                    //ud.m_level_number = 7;
            currentlist = 1;
            cmenu(101);
                }
        else if(PLUTOPAK && x == 4 /*&& boardfilename[0]*/)
                {
                    //ud.m_volume_number = 0;
                    //ud.m_level_number = 7;
            currentlist = 1;
            cmenu(101);
                }

                else
                {
                    ud.m_volume_number = x;
                    ud.m_level_number = 0;
                    cmenu(110);
                }
}
            }
            else if(x == -1)
            {
                if(ps[myconnectindex].gm&MODE_GAME) cmenu(50);
                else cmenu(0);
            }

            menutext(160,60,SHX(-2),PHX(-2),volume_names[0]);

            c = 80;
if (VOLUMEONE) {
            menutext(160,60+20,SHX(-3),1,volume_names[1]);
            menutext(160,60+20+20,SHX(-4),1,volume_names[2]);
if (PLUTOPAK)
            menutext(160,60+20+20,SHX(-5),1,volume_names[3]);
} else {
            menutext(160,60+20,SHX(-3),PHX(-3),volume_names[1]);
            menutext(160,60+20+20,SHX(-4),PHX(-4),volume_names[2]);
if (PLUTOPAK) {
        menutext(160,60+20+20+20,SHX(-5),PHX(-5),volume_names[3]);
//            if(boardfilename[0])
//            {
                menutext(160,60+20+20+20+20,SHX(-6),PHX(-6),"USER MAP");
//                gametextpal(160,60+20+20+20+20+3,boardfilename,16+(sintable[(totalclock<<4)&2047]>>11),2);
//            }
} else {
//            if(boardfilename[0])
//            {
                menutext(160,60+20+20+20,SHX(-6),PHX(-6),"USER MAP");
//                gametext(160,60+20+20+20+6,boardfilename,2,2+8+16);
//            }
}

}
            break;

    case 101:
        if (boardfilename[0] == 0) strcpy(boardfilename, "./");
        Bcorrectfilename(boardfilename,1);
        getfilenames(boardfilename,"*.map");
        cmenu(102);
        KB_FlushKeyboardQueue();
        // fall through
    case 102:
            rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,"SELECT A USER MAP");

        // black translucent background underneath file lists
        rotatesprite(0<<16, 0<<16, 65536l<<5, 0, BLANK, 0, 0, 10+16+1+32,
                scale(40-4,xdim,320),scale(12+32-2,ydim,200),
                scale(320-40+4,xdim,320)-1,scale(12+32+112+4,ydim,200)-1);

        // path
        minitext(52,32,boardfilename,0,26);

        {   // JBF 20040208: seek to first name matching pressed character
            CACHE1D_FIND_REC *seeker = currentlist ? findfiles : finddirs;
            char ch2, ch;
            ch = KB_Getch();
            if (ch > 0 && ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'))) {
                if (ch >= 'a') ch -= ('a'-'A');
                while (seeker) {
                    ch2 = seeker->name[0];
                    if (ch2 >= 'a' && ch2 <= 'z') ch2 -= ('a'-'A');
                    if (ch2 == ch) break;
                    seeker = seeker->next;
                }
                if (seeker) {
                    if (currentlist) findfileshigh = seeker;
                    else finddirshigh = seeker;
                    sound(KICK_HIT);
                }
            }
        }
        
        gametext(40+4,12+32,"DIRECTORIES",0,2+8+16);
        
        if (finddirshigh) {
            dir = finddirshigh;
            for(i=0; i<2; i++) if (!dir->prev) break; else dir=dir->prev;
            for(i=2; i>-2 && dir; i--, dir=dir->next) {
                if (dir == finddirshigh) c=0; else c=16;
                minitextshade(40,1+12+32+8*(3-i),dir->name,c,0,26);
            }
        }

        gametext(40+4,8+32+40+8-1,"MAP FILES",0,2+8+16);

        if (findfileshigh) {
            dir = findfileshigh;
            for(i=0; i<4; i++) if (!dir->prev) break; else dir=dir->prev;
            for(i=4; i>-4 && dir; i--, dir=dir->next) {
                if (dir == findfileshigh) c=0; else c=16;
                minitextshade(40,(8+32+8*5)+8*(6-i),dir->name,c,2,26);
            }
        }

        if (uinfo.dir == dir_West || uinfo.dir == dir_East || KB_KeyPressed(sc_Tab))
        {
            KB_ClearKeyDown( sc_Tab );
            currentlist = 1-currentlist;
            sound(KICK_HIT);
        }
        
        onbar = 0;
        probey = 2;
        if (currentlist == 0) x = probe(50,12+32+16+4,0,3);
        else x = probe(50,8+32+40+40+4,0,3);

        if (probey == 1) {
            if (currentlist == 0) {
                if (finddirshigh)
                    if (finddirshigh->prev) finddirshigh = finddirshigh->prev;
            } else {
                if (findfileshigh)
                    if (findfileshigh->prev) findfileshigh = findfileshigh->prev;
            }
        } else if (probey == 0) {
            if (currentlist == 0) {
                if (finddirshigh)
                    if (finddirshigh->next) finddirshigh = finddirshigh->next;
            } else {
                if (findfileshigh)
                    if (findfileshigh->next) findfileshigh = findfileshigh->next;
            }
        }

        if(x == -1) {
            cmenu(100);
            clearfilenames();
            boardfilename[0] = 0;
        }
        else if(x >= 0)
        {
            if (currentlist == 0) {
                if (!finddirshigh) break;
                strcat(boardfilename, finddirshigh->name);
                strcat(boardfilename, "/");
                Bcorrectfilename(boardfilename, 1);
                cmenu(101);
                KB_FlushKeyboardQueue();
            } else {
                if (!findfileshigh) break;
                strcat(boardfilename, findfileshigh->name);
                ud.m_volume_number = 0;
                ud.m_level_number = 7;
                cmenu(110);
            }
            clearfilenames();
        }
        break;
        

        case 110:
            c = (320>>1);
            rotatesprite(c<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(c,24,0,0,"SELECT SKILL");
            x = probe(c,70,19,4);
            if(x >= 0)
            {
                switch(x)
                {
                    case 0: globalskillsound = JIBBED_ACTOR6;break;
                    case 1: globalskillsound = BONUS_SPEECH1;break;
                    case 2: globalskillsound = DUKE_GETWEAPON2;break;
                    case 3: globalskillsound = JIBBED_ACTOR5;break;
                }

                sound(globalskillsound);

                ud.m_player_skill = x+1;
                if(x == 3) ud.m_respawn_monsters = 1;
                else ud.m_respawn_monsters = 0;

                ud.m_monsters_off = ud.monsters_off = 0;

                ud.m_respawn_items = 0;
                ud.m_respawn_inventory = 0;

                ud.multimode = 1;

                if(ud.m_volume_number == 3)
                {
                    flushperms();
                    setview(0,0,xdim-1,ydim-1);
                    clearallviews(0L);
                    nextpage();
                }

                newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);
                if (enterlevel(MODE_GAME)) backtomenu();
            }
            else if(x == -1)
            {
                cmenu(100);
                KB_FlushKeyboardQueue();
            }

            menutext(c,70,SHX(-2),PHX(-2),skill_names[0]);
            menutext(c,70+19,SHX(-3),PHX(-3),skill_names[1]);
            menutext(c,70+19+19,SHX(-4),PHX(-4),skill_names[2]);
            menutext(c,70+19+19+19,SHX(-5),PHX(-5),skill_names[3]);
            break;

        case 201:   // Game options.
            rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,15,0,0,"GAME OPTIONS");

            c = (320>>1)-120;

        {
            int io, ii, yy, d=c+160+40, enabled;
            static const char *opts[] = {
                "Crosshair",
                "Level stats",
                "Status bar size",
                "-",
                "Mouse aiming",
                "Invert mouse aim",
                "Mouse aiming type",
                "Auto-aiming",
                "Run key style",
                "Auto weapon switch",
                "-",
                "Screen size",
                "Detail",
                "Shadows",
                "Screen tilting",
                "-",
                "Record demo",
                "Show startup window",
                NULL
            };

            yy = 31;
            for (ii=io=0; opts[ii]; ii++) {
                if (opts[ii][0] == '-' && !opts[ii][1]) {
                    if (io <= probey) yy += 4;
                    continue;
                }
                if (io < probey) yy += 8;
                io++;
            }

            onbar = (probey == 2 || probey == 9);
            x = probesm(c,yy+5,0,io);

            if (x == -1) { cmenu(200); break; }
#define OFFSHADE 16 
            yy = 31;
            for (ii=io=0; opts[ii]; ii++) {
                if (opts[ii][0] == '-' && !opts[ii][1]) {
                    yy += 4;
                    continue;
                }
                enabled = 1;
                switch (io) {
                    case 0:  if (x==io) ud.crosshair = 1-ud.crosshair;
                             gametextpal(d,yy, ud.crosshair ? "On" : "Off", 0, 0); break;
                    case 1:  if (x==io) ud.levelstats = 1-ud.levelstats;
                             gametextpal(d,yy, ud.levelstats ? "Shown" : "Hidden", 0, 0); break;
                    case 2:
                        {
                            short sbs, sbsl;
                            sbs = sbsl = scale(min(7,max(0,ud.statusbarscale-1)),63,7);
                            barsm(d+8,yy+7, &sbs,9,x==io,SHX(-5),PHX(-5));
                            if (x == io && sbs != sbsl) {
                                sbs = 1+sbs/9;
                                ud.statusbarscale = sbs;
                                setstatusbarscale(sbs);
                                vscrn();
                            }
                        }
                        break;
                    case 3:  enabled = !ud.mouseaiming;
                         if (enabled && x==io) myaimmode = 1-myaimmode;
                         gametextpal(d,yy, myaimmode ? "On" : "Off", enabled?0:OFFSHADE, 0); break;
                    case 4:  if (x==io) ud.mouseflip = 1-ud.mouseflip;
                         gametextpal(d,yy, ud.mouseflip ? "On" : "Off", 0, 0); break;
                    case 5:  if (ps[myconnectindex].gm&MODE_GAME || numplayers > 1) enabled = 0;
                         if (enabled && x==io) ud.mouseaiming = 1-ud.mouseaiming;
                                // don't change when in a multiplayer game
                                // because the state is sent during getnames()
                            // however, this will be fixed later
                         gametextpal(d,yy, ud.mouseaiming ? "Held" : "Toggle", enabled?0:OFFSHADE, 0); break;
                    case 6:  if (ps[myconnectindex].gm&MODE_GAME || numplayers > 1) enabled = 0;
                         if (enabled && x==io) AutoAim = 1-AutoAim;
                         gametextpal(d,yy, AutoAim ? "On" : "Off", enabled?0:OFFSHADE, 0); break;
                    case 7:  if (x==io) ud.runkey_mode = 1-ud.runkey_mode;
                         gametextpal(d,yy, ud.runkey_mode ? "Classic" : "Modern", 0, 0); break;
                    case 8:  if (ps[myconnectindex].gm&MODE_GAME || numplayers > 1) enabled = 0;
                         if (enabled && x==io) { ud.weaponswitch = (ud.weaponswitch == 3) ? 0 : ud.weaponswitch+1; }
                             { static const char *s[] = { "Off", "New", "Empty", "New+Empty" };
                                 gametextpal(d,yy, s[ud.weaponswitch], enabled?0:OFFSHADE, 0); break; }
                         break;
                    case 9:
                        {
                            s = ud.screen_size;
                            barsm(d+8,yy+7, &s,-4,x==io,SHX(-5),PHX(-5));
                            ud.screen_size = s;
                        }
                        break;
                    case 10: if (x==io) ud.detail = 1-ud.detail;
                         gametextpal(d,yy, ud.detail ? "High" : "Low", 0, 0); break;
                    case 11: if (x==io) ud.shadows = 1-ud.shadows;
                         gametextpal(d,yy, ud.shadows ? "On" : "Off", 0, 0); break;
                    case 12: if (x==io) ud.screen_tilting = 1-ud.screen_tilting;
                         gametextpal(d,yy, ud.screen_tilting ? "On" : "Off", 0, 0); break;  // original had a 'full' option
                    case 13: if (x==io) {
                                if( (ps[myconnectindex].gm&MODE_GAME) ) closedemowrite();
                                else ud.m_recstat = !ud.m_recstat;
                         }
                             if( (ps[myconnectindex].gm&MODE_GAME) && ud.m_recstat != 1 )
                             enabled = 0;
                         gametextpal(d,yy, ud.m_recstat==1 ? "On" : "Off", enabled?0:OFFSHADE, 0); break;
                    case 14: if (x==io) ForceSetup = 1-ForceSetup;
                         gametextpal(d,yy, ForceSetup ? "Yes" : "No", 0, 0); break;
                    default: break;
                }
                gametextpal(c,yy, opts[ii], enabled?0:OFFSHADE, 2);
                io++;
                yy += 8;
            }
        }
        break;
            
    case 200:   // Options menu.
        rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(320>>1,24,0,0,"OPTIONS");

        c = (200 - 18*5)>>1;

        onbar = 0;
        x = probe(160,c,18,5-NAM);

        switch (x) {
        case -1:
            if(ps[myconnectindex].gm&MODE_GAME) cmenu(50);else cmenu(0);
            break;

        case 0:
            cmenu(201);
            break;

        case 1:
            cmenu(700);
            break;

        case 2:
            {
                int dax = xdim, day = ydim, vm, vs;

                curvidmode = newvidmode = checkvideomode(&dax,&day,bpp,fullscreen,0);
                curvidset = newvidset = -1;
                changesmade = 0;

                // Work out each available combination of colour depth and fullscreen.
                numvidsets = 0;
                for (vm = 0; vm < validmodecnt && numvidsets < MAXVIDSETS; vm++) {
                    for (vs = 0; vs < numvidsets; vs++)
                        if (validmode[vm].bpp == vidsets[vs].bpp && (validmode[vm].fs&1) == vidsets[vs].fs)
                            break;
                    if (vs == numvidsets) {
                        vidsets[vs].bpp = validmode[vm].bpp;
                        vidsets[vs].fs = (validmode[vm].fs&1);
                        numvidsets++;
                    }
                    if (newvidset < 0 && vidsets[vs].bpp == bpp && vidsets[vs].fs == fullscreen) {
                        newvidset = vs;
                        if (newvidmode == 0x7fffffffl) newvidmode = vm;
                    }
                }
                if (newvidset < 0) newvidset = numvidsets - 1;
                curvidset = newvidset;

                cmenu(203);
            }
            break;
            
        case 3:
            cmenu(202);
            changesmade = -1;
            break;

        case 4:
#ifndef AUSTRALIA
            cmenu(10000);
#endif
            break;
        }

        menutext(160,c,0,0,"GAME OPTIONS");
        menutext(160,c+18,0,0,"SOUND OPTIONS");
        menutext(160,c+18+18,0,0,"VIDEO SETTINGS");
        menutext(160,c+18+18+18,0,0,"INPUT SETTINGS");
        if (!NAM) {
#ifndef AUSTRALIA
        menutext(160,c+18+18+18+18,0,0,"PARENTAL LOCK");
#else
        menutext(160,c+18+18+18+18,0,1,"PARENTAL LOCK");
#endif
        }
        break;

    case 202:   // Input settings.
        rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(320>>1,24,0,0,"INPUT SETTINGS");

        c = (200 - 18*5)>>1;

        onbar = 0;
        if (probey >= 3)
            x = probe(160,c+9,18,5);
        else
            x = probe(160,c,18,5);

        switch (x) {
        case -1:
            cmenu(200);
            probey = 3;
            break;

        case 0:
            currentlist = 0;
            // fall through
        case 1:
        case 2:
            if (x==2 && !CONTROL_JoyPresent) break;
            cmenu(204+x);
            break;

        case 3:
            CONFIG_SetDefaultKeyDefinitions(CONFIG_DEFAULTS_MODERN);
            CONFIG_SetMouseDefaults(CONFIG_DEFAULTS_MODERN);
            CONFIG_SetJoystickDefaults(CONFIG_DEFAULTS_MODERN);
            changesmade = CONFIG_DEFAULTS_MODERN;
            break;
        case 4:
            CONFIG_SetDefaultKeyDefinitions(CONFIG_DEFAULTS_CLASSIC);
            CONFIG_SetMouseDefaults(CONFIG_DEFAULTS_CLASSIC);
            CONFIG_SetJoystickDefaults(CONFIG_DEFAULTS_CLASSIC);
            changesmade = CONFIG_DEFAULTS_CLASSIC;
            break;
        }

        menutext(160,c,         0,0,"KEYS SETUP");
        menutext(160,c+18,      0,0,"MOUSE SETUP");
        menutext(160,c+18+18,   0,CONTROL_JoyPresent==0,"CONTROLLER SETUP");
        menutext(160,c+18+18+18+9, 0,0,"USE MODERN DEFAULTS");
        menutext(160,c+18+18+18+18+9,0,0,"USE CLASSIC DEFAULTS");
        if (changesmade == CONFIG_DEFAULTS_MODERN)
            gametext(160,158,"Modern defaults applied",0,2+8+16);
        else if (changesmade == CONFIG_DEFAULTS_CLASSIC)
            gametext(160,158,"Classic defaults applied",0,2+8+16);
        break;

    case 203:   // Video settings.
        rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(320>>1,24,0,0,"VIDEO SETTINGS");

        {
            int vs, vm;

            changesmade = 0;

            c = (320>>1)-120;
#if USE_POLYMOST && USE_OPENGL
            i = 7;
#else
            i = 5;
#endif
            onbar = (probey == 4);
            if (probey <= 2)
                x = probe(c+6,50,16,i);
            else if (probey == 3)
                x = probe(c+6,50+16+16+22,0,i);
            else
                x = probe(c+6,50+62-16-16-16,16,i);

            if ((probey >= 0 && probey <= 2) && (uinfo.dir == dir_West || uinfo.dir == dir_East)) {
                sound(PISTOL_BODYHIT);
                x = probey;
            }

            switch (x) {
                case -1:
                    cmenu(200);
                    probey = 2;
                    break;

                case 0: // Resolution.
                    if (uinfo.dir == dir_West) changesmade = -1;
                    else changesmade = 1;
                    break;

                case 1: // Colour mode.
                case 2: // Fullscreen.
                    for (vs = (newvidset + 1) % numvidsets; vs != newvidset; vs = (vs + 1) % numvidsets) {
                        if (x == 1 && vidsets[vs].fs != vidsets[newvidset].fs) continue;
                        if (x == 2 && vidsets[vs].bpp != vidsets[newvidset].bpp) continue;
                        changesmade = 1;
                        break;
                    }
                    newvidset = vs;
                    break;

                case 3: // Apply changes.
                    if (newvidmode != curvidmode) {
                        int pxdim, pydim, pfs, pbpp;
                        int nxdim, nydim, nfs, nbpp;

                        pxdim = xdim; pydim = ydim; pbpp = bpp; pfs = fullscreen;
                        nxdim = validmode[newvidmode].xdim;
                        nydim = validmode[newvidmode].ydim;
                        nfs   = validmode[newvidmode].fs;
                        nbpp  = validmode[newvidmode].bpp;

                        if (setgamemode(nfs, nxdim, nydim, nbpp) < 0) {
                            if (setgamemode(pfs, pxdim, pydim, pbpp) < 0) {
                                gameexit("Failed restoring old video mode.");
                            }
                        }
                        onvideomodechange(bpp > 8);
                        vscrn();

                        curvidmode = newvidmode; curvidset = newvidset;

                        ScreenMode = fullscreen;
                        ScreenWidth = xdim;
                        ScreenHeight = ydim;
                        ScreenBPP = bpp;
                    }
                    break;

                case 4: // Brightness.
                    break;

#if USE_POLYMOST && USE_OPENGL
                case 5: // Filtering.
                    if (bpp==8) break;
                    switch (gltexfiltermode) {
                        case 0: gltexfiltermode = 3; break;
                        case 3: gltexfiltermode = 5; break;
                        case 5: gltexfiltermode = 0; break;
                        default: gltexfiltermode = 3; break;
                    }
                    gltexapplyprops();
                    break;

                case 6: // Anisotropy.
                    if (bpp==8) break;
                    glanisotropy *= 2;
                    if (glanisotropy > glinfo.maxanisotropy) glanisotropy = 1;
                    gltexapplyprops();
                    break;
#endif
            }
            if (changesmade) {
                // Find the next/prev video mode matching the new vidset.
                int bestvm = newvidmode, besterr = INT_MAX, vmerr;
                for (vm = (newvidmode + changesmade) % validmodecnt; vm != newvidmode;
                        vm = (vm + changesmade) % validmodecnt) {
                    if ((validmode[vm].fs&1) != vidsets[newvidset].fs) continue;
                    if (validmode[vm].bpp != vidsets[newvidset].bpp) continue;
                    if (x == 0) {   // Resolution change.
                        bestvm = vm;
                        break;
                    }
                    if (validmode[vm].xdim == validmode[newvidmode].xdim &&
                            validmode[vm].ydim == validmode[newvidmode].ydim) {
                        bestvm = vm;
                        break;
                    }
                    vmerr = klabs((validmode[vm].xdim * validmode[vm].ydim) -
                        (validmode[newvidmode].xdim * validmode[newvidmode].ydim));
                    if (vmerr < besterr) {
                        bestvm = vm;
                        besterr = vmerr;
                    }
                }
                newvidmode = bestvm;
                changesmade = 0;
            }
        }

        menutext(c,50,0,0,"RESOLUTION");
        sprintf(buf,"%d x %d", validmode[newvidmode].xdim, validmode[newvidmode].ydim);
            gametext(c+154,50-8,buf,0,2+8+16);
        
        menutext(c,50+16,0,0,"VIDEO MODE");
            sprintf(buf, "%dbit %s", validmode[newvidmode].bpp,
                validmode[newvidmode].bpp > 8 ? "Polymost" : "Classic");
            gametext(c+154,50+16-8,buf,0,2+8+16);

        menutext(c,50+16+16,0,0,"FULLSCREEN");
            menutext(c+154,50+16+16,0,0,(validmode[newvidmode].fs&1)?"YES":"NO");

        menutext(c+16,50+16+16+22,0,newvidmode==curvidmode,"APPLY CHANGES");
        
        menutext(c,50+62+16,SHX(-6),PHX(-6),"BRIGHTNESS");
        {
            short ss = ud.brightness;
            bar(c+167,50+62+16,&ss,8,x==4,SHX(-6),PHX(-6));
            if(x==4) {
                ud.brightness = ss;
                setbrightness(ud.brightness>>2,&ps[myconnectindex].palette[0],0);
            }
        }

#if USE_POLYMOST && USE_OPENGL
        menutext(c,50+62+16+16,0,bpp==8,"FILTERING");
            switch (gltexfiltermode) {
                case 0: strcpy(buf,"NEAREST"); break;
                case 3: strcpy(buf,"BILINEAR"); break;
                case 5: strcpy(buf,"TRILINEAR"); break;
                default: strcpy(buf,"OTHER"); break;
            }
            menutext(c+154,50+62+16+16,0,bpp==8,buf);

        menutext(c,50+62+16+16+16,0,bpp==8,"ANISOTROPY");
            if (glanisotropy == 1) strcpy(buf,"NONE");
            else sprintf(buf,"%d-tap",glanisotropy);
            menutext(c+154,50+62+16+16+16,0,bpp==8,buf);
#endif
        break;

    case 204:
            rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,15,0,0,"KEYS SETUP");

        c = (320>>1)-120;

        onbar = 0;
        x = probe(0,0,0,NUMGAMEFUNCTIONS);

        if (x==-1) {
            cmenu(202);
            probey = 0;
        } else if (x>=0) {
            function = probey;
            whichkey = currentlist;
            cmenu(210);
            KB_FlushKeyboardQueue();
            KB_ClearLastScanCode();
            break;
        }

        // the top of our list
        m = probey - 6;
        if (m < 0) m = 0;
        else if (m + 13 >= NUMGAMEFUNCTIONS) m = NUMGAMEFUNCTIONS-13;
        
        if (probey == gamefunc_Show_Console) currentlist = 0;
        else if (uinfo.dir == dir_West || uinfo.dir == dir_East || KB_KeyPressed( sc_Tab )) {
            currentlist ^= 1;
            KB_ClearKeyDown( sc_Tab );
            sound(KICK_HIT);
        } else if (KB_KeyPressed( sc_Delete )) {
            KeyboardKeys[probey][currentlist] = 0;
            CONTROL_MapKey( probey, KeyboardKeys[probey][0], KeyboardKeys[probey][1] );
            sound(KICK_HIT);
            KB_ClearKeyDown( sc_Delete );
        }
        
        for (l=0; l < min(13,NUMGAMEFUNCTIONS); l++) {
            const char *p = CONFIG_FunctionNumToName(m+l);
            if (!p) continue;

            strcpy(buf, p);
            for (i=0;buf[i];i++) if (buf[i]=='_') buf[i] = ' ';
            minitextshade(70,30+l*8,buf,(m+l == probey)?0:16,0,10+16);

            //strcpy(buf, KB_ScanCodeToString(KeyboardKeys[m+l][0]));
            strcpy(buf, getkeyname(KeyboardKeys[m+l][0]));
            if (!buf[0]) strcpy(buf, "  -");
            minitextshade(70+100,30+l*8,buf,
                    (m+l == probey && !currentlist?0:16),2,10+16);

            //strcpy(buf, KB_ScanCodeToString(KeyboardKeys[m+l][1]));
            strcpy(buf, getkeyname(KeyboardKeys[m+l][1]));
            if (!buf[0]) strcpy(buf, "  -");
            minitextshade(70+120+34,30+l*8,buf,
                    (m+l == probey && currentlist?0:16),2,10+16);
        }

        gametext(160,140,"UP/DOWN = SELECT ACTION",0,2+8+16);
        gametext(160,140+9,"LEFT/RIGHT = SELECT LIST",0,2+8+16);
        gametext(160,140+9+9,"ENTER = MODIFY   DELETE = CLEAR",0,2+8+16);

        break;

    case 210: {
        int32 sc;

        rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(320>>1,15,0,0,"KEYS SETUP");

        gametext(320>>1,90,"PRESS THE KEY TO ASSIGN AS",0,2+8+16);
        sprintf(buf,"%s FOR \"%s\"", whichkey?"SECONDARY":"PRIMARY", CONFIG_FunctionNumToName(function));
        gametext(320>>1,90+9,buf,0,2+8+16);
        gametext(320>>1,90+9+9+9,"PRESS \"ESCAPE\" TO CANCEL",0,2+8+16);

        sc = KB_GetLastScanCode();
        if ( sc != sc_None ) {
            if ( sc == sc_Escape ) {
                sound(EXITMENUSOUND);
            } else {
                sound(PISTOL_BODYHIT);

                KeyboardKeys[function][whichkey] = KB_GetLastScanCode();
                if (function == gamefunc_Show_Console)
                    OSD_CaptureKey(KB_GetLastScanCode());
                else
                    CONTROL_MapKey( function, KeyboardKeys[function][0], KeyboardKeys[function][1] );
            }
        
            cmenu(204);

            currentlist = whichkey;
            probey = function;

            KB_ClearKeyDown(sc);
        }

        break;
    }

    case 205:
         rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,15,0,0,"MOUSE SETUP");

        c = 60-4;

        onbar = (probey == (MAXMOUSEBUTTONS-2)*2+2);
        if (probey < (MAXMOUSEBUTTONS-2)*2+2)
            x = probe(0,0,0,(MAXMOUSEBUTTONS-2)*2+2+2);
        else
            x = probe(c+6,125-((MAXMOUSEBUTTONS-2)*2+2)*16,16,(MAXMOUSEBUTTONS-2)*2+2+2);

        if (x==-1) {
            cmenu(202);
            probey = 1;
            break;
        } else if (x == (MAXMOUSEBUTTONS-2)*2+2) {
            // sensitivity
        } else if (x == (MAXMOUSEBUTTONS-2)*2+2+1) {
            //advanced
            cmenu(212);
            break;
        } else if (x >= 0) {
            //set an option
            cmenu(211);
            function = 0;
            whichkey = x;
            if (x < (MAXMOUSEBUTTONS-2)*2)
                probey = MouseFunctions[x>>1][x&1];
            else
                probey = MouseFunctions[x-4][0];
            if (probey < 0) probey = NUMGAMEFUNCTIONS-1;
            break;
        }

        for (l=0; l < (MAXMOUSEBUTTONS-2)*2+2; l++) {
            buf[0] = 0;
            if (l < (MAXMOUSEBUTTONS-2)*2) {
                if (l&1) {
                    Bstrcpy(buf, "Double ");
                    m = MouseFunctions[l>>1][1];
                } else
                    m = MouseFunctions[l>>1][0];
                Bstrcat(buf, mousebuttonnames[l>>1]);
            } else {
                Bstrcpy(buf, mousebuttonnames[l-(MAXMOUSEBUTTONS-2)]);
                m = MouseFunctions[l-(MAXMOUSEBUTTONS-2)][0];
            }

            minitextshade(c+20,30+l*8,buf,(l==probey)?0:16,0,10+16);

            if (m == -1)
                minitextshade(c+100+20,30+l*8,"  -NONE-",(l==probey)?0:16,0,10+16);
            else {
                strcpy(buf, CONFIG_FunctionNumToName(m));
                for (i=0;buf[i];i++) if (buf[i]=='_') buf[i] = ' ';
                minitextshade(c+100+20,30+l*8,buf,(l==probey)?0:16,0,10+16);
            }
        }

            {
                short sense;
                sense = CONTROL_GetMouseSensitivity()>>10;

                menutext(c,125,SHX(-7),PHX(-7),"SENSITIVITY");
                bar(c+167,125,&sense,4,x==(MAXMOUSEBUTTONS-2)*2+2,SHX(-7),PHX(-7));
                CONTROL_SetMouseSensitivity( sense<<10 );
            }

        menutext(c,125+16,0,0,"ADVANCED...");
        
        if (probey < (MAXMOUSEBUTTONS-2)*2+2) {
            gametext(160,149,"UP/DOWN = SELECT BUTTON",0,2+8+16);
            gametext(160,149+9,"ENTER = MODIFY",0,2+8+16);
        }
        break;

    case 211:
         rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            if (function == 0) menutext(320>>1,15,0,0,"MOUSE SETUP");
            else if (function == 1) menutext(320>>1,15,0,0,"ADVANCED MOUSE");
            else if (function == 2) menutext(320>>1,15,0,0,"CONTROLLER BUTTONS");
            else if (function == 3) menutext(320>>1,15,0,0,"CONTROLLER AXES");
    
        x = probe(0,0,0,NUMGAMEFUNCTIONS);

        if (x==-1) {
            if (function == 0) {    // mouse button
                cmenu(205);
                probey = whichkey;
            } else if (function == 1) { // mouse digital axis
                cmenu(212);
                probey = 2+(whichkey^2);
            } else if (function == 2) { // joystick button/hat
                cmenu(213);
                probey = whichkey;
            } else if (function == 3) { // joystick digital axis (max 12)
                cmenu((whichkey>>2)+214);
                probey = 1+((whichkey>>1)&1)*4+(whichkey&1);
            }
            break;
        } else if (x >= 0) {
            if (x == NUMGAMEFUNCTIONS-1) x = -1;
    
            if (function == 0) {
                if (whichkey < (MAXMOUSEBUTTONS-2)*2) {
                    MouseFunctions[whichkey>>1][whichkey&1] = x;
                    CONTROL_MapButton( x, whichkey>>1, whichkey&1, controldevice_mouse);
                } else {
                    MouseFunctions[whichkey-(MAXMOUSEBUTTONS-2)][0] = x;
                    CONTROL_MapButton( x, whichkey-(MAXMOUSEBUTTONS-2), 0, controldevice_mouse);
                }
                cmenu(205);
                probey = whichkey;
            } else if (function == 1) {
                MouseDigitalFunctions[whichkey>>1][whichkey&1] = x;
                CONTROL_MapDigitalAxis(whichkey>>1, x, whichkey&1, controldevice_mouse);
                cmenu(212);
                probey = 2+(whichkey^2);
            } else if (function == 2) {
				JoystickFunctions[whichkey>>1][whichkey&1] = x;
				CONTROL_MapButton( x, whichkey>>1, whichkey&1, controldevice_joystick);
                cmenu(213);
                probey = whichkey;
            } else if (function == 3) {
                JoystickDigitalFunctions[whichkey>>1][whichkey&1] = x;
                CONTROL_MapDigitalAxis(whichkey>>1, x, whichkey&1, controldevice_joystick);
				cmenu(214+(whichkey>>2));
                probey = 1+((whichkey>>1)&1)*4+(whichkey&1);
            }
            break;
        }

        gametext(320>>1,25,"SELECT A FUNCTION TO ASSIGN",0,2+8+16);

        if (function == 0) {
            if (whichkey < (MAXMOUSEBUTTONS-2)*2)
                sprintf(buf,"TO %s%s", (whichkey&1)?"DOUBLE-CLICKED ":"", mousebuttonnames[whichkey>>1]);
            else
                Bstrcpy(buf, mousebuttonnames[whichkey-(MAXMOUSEBUTTONS-2)]);
        } else if (function == 1) {
            Bstrcpy(buf,"TO DIGITAL ");
            switch (whichkey) {
                case 0: Bstrcat(buf, "LEFT"); break;
                case 1: Bstrcat(buf, "RIGHT"); break;
                case 2: Bstrcat(buf, "UP"); break;
                case 3: Bstrcat(buf, "DOWN"); break;
            }
        } else if (function == 2) {
            static const char *directions[] = { "UP", "RIGHT", "DOWN", "LEFT" };
			Bsprintf(buf,"TO %s%s", (whichkey&1)?"DOUBLE-CLICKED ":"", getjoyname(1,whichkey>>1));
        } else if (function == 3) {
            Bsprintf(buf,"TO DIGITAL %s %s",getjoyname(0,whichkey>>1),(whichkey&1)?"POSITIVE":"NEGATIVE");
        }

        gametext(320>>1,25+9,buf,0,2+8+16);

        if (KB_KeyPressed( sc_End )) { KB_ClearKeyDown(sc_End); probey = NUMGAMEFUNCTIONS-1; sound(KICK_HIT); }
        else if (KB_KeyPressed( sc_Home )) { KB_ClearKeyDown(sc_Home); probey = 0; sound(KICK_HIT); }
        
        m = probey - 6;
        if (m < 0) m = 0;
        else if (m + 13 >= NUMGAMEFUNCTIONS) m = NUMGAMEFUNCTIONS-13;

        for (l=0; l < min(13,NUMGAMEFUNCTIONS); l++) {
            if (l+m == NUMGAMEFUNCTIONS-1)
                strcpy(buf, "  -NONE-");
            else
                strcpy(buf, CONFIG_FunctionNumToName(m+l));

            for (i=0;buf[i];i++) if (buf[i]=='_') buf[i] = ' ';
            minitextshade(100,46+l*8,buf,(m+l == probey)?0:16,0,10+16);
        }

        gametext(320>>1,154,"PRESS \"ESCAPE\" TO CANCEL",0,2+8+16);

        break;

    case 212:
         rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,15,0,0,"ADVANCED MOUSE");

        c = (320>>1)-120;

        onbar = (probey == 0 || probey == 1);
        if (probey < 2)
            x = probe(c+6,40,16,6);
        else if (probey < 6) {
            m=50;
            x = probe(c+6+10,91-(10+10),10,6);
        } else {
            x = probe(c+6,140-(16+16+16+16+16+16),16,6);
        }

        switch (x) {
            case -1:
                cmenu(205);
                probey = (MAXMOUSEBUTTONS-2)*2+2+1;
                break;

            case 0:
                // x-axis scale
            case 1:
                // y-axis scale
                break;

            case 2:
                // digital up
            case 3:
                // digital down
            case 4:
                // digital left
            case 5:
                // digital right
                function = 1;
                whichkey = (x-2)^2; // flip the actual axis number
                cmenu(211);
                probey = MouseDigitalFunctions[whichkey>>1][whichkey&1];
                if (probey < 0) probey = NUMGAMEFUNCTIONS-1;
                break;

            case 6:
                // analogue x
            case 7:
                // analogue y
                l = MouseAnalogueAxes[x-6];
                if (l == analog_turning) l = analog_strafing;
                else if (l == analog_strafing) l = analog_lookingupanddown;
                else if (l == analog_lookingupanddown) l = analog_moving;
                else if (l == analog_moving) l = -1;
                else l = analog_turning;
                MouseAnalogueAxes[x-6] = l;
                CONTROL_MapAnalogAxis(x-6,l,controldevice_mouse);
                {
                    extern int32 mouseyaxismode;    // player.c
                    mouseyaxismode = -1;
                }
                break;

        }

        menutext(c,40,0,0,"X-AXIS SCALE");
        s = (MouseAnalogueScale[0]+262144) >> 13;
        bar(c+160+40,40,&s,1,x==0,0,0);
        l = ((int)s<<13)-262144;
        if (l != MouseAnalogueScale[0]) {
            CONTROL_SetAnalogAxisScale( 0, l, controldevice_mouse );
            MouseAnalogueScale[0] = l;
        }
        Bsprintf(buf,"%s%.2f",l>=0?" ":"",(float)l/65536.0);
        gametext(c+160-16,40-8,buf,0,2+8+16);
        
        menutext(c,40+16,0,0,"Y-AXIS SCALE");
        s = (MouseAnalogueScale[1]+262144) >> 13;
        bar(c+160+40,40+16,&s,1,x==1,0,0);
        l = ((int)s<<13)-262144;
        if (l != MouseAnalogueScale[1]) {
            CONTROL_SetAnalogAxisScale( 1, l, controldevice_mouse );
            MouseAnalogueScale[1] = l;
        }
        Bsprintf(buf,"%s%.2f",l>=0?" ":"",(float)l/65536.0);
        gametext(c+160-16,40+16-8,buf,0,2+8+16);
        
        menutext(c,40+16+16+8,0,0,"DIGITAL AXES ACTIONS");
        
        gametext(c+10,84,"UP:",0,2+8+16);
            if (MouseDigitalFunctions[1][0] < 0)
                strcpy(buf, "  -NONE-");
            else
                strcpy(buf, CONFIG_FunctionNumToName(MouseDigitalFunctions[1][0]));

            for (i=0;buf[i];i++) if (buf[i]=='_') buf[i] = ' ';
            minitext(c+10+60,85,buf,0,10+16);
        
        gametext(c+10,84+10,"DOWN:",0,2+8+16);
            if (MouseDigitalFunctions[1][1] < 0)
                strcpy(buf, "  -NONE-");
            else
                strcpy(buf, CONFIG_FunctionNumToName(MouseDigitalFunctions[1][1]));

            for (i=0;buf[i];i++) if (buf[i]=='_') buf[i] = ' ';
            minitext(c+10+60,85+10,buf,0,10+16);

        gametext(c+10,84+10+10,"LEFT:",0,2+8+16);
            if (MouseDigitalFunctions[0][0] < 0)
                strcpy(buf, "  -NONE-");
            else
                strcpy(buf, CONFIG_FunctionNumToName(MouseDigitalFunctions[0][0]));

            for (i=0;buf[i];i++) if (buf[i]=='_') buf[i] = ' ';
            minitext(c+10+60,85+10+10,buf,0,10+16);

        gametext(c+10,84+10+10+10,"RIGHT:",0,2+8+16);
            if (MouseDigitalFunctions[0][1] < 0)
                strcpy(buf, "  -NONE-");
            else
                strcpy(buf, CONFIG_FunctionNumToName(MouseDigitalFunctions[0][1]));

            for (i=0;buf[i];i++) if (buf[i]=='_') buf[i] = ' ';
            minitext(c+10+60,85+10+10+10,buf,0,10+16);

/* JBF 20040107: It would appear giving these options confuses some tinkerers, so they've
 * been moved to the bottom, and hidden in case I dare to reenable them again.
        menutext(c,116+16+8,0,0,"ANALOG X");
        if (CONFIG_AnalogNumToName( MouseAnalogueAxes[0] )) {
            p = CONFIG_AnalogNumToName( MouseAnalogueAxes[0] );
            if (p) {
                gametext(c+148+4,118+16, strchr(p,'_')+1, 0, 2+8+16 );
            }
        }
        if (probey == 6) gametext(160,158,"Default is \"turning\"",8,2+8+16);
        
        menutext(c,116+16+16+8,0,0,"ANALOG Y");
        if (CONFIG_AnalogNumToName( MouseAnalogueAxes[1] )) {
            p = CONFIG_AnalogNumToName( MouseAnalogueAxes[1] );
            if (p) {
                gametext(c+148+4,118+16+16, strchr(p,'_')+1, 0, 2+8+16 );
            }
        }
        if (probey == 7) gametext(160,158,"Default is \"moving\"",8,2+8+16);
*/
        break;

    case 206:
         rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"CONTROLLER SETUP");

        x = probe(160,100-18,18,3);

        switch (x) {
            case -1:
                cmenu(202);
                probey = 2;
                break;
            case 0:
            case 1:
                cmenu(213+x);
                break;
            case 2:
                cmenu(220);
                break;
        }

        menutext(160,100-18,0,0,"EDIT BUTTONS");
        menutext(160,100,0,0,"EDIT AXES");
        menutext(160,100+18,0,0,"DEAD ZONES");

        break;

    case 213:
         rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,15,0,0,"CONTROLLER BUTTONS");

        c = 2*joynumbuttons;

        x = probe(0,0,0,c);
            
        if (x == -1) {
            cmenu(206);
            probey = 0;
            break;
        } else if (x >= 0) {
            function = 2;
            whichkey = x;
            cmenu(211);
			probey = JoystickFunctions[x>>1][x&1];
            if (probey < 0) probey = NUMGAMEFUNCTIONS-1;
            break;
        }

        // the top of our list
        if (c < 13) m = 0;
        else {
            m = probey - 6;
            if (m < 0) m = 0;
            else if (m + 13 >= c) m = c-13;
        }
        
        for (l=0; l<min(13,c); l++) {
			sprintf(buf, "%s%s", ((l+m)&1)?"Double ":"", getjoyname(1,(l+m)>>1));
			x = JoystickFunctions[(l+m)>>1][(l+m)&1];
            minitextshade(80-4,33+l*8,buf,(m+l == probey)?0:16,0,10+16);

            if (x == -1)
                minitextshade(176,33+l*8,"  -NONE-",(m+l==probey)?0:16,0,10+16);
            else {
                strcpy(buf, CONFIG_FunctionNumToName(x));
                for (i=0;buf[i];i++) if (buf[i]=='_') buf[i] = ' ';
                minitextshade(176,33+l*8,buf,(m+l==probey)?0:16,0,10+16);
            }
        }
        
        gametext(160,149,"UP/DOWN = SELECT BUTTON",0,2+8+16);
        gametext(160,149+9,"ENTER = MODIFY",0,2+8+16);
        break;

    case 214:
    case 215:
    case 216:
    case 217:
    case 218:
    case 219: { // Two per page, 12 axes maximum.
        int thispage, twothispage;
         rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,15,0,0,"CONTROLLER AXES");

        thispage = current_menu-214;
        twothispage = (thispage*2+1 < joynumaxes);
        
        onbar = 0;
        switch (probey) {
            case 0:
            case 4: onbar = 1; x = probe(88,45+(probey==4)*64,0,1+(4<<twothispage)); break;
            case 1:
            case 2:
            case 5:
            case 6: x = probe(172+(probey==2||probey==6)*72,45+15+(probey==5||probey==6)*64,0,1+(4<<twothispage)); break;
            case 3:
            case 7: x = probe(88,45+15+15+(probey==7)*64,0,1+(4<<twothispage)); break;
            default: x = probe(60,79+79*twothispage,0,1+(4<<twothispage)); break;
        }

        switch (x) {
            case -1:
                cmenu(206);
                probey = 1;
                break;
            case 8:
                if (joynumaxes > 2) {
                    if (thispage == ((joynumaxes+1)/2)-1) cmenu(214);
                    else {
                        cmenu( current_menu+1 );
                    }
                }
                break;

            case 4: // bar
                if (!twothispage && joynumaxes > 2)
                    cmenu(214);
            case 0: break;

            case 1: // digitals
            case 2:
            case 5:
            case 6:
                function = 3;
                whichkey = ((thispage*2+(x==5||x==6)) << 1) + (x==2||x==6);
                cmenu(211);
                probey = JoystickDigitalFunctions[whichkey>>1][whichkey&1];
                if (probey < 0) probey = NUMGAMEFUNCTIONS-1;
                break;

            case 3: // analogues
            case 7:
                l = JoystickAnalogueAxes[thispage*2+(x==7)];
                if (l == analog_turning) l = analog_strafing;
                else if (l == analog_strafing) l = analog_lookingupanddown;
                else if (l == analog_lookingupanddown) l = analog_moving;
                else if (l == analog_moving) l = -1;
                else l = analog_turning;
                JoystickAnalogueAxes[thispage*2+(x==7)] = l;
                CONTROL_MapAnalogAxis(thispage*2+(x==7),l,controldevice_joystick);
                break;
            default:break;
        }

        menutext(42,32,0,0,getjoyname(0,thispage*2));
        if (twothispage) menutext(42,32+64,0,0,getjoyname(0,thispage*2+1));

        gametext(76,38,"SCALE",0,2+8+16);
        s = (JoystickAnalogueScale[thispage*2]+262144) >> 13;
        bar(140+56,38+8,&s,1,x==0,0,0);
        l = ((int)s<<13)-262144;
        if (l != JoystickAnalogueScale[thispage*2]) {
            CONTROL_SetAnalogAxisScale( thispage*2, l, controldevice_joystick );
            JoystickAnalogueScale[thispage*2] = l;
        }
        Bsprintf(buf,"%s%.2f",l>=0?" ":"",(float)l/65536.0);
        gametext(140,38,buf,0,2+8+16);

        gametext(76,38+15,"DIGITAL",0,2+8+16);
			gametext(140, 38+15, "-", 0, 2+8+16);
            if (JoystickDigitalFunctions[thispage*2][0] < 0)
                strcpy(buf, "  -NONE-");
            else
                strcpy(buf, CONFIG_FunctionNumToName(JoystickDigitalFunctions[thispage*2][0]));

            for (i=0;buf[i];i++) if (buf[i]=='_') buf[i] = ' ';
            minitext(140+12,38+16,buf,0,10+16);

			gametext(140+72, 38+15, "+", 0, 2+8+16);
            if (JoystickDigitalFunctions[thispage*2][1] < 0)
                strcpy(buf, "  -NONE-");
            else
                strcpy(buf, CONFIG_FunctionNumToName(JoystickDigitalFunctions[thispage*2][1]));

            for (i=0;buf[i];i++) if (buf[i]=='_') buf[i] = ' ';
            minitext(140+12+72,38+16,buf,0,10+16);
            
        gametext(76,38+15+15,"ANALOG",0,2+8+16);
        if (CONFIG_AnalogNumToName( JoystickAnalogueAxes[thispage*2] )) {
            const char *p = CONFIG_AnalogNumToName( JoystickAnalogueAxes[thispage*2] );
            if (p) {
                gametext(140+12,38+15+15, strchr(p,'_')+1, 0, 2+8+16 );
            }
        }
        
        if (twothispage) {
            gametext(76,38+64,"SCALE",0,2+8+16);
            s = (JoystickAnalogueScale[thispage*2+1]+262144) >> 13;
            bar(140+56,38+8+64,&s,1,x==4,0,0);
            l = ((int)s<<13)-262144;
            if (l != JoystickAnalogueScale[thispage*2+1]) {
                CONTROL_SetAnalogAxisScale( thispage*2+1, l, controldevice_joystick );
                JoystickAnalogueScale[thispage*2+1] = l;
            }
            Bsprintf(buf,"%s%.2f",l>=0?" ":"",(float)l/65536.0);
            gametext(140,38+64,buf,0,2+8+16);

            gametext(76,38+64+15,"DIGITAL",0,2+8+16);
				gametext(140, 38+64+15, "-", 0, 2+8+16);
                if (JoystickDigitalFunctions[thispage*2+1][0] < 0)
                    strcpy(buf, "  -NONE-");
                else
                    strcpy(buf, CONFIG_FunctionNumToName(JoystickDigitalFunctions[thispage*2+1][0]));

                for (i=0;buf[i];i++) if (buf[i]=='_') buf[i] = ' ';
                minitext(140+12,38+16+64,buf,0,10+16);

				gametext(140+72, 38+64+15, "+", 0, 2+8+16);
                if (JoystickDigitalFunctions[thispage*2+1][1] < 0)
                    strcpy(buf, "  -NONE-");
                else
                    strcpy(buf, CONFIG_FunctionNumToName(JoystickDigitalFunctions[thispage*2+1][1]));

                for (i=0;buf[i];i++) if (buf[i]=='_') buf[i] = ' ';
                minitext(140+12+72,38+16+64,buf,0,10+16);
            
            gametext(76,38+64+15+15,"ANALOG",0,2+8+16);
            if (CONFIG_AnalogNumToName( JoystickAnalogueAxes[thispage*2+1] )) {
                const char *p = CONFIG_AnalogNumToName( JoystickAnalogueAxes[thispage*2+1] );
                if (p) {
                    gametext(140+12,38+64+15+15, strchr(p,'_')+1, 0, 2+8+16 );
                }
            }
        }
        
        if (joynumaxes > 2) {
            menutext(42,twothispage?158:108,SHX(-10),(joynumaxes<=2),"NEXT...");
            sprintf(buf,"Page %d of %d",thispage+1,(joynumaxes+1)/2);
            gametext(320-100,158,buf,0,2+8+16);
        }
        break;
    }

    case 220:
    case 221:
    case 222: { // Four per page, 12 axes maximum.
        int first, last, haspages;
        unsigned short odx,dx,ody,dy;

        rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(320>>1,15,0,0,"CTLR. DEAD ZONES");

        first = 4*(current_menu-220);
        last  = min(4*(current_menu-220)+3, joynumaxes-1);
        haspages = joynumaxes>4;

        onbar = 1;
        if (probey == 2*(last-first+1))
            x = probe(60,38,15,2*(last-first+1)+haspages);
        else x = probe(320,38,15,2*(last-first+1)+haspages);
        if (x==-1) {
            cmenu(206);
            probey = 2;
            break;
        } else if (x==2*(last-first+1)+1 - 1) {    // +1 is 'next page' entry, -1 because 0-based.
            cmenu( last == joynumaxes-1 ? 220 : (current_menu+1) );
            probey = 0;
            break;
        }

        for (m=first;m<=last;m++) {
            menutext(42,38+30*(m-first),0,0,getjoyname(0,m));

            gametext(128,38+30*(m-first)-8,"DEAD",0,2+8+16);
            gametext(128,38+30*(m-first)-8+15,"SATU",0,2+8+16);

            dx = odx = min(64,64l*JoystickAnalogueDead[m]/32767l);
            dy = ody = min(64,64l*JoystickAnalogueSaturate[m]/32767l);

            bar(217,38+30*(m-first),(short*)&dx,4,x==((m-first)*2),0,0);
            bar(217,38+30*(m-first)+15,(short*)&dy,4,x==((m-first)*2+1),0,0);

            Bsprintf(buf,"%3d%%",100*dx/64); gametext(217-49,38+30*(m-first)-8,buf,0,2+8+16);
            Bsprintf(buf,"%3d%%",100*dy/64); gametext(217-49,38+30*(m-first)-8+15,buf,0,2+8+16);

            if (dx != odx) JoystickAnalogueDead[m]     = 32767l*dx/64l;
            if (dy != ody) JoystickAnalogueSaturate[m] = 32767l*dy/64l;
            if (dx != odx || dy != ody) {
                CONTROL_SetJoyAxisDead(m, JoystickAnalogueDead[m]);
                CONTROL_SetJoyAxisSaturate(m, JoystickAnalogueSaturate[m]);
            }
        }
        //gametext(160,158,"DEAD = DEAD ZONE, SAT. = SATURATION",0,2+8+16);
        if (haspages) {
            menutext(42,38+30*(last-first+1),0,0,"NEXT...");
            sprintf(buf,"Page %d of %d", 1+(current_menu-220), (joynumaxes+3)/4);
            gametext(320-100,158,buf,0,2+8+16);
        }
        break;      
    }
        
        case 700:
            c = (320>>1)-120;
            rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"SOUNDS");
            onbar = ( probey == 2 || probey == 3 );

            x = probe(c,50,16,7);
            switch(x)
            {
                case -1:
                    cmenu(200);
                    probey = 1;
                    break;
                case 0:
                    if (FXDevice >= 0)
                    {
                        SoundToggle = 1-SoundToggle;
                        if( SoundToggle == 0 )
                        {
                            FX_StopAllSounds();
                            clearsoundlocks();
                        }
                        onbar = 0;
                    }
                    break;
                case 1:

                    if(numplayers < 2)
                        if(MusicDevice >= 0)
                    {
                        MusicToggle = 1-MusicToggle;
                        if( MusicToggle == 0 ) MusicPause(1);
                        else
                        {
                            if(ud.recstat != 2 && ps[myconnectindex].gm&MODE_GAME)
                                playmusic(&music_fn[0][music_select][0]);
                            else playmusic(&env_music_fn[0][0]);

                            MusicPause(0);
                        }
                    }
                    onbar = 0;

                    break;
                case 4:
                    if(SoundToggle && (FXDevice >= 0)) VoiceToggle = 1-VoiceToggle;
                    onbar = 0;
                    break;
                case 5:
                    if(SoundToggle && (FXDevice >= 0)) AmbienceToggle = 1-AmbienceToggle;
                    onbar = 0;
                    break;
                case 6:
                    if(SoundToggle && (FXDevice >= 0))
                    {
                        ReverseStereo = 1-ReverseStereo;
                        FX_SetReverseStereo(ReverseStereo);
                    }
                    onbar = 0;
                    break;
                default:
                    onbar = 1;
                    break;
            }

            if(SoundToggle && FXDevice >= 0) menutext(c+160+40,50,0,(FXDevice<0),"ON");
            else menutext(c+160+40,50,0,(FXDevice<0),"OFF");

            if(MusicToggle && (MusicDevice >= 0) && (numplayers<2))
                menutext(c+160+40,50+16,0,(MusicDevice < 0),"ON");
            else menutext(c+160+40,50+16,0,(MusicDevice < 0),"OFF");

            menutext(c,50,SHX(-2),(FXDevice<0),"SOUND");
            menutext(c,50+16+16,SHX(-4),(FXDevice<0)||SoundToggle==0,"SOUND VOLUME");
            {
                l = FXVolume;
                s = FXVolume >> 2;
                bar(c+167+40,50+16+16,&s,4,(FXDevice>=0)&&x==2,SHX(-4),SoundToggle==0||(FXDevice<0));
                FXVolume = s << 2;
                if(l != FXVolume)
                    FX_SetVolume( (short) FXVolume );
            }
            menutext(c,50+16,SHX(-3),(MusicDevice<0),"MUSIC");
            menutext(c,50+16+16+16,SHX(-5),(MusicDevice<0)||MusicToggle==0,"MUSIC VOLUME");
            {
                l = MusicVolume;
                s = MusicVolume >> 2;
                bar(c+167+40,50+16+16+16,
                    &s,4,
                    (MusicDevice>=0) && x==3,SHX(-5),
                    MusicToggle==0||(MusicDevice<0));
                MusicVolume = s << 2;
                if(l != MusicVolume)
                    MusicSetVolume( (short) MusicVolume );

            }
            menutext(c,50+16+16+16+16,SHX(-6),(FXDevice<0)||SoundToggle==0,"DUKE TALK");
            menutext(c,50+16+16+16+16+16,SHX(-7),(FXDevice<0)||SoundToggle==0,"AMBIENCE");

            menutext(c,50+16+16+16+16+16+16,SHX(-8),(FXDevice<0)||SoundToggle==0,"FLIP STEREO");

            if(VoiceToggle) menutext(c+160+40,50+16+16+16+16,0,(FXDevice<0)||SoundToggle==0,"ON");
            else menutext(c+160+40,50+16+16+16+16,0,(FXDevice<0)||SoundToggle==0,"OFF");

            if(AmbienceToggle) menutext(c+160+40,50+16+16+16+16+16,0,(FXDevice<0)||SoundToggle==0,"ON");
            else menutext(c+160+40,50+16+16+16+16+16,0,(FXDevice<0)||SoundToggle==0,"OFF");

            if(ReverseStereo) menutext(c+160+40,50+16+16+16+16+16+16,0,(FXDevice<0)||SoundToggle==0,"ON");
            else menutext(c+160+40,50+16+16+16+16+16+16,0,(FXDevice<0)||SoundToggle==0,"OFF");


            break;

        case 350:
            cmenu(351);
            screencapt = 1;
            displayrooms(myconnectindex,65536);
            screencapt = 0;
            break;

        case 360:
        case 361:
        case 362:
        case 363:
        case 364:
        case 365:
        case 366:
        case 367:
        case 368:
        case 369:
        case 351:
        case 300:

            c = 320>>1;
            rotatesprite(c<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64,0,0,xdim-1,ydim-1);
            rotatesprite(c<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);

            if(current_menu == 300) menutext(c,24,0,0,"LOAD GAME");
            else menutext(c,24,0,0,"SAVE GAME");

            if(current_menu >= 360 && current_menu <= 369 )
            {
                sprintf(buf,"PLAYERS: %-2d                      ",ud.multimode);
                gametext(160,156,buf,0,2+8+16);
                sprintf(buf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill);
                gametext(160,168,buf,0,2+8+16);
                if (ud.volume_number == 0 && ud.level_number == 7)
                    gametext(160,180,boardfilename,0,2+8+16);

                x = strget((320>>1),184,&ud.savegame[current_menu-360][0],19, STRGET_NOECHO );

                if(x == -1)
                {
            //        readsavenames();
                    memcpy(ud.savegame[current_menu-360], ud.baksavegame, sizeof(ud.baksavegame));
                    ps[myconnectindex].gm = MODE_GAME;
                    if(ud.multimode < 2  && ud.recstat != 2)
                    {
                        ready2send = 1;
                        totalclock = ototalclock;
                    }
                    goto DISPLAYNAMES;
                }

                if( x == 1 )
                {
                    if( ud.savegame[current_menu-360][0] == 0 )
                    {
                        KB_FlushKeyboardQueue();
                        cmenu(351);
                    }
                    else
                    {
                        if(ud.multimode > 1)
                            saveplayer(-1-(current_menu-360));
                        else saveplayer(current_menu-360);
                        lastsavedpos = current_menu-360;
                        ps[myconnectindex].gm = MODE_GAME;

                        if(ud.multimode < 2  && ud.recstat != 2)
                        {
                            ready2send = 1;
                            totalclock = ototalclock;
                        }
                        KB_ClearKeyDown(sc_Escape);
                        sound(EXITMENUSOUND);
                    }
                }

                rotatesprite(101<<16,97<<16,65536>>1,512,TILE_SAVESHOT,-32,0,2+4+8+64,0,0,xdim-1,ydim-1);
                dispnames();
                rotatesprite((c+67+strlen(&ud.savegame[current_menu-360][0])*4)<<16,(50+12*probey)<<16,32768L-10240,0,SPINNINGNUKEICON+(((totalclock)>>3)%7),0,0,10,0,0,xdim-1,ydim-1);
                break;
            }

           last_threehundred = probey;

            x = probe(c+68,54,12,10);

          if(current_menu == 300)
          {
              if( ud.savegame[probey][0] )
              {
                  if( lastprobey != probey )
                  {
                     loadpheader(probey,&savehead);
                     lastprobey = probey;
                  }

                  rotatesprite(101<<16,97<<16,65536L>>1,512,TILE_LOADSHOT,-32,0,4+10+64,0,0,xdim-1,ydim-1);
                  sprintf(buf,"PLAYERS: %-2d                      ",savehead.numplr);
                  gametext(160,156,buf,0,2+8+16);
                  sprintf(buf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+savehead.volnum,1+savehead.levnum,savehead.plrskl);
                  gametext(160,168,buf,0,2+8+16);
                  if (savehead.volnum == 0 && savehead.levnum == 7)
                      gametext(160,180,savehead.boardfn,0,2+8+16);
              }
              else menutext(69,70,0,0,"EMPTY");
          }
          else
          {
              if( ud.savegame[probey][0] )
              {
                  if(lastprobey != probey)
                      loadpheader(probey,&savehead);
                  lastprobey = probey;
                  rotatesprite(101<<16,97<<16,65536L>>1,512,TILE_LOADSHOT,-32,0,4+10+64,0,0,xdim-1,ydim-1);
              }
              else menutext(69,70,0,0,"EMPTY");
              sprintf(buf,"PLAYERS: %-2d                      ",ud.multimode);
              gametext(160,156,buf,0,2+8+16);
              sprintf(buf,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill);
              gametext(160,168,buf,0,2+8+16);
              if (ud.volume_number == 0 && ud.level_number == 7)
                  gametext(160,180,boardfilename,0,2+8+16);
          }

            switch( x )
            {
                case -1:
                    if(current_menu == 300)
                    {
                        if( (ps[myconnectindex].gm&MODE_GAME) != MODE_GAME)
                        {
                            cmenu(0);
                            break;
                        }
                        else
                            ps[myconnectindex].gm &= ~MODE_MENU;
                    }
                    else
                        ps[myconnectindex].gm = MODE_GAME;

                    if(ud.multimode < 2 && ud.recstat != 2)
                    {
                        ready2send = 1;
                        totalclock = ototalclock;
                    }

                    break;
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                    if( current_menu == 300)
                    {
                        if( ud.savegame[x][0] )
                            current_menu = (1000+x);
                    }
                    else
                    {
                        memcpy(ud.baksavegame, ud.savegame[x], sizeof(ud.baksavegame));
                        if( ud.savegame[x][0] != 0)
                            current_menu = 2000+x;
                        else
                        {
                            KB_FlushKeyboardQueue();
                            current_menu = (360+x);
                            sprintf(ud.savegame[x], "GAME %d", x + 1);
                            inputloc = strlen(ud.savegame[x]);
                        }
                    }
                    break;
            }

            DISPLAYNAMES:
            dispnames();
            break;

        case 400:
        case 401:
            if (VOLUMEALL) goto VOLUME_ALL_40x;
            // fall through
        case 402:
        case 403:

            c = 320>>1;

            probe(0,0,0,0);

            if(uinfo.dir == dir_West || uinfo.dir == dir_North || KB_KeyPressed( sc_PgUp ))
            {
                KB_ClearKeyDown(sc_PgUp);

                sound(KICK_HIT);
                current_menu--;
                if(current_menu < 400) current_menu = 403;
            }
            else if(uinfo.dir == dir_East || uinfo.dir == dir_South || KB_KeyPressed( sc_PgDn ))
            {
                KB_ClearKeyDown(sc_PgDn);
                sound(KICK_HIT);
                current_menu++;
                if(current_menu > 403) current_menu = 400;
            }

            if(uinfo.button1)
            {
                if(ps[myconnectindex].gm&MODE_GAME)
                    cmenu(50);
                else cmenu(0);
                return;
            }

            flushperms();
            rotatesprite(0,0,65536L,0,ORDERING+current_menu-400,0,0,10+16+64,0,0,xdim-1,ydim-1);

        break;
VOLUME_ALL_40x:

            c = 320>>1;

            probe(0,0,0,0);

            if(uinfo.dir == dir_West || uinfo.dir == dir_North || KB_KeyPressed( sc_PgUp ))
            {
                KB_ClearKeyDown(sc_PgUp);

                sound(KICK_HIT);
                current_menu--;
                if(current_menu < 400) current_menu = 401;
            }
            else if(uinfo.dir == dir_East || uinfo.dir == dir_South || KB_KeyPressed( sc_PgDn ))
            {
                KB_ClearKeyDown(sc_PgDn);

                sound(KICK_HIT);
                current_menu++;
                if(current_menu > 401) current_menu = 400;
            }

            if(uinfo.button1)
            {
                if(ps[myconnectindex].gm&MODE_GAME)
                    cmenu(50);
                else cmenu(0);
                return;
            }

            flushperms();
            switch(current_menu)
            {
                case 400:
                    rotatesprite(0,0,65536L,0,TEXTSTORY,0,0,10+16+64, 0,0,xdim-1,ydim-1);
                    break;
                case 401:
                    rotatesprite(0,0,65536L,0,F1HELP,0,0,10+16+64, 0,0,xdim-1,ydim-1);
                    break;
            }

            break;

        case 500:
            c = 320>>1;

            gametext(c,90,"Are you sure you want to quit?",0,2+8+16);
            gametext(c,99,"(Y/N)",0,2+8+16);

            if(uinfo.button0 || KB_KeyPressed(sc_Y))
            {
                KB_FlushKeyboardQueue();

                if( gamequit == 0 && ( numplayers > 1 ) )
                {
                    if(ps[myconnectindex].gm&MODE_GAME)
                    {
                        gamequit = 1;
                        quittimer = totalclock+120;
                    }
                    else
                    {
                        sendlogoff();
                        gameexit(" ");
                    }
                }
                else if( numplayers < 2 )
                    gameexit(" ");

                if( ( totalclock > quittimer ) && ( gamequit == 1) )
                    gameexit("Timed out.");
            }

            x = probe(186,124,0,0);
            if(x == -1 || KB_KeyPressed(sc_N))
            {
                KB_ClearKeyDown(sc_N);
                quittimer = 0;
                if( ps[myconnectindex].gm&MODE_DEMO )
                    ps[myconnectindex].gm = MODE_DEMO;
                else
                {
                    ps[myconnectindex].gm &= ~MODE_MENU;
                    if(ud.multimode < 2  && ud.recstat != 2)
                    {
                        ready2send = 1;
                        totalclock = ototalclock;
                    }
                }
            }

            break;
        case 501:
            c = 320>>1;
            gametext(c,90,"Quit to Title?",0,2+8+16);
            gametext(c,99,"(Y/N)",0,2+8+16);

            if(uinfo.button0 || KB_KeyPressed(sc_Y))
            {
                KB_FlushKeyboardQueue();
                ps[myconnectindex].gm = MODE_DEMO;
                if(ud.recstat == 1)
                    closedemowrite();
                cmenu(0);
            }

            x = probe(186,124,0,0);

            if(x == -1 || KB_KeyPressed(sc_N))
            {
                ps[myconnectindex].gm &= ~MODE_MENU;
                if(ud.multimode < 2  && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
            }

            break;

        case 601:
            displayfragbar();
            rotatesprite(160<<16,29<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,34,0,0,&ud.user_name[myconnectindex][0]);

            sprintf(buf,"Waiting for master");
            gametext(160,50,buf,0,2+8+16);
            gametext(160,59,"to select level",0,2+8+16);

            if(uinfo.button1)
            {
                sound(EXITMENUSOUND);
                cmenu(0);
            }
            break;

        case 602:
            if(menunamecnt == 0)
            {
        //        getfilenames("SUBD");
                getfilenames(".","*.MAP");
                if (menunamecnt == 0)
                    cmenu(600);
            }
            // fall through
        case 603:
            c = (320>>1) - 120;
            displayfragbar();
            rotatesprite(320>>1<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"USER MAPS");
            for(x=0;x<menunamecnt;x++)
            {
                if(x == fileselect)
                    minitext(15 + (x/15)*54,32 + (x%15)*8,menuname[x],0,26);
                else minitext(15 + (x/15)*54,32 + (x%15)*8,menuname[x],16,26);
            }

            fileselect = probey;
            if (uinfo.dir == dir_West) {
                probey -= 15;
                if(probey < 0) probey += 15;
                else sound(KICK_HIT);
            } else if (uinfo.dir == dir_East) {
                probey += 15;
                if(probey >= menunamecnt)
                    probey -= 15;
                else sound(KICK_HIT);
            }

            onbar = 0;
            x = probe(0,0,0,menunamecnt);

            if(x == -1) cmenu(600);
            else if(x >= 0)
            {
                tempbuf[0] = 8;
                tempbuf[1] = ud.m_level_number = 6;
                tempbuf[2] = ud.m_volume_number = 0;
                tempbuf[3] = ud.m_player_skill+1;

                if(ud.player_skill == 3)
                    ud.m_respawn_monsters = 1;
                else ud.m_respawn_monsters = 0;

                if(ud.m_coop == 0) ud.m_respawn_items = 1;
                else ud.m_respawn_items = 0;

                ud.m_respawn_inventory = 1;

                tempbuf[4] = ud.m_monsters_off;
                tempbuf[5] = ud.m_respawn_monsters;
                tempbuf[6] = ud.m_respawn_items;
                tempbuf[7] = ud.m_respawn_inventory;
                tempbuf[8] = ud.m_coop;
                tempbuf[9] = ud.m_marker;

                x = strlen(menuname[probey]);

                copybufbyte(menuname[probey],tempbuf+10,x);
                copybufbyte(menuname[probey],boardfilename,x+1);

                for(c=connecthead;c>=0;c=connectpoint2[c])
                {
                    if (c != myconnectindex) sendpacket(c,tempbuf,x+10);
                    if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
                }

                newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill+1);
                if (enterlevel(MODE_GAME)) backtomenu();
            }
            break;

        case 600:
            c = (320>>1) - 120;
            if((ps[myconnectindex].gm&MODE_GAME) != MODE_GAME)
                displayfragbar();
            rotatesprite(160<<16,26<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,31,0,0,&ud.user_name[myconnectindex][0]);

            x = probe(c,57-8,16,8);

            switch(x)
            {
                case -1:
                    ud.m_recstat = 0;
                    if(ps[myconnectindex].gm&MODE_GAME) cmenu(50);
                    else cmenu(0);
                    break;
                case 0:
                    ud.m_coop++;
                    if(ud.m_coop == 3) ud.m_coop = 0;
                    break;
                case 1:
if (!VOLUMEONE) {
                    ud.m_volume_number++;
if (PLUTOPAK) {
                    if(ud.m_volume_number > 3) ud.m_volume_number = 0;
} else {
                    if(ud.m_volume_number > 2) ud.m_volume_number = 0;
}
                    if(ud.m_volume_number == 0 && ud.m_level_number > 6)
                        ud.m_level_number = 0;
                    if(ud.m_level_number > 10) ud.m_level_number = 0;
}
                    break;
                case 2:
                    ud.m_level_number++;
if (!VOLUMEONE) {
                    if(ud.m_volume_number == 0 && ud.m_level_number > 6)
                        ud.m_level_number = 0;
} else {
                    if(ud.m_volume_number == 0 && ud.m_level_number > 5)
                        ud.m_level_number = 0;
}
                    if(ud.m_level_number > 10) ud.m_level_number = 0;
                    break;
                case 3:
                    if(ud.m_monsters_off == 1 && ud.m_player_skill > 0)
                        ud.m_monsters_off = 0;

                    if(ud.m_monsters_off == 0)
                    {
                        ud.m_player_skill++;
                        if(ud.m_player_skill > 3)
                        {
                            ud.m_player_skill = 0;
                            ud.m_monsters_off = 1;
                        }
                    }
                    else ud.m_monsters_off = 0;

                    break;

                case 4:
                    if(ud.m_coop == 0)
                        ud.m_marker = !ud.m_marker;
                    break;

                case 5:
                    if(ud.m_coop == 1)
                        ud.m_ffire = !ud.m_ffire;
                    break;

                case 6:
                if (VOLUMEALL) {
                    if(boardfilename[0] == 0) break;

                    tempbuf[0] = 5;
                    tempbuf[1] = ud.m_level_number = 7;
                    tempbuf[2] = ud.m_volume_number = 0;
                    tempbuf[3] = ud.m_player_skill+1;

                    ud.level_number = ud.m_level_number;
                    ud.volume_number = ud.m_volume_number;

                    if( ud.m_player_skill == 3 ) ud.m_respawn_monsters = 1;
                    else ud.m_respawn_monsters = 0;

                    if(ud.m_coop == 0) ud.m_respawn_items = 1;
                    else ud.m_respawn_items = 0;

                    ud.m_respawn_inventory = 1;

                    tempbuf[4] = ud.m_monsters_off;
                    tempbuf[5] = ud.m_respawn_monsters;
                    tempbuf[6] = ud.m_respawn_items;
                    tempbuf[7] = ud.m_respawn_inventory;
                    tempbuf[8] = ud.m_coop;
                    tempbuf[9] = ud.m_marker;
                    tempbuf[10] = ud.m_ffire;

                    for(c=connecthead;c>=0;c=connectpoint2[c])
                    {
                        resetweapons(c);
                        resetinventory(c);
                    }
                    for(c=connecthead;c>=0;c=connectpoint2[c])
                    {
                        if (c != myconnectindex) sendpacket(c,tempbuf,11);
                        if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
                    }

                    newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill+1);
                    if (enterlevel(MODE_GAME)) backtomenu();

                    return;
                }
                // fall through
                case 7:

                    tempbuf[0] = 5;
                    tempbuf[1] = ud.m_level_number;
                    tempbuf[2] = ud.m_volume_number;
                    tempbuf[3] = ud.m_player_skill+1;

                    if( ud.m_player_skill == 3 ) ud.m_respawn_monsters = 1;
                    else ud.m_respawn_monsters = 0;

                    if(ud.m_coop == 0) ud.m_respawn_items = 1;
                    else ud.m_respawn_items = 0;

                    ud.m_respawn_inventory = 1;

                    tempbuf[4] = ud.m_monsters_off;
                    tempbuf[5] = ud.m_respawn_monsters;
                    tempbuf[6] = ud.m_respawn_items;
                    tempbuf[7] = ud.m_respawn_inventory;
                    tempbuf[8] = ud.m_coop;
                    tempbuf[9] = ud.m_marker;
                    tempbuf[10] = ud.m_ffire;

                    for(c=connecthead;c>=0;c=connectpoint2[c])
                    {
                        resetweapons(c);
                        resetinventory(c);
                    }
                    for(c=connecthead;c>=0;c=connectpoint2[c])
                    {
                        if(c != myconnectindex) sendpacket(c,tempbuf,11);
                        if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
                    }

                    newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill+1);
                    if (enterlevel(MODE_GAME)) backtomenu();

                    return;

            }

            c += 40;

            if(ud.m_coop==1) gametext(c+70,57-7-9,"COOPERATIVE PLAY",0,2+8+16);
            else if(ud.m_coop==2) gametext(c+70,57-7-9,NAM?"GRUNTMATCH (NO SPAWN)":"DUKEMATCH (NO SPAWN)",0,2+8+16);
            else gametext(c+70,57-7-9,NAM?"GRUNTMATCH (SPAWN)":"DUKEMATCH (SPAWN)",0,2+8+16);

if (VOLUMEONE) {
            gametext(c+70,57+16-7-9,volume_names[ud.m_volume_number],0,2+8+16);
} else {
            gametext(c+70,57+16-7-9,volume_names[ud.m_volume_number],0,2+8+16);
}

            gametext(c+70,57+16+16-7-9,&level_names[11*ud.m_volume_number+ud.m_level_number][0],0,2+8+16);

            if(ud.m_monsters_off == 0 || ud.m_player_skill > 0)
                gametext(c+70,57+16+16+16-7-9,skill_names[ud.m_player_skill],0,2+8+16);
            else gametext(c+70,57+16+16+16-7-9,"NONE",0,2+8+16);

            if(ud.m_coop == 0)
            {
                if(ud.m_marker)
                    gametext(c+70,57+16+16+16+16-7-9,"ON",0,2+8+16);
                else gametext(c+70,57+16+16+16+16-7-9,"OFF",0,2+8+16);
            }

            if(ud.m_coop == 1)
            {
                if(ud.m_ffire)
                    gametext(c+70,57+16+16+16+16+16-7-9,"ON",0,2+8+16);
                else gametext(c+70,57+16+16+16+16+16-7-9,"OFF",0,2+8+16);
            }

            c -= 44;

            menutext(c,57-9,SHX(-2),PHX(-2),"GAME TYPE");

if (VOLUMEONE) {
            sprintf(buf,"EPISODE %d",ud.m_volume_number+1);
            menutext(c,57+16-9,SHX(-3),1,buf);
} else {
            sprintf(buf,"EPISODE %d",ud.m_volume_number+1);
            menutext(c,57+16-9,SHX(-3),PHX(-3),buf);
}

            sprintf(buf,"LEVEL %d",ud.m_level_number+1);
            menutext(c,57+16+16-9,SHX(-4),PHX(-4),buf);
            menutext(c,57+16+16+16-9,SHX(-5),PHX(-5),"MONSTERS");

            if(ud.m_coop == 0)
                menutext(c,57+16+16+16+16-9,SHX(-6),PHX(-6),"MARKERS");
            else
                menutext(c,57+16+16+16+16-9,SHX(-6),1,"MARKERS");

            if(ud.m_coop == 1)
                menutext(c,57+16+16+16+16+16-9,SHX(-6),PHX(-6),"FR. FIRE");
            else menutext(c,57+16+16+16+16+16-9,SHX(-6),1,"FR. FIRE");

if (VOLUMEALL) {
            menutext(c,57+16+16+16+16+16+16-9,SHX(-7),boardfilename[0] == 0,"USER MAP");
            if( boardfilename[0] != 0 )
                gametext(c+70+44,57+16+16+16+16+16,boardfilename,0,2+8+16);
} else {
            menutext(c,57+16+16+16+16+16+16-9,SHX(-7),1,"USER MAP");
}

            menutext(c,57+16+16+16+16+16+16+16-9,SHX(-8),PHX(-8),"START GAME");

            break;
    }

    if( (ps[myconnectindex].gm&MODE_MENU) != MODE_MENU)
    {
        vscrn();
        cameraclock = totalclock;
        cameradist = 65536L;
    }
}

void palto(unsigned char r, unsigned char g, unsigned char b,int e)
{
    int tc;

    setpalettefade(r,g,b,e&127);
    if ((e&128) == 0) {
        nextpage();
        for (tc = totalclock; totalclock < tc + 4; handleevents(), getpackets() );
    }
}


void drawoverheadmap(int cposx, int cposy, int czoom, short cang)
{
        int i, j, k, l, x1, y1, x2=0, y2=0, x3, y3, x4, y4, ox, oy, xoff, yoff;
        int dax, day, cosang, sinang, xspan, yspan, sprx, spry;
        int xrepeat, yrepeat, z1, z2, startwall, endwall, tilenum, daang;
        int xvect, yvect, xvect2, yvect2;
        short p;
        unsigned char col;
        walltype *wal, *wal2;
        spritetype *spr;

        xvect = sintable[(-cang)&2047] * czoom;
        yvect = sintable[(1536-cang)&2047] * czoom;
        xvect2 = mulscale16(xvect,yxaspect);
        yvect2 = mulscale16(yvect,yxaspect);

                //Draw red lines
        for(i=0;i<numsectors;i++)
        {
                if (!(show2dsector[i>>3]&(1<<(i&7)))) continue;

                startwall = sector[i].wallptr;
                endwall = sector[i].wallptr + sector[i].wallnum;

                z1 = sector[i].ceilingz; z2 = sector[i].floorz;

                for(j=startwall,wal=&wall[startwall];j<endwall;j++,wal++)
                {
                        k = wal->nextwall; if (k < 0) continue;

                        //if ((show2dwall[j>>3]&(1<<(j&7))) == 0) continue;
                        //if ((k > j) && ((show2dwall[k>>3]&(1<<(k&7))) > 0)) continue;

                        if (sector[wal->nextsector].ceilingz == z1)
                                if (sector[wal->nextsector].floorz == z2)
                                        if (((wal->cstat|wall[wal->nextwall].cstat)&(16+32)) == 0) continue;

                        col = 139; //red
                        if ((wal->cstat|wall[wal->nextwall].cstat)&1) col = 234; //magenta

                        if (!(show2dsector[wal->nextsector>>3]&(1<<(wal->nextsector&7))))
                                col = 24;
            else continue;

                        ox = wal->x-cposx; oy = wal->y-cposy;
                        x1 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
                        y1 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);

                        wal2 = &wall[wal->point2];
                        ox = wal2->x-cposx; oy = wal2->y-cposy;
                        x2 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
                        y2 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);

                        drawline256(x1,y1,x2,y2,col);
                }
        }

                //Draw sprites
        k = ps[screenpeek].i;
        for(i=0;i<numsectors;i++)
        {
                if (!(show2dsector[i>>3]&(1<<(i&7)))) continue;
                for(j=headspritesect[i];j>=0;j=nextspritesect[j])
                        //if ((show2dsprite[j>>3]&(1<<(j&7))) > 0)
                        {
                spr = &sprite[j];

                if (j == k || (spr->cstat&0x8000) || spr->cstat == 257 || spr->xrepeat == 0) continue;

                                col = 71; //cyan
                                if (spr->cstat&1) col = 234; //magenta

                                sprx = spr->x;
                                spry = spr->y;

                if( (spr->cstat&257) != 0) switch (spr->cstat&48)
                                {
                    case 0: break;

                                                ox = sprx-cposx; oy = spry-cposy;
                                                x1 = dmulscale16(ox,xvect,-oy,yvect);
                                                y1 = dmulscale16(oy,xvect2,ox,yvect2);

                                                ox = (sintable[(spr->ang+512)&2047]>>7);
                                                oy = (sintable[(spr->ang)&2047]>>7);
                                                x2 = dmulscale16(ox,xvect,-oy,yvect);
                                                y2 = dmulscale16(oy,xvect,ox,yvect);

                                                x3 = mulscale16(x2,yxaspect);
                                                y3 = mulscale16(y2,yxaspect);

                                                drawline256(x1-x2+(xdim<<11),y1-y3+(ydim<<11),
                                                                                x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
                                                drawline256(x1-y2+(xdim<<11),y1+x3+(ydim<<11),
                                                                                x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
                                                drawline256(x1+y2+(xdim<<11),y1-x3+(ydim<<11),
                                                                                x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
                        break;

                                        case 16:
                        if( spr->picnum == LASERLINE )
                        {
                            x1 = sprx; y1 = spry;
                            tilenum = spr->picnum;
                            xoff = (int)((signed char)((picanm[tilenum]>>8)&255))+((int)spr->xoffset);
                            if ((spr->cstat&4) > 0) xoff = -xoff;
                            k = spr->ang; l = spr->xrepeat;
                            dax = sintable[k&2047]*l; day = sintable[(k+1536)&2047]*l;
                            l = tilesizx[tilenum]; k = (l>>1)+xoff;
                            x1 -= mulscale16(dax,k); x2 = x1+mulscale16(dax,l);
                            y1 -= mulscale16(day,k); y2 = y1+mulscale16(day,l);

                            ox = x1-cposx; oy = y1-cposy;
                            x1 = dmulscale16(ox,xvect,-oy,yvect);
                            y1 = dmulscale16(oy,xvect2,ox,yvect2);

                            ox = x2-cposx; oy = y2-cposy;
                            x2 = dmulscale16(ox,xvect,-oy,yvect);
                            y2 = dmulscale16(oy,xvect2,ox,yvect2);

                            drawline256(x1+(xdim<<11),y1+(ydim<<11),
                                                                                x2+(xdim<<11),y2+(ydim<<11),col);
                        }

                        break;

                    case 32:

                                                tilenum = spr->picnum;
                                                xoff = (int)((signed char)((picanm[tilenum]>>8)&255))+((int)spr->xoffset);
                                                yoff = (int)((signed char)((picanm[tilenum]>>16)&255))+((int)spr->yoffset);
                                                if ((spr->cstat&4) > 0) xoff = -xoff;
                                                if ((spr->cstat&8) > 0) yoff = -yoff;

                                                k = spr->ang;
                                                cosang = sintable[(k+512)&2047]; sinang = sintable[k];
                                                xspan = tilesizx[tilenum]; xrepeat = spr->xrepeat;
                                                yspan = tilesizy[tilenum]; yrepeat = spr->yrepeat;

                                                dax = ((xspan>>1)+xoff)*xrepeat; day = ((yspan>>1)+yoff)*yrepeat;
                                                x1 = sprx + dmulscale16(sinang,dax,cosang,day);
                                                y1 = spry + dmulscale16(sinang,day,-cosang,dax);
                                                l = xspan*xrepeat;
                                                x2 = x1 - mulscale16(sinang,l);
                                                y2 = y1 + mulscale16(cosang,l);
                                                l = yspan*yrepeat;
                                                k = -mulscale16(cosang,l); x3 = x2+k; x4 = x1+k;
                                                k = -mulscale16(sinang,l); y3 = y2+k; y4 = y1+k;

                                                ox = x1-cposx; oy = y1-cposy;
                                                x1 = dmulscale16(ox,xvect,-oy,yvect);
                                                y1 = dmulscale16(oy,xvect2,ox,yvect2);

                                                ox = x2-cposx; oy = y2-cposy;
                                                x2 = dmulscale16(ox,xvect,-oy,yvect);
                                                y2 = dmulscale16(oy,xvect2,ox,yvect2);

                                                ox = x3-cposx; oy = y3-cposy;
                                                x3 = dmulscale16(ox,xvect,-oy,yvect);
                                                y3 = dmulscale16(oy,xvect2,ox,yvect2);

                                                ox = x4-cposx; oy = y4-cposy;
                                                x4 = dmulscale16(ox,xvect,-oy,yvect);
                                                y4 = dmulscale16(oy,xvect2,ox,yvect2);

                                                drawline256(x1+(xdim<<11),y1+(ydim<<11),
                                                                                x2+(xdim<<11),y2+(ydim<<11),col);

                                                drawline256(x2+(xdim<<11),y2+(ydim<<11),
                                                                                x3+(xdim<<11),y3+(ydim<<11),col);

                                                drawline256(x3+(xdim<<11),y3+(ydim<<11),
                                                                                x4+(xdim<<11),y4+(ydim<<11),col);

                                                drawline256(x4+(xdim<<11),y4+(ydim<<11),
                                                                                x1+(xdim<<11),y1+(ydim<<11),col);

                                                break;
                                }
                        }
        }

                //Draw white lines
        for(i=0;i<numsectors;i++)
        {
                if (!(show2dsector[i>>3]&(1<<(i&7)))) continue;

                startwall = sector[i].wallptr;
                endwall = sector[i].wallptr + sector[i].wallnum;

                k = -1;
                for(j=startwall,wal=&wall[startwall];j<endwall;j++,wal++)
                {
                        if (wal->nextwall >= 0) continue;

                        //if ((show2dwall[j>>3]&(1<<(j&7))) == 0) continue;

                        if (tilesizx[wal->picnum] == 0) continue;
                        if (tilesizy[wal->picnum] == 0) continue;

                        if (j == k)
                                { x1 = x2; y1 = y2; }
                        else
                        {
                                ox = wal->x-cposx; oy = wal->y-cposy;
                                x1 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
                                y1 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);
                        }

                        k = wal->point2; wal2 = &wall[k];
                        ox = wal2->x-cposx; oy = wal2->y-cposy;
                        x2 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
                        y2 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);

                        drawline256(x1,y1,x2,y2,24);
                }
        }

         for(p=connecthead;p >= 0;p=connectpoint2[p])
         {
          if(ud.scrollmode && p == screenpeek) continue;

          ox = sprite[ps[p].i].x-cposx; oy = sprite[ps[p].i].y-cposy;
                  daang = (sprite[ps[p].i].ang-cang)&2047;
                  if (p == screenpeek) { ox = 0; oy = 0; daang = 0; }
                  x1 = mulscale(ox,xvect,16) - mulscale(oy,yvect,16);
                  y1 = mulscale(oy,xvect2,16) + mulscale(ox,yvect2,16);

          if(p == screenpeek || ud.coop == 1 )
          {
                if(sprite[ps[p].i].xvel > 16 && ps[p].on_ground)
                    i = APLAYERTOP+((totalclock>>4)&3);
                else
                    i = APLAYERTOP;

                j = klabs(ps[p].truefz-ps[p].posz)>>8;
                j = mulscale(czoom*(sprite[ps[p].i].yrepeat+j),yxaspect,16);

                if(j < 22000) j = 22000;
                else if(j > (65536<<1)) j = (65536<<1);

                rotatesprite((x1<<4)+(xdim<<15),(y1<<4)+(ydim<<15),j,
                    daang,i,sprite[ps[p].i].shade,sprite[ps[p].i].pal,
                    (sprite[ps[p].i].cstat&2)>>1,windowx1,windowy1,windowx2,windowy2);
          }
         }
}



void endanimsounds(int fr)
{
    switch(ud.volume_number)
    {
        case 0:break;
        case 1:
            switch(fr)
            {
                case 1:
                    sound(WIND_AMBIENCE);
                    break;
                case 26:
                    sound(ENDSEQVOL2SND1);
                    break;
                case 36:
                    sound(ENDSEQVOL2SND2);
                    break;
                case 54:
                    sound(THUD);
                    break;
                case 62:
                    sound(ENDSEQVOL2SND3);
                    break;
                case 75:
                    sound(ENDSEQVOL2SND4);
                    break;
                case 81:
                    sound(ENDSEQVOL2SND5);
                    break;
                case 115:
                    sound(ENDSEQVOL2SND6);
                    break;
                case 124:
                    sound(ENDSEQVOL2SND7);
                    break;
            }
            break;
        case 2:
            switch(fr)
            {
                case 1:
                    sound(WIND_REPEAT);
                    break;
                case 98:
                    sound(DUKE_GRUNT);
                    break;
                case 82+20:
                    sound(THUD);
                    sound(SQUISHED);
                    break;
                case 104+20:
                    sound(ENDSEQVOL3SND3);
                    break;
                case 114+20:
                    sound(ENDSEQVOL3SND2);
                    break;
                case 158:
                    sound(PIPEBOMB_EXPLODE);
                    break;
            }
            break;
    }
}

void logoanimsounds(int fr)
{
    switch(fr)
    {
        case 1:
            sound(FLY_BY);
            break;
        case 19:
            sound(PIPEBOMB_EXPLODE);
            break;
    }
}

void intro4animsounds(int fr)
{
    switch(fr)
    {
        case 1:
            sound(INTRO4_B);
            break;
        case 12:
        case 34:
            sound(SHORT_CIRCUIT);
            break;
        case 18:
            sound(INTRO4_5);
            break;
    }
}

void first4animsounds(int fr)
{
    switch(fr)
    {
        case 1:
            sound(INTRO4_1);
            break;
        case 12:
            sound(INTRO4_2);
            break;
        case 7:
            sound(INTRO4_3);
            break;
        case 26:
            sound(INTRO4_4);
            break;
    }
}

void intro42animsounds(int fr)
{
    switch(fr)
    {
        case 10:
            sound(INTRO4_6);
            break;
    }
}




void endanimvol41(int fr)
{
    switch(fr)
    {
        case 3:
            sound(DUKE_UNDERWATER);
            break;
        case 35:
            sound(VOL4ENDSND1);
            break;
    }
}

void endanimvol42(int fr)
{
    switch(fr)
    {
        case 11:
            sound(DUKE_UNDERWATER);
            break;
        case 20:
            sound(VOL4ENDSND1);
            break;
        case 39:
            sound(VOL4ENDSND2);
            break;
        case 50:
            FX_StopAllSounds();
            break;
    }
}

void endanimvol43(int fr)
{
    switch(fr)
    {
        case 1:
            sound(BOSS4_DEADSPEECH);
            break;
        case 40:
            sound(VOL4ENDSND1);
            sound(DUKE_UNDERWATER);
            break;
        case 50:
            sound(BIGBANG);
            break;
    }
}


void playanm(const char *fn,char t)
{
    unsigned char *animbuf, *palptr, palbuf[768];
    int i, j, k, length=0, numframes=0;
    int32 handle=-1;
    UserInput uinfo;

//    return;

    if(t != 7 && t != 9 && t != 10 && t != 11)
        KB_FlushKeyboardQueue();

    uinfo.dir = dir_None;
    uinfo.button0 = uinfo.button1 = FALSE;
    
    handle = kopen4load(fn,0);
    if(handle == -1) return;
    length = kfilelength(handle);

    walock[TILE_ANIM] = 219+t;

    allocache((void **)&animbuf,length,&walock[TILE_ANIM]);

    tilesizx[TILE_ANIM] = 200;
    tilesizy[TILE_ANIM] = 320;

    kread(handle,animbuf,length);
    kclose(handle);

    ANIM_LoadAnim (animbuf);
    numframes = ANIM_NumFrames();

    palptr = ANIM_GetPalette();
    for(i=0;i<256;i++)
    {
        j = i*3;
        palbuf[j+0] = (palptr[j+0]>>2);
        palbuf[j+1] = (palptr[j+1]>>2);
        palbuf[j+2] = (palptr[j+2]>>2);
    }

    setgamepalette(&ps[myconnectindex],palbuf,2);

    ototalclock = totalclock + 10;

    for(i=1;i<numframes;i++)
    {
        while(totalclock < ototalclock)
        {
            handleevents();
            getpackets();
            CONTROL_GetUserInput(&uinfo);
            if( KB_KeyWaiting() || uinfo.button0 || uinfo.button1 )
                goto ENDOFANIMLOOP;
        }

        if(t == 10) ototalclock += 14;
        else if(t == 9) ototalclock += 10;
        else if(t == 7) ototalclock += 18;
        else if(t == 6) ototalclock += 14;
        else if(t == 5) ototalclock += 9;
        else if(ud.volume_number == 3) ototalclock += 10;
        else if(ud.volume_number == 2) ototalclock += 10;
        else if(ud.volume_number == 1) ototalclock += 18;
        else                           ototalclock += 10;

        waloff[TILE_ANIM] = (intptr_t)(ANIM_DrawFrame(i));
#if USE_POLYMOST && USE_OPENGL
        invalidatetile(TILE_ANIM, 0, 1<<4);  // JBF 20031228
#endif
        clearallviews(0);
        rotatesprite(0<<16,0<<16,65536L,512,TILE_ANIM,0,0,2+4+8+16+64, 0,0,xdim-1,ydim-1);
        nextpage();

        if(t == 8) endanimvol41(i);
        else if(t == 10) endanimvol42(i);
        else if(t == 11) endanimvol43(i);
        else if(t == 9) intro42animsounds(i);
        else if(t == 7) intro4animsounds(i);
        else if(t == 6) first4animsounds(i);
        else if(t == 5) logoanimsounds(i);
        else if(t < 4) endanimsounds(i);
    }

    ENDOFANIMLOOP:

    CONTROL_ClearUserInput(&uinfo);
    ANIM_FreeAnim ();
    walock[TILE_ANIM] = 1;
}

/*
 * vim:ts=4:sw=4:tw=8:enc=utf-8:
 */


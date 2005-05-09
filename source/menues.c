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

#include "duke3d.h"
#include "mouse.h"
#include "animlib.h"
#include "osd.h"
#include <sys/stat.h>


struct savehead {
	char name[19];
	int32 numplr,volnum,levnum,plrskl;
	char boardfn[BMAX_PATH];
};

extern char inputloc;
extern int recfilep;
//extern char vgacompatible;
short probey=0,lastprobey=0,last_menu,globalskillsound=-1;
short sh,onbar,buttonstat,deletespot;
short last_zero,last_fifty,last_threehundred = 0;

static char fileselect = 1, menunamecnt, menuname[256][64], curpath[80], menupath[80];

static struct _directoryitem {
	struct _directoryitem *next, *prev;
	char name[64];
	unsigned long size;
	struct tm mtime;
} *filedirectory = (struct _directoryitem*)0, *dirdirectory = (struct _directoryitem*)0,
  *filehighlight = (struct _directoryitem*)0, *dirhighlight = (struct _directoryitem*)0;
static int numdirs=0, numfiles=0;
static int currentlist=0;

static int function, whichkey;
static int changesmade, newvidmode, curvidmode, newfullscreen;
static int vidsets[16] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 }, curvidset, newvidset = 0;

static char *mousebuttonnames[] = { "Left", "Right", "Middle", "Thumb", "Wheel Down", "Wheel Up" };


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
}


void savetemp(char *fn,long daptr,long dasiz)
{
    int fp;

    fp = Bopen(fn,BO_WRONLY|BO_CREAT|BO_TRUNC|BO_BINARY,0/*BS_IRUSR|BS_IWUSR|S_IRGRP|S_IWGRP*/);

    Bwrite(fp,(char *)daptr,dasiz);

    Bclose(fp);
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

int loadpheader(char spot,struct savehead *saveh)
{
	long i;
	char fn[13];
	long fil;
	long bv;

	strcpy(fn, "game0.sav");
    fn[4] = spot+'0';

	if ((fil = kopen4load(fn,0)) == -1) return(-1);

	walock[MAXTILES-3] = 255;

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

	if (waloff[MAXTILES-3] == 0) allocache(&waloff[MAXTILES-3],320*200,&walock[MAXTILES-3]);
	tilesizx[MAXTILES-3] = 200; tilesizy[MAXTILES-3] = 320;
	if (kdfread((char *)waloff[MAXTILES-3],320,200,fil) != 200) goto corrupt;
	invalidatetile(MAXTILES-3,0,255);

	kclose(fil);

	return(0);
corrupt:
	kclose(fil);
	return 1;
}


int loadplayer(signed char spot)
{
     short k,music_changed;
     char fn[13];
     char mpfn[13];
     char *fnptr, scriptptrs[MAXSCRIPTSIZE];
     long fil, bv, i, j, x;
     int32 nump;

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
         MUSIC_StopSong();

     if(numplayers > 1) {
         if (kdfread(&buf,19,1,fil) != 1) goto corrupt;
	 } else {
         if (kdfread(&ud.savegame[spot][0],19,1,fil) != 1) goto corrupt;
	 }

     music_changed = (music_select != (ud.volume_number*11) + ud.level_number);

         if (kdfread(&ud.volume_number,sizeof(ud.volume_number),1,fil) != 1) goto corrupt;
         if (kdfread(&ud.level_number,sizeof(ud.level_number),1,fil) != 1) goto corrupt;
         if (kdfread(&ud.player_skill,sizeof(ud.player_skill),1,fil) != 1) goto corrupt;
     if (kdfread(&boardfilename[0],BMAX_PATH,1,fil) != 1) goto corrupt;

         ud.m_level_number = ud.level_number;
         ud.m_volume_number = ud.volume_number;
         ud.m_player_skill = ud.player_skill;

                 //Fake read because lseek won't work with compression
     walock[MAXTILES-3] = 1;
     if (waloff[MAXTILES-3] == 0) allocache(&waloff[MAXTILES-3],320*200,&walock[MAXTILES-3]);
     tilesizx[MAXTILES-3] = 200; tilesizy[MAXTILES-3] = 320;
     if (kdfread((char *)waloff[MAXTILES-3],320,200,fil) != 200) goto corrupt;
	 invalidatetile(MAXTILES-3,0,255);

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
         if (kdfread(&msx[0],sizeof(long),sizeof(msx)/sizeof(long),fil) != sizeof(msx)/sizeof(long)) goto corrupt;
         if (kdfread(&msy[0],sizeof(long),sizeof(msy)/sizeof(long),fil) != sizeof(msy)/sizeof(long)) goto corrupt;
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

     if (kdfread(&scriptptrs[0],1,MAXSCRIPTSIZE,fil) != MAXSCRIPTSIZE) goto corrupt;
     if (kdfread(&script[0],4,MAXSCRIPTSIZE,fil) != MAXSCRIPTSIZE) goto corrupt;
     for(i=0;i<MAXSCRIPTSIZE;i++)
        if( scriptptrs[i] )
     {
         j = (long)script[i]+(long)&script[0];
         script[i] = j;
     }

     if (kdfread(&actorscrptr[0],4,MAXTILES,fil) != MAXTILES) goto corrupt;
     for(i=0;i<MAXTILES;i++)
         if(actorscrptr[i])
     {
        j = (long)actorscrptr[i]+(long)&script[0];
        actorscrptr[i] = (long *)j;
     }

     if (kdfread(&scriptptrs[0],1,MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
     if (kdfread(&hittype[0],sizeof(struct weaponhit),MAXSPRITES,fil) != MAXSPRITES) goto corrupt;

     for(i=0;i<MAXSPRITES;i++)
     {
        j = (long)(&script[0]);
        if( scriptptrs[i]&1 ) T2 += j;
        if( scriptptrs[i]&2 ) T5 += j;
        if( scriptptrs[i]&4 ) T6 += j;
     }

         if (kdfread(&lockclock,sizeof(lockclock),1,fil) != 1) goto corrupt;
     if (kdfread(&pskybits,sizeof(pskybits),1,fil) != 1) goto corrupt;
     if (kdfread(&pskyoff[0],sizeof(pskyoff[0]),MAXPSKYTILES,fil) != MAXPSKYTILES) goto corrupt;

         if (kdfread(&animatecnt,sizeof(animatecnt),1,fil) != 1) goto corrupt;
         if (kdfread(&animatesect[0],2,MAXANIMATES,fil) != MAXANIMATES) goto corrupt;
         if (kdfread(&animateptr[0],4,MAXANIMATES,fil) != MAXANIMATES) goto corrupt;
     for(i = animatecnt-1;i>=0;i--) animateptr[i] = (long *)((long)animateptr[i]+(long)(&sector[0]));
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
     docacheit();

//     if(music_changed == 0)	// JBF: why, pray-tell?
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
	 Bsprintf(tempbuf,"Save game file \"%s\" is corrupt.",fnptr);
	 gameexit(tempbuf);
	 return -1;
}

int saveplayer(signed char spot)
{
     long i, j;
     char fn[13];
     char mpfn[13];
     char *fnptr,scriptptrs[MAXSCRIPTSIZE];
         FILE *fil;
     long bv = BYTEVERSION;

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
     dfwrite((char *)waloff[MAXTILES-1],320,200,fil);

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
         dfwrite(&msx[0],sizeof(long),sizeof(msx)/sizeof(long),fil);
         dfwrite(&msy[0],sizeof(long),sizeof(msy)/sizeof(long),fil);
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

     for(i=0;i<MAXSCRIPTSIZE;i++)
     {
          if( (long)script[i] >= (long)(&script[0]) && (long)script[i] < (long)(&script[MAXSCRIPTSIZE]) )
          {
                scriptptrs[i] = 1;
                j = (long)script[i] - (long)&script[0];
                script[i] = j;
          }
          else scriptptrs[i] = 0;
     }

     dfwrite(&scriptptrs[0],1,MAXSCRIPTSIZE,fil);
     dfwrite(&script[0],4,MAXSCRIPTSIZE,fil);

     for(i=0;i<MAXSCRIPTSIZE;i++)
        if( scriptptrs[i] )
     {
        j = script[i]+(long)&script[0];
        script[i] = j;
     }

     for(i=0;i<MAXTILES;i++)
         if(actorscrptr[i])
     {
        j = (long)actorscrptr[i]-(long)&script[0];
        actorscrptr[i] = (long *)j;
     }
     dfwrite(&actorscrptr[0],4,MAXTILES,fil);
     for(i=0;i<MAXTILES;i++)
         if(actorscrptr[i])
     {
         j = (long)actorscrptr[i]+(long)&script[0];
         actorscrptr[i] = (long *)j;
     }

     for(i=0;i<MAXSPRITES;i++)
     {
        scriptptrs[i] = 0;

        if(actorscrptr[PN] == 0) continue;

        j = (long)&script[0];

        if(T2 >= j && T2 < (long)(&script[MAXSCRIPTSIZE]) )
        {
            scriptptrs[i] |= 1;
            T2 -= j;
        }
        if(T5 >= j && T5 < (long)(&script[MAXSCRIPTSIZE]) )
        {
            scriptptrs[i] |= 2;
            T5 -= j;
        }
        if(T6 >= j && T6 < (long)(&script[MAXSCRIPTSIZE]) )
        {
            scriptptrs[i] |= 4;
            T6 -= j;
        }
    }

    dfwrite(&scriptptrs[0],1,MAXSPRITES,fil);
    dfwrite(&hittype[0],sizeof(struct weaponhit),MAXSPRITES,fil);

    for(i=0;i<MAXSPRITES;i++)
    {
        if(actorscrptr[PN] == 0) continue;
        j = (long)&script[0];

        if(scriptptrs[i]&1)
            T2 += j;
        if(scriptptrs[i]&2)
            T5 += j;
        if(scriptptrs[i]&4)
            T6 += j;
    }

         dfwrite(&lockclock,sizeof(lockclock),1,fil);
     dfwrite(&pskybits,sizeof(pskybits),1,fil);
     dfwrite(&pskyoff[0],sizeof(pskyoff[0]),MAXPSKYTILES,fil);
         dfwrite(&animatecnt,sizeof(animatecnt),1,fil);
         dfwrite(&animatesect[0],2,MAXANIMATES,fil);
         for(i = animatecnt-1;i>=0;i--) animateptr[i] = (long *)((long)animateptr[i]-(long)(&sector[0]));
         dfwrite(&animateptr[0],4,MAXANIMATES,fil);
         for(i = animatecnt-1;i>=0;i--) animateptr[i] = (long *)((long)animateptr[i]+(long)(&sector[0]));
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

#define LMB (buttonstat&1)
#define RMB (buttonstat&2)

ControlInfo minfo;

long mi;

int probe(int x,int y,int i,int n)
{
    short centre, s;

    s = 1+(CONTROL_GetMouseSensitivity()>>4);

    if( ControllerType == 1 && CONTROL_MousePresent )
    {
        CONTROL_GetInput( &minfo );
        mi += minfo.dz;
    }

    else minfo.dz = minfo.dyaw = 0;

    if( x == (320>>1) )
        centre = 320>>2;
    else centre = 0;

    if(!buttonstat)
    {
        if( KB_KeyPressed( sc_UpArrow ) || KB_KeyPressed( sc_PgUp ) || KB_KeyPressed( sc_kpad_8 ) ||
            mi < -8192 )
        {
            mi = 0;
            KB_ClearKeyDown( sc_UpArrow );
            KB_ClearKeyDown( sc_kpad_8 );
            KB_ClearKeyDown( sc_PgUp );
            sound(KICK_HIT);

            probey--;
            if(probey < 0) probey = n-1;
            minfo.dz = 0;
        }
        if( KB_KeyPressed( sc_DownArrow ) || KB_KeyPressed( sc_PgDn ) || KB_KeyPressed( sc_kpad_2 )
            || mi > 8192 )
        {
            mi = 0;
            KB_ClearKeyDown( sc_DownArrow );
            KB_ClearKeyDown( sc_kpad_2 );
            KB_ClearKeyDown( sc_PgDn );
            sound(KICK_HIT);
            probey++;
            minfo.dz = 0;
        }
    }

    if(probey >= n)
        probey = 0;

    if(centre)
    {
//        rotatesprite(((320>>1)+(centre)+54)<<16,(y+(probey*i)-4)<<16,65536L,0,SPINNINGNUKEICON+6-((6+(totalclock>>3))%7),sh,0,10,0,0,xdim-1,ydim-1);
//        rotatesprite(((320>>1)-(centre)-54)<<16,(y+(probey*i)-4)<<16,65536L,0,SPINNINGNUKEICON+((totalclock>>3)%7),sh,0,10,0,0,xdim-1,ydim-1);

        rotatesprite(((320>>1)+(centre>>1)+70)<<16,(y+(probey*i)-4)<<16,65536L,0,SPINNINGNUKEICON+6-((6+(totalclock>>3))%7),sh,0,10,0,0,xdim-1,ydim-1);
        rotatesprite(((320>>1)-(centre>>1)-70)<<16,(y+(probey*i)-4)<<16,65536L,0,SPINNINGNUKEICON+((totalclock>>3)%7),sh,0,10,0,0,xdim-1,ydim-1);
    }
    else
        rotatesprite((x-tilesizx[BIGFNTCURSOR]-4)<<16,(y+(probey*i)-4)<<16,65536L,0,SPINNINGNUKEICON+(((totalclock>>3))%7),sh,0,10,0,0,xdim-1,ydim-1);

    if( KB_KeyPressed(sc_Space) || KB_KeyPressed( sc_kpad_Enter ) || KB_KeyPressed( sc_Enter ) || (LMB && !onbar) )
    {
        if(current_menu != 110)
            sound(PISTOL_BODYHIT);
        KB_ClearKeyDown( sc_Enter );
        KB_ClearKeyDown( sc_Space );
        KB_ClearKeyDown( sc_kpad_Enter );
        return(probey);
    }
    else if( KB_KeyPressed( sc_Escape ) || (RMB) )
    {
        onbar = 0;
        KB_ClearKeyDown( sc_Escape );
        sound(EXITMENUSOUND);
        return(-1);
    }
    else
    {
        if(onbar == 0) return(-probey-2);
        if ( KB_KeyPressed( sc_LeftArrow ) || KB_KeyPressed( sc_kpad_4 ) || ((buttonstat&1) && minfo.dyaw < -128 ) )
            return(probey);
        else if ( KB_KeyPressed( sc_RightArrow ) || KB_KeyPressed( sc_kpad_6 ) || ((buttonstat&1) && minfo.dyaw > 128 ) )
            return(probey);
        else return(-probey-2);
    }
}

int menutext(int x,int y,short s,short p,char *t)
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

int menutextc(int x,int y,short s,short p,char *t)
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


void bar(int x,int y,short *p,short dainc,char damodify,short s, short pa)
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
            if( KB_KeyPressed( sc_LeftArrow ) || KB_KeyPressed( sc_kpad_4 ) || ((buttonstat&1) && minfo.dyaw < -256 ) ) // && onbar) )
            {
                KB_ClearKeyDown( sc_LeftArrow );
                KB_ClearKeyDown( sc_kpad_4 );

                *p -= dainc;
                if(*p < 0)
                    *p = 0;
                sound(KICK_HIT);
            }
            if( KB_KeyPressed( sc_RightArrow ) || KB_KeyPressed( sc_kpad_6 ) || ((buttonstat&1) && minfo.dyaw > 256 ) )//&& onbar) )
            {
                KB_ClearKeyDown( sc_RightArrow );
                KB_ClearKeyDown( sc_kpad_6 );

                *p += dainc;
                if(*p > 63)
                    *p = 63;
                sound(KICK_HIT);
            }
        }
        else
        {
            if( KB_KeyPressed( sc_RightArrow ) || KB_KeyPressed( sc_kpad_6 ) || ((buttonstat&1) && minfo.dyaw > 256 ))//&& onbar ))
            {
                KB_ClearKeyDown( sc_RightArrow );
                KB_ClearKeyDown( sc_kpad_6 );

                *p -= dainc;
                if(*p < 0)
                    *p = 0;
                sound(KICK_HIT);
            }
            if( KB_KeyPressed( sc_LeftArrow ) || KB_KeyPressed( sc_kpad_4 ) || ((buttonstat&1) && minfo.dyaw < -256 ))// && onbar) )
            {
                KB_ClearKeyDown( sc_LeftArrow );
                KB_ClearKeyDown( sc_kpad_4 );

                *p += dainc;
                if(*p > 64)
                    *p = 64;
                sound(KICK_HIT);
            }
        }
    }

    xloc = *p;

    rotatesprite( (x+22)<<16,(y-3)<<16,65536L,0,SLIDEBAR,s,pa,10,0,0,xdim-1,ydim-1);
    if(rev == 0)
        rotatesprite( (x+xloc+1)<<16,(y+1)<<16,65536L,0,SLIDEBAR+1,s,pa,10,0,0,xdim-1,ydim-1);
    else
        rotatesprite( (x+(65-xloc) )<<16,(y+1)<<16,65536L,0,SLIDEBAR+1,s,pa,10,0,0,xdim-1,ydim-1);
}

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
	struct _directoryitem *d;

	while (filedirectory) {
		d = filedirectory->next;

		free(filedirectory);
		filedirectory = d;
	}

	while (dirdirectory) {
		d = dirdirectory->next;

		free(dirdirectory);
		dirdirectory = d;
	}

	filehighlight = 0;
	dirhighlight = 0;
	numfiles = numdirs = 0;
}

int getfilenames(char *path, char kind[])
{
	short type;
	struct Bdirent *fileinfo;
	BDIR *dir;
	struct _directoryitem *d;
	struct _directoryitem *directory;
	char buf[256], *p;
	long size;
	char *drives;

	if (Bstrcmp(kind,"SUBD") == 0) {
		type = 0;
		directory = dirdirectory;
	} else {
		type = 1;
		directory = filedirectory;
	}
	
#define newdirentry() \
				if (!d) { \
					directory = (struct _directoryitem *)Bmalloc(sizeof(struct _directoryitem)); \
					if (!directory) return 0; \
				 \
					d = directory; \
					if (type == 0) dirdirectory = d; \
					else filedirectory = d; \
					d->prev = NULL; \
				} else { \
					d->next = (struct _directoryitem *)Bmalloc(sizeof(struct _directoryitem)); \
					if (!d->next) return 0; \
				 \
					d->next->prev = d; \
					d = d->next; \
				}
	
	d = directory;
	if (d) while (d->next) d = d->next;

	if (!Bstrncasecmp(path,"GRP:",4)) {
		path += 4;	// ignore the GRP: prefix

		// searching groups
		if (type == 0) {
			// directories
			newdirentry();

			d->next = NULL;
			Bstrcpy(d->name, "..");
			numdirs++;

			goto dodrives;
		} else {
			// files
			beginsearchgroup(kind);
			while (getsearchgroupnext(buf,&size)) {
				//if (scandirectory(directory,buf)) continue;

				newdirentry();

				d->next = NULL;
				Bstrcpy(d->name, buf);
				d->size = size;
				memset(&d->mtime, 0, sizeof(struct tm));
				numfiles++;
			}
		}
		return 0;
	}
	
	Bstrcpy(buf, path);
	p = Bstrrchr(buf, '/');
	if (!p) p = Bstrrchr(buf, '\\');
	if (!p) return -1;

	p[1] = '.';
	p[2] = 0;

	if ((dir = Bopendir(buf)) == NULL) /*return -1;*/ goto dodrives;

	p[1] = 0;
	p++;

	while ((fileinfo = Breaddir(dir)) != NULL) {
		if (type == 1) {
			if (fileinfo->mode & BS_IFDIR) continue;
			if (!wildmatch(fileinfo->name, kind)) continue;
			//if ((p = Bstrrchr(fileinfo->name,'.')) == NULL) continue;
			//else if (Bstrcasecmp(p, kind)) continue;
		} else if ((type == 0) && !(fileinfo->mode & BS_IFDIR)) {
			continue;
		}

		if (!Bstrcmp(fileinfo->name, ".")) continue;
		if (fileinfo->namlen > 63) continue;	// too long

		newdirentry();

		d->next = NULL;
		Bstrcpy(d->name, fileinfo->name);
		if (type == 0) numdirs++;
		else {
			numfiles++;
			d->size = fileinfo->size;
			memcpy(&d->mtime, localtime((time_t*)&fileinfo->mtime), sizeof(struct tm));
		}
	}

	Bclosedir(dir);

	if (type != 0) return 0;

dodrives:	
	drives = p = Bgetsystemdrives();
	if (drives) {
		while (*p) {
			newdirentry();

			d->next = NULL;
			Bsprintf(d->name, "\xff[ %s ]", p);
			while (*p) p++; p++;
		}
		free(drives);
	}

	newdirentry();

	d->next = NULL;
	Bsprintf(d->name, "\xff< GRP: >");

#undef newdirentry
	
	return(0);
}

void sortfilenames(void)
{
	long i;
	struct _directoryitem *dira, *dirb;

	for (dira=dirdirectory; dira; dira=dira->next) {
		i=0;
		for (dirb=dirdirectory; dirb->next; dirb=dirb->next) {
			if (Bstrcasecmp(dirb->name, dirb->next->name) > 0) {
				swapbuf4(dirb->name, dirb->next->name, (64/4));
				i++;
			}
		}
		if (!i) break;
	}

	for (dira=filedirectory; dira; dira=dira->next) {
		i=0;
		for (dirb=filedirectory; dirb->next; dirb=dirb->next) {
			if (Bstrcasecmp(dirb->name, dirb->next->name) > 0) {
				swapbuf4(dirb->name, dirb->next->name, (64/4));
				swaplong(&dirb->size, &dirb->next->size);
				swapbuf4(&dirb->mtime, &dirb->next->mtime, (sizeof(struct tm)/4));
				i++;
			}
		}
		if (!i) break;
	}
}


long quittimer = 0;

void menus(void)
{
	struct _directoryitem *dir;
    short c,x,i;
    long l,m;
	char *p = NULL;

    getpackets();

    if(ControllerType == 1 && CONTROL_MousePresent)
    {
        if(buttonstat != 0 && !onbar)
        {
            x = MOUSE_GetButtons()<<3;
            if( x ) buttonstat = x<<3;
            else buttonstat = 0;
        }
        else
            buttonstat = MOUSE_GetButtons();
    }
    else buttonstat = 0;

    if( (ps[myconnectindex].gm&MODE_MENU) == 0 )
    {
        walock[MAXTILES-3] = 1;
        return;
    }

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
            gametext(160,77-8,"ARE MISSING OVER 75% OF THE TOTAL",0,2+8+16);
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

		case 20001:
        	rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
	        menutext(160,24,0,0,"NETWORK GAME");

			x = probe(160,100-18,18,3);

			if (x == -1) cmenu(0);
			else if (x == 2) cmenu(20010);
			else if (x == 1) cmenu(20020);
			else if (x == 0) cmenu(20002);
			
			menutext(160,100-18,0,0,"PLAYER SETUP");
			menutext(160,100,0,0,"JOIN GAME");
			menutext(160,100+18,0,0,"HOST GAME");
	    	break;

		case 20002:
		case 20003:
        	rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
	        menutext(160,24,0,0,"PLAYER SETUP");

			if (current_menu == 20002) {
				x = probe(46,50,20,2);

				if (x == -1) cmenu(20001);					
				else if (x == 0) {
					strcpy(buf, myname);
					inputloc = strlen(buf);
					current_menu = 20003;

					KB_ClearKeyDown(sc_Enter);
    	            KB_ClearKeyDown(sc_kpad_Enter);
					KB_FlushKeyboardQueue();
				} else if (x == 1) {
					// send colour update
				}
			} else {
				x = strget(40+100,50-9,buf,31,0);
				if (x) {
					if (x == 1) {
						strcpy(myname,buf);
						// send name update
					}

	                KB_ClearKeyDown(sc_Enter);
    	            KB_ClearKeyDown(sc_kpad_Enter);
        	        KB_FlushKeyboardQueue();

					current_menu = 20002;
				}
			}
			
			menutext(40,50,0,0,"NAME");
			if (current_menu == 20002) gametext(40+100,50-9,myname,0,2+8+16);

			menutext(40,50+20,0,0,"COLOR");
			rotatesprite((40+120)<<16,(50+20+(tilesizy[APLAYER]>>1))<<16,65536L,0,APLAYER,0,0,10,0,0,xdim-1,ydim-1);

			break;

		case 20010:
			rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
			menutext(160,24,0,0,"HOST NETWORK GAME");

			x = probe(46,50,80,2);

			if (x == -1) {
				cmenu(20001);
				probey = 2;
			}
			else if (x == 0) cmenu(20011);

			menutext(40,50,0,0,"GAME OPTIONS");
				minitext(90,60,            "GAME TYPE"    ,2,26);
				minitext(90,60+8,          "EPISODE"      ,2,26);
				minitext(90,60+8+8,        "LEVEL"        ,2,26);
				minitext(90,60+8+8+8,      "MONSTERS"     ,2,26);
				if (ud.m_coop == 0)
				minitext(90,60+8+8+8+8,    "MARKERS"      ,2,26);
				else if (ud.m_coop == 1)
				minitext(90,60+8+8+8+8,    "FRIENDLY FIRE",2,26);
				minitext(90,60+8+8+8+8+8,  "USER MAP"     ,2,26);

				if (ud.m_coop == 1)	minitext(90+60,60,"COOPERATIVE PLAY",0,26);
				else if (ud.m_coop == 2) minitext(90+60,60,"DUKEMATCH (NO SPAWN)",0,26);
				else minitext(90+60,60,"DUKEMATCH (SPAWN)",0,26);
				minitext(90+60,60+8,      volume_names[ud.m_volume_number],0,26);
				minitext(90+60,60+8+8,    level_names[11*ud.m_volume_number+ud.m_level_number],0,26);
				if (ud.m_monsters_off == 0 || ud.m_player_skill > 0)
				minitext(90+60,60+8+8+8,  skill_names[ud.m_player_skill],0,26);
				else minitext(90+60,60+8+8+8,  "NONE",0,28);
				if (ud.m_coop == 0) {
					if (ud.m_marker) minitext(90+60,60+8+8+8+8,"ON",0,26);
					else minitext(90+60,60+8+8+8+8,"OFF",0,26);
				} else if (ud.m_coop == 1) {
					if (ud.m_ffire) minitext(90+60,60+8+8+8+8,"ON",0,26);
					else minitext(90+60,60+8+8+8+8,"OFF",0,26);
				}
			
			menutext(40,50+80,0,0,"LAUNCH GAME");
			break;

		case 20011:
            c = (320>>1) - 120;
            rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,"NET GAME OPTIONS");

            x = probe(c,57-8,16,8);

            switch(x)
            {
                case -1:
                    cmenu(20010);
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
#ifndef ONELEVELDEMO
                    ud.m_level_number++;
if (!VOLUMEONE) {
                    if(ud.m_volume_number == 0 && ud.m_level_number > 6)
                        ud.m_level_number = 0;
} else {
                    if(ud.m_volume_number == 0 && ud.m_level_number > 5)
                        ud.m_level_number = 0;
}
                    if(ud.m_level_number > 10) ud.m_level_number = 0;
#endif
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
					// pick the user map
                    break;

                case 7:
					cmenu(20010);
                    break;
            }

            c += 40;

            if(ud.m_coop==1) gametext(c+70,57-7-9,"COOPERATIVE PLAY",0,2+8+16);
            else if(ud.m_coop==2) gametext(c+70,57-7-9,"DUKEMATCH (NO SPAWN)",0,2+8+16);
            else gametext(c+70,57-7-9,"DUKEMATCH (SPAWN)",0,2+8+16);

            gametext(c+70,57+16-7-9,volume_names[ud.m_volume_number],0,2+8+16);

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

            sprintf(tempbuf,"EPISODE %ld",ud.m_volume_number+1);
            menutext(c,57+16-9,SHX(-3),PHX(-3),tempbuf);

            sprintf(tempbuf,"LEVEL %ld",ud.m_level_number+1);
            menutext(c,57+16+16-9,SHX(-4),PHX(-4),tempbuf);
			
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

            menutext(c,57+16+16+16+16+16+16+16-9,SHX(-8),PHX(-8),"ACCEPT");
			break;

		case 20020:
		case 20021:	// editing server
		case 20022:	// editing port
			rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
			menutext(160,24,0,0,"JOIN NETWORK GAME");

			if (current_menu == 20020) {
				x = probe(46,50,20,3);

				if (x == -1) {
					cmenu(20001);
					probey = 1;
				} else if (x == 0) {
					strcpy(buf, "localhost");
					inputloc = strlen(buf);
					current_menu = 20021;
				} else if (x == 1) {
					strcpy(buf, "19014");
					inputloc = strlen(buf);
					current_menu = 20022;
				} else if (x == 2) {
				}
				KB_ClearKeyDown(sc_Enter);
                KB_ClearKeyDown(sc_kpad_Enter);
				KB_FlushKeyboardQueue();
			} else if (current_menu == 20021) {
				x = strget(40+100,50-9,buf,31,0);
				if (x) {
					if (x == 1) {
						//strcpy(myname,buf);
					}

	                KB_ClearKeyDown(sc_Enter);
    	            KB_ClearKeyDown(sc_kpad_Enter);
        	        KB_FlushKeyboardQueue();

					current_menu = 20020;
				}
			} else if (current_menu == 20022) {
				x = strget(40+100,50+20-9,buf,5,997);
				if (x) {
					if (x == 1) {
						//strcpy(myname,buf);
					}

	                KB_ClearKeyDown(sc_Enter);
    	            KB_ClearKeyDown(sc_kpad_Enter);
        	        KB_FlushKeyboardQueue();

					current_menu = 20020;
				}
			}
			
			menutext(40,50,0,0,"SERVER");
			if (current_menu != 20021) gametext(40+100,50-9,"server",0,2+8+16);

			menutext(40,50+20,0,0,"PORT");
			if (current_menu != 20022) {
				sprintf(tempbuf,"%d",19014);
				gametext(40+100,50+20-9,tempbuf,0,2+8+16);
			}

			menutext(160,50+20+20,0,0,"CONNECT");


			// ADDRESS
			// PORT
			// CONNECT
			break;

        case 15001:
        case 15000:

            gametext(160,90,"LOAD last game:",0,2+8+16);

            sprintf(tempbuf,"\"%s\"",ud.savegame[lastsavedpos]);
            gametext(160,99,tempbuf,0,2+8+16);

            gametext(160,99+9,"(Y/N)",0,2+8+16);

            if( KB_KeyPressed(sc_Escape) || KB_KeyPressed(sc_N) || RMB)
            {
                if(sprite[ps[myconnectindex].i].extra <= 0)
                {
                    enterlevel(MODE_GAME);
                    return;
                }

                KB_ClearKeyDown(sc_N);
                KB_ClearKeyDown(sc_Escape);

                ps[myconnectindex].gm &= ~MODE_MENU;
                if(ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
            }

            if(  KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB )
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

            c = 60;
            rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,"ADULT MODE");

            x = probe(60,50+16,16,2);
            if(x == -1) { cmenu(202); probey = 6; break; }

            menutext(c,50+16,SHX(-2),PHX(-2),"ADULT MODE");
            menutext(c,50+16+16,SHX(-3),PHX(-3),"ENTER PASSWORD");

            if(ud.lockout) menutext(c+160+40,50+16,0,0,"OFF");
            else menutext(c+160+40,50+16,0,0,"ON");

            if(current_menu == 10001)
            {
                gametext(160,50+16+16+16+16-12,"ENTER PASSWORD",0,2+8+16);
                x = strget((320>>1),50+16+16+16+16,buf,19, 998);

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
            rotatesprite(101<<16,97<<16,65536>>1,512,MAXTILES-3,-32,0,4+10+64,0,0,xdim-1,ydim-1);

            dispnames();

            sprintf(tempbuf,"PLAYERS: %-2ld                      ",savehead.numplr);
            gametext(160,156,tempbuf,0,2+8+16);

            sprintf(tempbuf,"EPISODE: %-2ld / LEVEL: %-2ld / SKILL: %-2ld",1+savehead.volnum,1+savehead.levnum,savehead.plrskl);
            gametext(160,168,tempbuf,0,2+8+16);

			if (savehead.volnum == 0 && savehead.levnum == 7)
				gametext(160,180,savehead.boardfn,0,2+8+16);
			
            gametext(160,90,"LOAD game:",0,2+8+16);
            sprintf(tempbuf,"\"%s\"",ud.savegame[current_menu-1000]);
            gametext(160,99,tempbuf,0,2+8+16);
            gametext(160,99+9,"(Y/N)",0,2+8+16);

            if( KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB )
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
            if( KB_KeyPressed(sc_N) || KB_KeyPressed(sc_Escape) || RMB)
            {
                KB_ClearKeyDown(sc_N);
                KB_ClearKeyDown(sc_Escape);
                sound(EXITMENUSOUND);
                if(ps[myconnectindex].gm&MODE_DEMO) cmenu(300);
                else
                {
                    ps[myconnectindex].gm &= ~MODE_MENU;
                    if(ud.multimode < 2 && ud.recstat != 2)
                    {
                        ready2send = 1;
                        totalclock = ototalclock;
                    }
                }
            }

            probe(186,124+9,0,0);

            break;

        case 1500:

            if( KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB )
            {
                KB_FlushKeyboardQueue();
                cmenu(100);
            }
            if( KB_KeyPressed(sc_N) || KB_KeyPressed(sc_Escape) || RMB)
            {
                KB_ClearKeyDown(sc_N);
                KB_ClearKeyDown(sc_Escape);
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

            rotatesprite(101<<16,97<<16,65536L>>1,512,MAXTILES-3,-32,0,4+10+64,0,0,xdim-1,ydim-1);
            sprintf(tempbuf,"PLAYERS: %-2ld                      ",ud.multimode);
            gametext(160,156,tempbuf,0,2+8+16);

            sprintf(tempbuf,"EPISODE: %-2ld / LEVEL: %-2ld / SKILL: %-2ld",1+ud.volume_number,1+ud.level_number,ud.player_skill);
            gametext(160,168,tempbuf,0,2+8+16);

			if (ud.volume_number == 0 && ud.level_number == 7)
				gametext(160,180,boardfilename,0,2+8+16);

		  	dispnames();

            gametext(160,90,"OVERWRITE previous SAVED game?",0,2+8+16);
            gametext(160,90+9,"(Y/N)",0,2+8+16);

            if( KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB )
            {
		inputloc = strlen(&ud.savegame[current_menu-2000][0]);

                cmenu(current_menu-2000+360);

                KB_FlushKeyboardQueue();
                break;
            }
            if( KB_KeyPressed(sc_N) || KB_KeyPressed(sc_Escape) || RMB)
            {
                KB_ClearKeyDown(sc_N);
                KB_ClearKeyDown(sc_Escape);
                cmenu(351);
                sound(EXITMENUSOUND);
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

            if(KB_KeyPressed(sc_Escape)) { cmenu(0); break; }

            if( KB_KeyPressed( sc_LeftArrow ) ||
                KB_KeyPressed( sc_kpad_4 ) ||
                KB_KeyPressed( sc_UpArrow ) ||
                KB_KeyPressed( sc_PgUp ) ||
                KB_KeyPressed( sc_kpad_8 ) )
            {
                KB_ClearKeyDown(sc_LeftArrow);
                KB_ClearKeyDown(sc_kpad_4);
                KB_ClearKeyDown(sc_UpArrow);
                KB_ClearKeyDown(sc_PgUp);
                KB_ClearKeyDown(sc_kpad_8);

                sound(KICK_HIT);
                current_menu--;
                if(current_menu < 990) current_menu = 990+l;
            }
            else if(
                KB_KeyPressed( sc_PgDn ) ||
                KB_KeyPressed( sc_Enter ) ||
                KB_KeyPressed( sc_Space ) ||
                KB_KeyPressed( sc_kpad_Enter ) ||
                KB_KeyPressed( sc_RightArrow ) ||
                KB_KeyPressed( sc_DownArrow ) ||
                KB_KeyPressed( sc_kpad_2 ) ||
                KB_KeyPressed( sc_kpad_9 ) ||
                KB_KeyPressed( sc_kpad_6 ) )
            {
                KB_ClearKeyDown(sc_PgDn);
                KB_ClearKeyDown(sc_Enter);
                KB_ClearKeyDown(sc_RightArrow);
                KB_ClearKeyDown(sc_kpad_Enter);
                KB_ClearKeyDown(sc_kpad_6);
                KB_ClearKeyDown(sc_kpad_9);
                KB_ClearKeyDown(sc_kpad_2);
                KB_ClearKeyDown(sc_DownArrow);
                KB_ClearKeyDown(sc_Space);
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
				case 993:	// JBF 20031220
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

				   p = "Icon by Lachlan \"NetNessie\" McDonald";
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
							"Richard \"TerminX\" Gobeille, Ben \"ProAsm\" Smit,",
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
							minitext(160-(Bstrlen(scroller[i])<<1), 100+10+(m*7)-l, (char*)scroller[i], 8, 10+16+128);
						}
				   }

				   for (i=0;i<2;i++) {
					   switch (i) {
						   case 0: p = "Visit http://jonof.edgenetwork.org/jfduke3d/ for"; break;
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
	    if (PLUTOPAK)	// JBF 20030804
            rotatesprite((c+100)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,(sintable[(totalclock<<4)&2047]>>11),0,2+8,0,0,xdim-1,ydim-1);
            x = probe(c,67,16,7);
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

			case 1: break;//cmenu(20001);break;	// JBF 20031128: I'm taking over the TEN menu option
			case 2: cmenu(202);break;	// JBF 20031205: was 200
                        case 3:
                            if(movesperpacket == 4 && connecthead != myconnectindex)
                                break;
                            cmenu(300);
                            break;
                        case 4: KB_FlushKeyboardQueue();cmenu(400);break;
                        case 5: cmenu(990);break;
                        case 6: cmenu(500);break;
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

	    menutext(c,67+16,0,1,"NETWORK GAME");

            menutext(c,67+16+16,SHX(-3),PHX(-3),"OPTIONS");

            if(movesperpacket == 4 && connecthead != myconnectindex)
                menutext(c,67+16+16+16,SHX(-4),1,"LOAD GAME");
            else menutext(c,67+16+16+16,SHX(-4),PHX(-4),"LOAD GAME");

if (!VOLUMEALL) {
            menutext(c,67+16+16+16+16,SHX(-5),PHX(-5),"HOW TO ORDER");
} else {
            menutext(c,67+16+16+16+16,SHX(-5),PHX(-5),"HELP");
}
            menutext(c,67+16+16+16+16+16,SHX(-6),PHX(-6),"CREDITS");

            menutext(c,67+16+16+16+16+16+16,SHX(-7),PHX(-7),"QUIT");
            break;

        case 50:
            c = (320>>1);
            rotatesprite(c<<16,32<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10,0,0,xdim-1,ydim-1);
	    if (PLUTOPAK)	// JBF 20030804
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
                    cmenu(202);		// JBF 20031205: was 200
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
	    if (boardfilename[0] == 0) {
		    strcpy(boardfilename, "./");
	    }
	    Bcorrectfilename(boardfilename,1);
            if(filedirectory == 0 && dirdirectory == 0)
            {
		clearfilenames();
                getfilenames(boardfilename,"SUBD");
                getfilenames(boardfilename,"*.MAP");
		filehighlight = filedirectory;
		dirhighlight = dirdirectory;
                sortfilenames();
		if (!filehighlight) currentlist = 0;
            }
	    cmenu(102);
		KB_FlushKeyboardQueue();
	case 102:
            rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,"SELECT A USER MAP");

	    // black translucent background underneath file lists
	    rotatesprite(0<<16, 0<<16, 65536l<<5, 0, BLANK, 0, 0, 10+16+1+32,
			    scale(40-4,xdim,320),scale(12+32-2,ydim,200),
			    scale(320-40+4,xdim,320)-1,scale(12+32+112+4,ydim,200)-1);

	    // path
	    minitext(52,32,boardfilename,17,26);

		{	// JBF 20040208: seek to first name matching pressed character
			struct _directoryitem *seeker = currentlist ? filedirectory : dirdirectory;
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
					if (currentlist) filehighlight = seeker;
					else dirhighlight = seeker;
					sound(KICK_HIT);
				}
			}
		}
	    
		gametext(40+4,12+32,"DIRECTORIES",0,2+8+16);
	    
	    if (dirhighlight) {
		dir = dirhighlight;
		for(i=0; i<2; i++) if (!dir->prev) break; else dir=dir->prev;
		for(i=2; i>-2 && dir; i--, dir=dir->next) {
		    if (dir == dirhighlight) c=0; else c=16;
		    minitext(40,1+12+32+8*(3-i),dir->name,c,26);
		}
	    }

		gametext(40+4,8+32+40+8-1,"MAP FILES",0,2+8+16);

	    if (filehighlight) {
		dir = filehighlight;
		for(i=0; i<4; i++) if (!dir->prev) break; else dir=dir->prev;
		for(i=4; i>-4 && dir; i--, dir=dir->next) {
			l = (boardfilename[0]=='G' && boardfilename[1]=='R' && boardfilename[2]=='P' && boardfilename[3]==':');
		    if (dir == filehighlight) c=21; else c=10;
		    minitext(40,(8+32+8*5)+8*(6-i),dir->name,c,26);
		    sprintf(tempbuf,"%ld",dir->size);
		    minitext(40+132,(8+32+8*5)+8*(6-i),tempbuf,c,26);
			if (l)
			sprintf(tempbuf,"(IN GROUP FILE)");
			else
		    sprintf(tempbuf,"%04d-%02d-%02d %02d:%02d:%02d",
				    1900+dir->mtime.tm_year,dir->mtime.tm_mon+1,dir->mtime.tm_mday,
				    dir->mtime.tm_hour,dir->mtime.tm_min,dir->mtime.tm_sec);
		    minitext(40+164,(8+32+8*5)+8*(6-i),tempbuf,l?20:c,26);
		}
	    }

            if( KB_KeyPressed( sc_LeftArrow ) || KB_KeyPressed( sc_kpad_4 ) || ((buttonstat&1) && minfo.dyaw < -256 ) ||
                KB_KeyPressed( sc_RightArrow ) || KB_KeyPressed( sc_kpad_6 ) || ((buttonstat&1) && minfo.dyaw > 256 ) ||
		KB_KeyPressed( sc_Tab ) )
            {
                KB_ClearKeyDown( sc_LeftArrow );
                KB_ClearKeyDown( sc_kpad_4 );
                KB_ClearKeyDown( sc_RightArrow );
                KB_ClearKeyDown( sc_kpad_6 );
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
			if (dirhighlight)
				if (dirhighlight->prev) dirhighlight = dirhighlight->prev;
		} else {
			if (filehighlight)
				if (filehighlight->prev) filehighlight = filehighlight->prev;
		}
	    } else if (probey == 0) {
		if (currentlist == 0) {
			if (dirhighlight)
				if (dirhighlight->next) dirhighlight = dirhighlight->next;
		} else {
			if (filehighlight)
				if (filehighlight->next) filehighlight = filehighlight->next;
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
		    if (!dirhighlight) break;
		    if (dirhighlight->name[0] == 0xff) {
			    strcpy(boardfilename, dirhighlight->name + 3);
			    boardfilename[strlen(boardfilename) - 2] = '/';
			    boardfilename[strlen(boardfilename) - 1] = 0;
		    } else {
				if (!Bstrcmp(dirhighlight->name, "..") && !Bstrcmp(boardfilename,"GRP:/"))
					Bstrcpy(boardfilename,"./");
				else {
				    strcat(boardfilename, dirhighlight->name);
				    strcat(boardfilename, "/");
				}
		    }
		    Bcorrectfilename(boardfilename, 1);
		    cmenu(101);
			KB_FlushKeyboardQueue();
		} else {
		    if (!filehighlight) break;
		    strcat(boardfilename, filehighlight->name);
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
                    clearview(0L);
                    nextpage();
                }

                newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);
                enterlevel(MODE_GAME);
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

        case 200:

            rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,15,0,0,"GAME OPTIONS");

            c = (320>>1)-120;

            onbar = (probey == 3);
            x = probe(c+6,31,15,9);

            if(x == -1)
		cmenu(202);	// JBF 20031205
                //{ if(ps[myconnectindex].gm&MODE_GAME) cmenu(50);else cmenu(0); }

            if(onbar == 0) switch(x)
            {
                case 0:
                    ud.detail = 1-ud.detail;
                    break;
                case 1:
                    ud.shadows = 1-ud.shadows;
                    break;
                case 2:
                    ud.screen_tilting = 1-ud.screen_tilting;
                    break;
				case 3:
					break;
				case 4:
			        ud.crosshair = 1-ud.crosshair;
					break;
				case 5:
					if (MouseAiming) break;
					myaimmode ^= 1;
					break;
                case 6:
                    ud.mouseflip = 1-ud.mouseflip;
                    break;
                case 7:
                    if( (ps[myconnectindex].gm&MODE_GAME) )
                    {
                        closedemowrite();
                        break;
                    }
                    ud.m_recstat = !ud.m_recstat;
                    break;
                case 8:
				    cmenu(201);
				    break;
            }

            if(ud.detail) menutext(c+160+40,31,0,0,"HIGH");
            else menutext(c+160+40,31,0,0,"LOW");

            if(ud.shadows) menutext(c+160+40,31+15,0,0,"ON");
            else menutext(c+160+40,31+15,0,0,"OFF");

            switch(ud.screen_tilting)
            {
                case 0: menutext(c+160+40,31+15+15,0,0,"OFF");break;
                case 1: menutext(c+160+40,31+15+15,0,0,"ON");break;
                case 2: menutext(c+160+40,31+15+15,0,0,"FULL");break;
            }

            menutext(c,31,SHX(-2),PHX(-2),"DETAIL");
            menutext(c,31+15,SHX(-3),PHX(-3),"SHADOWS");
            menutext(c,31+15+15,SHX(-4),PHX(-4),"SCREEN TILTING");
            menutext(c,31+15+15+15,SHX(-5),PHX(-5),"SCREEN SIZE");
                bar(c+167+40,31+15+15+15,(short *)&ud.screen_size,-4,x==3,SHX(-5),PHX(-5));

			menutext(c,31+15+15+15+15,0,0,"CROSSHAIR");
				menutext(c+160+40,31+15+15+15+15,0,0,ud.crosshair?"ON":"OFF");

			menutext(c,31+15+15+15+15+15,0,MouseAiming,"MOUSE AIMING");
				menutext(c+160+40,31+15+15+15+15+15,0,MouseAiming,myaimmode?"ON":"OFF");
				
            menutext(c,31+15+15+15+15+15+15,SHX(-7),PHX(-7),"INVERT MOUSE AIM");
                if(ud.mouseflip) menutext(c+160+40,31+15+15+15+15+15+15,SHX(-7),PHX(-7),"ON");
                else menutext(c+160+40,31+15+15+15+15+15+15,SHX(-7),PHX(-7),"OFF");

			if( (ps[myconnectindex].gm&MODE_GAME) && ud.m_recstat != 1 )
            {
                menutext(c,31+15+15+15+15+15+15+15,SHX(-10),1,"RECORD DEMO");
                menutext(c+160+40,31+15+15+15+15+15+15+15,SHX(-10),1,"OFF");
            }
            else
            {
                menutext(c,31+15+15+15+15+15+15+15,SHX(-10),PHX(-10),"RECORD DEMO");

                if(ud.m_recstat == 1)
                    menutext(c+160+40,31+15+15+15+15+15+15+15,SHX(-10),PHX(-10),"ON");
                else menutext(c+160+40,31+15+15+15+15+15+15+15,SHX(-10),PHX(-10),"OFF");
            }

				
            menutext(320>>1,31+15+15+15+15+15+15+15+15,SHX(-10),PHX(-10),"NEXT...");

			gametext(320-100,158,"Page 1 of 2",0,2+8+16);
            break;
	    
	    // JBF 20031129: Page 2 of game options
	case 201:
	
            rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,15,0,0,"GAME OPTIONS");

            c = (320>>1)-120;

            onbar = (probey==4);
            x = probe(c+6,31,15,6);

            if(x == -1)
		cmenu(202);	// JBF 20031205
                //{ if(ps[myconnectindex].gm&MODE_GAME) cmenu(50);else cmenu(0); }

            switch(x)
            {
		case 0:
		    // switch mouse aim type
		    if (ps[myconnectindex].gm&MODE_GAME || numplayers > 1) break;
		    				// don't change when in a multiplayer game
		    				// because the state is sent during getnames()
						// however, this will be fixed later
		    MouseAiming = 1-MouseAiming;
		    break;
                case 1:
                    // (en|dis)able autoaiming
		    if (ps[myconnectindex].gm&MODE_GAME || numplayers > 1) break;
		    				// don't change when in a multiplayer game
		    				// because the state is sent during getnames()
						// however, this will be fixed later
		    AutoAim = 1-AutoAim;
                    break;
                case 2:
                    ud.runkey_mode = 1-ud.runkey_mode;
                    break;
				case 3:
					ud.levelstats = !ud.levelstats;
					break;
				case 4: break;
                case 5:
		    cmenu(200);
                    break;
            }

            l = (ps[myconnectindex].gm&MODE_GAME || numplayers > 1);

	    menutext(c,31,0,l,"MOUSE AIM TYPE");
	    if (MouseAiming) menutext(c+160+40,31,0,l,"HELD");
            else menutext(c+160+40,31,0,l,"TOGGLE");
	    
            menutext(c,31+15,0,l,"AUTO-AIMING");
            if (!AutoAim) menutext(c+160+40,31+15,0,l,"OFF");
            else menutext(c+160+40,31+15,0,l,"ON");
	    
	    menutext(c,31+15+15,0,0,"RUN KEY STYLE");
            if (ud.runkey_mode) menutext(c+160+40,31+15+15,0,0,"CLASSIC");
            else menutext(c+160+40,31+15+15,0,0,"MODERN");

		menutext(c,31+15+15+15,0,0,"LEVEL STATS");
			if (ud.levelstats) menutext(c+160+40,31+15+15+15,0,0,"SHOWN");
			else menutext(c+160+40,31+15+15+15,0,0,"HIDDEN");

		menutext(c,31+15+15+15+15,0,0,"STATUSBAR SIZE");
			{
				short sbs, sbsl;
				sbs = sbsl = scale(max(0,ud.statusbarscale-50),63,100-50);
	            bar(c+167+40,31+15+15+15+15,(short *)&sbs,9,x==4,SHX(-5),PHX(-5));
				if (x == 4 && sbs != sbsl) {
					sbs = scale(sbs,100-50,63)+50;
					setstatusbarscale(sbs);
				}
			}
			
            menutext(320>>1,31+15+15+15+15+15,0,0,"PREVIOUS...");
			gametext(320-100,158,"Page 2 of 2",0,2+8+16);
	    break;

	    // JBF 20031205: Second level options menu selection
	case 202:
            rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"OPTIONS");

	    c = 200>>1;

            onbar = 0;
            x = probe(160,c-18-18-18,18,7);

	    switch (x) {
		case -1:
            if(ps[myconnectindex].gm&MODE_GAME) cmenu(50);else cmenu(0);
		    break;

		case 0:
		    cmenu(200);
		    break;

		case 1:
			cmenu(700);
			break;

		case 2:
			{
				long dax = xdim, day = ydim, daz;
				curvidmode = newvidmode = checkvideomode(&dax,&day,bpp,fullscreen);
				if (newvidmode == 0x7fffffffl) newvidmode = validmodecnt;
				newfullscreen = fullscreen;
				changesmade = 0;

				dax = 0;
				for (day = 0; day < validmodecnt; day++) {
					if (dax == sizeof(vidsets)/sizeof(vidsets[1])) break;
					for (daz = 0; daz < dax; daz++)
						if ((validmodebpp[day]|((validmodefs[day]&1)<<16)) == (vidsets[daz]&0x1ffffl)) break;
					if (vidsets[daz] != -1) continue;
					if (validmodebpp[day] == 8) {
						vidsets[dax++] = 8|((validmodefs[day]&1)<<16);
						vidsets[dax++] = 0x20000|8|((validmodefs[day]&1)<<16);
					} else
						vidsets[dax++] = 0x20000|validmodebpp[day]|((validmodefs[day]&1)<<16);
				}
				for (dax = 0; dax < (long)(sizeof(vidsets)/sizeof(vidsets[1])) && vidsets[dax] != -1; dax++)
					if (vidsets[dax] == (((getrendermode()>=2)<<17)|(fullscreen<<16)|bpp)) break;
				if (dax < (long)(sizeof(vidsets)/sizeof(vidsets[1]))) newvidset = dax;
				curvidset = newvidset;
				
				cmenu(203);
			}
		    break;
		    
		case 3:
		    currentlist = 0;
		case 4:
		case 5:
			if (x==5 && (!CONTROL_JoystickEnabled || !CONTROL_JoyPresent)) break;
		    cmenu(204+x-3);
		    break;

		case 6:
#ifndef AUSTRALIA
            cmenu(10000);
#endif
			break;
	    }

	    menutext(160,c-18-18-18,0,0,"GAME OPTIONS");
		menutext(160,c-18-18,   0,0,"SOUND OPTIONS");
	    menutext(160,c-18,      0,0,"VIDEO SETTINGS");
	    menutext(160,c,         0,0,"KEYS SETUP");
	    menutext(160,c+18,      0,0,"MOUSE SETUP");
		menutext(160,c+18+18,   0,CONTROL_JoyPresent==0 || CONTROL_JoystickEnabled==0,"JOYSTICK SETUP");
#ifndef AUSTRALIA
		menutext(160,c+18+18+18,0,0,"PARENTAL LOCK");
#else
		menutext(160,c+18+18+18,0,1,"PARENTAL LOCK");
#endif
	    break;

	    // JBF 20031206: Video settings menu
	case 203:
            rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"VIDEO SETTINGS");

	    c = (320>>1)-120;

#if defined(POLYMOST) && defined(USE_OPENGL)
		x = 7;
#else
		x = 5;
#endif
	    onbar = (probey == 4);
		if (probey == 0 || probey == 1 || probey == 2)
		    x = probe(c+6,50,16,x);
		else if (probey == 3)
			x = probe(c+6,50+16+16+22,0,x);
		else
			x = probe(c+6,50+62-16-16-16,16,x);

		if (probey==0 && (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_RightArrow))) {
			sound(PISTOL_BODYHIT);
			x=0;
		}
	    switch (x) {
		case -1:
		    cmenu(202);
		    probey = 2;
		    break;

		case 0:
		    do {
				if (KB_KeyPressed(sc_LeftArrow)) {
					newvidmode--;
					if (newvidmode < 0) newvidmode = validmodecnt-1;
				} else {
			        newvidmode++;
			        if (newvidmode >= validmodecnt) newvidmode = 0;
				}
		    } while ((validmodefs[newvidmode]&1) != ((vidsets[newvidset]>>16)&1) || validmodebpp[newvidmode] != (vidsets[newvidset] & 0x0ffff));
			//OSD_Printf("New mode is %dx%dx%d-%d %d\n",validmodexdim[newvidmode],validmodeydim[newvidmode],validmodebpp[newvidmode],validmodefs[newvidmode],newvidmode);
		    if ((curvidmode == 0x7fffffffl && newvidmode == validmodecnt) || curvidmode == newvidmode)
				changesmade &= ~1;
		    else
		        changesmade |= 1;
			KB_ClearKeyDown(sc_LeftArrow);
			KB_ClearKeyDown(sc_RightArrow);
		    break;

		case 1:
			{
				int lastvidset, lastvidmode, safevidmode = -1;
				lastvidset = newvidset;
				lastvidmode = newvidmode;
				// find the next vidset compatible with the current fullscreen setting
				while (vidsets[0] != -1) {
					newvidset++;
					if (newvidset == sizeof(vidsets)/sizeof(vidsets[0]) || vidsets[newvidset] == -1) { newvidset = -1; continue; }
					if (((vidsets[newvidset]>>16)&1) != newfullscreen) continue;
					break;
				}

				if ((vidsets[newvidset] & 0x0ffff) != (vidsets[lastvidset] & 0x0ffff)) {
					// adjust the video mode to something legal for the new vidset
					do {
						newvidmode++;
						if (newvidmode == lastvidmode) break;	// end of cycle
						if (newvidmode >= validmodecnt) newvidmode = 0;
						if (validmodebpp[newvidmode] == (vidsets[newvidset]&0x0ffff) &&
							validmodefs[newvidmode] == newfullscreen &&
							validmodexdim[newvidmode] <= validmodexdim[lastvidmode] &&
							   (safevidmode==-1?1:(validmodexdim[newvidmode]>=validmodexdim[safevidmode])) &&
							validmodeydim[newvidmode] <= validmodeydim[lastvidmode] &&
							   (safevidmode==-1?1:(validmodeydim[newvidmode]>=validmodeydim[safevidmode]))
							)
							safevidmode = newvidmode;
					} while (1);
					if (safevidmode == -1) {
						//OSD_Printf("No best fit!\n");
						newvidmode = lastvidmode;
						newvidset = lastvidset;
					} else {
						//OSD_Printf("Best fit is %dx%dx%d-%d %d\n",validmodexdim[safevidmode],validmodeydim[safevidmode],validmodebpp[safevidmode],validmodefs[safevidmode],safevidmode);
						newvidmode = safevidmode;
					}
				}
				if (newvidset != curvidset) changesmade |= 4; else changesmade &= ~4;
				if (newvidmode != curvidmode) changesmade |= 1; else changesmade &= ~1;
			}
			break;

		case 2:
		    newfullscreen = !newfullscreen;
			{
				int lastvidset, lastvidmode, safevidmode = -1, safevidset = -1;
				lastvidset = newvidset;
				lastvidmode = newvidmode;
				// find the next vidset compatible with the current fullscreen setting
				while (vidsets[0] != -1) {
					newvidset++;
					if (newvidset == lastvidset) break;
					if (newvidset == sizeof(vidsets)/sizeof(vidsets[0]) || vidsets[newvidset] == -1) { newvidset = -1; continue; }
					if (((vidsets[newvidset]>>16)&1) != newfullscreen) continue;
					if ((vidsets[newvidset] & 0x2ffff) != (vidsets[lastvidset] & 0x2ffff)) {
						if ((vidsets[newvidset] & 0x20000) == (vidsets[lastvidset] & 0x20000)) safevidset = newvidset;
						continue;
					}
					break;
				}
				if (newvidset == lastvidset) {
					if (safevidset == -1) {
						newfullscreen = !newfullscreen;
						break;
					} else {
						newvidset = safevidset;
					}
				}

				// adjust the video mode to something legal for the new vidset
				do {
					newvidmode++;
					if (newvidmode == lastvidmode) break;	// end of cycle
					if (newvidmode >= validmodecnt) newvidmode = 0;
					if (validmodebpp[newvidmode] == (vidsets[newvidset]&0x0ffff) &&
						validmodefs[newvidmode] == newfullscreen &&
						validmodexdim[newvidmode] <= validmodexdim[lastvidmode] &&
						   (safevidmode==-1?1:(validmodexdim[newvidmode]>=validmodexdim[safevidmode])) &&
						validmodeydim[newvidmode] <= validmodeydim[lastvidmode] &&
						   (safevidmode==-1?1:(validmodeydim[newvidmode]>=validmodeydim[safevidmode]))
						)
						safevidmode = newvidmode;
				} while (1);
				if (safevidmode == -1) {
					//OSD_Printf("No best fit!\n");
					newvidmode = lastvidmode;
					newvidset = lastvidset;
					newfullscreen = !newfullscreen;
				} else {
					//OSD_Printf("Best fit is %dx%dx%d-%d %d\n",validmodexdim[safevidmode],validmodeydim[safevidmode],validmodebpp[safevidmode],validmodefs[safevidmode],safevidmode);
					newvidmode = safevidmode;
				}
				if (newvidset != curvidset) changesmade |= 4; else changesmade &= ~4;
				if (newvidmode != curvidmode) changesmade |= 1; else changesmade &= ~1;
			}
		    if (newfullscreen == fullscreen) changesmade &= ~2; else changesmade |= 2;
		    break;

		case 3:
		    if (!changesmade) break;
			{
				long pxdim, pydim, pfs, pbpp, prend;
				long nxdim, nydim, nfs, nbpp, nrend;

				pxdim = xdim; pydim = ydim; pbpp = bpp; pfs = fullscreen; prend = getrendermode();
				nxdim = (newvidmode==validmodecnt)?xdim:validmodexdim[newvidmode];
				nydim = (newvidmode==validmodecnt)?ydim:validmodeydim[newvidmode];
				nfs   = newfullscreen;
				nbpp  = (newvidmode==validmodecnt)?bpp:validmodebpp[newvidmode];
				nrend = (vidsets[newvidset] & 0x20000) ? (nbpp==8?2:3) : 0;

				if (setgamemode(nfs, nxdim, nydim, nbpp) < 0) {
		    		if (setgamemode(pfs, pxdim, pydim, pbpp) < 0) {
						setrendermode(prend);
						gameexit("Failed restoring old video mode.");
					} else onvideomodechange(pbpp > 8);
				} else onvideomodechange(nbpp > 8);
				vscrn();
				setrendermode(nrend);

			    curvidmode = newvidmode; curvidset = newvidset;
			    changesmade = 0;

			    ScreenMode = fullscreen;
			    ScreenWidth = xdim;
		    	ScreenHeight = ydim;
				ScreenBPP = bpp;
			}
		    break;

		case 4:
			break;

#if defined(POLYMOST) && defined(USE_OPENGL)
		case 5:
			if (bpp==8) break;
			switch (gltexfiltermode) {
				case 0: gltexfiltermode = 3; break;
				case 3: gltexfiltermode = 5; break;
				case 5: gltexfiltermode = 0; break;
				default: gltexfiltermode = 3; break;
			}
			gltexapplyprops();
			break;

		case 6:
			if (bpp==8) break;
			glanisotropy *= 2;
			if (glanisotropy > glinfo.maxanisotropy) glanisotropy = 1;
			gltexapplyprops();
			break;
#endif
	    }

	    menutext(c,50,0,0,"RESOLUTION");
	    sprintf(tempbuf,"%ld x %ld",
			    (newvidmode==validmodecnt)?xdim:validmodexdim[newvidmode],
			    (newvidmode==validmodecnt)?ydim:validmodeydim[newvidmode]);
            gametext(c+154,50-8,tempbuf,0,2+8+16);
	    
	    menutext(c,50+16,0,0,"VIDEO MODE");
			sprintf(tempbuf, "%dbit %s", vidsets[newvidset]&0x0ffff, (vidsets[newvidset]&0x20000)?"Polymost":"Classic");
			gametext(c+154,50+16-8,tempbuf,0,2+8+16);

	    menutext(c,50+16+16,0,0,"FULLSCREEN");
            menutext(c+154,50+16+16,0,0,newfullscreen?"YES":"NO");

	    menutext(c+16,50+16+16+22,0,changesmade==0,"APPLY CHANGES");
	    
        menutext(c,50+62+16,SHX(-6),PHX(-6),"BRIGHTNESS");
            bar(c+167,50+62+16,(short *)&ud.brightness,8,x==4,SHX(-6),PHX(-6));
            if(x==4) setbrightness(ud.brightness>>2,&ps[myconnectindex].palette[0],0);

#if defined(POLYMOST) && defined(USE_OPENGL)
		menutext(c,50+62+16+16,0,bpp==8,"FILTERING");
			switch (gltexfiltermode) {
				case 0: strcpy(tempbuf,"NEAREST"); break;
				case 3: strcpy(tempbuf,"BILINEAR"); break;
				case 5: strcpy(tempbuf,"TRILINEAR"); break;
				default: strcpy(tempbuf,"OTHER"); break;
			}
			menutext(c+154,50+62+16+16,0,bpp==8,tempbuf);

		menutext(c,50+62+16+16+16,0,bpp==8,"ANISOTROPY");
			if (glanisotropy == 1) strcpy(tempbuf,"NONE");
			else sprintf(tempbuf,"%ld-tap",glanisotropy);
			menutext(c+154,50+62+16+16+16,0,bpp==8,tempbuf);
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
			probey = 3;
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
	    else if (KB_KeyPressed( sc_LeftArrow ) || KB_KeyPressed( sc_kpad_4 ) ||
		     KB_KeyPressed( sc_RightArrow ) || KB_KeyPressed( sc_kpad_6 ) ||
		     KB_KeyPressed( sc_Tab )) {
			currentlist ^= 1;
			KB_ClearKeyDown( sc_LeftArrow );
			KB_ClearKeyDown( sc_RightArrow );
			KB_ClearKeyDown( sc_kpad_4 );
			KB_ClearKeyDown( sc_kpad_6 );
			KB_ClearKeyDown( sc_Tab );
			sound(KICK_HIT);
	    } else if (KB_KeyPressed( sc_Delete )) {
			KeyboardKeys[probey][currentlist] = 0;
			CONTROL_MapKey( probey, KeyboardKeys[probey][0], KeyboardKeys[probey][1] );
			sound(KICK_HIT);
			KB_ClearKeyDown( sc_Delete );
	    }
	    
	    for (l=0; l < min(13,NUMGAMEFUNCTIONS); l++) {
			p = CONFIG_FunctionNumToName(m+l);
			if (!p) continue;

			strcpy(tempbuf, p);
			for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
			minitext(70,30+l*8,tempbuf,(m+l == probey)?0:16,10+16);

		    //strcpy(tempbuf, KB_ScanCodeToString(KeyboardKeys[m+l][0]));
			strcpy(tempbuf, getkeyname(KeyboardKeys[m+l][0]));
		    if (!tempbuf[0]) strcpy(tempbuf, "  -");
		    minitext(70+100,30+l*8,tempbuf,
				    (m+l == probey && !currentlist?21:10),10+16);

		    //strcpy(tempbuf, KB_ScanCodeToString(KeyboardKeys[m+l][1]));
			strcpy(tempbuf, getkeyname(KeyboardKeys[m+l][1]));
		    if (!tempbuf[0]) strcpy(tempbuf, "  -");
		    minitext(70+120+34,30+l*8,tempbuf,
				    (m+l == probey && currentlist?21:10),10+16);
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
	    sprintf(tempbuf,"%s FOR \"%s\"", whichkey?"SECONDARY":"PRIMARY", CONFIG_FunctionNumToName(function));
	    gametext(320>>1,90+9,tempbuf,0,2+8+16);
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
		    probey = 4;
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
			tempbuf[0] = 0;
			if (l < (MAXMOUSEBUTTONS-2)*2) {
				if (l&1) {
					Bstrcpy(tempbuf, "Double ");
					m = MouseFunctions[l>>1][1];
				} else
					m = MouseFunctions[l>>1][0];
				Bstrcat(tempbuf, mousebuttonnames[l>>1]);
			} else {
				Bstrcpy(tempbuf, mousebuttonnames[l-(MAXMOUSEBUTTONS-2)]);
				m = MouseFunctions[l-(MAXMOUSEBUTTONS-2)][0];
			}

			minitext(c+20,30+l*8,tempbuf,(l==probey)?0:16,10+16);

			if (m == -1)
				minitext(c+100+20,30+l*8,"  -NONE-",(l==probey)?0:16,10+16);
			else {
				strcpy(tempbuf, CONFIG_FunctionNumToName(m));
			    for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
				minitext(c+100+20,30+l*8,tempbuf,(l==probey)?0:16,10+16);
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
			else if (function == 2) menutext(320>>1,15,0,0,"JOYSTICK BUTTONS");
			else if (function == 3) menutext(320>>1,15,0,0,"JOYSTICK AXES");
	
		x = probe(0,0,0,NUMGAMEFUNCTIONS);

		if (x==-1) {
			if (function == 0) {	// mouse button
				cmenu(205);
				probey = whichkey;
			} else if (function == 1) {	// mouse digital axis
				cmenu(212);
				probey = 2+(whichkey^2);
			} else if (function == 2) {	// joystick button/hat
				cmenu(207);
				probey = whichkey;
			} else if (function == 3) {	// joystick digital axis
				cmenu((whichkey>>2)+208);
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
				if (whichkey < 2*joynumbuttons) {
					JoystickFunctions[whichkey>>1][whichkey&1] = x;
					CONTROL_MapButton( x, whichkey>>1, whichkey&1, controldevice_joystick);
				} else {
					JoystickFunctions[MAXJOYBUTTONS-4+(whichkey-2*joynumbuttons)][0] = x;
					CONTROL_MapButton( x, MAXJOYBUTTONS-4+(whichkey-2*joynumbuttons), 0, controldevice_joystick);
				}
				cmenu(207);
				probey = whichkey;
			} else if (function == 3) {
				JoystickDigitalFunctions[whichkey>>1][whichkey&1] = x;
				CONTROL_MapDigitalAxis(whichkey>>1, x, whichkey&1, controldevice_joystick);
				cmenu((whichkey>>2)+208);
				probey = 1+((whichkey>>1)&1)*4+(whichkey&1);
			}
			break;
		}

	    gametext(320>>1,25,"SELECT A FUNCTION TO ASSIGN",0,2+8+16);

		if (function == 0) {
			if (whichkey < (MAXMOUSEBUTTONS-2)*2)
			    sprintf(tempbuf,"TO %s%s", (whichkey&1)?"DOUBLE-CLICKED ":"", mousebuttonnames[whichkey>>1]);
			else
				Bstrcpy(tempbuf, mousebuttonnames[whichkey-(MAXMOUSEBUTTONS-2)]);
		} else if (function == 1) {
			Bstrcpy(tempbuf,"TO DIGITAL ");
			switch (whichkey) {
				case 0: Bstrcat(tempbuf, "LEFT"); break;
				case 1: Bstrcat(tempbuf, "RIGHT"); break;
				case 2: Bstrcat(tempbuf, "UP"); break;
				case 3: Bstrcat(tempbuf, "DOWN"); break;
			}
		} else if (function == 2) {
			static char *directions[] = { "UP", "RIGHT", "DOWN", "LEFT" };
			if (whichkey < 2*joynumbuttons)
				Bsprintf(tempbuf,"TO %s%s", (whichkey&1)?"DOUBLE-CLICKED ":"", getjoyname(1,whichkey>>1));
			else
				Bsprintf(tempbuf,"TO HAT %s", directions[whichkey-2*joynumbuttons]);
		} else if (function == 3) {
			Bsprintf(tempbuf,"TO DIGITAL %s %s",getjoyname(0,whichkey>>1),(whichkey&1)?"POSITIVE":"NEGATIVE");
		}

	    gametext(320>>1,25+9,tempbuf,0,2+8+16);

		if (KB_KeyPressed( sc_End )) { KB_ClearKeyDown(sc_End); probey = NUMGAMEFUNCTIONS-1; sound(KICK_HIT); }
		else if (KB_KeyPressed( sc_Home )) { KB_ClearKeyDown(sc_Home); probey = 0; sound(KICK_HIT); }
		
        m = probey - 6;
	    if (m < 0) m = 0;
	    else if (m + 13 >= NUMGAMEFUNCTIONS) m = NUMGAMEFUNCTIONS-13;

	    for (l=0; l < min(13,NUMGAMEFUNCTIONS); l++) {
			if (l+m == NUMGAMEFUNCTIONS-1)
				strcpy(tempbuf, "  -NONE-");
			else
			    strcpy(tempbuf, CONFIG_FunctionNumToName(m+l));

		    for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
            minitext(100,46+l*8,tempbuf,(m+l == probey)?0:16,10+16);
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
				whichkey = (x-2)^2;	// flip the actual axis number
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
				break;

		}

		menutext(c,40,0,0,"X-AXIS SCALE");
		l = (MouseAnalogueScale[0]+262144) >> 13;
		bar(c+160+40,40,(short *)&l,1,x==0,0,0);
		l = (l<<13)-262144;
		if (l != MouseAnalogueScale[0]) {
			CONTROL_SetAnalogAxisScale( 0, l, controldevice_mouse );
			MouseAnalogueScale[0] = l;
		}
		Bsprintf(tempbuf,"%s%.2f",l>=0?" ":"",(float)l/65536.0);
		gametext(c+160-16,40-8,tempbuf,0,2+8+16);
		
		menutext(c,40+16,0,0,"Y-AXIS SCALE");
		l = (MouseAnalogueScale[1]+262144) >> 13;
		bar(c+160+40,40+16,(short *)&l,1,x==1,0,0);
		l = (l<<13)-262144;
		if (l != MouseAnalogueScale[1]) {
			CONTROL_SetAnalogAxisScale( 1, l, controldevice_mouse );
			MouseAnalogueScale[1] = l;
		}
		Bsprintf(tempbuf,"%s%.2f",l>=0?" ":"",(float)l/65536.0);
		gametext(c+160-16,40+16-8,tempbuf,0,2+8+16);
		
		menutext(c,40+16+16+8,0,0,"DIGITAL AXES ACTIONS");
		
		gametext(c+10,84,"UP:",0,2+8+16);
			if (MouseDigitalFunctions[1][0] < 0)
				strcpy(tempbuf, "  -NONE-");
			else
			    strcpy(tempbuf, CONFIG_FunctionNumToName(MouseDigitalFunctions[1][0]));

		    for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
            minitext(c+10+60,85,tempbuf,0,10+16);
		
		gametext(c+10,84+10,"DOWN:",0,2+8+16);
			if (MouseDigitalFunctions[1][1] < 0)
				strcpy(tempbuf, "  -NONE-");
			else
			    strcpy(tempbuf, CONFIG_FunctionNumToName(MouseDigitalFunctions[1][1]));

		    for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
            minitext(c+10+60,85+10,tempbuf,0,10+16);

		gametext(c+10,84+10+10,"LEFT:",0,2+8+16);
			if (MouseDigitalFunctions[0][0] < 0)
				strcpy(tempbuf, "  -NONE-");
			else
			    strcpy(tempbuf, CONFIG_FunctionNumToName(MouseDigitalFunctions[0][0]));

		    for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
            minitext(c+10+60,85+10+10,tempbuf,0,10+16);

		gametext(c+10,84+10+10+10,"RIGHT:",0,2+8+16);
			if (MouseDigitalFunctions[0][1] < 0)
				strcpy(tempbuf, "  -NONE-");
			else
			    strcpy(tempbuf, CONFIG_FunctionNumToName(MouseDigitalFunctions[0][1]));

		    for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
            minitext(c+10+60,85+10+10+10,tempbuf,0,10+16);

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
            menutext(320>>1,24,0,0,"JOYSTICK SETUP");

		x = probe(160,100-18,18,3);

	    switch (x) {
			case -1:
		    	cmenu(202);
			    probey = 5;
			    break;
			case 0:
			case 1:
				cmenu(207+x);
				break;
			case 2:
				cmenu(213);
				break;
		}

		menutext(160,100-18,0,0,"EDIT BUTTONS");
		menutext(160,100,0,0,"EDIT AXES");
		menutext(160,100+18,0,0,"DEAD ZONES");

		break;

	case 207:
         rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,15,0,0,"JOYSTICK BUTTONS");

		c = 2*joynumbuttons + 4*(joynumhats>0);

		x = probe(0,0,0,c);
			
	    if (x == -1) {
	    	cmenu(206);
		    probey = 0;
		    break;
		} else if (x >= 0) {
			function = 2;
			whichkey = x;
			cmenu(211);
			if (x < 2*joynumbuttons) {
				probey = JoystickFunctions[x>>1][x&1];
			} else {
				probey = JoystickFunctions[MAXJOYBUTTONS-4+x-2*joynumbuttons][0];
			}
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
			if (m+l < 2*joynumbuttons) {
				sprintf(tempbuf, "%s%s", ((l+m)&1)?"Double ":"", getjoyname(1,(l+m)>>1));
				x = JoystickFunctions[(l+m)>>1][(l+m)&1];
			} else {
				static char *directions[] = { "Up", "Right", "Down", "Left" };
				sprintf(tempbuf, "Hat %s", directions[(l+m)-2*joynumbuttons]);
				x = JoystickFunctions[MAXJOYBUTTONS-4+(l+m)-2*joynumbuttons][0];
			}
			minitext(80-4,33+l*8,tempbuf,(m+l == probey)?0:16,10+16);

			if (x == -1)
				minitext(176,33+l*8,"  -NONE-",(m+l==probey)?0:16,10+16);
			else {
				strcpy(tempbuf, CONFIG_FunctionNumToName(x));
			    for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
				minitext(176,33+l*8,tempbuf,(m+l==probey)?0:16,10+16);
			}
		}
		
	    gametext(160,149,"UP/DOWN = SELECT BUTTON",0,2+8+16);
	    gametext(160,149+9,"ENTER = MODIFY",0,2+8+16);
		break;

	case 208:
	case 209:
	case 217:
	case 218:
	case 219:
	case 220:
	case 221:
	case 222: {
		int thispage, twothispage;
         rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,15,0,0,"JOYSTICK AXES");

		thispage = (current_menu < 217) ? (current_menu-208) : (current_menu-217)+2;
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
					if (thispage == ((joynumaxes+1)/2)-1) cmenu(208);
					else {
						if (current_menu == 209) cmenu(217);
						else cmenu( current_menu+1 );
					}
				}
				break;

			case 4: // bar
				if (!twothispage && joynumaxes > 2)
					cmenu(208);
			case 0: break;

			case 1:	// digitals
			case 2:
			case 5:
			case 6:
				function = 3;
				whichkey = ((thispage*2+(x==5||x==6)) << 1) + (x==2||x==6);
				cmenu(211);
				probey = JoystickDigitalFunctions[whichkey>>1][whichkey&1];
				if (probey < 0) probey = NUMGAMEFUNCTIONS-1;
				break;

			case 3:	// analogues
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
		l = (JoystickAnalogueScale[thispage*2]+262144) >> 13;
		bar(140+56,38+8,(short *)&l,1,x==0,0,0);
		l = (l<<13)-262144;
		if (l != JoystickAnalogueScale[thispage*2]) {
			CONTROL_SetAnalogAxisScale( thispage*2, l, controldevice_joystick );
			JoystickAnalogueScale[thispage*2] = l;
		}
		Bsprintf(tempbuf,"%s%.2f",l>=0?" ":"",(float)l/65536.0);
		gametext(140,38,tempbuf,0,2+8+16);

		gametext(76,38+15,"DIGITAL",0,2+8+16);
			if (JoystickDigitalFunctions[thispage*2][0] < 0)
				strcpy(tempbuf, "  -NONE-");
			else
			    strcpy(tempbuf, CONFIG_FunctionNumToName(JoystickDigitalFunctions[thispage*2][0]));

		    for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
            minitext(140+12,38+15,tempbuf,0,10+16);

			if (JoystickDigitalFunctions[thispage*2][1] < 0)
				strcpy(tempbuf, "  -NONE-");
			else
			    strcpy(tempbuf, CONFIG_FunctionNumToName(JoystickDigitalFunctions[thispage*2][1]));

		    for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
            minitext(140+12+72,38+15,tempbuf,0,10+16);
			
		gametext(76,38+15+15,"ANALOG",0,2+8+16);
		if (CONFIG_AnalogNumToName( JoystickAnalogueAxes[thispage*2] )) {
			p = CONFIG_AnalogNumToName( JoystickAnalogueAxes[thispage*2] );
			if (p) {
				gametext(140+12,38+15+15, strchr(p,'_')+1, 0, 2+8+16 );
			}
		}
		
		if (twothispage) {
			gametext(76,38+64,"SCALE",0,2+8+16);
			l = (JoystickAnalogueScale[thispage*2+1]+262144) >> 13;
			bar(140+56,38+8+64,(short *)&l,1,x==4,0,0);
			l = (l<<13)-262144;
			if (l != JoystickAnalogueScale[thispage*2+1]) {
				CONTROL_SetAnalogAxisScale( thispage*2+1, l, controldevice_joystick );
				JoystickAnalogueScale[thispage*2+1] = l;
			}
			Bsprintf(tempbuf,"%s%.2f",l>=0?" ":"",(float)l/65536.0);
			gametext(140,38+64,tempbuf,0,2+8+16);

			gametext(76,38+64+15,"DIGITAL",0,2+8+16);
				if (JoystickDigitalFunctions[thispage*2+1][0] < 0)
					strcpy(tempbuf, "  -NONE-");
				else
					strcpy(tempbuf, CONFIG_FunctionNumToName(JoystickDigitalFunctions[thispage*2+1][0]));

				for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
				minitext(140+12,38+15+64,tempbuf,0,10+16);

				if (JoystickDigitalFunctions[thispage*2+1][1] < 0)
					strcpy(tempbuf, "  -NONE-");
				else
					strcpy(tempbuf, CONFIG_FunctionNumToName(JoystickDigitalFunctions[thispage*2+1][1]));

				for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
				minitext(140+12+72,38+15+64,tempbuf,0,10+16);
			
			gametext(76,38+64+15+15,"ANALOG",0,2+8+16);
			if (CONFIG_AnalogNumToName( JoystickAnalogueAxes[thispage*2+1] )) {
				p = CONFIG_AnalogNumToName( JoystickAnalogueAxes[thispage*2+1] );
				if (p) {
					gametext(140+12,38+64+15+15, strchr(p,'_')+1, 0, 2+8+16 );
				}
			}
		}
		
		if (joynumaxes > 2) {
		    menutext(320>>1,twothispage?158:108,SHX(-10),(joynumaxes<=2),"NEXT...");
			sprintf(tempbuf,"Page %d of %d",thispage+1,(joynumaxes+1)/2);
			gametext(320-100,158,tempbuf,0,2+8+16);
		}
		break;
	}

	case 213:
	case 214:
	case 215:
	case 216: {	// Pray this is enough pages for now :-|
		int first,last;
         rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"JOY DEAD ZONES");

			first = 4*(current_menu-213);
			last  = min(4*(current_menu-213)+4,joynumaxes);

			onbar = 1;
			x = probe(320,48,15,2*(last-first)+(joynumaxes>4));

			if (x==-1) {
				cmenu(206);
				probey = 2;
				break;
			} else if (x==2*(last-first) && joynumaxes>4) {
				cmenu( (current_menu-213) == (joynumaxes/4) ? 213 : (current_menu+1) );
				probey = 0;
				break;
			}

			for (m=first;m<last;m++) {
				unsigned short odx,dx,ody,dy;
				menutext(32,48+30*(m-first),0,0,getjoyname(0,m));

				gametext(128,48+30*(m-first)-8,"DEAD",0,2+8+16);
				gametext(128,48+30*(m-first)-8+15,"SATU",0,2+8+16);
				
				dx = odx = min(64,64l*JoystickAnalogueDead[m]/10000l);
				dy = ody = min(64,64l*JoystickAnalogueSaturate[m]/10000l);

				bar(217,48+30*(m-first),&dx,4,x==((m-first)*2),0,0);
				bar(217,48+30*(m-first)+15,&dy,4,x==((m-first)*2+1),0,0);

				Bsprintf(tempbuf,"%3d%%",100*dx/64); gametext(217-49,48+30*(m-first)-8,tempbuf,0,2+8+16);
				Bsprintf(tempbuf,"%3d%%",100*dy/64); gametext(217-49,48+30*(m-first)-8+15,tempbuf,0,2+8+16);

				if (dx != odx) JoystickAnalogueDead[m]     = 10000l*dx/64l;
				if (dy != ody) JoystickAnalogueSaturate[m] = 10000l*dy/64l;
				if (dx != odx || dy != ody)
					setjoydeadzone(m,JoystickAnalogueDead[m],JoystickAnalogueSaturate[m]);
			}
		//gametext(160,158,"DEAD = DEAD ZONE, SAT. = SATURATION",0,2+8+16);
		if (joynumaxes>4) {
			menutext(32,48+30*(last-first),0,0,"NEXT...");
			sprintf(tempbuf,"Page %d of %d", 1+(current_menu-213), (joynumaxes+3)/4);
			gametext(320-100,158,tempbuf,0,2+8+16);
		}
		break;		
	}
	    
        case 700:
		case 701:	// JBF 20041220: A hack to stop the game exiting the menu directly to the game if one is running
            c = (320>>1)-120;
            rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"SOUNDS");
            onbar = ( probey == 2 || probey == 3 );

            x = probe(c,50,16,7);
            switch(x)
            {
                case -1:
                    if(ps[myconnectindex].gm&MODE_GAME && current_menu == 701)
                    {
                        ps[myconnectindex].gm &= ~MODE_MENU;
                        if(ud.multimode < 2  && ud.recstat != 2)
                        {
                            ready2send = 1;
                            totalclock = ototalclock;
                        }
                    }

                    else cmenu(202);
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
                        if( MusicToggle == 0 ) MUSIC_Pause();
                        else
                        {
                            if(ud.recstat != 2 && ps[myconnectindex].gm&MODE_GAME)
                                playmusic(&music_fn[0][music_select][0]);
                            else playmusic(&env_music_fn[0][0]);

                            MUSIC_Continue();
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
                FXVolume >>= 2;
                bar(c+167+40,50+16+16,(short *)&FXVolume,4,(FXDevice>=0)&&x==2,SHX(-4),SoundToggle==0||(FXDevice<0));
                if(l != FXVolume)
                    FXVolume <<= 2;
                if(l != FXVolume)
                    FX_SetVolume( (short) FXVolume );
            }
            menutext(c,50+16,SHX(-3),(MusicDevice<0),"MUSIC");
            menutext(c,50+16+16+16,SHX(-5),(MusicDevice<0)||MusicToggle==0,"MUSIC VOLUME");
            {
                l = MusicVolume;
                MusicVolume >>= 2;
                bar(c+167+40,50+16+16+16,
                    (short *)&MusicVolume,4,
                    (MusicDevice>=0) && x==3,SHX(-5),
                    MusicToggle==0||(MusicDevice<0));
                MusicVolume <<= 2;
                if(l != MusicVolume)
                    MUSIC_SetVolume( (short) MusicVolume );

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
            //savetemp("duke3d.tmp",waloff[MAXTILES-1],160*100);
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
                sprintf(tempbuf,"PLAYERS: %-2ld                      ",ud.multimode);
                gametext(160,156,tempbuf,0,2+8+16);
                sprintf(tempbuf,"EPISODE: %-2ld / LEVEL: %-2ld / SKILL: %-2ld",1+ud.volume_number,1+ud.level_number,ud.player_skill);
                gametext(160,168,tempbuf,0,2+8+16);
				if (ud.volume_number == 0 && ud.level_number == 7)
					gametext(160,180,boardfilename,0,2+8+16);

                x = strget((320>>1),184,&ud.savegame[current_menu-360][0],19, 999 );

                if(x == -1)
                {
            //        readsavenames();
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

                rotatesprite(101<<16,97<<16,65536>>1,512,MAXTILES-1,-32,0,2+4+8+64,0,0,xdim-1,ydim-1);
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

                  rotatesprite(101<<16,97<<16,65536L>>1,512,MAXTILES-3,-32,0,4+10+64,0,0,xdim-1,ydim-1);
                  sprintf(tempbuf,"PLAYERS: %-2ld                      ",savehead.numplr);
                  gametext(160,156,tempbuf,0,2+8+16);
                  sprintf(tempbuf,"EPISODE: %-2ld / LEVEL: %-2ld / SKILL: %-2ld",1+savehead.volnum,1+savehead.levnum,savehead.plrskl);
                  gametext(160,168,tempbuf,0,2+8+16);
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
                  rotatesprite(101<<16,97<<16,65536L>>1,512,MAXTILES-3,-32,0,4+10+64,0,0,xdim-1,ydim-1);
              }
              else menutext(69,70,0,0,"EMPTY");
              sprintf(tempbuf,"PLAYERS: %-2ld                      ",ud.multimode);
              gametext(160,156,tempbuf,0,2+8+16);
              sprintf(tempbuf,"EPISODE: %-2ld / LEVEL: %-2ld / SKILL: %-2ld",1+ud.volume_number,1+ud.level_number,ud.player_skill);
              gametext(160,168,tempbuf,0,2+8+16);
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
                        if( ud.savegame[x][0] != 0)
                            current_menu = 2000+x;
                        else
                        {
                            KB_FlushKeyboardQueue();
                            current_menu = (360+x);
                            ud.savegame[x][0] = 0;
                            inputloc = 0;
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
        case 402:
        case 403:

            c = 320>>1;

            if( KB_KeyPressed( sc_LeftArrow ) ||
                KB_KeyPressed( sc_kpad_4 ) ||
                KB_KeyPressed( sc_UpArrow ) ||
                KB_KeyPressed( sc_PgUp ) ||
                KB_KeyPressed( sc_kpad_8 ) )
            {
                KB_ClearKeyDown(sc_LeftArrow);
                KB_ClearKeyDown(sc_kpad_4);
                KB_ClearKeyDown(sc_UpArrow);
                KB_ClearKeyDown(sc_PgUp);
                KB_ClearKeyDown(sc_kpad_8);

                sound(KICK_HIT);
                current_menu--;
                if(current_menu < 400) current_menu = 403;
            }
            else if(
                KB_KeyPressed( sc_PgDn ) ||
                KB_KeyPressed( sc_Enter ) ||
                KB_KeyPressed( sc_kpad_Enter ) ||
                KB_KeyPressed( sc_RightArrow ) ||
                KB_KeyPressed( sc_DownArrow ) ||
                KB_KeyPressed( sc_kpad_2 ) ||
                KB_KeyPressed( sc_kpad_9 ) ||
                KB_KeyPressed( sc_Space ) ||
                KB_KeyPressed( sc_kpad_6 ) )
            {
                KB_ClearKeyDown(sc_PgDn);
                KB_ClearKeyDown(sc_Enter);
                KB_ClearKeyDown(sc_RightArrow);
                KB_ClearKeyDown(sc_kpad_Enter);
                KB_ClearKeyDown(sc_kpad_6);
                KB_ClearKeyDown(sc_kpad_9);
                KB_ClearKeyDown(sc_kpad_2);
                KB_ClearKeyDown(sc_DownArrow);
                KB_ClearKeyDown(sc_Space);
                sound(KICK_HIT);
                current_menu++;
                if(current_menu > 403) current_menu = 400;
            }

            if( KB_KeyPressed(sc_Escape) )
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

            if( KB_KeyPressed( sc_LeftArrow ) ||
                KB_KeyPressed( sc_kpad_4 ) ||
                KB_KeyPressed( sc_UpArrow ) ||
                KB_KeyPressed( sc_PgUp ) ||
                KB_KeyPressed( sc_kpad_8 ) )
            {
                KB_ClearKeyDown(sc_LeftArrow);
                KB_ClearKeyDown(sc_kpad_4);
                KB_ClearKeyDown(sc_UpArrow);
                KB_ClearKeyDown(sc_PgUp);
                KB_ClearKeyDown(sc_kpad_8);

                sound(KICK_HIT);
                current_menu--;
                if(current_menu < 400) current_menu = 401;
            }
            else if(
                KB_KeyPressed( sc_PgDn ) ||
                KB_KeyPressed( sc_Enter ) ||
                KB_KeyPressed( sc_kpad_Enter ) ||
                KB_KeyPressed( sc_RightArrow ) ||
                KB_KeyPressed( sc_DownArrow ) ||
                KB_KeyPressed( sc_kpad_2 ) ||
                KB_KeyPressed( sc_kpad_9 ) ||
                KB_KeyPressed( sc_Space ) ||
                KB_KeyPressed( sc_kpad_6 ) )
            {
                KB_ClearKeyDown(sc_PgDn);
                KB_ClearKeyDown(sc_Enter);
                KB_ClearKeyDown(sc_RightArrow);
                KB_ClearKeyDown(sc_kpad_Enter);
                KB_ClearKeyDown(sc_kpad_6);
                KB_ClearKeyDown(sc_kpad_9);
                KB_ClearKeyDown(sc_kpad_2);
                KB_ClearKeyDown(sc_DownArrow);
                KB_ClearKeyDown(sc_Space);
                sound(KICK_HIT);
                current_menu++;
                if(current_menu > 401) current_menu = 400;
            }

            if( KB_KeyPressed(sc_Escape) )
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

            if( KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB )
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
            if(x == -1 || KB_KeyPressed(sc_N) || RMB)
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

            if( KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB )
            {
                KB_FlushKeyboardQueue();
                ps[myconnectindex].gm = MODE_DEMO;
                if(ud.recstat == 1)
                    closedemowrite();
                cmenu(0);
            }

            x = probe(186,124,0,0);

            if(x == -1 || KB_KeyPressed(sc_N) || RMB)
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

            sprintf(tempbuf,"Waiting for master");
            gametext(160,50,tempbuf,0,2+8+16);
            gametext(160,59,"to select level",0,2+8+16);

            if( KB_KeyPressed(sc_Escape) )
            {
                KB_ClearKeyDown(sc_Escape);
                sound(EXITMENUSOUND);
                cmenu(0);
            }
            break;

        case 602:
            if(menunamecnt == 0)
            {
        //        getfilenames("SUBD");
                getfilenames(".","*.MAP");
                sortfilenames();
                if (menunamecnt == 0)
                    cmenu(600);
            }
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
            if( KB_KeyPressed( sc_LeftArrow ) || KB_KeyPressed( sc_kpad_4 ) || ((buttonstat&1) && minfo.dyaw < -256 ) )
            {
                KB_ClearKeyDown( sc_LeftArrow );
                KB_ClearKeyDown( sc_kpad_4 );
                probey -= 15;
                if(probey < 0) probey += 15;
                else sound(KICK_HIT);
            }
            if( KB_KeyPressed( sc_RightArrow ) || KB_KeyPressed( sc_kpad_6 ) || ((buttonstat&1) && minfo.dyaw > 256 ) )
            {
                KB_ClearKeyDown( sc_RightArrow );
                KB_ClearKeyDown( sc_kpad_6 );
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
                enterlevel(MODE_GAME);
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
#ifndef ONELEVELDEMO
                    ud.m_level_number++;
if (!VOLUMEONE) {
                    if(ud.m_volume_number == 0 && ud.m_level_number > 6)
                        ud.m_level_number = 0;
} else {
                    if(ud.m_volume_number == 0 && ud.m_level_number > 5)
                        ud.m_level_number = 0;
}
                    if(ud.m_level_number > 10) ud.m_level_number = 0;
#endif
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
                    enterlevel(MODE_GAME);

                    return;
}
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
                    enterlevel(MODE_GAME);

                    return;

            }

            c += 40;

            if(ud.m_coop==1) gametext(c+70,57-7-9,"COOPERATIVE PLAY",0,2+8+16);
            else if(ud.m_coop==2) gametext(c+70,57-7-9,"DUKEMATCH (NO SPAWN)",0,2+8+16);
            else gametext(c+70,57-7-9,"DUKEMATCH (SPAWN)",0,2+8+16);

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
            sprintf(tempbuf,"EPISODE %ld",ud.m_volume_number+1);
            menutext(c,57+16-9,SHX(-3),1,tempbuf);
} else {
            sprintf(tempbuf,"EPISODE %ld",ud.m_volume_number+1);
            menutext(c,57+16-9,SHX(-3),PHX(-3),tempbuf);
}

#ifndef ONELEVELDEMO
            sprintf(tempbuf,"LEVEL %ld",ud.m_level_number+1);
            menutext(c,57+16+16-9,SHX(-4),PHX(-4),tempbuf);
#else
            sprintf(tempbuf,"LEVEL %ld",ud.m_level_number+1);
            menutext(c,57+16+16-9,SHX(-4),1,tempbuf);
#endif
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

void palto(char r,char g,char b,long e)
{
    int i;
    char temparray[768];
    long tc;
/*
    for(i=0;i<768;i+=3)
    {
        temparray[i  ] =
            ps[myconnectindex].palette[i+0]+((((long)r-(long)ps[myconnectindex].palette[i+0])*(long)(e&127))>>6);
        temparray[i+1] =
            ps[myconnectindex].palette[i+1]+((((long)g-(long)ps[myconnectindex].palette[i+1])*(long)(e&127))>>6);
        temparray[i+2] =
            ps[myconnectindex].palette[i+2]+((((long)b-(long)ps[myconnectindex].palette[i+2])*(long)(e&127))>>6);
    }
*/

    //setbrightness(ud.brightness>>2,temparray);
	setpalettefade(r,g,b,e&127);
	if (getrendermode() >= 3) pus = pub = NUMPAGES;	// JBF 20040110: redraw the status bar next time
    if ((e&128) == 0) {
	    nextpage();
	    for (tc = totalclock; totalclock < tc + 4; getpackets() );
    }
}


void drawoverheadmap(long cposx, long cposy, long czoom, short cang)
{
        long i, j, k, l, x1, y1, x2=0, y2=0, x3, y3, x4, y4, ox, oy, xoff, yoff;
        long dax, day, cosang, sinang, xspan, yspan, sprx, spry;
        long xrepeat, yrepeat, z1, z2, startwall, endwall, tilenum, daang;
        long xvect, yvect, xvect2, yvect2;
        short p;
        char col;
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
                            xoff = (long)((signed char)((picanm[tilenum]>>8)&255))+((long)spr->xoffset);
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
                                                xoff = (long)((signed char)((picanm[tilenum]>>8)&255))+((long)spr->xoffset);
                                                yoff = (long)((signed char)((picanm[tilenum]>>16)&255))+((long)spr->yoffset);
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



void endanimsounds(long fr)
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

void logoanimsounds(long fr)
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

void intro4animsounds(long fr)
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

void first4animsounds(long fr)
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

void intro42animsounds(long fr)
{
    switch(fr)
    {
        case 10:
            sound(INTRO4_6);
            break;
    }
}




void endanimvol41(long fr)
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

void endanimvol42(long fr)
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

void endanimvol43(long fr)
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


long lastanimhack=0;
void playanm(char *fn,char t)
{
        char *animbuf, *palptr;
    long i, j, k, length=0, numframes=0;
    int32 handle=-1;

//    return;

    if(t != 7 && t != 9 && t != 10 && t != 11)
        KB_FlushKeyboardQueue();

    if( KB_KeyWaiting() )
    {
        FX_StopAllSounds();
        goto ENDOFANIMLOOP;
    }

        handle = kopen4load(fn,0);
        if(handle == -1) return;
        length = kfilelength(handle);

    walock[MAXTILES-3-t] = 219+t;

    if(anim == 0 || lastanimhack != (MAXTILES-3-t))
        allocache((long *)&anim,length+sizeof(anim_t),&walock[MAXTILES-3-t]);

    animbuf = (char *)(FP_OFF(anim)+sizeof(anim_t));

    lastanimhack = (MAXTILES-3-t);

    tilesizx[MAXTILES-3-t] = 200;
    tilesizy[MAXTILES-3-t] = 320;

        kread(handle,animbuf,length);
        kclose(handle);

        ANIM_LoadAnim (animbuf);
        numframes = ANIM_NumFrames();

        palptr = ANIM_GetPalette();
        for(i=0;i<256;i++)
        {
			/*
                j = (i<<2); k = j-i;
                tempbuf[j+0] = (palptr[k+2]>>2);
                tempbuf[j+1] = (palptr[k+1]>>2);
                tempbuf[j+2] = (palptr[k+0]>>2);
                tempbuf[j+3] = 0;
				*/
			j = i*3;
                tempbuf[j+0] = (palptr[j+0]>>2);
                tempbuf[j+1] = (palptr[j+1]>>2);
                tempbuf[j+2] = (palptr[j+2]>>2);
        }

        //setpalette(0L,256L,tempbuf);
		//setbrightness(ud.brightness>>2,tempbuf,2);
		setgamepalette(&ps[myconnectindex],tempbuf,2);

    ototalclock = totalclock + 10;

        for(i=1;i<numframes;i++)
        {
       while(totalclock < ototalclock)
       {
          if( KB_KeyWaiting() )
              goto ENDOFANIMLOOP;
          getpackets();
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

       waloff[MAXTILES-3-t] = FP_OFF(ANIM_DrawFrame(i));
	   invalidatetile(MAXTILES-3-t, 0, 1<<4);	// JBF 20031228
       rotatesprite(0<<16,0<<16,65536L,512,MAXTILES-3-t,0,0,2+4+8+16+64, 0,0,xdim-1,ydim-1);
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

    ANIM_FreeAnim ();
    walock[MAXTILES-3-t] = 1;
}

/*
 * vim:ts=4:sw=4:tw=8:
 */


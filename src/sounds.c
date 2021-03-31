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

//#include <conio.h>
#include <stdio.h>
#include <string.h>
#include "duke3d.h"

/* thanks retroarch */
static char *string_to_lower(char *s)
{
   char *cs = (char *)s;
   for ( ; *cs != '\0'; cs++)
      *cs = tolower((unsigned char)*cs);
   return s;
}

int loadsoundoverride(char *base, char *fn)
{
    int fp = -1;

    char name[64];
    char ext[8];
    char path[128];

    sscanf(fn, "%[^.]%s", name, ext);

    if (*ext) {
        // we've been asked to load a file, but first
        // let's see if there's an ogg with the same base name
        // lying around
        snprintf(path, sizeof(path), "%s/%s.ogg", base, name);
        string_to_lower(path);
        fp = kopen4load(path, 0);
    }

    if (fp < 0) {
        // just use what we've been given
        fp = kopen4load(fn, 0);
    }

    return fp;
}

#define LOUDESTVOLUME 150

#define MUSIC_ID  -65536

int backflag,numenvsnds;

static int MusicIsWaveform = 0;
static char * MusicPtr = 0;
static int MusicLen = 0;
static int MusicVoice = -1;
static int MusicPaused = 0;


/*
===================
=
= SoundStartup
=
===================
*/

void SoundStartup( void )
{
   int32 status;
    int fxdevicetype;
    void * initdata = 0;

   // if they chose None lets return
    if (FXDevice < 0) {
        return;
    } else if (FXDevice == 0) {
        fxdevicetype = ASS_AutoDetect;
    } else {
        fxdevicetype = FXDevice - 1;
    }
    
    #ifdef _WIN32
    initdata = (void *) win_gethwnd();
    #endif

   status = FX_Init( fxdevicetype, NumVoices, &NumChannels, &NumBits, &MixRate, initdata );
   if ( status == FX_Ok ) {
      FX_SetVolume( FXVolume );
      FX_SetReverseStereo(ReverseStereo);
	  status = FX_SetCallBack( testcallback );
  }

   if ( status != FX_Ok ) {
	  sprintf(buf, "Sound startup error: %s", FX_ErrorString( FX_Error ));
	  gameexit(buf);
   }
	
	FXDevice = 0;
}

/*
===================
=
= SoundShutdown
=
===================
*/

void SoundShutdown( void )
{
   int32 status;

   // if they chose None lets return
   if (FXDevice < 0)
      return;
   
   if (MusicVoice >= 0) {
      MusicShutdown();
   }

   status = FX_Shutdown();
   if ( status != FX_Ok ) {
	  sprintf(buf, "Sound shutdown error: %s", FX_ErrorString( FX_Error ));
      gameexit(buf);
   }
}

/*
===================
=
= MusicStartup
=
===================
*/

void MusicStartup( void )
   {
   int32 status;
   int musicdevicetype;

   // if they chose None lets return
   if (MusicDevice < 0) {
      return;
   } else if (MusicDevice == 0) {
      musicdevicetype = ASS_AutoDetect;
   } else {
      musicdevicetype = MusicDevice - 1;
   }
   
   status = MUSIC_Init( musicdevicetype, MusicParams );

   if ( status == MUSIC_Ok )
      {
      MUSIC_SetVolume( MusicVolume );
      }
   else
   {
      buildprintf("Couldn't find selected sound card, or, error w/ sound card itself.\n");

      SoundShutdown();
      uninittimer();
      uninitengine();
      CONTROL_Shutdown();
      CONFIG_WriteSetup();
      KB_Shutdown();
      uninitgroupfile();
      //unlink("duke3d.tmp");
      exit(-1);
   }
}

/*
===================
=
= MusicShutdown
=
===================
*/

void MusicShutdown( void )
   {
   int32 status;

   // if they chose None lets return
   if (MusicDevice < 0)
      return;
   
   stopmusic();
   
   status = MUSIC_Shutdown();
   if ( status != MUSIC_Ok )
      {
      Error( MUSIC_ErrorString( MUSIC_ErrorCode ));
      }
   }

void MusicPause( int onf )
{
   if (MusicPaused == onf || (MusicIsWaveform && MusicVoice < 0)) {
      return;
   }
   
   if (onf) {
      if (MusicIsWaveform) {
         FX_PauseSound(MusicVoice, TRUE);
      } else {
         MUSIC_Pause();
      }
   } else {
      if (MusicIsWaveform) {
         FX_PauseSound(MusicVoice, FALSE);
      } else {
         MUSIC_Continue();
      }
   }
   
   MusicPaused = onf;
}

void MusicSetVolume(int volume)
{
   if (MusicIsWaveform && MusicVoice >= 0) {
      //FX_SetVoiceVolume(MusicVoice, volume);
   } else if (!MusicIsWaveform) {
      MUSIC_SetVolume(volume);
   }
}

int USRHOOKS_GetMem(char **ptr, unsigned int size )
{
   *ptr = malloc(size);

   if (*ptr == NULL)
      return(USRHOOKS_Error);

   return( USRHOOKS_Ok);

}

int USRHOOKS_FreeMem(char *ptr)
{
   free(ptr);
   return( USRHOOKS_Ok);
}

unsigned char menunum=0;

void intomenusounds(void)
{
    short i;
    short menusnds[] =
    {
        LASERTRIP_EXPLODE,
        DUKE_GRUNT,
        DUKE_LAND_HURT,
        CHAINGUN_FIRE,
        SQUISHED,
        KICK_HIT,
        PISTOL_RICOCHET,
        PISTOL_BODYHIT,
        PISTOL_FIRE,
        SHOTGUN_FIRE,
        BOS1_WALK,
        RPG_EXPLODE,
        PIPEBOMB_BOUNCE,
        PIPEBOMB_EXPLODE,
        NITEVISION_ONOFF,
        RPG_SHOOT,
        SELECT_WEAPON
    };
    sound(menusnds[menunum++]);
    menunum %= 17;
}

void playmusic(char *fn)
{
    int fp;

    if (MusicToggle == 0) return;
    if (MusicDevice < 0) return;

    stopmusic();

    fp = loadsoundoverride("music", fn);

    if (fp == -1) {
        return;
    }

    MusicLen = kfilelength( fp );
    MusicPtr = (char *) malloc(MusicLen);
    kread( fp, MusicPtr, MusicLen);
    kclose( fp );
    
    if (!memcmp(MusicPtr, "MThd", 4)) {
       MUSIC_PlaySong( MusicPtr, MusicLen, MUSIC_LoopSong );
       MusicIsWaveform = 0;
    } else {
       MusicVoice = FX_PlayLoopedAuto(MusicPtr, MusicLen, 0, 0, 0,
                                      MusicVolume, MusicVolume, MusicVolume,
				      FX_MUSIC_PRIORITY, MUSIC_ID);
       MusicIsWaveform = 1;
    }

    MusicPaused = 0;
}

void stopmusic(void)
{
    if (MusicIsWaveform && MusicVoice >= 0) {
       FX_StopSound(MusicVoice);
       MusicVoice = -1;
    } else if (!MusicIsWaveform) {
       MUSIC_StopSong();
    }

    MusicPaused = 0;

    if (MusicPtr) {
       free(MusicPtr);
       MusicPtr = 0;
       MusicLen = 0;
    }
}

char loadsound(unsigned short num)
{
    int   fp, l;

    if(num >= NUM_SOUNDS || SoundToggle == 0) return 0;
    if (FXDevice < 0) return 0;

    fp = loadsoundoverride("sound", sounds[num]);

    if(fp == -1)
    {
        sprintf(&fta_quotes[113][0],"Sound %s(#%d) not found.",sounds[num],num);
        FTA(113,&ps[myconnectindex]);
        return 0;
    }

    l = kfilelength( fp );
    soundsiz[num] = l;

    Sound[num].lock = 200;

    allocache((void **)&Sound[num].ptr,l,&Sound[num].lock);
    kread( fp, Sound[num].ptr , l);
    kclose( fp );
    return 1;
}

int xyzsound(short num,short i,int x,int y,int z)
{
    int sndist, cx, cy, cz, j,k;
    short pitche,pitchs,cs;
    int voice, sndang, ca, pitch;

//    if(num != 358) return 0;

    if( num >= NUM_SOUNDS ||
        FXDevice < 0 ||
        ( (soundm[num]&8) && ud.lockout ) ||
        SoundToggle == 0 ||
        Sound[num].num > 3 ||
        FX_VoiceAvailable(soundpr[num]) == 0 ||
        (ps[myconnectindex].timebeforeexit > 0 && ps[myconnectindex].timebeforeexit <= 26*3) ||
        ps[myconnectindex].gm&MODE_MENU) return -1;

    if( soundm[num]&128 )
    {
        sound(num);
        return 0;
    }

    if( soundm[num]&4 )
    {
        if(VoiceToggle==0 || (ud.multimode > 1 && PN == APLAYER && sprite[i].yvel != screenpeek && ud.coop != 1) ) return -1;

        for(j=0;j<NUM_SOUNDS;j++)
          for(k=0;k<Sound[j].num;k++)
            if( (Sound[j].num > 0) && (soundm[j]&4) )
              return -1;
    }

    cx = ps[screenpeek].oposx;
    cy = ps[screenpeek].oposy;
    cz = ps[screenpeek].oposz;
    cs = ps[screenpeek].cursectnum;
    ca = ps[screenpeek].ang+ps[screenpeek].look_ang;

    sndist = FindDistance3D((cx-x),(cy-y),(cz-z)>>4);

    if( i >= 0 && (soundm[num]&16) == 0 && PN == MUSICANDSFX && SLT < 999 && (sector[SECT].lotag&0xff) < 9 )
        sndist = divscale14(sndist,(SHT+1));

    pitchs = soundps[num];
    pitche = soundpe[num];
    j = klabs(pitche-pitchs);

    if(j)
    {
        if( pitchs < pitche )
             pitch = pitchs + ( rand()%j );
        else pitch = pitche + ( rand()%j );
    }
    else pitch = pitchs;

    sndist += soundvo[num];
    if(sndist < 0) sndist = 0;
    if( sndist && PN != MUSICANDSFX && !cansee(cx,cy,cz-(24<<8),cs,SX,SY,SZ-(24<<8),SECT) )
        sndist += sndist>>5;

    switch(num)
    {
        case PIPEBOMB_EXPLODE:
        case LASERTRIP_EXPLODE:
        case RPG_EXPLODE:
            if(sndist > (6144) )
                sndist = 6144;
            if(sector[ps[screenpeek].cursectnum].lotag == 2)
                pitch -= 1024;
            break;
        default:
            if(sector[ps[screenpeek].cursectnum].lotag == 2 && (soundm[num]&4) == 0)
                pitch = -768;
            if( sndist > 31444 && PN != MUSICANDSFX)
                return -1;
            break;
    }


    if( Sound[num].num > 0 && PN != MUSICANDSFX )
    {
        if( SoundOwner[num][0].i == i ) stopsound(num);
        else if( Sound[num].num > 1 ) stopsound(num);
        else if( badguy(&sprite[i]) && sprite[i].extra <= 0 ) stopsound(num);
    }

    if( PN == APLAYER && sprite[i].yvel == screenpeek )
    {
        sndang = 0;
        sndist = 0;
    }
    else
    {
        sndang = 2048 + ca - getangle(cx-x,cy-y);
        sndang &= 2047;
    }

    if(Sound[num].ptr == 0) { if( loadsound(num) == 0 ) return 0; }
    else
    {
       if (Sound[num].lock < 200)
          Sound[num].lock = 200;
       else Sound[num].lock++;
    }

    if( soundm[num]&16 ) sndist = 0;

    if(sndist < ((255-LOUDESTVOLUME)<<6) )
        sndist = ((255-LOUDESTVOLUME)<<6);

    if( soundm[num]&1 )
    {
        if(Sound[num].num > 0) return -1;

        voice = FX_PlayLoopedAuto( Sound[num].ptr, soundsiz[num], 0, -1,
                    pitch,sndist>>6,sndist>>6,0,soundpr[num],num);
    }
    else
    {
        voice = FX_PlayAuto3D( Sound[ num ].ptr, soundsiz[num], pitch,sndang>>6,sndist>>6, soundpr[num], num );
    }

    if ( voice > FX_Ok )
    {
        SoundOwner[num][Sound[num].num].i = i;
        SoundOwner[num][Sound[num].num].voice = voice;
        Sound[num].num++;
		  Sound[num].numall++;
    }
    else Sound[num].lock--;
    return (voice);
}

void sound(short num)
{
    short pitch,pitche,pitchs,cx;
    int voice;
    int start;

    if (FXDevice < 0) return;
    if(SoundToggle==0) return;
    if(VoiceToggle==0 && (soundm[num]&4) ) return;
    if( (soundm[num]&8) && ud.lockout ) return;
    if(FX_VoiceAvailable(soundpr[num]) == 0) return;

    pitchs = soundps[num];
    pitche = soundpe[num];
    cx = klabs(pitche-pitchs);

    if(cx)
    {
        if( pitchs < pitche )
             pitch = pitchs + ( rand()%cx );
        else pitch = pitche + ( rand()%cx );
    }
    else pitch = pitchs;

    if(Sound[num].ptr == 0) { if( loadsound(num) == 0 ) return; }
    else
    {
       if (Sound[num].lock < 200)
          Sound[num].lock = 200;
       else Sound[num].lock++;
    }

    if( soundm[num]&1 )
    {
         voice = FX_PlayLoopedAuto( Sound[num].ptr, soundsiz[num], 0, -1,
                 pitch,LOUDESTVOLUME,LOUDESTVOLUME,LOUDESTVOLUME,soundpr[num],num);
    }
    else
    {
        voice = FX_PlayAuto3D( Sound[ num ].ptr, soundsiz[num], pitch,0,255-LOUDESTVOLUME,soundpr[num], num );
    }

    if(voice > FX_Ok) {
		 Sound[num].numall++;
		 return;
	 }
    Sound[num].lock--;
}

int spritesound(unsigned short num, short i)
{
    if(num >= NUM_SOUNDS) return -1;
    return xyzsound(num,i,SX,SY,SZ);
}

void stopspritesound(short num, short UNUSED(i))
{
	stopsound(num);
}

void stopsound(short num)
{
    if(Sound[num].num > 0)
    {
        FX_StopSound(SoundOwner[num][Sound[num].num-1].voice);
        //testcallback(num);
    }
}

void stopenvsound(short num,short i)
{
    short j, k;

    if(Sound[num].num > 0)
    {
        k = Sound[num].num;
        for(j=0;j<k;j++)
           if(SoundOwner[num][j].i == i)
        {
            FX_StopSound(SoundOwner[num][j].voice);
            break;
        }
    }
}

void pan3dsound(void)
{
    int sndist, sx, sy, sz, cx, cy, cz;
    short sndang,ca,j,k,i,cs;

    numenvsnds = 0;

    if(ud.camerasprite == -1)
    {
        cx = ps[screenpeek].oposx;
        cy = ps[screenpeek].oposy;
        cz = ps[screenpeek].oposz;
        cs = ps[screenpeek].cursectnum;
        ca = ps[screenpeek].ang+ps[screenpeek].look_ang;
    }
    else
    {
        cx = sprite[ud.camerasprite].x;
        cy = sprite[ud.camerasprite].y;
        cz = sprite[ud.camerasprite].z;
        cs = sprite[ud.camerasprite].sectnum;
        ca = sprite[ud.camerasprite].ang;
    }

    for(j=0;j<NUM_SOUNDS;j++) for(k=0;k<Sound[j].num;k++)
    {
        i = SoundOwner[j][k].i;

        sx = sprite[i].x;
        sy = sprite[i].y;
        sz = sprite[i].z;

        if( PN == APLAYER && sprite[i].yvel == screenpeek)
        {
            sndang = 0;
            sndist = 0;
        }
        else
        {
            sndang = 2048 + ca - getangle(cx-sx,cy-sy);
            sndang &= 2047;
            sndist = FindDistance3D((cx-sx),(cy-sy),(cz-sz)>>4);
            if( i >= 0 && (soundm[j]&16) == 0 && PN == MUSICANDSFX && SLT < 999 && (sector[SECT].lotag&0xff) < 9 )
                sndist = divscale14(sndist,(SHT+1));
        }

        sndist += soundvo[j];
        if(sndist < 0) sndist = 0;

        if( sndist && PN != MUSICANDSFX && !cansee(cx,cy,cz-(24<<8),cs,sx,sy,sz-(24<<8),SECT) )
            sndist += sndist>>5;

        if(PN == MUSICANDSFX && SLT < 999)
            numenvsnds++;

        switch(j)
        {
            case PIPEBOMB_EXPLODE:
            case LASERTRIP_EXPLODE:
            case RPG_EXPLODE:
                if(sndist > (6144)) sndist = (6144);
                break;
            default:
                if( sndist > 31444 && PN != MUSICANDSFX)
                {
                    stopsound(j);
                    continue;
                }
        }

        if(Sound[j].ptr == 0 && loadsound(j) == 0 ) continue;
        if( soundm[j]&16 ) sndist = 0;

        if(sndist < ((255-LOUDESTVOLUME)<<6) )
            sndist = ((255-LOUDESTVOLUME)<<6);

        FX_Pan3D(SoundOwner[j][k].voice,sndang>>6,sndist>>6);
    }
}

void testcallback(unsigned int num)
{
    short tempi,tempj,tempk;

   if ((int) num == MUSIC_ID) {
      return;
   }
   
        if((int)num < 0)
        {
            if(lumplockbyte[-num] >= 200)
                lumplockbyte[-num]--;
            return;
        }

        tempk = Sound[num].num;

        if(tempk > 0)
        {
            if( (soundm[num]&16) == 0)
                for(tempj=0;tempj<tempk;tempj++)
            {
                tempi = SoundOwner[num][tempj].i;
                if(sprite[tempi].picnum == MUSICANDSFX && sector[sprite[tempi].sectnum].lotag < 3 && sprite[tempi].lotag < 999)
                {
                    hittype[tempi].temp_data[0] = 0;
                    if( (tempj + 1) < tempk )
                    {
                        SoundOwner[num][tempj].voice = SoundOwner[num][tempk-1].voice;
                        SoundOwner[num][tempj].i     = SoundOwner[num][tempk-1].i;
                    }
                    break;
                }
            }

            Sound[num].num--;
            SoundOwner[num][tempk-1].i = -1;
        }

		  Sound[num].numall--;
        Sound[num].lock--;
}

void clearsoundlocks(void)
{
    int i;

    for(i=0;i<NUM_SOUNDS;i++)
        if(Sound[i].lock >= 200)
            Sound[i].lock = 199;

    for(i=0;i<11;i++)
        if(lumplockbyte[i] >= 200)
            lumplockbyte[i] = 199;
}

int isspritemakingsound(short i, int num)
{
	int j, k;
	if (num < 0) {
		// is sprite making any sound at all (expensive)
		for (j = 0; j < NUM_SOUNDS; j++) {
			for (k = 0; k < Sound[j].num; k++) {
            if (SoundOwner[j][k].i == i) {
					return 1;
				}
			}
		}
	} else {
		for (k = 0; k < Sound[num].num; k++) {
			if (SoundOwner[num][k].i == i) {
				return 1;
			}
		}
	}

	return 0;
}

// xyz: 0 - sound(), 1 - xyzsound(), 2 - either
int issoundplaying(int num, int xyz)
{
    if (xyz == 0) {
        return Sound[num].numall - Sound[num].num;
    } else if (xyz == 1) {
	return Sound[num].num;
    } else {
        return Sound[num].numall;
    }
}

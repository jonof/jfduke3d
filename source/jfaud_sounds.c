/*
 * Audio support for JFDuke3D using JFAud
 * by Jonathon Fowler (jonof@edgenetwork.org)
 * 
 * Duke Nukem 3D is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Original Source: 1996 - Todd Replogle
 * Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
 */

#include "jfaud.h"
#include "types.h"
#include "duke3d.h"
#include "osd.h"

// soundm&1 = loop
// soundm&2 = musicandsfx
// soundm&4 = duke speech
// soundm&8 = parental lockout
// soundm&16 = no rolloff
// soundm&128 = player local

#define UNITSPERMETRE 512.0

static int call_open(const char *fn, const char *subfn, JFAudFH *h, JFAudRawFormat **raw)
{
	*raw = NULL;
	memset(h, 0, sizeof(JFAudFH));
	h->fh = kopen4load(fn,0);
	return h->fh;
}
static int call_close(JFAudFH *h) { kclose(h->fh); return 0; }
static int call_read(JFAudFH *h, void *buf, int len) { return kread(h->fh,buf,len); }
static int call_seek(JFAudFH *h, int off, int whence) { return klseek(h->fh,off,whence); }
static int call_tell(JFAudFH *h) { return ktell(h->fh); }
static int call_filelen(JFAudFH *h) { return kfilelength(h->fh); }
static void call_logmsg(const char *str) { initprintf("%s\n",str); }

// this whole handle list could probably be done a whole lot more efficiently
static int inited = 0;
#define AbsoluteMaxVoices 30	// SBLive cards have 31 hardware voices. Leave a spare.
static struct _sndtyp {
	int handle;
	int soundnum;
	int owner;
} sfx[AbsoluteMaxVoices];

static void keephandle(int handle, int soundnum, int owner)
{
	int i,freeh=-1;
	for (i=AbsoluteMaxVoices-1;i>=0;i--) {
		if (sfx[i].handle==-1 && freeh<0) freeh=i;
		else if (sfx[i].handle == handle) { freeh=i; break; }
	}
	if (freeh<0) {
		OSD_Printf("Warning: keephandle() exhausted handle space!\n");
		return;
	}

	if (sfx[freeh].handle >= 0) Sound[soundnum].num--;

	sfx[freeh].handle = handle;
	sfx[freeh].soundnum = soundnum;
	sfx[freeh].owner = owner;
}

long numenvsnds = 0;


/*
 *	The formula for translating an AudioLib pitch is:
 *	
 *	   jfaudpitch = 1.00057779 ^ audiolibpitch
 *	
 *	Which results in
 *	   -1200 audiolib = 0.5 jfaud = one octave lower
 *	   0 audiolib     = 1.0 jfaud = unshifted
 *	   1200 audiolib  = 2.0 jfaud = one octave higher
 * 
	printf("static float pitchtable[] = {\n");
	for (i = -2400, j = 0; i <= 2400; i+=100,j++) {
		if (j%12==0) printf("\t"); else printf(" ");
		printf("%.5f,", (float)pow(1.00057779, i));
		if (j%12==11) printf("\n");
	}
	printf("\n};\n");
*/
static float pitchtable[] = {
	0.25000, 0.26487, 0.28062, 0.29730, 0.31498, 0.33371, 0.35355, 0.37458, 0.39685, 0.42045, 0.44545, 0.47194,
	0.50000, 0.52973, 0.56123, 0.59460, 0.62996, 0.66742, 0.70711, 0.74915, 0.79370, 0.84090, 0.89090, 0.94387,
	1.00000, 1.05946, 1.12246, 1.18921, 1.25992, 1.33484, 1.41421, 1.49831, 1.58740, 1.68179, 1.78180, 1.88775,
	2.00000, 2.11893, 2.24493, 2.37842, 2.51984, 2.66968, 2.82843, 2.99662, 3.17481, 3.36359, 3.56360, 3.77550,
	4.00000
};
static float translatepitch(int p)
{
	float t;
	p = max(-2400,min(p,2400));	// clamp to +/- 2 8'ves
	t = pitchtable[ 24 + p / 100 ];	// pitchtable[24] == 1.0
	return t;//max(0.0,min(t,2.0));	// AL seems to only allow a maximum pitch upscale of 2.0x...!?
}


void SoundStartup(void)
{
	JFAudCfg parms = {
		"", "", "",	// wave, midi, cda
		0, 0, 1,	// #chans, fxsamplerate, synchronous
		call_open, call_close, call_read, call_seek, call_tell, call_filelen, call_logmsg
	};
	int i;
	jfauderr err;

	if (inited) return;
	
	parms.samplerate = MixRate;
	parms.maxvoices = min(AbsoluteMaxVoices,NumVoices);
	inited = 0;

	JFAud_setdebuglevel(0);//DEBUGLVL_ALL);
	
	err = JFAud_init(&parms);
	if (err != jfauderr_ok) {
		initprintf("Audio initialisation error: %s\n", JFAud_errstr(err));
		return;
	}

	inited = 1;
	
	for (i=AbsoluteMaxVoices-1; i>=0; i--) sfx[i].handle = -1;
}

void SoundShutdown(void)
{
	jfauderr err;

	if (!inited) return;

	err = JFAud_uninit();
	if (err != jfauderr_ok) {
		initprintf("Audio uninitialisation error: %s\n", JFAud_errstr(err));
		return;
	}

	inited = 0;
}

void MusicStartup(void)
{
}

void MusicShutdown(void)
{
}

void AudioUpdate(void)
{
	int i;
	JFAudPlayMode pm;
	
	if (!inited) return;

	JFAud_update();
	
	for (i=AbsoluteMaxVoices-1;i>=0;i--) {
		if (sfx[i].handle < 0) continue;

		if (JFAud_getsoundplaymode(sfx[i].handle, &pm) != jfauderr_ok) continue;
		//OSD_Printf("sfx(%s) handle=%d playmode=%d\n",sounds[ sfx[i].soundnum ],sfx[i].handle,pm);
		if (pm != JFAudPlayMode_stopped) continue;

		JFAud_killsound(sfx[i].handle);	// release the channel
		sfx[i].handle = -1;
		Sound[sfx[i].soundnum].num--;
	}
}


static char menunum = 0;
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
    menunum %= sizeof(menusnds)/sizeof(menusnds[0]);
}

void playmusic(char *fn)
{
	jfauderr err;

	if (!inited) return;

	err = JFAud_playmusic(fn, NULL, JFAudPlayMode_playing, 0, NULL);
	if (err != jfauderr_ok) {
		initprintf("Music playback failed: %s\n", JFAud_errstr(err));
		return;
	}
}

char loadsound(unsigned short num)
{
	return 1;
}

int isspritemakingsound(short i, int num)	// if num<0, check if making any sound at all
{
	int j;
	
	for (j=AbsoluteMaxVoices-1;j>=0;j--) {
		if (sfx[j].handle < 0) continue;
		if (sfx[j].owner == i)
			if (num < 0 || (/*num >= 0 &&*/ sfx[j].soundnum == num))
				return sfx[j].soundnum;
	}

	return -1;
}
#include <math.h>
int xyzsound(short num, short i, long x, long y, long z)	// x,y,z is sound origin
{
	float pitch=1.0, gain=1.0, rolloff=1.0;
	char loop=0, global=0;
	int handl;
	JFAudProp props[6];
	jfauderr err;
	
	if (!inited ||
	    num >= NUM_SOUNDS ||
	    ((soundm[num] & 8) && ud.lockout) ||	// parental mode
	    SoundToggle == 0 ||
	    (ps[myconnectindex].timebeforeexit > 0 && ps[myconnectindex].timebeforeexit <= 26*3) ||
	    (ps[myconnectindex].gm & MODE_MENU)
	   ) return -1;

	// tickle the coordinates into jfaud/openal's representation
	x = -x;
	z = (z>>4);	// Z values are 16* finer than the X-Y axes

	if (soundm[num] & 128) {
		// non-3D sound effect
		sound(num);
		return 0;
	}

	if (soundm[num] & 4) {
		// Duke speech, one at a time only
		int j;
		
		if (VoiceToggle == 0 ||
		    (ud.multimode > 1 && PN == APLAYER && sprite[i].yvel != screenpeek && ud.coop != 1)
		   ) return -1;

		// FIXME: seek out any playing Duke voices and exit early if one is found
		for (j=AbsoluteMaxVoices-1;j>=0;j--) {
			if (sfx[j].handle < 0 || sfx[j].owner < 0) continue;
			if (soundm[ sfx[j].soundnum ] & 4)
				return -1;
		}
	}
	
	// This does the ranging of musicandsfx heard distance
	if( i >= 0 && (soundm[num]&16) == 0 && PN == MUSICANDSFX && SLT < 999 && (sector[SECT].lotag&0xff) < 9 )
		rolloff = (powf(10,1.0/20.0) - 1) / (((float)(SHT+1)/UNITSPERMETRE - 1.0)/1.0);

	{
		short pitchvar, pitchbase;
		
		pitchvar = klabs(soundpe[num] - soundps[num]);
		if (pitchvar > 0) {
			if (soundps[num] < soundpe[num])
				pitchbase = soundps[num];
			else
				pitchbase = soundpe[num];
			pitch = translatepitch(pitchbase + (rand() % pitchvar));
		} else {
			pitch = translatepitch(soundps[num]);
		}
	}
	
	gain += (float)soundvo[num]/(150.0*64.0);
	if (gain < 0.0) gain = 0.0; else if (gain > 1.0) gain = 1.0;

	// fake some occlusion
	if( gain > 0.0 && PN != MUSICANDSFX &&
	    !cansee(ps[screenpeek].oposx, ps[screenpeek].oposy, ps[screenpeek].oposz-(24<<8), ps[screenpeek].cursectnum,SX,SY,SZ-(24<<8),SECT)
	  )
		gain *= 0.8; // I guess what I've got here can somewhat approximate the effect of "sndist += sndist>>5;"

    	switch(num)
	{
		case PIPEBOMB_EXPLODE:
		case LASERTRIP_EXPLODE:
		case RPG_EXPLODE:
			global = 1;
			if(sector[ps[screenpeek].cursectnum].lotag == 2)
				pitch *= translatepitch(-1200);
			break;
		default:
			if(sector[ps[screenpeek].cursectnum].lotag == 2 && (soundm[num]&4) == 0)
				pitch *= translatepitch(-768);
			//if( sndist > 31444 && PN != MUSICANDSFX)	// sounds really far away never play
			//	return -1;
			break;
	}
	
	if( Sound[num].num > 0 && PN != MUSICANDSFX )
	{
		if( SoundOwner[num][0].i == i ) stopsound(num);
		else if( Sound[num].num > 1 ) stopsound(num);
		else if( badguy(&sprite[i]) && sprite[i].extra <= 0 ) stopsound(num);
	}

	if (soundm[num] & 1) loop = 1;
	if (soundm[num] & 16) global = 1;

	if (global) rolloff=0.0;

	props[0].prop = JFAudProp_pitch;
	props[0].val.f = pitch;
	props[1].prop = JFAudProp_gain;
	props[1].val.f = gain;
	props[2].prop = JFAudProp_fx;
	props[2].val.i = 1|2;	// 1=nearest, 2=stereo2mono
	
	if (PN == APLAYER && sprite[i].yvel == screenpeek) {
		props[3].prop = JFAudProp_rolloff;
		props[3].val.f = 0.0;
		props[4].prop = JFAudProp_posrel;
		props[4].val.i = 0;	// player-relative
		props[5].prop = JFAudProp_position;
		props[5].val.v[0] = 0.0;
		props[5].val.v[1] = 0.0;
		props[5].val.v[2] = 0.0;
		initprintf("Playing %s player-relative gain=%f pitch=%f loop=%d\n",
				sounds[num], gain,pitch,loop);
	} else {
		props[3].prop = JFAudProp_rolloff;
		props[3].val.f = rolloff;
		props[4].prop = JFAudProp_posrel;
		props[4].val.i = 1;	// world-relative
		props[5].prop = JFAudProp_position;
		props[5].val.v[0] = (float)x/UNITSPERMETRE;
		props[5].val.v[1] = (float)y/UNITSPERMETRE;
		props[5].val.v[2] = (float)z/UNITSPERMETRE;
		initprintf("Playing %s 3D gain=%f pitch=%f rolloff=%f loop=%d global=%d at %f,%f,%f\n",
				sounds[num], gain,pitch,rolloff,loop,global,
				(float)x/UNITSPERMETRE,(float)y/UNITSPERMETRE,(float)z/UNITSPERMETRE
		    );
	}
	err = JFAud_playsound(&handl, sounds[num], NULL, soundpr[num], JFAudPlayMode_playing, sizeof(props)/sizeof(JFAudProp), props);
	if (err != jfauderr_ok) {
		initprintf("Playback of sound %s failed: %s\n",sounds[num], JFAud_errstr(err));
		return -1;
	}

	keephandle(handl, num, i);
	Sound[num].num++;
	
	return 0;
}

void sound(short num)
{
	float pitch=1.0;
	char loop=0;
	int handl;
	JFAudProp props[6];
	jfauderr err;
	
	if (!inited ||
	    num >= NUM_SOUNDS ||
	    SoundToggle == 0 ||
	    ((soundm[num] & 4) && VoiceToggle == 0) ||
	    ((soundm[num] & 8) && ud.lockout)	// parental mode
	   ) return;

	{
		short pitchvar, pitchbase;
		
		pitchvar = klabs(soundpe[num] - soundps[num]);
		if (pitchvar > 0) {
			if (soundps[num] < soundpe[num])
				pitchbase = soundps[num];
			else
				pitchbase = soundpe[num];
			pitch = translatepitch(pitchbase + (rand() % pitchvar));
		} else {
			pitch = translatepitch(soundps[num]);
		}
	}

	if (soundm[num] & 1) loop = 1;

	props[0].prop = JFAudProp_pitch;
	props[0].val.f = pitch;
	props[1].prop = JFAudProp_gain;
	props[1].val.f = 1.0;
	props[2].prop = JFAudProp_fx;
	props[2].val.i = 1|2;	// 1=nearest, 2=stereo2mono
	props[3].prop = JFAudProp_rolloff;
	props[3].val.f = 0.0;
	props[4].prop = JFAudProp_posrel;
	props[4].val.i = 0;	// player-relative
	props[5].prop = JFAudProp_position;
	props[5].val.v[0] = 0.0;
	props[5].val.v[1] = 0.0;
	props[5].val.v[2] = 0.0;
	
	initprintf("Playing %s pitch=%f loop=%d\n", sounds[num], pitch,loop);
	err = JFAud_playsound(&handl, sounds[num], NULL, soundpr[num], JFAudPlayMode_playing, sizeof(props)/sizeof(JFAudProp), props);
	if (err != jfauderr_ok) {
		initprintf("Playback of sound %s failed: %s\n",sounds[num], JFAud_errstr(err));
		return;
	}

	keephandle(handl, num, -1);
	Sound[num].num++;
}

int spritesound(unsigned short num, short i)
{
	if (num >= NUM_SOUNDS) return -1;
	return xyzsound(num,i,SX,SY,SZ);
}

void stopsound(short num)
{
}

void stopenvsound(short num, short i)
{
	int j;
	
	for (j=AbsoluteMaxVoices-1;j>=0;j--) {
		if (sfx[j].handle < 0) continue;
		if (sfx[j].owner != i) continue;

		JFAud_killsound(sfx[j].handle);	// release the channel
		sfx[j].handle = -1;
		Sound[sfx[j].soundnum].num--;
	}
}

void pan3dsound(void)
{
	int j;
	short i;
	long cx, cy, cz;
	short ca,cs;
	JFAudProp props[2];
	jfauderr err;

	if (!inited) return;

	if(ud.camerasprite == -1) {
		cx = ps[screenpeek].oposx;
		cy = ps[screenpeek].oposy;
		cz = ps[screenpeek].oposz;
		cs = ps[screenpeek].cursectnum;
		ca = ps[screenpeek].ang+ps[screenpeek].look_ang;
	} else {
		cx = sprite[ud.camerasprite].x;
		cy = sprite[ud.camerasprite].y;
		cz = sprite[ud.camerasprite].z;
		cs = sprite[ud.camerasprite].sectnum;
		ca = sprite[ud.camerasprite].ang;
	}

	cx = -cx;
	cz = (cz>>4);

	props[0].prop = JFAudProp_position;
	props[0].val.v[0] = (float)cx/UNITSPERMETRE;
	props[0].val.v[1] = (float)cy/UNITSPERMETRE;
	props[0].val.v[2] = (float)cz/UNITSPERMETRE;
	props[1].prop = JFAudProp_orient;
	props[1].val.v2[0] = (float)sintable[(ca-512)&2047]/16384.0;
	props[1].val.v2[1] = (float)sintable[ca&2047]/16384.0;
	props[1].val.v2[2] = 0.0;
	props[1].val.v2[3] = 0.0;
	props[1].val.v2[4] = 0.0;
	props[1].val.v2[5] = 1.0;

	/*
	initprintf("Player at %f,%f,%f facing %f,%f,%f\n",
			(float)cx/UNITSPERMETRE,(float)cy/UNITSPERMETRE,(float)cz/UNITSPERMETRE,
			(float)sintable[ca]/16384.0, (float)sintable[(ca+512)&2047]/16384.0, 0.0
		    );
	*/
	err = JFAud_setlistenerprops(sizeof(props)/sizeof(JFAudProp), props);
	if (err != jfauderr_ok) {
		OSD_Printf("JFAud_setlistenerprops() error %s\n", JFAud_errstr(err));
	}

	for (j=AbsoluteMaxVoices-1;j>=0;j--) {
		float gain=1.0;
		
		if (sfx[j].owner < 0 || sfx[j].handle < 0) continue;

		i = sfx[j].owner;

		cx = sprite[i].x;
		cy = sprite[i].y;
		cz = sprite[i].z;

		cx = -cx;
		cz = (cz>>4);

		// A sound may move from player-relative 3D if the viewpoint shifts from the player
		// through a viewscreen or viewpoint switching
		if (PN == APLAYER && sprite[i].yvel == screenpeek) {
			props[0].prop = JFAudProp_position;
			props[0].val.v[0] = 0.0;
			props[0].val.v[1] = 0.0;
			props[0].val.v[2] = 0.0;
		} else {
			props[0].prop = JFAudProp_position;
			props[0].val.v[0] = (float)cx/UNITSPERMETRE;
			props[0].val.v[1] = (float)cy/UNITSPERMETRE;
			props[0].val.v[2] = (float)cz/UNITSPERMETRE;

			gain += (float)soundvo[sfx[j].soundnum]/(150.0*64.0);
			if (gain < 0.0) gain = 0.0; else if (gain > 1.0) gain = 1.0;

			// fake some occlusion
			if( gain > 0.0 && PN != MUSICANDSFX && !cansee(cx,cy,cz-(24<<8),cs,SX,SY,SZ-(24<<8),SECT) )
				gain *= 0.8;
		}
		props[1].prop = JFAudProp_gain;
		props[1].val.f = gain;

		err = JFAud_setsoundprops(sfx[j].handle, sizeof(props)/sizeof(JFAudProp), props);
		if (err != jfauderr_ok) {
			OSD_Printf("JFAud_setsoundprops() error %s\n", JFAud_errstr(err));
		}
	}
}

void testcallback(unsigned long num)
{
}

void clearsoundlocks(void)
{
}

void FX_SetVolume( int volume )
{
}

void FX_SetReverseStereo( int setting )
{
}

void FX_SetReverb( int reverb )
{
}

void FX_SetReverbDelay( int delay )
{
}

int FX_VoiceAvailable( int priority )
{
	return 0;
}

int FX_PlayVOC3D( char *ptr, int pitchoffset, int angle, int distance,
       int priority, unsigned long callbackval )
{
	return 0;
}

int FX_PlayWAV3D( char *ptr, int pitchoffset, int angle, int distance,
       int priority, unsigned long callbackval )
{
	return 0;
}

int FX_StopSound( int handle )
{
	return 0;
}

int FX_StopAllSounds( void )
{
	int j;
	
	for (j=AbsoluteMaxVoices-1;j>=0;j--) {
		if (sfx[j].handle < 0) continue;

		JFAud_killsound(sfx[j].handle);	// release the channel
		sfx[j].handle = -1;
		Sound[sfx[j].soundnum].num--;
	}

	return 0;
}

void MUSIC_SetVolume( int volume )
{
}

void MUSIC_Pause( void )
{
	if (inited) JFAud_setmusicplaymode(JFAudPlayMode_paused);
}

void MUSIC_Continue( void )
{
	if (inited) JFAud_setmusicplaymode(JFAudPlayMode_playing);
}

int MUSIC_StopSong( void )
{
	if (inited) JFAud_setmusicplaymode(JFAudPlayMode_stopped);

	return 0;
}

void MUSIC_RegisterTimbreBank( unsigned char *timbres )
{
}




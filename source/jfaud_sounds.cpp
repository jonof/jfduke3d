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

#include <jfaud/jfaud.hpp>

#include "types.h"
extern "C" {
#include "duke3d.h"
#include "osd.h"
	long numenvsnds;
}

#include <cmath>

class KenFile : public JFAudFile {
private:
	int fh;
public:
	KenFile(const char *filename, const char *subfilename)
		: JFAudFile(filename, subfilename)
	{
		fh = kopen4load(const_cast<char*>(filename), 0);
	}

	virtual ~KenFile()
	{
		if (fh >= 0) kclose(fh);
	}

	virtual bool IsOpen(void) const { return fh >= 0; }

	virtual int Read(int nbytes, void *buf)
	{
		if (fh < 0) return -1;
		return kread(fh, buf, nbytes);
	}
	virtual int Seek(int pos, SeekFrom where)
	{
		int when;
		if (fh < 0) return -1;
		switch (where) {
			case JFAudFile::Set: when = SEEK_SET; break;
			case JFAudFile::Cur: when = SEEK_CUR; break;
			case JFAudFile::End: when = SEEK_END; break;
			default: return -1;
		}
		return klseek(fh, pos, when);
	}
	virtual int Tell(void) const
	{
		if (fh < 0) return -1;
		return klseek(fh, 0, SEEK_CUR);
	}
	virtual int Length(void) const
	{
		if (fh < 0) return -1;
		return kfilelength(fh);
	}
};

static JFAudFile *openfile(const char *fn, const char *subfn)
{
	return static_cast<JFAudFile*>(new KenFile(fn,subfn));
}

static void logfunc(const char *s) { initprintf("%s", s); }

typedef struct {
	JFAudMixerChannel *chan;
	int owner;	// sprite number
	int soundnum;	// sound number
} SoundChannel;

static SoundChannel *chans = NULL;
static JFAud *jfaud = NULL;

static bool havemidi = false;


static void stopcallback(int r)
{
	jfaud->FreeSound(chans[r].chan);
	chans[r].chan = NULL;
	chans[r].owner = -1;
	
	testcallback(chans[r].soundnum);
}

void testcallback(unsigned long num)
{
	short tempi,tempj,tempk;
	
	if ((long)num < 0) {
		if(lumplockbyte[-num] >= 200) lumplockbyte[-num]--;
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
	
	Sound[num].lock--;
}

static int keephandle(JFAudMixerChannel *handle, int soundnum, int owner)
{
	int i, freeh=-1;
	for (i=NumVoices-1;i>=0;i--) {
		if (!chans[i].chan && freeh<0) freeh=i;
		else if (chans[i].chan == handle) { freeh=i; break; }
	}
	if (freeh<0) {
		OSD_Printf("Warning: keephandle() exhausted handle space!\n");
		return -1;
	}

	if (chans[freeh].chan >= 0) Sound[soundnum].num--;

	chans[freeh].chan = handle;
	chans[freeh].soundnum = soundnum;
	chans[freeh].owner = owner;
	
	return freeh;
}

void SoundStartup(void)
{
	int i;

	if (jfaud) return;

	JFAud_SetLogFunc(logfunc);

	jfaud = new JFAud();
	if (!jfaud) return;

	jfaud->SetUserOpenFunc(openfile);

	if (!jfaud->InitWave(NULL, NumVoices, MixRate)) {
		delete jfaud;
		jfaud = NULL;
		return;
	}
	
	chans = new SoundChannel[NumVoices];
	if (!chans) {
		delete jfaud;
		jfaud = NULL;
		return;
	}

	for (i=NumVoices-1; i>=0; i--) {
		chans[i].owner = -1;
	}
	
	if (jfaud->InitMIDI(NULL)) havemidi = true;
}

void SoundShutdown(void)
{
	if (jfaud) delete jfaud;
	if (chans) delete [] chans;
	jfaud = NULL;
	chans = NULL;
	havemidi = false;
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

	if (!jfaud) return;
	for (i=NumVoices-1; i>=0; i--) {
		if (chans[i].chan && !jfaud->IsValidSound(chans[i].chan))
			chans[i].chan = NULL;
	}
	jfaud->Update();
}


// soundm&1 = loop
// soundm&2 = musicandsfx
// soundm&4 = duke speech
// soundm&8 = parental lockout
// soundm&16 = no rolloff
// soundm&128 = player local

#define UNITSPERMETRE 512.0


static char menunum = 0;
void intomenusounds(void)
{
	short i;
	short menusnds[] = {
		LASERTRIP_EXPLODE,	DUKE_GRUNT,			DUKE_LAND_HURT,
		CHAINGUN_FIRE,		SQUISHED,			KICK_HIT,
		PISTOL_RICOCHET,	PISTOL_BODYHIT,		PISTOL_FIRE,
		SHOTGUN_FIRE,		BOS1_WALK,			RPG_EXPLODE,
		PIPEBOMB_BOUNCE,	PIPEBOMB_EXPLODE,	NITEVISION_ONOFF,
		RPG_SHOOT,			SELECT_WEAPON
	};
	sound(menusnds[menunum++]);
	menunum %= sizeof(menusnds)/sizeof(menusnds[0]);
}

void playmusic(char *fn)
{
	char dafn[BMAX_PATH], *dotpos;
	int i;
	const char *extns[] = { ".ogg",".mp3",".mid", NULL };

	if (!jfaud) return;

	dotpos = Bstrrchr(fn,'.');
	if (dotpos && Bstrcasecmp(dotpos,".mid")) {
		// has extension but isn't midi
		jfaud->PlayMusic(fn, NULL);
	} else {
		Bstrcpy(dafn,fn);
		dotpos = Bstrrchr(dafn,'.');
		if (!dotpos) dotpos = dafn+strlen(dafn);

		for (i=0; extns[i]; i++) {
			if (!havemidi && !Bstrcmp(extns[i],".mid")) continue;
			Bstrcpy(dotpos, extns[i]);
			if (jfaud->PlayMusic(dafn, NULL)) return;
		}
	}
}

char loadsound(unsigned short num) { return 1; }

int isspritemakingsound(short i, int num)	// if num<0, check if making any sound at all
{
	int j;
	
	if (!jfaud) return -1;
	for (j=NumVoices-1; j>=0; j--) {
		if (!chans[j].chan || !jfaud->IsValidSound(chans[j].chan)) continue;
		if (chans[j].owner == i)
			if (num < 0 || (/*num >= 0 &&*/ chans[j].soundnum == num))
				return chans[j].soundnum;
	}

	return -1;
}

int xyzsound(short num, short i, long x, long y, long z)
{
	JFAudMixerChannel *chan;
	int r, global = 0;
	float gain = 1.0;

	if (!jfaud ||
	    num >= NUM_SOUNDS ||
	    ((soundm[num] & 8) && ud.lockout) ||	// parental mode
	    SoundToggle == 0 ||
	    (ps[myconnectindex].timebeforeexit > 0 && ps[myconnectindex].timebeforeexit <= 26*3) ||
	    (ps[myconnectindex].gm & MODE_MENU)
	   ) return -1;

	x = -x;
	z >>= 4;
	
	if (soundm[num] & 128) {
		sound(num);
		return 0;
	}
	
	if (soundm[num] & 4) {
		// Duke speech, one at a time only
		int j;
		
		if (VoiceToggle == 0 ||
		    (ud.multimode > 1 && PN == APLAYER && sprite[i].yvel != screenpeek && ud.coop != 1)
		   ) return -1;

		for (j=NumVoices-1; j>=0; j--) {
			if (!chans[j].chan || chans[j].owner < 0) continue;
			if (soundm[ chans[j].soundnum ] & 4) return -1;
		}
	}
	
	// XXX: here goes musicandsfx ranging
	
	// XXX: pitch
	
	// XXX: gain
	
	// XXX: occlusion fakery
	
	switch(num)
	{
		case PIPEBOMB_EXPLODE:
		case LASERTRIP_EXPLODE:
		case RPG_EXPLODE:
			gain = 1.0;
			global = 1;
			//if (sector[ps[screenpeek].cursectnum].lotag == 2) pitch -= 1024;
			break;
		default:
			//if(sector[ps[screenpeek].cursectnum].lotag == 2 && (soundm[num]&4) == 0)
			//	pitch = -768;
			//if( sndist > 31444 && PN != MUSICANDSFX)
			//	return -1;
			break;
	}

	// XXX: this is shit
	if( Sound[num].num > 0 && PN != MUSICANDSFX )
	{
		if( SoundOwner[num][0].i == i ) stopsound(num);
		else if( Sound[num].num > 1 ) stopsound(num);
		else if( badguy(&sprite[i]) && sprite[i].extra <= 0 ) stopsound(num);
	}
	
	chan = jfaud->PlaySound(sounds[num], NULL, soundpr[num]);
	if (!chan) return -1;
	
	chan->SetGain(gain);
	// chan->SetPitch(pitch);
	if (soundm[num] & 1) chan->SetLoop(true);
	if (soundm[num] & 16) global = 1;
	
	if (PN == APLAYER && sprite[i].yvel == screenpeek) {
		chan->SetRolloff(0.0);
		chan->SetFollowListener(true);
		chan->SetPosition(0.0, 0.0, 0.0);
	} else {
		chan->SetRolloff(global ? 0.0 : 1.0);
		chan->SetFollowListener(false);
		chan->SetPosition((float)x/UNITSPERMETRE, (float)y/UNITSPERMETRE, (float)z/UNITSPERMETRE);
	}
	
	r = keephandle(chan, num, i);
	if (r >= 0) chan->SetStopCallback(stopcallback, r);
	chan->Play();
	
	Sound[num].num++;

	return 0;
}

void sound(short num)
{
	JFAudMixerChannel *chan;
	int r;

	if (!jfaud ||
	    num >= NUM_SOUNDS ||
	    SoundToggle == 0 ||
	    ((soundm[num] & 4) && VoiceToggle == 0) ||
	    ((soundm[num] & 8) && ud.lockout)	// parental mode
	   ) return;

	// XXX: pitch

	chan = jfaud->PlaySound(sounds[num], NULL, soundpr[num]);
	if (!chan) return;

	chan->SetGain(1.0);
	// chan->SetPitch(pitch);
	if (soundm[num] & 1) chan->SetLoop(true);
	chan->SetRolloff(0.0);
	chan->SetFollowListener(true);
	chan->SetPosition(0.0, 0.0, 0.0);

	r = keephandle(chan, num, -1);
	if (r >= 0) chan->SetStopCallback(stopcallback, r);
	chan->Play();

	Sound[num].num++;
}

int spritesound(unsigned short num, short i)
{
	if (num >= NUM_SOUNDS) return -1;
	return xyzsound(num,i,SX,SY,SZ);
}

void stopsound(short num)
{
	int j;
	
	for (j=NumVoices-1;j>=0;j--) {
		if (!chans[j].chan || !jfaud->IsValidSound(chans[j].chan) || chans[j].soundnum != num) continue;
		
		jfaud->FreeSound(chans[j].chan);
		chans[j].chan = NULL;
		chans[j].owner = -1;
		Sound[num].num--;
	}
}

void stopenvsound(short num, short i)
{
	int j;
	
	for (j=NumVoices-1;j>=0;j--) {
		if (!chans[j].chan || !jfaud->IsValidSound(chans[j].chan) || chans[j].owner != i) continue;

		jfaud->FreeSound(chans[j].chan);
		chans[j].chan = NULL;
		chans[j].owner = -1;
		Sound[chans[j].soundnum].num--;
	}
}

void pan3dsound(void)
{
	JFAudMixer *mix;
	int j, global=0;
	short i;
	long cx, cy, cz;
	short ca,cs;
	float gain = 1.0;

	numenvsnds = 0;
	if (!jfaud) return;

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
	cz >>= 4;

	mix = jfaud->GetWave();
	mix->SetListenerPosition((float)cx/UNITSPERMETRE, (float)cy/UNITSPERMETRE, (float)cz/UNITSPERMETRE);
	mix->SetListenerOrientation((float)sintable[(ca-512)&2047]/16384.0, (float)sintable[ca&2047]/16384.0, 0.0,
		0.0, 0.0, 1.0);
	
	for (j=NumVoices-1; j>=0; j--) {
		if (!chans[j].chan || !jfaud->IsValidSound(chans[j].chan) || chans[j].owner < 0) continue;

		i = chans[j].owner;

		cx = sprite[i].x;
		cy = sprite[i].y;
		cz = sprite[i].z;

		cx = -cx;
		cz >>= 4;

		if(PN == MUSICANDSFX && SLT < 999) numenvsnds++;
		if( soundm[j]&16 ) global = 1;
		
		switch(j) {
			case PIPEBOMB_EXPLODE:
			case LASERTRIP_EXPLODE:
			case RPG_EXPLODE:
				gain = 1.0;
				global = 1;
				break;
			default:
				//if( sndist > 31444 && PN != MUSICANDSFX) {
				//	stopsound(j);
				//	continue;
				//}
				break;
		}
		
		// A sound may move from player-relative 3D if the viewpoint shifts from the player
		// through a viewscreen or viewpoint switching
		if (PN == APLAYER && sprite[i].yvel == screenpeek) {
			chans[j].chan->SetRolloff(0.0);
			chans[j].chan->SetFollowListener(true);
			chans[j].chan->SetPosition(0.0, 0.0, 0.0);
		} else {
			chans[j].chan->SetRolloff(global ? 0.0 : 1.0);
			chans[j].chan->SetFollowListener(false);
			chans[j].chan->SetPosition((float)cx/UNITSPERMETRE, (float)cy/UNITSPERMETRE, (float)cz/UNITSPERMETRE);

			// XXX: gain and occlusion
		}
	}
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
	int j;
	
	for (j=NumVoices-1;j>=0;j--) {
		if (!chans[j].chan || !jfaud->IsValidSound(chans[j].chan)) return 1;
	}
	return 0;
}

int FX_PlayVOC3D( char *ptr, int pitchoffset, int angle, int distance,
       int priority, unsigned long callbackval )
{
	printf("FX_PlayVOC3D()\n");
	return 0;
}

int FX_PlayWAV3D( char *ptr, int pitchoffset, int angle, int distance,
       int priority, unsigned long callbackval )
{
	printf("FX_PlayWAV3D()\n");
	return 0;
}

int FX_StopSound( int handle )
{
	printf("FX_StopSound()\n");
	return 0;
}

int FX_StopAllSounds( void )
{
	int j;
	
	for (j=NumVoices-1; j>=0; j--) {
		if (!chans[j].chan || !jfaud->IsValidSound(chans[j].chan)) continue;

		jfaud->FreeSound(chans[j].chan);
		chans[j].chan = NULL;
		chans[j].owner = -1;
		Sound[chans[j].soundnum].num--;
	}

	return 0;
}

void MUSIC_SetVolume( int volume )
{
}

void MUSIC_Pause( void )
{
	if (jfaud) jfaud->PauseMusic(true);
}

void MUSIC_Continue( void )
{
	if (jfaud) jfaud->PauseMusic(false);
}

int MUSIC_StopSong( void )
{
	if (jfaud) jfaud->StopMusic();
	return 0;
}

void MUSIC_RegisterTimbreBank( unsigned char *timbres )
{
}


#include "osdcmds.h"
#include "osd.h"
#include "baselayer.h"
#include "duke3d.h"
#include "crc32.h"

struct osdcmd_cheatsinfo osdcmd_cheatsinfo_stat;

int osdcmd_quit(const osdfuncparm_t *parm)
{
	parm=parm;
	if (numplayers > 1) {
		if(!(ps[myconnectindex].gm&MODE_GAME))
			sendlogoff();
	}
	gameexit(" ");

	return OSDCMD_OK;
}

int osdcmd_echo(const osdfuncparm_t *parm)
{
	const char *c;

	for (c = parm->raw; *c && *c != ' '; c++) ;
	for (; *c && *c == ' '; c++) ;
	OSD_Printf("%s\n", c);
	
	return OSDCMD_OK;
}

int osdcmd_changelevel(const osdfuncparm_t *parm)
{
	int volume=0,level,i;
	char *p;

	if (!VOLUMEONE) {
		volume = strtol(parm->parms[0], &p, 10) - 1;
		if (p[0]) return OSDCMD_SHOWHELP;
		level = strtol(parm->parms[1], &p, 10) - 1;
		if (p[0]) return OSDCMD_SHOWHELP;
	} else {
		level = strtol(parm->parms[0], &p, 10) - 1;
		if (p[0]) return OSDCMD_SHOWHELP;
	}

	if (volume < 0) return OSDCMD_SHOWHELP;
	if (level < 0) return OSDCMD_SHOWHELP;

	if (!VOLUMEONE) {
		if (PLUTOPAK) {
			if (volume > 3) {
				OSD_Printf("Error: invalid volume number (range 1-4)\n");
				return OSDCMD_OK;
			}
		} else {
			if (volume > 2) {
				OSD_Printf("Error: invalid volume number (range 1-3)\n");
				return OSDCMD_OK;
			}
		}
	}

	if (volume == 0) {
		if (level > 5) {
			OSD_Printf("Error: invalid volume 1 level number (range 1-6)\n");
			return OSDCMD_OK;
		}
	} else {
		if (level > 10) {
			OSD_Printf("Error: invalid volume 2+ level number (range 1-11)\n");
			return OSDCMD_SHOWHELP;
		}
	}

	if (ps[myconnectindex].gm & MODE_GAME) {
		// in-game behave like a cheat
		osdcmd_cheatsinfo_stat.cheatnum = 2;
		osdcmd_cheatsinfo_stat.volume   = volume;
		osdcmd_cheatsinfo_stat.level    = level;
	} else {
		// out-of-game behave like a menu command
		osdcmd_cheatsinfo_stat.cheatnum = -1;
		
		ud.m_volume_number = volume;
		ud.m_level_number = level;
		
                ud.m_monsters_off = ud.monsters_off = 0;

                ud.m_respawn_items = 0;
                ud.m_respawn_inventory = 0;

                ud.multimode = 1;

                newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);
                enterlevel(MODE_GAME);
	}

	return OSDCMD_OK;
}

int osdcmd_map(const osdfuncparm_t *parm)
{
	int i;
	char filename[256];

	strcpy(filename,parm->parms[0]);
	if( strchr(filename,'.') == 0)
        	strcat(filename,".map");

	if ((i = kopen4load(filename,0)) < 0) {
		OSD_Printf("Error: Map file \"%s\" does not exist.\n", filename);
		return OSDCMD_OK;
	}
	kclose(i);

	strcpy(boardfilename, filename);
	
	if (ps[myconnectindex].gm & MODE_GAME) {
		// in-game behave like a cheat
		osdcmd_cheatsinfo_stat.cheatnum = 2;
		osdcmd_cheatsinfo_stat.volume = 0;
		osdcmd_cheatsinfo_stat.level = 7;
	} else {
		// out-of-game behave like a menu command
		osdcmd_cheatsinfo_stat.cheatnum = -1;

		ud.m_volume_number = 0;
		ud.m_level_number = 7;

                ud.m_monsters_off = ud.monsters_off = 0;

                ud.m_respawn_items = 0;
                ud.m_respawn_inventory = 0;

                ud.multimode = 1;
		
	 	newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);
	        enterlevel(MODE_GAME);
	}
       
	return OSDCMD_OK;
}

int osdcmd_god(const osdfuncparm_t *parm)
{
	parm=parm;
	if (ps[myconnectindex].gm & MODE_GAME) {
		osdcmd_cheatsinfo_stat.cheatnum = 0;
	} else {
		OSD_Printf("Can't toggle God mode. Not in a game.\n");
	}
	
	return OSDCMD_OK;
}

int osdcmd_noclip(const osdfuncparm_t *parm)
{
	parm=parm;
	if (ps[myconnectindex].gm & MODE_GAME) {
		osdcmd_cheatsinfo_stat.cheatnum = 20;
	} else {
		OSD_Printf("Can't toggle clipping mode. Not in a game.\n");
	}
	
	return OSDCMD_OK;
}

int osdcmd_fileinfo(const osdfuncparm_t *parm)
{
	unsigned long crc, length;
	int i,j;
	char buf[256];

	if ((i = kopen4load((char *)parm->parms[0],0)) < 0) {
		OSD_Printf("Error: File \"%s\" does not exist.\n", parm->parms[0]);
		return OSDCMD_OK;
	}

	length = kfilelength(i);

	crc32init(&crc);
	do {
		j = kread(i,buf,256);
		crc32block(&crc,buf,j);
	} while (j == 256);
	crc32finish(&crc);
	
	kclose(i);

	OSD_Printf("File: %s\n"
	           "  File size: %d\n"
		   "  CRC-32:    %08X\n",
		   parm->parms[0], length, crc);

	return OSDCMD_OK;
}

static int osdcmd_restartvid(const osdfuncparm_t *parm)
{
	extern long qsetmode;
	
	resetvideomode();
	if (setgamemode(ScreenMode,ScreenWidth,ScreenHeight,ScreenBPP))
		OSD_Printf("restartvid: Reset failed...\n");
	vscrn();

	return OSDCMD_OK;
}

static int osdcmd_vidmode(const osdfuncparm_t *parm)
{
	if (parm->numparms < 1 || parm->numparms > 3) return OSDCMD_SHOWHELP;

	switch (parm->numparms) {
		case 1:	// bpp switch
			ScreenBPP = Batol(parm->parms[0]);
			break;
		case 2: // res switch
			ScreenWidth = Batol(parm->parms[0]);
			ScreenHeight = Batol(parm->parms[1]);
			break;
		case 3:	// res & bpp switch
			ScreenWidth = Batol(parm->parms[0]);
			ScreenHeight = Batol(parm->parms[1]);
			ScreenBPP = Batol(parm->parms[2]);
			break;
	}

	if (setgamemode(fullscreen,ScreenWidth,ScreenHeight,ScreenBPP))
		OSD_Printf("vidmode: Mode change failed!\n");
	vscrn();
	return OSDCMD_OK;
}

static int osdcmd_setstatusbarscale(const osdfuncparm_t *parm)
{
	if (parm->numparms == 0) {
		OSD_Printf("Status bar scale is %d%%\n", ud.statusbarscale);
		return OSDCMD_OK;
	} else if (parm->numparms != 1) return OSDCMD_SHOWHELP;

	setstatusbarscale(Batol(parm->parms[0]));
	OSD_Printf("Status bar scale set to %d%%\n", ud.statusbarscale);
	return OSDCMD_OK;
}


int osdcmd_validatebool(void *a)
{
	int *i = (int *)a;

	if (*i>0) *i = 1;
	else *i = 0;

	return 1;
}

static void onvideomodechange(int newmode)
{
	char *pal;

	if (newmode) {
		if (ps[screenpeek].palette == palette ||
		    ps[screenpeek].palette == waterpal ||
		    ps[screenpeek].palette == slimepal)
			pal = palette;
		else
			pal = ps[screenpeek].palette;
	} else {
		pal = ps[screenpeek].palette;
	}

	setbrightness(ud.brightness>>2, pal, 0);
	restorepalette = 1;
}

int registerosdcommands(void)
{
	osdcmd_cheatsinfo_stat.cheatnum = -1;

	OSD_RegisterFunction("echo",0,"echo [text]: echoes text to the console", osdcmd_echo);

if (VOLUMEONE) {
	OSD_RegisterFunction("changelevel",1,"changelevel <level>: warps to the given level", osdcmd_changelevel);
} else {
	OSD_RegisterFunction("changelevel",2,"changelevel <volume> <level>: warps to the given level", osdcmd_changelevel);
	OSD_RegisterFunction("map",1,"map <mapfile>: loads the given user map", osdcmd_map);
}
	OSD_RegisterFunction("god",0,"god: toggles god mode", osdcmd_god);
	OSD_RegisterFunction("noclip",0,"noclip: toggles clipping mode", osdcmd_noclip);

	OSD_RegisterFunction("setstatusbarscale",0,"setstatusbarscale [percent]: changes the status bar scale", osdcmd_setstatusbarscale);
	
	OSD_RegisterFunction("fileinfo",1,"fileinfo <file>: gets a file's information", osdcmd_fileinfo);
	OSD_RegisterFunction("quit",0,"quit: exits the game immediately", osdcmd_quit);

	OSD_RegisterVariable("myname",OSDVAR_STRING,myname,32,NULL);
	OSD_RegisterVariable("showfps",OSDVAR_INTEGER,&ud.tickrate,1,osdcmd_validatebool);
	OSD_RegisterVariable("showcoords",OSDVAR_INTEGER,&ud.coords,1,osdcmd_validatebool);

	OSD_RegisterVariable("bpp", OSDVAR_INTEGER, &ScreenBPP, 0, osd_internal_validate_integer);
	OSD_RegisterFunction("restartvid",0,"restartvid: reinitialised the video mode",osdcmd_restartvid);
	OSD_RegisterFunction("vidmode",1,"vidmode [xdim ydim] [bpp]: immediately change the video mode",osdcmd_vidmode);
	
	baselayer_onvideomodechange = onvideomodechange;

	return 0;
}


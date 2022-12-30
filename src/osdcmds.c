#include "duke3d.h"
#include "osdcmds.h"
#include "osd.h"
#include "crc32.h"

#include <ctype.h>

struct osdcmd_cheatsinfo osdcmd_cheatsinfo_stat;

int osdcmd_quit(const osdfuncparm_t *parm)
{
    extern int quittimer;
    (void)parm;
    if (!gamequit && numplayers > 1) {
        if((ps[myconnectindex].gm&MODE_GAME)) {
            gamequit = 1;
            quittimer = totalclock+120;
        } else {
            sendlogoff();
            gameexit(" ");
        }
    } else if (numplayers < 2)
        gameexit(" ");

    return OSDCMD_OK;
}

int osdcmd_changelevel(const osdfuncparm_t *parm)
{
    int volume=0,level;
    char *p;

    if (!VOLUMEONE) {
        if (parm->numparms != 2) return OSDCMD_SHOWHELP;

        volume = (int)strtol(parm->parms[0], &p, 10) - 1;
        if (p[0]) return OSDCMD_SHOWHELP;
        level = (int)strtol(parm->parms[1], &p, 10) - 1;
        if (p[0]) return OSDCMD_SHOWHELP;
    } else {
        if (parm->numparms != 1) return OSDCMD_SHOWHELP;

        level = (int)strtol(parm->parms[0], &p, 10) - 1;
        if (p[0]) return OSDCMD_SHOWHELP;
    }

    if (volume < 0) return OSDCMD_SHOWHELP;
    if (level < 0) return OSDCMD_SHOWHELP;

    if (!VOLUMEONE) {
        if (PLUTOPAK) {
            if (volume > 3) {
                buildprintf("changelevel: invalid volume number (range 1-4)\n");
                return OSDCMD_OK;
            }
        } else {
            if (volume > 2) {
                buildprintf("changelevel: invalid volume number (range 1-3)\n");
                return OSDCMD_OK;
            }
        }
    }

    if (volume == 0) {
        if (level > 5) {
            buildprintf("changelevel: invalid volume 1 level number (range 1-6)\n");
            return OSDCMD_OK;
        }
    } else {
        if (level > 10) {
            buildprintf("changelevel: invalid volume 2+ level number (range 1-11)\n");
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
                if (enterlevel(MODE_GAME)) backtomenu();
    }

    return OSDCMD_OK;
}

int osdcmd_map(const osdfuncparm_t *parm)
{
    int i;
    char filename[256];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;
    
    strcpy(filename,parm->parms[0]);
    if( strchr(filename,'.') == 0)
            strcat(filename,".map");

    if ((i = kopen4load(filename,0)) < 0) {
        buildprintf("map: file \"%s\" does not exist.\n", filename);
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
                if (enterlevel(MODE_GAME)) backtomenu();
    }
       
    return OSDCMD_OK;
}

int osdcmd_god(const osdfuncparm_t *parm)
{
    (void)parm;
    if (numplayers == 1 && ps[myconnectindex].gm & MODE_GAME) {
        osdcmd_cheatsinfo_stat.cheatnum = 0;
    } else {
        buildprintf("god: Not in a single-player game.\n");
    }
    
    return OSDCMD_OK;
}

int osdcmd_noclip(const osdfuncparm_t *parm)
{
    (void)parm;
    if (numplayers == 1 && ps[myconnectindex].gm & MODE_GAME) {
        osdcmd_cheatsinfo_stat.cheatnum = 20;
    } else {
        buildprintf("noclip: Not in a single-player game.\n");
    }
    
    return OSDCMD_OK;
}

int osdcmd_fileinfo(const osdfuncparm_t *parm)
{
    unsigned int crc, length;
    int i,j;
    unsigned char buf[256];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;
    
    if ((i = kopen4load(parm->parms[0],0)) < 0) {
        buildprintf("fileinfo: File \"%s\" does not exist.\n", parm->parms[0]);
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

    buildprintf("fileinfo: %s\n"
               "  File size: %d\n"
           "  CRC-32:    %08X\n",
           parm->parms[0], length, crc);

    return OSDCMD_OK;
}

static int osdcmd_restartvid(const osdfuncparm_t *parm)
{
    (void)parm;
    
    resetvideomode();
    if (setgamemode(ScreenMode,ScreenWidth,ScreenHeight,ScreenBPP))
        gameexit("restartvid: Reset failed...\n");
    onvideomodechange();

    return OSDCMD_OK;
}

static int osdcmd_vidmode(const osdfuncparm_t *parm)
{
    int newbpp = ScreenBPP, newwidth = ScreenWidth,
        newheight = ScreenHeight, newfs = ScreenMode;
    if (parm->numparms < 1 || parm->numparms > 4) return OSDCMD_SHOWHELP;

    switch (parm->numparms) {
        case 1: // bpp switch
            newbpp = atoi(parm->parms[0]);
            break;
        case 2: // res switch
            newwidth = atoi(parm->parms[0]);
            newheight = atoi(parm->parms[1]);
            break;
        case 3: // res & bpp switch
        case 4:
            newwidth = atoi(parm->parms[0]);
            newheight = atoi(parm->parms[1]);
            newbpp = atoi(parm->parms[2]);
            if (parm->numparms == 4)
                newfs = (atoi(parm->parms[3]) != 0);
            break;
    }

    if (setgamemode(newfs,newwidth,newheight,newbpp)) {
        buildprintf("vidmode: Mode change failed!\n");
        if (setgamemode(ScreenMode, ScreenWidth, ScreenHeight, ScreenBPP))
            gameexit("vidmode: Reset failed!\n");
    }
    ScreenBPP = newbpp; ScreenWidth = newwidth; ScreenHeight = newheight;
    ScreenMode = newfs;
    onvideomodechange();
    return OSDCMD_OK;
}

static int osdcmd_setstatusbarscale(const osdfuncparm_t *parm)
{
    if (parm->numparms == 1) {
        ud.statusbarscale = max(1, min(8, atoi(parm->parms[0])));
        setstatusbarscale(ud.statusbarscale);
        vscrn();
    } else if (parm->numparms > 1) return OSDCMD_SHOWHELP;

    buildprintf("setstatusbarscale: scale is %d (%d%%)\n", ud.statusbarscale, 100*ud.statusbarscale/8);
    return OSDCMD_OK;
}

static int osdcmd_spawn(const osdfuncparm_t *parm)
{
    int x=0,y=0,z=0;
    unsigned short cstat=0,picnum=0;
    unsigned char pal=0;
    short ang=0;
    short set=0, idx;

    if (numplayers > 1 || !(ps[myconnectindex].gm & MODE_GAME)) {
        buildprintf("spawn: Can't spawn sprites in multiplayer games or demos\n");
        return OSDCMD_OK;
    }
    
    switch (parm->numparms) {
        case 7: // x,y,z
            x = atoi(parm->parms[4]);
            y = atoi(parm->parms[5]);
            z = atoi(parm->parms[6]);
            set |= 8;
            // fall through
        case 4: // ang
            ang = atoi(parm->parms[3]) & 2047;
            set |= 4;
            // fall through
        case 3: // cstat
            cstat = (unsigned short)atoi(parm->parms[2]);
            set |= 2;
            // fall through
        case 2: // pal
            pal = (unsigned char)atoi(parm->parms[1]);
            set |= 1;
            // fall through
        case 1: // tile number
            if (isdigit(parm->parms[0][0])) {
                picnum = (unsigned short)atoi(parm->parms[0]);
            } else {
                int i,j;
                for (j=0; j<2; j++) {
                    for (i=0; i<labelcnt; i++) {
                        if (
                            (j == 0 && !Bstrcmp(label+(i * MAXLABELLEN),     parm->parms[0])) ||
                            (j == 1 && !Bstrcasecmp(label+(i * MAXLABELLEN), parm->parms[0]))
                           ) {
                            picnum = (unsigned short)labelcode[i];
                            break;
                        }
                    }
                    if (i<labelcnt) break;
                }
                if (i==labelcnt) {
                    buildprintf("spawn: Invalid tile label given\n");
                    return OSDCMD_OK;
                }
            }
            
            if (picnum >= MAXTILES) {
                buildprintf("spawn: Invalid tile number\n");
                return OSDCMD_OK;
            }
            break;
        default:
            return OSDCMD_SHOWHELP;
    }

    idx = spawn(ps[myconnectindex].i, (short)picnum);
    if (set & 1) sprite[idx].pal = (char)pal;
    if (set & 2) sprite[idx].cstat = (short)cstat;
    if (set & 4) sprite[idx].ang = ang;
    if (set & 8) {
        if (setsprite(idx, x,y,z) < 0) {
            buildprintf("spawn: Sprite can't be spawned into null space\n");
            deletesprite(idx);
        }
    }
    
    return OSDCMD_OK;
}

static int osdcmd_vars(const osdfuncparm_t *parm)
{
    int showval = (parm->numparms < 1);
    
    if (!Bstrcasecmp(parm->name, "myname")) {
        if (showval) { buildprintf("Your name is \"%s\"\n", myname); }
        else {
            Bstrncpy(myname, parm->parms[0], sizeof(myname)-1);
            myname[sizeof(myname)-1] = 0;
            // XXX: now send the update over the wire
        }
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->name, "showfps")) {
        if (showval) { buildprintf("showfps is %d\n", ud.tickrate); }
        else ud.tickrate = (atoi(parm->parms[0]) != 0);
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->name, "showcoords")) {
        if (showval) { buildprintf("showcoords is %d\n", ud.coords); }
        else ud.coords = (atoi(parm->parms[0]) != 0);
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->name, "useprecache")) {
        if (showval) { buildprintf("useprecache is %d\n", useprecache); }
        else useprecache = (atoi(parm->parms[0]) != 0);
        return OSDCMD_OK;
    }
    return OSDCMD_SHOWHELP;
}

int osdcmd_usemousejoy(const osdfuncparm_t *parm)
{
    int showval = (parm->numparms < 1);
    if (!Bstrcasecmp(parm->name, "usemouse")) {
        if (showval) { buildprintf("usemouse is %d\n", UseMouse); }
        else {
            UseMouse = (atoi(parm->parms[0]) != 0);
            CONTROL_MouseEnabled = (UseMouse && CONTROL_MousePresent);
        }
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->name, "usejoystick")) {
        if (showval) { buildprintf("usejoystick is %d\n", UseJoystick); }
        else {
            UseJoystick = (atoi(parm->parms[0]) != 0);
            CONTROL_JoystickEnabled = (UseJoystick && CONTROL_JoyPresent);
        }
        return OSDCMD_OK;
    }
    return OSDCMD_SHOWHELP;
}

int registerosdcommands(void)
{
    osdcmd_cheatsinfo_stat.cheatnum = -1;

if (VOLUMEONE) {
    OSD_RegisterFunction("changelevel","changelevel <level>: warps to the given level", osdcmd_changelevel);
} else {
    OSD_RegisterFunction("changelevel","changelevel <volume> <level>: warps to the given level", osdcmd_changelevel);
    OSD_RegisterFunction("map","map <mapfile>: loads the given user map", osdcmd_map);
}
    OSD_RegisterFunction("god","god: toggles god mode", osdcmd_god);
    OSD_RegisterFunction("noclip","noclip: toggles clipping mode", osdcmd_noclip);

    OSD_RegisterFunction("setstatusbarscale","setstatusbarscale [percent]: changes the status bar scale", osdcmd_setstatusbarscale);
    OSD_RegisterFunction("spawn","spawn <picnum> [palnum] [cstat] [ang] [x y z]: spawns a sprite with the given properties",osdcmd_spawn);
    
    OSD_RegisterFunction("fileinfo","fileinfo <file>: gets a file's information", osdcmd_fileinfo);
    OSD_RegisterFunction("quit","quit: exits the game immediately", osdcmd_quit);

    OSD_RegisterFunction("myname","myname: change your multiplayer nickname", osdcmd_vars);
    OSD_RegisterFunction("showfps","showfps: show the frame rate counter", osdcmd_vars);
    OSD_RegisterFunction("showcoords","showcoords: show your position in the game world", osdcmd_vars);
    OSD_RegisterFunction("useprecache","useprecache: enable/disable the pre-level caching routine", osdcmd_vars);

    OSD_RegisterFunction("restartvid","restartvid: reinitialised the video mode",osdcmd_restartvid);
    OSD_RegisterFunction("vidmode","vidmode [xdim ydim] [bpp] [fullscreen]: immediately change the video mode",osdcmd_vidmode);

    OSD_RegisterFunction("usemouse","usemouse: enables input from the mouse if it is present",osdcmd_usemousejoy);
    OSD_RegisterFunction("usejoystick","usejoystick: enables input from the controller if it is present",osdcmd_usemousejoy);
    
    return 0;
}


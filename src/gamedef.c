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
#include <assert.h>

int conversion = 13;    // by default we think we're 1.3d until compilation informs us otherwise

extern short otherp;

static short total_lines,line_number;
static char checking_ifelse,parsing_state;
static short num_squigilly_brackets;

static short g_i,g_p;
static int g_x;
static int *g_t;
static spritetype *g_sp;

static char compilefile[255] = "(none)";    // file we're currently compiling

enum labeltypes {
    LABEL_ANY    = -1,
    LABEL_DEFINE = 1,
    LABEL_STATE  = 2,
    LABEL_ACTOR  = 4,
    LABEL_ACTION = 8,
    LABEL_AI     = 16,
    LABEL_MOVE   = 32,
};

static const char *labeltypenames[] = {
    "define",
    "state",
    "actor",
    "action",
    "ai",
    "move"
};

static void translatelabeltype(int type, char *buf)
{
    int i;
    
    buf[0] = 0;
    for (i=0;i<6;i++) {
        if (!(type & (1<<i))) continue;
        if (buf[0]) Bstrcat(buf, " or ");
        Bstrcat(buf, labeltypenames[i]);
    }
}

#define NUMKEYWORDS     (int)(sizeof(keyw)/sizeof(keyw[0]))

static const char *keyw[/*NUMKEYWORDS*/] =
{
    "definelevelname",  // 0
    "actor",            // 1    [#]
    "addammo",          // 2    [#]
    "ifrnd",            // 3    [C]
    "enda",             // 4    [:]
    "ifcansee",         // 5    [C]
    "ifhitweapon",      // 6    [#]
    "action",           // 7    [#]
    "ifpdistl",         // 8    [#]
    "ifpdistg",         // 9    [#]
    "else",             // 10   [#]
    "strength",         // 11   [#]
    "break",            // 12   [#]
    "shoot",            // 13   [#]
    "palfrom",          // 14   [#]
    "sound",            // 15   [filename.voc]
    "fall",             // 16   []
    "state",            // 17
    "ends",             // 18
    "define",           // 19
    "//",               // 20
    "ifai",             // 21
    "killit",           // 22
    "addweapon",        // 23
    "ai",               // 24
    "addphealth",       // 25
    "ifdead",           // 26
    "ifsquished",       // 27
    "sizeto",           // 28
    "{",                // 29
    "}",                // 30
    "spawn",            // 31
    "move",             // 32
    "ifwasweapon",      // 33
    "ifaction",         // 34
    "ifactioncount",    // 35
    "resetactioncount", // 36
    "debris",           // 37
    "pstomp",           // 38
    "/*",               // 39
    "cstat",            // 40
    "ifmove",           // 41
    "resetplayer",      // 42
    "ifonwater",        // 43
    "ifinwater",        // 44
    "ifcanshoottarget", // 45
    "ifcount",          // 46
    "resetcount",       // 47
    "addinventory",     // 48
    "ifactornotstayput",// 49
    "hitradius",        // 50
    "ifp",              // 51
    "count",            // 52
    "ifactor",          // 53
    "music",            // 54
    "include",          // 55
    "ifstrength",       // 56
    "definesound",      // 57
    "guts",             // 58
    "ifspawnedby",      // 59
    "gamestartup",      // 60
    "wackplayer",       // 61
    "ifgapzl",          // 62
    "ifhitspace",       // 63
    "ifoutside",        // 64
    "ifmultiplayer",    // 65
    "operate",          // 66
    "ifinspace",        // 67
    "debug",            // 68
    "endofgame",        // 69
    "ifbulletnear",     // 70
    "ifrespawn",        // 71
    "iffloordistl",     // 72
    "ifceilingdistl",   // 73
    "spritepal",        // 74
    "ifpinventory",     // 75
    "betaname",         // 76
    "cactor",           // 77
    "ifphealthl",       // 78
    "definequote",      // 79
    "quote",            // 80
    "ifinouterspace",   // 81
    "ifnotmoving",      // 82
    "respawnhitag",     // 83
    "tip",              // 84
    "ifspritepal",      // 85
    "money",            // 86
    "soundonce",        // 87
    "addkills",         // 88
    "stopsound",        // 89
    "ifawayfromwall",   // 90
    "ifcanseetarget",   // 91
    "globalsound",      // 92
    "lotsofglass",      // 93
    "ifgotweaponce",    // 94
    "getlastpal",       // 95
    "pkick",            // 96
    "mikesnd",          // 97
    "useractor",        // 98
    "sizeat",           // 99
    "addstrength",      // 100   [#]
    "cstator",          // 101
    "mail",             // 102
    "paper",            // 103
    "tossweapon",       // 104
    "sleeptime",        // 105
    "nullop",           // 106
    "definevolumename", // 107
    "defineskillname",  // 108
    "ifnosounds",       // 109
    "clipdist",         // 110
    "ifangdiffl",       // 111
};


short getincangle(short a,short na)
{
    a &= 2047;
    na &= 2047;

    if(klabs(a-na) < 1024)
        return (na-a);
    else
    {
        if(na > 1024) na -= 2048;
        if(a > 1024) a -= 2048;

        na -= 2048;
        a -= 2048;
        return (na-a);
    }
}

char ispecial(char c)
{
    if(c == 0x0a)
    {
        line_number++;
        return 1;
    }

    if(c == ' ' || c == 0x0d)
        return 1;

    return 0;
}

char isaltok(char c)
{
    return ( isalnum(c) || c == '{' || c == '}' || c == '/' || c == '*' || c == '-' || c == '_' || c == '.');
}

void getglobalz(short i)
{
    int hz,lz,zr;

    spritetype *s = &sprite[i];

    if( s->statnum == 10 || s->statnum == 6 || s->statnum == 2 || s->statnum == 1 || s->statnum == 4)
    {
        if(s->statnum == 4)
            zr = 4L;
        else zr = 127L;

        getzrange(s->x,s->y,s->z-(FOURSLEIGHT),s->sectnum,&hittype[i].ceilingz,&hz,&hittype[i].floorz,&lz,zr,CLIPMASK0);

        if( (lz&49152) == 49152 && (sprite[lz&(MAXSPRITES-1)].cstat&48) == 0 )
        {
            lz &= (MAXSPRITES-1);
            if( badguy(&sprite[lz]) && sprite[lz].pal != 1)
            {
                if( s->statnum != 4 )
                {
                    hittype[i].dispicnum = -4; // No shadows on actors
                    s->xvel = -256;
                    ssp(i,CLIPMASK0);
                }
            }
            else if(sprite[lz].picnum == APLAYER && badguy(s) )
            {
                hittype[i].dispicnum = -4; // No shadows on actors
                s->xvel = -256;
                ssp(i,CLIPMASK0);
            }
            else if(s->statnum == 4 && sprite[lz].picnum == APLAYER)
                if(s->owner == lz)
            {
                hittype[i].ceilingz = sector[s->sectnum].ceilingz;
                hittype[i].floorz   = sector[s->sectnum].floorz;
            }
        }
    }
    else
    {
        hittype[i].ceilingz = sector[s->sectnum].ceilingz;
        hittype[i].floorz   = sector[s->sectnum].floorz;
    }
}


void makeitfall(short i)
{
    spritetype *s = &sprite[i];
    int hz,lz,c;

    if( floorspace(s->sectnum) )
        c = 0;
    else
    {
        if( ceilingspace(s->sectnum) || sector[s->sectnum].lotag == 2)
            c = gc/6;
        else c = gc;
    }

    if( ( s->statnum == 1 || s->statnum == 10 || s->statnum == 2 || s->statnum == 6 ) )
        getzrange(s->x,s->y,s->z-(FOURSLEIGHT),s->sectnum,&hittype[i].ceilingz,&hz,&hittype[i].floorz,&lz,127L,CLIPMASK0);
    else
    {
        hittype[i].ceilingz = sector[s->sectnum].ceilingz;
        hittype[i].floorz   = sector[s->sectnum].floorz;
    }

    if( s->z < hittype[i].floorz-(FOURSLEIGHT) )
    {
        if( sector[s->sectnum].lotag == 2 && s->zvel > 3122 )
            s->zvel = 3144;
        if(s->zvel < 6144)
            s->zvel += c;
        else s->zvel = 6144;
        s->z += s->zvel;
    }
    if( s->z >= hittype[i].floorz-(FOURSLEIGHT) )
    {
        s->z = hittype[i].floorz - FOURSLEIGHT;
        s->zvel = 0;
    }
}


void getlabel(void)
{
    int i;

    while( isalnum(*textptr) == 0 )
    {
        if(*textptr == 0x0a) line_number++;
        textptr++;
        if( *textptr == 0)
            return;
    }

    i = 0;
    while( ispecial(*textptr) == 0 ) {
        if (i < MAXLABELLEN - 1) {
            label[labelcnt * MAXLABELLEN + i] = *textptr;
            i++;
        }
        textptr++;
    }
    label[labelcnt * MAXLABELLEN + i] = 0;
}

int keyword(void)
{
    int i;
    char *temptextptr;

    temptextptr = textptr;

    while( isaltok(*temptextptr) == 0 )
    {
        temptextptr++;
        if( *temptextptr == 0 )
            return 0;
    }

    i = 0;
    while( isaltok(*temptextptr) )
    {
        tempbuf[i] = *(temptextptr++);
        i++;
    }
    tempbuf[i] = 0;

    for(i=0;i<NUMKEYWORDS;i++)
        if( strcmp( tempbuf,keyw[i]) == 0 )
            return i;

    return -1;
}

int transword(void) //Returns its code #
{
    int i, l;

    while( isaltok(*textptr) == 0 )
    {
        if(*textptr == 0x0a) line_number++;
        if( *textptr == 0 )
            return -1;
        textptr++;
    }

    l = 0;
    while( isaltok(*(textptr+l)) )
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    for(i=0;i<NUMKEYWORDS;i++)
    {
        if( strcmp( tempbuf,keyw[i]) == 0 )
        {
            *scriptptr = i;
            textptr += l;
            scriptptr++;
            return i;
        }
    }

    textptr += l;

    if( tempbuf[0] == '{' && tempbuf[1] != 0)
        initprintf("  * ERROR!(L%d %s) Expecting a SPACE or CR between '{' and '%s'.\n",line_number,compilefile,tempbuf+1);
    else if( tempbuf[0] == '}' && tempbuf[1] != 0)
        initprintf("  * ERROR!(L%d %s) Expecting a SPACE or CR between '}' and '%s'.\n",line_number,compilefile,tempbuf+1);
    else if( tempbuf[0] == '/' && tempbuf[1] == '/' && tempbuf[2] != 0 )
        initprintf("  * ERROR!(L%d %s) Expecting a SPACE between '//' and '%s'.\n",line_number,compilefile,tempbuf+2);
    else if( tempbuf[0] == '/' && tempbuf[1] == '*' && tempbuf[2] != 0 )
        initprintf("  * ERROR!(L%d %s) Expecting a SPACE between '/*' and '%s'.\n",line_number,compilefile,tempbuf+2);
    else if( tempbuf[0] == '*' && tempbuf[1] == '/' && tempbuf[2] != 0 )
        initprintf("  * ERROR!(L%d %s) Expecting a SPACE between '*/' and '%s'.\n",line_number,compilefile,tempbuf+2);
    else initprintf("  * ERROR!(L%d %s) Expecting key word, but found '%s'.\n",line_number,compilefile,tempbuf);

    error++;
    return -1;
}

int transnum(int type)
{
    int i, l;

    while( isaltok(*textptr) == 0 )
    {
        if(*textptr == 0x0a) line_number++;
        textptr++;
        if( *textptr == 0 )
            return -1;  // eof
    }


    l = 0;
    while( isaltok(*(textptr+l)) )
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    for(i=0;i<NUMKEYWORDS;i++)
        if( strcmp( tempbuf,keyw[i]) == 0 )
        {
            error++;
            initprintf("  * ERROR!(L%d %s) Symbol '%s' is a key word.\n",line_number,compilefile,tempbuf);
            textptr+=l;
        }


    for(i=0;i<labelcnt;i++)
    {
        if( !Bstrcmp(tempbuf,label+(i * MAXLABELLEN)) )
        {
            char el[64], gl[64];

            if (labeltype[i] & type) {
                *(scriptptr++) = labelcode[i];
                textptr += l;
                return labeltype[i];
            }
            *(scriptptr++) = 0;
            textptr += l;
            translatelabeltype(type, el);
            translatelabeltype(labeltype[i], gl);
            initprintf("  * WARNING!(L%d %s) Expected a '%s' label but found a '%s' label instead.\n",line_number,compilefile,el,gl);
            return -1;  // valid label name, but wrong type
        }
    }

    if( isdigit(*textptr) == 0 && *textptr != '-')
    {
        initprintf("  * ERROR!(L%d %s) Parameter '%s' is undefined.\n",line_number,compilefile,tempbuf);
        error++;
        textptr+=l;
        return -1;  // error!
    }

    *scriptptr = atol(textptr);
    scriptptr++;

    textptr += l;

    return 0;   // literal value
}


/**
 * Encode a scriptptr into a form suitable for portably
 * inserting into bytecode. We store the pointer as INT_MIN
 * plus the offset from the start of the script buffer, just
 * to make it perhaps a little more obvious what is happening.
 */
int encodescriptptr(int *scptr)
{
    int offs = (int)(scptr - script);
    assert(offs >= 0);
    assert(offs < MAXSCRIPTSIZE);
    return INT_MIN+offs;
}

/**
 * Decode an encoded representation of a scriptptr
 */
int *decodescriptptr(int scptr)
{
    assert(scptr <= 0);
    return script + (scptr - INT_MIN);
}

char parsecommand(void)
{
    int i, j, k, *tempscrptr, tw;
    char done;

    if ((unsigned)(scriptptr-script) > MAXSCRIPTSIZE) {
        Bsprintf(tempbuf,"FATAL ERROR: Compiled size of CON code exceeds maximum size!\n"
            "Please notify JonoF so the maximum may be increased in a future release.");
        gameexit(tempbuf);
    }

    if( error > 12 || ( *textptr == '\0' ) || ( *(textptr+1) == '\0' ) ) return 1;

    tw = transword();

    switch(tw)
    {
        default:
        case -1:
            return 0; //End
        case 39:    //multi-line comment
            scriptptr--;
            j = line_number;
            do
            {
                textptr++;
                if(*textptr == 0x0a) line_number++;
                if( *textptr == 0 )
                {
                    initprintf("  * ERROR!(L%d %s) Found '/*' with no '*/'.\n",j,compilefile);
                    error++;
                    return 0;
                }
            }
            while( *textptr != '*' || *(textptr+1) != '/' );
            textptr+=2;
            return 0;
        case 17:    //state
            if( parsing_actor == 0 && parsing_state == 0 )
            {
                getlabel();
                scriptptr--;
                labelcode[labelcnt] = encodescriptptr(scriptptr);
                labeltype[labelcnt] = LABEL_STATE;
                labelcnt++;

                parsing_state = 1;

                return 0;
            }

            getlabel();

            for(i=0;i<NUMKEYWORDS;i++)
                if( strcmp( label+(labelcnt * MAXLABELLEN),keyw[i]) == 0 )
                {
                    error++;
                    initprintf("  * ERROR!(L%d %s) Symbol '%s' is a key word.\n",line_number,compilefile,label+(labelcnt * MAXLABELLEN));
                    return 0;
                }

            for(j=0;j<labelcnt;j++)
            {
                if( !Bstrcmp(label+(j * MAXLABELLEN),label+(labelcnt * MAXLABELLEN)) )
                {
                    if (labeltype[j] & LABEL_STATE) {
                        *scriptptr = labelcode[j];
                        break;
                    } else {
                        char gl[64];

                        translatelabeltype(labeltype[j], gl);
                        initprintf("  * WARNING!(L%d %s) Expected a state label, found a %s instead. Neutering.\n",
                            line_number,compilefile,gl);
                        *(scriptptr-1) = 106;   // nullop
                        return 0;
                    }
                }
            }

            if(j==labelcnt)
            {
                initprintf("  * ERROR!(L%d %s) State '%s' not found.\n",line_number,compilefile,label+(labelcnt * MAXLABELLEN));
                error++;
            }
            scriptptr++;
            return 0;

        case 15:    //sound
        case 92:    //globalsound
        case 87:    //soundonce
        case 89:    //stopsound
        case 93:    //lotsofglass
            transnum(LABEL_DEFINE);
            return 0;

        case 18:    //ends
            if( parsing_state == 0 )
            {
                initprintf("  * ERROR!(L%d %s) Found 'ends' with no 'state'.\n",line_number,compilefile);
                error++;
            }
//            else
            {
                if( num_squigilly_brackets > 0 )
                {
                    initprintf("  * ERROR!(L%d %s) Found more '{' than '}' before 'ends'.\n",line_number,compilefile);
                    error++;
                }
                if( num_squigilly_brackets < 0 )
                {
                    initprintf("  * ERROR!(L%d %s) Found more '}' than '{' before 'ends'.\n",line_number,compilefile);
                    error++;
                }
                parsing_state = 0;
            }
            return 0;
        case 19:    //define
            getlabel();
            // Check to see it's already defined

            for(i=0;i<NUMKEYWORDS;i++)
                if( strcmp( label+(labelcnt * MAXLABELLEN),keyw[i]) == 0 )
                {
                    error++;
                    initprintf("  * ERROR!(L%d %s) Symbol '%s' is a key word.\n",line_number,compilefile,label+(labelcnt * MAXLABELLEN));
                    return 0;
                }

            for(i=0;i<labelcnt;i++)
            {
                if( strcmp(label+(labelcnt * MAXLABELLEN),label+(i * MAXLABELLEN)) == 0 )
                {
                    warning++;
                    initprintf("  * WARNING.(L%d %s) Duplicate definition '%s' ignored.\n",line_number,compilefile,label+(labelcnt * MAXLABELLEN));
                    break;
                }
            }

            transnum(LABEL_DEFINE);
            if(i == labelcnt) {
                labelcode[labelcnt] = *(scriptptr-1);
                labeltype[labelcnt] = LABEL_DEFINE;
                labelcnt++;
            }
            scriptptr -= 2;
            return 0;
        case 14:    //palfrom

            for(j = 0;j < 4;j++)
            {
                if( keyword() == -1 )
                    transnum(LABEL_DEFINE);
                else break;
            }

            while(j < 4)
            {
                *scriptptr = 0;
                scriptptr++;
                j++;
            }
            return 0;

        case 32:    //move
            if( parsing_actor || parsing_state )
            {
                transnum(LABEL_MOVE);

                j = 0;
                while(keyword() == -1)
                {
                    transnum(LABEL_DEFINE);
                    scriptptr--;
                    j |= *scriptptr;
                }
                *scriptptr = j;
                scriptptr++;
            }
            else
            {
                scriptptr--;
                getlabel();
                // Check to see it's already defined

                for(i=0;i<NUMKEYWORDS;i++)
                    if( strcmp( label+(labelcnt * MAXLABELLEN),keyw[i]) == 0 )
                    {
                        error++;
                        initprintf("  * ERROR!(L%d %s) Symbol '%s' is a key word.\n",line_number,compilefile,label+(labelcnt * MAXLABELLEN));
                        return 0;
                    }

                for(i=0;i<labelcnt;i++)
                    if( strcmp(label+(labelcnt * MAXLABELLEN),label+(i * MAXLABELLEN)) == 0 )
                    {
                        warning++;
                        initprintf("  * WARNING.(L%d %s) Duplicate move '%s' ignored.\n",line_number,compilefile,label+(labelcnt * MAXLABELLEN));
                        break;
                    }
                if(i == labelcnt) {
                    labelcode[labelcnt] = encodescriptptr(scriptptr);
                    labeltype[labelcnt] = LABEL_MOVE;
                    labelcnt++;
                }
                for(j=0;j<2;j++)
                {
                    if(keyword() >= 0) break;
                    transnum(LABEL_DEFINE);
                }
                for(k=j;k<2;k++)
                {
                    *scriptptr = 0;
                    scriptptr++;
                }
            }
            return 0;

        case 54:    //music
            {
                scriptptr--;
                transnum(LABEL_DEFINE); // Volume Number (0/4)
                scriptptr--;

                k = *scriptptr-1;

                if(k >= 0) // if it's background music
                {
                    i = 0;
                    while(keyword() == -1)
                    {
                        while( isaltok(*textptr) == 0 )
                        {
                            if(*textptr == 0x0a) line_number++;
                            textptr++;
                            if( *textptr == 0 ) break;
                        }
                        j = 0;
                        while( isaltok(*(textptr+j)) )
                        {
                            music_fn[k][i][j] = textptr[j];
                            j++;
                        }
                        music_fn[k][i][j] = '\0';
                        textptr += j;
                        if(i > 9) break;
                        i++;
                    }
                }
                else
                {
                    i = 0;
                    while(keyword() == -1)
                    {
                        while( isaltok(*textptr) == 0 )
                        {
                            if(*textptr == 0x0a) line_number++;
                            textptr++;
                            if( *textptr == 0 ) break;
                        }
                        j = 0;
                        while( isaltok(*(textptr+j)) )
                        {
                            env_music_fn[i][j] = textptr[j];
                            j++;
                        }
                        env_music_fn[i][j] = '\0';

                        textptr += j;
                        if(i > 9) break;
                        i++;
                    }
                }
            }
            return 0;
        case 55:    //include
            scriptptr--;
            while( isaltok(*textptr) == 0 )
            {
                if(*textptr == 0x0a) line_number++;
                textptr++;
                if( *textptr == 0 ) break;
            }
            j = 0;
            while( isaltok(*textptr) )
            {
                tempbuf[j] = *(textptr++);
                j++;
            }
            tempbuf[j] = '\0';

            {
                short temp_line_number;
                char  temp_ifelse_check;
                char *origtptr, *mptr;
                char parentcompilefile[255];
                int fp;

                fp = kopen4load(tempbuf,loadfromgrouponly);
                if(fp < 0)
                {
                    error++;
                    initprintf("  * ERROR!(L%d %s) Could not find '%s'.\n",line_number,compilefile,tempbuf);
                    return 0;
                }

                j = kfilelength(fp);

                mptr = Bmalloc(j+1);
                if (!mptr) {
                    kclose(fp);
                    error++;
                    initprintf("  * ERROR!(L%d %s) Could not allocate %d bytes to include '%s'.\n",
                        line_number,compilefile,j,tempbuf);
                    return 0;
                }

                initprintf("Including: %s (%d bytes)\n",tempbuf, j);
                kread(fp, mptr, j);
                kclose(fp);
                mptr[j] = 0;

                origtptr = textptr;

                strcpy(parentcompilefile, compilefile);
                strcpy(compilefile, tempbuf);
                temp_line_number = line_number;
                line_number = 1;
                temp_ifelse_check = checking_ifelse;
                checking_ifelse = 0;

                textptr = mptr;

                do done = parsecommand(); while (!done);

                strcpy(compilefile, parentcompilefile);
                total_lines += line_number;
                line_number = temp_line_number;
                checking_ifelse = temp_ifelse_check;
                
                textptr = origtptr;
                
                Bfree(mptr);
            }

            return 0;
        case 24:    //ai
            if( parsing_actor || parsing_state )
                transnum(LABEL_AI);
            else
            {
                scriptptr--;
                getlabel();

                for(i=0;i<NUMKEYWORDS;i++)
                    if( strcmp( label+(labelcnt * MAXLABELLEN),keyw[i]) == 0 )
                    {
                        error++;
                        initprintf("  * ERROR!(L%d %s) Symbol '%s' is a key word.\n",line_number,compilefile,label+(labelcnt * MAXLABELLEN));
                        return 0;
                    }

                for(i=0;i<labelcnt;i++)
                    if( strcmp(label+(labelcnt * MAXLABELLEN),label+(i * MAXLABELLEN)) == 0 )
                    {
                        warning++;
                        initprintf("  * WARNING.(L%d %s) Duplicate ai '%s' ignored.\n",line_number,compilefile,label+(labelcnt * MAXLABELLEN));
                        break;
                    }

                if(i == labelcnt) {
                    labelcode[labelcnt] = encodescriptptr(scriptptr);
                    labeltype[labelcnt] = LABEL_AI;
                    labelcnt++;
                }

                for(j=0;j<3;j++)
                {
                    if(keyword() >= 0) break;
                    if(j == 1)
                        transnum(LABEL_ACTION);
                    else if(j == 2)
                    {
                        transnum(LABEL_MOVE);
                        k = 0;
                        while(keyword() == -1)
                        {
                            transnum(LABEL_DEFINE);
                            scriptptr--;
                            k |= *scriptptr;
                        }
                        *scriptptr = k;
                        scriptptr++;
                        return 0;
                    }
                }
                for(k=j;k<3;k++)
                {
                    *scriptptr = 0;
                    scriptptr++;
                }
            }
            return 0;

        case 7:     //action
            if( parsing_actor || parsing_state )
                transnum(LABEL_ACTION);
            else
            {
                scriptptr--;
                getlabel();
                // Check to see it's already defined

                for(i=0;i<NUMKEYWORDS;i++)
                    if( strcmp( label+(labelcnt * MAXLABELLEN),keyw[i]) == 0 )
                    {
                        error++;
                        initprintf("  * ERROR!(L%d %s) Symbol '%s' is a key word.\n",line_number,compilefile,label+(labelcnt * MAXLABELLEN));
                        return 0;
                    }

                for(i=0;i<labelcnt;i++)
                    if( strcmp(label+(labelcnt * MAXLABELLEN),label+(i * MAXLABELLEN)) == 0 )
                    {
                        warning++;
                        initprintf("  * WARNING.(L%d %s) Duplicate action '%s' ignored.\n",line_number,compilefile,label+(labelcnt * MAXLABELLEN));
                        break;
                    }

                if(i == labelcnt) {
                    labelcode[labelcnt] = encodescriptptr(scriptptr);
                    labeltype[labelcnt] = LABEL_ACTION;
                    labelcnt++;
                }

                for(j=0;j<5;j++)
                {
                    if(keyword() >= 0) break;
                    transnum(LABEL_DEFINE);
                }
                for(k=j;k<5;k++)
                {
                    *scriptptr = 0;
                    scriptptr++;
                }
            }
            return 0;

        case 1:     //actor
            if( parsing_state )
            {
                initprintf("  * ERROR!(L%d %s) Found 'actor' within 'state'.\n",line_number,compilefile);
                error++;
            }

            if( parsing_actor )
            {
                initprintf("  * ERROR!(L%d %s) Found 'actor' within 'actor'.\n",line_number,compilefile);
                error++;
            }

            num_squigilly_brackets = 0;
            scriptptr--;
            parsing_actor = scriptptr;

            transnum(LABEL_DEFINE);
            scriptptr--;
            actorscrptr[*scriptptr] = parsing_actor;

            for(j=0;j<4;j++)
            {
                *(parsing_actor+j) = 0;
                if(j == 3)
                {
                    j = 0;
                    while(keyword() == -1)
                    {
                        transnum(LABEL_DEFINE);
                        scriptptr--;
                        j |= *scriptptr;
                    }
                    *scriptptr = j;
                    scriptptr++;
                    break;
                }
                else
                {
                    if(keyword() >= 0)
                    {
                        for (i=4-j; i>0; i--) *(scriptptr++) = 0;
                        break;
                    }
                    switch (j)
                    {
                        case 0: transnum(LABEL_DEFINE); break;
                        case 1: transnum(LABEL_ACTION); break;
                        case 2: transnum(LABEL_MOVE|LABEL_DEFINE); break;
                    }
                    *(parsing_actor+j) = *(scriptptr-1);
                }
            }

            checking_ifelse = 0;

            return 0;

        case 98:    //useractor

            if( parsing_state )
            {
                initprintf("  * ERROR!(L%d %s) Found 'useritem' within 'state'.\n",line_number,compilefile);
                error++;
            }

            if( parsing_actor )
            {
                initprintf("  * ERROR!(L%d %s) Found 'useritem' within 'actor'.\n",line_number,compilefile);
                error++;
            }

            num_squigilly_brackets = 0;
            scriptptr--;
            parsing_actor = scriptptr;

            transnum(LABEL_DEFINE);
            scriptptr--;
            j = *scriptptr;

            transnum(LABEL_DEFINE);
            scriptptr--;
            actorscrptr[*scriptptr] = parsing_actor;
            actortype[*scriptptr] = j;

            for(j=0;j<4;j++)
            {
                *(parsing_actor+j) = 0;
                if(j == 3)
                {
                    j = 0;
                    while(keyword() == -1)
                    {
                        transnum(LABEL_DEFINE);
                        scriptptr--;
                        j |= *scriptptr;
                    }
                    *scriptptr = j;
                    scriptptr++;
                    break;
                }
                else
                {
                    if(keyword() >= 0)
                    {
                        for (i=4-j; i>0; i--) *(scriptptr++) = 0;
                        break;
                    }
                    switch (j)
                    {
                        case 0: transnum(LABEL_DEFINE); break;
                        case 1: transnum(LABEL_ACTION); break;
                        case 2: transnum(LABEL_MOVE|LABEL_DEFINE); break;
                    }
                    *(parsing_actor+j) = *(scriptptr-1);
                }
            }

            checking_ifelse = 0;

            return 0;

        case 11:    //strength
        case 13:    //shoot
        case 25:    //addphealth
        case 31:    //spawn
        case 40:    //cstat
        case 52:    //count
        case 69:    //endofgame
        case 74:    //spritepal
        case 77:    //cactor
        case 80:    //quote
        case 86:    //money
        case 88:    //addkills
        case 68:    //debug
        case 100:   //addstrength
        case 101:   //cstator
        case 102:   //mail
        case 103:   //paper
        case 105:   //sleeptime
        case 110:   //clipdist
            transnum(LABEL_DEFINE);
            return 0;

        case 2:     //addammo
        case 23:    //addweapon
        case 28:    //sizeto
        case 99:    //sizeat
        case 37:    //debris
        case 48:    //addinventory
        case 58:    //guts
            transnum(LABEL_DEFINE);
            transnum(LABEL_DEFINE);
            break;
        case 50:    //hitradius
            transnum(LABEL_DEFINE);
            transnum(LABEL_DEFINE);
            transnum(LABEL_DEFINE);
            transnum(LABEL_DEFINE);
            transnum(LABEL_DEFINE);
            break;
        case 10:    //else
            if( checking_ifelse )
            {
                checking_ifelse--;
                tempscrptr = scriptptr;
                scriptptr++; //Leave a spot for the fail location
                parsecommand();
                *tempscrptr = encodescriptptr(scriptptr);
            }
            else
            {
                scriptptr--;
                error++;
                initprintf("  * ERROR!(L%d %s) Found 'else' with no 'if'.\n",line_number,compilefile);
            }

            return 0;

        case 75:    //ifpinventory
            transnum(LABEL_DEFINE);
        case 3:     //ifrnd
        case 8:     //ifpdistl
        case 9:     //ifpdistg
        case 21:    //ifai
        case 33:    //ifwasweapon
        case 34:    //ifaction
        case 35:    //ifactioncount
        case 41:    //ifmove
        case 46:    //ifcount
        case 53:    //ifactor
        case 56:    //ifstrength
        case 59:    //ifspawnedby
        case 62:    //ifgapzl
        case 72:    //iffloordistl
        case 73:    //ifceilingdistl
        case 78:    //ifphealthl
        case 85:    //ifspritepal
        case 94:    //ifgotweaponce
        case 111:   //ifangdiffl
            switch (tw) {
                case 21: transnum(LABEL_AI); break;
                case 34: transnum(LABEL_ACTION); break;
                case 41: transnum(LABEL_MOVE); break;
                default: transnum(LABEL_DEFINE); break;
            }
        case 43:    //ifonwater
        case 44:    //ifinwater
        case 49:    //ifactornotstayput
        case 5:     //ifcansee
        case 6:     //ifhitweapon
        case 27:    //ifsquished
        case 26:    //ifdead
        case 45:    //ifcanshoottarget
        case 51:    //ifp
        case 63:    //ifhitspace
        case 64:    //ifoutside
        case 65:    //ifmultiplayer
        case 67:    //ifinspace
        case 70:    //ifbulletnear
        case 71:    //ifrespawn
        case 81:    //ifinouterspace
        case 82:    //ifnotmoving
        case 90:    //ifawayfromwall
        case 91:    //ifcanseetarget
        case 109:   //ifnosounds

            if(tw == 51)
            {
                j = 0;
                do
                {
                    transnum(LABEL_DEFINE);
                    scriptptr--;
                    j |= *scriptptr;
                }
                while(keyword() == -1);
                *scriptptr = j;
                scriptptr++;
            }

            tempscrptr = scriptptr;
            scriptptr++; //Leave a spot for the fail location

            do
            {
                j = keyword();
                if(j == 20 || j == 39)
                    parsecommand();
            } while(j == 20 || j == 39);

            parsecommand();

            *tempscrptr = encodescriptptr(scriptptr);

            checking_ifelse++;
            return 0;
        case 29:    //{
            num_squigilly_brackets++;
            do
                done = parsecommand();
            while( done == 0 );
            return 0;
        case 30:    //}
            num_squigilly_brackets--;
            if( num_squigilly_brackets < 0 )
            {
                initprintf("  * ERROR!(L%d %s) Found more '}' than '{'.\n",line_number,compilefile);
                error++;
            }
            return 1;
        case 76:    //betaname
            scriptptr--;
            j = 0;
            while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 )      // JBF 20040127: end of file checked
            {
                betaname[j] = *textptr;
                j++; textptr++;
            }
            betaname[j] = 0;
            return 0;
        case 20:    //single-line comment
            scriptptr--; //Negate the rem
            while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 )      // JBF 20040127: end of file checked
                textptr++;

            // line_number++;
            return 0;

        case 107:   //definevolumename
            scriptptr--;
            transnum(LABEL_DEFINE);
            scriptptr--;
            j = *scriptptr;
            while( *textptr == ' ' ) textptr++;

            if (j < 0 || j >= 4)
            {
                initprintf("  * ERROR!(L%d %s) Volume number exceeds maximum volume count.\n",line_number,compilefile);
                error++;
                while( *textptr != 0x0a && *textptr != 0 ) textptr++;
                break;
            }
        
            i = 0;

            while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 )      // JBF 20040127: end of file checked
            {
                volume_names[j][i] = toupper(*textptr);
                textptr++,i++;
                if(i >= 32)
                {
                    initprintf("  * ERROR!(L%d %s) Volume name exceeds character size limit of 32.\n",line_number,compilefile);
                    error++;
                    while( *textptr != 0x0a && *textptr != 0 ) textptr++;       // JBF 20040127: end of file checked
                    break;
                }
            }
            volume_names[j][i] = '\0';
            return 0;
        case 108:   //defineskillname
            scriptptr--;
            transnum(LABEL_DEFINE);
            scriptptr--;
            j = *scriptptr;
            while( *textptr == ' ' ) textptr++;

            if (j < 0 || j >= 5)
            {
                initprintf("  * ERROR!(L%d %s) Skill number exceeds maximum skill count.\n",line_number,compilefile);
                error++;
                while( *textptr != 0x0a && *textptr != 0 ) textptr++;
                break;
            }
        
            i = 0;

            while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 )      // JBF 20040127: end of file checked
            {
                skill_names[j][i] = toupper(*textptr);
                textptr++,i++;
                if(i >= 32)
                {
                    initprintf("  * ERROR!(L%d %s) Skill name exceeds character size limit of 32.\n",line_number,compilefile);
                    error++;
                    while( *textptr != 0x0a && *textptr != 0 ) textptr++;       // JBF 20040127: end of file checked
                    break;
                }
            }
            skill_names[j][i] = '\0';
            return 0;

        case 0:     //definelevelname
            scriptptr--;
            transnum(LABEL_DEFINE);
            scriptptr--;
            j = *scriptptr;
            transnum(LABEL_DEFINE);
            scriptptr--;
            k = *scriptptr;
            while( *textptr == ' ' ) textptr++;

            if (j < 0 || j >= 4)
            {
                initprintf("  * WARNING!(L%d %s) Volume number exceeds maximum volume count.\n",line_number,compilefile);
                warning++;
                while( *textptr != 0x0a && *textptr != 0 ) textptr++;
                break;
            }
            if (k < 0 || k >= 11)
            {
                initprintf("  * WARNING!(L%d %s) Level number exceeds maximum levels-per-episode count.\n",
                line_number,compilefile);
                warning++;
                while( *textptr != 0x0a && *textptr != 0 ) textptr++;
                break;
            }
        
            i = 0;
            while( *textptr != ' ' && *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 )   // JBF 20040127: end of file checked
            {
                level_file_names[j*11+k][i] = *textptr;
                textptr++,i++;
                if(i > 127)
                {
                    initprintf("  * ERROR!(L%d %s) Level file name exceeds character size limit of 128.\n",line_number,compilefile);
                    error++;
                    while( *textptr != ' ' && *textptr != 0) textptr++;     // JBF 20040127: end of file checked
                    break;
                }
            }
            level_names[j*11+k][i] = '\0';

            while( *textptr == ' ' ) textptr++;

            partime[j*11+k] =
                (((*(textptr+0)-'0')*10+(*(textptr+1)-'0'))*26*60)+
                (((*(textptr+3)-'0')*10+(*(textptr+4)-'0'))*26);

            textptr += 5;
            while( *textptr == ' ' ) textptr++;

            designertime[j*11+k] =
                (((*(textptr+0)-'0')*10+(*(textptr+1)-'0'))*26*60)+
                (((*(textptr+3)-'0')*10+(*(textptr+4)-'0'))*26);

            textptr += 5;
            while( *textptr == ' ' ) textptr++;

            i = 0;

            while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 )      // JBF 20040127: end of file checked
            {
                level_names[j*11+k][i] = toupper(*textptr);
                textptr++,i++;
                if(i >= 32)
                {
                    initprintf("  * ERROR!(L%d %s) Level name exceeds character size limit of 32.\n",line_number,compilefile);
                    error++;
                    while( *textptr != 0x0a && *textptr != 0 ) textptr++;       // JBF 20040127: end of file checked
                    break;
                }
            }
            level_names[j*11+k][i] = '\0';
            return 0;

        case 79:    //definequote
            scriptptr--;
            transnum(LABEL_DEFINE);
            k = *(scriptptr-1);
            if(k >= NUMOFFIRSTTIMEACTIVE)
            {
                initprintf("  * ERROR!(L%d %s) Quote amount exceeds limit of %d characters.\n",line_number,compilefile,NUMOFFIRSTTIMEACTIVE);
                error++;
            }
            scriptptr--;
            i = 0;
            while( *textptr == ' ' )
                textptr++;

            while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 )      // JBF 20040127: end of file checked
            {
                fta_quotes[k][i] = *textptr;
                textptr++,i++;
                if(i >= 64)
                {
                    initprintf("  * ERROR!(L%d %s) Quote exceeds character size limit of 64.\n",line_number,compilefile);
                    error++;
                    while( *textptr != 0x0a && *textptr != 0 ) textptr++;       // JBF 20040127: end of file checked
                    break;
                }
            }
            fta_quotes[k][i] = '\0';
            return 0;
        case 57:    //definesound
            scriptptr--;
            transnum(LABEL_DEFINE);
            k = *(scriptptr-1);
            if(k >= NUM_SOUNDS)
            {
                initprintf("  * ERROR!(L%d %s) Exceeded sound limit of %d.\n",line_number,compilefile,NUM_SOUNDS);
                error++;
            }
            scriptptr--;
            i = 0;
            while( *textptr == ' ')
                textptr++;

            while( *textptr != ' ' && *textptr != 0 )       // JBF 20040127: end of file checked
            {
                sounds[k][i] = *textptr;
                textptr++,i++;
                if(i >= 13)
                {
                    initprintf("%s\n",sounds[k]);
                    initprintf("  * ERROR!(L%d %s) Sound filename exceeded limit of 13 characters.\n",line_number,compilefile);
                    error++;
                    while( *textptr != ' ' && *textptr != 0 ) textptr++;        // JBF 20040127: end of file checked
                    break;
                }
            }
            sounds[k][i] = '\0';

            transnum(LABEL_DEFINE);
            soundps[k] = *(scriptptr-1);
            scriptptr--;
            transnum(LABEL_DEFINE);
            soundpe[k] = *(scriptptr-1);
            scriptptr--;
            transnum(LABEL_DEFINE);
            soundpr[k] = *(scriptptr-1);
            scriptptr--;
            transnum(LABEL_DEFINE);
            soundm[k] = *(scriptptr-1);
            scriptptr--;
            transnum(LABEL_DEFINE);
            soundvo[k] = *(scriptptr-1);
            scriptptr--;
            return 0;

        case 4:     //enda
            if( parsing_actor == 0 )
            {
                initprintf("  * ERROR!(L%d %s) Found 'enda' without defining 'actor'.\n",line_number,compilefile);
                error++;
            }
//            else
            {
                if( num_squigilly_brackets > 0 )
                {
                    initprintf("  * ERROR!(L%d %s) Found more '{' than '}' before 'enda'.\n",line_number,compilefile);
                    error++;
                }
                parsing_actor = 0;
            }

            return 0;
        case 12:    //break
        case 16:    //fall
        case 84:    //tip
        case 22:    //killit
        case 36:    //resetactioncount
        case 38:    //pstomp
        case 42:    //resetplayer
        case 47:    //resetcount
        case 61:    //wackplayer
        case 66:    //operate
        case 83:    //respawnhitag
        case 95:    //getlastpal
        case 96:    //pkick
        case 97:    //mikesnd
        case 104:   //tossweapon
        case 106:   //nullop
            return 0;
        case 60:    //gamestartup
            {
                int params[30];

                scriptptr--;
                for(j = 0; j < 30; j++)
                    {
                    transnum(LABEL_DEFINE);
                    scriptptr--;
                    params[j] = *scriptptr;

                    if (j != 25) continue;

                    if (keyword() != -1) {
                        initprintf("Looks like Standard CON files.\n");
                        break;
                    } else {
                        conversion = 14;
                        initprintf("Looks like Atomic Edition CON files.\n");
                    }
                }
                
                /*
                v1.3d               v1.5
                DEFAULTVISIBILITY   DEFAULTVISIBILITY
                GENERICIMPACTDAMAGE GENERICIMPACTDAMAGE
                MAXPLAYERHEALTH     MAXPLAYERHEALTH
                STARTARMORHEALTH    STARTARMORHEALTH
                RESPAWNACTORTIME    RESPAWNACTORTIME
                RESPAWNITEMTIME     RESPAWNITEMTIME
                RUNNINGSPEED        RUNNINGSPEED
                RPGBLASTRADIUS      GRAVITATIONALCONSTANT
                PIPEBOMBRADIUS      RPGBLASTRADIUS
                SHRINKERBLASTRADIUS PIPEBOMBRADIUS
                TRIPBOMBBLASTRADIUS SHRINKERBLASTRADIUS
                MORTERBLASTRADIUS   TRIPBOMBBLASTRADIUS
                BOUNCEMINEBLASTRADIUS   MORTERBLASTRADIUS
                SEENINEBLASTRADIUS  BOUNCEMINEBLASTRADIUS
                MAXPISTOLAMMO       SEENINEBLASTRADIUS
                MAXSHOTGUNAMMO      MAXPISTOLAMMO
                MAXCHAINGUNAMMO     MAXSHOTGUNAMMO
                MAXRPGAMMO          MAXCHAINGUNAMMO
                MAXHANDBOMBAMMO     MAXRPGAMMO
                MAXSHRINKERAMMO     MAXHANDBOMBAMMO
                MAXDEVISTATORAMMO   MAXSHRINKERAMMO
                MAXTRIPBOMBAMMO     MAXDEVISTATORAMMO
                MAXFREEZEAMMO       MAXTRIPBOMBAMMO
                CAMERASDESTRUCTABLE MAXFREEZEAMMO
                NUMFREEZEBOUNCES    MAXGROWAMMO
                FREEZERHURTOWNER    CAMERASDESTRUCTABLE
                                    NUMFREEZEBOUNCES
                                    FREEZERHURTOWNER
                                    QSIZE
                                    TRIPBOMBLASERMODE
                */

                j = 0;
                ud.const_visibility = params[j++];
                impact_damage = params[j++];
                max_player_health = params[j++];
                max_armour_amount = params[j++];
                respawnactortime = params[j++];
                respawnitemtime = params[j++];
                dukefriction = params[j++];
                if (conversion == 14) gc = params[j++];
                rpgblastradius = params[j++];
                pipebombblastradius = params[j++];
                shrinkerblastradius = params[j++];
                tripbombblastradius = params[j++];
                morterblastradius = params[j++];
                bouncemineblastradius = params[j++];
                seenineblastradius = params[j++];
                max_ammo_amount[PISTOL_WEAPON] = params[j++];
                max_ammo_amount[SHOTGUN_WEAPON] = params[j++];
                max_ammo_amount[CHAINGUN_WEAPON] = params[j++];
                max_ammo_amount[RPG_WEAPON] = params[j++];
                max_ammo_amount[HANDBOMB_WEAPON] = params[j++];
                max_ammo_amount[SHRINKER_WEAPON] = params[j++];
                max_ammo_amount[DEVISTATOR_WEAPON] = params[j++];
                max_ammo_amount[TRIPBOMB_WEAPON] = params[j++];
                max_ammo_amount[FREEZE_WEAPON] = params[j++];
                if (conversion == 14) max_ammo_amount[GROW_WEAPON] = params[j++];
                camerashitable = params[j++];
                numfreezebounces = params[j++];
                freezerhurtowner = params[j++];
                if (conversion == 14) {
                    spriteqamount = params[j++];
                    if(spriteqamount > 1024) spriteqamount = 1024;
                    else if(spriteqamount < 0) spriteqamount = 0;
                    lasermode = params[j++];
                }
            }
            return 0;
    }
    return 0;
}


void passone(void)
{

    while( parsecommand() == 0 );

    if( (error+warning) > 12)
        initprintf(  "  * ERROR! Too many warnings or errors.\n");

}

static const char *defaultcons[3] =
{
     "GAME.CON",
     "USER.CON",
     "DEFS.CON"
};

void copydefaultcons(void)
{
    int i, fs, fpi;
    FILE *fpo;

    for(i=0;i<3;i++)
    {
        fpi = kopen4load( defaultcons[i] , 1 );
        if (fpi < 0) continue;
    
        fpo = fopen( defaultcons[i],"wb");
        if (fpo == NULL) {
            kclose(fpi);
            continue;
        }

        fs = kfilelength(fpi);

        kread(fpi,&hittype[0],fs);
        fwrite(&hittype[0],fs,1,fpo);

        kclose(fpi);
        fclose(fpo);
    }
}

void loadefs(const char *filenam)
{
    char *mptr;
    int i;
    int fs,fp;

    fp = kopen4load(filenam,loadfromgrouponly);
    if( fp == -1 )
    {
        if( loadfromgrouponly == 1 )
            gameexit("\nMissing con file(s).");
        else {
            sprintf(tempbuf,
                "CON file \"%s\" was not found.\n"
                "\n"
                "Check that the \"%s\" file is in the JFDuke3D directory "
                "and try running the game again.",
                filenam, defaultduke3dgrp);
            gameexit(tempbuf);
            return;
        }
        
        //loadfromgrouponly = 1;
        return; //Not there
    }
    
    fs = kfilelength(fp);

    initprintf("Compiling: %s (%d bytes)\n",filenam,fs);

    mptr = Bmalloc(fs+1);
    if (!mptr) {
        Bsprintf(tempbuf,"Failed allocating %d byte CON text buffer.", fs+1);
        gameexit(tempbuf);
    }
    mptr[fs] = 0;
        
    textptr = mptr;
    
    kread(fp,textptr,fs);
    kclose(fp);

    //textptr[fs - 2] = 0;

    clearbuf(actorscrptr,MAXTILES,0L);  // JBF 20040531: MAXSPRITES? I think Todd meant MAXTILES...
    clearbufbyte(actortype,MAXTILES,0L);
    clearbufbyte(script,sizeof(script),0l); // JBF 20040531: yes? no?

    labelcnt = 0;
    scriptptr = script+1;
    warning = 0;
    error = 0;
    line_number = 1;
    total_lines = 0;

    strcpy(compilefile, filenam);   // JBF 20031130: Store currently compiling file name
    passone(); //Tokenize
    *script = encodescriptptr(scriptptr);

    Bfree(mptr);

    if(warning|error)
        initprintf("Found %d warning(s), %d error(s).\n",warning,error);

    if( error == 0 && warning != 0)
    {
        if( groupfile != -1 && loadfromgrouponly == 0 )
        {
            //initprintf("Warnings found when compiling CON files. Use default CONs instead? (Y/N)\n");
        i=wm_ynbox("CON File Compilation Warning", "Warnings found when compiling CON files. Use default CONs instead?");
        if (i) i = 'y';
        if(i == 'y' || i == 'Y' )
            {
                loadfromgrouponly = 1;
                initprintf(" Yes\n");
                return;
            }
        }
    }

    if(error)
    {
        if( loadfromgrouponly )
        {
            sprintf(buf,"\nError compiling CON files.");
            gameexit(buf);
        }
        else
        {
            if( groupfile != -1 && loadfromgrouponly == 0 )
            {
                //initprintf("Errors found when compiling CON files.  Use default CONs instead? (Y/N)\n");
                i=wm_ynbox("CON File Compilation Error", "Errors found when compiling CON files. Use default CONs instead?");
                if (i) i = 'y';
                if( i == 'y' || i == 'Y' )
                {
                    initprintf(" Yes\n");
                    loadfromgrouponly = 1;
                    return;
                }
                else gameexit("");
            }
        }
    }
    else
    {
        total_lines += line_number;
        initprintf("Code Size: %d bytes (%d labels).\n",(int)((scriptptr-script)<<2)-4,labelcnt);
    }
}

char dodge(spritetype *s)
{
    short i;
    int bx,by,mx,my,bxvect,byvect,mxvect,myvect,d;

    mx = s->x;
    my = s->y;
    mxvect = sintable[(s->ang+512)&2047]; myvect = sintable[s->ang&2047];

    for(i=headspritestat[4];i>=0;i=nextspritestat[i]) //weapons list
    {
        if( OW == i || SECT != s->sectnum)
            continue;

        bx = SX-mx;
        by = SY-my;
        bxvect = sintable[(SA+512)&2047]; byvect = sintable[SA&2047];

        if (mxvect*bx + myvect*by >= 0)
            if (bxvect*bx + byvect*by < 0)
        {
            d = bxvect*by - byvect*bx;
            if (klabs(d) < 65536*64)
            {
                s->ang -= 512+(TRAND&1024);
                return 1;
            }
        }
    }
    return 0;
}

short furthestangle(short i,short angs)
{
    short j, hitsect,hitwall,hitspr,furthest_angle=0, angincs;
    int hx, hy, hz, d, greatestd;
    spritetype *s = &sprite[i];

    greatestd = -(1<<30);
    angincs = 2048/angs;

    if(s->picnum != APLAYER)
        if( (g_t[0]&63) > 2 ) return( s->ang + 1024 );

    for(j=s->ang;j<(2048+s->ang);j+=angincs)
    {
        hitscan(s->x, s->y, s->z-(8<<8), s->sectnum,
            sintable[(j+512)&2047],
            sintable[j&2047],0,
            &hitsect,&hitwall,&hitspr,&hx,&hy,&hz,CLIPMASK1);

        d = klabs(hx-s->x) + klabs(hy-s->y);

        if(d > greatestd)
        {
            greatestd = d;
            furthest_angle = j;
        }
    }
    return (furthest_angle&2047);
}

short furthestcanseepoint(short i,spritetype *ts,int *dax,int *day)
{
    short j, hitsect,hitwall,hitspr, angincs, tempang;
    int hx, hy, hz, d, da;//, d, cd, ca,tempx,tempy,cx,cy;
    spritetype *s = &sprite[i];

    if( (g_t[0]&63) ) return -1;

    if(ud.multimode < 2 && ud.player_skill < 3)
        angincs = 2048/2;
    else angincs = 2048/(1+(TRAND&1));

    for(j=ts->ang;j<(2048+ts->ang);j+=(angincs-(TRAND&511)))
    {
        hitscan(ts->x, ts->y, ts->z-(16<<8), ts->sectnum,
            sintable[(j+512)&2047],
            sintable[j&2047],16384-(TRAND&32767),
            &hitsect,&hitwall,&hitspr,&hx,&hy,&hz,CLIPMASK1);

        d = klabs(hx-ts->x)+klabs(hy-ts->y);
        da = klabs(hx-s->x)+klabs(hy-s->y);

        if( d < da )
            if(cansee(hx,hy,hz,hitsect,s->x,s->y,s->z-(16<<8),s->sectnum))
        {
            *dax = hx;
            *day = hy;
            return hitsect;
        }
    }
    return -1;
}




void alterang(short a)
{
    short aang, angdif, goalang,j;
    int ticselapsed, *moveptr;

    moveptr = decodescriptptr(g_t[1]);

    ticselapsed = (g_t[0])&31;

    aang = g_sp->ang;

    g_sp->xvel += (*moveptr-g_sp->xvel)/5;
    if(g_sp->zvel < 648) g_sp->zvel += ((*(moveptr+1)<<4)-g_sp->zvel)/5;

    if(a&seekplayer)
    {
        j = ps[g_p].holoduke_on;

        if(j >= 0 && cansee(sprite[j].x,sprite[j].y,sprite[j].z,sprite[j].sectnum,g_sp->x,g_sp->y,g_sp->z,g_sp->sectnum) )
            g_sp->owner = j;
        else g_sp->owner = ps[g_p].i;

        if(sprite[g_sp->owner].picnum == APLAYER)
            goalang = getangle(hittype[g_i].lastvx-g_sp->x,hittype[g_i].lastvy-g_sp->y);
        else
            goalang = getangle(sprite[g_sp->owner].x-g_sp->x,sprite[g_sp->owner].y-g_sp->y);

        if(g_sp->xvel && g_sp->picnum != DRONE)
        {
            angdif = getincangle(aang,goalang);

            if(ticselapsed < 2)
            {
                if( klabs(angdif) < 256)
                {
                    j = 128-(TRAND&256);
                    g_sp->ang += j;
                    if( hits(g_i) < 844 )
                        g_sp->ang -= j;
                }
            }
            else if(ticselapsed > 18 && ticselapsed < 26) // choose
            {
                if(klabs(angdif>>2) < 128) g_sp->ang = goalang;
                else g_sp->ang += angdif>>2;
            }
        }
        else g_sp->ang = goalang;
    }

    if(ticselapsed < 1)
    {
        j = 2;
        if(a&furthestdir)
        {
            goalang = furthestangle(g_i,j);
            g_sp->ang = goalang;
            g_sp->owner = ps[g_p].i;
        }

        if(a&fleeenemy)
        {
            goalang = furthestangle(g_i,j);
            g_sp->ang = goalang; // += angdif; //  = getincangle(aang,goalang)>>1;
        }
    }
}

void move()
{
    int l, *moveptr;
    short j, a, goalang, angdif;
    int daxvel;

    a = g_sp->hitag;

    if(a == -1) a = 0;

    g_t[0]++;

    if(a&face_player)
    {
        if(ps[g_p].newowner >= 0)
            goalang = getangle(ps[g_p].oposx-g_sp->x,ps[g_p].oposy-g_sp->y);
        else goalang = getangle(ps[g_p].posx-g_sp->x,ps[g_p].posy-g_sp->y);
        angdif = getincangle(g_sp->ang,goalang)>>2;
        if(angdif > -8 && angdif < 0) angdif = 0;
        g_sp->ang += angdif;
    }

    if(a&spin)
        g_sp->ang += sintable[ ((g_t[0]<<3)&2047) ]>>6;

    if(a&face_player_slow)
    {
        if(ps[g_p].newowner >= 0)
            goalang = getangle(ps[g_p].oposx-g_sp->x,ps[g_p].oposy-g_sp->y);
        else goalang = getangle(ps[g_p].posx-g_sp->x,ps[g_p].posy-g_sp->y);
        angdif = ksgn(getincangle(g_sp->ang,goalang))<<5;
        if(angdif > -32 && angdif < 0)
        {
            angdif = 0;
            g_sp->ang = goalang;
        }
        g_sp->ang += angdif;
    }


    if((a&jumptoplayer) == jumptoplayer)
    {
        if(g_t[0] < 16)
            g_sp->zvel -= (sintable[(512+(g_t[0]<<4))&2047]>>5);
    }

    if(a&face_player_smart)
    {
        int newx,newy;

        newx = ps[g_p].posx+(ps[g_p].posxv/768);
        newy = ps[g_p].posy+(ps[g_p].posyv/768);
        goalang = getangle(newx-g_sp->x,newy-g_sp->y);
        angdif = getincangle(g_sp->ang,goalang)>>2;
        if(angdif > -8 && angdif < 0) angdif = 0;
        g_sp->ang += angdif;
    }

    if( g_t[1] == 0 || a == 0 )
    {
        if( ( badguy(g_sp) && g_sp->extra <= 0 ) || (hittype[g_i].bposx != g_sp->x) || (hittype[g_i].bposy != g_sp->y) )
        {
            hittype[g_i].bposx = g_sp->x;
            hittype[g_i].bposy = g_sp->y;
            setsprite(g_i,g_sp->x,g_sp->y,g_sp->z);
        }
        return;
    }

    moveptr = decodescriptptr(g_t[1]);

    if(a&geth) g_sp->xvel += (*moveptr-g_sp->xvel)>>1;
    if(a&getv) g_sp->zvel += ((*(moveptr+1)<<4)-g_sp->zvel)>>1;

    if(a&dodgebullet)
        dodge(g_sp);

    if(g_sp->picnum != APLAYER)
        alterang(a);

    if(g_sp->xvel > -6 && g_sp->xvel < 6 ) g_sp->xvel = 0;

    a = badguy(g_sp);

    if(g_sp->xvel || g_sp->zvel)
    {
        if(a && g_sp->picnum != ROTATEGUN)
        {
            if( (g_sp->picnum == DRONE || g_sp->picnum == COMMANDER) && g_sp->extra > 0)
            {
                if(g_sp->picnum == COMMANDER)
                {
                    hittype[g_i].floorz = l = getflorzofslope(g_sp->sectnum,g_sp->x,g_sp->y);
                    if( g_sp->z > (l-(8<<8)) )
                    {
                        if( g_sp->z > (l-(8<<8)) ) g_sp->z = l-(8<<8);
                        g_sp->zvel = 0;
                    }

                    hittype[g_i].ceilingz = l = getceilzofslope(g_sp->sectnum,g_sp->x,g_sp->y);
                    if( (g_sp->z-l) < (80<<8) )
                    {
                        g_sp->z = l+(80<<8);
                        g_sp->zvel = 0;
                    }
                }
                else
                {
                    if( g_sp->zvel > 0 )
                    {
                        hittype[g_i].floorz = l = getflorzofslope(g_sp->sectnum,g_sp->x,g_sp->y);
                        if( g_sp->z > (l-(30<<8)) )
                            g_sp->z = l-(30<<8);
                    }
                    else
                    {
                        hittype[g_i].ceilingz = l = getceilzofslope(g_sp->sectnum,g_sp->x,g_sp->y);
                        if( (g_sp->z-l) < (50<<8) )
                        {
                            g_sp->z = l+(50<<8);
                            g_sp->zvel = 0;
                        }
                    }
                }
            }
            else if(g_sp->picnum != ORGANTIC)
            {
                if(g_sp->zvel > 0 && hittype[g_i].floorz < g_sp->z)
                    g_sp->z = hittype[g_i].floorz;
                if( g_sp->zvel < 0)
                {
                    l = getceilzofslope(g_sp->sectnum,g_sp->x,g_sp->y);
                    if( (g_sp->z-l) < (66<<8) )
                    {
                        g_sp->z = l+(66<<8);
                        g_sp->zvel >>= 1;
                    }
                }
            }
        }
        else if(g_sp->picnum == APLAYER)
            if( (g_sp->z-hittype[g_i].ceilingz) < (32<<8) )
                g_sp->z = hittype[g_i].ceilingz+(32<<8);

        daxvel = g_sp->xvel;
        angdif = g_sp->ang;

        if( a && g_sp->picnum != ROTATEGUN )
        {
            if( g_x < 960 && g_sp->xrepeat > 16 )
            {

                daxvel = -(1024-g_x);
                angdif = getangle(ps[g_p].posx-g_sp->x,ps[g_p].posy-g_sp->y);

                if(g_x < 512)
                {
                    ps[g_p].posxv = 0;
                    ps[g_p].posyv = 0;
                }
                else
                {
                    ps[g_p].posxv = mulscale(ps[g_p].posxv,dukefriction-0x2000,16);
                    ps[g_p].posyv = mulscale(ps[g_p].posyv,dukefriction-0x2000,16);
                }
            }
            else if(g_sp->picnum != DRONE && g_sp->picnum != SHARK && g_sp->picnum != COMMANDER)
            {
                if( hittype[g_i].bposz != g_sp->z || ( ud.multimode < 2 && ud.player_skill < 2 ) )
                {
                    if( (g_t[0]&1) || ps[g_p].actorsqu == g_i ) return;
                    else daxvel <<= 1;
                }
                else
                {
                    if( (g_t[0]&3) || ps[g_p].actorsqu == g_i ) return;
                    else daxvel <<= 2;
                }
            }
        }

        hittype[g_i].movflag = movesprite(g_i,
            (daxvel*(sintable[(angdif+512)&2047]))>>14,
            (daxvel*(sintable[angdif&2047]))>>14,g_sp->zvel,CLIPMASK0);
   }

   if( a )
   {
       if (sector[g_sp->sectnum].ceilingstat&1)
           g_sp->shade += (sector[g_sp->sectnum].ceilingshade-g_sp->shade)>>1;
       else g_sp->shade += (sector[g_sp->sectnum].floorshade-g_sp->shade)>>1;

       if( sector[g_sp->sectnum].floorpicnum == MIRROR )
           deletesprite(g_i);
   }
}

char parse(void);

void parseifelse(int condition)
{
    if( condition )
    {
        insptr+=2;
        parse();
    }
    else
    {
        insptr = decodescriptptr(*(insptr+1));
        if(*insptr == 10)   //else
        {
            insptr+=2;
            parse();
        }
    }
}

char parse(void)
{
    int j, l, s;

    if(killit_flag) return 1;

    switch(*insptr)
    {
        case 3:     //ifrnd
            insptr++;
            parseifelse( rnd(*insptr));
            break;
        case 45:    //ifcanshoottarget

            if(g_x > 1024)
            {
                short temphit, sclip, angdif;

                if( badguy(g_sp) && g_sp->xrepeat > 56 )
                {
                    sclip = 3084;
                    angdif = 48;
                }
                else
                {
                    sclip = 768;
                    angdif = 16;
                }

                j = hitasprite(g_i,&temphit);
                if(j == (1<<30))
                {
                    parseifelse(1);
                    break;
                }
                if(j > sclip)
                {
                    if(temphit >= 0 && sprite[temphit].picnum == g_sp->picnum)
                        j = 0;
                    else
                    {
                        g_sp->ang += angdif;j = hitasprite(g_i,&temphit);g_sp->ang -= angdif;
                        if(j > sclip)
                        {
                            if(temphit >= 0 && sprite[temphit].picnum == g_sp->picnum)
                                j = 0;
                            else
                            {
                                g_sp->ang -= angdif;j = hitasprite(g_i,&temphit);g_sp->ang += angdif;
                                if( j > 768 )
                                {
                                    if(temphit >= 0 && sprite[temphit].picnum == g_sp->picnum)
                                        j = 0;
                                    else j = 1;
                                }
                                else j = 0;
                            }
                        }
                        else j = 0;
                    }
                }
                else j =  0;
            }
            else j = 1;

            parseifelse(j);
            break;
        case 91:    //ifcanseetarget
            j = cansee(g_sp->x,g_sp->y,g_sp->z-((TRAND&41)<<8),g_sp->sectnum,ps[g_p].posx,ps[g_p].posy,ps[g_p].posz/*-((TRAND&41)<<8)*/,sprite[ps[g_p].i].sectnum);
            parseifelse(j);
            if( j ) hittype[g_i].timetosleep = SLEEPTIME;
            break;

        case 49:    //ifactornotstayput
            parseifelse(hittype[g_i].actorstayput == -1);
            break;
        case 5:     //ifcansee
        {
            spritetype *s;
            short sect;

            if(ps[g_p].holoduke_on >= 0)
            {
                s = &sprite[ps[g_p].holoduke_on];
                j = cansee(g_sp->x,g_sp->y,g_sp->z-(TRAND&((32<<8)-1)),g_sp->sectnum,
                       s->x,s->y,s->z,s->sectnum);
                if(j == 0)
                    s = &sprite[ps[g_p].i];
            }
            else s = &sprite[ps[g_p].i];

            j = cansee(g_sp->x,g_sp->y,g_sp->z-(TRAND&((47<<8))),g_sp->sectnum,
                s->x,s->y,s->z-(24<<8),s->sectnum);

            if(j == 0)
            {
                if( ( klabs(hittype[g_i].lastvx-g_sp->x)+klabs(hittype[g_i].lastvy-g_sp->y) ) <
                    ( klabs(hittype[g_i].lastvx-s->x)+klabs(hittype[g_i].lastvy-s->y) ) )
                        j = 0;

                if( j == 0 )
                {
                    j = furthestcanseepoint(g_i,s,&hittype[g_i].lastvx,&hittype[g_i].lastvy);

                    if(j == -1) j = 0;
                    else j = 1;
                }
            }
            else
            {
                hittype[g_i].lastvx = s->x;
                hittype[g_i].lastvy = s->y;
            }

            if( j == 1 && ( g_sp->statnum == 1 || g_sp->statnum == 6 ) )
                hittype[g_i].timetosleep = SLEEPTIME;

            parseifelse(j == 1);
            break;
        }

        case 6:     //ifhitweapon
            parseifelse(ifhitbyweapon(g_i) >= 0);
            break;
        case 27:    //ifsquished
            parseifelse( ifsquished(g_i, g_p) == 1);
            break;
        case 26:    //ifdead
            {
                j = g_sp->extra;
                if(g_sp->picnum == APLAYER)
                    j--;
                parseifelse(j < 0);
            }
            break;
        case 24:    //ai
            insptr++;
            g_t[5] = *insptr;                          // Ai
            g_t[4] = *(decodescriptptr(g_t[5]));       // Action
            g_t[1] = *(decodescriptptr(g_t[5])+1);     // move
            g_sp->hitag = *(decodescriptptr(g_t[5])+2);
            g_t[0] = g_t[2] = g_t[3] = 0;
            if(g_sp->hitag&random_angle)
                g_sp->ang = TRAND&2047;
            insptr++;
            break;
        case 7:     //action
            insptr++;
            g_t[2] = 0;
            g_t[3] = 0;
            g_t[4] = *insptr;
            insptr++;
            break;

        case 8:     //ifpdistl
            insptr++;
            parseifelse(g_x < *insptr);
            if(g_x > MAXSLEEPDIST && hittype[g_i].timetosleep == 0)
                hittype[g_i].timetosleep = SLEEPTIME;
            break;
        case 9:     //ifpdistg
            insptr++;
            parseifelse(g_x > *insptr);
            if(g_x > MAXSLEEPDIST && hittype[g_i].timetosleep == 0)
                hittype[g_i].timetosleep = SLEEPTIME;
            break;
        case 10:    //else
            insptr = decodescriptptr(*(insptr+1));
            break;
        case 100:   //addstrength
            insptr++;
            g_sp->extra += *insptr;
            insptr++;
            break;
        case 11:    //strength
            insptr++;
            g_sp->extra = *insptr;
            insptr++;
            break;
        case 94:    //ifgotweaponce
            insptr++;

            if(ud.coop >= 1 && ud.multimode > 1)
            {
                if(*insptr == 0)
                {
                    for(j=0;j < ps[g_p].weapreccnt;j++)
                        if( ps[g_p].weaprecs[j] == g_sp->picnum )
                            break;

                    parseifelse(j < ps[g_p].weapreccnt && g_sp->owner == g_i);
                }
                else if(ps[g_p].weapreccnt < 16)
                {
                    ps[g_p].weaprecs[ps[g_p].weapreccnt++] = g_sp->picnum;
                    parseifelse(g_sp->owner == g_i);
                }
            }
            else parseifelse(0);
            break;
        case 95:    //getlastpal
            insptr++;
            if(g_sp->picnum == APLAYER)
                g_sp->pal = ps[g_sp->yvel].palookup;
            else g_sp->pal = hittype[g_i].tempang;
            hittype[g_i].tempang = 0;
            break;
        case 104:   //tossweapon
            insptr++;
            checkweapons(&ps[g_sp->yvel]);
            break;
        case 106:   //nullop
            insptr++;
            break;
        case 97:    //mikesnd
            insptr++;
            if(!isspritemakingsound(g_i,g_sp->yvel))
                spritesound(g_sp->yvel,g_i);
            break;
        case 96:    //pkick
            insptr++;

            if( ud.multimode > 1 && g_sp->picnum == APLAYER )
            {
                if(ps[otherp].quick_kick == 0)
                    ps[otherp].quick_kick = 14;
            }
            else if(g_sp->picnum != APLAYER && ps[g_p].quick_kick == 0)
                ps[g_p].quick_kick = 14;
            break;
        case 28:    //sizeto
            insptr++;

            // JBF 20030805: As I understand it, if xrepeat becomes 0 it basically kills the
            // sprite, which is why the "sizeto 0 41" calls in 1.3d became "sizeto 4 41" in
            // 1.4, so instead of patching the CONs I'll surruptitiously patch the code here
            if (!PLUTOPAK && *insptr == 0) *insptr = 4;
        
            j = ((*insptr)-g_sp->xrepeat)<<1;
            g_sp->xrepeat += ksgn(j);

            insptr++;

            if( ( g_sp->picnum == APLAYER && g_sp->yrepeat < 36 ) || *insptr < g_sp->yrepeat || ((g_sp->yrepeat*(tilesizy[g_sp->picnum]+8))<<2) < (hittype[g_i].floorz - hittype[g_i].ceilingz) )
            {
                j = ((*insptr)-g_sp->yrepeat)<<1;
                if( klabs(j) ) g_sp->yrepeat += ksgn(j);
            }

            insptr++;

            break;
        case 99:    //sizeat
            insptr++;
            g_sp->xrepeat = (unsigned char) *insptr;
            insptr++;
            g_sp->yrepeat = (unsigned char) *insptr;
            insptr++;
            break;
        case 13:    //shoot
            insptr++;
            shoot(g_i,(short)*insptr);
            insptr++;
            break;
        case 87:    //soundonce
            insptr++;
            if(!isspritemakingsound(g_i,*insptr))
                spritesound((short) *insptr,g_i);
            insptr++;
            break;
        case 89:    //stopsound
            insptr++;
            if(isspritemakingsound(g_i,*insptr))
                stopspritesound((short)*insptr,g_i);
            insptr++;
            break;
        case 92:    //globalsound
            insptr++;
            if(g_p == screenpeek || ud.coop==1)
                spritesound((short) *insptr,ps[screenpeek].i);
            insptr++;
            break;
        case 15:    //sound
            insptr++;
            spritesound((short) *insptr,g_i);
            insptr++;
            break;
        case 84:    //top
            insptr++;
            ps[g_p].tipincs = 26;
            break;
        case 16:    //fall
            insptr++;
            g_sp->xoffset = 0;
            g_sp->yoffset = 0;
//            if(!gotz)
            {
                int c;

                if( floorspace(g_sp->sectnum) )
                    c = 0;
                else
                {
                    if( ceilingspace(g_sp->sectnum) || sector[g_sp->sectnum].lotag == 2)
                        c = gc/6;
                    else c = gc;
                }

                if( hittype[g_i].cgg <= 0 || (sector[g_sp->sectnum].floorstat&2) )
                {
                    getglobalz(g_i);
                    hittype[g_i].cgg = 6;
                }
                else hittype[g_i].cgg --;

                if( g_sp->z < (hittype[g_i].floorz-FOURSLEIGHT) )
                {
                    g_sp->zvel += c;
                    g_sp->z+=g_sp->zvel;

                    if(g_sp->zvel > 6144) g_sp->zvel = 6144;
                }
                else
                {
                    g_sp->z = hittype[g_i].floorz - FOURSLEIGHT;

                    if( badguy(g_sp) || ( g_sp->picnum == APLAYER && g_sp->owner >= 0) )
                    {

                        if( g_sp->zvel > 3084 && g_sp->extra <= 1)
                        {
                            if(g_sp->pal != 1 && g_sp->picnum != DRONE)
                            {
                                if(g_sp->picnum == APLAYER && g_sp->extra > 0)
                                    goto SKIPJIBS;
                                guts(g_sp,JIBS6,15,g_p);
                                spritesound(SQUISHED,g_i);
                                spawn(g_i,BLOODPOOL);
                            }

                            SKIPJIBS:

                            hittype[g_i].picnum = SHOTSPARK1;
                            hittype[g_i].extra = 1;
                            g_sp->zvel = 0;
                        }
                        else if(g_sp->zvel > 2048 && sector[g_sp->sectnum].lotag != 1)
                        {

                            j = g_sp->sectnum;
                            pushmove(&g_sp->x,&g_sp->y,&g_sp->z,(short*)&j,128L,(4L<<8),(4L<<8),CLIPMASK0);
                            if(j != g_sp->sectnum && j >= 0 && j < MAXSECTORS)
                                changespritesect(g_i,j);

                            spritesound(THUD,g_i);
                        }
                    }
                    if(sector[g_sp->sectnum].lotag == 1)
                        switch (g_sp->picnum)
                        {
                            case OCTABRAIN:
                            case COMMANDER:
                            case DRONE:
                                break;
                            default:
                                g_sp->z += (24<<8);
                                break;
                        }
                    else g_sp->zvel = 0;
                }
            }

            break;
        case 4:     //enda
        case 12:    //break
        case 18:    //ends
            return 1;
        case 30:    //}
            insptr++;
            return 1;
        case 2:     //addammo
            insptr++;
            if( ps[g_p].ammo_amount[*insptr] >= max_ammo_amount[*insptr] )
            {
                killit_flag = 2;
                break;
            }
            addammo( *insptr, &ps[g_p], *(insptr+1) );
            if(ps[g_p].curr_weapon == KNEE_WEAPON)
                if( ps[g_p].gotweapon[*insptr] )
                    addweapon( &ps[g_p], *insptr );
            insptr += 2;
            break;
        case 86:    //money
            insptr++;
            lotsofmoney(g_sp,*insptr);
            insptr++;
            break;
        case 102:   //mail
            insptr++;
            lotsofmail(g_sp,*insptr);
            insptr++;
            break;
        case 105:   //sleeptime
            insptr++;
            hittype[g_i].timetosleep = (short)*insptr;
            insptr++;
            break;
        case 103:   //paper
            insptr++;
            lotsofpaper(g_sp,*insptr);
            insptr++;
            break;
        case 88:    //addkills
            insptr++;
            ps[g_p].actors_killed += *insptr;
            hittype[g_i].actorstayput = -1;
            insptr++;
            break;
        case 93:    //lotsofglass
            insptr++;
            spriteglass(g_i,*insptr);
            insptr++;
            break;
        case 22:    //killit
            insptr++;
            killit_flag = 1;
            break;
        case 23:    //addweapon
            insptr++;
            if( ps[g_p].gotweapon[*insptr] == 0 ) {
                if (!(ps[g_p].weaponswitch & 1)) addweaponnoswitch(&ps[g_p], *insptr);
                else addweapon( &ps[g_p], *insptr );
            }
            else if( ps[g_p].ammo_amount[*insptr] >= max_ammo_amount[*insptr] )
            {
                 killit_flag = 2;
                 break;
            }
            addammo( *insptr, &ps[g_p], *(insptr+1) );
            if(ps[g_p].curr_weapon == KNEE_WEAPON)
                if( ps[g_p].gotweapon[*insptr] && (ps[g_p].weaponswitch & 1) )
                    addweapon( &ps[g_p], *insptr );
            insptr+=2;
            break;
        case 68:    //debug
            insptr++;
            printf("%d\n",*insptr);
            insptr++;
            break;
        case 69:    //endofgame
            insptr++;
            ps[g_p].timebeforeexit = *insptr;
            ps[g_p].customexitsound = -1;
            ud.eog = 1;
            insptr++;
            break;
        case 25:    //addphealth
            insptr++;

            if(ps[g_p].newowner >= 0)
            {
                ps[g_p].newowner = -1;
                ps[g_p].posx = ps[g_p].oposx;
                ps[g_p].posy = ps[g_p].oposy;
                ps[g_p].posz = ps[g_p].oposz;
                ps[g_p].ang = ps[g_p].oang;
                updatesector(ps[g_p].posx,ps[g_p].posy,&ps[g_p].cursectnum);
                setpal(&ps[g_p]);

                j = headspritestat[1];
                while(j >= 0)
                {
                    if(sprite[j].picnum==CAMERA1)
                        sprite[j].yvel = 0;
                    j = nextspritestat[j];
                }
            }

            j = sprite[ps[g_p].i].extra;

            if(g_sp->picnum != ATOMICHEALTH)
            {
                if( j > max_player_health && *insptr > 0 )
                {
                    insptr++;
                    break;
                }
                else
                {
                    if(j > 0)
                        j += *insptr;
                    if ( j > max_player_health && *insptr > 0 )
                        j = max_player_health;
                }
            }
            else
            {
                if( j > 0 )
                    j += *insptr;
                if ( j > (max_player_health<<1) )
                    j = (max_player_health<<1);
            }

            if(j < 0) j = 0;

            if(ud.god == 0)
            {
                if(*insptr > 0)
                {
                    if( ( j - *insptr ) < (max_player_health>>2) &&
                        j >= (max_player_health>>2) )
                            spritesound(DUKE_GOTHEALTHATLOW,ps[g_p].i);

                    ps[g_p].last_extra = j;
                }

                sprite[ps[g_p].i].extra = j;
            }

            insptr++;
            break;
        case 17:    //state
            {
                int *tempscrptr;

                tempscrptr = insptr+2;

                insptr = decodescriptptr(*(insptr+1));
                while(1) if(parse()) break;
                insptr = tempscrptr;
            }
            break;
        case 29:    //{
            insptr++;
            while(1) if(parse()) break;
            break;
        case 32:    //move
            g_t[0]=0;
            insptr++;
            g_t[1] = *insptr;
            insptr++;
            g_sp->hitag = *insptr;
            insptr++;
            if(g_sp->hitag&random_angle)
                g_sp->ang = TRAND&2047;
            break;
        case 31:    //spawn
            insptr++;
            if(g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
                spawn(g_i,*insptr);
            insptr++;
            break;
        case 33:    //ifwasweapon
            insptr++;
            parseifelse( hittype[g_i].picnum == *insptr);
            break;
        case 21:    //ifai
            insptr++;
            parseifelse(g_t[5] == *insptr);
            break;
        case 34:    //ifaction
            insptr++;
            parseifelse(g_t[4] == *insptr);
            break;
        case 35:    //ifactioncount
            insptr++;
            parseifelse(g_t[2] >= *insptr);
            break;
        case 36:    //resetactioncount
            insptr++;
            g_t[2] = 0;
            break;
        case 37:    //debris
            {
                short dnum;

                insptr++;
                dnum = *insptr;
                insptr++;

                if(g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
                    for(j=(*insptr)-1;j>=0;j--)
                {
                    if(g_sp->picnum == BLIMP && dnum == SCRAP1)
                        s = 0;
                    else s = (TRAND%3);

                    l = EGS(g_sp->sectnum,
                            g_sp->x+(TRAND&255)-128,g_sp->y+(TRAND&255)-128,g_sp->z-(8<<8)-(TRAND&8191),
                            dnum+s,g_sp->shade,32+(TRAND&15),32+(TRAND&15),
                            TRAND&2047,(TRAND&127)+32,
                            -(TRAND&2047),g_i,5);
                    if(g_sp->picnum == BLIMP && dnum == SCRAP1)
                        sprite[l].yvel = weaponsandammosprites[j%14];
                    else sprite[l].yvel = -1;
                    sprite[l].pal = g_sp->pal;
                }
                insptr++;
            }
            break;
        case 52:    //count
            insptr++;
            g_t[0] = (short) *insptr;
            insptr++;
            break;
        case 101:   //cstator
            insptr++;
            g_sp->cstat |= (short)*insptr;
            insptr++;
            break;
        case 110:   //clipdist
            insptr++;
            g_sp->clipdist = (short) *insptr;
            insptr++;
            break;
        case 40:    //cstat
            insptr++;
            g_sp->cstat = (short) *insptr;
            insptr++;
            break;
        case 41:    //ifmove
            insptr++;
            parseifelse(g_t[1] == *insptr);
            break;
        case 42:    //resetplayer
            insptr++;

            if(ud.multimode < 2)
            {
                if( lastsavedpos >= 0 && ud.recstat != 2 )
                {
                    ps[g_p].gm = MODE_MENU;
                    KB_ClearKeyDown(sc_Space);
                    cmenu(15000);
                }
                else ps[g_p].gm = MODE_RESTART;
                killit_flag = 2;
            }
            else
            {
                pickrandomspot(g_p);
                g_sp->x = hittype[g_i].bposx = ps[g_p].bobposx = ps[g_p].oposx = ps[g_p].posx;
                g_sp->y = hittype[g_i].bposy = ps[g_p].bobposy = ps[g_p].oposy =ps[g_p].posy;
                g_sp->z = hittype[g_i].bposy = ps[g_p].oposz =ps[g_p].posz;
                updatesector(ps[g_p].posx,ps[g_p].posy,&ps[g_p].cursectnum);
                setsprite(ps[g_p].i,ps[g_p].posx,ps[g_p].posy,ps[g_p].posz+PHEIGHT);
                g_sp->cstat = 257;

                g_sp->shade = -12;
                g_sp->clipdist = 64;
                g_sp->xrepeat = 42;
                g_sp->yrepeat = 36;
                g_sp->owner = g_i;
                g_sp->xoffset = 0;
                g_sp->pal = ps[g_p].palookup;

                ps[g_p].last_extra = g_sp->extra = max_player_health;
                ps[g_p].wantweaponfire = -1;
                ps[g_p].horiz = 100;
                ps[g_p].on_crane = -1;
                ps[g_p].frag_ps = g_p;
                ps[g_p].horizoff = 0;
                ps[g_p].opyoff = 0;
                ps[g_p].wackedbyactor = -1;
                ps[g_p].shield_amount = max_armour_amount;
                ps[g_p].dead_flag = 0;
                ps[g_p].pals_time = 0;
                ps[g_p].footprintcount = 0;
                ps[g_p].weapreccnt = 0;
                ps[g_p].fta = 0;
                ps[g_p].ftq = 0;
                ps[g_p].posxv = ps[g_p].posyv = 0;
                ps[g_p].rotscrnang = 0;
        // JBF 20031220: I don't reset orotscrnang otherwise we don't get a smooth reset

                ps[g_p].falling_counter = 0;

                hittype[g_i].extra = -1;
                hittype[g_i].owner = g_i;

                hittype[g_i].cgg = 0;
                hittype[g_i].movflag = 0;
                hittype[g_i].tempang = 0;
                hittype[g_i].actorstayput = -1;
                hittype[g_i].dispicnum = 0;
                hittype[g_i].owner = ps[g_p].i;

                resetinventory(g_p);
                resetweapons(g_p);

                cameradist = 0;
                cameraclock = totalclock;
            }
            setpal(&ps[g_p]);

            break;
        case 43:    //ifonwater
            parseifelse( klabs(g_sp->z-sector[g_sp->sectnum].floorz) < (32<<8) && sector[g_sp->sectnum].lotag == 1);
            break;
        case 44:    //ifinwater
            parseifelse( sector[g_sp->sectnum].lotag == 2);
            break;
        case 46:    //ifcount
            insptr++;
            parseifelse(g_t[0] >= *insptr);
            break;
        case 53:    //ifactor
            insptr++;
            parseifelse(g_sp->picnum == *insptr);
            break;
        case 47:    //resetcount
            insptr++;
            g_t[0] = 0;
            break;
        case 48:    //addinventory
            insptr+=2;
            switch(*(insptr-1))
            {
                case 0:
                    ps[g_p].steroids_amount = *insptr;
                    ps[g_p].inven_icon = 2;
                    break;
                case 1:
                    ps[g_p].shield_amount +=          *insptr;// 100;
                    if(ps[g_p].shield_amount > max_player_health)
                        ps[g_p].shield_amount = max_player_health;
                    break;
                case 2:
                    ps[g_p].scuba_amount =             *insptr;// 1600;
                    ps[g_p].inven_icon = 6;
                    break;
                case 3:
                    ps[g_p].holoduke_amount =          *insptr;// 1600;
                    ps[g_p].inven_icon = 3;
                    break;
                case 4:
                    ps[g_p].jetpack_amount =           *insptr;// 1600;
                    ps[g_p].inven_icon = 4;
                    break;
                case 6:
                    switch(g_sp->pal)
                    {
                        case  0: ps[g_p].got_access |= 1;break;
                        case 21: ps[g_p].got_access |= 2;break;
                        case 23: ps[g_p].got_access |= 4;break;
                    }
                    break;
                case 7:
                    ps[g_p].heat_amount = *insptr;
                    ps[g_p].inven_icon = 5;
                    break;
                case 9:
                    ps[g_p].inven_icon = 1;
                    ps[g_p].firstaid_amount = *insptr;
                    break;
                case 10:
                    ps[g_p].inven_icon = 7;
                    ps[g_p].boot_amount = *insptr;
                    break;
            }
            insptr++;
            break;
        case 50:    //hitradius
            hitradius(g_i,*(insptr+1),*(insptr+2),*(insptr+3),*(insptr+4),*(insptr+5));
            insptr+=6;
            break;
        case 51:    //ifp
            {
                insptr++;

                l = *insptr;
                j = 0;

                s = g_sp->xvel;

                if( (l&8) && ps[g_p].on_ground && (sync[g_p].bits&2) )
                       j = 1;
                else if( (l&16) && ps[g_p].jumping_counter == 0 && !ps[g_p].on_ground &&
                    ps[g_p].poszv > 2048 )
                        j = 1;
                else if( (l&32) && ps[g_p].jumping_counter > 348 )
                       j = 1;
                else if( (l&1) && s >= 0 && s < 8)
                       j = 1;
                else if( (l&2) && s >= 8 && !(sync[g_p].bits&(1<<5)) )
                       j = 1;
                else if( (l&4) && s >= 8 && sync[g_p].bits&(1<<5) )
                       j = 1;
                else if( (l&64) && ps[g_p].posz < (g_sp->z-(48<<8)) )
                       j = 1;
                else if( (l&128) && s <= -8 && !(sync[g_p].bits&(1<<5)) )
                       j = 1;
                else if( (l&256) && s <= -8 && (sync[g_p].bits&(1<<5)) )
                       j = 1;
                else if( (l&512) && ( ps[g_p].quick_kick > 0 || ( ps[g_p].curr_weapon == KNEE_WEAPON && ps[g_p].kickback_pic > 0 ) ) )
                       j = 1;
                else if( (l&1024) && sprite[ps[g_p].i].xrepeat < 32 )
                       j = 1;
                else if( (l&2048) && ps[g_p].jetpack_on )
                       j = 1;
                else if( (l&4096) && ps[g_p].steroids_amount > 0 && ps[g_p].steroids_amount < 400 )
                       j = 1;
                else if( (l&8192) && ps[g_p].on_ground)
                       j = 1;
                else if( (l&16384) && sprite[ps[g_p].i].xrepeat > 32 && sprite[ps[g_p].i].extra > 0 && ps[g_p].timebeforeexit == 0 )
                       j = 1;
                else if( (l&32768) && sprite[ps[g_p].i].extra <= 0)
                       j = 1;
                else if( (l&65536L) )
                {
                    if(g_sp->picnum == APLAYER && ud.multimode > 1)
                        j = getincangle(ps[otherp].ang,getangle(ps[g_p].posx-ps[otherp].posx,ps[g_p].posy-ps[otherp].posy));
                    else
                        j = getincangle(ps[g_p].ang,getangle(g_sp->x-ps[g_p].posx,g_sp->y-ps[g_p].posy));

                    if( j > -128 && j < 128 )
                        j = 1;
                    else
                        j = 0;
                }

                parseifelse((int) j);

            }
            break;
        case 56:    //ifstrength
            insptr++;
            parseifelse(g_sp->extra <= *insptr);
            break;
        case 58:    //guts
            insptr += 2;
            guts(g_sp,*(insptr-1),*insptr,g_p);
            insptr++;
            break;
        case 59:    //ifspawnedby
            insptr++;
//            if(g_sp->owner >= 0 && sprite[g_sp->owner].picnum == *insptr)
  //              parseifelse(1);
//            else
            parseifelse( hittype[g_i].picnum == *insptr);
            break;
        case 61:    //wackplayer
            insptr++;
            forceplayerangle(&ps[g_p]);
            return 0;
        case 62:    //ifgapzl
            insptr++;
            parseifelse( (( hittype[g_i].floorz - hittype[g_i].ceilingz ) >> 8 ) < *insptr);
            break;
        case 63:    //ifhitspace
            parseifelse( sync[g_p].bits&(1<<29));
            break;
        case 64:    //ifoutside
            parseifelse(sector[g_sp->sectnum].ceilingstat&1);
            break;
        case 65:    //ifmultiplayer
            parseifelse(ud.multimode > 1);
            break;
        case 66:    //operate
            insptr++;
            if( sector[g_sp->sectnum].lotag == 0 )
            {
                neartag(g_sp->x,g_sp->y,g_sp->z-(32<<8),g_sp->sectnum,g_sp->ang,&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,768L,1);
                if( neartagsector >= 0 && isanearoperator(sector[neartagsector].lotag) )
                    if( (sector[neartagsector].lotag&0xff) == 23 || sector[neartagsector].floorz == sector[neartagsector].ceilingz )
                        if( (sector[neartagsector].lotag&16384) == 0 )
                            if( (sector[neartagsector].lotag&32768) == 0 )
                        {
                            j = headspritesect[neartagsector];
                            while(j >= 0)
                            {
                                if(sprite[j].picnum == ACTIVATOR)
                                    break;
                                j = nextspritesect[j];
                            }
                            if(j == -1)
                                operatesectors(neartagsector,g_i);
                        }
            }
            break;
        case 67:    //ifinspace
            parseifelse(ceilingspace(g_sp->sectnum));
            break;

        case 74:    //spritepal
            insptr++;
            if(g_sp->picnum != APLAYER)
                hittype[g_i].tempang = g_sp->pal;
            g_sp->pal = *insptr;
            insptr++;
            break;

        case 77:    //cactor
            insptr++;
            g_sp->picnum = *insptr;
            insptr++;
            break;

        case 70:    //ifbulletnear
            parseifelse( dodge(g_sp) == 1);
            break;
        case 71:    //ifrespawn
            if( badguy(g_sp) )
                parseifelse( ud.respawn_monsters );
            else if( inventory(g_sp) )
                parseifelse( ud.respawn_inventory );
            else
                parseifelse( ud.respawn_items );
            break;
        case 72:    //iffloordistl
            insptr++;
//            getglobalz(g_i);
            parseifelse( (hittype[g_i].floorz - g_sp->z) <= ((*insptr)<<8));
            break;
        case 73:    //ifceilingdistl
            insptr++;
//            getglobalz(g_i);
            parseifelse( ( g_sp->z - hittype[g_i].ceilingz ) <= ((*insptr)<<8));
            break;
        case 14:    //palfrom

            insptr++;
            ps[g_p].pals_time = *insptr;
            insptr++;
            for(j=0;j<3;j++)
            {
                ps[g_p].pals[j] = *insptr;
                insptr++;
            }
            break;

        case 78:    //ifphealthl
            insptr++;
            parseifelse( sprite[ps[g_p].i].extra < *insptr);
            break;

        case 75:    //ifpinventory
            {
                insptr++;
                j = 0;
                switch(*(insptr++))
                {
                    case 0:if( ps[g_p].steroids_amount != *insptr)
                           j = 1;
                        break;
                    case 1:if(ps[g_p].shield_amount != max_player_health )
                            j = 1;
                        break;
                    case 2:if(ps[g_p].scuba_amount != *insptr) j = 1;break;
                    case 3:if(ps[g_p].holoduke_amount != *insptr) j = 1;break;
                    case 4:if(ps[g_p].jetpack_amount != *insptr) j = 1;break;
                    case 6:
                        switch(g_sp->pal)
                        {
                            case  0: if(ps[g_p].got_access&1) j = 1;break;
                            case 21: if(ps[g_p].got_access&2) j = 1;break;
                            case 23: if(ps[g_p].got_access&4) j = 1;break;
                        }
                        break;
                    case 7:if(ps[g_p].heat_amount != *insptr) j = 1;break;
                    case 9:
                        if(ps[g_p].firstaid_amount != *insptr) j = 1;break;
                    case 10:
                        if(ps[g_p].boot_amount != *insptr) j = 1;break;
                }

                parseifelse(j);
                break;
            }
        case 38:    //pstomp
            insptr++;
            if( ps[g_p].knee_incs == 0 && sprite[ps[g_p].i].xrepeat >= 40 )
                if( cansee(g_sp->x,g_sp->y,g_sp->z-(4<<8),g_sp->sectnum,ps[g_p].posx,ps[g_p].posy,ps[g_p].posz+(16<<8),sprite[ps[g_p].i].sectnum) )
            {
                ps[g_p].knee_incs = 1;
                if(ps[g_p].weapon_pos == 0)
                    ps[g_p].weapon_pos = -1;
                ps[g_p].actorsqu = g_i;
            }
            break;
        case 90:    //ifawayfromwall
            {
                short s1;

                s1 = g_sp->sectnum;

                j = 0;

                    updatesector(g_sp->x+108,g_sp->y+108,&s1);
                    if( s1 == g_sp->sectnum )
                    {
                        updatesector(g_sp->x-108,g_sp->y-108,&s1);
                        if( s1 == g_sp->sectnum )
                        {
                            updatesector(g_sp->x+108,g_sp->y-108,&s1);
                            if( s1 == g_sp->sectnum )
                            {
                                updatesector(g_sp->x-108,g_sp->y+108,&s1);
                                if( s1 == g_sp->sectnum )
                                    j = 1;
                            }
                        }
                    }
                    parseifelse( j );
            }

            break;
        case 80:    //quote
            insptr++;
            FTA(*insptr,&ps[g_p]);
            insptr++;
            break;
        case 81:    //ifinouterspace
            parseifelse( floorspace(g_sp->sectnum));
            break;
        case 82:    //ifnotmoving
            parseifelse( (hittype[g_i].movflag&49152) > 16384 );
            break;
        case 83:    //respawnhitag
            insptr++;
            switch(g_sp->picnum)
            {
                case FEM1:
                case FEM2:
                case FEM3:
                case FEM4:
                case FEM5:
                case FEM6:
                case FEM7:
                case FEM8:
                case FEM9:
                case FEM10:
                case PODFEM1:
                case NAKED1:
                case STATUE:
                    if(g_sp->yvel) operaterespawns(g_sp->yvel);
                    break;
                default:
                    if(g_sp->hitag >= 0) operaterespawns(g_sp->hitag);
                    break;
            }
            break;
        case 85:    //ifspritepal
            insptr++;
            parseifelse( g_sp->pal == *insptr);
            break;

        case 111:   //ifangdiffl
            insptr++;
            j = klabs(getincangle(ps[g_p].ang,g_sp->ang));
            parseifelse( j <= *insptr);
            break;

        case 109:   //ifnosounds

            for(j=1;j<NUM_SOUNDS;j++)
                if( SoundOwner[j][0].i == g_i )
                    break;

            parseifelse( j == NUM_SOUNDS );
            break;
        default:
            killit_flag = 1;
            break;
    }
    return 0;
}

void execute(short i,short p,int x)
{
    char done;

    g_i = i;
    g_p = p;
    g_x = x;
    g_sp = &sprite[g_i];
    g_t = &hittype[g_i].temp_data[0];

    if( actorscrptr[g_sp->picnum] == 0 ) return;

    insptr = 4 + (actorscrptr[g_sp->picnum]);

    killit_flag = 0;

    if(g_sp->sectnum < 0 || g_sp->sectnum >= MAXSECTORS)
    {
        if(badguy(g_sp))
            ps[g_p].actors_killed++;
        deletesprite(g_i);
        return;
    }

    if(g_t[4])
    {
        g_sp->lotag += TICSPERFRAME;
        if(g_sp->lotag > *(decodescriptptr(g_t[4])+4) )
        {
            g_t[2]++;
            g_sp->lotag = 0;
            g_t[3] +=  *(decodescriptptr(g_t[4])+3);
        }
        if( klabs(g_t[3]) >= klabs( *(decodescriptptr(g_t[4])+1) * *(decodescriptptr(g_t[4])+3) ) )
            g_t[3] = 0;
    }

    do
        done = parse();
    while( done == 0 );

    if(killit_flag == 1)
    {
        if(ps[g_p].actorsqu == g_i)
            ps[g_p].actorsqu = -1;
        deletesprite(g_i);
    }
    else
    {
        move();

        if( g_sp->statnum == 1)
        {
            if( badguy(g_sp) )
            {
                if( g_sp->xrepeat > 60 ) return;
                if( ud.respawn_monsters == 1 && g_sp->extra <= 0 ) return;
            }
            else if( ud.respawn_items == 1 && (g_sp->cstat&32768) ) return;

            if(hittype[g_i].timetosleep > 1)
                hittype[g_i].timetosleep--;
            else if(hittype[g_i].timetosleep == 1)
                 changespritestat(g_i,2);
        }

        else if(g_sp->statnum == 6)
            switch(g_sp->picnum)
            {
                case RUBBERCAN:
                case EXPLODINGBARREL:
                case WOODENHORSE:
                case HORSEONSIDE:
                case CANWITHSOMETHING:
                case FIREBARREL:
                case NUKEBARREL:
                case NUKEBARRELDENTED:
                case NUKEBARRELLEAKED:
                case TRIPBOMB:
                case EGG:
                    if(hittype[g_i].timetosleep > 1)
                        hittype[g_i].timetosleep--;
                    else if(hittype[g_i].timetosleep == 1)
                        changespritestat(g_i,2);
                    break;
            }
    }
}





// "Duke 2000"
// "Virchua Duke"
// "Son of Death
// "Cromium"
// "Potent"
// "Flotsom"

// Volume One
// "Duke is brain dead",
// "BOOT TO THE HEAD"
// Damage too duke
// Weapons are computer cont.  Only logical thinking
// is disappearing.
// " Flips! "
// Flash on screen, inst.
// "BUMS"
// "JAIL"/"MENTAL WARD (Cop code for looney?  T. asks Cop.)"
// "GUTS OR GLORY"

// ( Duke's Mission

// Duke:    "Looks like some kind of transporter...?"
// Byte:    "...YES"

// Duke:    "Waa, here goes nuthin'. "
// (Duke puts r. arm in device)

// Duke:    AAAAHHHHHHHHHHHHHHHHHHHHHHHHH!!!
// (Duke's arm is seved.)
// Byte:    NO.NO.NO.NO.NO.NO.NO...
// ( Byte directs duke to the nearest heat source)
// (Shut Up Mode)
// ( Duke Staggers, end of arm bleeding, usual oozing arm guts. )
// Byte: Left, Left, Left, Left, Right.
// ( Duke, loozing consc, trips on broken pipe, )
// ( hits temple on edge of step. )
// ( Rats everywhere, byte pushing them away with weapon,
// ( eventually covered, show usual groosums, Duke appears dead
// ( Duke wakes up, in hospital, vision less blurry
// ( Hospital doing brain scan, 1/3 cran. mass MISSING!
// Doc: Hummm?  ( Grabbing upper lip to "appear" smart. )

// Stand back boys

// Schrapnel has busted my scull!
// Now I'm insane, Mental ward, got to escape.
// Search light everywhere.

// (M)Mendor, The Tree Dweller.
// (M)BashMan, The Destructor.
// (M)Lash, The Scavenger.
// (F)Mag, The Slut.
// (F)
// NRA OR SOMETHIN'

// Duke Nukem
// 5th Dimention
// Pentagon Man!


// I Hope your not stupid!
// The 70's meet the future.
// Dirty Harry style.  70's music with futuristic edge
// The Instant De-Welder(tm)
// I think I'm going to puke...
// Badge attitude.
// He's got a Badge(LA 3322), a Bulldog, a Bronco (beat up/bondoed).
// Gfx:
// Lite rail systems
// A church.  Large cross
// Sniper Scope,
// Really use the phone
// The Boiler Room
// The IRS, nuking other government buildings?
// You wouldn't have a belt of booz, would ya?
// Slow turning signes
// More persise shooting/descructions
// Faces, use phoneoms and its lookup.  Talking, getting in fights.
// Drug dealers, pimps, and all galore
// Weapons, Anything lying around.
// Trees to clime, burning trees.
// Sledge Hammer, Sledge hammer with Spike
// sancurary, get away from it all.
// Goodlife = ( War + Greed ) / Peace
// Monsterism           (ACTION)
// Global Hunter        (RPG)
// Slick a Wick         (PUZZLE)
// Roach Condo          (FUNNY)
// AntiProfit           (RPG)
// Pen Patrol           (TD SIM)
// 97.5 KPIG! - Wanker County
// "Fauna" - Native Indiginouns Animal Life


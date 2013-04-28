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

#ifndef __funct_h__
#define __funct_h__

struct player_struct;	// JBF: duke3d.h defines this later

extern void sendscore(char *s);
extern void SoundStartup(void );
extern void SoundShutdown(void );
extern void MusicStartup(void );
extern void MusicShutdown(void );
extern void MusicSetVolume(int);
extern int USRHOOKS_GetMem(char **ptr,unsigned int size);
extern int USRHOOKS_FreeMem(char *ptr);
extern void intomenusounds(void );
extern void playmusic(char *fn);
extern void stopmusic(void);
extern char loadsound(unsigned short num);
extern int xyzsound(short num,short i,int x,int y,int z);
extern void sound(short num);
extern int spritesound(unsigned short num,short i);
extern void stopsound(short num);
extern void stopenvsound(short num,short i);
extern void pan3dsound(void );
extern void testcallback(unsigned int num);
extern void clearsoundlocks(void);
extern short callsound(short sn,short whatsprite);
extern short check_activator_motion(short lotag);
extern char isadoorwall(short dapic);
extern char isanunderoperator(short lotag);
extern char isanearoperator(short lotag);
extern short checkcursectnums(short sect);
extern int ldist(spritetype *s1,spritetype *s2);
extern int dist(spritetype *s1,spritetype *s2);
extern short findplayer(spritetype *s,int *d);
extern short findotherplayer(short p,int *d);
extern void doanimations(void );
extern int getanimationgoal(int *animptr);
extern int setanimation(short animsect,int *animptr,int thegoal,int thevel);
extern void animatecamsprite(void );
extern void animatewalls(void );
extern char activatewarpelevators(short s,short d);
extern void operatesectors(short sn,short ii);
extern void operaterespawns(short low);
extern void operateactivators(short low,short snum);
extern void operatemasterswitches(short low);
extern void operateforcefields(short s,short low);
extern char checkhitswitch(short snum,int w,char switchtype);
extern void activatebysector(short sect,short j);
extern void checkhitwall(short spr,short dawallnum,int x,int y,int z,short atwith);
extern void checkplayerhurt(struct player_struct *p,short j);
extern char checkhitceiling(short sn);
extern void checkhitsprite(short i,short sn);
extern void allignwarpelevators(void );
extern void cheatkeys(short snum);
extern void checksectors(short snum);
extern int32 RTS_AddFile(char *filename);
extern void RTS_Init(char *filename);
extern int32 RTS_NumSounds(void );
extern int32 RTS_SoundLength(int32 lump);
extern char *RTS_GetSoundName(int32 i);
extern void RTS_ReadLump(int32 lump,void *dest);
extern void *RTS_GetSound(int32 lump);
extern void docacheit(void);
extern void xyzmirror(short i,short wn);
extern void vscrn(void );
extern void pickrandomspot(short snum);
extern void resetplayerstats(short snum);
extern void resetweapons(short snum);
extern void resetinventory(short snum);
extern void resetprestat(short snum,unsigned char g);
extern void setupbackdrop(short backpicnum);
extern void cachespritenum(short i);
extern void cachegoodsprites(void );
extern void prelevel(unsigned char g);
extern void newgame(char vn,char ln,char sk);
extern void resetpspritevars(unsigned char g);
extern void resettimevars(void );
extern void genspriteremaps(void );
extern void waitforeverybody(void);
extern char getsound(unsigned short num);
extern void precachenecessarysounds(void );
extern void cacheit(void );
extern void dofrontscreens(const char *);
extern void clearfifo(void);
extern void resetmys(void);
extern int  enterlevel(unsigned char g);
extern void backtomenu(void);
extern void setpal(struct player_struct *p);
extern void incur_damage(struct player_struct *p);
extern void quickkill(struct player_struct *p);
extern void forceplayerangle(struct player_struct *p);
extern void tracers(int x1,int y1,int z1,int x2,int y2,int z2,int n);
extern int hits(short i);
extern int hitasprite(short i,short *hitsp);
extern int hitawall(struct player_struct *p,short *hitw);
extern short aim(spritetype *s,short aang);
extern void shoot(short i,short atwith);
extern void displayloogie(short snum);
extern char animatefist(short gs,short snum);
extern char animateknee(short gs,short snum);
extern char animateknuckles(short gs,short snum);
extern void displaymasks(short snum);
extern char animatetip(short gs,short snum);
extern char animateaccess(short gs,short snum);
extern void displayweapon(short snum);
extern void getinput(short snum);
extern char doincrements(struct player_struct *p);
extern void checkweapons(struct player_struct *p);
extern void processinput(short snum);
extern void cmenu(short cm);
extern void getangplayers(short snum);
//extern int loadpheader(char spot,int32 *vn,int32 *ln,int32 *psk,char *bfn,int32 *numplr);
extern int loadplayer(signed char spot);
extern int saveplayer(signed char spot);
extern void sendgameinfo(void );
extern int probe(int x,int y,int i,int n);
extern int menutext(int x,int y,short s,short p,const char *t);
extern int menutextc(int x,int y,short s,short p,const char *t);
extern void bar(int x,int y,short *p,short dainc,char damodify,short s,short pa);
extern void barsm(int x,int y,short *p,short dainc,char damodify,short s,short pa);
extern void dispnames(void );
extern int getfilenames(char *path, char kind[]);
extern void sortfilenames(void);
extern void menus(void );
extern void palto(unsigned char r,unsigned char g,unsigned char b,int e);
extern void drawoverheadmap(int cposx,int cposy,int czoom,short cang);
extern void playanm(const char *fn,char);
extern short getincangle(short a,short na);
extern char ispecial(char c);
extern char isaltok(char c);
extern void getglobalz(short i);
extern void makeitfall(short i);
extern void getlabel(void );
extern int keyword(void );
extern int transword(void );
extern int transnum(int type);
extern char parsecommand(void );
extern void passone(void );
extern void loadefs(const char *fn);
extern char dodge(spritetype *s);
extern short furthestangle(short i,short angs);
extern short furthestcanseepoint(short i,spritetype *ts,int *dax,int *day);
extern void alterang(short a);
extern void move(void);
extern void parseifelse(int condition);
extern char parse(void );
extern void execute(short i,short p,int x);
extern void overwritesprite(int thex,int they,short tilenum,signed char shade,unsigned char stat,unsigned char dapalnum);
extern void timerhandler(void);
extern int gametext(int x,int y,const char *t,char s,short dabits);
extern int gametextpal(int x,int y,const char *t,char s,unsigned char p);
extern int gametextpart(int x,int y,const char *t,char s,short p);
extern int minitext(int x,int y,const char *t,unsigned char p,short sb);
extern int minitextshade(int x,int y,const char *t,char s,unsigned char p,short sb);
extern void gamenumber(int x,int y,int n,char s);
extern void Shutdown(void );
extern void allowtimetocorrecterrorswhenquitting(void );
extern void getpackets(void );
extern void faketimerhandler(void);
extern void checksync(void );
extern void check_fta_sounds(short i);
extern short inventory(spritetype *s);
extern short badguy(spritetype *s);
extern short badguypic(short pn);
extern void myos(int x,int y,short tilenum,signed char shade,char orientation);
extern void myospal(int x,int y,short tilenum,signed char shade,char orientation,char p);
extern void invennum(int x,int y,unsigned char num1,char ha,unsigned char sbits);
extern void weaponnum(short ind,int x,int y,int num1,int num2,char ha);
extern void weaponnum999(char ind,int x,int y,int num1,int num2,char ha);
extern void weapon_amounts(struct player_struct *p,int x,int y,int u);
extern void digitalnumber(int x,int y,int n,char s,unsigned char cs);
extern void scratchmarks(int x,int y,int n,char s,unsigned char p);
extern void displayinventory(struct player_struct *p);
extern void displayfragbar(void );
extern void coolgaugetext(short snum);
extern void tics(void );
extern void clocks(void );
extern void coords(short snum);
extern void operatefta(void);
extern void FTA(short q,struct player_struct *p);
extern void showtwoscreens(void );
extern void binscreen(void );
extern void gameexit(const char *t);
extern short strget(short x,short y,char *t,short dalen,short c);
extern void displayrest(int smoothratio);
extern void updatesectorz(int x,int y,int z,short *sectnum);
extern void view(struct player_struct *pp,int *vx,int *vy,int *vz,short *vsectnum,short ang,short horiz);
extern void drawbackground(void );
extern void displayrooms(short snum,int smoothratio);
extern short LocateTheLocator(short n,short sn);
extern short EGS(short whatsect,int s_x,int s_y,int s_z,short s_pn,signed char s_s,signed char s_xr,signed char s_yr,short s_a,short s_ve,int s_zv,short s_ow,signed char s_ss);
extern char wallswitchcheck(short i);
extern short spawn(short j,short pn);
extern void animatesprites(int x,int y,short a,int smoothratio);
extern void cheats(void );
extern void nonsharedkeys(void );
extern void comlinehelp(void);
extern void checkcommandline(int argc,char const * const *argv);
extern void Logo(void );
extern void loadtmb(void );
extern void compilecons(void );
extern int encodescriptptr(int *scptr);
extern int *decodescriptptr(int scptr);
extern void Startup(void );
extern void getnames(void );
extern char opendemoread(char which_demo);
extern void opendemowrite(void );
extern void record(void );
extern void closedemowrite(void );
extern int playback(void );
extern char moveloop(void);
extern void fakedomovethingscorrect(void);
extern void fakedomovethings(void );
extern char domovethings(void );
extern void displaybonuspics(short x,short y,short p);
extern void doorders(void );
extern void dobonus(char bonusonly);
extern void cameratext(short i);
extern void vglass(int x,int y,short a,short wn,short n);
extern void lotsofglass(short i,short wallnum,short n);
extern void spriteglass(short i,short n);
extern void ceilingglass(short i,short sectnum,short n);
extern void lotsofcolourglass(short i,short wallnum,short n);
extern void SetupGameButtons(void );
extern int GetTime(void );
extern void CenterCenter(void );
extern void UpperLeft(void );
extern void LowerRight(void );
extern void CenterThrottle(void );
extern void CenterRudder(void );
extern void CONFIG_GetSetupFilename(void );
extern int32 CONFIG_FunctionNameToNum(const char *func);
extern const char *CONFIG_FunctionNumToName(int32 func);
extern int32 CONFIG_AnalogNameToNum(const char *func);
extern const char *CONFIG_AnalogNumToName(int32 func);
extern void CONFIG_SetDefaults(void );
extern void CONFIG_ReadKeys(void );
extern void readsavenames(void );
extern int32 CONFIG_ReadSetup(void );
extern void CONFIG_WriteSetup(void );
extern void CheckAnimStarted(char *funcname);
extern uint16 findpage(uint16 framenumber);
extern void loadpage(uint16 pagenumber,uint16 *pagepointer);
extern void CPlayRunSkipDump(unsigned char *srcP,unsigned char *dstP);
extern void renderframe(uint16 framenumber,uint16 *pagepointer);
extern void drawframe(uint16 framenumber);
extern void updateinterpolations(void);
extern void setinterpolation(int *posptr);
extern void stopinterpolation(int *posptr);
extern void dointerpolations(int smoothratio);
extern void restoreinterpolations(void);
extern int ceilingspace(short sectnum);
extern int floorspace(short sectnum);
extern void addammo(short weapon,struct player_struct *p,short amount);
extern void addweaponnoswitch(struct player_struct *p,short weapon);
extern void addweapon(struct player_struct *p,short weapon);
extern void checkavailinven(struct player_struct *p);
extern void checkavailweapon(struct player_struct *p);
extern int ifsquished(short i,short p);
extern void hitradius(short i,int r,int hp1,int hp2,int hp3,int hp4);
extern int movesprite(short spritenum,int xchange,int ychange,int zchange,unsigned int cliptype);
extern short ssp(short i,unsigned int cliptype);
extern void insertspriteq(short i);
extern void lotsofmoney(spritetype *s,short n);
extern void lotsofmail(spritetype *s, short n);
extern void lotsofpaper(spritetype *s, short n);
extern void guts(spritetype *s,short gtype,short n,short p);
extern void setsectinterpolate(short i);
extern void clearsectinterpolate(short i);
extern void ms(short i);
extern void movefta(void );
extern short ifhitsectors(short sectnum);
extern short ifhitbyweapon(short sn);
extern void movecyclers(void );
extern void movedummyplayers(void );
extern void moveplayers(void );
extern void movefx(void );
extern void movefallers(void );
extern void movestandables(void );
extern void bounce(short i);
extern void moveweapons(void );
extern void movetransports(void );
extern void moveeffectors(void );
extern void moveactors(void );
extern void moveexplosions(void );

// game.c
extern void setstatusbarscale(int sc);

extern void setgamepalette(struct player_struct *player, unsigned char *pal, int set);
extern void fadepal(int r, int g, int b, int start, int end, int step);

extern int isspritemakingsound(short i, int num);
extern int issoundplaying(int num, int xyz);
extern void stopspritesound(short num, short i);

#endif // __funct_h__

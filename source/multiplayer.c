/*
 * Extensions to Duke for new multiplayer abilities
 * by Jonathon Fowler
 * jonof@edgenetwork.org
 */

#include "duke3d.h"


int showmultidiags = 0;
int netmode = -1, nethostplayers=0;
char netjoinhost[64]="";
multiaddr netjoinhostaddr;
static int netuniqueidtoken = 0, netgameoperationmode = 0;


int processnetworkrequests(void)
{
	long i,j,k,l;
	multiaddr addr,addr2;

	if (netmode < 0) return -1;

	if (netmode == 0) {
		i = 255;
		if (fetchoobmessage(&addr, packbuf, &i)) {
			if (packbuf[0] == MSG_CMD_GETGAMEINFO) {
				debugprintf("got MSG_CMD_GETGAMEINFO");
				if (packbuf[1] != MSGPROTOVER) {
					packbuf[0] = MSG_RSP_BADPROTO;
					debugprintf("responding MSG_RSP_BADPROTO");
					i = 1;
				} else {
					i = 0;
					packbuf[i++] = MSG_RSP_GAMEINFO;
					if (VOLUMEONE) { Bmemcpy(&packbuf[i], "Duke3DSW", 8); }
					else if (PLUTOPAK) { Bmemcpy(&packbuf[i], "Duke3D15", 8); }
					else { Bmemcpy(&packbuf[i], "Duke3D13", 8); }
					i+=8;
					packbuf[i++] = nethostplayers;
					debugprintf("responding MSG_RSP_GAMEINFO");
				}
				sendoobpacketto(&addr, packbuf, i);
			} else if (packbuf[0] == MSG_CMD_JOINGAME) {
				debugprintf("got MSG_CMD_JOINGAME");
				// see if the player is already in the game
				j = -1;
				for (k = connecthead; k != -1; k = connectpoint2[k]) {
					multigetpeeraddr(k, &addr2, &i);
					if (!multicompareaddr(&addr,&addr2))
						j = i;
				}
				i = 0;
				if (j < 0) {	// not in the game
					if (netgameoperationmode == 0) {
						if (numplayers == nethostplayers) {
							packbuf[0] = MSG_RSP_GAMEFULL;
							i = 1;
							debugprintf("responding MSG_RSP_GAMEFULL");
						} else {
							i = 0;
							j = netuniqueidtoken++;

							// broadcast the joining of the new player to the rest of the team
							packbuf[0] = 6;	// some game-level message specifically used at this point for joiners
							packbuf[1] = j&255;
							packbuf[2] = (j>>8)&255;
							sendpacket(-1,packbuf,3);

							// now add the new player to our game
							multiaddrtostring(&addr, packbuf, 255);
							multiaddplayer(packbuf, j);
							initprintf("A player just joined.\n");
						}
					} else {
						packbuf[0] = MSG_RSP_GAMEINPROG;
						i = 1;
						debugprintf("responding MSG_RSP_GAMEINPROG");
					}
				}

				if (i==0) {	// game was not full, so construct a packet to send to the joiner
					packbuf[i++] = MSG_RSP_JOINACCEPTED;
					packbuf[i++] = j & 255;
					packbuf[i++] = (j>>8)&255;
					packbuf[i++] = numplayers-2;
					for (k=connecthead; k!=-1; k=connectpoint2[k]) {
						multigetpeeraddr(k,0,&l);
						if (l==myuniqueid || l==j) continue;
						packbuf[i++] = l & 255;
						packbuf[i++] = (l>>8)&255;
					}
					debugprintf("responding MSG_RSP_JOINACCEPTED");
				}

				sendoobpacketto(&addr, packbuf, i);
			}
		}
	} else {
		if (netgameoperationmode == 1) return 0;

		if ((i=getpacket(&j,packbuf))>0) {
			if (packbuf[0] == 249) {	// some game-level message specifically used at this point for joiners
				j = packbuf[1];
				j |= (short)packbuf[2] << 8;
				multiaddplayer(0, j);
				initprintf("A player just joined.\n");
			}
		}
					
		i = 255;
		if (fetchoobmessage(&addr, packbuf, &i)) {
			if (packbuf[0] == MSG_RSP_BADPROTO) {
				initprintf("Host is using a different protocol version.\n");
				return -1;
			} else if (packbuf[0] == MSG_RSP_NOGAME) {
				initprintf("Host is not running a game.\n");
				return -1;
			} else if (packbuf[0] == MSG_RSP_GAMEINFO) {
				i=1;
				if (Bmemcmp(&packbuf[i], "Duke3D", 6)) {
					initprintf("Host is not running Duke Nukem 3D.\n");
					return -1;
				}
				if (VOLUMEONE && Bmemcmp(&packbuf[i], "Duke3DSW", 8)) {
					initprintf("Host is not running Duke Nukem 3D Shareware.\n");
					return -1;
				} else if (PLUTOPAK && Bmemcmp(&packbuf[i], "Duke3D15", 8)) {
					initprintf("Host is not running Duke Nukem 3D Atomic Edition.\n");
					return -1;
				} else if ((!VOLUMEONE && !PLUTOPAK) && Bmemcmp(&packbuf[i], "Duke3D13", 8)) {
					initprintf("Host is not running Duke Nukem 3D Standard Edition.\n");
					return -1;
				}
				i+=8;
							
				nethostplayers = packbuf[i++];
				netmode = 2;

				initprintf("Game is a %d player game.\n", nethostplayers);
						
				ototalclock = 0; totalclock = TICRATE;
			} else if (packbuf[0] == MSG_RSP_GAMEINPROG) {
				initprintf("Game is in progress.\n");
				return -1;
			} else if (packbuf[0] == MSG_RSP_GAMEFULL) {
				initprintf("Game is full.\n");
				return -1;
			} else if (packbuf[0] == MSG_RSP_JOINACCEPTED) {
				initprintf("Join accepted.\n");
				multiaddplayer(netjoinhost,0);
				i=1;
				myuniqueid = packbuf[i++];
				myuniqueid |= (short)packbuf[i++] << 8;
				multiaddplayer("localhost",myuniqueid);
				j = packbuf[i++];
				for (; j>0; j++) {
					k = packbuf[i++];
					k |= (short)packbuf[i++];
					multiaddplayer(0,k);
					initprintf("A player is already waiting.\n");
				}
				netmode = 3;
			}
		}
		if (netmode == 1) {
			// waiting for game info
			if (totalclock - ototalclock < TICRATE) return 0;
			ototalclock = totalclock;

			packbuf[0] = MSG_CMD_GETGAMEINFO;
			packbuf[1] = MSGPROTOVER;
			sendoobpacketto(&netjoinhostaddr, packbuf, 2);
		} else if (netmode == 2) {
			if (totalclock - ototalclock < TICRATE) return 0;
			ototalclock = totalclock;

			packbuf[0] = MSG_CMD_JOINGAME;
			sendoobpacketto(&netjoinhostaddr, packbuf, 1);
		}
	}

	return 0;
}

int startupnetworkgame(void)
{
	netgameoperationmode = 0;

	if (netmode == 0) {
		myuniqueid = netuniqueidtoken++;
		multiaddplayer("localhost",myuniqueid);
		initprintf(" - Waiting for %d players to join...\n",nethostplayers-1);

		do {
			if (handleevents()) {
				if (quitevent) {
					keystatus[1] = 1;
					quitevent = 0;
				}
			}

			if (keystatus[1]) {
				keystatus[1] = 0;
				initprintf("Aborting multiplayer game.\n");
				return -1;
			}

			if (processnetworkrequests() < 0) return -1;
		} while (numplayers<nethostplayers);

		netgameoperationmode = 1;
		return 0;

	} else if (netmode == 1) {
		ototalclock = 0; totalclock = TICRATE;
		while (netmode != 4) {
			if (handleevents()) {
				if (quitevent) {
					keystatus[1] = 1;
					quitevent = 0;
				}
			}

			if (keystatus[1]) {
				keystatus[1] = 0;
				initprintf("Aborting multiplayer game.\n");
				return -1;
			}

			if (netmode == 3 && numplayers>=nethostplayers) { netmode = 4; break; }

			if (processnetworkrequests() < 0) return -1;
		}

		netgameoperationmode = 1;
		return 0;
	}
	
	return -1;
}


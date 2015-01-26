/* @(#)$Id: globalvar.h,v 1.12 2000/10/24 15:15:53 seks Exp $ */

/* Undernet Channel Service (X)
 * Copyright (C) 1995-2002 Robin Thellend
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * The author can be contact by email at <csfeedback@robin.pfft.net>
 *
 * Please note that this software is unsupported and mostly
 * obsolete. It was replaced by GNUworld/CMaster. See
 * http://gnuworld.sourceforge.net/ for more information.
 */

#if !defined(__FreeBSD__) && !defined(__linux__)

extern int errno;
extern char *sys_errlist[];
#endif
extern char mynick[NICK_LENGTH];
extern char myuser[USERNAME_LENGTH];
extern char mysite[SITE_LENGTH];
extern char myrealname[REALNAME_LENGTH];
extern char server[SERVER_NAME_LENGTH];
extern int logfile;
extern RegUser *UserList[1000];
extern ShitUser *ShitList[1000];
extern aluser *Lusers[1000];
extern achannel *ChannelList[1000];
extern adefchan *DefChanList;
extern anevent *EventList;
extern aserver *ServerList;
extern aserver VirtualServer;
extern dbquery *DBQuery;
extern dbsync *DBSync;
extern syncchan *SyncChan;
#ifdef DOHTTP
extern http_socket *HttpList;
extern http_file_pipe *FilePipes;
#endif
extern misc_socket *MiscList;
extern irc_socket Irc;
extern char *TmpPtr;
extern time_t now;
extern time_t logTS;
extern time_t TSoffset;
extern time_t TSonline;
extern time_t TSconnect;
extern unsigned long long TTLREADBYTES;
extern unsigned long long TTLSENTBYTES;
extern unsigned long TTLALLOCMEM;
extern unsigned long long HTTPTTLSENTBYTES;
extern alang Lang[NO_LANG];
extern char *replies[][NO_LANG];
#ifdef FAKE_UWORLD
extern int Uworld_status;
extern time_t UworldTS,UworldServTS;
#endif
#ifdef NICKSERV
extern int NServ_status;
#endif
extern unsigned long MEM_buffers;
extern unsigned long NB_avail_buffer_blocks;
extern unsigned long NB_alloc_buffer_blocks;
extern long CurrentSendQ;

extern int DB_Save_Status;
extern char DB_Save_Nick[NICK_LENGTH];

#define MALLOC(X)  ((TmpPtr=(char *)malloc(X))? \
	TmpPtr : (char *)quit("ERROR: malloc() failed", 1));\
	TTLALLOCMEM+=X

#define close(X)   if((X)>=0) close(X)
#define ABS(X)  ((X>0)?(X):(-(X)))

/* I use '}' and ']' instead of 'z' and 'Z'
 * in order to respect RFC-1459 (section 2.2)
 */ 
#undef toupper
#undef tolower
#if 0
#define toupper(X) ( ((X)>='a'&&(X)<='~') ? ((X)&223) : (X) )
#define tolower(X) ( ((X)>='A'&&(X)<='^') ? ((X)|32) : (X) )
#endif
#define toupper(X) ( ((X)>='a'&&(X)<='~') || ((unsigned char)(X)>=0xe0&&(unsigned char)(X)<=0xff&&(unsigned char)(X)!=0xf7)? ((X)&223) : (X) )
#define tolower(X) ( ((X)>='A'&&(X)<='^') || ((unsigned char)(X)>=0xc0&&(unsigned char)(X)<=0xdf&&(unsigned char)(X)!=0xd7)? ((X)|32) : (X) )

#define touppertmp(X) ( ((X)>='a'&&(X)<='~') || ((unsigned char)(X)>=0xe0&&(unsigned char)(X)<=0xff&&(unsigned char)(X)!=0xf7)? ((X)&223) : (X) )
#define tolowertmp(X) ( ((X)>='A'&&(X)<='^') || ((unsigned char)(X)>=0xc0&&(unsigned char)(X)<=0xdf&&(unsigned char)(X)!=0xd7)? ((X)|32) : (X) )

#define strcasecmp mycasecmp
#define strcmp mycasecmp

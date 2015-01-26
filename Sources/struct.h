/* @(#)$Id: struct.h,v 1.6 1997/07/01 21:51:09 cvs Exp $ */

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

#ifdef NICKSERV
#include "nickserv.h"
#endif

typedef struct filehdr {
        unsigned char magic;
        unsigned int no;
} filehdr;

typedef struct RegUser {
	char *realname;
	char *match;
	char *channel;
	char *passwd;
	char *modif;
	int access;
	unsigned long flags;
	time_t suspend;
	time_t lastseen;
	off_t offset;
	int modified;
	int inuse;
	time_t lastused;
	struct RegUser *next;
} RegUser;


typedef struct dbuser {
	unsigned char header[2];
	char nick[80];
	char match[80];
	char passwd[20];
	char channel[50];
	char modif[80];
	int access;
	unsigned long flags;
	time_t suspend;
	time_t lastseen;
	unsigned char footer[2];
} dbuser;


typedef struct achannel {
	char *name;
	int AmChanOp;
	int on;
	int MassDeopPro;
	int NickFloodPro;
	int MsgFloodPro;
	int lang;
	time_t TS;
	time_t lastact;
	time_t lasttopic;
	unsigned long flags;
	unsigned long uflags;
	char mode[80];
	char lastjoin[20];
	struct modequeue *modebuff;
	struct aban *bans;
	struct auser *users;
	struct achannel *next;
} achannel;

typedef struct adefchan {
	char name[50];
	char mode[80];
	char url[80];
	char topic[80];
	int MassDeopPro;
	int NickFloodPro;
	int MsgFloodPro;
	int lang;
	time_t TS;
	unsigned long flags;
	unsigned long uflags;
	struct adefchan *next;
} adefchan;

typedef struct achannelnode {
	struct achannel *N;
	struct anickchange *nickhist;
	struct achannelnode *next;
} achannelnode;

typedef struct aserver {
	char *name;
	time_t TS;
	struct asuser *users[100];
	struct aserver *up;
	struct aserver *down;
	struct aserver *next;
} aserver;

typedef struct asuser {
	struct aluser *N;
	struct asuser *next;
} asuser;

typedef struct aluser {
	char *nick;
	char *username;
	char *site;
	time_t time;
	char mode;
	struct aserver *server;
	struct achannelnode *channel;
	struct avalchan *valchan;
	struct aluser *next;
#ifdef NICKSERV
	struct aregnick *regnick;
	time_t reglimit;
#endif
} aluser;

typedef struct avalchan {
	char *name;
	RegUser *reg;
	struct avalchan *next;
} avalchan;

typedef struct auser {
        struct aluser *N;
        char chanop;
        time_t lastact;
        struct amsg *msghist;
        struct adeop *deophist;
        struct auser *next;
} auser;

typedef struct aban {
	char pattern[80];
	struct aban *next;
} aban;

typedef struct adeop {
	time_t time;
	char nick[NICK_LENGTH];
	struct adeop *next;
} adeop;

typedef struct anickchange {
	time_t time;
	char nick[NICK_LENGTH];
	struct anickchange *next;
} anickchange;

typedef struct modequeue {
	int AsServer;
	char flag[3];
	char arg[80];
	struct modequeue *prev;
	struct modequeue *next;
} modequeue;

typedef struct amsg {
	time_t time;
	int length;
	struct amsg *next;
} amsg;

typedef struct ShitUser {
	time_t time;
	time_t expiration;
	char *match;
	char *from;
	char *reason;
	char *channel;
	int level;
	struct ShitUser *next;
} ShitUser;

typedef struct anevent {
	time_t time;
	int event;
	char param[80];
	struct anevent *next;
} anevent;

typedef struct alang {
	int no;
	char *abbr;
	char *name;
} alang;

struct buffer_block {
  char buf[BUFFER_BLOCK_SIZE];
  short offset_read;
  short offset_write;
  struct buffer_block *next;
};

typedef struct irc_socket {
  int fd;
  time_t TS;
  struct buffer_block *inbuf;
  struct buffer_block *outbuf;
  struct irc_socket *next;
} irc_socket;

typedef struct http_raw {
  unsigned long key;
} http_raw;

typedef struct http_post {
  char path[80];
  char protocol[80];
  int count;
  int ready;
} http_post;

typedef struct http_socket {
  int fd;
  int status;
  int dbio;
  time_t TS;
  time_t since;
  void *hook;
  struct buffer_block *inbuf;
  struct buffer_block *outbuf;
  struct sockaddr_in peer;
  struct http_socket *next;
} http_socket;

typedef struct http_file_pipe {
  int fd;
  http_socket *hsock;
  struct http_file_pipe *next;
} http_file_pipe;

typedef struct misc_socket {
  int fd;
  int type;
  int status;
  time_t TS;
  char link[80];
  struct buffer_block *inbuf;
  struct buffer_block *outbuf;
  struct misc_socket *next;
} misc_socket;


#define DBCALLBACK(X) void (*X)(int *,off_t,int,void *,void *,dbuser*,int)

typedef struct dbquery {
  int fd;
  off_t offset;
  time_t time;
  unsigned int type;
  int action;
  int count;
  DBCALLBACK(callback);
  struct buffer_block *buf;
  void *hook1;
  void *hook2;
  struct dbquery *next;
  char channel[80];
  char info[80];
  char passwd[80];
} dbquery;


typedef struct dbsync {
  int fd;
  off_t offset;
  time_t time;
  int type;   /* update, delete, add */
  int status; /* pending_write, seeking_empty_slot */
  RegUser **reg;
  struct buffer_block *buf;
  struct dbsync *next;
} dbsync;

typedef struct syncchan {
  char name[50];
  struct syncchan *next;
} syncchan;


/* @(#)$Id: change_byte_order.c,v 1.3 1996/11/13 00:40:34 seks Exp $ */

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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <netinet/in.h>
#include "defines.h"
#include "struct.h"
#include "version.h"
#include "../config.h"

typedef struct DiskUser {
        char realname[80];
        char match[80];
        int Access;
        char passwd[20];
        char channel[80];
        unsigned long flags;
        char reserved[20];
        time_t suspend;
        time_t lastseen;
} DiskUser;

typedef struct ShitDisk {
        time_t time;
        time_t expiration;
        char match[80];
        char from[80];
        char reason[200];
        char channel[50];
        int level;
} ShitDisk;


void main(void)
{
	FILE *in;
	FILE *out;
	DiskUser olduser;
	DiskUser user;
	adefchan oldchan;
	adefchan chan;
	ShitDisk oldshit,shit;
	time_t now;
	char swap, *ptr;


	if(chdir(HOMEDIR)<0){
		perror(HOMEDIR);
		exit(1);
	}

	now=time(NULL);

	out=fopen("userlist.dat.new","w");
	if(out==NULL){
		perror("userlist.dat.new");
		exit(1);
	}

	in=fopen(USERFILE,"r");
	if(in==NULL){
		perror(USERFILE);
		return;
	}
	while(fread(&user,sizeof(DiskUser),1,in)>0){
		ptr=(char *)&user.Access;
		swap=ptr[0]; ptr[0]=ptr[3]; ptr[3]=swap;
		swap=ptr[1]; ptr[1]=ptr[2]; ptr[2]=swap;

		ptr=(char *)&user.flags;
                swap=ptr[0]; ptr[0]=ptr[3]; ptr[3]=swap;
                swap=ptr[1]; ptr[1]=ptr[2]; ptr[2]=swap;

		ptr=(char *)&user.suspend;
                swap=ptr[0]; ptr[0]=ptr[3]; ptr[3]=swap;
                swap=ptr[1]; ptr[1]=ptr[2]; ptr[2]=swap;

		ptr=(char *)&user.lastseen;
                swap=ptr[0]; ptr[0]=ptr[3]; ptr[3]=swap;
                swap=ptr[1]; ptr[1]=ptr[2]; ptr[2]=swap;

		fwrite(&user,sizeof(DiskUser),1,out);
	}

	fclose(in);
	fclose(out);
        out=fopen("channellist.dat.new","w");
        if(out==NULL){
                perror("channellist.dat.new");
                exit(1);
        }

	in=fopen(DEFAULT_CHANNELS_FILE,"r");
	if(in==NULL){
		perror(DEFAULT_CHANNELS_FILE);
		exit(1);
	}

	while(fread(&chan,sizeof(adefchan),1,in)>0){
		ptr=(char *)&chan.MassDeopPro;
		swap=ptr[0]; ptr[0]=ptr[3]; ptr[3]=swap;
		swap=ptr[1]; ptr[1]=ptr[2]; ptr[2]=swap;

		ptr=(char *)&chan.NickFloodPro;
                swap=ptr[0]; ptr[0]=ptr[3]; ptr[3]=swap;
                swap=ptr[1]; ptr[1]=ptr[2]; ptr[2]=swap;

		ptr=(char *)&chan.MsgFloodPro;
                swap=ptr[0]; ptr[0]=ptr[3]; ptr[3]=swap;
                swap=ptr[1]; ptr[1]=ptr[2]; ptr[2]=swap;

		ptr=(char *)&chan.TS;
                swap=ptr[0]; ptr[0]=ptr[3]; ptr[3]=swap;
                swap=ptr[1]; ptr[1]=ptr[2]; ptr[2]=swap;

		ptr=(char *)&chan.flags;
                swap=ptr[0]; ptr[0]=ptr[3]; ptr[3]=swap;
                swap=ptr[1]; ptr[1]=ptr[2]; ptr[2]=swap;

		ptr=(char *)&chan.uflags;
                swap=ptr[0]; ptr[0]=ptr[3]; ptr[3]=swap;
                swap=ptr[1]; ptr[1]=ptr[2]; ptr[2]=swap;

		fwrite(&chan,sizeof(adefchan),1,out);
	}

	fclose(in);
	fclose(out);

        out=fopen("shitlist.dat.new","w");
        if(out==NULL){
                perror("shitlist.dat.new");
                exit(1);
        }

        in=fopen(SHITLIST_FILE,"r");
        if(in==NULL){
                perror(SHITLIST_FILE);
                exit(1);
        }

	while(fread(&shit,sizeof(ShitDisk),1,in)>0){
		ptr=(char *)&shit.time;
                swap=ptr[0]; ptr[0]=ptr[3]; ptr[3]=swap;
                swap=ptr[1]; ptr[1]=ptr[2]; ptr[2]=swap;

		ptr=(char *)&shit.expiration;
                swap=ptr[0]; ptr[0]=ptr[3]; ptr[3]=swap;
                swap=ptr[1]; ptr[1]=ptr[2]; ptr[2]=swap;

		ptr=(char *)&shit.level;
                swap=ptr[0]; ptr[0]=ptr[3]; ptr[3]=swap;
                swap=ptr[1]; ptr[1]=ptr[2]; ptr[2]=swap;

		fwrite(&shit,sizeof(ShitDisk),1,out);
	}

	fclose(in);
	fclose(out);
}

/* @(#)$Id: list.c,v 1.3 1996/11/13 00:40:43 seks Exp $ */

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
#include <string.h>
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

void main(void)
{
	FILE *in1,*in2;
	FILE *out;
	char *ptr;
	adefchan channel;
	DiskUser user;
	time_t currtime;


	if(chdir(HOMEDIR)<0){
		perror(HOMEDIR);
		exit(1);
	}

	out=fopen("Registered_Channels","w");
	if(out==NULL){
		fclose(in1);
		perror("Registered_Channels");
		exit(1);
	}

	currtime=time(NULL);

	fprintf(out,
	  "%s\n"
	  "(c) 1995,1996 by Robin Thellend <intru@info.polymtl.ca>\n"
	  "\n"
	  "This is the list of currently registered channels on Undernet.\n"
	  "Note that secret and private channels are NOT showed here.\n"
	  "This file is updated every hour.\n"
	  "\n"
	  "Last update: %s"
	  "Montreal's local time is used.\n\n",
	  VERSION,ctime(&currtime));

	in1=fopen(DEFAULT_CHANNELS_FILE,"r");
	if(in1==NULL){
		fprintf(out,"There are no registered channels\n");
		return;
	}

	while(fread(&channel,sizeof(adefchan),1,in1)>0){
		ptr=strchr(channel.mode,' ');
		if(ptr!=NULL)
			*ptr='\0';

		/* don't show secret or private channel */
		if(strpbrk(channel.mode,"sp"))
			continue;

		fprintf(out,"%s  Created: %s",channel.name,ctime(&channel.TS));

		in2=fopen(USERFILE,"r");
		if(in2==NULL){
			fprintf(out,"Can't open \"%s\"\n",USERFILE);
		}else{
			while(fread(&user,sizeof(DiskUser),1,in2)>0){
				if(!strcasecmp(user.channel,channel.name)&&
				   user.Access>=SET_DEFAULT_LEVEL){
					fprintf(out,"  %s %s  Last seen: %s",
					    user.realname,user.match,
					    ctime(&user.lastseen));
				}
			}
			fclose(in2);
		}
		fprintf(out,"\n");
	}

	fclose(in1);
	fclose(out);
}

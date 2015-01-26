/* @(#)$Id: listall.c,v 1.3 1996/11/13 00:40:43 seks Exp $ */

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
        char passwd[20];
        char channel[80];
	char modif[80];
        int Access;
        unsigned long flags;
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

	out=stdout;

	currtime=time(NULL);

	in2=fopen(USERFILE,"r");
	while(fread(&user,sizeof(DiskUser),1,in2)>0){
		for(ptr=user.channel;*ptr;ptr++)
			*ptr=tolower(*ptr);
		fprintf(out,"[%s] %s %s (%d) Last modif: %s, Last seen: %s",
		    user.channel,
		    user.realname,user.match,user.Access,
		    user.modif,ctime(&user.lastseen));
	}
	fclose(in2);
	fclose(out);
}

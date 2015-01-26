/* @(#)$Id: nick.c,v 1.3 1996/11/13 00:40:44 seks Exp $ */

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

#include "h.h"

void NickInUse(void)
{
	char buffer[200];

	sprintf(buffer,":%s SQUIT %s 0 :Unexpected nick collision\n",
		SERVERNAME,SERVERNAME);
	sendtoserv(buffer);
	dumpbuff();
	close(Irc.fd);
	Irc.fd=-1;
	QuitAll();
	if(reconnect(server)){
		try_later(server);
	}
#if 0
	char *ptr;
	achannel *chan;
	int index=0;

	do{
		if(!strcmp(mynick,DEFAULT_NICKNAME)){
			strcat(mynick,"1");
		}else{
			ptr = (mynick+strlen(mynick)-1);
			(*ptr)++;
			if(*ptr>'z') *ptr='0';
		}
	}while(ToLuser(mynick)!=NULL);

	sprintf(buffer,
		"ERROR: ARGH! Nick already in use! Changing to %s",mynick);
	log(buffer);

	for(index=0;index<1000;index++){
		chan=ChannelList[index];
		while(chan!=NULL){
			chan->on=chan->AmChanOp=0;
			chan=chan->next;
		}
	}
	/*logTS=time(NULL);*/
	signon();
	joindefault();
#endif
}

void ChNick(char *newnick)
{
	char buffer[80];
	sprintf(buffer,":%s NICK %s :%ld\n",mynick,newnick,now);
	sendtoserv(buffer);
	strcpy(mynick,newnick);
}

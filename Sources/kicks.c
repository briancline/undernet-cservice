/* @(#)$Id: kicks.c,v 1.5 1997/07/18 07:55:04 cvs Exp $ */

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

void kick(char *source,char *chanarg,char *args)
{
	char buffer[500];
	char nick[80];
	char channel[80];
	char *comment;
	int found=0;
	achannel *chan;
	auser *user;

	if(*args=='#'){
		GetWord(0,args,channel);
		GetWord(1,args,nick);
		comment=ToWord(2,args);
	}else{
		GetWord(0,args,nick);
		comment=ToWord(1,args);
		strcpy(channel,chanarg);
		GuessChannel(source,channel);
	}

	if(strlen(comment)>200)
		comment[200]='\0';

	if(!strcmp(channel,"*") || !*nick){
		notice(source,"SYNTAX: kick [#channel] <nick|pattern> [reason]");
		return;
	}

	chan=ToChannel(channel);
	if(!chan||!chan->on){
		if(*source)
			notice(source,replies[RPL_NOTONCHANNEL][L_DEFAULT]);
		return;
	}

	if(*source && (chan->flags&CFL_OPONLY)){
		notice(source,replies[RPL_OPONLY][chan->lang]);
		return;
	}

	if(!chan->AmChanOp){
		if(*source)
			notice(source,replies[RPL_NOTCHANOP][chan->lang]);
		return;
	}

	/* check whether there are wildcards or not.
	 * if there are wildcards, it's a masskick and nick is a match pattern
	 * otherwise, it's an ordinary kick and nick is the nick to kick
	 */
	if(!strpbrk(nick,"*?")){
#ifdef DEBUG
		printf("KICK REQUEST (NO WILDCARDS)\nSOURCE %s\nCHANNEL %s\nTARGET %s\n",
			source,channel,nick);
#endif		
		if(*source&&Access(channel,source)<KICK_LEVEL){
			ReplyNotAccess(source,channel);
			return;
		}

		user=ToUser(channel,nick);
		if(!user) return;

		if(*comment){
			if(*source)
				sprintf(buffer,":%s KICK %s %s :(%s) %s\n",
					mynick,channel,nick,source,comment);
			else
				sprintf(buffer,":%s KICK %s %s :%s\n",
					mynick,channel,nick,comment);
		}else{
			if(*source)
				sprintf(buffer,":%s KICK %s %s :From %s\n",
					mynick,channel,nick,source);
			else
				sprintf(buffer,":%s KICK %s %s :%s\n",
					mynick,channel,nick,mynick);
		}
		sendtoserv(buffer);
		sprintf(buffer,"I KICK %s OFF %s",nick,channel);
		log(buffer);
	}else{
#ifdef DEBUG
		printf("KICK REQUEST (WITH WILDCARDS)\nSOURCE %s\nCHANNEL %s\nTARGET %s\n",
			source,channel,nick);
#endif
		if(*source&&Access(channel,source)<MASS_KICK_LEVEL){
			ReplyNotAccess(source,channel);
			return;
		}

		user=chan->users;
		while(user){
			sprintf(buffer,"%s!%s@%s",user->N->nick,user->N->username,user->N->site);
			if(match(buffer,nick)&&(!*source||strcasecmp(user->N->nick,source))){
				if(*comment){
					if(*source)
						sprintf(buffer,":%s KICK %s %s :(%s) %s\n",mynick,channel,user->N->nick,source,comment);
					else
						sprintf(buffer,":%s KICK %s %s :%s\n",mynick,channel,user->N->nick,comment);
				}else{
					if(*source)
						sprintf(buffer,":%s KICK %s %s :From %s\n",
						        mynick,channel,user->N->nick,source);
					else
						sprintf(buffer,":%s KICK %s %s :%s\n",
							mynick,channel,user->N->nick,mynick);
				}
				sendtoserv(buffer);
				sprintf(buffer,"I KICK %s OFF %s",user->N->nick,channel);
				log(buffer);
				found=1;
			}
			user=user->next;
		}
		if(*source && !found)
			notice(source,replies[RPL_NOMATCH][chan->lang]);
	}
}

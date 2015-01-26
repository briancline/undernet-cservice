/* @(#)$Id: defchan.c,v 1.7 1999/12/19 16:33:13 seks Exp $ */

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

#define FORMATNO 0x02

typedef struct adefchan0 {
        char name[50];
        char mode[80];
        char reserved[20];
        int MassDeopPro;
        int NickFloodPro;
        int MsgFloodPro;
        time_t TS;
        unsigned long flags;
        unsigned long uflags;
        struct adefchan *next;
} adefchan0;

typedef struct adefchan1 {
        char name[50];
        char mode[80];
        int MassDeopPro;
        int NickFloodPro;
        int MsgFloodPro;
        int lang;
        time_t TS;
        unsigned long flags;
        unsigned long uflags;
        struct adefchan *next;
} adefchan1;

static int active=0;

void SearchChan(char *source,char *ch,char *args)
{
	char buffer[200],keylist[512];
	char *tok[16];
	adefchan *list[11];
	register adefchan *curr;
	register int found=0,i=0;

	if(!*args){
		notice(source,"SYNTAX: search <keywords>");
		return;
	}

	strcpy(keylist,args);

	tok[0]=strtok(keylist," ");
	while(i<16 && (tok[++i]=strtok(NULL," "))!=NULL);
	tok[i]=NULL;

	for(curr=DefChanList; curr; curr=curr->next){
		if((key_match(curr->name,tok)||key_match(curr->topic,tok)) &&
		   !IsSet(curr->name,'s',"") && !IsSet(curr->name,'p',"")){
			list[found]=curr;
			found++;
			if(found>10){
				sprintf(buffer,"There are more than 10 entries matching [%s]",args);
				notice(source,buffer);
				sprintf(buffer,"Please restrict your search mask");
				notice(source,buffer);
				return;
			}
		}
	}

	if(!found){
		sprintf(buffer,"No matching entries for [%s]",args);
		notice(source,buffer);
		return;
	}

	for(i=0; i<found; i++){
		sprintf(buffer,"%-15s - %s",list[i]->name,list[i]->topic);
		notice(source,buffer);
	}
}


void AddChan(char *source,char *ch,char *args)
{
	char buffer[200];
	char channel[80];
	register adefchan *curr,**scan;
	register achannel *chan;
	register int found=0;

	if(*args=='#'){
		GetWord(0,args,channel);
	}else{
		strcpy(channel,ch);
		GuessChannel(source,channel);
	}

	if(!strcmp(channel,"*")){
		notice(source,"SYNTAX: addchan <channel>");
		return;
	}

	chan=ToChannel(channel);
	if(chan==NULL || !chan->on){
		notice(source,replies[RPL_NOTONCHANNEL][L_DEFAULT]);
		return;
	}

	if(Access(channel,source)<SET_DEFAULT_LEVEL){
		ReplyNotAccess(source,channel);
		return;
	}

	chan=ChannelList[cl_hash(channel)];

	while(chan && !found){
		if(!strcasecmp(chan->name,channel)){
			/* look if the channel is already in the
			 * default channels list
			 */

			curr=DefChanList;
			while(curr&&strcasecmp(curr->name,channel))
				curr=curr->next;

			/* The channel *IS NOT* in the list */
			if(curr==NULL){
				curr=(adefchan *)MALLOC(sizeof(adefchan));
	                        scan=&DefChanList;
	                        while(*scan && strcasecmp((*scan)->name,chan->name)<0)
	                                scan=&(*scan)->next;
	                        curr->next=*scan;
	                        *scan=curr;

				curr->url[0]='\0';
				curr->topic[0]='\0';
			}

			strcpy(curr->name,chan->name);
			strcpy(curr->mode,chan->mode);
			curr->MassDeopPro=chan->MassDeopPro;
			curr->NickFloodPro=chan->NickFloodPro;
			curr->MsgFloodPro=chan->MsgFloodPro;
			curr->lang=chan->lang;
			curr->flags=chan->flags;
			curr->uflags=chan->uflags;
			curr->TS=chan->TS;

			found=1;
		}else
			chan=chan->next;
	}
	sprintf(buffer,"I ADD DEFAULT CHANNEL %s",channel);
	log(buffer);

	if(found){
		notice(source,replies[RPL_SETCHANDEFS][chan->lang]);
#ifdef BACKUP
		notice(source,"This modification will not be permanent");
#endif
	}else{
		notice(source,replies[RPL_NOTONCHANNEL][L_DEFAULT]);
	}
}

void RemChan(char *source, char *ch, char *arg)
{
	char buffer[200];
	char channel[80];
	register adefchan *chan,*prev;

	if(*arg=='#'){
		GetWord(0,arg,channel);
	}else{
		strcpy(channel,ch);
		GuessChannel(source,channel);
	}

	if(!strcmp(channel,"*")){
		notice(source,"SYNTAX: remchan <channel>");
		return;
	}

	if(*source && Access(channel,source)<SET_DEFAULT_LEVEL){
		ReplyNotAccess(source,channel);
		return;
	}

	prev=NULL;
	for(chan=DefChanList;chan&&strcasecmp(channel,chan->name);chan=chan->next)
		prev=chan;

	if(!chan){
		notice(source,replies[RPL_NOTDEF][L_DEFAULT]);
		return;
	}

	if(prev)
		prev->next=chan->next;
	else
		DefChanList=chan->next;

	TTLALLOCMEM-=sizeof(adefchan);
	free(chan);

	sprintf(buffer,"I REMOVE DEFAULT CHANNEL %s",channel);
	log(buffer);
	if(*source){
		notice(source,replies[RPL_REMDEF][L_DEFAULT]);
#ifdef BACKUP
		notice(source,"This modification will not be permanent");
#endif
	}
}

void SaveDefs(char *source)
{
	register int file;
	register adefchan *curr;
	adefchan buff;
	filehdr hdr;
	char global[]="*";
	char buffer[200];

	if(*source&&Access(global,source)<SAVE_DEFAULTS_LEVEL){
		notice(source,"Your admin Access is too low!");
		return;
	}

	if(*source)
		notice(source,"Saving defaults file...");

	if(active)
		return;
	active=1;

	sprintf(buffer,":%s AWAY :Busy saving precious channel list\n",mynick);
	sendtoserv(buffer);
        dumpbuff();

	alarm(5); /* avoid NFS hangs */
	file=open(DEFAULT_CHANNELS_FILE".new",O_WRONLY|O_CREAT|O_TRUNC,0600);
	alarm(0);

	if(file<0){
		if(*source)
			notice(source,"Error while opening file. Aborted!");
		log("ERROR Saving defaults channels");
	}else{
		hdr.magic=0xff;
		hdr.no=FORMATNO;
		alarm(2);
		if(write(file,&hdr,sizeof(hdr))<=0){
			alarm(0);
			close(file);
			log("ERROR: Can't save channel list");
			log((char *)sys_errlist[errno]);
			alarm(2);
			remove(DEFAULT_CHANNELS_FILE".new");
			alarm(0);
			active=0;
			sprintf(buffer,":%s AWAY\n",mynick);
			sendtoserv(buffer);
			return;
		}
		alarm(0);

		curr=DefChanList;
		while(curr){
                        achannel *chan = ToChannel(curr->name);

			buff = *curr;
                        if(chan)
				buff.TS = chan->TS;
			else
				buff.TS = curr->TS;
			buff.flags = curr->flags;

			alarm(2);
			if(write(file,&buff,sizeof(adefchan))<=0){
				alarm(0);
				close(file);
				log("ERROR: Can't save channel list");
				log((char *)sys_errlist[errno]);
				alarm(2);
				remove(DEFAULT_CHANNELS_FILE".new");
				alarm(0);
				active=0;
				sprintf(buffer,":%s AWAY\n",mynick);
				sendtoserv(buffer);
				return;
			}
			alarm(0);
			curr=curr->next;
		}
		close(file);
		alarm(20);
		rename(DEFAULT_CHANNELS_FILE".new",DEFAULT_CHANNELS_FILE);
		alarm(0);
		if(*source)
			notice(source,"Done.");
	}
	active=0;
	sprintf(buffer,":%s AWAY\n",mynick);
	sendtoserv(buffer);
}

void LoadDefs(char *source)
{
	register adefchan *curr,**scan;
	adefchan buffer;
	adefchan0 bufold0;
	adefchan1 bufold1;
	filehdr hdr;
	register int file;
	char global[]="*";

	if(*source&&Access(global,source)<LOAD_DEFAULT_LEVEL){
		notice(source,"Your admin Access is too low!");
		return;
	}

	if(active)
		return;
	active=1;

	file=open(DEFAULT_CHANNELS_FILE,O_RDONLY);
	if(file<0){
		if(*source) notice(source,"Error opening file! Aborted.");
		log("ERROR Loading the default channels");
		active=0;
		return;
	}

	/* empty existing defaults */

	while((curr=DefChanList)!=NULL){
		DefChanList=DefChanList->next;
		TTLALLOCMEM-=sizeof(adefchan);
		free(curr);
	}

	/* find file format */
	read(file,&hdr,sizeof(hdr));
	if(hdr.magic!=0xff){
		lseek(file,0L,SEEK_SET);
		hdr.no=0x00;
	}

	if(hdr.no == FORMATNO){
		scan=&DefChanList;
		while(read(file,&buffer,sizeof(adefchan))>0){
#ifdef DEBUG
			printf("DEFCHAN: %s %d %d %d %ld\n",buffer.name,
				buffer.MassDeopPro,buffer.NickFloodPro,
				buffer.MsgFloodPro,buffer.flags);
			printf("TS: %lu\n",buffer.TS);
#endif
			if(logTS==0||buffer.TS<logTS){
#ifdef DEBUG
				printf("TS is older than %lu\n",logTS);
#endif
				logTS=buffer.TS;
			}
	
			if(buffer.TS<0){
				/* if the TS of a channel is negative
				 * the file is prolly corrupted!
				 */
				log("Channel file is corrupted");
				quit("LoadDefs(): corrupted file!", 1);
			}
	
			curr=(adefchan *)MALLOC(sizeof(adefchan));
			*curr=buffer;
			curr->next=*scan;
			*scan=curr;
			scan=&(*scan)->next;
		}
	}else if(hdr.no == 0x01){
		while(read(file,&bufold1,sizeof(adefchan1))>0){
#ifdef DEBUG
			printf("DEFCHAN: %s %d %d %d %ld\n",bufold1.name,
				bufold1.MassDeopPro,bufold1.NickFloodPro,
				bufold1.MsgFloodPro,bufold1.flags);
			printf("TS: %lu\n",bufold1.TS);
#endif
			if(logTS==0||bufold1.TS<logTS){
#ifdef DEBUG
				printf("TS is older than %lu\n",logTS);
#endif
				logTS=bufold1.TS;
			}
	
			if(bufold1.TS<0){
				/* if the TS of a channel is negative
				 * the file is prolly corrupted!
				 */
				log("Channel file is corrupted");
				quit("LoadDefs(): corrupted file!", 1);
			}
	
			curr=(adefchan *)MALLOC(sizeof(adefchan));
			strcpy(curr->name,bufold1.name);
			strcpy(curr->mode,bufold1.mode);
			curr->url[0]='\0';
			curr->topic[0]='\0';
			curr->MassDeopPro=bufold1.MassDeopPro;
			curr->NickFloodPro=bufold1.NickFloodPro;
			curr->MsgFloodPro=bufold1.MsgFloodPro;
			curr->lang=bufold1.lang;
			curr->TS=bufold1.TS;
			curr->flags=bufold1.flags;
			curr->uflags=bufold1.uflags;
			scan=&DefChanList;
			while(*scan && strcasecmp((*scan)->name,curr->name)<0)
				scan=&(*scan)->next;
			curr->next=*scan;
			*scan=curr;
		}
	}else{  /* old file format */
		while(read(file,&bufold0,sizeof(adefchan0))>0){
#ifdef DEBUG
			printf("DEFCHAN: %s %d %d %d %ld\n",bufold0.name,
				bufold0.MassDeopPro,bufold0.NickFloodPro,
				bufold0.MsgFloodPro,bufold0.flags);
			printf("TS: %lu\n",bufold0.TS);
#endif
			if(logTS==0||bufold0.TS<logTS){
#ifdef DEBUG
				printf("TS is older than %lu\n",logTS);
#endif
				logTS=bufold0.TS;
			}
	
			if(bufold0.TS<0){
				/* if the TS of a channel is negative
				 * the file is prolly corrupted!
				 */
				log("Channel file is corrupted");
				quit("LoadDefs(): corrupted file!", 1);
			}
	
			curr=(adefchan *)MALLOC(sizeof(adefchan));
			strcpy(curr->name,bufold0.name);
			strcpy(curr->mode,bufold0.mode);
			curr->url[0]='\0';
			curr->topic[0]='\0';
			curr->MassDeopPro=bufold0.MassDeopPro;
			curr->NickFloodPro=bufold0.NickFloodPro;
			curr->MsgFloodPro=bufold0.MsgFloodPro;
			curr->lang=L_DEFAULT;
			curr->TS=bufold0.TS;
			curr->flags=bufold0.flags;
			curr->uflags=bufold0.uflags;
			scan=&DefChanList;
			while(*scan && strcasecmp((*scan)->name,curr->name)<0)
				scan=&(*scan)->next;
			curr->next=*scan;
			*scan=curr;
		}
	}	

	close(file);

	if(*source)
		notice(source,"Defaults file loaded!");

	active=0;
}

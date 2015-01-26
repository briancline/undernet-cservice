/* @(#)$Id: special.c,v 1.3 1996/11/13 00:40:49 seks Exp $ */

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

#define SPECFILE "special.log"

void SpecLog(char *text)
{
	int fd;
	char date[80],buffer[1024];
	strcpy(date,ctime(&now));
	*strchr(date,'\n')='\0';

	alarm(2);
	if((fd=open(SPECFILE,O_WRONLY|O_CREAT|O_APPEND,0600))>=0){
		alarm(0);
		sprintf(buffer,"%s: %s\n",date,text);
		alarm(2);
		write(fd,buffer,strlen(buffer));
		alarm(0);
		close(fd);
	}
	alarm(0);
}


void logmap(aserver *server,FILE *fp)
{
        static char prefix[80]="";
        static int offset=0;
        asuser *suser;
        int nbusers=0,i;


        if(server==NULL){
                return;
        }

        /* count number of users */
	for(i=0;i<100;i++){
	        suser=server->users[i];
	        while(suser!=NULL){
	                nbusers++;
	                suser=suser->next;
	        }
	}

        if(server->next==NULL){
                fprintf(fp,"%s`-%s  (%d user%s)\n",prefix,server->name,nbusers,(nbusers>1)?"s":"");
        }else{
                fprintf(fp,"%s|-%s  (%d user%s)\n",prefix,server->name,nbusers,(nbusers>1)?"s":"");
        }

        if(server->next!=NULL)
                strcpy(prefix+offset,"| ");
        else
                strcpy(prefix+offset,"  ");

        offset+=2;
        logmap(server->down,fp);
        offset-=2;
        prefix[offset]='\0';

        logmap(server->next,fp);
}

void SpecMap(void)
{
	FILE *fp;
	if((fp=fopen(SPECFILE,"a"))!=NULL){
		logmap(ServerList,fp);
		fclose(fp);
	}
}

#ifdef CHANNEL_LOG
void LogChan(void)
{
	register achannel *chan;
	register auser *user;
	register aluser *luser;
	register int i,isreg,count;
	register FILE *fp;
	char mode[80];
	char flag;

	if((fp=fopen(CHANNEL_LOG,"a"))==NULL)
		return;

	fprintf(fp,"%ld %s",now,ctime(&now));

	for(i=0;i<1000;i++){
		for(chan=ChannelList[i];chan!=NULL;chan=chan->next){
			count=isreg=(chan->on)?1:0;

			for(user=chan->users;user;user=user->next){
				count++;
				if(!isreg &&
				 !strcasecmp(user->N->username,DEFAULT_USERNAME)&&
				 !strcasecmp(user->N->site,DEFAULT_HOSTNAME)){
					isreg=1;
				}
			}
			GetWord(0,chan->mode,mode);
			if(strchr(mode,'s'))
				flag='S';
			else if(strchr(mode,'p'))
				flag='P';
			else
				flag='+';

			fprintf(fp,"%s %c %d %s\n",
				chan->name,flag,count,isreg?"REG'D":"");
		}
	}
	count=0;
	for(i=0;i<1000;i++){
		luser=Lusers[i];
		while(luser!=NULL){
			count++;
			luser=luser->next;
		}
	}
	fprintf(fp,"* %d \n",count);

	fclose(fp);
}
#endif

#ifdef HISTORY
void HistLog(char *text)
{
        int fd;
        char date[80],buffer[1024];
        strcpy(date,ctime(&now));
        *strchr(date,'\n')='\0';

        alarm(2);
        if((fd=open("hist.log",O_WRONLY|O_CREAT|O_APPEND,0600))>=0){
                alarm(0);
                sprintf(buffer,"%s: %s\n",date,text);
                alarm(2);
                write(fd,buffer,strlen(buffer));
                alarm(0);
                close(fd);
        }
        alarm(0);
}

void History(char *line)
{
	static char Log[25][512];
	static int offset=0;

	if(line==NULL){
		int i=offset,top=offset-1;
		if(top==-1)
			top=25;
		while(i!=top){
			if(i==25)
				i=0;
			HistLog(Log[i]);
			i++;
		}
	}
	else
	{
		strncpy(Log[offset],line,511);
		Log[offset][511]='\0';
		if(++offset==25)
			offset=0;
	}
}
#endif

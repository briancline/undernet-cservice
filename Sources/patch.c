/* @(#)$Id: patch.c,v 1.3 1996/11/13 00:40:46 seks Exp $ */

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
#include <stdarg.h>

#ifdef UPGRADE

static void misc_closed_conn(misc_socket *);

void upgrade(char *source, char *args)
{
	char global[]="*";

	if(Access(global,source)<LEVEL_UPGRADE){
		notice(source,"This command is not for you");
		return;
	}

	open_patch_socket(source);
}

void open_patch_socket(char *source)
{
	misc_socket *msock;
	struct sockaddr_in socketname;
	struct hostent *remote_host;
	char pserver[80]=PATCH_SERVER, buffer[200], *ptr;
	int port;

	if((ptr=strchr(pserver,':'))==NULL){
		notice(source,"PATCH_SERVER needs to be \"server:port\"!");
		return;
	}
	*(ptr++)='\0';
	sscanf(ptr,"%d",&port);

#ifdef DEBUG
	printf("Connecting to patch server %s on port %d\n",pserver,port);
#endif
	sprintf(buffer,"Connecting to patch server %s on port %d",pserver,port);
	log(buffer);

	notice(source,"Attempting to contact patch server...");

	msock=(misc_socket *)MALLOC(sizeof(misc_socket));
	msock->type=MISC_GETPATCH;
	msock->status=MISC_CONNECTING;
	msock->TS=now;
	strcpy(msock->link,source);
	msock->inbuf=NULL;
	msock->outbuf=NULL;

	if((msock->fd=socket(AF_INET,SOCK_STREAM,0))<0){
		free(msock);
		sprintf(buffer,"Can't assigned fd for socket: %s",sys_errlist[errno]);
		notice(source,buffer);
		return;
	}

	fcntl(msock->fd,F_SETFL,O_NONBLOCK);

	socketname.sin_family=AF_INET;
	if((remote_host=gethostbyname(pserver))==NULL){
		sprintf(buffer,"gethostbyname() failed for %s: %s",pserver,sys_errlist[errno]);
		notice(source,buffer);
		close(msock->fd);
		free(msock);
		return;
	}

	memcpy((void *)&socketname.sin_addr,(void *)remote_host->h_addr,remote_host->h_length);
	socketname.sin_port = htons(port);

	if(connect(msock->fd,(struct sockaddr *)&socketname,sizeof(socketname))<0
	   && errno != EINPROGRESS){
		close(msock->fd);
		sprintf(buffer,"Can't connect() to %s: %s",pserver,sys_errlist[errno]);
		notice(source,buffer);
		free(msock);
		return;
	}

	msock->next=MiscList;
	MiscList=msock;

	send_misc_handshake(msock);
}


int readfrom_misc(misc_socket *msock)
{
	void parse_misc(misc_socket *,char *);
	char buf[1024];
	int length;
	if((length=read(msock->fd,buf,1023))<=0){
		if(errno==EWOULDBLOCK || errno==EAGAIN){
			return 0;
		}else{
			misc_closed_conn(msock);
			close(msock->fd);
			msock->fd=-1;
			msock->status=MISC_ERROR;
			return -1;
		}
	}
	buf[length]='\0';

	copy_to_buffer(&msock->inbuf,buf,length);

	if(find_char_in_buffer(&msock->inbuf,'\n',1023)){
		while(find_char_in_buffer(&msock->inbuf,'\n',1023)){
			copy_from_buffer(&msock->inbuf,buf,'\n',1023);
			parse_misc(msock,buf);
		}
	}

	return 1;
}


int flush_misc_buffer(misc_socket *msock)
{
	char buf[1024];
	int length;
	int count;

	if(msock->status==MISC_CONNECTING){
		msock->status=MISC_HANDSHAKE;
		notice(msock->link,"Connected.");
	}

	if(msock==NULL || msock->outbuf==NULL)
		return -1;

	while((count = look_in_buffer(&msock->outbuf,buf,'\0',1023))>0){
		if((length=write(msock->fd,buf,count))<=0){
			if((errno==EWOULDBLOCK || errno==EAGAIN) && length!=0){
				return 0;
			}else{
				misc_closed_conn(msock);
				close(msock->fd);
				msock->fd=-1;
				msock->status=MISC_ERROR;
				return -1;
			}
		}else{
			skip_char_in_buffer(&msock->outbuf,length);
		}
	}

	return 0;
}


long sendto_misc(misc_socket *msock, char *format, ...)
{
	va_list args;
	char string[1024];

	va_start(args,format);
	vsprintf(string,format,args);
#ifdef DEBUG
	vprintf(format,args);
#endif
	va_end(args);

	return copy_to_buffer(&msock->outbuf,string,strlen(string));
}


void send_misc_handshake(misc_socket *msock)
{
	struct stat stlast;
	char tag[80], buffer[200];
	FILE *fp;

	if(msock->type==MISC_GETPATCH){
		if(stat("lastupgrade",&stlast)<0){
			sprintf(buffer,"lastupgrade: %s",
				sys_errlist[errno]);
			notice(msock->link,buffer);
			close(msock->fd);
			msock->status=MISC_ERROR;
		}
		fp=fopen("lastupgrade","r");
		if(fp==NULL){
			close(msock->fd);
			msock->fd=-1;
			msock->status=MISC_ERROR;
			sprintf(buffer,"lastupgrade: %s",sys_errlist[errno]);
			notice(msock->link,buffer);
			return;
		}
		fgets(tag,80,fp);
		fclose(fp);

		sendto_misc(msock,"%s\n%s\n",GETPATCHPASS,tag);
	}
}


static void misc_closed_conn(misc_socket *msock)
{
	int pipefd[2],i;
	char *ptr;

	if(msock->type==MISC_GETPATCH){
		if(msock->status==MISC_CONNECTING)
			notice(msock->link,"Connection refused.");
		else if(msock->status==MISC_HANDSHAKE)
			notice(msock->link,"Handshake failed.");
		else
			notice(msock->link,"Connection closed.");
	}else if(msock->type==MISC_PIPE_PATCH){
		notice(msock->link,"patch process exited.");

		pipe(pipefd);
		switch(fork()){
			case 0:
			  dup2(pipefd[1],1);
			  dup2(pipefd[1],2);
			  close(0);
			  for(i=3;i<MAX_CONNECTIONS;i++) close(i);
			  execl(MAKE,MAKE,(char *)0);
			  exit(-1);
			case -1:
			  notice(msock->link,"fork() failed!");
			  /* socket already closed.. */
			  break;
			default:
			  notice(msock->link,"Spawning make..");
			  close(pipefd[1]);
			  ptr=msock->link;
			  msock=(misc_socket *)MALLOC(sizeof(misc_socket));
			  msock->fd=pipefd[0];
			  msock->type=MISC_PIPE_MAKE;
			  msock->status=MISC_RECV;
			  msock->TS=now;
			  strcpy(msock->link,ptr);
			  msock->inbuf=NULL;
			  msock->outbuf=NULL;
			  msock->next=MiscList;
			  MiscList=msock;
			  break;
		}
	}else if(msock->type==MISC_PIPE_MAKE){
		notice(msock->link,"make process exited.");
	}else{
		notice(msock->link,"Connection closed.");
	}
}

void parse_misc(misc_socket *msock, char *line)
{
	FILE *fp;
	int fdpipe[2],i;
	char *ptr;

	if(msock->status==MISC_CONNECTING){
		msock->status=MISC_HANDSHAKE;
		notice(msock->link,"Connected!");
	}

	while((ptr=strpbrk(line,"\r\n"))!=NULL)
		*ptr='\0';

	if(!*line)
		return;

	if(msock->type==MISC_GETPATCH){
		if(msock->status==MISC_HANDSHAKE){
			if(strcmp(RECPATCHPASS,line)){
				close(msock->fd);
				msock->fd=-1;
				msock->status=MISC_ERROR;
				notice(msock->link,"Bad password!");
				return;
			}
			msock->status=MISC_RECV;
		}else if(msock->status==MISC_RECV){
			fp=fopen("lastupgrade","w");
			if(fp!=NULL){
				fprintf(fp,"%s\n",line);
				fclose(fp);
			}

			/* spawn patch */
			pipe(fdpipe);
			switch(fork()){
				case 0:
				  fcntl(msock->fd,F_SETFL,0);
				  dup2(msock->fd,0);
				  dup2(fdpipe[1],1);
				  dup2(fdpipe[1],2);
				  for(i=3;i<MAX_CONNECTIONS;i++) close(i);
				  putenv("IFS="); /* I'm paranoid */
				  execl("/bin/sh","[patch script]","-c",
				    "tee upgrade.patch | "PATCH" -p1",
				    (char *)0);
				  exit(-1);
				case -1:
				  notice(msock->link,"fork() failed");
				  close(msock->fd);
				  msock->fd=-1;
				  msock->status=MISC_ERROR;
				  break;
				default:
				  notice(msock->link,"Spawning patch script..");
				  close(msock->fd);
				  while(msock->inbuf!=NULL){
				    char buf[512];
				    int l;
				    l=copy_from_buffer(&msock->inbuf,buf,'\0',511);
				    write(fdpipe[1],buf,l);
				  }
				  close(fdpipe[1]);
				  msock->fd=fdpipe[0];
				  msock->type=MISC_PIPE_PATCH;
				  zap_buffer(&msock->outbuf);
				  break;
			}
		}
	}else if(msock->type==MISC_PIPE_PATCH || msock->type==MISC_PIPE_MAKE){
		notice(msock->link,line);
	}
}
#endif /* UPGRADE */

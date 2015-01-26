/* @(#)$Id: mkhtpasswd.c,v 1.1 1997/04/09 14:04:01 cvs Exp $ */

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

#include "Sources/h.h"

struct node {
  dbuser dbu;
  struct node *next;
};

time_t now;

int ul_hash(char *channel)
{
        register int i,j=0;
        for(i=1;i<strlen(channel);i++)
                j+=(unsigned char)toupper(channel[i]);
        return (j%1000);
}


char *make_dbfname(char *channel)
{
  static char fname[600];
  char tmp[600];
  register char *ptr;

  strcpy(tmp,channel);
  for(ptr=tmp; *ptr; ptr++)
  {
    if(*ptr=='/')
      *ptr=' ';
    else
      *ptr=tolower(*ptr);
  }

  sprintf(fname,"db/channels/%04X/%s",ul_hash(channel),tmp);
  return fname;
}

void do_channel(char *channel)
{
  char salts[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";
  char passwd[20],salt[3];
  dbuser dbu;
  int fd;

  fd=open(make_dbfname(channel),O_RDONLY);
  if(fd<0)
  {
    fprintf(stderr,"Can't open %s [%s]\n",
            make_dbfname(channel),sys_errlist[errno]);
    return;
  }

  while(read(fd,&dbu,sizeof(dbuser))==sizeof(dbuser))
  {
    if(dbu.header[0]!=0xFF || dbu.header[1]!=0xFF ||
       dbu.footer[0]!=0xFF || dbu.footer[1]!=0xFF)
    {
      continue;
    }
    if(strcasecmp(channel,dbu.channel))
      continue;

    if(!strcmp(dbu.match,"!DEL!"))
      continue;

    salt[0] = salts[random()%64];
    salt[1] = salts[random()%64];
    salt[2] = '\0';

    strcpy(passwd,crypt(dbu.passwd,salt));
    printf("%s:%s\n",dbu.nick,passwd);
  }
  close(fd);
}


void main(int argc, char **argv)
{
  if(chdir(HOMEDIR)<0)
  {
    perror(HOMEDIR);
    exit(1);
  }


  if(argc!=2)
  {
    fprintf(stderr,"usage: %s <channel>\n",argv[0]);
    exit(1);
  }

  time(&now);
  srandom(now);

  do_channel(argv[1]);
}


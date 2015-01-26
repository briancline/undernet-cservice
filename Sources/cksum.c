/* $Id: cksum.c,v 1.1 1997/06/30 03:48:05 cvs Exp $
 */

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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


void cksum(char *file, unsigned int *sum1, unsigned int *sum2)
{
  int fd=-1, size;
  unsigned char buffer[1024];

  if(strchr(file,'/') != NULL)
  {
    fd = open(file,O_RDONLY);
  }
  else  /* check path */
  {
    char *tok,*path = getenv("PATH");
    if(path == NULL)
      return;
    for(tok=strtok(path,":"); tok!=NULL; tok=strtok(NULL,":"))/* mangles PATH*/
    {
      strncpy(buffer,tok,1023);
      strncat(buffer,"/",1023);
      strncat(buffer,file,1023);
      buffer[1023] = '\0';
      fd = open(buffer,O_RDONLY);
      if(fd >= 0)
        break;
    }
  }
    
  if(fd<0)
  {
    return;
  }

  *sum1 = *sum2 = 0;

  while((size=read(fd,buffer,1023))>0)
  {
    while(size--)
    {
      *sum1 += buffer[size];
      *sum2 += (size&1)?-buffer[size]:buffer[size];
    }
  }

  close(fd);
}


#ifdef MAIN
int main(int argc, char **argv)
{
  unsigned int sum,pro;

  if(argc != 2)
  {
    fprintf(stderr,"usage: %s file\n",argv[0]);
    exit(1);
  }

  cksum(argv[1],&sum,&pro);
  printf("-DBINCKSUM1=0x%x -DBINCKSUM2=0x%x\n",sum,pro);
  exit(0);
}
#endif


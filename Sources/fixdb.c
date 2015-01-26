/* @(#)$Id: fixdb.c,v 1.5 1998/01/12 20:02:17 seks Exp $ */

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
#include "dirent.h"
#define SPECFILE "special.log"

struct node
{
  dbuser dbu;
  struct node *next;
};

unsigned long ttlread = 0L, ttlwrite = 0L;
time_t now;

void SpecLog(char *text)
{
  int fd;
  char date[80], buffer[1024];
  strcpy(date, ctime(&now));
  *strchr(date, '\n') = '\0';

  if ((fd = open(SPECFILE, O_WRONLY | O_CREAT | O_APPEND, 0600)) >= 0)
  {
    sprintf(buffer, "%s: %s\n", date, text);
    write(fd, buffer, strlen(buffer));
    close(fd);
  }
}


void fix_user_file(char *file, char *channel)
{
  char tmpfile[256], buffer[200];
  struct node *List = NULL, *tmp;
  dbuser dbu;
  int fd;

  fd = open(file, O_RDONLY);
  if (fd < 0)
  {
    fprintf(stderr, "Can't open %s [%s]\n", file, sys_errlist[errno]);
    return;
  }

  while (read(fd, &dbu, sizeof(dbuser)) == sizeof(dbuser))
  {
    ttlread += sizeof(dbuser);

    if (dbu.header[0] != 0xFF || dbu.header[1] != 0xFF ||
      dbu.footer[0] != 0xFF || dbu.footer[1] != 0xFF)
    {
      continue;
    }
    if (strcasecmp(channel, dbu.channel))
      continue;

    if (!strcmp(dbu.match, "!DEL!"))
      continue;

    if (dbu.lastseen + USERLIST_EXP_TIME <= now)
    {
      sprintf(buffer, "Exp: %s [%s] (%d) on %s",
	dbu.nick, dbu.match, dbu.access, dbu.channel);
      SpecLog(buffer);
      continue;
    }

    tmp = (struct node *)malloc(sizeof(struct node));
    memcpy(&tmp->dbu, &dbu, sizeof(dbuser));
    tmp->next = List;
    List = tmp;
  }
  close(fd);

  sprintf(tmpfile, "%s.new", file);

  fd = open(tmpfile, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd < 0)
  {
    fprintf(stderr, "Can't open %s\n", tmpfile);
    /* don't return.. need to free the list! */
  }

  while ((tmp = List) != NULL)
  {
    write(fd, &tmp->dbu, sizeof(dbuser));
    ttlwrite += sizeof(dbuser);
    List = List->next;
    free(tmp);
  }
  close(fd);

  if (fd >= 0)
    rename(tmpfile, file);
}

void fix_user_db(void)
{
  DIR *dp;
  struct dirent *ent;
  char dir[256], file[256], channel[80], *ptr;
  int count;

  for (count = 0; count < 1000; count++)
  {
    sprintf(dir, "db/channels/%04X", count);
    dp = opendir(dir);
    if (dp == NULL)
    {
      fprintf(stderr, "Can't read %s [%s]\n", dir, sys_errlist[errno]);
      continue;
    }
    while ((ent = readdir(dp)) != NULL)
    {
      if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
	continue;
      strcpy(channel, ent->d_name);
      for (ptr = channel; *ptr; ptr++)
	if (*ptr == ' ')
	  *ptr = '/';
      sprintf(file, "db/channels/%04X/%s", count, ent->d_name);

      /* If no comma in channel name, proceed with clean up */
      if (strchr(channel, ',') == NULL)
	fix_user_file(file, channel);
    }
    closedir(dp);
  }
}

int main(void)
{
  int pid = 0;
  FILE *fp;

  if (chdir(HOMEDIR) < 0)
  {
    perror(HOMEDIR);
    exit(1);
  }

  if ((fp = fopen(PIDFILE, "r")) != NULL)
  {
    fscanf(fp, "%d", &pid);
    fclose(fp);
    if (!kill((pid_t) pid, 0))
    {
      fprintf(stderr, "Detected cs is running [pid %d]. Abort.\n", pid);
      exit(1);
    }
  }

  if (rename("cs", "cs.fixdb") < 0)
  {
    perror("rename");
    exit(1);
  }

  if (symlink("/bin/true", "cs") < 0)
  {
    perror("symlink");
    exit(1);
  }

  time(&now);

  fix_user_db();

  if (unlink("cs") < 0)
    perror("unlink");

  if (rename("cs.fixdb", "cs") < 0)
    perror("rename");

  printf("ttlread= %ld  ttlwrite= %ld\n", ttlread, ttlwrite);

  return 0;
}

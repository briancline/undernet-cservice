/* @(#)$Id: servers.c,v 1.9 1998/01/25 18:35:47 seks Exp $ */

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

aserver **FindServer(aserver ** head, char *name)
{
  register aserver **tmp;

  if (head == NULL || *head == NULL)
    return NULL;

  while (*head != NULL)
  {
    if (!strcasecmp((*head)->name, name))
    {
      return head;
    }
    if ((tmp = FindServer(&(*head)->down, name)) != NULL)
    {
      return tmp;
    }
    else
    {
      head = &(*head)->next;
    }
  }

  return NULL;
}

aserver *ToServer(char *name)
{
  return (*FindServer(&ServerList, name));
}

void onserver(char *source, char *newserver, char *args)
{
  register aserver *head;
  register aserver *tmp;
  register int i;
  char TS[80];

  if (source[0] != '\0')
  {
    head = ToServer(source);
  }
  else
  {
    head = NULL;
  }

#ifdef BACKUP
  if (!strcasecmp(newserver, MAIN_SERVERNAME))
  {
    quit(MAIN_NICK " is back", 0);
  }
#endif

  tmp = (aserver *) MALLOC(sizeof(aserver));
  tmp->name = (char *)MALLOC(strlen(newserver) + 1);
  strcpy(tmp->name, newserver);
  GetWord(2, args, TS);
  tmp->TS = atol(TS);
  if (head == NULL)
  {
    tmp->next = ServerList;
    ServerList = tmp;
    TSoffset = tmp->TS - now;
#ifdef DEBUG
    printf("New connection: my time: %ld  others' time %ld (%ld)\n",
      now, tmp->TS, TSoffset);
#endif
  }
  else
  {
    tmp->next = head->down;
    head->down = tmp;
  }
  tmp->up = head;
  tmp->down = NULL;
  for (i = 0; i < 100; i++)
    tmp->users[i] = NULL;
}

void onsquit(char *source, char *theserver, char *args)
{
  register aserver *serv, **s;
  register int i;
  char TS[80];

#ifdef FAKE_UWORLD
  if (!strcasecmp(theserver, UFAKE_SERVER) && Uworld_status == 1)
  {
    GetWord(0, args, TS);
    if (atol(TS) == UworldServTS)
    {
      char buffer[200];
      sprintf(buffer, "%s squitted", UFAKE_NICK);
      log(buffer);
      Uworld_status = 0;
    }
    return;
  }
#endif

  s = FindServer(&ServerList, theserver);

  if (s == NULL)
  {
    char buffer[200];
    sprintf(buffer, "ERROR: SQUIT unknown server %s (from %s)",
      theserver, source);
    log(buffer);
    return;
  }

  serv = *s;

#ifdef DEBUG
  printf("SQUIT: %s %s\n", source, theserver);
#endif

  if (args != NULL)
  {
    GetWord(0, args, TS);
#ifdef DEBUG
    if (s != NULL)
      printf("ConnectTS: %ld SquitTS: %ld\n", serv->TS, atol(TS));
#endif
  }

  if (serv != ServerList && args != NULL && serv->TS != atol(TS))
  {
#ifdef DEBUG
    printf("TS's are different.. ignoring squit!\n");
#endif
    return;
  }

  while (serv->down != NULL)
  {
    onsquit(NULL, serv->down->name, NULL);
  }

  for (i = 0; i < 100; i++)
  {
    while (serv->users[i] != NULL)
    {
      onquit(serv->users[i]->N->nick);
    }
  }

  TTLALLOCMEM -= strlen(serv->name) + 1;
  free(serv->name);
  *s = serv->next;
  TTLALLOCMEM -= sizeof(aserver);
  free(serv);
}

void showmap(char *source)
{
  int count = 0;

  if (CurrentSendQ > HIGHSENDQTHRESHOLD)
  {
    notice(source, "Cannot process your request at this time. Try again later.");
    return;
  }
  notice(source, SERVERNAME);
  showserv(source, ServerList, &count);
  CheckFloodFlood(source, count);
}

void showserv(char *source, aserver * server, int *count)
{
  static char prefix[80] = "";
  static int offset = 0;
  char buffer[200];
  register asuser *suser;
  register int nbusers = 0, i;

  if (server == NULL)
  {
    return;
  }

  (*count)++;	/* number of servers */

  /* count number of users */
  for (i = 0; i < 100; i++)
  {
    suser = server->users[i];
    while (suser != NULL)
    {
      nbusers++;
      suser = suser->next;
    }
  }

  if (server->next == NULL)
  {
    sprintf(buffer, "%s`-%s  (%d client%s)", prefix, server->name, nbusers, (nbusers != 1) ? "s" : "");
  }
  else
  {
    sprintf(buffer, "%s|-%s  (%d client%s)", prefix, server->name, nbusers, (nbusers != 1) ? "s" : "");
  }
  notice(source, buffer);

  if (server->next != NULL)
    strcpy(prefix + offset, "| ");
  else
    strcpy(prefix + offset, "  ");

  offset += 2;
  showserv(source, server->down, count);
  offset -= 2;
  prefix[offset] = '\0';

  showserv(source, server->next, count);
}

void onsettime(char *source, char *value)
{
  char buffer[200];

  TSoffset = atol(value) - now;
  sprintf(buffer, "SETTIME from %s (%s) (%ld)", source, value, TSoffset);
  log(buffer);
#ifdef DEBUG
  puts(buffer);
#endif
}

void showversion(char *source)
{
  char buffer[200];

  sprintf(buffer, ":%s 351 %s . %s :%s\n", SERVERNAME, source, SERVERNAME, VERSION);
  sendtoserv(buffer);
}

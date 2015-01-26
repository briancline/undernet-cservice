/* @(#)$Id: ignore.c,v 1.19 2000/10/24 16:04:24 seks Exp $ */

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

typedef struct aprivmsg
{
  char user[200];
  time_t time;
  int length;
  struct aprivmsg *next;
}
aprivmsg;

typedef struct aignore
{
  char mask[200];
  time_t time;
  struct aignore *next;
}
aignore;


static aprivmsg *MessageList = NULL;
static aprivmsg *FloodList = NULL;
static aprivmsg *AddUserFloodList = NULL;
static aignore *IgnoreList = NULL;

void add_silence(char *, char *);
void rem_silence(char *);

int CheckPrivateFlood(char *source, int length, char *type)
{
  char buffer[200];
  char userhost[200];
  char global[] = "*";
  register aluser *user;
  register aprivmsg *msg, *prev;
  register int count, size;

  if ((user = ToLuser(source)) == NULL)
  {
    sprintf(buffer, "ERROR! CheckPrivateFlood(): %s not found", source);
    log(buffer);
    return 1;
  }

  /* Don't check flood from admins */
  if (IsValid(user, global) && Access(global, source) >= 1)
     return 0;

  if (!strcasecmp(user->username, DEFAULT_USERNAME) &&
    !strcasecmp(user->site, DEFAULT_HOSTNAME))
  {
    return 0;
  }

  /* clean messages older than 30 seconds */
  prev = NULL;
  msg = MessageList;
  while (msg != NULL)
  {
    if (msg->time < (now - 30))
    {
      if (prev)
      {
	prev->next = msg->next;
	TTLALLOCMEM -= sizeof(aprivmsg);
	free(msg);
	msg = prev->next;
      }
      else
      {
	MessageList = msg->next;
	TTLALLOCMEM -= sizeof(aprivmsg);
	free(msg);
	msg = MessageList;
      }
    }
    else
    {
      prev = msg;
      msg = msg->next;
    }
  }

  /* now add the current message in the list */
  sprintf(userhost, "%s@%s", user->username, user->site);
  msg = (aprivmsg *) MALLOC(sizeof(aprivmsg));
  strcpy(msg->user, userhost);
  msg->time = now;
  msg->length = length;
  msg->next = MessageList;
  MessageList = msg;

  /* count how many messages were received from that user
   * and also how many bytes
   */
  count = size = 0;
  for (msg = MessageList; msg != NULL; msg = msg->next)
  {
    if (compare(userhost, msg->user))
    {
      count++;
      size += msg->length;
    }
  }

  if (count >= PRIVATE_FLOOD_RATE || size >= PRIVATE_FLOOD_SIZE)
  {
#ifdef DEBUG
    printf("FLOODED by %s!%s\n", user->nick, userhost);
#endif
    if (!IsIgnored(source))
    {
      sprintf(buffer, "%sFLOOD from %s!%s", type, user->nick, userhost);
      log(buffer);

      sprintf(buffer, "%sFLOOD from %s!%s (%s)", type,
	user->nick, userhost, user->server->name);
      broadcast(buffer, 1);
    }
    MakeBanMask(user, buffer);
    AddIgnore(source, buffer, IGNORE_TIME);
    return 1;
  }

  return 0;
}

void AddIgnore(char *source, char *mask, int t)
{
  char buffer[200];
  char *site;
  aignore *curr;
  int count;

  if (IsIgnored(source))
    return;

  /* count number of ignores from the same site */
  site = strchr(mask, '@') + 1;
  count = 0;
  curr = IgnoreList;
  while (curr)
  {
    if (!mycasecmp(strchr(curr->mask, '@') + 1, site))
      count++;
    curr = curr->next;
  }

  /* add the entry in the list */
  curr = (aignore *) MALLOC(sizeof(aignore));
  curr->time = now + t;
  curr->next = IgnoreList;
  IgnoreList = curr;
  strcpy(curr->mask, mask);

  add_silence(source, curr->mask);
  notice(source, "I don't like being flooded! "
    "I will ignore you from now on.");

  /* schedule ignore removal */
  AddEvent(EVENT_CLEAN_IGNORES, curr->time, "");

  /* If there are already too many ignores from this site
   * ignore the whole site
   */
  if (count >= (MAX_IGNORE_PER_SITE - 1))
  {
    curr = (aignore *) MALLOC(sizeof(aignore));
    curr->time = now + 300;
    curr->next = IgnoreList;
    IgnoreList = curr;

#ifdef DEBUG
    printf("TOO MANY IGNORES FROM THE SAME SITE\n");
#endif
    log("Too many ignores.. ignoring the whole site");
    sprintf(curr->mask, "*!*@%s", site);
    sprintf(buffer, "Ignoring the whole site! (%s)", site);
    broadcast(buffer, 1);
    add_silence(source, curr->mask);
    AddEvent(EVENT_CLEAN_IGNORES, curr->time, "");
  }
}

void CleanIgnores(void)
{
  char buffer[200];
  aignore *curr, *prev;

  prev = NULL;
  curr = IgnoreList;
  while (curr != NULL)
  {
    if (curr->time <= now)
    {
      sprintf(buffer, "Removing ignore for %s", curr->mask);
      log(buffer);
      rem_silence(curr->mask);
      if (prev != NULL)
      {
	prev->next = curr->next;
	TTLALLOCMEM -= sizeof(aignore);
	free(curr);
	curr = prev->next;
      }
      else
      {
	IgnoreList = curr->next;
	TTLALLOCMEM -= sizeof(aignore);
	free(curr);
	curr = IgnoreList;
      }
    }
    else
    {
      prev = curr;
      curr = curr->next;
    }
  }
}

int IsIgnored(char *nick)
{
  char userhost[200];
  register aignore *curr;
  register aluser *user;
  register int found = 0;

  if ((user = ToLuser(nick)) == NULL)
  {
    return 1;
  }

  sprintf(userhost, "%s!%s@%s", user->nick, user->username, user->site);

  curr = IgnoreList;
  while (curr != NULL && !found)
  {
    if (compare(userhost, curr->mask))
    {
      add_silence(nick, curr->mask);
      found++;
    }
    curr = curr->next;
  }

  return (found);
}

void ShowIgnoreList(char *source, char *channel, char *args)
{
  char buffer[200];
  aignore *curr = IgnoreList;
  time_t m;

  if (curr == NULL)
  {
    notice(source, "Ignore list is empty");
  }
  else
  {
    notice(source, "Ignore list:");
    while (curr != NULL)
    {
      m = (curr->time - now) / 60 + 1;
      sprintf(buffer, "%s for %ld minute%s",
	curr->mask, m, (m > 1) ? "s" : "");
      notice(source, buffer);
      curr = curr->next;
    }
  }
}

void add_silence(char *nick, char *mask)
{
  char buffer[200];

  sprintf(buffer, ":%s SILENCE %s :%s\n", mynick, nick, mask);
  sendtoserv(buffer);
#ifdef FAKE_UWORLD
  if (Uworld_status == 1)
  {
    sprintf(buffer, ":%s SILENCE %s :%s\n", UFAKE_NICK, nick, mask);
    sendtoserv(buffer);
  }
#endif
}

void rem_silence(char *mask)
{
  char buffer[200];

  sprintf(buffer, ":%s SILENCE * -%s\n", mynick, mask);
  sendtoserv(buffer);
#ifdef FAKE_UWORLD
  if (Uworld_status == 1)
  {
    sprintf(buffer, ":%s SILENCE * -%s\n", UFAKE_NICK, mask);
    sendtoserv(buffer);
  }
#endif
}


int CheckFloodFlood(char *source, int length)
{
  char buffer[200];
  char userhost[200];
  char global[] = "*";
  register aluser *user;
  register aprivmsg *msg, *prev;
  register int count, size;

  if ((user = ToLuser(source)) == NULL)
  {
    sprintf(buffer, "ERROR! CheckFloodFlood(): %s not found", source);
    log(buffer);
    return 1;
  }

  /* Don't check flood from admins */
  if (IsValid(user, global) && Access(global, source) >= 1)
     return 0;

  if (!strcasecmp(user->username, DEFAULT_USERNAME) &&
    !strcasecmp(user->site, DEFAULT_HOSTNAME))
  {
    return 0;
  }

  /* clean messages older than 30 seconds */
  prev = NULL;
  msg = FloodList;
  while (msg != NULL)
  {
    if (msg->time < (now - 30))
    {
      if (prev)
      {
	prev->next = msg->next;
	TTLALLOCMEM -= sizeof(aprivmsg);
	free(msg);
	msg = prev->next;
      }
      else
      {
	FloodList = msg->next;
	TTLALLOCMEM -= sizeof(aprivmsg);
	free(msg);
	msg = FloodList;
      }
    }
    else
    {
      prev = msg;
      msg = msg->next;
    }
  }

  /* now add the current message in the list */
  sprintf(userhost, "%s@%s", user->username, user->site);
  msg = (aprivmsg *) MALLOC(sizeof(aprivmsg));
  strcpy(msg->user, userhost);
  msg->time = now;
  msg->length = length;
  msg->next = FloodList;
  FloodList = msg;

  /* count how many messages were received from that user
   * and also how many bytes
   */
  count = size = 0;
  for (msg = FloodList; msg != NULL; msg = msg->next)
  {
    if (compare(userhost, msg->user))
    {
      count++;
      size += msg->length;
    }
  }

  if (count >= FLOOD_FLOOD_RATE || size >= FLOOD_FLOOD_SIZE)
  {
    if (!IsIgnored(source))
    {
      sprintf(buffer, "OUTPUT-FLOOD from %s!%s", user->nick, userhost);
      log(buffer);

      sprintf(buffer, "OUTPUT-FLOOD from %s!%s (%s)",
	user->nick, userhost, user->server->name);
      broadcast(buffer, 1);
    }
    MakeBanMask(user, buffer);
    AddIgnore(source, buffer, FLOOD_FLOOD_IGNORE);
    return 1;
  }
  return 0;
}


int CheckAdduserFlood(char *source, char *channel)
{
  char buffer[200];
  char userhost[200];
  char global[] = "*";
  register aluser *user;
  register aprivmsg *msg, *prev;
  register int count;

  if ((user = ToLuser(source)) == NULL)
  {
    sprintf(buffer, "ERROR! CheckAdduserFlood(): %s not found", source);
    log(buffer);
    return 1;
  }

  /* Don't check flood from admins */
  if (IsValid(user, global) && Access(global, source) >= 1)
     return 0;

  if (!strcasecmp(user->username, DEFAULT_USERNAME) &&
    !strcasecmp(user->site, DEFAULT_HOSTNAME))
  {
    return 0;
  }

  /* clean messages older than 1 hour */
  prev = NULL;
  msg = AddUserFloodList;
  while (msg != NULL)
  {
    if (msg->time < (now - 3600))
    {
      if (prev)
      {
	prev->next = msg->next;
	TTLALLOCMEM -= sizeof(aprivmsg);
	free(msg);
	msg = prev->next;
      }
      else
      {
	AddUserFloodList = msg->next;
	TTLALLOCMEM -= sizeof(aprivmsg);
	free(msg);
	msg = AddUserFloodList;
      }
    }
    else
    {
      prev = msg;
      msg = msg->next;
    }
  }

  sprintf(userhost, "%s@%s", user->username, user->site);

  /* now add the current message in the list */
  msg = (aprivmsg *) MALLOC(sizeof(aprivmsg));
  strcpy(msg->user, channel);
  msg->time = now;
  msg->length = 0;
  msg->next = AddUserFloodList;
  AddUserFloodList = msg;

  /* count how many messages were received from that user
   * and also how many bytes
   */
  count = 0;
  for (msg = AddUserFloodList; msg != NULL; msg = msg->next)
  {
    if (!strcmp(channel, msg->user))
    {
      count++;
    }
  }

  if (count >= 20)
  {
    if (!IsIgnored(source))
    {
      sprintf(buffer, "ADDUSER-FLOOD from %s!%s [%s]", user->nick, userhost, channel);
      log(buffer);

      sprintf(buffer, "ADDUSER-FLOOD from %s!%s [%s] (%s)",
	user->nick, userhost, channel, user->server->name);
      broadcast(buffer, 1);
    }
    MakeBanMask(user, buffer);
    AddIgnore(source, buffer, 120);
    return 1;
  }
  return 0;
}


void AdminRemoveIgnore(char *source, char *ch, char *args)
{
  char mask[200], global[] = "*";
  char buffer[200];
  aignore **curr, *tmp;
  int change = 0;

  if (Access(global, source) < XADMIN_LEVEL)
  {
    ReplyNotAccess(source, global);
    return;
  }

  GetWord(0, args, mask);

  if (!*mask)
  {
    notice(source, "SYNTAX: remignore <mask|all>");
    return;
  }

  curr = &IgnoreList;
  while (*curr != NULL)
  {
    if (!strcasecmp((*curr)->mask, mask) || !strcasecmp(mask, "all"))
    {
      change = 1;
      sprintf(buffer, "removed %s", (*curr)->mask);
      notice(source, buffer);
      log(buffer);
      rem_silence((*curr)->mask);

      tmp = *curr;
      *curr = tmp->next;

      free(tmp);
      TTLALLOCMEM -= sizeof(aignore);
    }
    else
    {
      curr = &(*curr)->next;
    }
  }

  if (change)
  {
    sprintf(buffer, "%s removed ignore for [%s]", source, mask);
    broadcast(buffer, 1);
  }
}

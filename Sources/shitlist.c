/* @(#)$Id: shitlist.c,v 1.12 2000/01/28 01:29:14 lgm Exp $ */

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

typedef struct ShitDisk
{
  time_t time;
  time_t expiration;
  char match[80];
  char from[80];
  char reason[200];
  char channel[50];
  int level;
}
ShitDisk;

static int active = 0;

int sl_hash(char *channel)
{
  register int i, j = 0;
  for (i = 1; i < strlen(channel); i++)
    j += (unsigned char)toupper(channel[i]);
  return (j % 1000);
}

void AddToShitList(char *source, char *ch, char *args, int force)
{
  char buffer[1024];
  char srcuh[200];
  char channel[80];
  char pattern[200];
  char strtime[80];
  char strlevel[80];
  char *reason;
  time_t exp;
  int shitlevel;
  int srcAccess;
  int exact;
  register char *ptr1, *ptr2;
  register aluser *luser;
  register ShitUser *curr;
  register achannel *chan;

  if (*args == '#')
  {
    GetWord(0, args, channel);
    args = ToWord(1, args);
  }
  else
  {
    strcpy(channel, ch);
    GuessChannel(source, channel);
  }

  if (*source && force == 0)
  {
    chan = ToChannel(channel);
    if (chan != NULL && (chan->flags & CFL_OPONLY))
    {
      notice(source, replies[RPL_OPONLY][chan->lang]);
      return;
    }
  }

  if (!strcmp(channel, "*"))
  {
    notice(source, "SYNTAX: ban <#channel> <nick|address> "
      "<duration in hours> <level> <reason>");
    return;
  }

  if (!*source || force > 0)
    srcAccess = (MASTER_ACCESS + 1);
  else
    srcAccess = Access(channel, source);

  if (srcAccess < ADD_TO_SHITLIST_LEVEL)
  {
    ReplyNotAccess(source, channel);
    return;
  }

  GetWord(0, args, pattern);
  GetWord(1, args, strtime);
  GetWord(2, args, strlevel);
  reason = ToWord(3, args);

  if (strlen(reason) > 200)
    reason[199] = '\0';

  if (!*pattern || (*strtime != '\0' && !isdigit(*strtime)))
  {
    notice(source, "SYNTAX: ban [#channel] <nick|address> "
      "[duration in hours] [level] [reason]");
    return;
  }

  if (!*strtime)
  {
    exp = SHITLIST_DEFAULT_TIME;
  }
  else
  {
    exp = atoi(strtime);
  }

  if (!*strlevel)
  {
    shitlevel = AUTO_KICK_SHIT_LEVEL;
  }
  else
  {
    shitlevel = atoi(strlevel);
  }

  exp *= 3600;
  if (exp < 0 || exp > (MAX_BAN_DURATION * 24 * 3600))
  {
    sprintf(buffer, "Invalid duration (Max %d days)", MAX_BAN_DURATION);
    notice(source, buffer);
    return;
  }

  if (shitlevel < 1 || shitlevel > 1000)
  {
    notice(source, "Ban level must be in the range 1-1000");
    return;
  }

  if (srcAccess < shitlevel)
  {
    notice(source, "Can't ban to a higher level than your access level.");
    return;
  }

  exp += now;

  if ((luser = ToLuser(source)) != NULL)
  {
    sprintf(srcuh, "%s!%s@%s", luser->nick, luser->username, luser->site);
  }
  else
  {
    strcpy(srcuh, source);
  }

  /* look if the user is on the channel, if so, take his address */

  luser = ToLuser(pattern);
  if (luser != NULL)
  {
    MakeBanMask(luser, pattern);
    exact = 0;
  }
  else
  {
    if ((ptr2 = strchr(pattern, '@')) == NULL)
    {
      strcat(pattern, "@*");
      ptr2 = strchr(pattern, '@');
    }
    if ((ptr1 = strchr(pattern, '!')) == NULL)
    {
      char tmp[200];
      sprintf(tmp, "*!%s", pattern);
      strncpy(pattern, tmp, 200);
      pattern[199] = '\0';
      ptr1 = pattern + 1;
    }
    if (ptr1 > ptr2)
    {
      notice(source, "Illegal ban mask");
      return;
    }
    exact = 1;
  }

  /* count number of bans.. if it's > MAX_BAN.. refuse to add */
  if (!force && *source && shitlevel > 0)
  {
    register int count = 0;
    curr = ShitList[sl_hash(channel)];
    while (curr)
    {
      if (!strcasecmp(curr->channel, channel))
	count++;
      curr = curr->next;
    }
    if (count > MAX_BAN)
    {
      notice(source, "Sorry, there are too many bans on"
	" your channel. You'll have to remove some first.");
      return;
    }
  }

  /* Now, seek thru the ShitList if the pattern is already there.
     if it is, only change the information already present */
  curr = ShitList[sl_hash(channel)];
  if (exact)
  {
    while (curr && (strcasecmp(curr->match, pattern) || strcasecmp(curr->channel, channel)))
    {
      curr = curr->next;
    }
  }
  else
  {
    while (curr && (!match(pattern, curr->match) || strcasecmp(curr->channel, channel)))
    {
      curr = curr->next;
    }
  }

  if (curr)
  {
    /* if the user is already on the shitlist.. */

    /* if this is the result of a flood protection.. we have to
       make sure the user is not already shitlisted for a longer
       time.. */

    if (shitlevel == 0 || curr->expiration < exp)
      curr->expiration = exp;

    curr->time = now;

    /* if this is the result of a flood protection.. we have to
       make sure the user is not already shitlisted at a higher
       level.. */

    if (shitlevel == 0 || curr->level < shitlevel)
      curr->level = shitlevel;

    TTLALLOCMEM -= strlen(curr->from) + 1;
    free(curr->from);
    curr->from = (char *)MALLOC(strlen(srcuh) + 1);
    strcpy(curr->from, srcuh);

    TTLALLOCMEM -= strlen(curr->reason) + 1;
    free(curr->reason);
    curr->reason = (char *)MALLOC(strlen(reason) + 1);
    strcpy(curr->reason, reason);
  }
  else
  {
#ifdef DEBUG
    printf("TIME: %ld EXP: %ld LEVEL %d\n", now, exp, shitlevel);
#endif
    /* if the user is NOT already on the shitlist */
    /* first, create a new structure */

    curr = (ShitUser *) MALLOC(sizeof(ShitUser));

    curr->time = now;
    curr->expiration = exp;
    curr->level = shitlevel;

    curr->match = (char *)MALLOC(strlen(pattern) + 1);
    strcpy(curr->match, pattern);

    curr->from = (char *)MALLOC(strlen(srcuh) + 1);
    strcpy(curr->from, srcuh);

    curr->reason = (char *)MALLOC(strlen(reason) + 1);
    strcpy(curr->reason, reason);

    curr->channel = (char *)MALLOC(strlen(channel) + 1);
    strcpy(curr->channel, channel);

    /* Then, link it to the list.. */

    curr->next = ShitList[sl_hash(channel)];
    ShitList[sl_hash(channel)] = curr;

  }

  /* schedule the removal of the entry..
   */
  AddEvent(EVENT_CLEANSHITLIST, exp, channel);


  if (*source && !force)
    notice(source, "Ban list updated");

  if (shitlevel >= AUTO_KICK_SHIT_LEVEL && !force)
  {
#ifdef DEBUG
    printf("Calling mban(\"\",%s,%s)\n", channel, pattern);
#endif
    mban("", channel, pattern);

    sprintf(buffer, "%s (%s) %s", pattern, source, reason);
    kick("", channel, buffer);
  }

  /* Now, clean the shitlist */
  if (!force)
    CleanShitList("", channel);
}

void RemShitList(char *source, char *ch, char *args, int force)
{
  char channel[80];
  char pattern[2][200];
  register aluser *luser;
  register ShitUser *curr;
  register achannel *chan;
  int srcaccess, exact;

  if (*args == '#')
  {
    GetWord(0, args, channel);
    args = ToWord(1, args);
  }
  else
  {
    strcpy(channel, ch);
    GuessChannel(source, channel);
  }

  if (*source && force == 0)
  {
    chan = ToChannel(channel);
    if (chan != NULL && (chan->flags & CFL_OPONLY))
    {
      notice(source, replies[RPL_OPONLY][chan->lang]);
      return;
    }
  }

  if (!strcmp(channel, "*"))
  {
    notice(source, "SYNTAX: unban <#channel> <nick|address>");
    return;
  }

  if (!*source || force > 0)
    srcaccess = (MASTER_ACCESS + 1);
  else
    srcaccess = Access(channel, source);

  if (srcaccess < ADD_TO_SHITLIST_LEVEL)
  {
    ReplyNotAccess(source, channel);
    return;
  }

  GetWord(0, args, pattern[0]);

  if (!*pattern[0])
  {
    notice(source, "SYNTAX: unban [#channel] <nick|address>");
    return;
  }

  /* look if the user is on the channel, if so, take his address */

  luser = ToLuser(pattern[0]);
  if (luser != NULL)
  {
    sprintf(pattern[1], "%s!%s@%s",
      luser->nick, luser->username, luser->site);
    exact = 0;
  }
  else
  {
    exact = 1;
  }

  /* Now, seek thru the ShitList if the pattern is already there. */

  curr = ShitList[sl_hash(channel)];
  if (exact)
  {
    while (curr && (strcasecmp(curr->match, pattern[0]) || strcasecmp(curr->channel, channel) || srcaccess < curr->level))
    {
      curr = curr->next;
    }
  }
  else
  {
    while (curr && (!match(pattern[1], curr->match) || strcasecmp(curr->channel, channel) || srcaccess < curr->level))
    {
      curr = curr->next;
    }
  }

  if (curr)
  {
    curr->expiration = now - 1;
    if (*source && !force)
      notice(source, "Ban list updated");
    CleanShitList("", channel);
  }
  else
  {
    unban("", channel, pattern[0]);
  }
}


void CleanShitList(char *source, char *channel)
{
  char buffer[200];
  register ShitUser *curr, *prev;
  register achannel *chan;
  int i;

  if (*source && Access(channel, source) < CLEAN_SHITLIST_LEVEL)
  {
    notice(source, "Your admin Access is too low");
    return;
  }

  i = sl_hash(channel);
  curr = ShitList[i];
  prev = NULL;

  while (curr)
  {
    chan = ToChannel(curr->channel);
    if (chan == NULL || (curr->expiration <= now && !strcasecmp(channel, chan->name))
      || curr->level == 0)
    {
      if (chan != NULL && chan->on && chan->AmChanOp)
      {
	unban("", curr->channel, curr->match);
      }
      if (prev)
      {
	prev->next = curr->next;
	TTLALLOCMEM -= strlen(curr->match) + 1;
	free(curr->match);
	TTLALLOCMEM -= strlen(curr->from) + 1;
	free(curr->from);
	TTLALLOCMEM -= strlen(curr->reason) + 1;
	free(curr->reason);
	TTLALLOCMEM -= strlen(curr->channel) + 1;
	free(curr->channel);
	TTLALLOCMEM -= sizeof(ShitUser);
	free(curr);
	curr = prev->next;
      }
      else
      {
	ShitList[i] = curr->next;
	TTLALLOCMEM -= strlen(curr->match) + 1;
	free(curr->match);
	TTLALLOCMEM -= strlen(curr->from) + 1;
	free(curr->from);
	TTLALLOCMEM -= strlen(curr->reason) + 1;
	free(curr->reason);
	TTLALLOCMEM -= strlen(curr->channel) + 1;
	free(curr->channel);
	TTLALLOCMEM -= sizeof(ShitUser);
	free(curr);
	curr = ShitList[i];
      }
    }
    else
    {
      prev = curr;
      curr = curr->next;
    }
  }

  if (*source)
    notice(source, "Ban list is now up-to-date");

  sprintf(buffer, "Cleaned banlist on %s", channel);
  log(buffer);
}

int IsShit(char *channel, char *user, char *out, char *reason)
{
  register ShitUser *curr;
  register aluser *luser;
  char uh[200];

  if (strchr(user, '!') != NULL)
  {
    strcpy(uh, user);
  }
  else
  {
    luser = ToLuser(user);
    sprintf(uh, "%s!%s@%s", luser->nick, luser->username, luser->site);
  }

  curr = ShitList[sl_hash(channel)];

#ifdef DEBUG
  printf("IsShit(%s,%s,%s)\n", channel, user, out);
#endif

  while (curr && (!match(channel, curr->channel) || !match(uh, curr->match)))
    curr = curr->next;

#ifdef DEBUG
  if (curr)
    printf("Banlevel: %d\n", curr->level);
#endif

  if (curr)
  {
    if (out != NULL)
      strcpy(out, curr->match);
    if (reason != NULL)
      strcpy(reason, curr->reason);
    return (curr->level);
  }
  else
  {
    return 0;
  }
}

void ShowShitList(char *source, char *ch, char *args)
{
  register ShitUser *curr;
  struct tm *tp;
  char buffer[1024], global[] = "*";
  char channel[80];
  int found = 0;

  if (*args == '#')
  {
    GetWord(0, args, channel);
    args = ToWord(1, args);
  }
  else
  {
    strcpy(channel, ch);
    GuessChannel(source, channel);
  }

  if (!strcmp(channel, "*") || !*args)
  {
    notice(source, "SYNTAX: lbanlist [#channel] <search pattern>");
    return;
  }

  if (!ToUser(channel, source) &&
    Access(channel, source) < 500 && Access(global, source) < 500)
  {
    notice(source, "You are not on that channel");
    return;
  }


  curr = ShitList[sl_hash(channel)];
  while (curr)
  {
    if (!strcasecmp(channel, curr->channel) && match(curr->match, args))
    {
      found++;
    }
    curr = curr->next;
  }
  if ((found > 15) && (source[0] != '+'))
  {
    sprintf(buffer, "There are %d matching entries. Please use a userhost mask to narrow down the list.", found);
    notice(source, buffer);
    return;
  }

  if (found == 0)
  {
    sprintf(buffer, "*** No entry matching with %s ***", args);
    notice(source, buffer);
    return;
  }


  sprintf(buffer, "*** Ban List for channel %s ***", channel);
  notice(source, buffer);

  curr = ShitList[sl_hash(channel)];

  while (curr)
  {
    if (!strcasecmp(channel, curr->channel) && match(curr->match, args))
    {
      sprintf(buffer, "%s %s Level: %d", curr->channel,
	curr->match, curr->level);
      notice(source, buffer);

      sprintf(buffer, "ADDED BY: %s (%s)", curr->from,
	(*curr->reason) ? curr->reason : "No reason given");
      notice(source, buffer);

      tp = gmtime(&curr->time);
      sprintf(buffer, "SINCE: %sUCT", asctime(tp));
      *strchr(buffer, '\n') = ' ';
      notice(source, buffer);

      sprintf(buffer, "EXP: %s", time_remaining(curr->expiration - now));
      notice(source, buffer);
    }
    curr = curr->next;
  }

  notice(source, "*** END ***");
}

void SaveShitList(char *source, char *channel)
{
  ShitDisk tmp;
  register ShitUser *user;
  register int file;
  char buffer[200];
  int i;

  if (*source && Access(channel, source) < SAVE_SHITLIST_LEVEL)
  {
    notice(source, "Your admin Access is too low!");
    return;
  }

  if (active)
    return;
  active = 1;

  alarm(5);	/* avoid NFS hangs */
  file = open(SHITLIST_FILE ".new", O_WRONLY | O_CREAT | O_TRUNC, 0600);
  alarm(0);

  if (file < 0)
  {
    if (*source)
      notice(source, "Error opening BanList file! Aborted.");
    log("Error saving shitlist");
    active = 0;
    return;
  }

  sprintf(buffer, ":%s AWAY :Busy saving precious ban list\n", mynick);
  sendtoserv(buffer);
  dumpbuff();

  for (i = 0; i < 1000; i++)
  {
    user = ShitList[i];
    while (user)
    {
      tmp.time = user->time;
      tmp.expiration = user->expiration;
      strncpy(tmp.match, user->match, 79);
      tmp.match[79] = '\0';
      strncpy(tmp.from, user->from, 79);
      tmp.from[79] = '\0';
      strncpy(tmp.reason, user->reason, 199);
      tmp.reason[199] = '\0';
      strncpy(tmp.channel, user->channel, 49);
      tmp.channel[49] = '\0';
      tmp.level = user->level;

      alarm(2);
      if (write(file, &tmp, sizeof(ShitDisk)) <= 0)
      {
	alarm(0);
	close(file);
	log("ERROR: Can't save banlist");
	log((char *)sys_errlist[errno]);
	alarm(2);
	remove(SHITLIST_FILE ".new");
	alarm(0);
	active = 0;
	sprintf(buffer, ":%s AWAY\n", mynick);
	sendtoserv(buffer);
	return;
      }
      alarm(0);

      user = user->next;
    }
  }

  close(file);
  alarm(20);
  rename(SHITLIST_FILE ".new", SHITLIST_FILE);
  alarm(0);
  if (*source)
    notice(source, "banlist saved.");
  active = 0;
  sprintf(buffer, ":%s AWAY\n", mynick);
  sendtoserv(buffer);
}

void LoadShitList(char *source)
{
  ShitDisk tmp;
  ShitUser *user;
  int file;
  int i;

  if (*source && Access("*", source) < LOAD_SHITLIST_LEVEL)
  {
    notice(source, "Your admin access is too low!");
    return;
  }

  if (active)
    return;
  active = 1;

  file = open(SHITLIST_FILE, O_RDONLY);
  if (file < 0)
  {
    if (*source)
      notice(source, "Error opening BanList file! Aborted.");
    log("ERROR loading banlist");
    active = 0;
    return;
  }

  /* empty existing shitlist */
  for (i = 0; i < 1000; i++)
  {
    while ((user = ShitList[i]) != NULL)
    {
      ShitList[i] = ShitList[i]->next;
      TTLALLOCMEM -= strlen(user->match) + 1;
      free(user->match);
      TTLALLOCMEM -= strlen(user->from) + 1;
      free(user->from);
      TTLALLOCMEM -= strlen(user->reason) + 1;
      free(user->reason);
      TTLALLOCMEM -= strlen(user->channel) + 1;
      free(user->channel);
      TTLALLOCMEM -= sizeof(ShitUser);
      free(user);
    }
  }

  while (read(file, &tmp, sizeof(ShitDisk)) > 0)
  {
    if (tmp.expiration < now)
      continue;
    user = (ShitUser *) MALLOC(sizeof(ShitUser));
    user->time = tmp.time;
    user->expiration = tmp.expiration;
    if (tmp.level > 500)
      tmp.level = 500;
    user->level = tmp.level;
    user->match = (char *)MALLOC(strlen(tmp.match) + 1);
    strcpy(user->match, tmp.match);
    user->from = (char *)MALLOC(strlen(tmp.from) + 1);
    strcpy(user->from, tmp.from);
    user->reason = (char *)MALLOC(strlen(tmp.reason) + 1);
    strcpy(user->reason, tmp.reason);
    user->channel = (char *)MALLOC(strlen(tmp.channel) + 1);
    strcpy(user->channel, tmp.channel);

    user->next = ShitList[sl_hash(tmp.channel)];
    ShitList[sl_hash(tmp.channel)] = user;
  }

  close(file);

  if (*source)
    notice(source, "BanList loaded!");

  active = 0;
}

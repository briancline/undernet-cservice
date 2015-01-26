/* @(#)$Id: bans.c,v 1.12 1999/04/04 17:00:03 seks Exp $ */

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

void ban(char *source, char *chan, char *nicklist)
{
  char buffer[300];
  char OneNick[NICK_LENGTH];
  char channel[CHANNELNAME_LENGTH];
  register auser *user;
  register aluser *luser;
  register achannel *ch;
  register int i = 0;

  if (*nicklist == '#')
  {
    GetWord(0, nicklist, channel);
    nicklist = ToWord(1, nicklist);
  }
  else
  {
    strcpy(channel, chan);
    GuessChannel(source, channel);
  }

  if (!strcmp(channel, "*"))
  {
    notice(source, "SYNTAX: ban <channel> <nick1|addr1> [nick2|addr2] [...]");
    return;
  }

#ifdef DEBUG
  printf("BAN REQUEST FOR %s\nON CHANNEL %s\nBY %s (%d)\n",
    nicklist, channel, source, Access(channel, source));
#endif

  if (*source && Access(channel, source) < BAN_LEVEL)
  {
    notice(source, "Your Access on this channel is too low");
    return;
  }

  ch = ToChannel(channel);

  /* I'm not on this channel.. so screw it! */
  if (ch == NULL || !ch->on)
  {
    notice(source, "I'm NOT on that channel!");
    return;
  }

  if (!ch->AmChanOp)
  {
    notice(source, "I'm not channel operator!");
    return;
  }

  if (!*nicklist)
  {
    notice(source, "SYNTAX: ban <channel> <nick1|addr1> [nick2|addr2] [...]");
    return;
  }

  GetWord(0, nicklist, OneNick);

  while (*OneNick)
  {
    luser = ToLuser(OneNick);
    if (luser)
      sprintf(buffer, "%s!%s@%s", luser->nick,
	luser->username, luser->site);
    if (luser)
    {
      sprintf(buffer, "I BAN %s!%s@%s ON %s", luser->nick,
	luser->username, luser->site, channel);
      log(buffer);

      user = ToUser(channel, OneNick);
      if (user && user->chanop)
      {
	changemode(channel, "-o", user->N->nick, 0);
      }

      MakeBanMask(luser, buffer);
      changemode(channel, "+b", buffer, 0);
    }
    GetWord(++i, nicklist, OneNick);
  }
  flushmode(channel);
}

void mban(char *source, char *ch, char *args)
{
  char buffer[200];
  char channel[80];
  register int found = 0;
  register achannel *chan;
  register auser *user;

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

  if (!strcmp(channel, "*"))
  {
    notice(source, "SYNTAX: mban <channel> <nick!username@hostname>");
    return;
  }

  if ((chan = ToChannel(channel)) == NULL || !chan->on)
  {
    if (*source)
      notice(source, "I'm NOT on that channel!");
    return;
  }

  if (!chan->AmChanOp)
  {
    if (*source)
      notice(source, "I am NOT channel operator!");
    return;
  }

  if (*source && Access(channel, source) < MASS_BAN_LEVEL)
  {
    notice(source, "Your Access on this channel is too low!");
    return;
  }

  if (!*args)
  {
    notice(source, "SYNTAX: mban <channel> <nick!username@hostname>");
    return;
  }

  user = chan->users;

  while (user)
  {
    sprintf(buffer, "%s!%s@%s",
      user->N->nick, user->N->username, user->N->site);
    if (match(buffer, args))
    {
      sprintf(buffer, "I BAN %s!%s@%s on %s (%s)", user->N->nick,
	user->N->username, user->N->site, channel, args);
      log(buffer);

      if (user->chanop)
      {
	changemode(channel, "-o", user->N->nick, 0);
	user->chanop = 0;
      }

      /*MakeBanMask(user->N,buffer); */
      if (!found)
	changemode(channel, "+b", args, 0);
      found = 1;
    }
    user = user->next;
  }
  if (found)
    flushmode(channel);
  else if (*source)
  {
    notice(source, "No match.");
  }
}

void unban(char *source, char *ch, char *list)
{
  register aban *bans;
  register aluser *luser;
  register achannel *chan;
  char channel[CHANNELNAME_LENGTH];
  char buffer[512];
  char one[200];
  register int found;
  register int i, exact;

  if (*list == '#')
  {
    GetWord(0, list, channel);
    list = ToWord(1, list);
  }
  else
  {
    strcpy(channel, ch);
    GuessChannel(source, channel);
  }

  chan = ToChannel(channel);
  if (chan == NULL || !chan->on)
  {
    notice(source, "I am NOT on that channel!");
    return;
  }

  if (!chan->AmChanOp)
  {
    notice(source, "I am not channel operator!");
    return;
  }

  if (*source && Access(channel, source) < BAN_LEVEL)
  {
    notice(source, "You're Access on this channel is too low");
    return;
  }

  if (!*list)
  {
    notice(source, "SYNTAX: unban [#channel] <nick1|addr1> [<nick2|addr2>] [...]");
    return;
  }

  i = 0;
  GetWord(0, list, one);
  while (*one)
  {
    found = 0;
    luser = ToLuser(one);
    if (luser != NULL)
    {
      sprintf(one, "%s!%s@%s", luser->nick, luser->username,
	luser->site);
      exact = 0;
    }
    else
    {
      exact = 1;
    }
    bans = chan->bans;
    while (bans != NULL)
    {
      if ((!exact && match(one, bans->pattern)) ||
	(exact && !strcasecmp(one, bans->pattern)))
      {
	sprintf(buffer, "I UNBAN %s ON %s",
	  bans->pattern, chan->name);
	log(buffer);
	changemode(channel, "-b", bans->pattern, 0);
	RemBan(channel, bans->pattern);
	bans = chan->bans;
	found = 1;
	if (exact)
	  break;
      }
      else
	bans = bans->next;
    }
    if (*source && !found)
    {
      sprintf(buffer, "%s is not in %s's banlist!",
	one, channel);
      notice(source, buffer);
    }
    GetWord(++i, list, one);
  }
  flushmode(channel);
}


void showbanlist(char *source, char *ch, char *args)
{
  char buffer[200];
  char channel[80];
  register achannel *chan;
  register aban *curr;

  if (*args == '#')
  {
    GetWord(0, args, channel);
  }
  else
  {
    strcpy(channel, ch);
    GuessChannel(source, channel);
  }

  if (!strcmp(channel, "*"))
  {
    notice(source, "SYNTAX: banlist <channel>");
    return;
  }

  /* user must be on a channel to see the ban list
   */
  if (ToUser(channel, source) == NULL)
  {
    sprintf(buffer, "%s: You are not on that channel", channel);
    notice(source, buffer);
    return;
  }

  chan = ToChannel(channel);
  curr = chan->bans;
  if (curr == NULL)
  {
    sprintf(buffer, "%s: ban list is empty.", channel);
    notice(source, buffer);
    return;
  }

  while (curr != NULL)
  {
    notice(source, curr->pattern);
    curr = curr->next;
  }

  sprintf(buffer, "%s: End of ban list", channel);
  notice(source, buffer);
}

void MakeBanMask(aluser * luser, char *output)
{
  register int isip = 1;
  register int i, j;
  int a1, a2, a3, a4;
  char hostmask[200];
  register char *ptr;

  if (luser == NULL)
  {
    /* Don't do anything */
    return;
  }

  /* check if hostname is a numeric IP address
   */
  for (i = 0; luser->site[i] != '\0' && isip; i++)
  {
    if (!isdigit(luser->site[i]) && luser->site[i] != '.')
      isip = 0;
  }

  if (isip)
  {
    sscanf(luser->site, "%d.%d.%d.%d", &a1, &a2, &a3, &a4);

    if (a1 <= 127)
    {	/* class A */
      /*sprintf(hostmask,"%d.*",a1); *shrugs* */
      sprintf(hostmask, "%d.%d.*", a1, a2);
    }
    else if (a1 <= 191)
    {	/* class B */
      sprintf(hostmask, "%d.%d.*", a1, a2);
    }
    else
    {	/* class C */
      sprintf(hostmask, "%d.%d.%d.*", a1, a2, a3);
    }
  }
  else
  {	/* not numeric address */
    ptr = luser->site + strlen(luser->site);
    i = 0;

    if (!strcasecmp(luser->site + strlen(luser->site) - 3, ".AU") ||
      !strncasecmp(luser->site + strlen(luser->site) - 7, ".NET.", 4) ||
      !strncasecmp(luser->site + strlen(luser->site) - 7, ".COM.", 4) ||
      !strncasecmp(luser->site + strlen(luser->site) - 7, ".EDU.", 4) ||
      !strncasecmp(luser->site + strlen(luser->site) - 6, ".AC.", 3))
      j = 3;
    else
      j = 2;

    while (i != j && ptr != luser->site)
    {
      if (*ptr == '.')
	i++;
      ptr--;
    }
    if (i == j)
      ptr += 2;

    if (ptr == luser->site)
      strcpy(hostmask, ptr);
    else
      sprintf(hostmask, "*.%s", ptr);
  }

  if (!strncasecmp(luser->username, "^wld", 4))
  {
    /* special case for telnet users */
    sprintf(output, "*!^wld*@%s", hostmask);
  }
  else
  {
    ptr = luser->username;
    while (strlen(ptr) == 10 || *ptr == '~')
      ptr++;

    sprintf(output, "*!*%s@%s", ptr, hostmask);
  }
}

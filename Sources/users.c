/* @(#)$Id: users.c,v 1.7 1998/11/21 14:58:43 seks Exp $ */

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

int lu_hash(char *nick)
{
  register int i, j;

  for (i = 0, j = 0; i < strlen(nick); i++)
    j += toupper((unsigned char)nick[i]);

  return (j % 1000);
}

int su_hash(char *nick)
{
  register int i, j;

  for (i = 0, j = 0; i < strlen(nick); i++)
    j += toupper((unsigned char)nick[i]);

  return (j % 100);
}

aluser *ToLuser(char *nick)
{
  register aluser *curr;
  curr = Lusers[lu_hash(nick)];
  while (curr && strcasecmp(nick, curr->nick))
    curr = curr->next;

  return (curr);
}

void onnick(char *source, char *newnick, char *body)
{
  register aluser *user, **u;
  register asuser *suser, **s;
  register aserver *serv;
  char username[80];
  char hostname[80];
  char TS[80];
  char server[80];
  register achannelnode *chan;
  register anickchange *curr, *prec;
  char buffer[512];
  int i = 0;


#ifdef DEBUG
  printf("NICK: %s --> %s ...\n", source, newnick);
#endif

  /* a new user */
  if (!ToLuser(source))
  {	/* Not a user, so a server or nothing */
    if (strchr(source, '.') == NULL)
    {
      /* Source is not a user and not a server either */
      return;
    }

    if (!strcasecmp(newnick, mynick))
    {
      log("ERROR: I'm nick collided");
#ifdef DEBUG
      printf("ARGH!!! I'M NICK COLLIDED!\n");
#endif
      GetWord(1, body, TS);
      GetWord(2, body, username);
      GetWord(3, body, hostname);

      if (atol(TS) <= logTS &&
	strcasecmp(username, myuser) &&
	strcasecmp(hostname, mysite))
      {
	NickInUse();
	log(source);
	log(newnick);
	log(body);
      }
      else
      {
	onquit(source);
	return;		/*ignore */
      }
#ifdef BACKUP
    }
    else if (!strcasecmp(newnick, MAIN_NICK))
    {
      return;	/* ignore */
#endif
    }
    else if (ToLuser(newnick))
    {
#ifdef DEBUG
      printf("ARGH!!! NICK COLLISION\n");
#endif
      onquit(newnick);
    }
    GetWord(1, body, TS);
    GetWord(2, body, username);
    GetWord(3, body, hostname);
    GetWord(4, body, server);

#ifdef FAKE_UWORLD
    if (Uworld_status == 1 && !strcasecmp(newnick, UFAKE_NICK))
    {
      if (atol(TS) <= UworldTS && atol(TS) != 0 &&
	strcasecmp(username, UFAKE_NICK) &&
	strcasecmp(hostname, UFAKE_HOST))
      {
	sprintf(buffer, "%s nick collided", UFAKE_NICK);
	log(buffer);
	Uworld_status = 0;
	KillUworld("nick collision");
	return;		/* ignore if younger */
      }
    }
#endif

    user = (aluser *) MALLOC(sizeof(aluser));

    user->nick = (char *)MALLOC(strlen(newnick) + 1);
    strcpy(user->nick, newnick);

    user->username = (char *)MALLOC(strlen(username) + 1);
    strcpy(user->username, username);

    user->site = (char *)MALLOC(strlen(hostname) + 1);
    strcpy(user->site, hostname);

    if (*newnick == '+')
      serv = &VirtualServer;
    else
      serv = ToServer(server);

    user->server = serv;

    user->time = atol(TS);
    user->mode = 0;

    user->channel = NULL;
    user->valchan = NULL;

    user->next = Lusers[lu_hash(newnick)];
    Lusers[lu_hash(newnick)] = user;

    /* add user in server's userlist
     */
    suser = (asuser *) MALLOC(sizeof(asuser));
    suser->N = user;
    suser->next = serv->users[su_hash(newnick)];
    serv->users[su_hash(newnick)] = suser;

#ifdef NICKSERV
    nserv_nick(newnick, user);
#endif
  }
  else
  {	/* nick change */

#if 0
    if (!strcasecmp(source, DEFAULT_NICKNAME) &&
      strcasecmp(newnick, DEFAULT_NICKNAME))
    {
      ChNick(DEFAULT_NICKNAME);
    }
#endif
    if (!strcasecmp(newnick, mynick))
    {
#ifdef DEBUG
      printf("ARGH!!! I'M NICK COLLIDED!\n");
#endif
      GetWord(0, body, TS);
      if (atol(TS + 1) <= logTS)
      {
	NickInUse();
	log(source);
	log(newnick);
	log(body);
      }
      else
      {
	onquit(source);
	return;		/*ignore */
      }
    }

    u = &Lusers[lu_hash(source)];
    while (*u && strcasecmp(source, (*u)->nick))
      u = &(*u)->next;
    user = *u;

#ifdef NICKSERV
    nserv_nick(newnick, user);
#endif

    if (user == NULL)
      quit("ERROR! onnick() can't find user", 1);

    s = &user->server->users[su_hash(source)];
    while (*s && strcasecmp((*s)->N->nick, user->nick))
      s = &(*s)->next;
    suser = *s;

    /* change the nick in memory */

    TTLALLOCMEM -= strlen(user->nick) + 1;
    free(user->nick);
    user->nick = (char *)MALLOC(strlen(newnick) + 1);
    strcpy(user->nick, newnick);

    /* now relocate the structure */
    *u = user->next;
    user->next = Lusers[lu_hash(newnick)];
    Lusers[lu_hash(newnick)] = user;

    *s = suser->next;
    suser->next = user->server->users[su_hash(newnick)];
    user->server->users[su_hash(newnick)] = suser;

    /* NICK FLOOD PROTECTION */
    /* 1st wipe old nick changes off */
    chan = user->channel;
    while (chan)
    {
      curr = chan->nickhist;
      prec = NULL;

      /* if not on channel.. ignore nick flood pro */
      if (!chan->N->on)
      {
	chan = chan->next;
	continue;	/* yurk.. as bad as a goto ;) */
      }

      while (curr)
      {
	if (curr->time < (now - 15))
	{
	  if (prec)
	  {
	    prec->next = curr->next;
	    TTLALLOCMEM -= sizeof(anickchange);
	    free(curr);
	    curr = prec->next;
	  }
	  else
	  {
	    chan->nickhist = curr->next;
	    TTLALLOCMEM -= sizeof(anickchange);
	    free(curr);
	    curr = chan->nickhist;
	  }
	}
	else
	{
	  prec = curr;
	  curr = curr->next;
	}
      }

      /* now add the new nick change to the history */
      curr = (anickchange *) MALLOC(sizeof(anickchange));
      strcpy(curr->nick, source);
      curr->time = now;		/* a lil confusing :( */
      curr->next = chan->nickhist;
      chan->nickhist = curr;

      /* now count the nick changes in history
         if there are more than allowed.. grrrr */
      for (i = 0, curr = chan->nickhist; curr;
	curr = curr->next, i++);

      if (i == chan->N->NickFloodPro && chan->N->NickFloodPro != 0
	&& chan->N->on)
      {
	sprintf(buffer, "%s!%s@%s", user->nick, user->username, user->site);
	notice(newnick,
	  "### NICK FLOOD PROTECTION ACTIVATED ###");
	sprintf(buffer, "%s %d", newnick,
	  NICK_FLOOD_SUSPEND_TIME);
	suspend("", chan->N->name, buffer);
	ban("", chan->N->name, newnick);
      }
      chan = chan->next;
    }
  }
}

void onquit(char *nick)
{
  register aluser *user, **u;
  register asuser *suser, **s;
  register avalchan *valchan;

#ifdef DEBUG
  printf("Detected user quit..\n");
#endif
  u = &Lusers[lu_hash(nick)];
  while (*u && strcasecmp(nick, (*u)->nick))
    u = &(*u)->next;

  user = *u;

  if (user == NULL)
  {
    log("ERROR: onquit() can't find user!");
#ifdef HISTORY
    History(NULL);
#endif
    return;
  }

  /* remove from memory */
  while ((valchan = user->valchan) != NULL)
  {
#ifdef DEBUG
    printf("\twas validated on %s\n", valchan->name);
#endif
    valchan->reg->inuse--;
    user->valchan = valchan->next;
    TTLALLOCMEM -= strlen(valchan->name) + 1;
    free(valchan->name);
    TTLALLOCMEM -= sizeof(avalchan);
    free(valchan);
  }
  while (user->channel != NULL)
  {
#ifdef DEBUG
    printf("\twas on %s\n", user->channel->N->name);
#endif
    /* onpart() free's the chan structure
     * we can't do chan=chan->next after the 
     * onpart() call. We must start from the 
     * beginning of the list every time
     */
    onpart(nick, user->channel->N->name);
  }

  /* remove user from server's userlist
   */
  s = &user->server->users[su_hash(user->nick)];
  while (*s != NULL && strcasecmp((*s)->N->nick, user->nick))
  {
    s = &(*s)->next;
  }

  if (*s != NULL)
  {
    suser = *s;
    *s = (*s)->next;
    TTLALLOCMEM -= sizeof(asuser);
    free(suser);
  }
  else
  {
    log("ERROR: onquit()  user not found in server's userlist!");
  }

  *u = user->next;
#if 0
  if (!strcasecmp(user->nick, DEFAULT_NICKNAME))
  {
    ChNick(DEFAULT_NICKNAME);
  }
#endif

#ifdef NICKSERV
  nserv_quit(user);
#endif

  TTLALLOCMEM -= strlen(user->nick) + 1;
  free(user->nick);
  TTLALLOCMEM -= strlen(user->username) + 1;
  free(user->username);
  TTLALLOCMEM -= strlen(user->site) + 1;
  free(user->site);
  TTLALLOCMEM -= sizeof(aluser);
  free(user);
}

void onkill(char *source, char *target, char *comment)
{
  char buffer[200];

  if (!strcasecmp(target, mynick))
  {
#if 0
    /* ignore kill for nick collisions because we
     * already check in onnick() if we're collided.
     * This kill is prolly a lost  kill resulting of
     * another nick collision..
     */
    if (strstr(comment, "older nick overruled") ||
      strstr(comment, "collided yourself"))
    {
      log("ERROR: Nick collision on me?");
      return;
    }
#endif
    sprintf(buffer, ":%s SQUIT %s 0 :killed by %s\n",
      SERVERNAME, SERVERNAME, source);
    sendtoserv(buffer);
    dumpbuff();
    close(Irc.fd);
    Irc.fd = -1;
    if (reconnect(server))
    {
      try_later(server);
    }
#ifdef BACKUP
  }
  else if (!strcasecmp(target, MAIN_NICK))
  {
    quit(MAIN_NICK " is back", 0);
#endif
#ifdef FAKE_UWORLD
  }
  else if (!strcasecmp(target, UFAKE_NICK))
  {
    char buffer[200];
    sprintf(buffer, "%s is KILLED by %s", UFAKE_NICK, source);
    log(buffer);
    Uworld_status = 0;
    KillUworld("Killed");
#endif
  }
  else
    onquit(target);
}

void onwhois(char *source, char *nick)
{
  register aluser *user;
  register auser *usr;
  register achannelnode *chan;
  char buffer[512];

  user = ToLuser(nick);

  if (user == NULL)
  {
    sprintf(buffer, ":%s 401 %s %s :No such nick\n", SERVERNAME, source, nick);
    sendtoserv(buffer);
  }
  else
  {
    sprintf(buffer, ":%s 311 %s %s %s %s * :\n", SERVERNAME, source, user->nick,
      user->username, user->site);
    sendtoserv(buffer);

    chan = user->channel;
    if (chan != NULL && strcmp(user->nick, "X") && strcmp(user->nick, "W"))
    {
      sprintf(buffer, ":%s 319 %s %s :", SERVERNAME, source, nick);
      while (chan != NULL)
      {
	/* show a channel only if it is
	 * not +s or +p
	 */
	if (!IsSet(chan->N->name, 's', "") &&
	  !IsSet(chan->N->name, 'p', ""))
	{
	  usr = ToUser(chan->N->name, nick);
	  if (usr->chanop)
	    strcat(buffer, "@");
	  strcat(buffer, chan->N->name);
	  strcat(buffer, " ");
	}
	chan = chan->next;
	if (strlen(buffer) > 300)
	{
	  strcat(buffer, "\n");
	  sendtoserv(buffer);
	  sprintf(buffer, ":%s 319 %s %s :",
	    SERVERNAME, source, nick);
	}
      }
      strcat(buffer, "\n");
      sendtoserv(buffer);
    }
    sprintf(buffer, ":%s 312 %s %s %s :\n", SERVERNAME, source, source, user->server->name);
    sendtoserv(buffer);

    if (user->mode & LFL_ISOPER)
    {
      sprintf(buffer, ":%s 313 %s %s :is an IRC Operator\n",
	SERVERNAME, source, user->nick);
      sendtoserv(buffer);
    }
  }

  sprintf(buffer, ":%s 318 %s :End of /WHOIS list.\n", SERVERNAME, source);
  sendtoserv(buffer);
}

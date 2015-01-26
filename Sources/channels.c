/* @(#)$Id: channels.c,v 1.29 2000/02/26 15:59:30 seks Exp $ */

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

int cl_hash(char *channel)
{
  register int i, j;

  for (i = j = 0; i < strlen(channel); i++)
    j += (unsigned char)toupper(channel[i]);
  return (j % 1000);
}

void NewChannel(char *channelname, time_t TS, int on)
{
  register achannel *chan;
  register adefchan *defs;
  char buffer[200];

  if (!channelname || !*channelname)
  {
    log("ERROR: NewChannel(): empty channel string!!!");
    log(channelname);
  }

  /* find defaults (if any) */
  for (defs = DefChanList; defs && strcasecmp(defs->name, channelname);
    defs = defs->next);

  chan = (achannel *) MALLOC(sizeof(achannel));

  chan->name = (char *)MALLOC(strlen(channelname) + 1);
  strcpy(chan->name, channelname);
  chan->AmChanOp = on;
  chan->on = on;
  chan->lastact = now;
  chan->lasttopic = now;

  chan->lang = L_DEFAULT;

  if (defs)
  {
    chan->MassDeopPro = defs->MassDeopPro;
    chan->NickFloodPro = defs->NickFloodPro;
    chan->MsgFloodPro = defs->MsgFloodPro;
    chan->flags = defs->flags;
    chan->uflags = defs->uflags;
    chan->TS = defs->TS;
    strcpy(chan->mode, defs->mode);
  }
  else
  {
    /* channel default settings */
    chan->MassDeopPro = MAX_DEOP_RATE;
    chan->NickFloodPro = MAX_NICKCHANGE_RATE;
    chan->MsgFloodPro = MAX_PUBLIC_MSG_RATE;
    chan->flags = CFL_ALWAYSOP;
    chan->uflags = 0;
    chan->TS = TS;
    chan->mode[0] = '\0';
  }
  strcpy(chan->lastjoin, "");
  chan->bans = NULL;
  chan->users = NULL;
  chan->modebuff = NULL;
  chan->next = ChannelList[cl_hash(channelname)];
  ChannelList[cl_hash(channelname)] = chan;
  if (on)
  {
    changemode(chan->name, "+o", mynick, 1);
    if (*chan->mode != '\0')
    {
      strcpy(buffer, "-");
      strcat(buffer, chan->mode);
      bounce(chan->name, buffer, chan->TS);
    }
    flushmode(chan->name);
  }
}

void DelChannel(char *channelname)
{
  register achannel *chan;
  register achannel *prec = NULL;
  register auser *user;
  register modequeue *mode;
  register aban *ban;

  chan = ChannelList[cl_hash(channelname)];

  while (chan)
  {
    if (!strcasecmp(chan->name, channelname))
    {
      while ((user = chan->users) != NULL)
      {
	chan->users = user->next;
	FreeUser(user);
      }
      if (prec)
      {
	prec->next = chan->next;
	while ((mode = chan->modebuff) != NULL)
	{
	  chan->modebuff = mode->next;
	  TTLALLOCMEM -= sizeof(modequeue);
	  free(mode);
	}
	while ((ban = chan->bans) != NULL)
	{
	  chan->bans = ban->next;
	  TTLALLOCMEM -= sizeof(aban);
	  free(ban);
	}
	TTLALLOCMEM -= strlen(chan->name) + 1;
	free(chan->name);
	TTLALLOCMEM -= sizeof(achannel);
	free(chan);
	chan = prec->next;
      }
      else
      {
	ChannelList[cl_hash(channelname)] = chan->next;
	while ((mode = chan->modebuff) != NULL)
	{
	  chan->modebuff = mode->next;
	  TTLALLOCMEM -= sizeof(modequeue);
	  free(mode);
	}
	while ((ban = chan->bans) != NULL)
	{
	  chan->bans = ban->next;
	  TTLALLOCMEM -= sizeof(aban);
	  free(ban);
	}
	TTLALLOCMEM -= strlen(chan->name) + 1;
	free(chan->name);
	TTLALLOCMEM -= sizeof(achannel);
	free(chan);
	chan = ChannelList[cl_hash(channelname)];
      }
      break;
    }
    else
    {
      prec = chan;
      chan = chan->next;
    }
  }
}

void FreeUser(auser * user)
{
  register adeop *nodedeop;
  register amsg *nodemsg;
  while ((nodedeop = user->deophist) != NULL)
  {
    user->deophist = nodedeop->next;
    TTLALLOCMEM -= sizeof(adeop);
    free(nodedeop);
  }

  while ((nodemsg = user->msghist) != NULL)
  {
    user->msghist = nodemsg->next;
    TTLALLOCMEM -= sizeof(amsg);
    free(nodemsg);
  }
  TTLALLOCMEM -= sizeof(auser);
  free(user);
  /* phew! */
}


achannel *ToChannel(char *channel)
{
  register achannel *curr = ChannelList[cl_hash(channel)];

  while (curr && strcasecmp(curr->name, channel) != 0)
  {
    curr = curr->next;
  }
  return curr;
}

auser *ToUser(char *channel, char *nick)
{
  register auser *curr;
  register achannel *chan;

  chan = ToChannel(channel);
  if (!chan)
  {
    curr = NULL;
#ifdef DEBUG
    printf("ToUser(): channel not found!\n");
#endif
  }
  else
    curr = chan->users;
#ifdef DEBUG
  printf("Looking for %s...\n", nick);
#endif

  while (curr && strcasecmp(nick, curr->N->nick))
  {
#ifdef DEBUG
    printf("ToUser(): is on -> %s!%s@%s\n",
      curr->N->nick, curr->N->username, curr->N->site);
#endif
    curr = curr->next;
  }
#ifdef DEBUG
  if (!curr)
    printf("ToUser(): user not found!\n");
#endif
  return (curr);
}


void GetOps(char *channel)
{
  char buffer[200];
  register aluser *user;

#ifdef FAKE_UWORLD
  if (Uworld_status == 1)
  {
    sprintf(buffer, ":%s MODE %s +o %s\n",
      UFAKE_SERVER, channel, mynick);
    sendtoserv(buffer);
    log("REOP by fake Uworld");
    sprintf(buffer, "+o %s", mynick);
    ModeChange(UFAKE_SERVER, channel, buffer);
    return;
  }
#endif

  if ((user = ToLuser(UWORLD)) != NULL && !match(user->site, UWORLD_HOST))
    user = NULL;

#ifdef UWORLD2
  if (user == NULL)
  {
    if ((user = ToLuser(UWORLD2)) != NULL && !match(user->site, UWORLD2_HOST))
      user = NULL;
  }
#endif

  if (user != NULL)
  {
    sprintf(buffer, ":%s PRIVMSG %s :" UWORLD_COMMAND "\n",
      mynick, user->nick, channel);
    sendtoserv(buffer);
    sprintf(buffer, "I ASK %s FOR REOP ON %s", user->nick, channel);
    log(buffer);
  }
#ifdef DEBUG
  else
  {
    printf("GetOps(): %s isn't online...\n", UWORLD);
  }
#endif
  /* If Uworld is not present, queue the request
   * and try again later..
   * If he's present, send the request and check later on
   * if it worked.
   */
  AddEvent(EVENT_GETOPS, now + GETOPS_FREQ, channel);
}

int GuessChannel(char *nick, char *output)
{
  register aluser *luser;
  register avalchan *vchan;
  register achannelnode *nchan;
  register int found = 0;
  char tmp[80];

  luser = ToLuser(nick);
  if (luser == NULL)
  {
    return 0;
  }

  vchan = luser->valchan;

  if (vchan && !strcmp(vchan->name, "*"))
    vchan = vchan->next;

  if (vchan && !vchan->next)
  {
    strcpy(output, vchan->name);
    return 1;
  }

  nchan = luser->channel;
  while (nchan != NULL)
  {
    if (nchan->N->on)
    {
      found++;
      strcpy(tmp, nchan->N->name);
    }
    nchan = nchan->next;
  }

  if (found == 1)
  {
    strcpy(output, tmp);
    return 1;
  }

  return 0;
}

int IsOpless(char *channel)
{
  register achannel *chan;
  register auser *user;

  chan = ToChannel(channel);
  if (chan == NULL)
    return 0;

  user = chan->users;
  while (user != NULL && !user->chanop)
    user = user->next;

  if (user != NULL || chan->AmChanOp)
    return 0;
  else
    return 1;
}

void onopless(char *channel)
{
  register achannel *chan;

  if ((chan = ToChannel(channel)) == NULL || !chan->on)
  {
    /* don't do anything if not on channel
     */
    return;
  }

  GetOps(channel);
}

void oninvite(char *source, char *channel)
{
  char buffer[1024];
#ifdef DEBUG
  printf("Received INVITE from %s to %s\n", source, channel);
#endif
  sprintf(buffer, "I'M INVITED ON %s BY %s", channel, source);
  log(buffer);

  if (strlen(channel) > 150)
  {
    notice(source, "yeah.. I know that game!");
    return;
  }

  if (Access(channel, source) >= LEVEL_JOIN)
  {
    join(source, channel, "");
  }
}

int IsReg(char *channel)
{
  struct stat st;
  register struct RegUser *reg;

  if (stat(make_dbfname(channel), &st) < 0)
  {
    for (reg = UserList[ul_hash(channel)]; reg != NULL; reg = reg->next)
      if (!strcasecmp(reg->channel, channel))
	break;
    if (reg == NULL)
      return 0;
  }

  return 1;
}


void join(char *source, char *chan, char *arg)
{
  char buffer[1024];
  char channel[80];
  register achannel *ch;
  register auser *user;
  register char *ptr;

  if (*arg == '#')
  {
    GetWord(0, arg, channel);
  }
  else
  {
    GetWord(0, chan, channel);
  }

  if (strchr(channel, ',') != NULL)
  {
    if (*source)
      notice(source, "Invalid channel name!");
    return;
  }

  if (!strcmp(channel, "*"))
  {
    notice(source, "SYNTAX: join <channel>");
    return;
  }

  for (ptr = channel; *ptr; ptr++)
  {
    *ptr = tolowertmp(*ptr);
  }

  if (*source && Access(channel, source) < LEVEL_JOIN)
  {
    ReplyNotAccess(source, channel);
    return;
  }

  if (*source && !IsReg(channel))
  {
    notice(source, "That channel is not registered");
    return;
  }

  /* The server's messages are unreliable for *long* channel names
     so I decided that the bot will simply NOT join any channel
     which have names longer than 45 characters... no big deal!      */

  if (strlen(channel) <= 45)
  {
    ch = ToChannel(channel);
    if (ch != NULL)
    {
      for (user = ch->users; user != NULL; user = user->next)
      {
	if (!strcasecmp(user->N->username, DEFAULT_USERNAME) &&
	  !strcasecmp(user->N->site, DEFAULT_HOSTNAME))
	{
	  if (*source)
	  {
	    sprintf(buffer, "%s is already on that channel", user->N->nick);
	    notice(source, buffer);
	  }
	  return;
	}
      }
    }
    sprintf(buffer, "I JOIN %s", channel);
    log(buffer);
    sprintf(buffer, ":%s JOIN %s\n", mynick, channel);
    sendtoserv(buffer);
    if (ch == NULL)
    {
      NewChannel(channel, now, 1);
    }
    else
    {
      ch->on = 1;
      ch->lastact = now;
      if (IsOpless(channel) || ((ch->flags & CFL_ALWAYSOP) &&
	  !ch->AmChanOp))
	AddEvent(EVENT_GETOPS,
	  now + GETOPS_ONJOIN_DELAY, channel);
    }
  }
#ifdef DEBUG
  else
    printf("ARGH! *VERY* long channel name.. won't join :/\n");
#endif
}

void joindefault(void)
{
  register adefchan *chan;
  register achannel *ch;

  chan = DefChanList;
  while (chan)
  {
    ch = ToChannel(chan->name);
    /* join the channel if not already on */
    if (ch == NULL || ch->on == 0)
      join("", chan->name, "");
    chan = chan->next;
  }
}

void part(char *source, char *chan, char *arg)
{
  char buffer[512];
  char channel[80];
  register achannel *ch;

  if (*arg == '#')
  {
    GetWord(0, arg, channel);
  }
  else
  {
    GetWord(0, chan, channel);
    GuessChannel(source, channel);
  }

  if (!strcmp(channel, "*"))
  {
    notice(source, "SYNTAX: part <channel>");
    return;
  }

  ch = ToChannel(channel);
  if (ch == NULL)
  {
    notice(source, replies[RPL_NOTONCHANNEL][L_DEFAULT]);
    return;
  }
  if (*source && Access(channel, source) < LEVEL_PART)
  {
    ReplyNotAccess(source, channel);
    return;
  }

  ch->on = 0;
  ch->AmChanOp = 0;
  if (ch->users == NULL)
    DelChannel(channel);

  sprintf(buffer, "I LEAVE %s", channel);
  log(buffer);

  sprintf(buffer, ":%s PART %s\n", mynick, channel);
  sendtoserv(buffer);
}

void invite(char *source, char *ch, char *args)
{
  char buffer[512];
  char channel[80];
  char target[80];
  register achannel *chan;

  if (*args == '#')
  {
    GetWord(0, args, channel);
    args = ToWord(1, args);
  }
  else
  {
    GetWord(0, ch, channel);
    GuessChannel(source, channel);
  }

  if (!strcmp(channel, "*"))
  {
    notice(source, "SYNTAX: invite [channel]");
    return;
  }

  if ((chan = ToChannel(channel)) == NULL || !chan->on)
  {
    notice(source, replies[RPL_NOTONCHANNEL][L_DEFAULT]);
    return;
  }

  if (chan->flags & CFL_OPONLY)
  {
    notice(source, replies[RPL_OPONLY][chan->lang]);
    return;
  }

  if (Access(channel, source) < INVITE_LEVEL)
  {
    ReplyNotAccess(source, channel);
    return;
  }


  if (IsSet(channel, 'i', NULL) && !chan->AmChanOp)
  {
    notice(source, replies[RPL_NOTCHANOP][chan->lang]);
    return;
  }

  GetWord(0, args, target);

  if (*target != '\0' && strcasecmp(target, source) && (source[0] != '+'))
  {
    notice(source, "You are only allowed to invite yourself");
    return;
  }

  if(*target == '\0')
    strcpy(target, source);

  if (ToUser(channel, target) != NULL)
  {
    notice(source, replies[RPL_ALREADYONCHANNEL][chan->lang]);
    return;
  }

  sprintf(buffer, ":%s INVITE %s %s\n", mynick, target, channel);
  sendtoserv(buffer);

  /* No need to confirm the invitation since it was sent to
   * the user already!
   */
  if (strcasecmp(target, source))
  {
    sprintf(buffer, replies[RPL_IINVITED][chan->lang], target, channel);
    notice(source, buffer);
    sprintf(buffer, replies[RPL_YOUAREINVITED][chan->lang], source, channel);
    notice(target, buffer);
  }
}

void onjoin(char *source, char *channel)
{
  char buffer[512];
  char mask[200], reason[200];
  char *ptr;
  register achannel *chan;
  register achannelnode *c;
  register aluser *user;
  register auser *usr, *tmp;
  register RegUser *reg;

#ifdef DEBUG
  printf("onjoin( %s, %s)\n", source, channel);
#endif

  user = ToLuser(source);
  if (user == NULL)
  {
    /* a server would send a KILL, but I think
     * it'll be ok to ignore it
     */
    sprintf(buffer, "ERROR onjoin(): Unknown USER %s!", source);
    log(buffer);
    return;
  }

  if (channel[0] == ':')
    channel++;

  while (channel)
  {
    ptr = strchr(channel, ',');
    if (ptr != NULL)
      *(ptr++) = '\0';

    /* join 0 */
    if (!strcmp(channel, "0"))
    {
      while ((c = user->channel) != NULL)
      {
	onpart(source, c->N->name);
      }
      channel = ptr;
      continue;
    }
#ifdef DEBUG
    printf("JOIN: %s!%s@%s on channel %s\n",
      user->nick, user->username, user->site, channel);
#endif

    if (!*channel)
    {
      log("ERROR: onjoin(): JOIN to null channel!");
      log(source);
      log(channel);
      channel = ptr;
      continue;
    }

    chan = ToChannel(channel);
    if (!chan)
    {
#ifdef DEBUG
      printf("onjoin(): New channel\n");
#endif
      NewChannel(channel, now + TSoffset, 0);
      chan = ToChannel(channel);
    }

    chan->lastact = now;

    c = (achannelnode *) MALLOC(sizeof(achannelnode));
    c->N = chan;
    c->nickhist = NULL;
    c->next = user->channel;
    user->channel = c;

    strcpy(chan->lastjoin, source);

    usr = (auser *) MALLOC(sizeof(auser));

    usr->chanop = 0;
    usr->deophist = NULL;
    usr->lastact = now;
    usr->msghist = NULL;
    usr->N = user;
    usr->next = chan->users;
    chan->users = usr;

    if (chan->on)
    {
      for (tmp = chan->users; tmp != NULL; tmp = tmp->next)
      {
	if (!strcasecmp(tmp->N->username, DEFAULT_USERNAME)
	  && !strcasecmp(tmp->N->site, DEFAULT_HOSTNAME)
	  && tmp->N->time <= logTS)
	{
	  sprintf(buffer, "PAL's already on %s (%ld <= %ld)",
	    chan->name, tmp->N->time, logTS);
	  log(buffer);
	  part("", chan->name, "");
	}
      }
    }


    try_find(channel, user);
    reg = IsValid(user, channel);

    sprintf(buffer, "%s!%s@%s", user->nick, user->username, user->site);

    if (chan->on && chan->AmChanOp &&
      IsShit(channel, buffer, mask, reason) >= AUTO_KICK_SHIT_LEVEL)
    {
      log("Detected banned user (AUTO-KICK LEVEL)");
      notice(source, "*** Sorry. You are on my banlist ***");
      mban("", channel, mask);
      sprintf(buffer, "%s %s [%s]", mask, mask, reason);
      kick("", channel, buffer);
    }
    else if (chan->on && chan->AmChanOp
	&& reg && (reg->flags & UFL_AUTOOP)
	&& reg->suspend < now
	&& *reg->passwd != '\0'
	&& !(chan->flags & CFL_NOOP)
	&& IsShit(channel, buffer, NULL, NULL) < NO_OP_SHIT_LEVEL
      )
    {
      op("", channel, source);
    }

    if (chan->on && chan->AmChanOp && (chan->flags & CFL_AUTOTOPIC) &&
      chan->lasttopic + AUTOTOPIC_FREQ < now)
    {
      adefchan *def = DefChanList;
      while (def != NULL && strcasecmp(def->name, chan->name))
	def = def->next;
      if (def != NULL && (def->url || def->topic))
      {
	chan->lasttopic = now;
	sprintf(buffer, "%s (%s)", (def->topic) ? def->topic : "",
	  (def->url) ? def->url : "");
	topic("", chan->name, buffer);
      }
    }
    channel = ptr;
  }
}

void onpart(char *nick, char *channel)
{
  char buffer[200];
  register auser *user;
  register aluser *luser;
  register auser *prec = NULL;
  register achannel *chan;
  register achannelnode **ch, *c;
  register anickchange *nickhist;

#ifdef DEBUG
  printf("PART: %s from %s\n", nick, channel);
#endif

  luser = ToLuser(nick);
  if (luser == NULL)
  {
    sprintf(buffer, "ERROR: onpart(): Unknown USER %s!", nick);
    log(buffer);
#ifdef HISTORY
    History(NULL);
#endif

    return;
  }
  chan = ToChannel(channel);

  if (chan == NULL)
  {
    user = NULL;
    log("ERROR: onpart(): null channel!? /*core dumped*/");
    log(nick);
    log(channel);
    /*dumpcore(""); */
    return;
  }
  else
  {
    chan->lastact = now;
    user = chan->users;
  }

  while (user)
  {
    if (!strcasecmp(user->N->nick, nick))
    {
      /* remove the structure from mem */

      if (prec)
      {
	prec->next = user->next;
	FreeUser(user);
	user = prec->next;
      }
      else
      {
	chan->users = user->next;
	FreeUser(user);
	user = chan->users;
      }
      break;
    }
    else
    {
      prec = user;
      user = user->next;
    }
  }

  ch = &luser->channel;
  while (*ch && strcasecmp(channel, (*ch)->N->name))
    ch = &(*ch)->next;
  c = *ch;
  if (c == NULL)
  {
#ifdef DEBUG
    printf("WARNING: onpart(): channel %s not found!\n", channel);
#endif
  }
  else
  {
    *ch = c->next;

    while ((nickhist = c->nickhist) != NULL)
    {
      c->nickhist = nickhist->next;
      TTLALLOCMEM -= sizeof(anickchange);
      free(nickhist);
    }
    TTLALLOCMEM -= sizeof(achannelnode);
    free(c);
  }

  if (chan != NULL && chan->users == NULL && !chan->on)
  {
    DelChannel(channel);
  }
  if (IsOpless(channel))
    onopless(channel);
}

void onkick(char *source, char *channel, char *body)
{
  char nick[NICK_LENGTH];
  char buffer[1024];
  register achannel *chan;
  register auser *user;
  register int count;

  GetWord(0, body, nick);
  if (!strcasecmp(mynick, nick))
  {
    user = ToUser(channel, source);
    if (user == NULL)
    {
      sprintf(buffer, "I'M KICKED OFF %s by %s",
	channel, source);
    }
    else
    {
      sprintf(buffer, "I'M KICKED OFF %s BY %s!%s@%s (%d) %s",
	channel, user->N->nick, user->N->username, user->N->site,
	Access(channel, source), ToWord(1, body));
    }
    log(buffer);
    broadcast(buffer, 0);
    chan = ToChannel(channel);
    if ((chan->flags & CFL_ALWAYSOP) && Access(channel, source) >= ALWAYSOP_OVERRIDE_LEVEL)
    {
      chan->flags &= ~CFL_ALWAYSOP;
      sprintf(buffer, "AlwaysOp is turned off on %s", channel);
      log(buffer);
      RemChan(source, channel, channel);
      notice(source, replies[RPL_ALWAYSOPWASACTIVE][chan->lang]);
    }
    if (chan->flags & CFL_ALWAYSOP)
    {
      /* In AlwaysOp mode, if someone KICKs the bot, he/she
       * gets deops/suspended/shitlisted for predetermined
       * time and levels
       */
      if (user != NULL)
      {
	count = IsShit(channel, source, NULL, NULL);
	notice(source, replies[RPL_ALWAYSOP][chan->lang]);
	switch (count)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	  log("First warning");
	  notice(source, replies[RPL_KICK1ST][chan->lang]);
	  changemode(channel, "-o", source, 0);
	  flushmode(channel);
	  user->chanop = 0;
	  break;

	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	  log("Second warning");
	  notice(source, replies[RPL_KICK2ND][chan->lang]);
	  kick("", channel, source);
	  break;

	default:
	  changemode(channel, "-o", source, 0);
	  flushmode(channel);
	  user->chanop = 0;
	  sprintf(buffer, "%s!%s@%s %d",
	    user->N->nick, user->N->username,
	    user->N->site,
	    DEOPME_SUSPEND_TIME);
	  suspend("", channel, buffer);
	  sprintf(buffer, "Suspended %s!%s@%s on %s for repeatedly kicking me",
	    user->N->nick, user->N->username, user->N->site, channel);
	  SpecLog(buffer);
	  broadcast(buffer, 0);
	}
	sprintf(buffer, "%s %d %d *** KICK WHILE ALWAYSOP ACTIVE ***",
	  source, DEOP_SHITLIST_TIME,
	  (count < 10) ? count + 5 :
	  DEOP_SHITLIST_LEVEL);
	AddToShitList("", channel, buffer, 0);
      }
      part("", channel, "");
      join("", channel, "");
      GetOps(channel);
    }
    else
    {
      part("", channel, "");
    }
  }
  CheckFlood(source, channel, 80);
}

void QuitAll(void)
{
  register int j;

  if (ServerList != NULL)
    onsquit("", ServerList->name, NULL);

  for (j = 0; j < 1000; j++)
  {
    while (ChannelList[j] != NULL)
    {
      DelChannel(ChannelList[j]->name);
    }
  }
}

void CheckIdleChannels(void)
{
  register achannel *chan, *tmp;
  register int i;
  char buffer[200];

  for (i = 0; i < 1000; i++)
  {
    chan = ChannelList[i];
    while (chan != NULL)
    {
      tmp = chan->next;
      if (chan->on && chan->lastact + MAX_IDLE_TIME <= now)
      {
	sprintf(buffer, "Channel %s has exceeded the idle time limit of %.2f hours", chan->name, (float)MAX_IDLE_TIME / 3600.0);
	SpecLog(buffer);
	part("", chan->name, "");
	RemChan("", chan->name, "");
      }
      chan = tmp;
    }
  }
}


void topic(char *source, char *chan, char *args)
{
  char buffer[1024];
  char channel[80];
  register achannel *ch;
  strcpy(channel, chan);

  if (*source)
  {
    if (*args == '#')
    {
      GetWord(0, args, channel);
      args = ToWord(1, args);
    }
    else
    {
      strcpy(channel, chan);
      GuessChannel(source, channel);
    }

    if (!strcmp(channel, "*"))
    {
      notice(source, "SYNTAX: topic <channel> <new topic>");
      return;
    }

    if (Access(channel, source) < TOPIC_LEVEL)
    {
      ReplyNotAccess(source, channel);
      return;
    }

    if (!*args)
    {
      notice(source, "SYNTAX: topic [channel] <new topic>");
      return;
    }
  }

  ch = ToChannel(channel);
  if (ch == NULL)
  {
    notice(source, replies[RPL_NOTONCHANNEL][L_DEFAULT]);
    return;
  }

  if (ch->flags & CFL_OPONLY)
  {
    notice(source, replies[RPL_OPONLY][ch->lang]);
    return;
  }

  ch->lastact = now;
  sprintf(buffer, "I CHANGED TOPIC on %s to %s", channel, args);
  log(buffer);

  sprintf(buffer, ":%s TOPIC %s :%s\n", mynick, channel, args);
  sendtoserv(buffer);
}

void ontopic(char *source, char *target, char *body)
{
  CheckFlood(source, target, strlen(body) + 60);
}

void onnotice(char *source, char *target, char *body)
{
  if (*target == '#')
    CheckFlood(source, target, strlen(body) + 10);
  else
    CheckPrivateFlood(source, strlen(body) + 10, "NOTICE-");
}

void SetChanFlag(char *source, char *ch, char *args)
{
  char buffer[200];
  char channel[80];
  char variable[80];
  char svalue[80];
  register int value, acc, i, found;
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

  acc = Access(channel, source);

  GetWord(0, args, variable);
  GetWord(1, args, svalue);

  if (!strcmp(channel, "*") || !*variable)
  {
    notice(source, "SYNTAX: set [#channel] <variable> <value>");
    return;
  }

  chan = ToChannel(channel);
  if (chan == NULL)
  {
    notice(source, replies[RPL_CHANNOTEXIST][L_DEFAULT]);
    return;
  }

  value = atoi(svalue);

  if (!strncasecmp(variable, "FLOODPRO", strlen(variable)))
  {
    if (acc < CH_FLOOD_LIMIT_LEVEL)
    {
      ReplyNotAccess(source, channel);
      return;
    }
    if (value != 0 && (value < 3 || value > 20))
    {
      notice(source, replies[RPL_BADFLOODLIMIT][chan->lang]);
      return;
    }

    chan->MsgFloodPro = value;
    sprintf(buffer, replies[RPL_SETFLOODLIMIT][chan->lang], value);
    notice(source, buffer);

    sprintf(buffer, "%s SET FLOODPRO ON %s TO %d",
      source, channel, value);
    log(buffer);

  }
  else if (!strncasecmp(variable, "NICKFLOODPRO", strlen(variable)))
  {
    if (acc < CH_NICK_FLOOD_LIMIT_LEVEL)
    {
      ReplyNotAccess(source, channel);
      return;
    }
    if (value != 0 && (value < 3 || value > 10))
    {
      notice(source, replies[RPL_BADNICKFLOODLIMIT][chan->lang]);
      return;
    }

    chan->NickFloodPro = value;
    sprintf(buffer, replies[RPL_SETNICKFLOODLIMIT][chan->lang], value);
    notice(source, buffer);

    sprintf(buffer, "%s SET NICKFLOODPRO ON %s TO %d",
      source, channel, value);
    log(buffer);

  }
  else if (!strncasecmp(variable, "MASSDEOPPRO", strlen(variable)))
  {
    if (acc < CH_MASSDEOP_LIMIT_LEVEL)
    {
      ReplyNotAccess(source, channel);
      return;
    }
    if (value != 0 && (value < 3 || value > 10))
    {
      notice(source, replies[RPL_BADMASSDEOPLIMIT][chan->lang]);
      return;
    }

    chan->MassDeopPro = value;
    sprintf(buffer, replies[RPL_SETMASSDEOPLIMIT][chan->lang], value);
    notice(source, buffer);

    sprintf(buffer, "%s SET MASSDEOPPRO ON %s TO %d",
      source, channel, value);
    log(buffer);


  }
  else if (!strncasecmp(variable, "NOOP", strlen(variable)))
  {
    if (acc < CH_NOOP_LEVEL)
    {
      ReplyNotAccess(source, channel);
      return;
    }
    if (!strcasecmp(svalue, "ON") || !strcasecmp(svalue, "EIN"))
    {
      chan->flags |= CFL_NOOP;
      notice(source, replies[RPL_NOOPON][chan->lang]);
      massdeop(channel);
      sprintf(buffer, "%s SET NOOP ON %s ON", source, channel);
      log(buffer);
    }
    else if (!strcasecmp(svalue, "OFF") || !strcasecmp(svalue, "AUS"))
    {
      chan->flags &= ~CFL_NOOP;
      notice(source, replies[RPL_NOOPOFF][chan->lang]);
      sprintf(buffer, "%s SET NOOP ON %s OFF", source, channel);
      log(buffer);
    }
    else
      notice(source, replies[RPL_BADNOOP][chan->lang]);

  }
  else if (!strncasecmp(variable, "ALWAYSOP", strlen(variable)))
  {
    if (acc < CH_ALWAYSOP_LEVEL)
    {
      ReplyNotAccess(source, channel);
      return;
    }
    if (!strcasecmp(svalue, "ON") || !strcasecmp(svalue, "EIN"))
    {
      chan->flags |= CFL_ALWAYSOP;
      notice(source, replies[RPL_ALWAYSOPON][chan->lang]);
      sprintf(buffer, "%s SET ALWAYSOP ON %s ON",
	source, channel);
      log(buffer);
      /* looks like ppl would like X to op himself when
       * ALWAYSOP is truned on... fine ;)
       */
      if (!chan->AmChanOp)
      {
	GetOps(chan->name);
      }
    }
    else if (!strcasecmp(svalue, "OFF") || !strcasecmp(svalue, "AUS"))
    {
      chan->flags &= ~CFL_ALWAYSOP;
      notice(source, replies[RPL_ALWAYSOPOFF][chan->lang]);
      sprintf(buffer, "%s SET ALWAYSOP ON %s OFF",
	source, channel);
      log(buffer);
    }
    else
      notice(source, replies[RPL_BADALWAYSOP][chan->lang]);
  }
  else if (!strncasecmp(variable, "OPONLY", strlen(variable)))
  {
    if (acc < CH_OPONLY_LEVEL)
    {
      ReplyNotAccess(source, channel);
      return;
    }
    if (!strcasecmp(svalue, "ON") || !strcasecmp(svalue, "EIN"))
    {
      chan->flags |= CFL_OPONLY;
      notice(source, replies[RPL_OPONLYON][chan->lang]);
      sprintf(buffer, "%s SET OPONLY ON %s ON",
	source, channel);
      log(buffer);
    }
    else if (!strcasecmp(svalue, "OFF") || !strcasecmp(svalue, "AUS"))
    {
      chan->flags &= ~CFL_OPONLY;
      notice(source, replies[RPL_OPONLYOFF][chan->lang]);
      sprintf(buffer, "%s SET OPONLY ON %s OFF",
	source, channel);
      log(buffer);
    }
    else
      notice(source, replies[RPL_BADOPONLY][chan->lang]);
  }
  else if (!strncasecmp(variable, "STRICTOP", strlen(variable)))
  {
    if (acc < CH_STRICTOP_LEVEL)
    {
      ReplyNotAccess(source, channel);
      return;
    }
    if (!strcasecmp(svalue, "ON") || !strcasecmp(svalue, "EIN"))
    {
      chan->flags |= CFL_STRICTOP;
      notice(source, replies[RPL_STRICTOPON][chan->lang]);
      sprintf(buffer, "%s SET STRICTOP ON %s ON",
	source, channel);
      log(buffer);
    }
    else if (!strcasecmp(svalue, "OFF") || !strcasecmp(svalue, "AUS"))
    {
      chan->flags &= ~CFL_STRICTOP;
      notice(source, replies[RPL_STRICTOPOFF][chan->lang]);
      sprintf(buffer, "%s SET STRICTOP ON %s OFF",
	source, channel);
      log(buffer);
    }
    else
      notice(source, replies[RPL_BADSTRICTOP][chan->lang]);
  }
  else if (!strncasecmp(variable, "USERFLAGS", strlen(variable)))
  {
    if (acc < CH_USERFLAGS_LEVEL)
    {
      ReplyNotAccess(source, channel);
      return;
    }
    if (value < 0 || value > 3)
    {
      notice(source, replies[RPL_BADUSERFLAGS][chan->lang]);
      return;
    }
    chan->uflags = (unsigned long)value;
    sprintf(buffer, replies[RPL_SETUSERFLAGS][chan->lang], value);
    notice(source, buffer);
    sprintf(buffer, "%s SET USERFLAGS TO %d on %s",
      source, value, channel);
    log(buffer);
  }
  else if (!strncasecmp(variable, "LANGUAGE", strlen(variable)))
  {
    if (acc < CH_LANG_LEVEL)
    {
      ReplyNotAccess(source, channel);
      return;
    }
    found = 0;
    for (i = 0; i < NO_LANG; i++)
    {
      if (!strcasecmp(svalue, Lang[i].abbr) ||
	!strcasecmp(svalue, Lang[i].name))
      {
	chan->lang = i;
	found = 1;
	break;
      }
    }
    if (!found)
    {
      strcpy(buffer, replies[RPL_KNOWNLANG][chan->lang]);
      for (i = 0; i < NO_LANG; i++)
      {
	strcat(buffer, " ");
	strcat(buffer, Lang[i].abbr);
	strcat(buffer, " (");
	strcat(buffer, Lang[i].name);
	strcat(buffer, ")");
      }
      notice(source, buffer);
    }
    else
    {
      sprintf(buffer, replies[RPL_SETLANG][chan->lang],
	Lang[chan->lang].abbr, Lang[chan->lang].name);
      notice(source, buffer);
    }
  }
  else if (!strncasecmp(variable, "TOPIC", strlen(variable)) ||
    !strncasecmp(variable, "DESCRIPTION", strlen(variable)))
  {
    register adefchan *def;
    if (acc < CH_TOPIC_LEVEL)
    {
      ReplyNotAccess(source, channel);
      return;
    }
    def = DefChanList;
    while (def && strcasecmp(def->name, channel))
      def = def->next;
    if (!def)
    {
      notice(source, "You must use 'addchan' first.");
      return;
    }
    strncpy(def->topic, ToWord(1, args), 79);
    def->topic[79] = '\0';
    notice(source, "New description set");
  }
  else if (!strncasecmp(variable, "URL", strlen(variable)))
  {
    register adefchan *def;
    register char *ptr;
    if (acc < CH_URL_LEVEL)
    {
      ReplyNotAccess(source, channel);
      return;
    }
    def = DefChanList;
    while (def && strcasecmp(def->name, channel))
      def = def->next;
    if (!def)
    {
      notice(source, "You must use 'addchan' first.");
      return;
    }
    for (ptr = svalue; *ptr; ptr++)
    {
      if (*(unsigned char *)ptr >= 0x80 || *ptr < 0x20 ||
	strchr("<>;\"&\\", *ptr))
      {
	notice(source, "Invalid URL");
	return;
      }
    }
    strncpy(def->url, svalue, 79);
    def->url[79] = '\0';
    notice(source, "New URL set");
  }
  else if (!strncasecmp(variable, "AUTOTOPIC", strlen(variable)))
  {
    if (acc < CH_AUTOTOPIC_LEVEL)
    {
      ReplyNotAccess(source, channel);
      return;
    }
    if (!strcasecmp(svalue, "ON") || !strcasecmp(svalue, "EIN"))
    {
      chan->flags |= CFL_AUTOTOPIC;
      notice(source, replies[RPL_AUTOTOPICON][chan->lang]);
      sprintf(buffer, "%s SET AUTOTOPIC ON %s ON",
	source, channel);
      log(buffer);
    }
    else if (!strcasecmp(svalue, "OFF") || !strcasecmp(svalue, "AUS"))
    {
      chan->flags &= ~CFL_AUTOTOPIC;
      notice(source, replies[RPL_AUTOTOPICOFF][chan->lang]);
      sprintf(buffer, "%s SET AUTOTOPIC ON %s OFF",
	source, channel);
      log(buffer);
    }
    else
      notice(source, replies[RPL_BADAUTOTOPIC][chan->lang]);
  }

}

void showstatus(char *source, char *ch, char *args)
{
  char buffer[512];
  char channel[80];
  char global[] = "*";
  register achannel *chan;
  register auser *user;
  register aluser *luser;
  register achannelnode *lchan;
  register int i, j, k, isoper = 0;
  register time_t t;
  register int days, hours, mins, secs;
  register RegUser *reg;
  int srcaccess;

  if (*args == '#' || *args == '*')
  {
    GetWord(0, args, channel);
    args = ToWord(1, args);
  }
  else
  {
    strcpy(channel, ch);
    GuessChannel(source, channel);
  }

  luser = ToLuser(source);
  if (luser)
    isoper = (luser->mode & LFL_ISOPER) ? 1 : 0;

  if (!strcmp(channel, "*") && !IsValid(luser, channel) && !isoper)
  {
    notice(source, "SYNTAX: status <channel>");
    return;
  }

  if ((srcaccess = Access(channel, source)) < STATUS_ACCESS &&
    Access(global, source) < XADMIN_LEVEL && !isoper)
  {
    ReplyNotAccess(source, channel);
    return;
  }

  if (!strcmp(channel, "*"))
  {
    /* count the number of channels */
    for (i = j = k = 0; j < 1000; j++)
    {
      chan = ChannelList[j];
      while (chan)
      {
	if (chan->on)
	  i++;
	k++;
	chan = chan->next;
      }
    }

    sprintf(buffer, "I'm on %d channel%c (out of %d)", i, (i > 1) ? 's' : ' ', k);
    notice(source, buffer);

    for (i = j = k = 0; j < 1000; j++)
    {
      luser = Lusers[j];
      while (luser)
      {
	lchan = luser->channel;
	while (lchan)
	{
	  if (lchan->N->on)
	  {
	    i++;
	    break;
	  }
	  lchan = lchan->next;
	}
	k++;
	luser = luser->next;
      }
    }
    sprintf(buffer, "I'm seeing %d users (out of %d)", i, k + 1 /* add self */ );
    notice(source, buffer);

    sprintf(buffer, "Total read bytes: %lu  Total sent bytes: %lu",
      TTLREADBYTES, TTLSENTBYTES);
    notice(source, buffer);
    sprintf(buffer, "Total sent bytes in http: %lu",
      HTTPTTLSENTBYTES);
    notice(source, buffer);
    sprintf(buffer, "Total allocated memory: %lu", TTLALLOCMEM);
    notice(source, buffer);

    t = now - TSonline;
    days = (int)t / 86400;
    t %= 86400;
    hours = (int)t / 3600;
    t %= 3600;
    mins = (int)t / 60;
    t %= 60;
    secs = (int)t;

    if (days > 0)
      sprintf(buffer, "Service Up %d day%s %d:%s%d:%s%d",
	days, (days > 1) ? "s" : "", hours,
	(mins <= 9) ? "0" : "", mins,
	(secs <= 9) ? "0" : "", secs);
    else
      sprintf(buffer, "Service Up %d:%s%d:%s%d",
	hours,
	(mins <= 9) ? "0" : "", mins,
	(secs <= 9) ? "0" : "", secs);
    notice(source, buffer);
  }
  else
  {
    chan = ToChannel(channel);
    if (!chan || !chan->on)
    {
      notice(source, replies[RPL_NOTONCHANNEL][L_DEFAULT]);
      return;
    }

    /* count the number of users */
    i = j = 0;
    user = chan->users;
    while (user)
    {
      if (user->chanop)
	j++;
      i++;
      user = user->next;
    }
    if (chan->AmChanOp)
      j++;

    if (chan->lang != L_GERMAN)
    {
      sprintf(buffer, replies[RPL_STATUS1][chan->lang],
	channel, i + 1, (i > 0) ? "s" : "", j, (j > 1) ? "s" : "");
    }
    else
    {
      sprintf(buffer, replies[RPL_STATUS1][chan->lang],
	channel, i + 1, j);
    }
    notice(source, buffer);

    if (srcaccess >= STATUS_ACCESS_MODE || isoper)
    {
      sprintf(buffer, replies[RPL_STATUS2][chan->lang], chan->mode);
      notice(source, buffer);
    }

    sprintf(buffer, "MassDeopPro: %3d  NickFloodPro: %3d  FloodPro: %3d",
      chan->MassDeopPro,
      chan->NickFloodPro,
      chan->MsgFloodPro);
    notice(source, buffer);

    if (chan->lang != L_GERMAN)
    {
      sprintf(buffer, "NoOp: %3s  AlwaysOp: %3s  OpOnly: %3s  StrictOp: %3s  AutoTopic: %3s",
	(chan->flags & CFL_NOOP) ? "ON" : "OFF",
	(chan->flags & CFL_ALWAYSOP) ? "ON" : "OFF",
	(chan->flags & CFL_OPONLY) ? "ON" : "OFF",
	(chan->flags & CFL_STRICTOP) ? "ON" : "OFF",
	(chan->flags & CFL_AUTOTOPIC) ? "ON" : "OFF");
    }
    else
    {
      sprintf(buffer, "NoOp: %3s  AlwaysOp: %3s  OpOnly: %3s  StrictOp: %3s  AutoTopic: %3s",
	(chan->flags & CFL_NOOP) ? "EIN" : "AUS",
	(chan->flags & CFL_ALWAYSOP) ? "EIN" : "AUS",
	(chan->flags & CFL_OPONLY) ? "EIN" : "AUS",
	(chan->flags & CFL_STRICTOP) ? "EIN" : "AUS",
	(chan->flags & CFL_AUTOTOPIC) ? "EIN" : "AUS");
    }
    notice(source, buffer);

    if (chan->uflags)
    {
      sprintf(buffer, replies[RPL_STATUS3][chan->lang],
	(chan->uflags & UFL_AUTOOP) ? " AUTOOP" : "");
      notice(source, buffer);
    }

    sprintf(buffer, replies[RPL_STATUS4][chan->lang], Lang[chan->lang].name);
    notice(source, buffer);

    i = (now - chan->lastact);
    if (i != 0)
    {
      sprintf(buffer, replies[RPL_STATUS5][chan->lang],
	i, (i > 1) ? "s" : "");
      notice(source, buffer);
    }

    strcpy(buffer, "Auth:");
    for (j = 0; j < 1000; j++)
    {
      luser = Lusers[j];
      while (luser)
      {
	if ((reg = IsValid(luser, chan->name)) != NULL)
	{
	  sprintf(buffer + strlen(buffer), " %s(%s%d)", luser->nick,
	    *reg->channel == '*' ? "*" : "", reg->access);
	}

	if (strlen(buffer) > 400)
	{
	  notice(source, buffer);
	  strcpy(buffer, "Auth:");
	}

	luser = luser->next;
      }
    }

    if (strlen(buffer) > 5)
      notice(source, buffer);
  }
}

void SendBurst(void)
{
  register achannel *chan;
  register adefchan *defs;
  char buffer[512], buffer2[200];
  register char *ptr;

  sprintf(buffer, ":%s JOIN ", mynick);

  /* find defaults (if any) */
  for (defs = DefChanList; defs != NULL; defs = defs->next)
  {

    chan = (achannel *) MALLOC(sizeof(achannel));

    for (ptr = defs->name; *ptr; ptr++)
    {
      *ptr = tolowertmp(*ptr);
    }

    chan->name = (char *)MALLOC(strlen(defs->name) + 1);
    strcpy(chan->name, defs->name);
    chan->AmChanOp = 1;
    chan->on = 1;
    chan->lastact = now;
    chan->lasttopic = now;
    chan->lang = 0;
    chan->MassDeopPro = defs->MassDeopPro;
    chan->NickFloodPro = defs->NickFloodPro;
    chan->MsgFloodPro = defs->MsgFloodPro;
    chan->flags = defs->flags;
    chan->uflags = defs->uflags;
    chan->TS = defs->TS;
    strcpy(chan->mode, defs->mode);

    strcpy(chan->lastjoin, "");
    chan->bans = NULL;
    chan->users = NULL;
    chan->modebuff = NULL;
    chan->next = ChannelList[cl_hash(chan->name)];
    ChannelList[cl_hash(chan->name)] = chan;

    if (*ToWord(2, buffer))
      strcat(buffer, ",");
    strcat(buffer, chan->name);

    if (defs->next == NULL || strlen(buffer) > 400)
    {
      strcat(buffer, "\n");
      sendtoserv(buffer);
      sprintf(buffer, ":%s JOIN ", mynick);
    }

    changemode(chan->name, "+o", mynick, 1);
    if (*chan->mode != '\0')
    {
      strcpy(buffer2, "-");
      strcat(buffer2, chan->mode);
      bounce(chan->name, buffer2, chan->TS);
    }
  }

  for (defs = DefChanList; defs != NULL; defs = defs->next)
    flushmode(defs->name);
}

void showusers(char *chan)
{
  auser *user;
  aluser *u;
  achannel *channel;
  int index = 0;

#ifdef DEBUG
  printf("ACK\n");
#endif

  for (index = 0; index < 1000; index++)
  {
    channel = ChannelList[index];

    while (channel)
    {
      if (match(channel->name, chan))
      {
	user = channel->users;

	while (user)
	{
	  u = user->N;
	  printf("%s: %s!%s@%s  %d\n",
	    channel->name, u->nick, u->username,
	    u->site, user->chanop);
	  user = user->next;
	}
	printf("MODE FOR %s is %s\n",
	  channel->name, channel->mode);
      }
      channel = channel->next;
    }
  }
}

void showchannels(void)
{
  achannel *channel;
  aban *b;
  modequeue *mode;
  int index;

#ifdef DEBUG
  printf("ACK\n");
#endif

  for (index = 0; index < 1000; index++)
  {
    channel = ChannelList[index];

    while (channel != NULL)
    {
      printf("%s TS: %ld on: %d amchanop: %d\n",
	channel->name,
	channel->TS, channel->on, channel->AmChanOp);
      b = channel->bans;
      printf("%s - banlist\n", channel->name);
      while (b != NULL)
      {
	printf("\t%s\n", b->pattern);
	b = b->next;
      }
      printf("%s - modebuff\n", channel->name);
      mode = channel->modebuff;
      while (mode != NULL)
      {
	printf("\t%d %s %s\n", mode->AsServer, mode->flag, mode->arg);
	mode = mode->next;
      }
      channel = channel->next;
    }
  }
}

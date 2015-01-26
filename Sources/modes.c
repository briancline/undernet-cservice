/* @(#)$Id: modes.c,v 1.10 1999/04/04 17:00:11 seks Exp $ */

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

static void AddFlag(char *, char, char *);
static void RemFlag(char *, char);

void setchanmode(char *line)
{
  register char *mode;
  register achannel *chan;

  if ((mode = strchr(line, ' ')) != NULL)
    *(mode++) = 0;
  chan = ToChannel(line);
  if (!chan)
    return;
  strcpy(chan->mode, mode + 1);		/*  why +1 ???? coz I don't want the + */
}

void AddFlag(char *string, char flag, char *arg)
{
  char newmode[15];
  char newarg[200];

  if (strchr(string, flag))
    RemFlag(string, flag);

  GetWord(0, string, newmode);
  *(newmode + strlen(newmode) + 1) = 0;
  *(newmode + strlen(newmode)) = flag;
  strcpy(newarg, ToWord(1, string));
  if (arg)
  {
    strcat(newarg, arg);
    strcat(newarg, " ");
  }

  sprintf(string, "%s %s", newmode, newarg);
}

void RemFlag(char *string, char flag)
{
  char newmode[15];
  char newarg[200];
  register char *ptr;
  register char *curr = newmode;
  register int count = 0;

  GetWord(0, string, newmode);
  strcpy(newarg, ToWord(1, string));

  while (*curr)
  {
    ptr = strchr("lk", *curr);
    if (*curr == flag)
    {
      strcpy(curr, curr + 1);
      if (ptr)
	strcpy(ToWord(count, newarg), ToWord(count + 1, newarg));
    }
    else
    {
      if (ptr)
	count++;
      curr++;
    }
  }
  sprintf(string, "%s %s", newmode, newarg);
}

int IsSet(char *channel, char flag, char *param)
{
  register achannel *chan;
  register char *setmode;
  char setflags[20];
  char setparam[80];
  register int out;
  register int i, j;

  chan = ToChannel(channel);
  if (chan == NULL)
    return 0;

  setmode = chan->mode;
  GetWord(0, setmode, setflags);

  j = 0;
  for (i = 0; setflags[i] != '\0' && setflags[i] != flag; i++)
  {
    if (setflags[i] == 'l' || setflags[i] == 'k')
      j++;
  }

  if (setflags[i] != '\0')
  {
    if (flag == 'l' || flag == 'k')
    {
      GetWord(j + 1, setmode, setparam);
      if (!strcmp(setparam, param))
	out = 1;
      else
	out = 0;
    }
    else
    {
      out = 1;
    }
  }
  else
  {
    out = 0;
  }
#ifdef DEBUG
  printf("IsSet(): +%c %s on %s (%d)\n", flag, param, channel, out);
#endif
  return (out);
}

void bounce(char *channel, char *change, time_t TS)
{
  char buffer[200];
  char arg[200];
  register char sign = '+';
  register auser *user;
  register achannel *chan;
  register int pos = 1;
  register int i;

  chan = ToChannel(channel);

  for (i = 0; change[i] != '\0' && change[i] != ' '; i++)
  {
    if (change[i] == '+')
      sign = '+';
    else if (change[i] == '-')
      sign = '-';
    else if (change[i] == 'o')
    {
      GetWord(pos++, change, arg);
      user = ToUser(channel, arg);
      if (user == NULL)
	continue;
      if (sign == '+')
      {
	if (!user->chanop)
	{
	  changemode(channel, "-o", arg, 1);
	  user->chanop = 0;
	}
      }
      else
      {
	if (user->chanop)
	{
	  /*
	     changemode(channel,"+o",arg,1);
	     user->chanop=1;
	   */
	  /* don't bounce -o */
	  user->chanop = 0;
	}
      }
    }
    else if (change[i] == 'v')
    {
      GetWord(pos++, change, arg);
      if (sign == '+')
	changemode(channel, "-v", arg, 1);
      else
	changemode(channel, "+v", arg, 1);
    }
    else if (change[i] == 'k')
    {
      GetWord(pos++, change, arg);
      if (sign == '+')
      {
	if (!IsSet(channel, 'k', arg))
	  changemode(channel, "-k", arg, 1);
      }
      else
      {
	if (IsSet(channel, 'k', arg))
	  changemode(channel, "+k", arg, 1);
      }
    }
    else if (change[i] == 'l')
    {
      pos++;
      if (sign == '+' && !IsSet(channel, 'l', arg))
	changemode(channel, "-l", "", 1);

    }
    else if ((IsSet(channel, change[i], "") && sign == '-') ||
      (!IsSet(channel, change[i], "") && sign == '+'))
    {
      sprintf(buffer, "%c%c",
	(sign == '+') ? '-' : '+',
	change[i]);
      changemode(channel, buffer, "", 1);
    }
  }
  AddEvent(EVENT_FLUSHMODEBUFF, now + MODE_DELAY, channel);
}


void ModeChange(char *source, char *channel, char *change)
{
  register char sign = '+';
  char arg[200];
  register char *ptr;
  register achannel *chan;
  register aluser *u;
  register int count = 1;
  int desync = 0;

#ifdef DEBUG
  printf("ModeChange(%s, %s, %s)\n", source, channel, change);
#endif
  chan = ToChannel(channel);
  /* if chan == NULL at this point.. we're dealing with a 
   * user mode change
   */
  if (chan != NULL)
    chan->lastact = now;

  u = ToLuser(source);
  if (chan == NULL && u == NULL)
  {
    return;	/* probably a lost MODE */
  }
  if (u == NULL)
  {
    /* This is a mode change from a server.. if it is not
     * a +b, then the last argument is a TS
     */
    for (ptr = change; *ptr != 'b' && *ptr != ' '; ptr++);
    if (*ptr != 'b' && strcasecmp(UWORLD_SERVER, source)
#ifdef UWORLD2
      && strcasecmp(UWORLD2_SERVER, source)
#endif
#ifdef FAKE_UWORLD
      && strcasecmp(UFAKE_SERVER, source)
#endif
      )
    {
      ptr = change + strlen(change);
      while (*ptr != ' ')
	ptr--;
      *(ptr++) = '\0';
      if (atol(ptr) > chan->TS && atol(ptr) != 0 && chan->on)
      {
	log(source);
	log(channel);
	log(change);
	log(ptr);
	bounce(channel, change, chan->TS);
	return;
      }
      if (atol(ptr) != 0)
	chan->TS = atol(ptr);
    }
  }

  while (*change && *change != ' ')
  {
    if (*change == '+')
    {
      sign = '+';
    }
    else if (*change == '-')
    {
      sign = '-';
    }
    else
    {
      if (sign == '+')
      {
	if (chan != NULL)
	{
	  ptr = strchr("blkov", *change);
	  if (ptr)
	  {
	    GetWord(count, change, arg);
	    if (strchr("lk", *change) && *arg)
	      AddFlag(chan->mode, *change, arg);
	    count++;
	  }
	  else
	  {
	    if (strchr("imntps", *change))
	      AddFlag(chan->mode, *change, NULL);
	  }
	}
	else
	{
	  /* user mode change (+) */
	  if (*change == 'o')
	  {
	    u->mode |= LFL_ISOPER;
#ifdef DEBUG
	    printf("%s is now IRCOP\n", source);
#endif
	  }
	}
      }
      else
      {
	if (chan != NULL)
	{
	  ptr = strchr("bkov", *change);
	  if (strchr("ilkmntps", *change))
	    RemFlag(chan->mode, *change);
	  if (ptr)
	  {
	    GetWord(count, change, arg);
	    count++;
	  }
	}
	else
	{
	  /* user mode change (-) */
	  if (*change == 'o')
	  {
	    u->mode &= ~LFL_ISOPER;
#ifdef DEBUG
	    printf("%s is no longer IRCOP\n", source);
#endif
	  }
	}
      }

/** other tests should go here **/

      if (chan != NULL)
      {
	if (*change == 'o')
	{
	  if (sign == '+')
	    onop(source, channel, arg);
	  else
	    ondeop(source, channel, arg, &desync);
	}
	else if (*change == 'b')
	{
	  if (sign == '+')
	    onban(source, channel, arg);
	  else
	    onunban(source, channel, arg);
	}
	else if (*change == 'l')
	{
	  if (sign == '+' && atoi(arg) < 2)
	  {
	    if (chan->on && chan->AmChanOp)
	    {
	      changemode(channel, "-l", "", 0);
	      RemFlag(chan->mode, *change);
	    }
	  }
	}
      }
    }
    change++;
    if (*change == ' ')
    {
      change = ToWord(count, change);
      count = 1;
    }
  }
  if (chan != NULL)
  {
    if (desync)
    {
      part("", channel, "");
      join("", channel, "");
    }
    CheckFlood(source, channel, 80);
    flushmode(channel);
  }
}

void onop(char *source, char *channel, char *target)
{
  register auser *user;
  register aluser *luser;
  register achannel *chan;
  char buffer[200];

  chan = ToChannel(channel);
  luser = ToLuser(source);

  user = ToUser(channel, target);

  if (!strcasecmp(mynick, target))
  {
    if (!chan)
    {
      log("ERROR: onop() channel not found!");
      return;
    }

    chan->AmChanOp = 1;
  }
  else
  {
#ifdef DEBUG
    printf("OP for %s on %s\n", target, channel);
#endif
    if (!user)
    {
      sprintf(buffer, "ERROR: onop() user not found (%s)",
	target);
      log(buffer);
      return;
    }

    user->chanop = 1;

    sprintf(buffer, "%s!%s@%s", user->N->nick, user->N->username, user->N->site);

    if (luser && chan->on && (chan->flags & CFL_NOOP))
    {
      sprintf(buffer, "NoOp MODE! deopping %s and %s",
	target, source);
      log(buffer);
      notice(target, replies[RPL_NOOP][chan->lang]);
      notice(source, replies[RPL_NOOP][chan->lang]);
      changemode(channel, "-o", target, 0);
      changemode(channel, "-o", source, 0);
      user->chanop = 0;
      user = ToUser(channel, source);
      if (user != NULL)
      {
	user->chanop = 0;
      }

    }
    else if (luser && chan->on && (chan->flags & CFL_STRICTOP) &&
      Access(channel, target) < OP_LEVEL)
    {
      sprintf(buffer, "StrictOp MODE! deopping %s and %s",
	target, source);
      log(buffer);
      notice(target, "Only authenticated users may be op in StrictOp mode");
      notice(source, "Only authenticated users may be op in StrictOp mode");
      changemode(channel, "-o", target, 0);
      user->chanop = 0;
      if (Access(channel, source) < OP_LEVEL)
      {
	changemode(channel, "-o", source, 0);
	user = ToUser(channel, source);
	if (user != NULL)
	{
	  user->chanop = 0;
	}
      }

    }
    else if (luser && chan->on && IsShit(channel, buffer, NULL, NULL) >= NO_OP_SHIT_LEVEL)
    {
      if (Access(channel, source) < ACCESS_BAN_PRIORITY)
      {
	sprintf(buffer, "%s is shitlisted (NO-OP LEVEL)!"
	  "Deopping %s and %s", target, target, source);
	log(buffer);
	notice(target, replies[RPL_CANTBEOP][chan->lang]);
	notice(source, replies[RPL_CANTBEOPPED][chan->lang]);
	changemode(channel, "-o", target, 0);
	changemode(channel, "-o", source, 0);
	user->chanop = 0;
	user = ToUser(channel, source);
	if (user != NULL)
	  user->chanop = 0;

	sprintf(buffer, "%s %d", source, SUSPEND_TIME_FOR_OPPING_A_SHITLISTED_USER);
	suspend("", channel, buffer);
      }
      else
      {
	unban(source, channel, target);
      }
    }
  }

/** further tests should be put here **/
#ifdef NICKSERV
  nserv_onop(channel, user);
#endif
}

void ondeop(char *source, char *channel, char *target, int *desync)
{
  char buffer[200];
  register auser *user1, *user2;
  register achannel *chan;
  register adeop *curr, *prec;
  register int i;

  chan = ToChannel(channel);
  if (!chan)
  {
    log("ERROR: ondeop() channel not found!");
    return;
  }

  user1 = ToUser(channel, source);
  user2 = ToUser(channel, target);

  if (chan->on && user1 == NULL)
  {	/* desync! */
    if (user2 != NULL)
    {
      user2->chanop = 0;
    }
    sprintf(buffer, "DESYNC detected! (%s deopped %s on %s)",
      source, target, channel);
    log(buffer);

/*              part("",channel,"");
   join("",channel,""); now doing this in ChangeMode */

    *desync = 1;

    return;
  }

  if (!strcasecmp(mynick, target))
  {
    if ((chan->flags & CFL_ALWAYSOP) &&
      Access(channel, source) >= ALWAYSOP_OVERRIDE_LEVEL)
    {
      chan->flags &= ~CFL_ALWAYSOP;
      sprintf(buffer, "AlwaysOp is turned off on %s (deopped by %s!%s@%s)",
	channel, user1->N->nick, user1->N->username, user1->N->site);
      log(buffer);
      notice(source, replies[RPL_ALWAYSOPWASACTIVE][chan->lang]);
    }
    if (chan->flags & CFL_ALWAYSOP)
    {
      if (user1 != NULL)
      {
	sprintf(buffer, "DEOPPED by %s!%s@%s (%d) while AlwaysOp active on %s",
	  user1->N->nick, user1->N->username, user1->N->site,
	  Access(channel, source), channel);
	log(buffer);
	broadcast(buffer, 0);
	i = IsShit(channel, source, NULL, NULL);
	notice(source, replies[RPL_ALWAYSOP][chan->lang]);
	switch (i)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	  log("First warning");
	  notice(source, replies[RPL_DEOPPED1ST][chan->lang]);
	  changemode(channel, "-o", source, 0);
	  flushmode(channel);
	  user1->chanop = 0;
	  break;

	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	  log("Second warning");
	  notice(source, replies[RPL_DEOPPED2ND][chan->lang]);
	  kick("", channel, source);
	  break;

	default:
	  notice(source, "I warned you!");
	  changemode(channel, "-o", source, 0);
	  flushmode(channel);
	  user1->chanop = 0;
	  sprintf(buffer, "%s!%s@%s %d",
	    user1->N->nick, user1->N->username,
	    user1->N->site,
	    DEOPME_SUSPEND_TIME);
	  suspend("", channel, buffer);
	  sprintf(buffer, "Suspended %s!%s@%s on %s for repeatedly deopping me",
	    user1->N->nick, user1->N->username, user1->N->site, channel);
	  SpecLog(buffer);
	  broadcast(buffer, 0);
	}
	sprintf(buffer, "%s %d %d *** DEOP WHILE ALWAYSOP ACTIVE ***",
	  source, DEOP_SHITLIST_TIME,
	  (i < 10) ? i + 5 :
	  DEOP_SHITLIST_LEVEL);
	AddToShitList("", channel, buffer, 0);
	chan->AmChanOp = 0;	/* to remove if I reop myself */
	GetOps(channel);
      }
    }
    else
    {
      chan->AmChanOp = 0;
      if (IsOpless(channel))
	onopless(channel);
    }
  }

  /* massdeop protection does NOT apply to the bot itself!
     if(!strcasecmp(source,mynick)) return; obsolete */

  /* No protection against server deop :/
     Hey! I'm not gonna /squit it just for doing a deop!! ;) */
  if (!user1)
    return;

  if (user2)
    user2->chanop = 0;

  chan = ToChannel(channel);
  if (!chan)
  {
    log("ERROR: ondeop() channel not found!");
    return;
  }

  /* If not on channel.. do NOT check for massdeop
   */
  if (!chan->on)
    return;

  curr = user1->deophist;
  prec = NULL;

  /* ok.. let's free the *old* deops from history */
  while (curr)
  {
    if (curr->time < (now - 15) || !strcasecmp(curr->nick, target))
    {
      if (prec)
      {
	prec->next = curr->next;
	TTLALLOCMEM -= sizeof(adeop);
	free(curr);
	curr = prec->next;
      }
      else
      {
	user1->deophist = curr->next;
	TTLALLOCMEM -= sizeof(adeop);
	free(curr);
	curr = user1->deophist;
      }
    }
    else
    {
      prec = curr;
      curr = curr->next;
    }
  }

  /* now store the deop in the user's deop history */
  curr = (adeop *) MALLOC(sizeof(adeop));
  strcpy(curr->nick, target);
  curr->time = now;
  curr->next = user1->deophist;
  user1->deophist = curr;

  /* now we count the number of deop in the user's history
     if it's equal to the max number of deops per 15 seconds
     interval.. activate the massdeop protection  */
  for (curr = user1->deophist, i = 0; curr; curr = curr->next, i++);

  if (i == chan->MassDeopPro && chan->MassDeopPro != 0 && chan->on)
  {
    sprintf(buffer, "MASSDEOP from %s on %s", source, channel);
    log(buffer);
    notice(source, "### MASSDEOP PROTECTION ACTIVATED ###");
    if (Access(channel, source) >= MASSDEOP_IMMUNE_LEVEL)
    {
      notice(source, "You are lucky your access is so high :P");
    }
    else
    {
      sprintf(buffer, "%s %d", source, MASSDEOP_SUSPEND_TIME);
      suspend("", channel, buffer);
      sprintf(buffer, "%s %d %d *** MASSDEOP ***", source,
	MASSDEOP_SHITLIST_TIME,
	MASSDEOP_SHITLIST_LEVEL);
      AddToShitList("", channel, buffer, 0);
      sprintf(buffer, "%s ### MASSDEOP PROTECTION ###", source);
      kick("", channel, buffer);
    }
  }
}

void onban(char *nick, char *channel, char *pattern)
{
#ifdef DEBUG
  printf("Wah! BAN FROM: %s\nON CHANNEL %s\nTARGET: %s\n",
    nick, channel, pattern);
#endif
  AddBan(channel, pattern);
}

void onunban(char *source, char *channel, char *pattern)
{
  /*register achannel *chan; */

  RemBan(channel, pattern);

  /* mode -b no longer removes the ban from X's banlist
     ** it has to be removed with the 'unban' command.
     chan=ToChannel(channel);
     if(chan!=NULL && chan->on)
     RemShitList(source,channel,pattern,1);
     * */
}

void AddBan(char *channel, char *pattern)
{
  register achannel *chan;
  register aban *theban;
  char buffer[200];

  chan = ToChannel(channel);
  if (chan == NULL)
  {
    sprintf(buffer, "ERROR: AddBan(): can't find %s", channel);
    log(buffer);
    return;
  }

  /* check if ban isn't already there */
  for (theban = chan->bans; theban != NULL; theban = theban->next)
  {
    if (!strncasecmp(theban->pattern, pattern, 79))
    {
      return;
    }
  }

  theban = (aban *) MALLOC(sizeof(aban));
  strncpy(theban->pattern, pattern, 79);
  theban->pattern[79] = '\0';
  theban->next = chan->bans;
  chan->bans = theban;
}


void RemBan(char *channel, char *pattern)
{
  register achannel *chan;
  register aban **b, *theban = NULL;

  chan = ToChannel(channel);
  b = &chan->bans;
  while (*b != NULL)
  {
    if (!strncasecmp((*b)->pattern, pattern, 79))
    {
      theban = *b;
      *b = theban->next;
      TTLALLOCMEM -= sizeof(aban);
      free(theban);
    }
    else
    {
      b = &(*b)->next;
    }
  }
#ifdef DEBUG
  if (theban == NULL)
  {
    printf("WARNING: RemBan(): pattern NOT found!!\n");
  }
#endif
}


void changemode(char *channel, char *flag, char *arg, int AsServer)
{
  register achannel *chan;
  register modequeue *mode, *curr;

#ifdef DEBUG
  printf("Queueing mode change for channel %s %s %s %d\n", channel, flag, arg, AsServer);
#endif

  chan = ToChannel(channel);
  if (!AsServer && (!chan || !chan->on))
    return;	/* not on the channel.. ignore */

  /* first, cancel previous contradicting mode changes..  
     ex.. mode #test +o-o+o-o+o .. the bot won't do that..  */

  mode = chan->modebuff;
  while (mode)
  {
    if (!strcmp(arg, mode->arg) && flag[1] == mode->flag[1] &&
      ((flag[0] == '+' && mode->flag[0] == '-') ||
	(flag[0] == '-' && mode->flag[0] == '+')))
    {
      if (mode->prev)
	mode->prev->next = mode->next;
      else
	chan->modebuff = mode->next;

      if (mode->next)
	mode->next->prev = mode->prev;

      curr = mode;
      TTLALLOCMEM -= sizeof(modequeue);
      free(curr);
    }
    else if (!strcmp(arg, mode->arg) && flag[1] == mode->flag[1] &&
      flag[0] == mode->flag[0])
      return;
    mode = mode->next;
  }

  curr = (modequeue *) MALLOC(sizeof(modequeue));
  strcpy(curr->arg, arg);
  strcpy(curr->flag, flag);
  curr->AsServer = AsServer;
  curr->next = NULL;

  for (mode = chan->modebuff; mode && mode->next; mode = mode->next);

  if (mode)
  {
    curr->prev = mode;
    mode->next = curr;
  }
  else
  {
    chan->modebuff = curr;
    curr->prev = NULL;
  }
}

void flushmode(char *channel)
{
  char buffer[500];
  register achannel *chan;
  register modequeue *mode, *tmp;
  char flags[20] = "";
  char args[500] = "";
  register char lastsign;
  register int AsServer;
  register int count = 0;

#ifdef DEBUG
  printf("Flushing mode change buffer.. for channel %s\n", channel);
#endif

  /* gotta pass thru the list twice:
   * the first time, check for Server mode changes
   * the second time, check for user mode changes
   */

  chan = ToChannel(channel);

  if (chan == NULL)
    return;

#ifdef FAKE_UWORLD
#define ASSERVERMAX 2
#else
#define ASSERVERMAX 1
#endif
  for (AsServer = ASSERVERMAX; AsServer >= 0; AsServer--)
  {
#undef ASSERVERMAX
    lastsign = '\0';
    mode = chan->modebuff;
    while (mode != NULL)
    {
      if (!strcmp(mode->flag, "-b"))
      {
	RemBan(channel, mode->arg);
      }
      else if (!strcmp(mode->flag, "+b"))
      {
	AddBan(channel, mode->arg);
      }
      if (AsServer == mode->AsServer)
      {
	if (mode->arg[0] != '\0')
	{
	  count++;
	  strcat(args, " ");
	  strcat(args, mode->arg);
	}
	if (lastsign != mode->flag[0])
	{
	  strcat(flags, mode->flag);
	  lastsign = mode->flag[0];
	}
	else
	{
	  strcat(flags, mode->flag + 1);
	}
	if (mode->prev)
	  mode->prev->next = mode->next;
	else
	  chan->modebuff = mode->next;
	if (mode->next)
	  mode->next->prev = mode->prev;
	tmp = mode;
	mode = mode->next;
	TTLALLOCMEM -= sizeof(modequeue);
	free(tmp);
      }
      else
      {
	mode = mode->next;
      }

      if (count == MAX_MODE_PER_LINE || !mode)
      {
	if (AsServer == 0 && chan->AmChanOp && *flags != '\0')
	{
	  sprintf(buffer, ":%s MODE %s %s%s\n",
	    mynick, channel, flags, args);
	  sendtoserv(buffer);
	}
	else if (AsServer == 1 && *flags != '\0')
	{
	  sprintf(buffer, ":%s MODE %s %s%s %ld\n",
	    SERVERNAME, channel,
	    flags, args, chan->TS);
	  sendtoserv(buffer);
	}
#ifdef FAKE_UWORLD
	else if (AsServer == 2 && Uworld_status == 1 && *flags != '\0')
	{
	  sprintf(buffer, ":%s MODE %s %s%s\n",
	    UFAKE_SERVER, channel,
	    flags, args);
	  sendtoserv(buffer);
	}
#endif
	count = *args = *flags = 0;
	lastsign = '+';
      }
    }
  }
}

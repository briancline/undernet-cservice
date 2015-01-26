/* @(#)$Id: opcom.c,v 1.19 2000/10/24 16:07:33 lgm Exp $ */

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

void CalmDown(char *source, char *chan, char *args)
{
  register aluser *user;
  register achannel *ch;
  char buffer[200], channel[80], global[] = "*";

  if ((user = ToLuser(source)) == NULL)
  {
    log("ERROR: CalmDown() can't locate user!");
    return;
  }

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

  if (!(user->mode & LFL_ISOPER) && Access(global, source) < XADMIN_LEVEL)
  {
    notice(source, "This command is reserved for IRC Operators");
    return;
  }

  if (!strcmp(channel, "*"))
  {
    notice(source, "SYNTAX: calmdown [channel]");
    return;
  }

  ch = ToChannel(channel);

  if (ch == NULL || !ch->on)
  {
    notice(source, replies[RPL_NOTONCHANNEL][L_DEFAULT]);
    return;
  }

  sprintf(buffer, ":%s WALLOPS :%s!%s@%s is asking me to calm down on %s\n",
    SERVERNAME, user->nick, user->username, user->site, ch->name);
  sendtoserv(buffer);

  sprintf(buffer, "%s!%s@%s is asking me to calm down on %s\n",
    user->nick, user->username, user->site, ch->name);
  broadcast(buffer, 0);

  sprintf(buffer, "CALMDOWN requested by %s!%s@%s on %s",
    user->nick, user->username, user->site, ch->name);
  SpecLog(buffer);

  /*
     part("",channel,"");
     RemChan("",channel,"");
   */
  ch->flags |= CFL_NOOP;
  massdeop(channel);
#ifdef CALMDOWNTOPIC
  topic("", channel, CALMDOWNTOPIC);
#endif
}


void OperJoin(char *source, char *chan, char *args)
{
  register aluser *user;
  register achannel *ch;
  char buffer[200], channel[80];

  if ((user = ToLuser(source)) == NULL)
  {
    log("ERROR: OperJoin() can't locate user!");
    return;
  }

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

  if (!(user->mode & LFL_ISOPER))
  {
    notice(source, "This command is reserved for IRC Operators");
    return;
  }

  if (!strcmp(channel, "*"))
  {
    notice(source, "SYNTAX: operjoin <channel>");
    return;
  }

  if (!IsReg(channel))
  {
    notice(source, "That channel is not registered");
    return;
  }

  ch = ToChannel(channel);

  if (ch != NULL && ch->on)
  {
    notice(source, "I am already on that channel");
    return;
  }

  sprintf(buffer, ":%s WALLOPS :%s!%s@%s is asking me to join channel %s\n",
    SERVERNAME, user->nick, user->username, user->site, channel);
  sendtoserv(buffer);

  sprintf(buffer, "%s!%s@%s is asking me join channel %s\n",
    user->nick, user->username, user->site, channel);
  broadcast(buffer, 0);

  sprintf(buffer, "OPERJOIN requested by %s!%s@%s on %s",
    user->nick, user->username, user->site, channel);
  SpecLog(buffer);

  join("", channel, "");
}


void OperPart(char *source, char *chan, char *args)
{
  register aluser *user;
  register achannel *ch;
  char buffer[200], channel[80];

  if ((user = ToLuser(source)) == NULL)
  {
    log("ERROR: OperPart() can't locate user!");
    return;
  }

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

  if (!(user->mode & LFL_ISOPER))
  {
    notice(source, "This command is reserved for IRC Operators");
    return;
  }

  if (!strcmp(channel, "*"))
  {
    notice(source, "SYNTAX: operpart [channel]");
    return;
  }

  ch = ToChannel(channel);

  if (ch == NULL || !ch->on)
  {
    notice(source, "I am not on that channel");
    return;
  }

  sprintf(buffer, ":%s WALLOPS :%s!%s@%s is asking me to leave channel %s\n",
    SERVERNAME, user->nick, user->username, user->site, channel);
  sendtoserv(buffer);

  sprintf(buffer, "%s!%s@%s is asking me to leave channel %s\n",
    user->nick, user->username, user->site, channel);
  broadcast(buffer, 0);

  sprintf(buffer, "OPERPART requested by %s!%s@%s on %s",
    user->nick, user->username, user->site, channel);
  SpecLog(buffer);

  part("", channel, "");
}


void ClearMode(char *source, char *chan, char *args)
{
  register aluser *user;
  register achannel *ch;
  register char *curr;
  register int i;
  char buffer[200], channel[80], arg[80];

  if ((user = ToLuser(source)) == NULL)
  {
    log("ERROR: ClearMode() can't locate user!");
    return;
  }

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
    notice(source, "SYNTAX: clearmode [channel]");
    return;
  }

  if (Access(channel, source) < CLEARMODE_LEVEL)
  {
    if (user->mode & LFL_ISOPER)
    {
      sprintf(buffer, ":%s WALLOPS :%s is using clearmode on %s\n",
	SERVERNAME, source, channel);
      sendtoserv(buffer);
    }
    else
    {
      ReplyNotAccess(source, channel);
      return;
    }
  }

  if ((ch = ToChannel(channel)) == NULL)
  {
    notice(source, "No such channel!");
    return;
  }
  if (!ch->on || !ch->AmChanOp)
  {
    notice(source, replies[RPL_NOTCHANOP][ch->lang]);
    return;
  }

  sprintf(buffer, "CLEARMODE on %s requested by %s!%s@%s",
    channel, user->nick, user->username, user->site);
  log(buffer);

  curr = ch->mode;
  i = 1;

  while (*curr && *curr != ' ')
  {
    if (strchr("kl", *curr))
    {
      GetWord(i++, ch->mode, arg);
      if (*curr == 'k')
      {
	changemode(channel, "-k", arg, 0);
      }
      else
      {
	changemode(channel, "-l", "", 0);
      }
    }
    else
    {
      sprintf(buffer, "-%c", *curr);
      changemode(channel, buffer, "", 0);
    }

    curr++;
  }

  ch->mode[0] = '\0';
  flushmode(channel);
}


void verify(char *source, char *arg)
{
  aluser *luser;
  char buffer[200], nick[80], global[] = "*", *oper;
  int acc;

  GetWord(0, arg, nick);
  if (!*nick)
  {
    notice(source, "SYNTAX: verify <nick>");
    return;
  }

  luser = ToLuser(nick);
  if (luser == NULL)
  {
    sprintf(buffer, "No such nick: %s", nick);
    notice(source, buffer);
    return;
  }

  if (luser->mode & LFL_ISOPER)
  {
    oper = " and an IRC operator";
  }
  else
  {
    oper = "";
  }

  acc = Access(global, nick);
  if (acc == 1000)
  {
    sprintf(buffer, "%s!%s@%s is my daddy%s",
      luser->nick, luser->username, luser->site, oper);
  }
  else if (acc > 900)
  {
    sprintf(buffer, "%s!%s@%s is an Official %s coder%s",
      luser->nick, luser->username, luser->site, VERIFY_ID, oper);
  }
  else if (acc == 900)
  {
    sprintf(buffer, "%s!%s@%s is my grandma%s",
      luser->nick, luser->username, luser->site, oper);
  }
  else if (acc == 800)
  {
    sprintf(buffer, "%s!%s@%s is a Co-ordinator of %s%s",
      luser->nick, luser->username, luser->site, VERIFY_ID, oper);
  }
  else if (acc > 500)
  {
    sprintf(buffer, "%s!%s@%s is an Official %s admin%s",
      luser->nick, luser->username, luser->site, VERIFY_ID, oper);
  }
  else if (acc > 0)
  {
    sprintf(buffer, "%s!%s@%s is an Official %s helper%s",
      luser->nick, luser->username, luser->site, VERIFY_ID, oper);
  }
  else if (acc < 0)
  {
    sprintf(buffer, "%s!%s@%s is a well-known TROUBLEMAKER%s",
      luser->nick, luser->username, luser->site, oper);
  }
  else if (luser->mode & LFL_ISOPER)
  {
    sprintf(buffer, "%s!%s@%s is an IRC operator",
      luser->nick, luser->username, luser->site);
  }
  else
  {
    sprintf(buffer, "%s!%s@%s is NOT an authenticated %s representative",
      luser->nick, luser->username, luser->site, VERIFY_ID);
  }
  notice(source, buffer);
}

#ifdef FAKE_UWORLD
void Uworld_switch(char *source, char *ch, char *args)
{
  char buffer[200];
  aluser *user, *user2;

  user = ToLuser(source);
  if (!user || !(user->mode & LFL_ISOPER))
  {
    notice(source, "This command is reserved to IRC Operators");
    return;
  }

  if (!strcasecmp(args, "ON"))
  {
    if (Uworld_status == 1)
    {
      sprintf(buffer, "%s is already active", UFAKE_NICK);
      notice(source, buffer);
    }
    else
    {
      if ((user2 = ToLuser(UFAKE_NICK)) != NULL &&
	!strcmp(user2->site, DEFAULT_HOSTNAME))
      {
	sprintf(buffer, "%s already present!", UFAKE_NICK);
	notice(source, buffer);
	return;
      }
      if (user2 != NULL)
      {
	onquit(UFAKE_NICK);
      }
      if (FindServer(&ServerList, UFAKE_SERVER) != NULL)
      {
	sprintf(buffer, "%s already present!", UFAKE_SERVER);
	notice(source, buffer);
	return;
      }
      sprintf(buffer, ":%s WALLOPS :%s activated %s\n",
	SERVERNAME, source, UFAKE_NICK);
      sendtoserv(buffer);
      Uworld_status = 1;
      IntroduceUworld();
      sprintf(buffer, "%s activated by %s!%s@%s",
	UFAKE_NICK, user->nick, user->username, user->site);
      log(buffer);
    }
  }
  else if (!strcasecmp(args, "OFF"))
  {
    if (Uworld_status == 0)
    {
      sprintf(buffer, "%s is not active", UFAKE_NICK);
      notice(source, buffer);
    }
    else
    {
      sprintf(buffer, ":%s WALLOPS :%s deactivated %s\n",
	SERVERNAME, source, UFAKE_NICK);
      sendtoserv(buffer);
      Uworld_status = 0;
      sprintf(buffer, "Deactivated by %s!%s@%s",
	user->nick, user->username, user->site);
      KillUworld(buffer);
      sprintf(buffer, "%s deactivated by %s!%s@%s",
	UFAKE_NICK, user->nick, user->username, user->site);
      log(buffer);
    }
  }
  else
  {
    notice(source, "SYNTAX: uworld [ON|OFF]");
  }
}

void parse_uworld_command(char *source, char *args)
{
  char command[80];
  GetWord(0, args, command);
  args = ToWord(1, args);

  if (!strcasecmp(command, "OPCOM"))
    Uworld_opcom(source, args);
  else if (!strcasecmp(command, "CLEARCHAN"))
    ClearChan(source, args);
  else if (!strcasecmp(command, "REOP"))
    Uworld_reop(source, args);
}

void Uworld_opcom(char *source, char *args)
{
  char buffer[512];
  char command[80];
  char channel[80];
  aluser *user;

  user = ToLuser(source);
  if (!user || !(user->mode & LFL_ISOPER))
  {
    notice(source, "This command is reserved to IRC Operators");
    return;
  }

  sprintf(buffer, "%s!%s@%s", user->nick, user->username, user->site);
  if (IsOperSuspended(buffer))
  {
    notice(source, "Sorry, Your Uworld access is suspended");
    return;
  }

  GetWord(0, args, command);
  GetWord(1, args, channel);
  args = ToWord(2, args);

  if (!strcasecmp(command, "MODE"))
  {
    if (*channel != '#')
    {
      sprintf(buffer, ":%s NOTICE %s :bad channel name\n",
	UFAKE_NICK, source);
      sendtoserv(buffer);
      return;
    }
    sprintf(buffer, ":%s WALLOPS :%s is using %s to: MODE %s %s\n",
      UFAKE_SERVER, source, UFAKE_NICK, channel, args);
    sendtoserv(buffer);
    sprintf(buffer, ":%s MODE %s %s\n", UFAKE_SERVER, channel, args);
    sendtoserv(buffer);
    sprintf(buffer, "OPCOM MODE %s %s (%s!%s@%s)",
      channel, args, user->nick, user->username, user->site);
    log(buffer);
    ModeChange(UFAKE_SERVER, channel, args);
  }
  else
  {
    sprintf(buffer, ":%s NOTICE %s :(yet) unsupported command (%s)\n",
      UFAKE_NICK, source, command);
    sendtoserv(buffer);
  }
}


void ClearChan(char *source, char *args)
{
  register aluser *user;
  register auser *chanuser;
  register achannel *ch;
  register char *curr;
  register aban *oneban;
  register int i;
  char buffer[200], channel[80], arg[80];

  if ((user = ToLuser(source)) == NULL)
  {
    log("ERROR: ClearMode() can't locate user!");
    return;
  }

  GetWord(0, args, channel);

  if (!strcmp(channel, "*"))
  {
    notice(source, "SYNTAX: clearmode [channel]");
    return;
  }

  if (!user->mode & LFL_ISOPER)
  {
    sprintf(buffer, ":%s NOTICE %s :This command is reserved to IRC Operators\n",
      UFAKE_NICK, source);
    sendtoserv(buffer);
    return;
  }

  sprintf(buffer, "%s!%s@%s", user->nick, user->username, user->site);
  if (IsOperSuspended(buffer))
  {
    notice(source, "Sorry, Your Uworld access is suspended");
    return;
  }

  if ((ch = ToChannel(channel)) == NULL)
  {
    sprintf(buffer, ":%s NOTICE %s :That channel does not exist\n",
      UFAKE_NICK, source);
    sendtoserv(buffer);
    return;
  }

  sprintf(buffer, ":%s WALLOPS :%s is using %s to: CLEARCHAN %s\n",
    UFAKE_SERVER, source, UFAKE_NICK, channel);
  sendtoserv(buffer);

  sprintf(buffer, "CLEARCHAN on %s requested by %s!%s@%s",
    channel, user->nick, user->username, user->site);
  log(buffer);

  chanuser = ch->users;
  while (chanuser != NULL)
  {
    if (chanuser->chanop)
    {
      changemode(channel, "-o", chanuser->N->nick, 2);
      chanuser->chanop = 0;
    }
    chanuser = chanuser->next;
  }

  curr = ch->mode;
  i = 1;
  while (*curr && *curr != ' ')
  {
    if (strchr("kl", *curr))
    {
      GetWord(i++, ch->mode, arg);
      if (*curr == 'k')
      {
	changemode(channel, "-k", arg, 2);
      }
      else
      {
	changemode(channel, "-l", "", 2);
      }
    }
    else
    {
      sprintf(buffer, "-%c", *curr);
      changemode(channel, buffer, "", 2);
    }

    curr++;
  }

  ch->mode[0] = '\0';

  for (oneban = ch->bans; oneban != NULL; oneban = oneban->next)
  {
    changemode(channel, "-b", oneban->pattern, 2);
  }
  flushmode(channel);
}

void Uworld_reop(char *source, char *channel)
{
  register aluser *user;
  char buffer[200];

  user = ToLuser(source);
  if (user != NULL && !strcasecmp(user->username, DEFAULT_USERNAME) &&
    !strcasecmp(user->site, DEFAULT_HOSTNAME))
  {
    sprintf(buffer, "%s called for REOP on %s", source, channel);
    log(buffer);
    changemode(channel, "+o", source, 2);
    flushmode(channel);
  }
}


void OperSuspend(char *source, char *args)
{
  FILE *in, *out;
  char Mask[200], strtimeout[80], global[] = "*", buffer[120], tmp[80],
  *mask = Mask, *ptr;
  time_t timeout;
  int add = 1, count = 0;

  if (Access(global, source) < 900)
  {
    notice(source, "You don't have access to that command");
    return;
  }

  GetWord(0, args, mask);
  GetWord(1, args, strtimeout);

  if (!*mask)
  {
    notice(source, "SYNTAX: opersuspend [+/-]<mask> [timeout]");
    return;
  }

  if (*mask == '+')
  {
    add = 1;
    mask++;
  }
  else if (*mask == '-')
  {
    add = 0;
    mask++;
  }

  if (add)
  {
    out = fopen("opersuspend.dat", "a");
    if (!out)
    {
      notice(source, "Can't open file opersuspend.dat :(");
      return;
    }

    timeout = atol(strtimeout);
    if (timeout <= 0)
      timeout = 1999999999;
    else
      timeout += now;

    fprintf(out, "%s %ld\n", mask, timeout);
    fclose(out);
    notice(source, "1 entry added");
  }
  else
  {
    in = fopen("opersuspend.dat", "r");
    if (!in)
    {
      notice(source, "Can't open file opersuspend.dat :(");
      return;
    }
    out = fopen("opersuspend.dat.new", "w");
    if (!in)
    {
      notice(source, "Can't open file opersuspend.dat.new :(");
      return;
    }

    while (fgets(buffer, 120, in) != NULL)
    {
      if ((ptr = strchr(buffer, '\n')) != NULL)
	*ptr = '\0';
      GetWord(0, buffer, tmp);
      if (strcasecmp(tmp, mask) && atol(ToWord(1, buffer)) > now)
	fprintf(out, "%s\n", buffer);
      else
	count++;
    }
    fclose(in);
    fclose(out);
    rename("opersuspend.dat.new", "opersuspend.dat");

    sprintf(buffer, "Removed %d  entry(ies)", count);
    notice(source, buffer);
  }
}


int IsOperSuspended(char *userhost)
{
  FILE *fp;
  char buffer[120], mask[200], *ptr;

  if ((fp = fopen("opersuspend.dat", "r")) == NULL)
    return 0;

  while (fgets(buffer, 120, fp) != NULL)
  {
    if ((ptr = strchr(buffer, '\n')) != NULL)
      *ptr = '\0';
    GetWord(0, buffer, mask);

    if (atol(ToWord(1, buffer)) > now && match(userhost, mask))
    {
      fclose(fp);
      return 1;
    }
  }
  fclose(fp);
  return 0;
}

#endif

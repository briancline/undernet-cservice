/* @(#)$Id: privmsg.c,v 1.22 2000/10/24 16:04:24 seks Exp $ */

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

void privmsg(char *source, char *target, char *body)
{
  register auser *user;
  register achannel *chan;
  register char *ptr;
  char global[] = "*";

#ifdef DEBUG
  printf("PRIVMSG from %s to %s\n", source, target);
#endif

  if ((*target == '#' || *target == '$') && strchr(target, '*'))
  {
#ifdef DEBUG
    printf("Received WALL... ignoring.. \n");
#endif
    return;
  }

  /* CTCP */
  if (body[1] == '\001' && (ptr = strchr(body + 2, '\001')) != NULL)
  {
    if (*target == '#' || *target == '&')
    {
      if (CheckFlood(source, target, strlen(ToWord(1, body + 1))))
	return;
    }

    *ptr = '\0';
    if (!IsIgnored(source) &&
      !CheckPrivateFlood(source, strlen(body), "CTCP-"))
      parse_ctcp(source, target, body + 2);
  }

  /* PRIVMSG TO A CHANNEL */
  else if (*target == '#' || *target == '&')
  {
    /* PRIVMSG #blah@channels.undernet.org ?? */
    if (!(chan = ToChannel(target)))
      return;
    chan->lastact = now;
    user = ToUser(target, source);
    if (!user)
      return;	/* not on channel.. happens if not +n */
    user->lastact = now;

    if (CheckFlood(source, target, strlen(ToWord(1, body + 1))))
      return;

    if (!strncmp(body + 1, COMMAND_PREFIX, strlen(COMMAND_PREFIX)))
      parse_command(source, target, target, body + strlen(COMMAND_PREFIX) + 1);

    /* PRIVATE PRIVMSG */
  }
  else
  {
    if (!IsIgnored(source) && !CheckPrivateFlood(source, strlen(body), "MSG-"))
    {
#ifdef FAKE_UWORLD
      if (!strcasecmp(target, UFAKE_NICK))
      {
	parse_uworld_command(source, body + 1);
      }
      else
#endif
	parse_command(source, target, global, body + 1);
    }
  }
}

void parse_command(char *source, char *target, char *channel, char *commandline)
{
  char buffer[1024];
  char command[80];
  char global[] = "*";
  register aluser *user;

  GetWord(0, commandline, command);

#ifdef DEBUG
  printf("PARSING COMMAND: %s\nCOMMAND: %s\nARGS: %s\nSOURCE: %s\n",
    commandline, command, ToWord(1, commandline), source);
#endif

  user = ToLuser(source);

  /* all commands must come from a user */
  if (user == NULL)
    return;

  if (strcasecmp(command, "pass") && strcasecmp(command, "login") &&
    strcasecmp(command, "newpass"))
  {
    sprintf(buffer, "COMMAND FROM %s!%s@%s on %s: %s",
      user->nick, user->username, user->site,
      channel, commandline);
    log(buffer);
  }
  else
  {
    sprintf(buffer, "COMMAND FROM %s!%s@%s on %s: %s XXXXXXX",
      user->nick, user->username, user->site,
      channel, command);
    log(buffer);
  }

  if (!strcmp(command, "showcommands"))
    showcommands(source, channel, ToWord(1, commandline));

  else if (!strcmp(command, "pass") || !strcmp(command, "login"))
    validate(source, target, ToWord(1, commandline));

  else if (!strcmp(command, "deauth"))
    DeAuth(source, channel, ToWord(1, commandline));

    else if (!strcmp(command, "die") && Access(global, source) >= LEVEL_DIE)
       quit(ToWord(1, commandline), 0);

      else if (!strcmp(command, "restart") && Access(global, source) >= LEVEL_DIE)
	 restart(ToWord(1, commandline));	/* added by Kev; restarts */

      else if (!strcmp(command, "search"))
	SearchChan(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "join"))
	join(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "part"))
	part(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "op"))
	op(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "deop"))
	deop(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "banlist"))
	showbanlist(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "kick"))
	kick(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "invite"))
	invite(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "topic"))
	topic(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "adduser"))
	AddUser(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "remuser"))
	RemoveUser(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "modinfo"))
	ModUserInfo(source, target, channel, ToWord(1, commandline));

      else if (!strcmp(command, "newpass"))
	ChPass(source, target, ToWord(1, commandline));

      else if (!strcmp(command, "set"))
	SetChanFlag(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "access"))
	showaccess(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "suspend"))
	suspend(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "unsuspend"))
	unsuspend(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "saveuserlist"))
	SaveUserList(source, channel);

      else if (!strcmp(command, "lbanlist"))
	ShowShitList(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "ban"))
	AddToShitList(source, channel, ToWord(1, commandline), 0);

      else if (!strcmp(command, "unban"))
	RemShitList(source, channel, ToWord(1, commandline), 0);

      else if (!strcmp(command, "cleanbanlist"))
	CleanShitList(source, ToWord(1, commandline));

      else if (!strcmp(command, "addchan"))
	AddChan(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "remchan"))
	RemChan(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "savedefs"))
	SaveDefs(source);

      else if (!strcmp(command, "loaddefs"))
	LoadDefs(source);

      else if (!strcmp(command, "saveshitlist"))
	SaveShitList(source, channel);

      else if (!strcmp(command, "loadshitlist"))
	LoadShitList(source);

      else if (!strcmp(command, "status"))
	showstatus(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "map"))
	showmap(source);

      else if (!strcmp(command, "help"))
	showhelp(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "chaninfo"))
	ShowChanInfo(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "motd"))
	showmotd(source);

      else if (!strcmp(command, "isreg"))
        isreg(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "core"))
	dumpcore(source);

#ifdef RUSAGE_SELF
      else if (!strcmp(command, "rusage"))
	show_rusage(source);
#endif

      else if (!strcmp(command, "showignore"))
	ShowIgnoreList(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "remignore"))
	AdminRemoveIgnore(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "calmdown"))
	CalmDown(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "operjoin"))
	OperJoin(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "operpart"))
	OperPart(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "clearmode"))
	ClearMode(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "purge"))
	purge(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "verify"))
	verify(source, ToWord(1, commandline));

#ifdef UPGRADE
      else if (!strcmp(command, "upgrade"))
	upgrade(source, ToWord(1, commandline));
#endif

      else if (!strcmp(command, "random"))
	RandomChannel(source);

      else if (!strcmp(command, "say"))
	Say(source, ToWord(1, commandline));

      else if (!strcmp(command, "servnotice"))
	ServNotice(source, ToWord(1, commandline));

      else if (!strcmp(command, "fuck"))
	notice(source, "This command is obsolete");

#ifdef DEBUG
      else if (!strcmp(command, "db"))
	db_test(source, channel, ToWord(1, commandline));
#endif

#ifdef DOHTTP
      else if (!strcmp(command, "rehash"))
	read_http_conf(source);
#endif
#ifdef FAKE_UWORLD
      else if (!strcmp(command, "uworld"))
	Uworld_switch(source, channel, ToWord(1, commandline));

      else if (!strcmp(command, "opersuspend"))
	OperSuspend(source, ToWord(1, commandline));
#endif

#ifdef DEBUG
      else if (!strcmp(command, "showusers"))
	showusers(ToWord(1, commandline));

      else if (!strcmp(command, "showchannels"))
	showchannels();
#endif

#ifdef NICKSERV
      else if (!strcmp(command, "nickserv"))
	nserv_nickserv(source, ToWord(1, commandline));

      else if (!strcmp(command, "addnick"))
	nserv_addnick(source, ToWord(1, commandline));

      else if (!strcmp(command, "remnick"))
	nserv_remnick(source, ToWord(1, commandline));

      else if (!strcmp(command, "addmask"))
	nserv_addmask(source, ToWord(1, commandline));

      else if (!strcmp(command, "remmask"))
	nserv_remmask(source, ToWord(1, commandline));

      else if (!strcmp(command, "nickinfo"))
	nserv_nickinfo(source, ToWord(1, commandline));

      else if (!strcmp(command, "identify"))
	nserv_identify(source, ToWord(1, commandline));

      else if (!strcmp(command, "ghost"))
	nserv_ghost(source, ToWord(1, commandline));

      else if (!strcmp(command, "nicknewpass"))
	nserv_nicknewpass(source, ToWord(1, commandline));

      else if (!strcmp(command, "nicknewemail"))
	nserv_nicknewemail(source, ToWord(1, commandline));
#endif
#ifdef DOHTTP
      else if (!strcmp(command, "dccme"))
	DccMe(source, ToWord(1, commandline));
#endif
}

void parse_ctcp(char *source, char *target, char *body)
{
  char func[80];
  char buffer[1024];
  char tmp[80];

  GetWord(0, body, func);
  body = ToWord(1, body);

  if (strcmp(func, "ACTION"))
  {
    sprintf(buffer, "CTCP %s from %s [%s]", func, source, body);
    log(buffer);
  }

  if (match(func, "PING"))
  {
    sprintf(buffer, "\001PING %s\001", body);
    notice(source, buffer);
  }
  else if (match(func, "TIME"))
  {
    strcpy(tmp, ctime(&now));
    *strchr(tmp, '\n') = '\0';
    sprintf(buffer, "\001TIME %s\001", tmp);
    notice(source, buffer);
  }
  else if (match(func, "ITIME"))
  {
    sprintf(buffer, "\001ITIME @%03ld\001",
      1000 * ((now + 3600) % 86400) / 86400);
    notice(source, buffer);
  }
  else if (match(func, "VERSION"))
  {
    sprintf(buffer, "\001VERSION %s\001", VERSION);
    notice(source, buffer);
  }
  else if (match(func, "GENDER"))
  {
    notice(source, "\001GENDER I'm a male bot! Are you a pretty young bottesse?\001");
  }
}

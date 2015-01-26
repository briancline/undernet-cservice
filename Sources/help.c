/* @(#)$Id: help.c,v 1.22 2000/10/02 00:55:13 lgm Exp $ */

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

static struct item
{
  char *name;
  int Access;
  char *file;
}
commands[] =
{
  {
    "showcommands", 0, "SHOWCOMMANDS"
  }
  ,
  {
    "status", STATUS_ACCESS, "STATUS"
  }
  ,
  {
    "join", LEVEL_JOIN, "JOIN"
  }
  ,
  {
    "invite", INVITE_LEVEL, "INVITE"
  }
  ,
  {
    "part", LEVEL_PART, "JOIN"
  }
  ,
  {
    "topic", TOPIC_LEVEL, "TOPIC"
  }
  ,
  {
    "kick", KICK_LEVEL, "KICK"
  }
  ,
  {
    "ban", BAN_LEVEL, "BAN"
  }
  ,
  {
    "unban", BAN_LEVEL, "BAN"
  }
  ,
  {
    "banlist", 0, "BAN"
  }
  ,
  {
    "lbanlist", 0, "BAN"
  }
  ,
  {
    "adduser", ADD_USER_LEVEL, "ADDUSER"
  }
  ,
  {
    "access", SHOW_ACCESS_LEVEL, "ACCESS"
  }
  ,
  {
    "verify", 0, "VERIFY"
  }
  ,
  {
    "remuser", REMOVE_USER_LEVEL, "ADDUSER"
  }
  ,
  {
    "modinfo", MOD_USERINFO_LEVEL, "ADDUSER"
  }
  ,
  {
    "purge", XADMIN_LEVEL, "XADMIN"
  }
  ,
  {
    "isreg", 0, "ISREG"
  }
  ,
  {
    "set", CH_FLOOD_LIMIT_LEVEL, "SET"
  }
  ,
  {
    "suspend", LEVEL_SUSPEND, "SUSPEND"
  }
  ,
  {
    "unsuspend", LEVEL_SUSPEND, "SUSPEND"
  }
  ,
  {
    "saveuserlist", SAVE_USERLIST_LEVEL, "SAVEUSERLIST"
  }
  ,
  {
    "addchan", SET_DEFAULT_LEVEL, "DEFCHAN"
  }
  ,
  {
    "remchan", SET_DEFAULT_LEVEL, "DEFCHAN"
  }
  ,
  {
    "savedefs", SAVE_DEFAULTS_LEVEL, "DEFCHAN"
  }
  ,
  {
    "loaddefs", LOAD_DEFAULT_LEVEL, "LOADDEFS"
  }
  ,
  {
    "savebanlist", SAVE_SHITLIST_LEVEL, "SAVEBANLIST"
  }
  ,
  {
    "loadbanlist", LOAD_SHITLIST_LEVEL, "LOADBANLIST"
  }
  ,
  {
    "pass", 0, "PASSWORD"
  }
  ,
  {
    "login", 0, "PASSWORD"
  }
  ,
  {
    "newpass", 0, "PASSWORD"
  }
  ,
  {
    "deauth", 0, "PASSWORD"
  }
  ,
  {
    "die", LEVEL_DIE, "DIE"
  }
  ,
  {
    "restart", LEVEL_DIE, "RESTART"
  }
  ,
  {
    "core", LEVEL_CORE, "CORE"
  }
  ,
  {
    "rusage", RUSAGE_ACCESS, "RUSAGE"
  }
  ,
#ifdef UPGRADE
  {
    "upgrade", LEVEL_UPGRADE, "UPGRADE"
  }
  ,
#endif
  {
    "op", OP_LEVEL, "OP"
  }
  ,
  {
    "deop", OP_LEVEL, "OP"
  }
  ,
  {
    "clearmode", CLEARMODE_LEVEL, "CLEARMODE"
  }
  ,
  {
    "map", 0, "MAP"
  }
  ,
  {
    "help", 0, "HELP"
  }
  ,
  {
    "motd", 0, "MOTD"
  }
  ,
  {
    "showignore", 0, "SHOWIGNORE"
  }
  ,
  {
    "remignore", XADMIN_LEVEL, "REMIGNORE"
  }
  ,
  {
    "chaninfo", 0, "CHANINFO"
  }
  ,
  {
    NULL, -1, NULL
  }
};
static int N = 0;

static void sort(void)
{
  struct item tmp;
  register int i, j, k;

  /* sort the command list using a simple bubble sort algo.
   * 1st sort the commands by decreasing access
   */
  for (i = 0; i < N - 1; i++)
  {
    for (j = N - 1; j > i; j--)
    {
      if (commands[j - 1].Access < commands[j].Access)
      {
	tmp = commands[j - 1];
	commands[j - 1] = commands[j];
	commands[j] = tmp;
      }
    }
  }

  /* Then, sort the commands by alphabetical order
   */
  i = k = 0;
  while (k < N - 1)
  {
    while (commands[k].Access == commands[i].Access)
      k++;

    while (i < k - 1)
    {
      for (j = k - 1; j > i; j--)
      {
	if (strcmp(commands[j - 1].name, commands[j].name) > 0)
	{
	  tmp = commands[j - 1];
	  commands[j - 1] = commands[j];
	  commands[j] = tmp;
	}
      }
      i++;
    }

    i = k;
  }
}

static void listinit(void)
{
  static int sorted = 0;

  /* count the number of commands
   */
  if (N == 0)
  {
    for (N = 0; commands[N].name != NULL; N++);
    N--;
  }

  /* sort the list if not already done
   */
  if (!sorted)
  {
    sort();
    sorted = 1;
  }
}

void showcommands(char *source, char *chan, char *args)
{
  char buffer[500];
  char channel[80];
  register aluser *luser;
  register int useraccess;
  register int i, j;

  if (*args == '#' || *args == '*')
    GetWord(0, args, channel);
  else
  {
    strcpy(channel, chan);
    GuessChannel(source, channel);
  }

  luser = ToLuser(source);
  if (!luser)
    return;

  if (!strcmp(channel, "*") && !IsValid(luser, channel))
  {
    notice(source, "SYNTAX: showcommands <channel>");
    return;
  }

  useraccess = LAccess(channel, luser);

  listinit();

  /* Now show the sorted command list to the user.
   */
  i = useraccess;
  sprintf(buffer, "Level%5d:", i);

  for (j = 0; j < N + 1; j++)
  {
    if (commands[j].Access < i)
    {
      if (strlen(buffer) > 13)
	notice(source, buffer);
      i = commands[j].Access;
      sprintf(buffer, "Level%5d:", i);
    }

    if (commands[j].Access <= useraccess)
    {
      strcat(buffer, " ");
      strcat(buffer, commands[j].name);
    }
  }

  notice(source, buffer);
}

void showhelp(char *source, char *chan, char *args)
{
  struct buffer_block *dyn = NULL;
  char buffer[512], word[80], *ptr;
  int i, l, index = 0, found = 0, file, linecount = 0;

  if (CurrentSendQ > HIGHSENDQTHRESHOLD)
  {
    notice(source, "Cannot process your request at this time. Try again later.");
    return;
  }

  GetWord(0, args, word);
  if (!*word)
    strcpy(word, "help");

  listinit();

  /* find the command index */
  for (i = 0; i <= N; i++)
  {
    if (!strcasecmp(word, commands[i].name))
    {
      found = 1;
      index = i;
      break;
    }
    else if (!strncasecmp(word, commands[i].name, strlen(word)))
    {
      found++;
      index = i;
    }
  }

  if (found > 1)
  {
    sprintf(buffer, "%s is ambiguous", word);
    notice(source, buffer);
    return;
  }
  else if (found == 0)
  {
    if (!strcasecmp(word, "INFO"))
    {
      sprintf(buffer, "%s/INFO", HELP_DIR);
    }
    else if (!strcasecmp(word, "FORM"))
    {
      sprintf(buffer, "%s/FORM", HELP_DIR);
    }
    else
    {
      sprintf(buffer, "No help on %s. Please use "
	"showcommands to get a list of commands "
	"available to you", word);
      notice(source, buffer);
      return;
    }
  }
  else
  {
    sprintf(buffer, "%s/%s", HELP_DIR, commands[index].file);
  }

  alarm(2);
  file = open(buffer, O_RDONLY);
  alarm(0);
  if (file < 0)
  {
    if (found)
      sprintf(buffer, "The help file for command %s "
	"is not available", commands[index].name);
    else
      sprintf(buffer, "This help file is not available");

    notice(source, buffer);
    return;
  }

  if (found)
  {
    sprintf(buffer,
      " HELP on %-20s               Minimum access: %4d ",
      commands[index].name, commands[index].Access);
    notice(source, buffer);
  }

  alarm(3);
  while ((l = read(file, buffer, 511)) > 0)
  {
    copy_to_buffer(&dyn, buffer, l);
  }
  alarm(0);
  close(file);

  while (dyn != NULL)
  {
    copy_from_buffer(&dyn, buffer, '\n', 199);
    if ((ptr = strchr(buffer, '\n')) != NULL)
      *ptr = '\0';
    else
      continue;
    if (*buffer == '\0')
      strcpy(buffer, " ");
    string_swap(buffer, 512, "$NICK", mynick);
    string_swap(buffer, 512, "$SERVER", SERVERNAME);
    notice(source, buffer);
    linecount++;
  }

  if (linecount > 0)
    CheckFloodFlood(source, linecount);
}

void showmotd(char *source)
{
  struct buffer_block *dyn = NULL;
  char buffer[512];
  int fd, l;

  alarm(3);
  fd = open(MOTD_FILE, O_RDONLY);
  if (fd < 0)
  {
    notice(source, "MOTD is empty");
    return;
  }

  if (CurrentSendQ > HIGHSENDQTHRESHOLD)
  {
    notice(source, "Cannot process your request at this time. Try again later.");
    return;
  }

  while ((l = read(fd, buffer, 511)) > 0)
  {
    copy_to_buffer(&dyn, buffer, l);
  }
  close(fd);
  alarm(0);

  while (dyn != NULL)
  {
    char *ptr;
    copy_from_buffer(&dyn, buffer, '\n', 199);
    if ((ptr = strchr(buffer, '\n')) != NULL)
      *ptr = '\0';
    else
      continue;
    if (*buffer == '\0')
      strcpy(buffer, " ");
    notice(source, buffer);
  }
}


static void
 chaninfo_callback(int *fd, off_t off, int action, void *hook1, void *hook2,
  dbuser * dbu, int count)
{
  char buffer[512];
  register adefchan *def;
  time_t t;

  if (count == 1 && dbu != NULL)
  {
    sprintf(buffer, "%s is registered by:", (char *)hook2);
    notice((char *)hook1, buffer);
  }

  if (dbu == NULL)
  {
    if (count == 0)
    {
      notice((char *)hook1, "That channel is not registered");
    }
    else
    {
      def = DefChanList;
      while (def && strcasecmp(def->name, (char *)hook2))
	def = def->next;

      if (def)
      {
	if (*def->topic)
	{
	  sprintf(buffer, "Desc: %s", def->topic);
	  notice((char *)hook1, buffer);
	}
	if (*def->url)
	{
	  sprintf(buffer, "URL: %s", def->url);
	  notice((char *)hook1, buffer);
	}
      }
    }
    free(hook1);
    free(hook2);
  }
  else if (dbu->access >= 500)
  {
    t = now - dbu->lastseen;

    sprintf(buffer, "%s (%s) last seen: %s ago",
      dbu->nick, dbu->match,
      time_remaining(t));
    notice((char *)hook1, buffer);
  }
}

void ShowChanInfo(char *source, char *chan, char *args)
{
  char channel[80], *hook1, *hook2;

  if (*args == '#')
    GetWord(0, args, channel);
  else
  {
    strcpy(channel, chan);
    GuessChannel(source, channel);
  }

  if (!strcmp(channel, "*"))
  {
    notice(source, "SYNTAX: chaninfo <channel>");
    return;
  }

  hook1 = (char *)malloc(strlen(source) + 1);
  strcpy(hook1, source);
  hook2 = (char *)malloc(strlen(channel) + 1);
  strcpy(hook2, channel);

  db_fetch(channel, DBGETALLCMP, "", NULL, 0, hook1, hook2, chaninfo_callback);
}


void isreg(char *source, char *chan, char *args)
{
  char channel[80], buffer[512];
  struct stat st;

  if (*args == '#')
    GetWord(0, args, channel);
  else
  {
    strcpy(channel, chan);
    GuessChannel(source, channel);
  }

  if (!strcmp(channel, "*"))
  {
    notice(source, "SYNTAX: isreg <channel>");
    return;
  }

  if (stat(make_dbfname(channel), &st) < 0)
    sprintf(buffer, "%s is not registered.",channel);
  else
    sprintf(buffer, "%s is registered.",channel);
  notice(source, buffer);
}


#ifdef DOHTTP
void http_show_help(http_socket * hsock, char *command)
{
  char buffer[512], buffer2[200];
  struct buffer_block *dyn = NULL;
  register char *ptr, *ptr2;
  register int i, index = 0, found = 0;
  int file, l;

  listinit();

  /* find the command index */
  for (i = 0; i <= N; i++)
  {
    if (!strcasecmp(command, commands[i].name))
    {
      found = 1;
      index = i;
      break;
    }
    else if (!strncasecmp(command, commands[i].name, strlen(command)))
    {
      found++;
      index = i;
    }
  }

  buffer[0] = '\0';

  sendto_http(hsock, HTTP_HEADER, command);
  sendto_http(hsock, HTTP_BODY);

  sendto_http(hsock, "<H1>HELP ON %s</H1>\n", (*command) ? command : "COMMANDS");
  sendto_http(hsock, "<PRE>\n");

  if (found > 1)
  {
    if (*command)
      sendto_http(hsock, "%s is ambiguous\n", command);
  }
  else if (found == 0)
  {
    if (!strcasecmp(command, "INFO"))
    {
      sprintf(buffer, "%s/INFO", HELP_DIR);
    }
    else if (!strcasecmp(command, "FORM"))
    {
      sprintf(buffer, "%s/FORM", HELP_DIR);
    }
    else
    {
      sendto_http(hsock, "No help on %s.\n", command);
    }
  }
  else
  {
    sprintf(buffer, "%s/%s", HELP_DIR, commands[index].file);
  }

  if (buffer[0] != '\0')
  {
    alarm(2);
    file = open(buffer, O_RDONLY);
    if (file < 0)
    {
      alarm(0);
      if (found)
	sendto_http(hsock, "The help file for command %s "
	  "is not available\n", commands[index].name);
      else
	sendto_http(hsock, "This help file is not available\n");
    }
    else
    {
      alarm(2);
      while ((l = read(file, buffer, 511)) > 0)
      {
	copy_to_buffer(&dyn, buffer, l);
      }
      alarm(0);
      close(file);

      while (dyn != NULL)
      {
	copy_from_buffer(&dyn, buffer, '\n', 199);
	if ((ptr = strchr(buffer, '\n')) == NULL)
	  continue;
	if ((ptr = strstr(buffer, "$NICK")) != NULL)
	{
	  *ptr = '\0';
	  sprintf(buffer2, "%s%s%s", buffer, mynick, ptr + 5);
	  strcpy(buffer, buffer2);
	}

	/* Some nasty char quoting required.. */
	ptr = buffer;
	ptr2 = buffer2;
	do
	{
	  if (*ptr == '<')
	  {
	    ptr2[0] = '&';
	    ptr2[1] = 'l';
	    ptr2[2] = 't';
	    ptr2[3] = ';';
	    ptr2 += 4;
	  }
	  else if (*ptr == '>')
	  {
	    ptr2[0] = '&';
	    ptr2[1] = 'g';
	    ptr2[2] = 't';
	    ptr2[3] = ';';
	    ptr2 += 4;
	  }
	  else
	  {
	    ptr2[0] = ptr[0];
	    ptr2++;
	  }
	}
	while (*(ptr++) != '\0');
	sendto_http(hsock, "%s", buffer2);
      }
    }
    sendto_http(hsock, "</PRE>\n");
  }
  else
  {	/* show list of commands */
    sendto_http(hsock, "</PRE>\n<H2>List of commands:</H2>\n");
    sendto_http(hsock, "<FONT SIZE=+1>\n");
    for (i = 0; i < N + 1; i++)
    {
      if (commands[i].Access <= 500)
	sendto_http(hsock, "<A HREF=\"/HELP/%s\">%s</A>\n",
	  commands[i].name, commands[i].name);
    }
    sendto_http(hsock, "</FONT><P>\n");
  }

  sendto_http(hsock, "<HR>%s\n", HTTP_FOOTER);
  sendto_http(hsock, "</BODY></HTML>\n");
  hsock->status = HTTP_ENDING;
}
#endif

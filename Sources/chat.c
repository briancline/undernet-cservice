/* @(#)$Id: chat.c,v 1.10 2000/09/09 15:38:13 seks Exp $ */

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

#ifdef DOHTTP

struct chat_user
{
  char user[10];
  char host[80];
  aluser *luser;
};


void chat_sendtoall(char *from, char *msg)
{
  register http_socket *scan = HttpList;
  while (scan != NULL)
  {
    if (scan->status == HTTP_CHAT && ((struct chat_user *)scan->hook)->luser)
    {
      if (from)
	sendto_http(scan, "<%s> %s\n", from, msg);
      else
	sendto_http(scan, "%s\n", msg);
    }
    scan = scan->next;
  }
}

static void login_ok(struct http_socket *hsock, RegUser * reg)
{
  char buffer[512], nick[15];
  struct chat_user *cu = (struct chat_user *)hsock->hook;
  register http_socket *scan;
  int count = 0;

  sprintf(buffer, "1 %ld %s %s %s", now, cu->user + 1, cu->host, VirtualServer.name);

  strcpy(nick, cu->user);

  while (ToLuser(nick) != NULL)
    sprintf(nick, "+%d%s", ++count, cu->user + 1);

  nick[9] = '\0';
  onnick(VirtualServer.name, nick, buffer);

  cu->luser = ToLuser(nick);

  cu->luser->valchan = (avalchan *) MALLOC(sizeof(avalchan));
  cu->luser->valchan->name = (char *)MALLOC(strlen(reg->channel) + 1);
  strcpy(cu->luser->valchan->name, reg->channel);
  cu->luser->valchan->reg = reg;
  reg->inuse++;
  reg->lastseen = now;
  reg->modified = 1;
  cu->luser->valchan->next = NULL;

  sendto_http(hsock, "Welcome to %s's chat line\n", mynick);

  count = 0;
  sprintf(buffer, "*** Currently on: ");

  for (scan = HttpList; scan != NULL; scan = scan->next)
  {
    if (scan->status != HTTP_CHAT || ((struct chat_user *)scan->hook)->luser == NULL)
      continue;
    if (strlen(buffer) > 70)
    {
      sendto_http(hsock, "%s\n", buffer);
      sprintf(buffer, "*** ");
    }
    strcat(buffer, ((struct chat_user *)scan->hook)->luser->nick + 1);
    strcat(buffer, " ");
  }
  sendto_http(hsock, "%s\n", buffer);

  sprintf(buffer, "*** JOIN: %s@%s", cu->luser->nick + 1, cu->host);
  chat_sendtoall(NULL, buffer);
}


static void
 chat_login_callback(int *fd, off_t off, int action, void *hook1, void *hook2,
  dbuser * dbu, int count)
{
  extern RegUser *load_dbuser(off_t, dbuser *);
  register RegUser *reg;
  struct http_socket *hsock = (struct http_socket *)hook1;
  struct chat_user *cu;
  char userhost[200];

  if (dbu != NULL)
  {
    cu = (struct chat_user *)hsock->hook;
    reg = load_dbuser(off, dbu);

    sprintf(userhost, "%s!%s@%s", cu->user, cu->user + 1, cu->host);

    if (reg != NULL && *reg->match == '+' && match(userhost, reg->match))
    {
#ifdef DEBUG
      printf("login_ok: %s %s\n", userhost, reg->match);
#endif
      login_ok(hsock, reg);
    }
    else
    {
      sendto_http(hsock, "Authentication failed!\n");
      hsock->status = HTTP_ENDING;
    }
  }
  else
  {
    if (count == 0)
    {
      sendto_http(hsock, "Authentication failed!\n");
      hsock->status = HTTP_ENDING;
      hsock->dbio = 0;
    }
  }
}


void chat_login(struct http_socket *hsock, char *user, char *password)
{
  char userhost[200], channel[] = "*", buffer[200];
  struct chat_user *cu;
  register RegUser *ruser;

  cu = (struct chat_user *)malloc(sizeof(struct chat_user));
  sprintf(buffer, "+%s", user);
  strncpy(cu->user, buffer, 9);
  cu->user[9] = '\0';
  strcpy(cu->host, inet_ntoa(hsock->peer.sin_addr));
  cu->luser = NULL;
  hsock->hook = (void *)cu;

  sprintf(userhost, "%s!%s@%s", cu->user, cu->user + 1, cu->host);

  ruser = UserList[ul_hash(channel)];
  while (ruser != NULL)
  {
    if (!strcasecmp(ruser->channel, channel) &&
      *ruser->match == '+' && match(userhost, ruser->match) &&
      !strcmp(ruser->passwd, password))
      break;
    ruser = ruser->next;
  }

  if (ruser == NULL)
  {	/* ok.. not in memory.. send db query */
    db_fetch(channel, DBGETUHPASS, userhost, password, 0,
      hsock, NULL, chat_login_callback);
    hsock->dbio = 1;
  }
  else
  {
#ifdef DEBUG
    printf("login_ok: %s %s\n", userhost, ruser->match);
#endif
    login_ok(hsock, ruser);
  }
}


void chat_notice(char *user, char *msg)
{
  register http_socket *scan = HttpList;
  register struct chat_user *cu;

#ifdef DEBUG
  printf("chat: ->%s %s\n", user, msg);
#endif

  while (scan != NULL)
  {
    cu = (struct chat_user *)scan->hook;
    if (scan->status == HTTP_CHAT && cu->luser &&
      !strcasecmp(cu->luser->nick, user))
    {
      sendto_http(scan, "-%s- %s\n", mynick, msg);
      break;
    }
    scan = scan->next;
  }
}


void parse_chat(struct http_socket *hsock, char *line)
{
  char buffer[200], global[] = "*", *ptr;

  if (hsock == NULL || hsock->hook == NULL ||
    ((struct chat_user *)hsock->hook)->luser == NULL)
    return;

  if ((ptr = strpbrk(line, "\r\n")) != NULL)
    *ptr = '\0';

#ifdef DEBUG
  printf("chat: <%s> %s\n", ((struct chat_user *)hsock->hook)->luser->nick + 1, line);
#endif

  if ((*line == '/') || (*line == '.'))
  {
    sprintf(buffer, "%s@%s", mynick, SERVERNAME);
    parse_command(((struct chat_user *)hsock->hook)->luser->nick,
      buffer, global, line + 1);
  }
  else
    chat_sendtoall(((struct chat_user *)hsock->hook)->luser->nick + 1, line);
}


void chat_close(http_socket * hsock, char *comment)
{
  char buffer[512];
  struct chat_user *cu = (struct chat_user *)hsock->hook;

  if (cu->luser)
  {
    sprintf(buffer, "*** LEAVE: %s@%s", cu->luser->nick + 1, cu->host);
    chat_sendtoall(NULL, buffer);
    onquit(cu->luser->nick);
  }
}


void DccMe(char *source, char *arg)
{
  char global[] = "*", buffer[512];
  if(Access(global, source) >= 600)
  {
    unsigned long addr = inet_addr(BINDADDR);
    addr = ntohl(addr);
    sprintf(buffer, ":%s PRIVMSG %s :\001DCC CHAT chat %u %u\001\n",
            mynick, source, addr, HTTP_PORT);
    sendtoserv(buffer);
  }
}
#endif

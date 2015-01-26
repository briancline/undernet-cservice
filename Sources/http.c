/* @(#)$Id: http.c,v 1.31 2000/10/26 03:00:35 seks Exp $ */

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
#include <stdarg.h>

void cold_save_one(RegUser * reg);

struct http_deny
{
  struct in_addr addr;
  struct http_deny *next;
}
*HttpDeny = NULL;

#ifdef DEBUG
static void show_list(void)
{
  register http_socket *tmp;
  register http_file_pipe *fpipe;

  printf("http fd list: ");
  for (tmp = HttpList; tmp != NULL; tmp = tmp->next)
  {
    printf("%d ", tmp->fd);
    switch (tmp->status)
    {
    case HTTP_ERROR:
      printf("(ERROR) ");
      break;
    case HTTP_LISTEN:
      printf("(LISTEN) ");
      break;
    case HTTP_ACTIVE:
      printf("(ACTIVE) ");
      break;
    case HTTP_ENDING:
      printf("(ENDING) ");
      break;
    case HTTP_RECV_POST:
      printf("(POST) ");
      break;
    case HTTP_PIPE:
      printf("(PIPE) ");
      break;
    default:
      printf("(----) ");
      break;
    }
  }
  printf("\nFiles: ");

  for (fpipe = FilePipes; fpipe != NULL; fpipe = fpipe->next)
  {
    printf("%d (FILE->%d) ", fpipe->fd, fpipe->hsock->fd);
  }
  printf("\n");
}
#endif

void http_log(char *fmt,...)
{
  char buffer[1024], date[80];
  va_list ap;
  int fd;

  strcpy(date, ctime(&now));
  *strchr(date, '\n') = '\0';

  va_start(ap, fmt);
  alarm(2);
  if ((fd = open(HTTP_LOG, O_WRONLY | O_CREAT | O_APPEND, 0600)) >= 0)
  {
    alarm(0);
    sprintf(buffer, "%s: ", date);
    alarm(2);
    write(fd, buffer, strlen(buffer));
    alarm(0);
    vsprintf(buffer, fmt, ap);
    alarm(2);
    write(fd, buffer, strlen(buffer));
    write(fd, "\n", 1);
    alarm(0);
    close(fd);
  }
  alarm(0);
  va_end(ap);
}


void quote_html(char *in, char *out)
{
  do
  {
    if (*in == '<')
    {
      out[0] = '&';
      out[1] = 'l';
      out[2] = 't';
      out[3] = ';';
      out += 4;
    }
    else if (*in == '>')
    {
      out[0] = '&';
      out[1] = 'g';
      out[2] = 't';
      out[3] = ';';
      out += 4;
    }
    else if (*in == '&')
    {
      out[0] = '&';
      out[1] = 'a';
      out[2] = 'm';
      out[3] = 'p';
      out[4] = ';';
      out += 5;
    }
    else
    {
      out[0] = in[0];
      out++;
    }
  }
  while (*(in++) != '\0');
}

static http_socket *new_struct(void)
{
  register http_socket *tmp;

  tmp = (http_socket *) MALLOC(sizeof(http_socket));
#ifdef DEBUG
  printf("New http_socket struct at %p\n", tmp);
#endif
  tmp->next = NULL;
  tmp->inbuf = NULL;
  tmp->outbuf = NULL;
  tmp->hook = NULL;
  tmp->fd = -1;
  tmp->status = -1;
  tmp->dbio = 0;
  tmp->TS = now;
  tmp->since = now;

  return tmp;
}

static void add_struct(http_socket * new)
{
  new->next = HttpList;
  HttpList = new;
#ifdef DEBUG
  show_list();
#endif
}

void remove_httpsock(http_socket * old)
{
  extern void chat_quit(struct http_socket *hsock);
  register http_socket *tmp = HttpList;

  if (old->dbio)
    return;	/* can't free yet.. there are other pointers
		   * the that structure */

  if (old->inbuf)
    zap_buffer(&old->inbuf);
  if (old->outbuf)
    zap_buffer(&old->outbuf);

  if (old->hook)
    free(old->hook);

  if (tmp == old)
  {
    HttpList = HttpList->next;
    free(old);
  }
  else
  {
    while (tmp->next != old)
    {
      tmp = tmp->next;
      if (tmp == NULL)
      {
	http_log("ERROR: remove_struct() tmp->next==NULL");
	return;
      }
    }
    tmp->next = old->next;
    free(old);
  }
#ifdef DEBUG
  show_list();
#endif
}

void open_http(void)
{
  register http_socket *new;
  struct sockaddr_in myaddr;
  int opt = 1;

  new = new_struct();
  new->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (new->fd < 0)
  {
    http_log("ERROR: open_http() socket() failed!");
    http_log("%d: %s", errno, sys_errlist[errno]);
    free(new);
    return;
  }

#ifdef SO_REUSEADDR
  if (setsockopt(new->fd, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof(opt)) < 0)
  {
    http_log("ERROR: open_http() setsockopt() failed!");
    http_log("%d: %s", errno, sys_errlist[errno]);
    close(new->fd);
    free(new);
    return;
  }
#endif
  if (fcntl(new->fd, F_SETFL, O_NONBLOCK) < 0)
  {
    http_log("ERROR: open_http() fcntl() failed!");
    http_log("%d: %s", errno, sys_errlist[errno]);
    close(new->fd);
    free(new);
    return;
  }

  memset(&myaddr, 0, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_port = htons(HTTP_PORT);
#ifdef BINDADDR
  myaddr.sin_addr.s_addr = inet_addr(BINDADDR);
#else
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
  if (bind(new->fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
  {
    http_log("ERROR: open_http() bind() failed!");
    http_log("%d: %s", errno, sys_errlist[errno]);
    close(new->fd);
    free(new);
    /*return; */
    /* Can't bind to local port.. exit! */
    exit(0);
  }

  if (listen(new->fd, 10) < 0)
  {
    http_log("ERROR: open_http() listen() failed!");
    http_log("%d: %s", errno, sys_errlist[errno]);
    close(new->fd);
    free(new);
    return;
  }

  new->status = HTTP_LISTEN;
  add_struct(new);
}

void read_http_conf(char *source)
{
  register struct http_deny *tmp;
  char buffer[80], ip[80], global[] = "*";
  FILE *fp;

  if (*source && Access(global, source) < MASTER_ACCESS)
  {
    ReplyNotAccess(source, global);
    return;
  }

  while ((tmp = HttpDeny) != NULL)
  {
    HttpDeny = HttpDeny->next;
    free(tmp);
  }

  if ((fp = fopen(HTTP_DENY, "r")) != NULL)
  {
    while (fgets(buffer, 79, fp) != NULL)
    {
      if (*buffer == '#' || *buffer == '\n' || *buffer == '\0')
	continue;
      sscanf(buffer, "%s\n", ip);
      tmp = (struct http_deny *)MALLOC(sizeof(struct http_deny));
      tmp->addr.s_addr = inet_addr(ip);
      tmp->next = HttpDeny;
      HttpDeny = tmp;
    }
    fclose(fp);
    if (*source)
      notice(source, "done.");
  }
  else
  {
    if (*source)
      notice(source, "Can't open file: " HTTP_DENY);
  }
}

static int check_http_deny(struct in_addr *addr)
{
  char tmp[5];
  register struct http_deny *scan = HttpDeny;

  tmp[4] = '\0';
  while (scan != NULL)
  {
    /* Not sure which one is faster...            */
    /* tmp[0]=((char *)&(scan->addr.s_addr))[0]; */
    /* tmp[1]=((char *)&(scan->addr.s_addr))[1]; */
    /* tmp[2]=((char *)&(scan->addr.s_addr))[2]; */
    /* tmp[3]=((char *)&(scan->addr.s_addr))[3]; */
    memcpy((void *)tmp, (void *)&(scan->addr.s_addr), 4);
    if (!memcmp((void *)tmp, (void *)&(addr->s_addr), strlen(tmp)))
      return 1;
    scan = scan->next;
  }
  return 0;
}

void http_accept(int sock)
{
  register http_socket *new;
  int size;
  struct in_addr addr;

  new = new_struct();

  size = sizeof(new->peer);
  new->fd = accept(sock, (struct sockaddr *)&new->peer, &size);
  if (size != sizeof(new->peer))
  {
    http_log("WARNING: uh huh.. size changed in accept()!"
      " %d != %d", size, (int)sizeof(new->peer));
  }
  if (new->fd < 0)
  {
    http_log("ERROR: http_accept() accept() failed!");
    http_log("%d: %s", errno, sys_errlist[errno]);
    free(new);
    return;
  }

  if (fcntl(new->fd, F_SETFL, O_NONBLOCK) < 0)
  {
    http_log("ERROR: open_accept() fcntl() failed!");
    http_log("%d: %s", errno, sys_errlist[errno]);
    close(new->fd);
    free(new);
    return;
  }

  new->status = HTTP_ACTIVE;
  add_struct(new);

  addr.s_addr = new->peer.sin_addr.s_addr;
  if (check_http_deny(&new->peer.sin_addr))
  {
    http_log("Connection refused (%s)", inet_ntoa(addr));
    sendto_http(new, HTTP_HEADER, "You're banned!");
    sendto_http(new, HTTP_BODY);
    sendto_http(new, "<H1>Due to abuse from users of your site "
      "(%s), connections are no longer accepted.</H1>\n", inet_ntoa(addr));
    sendto_http(new, "<HR>%s\n</BODY>\n", HTTP_FOOTER);
    new->status = HTTP_ENDING;
    return;
  }

  http_log("[%d] HTTP CONNECTION (%s)", new->fd, inet_ntoa(addr));

  if (new->fd >= MAX_CONNECTIONS)
  {
    new->status = HTTP_ENDING;
    sendto_http(new, HTTP_HEADER "\n", "Too many connections!");
    http_log("ERROR: reached MAX_CONNECTIONS!!");
  }
}


static void
 http_userlist_callback(int *fd, off_t off, int action, void *hook1, void *hook2,
  dbuser * dbu, int count)
{
  time_t t;
  int days, hours, mins, secs;

  if (dbu == NULL)
  {
    if (count == 0)
      sendto_http((http_socket *) hook1,
	"Userlist is empty on %s!<p>\n", mynick);
    sendto_http((http_socket *) hook1, "<HR>%s\n", HTTP_FOOTER);
    sendto_http((http_socket *) hook1, "</BODY>\n");
    ((http_socket *) hook1)->status = HTTP_ENDING;
    ((http_socket *) hook1)->dbio = 0;
    free(hook2);
  }
  else
  {
    sendto_http((http_socket *) hook1,
      "USER: %s (%s) ACCESS: %d<br>\n",
      dbu->nick, dbu->match, dbu->access);
    sendto_http((http_socket *) hook1,
      "CHANNEL: %s -- AUTOOP: %s<br>\n",
      dbu->channel,
      (dbu->flags & UFL_AUTOOP) ? "ON" : "OFF");
    if (dbu->suspend > now)
    {
      sendto_http((http_socket *) hook1,
	"SUSPEND EXP: %s<br>\n",
	time_remaining(dbu->suspend - now));
    }
    t = now - dbu->lastseen;
    days = (int)t / 86400;
    t %= 86400;
    hours = (int)t / 3600;
    t %= 3600;
    mins = (int)t / 60;
    t %= 60;
    secs = (int)t;

    if (days > 0)
      sendto_http((http_socket *) hook1,
	"LAST SEEN: %d days, %02d:%02d:%02d ago<br>\n",
	days, hours, mins, secs);
    else
      sendto_http((http_socket *) hook1,
	"LAST SEEN: %02d:%02d:%02d ago<br>\n",
	hours, mins, secs);
    if (dbu->modif[0] == '\0')
      sendto_http((http_socket *) hook1,
	"LAST MODIF: unknown<p>\n");
    else
      sendto_http((http_socket *) hook1,
	"LAST MODIF: %s<p>\n", dbu->modif);
  }
}

static void http_show_userlist(http_socket * hsock, char *channel, char *protocol)
{
  char date[80];
  struct tm *timeptr;
  int *hook;

  timeptr = gmtime(&now);
  sprintf(date, "%sUTC", asctime(timeptr));
  *strchr(date, '\n') = ' ';

  sendto_http(hsock, "HTTP/1.0 200 Document follows%c", 10);
  sendto_http(hsock, "Date: %s%c", date, 10);
  sendto_http(hsock, "Server: CS/1.0%c", 10);
  sendto_http(hsock, "Content-type: text/html%c%c", 10, 10);

  sendto_http(hsock, HTTP_HEADER, "User list");
  sendto_http(hsock, HTTP_BODY);
  sendto_http(hsock, "\n<H1>User list for channel %s</H1>\n", channel);
  sendto_http(hsock, "%s<p>\n", date);

  if (!strcmp(channel, "*"))
  {
    sendto_http(hsock, "Userlist is empty on %s!<p>\n", mynick);
    sendto_http(hsock, "<HR>%s\n", HTTP_FOOTER);
    sendto_http(hsock, "</BODY>\n");
    hsock->status = HTTP_ENDING;
    hsock->dbio = 0;
  }
  else
  {
    if (!strcmp(channel, "secret_admin_list"))
    {
        strcpy(channel, "*"); /* Enable us to see the * userlist via the web */
    }
    hook = (int *)malloc(sizeof(int));
    *hook = 0;

    db_fetch(channel, DBGETALLCMP, "", NULL, 0, hsock, hook,
      http_userlist_callback);
    hsock->dbio = 1;
  }
}


static void http_show_banlist(http_socket * hsock, char *channel, char *protocol)
{
  char date[80];
  register ShitUser *curr;
  struct tm *timeptr;
  int found = 0;

  timeptr = gmtime(&now);
  sprintf(date, "%sUTC", asctime(timeptr));
  *strchr(date, '\n') = ' ';

  sendto_http(hsock, "HTTP/1.0 200 Document follows%c", 10);
  sendto_http(hsock, "Date: %s%c", date, 10);
  sendto_http(hsock, "Server: CS/1.0%c", 10);
  sendto_http(hsock, "Content-type: text/html%c%c", 10, 10);

  sendto_http(hsock, HTTP_HEADER, "Ban list");
  sendto_http(hsock, HTTP_BODY);
  sendto_http(hsock, "<H1>Ban list for channel %s</H1>\n", channel);
  sendto_http(hsock, "%s<p>\n", date);

  sendto_http(hsock, HTTP_BAN_DISCLAIMER);
  curr = ShitList[sl_hash(channel)];
  while (curr)
  {
    if (!strcasecmp(channel, curr->channel))
    {
      found++;
      sendto_http(hsock, "%s %s Level: %d<br>\n",
	curr->channel, curr->match, curr->level);
      sendto_http(hsock, "ADDED BY: %s (%s)<br>\n", curr->from,
	(*curr->reason) ? curr->reason : "No reason given");

      timeptr = gmtime(&curr->time);
      sprintf(date, "%sUCT", asctime(timeptr));
      *strchr(date, '\n') = ' ';
      sendto_http(hsock, "SINCE: %s<br>\n", date);

      sendto_http(hsock, "EXP: %s<p>\n",
	time_remaining(curr->expiration - now));
    }
    curr = curr->next;
  }

  if (!found)
  {
    sendto_http(hsock, "Ban list is empty!<p>\n");
  }

  sendto_http(hsock, "<HR>%s\n", HTTP_FOOTER);
  sendto_http(hsock, "</BODY>\n");
  hsock->status = HTTP_ENDING;
}


static void http_show_list(http_socket * hsock, char *keylist)
{
  char date[80], top[512], *tok[16];
  register adefchan *def;
  int found = 0, i = 0;
  struct tm *timeptr;

  timeptr = gmtime(&now);
  sprintf(date, "%sUTC", asctime(timeptr));
  *strchr(date, '\n') = ' ';

  sendto_http(hsock, "HTTP/1.0 200 Document follows\n\n");

  sendto_http(hsock, HTTP_HEADER, "Channel list");
  sendto_http(hsock, HTTP_BODY);
  if (*keylist)
  {
    sendto_http(hsock, "<H1>Channel search result for [%s]</H1>\n", keylist);
  }
  else
  {
    sendto_http(hsock, "<H1>Channel search result: Whole list</H1>\n");
  }
  sendto_http(hsock, "%s<p>\n", date);

  tok[0] = strtok(keylist, " ");
  while (i < 15 && (tok[++i] = strtok(NULL, " ")) != NULL);
  tok[i] = NULL;

  def = DefChanList;
  while (def)
  {
    if ((!*keylist || key_match(def->name, tok) ||
	key_match(def->topic, tok)) &&
      !IsSet(def->name, 's', "") && !IsSet(def->name, 'p', ""))
    {
      found = 1;
      quote_html(def->topic, top);
      if (*def->url)
	sendto_http(hsock, "<A HREF=\"%s\" TARGET=\"_top\">%s</A> %s<br>\n",
	  def->url, def->name, top);
      else
	sendto_http(hsock, "%s %s<br>\n", def->name, top);
    }
    def = def->next;
  }

  if (!found)
  {
    sendto_http(hsock, "No match!<p>\n");
  }

  sendto_http(hsock, "<P><P><HR>%s\n", HTTP_FOOTER);
  hsock->status = HTTP_ENDING;
}


static void
 http_chaninfo_callback(int *fd, off_t off, int action, void *hook1, void *hook2,
  dbuser * dbu, int count)
{
  register adefchan *def;
  time_t t;

  if (count == 1 && dbu != NULL)
  {
    sendto_http((http_socket *) hook1,
      "%s is registered by:<P>\n", (char *)hook2);
  }

  if (dbu == NULL)
  {
    if (count == 0)
    {
      sendto_http((http_socket *) hook1,
	"Channel %s is not registered",
	(char *)hook2);
    }
    else
    {
      def = DefChanList;
      while (def && strcasecmp((char *)hook2, def->name))
	def = def->next;

      if (def)
      {
	char top[512];
	quote_html(def->topic, top);
	sendto_http((http_socket *) hook1, "Desc: %s<br>\n", top);
	sendto_http((http_socket *) hook1, "URL: <A HREF=\"%s\" TARGET=\"_top\">%s</A><br>\n",
	  def->url, def->url);
      }
    }
    sendto_http((http_socket *) hook1, "<P><P><HR>%s\n", HTTP_FOOTER);
    ((http_socket *) hook1)->status = HTTP_ENDING;
    ((http_socket *) hook1)->dbio = 0;
    free(hook2);
  }
  else if (dbu->access >= 500)
  {
    t = now - dbu->lastseen;

    sendto_http((http_socket *) hook1, "%s (%s) last seen: %s ago<br>\n",
      dbu->nick, dbu->match,
      time_remaining(t));
  }
}

static void http_show_chaninfo(http_socket * hsock, char *channel)
{
  char date[80], *hook;
  struct tm *timeptr;

  timeptr = gmtime(&now);
  sprintf(date, "%sUCT", asctime(timeptr));
  *strchr(date, '\n') = ' ';

  sendto_http(hsock, HTTP_HEADER, "Channel information");
  sendto_http(hsock, HTTP_BODY);
  sendto_http(hsock, "<H1>Channel information</H1>\n");
  sendto_http(hsock, "%s<p>", date);

  hook = (char *)malloc(strlen(channel) + 1);
  strcpy(hook, channel);
  db_fetch(channel, DBGETALLCMP, "", NULL, 0, hsock, hook,
    http_chaninfo_callback);
  hsock->dbio = 1;
}


static void http_show_whois(http_socket * hsock, char *nick)
{
  register aluser *user;
  register achannelnode *chan;
  register auser *usr;

  char date[80];
  struct tm *timeptr;

  timeptr = gmtime(&now);
  sprintf(date, "%sUTC", asctime(timeptr));
  *strchr(date, '\n') = ' ';

#ifdef DEBUG
  printf("HTTP WHOIS: \"%s\"\n", nick);
#endif
  sendto_http(hsock, HTTP_HEADER, "WHOIS");
  sendto_http(hsock, HTTP_BODY);
  sendto_http(hsock, "<H1>WHOIS Information</H1>\n");
  sendto_http(hsock, "%s<p>\n", date);

  user = ToLuser(nick);

  if (user == NULL)
  {
    sendto_http(hsock, "*** %s: No such nick<p>\n", nick);
  }
  else
  {
    sendto_http(hsock, "*** %s is (%s@%s)<br>\n",
      user->nick, user->username, user->site);
    chan = user->channel;
    if (chan != NULL)
    {
      sendto_http(hsock, "*** on channels: ");
      while (chan != NULL)
      {
	if (!IsSet(chan->N->name, 's', "") &&
	  !IsSet(chan->N->name, 'p', ""))
	{
	  usr = ToUser(chan->N->name, nick);
	  if (usr->chanop)
	    sendto_http(hsock, "@%s ", chan->N->name);
	  else
	    sendto_http(hsock, "%s ", chan->N->name);
	}
	chan = chan->next;
      }
      sendto_http(hsock, "<br>\n");
    }
    sendto_http(hsock, "*** on irc via server %s<br>\n", user->server->name);

    if (user->mode & LFL_ISOPER)
    {
      sendto_http(hsock, "*** %s is an IRC Operator<br>\n", user->nick);
    }
  }

  sendto_http(hsock, "<P><P><P><HR>%s\n", HTTP_FOOTER);
  hsock->status = HTTP_ENDING;
}


static int get_file_pipe(http_socket * hsock, int fd)
{
  register http_file_pipe *new;

  new = (http_file_pipe *) malloc(sizeof(http_file_pipe));
  if (new == NULL)
    return -1;

  new->fd = fd;
  new->hsock = hsock;
  new->next = FilePipes;
  FilePipes = new;

  new->hsock->status = HTTP_PIPE;
#ifdef DEBUG
  show_list();
#endif
  return 0;
}

void destroy_file_pipe(http_file_pipe * old)
{
  register http_file_pipe **tmp = &FilePipes;

  while (*tmp != NULL && *tmp != old)
  {
    tmp = &(*tmp)->next;
  }

  if (*tmp != NULL)
  {
    *tmp = old->next;
    free(old);
  }
#ifdef DEBUG
  show_list();
#endif
}

void readfrom_file(http_file_pipe * fpipe)
{
  char buffer[2048];
  int length;

  if (fpipe->hsock->status == HTTP_ERROR)
  {
    close(fpipe->fd);
    fpipe->fd = -1;
    destroy_file_pipe(fpipe);
    return;
  }

  if ((length = read(fpipe->fd, buffer, 2047)) > 0)
  {
    copy_to_buffer(&fpipe->hsock->outbuf, buffer, length);
  }
  else if (length == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
  {
    fpipe->hsock->status = HTTP_ENDING;
    close(fpipe->fd);
    fpipe->fd = -1;
    destroy_file_pipe(fpipe);
  }
  return;
}


static void http_send_file(http_socket * hsock, char *fname, char *protocol)
{
  char file[256], date[80];
  struct stat sbuf;
  struct tm *timeptr;
  register char *ptr;
  int fd, cnt;
  int tmpfd;

#ifdef DEBUG
  printf("fname= \"%s\"\n", fname);
#endif
  cnt = 0;
  for (ptr = fname; *ptr; ptr++)
  {
    if (*ptr == '.')
      cnt++;
    if ((!isalnum(*ptr) && !strchr("./_-", *ptr)) || cnt > 1)
    {
      hsock->status = HTTP_ERROR;
      close(hsock->fd);
      hsock->fd = -1;
      return;
    }
  }

  sprintf(file, "HTTP/%s", fname);
  if ((fd = open(file, O_RDONLY)) < 0 || fstat(fd, &sbuf) < 0 ||
    fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
  {
    hsock->status = HTTP_ERROR;
    if (fd >= 0)
      close(fd);
    if (hsock->fd >= 0)
    {
      close(hsock->fd);
      hsock->fd = -1;
    }
    return;
  }

  sprintf(file, "HTTP/%s.#", fname);
  if ((tmpfd = open(file, O_RDWR | O_CREAT, 0600)) >= 0)
  {
    char buf[80] = "";
    cnt = 0;
    if (read(tmpfd, buf, 79) > 0)
    {
      sscanf(buf, "%d", &cnt);
      lseek(tmpfd, 0L, SEEK_SET);
    }
    sprintf(buf, "%d\n", ++cnt);
    write(tmpfd, buf, strlen(buf));
    close(tmpfd);
  }

  sendto_http(hsock, "HTTP/1.0 200 Document follows%c", 10);
  timeptr = gmtime(&now);
  sprintf(date, "%sUTC", asctime(timeptr));
  *strchr(date, '\n') = ' ';
  sendto_http(hsock, "Date: %s%c", date, 10);
  sendto_http(hsock, "Server: CS/1.0%c", 10);
  sendto_http(hsock, "Mime-version: 1.0%c", 10);

  if (strstr(fname, ".gif"))
  {
    sendto_http(hsock, "Content-type: image/gif%c", 10);
  }
  else if (strstr(fname, ".jpg"))
  {
    sendto_http(hsock, "Content-type: image/jpeg%c", 10);
  }
  else if (strstr(fname, ".html"))
  {
    sendto_http(hsock, "Content-type: text/html%c", 10);
  }
  else if (strstr(fname, ".txt"))
  {
    sendto_http(hsock, "Content-type: text/plain%c", 10);
  }
  else
  {
    sendto_http(hsock, "Content-type: */*%c", 10);
  }

  timeptr = gmtime(&sbuf.st_mtime);
  sprintf(date, "%sUTC", asctime(timeptr));
  *strchr(date, '\n') = ' ';
  sendto_http(hsock, "Last-modified: %s%c", date, 10);
  sendto_http(hsock, "Content-length: %ld\n\n", sbuf.st_size);

  if (get_file_pipe(hsock, fd) < 0)
  {
    hsock->status = HTTP_ERROR;
    close(hsock->fd);
    hsock->fd = -1;
  }
}


static void send_http_error(http_socket * hsock)
{
  sendto_http(hsock, HTTP_HEADER, "ERROR");
  sendto_http(hsock, HTTP_BODY);
  sendto_http(hsock, "<H1>Error in request</H1>\n");
  sendto_http(hsock, "<P><HR>%s\n", HTTP_FOOTER);
}

static void parse_get(http_socket * hsock, char *path, char *protocol)
{
  extern void http_show_help(http_socket *, char *);
  char channel[80];
  register char *ptr;

#ifdef DEBUG
  printf("GET: %s\n", path);
#endif
  if (*path != '/')
  {
    send_http_error(hsock);
    hsock->status = HTTP_ENDING;
    return;
  }

  if ((ptr = strchr(path + 1, '/')) == NULL)
  {
    ptr = path + strlen(path);
  }
  else
  {
    *(ptr++) = '\0';
  }
  sprintf(channel, "#%s", ptr);

  if (!strcasecmp(path + 1, "USERLIST"))
  {
    http_show_userlist(hsock, channel, protocol);
  }
  else if (!strcasecmp(path + 1, "BANLIST"))
  {
    http_show_banlist(hsock, channel, protocol);
  }
  else if (!strcasecmp(path + 1, "CHANINFO"))
  {
    http_show_chaninfo(hsock, channel);
  }
  else if (!strcasecmp(path + 1, "LIST"))
  {
    http_show_list(hsock, channel);
  }
  else if (!strcasecmp(path + 1, "WHOIS"))
  {
    http_show_whois(hsock, ptr);
  }
  else if (!strcasecmp(path + 1, "FILES"))
  {
    http_send_file(hsock, ptr, protocol);
  }
  else if (!strcasecmp(path + 1, "HELP"))
  {
    http_show_help(hsock, ptr);
  }
  else
  {
#ifdef HTTP_REDIRECT
    sendto_http(hsock, "HTTP/1.0 301 Document was moved\n");
    sendto_http(hsock, "Location: " HTTP_REDIRECT "\n\n");
    sendto_http(hsock, "<TITLE>Document has moved</TITLE>\n");
    sendto_http(hsock, "<H1>Document was moved to "
      "<A HREF=\"" HTTP_REDIRECT "\" TARGET=\"_top\">"
      HTTP_REDIRECT "</A></H1>\n");
    hsock->status = HTTP_ENDING;
#else
    http_send_file(hsock, "index.html", protocol);
#endif
  }
}

static int auth_raw(u_long addr)
{
  FILE *fp;
  char buffer[80], *ptr;
  if ((fp = fopen("raw.auth", "r")) == NULL)
    return 0;

  while (fgets(buffer, 79, fp) != NULL)
  {
    if ((ptr = strpbrk(buffer, "\r\n")) != NULL)
      *ptr = '\0';
    if (inet_addr(buffer) == addr)
    {
      fclose(fp);
      return 1;
    }
  }
  fclose(fp);
  return 0;
}

static void proc_raw(http_socket * hsock, char *line)
{
  register http_raw *raw = (http_raw *) hsock->hook;
  register char *ptr;
  unsigned long key = 0;

  if ((ptr = strpbrk(line, "\r\n")) != NULL)
    *ptr = '\0';

  if (raw == NULL)
  {
    if (!auth_raw(hsock->peer.sin_addr.s_addr))
    {
      hsock->status = HTTP_ERROR;
      close(hsock->fd);
      hsock->fd = -1;
      return;
    }
    hsock->hook = (void *)MALLOC(sizeof(http_raw));
    ((http_raw *) hsock->hook)->key =
      (((unsigned long)hsock->hook % 0xFFFF) +
      (((HTTPTTLSENTBYTES) & 0xFFFF) << 16)) ^ now;
    sendto_http(hsock, "PING %lu\n", ((http_raw *) hsock->hook)->key);
    return;
  }

  if (raw->key != 0)
  {
    if (strncasecmp(line, "PONG ", 5))
    {
      hsock->status = HTTP_ERROR;
      close(hsock->fd);
      hsock->fd = -1;
      return;
    }

    sscanf(line + 5, "%lu", &key);
    if (key != raw->key)
    {
      hsock->status = HTTP_ERROR;
      close(hsock->fd);
      hsock->fd = -1;
      return;
    }
    raw->key = 0;
  }
  else
  {
    char target[80], cmd[80];
    GetWord(0, line, cmd);
    GetWord(1, line, target);
    if (!strcasecmp(cmd, "ISREG"))
    {
      struct stat st;
      if (stat(make_dbfname(target), &st) < 0)
	sendto_http(hsock, "NO\n");
      else
	sendto_http(hsock, "YES\n");
    }
    else if (!strcasecmp(cmd, "REG"))
    {
      char buf[256];
      struct stat st;
      RegUser *reg;
      int bad = 0;
      if (stat(make_dbfname(target), &st) >= 0)
	bad = 1;
      if (!bad)
      {
	reg = UserList[ul_hash(target)];
	while (reg != NULL && (strcasecmp(reg->channel, target) || reg->access == 0))
	  reg = reg->next;
	if (reg != NULL)
	  bad = 1;
      }
      if (bad)
      {
	sendto_http(hsock, "ERROR Userlist not empty\n");
      }
      else
      {
	char tmp[80];
	reg = (RegUser *) MALLOC(sizeof(RegUser));
	memset(reg, 0, sizeof(RegUser));
	GetWord(2, line, tmp);
	reg->realname = (char *)MALLOC(strlen(tmp) + 1);
	strcpy(reg->realname, tmp);
	GetWord(3, line, tmp);
	reg->match = (char *)MALLOC(strlen(tmp) + 1);
	strcpy(reg->match, tmp);
	GetWord(4, line, tmp);
	reg->modif = (char *)MALLOC(strlen(tmp) + 1);
	strcpy(reg->modif, tmp);
	reg->channel = (char *)MALLOC(strlen(target) + 1);
	strcpy(reg->channel, target);
	GetWord(6, line, tmp);
	if (tmp && tmp[0] != '\0')
	{
	  reg->passwd = (char *)MALLOC(strlen(tmp) + 1);
	  strcpy(reg->passwd, tmp);
	}
	else
	{
	  reg->passwd = (char *)MALLOC(1);
	  reg->passwd[0] = '\0';
	}
	reg->access = 500;
	reg->suspend = 0;
	reg->lastseen = now;
	reg->flags = 0;
	reg->offset = (off_t) - 1;
	reg->modified = 1;
	reg->next = UserList[ul_hash(target)];
	UserList[ul_hash(target)] = reg;
	cold_save_one(reg);

	sendto_http(hsock, "DONE\n");
	sprintf(buf, "%s registered %s to %s", reg->modif,
	  reg->channel, reg->match);
	broadcast(buf, 0);
      }
    }
    hsock->status = HTTP_ENDING;
  }
}

static void parse_post(http_socket * hsock, char *line)
{
  extern void http_show_help(http_socket *, char *);
  char buffer[512], tmp[10], *channel = NULL, *nick = NULL, *arg = NULL;
  register char *ptr1, *ptr2;
  int value;
  register http_post *post;

  post = (http_post *) hsock->hook;

  if (post->count++ > 30)
  {
    hsock->status = HTTP_ERROR;
    close(hsock->fd);
    hsock->fd = -1;
    return;
  }

#ifdef check_referer
  if (!strncasecmp(line, "Referer: ", 9) && check_referer(line + 9))
  {
    sendto_http(hsock, "HTTP/1.0 302 Check there\n"
      "Location: %s\n\n", HTTP_BAD_REFERER);
    hsock->status = HTTP_ENDING;
    return;
  }
#endif

  while ((ptr1 = strpbrk(line, "\r\n")) != NULL)
    *ptr1 = '\0';

  if (!strcasecmp(line, "Content-Type: application/x-www-form-urlencoded"))
  {
    post->ready = 1;
    return;
  }

  if (*line == '\0')
  {
    post->ready = 2;
    return;
  }

  if (post->ready != 2)
  {
    return;
  }

#ifdef DEBUG
  printf("line= \"%s\"\n", line);
#endif

  for (ptr1 = line, ptr2 = buffer; *ptr1 && (ptr2 - buffer) < 511; ptr1++)
  {
    if (ptr1[0] == '%' && ptr1[1] && ptr1[2])
    {
      tmp[0] = ptr1[1];
      tmp[1] = ptr1[2];
      tmp[2] = '\0';
      sscanf(tmp, "%x", &value);
      *(ptr2++) = value;
      ptr1 += 2;
    }
    else if (ptr1[0] == '&')
    {
      *(ptr2++) = ' ';
    }
    else
    {
      *(ptr2++) = *ptr1;
    }
  }
  *ptr2 = '\0';

#ifdef DEBUG
  printf("LINE= \"%s\"\n", buffer);
#endif
  http_log("LINE: %s", buffer);

  if (*post->path != '/')
  {
    hsock->status = HTTP_ERROR;
    close(hsock->fd);
    hsock->fd = -1;
    return;
  }
  if ((ptr1 = strchr(post->path + 1, '/')) != NULL)
  {
    *ptr1 = '\0';
  }

  ptr1 = buffer;
  if ((channel = strstr(ptr1, "CHANNEL=")) != NULL)
  {
    channel += 8;
    if ((ptr1 = strchr(channel, ' ')) != NULL)
    {
      *(ptr1++) = '\0';
    }
  }
  if (ptr1 && (nick = strstr(ptr1, "NICK=")) != NULL)
  {
    nick += 5;
    if ((ptr1 = strchr(nick, ' ')) != NULL)
    {
      *(ptr1++) = '\0';
    }
  }
  if (ptr1 && (arg = strstr(ptr1, "ARG=")) != NULL)
  {
    arg += 4;
    if ((ptr1 = strchr(arg, ' ')) != NULL)
    {
      *(ptr1++) = '\0';
    }
  }

  if (!strcasecmp(post->path + 1, "USERLIST"))
  {
    if (channel == NULL)
    {
      hsock->status = HTTP_ERROR;
      close(hsock->fd);
      hsock->fd = -1;
      return;
    }
    http_show_userlist(hsock, channel, post->protocol);
  }
  else if (!strcasecmp(post->path + 1, "BANLIST"))
  {
    if (channel == NULL)
    {
      hsock->status = HTTP_ERROR;
      close(hsock->fd);
      hsock->fd = -1;
      return;
    }
    http_show_banlist(hsock, channel, post->protocol);
  }
  else if (!strcasecmp(post->path + 1, "CHANINFO"))
  {
    if (channel == NULL)
    {
      hsock->status = HTTP_ERROR;
      close(hsock->fd);
      hsock->fd = -1;
      return;
    }
    http_show_chaninfo(hsock, channel);
  }
  else if (!strcasecmp(post->path + 1, "LIST"))
  {
    if (channel == NULL)
    {
      hsock->status = HTTP_ERROR;
      close(hsock->fd);
      hsock->fd = -1;
      return;
    }
    http_show_list(hsock, channel);
  }
  else if (!strcasecmp(post->path + 1, "WHOIS"))
  {
    if (nick == NULL)
    {
      hsock->status = HTTP_ERROR;
      close(hsock->fd);
      hsock->fd = -1;
      return;
    }
    http_show_whois(hsock, nick);
  }
  else if (!strcasecmp(post->path + 1, "HELP"))
  {
    if (arg == NULL)
    {
      hsock->status = HTTP_ERROR;
      close(hsock->fd);
      hsock->fd = -1;
      return;
    }
    http_show_help(hsock, arg);
  }
  else
  {
    hsock->status = HTTP_ERROR;
    close(hsock->fd);
    hsock->fd = -1;
    return;
  }
}


void parse_http(http_socket * hsock, char *buf)
{
  extern void parse_chat(struct http_socket *, char *);
  extern void chat_login(struct http_socket *, char *, char *);
  char method[80], path[80], protocol[80], *ptr;

#ifdef DEBUG
  printf("#IN# %s\n", buf);
#endif
  if (hsock->status == HTTP_RECV_POST)
  {
    parse_post(hsock, buf);
    return;
  }
  if (hsock->status == HTTP_CSRAW)
  {
    proc_raw(hsock, buf);
    return;
  }
  if (hsock->status == HTTP_CHAT)
  {
    parse_chat(hsock, buf);
    return;
  }

  if ((ptr = strpbrk(buf, "\r\n")) != NULL)
    *ptr = '\0';

  GetWord(0, buf, method);
  GetWord(1, buf, path);
  GetWord(2, buf, protocol);
  if (*protocol == '\0')
    strcpy(protocol, "HTTP/1.0");

  http_log("[%d] %s %s %s", hsock->fd, method, path, protocol);

  if (!strcasecmp(method, "GET"))
  {
    parse_get(hsock, path, protocol);
  }
  else if (!strcasecmp(method, "POST"))
  {
    hsock->status = HTTP_RECV_POST;
    hsock->hook = (void *)MALLOC(sizeof(http_post));
    strcpy(((http_post *) hsock->hook)->path, path);
    strcpy(((http_post *) hsock->hook)->protocol, protocol);
    ((http_post *) hsock->hook)->count = 0;
    ((http_post *) hsock->hook)->ready = 0;
  }
  else if (!strcasecmp(method, "CSRAW"))
  {
    hsock->status = HTTP_CSRAW;
    proc_raw(hsock, buf + 6);
  }
  else if (!strcasecmp(method, "CHAT"))
  {
    if ((ptr = strchr(path, '/')) == NULL)
    {
      hsock->status = HTTP_ERROR;
      close(hsock->fd);
      hsock->fd = -1;
    }
    else
    {
      *(ptr++) = '\0';
      hsock->status = HTTP_CHAT;
      chat_login(hsock, path, ptr);
    }
  }
/*
   else{
   hsock->status=HTTP_ERROR;
   close(hsock->fd);
   hsock->fd=-1;
   }
 */
}

#endif

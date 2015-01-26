/* @(#)$Id: connect.c,v 1.12 1998/01/25 18:35:42 seks Exp $ */

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
#ifndef FD_ZERO
#define FD_ZERO(set)      (((set)->fds_bits[0]) = 0)
#define FD_SET(s1, set)   (((set)->fds_bits[0]) |= 1 << (s1))
#define FD_ISSET(s1, set) (((set)->fds_bits[0]) & (1 << (s1)))
#define FD_SETSIZE        30
#endif

int read_from_server(int);
int write_to_server(irc_socket *);

int connection(char *serv)
{
  char buffer[200];
  int portnum;
  char *ptr;
  struct sockaddr_in socketname;
#ifdef BINDADDR
  struct sockaddr_in myname;
#endif
  struct hostent *remote_host;

  if ((ptr = strchr(serv, ':')) != NULL)
  {
    *(ptr++) = 0;
    sscanf(ptr, "%d", &portnum);
#ifdef DEBUG
    printf("Server: %s\nPort: %d\n", serv, portnum);
#endif
  }
  else
  {
    portnum = DEFAULT_PORTNUM;
  }

  sprintf(buffer, "CONNECTING TO %s ON PORT %d", serv, portnum);
  log(buffer);

  if (Irc.outbuf)
    zap_buffer(&Irc.outbuf);
  if (Irc.inbuf)
    zap_buffer(&Irc.inbuf);
  read_from_server(1);	/* reset input buffer */

  now = time(NULL);

  /* open an inet socket */
  if ((Irc.fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
#ifdef DEBUG
    fprintf(stderr, "ERROR: Can't assign fd for socket, darn!\n");
#endif
    log("ERROR ASSIGNING FD FOR SOCKET");
    exit(1);
  }

#ifdef BINDADDR
  memset(&myname, 0, sizeof(myname));
  myname.sin_family = AF_INET;
  myname.sin_port = 0;
  myname.sin_addr.s_addr = inet_addr(BINDADDR);
  if (bind(Irc.fd, (struct sockaddr *)&myname, sizeof(myname)) < 0)
  {
#ifdef DEBUG
    fprintf(stderr, "ERROR: Can't bind local address %s\n", BINDADDR);
#endif
    log("Can't bind local address");
    log(BINDADDR);
    exit(1);
  }
#endif

  /* make it non-blocking */
  fcntl(Irc.fd, F_SETFL, O_NONBLOCK);

  /* lookup host */
  socketname.sin_family = AF_INET;
  if ((remote_host = gethostbyname(serv)) == NULL)
  {
#ifdef DEBUG
    fprintf(stderr, "ERROR: Host %s is unknown, wtf?\n", serv);
#endif
    log("ERROR: BULLSHIT HOST!");
    exit(1);
  }
  memcpy((void *)&socketname.sin_addr, (void *)remote_host->h_addr, remote_host->h_length);
  socketname.sin_port = htons(portnum);

  /* connect socket */
  if (connect(Irc.fd, (struct sockaddr *)&socketname, sizeof(socketname)) < 0
    && errno != EINPROGRESS)
  {
    close(Irc.fd);
    Irc.fd = -1;
#ifdef DEBUG
    printf("ERROR: connect() %d: %s\n", errno, strerror(errno));
#endif
    log("ERROR: COULDN'T CONNECT");
    return (1);
  }
  TSconnect = Irc.TS = now = time(NULL);
  errno = 0;
  return 0;
}

int wait_msg(void)
{
  fd_set readfds, writefds;
  struct timeval timeout;
  static int pingflag = 0;
  int maxfd = -1;
  int to;
#ifdef DOHTTP
  void http_log(char *fmt,...);
  register http_socket *hsock, *hsocktmp;
  register http_file_pipe *fpipe, *fpipetmp;
#endif
#ifdef UPGRADE
  register misc_socket *msock, *msocktmp;
#endif
  register dbquery **dq, *dqtmp;
  register dbsync **ds, *dstmp;

  /* initialize the fd_sets
   */
  FD_ZERO(&readfds);
  FD_ZERO(&writefds);

  /* set timeout
   */
  if (EventList != NULL)
  {
    to = EventList->time - now;
    if (to < PING_FREQ)
      timeout.tv_sec = (to > 0) ? to : 0;
    else
      timeout.tv_sec = PING_FREQ;
  }
  else
  {
    timeout.tv_sec = PING_FREQ;
  }
  timeout.tv_usec = 0;

  /* Check the DBQuery list before the server fd because
   * end_db_read() might writing something in the server's
   * buffer.
   */
  dq = &DBQuery;
  while (*dq)
  {
    if ((*dq)->fd < 0)
    {
      dqtmp = *dq;
      *dq = (*dq)->next;
      end_db_read(dqtmp);
      free(dqtmp);
    }
    else
    {
      if ((*dq)->fd > maxfd)
	maxfd = (*dq)->fd;
      FD_SET((*dq)->fd, &readfds);
      dq = &(*dq)->next;
    }
  }

  ds = &DBSync;
  while (*ds)
  {
    if ((*ds)->fd < 0)
    {
      dstmp = *ds;
      *ds = (*ds)->next;
      end_db_sync(dstmp);
      free(dstmp);
    }
    else
    {
      if ((*ds)->fd > maxfd)
	maxfd = (*ds)->fd;
      if ((*ds)->status == SYNC_PENDWRITE)
	FD_SET((*ds)->fd, &writefds);
      else
	FD_SET((*ds)->fd, &readfds);
      ds = &(*ds)->next;
    }
  }


  /* check if the uplink's socket is ready for reading..
   */
  FD_SET(Irc.fd, &readfds);

  /* check if the uplink's socket is ready for writing only if
   * we have something to write
   */
  if (Irc.outbuf != NULL)
    FD_SET(Irc.fd, &writefds);

  if (Irc.fd > maxfd)
    maxfd = Irc.fd;

#ifdef DOHTTP
  fpipe = FilePipes;
  while (fpipe != NULL)
  {
    if (fpipe->fd > maxfd)
      maxfd = fpipe->fd;

    FD_SET(fpipe->fd, &readfds);
    fpipe = fpipe->next;
  }

  hsock = HttpList;
  while (hsock != NULL)
  {
    if (hsock->fd < 0 || hsock->status == HTTP_ERROR)
    {
      hsock = hsock->next;	/* am I an idiot or what? -Kev */
      continue;		/* you should see the logs from below! -Kev */
    }
    if (hsock->fd > maxfd)
      maxfd = hsock->fd;
    if (hsock->status != HTTP_LISTEN &&
      hsock->status != HTTP_CHAT &&
      hsock->since + 2 * HTTP_TIMEOUT < now)
    {
      http_log("ERROR: absolute timeout for fd %d (%d)",
	hsock->fd, hsock->status);
      close(hsock->fd);
      hsock->fd = -1;
      hsock->status = HTTP_ERROR;
    }
    if (hsock->status != HTTP_LISTEN &&
      hsock->status != HTTP_CHAT &&
      hsock->TS + HTTP_TIMEOUT < now)
    {
      close(hsock->fd);
      hsock->fd = -1;
      hsock->status = HTTP_ERROR;
    }
    if (hsock->status == HTTP_ENDING &&
      hsock->outbuf == NULL)
    {
      close(hsock->fd);
      hsock->fd = -1;
      hsock->status = HTTP_ERROR;
    }
    if (hsock->status != HTTP_ERROR)
    {
      if (hsock->status != HTTP_ENDING &&
	hsock->status != HTTP_PIPE)
	FD_SET(hsock->fd, &readfds);
      if (hsock->outbuf != NULL)
	FD_SET(hsock->fd, &writefds);
    }
    hsock = hsock->next;
  }
#endif

#ifdef UPGRADE
  msock = MiscList;
  while (msock != NULL)
  {
    if (msock->fd > maxfd)
      maxfd = msock->fd;
    if (msock->TS + MISC_TIMEOUT < now)
    {
      close(msock->fd);
      msock->fd = -1;
      msock->status = MISC_ERROR;
      notice(msock->link, "connection timeout!");
    }
    if (msock->status != MISC_ERROR)
    {
      FD_SET(msock->fd, &readfds);
      if (msock->outbuf != NULL)
	FD_SET(msock->fd, &writefds);
    }
    msock = msock->next;
  }
#endif

  /* Wait till the socket is ready for reading
   * and/or writing and/or a timeout
   */
  if (select(maxfd + 1, &readfds, &writefds, NULL, &timeout) < 0)
  {
    log("ERROR: select()");
    log((char *)sys_errlist[errno]);
    return (-1);
  }

  now = time(NULL);

  /* The uplink's socket is ready for reading... */
  if (FD_ISSET(Irc.fd, &readfds))
  {
    if (read_from_server(0) < 0)
    {
      log("ERROR: in read_from_server()");
      return (-1);
    }
    pingflag = 0;
    Irc.TS = now;
  }

  /* The uplink's socket is ready for writing... */
  if (FD_ISSET(Irc.fd, &writefds))
  {
    if (write_to_server(&Irc) < 0)
    {
      log("ERROR: in write_to_server()");
      return (-1);
    }
  }

#ifdef DOHTTP
  fpipe = FilePipes;
  while (fpipe != NULL)
  {
    fpipetmp = fpipe;
    fpipe = fpipe->next;
    if (FD_ISSET(fpipetmp->fd, &readfds))
    {
      readfrom_file(fpipetmp);
    }
  }

  hsock = HttpList;
  while (hsock != NULL)
  {
    while (hsock != NULL && hsock->status == HTTP_ERROR)
    {
      hsocktmp = hsock;
      hsock = hsock->next;
      remove_httpsock(hsocktmp);
    }
    if (hsock == NULL)
      break;

    if (hsock->fd >= 0 && FD_ISSET(hsock->fd, &readfds))
    {
      if (hsock->status == HTTP_LISTEN)
      {
	http_accept(hsock->fd);
      }
      else
      {
	readfrom_http(hsock);
	hsock->TS = now;
      }
    }
    if (hsock->fd >= 0 && FD_ISSET(hsock->fd, &writefds))
    {
      if (flush_http_buffer(hsock) != -1)
	hsock->TS = now;
      if (hsock->status == HTTP_ENDING && hsock->outbuf == NULL)
      {
	close(hsock->fd);
	hsock->fd = -1;
	hsock->status = HTTP_ERROR;
      }
    }
    hsock = hsock->next;
  }
#endif

#ifdef UPGRADE
  msocktmp = NULL;
  msock = MiscList;
  while (msock != NULL)
  {
    while (msock != NULL && msock->status == MISC_ERROR)
    {
      if (msocktmp == NULL)
      {
	MiscList = msock->next;
	free(msock);
	msock = MiscList;
      }
      else
      {
	msocktmp->next = msock->next;
	free(msock);
	msock = msocktmp->next;
      }
    }
    if (msock == NULL)
      break;

    if (msock->fd >= 0 && FD_ISSET(msock->fd, &readfds))
    {
      readfrom_misc(msock);
      msock->TS = now;
    }
    if (msock->fd >= 0 && FD_ISSET(msock->fd, &writefds))
    {
      if (flush_misc_buffer(msock) != -1)
	msock->TS = now;
    }
    msocktmp = msock;
    msock = msock->next;
  }
#endif

  dqtmp = DBQuery;
  while (dqtmp)
  {
    if (FD_ISSET(dqtmp->fd, &readfds))
      read_db(dqtmp);
    dqtmp = dqtmp->next;
  }

  dstmp = DBSync;
  while (dstmp)
  {
    if (FD_ISSET(dstmp->fd, &readfds) || FD_ISSET(dstmp->fd, &writefds))
      db_sync_ready(dstmp);
    dstmp = dstmp->next;
  }

  if (EventList != NULL && now >= EventList->time)
    CheckEvent();

  /* send a PING to the server if nothing is received within
   * PING_FREQ seconds... if yet nothing arrives within 3 * PING_FREQ
   * seconds it's a ping timeout and the connection should be closed
   */
  if ((now - Irc.TS) >= (3 * PING_FREQ + 1))
  {
    log("Errr PING TIMEOUT");
#ifdef DEBUG
    printf("Ping timeout..\n");
#endif
    sendtoserv("ERROR :Connection timeout\n");
    return (-1);
  }
  else if (pingflag == 0 && (now - Irc.TS) >= (PING_FREQ - 1))
  {
    sendtoserv("PING :");
    sendtoserv(SERVERNAME);
    sendtoserv("\n");
    pingflag = 1;
  }

  if (DB_Save_Status != -1)
    do_cold_sync_slice();

  return 0;
}

void sendtoserv(char *msg)
{
#ifdef DEBUG
  printf("#OUT#%s", msg);
#endif
  CurrentSendQ = copy_to_buffer(&Irc.outbuf, msg, strlen(msg));

  if (CurrentSendQ > MAX_SENDQ)
  {
    log("ERROR: Reached MAX_SENDQ!!!");
    zap_buffer(&Irc.outbuf);
    close(Irc.fd);
    exit(-1);
  }
}

void dumpbuff(void)
{
  if (Irc.fd < 0)
    return;

  /* remove the O_NONBLOCK flag */
  fcntl(Irc.fd, F_SETFL, 0);

  write_to_server(&Irc);

  if (Irc.fd >= 0)
    /* reset the O_NONBLOCK flag */
    fcntl(Irc.fd, F_SETFL, O_NONBLOCK);
}

int read_from_server(int reset)
{
  static char inbuff[1025] = "";
  static char source[SERVER_NAME_LENGTH] = "";
  static char function[10] = "";
  static char target[513] = "";
  static char body[513] = "";

  static int in_pos = 0;
  static int in_offset = 0;
  static int length = 0;
  char *inchar;
  int end;
#ifdef HISTORY
  char buffer[600];
#endif

  if (reset)
  {
    *inbuff = *source = *function = *target = *body = '\0';
    in_pos = in_offset = length = 0;
    return 0;
  }
#ifdef DEBUG
  printf("#IN#");
#endif
  end = 1;
  while (end > 0)
  {
    errno = 0;
    if ((length = read(Irc.fd, inbuff, 1024)) <= 0)
    {
      if (errno == EWOULDBLOCK || errno == EAGAIN || length == 0)
      {
	end = 0;
      }
      else
      {
	end = -1;
	in_pos = in_offset = 0;
	*source = *function = *target = *body = '\0';
#ifdef DEBUG
	printf("Read error.. not EWOULDBLOCK!\n");
#endif
      }
      return (end);
    }
    TTLREADBYTES += length;

    inchar = inbuff;
    while (length--)
    {
#ifdef DEBUG
      putchar(*inchar);
#endif
      if (*inchar != '\n' && *inchar != '\r')
      {
	if (*inchar == ' ' && in_pos < 3)
	{
	  switch (in_pos)
	  {
	  case 0:
	    source[in_offset] = '\0';
	    break;

	  case 1:
	    function[in_offset] = '\0';
	    break;

	  case 2:
	    target[in_offset] = '\0';
	    break;

	  default:
	    exit(1);
	  }
	  in_pos++;
	  in_offset = 0;
	}
	else
	{
	  switch (in_pos)
	  {
	  case 0:
	    if (in_offset == 0 && *inchar != ':')
	    {
	      source[0] = '\0';
	      in_pos++;
	      function[in_offset++] = *inchar;
	    }
	    else
	      source[in_offset++] = *inchar;
	    break;

	  case 1:
	    function[in_offset++] = *inchar;
	    break;

	  case 2:
	    target[in_offset++] = *inchar;
	    break;

	  case 3:
	    body[in_offset++] = *inchar;
	    break;

	  default:	/*shouldn't happen */
	    exit(1);
	  }
	}
      }
      if (*inchar == '\n')
      {
	if (in_pos >= 3)
	  body[in_offset] = '\0';
	else
	{
	  body[0] = '\0';
	  switch (in_pos)
	  {
	  case 2:
	    target[in_offset] = '\0';
	    break;

	  case 1:
	    function[in_offset] = '\0';
	    break;

	  case 0:
	    source[in_offset] = '\0';

	  default:
	    break;
	  }
	}

	/* parse the received line */
#ifdef HISTORY
	sprintf(buffer, "%s %s %s %s", source, function, target, body);
	History(buffer);
#endif
	proc(source, function, target, body);
	in_pos = in_offset = end = 0;
	*source = *function = *target = *body = '\0';
      }
      inchar++;
    }	/* while */
  }	/*while */

  return 1;
}


int write_to_server(irc_socket * isock)
{
  char buf[1024];
  int length;
  int count;

  if (isock == NULL || isock->outbuf == NULL)
    return -1;

  while ((count = look_in_buffer(&isock->outbuf, buf, '\0', 1023)) > 0)
  {
    if ((length = write(isock->fd, buf, count)) <= 0)
    {
      if (errno == EWOULDBLOCK || errno == EAGAIN)
      {
	return 0;
      }
      else
      {
	close(isock->fd);
	isock->fd = -1;
	return -1;
      }
    }
    else
    {
      TTLSENTBYTES += length;
      CurrentSendQ -= length;
      skip_char_in_buffer(&isock->outbuf, length);
    }
  }
  return 0;
}

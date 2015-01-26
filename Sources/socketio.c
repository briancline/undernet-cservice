/* @(#)$Id: socketio.c,v 1.7 1998/06/23 23:35:08 seks Exp $ */

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

/* These routines were first developed for the telnet3d project.
 * Special thanks to Danny Mitchell (wildthang@irc) for his help.
 */


#include "h.h"

#ifdef DOHTTP
#include <stdarg.h>

extern void chat_close(http_socket *, char *);

int readfrom_http(http_socket * client)
{
  void http_log(char *fmt,...);
  char buf[1024];
  int length;

  if ((length = read(client->fd, buf, 1023)) <= 0)
  {
    if (errno == EWOULDBLOCK || errno == EAGAIN)
    {
      return 0;
    }
    else
    {
      if (client->status == HTTP_CHAT)
	chat_close(client, "read error");
      close(client->fd);
      client->fd = -1;
      client->status = HTTP_ERROR;
      return -1;
    }
  }
  buf[length] = '\0';
#ifdef DEBUG
  printf("AA: |%s|\n", buf);
#endif

  if (copy_to_buffer(&client->inbuf, buf, length) >= 4096)
  {
    http_log("ERROR: Recv'd more than 4K from client! (flood?)");
    if (client->status == HTTP_CHAT)
      chat_close(client, "Input packet too big");
    client->status = HTTP_ERROR;
    close(client->fd);
    client->fd = -1;
    return -1;
  }

  if (find_char_in_buffer(&client->inbuf, '\n', 1023))
  {
    while (find_char_in_buffer(&client->inbuf, '\n', 1023) &&
      client->status != HTTP_PIPE &&
      client->status != HTTP_ENDING &&
      client->status != HTTP_ERROR)
    {
      copy_from_buffer(&client->inbuf, buf, '\n', 1023);
      parse_http(client, buf);
    }
    /* Check for STUPID MSIE! who doesn't send a \n after post data
       ** so check for a = in the first 10 chars, assume *ugh* it's post data
     */
    if (find_char_in_buffer(&client->inbuf, '=', 10))
    {
      if (client->status != HTTP_PIPE &&
	client->status != HTTP_ENDING &&
	client->status != HTTP_ERROR)
      {
#ifdef DEBUG
	printf("SEND\n");
#endif
	copy_from_buffer(&client->inbuf, buf, '\0', 1023);
	parse_http(client, buf);
      }
    }
  }
  else if (length > 200)
  {
    if (client->status == HTTP_CHAT)
      chat_close(client, "line too long");
    client->status = HTTP_ERROR;
    close(client->fd);
    client->fd = -1;
    return -1;
  }
  return 1;
}

int flush_http_buffer(http_socket * client)
{
  http_file_pipe *fpipe;
  char buf[1024];
  int length;
  int count;

  if (client == NULL || client->outbuf == NULL)
    return -1;

  while ((count = look_in_buffer(&client->outbuf, buf, '\0', 1023)) > 0)
  {
    if ((length = write(client->fd, buf, count)) <= 0)
    {
      if ((errno == EWOULDBLOCK || errno == EAGAIN) && length != 0)
      {
	return 0;
      }
      else
      {
	if (client->status == HTTP_PIPE)
	{
	  for (fpipe = FilePipes; fpipe != NULL; fpipe = fpipe->next)
	  {
	    if (fpipe->hsock == client)
	    {
	      close(fpipe->fd);
	      destroy_file_pipe(fpipe);
	      break;
	    }
	  }
	}
	if (client->status == HTTP_CHAT)
	  chat_close(client, "write error");
	close(client->fd);
	client->fd = -1;
	client->status = HTTP_ERROR;
	return -1;
      }
    }
    else
    {
      skip_char_in_buffer(&client->outbuf, length);
      HTTPTTLSENTBYTES += length;
    }
  }

  return 0;
}

/* sendto_client() by SeKs <intru@step.polymtl.ca>
 * args (struct buffer_block **block, char *format, args...)
 * adds 'args' according to 'format' to the client's output
 * buffer.
 */
long sendto_http(http_socket * sck, char *format,...)
{
  va_list args;
  char string[1024];

  va_start(args, format);
  vsprintf(string, format, args);
  va_end(args);

  return copy_to_buffer(&sck->outbuf, string, strlen(string));
}

#endif /*  DOHTTP  */

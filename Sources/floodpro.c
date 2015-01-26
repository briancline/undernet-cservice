/* @(#)$Id: floodpro.c,v 1.4 1999/04/04 17:00:06 seks Exp $ */

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

int CheckFlood(char *source, char *channel, int length)
{
  char buffer[200];
  register achannel *chan;
  register auser *user;
  register amsg *msg, *prev;
  register int count = 0;

  chan = ToChannel(channel);
  user = ToUser(channel, source);

  if (!chan || !chan->on || !user)
    return 0;

  /* BEGINNING OF FLOOD PROTECTION ROUTINE */
  /* 1st clean msghist older than 15 seconds */
  prev = NULL;
  msg = user->msghist;
  while (msg)
  {
    if (msg->time < (now - 15))
    {
      if (prev)
      {
	prev->next = msg->next;
	TTLALLOCMEM -= sizeof(amsg);
	free(msg);
	msg = prev->next;
      }
      else
      {
	user->msghist = msg->next;
	TTLALLOCMEM -= sizeof(amsg);
	free(msg);
	msg = user->msghist;
      }
    }
    else
    {
      prev = msg;
      msg = msg->next;
    }
  }
  /* now add the current msg to the history */
  msg = (amsg *) MALLOC(sizeof(amsg));
  msg->time = now;
  msg->length = length;
  msg->next = user->msghist;
  user->msghist = msg;

  /* now count the number of entry in the history..
     if it's greater than the max allowed... bite!  */

  count = 0;
  for (msg = user->msghist; msg; msg = msg->next, count++);

  if (chan->MsgFloodPro != 0 && count == chan->MsgFloodPro)
  {
    notice(source, "### FLOOD PROTECTION ACTIVATED ###");
    sprintf(buffer, "%s!%s@%s %d", user->N->nick,
      user->N->username, user->N->site, PUBLIC_FLOOD_SUSPEND_TIME);
    suspend("", channel, buffer);

    count = IsShit(channel, source, NULL, NULL);
#ifdef DEBUG
    printf("Argh! %s has a shit level %d\n", source, count);
#endif
    sprintf(buffer, "FLOODPRO ACTIVATED BY %s ON %s", source, channel);
    log(buffer);

    switch (count)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
      log("First warning");
      sprintf(buffer, "%s That was not very smart!", source);
      break;

    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
      log("Second warning");
      sprintf(buffer, "%s You're pushing your luck too far!", source);
      break;

    default:
      sprintf(buffer, "%s That was one time too many", source);
    }

    kick("", channel, buffer);

    sprintf(buffer, "%s %d %d *** FLOOD ***",
      user->N->nick,
      PUBLIC_FLOOD_SHITLIST_TIME,
      (count < 10) ? count + 5 :
      PUBLIC_FLOOD_SHITLIST_LEVEL);
    AddToShitList("", channel, buffer, 0);

    return 1;
  }
  return 0;
}

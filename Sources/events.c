/* @(#)$Id: events.c,v 1.6 1998/01/02 18:30:09 seks Exp $ */

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

void InitEvent(void)
{
  AddEvent(EVENT_SAVESHITLIST, now + SHITLIST_SAVE_FREQ + 300, "");
  AddEvent(EVENT_SAVEDEFCHANNELS, now + DEFS_SAVE_FREQ + 600, "");
  AddEvent(EVENT_SYNC, now + SYNC_FREQ + 900, "");
  AddEvent(EVENT_CHECK_IDLE, now + CHECK_IDLE_FREQ, "");
  AddEvent(EVENT_RENAME_LOGFILE, now + RENAME_LOGFILE_FREQ, "");
  AddEvent(EVENT_SYNCDB, now + CACHE_TIMEOUT, "");
#ifdef CHANNEL_LOG
  AddEvent(EVENT_LOG_CHANNEL, now + CHANNEL_LOG_FREQ, "");
#endif
#ifdef NICKSERV
  AddEvent(EVENT_SAVENICKSERV, now + NSERV_SAVE_FREQ, "");
#endif
}

void AddEvent(int event, time_t time, char *param)
{
  register anevent **curr = &EventList;
  register anevent *new;

#ifdef DEBUG
  printf("AddEvent( %d, %ld, %s)\n", event, time, param);
#endif

  /* the events are store in ascending time order, so the new
   * event is inserted before the first event with a greater
   * time
   */
  while (*curr && (*curr)->time < time)
    curr = &(*curr)->next;

  /* create a new event structure
   */
  new = (anevent *) MALLOC(sizeof(anevent));
  new->time = time;
  new->event = event;
  strncpy(new->param, param, 79);
  new->param[79] = '\0';

  /* link the new structure to the list
   */
  new->next = *curr;
  *curr = new;
}

void CheckEvent(void)
{
  register anevent *curr;
  register achannel *chan;

  while (EventList != NULL && EventList->time <= now)
  {
    /* remove the event from the list */
    curr = EventList;
    EventList = EventList->next;

#ifdef DEBUG
    printf("Scheduled event %d\n", curr->event);
#endif
    switch (curr->event)
    {
    case EVENT_SAVEUSERLIST:
      /* replaced by SYNCDB */
      break;

    case EVENT_SAVESHITLIST:
      SaveShitList("", "");
      AddEvent(EVENT_SAVESHITLIST,
	now + SHITLIST_SAVE_FREQ, "");
      break;

    case EVENT_SAVEDEFCHANNELS:
      SaveDefs("");
      AddEvent(EVENT_SAVEDEFCHANNELS,
	now + DEFS_SAVE_FREQ, "");
      break;

    case EVENT_CLEANSHITLIST:
      CleanShitList("", curr->param);
      break;

    case EVENT_FLUSHMODEBUFF:
      flushmode(curr->param);
      break;

    case EVENT_GETOPS:
      chan = ToChannel(curr->param);
      if (chan != NULL && chan->on &&
	(IsOpless(curr->param) ||
	  ((chan->flags & CFL_ALWAYSOP) &&
	    !chan->AmChanOp)))
	GetOps(curr->param);
      break;

    case EVENT_SYNC:
      sync();
      AddEvent(EVENT_SYNC,
	now + SYNC_FREQ, "");
      break;

    case EVENT_CLEAN_IGNORES:
      CleanIgnores();
      break;

    case EVENT_CHECK_IDLE:
      CheckIdleChannels();
      AddEvent(EVENT_CHECK_IDLE, now + CHECK_IDLE_FREQ, "");
      break;

    case EVENT_RENAME_LOGFILE:
      log("Closing log file");
      close(logfile);
      rename(LOGFILE, LOGFILEBAK);
      logfile = open(LOGFILE, O_WRONLY | O_CREAT | O_APPEND, 0600);	/* "a" in case rename() fails */
      log("Opening log file");
      AddEvent(EVENT_RENAME_LOGFILE, now + RENAME_LOGFILE_FREQ, "");
      break;

    case EVENT_SYNCDB:
      if (DB_Save_Status == -1)
      {
	DB_Save_Status = 0;
	*DB_Save_Nick = '\0';
      }
      /*SaveUserList("",""); */
      /*
         gather_sync_channels();
         if(DBSync==NULL)
         sync_next_channel();
         sync_next_channel();
       */
      AddEvent(EVENT_SYNCDB, now + CACHE_TIMEOUT, "");
      break;

#ifdef CHANNEL_LOG
    case EVENT_LOG_CHANNEL:
      LogChan();
      AddEvent(EVENT_LOG_CHANNEL, now + CHANNEL_LOG_FREQ, "");
      break;
#endif
#ifdef NICKSERV
    case EVENT_CHECKREGNICK:
      nserv_checkregnick(curr->param);
      break;

    case EVENT_SAVENICKSERV:
      nserv_save();
      break;
#endif
    default:
#ifdef DEBUG
      printf("Err.. unknown event %d\n", curr->event);
#endif
      break;
    }

    TTLALLOCMEM -= sizeof(anevent);
    free(curr);
  }
}

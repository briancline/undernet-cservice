/* @(#)$Id: dbio.c,v 1.12 1998/01/02 18:30:08 seks Exp $ */

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


char *make_dbfname(char *channel)
{
  static char fname[600];
  char tmp[600];
  register char *ptr;

  strcpy(tmp, channel);
  for (ptr = tmp; *ptr; ptr++)
  {
    if (*ptr == '/')
      *ptr = ' ';
    else
      *ptr = tolower(*ptr);
  }

  sprintf(fname, "db/channels/%04X/%s", ul_hash(channel), tmp);
  return fname;
}


/* db_query()
 * This is used to *retreive* information from the database.
 * The arguments are:
 *   channel   := channel name (null terminated string)
 *   type      := query type, either of:
 *                 DBGETNICK  (Get entry by nick)
 *                 DBGET1STUH (Get first matching userhost)
 *                 DBGETALLUH (Get all matching userhosts)
 *                 DBCOUNTUH  (Count the number of matching userhosts)
 *                 ...
 *   info      := Either a nick or a userhost depending on type
 *   action    := value passed to the callback function
 *   hook1     := pointer passed to the callback function
 *   hook2     := pointer passed to the callback function
 *   callback  := callback function
 *
 * Since the database is accessed asychronously, 'hook' should not point to
 * a structure that can be free()'d by a function other than callback,
 * e.g. a pointer to a luser structure.
 */
int db_fetch(char *channel, unsigned int type, char *info, char *passwd,
  int action, void *hook1, void *hook2, DBCALLBACK(callback))
{
  dbquery *new;
  int fd;

  if ((fd = open(make_dbfname(channel), O_RDONLY | O_NONBLOCK)) < 0)
  {
    callback(&fd, (off_t) 0, action, hook1, hook2, NULL, 0);
    return -1;	/* caller can check errno to see want happened */
  }

  new = (dbquery *) malloc(sizeof(dbquery));
  new->fd = fd;
  new->offset = (off_t) 0;
  new->time = now;
  new->type = type;
  new->action = action;
  new->count = 0;
  new->callback = callback;
  new->buf = NULL;
  new->hook1 = hook1;
  new->hook2 = hook2;
  strcpy(new->info, info);
  strncpy(new->channel, channel, 79);
  new->channel[79] = '\0';
  if (passwd)
    strcpy(new->passwd, passwd);
  else
    new->passwd[0] = '\0';
  new->next = DBQuery;
  DBQuery = new;

  return 0;
}

/* fix_db()
 * Remove db entries that are not in the right file.
 */
static void fix_db(char *channel, off_t offset)
{
  register RegUser *reg;
  register int idx;

  idx = ul_hash(channel);
  /* check if already taken care of */
  reg = UserList[idx];
  while (reg != NULL && (reg->offset != offset || strcasecmp(reg->channel, channel)))
    reg = reg->next;

  if (reg)	/* already there.. disregard */
    return;

  reg = (RegUser *) MALLOC(sizeof(RegUser));
  memset(reg, 0, sizeof(RegUser));
  reg->realname = (char *)MALLOC(6);
  strcpy(reg->realname, "!DEL!");
  reg->passwd = (char *)MALLOC(1);
  *reg->passwd = '\0';
  reg->match = (char *)MALLOC(6);
  strcpy(reg->match, "!DEL!");
  reg->channel = (char *)MALLOC(strlen(channel) + 1);
  strcpy(reg->channel, channel);
  reg->modif = (char *)MALLOC(1);
  *reg->modif = '\0';
  reg->modified = 1;
  reg->offset = offset;
  reg->next = UserList[idx];
  UserList[idx] = reg;
}

/* read_db()
 * This should be called from the select() loop. Data is read sequentially
 * and the query is processed according to 'type'.
 */
void read_db(dbquery * query)
{
  struct dbuser buffer[11];
  int size, status, end = 0;

  size = read(query->fd, buffer, 10 * sizeof(dbuser));
  if (size <= 0)
  {
    if (errno != EAGAIN && errno != EWOULDBLOCK)
    {
      close(query->fd);
      query->fd = -1;
    }
    return;
  }

  copy_to_buffer(&query->buf, (char *)buffer, size);
  while (!end && look_in_buffer(&query->buf, (char *)buffer, '\0', sizeof(dbuser))
    == sizeof(dbuser))
  {
    if (buffer[0].header[0] == 0xFF && buffer[0].header[1] == 0xFF &&
      buffer[0].footer[0] == 0xFF && buffer[0].footer[1] == 0xFF)
    {
      status = 1;	/* block is used */
    }
    else if (buffer[0].header[0] == 0x00 && buffer[0].header[1] == 0x00 &&
      buffer[0].footer[0] == 0x00 && buffer[0].footer[1] == 0x00)
    {
      status = 0;	/* block is free */
    }
    else
    {
      status = -1;	/* block has been written to while reading it */
    }

    if (status == 1 && strcasecmp(query->channel, buffer[0].channel))
    {
      fix_db(query->channel, query->offset);
      skip_char_in_buffer(&query->buf, sizeof(dbuser));
      query->offset += sizeof(dbuser);
      continue;
    }


#ifdef DEBUG
    printf("hdr: %X%X nick: %s match: %s passwd: %s channel: %s "
      "modif: %s access: %d flags: %ld susp: %ld last: %ld ftr: %X%X"
      "sta: %d\n",
      buffer[0].header[0], buffer[0].header[1], buffer[0].nick, buffer[0].match, buffer[0].passwd,
      buffer[0].channel, buffer[0].modif, buffer[0].access, buffer[0].flags, buffer[0].suspend,
      buffer[0].lastseen, buffer[0].footer[0], buffer[0].footer[1], status);
#endif

    switch (query->type)
    {
    case DBGETNICK:
      if (status == 1 && !strcasecmp(buffer[0].nick, query->info))
      {
	close(query->fd);
	query->fd = -1;
	query->count++;
	query->callback(&query->fd, query->offset, query->action, query->hook1,
	  query->hook2, buffer, query->count);
	end = 1;
      }
      break;
    case DBGET1STUH:
      if (status == 1 && match(query->info, buffer[0].match))
      {
	close(query->fd);
	query->fd = -1;
	query->count++;
	query->callback(&query->fd, query->offset, query->action, query->hook1,
	  query->hook2, buffer, query->count);
	end = 1;
      }
      break;
    case DBGETALLUH:
      if (status == 1 && match(query->info, buffer[0].match))
      {
	query->count++;
	query->callback(&query->fd, query->offset, query->action, query->hook1,
	  query->hook2, buffer, query->count);
      }
      break;
    case DBGETUHPASS:
      if (status == 1 && match(query->info, buffer[0].match) &&
	!strcmp(query->passwd, buffer[0].passwd))
      {
	close(query->fd);
	query->fd = -1;
	query->count++;
	query->callback(&query->fd, query->offset, query->action, query->hook1,
	  query->hook2, buffer, query->count);
	end = 1;
      }
      break;
    case DBCOUNTUH:
      if (status == 1 && match(query->info, buffer[0].match))
      {
	query->count++;
      }
      break;
    case DBGET1STCMP:
      if (status == 1 && (!*query->info || compare(query->info, buffer[0].match)))
      {
	close(query->fd);
	query->fd = -1;
	query->count++;
	query->callback(&query->fd, query->offset, query->action, query->hook1,
	  query->hook2, buffer, query->count);
	end = 1;
      }
      break;
    case DBGETALLCMP:
      if (status == 1 && (!*query->info || compare(query->info, buffer[0].match)))
      {
	query->count++;
	query->callback(&query->fd, query->offset, query->action, query->hook1,
	  query->hook2, buffer, query->count);
      }
      break;
    case DBCNTCMP:
      if (status == 1 && (!*query->info || compare(query->info, buffer[0].match)))
      {
	query->count++;
      }
      break;
    case DBGET1STFREE:
      if (status == 0)
      {
	close(query->fd);
	query->fd = -1;
	query->count++;
	query->callback(&query->fd, query->offset, query->action, query->hook1,
	  query->hook2, buffer, query->count);
	end = 1;
      }
      break;
    }
    skip_char_in_buffer(&query->buf, sizeof(dbuser));
    query->offset += sizeof(dbuser);
    if (query->fd < 0)
      end = 1;
  }
}

void end_db_read(dbquery * query)
{
  zap_buffer(&query->buf);
  query->callback(&query->fd, query->offset, query->action, query->hook1,
    query->hook2, NULL, query->count);
}


void make_dbuser(RegUser * reg, dbuser * dbu)
{
  memset(dbu, 0, sizeof(dbuser));
  if (reg->access != 0)
  {
    strncpy(dbu->nick, reg->realname, 79);
    dbu->nick[79] = '\0';;
    strncpy(dbu->match, reg->match, 79);
    dbu->match[79] = '\0';
    strncpy(dbu->channel, reg->channel, 49);
    dbu->channel[49] = '\0';
    strncpy(dbu->passwd, reg->passwd, 19);
    dbu->passwd[19] = '\0';
    strncpy(dbu->modif, reg->modif, 79);
    dbu->modif[79] = '\0';
    dbu->access = reg->access;
    dbu->flags = reg->flags;
    dbu->suspend = reg->suspend;
    dbu->lastseen = reg->lastseen;
    dbu->header[0] = 0xFF;
    dbu->header[1] = 0xFF;
    dbu->footer[0] = 0xFF;
    dbu->footer[1] = 0xFF;
  }
}

void sync_next_channel(void)
{
  register syncchan *tmp;

  if ((tmp = SyncChan) != NULL)
  {
    SyncChan = SyncChan->next;
    db_sync(tmp->name);
    free(tmp);
  }
}

static void set_sync(dbsync * sync)
{
  struct dbuser dbu;

  if (*sync->reg == NULL)
  {
    close(sync->fd);
    sync->fd = -1;
    return;
  }

  if ((*sync->reg)->access == 0)	/* delete */
  {
    make_dbuser(*sync->reg, &dbu);
    zap_buffer(&sync->buf);
    copy_to_buffer(&sync->buf, (char *)&dbu, sizeof(dbuser));
    sync->type = SYNC_DELETE;
    sync->status = SYNC_PENDWRITE;
    sync->offset = (*sync->reg)->offset;
    lseek(sync->fd, (*sync->reg)->offset, SEEK_SET);
  }
  else if ((*sync->reg)->offset != (off_t) - 1)		/* update */
  {
    make_dbuser(*sync->reg, &dbu);
    zap_buffer(&sync->buf);
    copy_to_buffer(&sync->buf, (char *)&dbu, sizeof(dbuser));
    sync->type = SYNC_UPDATE;
    sync->status = SYNC_PENDWRITE;
    sync->offset = (*sync->reg)->offset;
    lseek(sync->fd, (*sync->reg)->offset, SEEK_SET);
  }
  else
    /* add */
  {
    sync->type = SYNC_ADD;
    sync->status = SYNC_SEEKFREE;
    sync->offset = (off_t) 0;
    lseek(sync->fd, 0L, SEEK_SET);
  }
}


static void sync_next(dbsync * sync, char *channel)
{
  register RegUser **reg = sync->reg;

  if (*reg == NULL)
  {
    close(sync->fd);
    sync->fd = -1;
    return;
  }

  while (*reg != NULL && ((*reg)->modified == 0 || (*reg)->access == 1000 ||
      strcasecmp((*reg)->channel, channel)))
  {
    if ((*reg)->inuse == 0 && (*reg)->lastused + CACHE_TIMEOUT < now)
      free_user(reg);
    else
      reg = &(*reg)->next;
  }

  sync->time = now;
  sync->reg = reg;
  set_sync(sync);
}



void db_sync(char *channel)
{
  register RegUser **reg, *tmp;
  register char *ch;
  register dbsync *sync;
  struct stat st;
  int fd;

#ifdef DEBUG
  printf("SYNC: %s\n", channel);
#endif

  if (DBSync != NULL && (DBSync->fd != -1 || DBSync->next != NULL))
  {
    log("ERROR: simultaneous syncs??");
    return;
  }

  ch = make_dbfname(channel);

  /* IF file does not exist.. remove all deletes and mark
   * all other entries as new.
   */
  if (stat(ch, &st) < 0)
  {
    /* remove all deletes */
    reg = &UserList[ul_hash(channel)];
    while (*reg)
    {
      (*reg)->offset = (off_t) - 1;
      if ((*reg)->access == 0 && (*reg)->inuse == 0 &&
	!strcasecmp((*reg)->channel, channel))
      {
	tmp = *reg;
	*reg = (*reg)->next;
	free_user(reg);
      }
      else
	reg = &(*reg)->next;
    }
    fd = open(ch, O_RDWR | O_CREAT | O_EXCL | O_NONBLOCK, 0600);
  }
  else
  {
    fd = open(ch, O_RDWR | O_NONBLOCK);
  }

  /* create new sync struct
   */
  sync = (dbsync *) malloc(sizeof(dbsync));
  sync->fd = fd;
  sync->reg = &UserList[ul_hash(channel)];
  sync->buf = NULL;
  sync->next = DBSync;
  DBSync = sync;

  sync_next(sync, channel);
}


void db_sync_ready(dbsync * sync)
{
  struct dbuser buffer[10];
  register RegUser **reg = sync->reg;
  int size;

#ifdef DEBUG
  printf("SYNC_READY: status= %d\n", sync->status);
#endif

  if (sync->status == SYNC_PENDWRITE)
  {
    size = look_in_buffer(&sync->buf, (char *)buffer, '\0', sizeof(dbuser));
    if (size == 0)
      zap_buffer(&sync->buf);
    if (sync->buf == NULL)
    {
      if ((*reg)->access == 0 && (*reg)->inuse == 0)
	free_user(reg);
      else
	(*reg)->modified = 0;
      sync_next(sync, (*reg)->channel);
      return;
    }
#ifdef DEBUG
    printf("PENDWRITE: %d bytes to write...", size);
#endif
    size = write(sync->fd, buffer, size);
#ifdef DEBUG
    printf("%d written\n", size);
#endif
    if (size <= 0)
    {
      if (errno != EWOULDBLOCK && errno != EAGAIN)
      {
	close(sync->fd);
	sync->fd = -1;
      }
      return;
    }
    skip_char_in_buffer(&sync->buf, size);
  }

  else if (sync->status == SYNC_SEEKFREE)
  {
    size = read(sync->fd, buffer, 10 * sizeof(dbuser));
    if (size <= 0)
    {
      if (errno != EAGAIN && errno != EWOULDBLOCK)
      {
	make_dbuser(*sync->reg, buffer);
	zap_buffer(&sync->buf);
	copy_to_buffer(&sync->buf, (char *)buffer, sizeof(dbuser));
	sync->status = SYNC_PENDWRITE;
	lseek(sync->fd, 0L, SEEK_END);
      }
      return;
    }

    copy_to_buffer(&sync->buf, (char *)buffer, size);
    while (look_in_buffer(&sync->buf, (char *)buffer, '\0', sizeof(dbuser))
      == sizeof(dbuser))
    {
      if (buffer[0].header[0] == 0x00 && buffer[0].header[1] == 0x00 &&
	buffer[0].footer[0] == 0x00 && buffer[0].footer[1] == 0x00)
      {
	make_dbuser(*sync->reg, buffer);
	zap_buffer(&sync->buf);
	copy_to_buffer(&sync->buf, (char *)buffer, sizeof(dbuser));
	sync->status = SYNC_PENDWRITE;
	(*sync->reg)->offset = sync->offset;
	lseek(sync->fd, sync->offset, SEEK_SET);
	return;
      }
      skip_char_in_buffer(&sync->buf, sizeof(dbuser));
      sync->offset += sizeof(dbuser);
    }
  }
}

void end_db_sync(dbsync * sync)
{
#ifdef DEBUG
  printf("SYNC_END\n");
#endif
  sync_next_channel();
  return;
}


void cold_save_one(RegUser * reg)
{
  struct stat st;
  dbuser dbu;
  char *fname;
  register int fd;
  off_t off;

  fname = make_dbfname(reg->channel);

  if (stat(fname, &st) < 0)	/* file doesn't exist -- don't save delete */
  {
    if (reg->access == 0)
      return;
    fd = open(fname, O_RDWR | O_CREAT, 0600);
  }
  else
    fd = open(fname, O_RDWR | O_EXCL);

  if (fd < 0)	/* hmmm? */
    return;

  if (reg->offset == (off_t) - 1)	/* new ==> seek for empty slot */
  {
    lseek(fd, 0L, SEEK_SET);
    off = (off_t) 0;
    while (read(fd, &dbu, sizeof(dbuser)) == sizeof(dbuser))
    {
      if (dbu.header[0] == 0x00 && dbu.header[1] == 0x00 &&
	dbu.footer[0] == 0x00 && dbu.footer[1] == 0x00)
	break;	/* found empty slot */
      off += sizeof(dbuser);
    }
  }
  else
    off = reg->offset;

  lseek(fd, off, SEEK_SET);	/* go to write position */

  if (reg->access != 0)
    make_dbuser(reg, &dbu);
  else
    memset(&dbu, 0, sizeof(dbuser));

  write(fd, &dbu, sizeof(dbuser));

  if (reg->access != 0)
    reg->offset = off;
  else
    reg->offset = (off_t) - 1;

  reg->modified = 0;

  close(fd);
}


void do_cold_sync_slice(void)
{
  register RegUser **reg;

  if (DB_Save_Status < 0)
    return;

  reg = &UserList[DB_Save_Status];
  while (*reg != NULL)
  {
    if ((*reg)->modified && (*reg)->access < 1000)
      cold_save_one(*reg);
    if ((*reg)->inuse == 0 && (*reg)->access < 1000 &&
      ((*reg)->access == 0 || (*reg)->lastused + CACHE_TIMEOUT < now))
      free_user(reg);
    else
      reg = &(*reg)->next;
  }

  if (++DB_Save_Status == 1000)
  {
    DB_Save_Status = -1;
    if (*DB_Save_Nick)
      notice(DB_Save_Nick, "Userlist sync complete!");
  }
}


void do_cold_sync(void)
{
  register dbsync *sync;
  register syncchan *schan;
  register RegUser **reg;
  register int i;
  char buffer[200];

  /* SET AWAY MESSAGE */
  sprintf(buffer, ":%s AWAY :Busy saving precious user list\n", mynick);
  sendtoserv(buffer);
  dumpbuff();

  /* First, cancel *all* sync requests */
  while ((sync = DBSync) != NULL)
  {
    DBSync = DBSync->next;
    if (sync->fd >= 0)
      close(sync->fd);
    zap_buffer(&sync->buf);
    free(sync);
  }

  /* Also, clear *all* pending syncs */
  while ((schan = SyncChan) != NULL)
  {
    SyncChan = SyncChan->next;
    free(schan);
  }

  /* Now, go thru *all* the user entries in memory and save them
   * to their respective files.
   */
  for (i = 0; i < 1000; i++)
  {
    reg = &UserList[i];
    while (*reg != NULL)
    {
      if ((*reg)->modified && (*reg)->access < 1000)
	cold_save_one(*reg);
      if ((*reg)->inuse == 0 && (*reg)->access < 1000 &&
	((*reg)->access == 0 || (*reg)->lastused + CACHE_TIMEOUT < now))
	free_user(reg);
      else
	reg = &(*reg)->next;
    }
  }
  sprintf(buffer, ":%s AWAY\n", mynick);
  sendtoserv(buffer);
}




#ifdef DEBUG
void db_test_callback(int *fd, off_t off, int action, void *hook1, void *hook2,
  dbuser * dbu, int count)
{
  char buffer[512];

  sprintf(buffer, "off: %ld act: %d hook=%p cnt: %d", off, action, hook1, count);
  notice((char *)hook1, buffer);

  if (dbu)
  {
    sprintf(buffer, "hdr: %X%X nick: %s match: %s passwd: %s channel: %s "
      "modif: %s access: %d flags: %ld susp: %ld last: %ld ftr: %X%X",
      dbu->header[0], dbu->header[1], dbu->nick, dbu->match, dbu->passwd,
      dbu->channel, dbu->modif, dbu->access, dbu->flags, dbu->suspend,
      dbu->lastseen, dbu->footer[0], dbu->footer[1]);
    notice((char *)hook1, buffer);
  }
  else
  {
    notice((char *)hook1, "End.");
    free(hook1);
  }
}

void db_test(char *source, char *chan, char *args)
{
  char channel[80], type[80], info[80], *hook;

  GetWord(0, args, channel);
  GetWord(1, args, type);
  GetWord(2, args, info);
  hook = (char *)malloc(strlen(source) + 1);
  strcpy(hook, source);

  if (!strcmp(type, "nick"))
    db_fetch(channel, DBGETNICK, info, NULL, 0, hook, NULL, db_test_callback);

  else if (!strcmp(type, "alluh"))
    db_fetch(channel, DBGETALLUH, info, NULL, 0, hook, NULL, db_test_callback);

  else if (!strcmp(type, "1stuh"))
    db_fetch(channel, DBGET1STUH, info, NULL, 0, hook, NULL, db_test_callback);

  else if (!strcmp(type, "count"))
    db_fetch(channel, DBCOUNTUH, info, NULL, 0, hook, NULL, db_test_callback);

  else if (!strcmp(type, "1stcmp"))
    db_fetch(channel, DBGET1STCMP, info, NULL, 0, hook, NULL, db_test_callback);

  else if (!strcmp(type, "allcmp"))
    db_fetch(channel, DBGETALLCMP, info, NULL, 0, hook, NULL, db_test_callback);

  else if (!strcmp(type, "cntcmp"))
    db_fetch(channel, DBCNTCMP, info, NULL, 0, hook, NULL, db_test_callback);

  else if (!strcmp(type, "sync"))
    db_sync(channel);
}
#endif

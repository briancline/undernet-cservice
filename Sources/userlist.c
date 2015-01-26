/* @(#)$Id: userlist.c,v 1.56 2000/10/24 16:14:43 seks Exp $ */

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

#define VALIDMASK "^.+!.*[^*?]@(((.+\\.)?[^*?]+\\.[a-z]+)|([0-9]+\\.[0-9]+\\.[0-9*]+(\\.[0-9*]+)?))$"

typedef struct OldDiskUser
{
  char realname[80];
  char match[80];
  char passwd[20];
  char channel[80];
  char modif[80];
  int Access;
  unsigned long flags;
  time_t suspend;
  time_t lastseen;
}
OldDiskUser;



struct modinfo_struct
{
  char source[20];
  char field[80];
  char newvalue[80];
};


struct showaccess_struct
{
  char source[80];
  char target[80];
  char chaninfo[80];
  char modifby[80];
  int nicksearch;
  int min, max;
  int mask, xmask;
  int modif;
};


int ul_hash(char *channel)
{
  register int i, j = 0;
  for (i = 1; i < strlen(channel); i++)
    j += (unsigned char)toupper(channel[i]);
  return (j % 1000);
}


void free_user(RegUser ** head)
{
  register RegUser *tmp = *head;

  if ((*head)->inuse != 0)
    log("ERROR!!!! free_user(): inuse != 0");

  *head = (*head)->next;
  TTLALLOCMEM -= strlen(tmp->realname) + 1;
  free(tmp->realname);
  TTLALLOCMEM -= strlen(tmp->match) + 1;
  free(tmp->match);
  TTLALLOCMEM -= strlen(tmp->passwd) + 1;
  free(tmp->passwd);
  TTLALLOCMEM -= strlen(tmp->channel) + 1;
  free(tmp->channel);
  TTLALLOCMEM -= strlen(tmp->modif) + 1;
  free(tmp->modif);
  TTLALLOCMEM -= sizeof(RegUser);
  free(tmp);
}
void LoadUserList(char *source)
{
  RegUser *curr;
  char fname[256];
  OldDiskUser tmp;
  dbuser db;
  struct stat st;
  int in, out, i;

  /* store master info */
  curr = (RegUser *) MALLOC(sizeof(RegUser));
  memset(curr, 0, sizeof(RegUser));

  curr->realname = (char *)MALLOC(strlen(MASTER_REALNAME) + 1);
  strcpy(curr->realname, MASTER_REALNAME);

  curr->access = MASTER_ACCESS;

  curr->passwd = (char *)MALLOC(strlen(MASTER_PASSWD) + 1);
  strcpy(curr->passwd, MASTER_PASSWD);

  curr->match = (char *)MALLOC(strlen(MASTER_MATCH) + 1);
  strcpy(curr->match, MASTER_MATCH);

  curr->channel = (char *)MALLOC(strlen(MASTER_CHANNEL) + 1);
  strcpy(curr->channel, MASTER_CHANNEL);

  curr->modif = (char *)MALLOC(1);
  curr->modif[0] = '\0';

  curr->flags = MASTER_FLAGS;
  curr->lastseen = now;
  curr->suspend = (time_t) 0;

  curr->modified = 0;
  curr->lastused = now + now;	/* Don't swap */
  curr->offset = (off_t) - 1;
  curr->next = UserList[0];
  UserList[0] = curr;

  /* We don't *load* the database anymore. But if it doesn't
   * exist, we need to create it, possibly from the old userlist.dat
   * file.
   */
  if (stat("db/channels/0000", &st) >= 0)
    return;

  mkdir("db", 0700);
  mkdir("db/channels", 0700);

  if (stat("db/channels", &st) < 0)
    return;

  for (i = 0; i < 1000; i++)
  {
    sprintf(fname, "db/channels/%04X", i);
    mkdir(fname, 0700);
  }

  if ((in = open(USERFILE, O_RDONLY)) < 0)
    return;

  while (read(in, &tmp, sizeof(tmp)) > 0)
  {
    if (tmp.Access == 0)
      continue;
    memset(&db, 0, sizeof(db));
    db.header[0] = 0xff;
    db.header[1] = 0xff;
    db.footer[0] = 0xff;
    db.footer[1] = 0xff;
    strcpy(db.nick, tmp.realname);
    strcpy(db.match, tmp.match);
    if (strcmp(tmp.passwd, "*"))
      strcpy(db.passwd, tmp.passwd);
    strcpy(db.channel, tmp.channel);
    strcpy(db.modif, tmp.modif);
    db.access = tmp.Access;
    db.flags = (tmp.flags & UFL_AUTOOP);
    db.suspend = tmp.suspend;
    db.lastseen = tmp.lastseen;
    if ((out = open(make_dbfname(tmp.channel),
	  O_WRONLY | O_APPEND | O_CREAT, 0600)) >= 0)
    {
      write(out, &db, sizeof(db));
      close(out);
    }
  }
}

RegUser *IsValid(aluser * luser, char *channel)
{
  register avalchan *valchan;

  if (luser == NULL)
    return NULL;

  valchan = luser->valchan;
  while (valchan != NULL)
  {
    if (!strcasecmp(valchan->name, channel))
      break;
    valchan = valchan->next;
  }
#ifdef DEBUG
  printf("IsValid() --> %p\n", valchan);
#endif

  return (valchan) ? valchan->reg : NULL;
}


void try_find(char *channel, aluser * user)
{
  register RegUser *reg;
  register avalchan *vchan;
  char userhost[200];
  sprintf(userhost, "%s!%s@%s", user->nick, user->username, user->site);

  if (IsValid(user, channel))
    return;

  reg = UserList[ul_hash(channel)];
  while (reg != NULL)
  {
    if (!strcasecmp(reg->channel, channel) &&
      match(userhost, reg->match) &&
      *reg->passwd == '\0')
    {
      vchan = (avalchan *) MALLOC(sizeof(avalchan));
      vchan->name = (char *)MALLOC(strlen(channel) + 1);
      strcpy(vchan->name, channel);
      vchan->reg = reg;
      reg->inuse++;
      reg->lastseen = now;
      reg->modified = 1;
      vchan->next = user->valchan;
      user->valchan = vchan;
      break;
    }
    reg = reg->next;
  }
}

int LAccess(char *channel, aluser * user)
{
  register avalchan *vchan;

  if (user == NULL)
    return 0;

  vchan = user->valchan;
  while (vchan != NULL && strcasecmp(channel, vchan->name))
    vchan = vchan->next;

#ifdef DEBUG
  printf("LAcccess()= %d\n", (vchan != NULL) ? vchan->reg->access : 0);
#endif
  if (vchan == NULL || vchan->reg->suspend >= now ||
    vchan->reg->passwd[0] == '\0')
    return 0;

  return vchan->reg->access;
}

int Access(char *channel, char *nick)
{
  return LAccess(channel, ToLuser(nick));
}


RegUser *load_dbuser(off_t offset, dbuser * dbu)
{
  register RegUser *new, *scan;
  register int idx;

#ifdef DEBUG
  if (dbu)
    printf("load: hdr: %X%X nick: %s match: %s passwd: %s channel: %s "
      "modif: %s access: %d flags: %ld susp: %ld last: %ld ftr: %X%X\n",
      dbu->header[0], dbu->header[1], dbu->nick, dbu->match, dbu->passwd,
      dbu->channel, dbu->modif, dbu->access, dbu->flags, dbu->suspend,
      dbu->lastseen, dbu->footer[0], dbu->footer[1]);
#endif

  idx = ul_hash(dbu->channel);

  /* Make sure that entry is not already on memory. If it is,
   * there is no need to load it again.
   */
  scan = UserList[idx];
  while (scan && (scan->offset != offset ||
      strcasecmp(scan->channel, dbu->channel)))
    scan = scan->next;

  if (scan != NULL)
  {
    if (scan->access == 0 || strcmp(dbu->nick, scan->realname))
      return NULL;
    return scan;
  }

  new = (RegUser *) MALLOC(sizeof(RegUser));
  memset(new, 0, sizeof(RegUser));
  new->realname = (char *)MALLOC(strlen(dbu->nick) + 1);
  strcpy(new->realname, dbu->nick);
  new->match = (char *)MALLOC(strlen(dbu->match) + 1);
  strcpy(new->match, dbu->match);
  new->channel = (char *)MALLOC(strlen(dbu->channel) + 1);
  strcpy(new->channel, dbu->channel);
  new->passwd = (char *)MALLOC(strlen(dbu->passwd) + 1);
  strcpy(new->passwd, dbu->passwd);
  new->modif = (char *)MALLOC(strlen(dbu->modif) + 1);
  strcpy(new->modif, dbu->modif);
  new->access = dbu->access;
  new->flags = dbu->flags;
  new->suspend = dbu->suspend;
  new->lastseen = dbu->lastseen;
  new->offset = offset;
  new->inuse = 0;
  new->modified = 0;
  new->lastused = now;
  new->next = UserList[idx];
  UserList[idx] = new;

  return new;
}


static void successful_auth(aluser * luser, char *channel, RegUser * reg)
{
  char buffer[512];
  register avalchan *vchan;
  register achannel *chan;
  time_t last;

  vchan = (avalchan *) MALLOC(sizeof(avalchan));
  vchan->name = (char *)MALLOC(strlen(channel) + 1);
  strcpy(vchan->name, channel);
  vchan->reg = reg;
  reg->inuse++;
  reg->lastseen = now;
  reg->modified = 1;
  vchan->next = luser->valchan;
  luser->valchan = vchan;

  if (reg->passwd[0] == '\0')
  {
    sprintf(buffer, "Passwords are now mandatory. "
      "Please use the 'newpass' command now.");
  }
  else
  {
    sprintf(buffer, "AUTHENTICATION SUCCESSFUL ON %s!",
      channel);
  }
  notice(luser->nick, buffer);
  if (reg->suspend > now)
    notice(luser->nick, "... however, your access is suspended!");

  sprintf(buffer, "AUTH: %s!%s@%s as %s on %s",
    luser->nick, luser->username, luser->site,
    reg->realname, reg->channel);
  log(buffer);

  last = (now - reg->lastseen) / 86400;
  if (last > 1)
  {
    sprintf(buffer, "I last saw you %ld days ago", last);
    notice(luser->nick, buffer);
  }
  if (strcmp(channel, "*"))
  {
    if (!strcmp(reg->channel, "*"))
    {
      sprintf(buffer, "You now have access on %s", channel);
      notice(luser->nick, buffer);
      sprintf(buffer, "%s is getting access on %s",
	luser->nick, channel);
      broadcast(buffer, 1);
    }
    chan = ToChannel(channel);
    sprintf(buffer, "%s!%s@%s", luser->nick, luser->username,
      luser->site);
    if (chan && chan->on && chan->AmChanOp
      && reg && (reg->flags & UFL_AUTOOP)
      && reg->suspend < now
      && !(chan->flags & CFL_NOOP)
      && IsShit(channel, buffer, NULL, NULL) < NO_OP_SHIT_LEVEL
      )
    {
      op("", channel, luser->nick);
    }

  }
  else
  {
    if (reg->access < 500)
      notice(luser->nick, "You are now an authenticated helper");
    else
      notice(luser->nick, "You now have administrative access");
  }
}

static void
 validate_callback(int *fd, off_t off, int action, void *hook1, void *hook2,
  dbuser * dbu, int count)
{
  register aluser *luser;
  register RegUser *reg;
  char userhost[200];
#ifdef DEBUG
  if (dbu)
    printf("vall: hdr: %X%X nick: %s match: %s passwd: %s channel: %s "
      "modif: %s access: %d flags: %ld susp: %ld last: %ld ftr: %X%X\n",
      dbu->header[0], dbu->header[1], dbu->nick, dbu->match, dbu->passwd,
      dbu->channel, dbu->modif, dbu->access, dbu->flags, dbu->suspend,
      dbu->lastseen, dbu->footer[0], dbu->footer[1]);
  else
    printf("vall: end.\n");
#endif

  if (dbu != NULL)
  {
    luser = ToLuser((char *)hook1);

    if (luser == NULL)
    {
      /* It is possible the user has quit or changed his nick
       * since the request for authentication was made. In
       * such a situation, we load the structure anyway, but
       * don't link it anywhere.
       */
      return;
    }

    reg = load_dbuser(off, dbu);

    sprintf(userhost, "%s!%s@%s", luser->nick, luser->username,
      luser->site);
    if (reg != NULL && match(userhost, reg->match))
    {
      successful_auth(luser, dbu->channel, reg);
    }
    else
    {
      char buf[200];
      sprintf(buf, "AUTHENTICATION FAILED ON %s!", dbu->channel);
      notice((char *)hook1, buf);
    }
  }
  else
  {
    if (count == 0)
    {
      char buf[200];
      sprintf(buf, "AUTHENTICATION FAILED ON %s!", (char *)hook2);
      notice((char *)hook1, buf);
    }
    free(hook1);
    free(hook2);
  }
}

void validate(char *source, char *target, char *args)
{
  char channel[80], passwd[80], userhost[200], buffer[200];
  register avalchan *vchan, **pvchan;
  register RegUser *ruser;
  register aluser *luser;

  luser = ToLuser(source);

  if (*args == '#' || *args == '*')
  {
    GetWord(0, args, channel);
    args = ToWord(1, args);
  }
  else
  {
    strcpy(channel, target);
    GuessChannel(source, channel);
    if (*channel != '#')
    {
      notice(source, "SYNTAX: login <channel> [password]");
      return;
    }
  }

  GetWord(0, args, passwd);

  if (*passwd && !strchr(target, '@'))
  {
    sprintf(buffer, "Please use /msg %s@%s login [channel] [password]",
      mynick, SERVERNAME);
    notice(source, buffer);
    return;
  }


#ifdef DEBUG
  printf("Authentication request..\nFROM: %s\nCHANNEL: %s\nPASSWORD: %s\n",
    source, channel, passwd);
#endif
  /* remove any existing link to a userlist structure for the
   * requested channel.
   */
  pvchan = &luser->valchan;
  while (*pvchan != NULL && strcasecmp((*pvchan)->name, channel))
    pvchan = &(*pvchan)->next;
  if (*pvchan != NULL)
  {
    vchan = *pvchan;
    *pvchan = (*pvchan)->next;
    vchan->reg->inuse--;
    vchan->reg->lastused = now;
    TTLALLOCMEM -= strlen(vchan->name) + 1;
    free(vchan->name);
    TTLALLOCMEM -= sizeof(avalchan);
    free(vchan);
  }

  sprintf(userhost, "%s!%s@%s", luser->nick, luser->username, luser->site);

  ruser = UserList[ul_hash(channel)];
  while (ruser != NULL)
  {
    if (!strcasecmp(ruser->channel, channel) &&
      match(userhost, ruser->match) &&
      !strcmp(ruser->passwd, passwd))
      break;
    ruser = ruser->next;
  }

  if (ruser == NULL)
  {	/* hmm might be in the admin list too.. */
    ruser = UserList[0];
    while (ruser != NULL)
    {
      if (!strcasecmp(ruser->channel, "*") &&
	match(userhost, ruser->match) &&
	!strcmp(ruser->passwd, passwd))
	break;
      ruser = ruser->next;
    }
  }

  if (ruser == NULL)
  {	/* ok.. not in memory.. send db query */
    char *hook1, *hook2;
    hook1 = (char *)malloc(strlen(source) + 1);
    hook2 = (char *)malloc(strlen(channel) + 1);
    strcpy(hook1, source);
    strcpy(hook2, channel);
    db_fetch(channel, DBGETUHPASS, userhost, passwd, 0,
      hook1, hook2, validate_callback);
  }
  else
  {
    successful_auth(luser, channel, ruser);
  }
}


void DeAuth(char *source, char *chan, char *args)
{
  char channel[80];
  char buffer[512];
  register aluser *luser;
  register avalchan *vchan, **pvchan;

  luser = ToLuser(source);

  if (*args == '#' || *args == '*')
  {
    GetWord(0, args, channel);
  }
  else
  {
    strcpy(channel, chan);
    GuessChannel(source, channel);
  }

  if (!*channel)
  {
    notice(source, "SYNTAX: deauth <#channel>");
    return;
  }

  pvchan = &luser->valchan;
  while (*pvchan != NULL && strcasecmp((*pvchan)->name, channel))
    pvchan = &(*pvchan)->next;
  if (*pvchan != NULL)
  {
    vchan = *pvchan;
    *pvchan = (*pvchan)->next;
    vchan->reg->inuse--;
    vchan->reg->lastused = now;
    TTLALLOCMEM -= strlen(vchan->name) + 1;
    free(vchan->name);
    TTLALLOCMEM -= sizeof(avalchan);
    free(vchan);
    sprintf(buffer, "You have been successfully deauthenticated on %s",channel);
    notice(source, buffer);
  }
  else
  {
    sprintf(buffer, "You do not appear to be authenticated on %s",channel);
    notice(source, buffer);
  }
}


static void
 adduser_callback(int *fd, off_t off, int action, void *hook1, void *hook2,
  dbuser * dbu, int count)
{
  RegUser *newuser = (RegUser *) hook2, *scan;
  char buffer[200];
  int ok = 0;

  if (dbu != NULL && count > 0)
  {
    /* check if entry was deleted */
    scan = UserList[ul_hash(dbu->channel)];
    while (scan != NULL && (scan->offset != off || scan->access != 0 ||
	strcasecmp(scan->channel, dbu->channel)))
    {
      scan = scan->next;
    }
    if (scan != NULL)
    {
      ok = 1;
    }
    else
    {
      if (load_dbuser(off, dbu) != NULL)
      {
	notice((char *)hook1, "This user is already present in the list.");
	sprintf(buffer, "NICK: %s MATCH: %s ACCESS: %d",
	  dbu->nick, dbu->match, dbu->access);
	notice((char *)hook1, buffer);
	free_user(&newuser);
      }
      else
	ok = 1;
    }
  }
  else if (count == 0)
  {
    ok = 1;
  }

  if (ok)
  {
    /* OK, go ahead and add the new entry in the list
     */
    newuser->next = UserList[ul_hash(newuser->channel)];
    UserList[ul_hash(newuser->channel)] = newuser;

    sprintf(buffer, "New user %s (%s) added on %s with access %d",
      newuser->realname, newuser->match, newuser->channel,
      newuser->access);
    notice((char *)hook1, buffer);
  }
  if (dbu == NULL)
    free(hook1);
}


void AddUser(char *source, char *ch, char *args)
{
  register RegUser *newuser, *curr;
  register aluser *luser, *src;
  register achannel *chan;
  void *hook;
  char buffer[500];
  char channel[80];
  char realname[80];
  char mask[80];
  char straccess[80];
  char password[80];
  char *ptr, *ptr2;
  int acc;
  int srcacs;
  RegUser *reg;
  time_t now1 = now + 3600;
  struct tm *tms = gmtime(&now1);

  src = ToLuser(source);
  if (*args == '#' || (*args == '*' && *(args + 1) == ' ' && IsValid(src, ch)))
  {
    GetWord(0, args, channel);
    args = ToWord(1, args);
  }
  else
  {
    strcpy(channel, ch);
    GuessChannel(source, channel);
  }

  if(CheckAdduserFlood(source, channel))
  {
    return;
  }

  GetWord(0, args, realname);
  GetWord(1, args, mask);
  if (isdigit(*mask) || *mask == '-')
  {
    strcpy(straccess, mask);
    GetWord(2, args, password);
    luser = ToLuser(realname);
    if (luser == NULL)
    {
      *mask = '\0';
    }
    else
    {
      MakeBanMask(luser, mask);
    }
  }
  else
  {
    GetWord(2, args, straccess);
    GetWord(3, args, password);
  }

  ptr = strchr(mask, '@');
  ptr2 = strchr(mask, '!');

  if (ptr == NULL || ptr2 == NULL || ptr2 > ptr ||
    strchr(ptr2 + 1, '!') || strchr(ptr + 1, '@'))
  {
    notice(source, "Invalid nick!user@host mask");
    return;
  }

  for (ptr = mask; *ptr; ptr++)
  {
    if (*ptr <= 32)
      break;
  }

  if (*ptr)
  {
    notice(source, "Invalid nick!user@host mask");
    return;
  }

  if (!regex_cmp(VALIDMASK, mask))
  {
    notice(source, "Invalid nick!user@host mask");
    return;
  }

  for (ptr = realname; *ptr; ptr++)
  {
    if (*ptr <= 32)
      break;
  }

  if (*ptr)
  {
    notice(source, "Invalid nick!");
    return;
  }

  if ((!strcmp(channel, "*") && !IsValid(src, channel)) ||
    !*realname || !*mask || !*straccess)
  {
    notice(source, "SYNTAX: adduser [#channel] <nick> [mask] <access> <password>");
    return;
  }

  srcacs = Access(channel, source);
  if (srcacs < ADD_USER_LEVEL)
  {
    ReplyNotAccess(source, channel);
    return;
  }

  if (*realname == '-' || strchr(realname, '*') || strchr(realname, '?'))
  {
    notice(source, "Can't add user.. bogus nick");
    return;
  }

  if (strlen(straccess) > 4)
  {
    notice(source, "Can't add user.. bogus access");
    return;
  }
  acc = atoi(straccess);

  /* can't add a user with a higher access than his! */

  if (srcacs <= acc)
  {
    notice(source, "Can't add a user with an access higher than or equal to yours");
    return;
  }

  if (acc == 0 || (*channel == '#' && acc < 0))
  {
    notice(source, "Can't add a user with access <= 0");
    return;
  }

  if (!*password && acc > 0)
  {
    notice(source, "A password must be supplied");
    return;
  }

  /* password must be at least 6 chars long! why?.. hmm why not! ;) */

  if (strlen(password) < 6)
  {
    notice(source, "Password must be at least 6 characters long");
    return;
  }

  if (strlen(password) > 18)
  {
    notice(source, "Password cannot be longer than 18 characters");
    return;
  }

  /* check if the user is not already in the list.. */
  curr = UserList[ul_hash(channel)];
  while (curr)
  {
    if (!strcasecmp(channel, curr->channel) &&
      !strcasecmp(curr->realname, realname))
    {
      break;
    }
    else
    {
      curr = curr->next;
    }
  }
  if (curr)
  {
    notice(source, "This user is already present in the list.");
    sprintf(buffer, "NICK: %s MATCH: %s ACCESS: %d",
      curr->realname, curr->match, curr->access);
    notice(source, buffer);
    return;
  }

  /* OK, there is no conflict with structures in memory. Now we
   * must check in the database. Create the structure and pass it
   * as a hook with the db query.
   */
  newuser = (RegUser *) MALLOC(sizeof(RegUser));
  memset(newuser, 0, sizeof(RegUser));

  newuser->realname = (char *)MALLOC(strlen(realname) + 1);
  strcpy(newuser->realname, realname);

  newuser->match = (char *)MALLOC(strlen(mask) + 1);
  strcpy(newuser->match, mask);

  newuser->access = acc;

  newuser->passwd = (char *)MALLOC(strlen(password) + 1);
  strcpy(newuser->passwd, password);

  newuser->channel = (char *)MALLOC(strlen(channel) + 1);
  strcpy(newuser->channel, channel);

  reg = IsValid(src, channel);

  sprintf(buffer, "%04d%02d%02d@%03ld (%s) %s!%s@%s",
    tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday,
    1000 * (now1 % 86400) / 86400,
    reg ? reg->realname : "?",
    src->nick, src->username, src->site);

  newuser->modif = (char *)MALLOC(strlen(buffer) + 1);
  strcpy(newuser->modif, buffer);

  newuser->suspend = 0;

  newuser->lastseen = now;

  newuser->offset = (off_t) - 1;
  newuser->modified = 1;

  newuser->next = NULL;

  if ((chan = ToChannel(channel)) != NULL)
  {
    newuser->flags = chan->uflags;
  }
  else
  {
    newuser->flags = 0;
  }

  hook = (char *)malloc(strlen(source) + 1);
  strcpy(hook, source);

  db_fetch(channel, DBGETNICK, newuser->realname, "", 0, hook, newuser,
    adduser_callback);
}


static void
 show_this_user_access_reg(char *source, RegUser * user, int modif)
{
  char buffer[512];
  time_t t;
  int days, hours, mins, secs;

  sprintf(buffer, "USER: %s (%s) ACCESS: %d L%s%s%s",
    user->realname, user->match, user->access,
    (user->modified == 1) ? "M" : "", (*user->passwd) ? "P" : "",
    (user->inuse > 0) ? "U" : "");
  notice(source, buffer);
  sprintf(buffer, "CHANNEL: %s -- AUTOOP: %s",
    user->channel,
    (user->flags & UFL_AUTOOP) ? "ON" : "OFF");
  notice(source, buffer);
  if (user->suspend > now)
  {
    sprintf(buffer, "SUSPEND EXP: %s",
      time_remaining(user->suspend - now));
    notice(source, buffer);
  }
  if (user->lastseen + MIN_LASTSEEN < now)
  {
    t = now - user->lastseen;
    days = (int)t / 86400;
    t %= 86400;
    hours = (int)t / 3600;
    t %= 3600;
    mins = (int)t / 60;
    t %= 60;
    secs = (int)t;

    *buffer = '\0';
    if (days > 0)
    {
      sprintf(buffer, "LAST SEEN: %d days, %02d:%02d:%02d ago",
	days, hours, mins, secs);
    }
    else
    {
      sprintf(buffer, "LAST SEEN: %02d:%02d:%02d ago",
	hours, mins, secs);
    }
    notice(source, buffer);
  }
  if (modif)
  {
    if (user->modif[0] == '\0')
      sprintf(buffer, "LAST MODIF: unknown");
    else
      sprintf(buffer, "LAST MODIF: %s", user->modif);
    notice(source, buffer);
  }
}



static void
 show_this_user_access_dbu(char *source, dbuser * dbu, int modif)
{
  char buffer[512];
  time_t t;
  int days, hours, mins, secs;

  sprintf(buffer, "USER: %s (%s) ACCESS: %d %s",
    dbu->nick, dbu->match, dbu->access,
    (*dbu->passwd) ? "P" : "");
  notice(source, buffer);
  sprintf(buffer, "CHANNEL: %s -- AUTOOP: %s",
    dbu->channel,
    (dbu->flags & UFL_AUTOOP) ? "ON" : "OFF");
  notice(source, buffer);
  if (dbu->suspend > now)
  {
    sprintf(buffer, "SUSPEND EXP: %s",
      time_remaining(dbu->suspend - now));
    notice(source, buffer);
  }
  if (dbu->lastseen + MIN_LASTSEEN < now)
  {
    t = now - dbu->lastseen;
    days = (int)t / 86400;
    t %= 86400;
    hours = (int)t / 3600;
    t %= 3600;
    mins = (int)t / 60;
    t %= 60;
    secs = (int)t;

    *buffer = '\0';
    if (days > 0)
    {
      sprintf(buffer, "LAST SEEN: %d days, %02d:%02d:%02d ago",
	days, hours, mins, secs);
    }
    else
    {
      sprintf(buffer, "LAST SEEN: %02d:%02d:%02d ago",
	hours, mins, secs);
    }
    notice(source, buffer);
  }
  if (modif)
  {
    if (dbu->modif[0] == '\0')
      sprintf(buffer, "LAST MODIF: unknown");
    else
      sprintf(buffer, "LAST MODIF: %s", dbu->modif);
    notice(source, buffer);
  }
}

static void
 showaccess_callback(int *fd, off_t off, int action, void *hook1, void *hook2,
  dbuser * dbu, int count)
{
  struct showaccess_struct *ptr = (struct showaccess_struct *)hook1;
  register RegUser *scan;
  int *cnt = (int *)hook2;

  if (dbu == NULL)
  {
    if (*cnt == 0 && count != 0)
      notice(ptr->source, "No match!");
    else if (*cnt != 0)
    {
      notice(ptr->source, "End of access list");
      CheckFloodFlood(ptr->source, (*cnt) + (*cnt));
    }
    else
      notice(ptr->source,
	"That channel doesn't appear to be registered");
    free(hook1);
    free(hook2);
    return;
  }

  scan = UserList[ul_hash(dbu->channel)];
  while (scan != NULL && (scan->offset != off ||
      strcasecmp(scan->channel, dbu->channel)))
    scan = scan->next;

  if (scan != NULL)
    return;

  if (((ptr->nicksearch && match(dbu->nick, ptr->target)) ||
      (!ptr->nicksearch && compare(ptr->target, dbu->match)) ||
      (ptr->nicksearch && match(ptr->chaninfo, dbu->match))) &&
    dbu->access >= ptr->min && dbu->access <= ptr->max &&
    (ptr->mask == 0 || (dbu->flags & ptr->mask) == ptr->mask) &&
    (ptr->xmask == 0 || (dbu->flags & ptr->xmask) == 0) &&
    (*ptr->modifby == '\0' || match(dbu->modif, ptr->modifby)))
  {
    show_this_user_access_dbu(ptr->source, dbu, ptr->modif);

    (*cnt)++;
    if (*cnt > 15 && ptr->source[0] != '+')
    {
      notice(ptr->source,
	"There are more than 15 matching entries");
      notice(ptr->source, "Please restrict your query");
      close(*fd);
      *fd = -1;
    }
  }
}


void showaccess(char *source, char *ch, char *args)
{
  char buffer[500], argument[80], channel[80];
  char realname[80], chaninfo[80] = "", modifby[80] = "";
  register RegUser *scan;
  register aluser *luser;
  register int nicksearch;
  int min = 0, max = MASTER_ACCESS, mask = 0, xmask = 0, modif = 0, srcacs,
   count;
  struct showaccess_struct *hook1;
  int *hook2;

  /* use another channel if provided as argument */
  if (*args == '#' || *args == '*')
  {
    GetWord(0, args, channel);
    args = ToWord(1, args);
  }
  else
  {
    strcpy(channel, ch);
    GuessChannel(source, channel);
  }

  /* if no name is provided.. use source */
  GetWord(0, args, realname);
  if (*realname == '-')
  {
    strcpy(realname, "*");
  }
  else if (!*realname)
  {
    strcpy(realname, source);
  }
  else
  {
    args = ToWord(1, args);
  }

  luser = ToLuser(realname);
  if (!strcmp(channel, "*") && !IsValid(ToLuser(source), channel))
  {
    notice(source, "SYNTAX: access <channel> [nick]");
    return;
  }

#ifdef DEBUG
  printf("SHOWACCESS:\nFROM: %s\nCHANNEL: %s\nWHO: %s\n",
    source, channel, args);
#endif

  /* parse arguments */
  while (*args)
  {
    GetWord(0, args, argument);
    args = ToWord(1, args);
    if (!strcasecmp(argument, "-min"))
    {
      min = atoi(args);
      args = ToWord(1, args);
    }
    else if (!strcasecmp(argument, "-max"))
    {
      max = atoi(args);
      args = ToWord(1, args);
    }
    else if (!strcasecmp(argument, "-autoop"))
    {
      mask |= UFL_AUTOOP;
    }
    else if (!strcasecmp(argument, "-noautoop"))
    {
      xmask |= UFL_AUTOOP;
    }
    else if (!strcasecmp(argument, "-modif"))
    {
      modif = 1;
      if (*args && *args != '-')
      {
	GetWord(0, args, modifby);
	args = ToWord(1, args);
      }
    }
    else
    {
      sprintf(buffer, "\"%s\": Unknown option", argument);
      notice(source, buffer);
      return;
    }
  }


  /* store the access of the person querying */
  srcacs = Access(channel, source);

  if (*source && srcacs < SHOW_ACCESS_LEVEL)
  {
    ReplyNotAccess(source, channel);
    return;
  }

  if (CurrentSendQ > HIGHSENDQTHRESHOLD)
  {
    notice(source, "Cannot process your request at this time. Try again later.");
    return;
  }


  /* if the user is online.. use his address */
  if (luser != NULL)
  {
    sprintf(chaninfo, "%s!%s@%s",
      luser->nick, luser->username, luser->site);
  }
#ifdef DEBUG
  printf("showaccess(): REALNAME= %s CHANINFO= %s\n", realname, chaninfo);
#endif

  nicksearch = 0;
  if (!strchr(realname, '@'))
  {
    if (strchr(realname, '!'))
      strcat(realname, "@*");
    else if (strchr(realname, '.'))
    {
      sprintf(buffer, "*!*@%s", realname);
      strcpy(realname, buffer);
    }
    else
      nicksearch = 1;
  }


  count = 0;
  scan = UserList[ul_hash(channel)];
  while (scan != NULL)
  {
    if (strcmp(scan->realname, "!DEL!") &&
      !strcasecmp(scan->channel, channel) &&
      ((nicksearch && match(scan->realname, realname)) ||
	(!nicksearch && compare(realname, scan->match)) ||
	(nicksearch && match(chaninfo, scan->match))) &&
      scan->access >= min && scan->access <= max &&
      (mask == 0 || (scan->flags & mask) == mask) &&
      (xmask == 0 || (scan->flags & xmask) == 0) &&
      (*modifby == '\0' || match(scan->modif, modifby)))
    {
      if (++count > 15 && source[0] != '+')
      {
	notice(source,
	  "There are more than 15 matching entries. "
	  "Please restrict you search");
	return;
      }
      show_this_user_access_reg(source, scan, modif);
    }
    scan = scan->next;
  }

  hook1 = (struct showaccess_struct *)
    malloc(sizeof(struct showaccess_struct));
  strcpy(hook1->source, source);
  strcpy(hook1->target, realname);
  strcpy(hook1->chaninfo, chaninfo);
  strcpy(hook1->modifby, modifby);
  hook1->nicksearch = nicksearch;
  hook1->min = min;
  hook1->max = max;
  hook1->mask = mask;
  hook1->xmask = xmask;
  hook1->modif = modif;

  hook2 = (int *)malloc(sizeof(int));
  *hook2 = count;

  db_fetch(channel, DBGETALLCMP, "", NULL, 0, hook1, hook2,
    showaccess_callback);
}


static void
 suspend_callback(int *fd, off_t off, int action, void *hook1, void *hook2,
  dbuser * dbu, int count)
{
  char buffer[512];
  register aluser *lusr;
  register RegUser *user;
  time_t exp;

  exp = action + now;

  if (count == 0)
  {
    if (*(char *)hook1)
      notice((char *)hook1, "No match!");
  }
  else if (dbu != NULL && (user = load_dbuser(off, dbu)) != NULL)
  {
#ifdef DEBUG
    printf("SUSPEND for %s till %ld\n",
      user->realname, exp);
    puts(ctime(&exp));
#endif

    if (*(char *)hook1 &&
      Access(user->channel, (char *)hook1) <= user->access)
    {
      sprintf(buffer, "User %s's access is higher than or equal to yours",
	user->realname);
      notice((char *)hook1, buffer);
    }
    /* If the suspension is the result of a
       flood protection (in which case 
       source==""), I want to make sure
       the user is not already suspended for
       a longer time! */

    else if (*(char *)hook1 || user->suspend < exp)
    {
      user->suspend = exp;
      user->modified = 1;
      if (*(char *)hook1 && (lusr = ToLuser((char *)hook1)))
      {
	RegUser *reg;
	time_t now1 = now + 3600;
	struct tm *tms = gmtime(&now1);

	reg = IsValid(lusr, user->channel);
	sprintf(buffer, "%04d%02d%02d@%03ld (%s) %s!%s@%s",
	  tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday,
	  1000 * (now1 % 86400) / 86400,
	  reg ? reg->realname : "?",
	  lusr->nick, lusr->username, lusr->site);

	TTLALLOCMEM -= strlen(user->modif) + 1;
	free(user->modif);
	user->modif = (char *)MALLOC(strlen(buffer) + 1);
	strcpy(user->modif, buffer);
      }
    }

    if (*(char *)hook1)
    {
      if (user->suspend > now)
      {
	sprintf(buffer,
	  "SUSPENSION for %s (%s) will expire in %s",
	  user->realname, user->match,
	  time_remaining(user->suspend - now));
	notice((char *)hook1, buffer);
      }
      else
      {
	sprintf(buffer, "SUSPENSION for %s (%s) is cancelled",
	  user->realname, user->match);
	notice((char *)hook1, buffer);
      }
    }
  }
  if (dbu == NULL)
  {
    notice((char *)hook1, "Done.");
    free(hook1);
  }
}



void unsuspend(char *source, char *ch, char *args)
{
  char channel[80];
  char target[100];
  char out[200];

  if (*args == '#' || (*args == '*' && IsValid(ToLuser(source), ch)))
  {
    GetWord(0, args, channel);
    args = ToWord(1, args);
  }
  else
  {
    strcpy(channel, ch);
    GuessChannel(source, channel);
  }

  GetWord(0, args, target);
  if (!strcmp(channel, "*") || !*target)
  {
    notice(source, "SYNTAX: unsuspend [#channel] <NICK|ADDRESS>");
    return;
  }
  sprintf(out, "%s %s 0", channel, target);
  suspend(source, ch, out);
}

void suspend(char *source, char *ch, char *args)
{
  char channel[80], target[80], timestring[80], *hook;
  int timeint;
  int acc;

  if (*args == '#' || (*args == '*' && IsValid(ToLuser(source), ch)))
  {
    GetWord(0, args, channel);
    args = ToWord(1, args);
  }
  else
  {
    strcpy(channel, ch);
    GuessChannel(source, channel);
  }

  if (*source)
    acc = Access(channel, source);
  else
    acc = MASTER_ACCESS + 1;

  if (acc < LEVEL_SUSPEND)
  {
    ReplyNotAccess(source, channel);
    return;
  }

  GetWord(0, args, target);
  GetWord(1, args, timestring);

  if ((!strcmp(channel, "*") && !IsValid(ToLuser(source), channel)) ||
    !*target || !*timestring)
  {
    notice(source, "SYNTAX: suspend [channel] <nick|userhost> <duration> [s|m|d]");
    return;
  }

  timeint = atoi(timestring);

  switch (*ToWord(2, args))
  {
  case 's':
  case 'S':
  case '\0':
    break;
  case 'm':
  case 'M':
    timeint *= 60;
    break;
  case 'h':
  case 'H':
    timeint *= 3600;
    break;
  case 'd':
  case 'D':
  case 'j':
  case 'J':
    timeint *= 86400;
    break;
  default:
    notice(source, "Bogus time units");
    return;
  }

  if (timeint < 0 || timeint > 31536000)
  {
    notice(source, "Invalid time");
    return;
  }

  hook = (char *)malloc(strlen(source) + 1);
  strcpy(hook, source);

  if (strpbrk(target, "!@") != NULL)
  {
    db_fetch(channel, DBGETALLCMP, target, NULL, timeint, hook, NULL,
      suspend_callback);
  }
  else
  {
    db_fetch(channel, DBGETNICK, target, NULL, timeint, hook, NULL,
      suspend_callback);
  }
}


void SaveUserList(char *source, char *channel)
{
  char global[] = "*", buffer[256];

  if (*source && Access(global, source) < SAVE_USERLIST_LEVEL)
  {
    notice(source, "Sorry! This command is not for you");
    return;
  }

  if (DB_Save_Status == -1)
  {
    notice(source, "Userlist sync initiated");
    DB_Save_Status = 0;
    strncpy(DB_Save_Nick, source, NICK_LENGTH - 1);
    DB_Save_Nick[NICK_LENGTH - 1] = '\0';
  }
  else
  {
    sprintf(buffer, "Userlist sync is already in progress (%d%%) [%s]",
      DB_Save_Status / 10, *DB_Save_Nick ? DB_Save_Nick : "auto");
    notice(source, buffer);
  }
  /*do_cold_sync();

     if(*source)
     notice(source,"Userlist saved.");
   */
}


static void
 remuser_callback(int *fd, off_t off, int action, void *hook1, void *hook2,
  dbuser * dbu, int count)
{
  register RegUser *user;
  char buffer[512];

  if (dbu == NULL)
  {
    if (count == 0)
    {
      notice((char *)hook1, "This user is not in my userlist");
      notice((char *)hook1, "Make sure you provide a full n!u@h");
    }
    else
    {
      notice((char *)hook1, "Done.");
    }
    free(hook1);
  }
  else
  {
    user = load_dbuser(off, dbu);
    if (user != NULL && user->access < action)
    {
      sprintf(buffer, "I REMOVE USER %s (%s) from %s",
	user->realname, user->match, user->channel);
      notice((char *)hook1, buffer);
      TTLALLOCMEM -= strlen(user->realname) + 1;
      free(user->realname);
      user->realname = (char *)MALLOC(6);
      strcpy(user->realname, "!DEL!");
      TTLALLOCMEM -= strlen(user->match) + 1;
      free(user->match);
      user->match = (char *)MALLOC(6);
      strcpy(user->match, "!DEL!");
      user->access = 0;
      user->flags = 0;
      /* if new, don't save */
      if (user->offset == (off_t) - 1)
	user->modified = 0;
      else
	user->modified = 1;
    }
  }
}


void RemoveUser(char *source, char *ch, char *arg)
{
  char buffer[500];
  char channel[80];
  char toremove[80];
  register RegUser *user;
  register aluser *src;
  int srcacs;
  int nicksearch;

  src = ToLuser(source);
  if (*arg == '#' || (*arg == '*' && *(arg + 1) == ' ' && IsValid(src, ch)))
  {
    GetWord(0, arg, channel);
    arg = ToWord(1, arg);
  }
  else
  {
    strcpy(channel, ch);
    GuessChannel(source, channel);
  }

  GetWord(0, arg, toremove);
  if ((!strcmp(channel, "*") && !IsValid(src, channel)) || !*toremove)
  {
    notice(source, "SYNTAX: remuser [#channel] <nick|address>");
    return;
  }

  if (*source)
    srcacs = Access(channel, source);
  else
    srcacs = MASTER_ACCESS;

  if (srcacs < REMOVE_USER_LEVEL)
  {
    ReplyNotAccess(source, channel);
    return;
  }

  if (strpbrk(toremove, "!@") != NULL)
    nicksearch = 0;
  else
    nicksearch = 1;

  user = UserList[ul_hash(channel)];

  while (user)
  {
    if (!strcasecmp(channel, user->channel) && (
	(!nicksearch && !strcasecmp(toremove, user->match)) ||
	(nicksearch && !strcasecmp(toremove, user->realname))))
      break;
    user = user->next;
  }

  if (user != NULL)
  {
    if (user->access >= srcacs)
    {
      notice(source, "This user's access is higher "
	"than yours. Won't remove him!");
    }
    else
    {
      sprintf(buffer, "I REMOVE USER %s (%s) from %s",
	user->realname, user->match, user->channel);
      notice(source, buffer);
      TTLALLOCMEM -= strlen(user->realname) + 1;
      free(user->realname);
      user->realname = (char *)MALLOC(6);
      strcpy(user->realname, "!DEL!");
      TTLALLOCMEM -= strlen(user->match) + 1;
      free(user->match);
      user->match = (char *)MALLOC(6);
      strcpy(user->match, "!DEL!");
      user->access = 0;
      user->flags = 0;
      if (user->offset == (off_t) - 1)
	user->modified = 0;
      else
	user->modified = 1;
    }
  }

  if ((user == NULL && nicksearch) || !nicksearch)
  {
    /* User entry is not found in memory. Send a database
     * query for it.
     */
    char *hook;
    hook = (char *)malloc(strlen(source) + 1);
    strcpy(hook, source);
    if (nicksearch)
      db_fetch(channel, DBGETNICK, toremove, NULL, srcacs, hook,
	NULL, remuser_callback);
    else
      db_fetch(channel, DBGETALLUH, toremove, NULL, srcacs, hook,
	NULL, remuser_callback);
  }
  else
  {
    notice(source, "Done.");
  }
}

void purge(char *source, char *ch, char *args)
{
  register ShitUser **shit, *tshit;
  register RegUser *user;
  register int index;
  char buffer[1024];
  char channel[80];
  char global[] = "*";
  char *comment;

  if (Access(global, source) < XADMIN_LEVEL)
  {
    notice(source, "Sorry. This command is reserved to X-admins.");
    return;
  }

  GetWord(0, args, channel);
  comment = ToWord(1, args);
  if (!*channel || *channel != '#' || !*comment)
  {
    notice(source, "SYNTAX: purge <channel> <comment>");
    return;
  }

  sprintf(buffer, "%s is purging channel %s (%s)",
    source, channel, comment);
  broadcast(buffer, 1);

  RemChan("", channel, "");
  part("", channel, "");

  shit = &ShitList[sl_hash(channel)];
  while (*shit != NULL)
  {
    if (!strcasecmp((*shit)->channel, channel))
    {
      tshit = *shit;
      *shit = (*shit)->next;
      TTLALLOCMEM -= strlen(tshit->match) + 1;
      free(tshit->match);
      TTLALLOCMEM -= strlen(tshit->from) + 1;
      free(tshit->from);
      TTLALLOCMEM -= strlen(tshit->reason) + 1;
      free(tshit->reason);
      TTLALLOCMEM -= strlen(tshit->channel) + 1;
      free(tshit->channel);
      TTLALLOCMEM -= sizeof(ShitUser);
      free(tshit);
    }
    else
      shit = &(*shit)->next;
  }

  index = ul_hash(channel);
  user = UserList[index];
  while (user != NULL)
  {
    if (!strcasecmp(channel, user->channel))
    {
      user->access = 0;
      user->flags = 0;
      user->modified = 0;	/* see the unlink() below */
      TTLALLOCMEM -= strlen(user->realname) + 1;
      free(user->realname);
      user->realname = (char *)MALLOC(6);
      strcpy(user->realname, "!DEL!");
      TTLALLOCMEM -= strlen(user->match) + 1;
      free(user->match);
      user->match = (char *)MALLOC(6);
      strcpy(user->match, "!DEL!");
    }
    user = user->next;
  }

  unlink(make_dbfname(channel));

  sprintf(buffer, "Channel %s has been disintegrated.. really.", channel);
  notice(source, buffer);
  sprintf(buffer, "PURGE (mis)used by %s on %s (%s)",
    source, channel, comment);
  SpecLog(buffer);
}


static void
 do_modinfo(char *source, RegUser * user, char *field, char *newvalue)
{
  char buffer[512];
  register aluser *luser;
  register avalchan *vchan;
  register char *ptr, *ptr2;
  register int change = 0;

  luser = ToLuser(source);
  if (luser == NULL)
    return;

  vchan = luser->valchan;
  while (vchan && strcasecmp(user->channel, vchan->name))
    vchan = vchan->next;

  if (vchan == NULL)
  {
    /* Something happened since the request was sent.
     * In any case, it should be ignored now.
     */
    return;
  }

  if (!strcasecmp(field, "NICK"))
  {
    notice(source, "The NICK option was disabled.");
  }
  else if (!strcasecmp(field, "MATCH") || !strcasecmp(field, "MASK"))
  {
    if (user->access < vchan->reg->access)
    {
      ptr = strchr(newvalue, '@');
      ptr2 = strchr(newvalue, '!');
      if (!ptr || !ptr2 || ptr2 > ptr || strchr(ptr2 + 1, '!') ||
	strchr(ptr + 1, '@') || !regex_cmp(VALIDMASK, newvalue))
      {
	notice(source, "Invalid nick!user@host mask");
      }
      else
      {
	for (ptr = newvalue; *ptr; ptr++)
	{
	  if (*ptr < 33)
	    break;
	}
	if (*ptr)
	{
	  notice(source, "Invalid nick!user@host mask");
	}
	else
	{
	  TTLALLOCMEM -= strlen(user->match) + 1;
	  free(user->match);
	  user->match = (char *)MALLOC(strlen(newvalue) + 1);
	  strcpy(user->match, newvalue);
	  sprintf(buffer, "New MATCH for %s is %s",
	    user->realname, user->match);
	  notice(source, buffer);
	  change = 1;
	}
      }
    }
  }
  else if (!strcasecmp(field, "ACCESS"))
  {
    if (user->access < vchan->reg->access)
    {
      if (strlen(newvalue) > 4 ||
	atoi(newvalue) >= vchan->reg->access ||
	atoi(newvalue) < 0)
      {
	notice(source, "Bogus access");
      }
      else
      {
	user->access = atoi(newvalue);
	sprintf(buffer, "New ACCESS for %s is %d",
	  user->realname, user->access);
	notice(source, buffer);
	change = 1;
      }
    }
  }
  else if (!strcasecmp(field, "AUTOOP"))
  {
    if (user->access < vchan->reg->access ||
      user == vchan->reg)
    {
      if (!strcasecmp(newvalue, "ON") ||
	!strcasecmp(newvalue, "YES"))
      {
	user->flags |= UFL_AUTOOP;
      }
      else if (!strcasecmp(newvalue, "OFF") ||
	!strcasecmp(newvalue, "NO"))
      {
	user->flags &= ~UFL_AUTOOP;
      }
      else
      {
	notice(source, "You must specify 'yes' or 'no'");
      }
      sprintf(buffer, "AUTOOP for %s is %s",
	user->realname, (user->flags & UFL_AUTOOP) ? "ON" : "OFF");
      notice(source, buffer);
      change = 1;
    }
  }
  else if (!strcasecmp(field, "PASSWORD"))
  {
    if (user->access < vchan->reg->access ||
      user == vchan->reg)
    {
      if (strlen(newvalue) < 6 || strlen(newvalue) > 18)
      {
	notice(source, "Password must be between 6 and 18 characters long");
      }
      else
      {
	TTLALLOCMEM -= strlen(user->passwd) + 1;
	free(user->passwd);
	user->passwd = (char *)MALLOC(strlen(newvalue) + 1);
	strcpy(user->passwd, newvalue);
	sprintf(buffer, "New PASSWORD set for %s",
	  user->realname);
	notice(source, buffer);
	change = 1;
      }
    }
  }
  else if (!strcasecmp(field, "REMPASS"))
  {
    notice(source, "REMPASS is no longer available. Try PASSWORD.");
  }
  if (change)
  {
    RegUser *reg;
    time_t now1 = now + 3600;
    struct tm *tms = gmtime(&now1);

    if (strcasecmp(field, "AUTOOP"))
    {
      reg = IsValid(luser, user->channel);
      sprintf(buffer, "%04d%02d%02d@%03ld (%s) %s!%s@%s",
	tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday,
	1000 * (now1 % 86400) / 86400,
	reg ? reg->realname : "?",
	luser->nick, luser->username, luser->site);
      TTLALLOCMEM -= strlen(user->modif) + 1;
      free(user->modif);
      user->modif = (char *)MALLOC(strlen(buffer) + 1);
      strcpy(user->modif, buffer);
    }
    user->modified = 1;
    user->lastused = now;
  }
  else
  {
    notice(source, "Entry was not modified");
  }
}


static void
 modinfo_callback(int *fd, off_t off, int action, void *hook1, void *hook2,
  dbuser * dbu, int count)
{
  register RegUser *user;

  struct modinfo_struct *ptr = (struct modinfo_struct *)hook1;

  if (count == 0)
  {
    notice(ptr->source, "No match.");
  }
  else if (dbu != NULL && (user = load_dbuser(off, dbu)) != NULL)
  {
    do_modinfo(ptr->source, user, ptr->field, ptr->newvalue);
  }
  if (dbu == NULL)
  {
    free(ptr);
  }
}


void ModUserInfo(char *source, char *msgtarget, char *ch, char *args)
{
  char channel[80];
  char field[80];
  char target[80];
  char newvalue[80];
  register RegUser *user;
  register aluser *luser;
  int nicksearch, srcacs, index;

  luser = ToLuser(source);
  if (*args == '#' || (*args == '*' && *(args + 1) == ' ' && IsValid(luser, ch)))
  {
    GetWord(0, args, channel);
    args = ToWord(1, args);
  }
  else
  {
    strcpy(channel, ch);
    GuessChannel(source, channel);
  }

  GetWord(0, args, field);
  GetWord(1, args, target);
  GetWord(2, args, newvalue);

#ifdef DEBUG
  printf("ModUserInfo()\nSOURCE: \"%s\"\nFIELD: \"%s\"\nTARGET: \"%s\"\nNEWVALUE: \"%s\"\n",
    source, field, target, newvalue);
#endif

  if ((!strcmp(channel, "*") && !IsValid(luser, channel)) ||
    !*field || !*target || !*newvalue)
  {
    notice(source, "SYNTAX: modinfo [#channel] <MATCH,ACCESS,AUTOOP,PASSWORD> <NICK,ADDRESS> <NEW VALUE>");
    return;
  }

  if (!strcasecmp(field, "PASSWORD") && !strchr(msgtarget, '@'))
  {
    char buffer[200];
    sprintf(buffer, "Please use /msg %s@%s", mynick, SERVERNAME);
    notice(source, buffer);
    return;
  }


  srcacs = LAccess(channel, luser);
  if (srcacs < MOD_USERINFO_LEVEL)
  {
    ReplyNotAccess(source, channel);
    return;
  }

  if (strpbrk(target, "!@") != NULL)
    nicksearch = 0;
  else
    nicksearch = 1;

  index = ul_hash(channel);
  user = UserList[index];

  while (user)
  {
    if (!strcasecmp(channel, user->channel) && (
	(!nicksearch && match(target, user->match)) ||
	(nicksearch && !strcasecmp(target, user->realname))))
      break;
    user = user->next;
  }

  if (user != NULL)
  {
    do_modinfo(source, user, field, newvalue);
  }
  else
  {
    struct modinfo_struct *hook;
    hook = (struct modinfo_struct *)
      malloc(sizeof(struct modinfo_struct));
    strcpy(hook->source, source);
    strcpy(hook->field, field);
    strcpy(hook->newvalue, newvalue);

    if (nicksearch)
      db_fetch(channel, DBGETNICK, target, NULL, 0, hook,
	NULL, modinfo_callback);
    else
      db_fetch(channel, DBGET1STUH, target, NULL, 0, hook,
	NULL, modinfo_callback);
  }
}

void ChPass(char *source, char *ch, char *args)
{
  char newpassword[80];
  char channel[80];
  char buffer[200];
  char userhost[200];
  register RegUser *user;
  register aluser *luser;
  RegUser *reg;
  time_t now1 = now + 3600;
  struct tm *tms = gmtime(&now1);

  if (*ch == '#')
  {
    notice(source, "Please DO NOT use the newpass command from a channel!");
    return;
  }

  if (!strchr(ch, '@'))
  {
    sprintf(buffer, "Please use /msg %s@%s newpass [channel] <new_password>",
      mynick, SERVERNAME);
    notice(source, buffer);
    return;
  }

  if (*args == '#' || *args == '*')
  {
    GetWord(0, args, channel);
    args = ToWord(1, args);
  }
  else
  {
    strcpy(channel, ch);
    GuessChannel(source, channel);
  }

  GetWord(0, args, newpassword);

  luser = ToLuser(source);
  user = IsValid(luser, channel);

  if ((!strcmp(channel, "*") && !user) ||
    !*newpassword || *ToWord(1,args) != '\0')
  {
    notice(source, "SYNTAX: newpass [channel] <new password>");
    return;
  }

  if(user && user->suspend > now)
  {
    notice(source, "You are suspended. Request is ignored.");
    return;
  }

  if (strlen(newpassword) < 6)
  {
    notice(source, "A password MUST be at least 6 characters long");
    return;
  }

  if (strlen(newpassword) > 19)
  {
    notice(source, "Password is too long (max 19 characters)");
    return;
  }

  sprintf(userhost, "%s!%s@%s", luser->nick, luser->username, luser->site);

  if (user == NULL || user->access <= 0)
  {
    notice(source, "You are not authenticated on that channel");
  }
  else
  {
    TTLALLOCMEM -= strlen(user->passwd) + 1;
    free(user->passwd);
    user->passwd = (char *)MALLOC(strlen(newpassword) + 1);
    strcpy(user->passwd, newpassword);
    TTLALLOCMEM -= strlen(user->modif) + 1;
    free(user->modif);

    reg = IsValid(luser, user->channel);
    sprintf(buffer, "%04d%02d%02d@%03ld (%s) %s!%s@%s",
      tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday,
      1000 * (now1 % 86400) / 86400,
      reg ? reg->realname : "?",
      luser->nick, luser->username, luser->site);

    user->modif = (char *)MALLOC(strlen(buffer) + 1);
    strcpy(user->modif, buffer);
    notice(source, "Password changed!");
    sprintf(buffer, "NEWPASS %s as %s on %s",
      userhost, user->realname, user->channel);
    log(buffer);
  }
  return;
}

static void add_sync_channel(char *name)
{
  register syncchan **scan = &SyncChan;

  while (*scan != NULL && strcasecmp((*scan)->name, name))
    scan = &(*scan)->next;

  if (*scan == NULL)
  {
    *scan = (syncchan *) malloc(sizeof(syncchan));
    strcpy((*scan)->name, name);
    (*scan)->next = NULL;
  }
}

void gather_sync_channels(void)
{
  register RegUser *reg;
  register int i;

  for (i = 0; i < 1000; i++)
  {
    reg = UserList[i];
    while (reg != NULL)
    {
      if (reg->modified ||
	(!reg->inuse && reg->lastused + CACHE_TIMEOUT < now))
	add_sync_channel(reg->channel);
      reg = reg->next;
    }
  }
}

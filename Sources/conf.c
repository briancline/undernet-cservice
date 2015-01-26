/* @(#)$Id: conf.c,v 1.5 1998/01/22 09:36:11 chaos Exp $ */

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

void read_conf(char *conf)
{
  FILE *fp;
  char buffer[1024], *ptr;

  fp = fopen(conf, "r");
  if (fp == NULL)
  {
    perror(conf);
    exit(1);
  }

  while (fgets(buffer, 1024, fp) != NULL)
  {
    if (*buffer == '#' || *buffer == '\0')
      continue;
    if ((ptr = strchr(buffer, '\n')) != NULL)
      *ptr = '\0';

    if (!strncasecmp(buffer, "NICKNAME ", 9))
    {
      strncpy(mynick, buffer + 9, NICK_LENGTH);
      mynick[NICK_LENGTH - 1] = '\0';
    }
    else if (!strncasecmp(buffer, "USERNAME ", 9))
    {
      strncpy(myuser, buffer + 9, USERNAME_LENGTH);
      myuser[USERNAME_LENGTH - 1] = '\0';
    }
    else if (!strncasecmp(buffer, "HOSTNAME ", 9))
    {
      strncpy(mysite, buffer + 9, SITE_LENGTH);
      mysite[SITE_LENGTH - 1] = '\0';
    }
    else if (!strncasecmp(buffer, "REALNAME ", 9))
    {
      strncpy(myrealname, buffer + 9, REALNAME_LENGTH);
      myrealname[REALNAME_LENGTH - 1] = '\0';
    }
    else if (!strncasecmp(buffer, "UPLINK ", 7))
    {
      strncpy(server, buffer + 7, SERVER_NAME_LENGTH);
      server[SERVER_NAME_LENGTH - 1] = '\0';
    }
    else if (!strncasecmp(buffer, "PORT ", 5))
    {
      DEFAULT_PORTNUM = atoi(buffer + 5);
    }
    else if (!strncasecmp(buffer, "UMODE ", 6))
    {
      strncpy(UMODE, buffer + 6, 10);
      UMODE[9] = '\0';
    }
    else if (!strncasecmp(buffer, "SERVERNAME ", 11))
    {
      strncpy(SERVERNAME, buffer + 11, 80);
      SERVERNAME[79] = '\0';
    }
    else if (!strncasecmp(buffer, "SERVERINFO ", 11))
    {
      strncpy(SERVERINFO, buffer + 11, 80);
      SERVERINFO[79] = '\0';
    }
    else if (!strncasecmp(buffer, "PASSWORD ", 9))
    {
      strncpy(PASSWORD, buffer + 9, 80);
      PASSWORD[79] = '\0';
    }
    else if (!strncasecmp(buffer, "HOMEDIR ", 8))
    {
      strncpy(HOMEDIR, buffer + 8, 256);
      HOMEDIR[255] = '\0';
    }
    else if (!strncasecmp(buffer, "UMASK ", 6))
    {
      sscanf(buffer + 6, "%d", &UMASK);
    }
    else if (!strncasecmp(buffer, "EXEC ", 5))
    {
      strncpy(EXEC_FILE, buffer + 5, 256);
      EXEC_FILE[255] = '\0';
    }
    else if (!strncasecmp(buffer, "MOTD ", 5))
    {
      strncpy(MOTD_FILE, buffer + 5, 256);
      MOTD_FILE[255] = '\0';
    }
    else if (!strncasecmp(buffer, "LOG ", 4))
    {
      strncpy(LOGFILE, buffer + 4, 256);
      LOGFILE[255] = '\0';
    }
    else if (!strncasecmp(buffer, "LOGBAK ", 7))
    {
      strncpy(LOGFILEBAK, buffer + 7, 256);
      LOGFILEBAK[255] = '\0';
    }
    else if (!strncasecmp(buffer, "PID ", 4))
    {
      strncpy(PIDFILE, buffer + 4, 256);
      PIDFILE[255] = '\0';
    }
    else if (!strncasecmp(buffer, "HELPDIR ", 8))
    {
      strncpy(HELP_DIR, buffer + 8, 256);
      HELP_DIR[255] = '\0';
    }
    else if (!strncasecmp(buffer, "MASTER_NICK ", 12))
    {
      strncpy(MASTER_REALNAME, buffer + 12, 80);
      MASTER_REALNAME[79] = '\0';
    }
    else if (!strncasecmp(buffer, "MASTER_MASK ", 12))
    {
      strncpy(MASTER_MATCH, buffer + 12, 80);
      MASTER_MATCH[79] = '\0';
    }
    else if (!strncasecmp(buffer, "MASTER_PASSWD ", 14))
    {
      strncpy(MASTER_PASSWD, buffer + 14, 20);
      MASTER_PASSWD[19] = '\0';
    }
    else if (!strncasecmp(buffer, "BROADCAST ", 10))
    {
      strncpy(BROADCAST_CHANNEL, buffer + 10, 80);
      BROADCAST_CHANNEL[79] = '\0';
    }
    else if (!strncasecmp(buffer, "VERIFYID ", 9))
    {
      strncpy(VERIFY_ID, buffer + 9, 256);
      VERIFY_ID[255] = '\0';
    }
    else if (!strncasecmp(buffer, "UWORLD_NICK ", 12))
    {
      strncpy(UWORLD, buffer + 12, 10);
      UWORLD[9] = '\0';
    }
    else if (!strncasecmp(buffer, "UWORLD_HOST ", 12))
    {
      strncpy(UWORLD_HOST, buffer + 12, 80);
      UWORLD_HOST[79] = '\0';
    }
    else if (!strncasecmp(buffer, "UWORLD_SERVER ", 14))
    {
      strncpy(UWORLD_SERVER, buffer + 14, 80);
      UWORLD_SERVER[79] = '\0';
    }
    else if (!strncasecmp(buffer, "UWORLD2_NICK ", 13))
    {
      strncpy(UWORLD2_NICK, buffer + 13, 10);
      UWORLD2_NICK[9] = '\0';
    }
    else if (!strncasecmp(buffer, "UWORLD2_HOST ", 13))
    {
      strncpy(UWORLD2_HOST, buffer + 13, 80);
      UWORLD2_HOST[79] = '\0';
    }
    else if (!strncasecmp(buffer, "UWORLD2_SERVER ", 15))
    {
      strncpy(UWORLD2_SERVER, buffer + 15, 80);
      UWORLD2_SERVER[79] = '\0';
    }
    else if (!strncasecmp(buffer, "CALMDOWNTOPIC ", 14))
    {
      strncpy(CALMDOWNTOPIC, buffer + 14, 512);
      CALMDOWNTOPIC[511] = '\0';
    }
    else if (!strncasecmp(buffer, "NSERV_NICK ", 11))
    {
#ifdef NICKSERV
      strncpy(NSERV_NICK, buffer + 11, 10);
      NSERV_NICK[9] = '\0';
#endif
    }
    else if (!strncasecmp(buffer, "NSERV_USER ", 11))
    {
#ifdef NICKSERV
      strncpy(NSERV_USER, buffer + 11, 10);
      NSERV_USER[9] = '\0';
#endif
    }
    else if (!strncasecmp(buffer, "NSERV_HOST ", 11))
    {
#ifdef NICKSERV
      strncpy(NSERV_HOST, buffer + 11, 80);
      NSERV_HOST[79] = '\0';
#endif
    }
    else if (!strncasecmp(buffer, "NSERV_INFO ", 11))
    {
#ifdef NICKSERV
      strncpy(NSERV_INFO, buffer + 11, 200);
      NSERV_INFO[199] = '\0';
#endif
    }
    else if (!strncasecmp(buffer, "NSERV_STAT ", 11))
    {
#ifdef NICKSERV
      if (!strncmp(buffer + 11, "on", 2))
	NServ_status = 1;
      else if (!strncmp(buffer + 11, "off", 3))
	NServ_status = 0;
      else
	fprintf(stderr, "NSERV_STAT: Unknown value. Must be ON or OFF\n");
#endif
    }
    else
    {
      fprintf(stderr, "%s: Unknown keyword \"%s\"\n", conf, buffer);
    }
  }
  fclose(fp);
}

/* @(#)$Id: defines.h,v 1.11 2000/06/04 16:54:16 seks Exp $ */

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

#define NICK_LENGTH 20
#define USERNAME_LENGTH 20
#define SITE_LENGTH 80
#define REALNAME_LENGTH 80
#define CHANNELNAME_LENGTH 80
#define SERVER_NAME_LENGTH 80
#define PASSWD_LENGTH 20

#define COMMAND_PREFIX "X "
#define MAX_MODE_PER_LINE 6

#define MAX_DEOP_RATE 3			/* deops within 15 seconds */
#define MASSDEOP_SUSPEND_TIME 600	/* seconds */
#define MASSDEOP_SHITLIST_TIME 1 	/* hours */
#define MASSDEOP_SHITLIST_LEVEL 20

#define MAX_NICKCHANGE_RATE 5		/* nick change within 15 seconds */
#define NICK_FLOOD_SUSPEND_TIME 600	/* seconds */

#define MAX_PUBLIC_MSG_RATE 7		/* within 15 seconds */
#define PUBLIC_FLOOD_SUSPEND_TIME 600	/* seconds */
#define PUBLIC_FLOOD_SHITLIST_TIME 24 	/* hour */
#define PUBLIC_FLOOD_SHITLIST_LEVEL 75

#define PRIVATE_FLOOD_RATE 10		/* messages per 30 seconds */
#define PRIVATE_FLOOD_SIZE 800		/* bytes per 30 seconds */
#define FLOOD_FLOOD_RATE 10             /* messages per 30 seconds */
#define FLOOD_FLOOD_SIZE 60		/* lines per 30 seconds */
#define MAX_IGNORE_PER_SITE 3		/* ignores per site */
#define IGNORE_TIME 3600		/* in seconds */
#define FLOOD_FLOOD_IGNORE 300		/* in seconds */

#define LEVEL_DIE 900
#define LEVEL_CORE 900
#define RUSAGE_ACCESS 900
#define LEVEL_UPGRADE 900
#define LEVEL_JOIN 450
#define LEVEL_PART 450
#define INVITE_LEVEL 100
#define OP_LEVEL 100
#define TOPIC_LEVEL 50
#define KICK_LEVEL 50
#define MASS_KICK_LEVEL 200
#define BAN_LEVEL 75
#define MASS_BAN_LEVEL 200
#define ADD_USER_LEVEL 400
#define SHOW_ACCESS_LEVEL 0
#define REMOVE_USER_LEVEL 400
#define MOD_USERINFO_LEVEL 400
#define LEVEL_SUSPEND 100
#define ADD_TO_SHITLIST_LEVEL 75
#define CLEAN_SHITLIST_LEVEL 200
#define SET_DEFAULT_LEVEL 450
#define LOAD_DEFAULT_LEVEL 999
#define SAVE_DEFAULTS_LEVEL 600
#define SAVE_SHITLIST_LEVEL 600
#define LOAD_SHITLIST_LEVEL 999
#define SAVE_USERLIST_LEVEL 600
#define LOAD_USERLIST_LEVEL 999
#define STATUS_ACCESS 1
#define STATUS_ACCESS_MODE 200
#define CH_FLOOD_LIMIT_LEVEL 500
#define CH_NICK_FLOOD_LIMIT_LEVEL 450
#define CH_MASSDEOP_LIMIT_LEVEL 450
#define CH_NOOP_LEVEL 500
#define CH_OPONLY_LEVEL 500
#define CH_AUTOTOPIC_LEVEL 450
#define CH_ALWAYSOP_LEVEL 450
#define CH_STRICTOP_LEVEL 500
#define CH_USERFLAGS_LEVEL 450
#define CH_LANG_LEVEL 500
#define CH_TOPIC_LEVEL 450
#define CH_URL_LEVEL 450
#define ACCESS_BAN_PRIORITY 450
#define ALWAYSOP_OVERRIDE_LEVEL 450
#define PROTECT_OVERRIDE_LEVEL 450
#define MASSDEOP_IMMUNE_LEVEL 450
#define CLEARMODE_LEVEL 400
#define XADMIN_LEVEL 750

#define AUTO_KICK_SHIT_LEVEL 75
#define NO_OP_SHIT_LEVEL 20
#define SHITLIST_DEFAULT_TIME (24*3) /* hours */
#define SUSPEND_TIME_FOR_OPPING_A_SHITLISTED_USER 600 	/* seconds */
#define SUSPEND_TIME_FOR_BANNING_A_PROTECTED_USER 600	/* seconds */
#define DEOPME_SUSPEND_TIME 3600	/* seconds */
#define DEOP_SHITLIST_TIME 1 	/* hours */
#define DEOP_SHITLIST_LEVEL 20
#define MAX_BAN 50

#define MIN_LASTSEEN (3*24*3600)	/* 3 days */

#define HTTP_LISTEN	1
#define HTTP_ACTIVE	2
#define HTTP_ENDING	3
#define HTTP_RECV_POST	4
#define HTTP_PIPE	5
#define HTTP_CSRAW	6
#define HTTP_CHAT	7
#define HTTP_ERROR	0

#define MISC_GETPATCH   1
#define MISC_PIPE_PATCH	2
#define MISC_PIPE_MAKE	3

#define MISC_ERROR      -1
#define MISC_CONNECTING 1
#define MISC_HANDSHAKE  2
#define MISC_RECV       3

#define BUFFER_BLOCK_SIZE 512

#define CACHE_TIMEOUT	4800    /* 1.5 hours */

#define AUTOTOPIC_FREQ  1800   /* 30 minutes*/

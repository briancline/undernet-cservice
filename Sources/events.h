/* @(#)$Id: events.h,v 1.4 1997/07/01 21:51:08 cvs Exp $ */

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

#define EVENT_SAVEUSERLIST 	1
#define EVENT_SAVESHITLIST 	2
#define EVENT_SAVEDEFCHANNELS 	3
#define EVENT_SYNC		4
#define EVENT_CLEANSHITLIST	5
#define EVENT_FLUSHMODEBUFF	6
#define EVENT_GETOPS		7
#define EVENT_CLEAN_IGNORES	8
#define EVENT_CHECK_IDLE	9
#define EVENT_RENAME_LOGFILE	10
#define EVENT_LOG_CHANNEL	11
#define EVENT_SYNCDB		12
#define EVENT_CHECKREGNICK	13
#define EVENT_SAVENICKSERV	14

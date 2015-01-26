/* @(#)$Id: nickserv.h,v 1.2 1997/07/18 07:55:05 cvs Exp $ */

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

#define NSERV_DELAY 60
#define NSERV_SAVE_FREQ 3600
#define NSERV_GLINETIME 60

struct aregmask {
  char mask[80];
  time_t lastused;
  struct aregmask *next;
};

struct aregnick {
  char nick[10];
  char password[20];
  char email[80];
  int ref;
  time_t created;
  time_t modified;
  struct aregmask *mask;
  struct aregnick *next;
};


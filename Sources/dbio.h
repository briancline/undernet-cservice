/* @(#)$Id: dbio.h,v 1.3 1996/11/13 00:40:36 seks Exp $ */

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

#define DBGETNICK	0x00000001
#define DBGET1STUH	0x00000002
#define DBGETALLUH	0x00000004
#define DBCOUNTUH	0x00000008
#define DBGET1STCMP	0x00000010
#define DBGETALLCMP	0x00000020
#define DBCNTCMP	0x00000040
#define DBGETUHPASS	0x00000080
#define DBGET1STFREE	0x00000100

#define SYNC_UPDATE	0x00000001
#define SYNC_DELETE	0x00000002
#define SYNC_ADD	0x00000004

#define SYNC_PENDWRITE	0x00000001
#define SYNC_SEEKFREE	0x00000002


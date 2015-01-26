/* @(#)$Id: replies.h,v 1.4 1996/11/13 00:40:47 seks Exp $ */

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

#define RPL_NOTONCHANNEL	0
#define RPL_NOTCHANOP		1
#define RPL_OPONLY		2
#define RPL_OPSELFONLY		3
#define RPL_NOSUCHNICK		4
#define RPL_ALREADYONCHANNEL	5
#define RPL_IINVITED		6
#define RPL_YOUAREINVITED	7
#define RPL_ALWAYSOPWASACTIVE	8
#define RPL_ALWAYSOP		9
#define RPL_KICK1ST		10
#define RPL_KICK2ND		11
#define RPL_CHANNOTEXIST	12
#define RPL_BADFLOODLIMIT	13
#define RPL_SETFLOODLIMIT	14
#define RPL_BADNICKFLOODLIMIT	15
#define RPL_SETNICKFLOODLIMIT	16
#define RPL_BADMASSDEOPLIMIT	17
#define RPL_SETMASSDEOPLIMIT	18
#define RPL_NOOPON		19
#define RPL_NOOPOFF		20
#define RPL_BADNOOP		21
#define RPL_ALWAYSOPON		22
#define RPL_ALWAYSOPOFF		23
#define RPL_BADALWAYSOP		24
#define RPL_OPONLYON		25
#define RPL_OPONLYOFF		26
#define RPL_BADOPONLY		27
#define RPL_AUTOTOPICON		28
#define RPL_AUTOTOPICOFF	29
#define RPL_BADAUTOTOPIC	30
#define RPL_STRICTOPON		31
#define RPL_STRICTOPOFF		32
#define RPL_BADSTRICTOP		33
#define RPL_BADUSERFLAGS	34
#define RPL_SETUSERFLAGS	35
#define RPL_KNOWNLANG		36
#define RPL_SETLANG		37
#define RPL_STATUS1		38
#define RPL_STATUS2		39
#define RPL_STATUS3		40
#define RPL_STATUS4		41
#define RPL_STATUS5		42
#define RPL_SETCHANDEFS		43
#define RPL_NOTDEF		44
#define RPL_REMDEF		45
#define RPL_NOMATCH		46
#define RPL_NOOP		47
#define RPL_CANTBEOP		48
#define RPL_CANTBEOPPED		49
#define RPL_DEOPPED1ST		50
#define RPL_DEOPPED2ND		51
#define RPL_DEOPPED3RD		52
#define RPL_USERISPROTECTED	53
#define RPL_YOUREOPPEDBY	54
#define RPL_USERNOTONCHANNEL	55
#define RPL_YOUREDEOPPEDBY	56

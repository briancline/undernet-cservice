# Undernet Channel Service (X)
# Copyright (C) 1995-2002 Robin Thellend
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# The author can be contact by email at <csfeedback@robin.pfft.net>
#
# Please note that this software is unsupported and mostly
# obsolete. It was replaced by GNUworld/CMaster. See
# http://gnuworld.sourceforge.net/ for more information.
#

CC = gcc
RM = /bin/rm

CFLAGS = -Wall -g -O0
#CFLAGS = -O6

#DEFINES = -DDEBUG
#DEFINES = -DBACKUP
#DEFINES = -DDEBUG_MALLOC -DHISTORY
#DEFINES= -DHISTORY
DEFINES=

# necessary for Solaris
#LIBS = -lsocket -lnsl

OBJECTS = bans.o buffer.o channels.o chat.o conf.o connect.o dbio.o debug.o \
	  defchan.o events.o floodpro.o help.o http.o ignore.o kicks.o \
	  match.o modes.o nick.o nickserv.o opcom.o ops.o patch.o privmsg.o \
	  replies.o servers.o shitlist.o socketio.o special.o userlist.o \
	  users.o version.o

SOURCES = bans.c buffer.c channels.c chat.c conf.c connect.c dbio.c debug.c \
	  defchan.c events.c floodpro.c help.c http.c ignore.c kicks.c \
	  match.c modes.c nick.c nickserv.c opcom.c ops.c patch.c privmsg.c \
	  replies.c servers.c shitlist.c socketio.c special.c userlist.c \
	  users.c version.c

MAKE = make -f Sources.mak 'CFLAGS=${CFLAGS}' 'CC=${CC}' 'DEFINES=${DEFINES}'\
            'LIBS=${LIBS}' 'SOURCES=${SOURCES}' 'OBJECTS=${OBJECTS}'\
            'RM=${RM}'

all:    cs fixdb showdb show_old_managers
	@echo 'All Done! :)'

cs:	dummy
	@echo "Making cs..."
	@if [ -f cs ] ; then \
	  mv -f cs cs.old; \
	  sleep 1; \
	 fi
	@cd Sources; ${MAKE}; cd ..;

list:   dummy
	@echo "Making list..."
	@cd Sources; ${MAKE} list; cd ..;

listall: dummy
	@echo "Making listall"
	@cd Sources; ${MAKE} listall; cd ..;

fixdb: dummy
	@echo "Making fixdb"
	@cd Sources; ${MAKE} fixdb; cd ..;

showdb: dummy
	@echo "Making showdb"
	@cd Sources; ${MAKE} showdb; cd ..;

show_old_managers: dummy
	@echo "Making show_old_managers"
	@cd Sources; ${MAKE} show_old_managers; cd ..;

clean:  dummy
	@cd Sources; ${MAKE} clean; cd ..;
	$(RM) -f core cs list listall cs.log *.bak

depend: dummy
	@cd Sources; ${MAKE} depend; cd ..;

love:
	-@echo "With you?? dream on! :P"

dummy:

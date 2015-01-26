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


all: ../cs

.c.o:
	${CC} ${CFLAGS} ${DEFINES} -c $<

../cs: ver ${OBJECTS} cksum.o cksum
	${CC} ${CFLAGS} ${DEFINES} ${LIBS} -o ../cs main.c cksum.o ${OBJECTS} -DBINCKSUM1=0 -DBINCKSUM2=0
	${CC} ${CFLAGS} ${DEFINES} ${LIBS} -o ../cs main.c cksum.o ${OBJECTS} `./cksum ../cs`

ver:
	@chmod +x version.SH
	@./version.SH

cksum: cksum.c
	${CC} ${CFLAGS} -o cksum cksum.c -DMAIN

list: ../list

../list: list.c
	${CC} -o ../list list.c

listall: ../listall

../listall: listall.c
	${CC} -o ../listall listall.c

fixdb: match.o
	${CC} fixdb.c match.o -DMAIN -o ../fixdb

showdb: match.o
	${CC} -Wall showdb.c match.o -DMAIN -o ../showdb

show_old_managers: match.o
	${CC} -Wall -o ../show_old_managers show_old_managers.c match.o -DMAIN

clean:
	$(RM) -f *.o *.bak

depend:
	-gcc -MM ${CFLAGS} ${SOURCES} > make.dep

backup: ${SOURCES} list.c listall.c
	-cd ..; backups/backup

love:
	-@echo "With you?? dream on! :P"

include make.dep

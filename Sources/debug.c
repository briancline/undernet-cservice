/* @(#)$Id: debug.c,v 1.3 1996/11/13 00:40:36 seks Exp $ */

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

#ifdef DEBUG_MALLOC

#include <stdarg.h>
#undef malloc
#undef free

static FILE *debug_malloc_file;

void open_debug_malloc(void)
{
  debug_malloc_file=fopen("malloc.log","w");
}

void close_debug_malloc(void)
{
  fclose(debug_malloc_file);
}

void log_malloc(char *fmt,...)
{
  va_list ap;
  va_start(ap,fmt);
  vfprintf(debug_malloc_file,fmt,ap);
  va_end(ap);
}

void *debug_malloc(char *file, int line, size_t size)
{
  register void *ptr;
  ptr=malloc(size);
  log_malloc("%s(%d):malloc(%ld)= %p\n",file,line,(long)size,ptr);
  return ptr;
}

void debug_free(char *file, int line, void *ptr)
{
  log_malloc("%s(%d):free(%p)\n",file,line,ptr);
  free(ptr);
}

#endif


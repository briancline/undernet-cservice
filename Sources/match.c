/* @(#)$Id: match.c,v 1.10 1999/03/06 19:15:34 seks Exp $ */

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
#include <regex.h>

static int matchX(char *, char *);

int mycasecmp(char *s1, char *s2)
{
  register unsigned char *c1 = (unsigned char *)s1, *c2 = (unsigned char *)s2;
  register int d;

  while (!(d = (toupper(*c1) - toupper(*c2))) && *c1)
  {
    c1++;
    c2++;
  }
  return d;
}


char *strcasestr(register char *s1, char *s2)
{
  register char *a, *b;

  while (*s1)
  {
    a = s1;
    b = s2;
    while (*b && *a && toupper(*a) == toupper(*b))
    {
      a++;
      b++;
    }
    if (!*b)
      return s1;
    s1++;
  }
  return NULL;
}


/* xmatch() returns 1 if both pat1 and pat2 can *possibly*
 * match the same string. It returns 0 otherwise.
 */
int xmatch(char *pat1, char *pat2)
{
  if (!strcmp(pat1, "*") || !strcmp(pat2, "*"))
    return 1;
  while (*pat1 && *pat2)
  {
    if (*pat1 == '*' && *pat2 == '*')
      return (matchX(pat1 + 1, pat2 + 1) || matchX(pat2 + 1, pat1 + 1));
    if (*pat1 == '*')
      return matchX(pat1 + 1, pat2);
    if (*pat2 == '*')
      return matchX(pat2 + 1, pat1);
    if (*pat1 == '?' || *pat2 == '?' || toupper(*pat1) == toupper(*pat2))
    {
      pat1++;
      pat2++;
      if ((!*pat1 && !strcmp(pat2, "*")) ||
	(!*pat2 && !strcmp(pat1, "*")))
	return 1;
    }
    else
      return 0;
  }
  if (*pat1 || *pat2)
    return 0;
  return 1;
}

static int matchX(char *pat1, char *pat2)
{
  do
  {
    if (xmatch(pat1, pat2))
      return 1;
  }
  while (*pat2++);
  return 0;
}


int match(register char *string, register char *mask)
{    
  char *stack[200][2] = {{NULL, NULL}}; 
  register char type = 0;
  register short quote, level = 0;

  while (*string)
  {  
    quote = 0;
    switch (*mask) 
    {
    case '*':
      type = '*';
      if (++level == 200)
        return 0;
      stack[level][0] = mask; 
      stack[level][1] = string; 
      while (*++mask == '*');   /* '*' eats any following '*' */
      break;  
    case '\\':
      quote++;
      mask++; 
      /* no break! */
    default:
      if (toupper(*string) == toupper(*mask) || (*mask == '?' && !quote))
      {       
        mask++; 
      }       
      else    
      {       
        while (stack[level][0] == NULL && --level > 0);
        if (level <= 0)
          return 0;
        mask = stack[level][0];
        string = stack[level][1];
        level--;
      }       
      string++;
    }
  }  
  while (*mask == '*')  /* Skip everything that can   */
    mask++;             /* match an empty string.     */

  return (*mask) ? 0 : 1;
}


/* compare() will return 1 if p1 and p2 can match the same
 * nick!user@host. p1 and p2 are nick!user@host masks.
 */
int compare(char *p1, char *p2)
{
  char tmp1[200], tmp2[200];
  register char *s1 = tmp1, *s2 = tmp2;
  register int ip1, ip2, in_uh = 0;

  do
  {
    ip1 = ip2 = 1;

    while (*p1 && *p1 != '!' && *p1 != '@')
    {
      if (ip1 && !isdigit(*p1) && *p1 != '.' && (*p1 != '*' || p1[1]) && (*p1 == '*' || s1 != tmp1))
	ip1 = 0;
      *(s1++) = *(p1++);
    }
    if (!in_uh && *p1 == '\0')
      in_uh = 1;

    while (*p2 && *p2 != '!' && *p2 != '@')
    {
      if (ip2 && !isdigit(*p2) && *p2 != '.' && (*p2 != '*' || p2[1]) && (*p2 == '*' || s2 != tmp2))
	ip2 = 0;
      *(s2++) = *(p2++);
    }

    *s2 = *s1 = '\0';

    if (*p1 != *p2)
    {
      if (*p1 == '!')
      {
	s1 = tmp1;
	if (*p1)
	  p1++;
	while (*p1 && *p1 != '@')
	{
	  if (ip1 && !isdigit(*p1) && *p1 != '.' && (*p1 != '*' || p1[1]) && (*p1 == '*' || s1 != tmp1))
	    ip1 = 0;
	  *(s1++) = *(p1++);
	}
	if (!in_uh && *p1 == '\0')
	  in_uh = 1;
      }
      else
      {
	s2 = tmp2;
	if (*p2)
	  p2++;
	while (*p2 && *p2 != '@')
	{
	  if (ip2 && !isdigit(*p2) && *p2 != '.' && (*p2 != '*' || p2[1]) && (*p1 == '*' || s2 != tmp2))
	    ip2 = 0;
	  *(s2++) = *(p2++);
	}
      }
      *s2 = *s1 = '\0';
    }
    if ((in_uh && ip1 != ip2) || !xmatch(tmp1, tmp2))
      return 0;

    if (*p1)
      p1++;
    if (*p2)
      p2++;

    s1 = tmp1;
    s2 = tmp2;

  }
  while (*p1);

  return 1;
}


int key_match(char *s, char *tok[])
{
  register int k;

  for (k = 0; tok[k] != NULL; k++)
  {
    if (!strcasestr(s, tok[k]))
      return 0;
  }

  return 1;
}

void string_swap(char *buf, size_t size, char *orig, char *new)
{
  char *ptr, *buf2;

  buf2 = (char *)alloca(sizeof(char) * size);
  while ((ptr = strstr((buf), orig)))
  {
    *ptr = '\0';
    strncpy(buf2, buf, size - 1);
    strncat(buf2, new, size - strlen(buf) - 1);
    strncat(buf2, ptr + strlen(orig), size - strlen(buf) - strlen(new) - 1);
    strcpy(buf, buf2);
  }
}


/* regex_cmp()   (stolen from RCSD)
 * This function compares a regular expression (patern) with a string.
 * Return value:  1 if there is a match
 *                0 otherwise
 *
 * Note: if patern is NULL, the last patern that was passed to the
 *       function is used.
 */
#define HAVE_REGEXEC
int regex_cmp(char *patern, char *string)
{
#if defined(HAVE_REGEXEC)
  static regex_t preg;
  static int called_once = 0;
  register int cmpv = 0, execv;

  if (patern != NULL)
  {
    if (called_once)
      regfree(&preg);
    cmpv = regcomp(&preg, patern, REG_EXTENDED | REG_ICASE | REG_NOSUB);
  }

  called_once = 1;

  if (cmpv == 0)
    execv = regexec(&preg, string, 0, NULL, 0);

  return cmpv ? 0 : execv ? 0 : 1;

#elif defined(HAVE_RE_EXEC)
  register char *ptr;

  ptr = re_comp(patern);
  return ptr ? 0 : re_exec(string);

#else
#error neither re_exec() nor rexexec() :/
#endif
}
#ifdef MATCHALONE
void main(void)
{
  char str1[80];
  char str2[80];
  puts("#1");
  gets(str1);
  puts("#2");
  gets(str2);
  printf("match: %d\n", match(str1, str2));
}
#endif

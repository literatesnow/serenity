/*
Copyright (C) 2004 Chris Cuthbertson

This file is part of Serenity.

Serenity is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Serenity is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Serenity; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "util.h"

/*
 * escape - adds escape characters to a string for a string
 * "'" is okay in php so it is not escaped.
 */
void escape(char *s, int max)
{
  char buff[MAX_ESCAPE_LENGTH+1]; //should be enough
  char *start;
  char *p;
  int i;

  i = 0;
  start = s;
  p = buff;
  while (*s)
  {
    if (*s == '\"' || *s == '\'' || *s == '\\')
      *p++ = '\\';
    if (i < MAX_ESCAPE_LENGTH+1)
    {
      *p++ = *s++;
      i++;
    }
  }
  *p = 0;

  i = 0;
  p = buff;
  s = start;
  while (*p)
  {
    if (i < max)
    {
      *s++ = *p++;
      i++;
    }
  }
  *s = 0;
}

/*
 * atoipz - like atoi but for ID numbers: returns number + 1 or 0 for bad number because 0 might be an ID
 */
int atoipz(char *s)
{
	int val;
	int c;
	
	if (*s == '-')
		return 0;
		
	val = 0;

	while (1)
  {
		c = *s++;
		if (c < '0' || c > '9')
			return val + 1;
		val = val * 10 + c - '0';
	}
	
	return 0;
}

/*
 * validip - check if an IP address is valid or not
 */
int validip(char *s)
{
  int i;
  int dot;

  i = 0;
  dot = 0;
  while (*s)
  {
    if (!((*s >= 48 && *s <= 57) || (*s == 46)))
      return 0;
    if (*s == 46)
      dot++;
    i++;
    *s++;
  }
  if ((dot == 3) && (i < 16))
    return 1;

  return 0;
}

/*
 * remainder - returns remainder of the queryid
 */
int remainder(char *s)
{
  char buff[8];
  char *p;

  p = buff;
  while (*s && *s != '.')
    *s++;

  if (*s)
    *s++;

  while (*s)
    *p++ = *s++;
  *p = 0;

  return atoi(buff);
}

/*
 * atolz - same as atoi but for unsigned long
 */
unsigned long atolz(char *s)
{
	unsigned long val;
	int c;
	
	if (*s == '-')
		return 1;
		
	val = 0;

	while (1)
  {
		c = *s++;
		if (c < '0' || c > '9')
			return val;
		val = val * 10 + c - '0';
	}
	
	return 1;
}

/*
 * datetime - fill a string with formatted date and time
 */
void datetime(char *s, int len)
{
  struct tm *timeinfo;
  time_t rawtime;

  time(&rawtime);
  timeinfo = localtime(&rawtime);

  strftime(s, len, "%Y-%m-%d %H:%M:%S", timeinfo);
}

/*
 * epoch - returns seconds from epoch (legacy)
 */
unsigned int epoch(void)
{
  time_t t;

  return time(&t);
}

/*
 * leapyear - test a year if it's a leap year
 */
int leapyear(int year)
{
  if (year % 4 == 0)
  {
    if (year % 100 == 0)
    {
      if (year % 400 == 0)
        return 1;
      else
        return 0;
    }
    else
      return 1;
  }
  return 0;
}

/*
 * monthdays - returns number of days in specified month
 */
int monthdays(int month, int year)
{
  switch (month + 1)
  {
    case 2: //feb 
      return (leapyear(year + 1900)) ? 29 : 28;

    case 4: //apr
    case 6: //jun
    case 9: //sep
    case 11: //nov
      return 30;

    case 1: //jan
    case 3: //mar
    case 5: //may
    case 7: //jul
    case 8: //aug
    case 10: //oct
    case 12: //dec
      return 31;

    default: return 1;
  }
}

/*
 * memcatz - memory concatination
 * I needed a function which worked like strlcat but on binary data, couldn't find one so had to make my own
 */
int memcatz(char *dst, int dstlen, const char *src, int srclen, int siz)
{
  if (dstlen + srclen > siz)
    return 0;

  dst += dstlen;

  memcpy(dst, src, srclen);
  dst[srclen] = 0;

  return 1;
}

// http://www.courtesan.com/todd/papers/strlcpy.html
/* $OpenBSD: strlcat.c and strlcpy.c,v 1.11 2003/06/17 21:56:24 millert Exp $
 *
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t strlcat(char *dst, const char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t strlcpy(char *dst, const char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}


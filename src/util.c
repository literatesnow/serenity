/*
  Copyright (C) 2004-2008 Chris Cuthbertson

  This file is part of Serenity.

  Serenity is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Serenity is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Serenity.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "util.h"

size_t pcrestrcpy(char *dst, const char *src, int pos, int ovec[], int max) {
  int len = (ovec[2 * pos + 1] - ovec[2 * pos]) + 1;
  if (len > max) {
    len = max;
  }
  return strlcpy(dst, src + ovec[2 * pos], len);
}

int pcreatoi(const char *src, int pos, int ovec[]) {
  char buf[32];

  pcrestrcpy(buf, src, pos, ovec, sizeof(buf));
  return atoi(buf);
}

#ifdef WIN32
double DoubleTime(void)
{
  double now;
  static double start = 0.0;
  
  if (!start)
    start = GetTickCount();

  now = GetTickCount();

  if (now < start)
    return (now / 1000.0) + (LONG_MAX - start / 1000.0);

  if ((now - start) == 0)
    return 0.0;

  return (now - start) / 1000.0;
}
#endif

#ifdef UNIX
double DoubleTime(void)
{
  struct timeval tp;
  static int secbase = 0;
  
  gettimeofday(&tp, NULL);
  
  if (!secbase)
  {
    secbase = tp.tv_sec;
    return tp.tv_usec / 1000000.0;
  }
  
  return (tp.tv_sec - secbase) + tp.tv_usec / 1000000.0;
}
#endif

char *workingdir(char *path)
{
  char *s, *p;
  int i;

  s = path;
  if (*s == '\"')
    s++;

  i = strlen(s);
  p = &path[(i > 0) ? i - 1 : i];

  while (i > 0 &&
#ifdef WIN32
    *p != '\\'
#endif
#ifdef UNIX
    *p != '/'
#endif
    )
  {
    p--;
    i--;
  }

  *(p + 1) = '\0';

  return s;
}

/*  $OpenBSD: strlcat.c and strlcpy.c,v 1.11 2003/06/17 21:56:24 millert Exp $
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
size_t strlcat(char *dst, const char *src, size_t siz) {
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
    return (dlen + strlen(s));
  while (*s != '\0')
  {
    if (n != 1)
    {
      *d++ = *s;
      n--;
    }
    s++;
  }
  *d = '\0';

  return (dlen + (s - src));  /* count does not include NUL */
}

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t strlcpy(char *dst, const char *src, size_t siz) {
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
      *d = '\0';    /* NUL-terminate dst */
    while (*s++)
      ;
  }

  return (s - src - 1);  /* count does not include NUL */
}


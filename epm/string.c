/*
 * "$Id: string.c 611 2005-01-11 21:37:42Z mike $"
 *
 *   String functions for the ESP Package Manager (EPM).
 *
 *   Copyright 1999-2005 by Easy Software Products.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 * Contents:
 *
 *   epm_strdup()      - Duplicate a string.
 *   epm_strcasecmp()  - Do a case-insensitive comparison.
 *   epm_strlcat()     - Safely concatenate two strings.
 *   epm_strlcpy()     - Safely copy two strings.
 *   epm_strncasecmp() - Do a case-insensitive comparison on up to N chars.
 */

/*
 * Include necessary headers...
 */

#include "epmstring.h"


/*
 * 'epm_strdup()' - Duplicate a string.
 */

#  ifndef HAVE_STRDUP
char *					/* O - New string pointer */
epm_strdup(const char *s)		/* I - String to duplicate */
{
  char	*t;				/* New string pointer */


  if (s == NULL)
    return (NULL);

  if ((t = malloc(strlen(s) + 1)) == NULL)
    return (NULL);

  return (strcpy(t, s));
}
#  endif /* !HAVE_STRDUP */


/*
 * 'epm_strcasecmp()' - Do a case-insensitive comparison.
 */

#  ifndef HAVE_STRCASECMP
int					/* O - Result of comparison (-1, 0, or 1) */
epm_strcasecmp(const char *s,		/* I - First string */
               const char *t)		/* I - Second string */
{
  while (*s != '\0' && *t != '\0')
  {
    if (tolower(*s) < tolower(*t))
      return (-1);
    else if (tolower(*s) > tolower(*t))
      return (1);

    s ++;
    t ++;
  }

  if (*s == '\0' && *t == '\0')
    return (0);
  else if (*s != '\0')
    return (1);
  else
    return (-1);
}
#  endif /* !HAVE_STRCASECMP */


#ifndef HAVE_STRLCAT
/*
 * 'epm_strlcat()' - Safely concatenate two strings.
 */

size_t					/* O - Length of string */
epm_strlcat(char       *dst,		/* O - Destination string */
            const char *src,		/* I - Source string */
	    size_t     size)		/* I - Size of destination string buffer */
{
  size_t	srclen;			/* Length of source string */
  size_t	dstlen;			/* Length of destination string */


 /*
  * Figure out how much room is left...
  */

  dstlen = strlen(dst);
  size   -= dstlen + 1;

  if (!size)
    return (dstlen);			/* No room, return immediately... */

 /*
  * Figure out how much room is needed...
  */

  srclen = strlen(src);

 /*
  * Copy the appropriate amount...
  */

  if (srclen > size)
    srclen = size;

  memcpy(dst + dstlen, src, srclen);
  dst[dstlen + srclen] = '\0';

  return (dstlen + srclen);
}
#endif /* !HAVE_STRLCAT */


#ifndef HAVE_STRLCPY
/*
 * 'epm_strlcpy()' - Safely copy two strings.
 */

size_t					/* O - Length of string */
epm_strlcpy(char       *dst,		/* O - Destination string */
            const char *src,		/* I - Source string */
	    size_t      size)		/* I - Size of destination string buffer */
{
  size_t	srclen;			/* Length of source string */


 /*
  * Figure out how much room is needed...
  */

  size --;

  srclen = strlen(src);

 /*
  * Copy the appropriate amount...
  */

  if (srclen > size)
    srclen = size;

  memcpy(dst, src, srclen);
  dst[srclen] = '\0';

  return (srclen);
}
#endif /* !HAVE_STRLCPY */


/*
 * 'epm_strncasecmp()' - Do a case-insensitive comparison on up to N chars.
 */

#  ifndef HAVE_STRNCASECMP
int					/* O - Result of comparison (-1, 0, or 1) */
epm_strncasecmp(const char *s,		/* I - First string */
                const char *t,		/* I - Second string */
		size_t     n)		/* I - Maximum number of characters to compare */
{
  while (*s != '\0' && *t != '\0' && n > 0)
  {
    if (tolower(*s) < tolower(*t))
      return (-1);
    else if (tolower(*s) > tolower(*t))
      return (1);

    s ++;
    t ++;
    n --;
  }

  if (n == 0)
    return (0);
  else if (*s == '\0' && *t == '\0')
    return (0);
  else if (*s != '\0')
    return (1);
  else
    return (-1);
}
#  endif /* !HAVE_STRNCASECMP */


/*
 * End of "$Id: string.c 611 2005-01-11 21:37:42Z mike $".
 */

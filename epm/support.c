/*
 * "$Id: support.c 733 2006-11-30 21:59:27Z mike $"
 *
 *   Support functions for the ESP Package Manager (EPM).
 *
 *   Copyright 1999-2006 by Easy Software Products.
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
 *   get_vernumber() - Convert a version string to a number...
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * 'get_vernumber()' - Convert a version string to a number...
 */

int					/* O - Version number */
get_vernumber(const char *version)	/* I - Version string */
{
  int		numbers[4],		/* Raw version numbers */
		nnumbers,		/* Number of numbers in version */
		temp,			/* Temporary version number */
		offset;			/* Offset for last version number */
  const char	*ptr;			/* Pointer into string */


 /*
  * Loop through the version number string and construct a version number.
  */

  memset(numbers, 0, sizeof(numbers));

  for (ptr = version; *ptr && !isdigit(*ptr & 255); ptr ++);
    /* Skip leading letters, periods, etc. */

  for (offset = 0, temp = 0, nnumbers = 0; *ptr && !isspace(*ptr & 255); ptr ++)
    if (isdigit(*ptr & 255))
      temp = temp * 10 + *ptr - '0';
    else
    {
     /*
      * Add each mini version number (m.n.p) and patch/pre stuff...
      */

      if (nnumbers < 4)
      {
        numbers[nnumbers] = temp;
	nnumbers ++;
      }

      temp = 0;

      if (*ptr == '.')
	offset = 0;
      else if (*ptr == 'p' || *ptr == '-')
      {
	if (strncmp(ptr, "pre", 3) == 0)
	{
	  ptr += 2;
	  offset = -20;
	}
	else
	  offset = 0;

        nnumbers = 3;
      }
      else if (*ptr == 'b')
      {
	offset   = -50;
        nnumbers = 3;
      }
      else /* if (*ptr == 'a') */
      {
	offset   = -100;
        nnumbers = 3;
      }
    }

  if (nnumbers < 4)
    numbers[nnumbers] = temp;

 /*
  * Compute the version number as MMmmPPpp + offset
  */

  return (((numbers[0] * 100 + numbers[1]) * 100 + numbers[2]) * 100 +
          numbers[3] + offset);
}


/*
 * End of "$Id: support.c 733 2006-11-30 21:59:27Z mike $".
 */

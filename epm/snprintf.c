/*
 * "$Id: snprintf.c 611 2005-01-11 21:37:42Z mike $"
 *
 *   snprintf functions for the ESP Package Manager (EPM).
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
 *   epm_vsnprintf() - Format a string into a fixed size buffer.
 *   epm_snprintf()  - Format a string into a fixed size buffer.
 */

/*
 * Include necessary headers...
 */

#include <stdio.h>
#include <ctype.h>
#include "epmstring.h"


#ifndef HAVE_VSNPRINTF
/*
 * 'vsnprintf()' - Format a string into a fixed size buffer.
 */

int					/* O - Number of bytes formatted */
epm_vsnprintf(char       *buffer,	/* O - Output buffer */
              size_t     bufsize,	/* O - Size of output buffer */
	      const char *format,	/* I - printf-style format string */
 	      va_list    ap)		/* I - Pointer to additional arguments */
{
  char		*bufptr,		/* Pointer to position in buffer */
		*bufend,		/* Pointer to end of buffer */
		sign,			/* Sign of format width */
		size,			/* Size character (h, l, L) */
		type;			/* Format type character */
  const char	*bufformat;		/* Start of format */
  int		width,			/* Width of field */
		prec;			/* Number of characters of precision */
  char		tformat[100],		/* Temporary format string for sprintf() */
		temp[1024];		/* Buffer for formatted numbers */
  char		*s;			/* Pointer to string */
  int		slen;			/* Length of string */


 /*
  * Loop through the format string, formatting as needed...
  */

  bufptr = buffer;
  bufend = buffer + bufsize - 1;

  while (*format && bufptr < bufend)
  {
    if (*format == '%')
    {
      bufformat = format;
      format ++;

      if (*format == '%')
      {
        *bufptr++ = *format++;
	continue;
      }
      else if (strchr(" -+#\'", *format))
        sign = *format++;
      else
        sign = 0;

      width = 0;
      while (isdigit(*format))
        width = width * 10 + *format++ - '0';

      if (*format == '.')
      {
        format ++;
	prec = 0;

	while (isdigit(*format))
          prec = prec * 10 + *format++ - '0';
      }
      else
        prec = -1;

      if (*format == 'l' && format[1] == 'l')
      {
        size = 'L';
	format += 2;
      }
      else if (*format == 'h' || *format == 'l' || *format == 'L')
        size = *format++;

      if (!*format)
        break;

      type = *format++;

      switch (type)
      {
	case 'E' : /* Floating point formats */
	case 'G' :
	case 'e' :
	case 'f' :
	case 'g' :
	    if ((format - bufformat + 1) > sizeof(tformat) ||
	        (width + 2) > sizeof(temp))
	      break;

	    strlcpy(tformat, bufformat, format - bufformat + 1);

	    sprintf(temp, tformat, va_arg(ap, double));

	    if ((bufptr + strlen(temp)) > bufend)
	    {
	      strncpy(bufptr, temp, bufend - bufptr);
	      bufptr = bufend;
	      break;
	    }
	    else
	    {
	      strcpy(bufptr, temp);
	      bufptr += strlen(temp);
	    }
	    break;

        case 'B' : /* Integer formats */
	case 'X' :
	case 'b' :
        case 'd' :
	case 'i' :
	case 'o' :
	case 'u' :
	case 'x' :
	    if ((format - bufformat + 1) > sizeof(tformat) ||
	        (width + 2) > sizeof(temp))
	      break;

	    strlcpy(tformat, bufformat, format - bufformat + 1);

	    sprintf(temp, tformat, va_arg(ap, int));

	    if ((bufptr + strlen(temp)) > bufend)
	    {
	      strncpy(bufptr, temp, bufend - bufptr);
	      bufptr = bufend;
	      break;
	    }
	    else
	    {
	      strcpy(bufptr, temp);
	      bufptr += strlen(temp);
	    }
	    break;
	    
	case 'p' : /* Pointer value */
	    if ((format - bufformat + 1) > sizeof(tformat) ||
	        (width + 2) > sizeof(temp))
	      break;

	    strlcpy(tformat, bufformat, format - bufformat + 1);

	    sprintf(temp, tformat, va_arg(ap, void *));

	    strlcpy(bufptr, temp, bufend - bufptr + 1);
	    bufptr += strlen(bufptr);
	    break;

        case 'c' : /* Character or character array */
	    if (width <= 1)
	      *bufptr++ = va_arg(ap, int);
	    else
	    {
	      if ((bufptr + width) > bufend)
	        width = bufend - bufptr;

	      memcpy(bufptr, va_arg(ap, char *), width);
	      bufptr += width;
	    }
	    break;

	case 's' : /* String */
	    if ((s = va_arg(ap, char *)) == NULL)
	      s = "(null)";

	    slen = strlen(s);
	    if (slen > width && prec != width)
	      width = slen;

	    if ((bufptr + width) > bufend)
	      width = bufend - bufptr;

            if (slen > width)
	      slen = width;

	    if (sign == '-')
	    {
	      strncpy(bufptr, s, slen);
	      memset(bufptr + slen, ' ', width - slen);
	    }
	    else
	    {
	      memset(bufptr, ' ', width - slen);
	      strncpy(bufptr + width - slen, s, slen);
	    }

	    bufptr += width;
	    break;

	case 'n' : /* Output number of chars so far */
	    if ((format - bufformat + 1) > sizeof(tformat) ||
	        (width + 2) > sizeof(temp))
	      break;

	    strlcpy(tformat, bufformat, format - bufformat + 1);

	    sprintf(temp, tformat, va_arg(ap, int));

	    strlcpy(bufptr, temp, bufend - bufptr + 1);
	    bufptr += strlen(bufptr);
	    break;
      }
    }
    else
      *bufptr++ = *format++;
  }

 /*
  * Nul-terminate the string and return the number of characters in it.
  */

  *bufptr = '\0';
  return (bufptr - buffer);
}
#endif /* !HAVE_VSNPRINT */


#ifndef HAVE_SNPRINTF
/*
 * 'epm_snprintf()' - Format a string into a fixed size buffer.
 */

int					/* O - Number of bytes formatted */
epm_snprintf(char       *buffer,	/* O - Output buffer */
             size_t     bufsize,	/* O - Size of output buffer */
             const char *format,	/* I - printf-style format string */
	     ...)			/* I - Additional arguments as needed */
{
  int		bytes;			/* Number of bytes formatted */
  va_list 	ap;			/* Pointer to additional arguments */


  va_start(ap, format);
  bytes = vsnprintf(buffer, bufsize, format, ap);
  va_end(ap);

  return (bytes);
}
#endif /* !HAVE_SNPRINTF */


/*
 * End of "$Id: snprintf.c 611 2005-01-11 21:37:42Z mike $".
 */


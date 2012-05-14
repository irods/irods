/*
 * "$Id: qprintf.c 826 2010-12-29 17:29:19Z mike $"
 *
 *   Quoted fprintf function for the ESP Package Manager (EPM).
 *
 *   Copyright 1999-2009 by Easy Software Products.
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
 *   qprintf() - Do formatted output to a file.
 */

/*
 * Include necessary headers...
 */

#include <stdio.h>
#include <ctype.h>
#include "epmstring.h"


/*
 * 'qprintf()' - Do formatted output to a file.
 */

int				/* O - Number of bytes formatted */
qprintf(FILE       *fp,		/* I - File to write to */
        const char *format,	/* I - printf-style format string */
	...)			/* I - Additional args as needed... */
{
  va_list	ap;		/* Pointer to additional arguments */
  int		bytes;		/* Bytes written */
  char		sign,		/* Sign of format width */
		size,		/* Size character (h, l, L) */
		type;		/* Format type character */
  const char	*bufformat;	/* Start of format */
  int		width,		/* Width of field */
		prec;		/* Number of characters of precision */
  char		tformat[100];	/* Temporary format string for fprintf() */
  char		*s;		/* Pointer to string */
  int		slen;		/* Length of string */
  int		i;		/* Looping var */


 /*
  * Loop through the format string, formatting as needed...
  */

  va_start(ap, format);

  bytes = 0;

  while (*format)
  {
    if (*format == '%')
    {
      bufformat = format;
      format ++;

      if (*format == '%')
      {
        putc(*format++, fp);
	bytes ++;
	continue;
      }
      else if (strchr(" -+#\'", *format))
        sign = *format++;
      else
        sign = 0;

      width = 0;
      while (isdigit(*format & 255))
        width = width * 10 + *format++ - '0';

      if (*format == '.')
      {
        format ++;
	prec = 0;

	while (isdigit(*format & 255))
          prec = prec * 10 + *format++ - '0';
      }
      else
        prec = -1;

      if (*format == 'l')
        size = *format++;
      else
        size = '\0';

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
	    if ((format - bufformat + 1) > sizeof(tformat))
	      break;

	    strlcpy(tformat, bufformat, (size_t)(format - bufformat + 1));

	    bytes += fprintf(fp, tformat, va_arg(ap, double));
	    break;

        case 'B' : /* Integer formats */
	case 'X' :
	case 'b' :
        case 'd' :
	case 'i' :
	case 'o' :
	case 'u' :
	case 'x' :
	    if ((format - bufformat + 1) > sizeof(tformat))
	      break;

	    strlcpy(tformat, bufformat, (size_t)(format - bufformat + 1));

            if (size == 'l')
	      bytes += fprintf(fp, tformat, va_arg(ap, long));
	    else
	      bytes += fprintf(fp, tformat, va_arg(ap, int));
	    break;
	    
	case 'p' : /* Pointer value */
	    if ((format - bufformat + 1) > sizeof(tformat))
	      break;

	    strlcpy(tformat, bufformat, (size_t)(format - bufformat + 1));

	    bytes += fprintf(fp, tformat, va_arg(ap, void *));
	    break;

        case 'c' : /* Character or character array */
	    if (width <= 1)
	    {
	      bytes ++;
	      putc(va_arg(ap, int), fp);
	    }
	    else
	    {
	      fwrite(va_arg(ap, char *), 1, (size_t)width, fp);
	      bytes += width;
	    }
	    break;

	case 's' : /* String */
	    if ((s = va_arg(ap, char *)) == NULL)
	      s = "(null)";

	    slen = strlen(s);
	    if (slen > width && prec != width)
	      width = slen;

            if (slen > width)
	      slen = width;

            if (sign != '-')
	    {
	      for (i = width - slen; i > 0; i --, bytes ++)
	        putc(' ', fp);
	    }

            for (i = slen; i > 0; i --, s ++, bytes ++)
	    {
	      if (strchr("`~#$%^&*()[{]}\\|;\'\"<>? ", *s))
	      {
	        putc('\\', fp);
		bytes ++;
	      }

	      putc(*s, fp);
	    }

            if (sign == '-')
	    {
	      for (i = width - slen; i > 0; i --, bytes ++)
	        putc(' ', fp);
	    }
	    break;
      }
    }
    else
    {
      putc(*format++, fp);
      bytes ++;
    }
  }

  va_end(ap);

 /*
  * Return the number of characters written.
  */

  return (bytes);
}


/*
 * End of "$Id: qprintf.c 826 2010-12-29 17:29:19Z mike $".
 */


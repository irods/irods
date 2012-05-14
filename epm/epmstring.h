/*
 * "$Id: epmstring.h 611 2005-01-11 21:37:42Z mike $"
 *
 *   String definitions for the ESP Package Manager (EPM).
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
 */

#ifndef _EPM_STRING_H_
#  define _EPM_STRING_H_

/*
 * Include necessary headers...
 */

#  include "config.h"
#  include <string.h>
#  include <stdarg.h>

#  ifdef HAVE_STRINGS_H
#    include <strings.h>
#  endif /* HAVE_STRINGS_H */

#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */


/*
 * Prototypes...
 */

#  ifndef HAVE_STRDUP
extern char	*epm_strdup(const char *);
#    define strdup epm_strdup
#  endif /* !HAVE_STRDUP */

#  ifndef HAVE_STRCASECMP
extern int	epm_strcasecmp(const char *, const char *);
#    define strcasecmp epm_strcasecmp
#  endif /* !HAVE_STRCASECMP */

#  ifndef HAVE_STRNCASECMP
extern int	epm_strncasecmp(const char *, const char *, size_t n);
#    define strncasecmp epm_strncasecmp
#  endif /* !HAVE_STRNCASECMP */

#  ifndef HAVE_STRLCAT
extern size_t epm_strlcat(char *, const char *, size_t);
#    define strlcat epm_strlcat
#  endif /* !HAVE_STRLCAT */

#  ifndef HAVE_STRLCPY
extern size_t epm_strlcpy(char *, const char *, size_t);
#    define strlcpy epm_strlcpy
#  endif /* !HAVE_STRLCPY */

#  ifndef HAVE_SNPRINTF
extern int	epm_snprintf(char *, size_t, const char *, ...)
#    ifdef __GNUC__
__attribute__ ((__format__ (__printf__, 3, 4)))
#    endif /* __GNUC__ */
;
#    define snprintf epm_snprintf
#  endif /* !HAVE_SNPRINTF */

#  ifndef HAVE_VSNPRINTF
extern int	epm_vsnprintf(char *, size_t, const char *, va_list);
#    define vsnprintf epm_vsnprintf
#  endif /* !HAVE_VSNPRINTF */

#  ifdef __cplusplus
}
#  endif /* __cplusplus */

#endif /* !_EPM_STRING_H_ */

/*
 * End of "$Id: epmstring.h 611 2005-01-11 21:37:42Z mike $".
 */

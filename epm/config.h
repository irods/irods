/* config.h.  Generated from config.h.in by configure.  */
/*
 * "$Id: config.h.in 648 2005-10-19 14:25:05Z mike $"
 *
 *   Configuration file for the ESP Package Manager (EPM).
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

/*
 * Version of software...
 */

#define EPM_VERSION "ESP Package Manager v4.2"


/*
 * Where are files stored?
 */

#define EPM_SOFTWARE "/etc/software"


/*
 * Where are binaries stored?
 */

#define EPM_BINDIR "/usr/bin"
#define EPM_DATADIR "/usr/share/epm"
#define EPM_LIBDIR "/usr/lib/epm"


/*
 * What options does the strip command take?
 */

#define EPM_STRIP "/usr/bin/strip"


/*
 * What command is used to build RPMs?
 */

#define EPM_RPMBUILD "/usr/bin/rpmbuild"


/*
 * What option is used to specify the RPM architecture?
 */

#define EPM_RPMARCH "--target "


/*
 * Does this version of RPM support the "topdir_" variable?
 */

#define EPM_RPMTOPDIR 1


/*
 * Compiler stuff...
 */

/* #undef const */
/* #undef __CHAR_UNSIGNED__ */


/*
 * Do we have the <strings.h> header file?
 */

#define HAVE_STRINGS_H 1


/*
 * Which header files do we use for the statfs() function?
 */

#define HAVE_SYS_MOUNT_H 1
#define HAVE_SYS_STATFS_H 1
#define HAVE_SYS_VFS_H 1
#define HAVE_SYS_PARAM_H 1


/*
 * Do we have the strXXX() functions?
 */

#define HAVE_STRCASECMP 1
#define HAVE_STRDUP 1
/* #undef HAVE_STRLCAT */
/* #undef HAVE_STRLCPY */
#define HAVE_STRNCASECMP 1


/*
 * Do we have the (v)snprintf() functions?
 */

#define HAVE_SNPRINTF 1
#define HAVE_VSNPRINTF 1


/*
 * Which directory functions and headers do we use?
 */

#define HAVE_DIRENT_H 1
/* #undef HAVE_SYS_DIR_H */
/* #undef HAVE_SYS_NDIR_H */
/* #undef HAVE_NDIR_H */


/*
 * Where is the "gzip" executable?
 */

#define EPM_GZIP "/bin/gzip"


/*
 * Compress files by default?
 */

#define EPM_COMPRESS 1


/*
 * End of "$Id: config.h.in 648 2005-10-19 14:25:05Z mike $".
 */

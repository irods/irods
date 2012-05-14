/*
 * "$Id: epm.h 833 2010-12-30 00:00:50Z mike $"
 *
 *   Definitions for the ESP Package Manager (EPM).
 *
 *   Copyright 1999-2010 by Easy Software Products.
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
 * Include necessary headers...
 */

#ifndef _EPM_H_
#  define _EPM_H_

#  include <stdio.h>
#  include <stdlib.h>
#  include <unistd.h>
#  include <limits.h>
#  include "epmstring.h"
#  include <ctype.h>
#  include <errno.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <time.h>
#  include <pwd.h>
#  include <grp.h>
#  include <sys/utsname.h>

#  if HAVE_DIRENT_H
#    include <dirent.h>
typedef struct dirent DIRENT;
#    define NAMLEN(dirent) strlen((dirent)->d_name)
#  else
#    if HAVE_SYS_NDIR_H
#      include <sys/ndir.h>
#    endif
#    if HAVE_SYS_DIR_H
#      include <sys/dir.h>
#    endif
#    if HAVE_NDIR_H
#      include <ndir.h>
#    endif
typedef struct direct DIRENT;
#    define NAMLEN(dirent) (dirent)->d_namlen
#  endif


#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */


/*
 * Macro to eliminate "variable was never referenced" errors...
 */

#  define REF(x)	(void)(x);


/*
 * TAR constants...
 */

#  define TAR_BLOCK	512		/* Number of bytes in a block */
#  define TAR_BLOCKS	10		/* Blocking factor */

#  define TAR_MAGIC	"ustar"		/* 5 chars and a null */
#  define TAR_VERSION	"00"		/* POSIX tar version */

#  define TAR_OLDNORMAL	'\0'		/* Normal disk file, Unix compat */
#  define TAR_NORMAL	'0'		/* Normal disk file */
#  define TAR_LINK	'1'		/* Link to previously dumped file */
#  define TAR_SYMLINK	'2'		/* Symbolic link */
#  define TAR_CHR	'3'		/* Character special file */
#  define TAR_BLK	'4'		/* Block special file */
#  define TAR_DIR	'5'		/* Directory */
#  define TAR_FIFO	'6'		/* FIFO special file */
#  define TAR_CONTIG	'7'		/* Contiguous file */

/*
 * Package formats...
 */

enum
{
  PACKAGE_PORTABLE,			/* Shell-script based EPM */
  PACKAGE_AIX,				/* AIX package format */
  PACKAGE_BSD,				/* BSD package format */
  PACKAGE_DEB,				/* Debian package format */
  PACKAGE_INST,				/* IRIX package format */
  PACKAGE_LSB,				/* LSB (RPM) package format */
  PACKAGE_LSB_SIGNED,			/* LSB (RPM) package format (signed) */
  PACKAGE_OSX,				/* MacOS X package format */
  PACKAGE_PKG,				/* AT&T package format (AIX, Solaris) */
  PACKAGE_RPM,				/* RedHat package format */
  PACKAGE_RPM_SIGNED,			/* RedHat package format (signed) */
  PACKAGE_SETLD,			/* Tru64 package format */
  PACKAGE_SLACKWARE,			/* Slackware package format */
  PACKAGE_SWINSTALL			/* HP-UX package format */
};

/*
 * Command types...
 */

enum
{
  COMMAND_PRE_INSTALL,			/* Command to run before install */
  COMMAND_POST_INSTALL,			/* Command to run after install */
  COMMAND_PRE_PATCH,			/* Command to run before patch */
  COMMAND_POST_PATCH,			/* Command to run after patch */
  COMMAND_PRE_REMOVE,			/* Command to run before remove */
  COMMAND_POST_REMOVE,			/* Command to run after remove */
  COMMAND_LITERAL			/* Literal (format-specific) data */
};

/*
 * Dependency types...
 */

enum
{
  DEPEND_REQUIRES,			/* This product requires */
  DEPEND_INCOMPAT,			/* This product is incompatible with */
  DEPEND_REPLACES,			/* This product replaces */
  DEPEND_PROVIDES			/* This product provides */
};


/*
 * Structures...
 */

typedef union				/**** TAR record format ****/
{
  unsigned char	all[TAR_BLOCK];		/* Raw data block */
  struct
  {
    char	pathname[100],		/* Destination path */
		mode[8],		/* Octal file permissions */
		uid[8],			/* Octal user ID */
		gid[8],			/* Octal group ID */
		size[12],		/* Octal size in bytes */
		mtime[12],		/* Octal modification time */
		chksum[8],		/* Octal checksum value */
		linkflag,		/* File type */
		linkname[100],		/* Source path for link */
		magic[6],		/* Magic string */
		version[2],		/* Format version */
		uname[32],		/* User name */
		gname[32],		/* Group name */
		devmajor[8],		/* Octal device major number */
		devminor[8],		/* Octal device minor number */
		prefix[155];		/* Prefix for long filenames */
  }	header;
} tar_t;

typedef struct				/**** TAR file ****/
{
  FILE		*file;			/* File to write to */
  int		blocks,			/* Number of blocks written */
		compressed;		/* Compressed output? */
} tarf_t;

typedef struct				/**** File to install ****/
{
  int		type;			/* Type of file */
  mode_t	mode;			/* Permissions of file */
  char		user[32],		/* Owner of file */
		group[32],		/* Group of file */
		src[512],		/* Source path */
		dst[512],		/* Destination path */
		options[256];		/* File options */
  const char	*subpackage;		/* Sub-package name */
} file_t;

typedef struct				/**** Install/Patch/Remove Commands ****/
{
  int		type;			/* Command type */
  char		*command;		/* Command string */
  const char	*subpackage;		/* Sub-package name */
  char		*section;		/* Literal section */
} command_t;

typedef struct				/**** Dependencies ****/
{
  int		type;			/* Dependency type */
  char		product[256];		/* Product name */
  char		version[2][256];	/* Product version string */
  int		vernumber[2];		/* Product version number */
  const char	*subpackage;		/* Sub-package name */
} depend_t;

typedef struct				/**** Description Structure ****/
{
  char		*description;		/* Description */
  const char	*subpackage;		/* Sub-package name */
} description_t;

typedef struct				/**** Distribution Structure ****/
{
  char		product[256],		/* Product name */
		version[256],		/* Product version string */
		release[256],		/* Product release string */
		copyright[256],		/* Product copyright */
		vendor[256],		/* Vendor name */
		packager[256],		/* Packager name */
		license[256],		/* License file to copy */
		readme[256];		/* README file to copy */
  int		num_subpackages;	/* Number of subpackages */
  char		**subpackages;		/* Subpackage names */
  int		num_descriptions;	/* Number of description strings */
  description_t	*descriptions;		/* Description strings */
  int		vernumber,		/* Version number */
		epoch;			/* Epoch number */
  int		num_commands;		/* Number of commands */
  command_t	*commands;		/* Commands */
  int		num_depends;		/* Number of dependencies */
  depend_t	*depends;		/* Dependencies */
  int		num_files;		/* Number of files */
  file_t	*files;			/* Files */
} dist_t;


/*
 * Globals...
 */

extern int		CompressFiles;	/* Compress package files? */
extern const char	*DataDir;	/* Directory for setup data files */
extern int		KeepFiles;	/* Keep intermediate files? */
extern const char	*SetupProgram;	/* Setup program */
extern const char	*SoftwareDir;	/* Software directory path */
extern const char	*UninstProgram;	/* Uninstall program */
extern int		Verbosity;	/* Be verbose? */


/*
 * Prototypes...
 */

extern void	add_command(dist_t *dist, FILE *fp, int type,
		            const char *command, const char *subpkg,
                            const char *section);
extern void	add_depend(dist_t *dist, int type, const char *line,
		           const char *subpkg);
extern void	add_description(dist_t *dist, FILE *fp, const char *description,
		                const char *subpkg);
extern file_t	*add_file(dist_t *dist, const char *subpkg);
extern char	*add_subpackage(dist_t *dist, const char *subpkg);
extern int	copy_file(const char *dst, const char *src,
		          mode_t mode, uid_t owner, gid_t group);
extern char	*find_subpackage(dist_t *dist, const char *subpkg);
extern void	free_dist(dist_t *dist);
extern const char *get_option(file_t *file, const char *name, const char *defval);
extern void	get_platform(struct utsname *platform);
extern const char *get_runlevels(file_t *file, const char *deflevels);
extern int	get_start(file_t *file, int defstart);
extern int	get_stop(file_t *file, int defstop);
extern int	get_vernumber(const char *version);
extern int	make_aix(const char *prodname, const char *directory,
		         const char *platname, dist_t *dist,
			 struct utsname *platform);
extern int	make_bsd(const char *prodname, const char *directory,
		         const char *platname, dist_t *dist,
			 struct utsname *platform);
extern int	make_deb(const char *prodname, const char *directory,
		         const char *platname, dist_t *dist,
			 struct utsname *platform);
extern int	make_directory(const char *directory, mode_t mode, uid_t owner,
		               gid_t group);
extern int	make_inst(const char *prodname, const char *directory,
		          const char *platname, dist_t *dist,
			  struct utsname *platform);
extern int	make_link(const char *dst, const char *src);
extern int	make_osx(const char *prodname, const char *directory,
		         const char *platname, dist_t *dist,
			 struct utsname *platform, const char *setup);
extern int	make_pkg(const char *prodname, const char *directory,
		         const char *platname, dist_t *dist,
			 struct utsname *platform);
extern int	make_portable(const char *prodname, const char *directory,
		              const char *platname, dist_t *dist,
			      struct utsname *platform, const char *setup,
			      const char *types);
extern int	make_rpm(int format, const char *prodname, const char *directory,
		         const char *platname, dist_t *dist,
			 struct utsname *platform, const char *setup,
			 const char *types);
extern int	make_setld(const char *prodname, const char *directory,
		           const char *platname, dist_t *dist,
			   struct utsname *platform);
extern int	make_slackware(const char *prodname, const char *directory,
		               const char *platname, dist_t *dist,
			       struct utsname *platform);
extern int	make_swinstall(const char *prodname, const char *directory,
		               const char *platname, dist_t *dist,
			       struct utsname *platform);
extern dist_t	*new_dist(void);
extern int	qprintf(FILE *fp, const char *format, ...);
extern dist_t	*read_dist(const char *filename, struct utsname *platform,
		           const char *format);
extern int	run_command(const char *directory, const char *command, ...)
#    ifdef __GNUC__
__attribute__ ((__format__ (__printf__, 2, 3)))
#    endif /* __GNUC__ */
;
extern void	sort_dist_files(dist_t *dist);
extern void	strip_execs(dist_t *dist);
extern int	tar_close(tarf_t *tar);
extern int	tar_directory(tarf_t *tar, const char *srcpath,
		              const char *dstpath);
extern int	tar_file(tarf_t *tar, const char *filename);
extern int	tar_header(tarf_t *tar, int type, mode_t mode, off_t size,
		           time_t mtime, const char *user, const char *group,
			   const char *pathname, const char *linkname);
extern tarf_t	*tar_open(const char *filename, int compress);
extern int	tar_package(tarf_t *tar, const char *ext,
		            const char *prodname, const char *directory,
		            const char *platname, dist_t *dist,
			    const char *subpackage);
extern int	unlink_directory(const char *directory);
extern int	unlink_package(const char *ext, const char *prodname,
		               const char *directory, const char *platname,
			       dist_t *dist, const char *subpackage);
extern int	write_dist(const char *listname, dist_t *dist);


#  ifdef __cplusplus
}
#  endif /* __cplusplus */

#endif /* !_EPM_H_ */


/*
 * End of "$Id: epm.h 833 2010-12-30 00:00:50Z mike $".
 */

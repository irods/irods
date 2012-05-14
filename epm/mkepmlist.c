/*
 * "$Id: mkepmlist.c 611 2005-01-11 21:37:42Z mike $"
 *
 *   List file generation utility for the ESP Package Manager (EPM).
 *
 *   Copyright 2003-2005 by Easy Software Products
 *   Copyright 2003 Andreas Voegele
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
 *   main()         - Scan directories to produce a distribution list.
 *   get_group()    - Get a group name for the given group ID.
 *   get_user()     - Get a user name for the given user ID.
 *   hash_deinit()  - Deinitialize a hash table.
 *   hash_id()      - Generate a hash for a user or group ID.
 *   hash_init()    - Initialize a hash table.
 *   hash_insert()  - Insert a new entry in a hash table.
 *   hash_search()  - Find a user or group ID in the hash table...
 *   info()         - Show the EPM copyright and license.
 *   process_dir()  - Process a directory...
 *   process_file() - Process a file...
 *   usage()        - Show command-line usage instructions.
 */

#include "epm.h"


/*
 * Lookup hash table structure...
 */

#define HASH_M 101

struct node
{
  unsigned	id;			/* User or group ID */
  char		*name;			/* User or group name */
};


/*
 * Globals...
 */

char		*DefaultUser = NULL,	/* Default user for entries */
		*DefaultGroup = NULL;	/* Default group for entries */
struct node	Users[HASH_M];		/* Hash table for users */
struct node	Groups[HASH_M];		/* Hash table for groups */


/*
 * Functions...
 */

char		*get_group(gid_t gid);
char		*get_user(uid_t uid);
void		hash_deinit(struct node *a);
unsigned	hash_id(unsigned id);
void		hash_init(struct node *a);
char		*hash_insert(struct node *a, unsigned id,
		             const char *name);
char		*hash_search(struct node *a, unsigned id);
void		info(void);
int		process_dir(const char *srcpath, const char *dstpath);
int		process_file(const char *src, const char *dstpath);
void		usage(void);


/*
 * 'main()' - Scan directories to produce a distribution list.
 */

int				/* O - Exit status */
main(int  argc,			/* I - Number of command-line arguments */
     char *argv[])		/* I - Command-line arguments */
{
  int		i;		/* Looping var */
  const char	*prefix,	/* Installation prefix */
		*dstpath;	/* Destination path  */
  char		dst[1024],	/* Destination */
		*ptr;		/* Pointer into filename */
  struct stat	info;		/* File information */


 /*
  * Initialize the hash tables...
  */

  hash_init(Users);
  hash_init(Groups);

 /*
  * Loop through the command-line arguments, processing directories as
  * needed...
  */

  prefix = NULL;

  for (i = 1; i < argc; i ++)
    if (strcmp(argv[i], "-u") == 0)
    {
     /*
      * -u username
      */

      i ++;

      if (i >= argc)
	usage();

      DefaultUser = argv[i];
    }
    else if (strcmp(argv[i], "-g") == 0)
    {
     /*
      * -g groupname
      */

      i ++;

      if (i >= argc)
	usage();

      DefaultGroup = argv[i];
    }
    else if (strcmp(argv[i], "--prefix") == 0)
    {
      i ++;

      if (i >= argc)
	usage();

      prefix = argv[i];
    }
    else if (argv[i][0] == '-')
    {
     /*
      * Unknown option...
      */

      fprintf(stderr, "Unknown option \"%s\"!\n", argv[i]);
      usage();
    }
    else
    {
     /*
      * Directory name...
      */

      if (prefix)
        dstpath = prefix;
      else
      {
        if (argv[i][0] == '/')
          dstpath = argv[i];
	else
	{
          if (argv[i][0] == '.')
	  {
	    for (dstpath = argv[i]; *dstpath == '.';)
	    {
	      if (strncmp(dstpath, "./", 2) == 0)
		dstpath += 2;
	      else if (strncmp(dstpath, "../", 3) == 0)
		dstpath += 3;
	      else
		break;
	    }

	    if (strcmp(dstpath, ".") == 0)
	      dstpath = "";
	  }
	  else
	    dstpath = argv[i];

          if (dstpath[0])
	  {
            snprintf(dst, sizeof(dst), "/%s", dstpath);
	    dstpath = dst;
	  }
	}
      }

      if (!lstat(argv[i], &info))
      {
	if (!S_ISDIR(info.st_mode))
	{
	  if (!prefix)
	  {
	   /*
	    * Remove trailing filename component from destination...
	    */

	    if (dstpath != dst)
	    {
	      strlcpy(dst, dstpath, sizeof(dst));
	      dstpath = dst;
	    }

	    if ((ptr = strrchr(dstpath, '/')) != NULL)
	      *ptr = '\0';
          }

          process_file(argv[i], dstpath);
	}
	else
	  process_dir(argv[i], dstpath);
      }
    }

 /*
  * Free any memory we have left allocated...
  */

  hash_deinit(Users);
  hash_deinit(Groups);
  
  return (0);
}


/*
 * 'get_group()' - Get a group name for the given group ID.
 */

char *				/* O - Name of group or NULL */
get_group(gid_t gid)		/* I - Group ID */
{
  char		*name;		/* Name of group */
  struct group	*gr;		/* Group entry for group */
  char		buf[16];	/* Temporary string buffer for group numbers */


 /*
  * Always return the default group if set...
  */

  if (DefaultGroup)
    return (DefaultGroup);
  
 /*
  * See if the group ID is already in the hash table...
  */

  if ((name = hash_search(Groups, gid)) != NULL)
    return (name);

 /*
  * Lookup the group ID in the password file...
  */

  setgrent();

  if ((gr = getgrgid(gid)) != NULL)
    name = gr->gr_name;
  else
  {
   /*
    * Unable to find group ID; just return the number...
    */

    sprintf(buf, "%u", (unsigned)gid);
    name = buf;
  }

 /*
  * Add the name to the hash table and return it...
  */

  return (hash_insert(Groups, gid, name));
}


/*
 * 'get_user()' - Get a user name for the given user ID.
 */

char *				/* O - Name of user or NULL */
get_user(uid_t uid)		/* I - User ID */
{
  char		*name;		/* Name of user */
  struct passwd	*pw;		/* Password entry for user */
  char		buf[16];	/* Temporary string buffer for user numbers */


 /*
  * Always return the default user if set...
  */

  if (DefaultUser)
    return (DefaultUser);
  
 /*
  * See if the user ID is already in the hash table...
  */

  if ((name = hash_search(Users, uid)) != NULL)
    return (name);

 /*
  * Lookup the user ID in the password file...
  */

  setpwent();

  if ((pw = getpwuid(uid)) != NULL)
    name = pw->pw_name;
  else
  {
   /*
    * Unable to find user ID; just return the number...
    */

    sprintf(buf, "%u", (unsigned)uid);
    name = buf;
  }

 /*
  * Add the name to the hash table and return it...
  */

  return (hash_insert(Users, uid, name));
}


/*
 * 'hash_deinit()' - Deinitialize a hash table.
 */

void
hash_deinit(struct node *a)	/* I - Hash table */
{
  unsigned	h;		/* Looping var */


  for (h = 0; h < HASH_M; h ++)
    if (a[h].name)
    {
      free(a[h].name);
      memset(a, 0, sizeof(struct node));
    }
}


/*
 * 'hash_id()' - Generate a hash for a user or group ID.
 */

unsigned			/* O - Hash number */
hash_id(unsigned id)		/* I - User or group ID */
{
  return (id % HASH_M);
}


/*
 * 'hash_init()' - Initialize a hash table.
 */

void
hash_init(struct node *a)	/* I - Hash table to initialize */
{
 /*
  * Zero out the hash table...
  */

  memset(a, 0, HASH_M * sizeof(struct node));
}


/*
 * 'hash_insert()' - Insert a new entry in a hash table.
 */

char *				/* O - New user or group name or NULL */
hash_insert(struct node *a,	/* I - Hash table */
            unsigned    id,	/* I - User or group ID */
            const char  *name)	/* I - User or group name */
{
  unsigned	h,		/* Current hash value */
		start;		/* Starting hash value */


  for (h = start = hash_id(id); a[h].name;)
  {
    h = (h + 1) % HASH_M;
    if (h == start)
      return (NULL);
  }

  a[h].id   = id;
  a[h].name = strdup(name);

  return (a[h].name);
}


/*
 * 'hash_search()' - Find a user or group ID in the hash table...
 */

char *				/* O - User or group name, or NULL */
hash_search(struct node *a,	/* I - Hash table */
            unsigned    id)	/* I - User or group ID */
{
  unsigned	h,		/* Current hash value */
		start;		/* Starting hash value */


  for (h = start = hash_id(id); a[h].name;)
  {
    h = (h + 1) % HASH_M;

    if (a[h].id == id)
      return (a[h].name);

    if (h == start)
      break;
  }

  return (NULL);
}


/*
 * 'info()' - Show the EPM copyright and license.
 */

void
info(void)
{
  puts(EPM_VERSION);
  puts("Copyright 1999-2005 by Easy Software Products.");
  puts("");
  puts("EPM is free software and comes with ABSOLUTELY NO WARRANTY; for details");
  puts("see the GNU General Public License in the file COPYING or at");
  puts("\"http://www.fsf.org/gpl.html\".  Report all problems to \"epm@easysw.com\".");
  puts("");
}


/*
 * 'process_dir()' - Process a directory...
 */

int				/* O - 0 on success, -1 on error */
process_dir(const char *srcpath,/* I - Source path */
            const char *dstpath)/* I - Destination path */
{
  DIR		*dir;		/* Directory to read from */
  struct dirent *dent;		/* Current directory entry */
  int		srclen;		/* Length of source path */
  char		src[1024];	/* Temporary source path */


 /*
  * Try opening the source directory...
  */

  if ((dir = opendir(srcpath)) == NULL)
  {
    fprintf(stderr, "mkepmlist: Unable to open directory \"%s\" - %s.\n",
            srcpath, strerror(errno));

    return (-1);
  }

 /*
  * Read from the directory...
  */

  srclen = strlen(srcpath);

  while ((dent = readdir(dir)) != NULL)
  {
   /*
    * Skip "." and ".."...
    */

    if (strcmp(dent->d_name, ".") == 0 ||
        strcmp(dent->d_name, "..") == 0)
      continue;

   /*
    * Process file...
    */

    if (srclen > 0 && srcpath[srclen - 1] == '/')
      snprintf(src, sizeof(src), "%s%s", srcpath, dent->d_name);
    else
      snprintf(src, sizeof(src), "%s/%s", srcpath, dent->d_name);

    if (process_file(src, dstpath))
    {
      closedir(dir);
      return (-1);
    }
  }

  closedir(dir);
  return (0);
}


/*
 * 'process_file()' - Process a file...
 */

int				/* O - 0 on success, -1 on error */
process_file(const char *src,	/* I - Source path */
             const char *dstpath)/* I - Destination path */
{
  const char	*srcptr;	/* Pointer into source path */
  struct stat	srcinfo;	/* Information on the source file */
  int		linklen,	/* Length of link path */
		dstlen;		/* Length of destination path */
  char		link[1024],	/* Link for source */
		dst[1024];	/* Temporary destination path */


 /*
  * Get source file info...
  */

  dstlen = strlen(dstpath);

  if ((srcptr = strrchr(src, '/')) != NULL)
    srcptr ++;
  else
    srcptr = src;

  if (dstlen > 0 && dstpath[dstlen - 1] == '/')
    snprintf(dst, sizeof(dst), "%s%s", dstpath, srcptr);
  else
    snprintf(dst, sizeof(dst), "%s/%s", dstpath, srcptr);

  if (lstat(src, &srcinfo))
  {
    fprintf(stderr, "mkepmlist: Unable to stat \"%s\" - %s.\n", src,
            strerror(errno));
    return (-1);
  }

 /*
  * Process accordingly...
  */

  if (S_ISDIR(srcinfo.st_mode))
  {
   /*
    * Directory...
    */

    qprintf(stdout, "d %o %s %s %s -\n", (unsigned)(srcinfo.st_mode & 07777),
	    get_user(srcinfo.st_uid), get_group(srcinfo.st_gid), dst);

    if (process_dir(src, dst))
      return (-1);
  }
  else if (S_ISLNK(srcinfo.st_mode))
  {
   /*
    * Symlink...
    */

    if ((linklen = readlink(src, link, sizeof(link) - 1)) < 0)
    {
      fprintf(stderr, "mkepmlist: Unable to read symlink \"%s\" - %s.\n",
	      src, strerror(errno));
      return (-1);
    }

    link[linklen] = '\0';

    qprintf(stdout, "l %o %s %s %s %s\n",
            (unsigned)(srcinfo.st_mode & 07777), get_user(srcinfo.st_uid),
	    get_group(srcinfo.st_gid), dst, link);
  }
  else if (S_ISREG(srcinfo.st_mode))
  {
   /*
    * Regular file...
    */

    qprintf(stdout, "f %o %s %s %s %s\n",
            (unsigned)(srcinfo.st_mode & 07777), get_user(srcinfo.st_uid),
	    get_group(srcinfo.st_gid), dst, src);
  }

  return (0);
}


/*
 * 'usage()' - Show command-line usage instructions.
 */

void
usage(void)
{
  info();

  puts("Usage: mkepmlist [options] directory [... directory] >filename.list");
  puts("Options:");
  puts("-g group              Set group name for files.");
  puts("-u user               Set user name for files.");
  puts("--prefix directory    Set directory prefix for files.");

  exit(1);
}


/*
 * End of "$Id: mkepmlist.c 611 2005-01-11 21:37:42Z mike $".
 */

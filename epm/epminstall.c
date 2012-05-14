/*
 * "$Id: epminstall.c 827 2010-12-29 17:32:40Z mike $"
 *
 *   Install program replacement for the ESP Package Manager (EPM).
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
 *
 * Contents:
 *
 *   main()       - Add or replace files, directories, and symlinks.
 *   find_file()  - Find a file in the distribution...
 *   info()       - Show the EPM copyright and license.
 *   usage()      - Show command-line usage instructions.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * Global variable used by dist functions...
 */

int		Verbosity = 0;


/*
 * Local functions...
 */

static file_t	*find_file(dist_t *dist, const char *dst);
static void	info(void);
static void	usage(void)
#ifdef __GNUC__
__attribute__((__noreturn__))
#endif /* __GNUC__ */
;


/*
 * 'main()' - Add or replace files, directories, and symlinks.
 */

int				/* O - Exit status */
main(int  argc,			/* I - Number of command-line arguments */
     char *argv[])		/* I - Command-line arguments */
{
  int		i;		/* Looping var */
  int		mode,		/* Permissions */
		directories;	/* Installing directories? */
  char		*user,		/* Owner */
		*group,		/* Group */
		*listname,	/* List filename */
		*src,		/* Source filename */
		dst[1024],	/* Destination filename */
		linkname[1024];	/* Symlink name */
  int		linklen;	/* Length of symlink */
  int		num_files;	/* Number of files to install */
  char		*files[1000];	/* Files to install */
  struct stat	fileinfo;	/* File information */
  dist_t	*dist;		/* Distribution */
  file_t	*file;		/* File in distribution */
  struct utsname platform;	/* Platform information */


 /*
  * Parse the command-line arguments...
  */

  num_files   = 0;
  mode        = 0;
  user        = "root";
  group       = "sys";
  directories = 0;

  if ((listname = getenv("EPMLIST")) == NULL)
    listname = "epm.list";

  for (i = 1; i < argc; i ++)
    if (strcmp(argv[i], "-b") == 0)
      continue;
    else if (strcmp(argv[i], "-c") == 0)
      continue;
    else if (strcmp(argv[i], "-d") == 0)
      directories = 1;
    else if (strcmp(argv[i], "-g") == 0)
    {
      i ++;
      if (i < argc)
	group = argv[i];
      else
	usage();
    }
    else if (strcmp(argv[i], "-m") == 0)
    {
      i ++;
      if (i < argc)
	mode = strtol(argv[i], NULL, 8);
      else
	usage();
    }
    else if (strcmp(argv[i], "-o") == 0)
    {
      i ++;
      if (i < argc)
	user = argv[i];
      else
	usage();
    }
    else if (strcmp(argv[i], "-s") == 0)
      continue;
    else if (strcmp(argv[i], "--list-file") == 0)
    {
      i ++;
      if (i < argc)
	listname = argv[i];
      else
	usage();
    }
    else if (argv[i][0] == '-')
      usage();
    else if (num_files < (sizeof(files) / sizeof(files[0])))
    {
      files[num_files] = argv[i];
      num_files ++;
    }
    else
    {
      fputs("epminstall: Too many filenames on command-line!\n", stderr);
      usage();
    }

  if (num_files == 0 || (num_files < 2 && !directories))
    usage();

  get_platform(&platform);

  if ((dist = read_dist(listname, &platform, "")) == NULL)
  {
    fprintf(stderr, "epminstall: Unable to read list file \"%s\": %s\n",
            listname, strerror(errno));
    return (1);
  }

 /*
  * Check to see if we are installing files or directories...
  */

  if (directories)
  {
    if (!mode)
      mode = 0755;

    for (i = 0; i < num_files; i ++)
    {
     /*
      * Copy the directory name into a new file entry, which is cleared
      * by add_file()...
      */

      if ((file = find_file(dist, files[i])) == NULL)
        file = add_file(dist, NULL);

      file->type = 'd';
      file->mode = mode & 07777;
      strlcpy(file->user, user, sizeof(file->user));
      strlcpy(file->group, group, sizeof(file->group));
      strlcpy(file->dst, files[i], sizeof(file->dst));
      strcpy(file->src, "-");
    }
  }
  else
  {
   /*
    * Check to see if we have 1 or more files and a directory or
    * a source and destination file...
    */

    if (num_files == 2)
    {
      file = find_file(dist, files[1]);

      if (file == NULL || file->type != 'd')
      {
        if (!file)
	  file = add_file(dist, NULL);

        if (stat(files[0], &fileinfo))
	{
	  fprintf(stderr, "epminstall: Unable to stat \"%s\": %s\n",
	          files[0], strerror(errno));
          fileinfo.st_mode = mode;
        }

        if (S_ISLNK(fileinfo.st_mode))
	{
	  file->type = 'l';

	  if ((linklen = readlink(files[0], linkname, sizeof(linkname) - 1)) < 0)
	  {
	    fprintf(stderr, "epminstall: Unable to read symlink \"%s\": %s\n",
	            files[0], strerror(errno));
            files[0] = "BROKEN-LINK";
	  }
	  else
	  {
	    linkname[linklen] = '\0';
	    files[0]          = linkname;
	  }
	}
	else
	  file->type = 'f';

        if (mode)
	  file->mode = mode & 07777;
	else if (fileinfo.st_mode & 0111)
	  file->mode = 0755;
	else
	  file->mode = 0644;

	strlcpy(file->user, user, sizeof(file->user));
	strlcpy(file->group, group, sizeof(file->group));
	strlcpy(file->dst, files[1], sizeof(file->dst));
	strlcpy(file->src, files[0], sizeof(file->src));
      }
      else
        num_files --;
    }

    if (num_files != 2)
    {
      if (num_files > 1)
        num_files --;

      if ((file = find_file(dist, files[num_files])) == NULL)
      {
       /*
        * Add the installation directory to the file list...
	*/

	file = add_file(dist, NULL);

	file->type = 'd';
	file->mode = 0755;
	strlcpy(file->user, user, sizeof(file->user));
	strlcpy(file->group, group, sizeof(file->group));
	strlcpy(file->dst, files[num_files], sizeof(file->dst));
	strcpy(file->src, "-");
      }
      else if (file->type != 'd')
      {
        fprintf(stderr, "epminstall: Destination path \"%s\" is not a directory!\n",
	        files[num_files]);
        return (1);
      }

      for (i = 0; i < num_files; i ++)
      {
       /*
	* Add/update the file in the distribution...
	*/

        if ((src = strrchr(files[i], '/')) != NULL)
	  src ++;
	else
	  src = files[i];

        snprintf(dst, sizeof(dst), "%s/%s", files[num_files], src);

	if ((file = find_file(dist, dst)) == NULL)
          file = add_file(dist, NULL);

        if (stat(files[i], &fileinfo))
	{
	  fprintf(stderr, "epminstall: Unable to stat \"%s\": %s\n",
	          files[i], strerror(errno));
          fileinfo.st_mode = mode;
        }

        if (S_ISLNK(fileinfo.st_mode))
	{
	  file->type = 'l';

	  if ((linklen = readlink(files[i], linkname, sizeof(linkname) - 1)) < 0)
	  {
	    fprintf(stderr, "epminstall: Unable to read symlink \"%s\": %s\n",
	            files[i], strerror(errno));
            files[i] = "BROKEN-LINK";
	  }
	  else
	  {
	    linkname[linklen] = '\0';
	    files[i]          = linkname;
	  }
	}
	else
	  file->type = 'f';

        if (mode)
	  file->mode = mode & 07777;
	else if (fileinfo.st_mode & 0111)
	  file->mode = 0755;
	else
	  file->mode = 0644;

	strlcpy(file->user, user, sizeof(file->user));
	strlcpy(file->group, group, sizeof(file->group));
	strlcpy(file->dst, dst, sizeof(file->dst));
	strlcpy(file->src, files[i], sizeof(file->src));
      }
    }
  }

 /*
  * Sort the files to make the final list file easier to check...
  */

  sort_dist_files(dist);

 /*
  * Write the distribution file...
  */

  if (write_dist(listname, dist))
  {
    fprintf(stderr, "epminstall: Unable to read list file \"%s\": %s\n",
            listname, strerror(errno));
    return (1);
  }

 /*
  * Return with no errors...
  */

  return (0);
}


/*
 * 'find_file()' - Find a file in the distribution...
 */

static file_t *			/* O - File entry or NULL */
find_file(dist_t     *dist,	/* I - Distribution to search */
          const char *dst)	/* I - Destination filename */
{
  int		i;		/* Looping var */
  file_t	*file;		/* Current file */


  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (strcmp(file->dst, dst) == 0)
      return (file);

  return (NULL);
}


/*
 * 'info()' - Show the EPM copyright and license.
 */

static void
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
 * 'usage()' - Show command-line usage instructions.
 */

static void
usage(void)
{
  info();

  puts("Usage: epminstall [options] file1 file2 ... fileN directory");
  puts("       epminstall [options] file1 file2");
  puts("       epminstall [options] -d directory1 directory2 ... directoryN");
  puts("Options:");
  puts("-g group");
  puts("    Set group of installed file(s).");
  puts("-m mode");
  puts("    Set permissions of installed file(s).");
  puts("-u owner");
  puts("    Set owner of installed file(s).");

  exit(1);
}


/*
 * End of "$Id: epminstall.c 827 2010-12-29 17:32:40Z mike $".
 */

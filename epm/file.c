/*
 * "$Id: file.c 841 2010-12-30 20:34:57Z mike $"
 *
 *   File functions for the ESP Package Manager (EPM).
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
 *   copy_file()        - Copy a file.
 *   make_directory()   - Make a directory.
 *   make_link()        - Make a symbolic link.
 *   strip_execs()      - Strip symbols from executable files in the
 *                        distribution.
 *   unlink_directory() - Delete a directory and all of its nodes.
 *   unlink_package()   - Delete a package file.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * 'copy_file()' - Copy a file.
 */

int					/* O - 0 on success, -1 on failure */
copy_file(const char *dst,		/* I - Destination file */
          const char *src,		/* I - Source file */
          mode_t     mode,		/* I - Permissions */
	  uid_t      owner,		/* I - Owner ID */
	  gid_t      group)		/* I - Group ID */
{
  FILE		*dstfile,		/* Destination file */
		*srcfile;		/* Source file */
  char		buffer[8192];		/* Copy buffer */
  char		*slash;			/* Pointer to trailing slash */
  size_t	bytes;			/* Number of bytes read/written */
  struct stat	srcinfo;		/* Source file information */


 /*
  * Check that the destination directory exists...
  */

  strcpy(buffer, dst);
  if ((slash = strrchr(buffer, '/')) != NULL)
    *slash = '\0';

  if (access(buffer, F_OK))
    make_directory(buffer, 0755, owner, group);

 /*
  * Try doing a hard link instead of a copy...
  */

  unlink(dst);

  if (!stat(src, &srcinfo) &&
      (!mode || srcinfo.st_mode == mode) &&
      (owner == (uid_t)-1 || srcinfo.st_uid == owner) &&
      (group == (gid_t)-1 || srcinfo.st_gid == group) &&
      !link(src, dst))
    return (0);

 /*
  * Open files...
  */

  if ((dstfile = fopen(dst, "wb")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create \"%s\" -\n     %s\n", dst,
            strerror(errno));
    return (-1);
  }

  if ((srcfile = fopen(src, "rb")) == NULL)
  {
    fclose(dstfile);
    unlink(dst);

    fprintf(stderr, "epm: Unable to open \"%s\" -\n     %s\n", src,
            strerror(errno));
    return (-1);
  }

 /*
  * Copy from src to dst...
  */

  while ((bytes = fread(buffer, 1, sizeof(buffer), srcfile)) > 0)
    if (fwrite(buffer, 1, bytes, dstfile) < bytes)
    {
      fprintf(stderr, "epm: Unable to write to \"%s\" -\n     %s\n", dst,
              strerror(errno));

      fclose(srcfile);
      fclose(dstfile);
      unlink(dst);

      return (-1);
    }

 /*
  * Close files, change permissions, and return...
  */

  fclose(srcfile);
  fclose(dstfile);

  if (mode)
    chmod(dst, mode);
  if (owner != (uid_t)-1 && group != (gid_t)-1)
    chown(dst, owner, group);

  return (0);
}


/*
 * 'make_directory()' - Make a directory.
 */

int					/* O - 0 = success, -1 = error */
make_directory(const char *directory,	/* I - Directory */
               mode_t     mode,		/* I - Permissions */
	       uid_t      owner,	/* I - Owner ID */
	       gid_t      group)	/* I - Group ID */
{
  char	buffer[8192],			/* Filename buffer */
	*bufptr;			/* Pointer into buffer */


  for (bufptr = buffer; *directory;)
  {
    if (*directory == '/' && bufptr > buffer)
    {
      *bufptr = '\0';

      if (access(buffer, F_OK))
      {
	mkdir(buffer, 0755);
	if (mode)
          chmod(buffer, mode | 0700);
	if (owner != (uid_t)-1 && group != (gid_t)-1)
	  chown(buffer, owner, group);
      }
    }

    *bufptr++ = *directory++;
  }

  *bufptr = '\0';

  if (access(buffer, F_OK))
  {
    mkdir(buffer, 0755);
    if (mode)
      chmod(buffer, mode | 0700);
    if (owner != (uid_t)-1 && group != (gid_t)-1)
      chown(buffer, owner, group);
  }

  return (0);
}


/*
 * 'make_link()' - Make a symbolic link.
 */

int					/* O - 0 = success, -1 = error */
make_link(const char *dst,		/* I - Destination file */
          const char *src)		/* I - Link */
{
  char	buffer[8192],			/* Copy buffer */
	*slash;				/* Pointer to trailing slash */


 /*
  * Check that the destination directory exists...
  */

  strcpy(buffer, dst);
  if ((slash = strrchr(buffer, '/')) != NULL)
    *slash = '\0';

  if (access(buffer, F_OK))
    make_directory(buffer, 0755, 0, 0);

 /* 
  * Make the symlink...
  */

  return (symlink(src, dst));
}


/*
 * 'strip_execs()' - Strip symbols from executable files in the distribution.
 */

void
strip_execs(dist_t *dist)		/* I - Distribution to strip... */
{
  int		i;			/* Looping var */
  file_t	*file;			/* Software file */
  FILE		*fp;			/* File pointer */
  char		header[4];		/* File header... */


 /*
  * Loop through the distribution files and strip any executable
  * files.
  */

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'f' && (file->mode & 0111) &&
        strstr(file->options, "nostrip()") == NULL)
    {
     /*
      * OK, this file has executable permissions; see if it is a
      * script...
      */

      if ((fp = fopen(file->src, "rb")) != NULL)
      {
       /*
        * Read the first 3 bytes of the file...
	*/

        if (fread(header, 1, sizeof(header) - 1, fp) == 0)
	  continue;

	header[sizeof(header) - 1] = '\0';

	fclose(fp);

       /*
        * Check for "#!/" or "#" + whitespace at the beginning of the file...
	*/

        if (!strncmp(header, "#!/", 3) ||
	    (header[0] == '#' && isspace(header[1] & 255)))
	  continue;
      }
      else
      {
       /*
        * File could not be opened; error out...
	*/

        fprintf(stderr, "epm: Unable to open file \"%s\" for destination "
	                "\"%s\" -\n     %s\n",
	        file->src, file->dst, strerror(errno));

        exit(1);
      }

     /*
      * Strip executables...
      */

      run_command(NULL, EPM_STRIP " %s", file->src);
    }
}


/*
 * 'unlink_directory()' - Delete a directory and all of its nodes.
 */

int					/* O - 0 on success, -1 on failure */
unlink_directory(const char *directory)	/* I - Directory */
{
  DIR		*dir;			/* Directory */
  DIRENT	*dent;			/* Directory entry */
  char		filename[1024];		/* Filename */
  struct stat	fileinfo;		/* Information on the source file */


 /*
  * Try opening the source directory...
  */

  if ((dir = opendir(directory)) == NULL)
  {
    fprintf(stderr, "epm: Unable to open directory \"%s\": %s\n", directory,
            strerror(errno));

    return (-1);
  }

 /*
  * Read from the directory...
  */

  while ((dent = readdir(dir)) != NULL)
  {
   /*
    * Skip "." and ".."...
    */

    if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
      continue;

   /*
    * Get file info...
    */

    snprintf(filename, sizeof(filename), "%s/%s", directory, dent->d_name);

    if (stat(filename, &fileinfo))
    {
      fprintf(stderr, "epm: Unable to stat \"%s\": %s\n", filename,
              strerror(errno));
      continue;
    }

   /*
    * Process accordingly...
    */

    if (Verbosity)
      puts(filename);

    if (S_ISDIR(fileinfo.st_mode))
    {
     /*
      * Directory...
      */

      if (unlink_directory(filename))
	goto fail;

      if (rmdir(filename))
      {
        fprintf(stderr, "epm: Unable to remove \"%s\": %s\n", filename,
	        strerror(errno));
	goto fail;
      }
    }
    else
    {
     /*
      * Regular file or symlink...
      */

      if (unlink(filename))
      {
        fprintf(stderr, "epm: Unable to remove \"%s\": %s\n", filename,
	        strerror(errno));
	goto fail;
      }
    }
  }

  closedir(dir);
  return (0);

  fail:

  closedir(dir);
  return (-1);
}


/*
 * 'unlink_package()' - Delete a package file.
 */

int					/* O - 0 on success, -1 on failure */
unlink_package(const char *ext,		/* I - Package filename extension */
               const char *prodname,	/* I - Product short name */
	       const char *directory,	/* I - Directory for distribution files */
               const char *platname,	/* I - Platform name */
               dist_t     *dist,	/* I - Distribution information */
	       const char *subpackage)	/* I - Subpackage */
{
  char		prodfull[255],		/* Full name of product */
		name[1024],		/* Full product name */
		filename[1024];		/* File to archive */


 /*
  * Figure out the full name of the distribution...
  */

  if (subpackage)
    snprintf(prodfull, sizeof(prodfull), "%s-%s", prodname, subpackage);
  else
    strlcpy(prodfull, prodname, sizeof(prodfull));

 /*
  * Then the subdirectory name...
  */

  if (dist->release[0])
    snprintf(name, sizeof(name), "%s-%s-%s", prodfull, dist->version,
             dist->release);
  else
    snprintf(name, sizeof(name), "%s-%s", prodfull, dist->version);

  if (platname[0])
  {
    strlcat(name, "-", sizeof(name));
    strlcat(name, platname, sizeof(name));
  }

  strlcat(name, ".", sizeof(name));
  strlcat(name, ext, sizeof(name));

 /*
  * Remove the package file...
  */

  snprintf(filename, sizeof(filename), "%s/%s", directory, name);

  if (unlink(filename))
  {
    fprintf(stderr, "epm: Unable to remove \"%s\" - %s\n", filename,
            strerror(errno));
    return (-1);
  }
  else
    return (0);
}


/*
 * End of "$Id: file.c 841 2010-12-30 20:34:57Z mike $".
 */

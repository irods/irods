/*
 * "$Id: tar.c 837 2010-12-30 00:25:29Z mike $"
 *
 *   TAR file functions for the ESP Package Manager (EPM).
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
 *   tar_close()     - Close a tar file, padding as needed.
 *   tar_directory() - Archive a directory.
 *   tar_file()      - Write the contents of a file...
 *   tar_header()    - Write a TAR header for the specified file...
 *   tar_open()      - Open a TAR file for writing.
 *   tar_package()   - Archive a package file.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * 'tar_close()' - Close a tar file, padding as needed.
 */

int					/* O - -1 on error, 0 on success */
tar_close(tarf_t *fp)			/* I - File to write to */
{
  size_t	blocks;			/* Number of blocks to write */
  int		status;			/* Return status */
  char		padding[TAR_BLOCKS * TAR_BLOCK];
					/* Padding for tar blocks */


  if (fp->blocks > 0)
  {
   /*
    * Zero the padding record...
    */

    memset(padding, 0, sizeof(padding));

   /*
    * Write a single 0 block to signal the end of the archive...
    */

    if (fwrite(padding, TAR_BLOCK, 1, fp->file) < 1)
      return (-1);

    fp->blocks ++;

   /*
    * Pad the tar files to TAR_BLOCKS blocks...  This is bullshit of course,
    * but old versions of tar need it...
    */

    status = 0;

    if ((blocks = fp->blocks % TAR_BLOCKS) > 0)
    {
      blocks = TAR_BLOCKS - blocks;

      if (fwrite(padding, TAR_BLOCK, blocks, fp->file) < blocks)
	status = -1;
    }
    else
    {
     /*
      * Sun tar needs at least 2 0 blocks...
      */

      if (fwrite(padding, TAR_BLOCK, blocks, fp->file) < blocks)
	status = -1;
    }
  }
  else
    status = 0;

 /*
  * Close the file and free memory...
  */

  if (fp->compressed)
  {
    if (pclose(fp->file))
      status = -1;
  }
  else if (fclose(fp->file))
    status = -1;

  free(fp);

  return (status);
}


/*
 * 'tar_directory()' - Archive a directory.
 */

int					/* O - 0 on success, -1 on error */
tar_directory(tarf_t     *tar,		/* I - Tar file to write to */
              const char *srcpath,	/* I - Source directory */
              const char *dstpath)	/* I - Destination directory */
{
  DIR		*dir;			/* Directory */
  DIRENT	*dent;			/* Directory entry */
  char		src[1024],		/* Source file */
		dst[1024],		/* Destination file */
		srclink[1024];		/* Symlink value */
  struct stat	srcinfo;		/* Information on the source file */


 /*
  * Range check input...
  */

  if (tar == NULL || srcpath == NULL || dstpath == NULL)
    return (-1);

 /*
  * Try opening the source directory...
  */

  if ((dir = opendir(srcpath)) == NULL)
  {
    fprintf(stderr, "epm: Unable to open directory \"%s\": %s\n", srcpath,
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
    * Get source file info...
    */

    snprintf(src, sizeof(src), "%s/%s", srcpath, dent->d_name);
    if (dstpath[0])
      snprintf(dst, sizeof(dst), "%s/%s", dstpath, dent->d_name);
    else
      strlcpy(dst, dent->d_name, sizeof(dst));

    if (stat(src, &srcinfo))
    {
      fprintf(stderr, "epm: Unable to stat \"%s\": %s\n", src,
              strerror(errno));
      continue;
    }

   /*
    * Process accordingly...
    */

    if (Verbosity)
      puts(dst);

    if (S_ISDIR(srcinfo.st_mode))
    {
     /*
      * Directory...
      */

      if (tar_header(tar, TAR_DIR, srcinfo.st_mode, 0,
                     srcinfo.st_mtime, "root", "sys", dst, NULL))
        goto fail;

      if (tar_directory(tar, src, dst))
        goto fail;
    }
    else if (S_ISLNK(srcinfo.st_mode))
    {
     /*
      * Symlink...
      */

      if (readlink(src, srclink, sizeof(srclink)) < 0)
        goto fail;

      if (tar_header(tar, TAR_SYMLINK, srcinfo.st_mode, 0,
                     srcinfo.st_mtime, "root", "sys", dst, srclink))
        goto fail;
    }
    else
    {
     /*
      * Regular file...
      */

      if (tar_header(tar, TAR_NORMAL, srcinfo.st_mode, srcinfo.st_size,
                     srcinfo.st_mtime, "root", "sys", dst, NULL))
        goto fail;

      if (tar_file(tar, src))
        goto fail;
    }
  }

  closedir(dir);
  return (0);

  fail:

  closedir(dir);
  return (-1);
}


/*
 * 'tar_file()' - Write the contents of a file...
 */

int					/* O - 0 on success, -1 on error */
tar_file(tarf_t     *fp,		/* I - Tar file to write to */
         const char *filename)		/* I - File to write */
{
  FILE		*file;			/* File to write */
  size_t	nbytes,			/* Number of bytes read */
		tbytes,			/* Total bytes read/written */
		fill;			/* Number of fill bytes needed */
  char		buffer[8192];		/* Copy buffer */


 /*
  * Try opening the file...
  */

  if ((file = fopen(filename, "rb")) == NULL)
    return (-1);

 /*
  * Copy the file to the tar file...
  */

  tbytes = 0;

  while ((nbytes = fread(buffer, 1, sizeof(buffer), file)) > 0)
  {
   /*
    * Zero fill the file to a 512 byte record as needed.
    */

    if (nbytes < sizeof(buffer))
    {
      fill = TAR_BLOCK - (nbytes & (TAR_BLOCK - 1));

      if (fill < TAR_BLOCK)
      {
        memset(buffer + nbytes, 0, fill);
        nbytes += fill;
      }
    }

   /*
    * Write the buffer to the file...
    */

    if (fwrite(buffer, 1, nbytes, fp->file) < nbytes)
    {
      fclose(file);
      return (-1);
    }

    tbytes     += nbytes;
    fp->blocks += nbytes / TAR_BLOCK;
  }

 /*
  * Close the file and return...
  */

  fclose(file);
  return (0);
}


/*
 * 'tar_header()' - Write a TAR header for the specified file...
 */

int					/* O - 0 on success, -1 on error */
tar_header(tarf_t     *fp,		/* I - Tar file to write to */
           int        type,		/* I - File type */
	   mode_t     mode,		/* I - File permissions */
	   off_t      size,		/* I - File size */
           time_t     mtime,		/* I - File modification time */
	   const char *user,		/* I - File owner */
	   const char *group,		/* I - File group */
	   const char *pathname,	/* I - File name */
	   const char *linkname)	/* I - File link name (for links only) */
{
  tar_t		record;			/* TAR header record */
  int		pathlen;		/* Length of pathname */
  const char	*pathsep;		/* Path separator */
  int		i,			/* Looping var... */
		sum;			/* Checksum */
  unsigned char	*sumptr;		/* Pointer into header record */
  struct passwd	*pwd;			/* Pointer to user record */
  struct group	*grp;			/* Pointer to group record */


 /*
  * Find the username and groupname IDs...
  */

  pwd = getpwnam(user);
  grp = getgrnam(group);

  endpwent();
  endgrent();

 /*
  * Format the header...
  */

  memset(&record, 0, sizeof(record));

  pathlen = strlen(pathname);

  if ((pathlen < (sizeof(record.header.pathname) - 1) && type != TAR_DIR) ||
      (pathlen < (sizeof(record.header.pathname) - 2) && type == TAR_DIR))
  {
   /*
    * Pathname is short enough to fit in the initial pathname field...
    */

    strcpy(record.header.pathname, pathname);
    if (type == TAR_DIR && pathname[pathlen - 1] != '/')
      record.header.pathname[pathlen] = '/';
  }
  else
  {
   /*
    * Copy directory information to prefix buffer and filename information
    * to pathname buffer.
    */

    if ((pathsep = strrchr(pathname, '/')) == NULL)
    {
     /*
      * No directory info...
      */

      fprintf(stderr, "epm: Pathname \"%s\" is too long for a tar file!\n",
              pathname);
      return (-1);
    }

    if ((pathsep - pathname) > (sizeof(record.header.prefix) - 1))
    {
     /*
      * Directory name too long...
      */

      fprintf(stderr, "epm: Pathname \"%s\" is too long for a tar file!\n",
              pathname);
      return (-1);
    }

    if ((pathlen - (pathsep - pathname)) > (sizeof(record.header.pathname) - 1))
    {
     /*
      * Filename too long...
      */

      fprintf(stderr, "epm: Pathname \"%s\" is too long for a tar file!\n",
              pathname);
      return (-1);
    }

    strcpy(record.header.pathname, pathsep + 1);
    if (type == TAR_DIR && pathname[pathlen - 1] != '/')
      record.header.pathname[pathlen - (pathsep - pathname + 1)] = '/';

    strlcpy(record.header.prefix, pathname, (size_t)(pathsep - pathname + 1));
  }

  sprintf(record.header.mode, "%-6o ", (unsigned)mode);
  sprintf(record.header.uid, "%o ", pwd == NULL ? 0 : (unsigned)pwd->pw_uid);
  sprintf(record.header.gid, "%o ", grp == NULL ? 0 : (unsigned)grp->gr_gid);
  sprintf(record.header.size, "%011o", (unsigned)size);
  sprintf(record.header.mtime, "%011o", (unsigned)mtime);
  memset(&(record.header.chksum), ' ', sizeof(record.header.chksum));
  record.header.linkflag = type;
  if (type == TAR_SYMLINK)
    strlcpy(record.header.linkname, linkname, sizeof(record.header.linkname));
  strcpy(record.header.magic, TAR_MAGIC);
  memcpy(record.header.version, TAR_VERSION, 2);
  strlcpy(record.header.uname, user, sizeof(record.header.uname));
  strlcpy(record.header.gname, group, sizeof(record.header.uname));

 /*
  * Compute the checksum of the header...
  */

  for (i = sizeof(record), sumptr = record.all, sum = 0; i > 0; i --)
    sum += *sumptr++;

 /*
  * Put the correct checksum in place and write the header...
  */

  sprintf(record.header.chksum, "%6o", sum);

  if (fwrite(&record, 1, sizeof(record), fp->file) < sizeof(record))
    return (-1);

  fp->blocks ++;
  return (0);
}


/*
 * 'tar_open()' - Open a TAR file for writing.
 */

tarf_t *				/* O - New tar file */
tar_open(const char *filename,		/* I - File to create */
         int        compress)		/* I - Compress with gzip? */
{
  tarf_t	*fp;			/* New tar file */
  char		command[1024];		/* Compression command */


 /*
  * Allocate memory for the tar file state...
  */

  if ((fp = calloc(sizeof(tarf_t), 1)) == NULL)
    return (NULL);

 /*
  * Open the output file or pipe into gzip for compressed output...
  */

  if (compress)
  {
    snprintf(command, sizeof(command), EPM_GZIP " > %s", filename);
    fp->file = popen(command, "w");
  }
  else
    fp->file = fopen(filename, "wb");

 /*
  * If we couldn't open the output file, abort...
  */

  if (fp->file == NULL)
  {
    free(fp);
    return (NULL);
  }

 /*
  * Save the compression state and return...
  */

  fp->compressed = compress;

  return (fp);
}


/*
 * 'tar_package()' - Archive a package file.
 */

int					/* O - 0 on success, -1 on failure */
tar_package(tarf_t     *tar,		/* I - Tar file */
            const char *ext,		/* I - Package filename extension */
            const char *prodname,	/* I - Product short name */
	    const char *directory,	/* I - Directory for distribution files */
            const char *platname,	/* I - Platform name */
            dist_t     *dist,		/* I - Distribution information */
	    const char *subpackage)	/* I - Subpackage */
{
  char		prodfull[255],		/* Full name of product */
		name[1024],		/* Full product name */
		filename[1024];		/* File to archive */
  struct stat	filestat;		/* File information */


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
  * Find out more about the package file...
  */

  snprintf(filename, sizeof(filename), "%s/%s", directory, name);
  if (stat(filename, &filestat))
  {
    fprintf(stderr, "epm: Error reading package file \"%s\": %s\n",
            filename, strerror(errno));
    return (-1);
  }

 /*
  * Write the header and the file...
  */

  if (tar_header(tar, TAR_NORMAL, (mode_t)0644, filestat.st_size,
                 filestat.st_mtime, "root", "root", name, NULL))
    return (-1);

  return (tar_file(tar, filename));
}


/*
 * End of "$Id: tar.c 837 2010-12-30 00:25:29Z mike $".
 */

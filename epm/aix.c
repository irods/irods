/*
 * "$Id: aix.c 839 2010-12-30 18:41:50Z mike $"
 *
 *   AIX package gateway for the ESP Package Manager (EPM).
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
 *   make_aix()     - Make an AIX software distribution package.
 *   aix_addfile()  - Add a file to the AIX directory list...
 *   aix_fileset()  - Write a subpackage description...
 *   aix_version()  - Generate an AIX version number.
 *   write_liblpp() - Create the liblpp.a file for the root or /usr parts.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * Directory information...
 */

typedef struct
{
  char	dst[1024];			/* Output directory */
  int	blocks;				/* Size of files in this directory */
} aixdir_t;


/*
 * Local functions...
 */

static int	aix_addfile(int type, const char *src, const char *dst,
		            int num_dirs, aixdir_t **dirs);
static void	aix_fileset(FILE *fp, const char *prodname, dist_t *dist,
		            const char *subpackage);
static char	*aix_version(const char *version);
static int	write_liblpp(const char *prodname,
		             const char *directory,
		             dist_t *dist, int root,
			     const char *subpackage);


/*
 * Local globals...
 */

static const char	*files[] =	/* Control files... */
			{
			  "al",
			  "cfgfiles",
			  "copyright",
			  "inventory",
			  "post_i",
			  "pre_i",
			  "unpost_i",
                          "unpre_i"
			};


/*
 * 'make_aix()' - Make an AIX software distribution package.
 */

int					/* O - 0 = success, 1 = fail */
make_aix(const char     *prodname,	/* I - Product short name */
         const char     *directory,	/* I - Directory for distribution files */
         const char     *platname,	/* I - Platform name */
         dist_t         *dist,		/* I - Distribution information */
	 struct utsname *platform)	/* I - Platform information */
{
  int			i;		/* Looping var */
  FILE			*fp;		/* Control file */
  char			name[1024],	/* Full product name */
			filename[1024],	/* Destination filename */
			current[1024];	/* Current directory */
  file_t		*file;		/* Current distribution file */
  struct passwd		*pwd;		/* Pointer to user record */
  struct group		*grp;		/* Pointer to group record */
  const char		*runlevels;	/* Run levels */


  REF(platform);

  if (Verbosity)
    puts("Creating AIX distribution...");

  if (dist->release[0])
  {
    if (platname[0])
      snprintf(name, sizeof(name), "%s-%s-%s-%s", prodname, dist->version,
               dist->release, platname);
    else
      snprintf(name, sizeof(name), "%s-%s-%s", prodname, dist->version,
               dist->release);
  }
  else if (platname[0])
    snprintf(name, sizeof(name), "%s-%s-%s", prodname, dist->version, platname);
  else
    snprintf(name, sizeof(name), "%s-%s", prodname, dist->version);

  getcwd(current, sizeof(current));

 /*
  * Write the lpp_name file for installp...
  */

  if (Verbosity)
    puts("Creating lpp_name file...");

  snprintf(filename, sizeof(filename), "%s/%s", directory, prodname);
  make_directory(filename, 0755, 0, 0);

  snprintf(filename, sizeof(filename), "%s/%s/lpp_name", directory, prodname);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create lpp_name file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  fprintf(fp, "4 R I %s {\n", prodname);

  aix_fileset(fp, prodname, dist, NULL);
  for (i = 0; i < dist->num_subpackages; i ++)
    aix_fileset(fp, prodname, dist, dist->subpackages[i]);

  fputs("}\n", fp);

  fclose(fp);

 /*
  * Write the root partition liblpp.a file...
  */

  write_liblpp(prodname, directory, dist, 1, NULL);
  for (i = 0; i < dist->num_subpackages; i ++)
    write_liblpp(prodname, directory, dist, 1, dist->subpackages[i]);

 /*
  * Write the usr partition liblpp.a file...
  */

  write_liblpp(prodname, directory, dist, 0, NULL);
  for (i = 0; i < dist->num_subpackages; i ++)
    write_liblpp(prodname, directory, dist, 0, dist->subpackages[i]);

 /*
  * Copy the files over...
  */

  if (Verbosity)
    puts("Copying temporary distribution files...");

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
  {
   /*
    * Find the username and groupname IDs...
    */

    pwd = getpwnam(file->user);
    grp = getgrnam(file->group);

    endpwent();
    endgrent();

   /*
    * Copy the file or make the directory or make the symlink as needed...
    */

    switch (tolower(file->type))
    {
      case 'c' :
      case 'f' :
          if (!strncmp(file->dst, "/export/", 8) ||
	      !strncmp(file->dst, "/opt/", 5) ||
	      !strncmp(file->dst, "/usr/", 5))
            snprintf(filename, sizeof(filename), "%s/%s%s", directory, prodname,
	             file->dst);
	  else
            snprintf(filename, sizeof(filename), "%s/%s/usr/lpp/%s/inst_root%s",
	             directory, prodname, prodname, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;
      case 'i' :
          for (runlevels = get_runlevels(file, "2");
	       isdigit(*runlevels & 255);
	       runlevels ++)
	  {
            snprintf(filename, sizeof(filename),
	             "%s/%s/usr/lpp/%s/inst_root/etc/rc.d/rc%c.d/S%02d%s",
	             directory, prodname, prodname, *runlevels,
		     get_start(file, 99), file->dst);

	    if (Verbosity > 1)
	      printf("%s -> %s...\n", file->src, filename);

	    if (copy_file(filename, file->src, file->mode,
	                  pwd ? pwd->pw_uid : 0, grp ? grp->gr_gid : 0))
	      return (1);

            snprintf(filename, sizeof(filename),
	             "%s/%s/usr/lpp/%s/inst_root/etc/rc.d/rc%c.d/K%02d%s",
	             directory, prodname, prodname, *runlevels,
		     get_stop(file, 0), file->dst);

	    if (Verbosity > 1)
	      printf("%s -> %s...\n", file->src, filename);

	    if (copy_file(filename, file->src, file->mode,
	                  pwd ? pwd->pw_uid : 0, grp ? grp->gr_gid : 0))
	      return (1);
	  }
          break;
      case 'd' :
          if (!strcmp(file->dst, "/export") ||
	      !strncmp(file->dst, "/export/", 8) ||
	      !strcmp(file->dst, "/opt") ||
	      !strncmp(file->dst, "/opt/", 5) ||
	      !strcmp(file->dst, "/usr") ||
	      !strncmp(file->dst, "/usr/", 5))
            snprintf(filename, sizeof(filename), "%s/%s%s", directory, prodname,
	             file->dst);
	  else
            snprintf(filename, sizeof(filename), "%s/%s/usr/lpp/%s/inst_root%s",
	             directory, prodname, prodname, file->dst);

	  if (Verbosity > 1)
	    printf("Directory %s...\n", filename);

          make_directory(filename, file->mode, pwd ? pwd->pw_uid : 0,
			 grp ? grp->gr_gid : 0);
          break;
      case 'l' :
          if (!strncmp(file->dst, "/export/", 8) ||
	      !strncmp(file->dst, "/opt/", 5) ||
	      !strncmp(file->dst, "/usr/", 5))
            snprintf(filename, sizeof(filename), "%s/%s%s", directory, prodname,
	             file->dst);
	  else
            snprintf(filename, sizeof(filename), "%s/%s/usr/lpp/%s/inst_root%s",
	             directory, prodname, prodname, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

          make_link(filename, file->src);
          break;
    }
  }

 /*
  * Build the distribution from the spec file...
  */

  if (Verbosity)
    puts("Building AIX binary distribution...");

  snprintf(filename, sizeof(filename), "%s/%s", directory, prodname);

  if (run_command(filename,
		  "sh -c \'find . -print | backup -i -f ../%s.bff -q %s\'",
                  prodname, Verbosity ? "-v" : ""))
    return (1);

 /*
  * Remove temporary files...
  */

  if (!KeepFiles)
  {
    if (Verbosity)
      puts("Removing temporary distribution files...");

    snprintf(filename, sizeof(filename), "%s/%s", directory, prodname);
    unlink_directory(filename);

    for (i = 0; i < (sizeof(files) / sizeof(files[0])); i ++)
    {
      snprintf(filename, sizeof(filename), "%s/%s.%s", directory, prodname,
               files[i]);
      unlink(filename);
    }

    snprintf(filename, sizeof(filename), "%s/lpp.README", directory);
    unlink(filename);
  }

  return (0);
}


/*
 * 'aix_addfile()' - Add a file to the AIX directory list...
 */

static int				/* O  - New number dirs */
aix_addfile(int        type,		/* I  - Filetype */
            const char *src,		/* I  - Source path */
            const char *dst,		/* I  - Destination path */
	    int        num_dirs,	/* I  - Number of directories */
            aixdir_t   **dirs)		/* IO - Directories */
{
  int		i, j;			/* Looping vars */
  int		blocks;			/* Blocks to add... */
  struct stat	fileinfo;		/* File information */
  aixdir_t	*temp;			/* Temporary pointer */
  char		dstpath[1024],		/* Destination path */
		*dstptr;		/* Pointer into destination */


 /*
  * Determine the destination path and block size...
  */

  strlcpy(dstpath, dst, sizeof(dstpath));

  if (type == 'd')
  {
    blocks = 1;
    dstptr = dstpath + strlen(dstpath) - 1;
  }
  else
  {
    dstptr = strrchr(dstpath, '/');

    if (type == 'l')
      blocks = 1;
    else if (!stat(src, &fileinfo))
      blocks = (fileinfo.st_size + 511) / 512;
    else
      blocks = 0;
  }

  if (dstptr && *dstptr == '/' && dstptr > dstpath)
    *dstptr = '\0';

 /*
  * Now see if the destination path is in the array...
  */

  temp = *dirs;

  for (i = 0; i < num_dirs; i ++)
    if ((j = strcmp(temp[i].dst, dstpath)) == 0)
    {
      temp[i].blocks += blocks;
      return (num_dirs);
    }
    else if (j > 0)
      break;

 /*
  * Not in the list; allocate a new one...
  */

  if (num_dirs == 0)
    temp = malloc(sizeof(aixdir_t));
  else
    temp = realloc(*dirs, (num_dirs + 1) * sizeof(aixdir_t));

  if (!temp)
    return (num_dirs);

  *dirs = temp;
  temp  += i;

  if (i < num_dirs)
    memmove(temp + 1, temp, (num_dirs - i) * sizeof(aixdir_t));

  strcpy(temp->dst, dstpath);
  temp->blocks = blocks;

  return (num_dirs + 1);
}


/*
 * 'aix_fileset()' - Write a subpackage description...
 */

static void
aix_fileset(FILE       *fp,		/* I - File to write to */
            const char *prodname,	/* I - Product name */
            dist_t     *dist,		/* I - Distribution */
	    const char *subpackage)	/* I - Subpackage */
{
  int			i;		/* Looping var */
  depend_t		*d;		/* Current dependency */
  file_t		*file;		/* Current distribution file */
  int			num_dirs;	/* Number of directories */
  aixdir_t		*dirs;		/* Directories */


 /*
  * Start fileset definition...
  */

  if (subpackage)
    fprintf(fp, "%s.%s", prodname, subpackage);
  else
    fprintf(fp, "%s", prodname);

  fprintf(fp, " %s 01 N B x ", aix_version(dist->version));

  if (subpackage)
  {
    for (i = 0; i < dist->num_descriptions; i ++)
    {
      if (dist->descriptions[i].subpackage == subpackage)
      {
	fprintf(fp, "%s\n", dist->descriptions[i].description);
	break;
      }
    }
  }
  else
    fprintf(fp, "%s\n", dist->product);


 /*
  * Dependencies...
  */

  fputs("[\n", fp);
  for (i = dist->num_depends, d = dist->depends; i > 0; i --, d ++)
    if (d->type == DEPEND_REQUIRES && d->subpackage == subpackage &&
        strcmp(d->product, "_self"))
      fprintf(fp, "*prereq %s %s\n", d->product, aix_version(d->version[0]));

 /*
  * Installation sizes...
  */

  fputs("%\n", fp);

  num_dirs = 0;
  dirs     = NULL;

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (file->subpackage == subpackage)
      num_dirs = aix_addfile(tolower(file->type), file->src, file->dst,
                             num_dirs, &dirs);

  for (i = 0; i < num_dirs; i ++)
    fprintf(fp, "%s %d\n", dirs[i].dst, dirs[i].blocks);

  if (num_dirs > 0)
    free(dirs);

 /*
  * This package supercedes which others?
  */

  fputs("%\n", fp);
  for (i = dist->num_depends, d = dist->depends; i > 0; i --, d ++)
    if (d->type == DEPEND_REPLACES && d->subpackage == subpackage &&
        strcmp(d->product, "_self"))
      fprintf(fp, "%s %s", d->product, aix_version(d->version[0]));

 /*
  * Fix information is only used for updates (patches)...
  */

  fputs("%\n", fp);
  fputs("]\n", fp);
}


/*
 * 'aix_version()' - Generate an AIX version number.
 */

static char *				/* O - AIX version number */
aix_version(const char *version)	/* I - EPM version number */
{
  int		verparts[4];		/* Version number parts */
  static char	aix[255];		/* AIX version number string */


 /*
  * AIX requires a four-part version number (M.m.p.r)...
  */

  memset(verparts, 0, sizeof(verparts));
  sscanf(version, "%d.%d.%d.%d", verparts + 0, verparts + 1,
         verparts + 2, verparts + 3);
  sprintf(aix, "%d.%d.%d.%d", verparts[0], verparts[1],
          verparts[2], verparts[3]);

  return (aix);
}


/*
 * 'write_liblpp()' - Create the liblpp.a file for the root or /usr parts.
 */

static int				/* O - 0 = success, 1 = fail */
write_liblpp(const char     *prodname,	/* I - Product short name */
             const char     *directory,	/* I - Directory for distribution files */
             dist_t         *dist,	/* I - Distribution information */
             int            root,	/* I - Root partition? */
	     const char     *subpackage)/* I - Subpackage */
{
  int			i;		/* Looping var */
  FILE			*fp;		/* Control file */
  char			filename[1024],	/* Destination filename */
			prodfull[1024];	/* Full product name */
  struct stat		fileinfo;	/* File information */
  command_t		*c;		/* Current command */
  file_t		*file;		/* Current distribution file */
  int			configcount;	/* Number of config files */
  int 			shared_file;	/* Shared file? */
  const char		*runlevels;	/* Run levels */


 /*
  * Progress info...
  */

  if (subpackage)
    snprintf(prodfull, sizeof(prodfull), "%s.%s", prodname, subpackage);
  else
    strlcpy(prodfull, prodname, sizeof(prodfull));

  if (Verbosity)
    printf("Updating %s partition liblpp.a file for %s...\n",
           root ? "root" : "shared", prodfull);

 /*
  * Write the product.al file for installp...
  */

  if (Verbosity > 1)
    puts("    Creating .al file...");

  snprintf(filename, sizeof(filename), "%s/%s.al", directory, prodfull);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create .al file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (file->subpackage == subpackage)
      switch (tolower(file->type))
      {
	case 'i' :
            for (runlevels = get_runlevels(file, "2");
		 isdigit(*runlevels & 255);
		 runlevels ++)
            {
	      if (root)
	        putc('.', fp);
	      else
        	qprintf(fp, "./usr/lpp/%s/inst_root", prodfull);

              if (*runlevels == '0')
        	qprintf(fp, "/etc/rc.d/rc0.d/K%02d%s\n",
		        get_stop(file, 0), file->dst);
	      else
        	qprintf(fp, "/etc/rc.d/rc%c.d/S%02d%s\n",
	        	*runlevels, get_start(file, 99), file->dst);
            }
	    break;

	default :
            shared_file = !(strcmp(file->dst, "/usr") && 
                            strncmp(file->dst, "/usr/", 5) && 
                            strcmp(file->dst, "/opt") && 
                            strncmp(file->dst, "/opt/", 5));

           /*
	    * Put file in root or share .al file as appropriate
            */

            if ((shared_file && !root) || (!shared_file && root))
              qprintf(fp, ".%s\n", file->dst);

           /*
	    * Put any root file in the share .al so it will be extracted
	    * to /usr/lpp/<prodfull>/inst_root directory.  I have no
	    * idea if this is really the way to do it but it seems to
	    * work...
	    */

            if (!shared_file && !root)
              qprintf(fp, "./usr/lpp/%s/inst_root%s\n", prodfull, file->dst);
	    break;
      }

  fclose(fp);

 /*
  * Write the product.cfgfiles file for installp...
  */

  if (Verbosity > 1)
    puts("    Creating .cfgfiles file...");

  snprintf(filename, sizeof(filename), "%s/%s.cfgfiles", directory, prodfull);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create .cfgfiles file \"%s\" - %s\n",
            filename, strerror(errno));
    return (1);
  }

  configcount = 0;

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'c' && file->subpackage == subpackage &&
        (strcmp(file->dst, "/usr") ||
	 strncmp(file->dst, "/usr/", 5) ||
	 strcmp(file->dst, "/opt") ||
	 strncmp(file->dst, "/opt/", 5)) == root)
    {
      qprintf(fp, ".%s hold_new\n", file->dst);
      configcount ++;
    }

  fclose(fp);

 /*
  * Write the product.copyright file for installp...
  */

  if (Verbosity > 1)
    puts("    Creating .copyright file...");

  snprintf(filename, sizeof(filename), "%s/%s.copyright", directory, prodfull);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create .copyright file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  fprintf(fp, "%s, %s\n%s\n%s\n", dist->product, aix_version(dist->version),
          dist->vendor, dist->copyright);

  fclose(fp);

  if (root)
  {
   /*
    * Write the product.pre_i file for installp...
    */

    if (Verbosity > 1)
      puts("    Creating .pre_i file...");

    snprintf(filename, sizeof(filename), "%s/%s.pre_i", directory, prodfull);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create .pre_i file \"%s\" - %s\n",
              filename, strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (c = dist->commands, i = dist->num_commands; i > 0; i --, c ++)
      if (c->type == COMMAND_PRE_INSTALL && c->subpackage == subpackage)
	fprintf(fp, "%s\n", c->command);

    fclose(fp);

   /*
    * Write the product.post_i file for installp...
    */

    if (Verbosity > 1)
      puts("    Creating .post_i file...");

    snprintf(filename, sizeof(filename), "%s/%s.post_i", directory, prodfull);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create .post_i file \"%s\" - %s\n",
              filename, strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (c = dist->commands, i = dist->num_commands; i > 0; i --, c ++)
      if (c->type == COMMAND_POST_INSTALL && c->subpackage == subpackage)
	fprintf(fp, "%s\n", c->command);

    fclose(fp);

   /*
    * Write the product.unpre_i file for installp...
    */

    if (Verbosity > 1)
      puts("	Creating .unpre_i file...");

    snprintf(filename, sizeof(filename), "%s/%s.unpre_i", directory, prodfull);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create .unpre_i file \"%s\" - %s\n",
              filename, strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (c = dist->commands, i = dist->num_commands; i > 0; i --, c ++)
      if (c->type == COMMAND_PRE_REMOVE && c->subpackage == subpackage)
	fprintf(fp, "%s\n", c->command);

    fclose(fp);

   /*
    * Write the product.unpost_i file for installp...
    */

    if (Verbosity > 1)
      puts("    Creating .unpost_i file...");

    snprintf(filename, sizeof(filename), "%s/%s.unpost_i", directory, prodfull);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create .unpost_i file \"%s\" - %s\n",
              filename, strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (c = dist->commands, i = dist->num_commands; i > 0; i --, c ++)
     if (c->type == COMMAND_POST_REMOVE && c->subpackage == subpackage)
	fprintf(fp, "%s\n", c->command);

    fclose(fp);
  }

 /*
  * Write the product.inventory file for installp...
  */

  if (Verbosity > 1)
    puts("    Creating .inventory file...");

  snprintf(filename, sizeof(filename), "%s/%s.inventory", directory, prodfull);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create .inventory file \"%s\" - %s\n",
            filename, strerror(errno));
    return (1);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
  {
    if (file->subpackage != subpackage)
      continue;

    if (root)
    {
      if (!strcmp(file->dst, "/usr") ||
	  !strncmp(file->dst, "/usr/", 5) ||
	  !strcmp(file->dst, "/opt") ||
	  !strncmp(file->dst, "/opt/", 5))
        continue;
    }
    else
    {
      if (tolower(file->type) == 'c' ||
          (strcmp(file->dst, "/usr") &&
	   strncmp(file->dst, "/usr/", 5) &&
	   strcmp(file->dst, "/opt") &&
	   strncmp(file->dst, "/opt/", 5)))
	continue;
    }

    switch (tolower(file->type))
    {
      case 'i' :
          for (runlevels = get_runlevels(file, "2");
	       isdigit(*runlevels & 255);
	       runlevels ++)
          {
	    if (*runlevels == '0')
	      qprintf(fp, "/etc/rc.d/rc0.d/K%02d%s:\n",
	              get_stop(file, 0), file->dst);
            else
	      qprintf(fp, "/etc/rc.d/rc%c.d/S%02d%s:\n",
	              *runlevels, get_start(file, 99), file->dst);

	    fprintf(fp, "    class=apply,inventory,%s\n", prodfull);

            fputs("    type=FILE\n", fp);
            if (!stat(file->src, &fileinfo))
	      fprintf(fp, "    size=%d\n", (int)fileinfo.st_size);

	    fprintf(fp, "    owner=%s\n", file->user);
	    fprintf(fp, "    group=%s\n", file->group);
	    fprintf(fp, "    mode=%04o\n", (unsigned)file->mode);
	    fputs("\n", fp);
	  }
	  break;
      default :
          qprintf(fp, "%s:\n", file->dst);
	  fprintf(fp, "    class=apply,inventory,%s\n", prodfull);

	  switch (tolower(file->type))
	  {
	    case 'd' :
        	fputs("    type=DIRECTORY\n", fp);
        	break;
	    case 'l' :
        	fputs("    type=SYMLINK\n", fp);
		qprintf(fp, "    target=%s\n", file->src);
        	break;
	    case 'c' :
        	fputs("    type=FILE\n", fp);
        	fputs("    size=VOLATILE\n", fp);
		break;
	    default :
        	fputs("    type=FILE\n", fp);
        	if (!stat(file->src, &fileinfo))
		  fprintf(fp, "    size=%ld\n", (long)fileinfo.st_size);
		break;
	  }

	  fprintf(fp, "    owner=%s\n", file->user);
	  fprintf(fp, "    group=%s\n", file->group);
	  fprintf(fp, "    mode=%04o\n", (unsigned)file->mode);
	  fputs("\n", fp);
	  break;
    }
  }

  fclose(fp);

 /*
  * Write the lpp.README file...
  */

  snprintf(filename, sizeof(filename), "%s/lpp.README", directory);

  if (dist->license[0])
    copy_file(filename, dist->license, 0644, 0, 0);
  else if (dist->readme[0])
    copy_file(filename, dist->readme, 0644, 0, 0);
  else if ((fp = fopen(filename, "w")) != NULL)
    fclose(fp);
  else
  {
    fprintf(stderr, "epm: Unable to create .README file \"%s\" - %s\n",
            filename, strerror(errno));
    return (1);
  }

 /*
  * Create the liblpp.a file...
  */

  if (Verbosity > 1)
    puts("    Creating liblpp.a archive...");

  if (root)
  {
    snprintf(filename, sizeof(filename), "%s/%s/usr/lpp/%s/inst_root",
             directory, prodname, prodname);
    make_directory(filename, 0755, 0, 0);

    snprintf(filename, sizeof(filename), "%s/usr/lpp/%s/inst_root/liblpp.a",
             prodname, prodname);
  }
  else
  {
    snprintf(filename, sizeof(filename), "%s/%s/usr/lpp/%s",
             directory, prodname, prodname);
    make_directory(filename, 0755, 0, 0);

    snprintf(filename, sizeof(filename), "%s/usr/lpp/%s/liblpp.a",
             prodname, prodname);
  }

  if (!subpackage)
    if (run_command(directory, "ar rc %s lpp.README", filename))
      return (1);

  for (i = 0; i < (sizeof(files) / sizeof(files[0])); i ++)
  {
    if (i >= 4 && !root)
      break;

    if (i == 1 && !configcount)
      continue;

    if (run_command(directory, "ar rc %s %s.%s",
                    filename, prodfull, files[i]))
      return (1);
  }

  return (0);
}


/*
 * End of "$Id: aix.c 839 2010-12-30 18:41:50Z mike $".
 */

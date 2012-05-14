/*
 * "$Id: slackware.c 833 2010-12-30 00:00:50Z mike $"
 *
 *   Slackware package gateway for the ESP Package Manager (EPM).
 *
 *   Copyright 2003-2010 by Easy Software Products.
 *
 *   Contributed by Alec Thomas <$givenname at korn dot ch>
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
 *   make_slackware()  - Make a Slackware software distribution package.
 *   make_subpackage() - Make a Slackware subpackage.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * Local functions...
 */

static int	make_subpackage(const char *prodname, const char *directory,
		                const char *platname, dist_t *dist,
				const char *subpackage);


/*
 * 'make_slackware()' - Make a Slackware software distribution packages.
 */

int					/* O - 0 = success, 1 = fail */
make_slackware(const char     *prodname,/* I - Product short name */
	       const char     *directory,
					/* I - Directory for distribution files */
	       const char     *platname,/* I - Platform name */
	       dist_t         *dist,	/* I - Distribution information */
	       struct utsname *platform)/* I - Platform information */
{
  int		i;			/* Looping var */


  REF(platform);

 /*
  * Check to see if we are being run as root...
  */

  if (getuid())
  {
    fprintf(stderr, "ERROR: Can only make Slackware packages as root\n");
    return (1);
  }

 /*
  * Create subpackages...
  */

  if (make_subpackage(prodname, directory, platname, dist, NULL))
    return (1);

  for (i = 0; i < dist->num_subpackages; i ++)
    if (make_subpackage(prodname, directory, platname, dist,
                        dist->subpackages[i]))
      return (1);

  return (0);
}


/*
 * 'make_subpackage()' - Make a Slackware subpackage.
 */

static int				/* O - 0 = success, 1 = fail */
make_subpackage(const char *prodname,	/* I - Product short name */
	        const char *directory,	/* I - Directory for distribution files */
	        const char *platname,	/* I - Platform name */
	        dist_t     *dist,	/* I - Distribution information */
	        const char *subpackage)	/* I - Subpackage name */
{
  int		i;			/* Looping var */
  FILE		*fp;			/* Spec file */
  char		prodfull[1024],		/* Full name of product */
		filename[1024],		/* Destination filename */
		pkgname[1024];		/* Package filename */
  file_t	*file;			/* Current distribution file */
  command_t	*c;			/* Current command */
  struct passwd	*pwd;			/* Pointer to user record */
  struct group	*grp;			/* Pointer to group record */
  struct utsname platform;		/* Original platform data */


 /*
  * Figure out the full product name...
  */

  if (subpackage)
    snprintf(prodfull, sizeof(prodfull), "%s-%s", prodname, subpackage);
  else
    strlcpy(prodfull, prodname, sizeof(prodfull));

  if (Verbosity)
    printf("Creating Slackware %s pkg distribution...\n", prodfull);

 /*
  * Slackware uses the real machine type in its package names...
  */

  if (uname(&platform))
  {
    fprintf(stderr, "ERROR: Can't get platform information - %s\n",
            strerror(errno));

    return (1);
  }

  snprintf(pkgname, sizeof(pkgname), "%s-%s-%s-%s.tgz", prodfull,
           dist->version, platform.machine, dist->release);

 /*
  * Make a copy of the distribution files...
  */

  if (Verbosity)
    puts("Copying temporary distribution files...");

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
  {
   /*
    * Check subpackage...
    */

    if (file->subpackage != subpackage)
      continue;

   /*
    * Find the user and group IDs...
    */

    pwd = getpwnam(file->user);
    grp = getgrnam(file->group);

    endpwent();
    endgrent();

   /*
    * Copy the file, make the directory, or make the symlink as needed...
    */

    switch (tolower(file->type))
    {
      case 'c' :
      case 'f' :
	  snprintf(filename, sizeof(filename), "%s/%s%s", directory, prodfull,
	           file->dst);

	  if (Verbosity > 1)
	    printf("F %s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
	                grp ? grp->gr_gid : 0))
	    return (1);
	  break;

      case 'i' :
	  snprintf(filename, sizeof(filename), "%s/%s/etc/rc.d/%s", directory,
		   prodfull, file->dst);

	  if (Verbosity > 1)
	    printf("I %s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
	  break;

      case 'd' :
	  snprintf(filename, sizeof(filename), "%s/%s%s", directory, prodfull,
	           file->dst);

	  if (Verbosity > 1)
	    printf("D %s...\n", filename);

	  make_directory(filename, file->mode, pwd ? pwd->pw_uid : 0,
			 grp ? grp->gr_gid : 0);
	  break;

      case 'l' :
	  snprintf(filename, sizeof(filename), "%s/%s%s", directory, prodfull,
	           file->dst);

	  if (Verbosity > 1)
	    printf("L %s -> %s...\n", file->src, filename);

	  make_link(filename, file->src);
	  break;
    }
  }

 /*
  * Write descriptions and post-install commands as needed...
  */

  for (i = 0; i < dist->num_descriptions; i ++)
    if (dist->descriptions[i].subpackage == subpackage)
      break;

  if (i < dist->num_descriptions)
  {
    snprintf(filename, sizeof(filename), "%s/%s/install", directory, prodfull);
    make_directory(filename, 0755, 0, 0);

    strlcat(filename, "/slack-desc", sizeof(filename));

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "ERROR: Couldn't create Slackware description file \"%s\" - %s\n",
              filename, strerror(errno));
      return (1);
    }

    fprintf(fp, "%s: %s\n%s:\n", prodfull, dist->product, prodfull);

    for (; i < dist->num_descriptions; i ++)
      if (dist->descriptions[i].subpackage == subpackage)
        fprintf(fp, "%s: %s\n", prodfull, dist->descriptions[i].description);

    fprintf(fp, "%s:\n", prodfull);
    fprintf(fp, "%s: (Vendor: %s, Packager: %s)\n", prodfull, dist->vendor,
            dist->packager);
    fprintf(fp, "%s:\n", prodfull);

    fclose(fp);
  }

  for (i = 0, c = dist->commands; i < dist->num_commands; i ++, c ++)
    if (c->subpackage == subpackage)
      break;

  if (i < dist->num_commands)
  {
    snprintf(filename, sizeof(filename), "%s/%s/install", directory, prodfull);
    make_directory(filename, 0755, 0, 0);

    strlcat(filename, "/doinst.sh", sizeof(filename));

    if (!(fp = fopen(filename, "w")))
    {
      fprintf(stderr, "ERROR: Couldn't create post install script \"%s\" - %s\n",
              filename, strerror(errno));
      return (1);
    }

    fputs("#!/bin/sh\n", fp);

    for (i = 0, c = dist->commands; i < dist->num_commands; i ++, c ++)
      if (c->subpackage == subpackage)
      {
	switch (c->type)
	{
	  case COMMAND_PRE_INSTALL :
	      fputs("WARNING: Package contains pre-install commands which are not supported\n"
		    "         by the Slackware packager.\n", stderr);
	      break;

	  case COMMAND_POST_INSTALL :
	      fprintf(fp, "%s\n", c->command);
	      break;

	  case COMMAND_PRE_REMOVE :
	      fputs("WARNING: Package contains pre-removal commands which are not supported\n"
		    "         by the Slackware packager.\n", stderr);
	      break;

	  case COMMAND_POST_REMOVE :
	      fputs("WARNING: Package contains post-removal commands which are not supported\n"
		    "         by the Slackware packager.\n", stderr);
	      break;
	}
      }

    fclose(fp);
  }

 /*
  * Remove any existing package of the same name, then create the
  * package...
  */

  snprintf(filename, sizeof(filename), "%s/%s", directory, pkgname);
  unlink(filename);

  if (Verbosity)
    puts("Building Slackware package...");

  snprintf(filename, sizeof(filename), "%s/%s", directory, prodfull);

  if (run_command(filename, "makepkg --linkadd y --chown n ../%s", pkgname))
    return (1);

 /*
  * Remove temporary files...
  */

  if (!KeepFiles)
  {
    if (Verbosity)
      puts("Removing temporary distribution files...");

    unlink_directory(filename);
  }

  return (0);
}


/*
 * End of "$Id: slackware.c 833 2010-12-30 00:00:50Z mike $".
 */

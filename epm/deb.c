/*
 * "$Id: deb.c 833 2010-12-30 00:00:50Z mike $"
 *
 *   Debian package gateway for the ESP Package Manager (EPM).
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
 *   make_deb()        - Make a Debian software distribution package.
 *   make_subpackage() - Make a subpackage...
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
		                struct utsname *platform,
				const char *subpackage);


/*
 * 'make_deb()' - Make a Debian software distribution package.
 */

int					/* O - 0 = success, 1 = fail */
make_deb(const char     *prodname,	/* I - Product short name */
         const char     *directory,	/* I - Directory for distribution files */
         const char     *platname,	/* I - Platform name */
         dist_t         *dist,		/* I - Distribution information */
	 struct utsname *platform)	/* I - Platform information */
{
  int		i;			/* Looping var */
  tarf_t	*tarfile;		/* Distribution tar file */
  char		name[1024],		/* Full product name */
		filename[1024];		/* File to archive */


  if (make_subpackage(prodname, directory, platname, dist, platform, NULL))
    return (1);

  for (i = 0; i < dist->num_subpackages; i ++)
    if (make_subpackage(prodname, directory, platname, dist, platform,
                        dist->subpackages[i]))
      return (1);

 /*
  * Build a compressed tar file to hold all of the subpackages...
  */

  if (dist->num_subpackages)
  {
   /*
    * Figure out the full name of the distribution...
    */

    if (dist->release[0])
      snprintf(name, sizeof(name), "%s-%s-%s", prodname, dist->version,
               dist->release);
    else
      snprintf(name, sizeof(name), "%s-%s", prodname, dist->version);

    if (platname[0])
    {
      strlcat(name, "-", sizeof(name));
      strlcat(name, platname, sizeof(name));
    }

   /*
    * Create a compressed tar file...
    */

    snprintf(filename, sizeof(filename), "%s/%s.deb.tgz", directory, name);

    if ((tarfile = tar_open(filename, 1)) == NULL)
      return (1);

   /*
    * Archive the main package and subpackages...
    */

    if (tar_package(tarfile, "deb", prodname, directory, platname, dist, NULL))
    {
      tar_close(tarfile);
      return (1);
    }

    for (i = 0; i < dist->num_subpackages; i ++)
    {
      if (tar_package(tarfile, "deb", prodname, directory, platname, dist,
                      dist->subpackages[i]))
      {
	tar_close(tarfile);
	return (1);
      }
    }
    
    tar_close(tarfile);
  }

 /*
  * Remove temporary files...
  */

  if (!KeepFiles && dist->num_subpackages)
  {
    if (Verbosity)
      puts("Removing temporary distribution files...");

   /*
    * Remove .deb files since they are now in a .tgz file...
    */

    unlink_package("deb", prodname, directory, platname, dist, NULL);

    for (i = 0; i < dist->num_subpackages; i ++)
      unlink_package("deb", prodname, directory, platname, dist,
                     dist->subpackages[i]);
  }

  return (0);
}


/*
 * 'make_subpackage()' - Make a subpackage...
 */

static int				/* O - 0 = success, 1 = fail */
make_subpackage(const char     *prodname,
					/* I - Product short name */
                const char     *directory,
					/* I - Directory for distribution files */
                const char     *platname,
					/* I - Platform name */
                dist_t         *dist,	/* I - Distribution information */
	        struct utsname *platform,
					/* I - Platform information */
		const char     *subpackage)
					/* I - Subpackage */
{
  int			i, j;		/* Looping vars */
  const char		*header;	/* Dependency header string */
  FILE			*fp;		/* Control file */
  char			prodfull[255],	/* Full name of product */
			name[1024],	/* Full product name */
			filename[1024];	/* Destination filename */
  command_t		*c;		/* Current command */
  depend_t		*d;		/* Current dependency */
  file_t		*file;		/* Current distribution file */
  struct passwd		*pwd;		/* Pointer to user record */
  struct group		*grp;		/* Pointer to group record */
  const char		*runlevels,	/* Run levels */
			*rlptr;		/* Pointer into runlevels */
  static const char	*depends[] =	/* Dependency names */
			{
			  "Depends:",
			  "Conflicts:",
			  "Replaces:",
			  "Provides:"
			};


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

  if (Verbosity)
    printf("Creating Debian %s distribution...\n", name);

 /*
  * Write the control file for DPKG...
  */

  if (Verbosity)
    puts("Creating control file...");

  snprintf(filename, sizeof(filename), "%s/%s", directory, name);
  mkdir(filename, 0777);
  strlcat(filename, "/DEBIAN", sizeof(filename));
  mkdir(filename, 0777);
  chmod(filename, 0755);

  strlcat(filename, "/control", sizeof(filename));

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create control file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  fprintf(fp, "Package: %s\n", prodfull);
  if (dist->release[0])
    fprintf(fp, "Version: %s-%s\n", dist->version, dist->release);
  else
    fprintf(fp, "Version: %s\n", dist->version);
  fprintf(fp, "Maintainer: %s\n", dist->vendor);

 /*
  * The Architecture attribute needs to match the uname info
  * (which we change in get_platform to a common name)
  */

  if (!strcmp(platform->machine, "intel"))
    fputs("Architecture: i386\n", fp);
  else if (!strcmp(platform->machine, "ppc"))
    fputs("Architecture: powerpc\n", fp);
  else
    fprintf(fp, "Architecture: %s\n", platform->machine);

  fprintf(fp, "Description: %s\n", dist->product);
  fprintf(fp, " Copyright: %s\n", dist->copyright);
  for (i = 0; i < dist->num_descriptions; i ++)
    if (dist->descriptions[i].subpackage == subpackage)
      fprintf(fp, " %s\n", dist->descriptions[i].description);

  for (j = DEPEND_REQUIRES; j <= DEPEND_PROVIDES; j ++)
  {
    for (i = dist->num_depends, d = dist->depends; i > 0; i --, d ++)
      if (d->type == j && d->subpackage == subpackage)
	break;

    if (i)
    {
      for (header = depends[j]; i > 0; i --, d ++, header = ",")
	if (d->type == j && d->subpackage == subpackage)
	{
	  if (!strcmp(d->product, "_self"))
            fprintf(fp, "%s %s", header, prodname);
	  else
            fprintf(fp, "%s %s", header, d->product);

	  if (d->vernumber[0] == 0)
	  {
	    if (d->vernumber[1] < INT_MAX)
              fprintf(fp, " (<= %s)", d->version[1]);
	  }
	  else
	  {
	    if (d->vernumber[1] < INT_MAX)
	      fprintf(fp, " (>= %s, <= %s)", d->version[0], d->version[1]);
	    else
	      fprintf(fp, " (>= %s)", d->version[0]);
	  }
	}

      putc('\n', fp);
    }
  }

  fclose(fp);

 /*
  * Write the preinst file for DPKG...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_INSTALL && c->subpackage == subpackage)
      break;

  if (i)
  {
    if (Verbosity)
      puts("Creating preinst script...");

    snprintf(filename, sizeof(filename), "%s/%s/DEBIAN/preinst", directory, name);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", filename,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (; i > 0; i --, c ++)
      if (c->type == COMMAND_PRE_INSTALL && c->subpackage == subpackage)
        fprintf(fp, "%s\n", c->command);

    fclose(fp);
  }

 /*
  * Write the postinst file for DPKG...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_INSTALL && c->subpackage == subpackage)
      break;

  if (!i)
    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i' && file->subpackage == subpackage)
        break;

  if (i)
  {
    if (Verbosity)
      puts("Creating postinst script...");

    snprintf(filename, sizeof(filename), "%s/%s/DEBIAN/postinst", directory, name);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", filename,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
      if (c->type == COMMAND_POST_INSTALL && c->subpackage == subpackage)
        fprintf(fp, "%s\n", c->command);

    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i' && file->subpackage == subpackage)
      {
        runlevels = get_runlevels(file, "02345");

        fprintf(fp, "update-rc.d %s start %02d", file->dst,
	        get_start(file, 99));

        for (rlptr = runlevels; isdigit(*rlptr & 255); rlptr ++)
	  if (*rlptr != '0')
	    fprintf(fp, " %c", *rlptr);

	if (strchr(runlevels, '0') != NULL)
          fprintf(fp, " . stop %02d 0", get_stop(file, 0));

        fputs(" . >/dev/null\n", fp);
        fprintf(fp, "/etc/init.d/%s start\n", file->dst);
      }

    fclose(fp);
  }

 /*
  * Write the prerm file for DPKG...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_REMOVE && c->subpackage == subpackage)
      break;

  if (!i)
    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i' && file->subpackage == subpackage)
        break;

  if (i)
  {
    if (Verbosity)
      puts("Creating prerm script...");

    snprintf(filename, sizeof(filename), "%s/%s/DEBIAN/prerm", directory, name);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", filename,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
      if (c->type == COMMAND_PRE_REMOVE && c->subpackage == subpackage)
        fprintf(fp, "%s\n", c->command);

    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i' && file->subpackage == subpackage)
        fprintf(fp, "/etc/init.d/%s stop\n", file->dst);

    fclose(fp);
  }

 /*
  * Write the postrm file for DPKG...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_REMOVE && c->subpackage == subpackage)
      break;

  if (!i)
    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i' && file->subpackage == subpackage)
        break;

  if (i)
  {
    if (Verbosity)
      puts("Creating postrm script...");

    snprintf(filename, sizeof(filename), "%s/%s/DEBIAN/postrm", directory, name);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", filename,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
      if (c->type == COMMAND_POST_REMOVE && c->subpackage == subpackage)
	fprintf(fp, "%s\n", c->command);

    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i' && file->subpackage == subpackage)
      {
        fputs("if [ purge = \"$1\" ]; then\n", fp);
        fprintf(fp, "	update-rc.d %s remove >/dev/null\n", file->dst);
        fputs("fi\n", fp);
      }

    fclose(fp);
  }

 /*
  * Write the conffiles file for DPKG...
  */

  if (Verbosity)
    puts("Creating conffiles...");

  snprintf(filename, sizeof(filename), "%s/%s/DEBIAN/conffiles", directory, name);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'c' && file->subpackage == subpackage)
      fprintf(fp, "%s\n", file->dst);
    else if (tolower(file->type) == 'i' && file->subpackage == subpackage)
      fprintf(fp, "/etc/init.d/%s\n", file->dst);

  fclose(fp);

 /*
  * Copy the files over...
  */

  if (Verbosity)
    puts("Copying temporary distribution files...");

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
  {
    if (file->subpackage != subpackage)
      continue;

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
          snprintf(filename, sizeof(filename), "%s/%s%s", directory, name,
	           file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;
      case 'i' :
          snprintf(filename, sizeof(filename), "%s/%s/etc/init.d/%s",
	           directory, name, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;
      case 'd' :
          snprintf(filename, sizeof(filename), "%s/%s%s", directory, name,
	           file->dst);

	  if (Verbosity > 1)
	    printf("Directory %s...\n", filename);

          make_directory(filename, file->mode, pwd ? pwd->pw_uid : 0,
			 grp ? grp->gr_gid : 0);
          break;
      case 'l' :
          snprintf(filename, sizeof(filename), "%s/%s%s", directory, name,
	           file->dst);

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
    printf("Building Debian %s binary distribution...\n", name);

  if (run_command(directory, "dpkg --build %s", name))
    return (1);

 /*
  * Remove temporary files...
  */

  if (!KeepFiles)
  {
    if (Verbosity)
      printf("Removing temporary %s distribution files...\n", name);

    snprintf(filename, sizeof(filename), "%s/%s", directory, name);
    unlink_directory(filename);
  }

  return (0);
}


/*
 * End of "$Id: deb.c 833 2010-12-30 00:00:50Z mike $".
 */

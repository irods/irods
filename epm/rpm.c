/*
 * "$Id: rpm.c 840 2010-12-30 20:30:03Z mike $"
 *
 *   Red Hat package gateway for the ESP Package Manager (EPM).
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
 *   make_rpm()   - Make a Red Hat software distribution package.
 *   move_rpms()  - Move RPM packages to the build directory...
 *   write_spec() - Write the subpackage-specific parts of the RPM spec file.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * Local functions...
 */

static int	move_rpms(const char *prodname, const char *directory,
		          const char *platname, dist_t *dist,
			  struct utsname *platform,
			  const char *rpmdir, const char *subpackage,
			  const char *release);
static int	write_spec(int format, const char *prodname, dist_t *dist,
		           FILE *fp, const char *subpackage);


/*
 * 'make_rpm()' - Make a Red Hat software distribution package.
 */

int					/* O - 0 = success, 1 = fail */
make_rpm(int            format,		/* I - Subformat */
         const char     *prodname,	/* I - Product short name */
         const char     *directory,	/* I - Directory for distribution files */
         const char     *platname,	/* I - Platform name */
         dist_t         *dist,		/* I - Distribution information */
	 struct utsname *platform,	/* I - Platform information */
         const char     *setup,		/* I - Setup GUI image */
         const char     *types)		/* I - Setup GUI install types */
{
  int		i;			/* Looping var */
  FILE		*fp;			/* Spec file */
  tarf_t	*tarfile;		/* Distribution tar file */
  char		specname[1024];		/* Spec filename */
  char		name[1024],		/* Product filename */
		filename[1024];		/* Destination filename */
  file_t	*file;			/* Current distribution file */
  char		absdir[1024];		/* Absolute directory */
  char		rpmdir[1024];		/* RPMDIR env var */
  char		release[256];		/* Release: number */
  const char	*build_option;		/* Additional rpmbuild option */


  if (Verbosity)
    puts("Creating RPM distribution...");

  if (directory[0] != '/')
  {
    char	current[1024];		/* Current directory */


    getcwd(current, sizeof(current));

    snprintf(absdir, sizeof(absdir), "%s/%s", current, directory);
  }
  else
    strlcpy(absdir, directory, sizeof(absdir));

 /*
  * Write the spec file for RPM...
  */

  if (Verbosity)
    puts("Creating spec file...");

  snprintf(specname, sizeof(specname), "%s/%s.spec", directory, prodname);

  if ((fp = fopen(specname, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create spec file \"%s\" - %s\n", specname,
            strerror(errno));
    return (1);
  }

  if (dist->release[0])
    strlcpy(release, dist->release, sizeof(release));
  else
    strlcpy(release, "0", sizeof(release));

  fprintf(fp, "Name: %s\n", prodname);
  fprintf(fp, "Version: %s\n", dist->version);
  if (dist->epoch)
    fprintf(fp, "Epoch: %d\n", dist->epoch);
  fprintf(fp, "Release: %s\n", release);
  fprintf(fp, "License: %s\n", dist->copyright);
  fprintf(fp, "Packager: %s\n", dist->packager);
  fprintf(fp, "Vendor: %s\n", dist->vendor);

  if (format == PACKAGE_LSB || format == PACKAGE_LSB_SIGNED)
    fputs("Requires: lsb >= 3.0\n", fp);

 /*
  * Tell RPM to put the distributions in the output directory...
  */

#ifdef EPM_RPMTOPDIR
  fprintf(fp, "%%define _topdir %s\n", absdir);
  strcpy(rpmdir, absdir);
#else
  if (getenv("RPMDIR"))
    strlcpy(rpmdir, getenv("RPMDIR"), sizeof(rpmdir));
  else if (!access("/usr/src/redhat", 0))
    strcpy(rpmdir, "/usr/src/redhat");
  else if (!access("/usr/src/Mandrake", 0))
    strcpy(rpmdir, "/usr/src/Mandrake");
  else
    strcpy(rpmdir, "/usr/src/RPM");
#endif /* EPM_RPMTOPDIR */

  snprintf(filename, sizeof(filename), "%s/RPMS", directory);

  make_directory(filename, 0777, getuid(), getgid());

  snprintf(filename, sizeof(filename), "%s/rpms", directory);
  symlink("RPMS", filename);

  if (!strcmp(platform->machine, "intel"))
    snprintf(filename, sizeof(filename), "%s/RPMS/i386", directory);
  else if (!strcmp(platform->machine, "ppc"))
    snprintf(filename, sizeof(filename), "%s/RPMS/powerpc", directory);
  else
    snprintf(filename, sizeof(filename), "%s/RPMS/%s", directory,
             platform->machine);

  make_directory(filename, 0777, getuid(), getgid());

 /*
  * Now list all of the subpackages...
  */

  write_spec(format, prodname, dist, fp, NULL);
  for (i = 0; i < dist->num_subpackages; i ++)
    write_spec(format, prodname, dist, fp, dist->subpackages[i]);

 /*
  * Close the spec file...
  */

  fclose(fp);

 /*
  * Copy the files over...
  */

  if (Verbosity)
    puts("Copying temporary distribution files...");

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
  {
   /*
    * Copy the file or make the directory or make the symlink as needed...
    */

    switch (tolower(file->type))
    {
      case 'c' :
      case 'f' :
          snprintf(filename, sizeof(filename), "%s/buildroot%s", directory, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, 0, -1, -1))
	    return (1);
          break;
      case 'i' :
          if (format == PACKAGE_LSB || format == PACKAGE_LSB_SIGNED)
	    snprintf(filename, sizeof(filename), "%s/buildroot/etc/init.d/%s",
		     directory, file->dst);
          else
	    snprintf(filename, sizeof(filename), "%s/buildroot%s/init.d/%s",
		     directory, SoftwareDir, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, 0, -1, -1))
	    return (1);
          break;
      case 'd' :
          snprintf(filename, sizeof(filename), "%s/buildroot%s", directory, file->dst);

	  if (Verbosity > 1)
	    printf("Directory %s...\n", filename);

          make_directory(filename, 0755, -1, -1);
          break;
      case 'l' :
          snprintf(filename, sizeof(filename), "%s/buildroot%s", directory, file->dst);

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
    puts("Building RPM binary distribution...");

  if (format == PACKAGE_LSB_SIGNED || format == PACKAGE_RPM_SIGNED)
    build_option = "-signed ";
  else
    build_option = "";

  if (!strcmp(platform->machine, "intel"))
  {
    if (run_command(NULL, EPM_RPMBUILD " -bb --buildroot \"%s/buildroot\" "
                          EPM_RPMARCH "i386 %s%s", absdir, build_option,
			  specname))
      return (1);
  }
  else if (!strcmp(platform->machine, "ppc"))
  {
    if (run_command(NULL, EPM_RPMBUILD " -bb --buildroot \"%s/buildroot\" "
                          EPM_RPMARCH "powerpc %s%s", absdir, build_option,
			  specname))
      return (1);
  }
  else if (run_command(NULL, EPM_RPMBUILD " -bb --buildroot \"%s/buildroot\" "
                       EPM_RPMARCH "%s %s%s", absdir, platform->machine,
		       build_option, specname))
    return (1);

 /*
  * Move the RPMs to the local directory and rename the RPMs using the
  * product name specified by the user...
  */

  move_rpms(prodname, directory, platname, dist, platform, rpmdir, NULL,
            release);

  for (i = 0; i < dist->num_subpackages; i ++)
    move_rpms(prodname, directory, platname, dist, platform, rpmdir,
              dist->subpackages[i], release);

 /*
  * Build a compressed tar file to hold all of the subpackages...
  */

  if (dist->num_subpackages || setup)
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

    snprintf(filename, sizeof(filename), "%s/%s.rpm.tgz", directory, name);

    if ((tarfile = tar_open(filename, 1)) == NULL)
      return (1);

   /*
    * Archive the setup and uninst GUIs and their data files...
    */

    if (setup)
    {
     /*
      * Include the ESP Software Installation Wizard (setup)...
      */

      const char	*setup_img;	/* Setup image name */
      struct stat	srcstat;	/* File information */


      if (stat(SetupProgram, &srcstat))
      {
	fprintf(stderr, "epm: Unable to stat GUI setup program %s - %s\n",
		SetupProgram, strerror(errno));
	tar_close(tarfile);
	return (-1);
      }

      if (tar_header(tarfile, TAR_NORMAL, 0555, srcstat.st_size,
	             srcstat.st_mtime, "root", "root", "setup", NULL) < 0)
      {
	fprintf(stderr, "epm: Error writing file header - %s\n",
		strerror(errno));
	tar_close(tarfile);
	return (-1);
      }

      if (tar_file(tarfile, SetupProgram) < 0)
      {
	fprintf(stderr, "epm: Error writing file data for setup -\n    %s\n",
		strerror(errno));
	tar_close(tarfile);
	return (-1);
      }

      if (Verbosity)
	printf("    %7.0fk setup\n", (srcstat.st_size + 1023) / 1024.0);

     /*
      * And the image file...
      */

      stat(setup, &srcstat);

      if (strlen(setup) > 4 && !strcmp(setup + strlen(setup) - 4, ".gif"))
	setup_img = "setup.gif";
      else
	setup_img = "setup.xpm";

      if (tar_header(tarfile, TAR_NORMAL, 0444, srcstat.st_size,
	             srcstat.st_mtime, "root", "root", setup_img, NULL) < 0)
      {
	fprintf(stderr, "epm: Error writing file header - %s\n",
		strerror(errno));
	tar_close(tarfile);
	return (-1);
      }

      if (tar_file(tarfile, setup) < 0)
      {
	fprintf(stderr, "epm: Error writing file data for %s -\n    %s\n",
		setup_img, strerror(errno));
	tar_close(tarfile);
	return (-1);
      }

      if (Verbosity)
	printf("    %7.0fk %s\n", (srcstat.st_size + 1023) / 1024.0, setup_img);

     /*
      * And the types file...
      */

      if (types)
      {
	stat(types, &srcstat);

	if (tar_header(tarfile, TAR_NORMAL, 0444, srcstat.st_size,
		       srcstat.st_mtime, "root", "root", types, NULL) < 0)
	{
	  fprintf(stderr, "epm: Error writing file header - %s\n",
		  strerror(errno));
          tar_close(tarfile);
	  return (-1);
	}

	if (tar_file(tarfile, types) < 0)
	{
	  fprintf(stderr, "epm: Error writing file data for setup.types -\n    %s\n",
		  strerror(errno));
          tar_close(tarfile);
	  return (-1);
	}

	if (Verbosity)
          printf("    %7.0fk setup.types\n", (srcstat.st_size + 1023) / 1024.0);
      }

     /*
      * Include the ESP Software Removal Wizard (uninst)...
      */

      if (stat(UninstProgram, &srcstat))
      {
	fprintf(stderr, "epm: Unable to stat GUI uninstall program %s - %s\n",
		UninstProgram, strerror(errno));
	tar_close(tarfile);
	return (-1);
      }

      if (tar_header(tarfile, TAR_NORMAL, 0555, srcstat.st_size,
	             srcstat.st_mtime, "root", "root", "uninst", NULL) < 0)
      {
	fprintf(stderr, "epm: Error writing file header - %s\n",
		strerror(errno));
	tar_close(tarfile);
	return (-1);
      }

      if (tar_file(tarfile, UninstProgram) < 0)
      {
	fprintf(stderr, "epm: Error writing file data for uninst -\n    %s\n",
		strerror(errno));
	tar_close(tarfile);
	return (-1);
      }

      if (Verbosity)
	printf("    %7.0fk uninst\n", (srcstat.st_size + 1023) / 1024.0);
    }

   /*
    * Archive the main package and subpackages...
    */

    if (tar_package(tarfile, "rpm", prodname, directory, platname, dist, NULL))
    {
      tar_close(tarfile);
      return (1);
    }

    for (i = 0; i < dist->num_subpackages; i ++)
    {
      if (tar_package(tarfile, "rpm", prodname, directory, platname, dist,
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

  if (!KeepFiles)
  {
    if (Verbosity)
      puts("Removing temporary distribution files...");

    snprintf(filename, sizeof(filename), "%s/RPMS", directory);
    unlink_directory(filename);

    snprintf(filename, sizeof(filename), "%s/rpms", directory);
    unlink(filename);

    snprintf(filename, sizeof(filename), "%s/buildroot", directory);
    unlink_directory(filename);

    unlink(specname);

    if (dist->num_subpackages)
    {
     /*
      * Remove .rpm files since they are now in a .tgz file...
      */

      unlink_package("rpm", prodname, directory, platname, dist, NULL);

      for (i = 0; i < dist->num_subpackages; i ++)
	unlink_package("rpm", prodname, directory, platname, dist,
                       dist->subpackages[i]);
    }
  }

  return (0);
}


/*
 * 'move_rpms()' - Move RPM packages to the build directory...
 */

static int				/* O - 0 = success, 1 = fail */
move_rpms(const char     *prodname,	/* I - Product short name */
          const char     *directory,	/* I - Directory for distribution files */
          const char     *platname,	/* I - Platform name */
          dist_t         *dist,		/* I - Distribution information */
	  struct utsname *platform,	/* I - Platform information */
	  const char     *rpmdir,	/* I - RPM directory */
          const char     *subpackage,	/* I - Subpackage name */
	  const char     *release)	/* I - Release: value */
{
  char		rpmname[1024];		/* RPM name */
  char		prodfull[1024];		/* Full product name */
  struct stat	rpminfo;		/* RPM file info */


 /*
  * Move the RPMs to the local directory and rename the RPMs using the
  * product name specified by the user...
  */

  if (subpackage)
    snprintf(prodfull, sizeof(prodfull), "%s-%s", prodname, subpackage);
  else
    strlcpy(prodfull, prodname, sizeof(prodfull));

  if (dist->release[0])
    snprintf(rpmname, sizeof(rpmname), "%s/%s-%s-%s", directory, prodfull,
             dist->version, dist->release);
  else
    snprintf(rpmname, sizeof(rpmname), "%s/%s-%s", directory, prodfull,
             dist->version);

  if (platname[0])
  {
    strlcat(rpmname, "-", sizeof(rpmname));
    strlcat(rpmname, platname, sizeof(rpmname));
  }

  strlcat(rpmname, ".rpm", sizeof(rpmname));

  if (!strcmp(platform->machine, "intel"))
    run_command(NULL, "/bin/mv %s/RPMS/i386/%s-%s-%s.i386.rpm %s",
		rpmdir, prodfull, dist->version, release,
		rpmname);
  else if (!strcmp(platform->machine, "ppc"))
    run_command(NULL, "/bin/mv %s/RPMS/powerpc/%s-%s-%s.powerpc.rpm %s",
		rpmdir, prodfull, dist->version, release,
		rpmname);
  else
    run_command(NULL, "/bin/mv %s/RPMS/%s/%s-%s-%s.%s.rpm %s",
		rpmdir, platform->machine, prodfull, dist->version,
		release, platform->machine, rpmname);

  if (Verbosity)
  {
    stat(rpmname, &rpminfo);

    printf("    %7.0fk  %s\n", rpminfo.st_size / 1024.0, rpmname);
  }

  return (0);
}


/*
 * 'write_spec()' - Write the subpackage-specific parts of the RPM spec file.
 */

static int				/* O - 0 on success, -1 on error */
write_spec(int        format,		/* I - Subformat */
           const char *prodname,	/* I - Product name */
	   dist_t     *dist,		/* I - Distribution */
           FILE       *fp,		/* I - Spec file */
           const char *subpackage)	/* I - Subpackage name */
{
  int		i;			/* Looping var */
  char		name[1024];		/* Full product name */
  const char	*product;		/* Product to depend on */
  file_t	*file;			/* Current distribution file */
  command_t	*c;			/* Current command */
  depend_t	*d;			/* Current dependency */
  const char	*runlevels;		/* Run levels */
  int		number;			/* Start/stop number */
  int		have_commands;		/* Have commands in current section? */


 /*
  * Get the name we'll use for the subpackage...
  */

  if (subpackage)
    snprintf(name, sizeof(name), " %s", subpackage);
  else
    name[0] = '\0';

 /*
  * Common stuff...
  */

  if (subpackage)
  {
    fprintf(fp, "%%package%s\n", name);
    fprintf(fp, "Summary: %s", dist->product);

    for (i = 0; i < dist->num_descriptions; i ++)
      if (dist->descriptions[i].subpackage == subpackage)
	break;

    if (i < dist->num_descriptions)
    {
      char	line[1024],		/* First line of description... */
		*ptr;			/* Pointer into line */


      strlcpy(line, dist->descriptions[i].description, sizeof(line));
      if ((ptr = strchr(line, '\n')) != NULL)
        *ptr = '\0';

      fprintf(fp, " - %s", line);
    }
    fputs("\n", fp);
  }
  else
    fprintf(fp, "Summary: %s\n", dist->product);

  fputs("Group: Applications\n", fp);

 /*
  * List all of the dependencies...
  */

  for (i = dist->num_depends, d = dist->depends; i > 0; i --, d ++)
  {
    if (d->subpackage != subpackage)
      continue;

    if (!strcmp(d->product, "_self"))
      product = prodname;
    else
      product = d->product;

    if (d->type == DEPEND_REQUIRES)
      fprintf(fp, "Requires: %s", product);
    else if (d->type == DEPEND_PROVIDES)
      fprintf(fp, "Provides: %s", product);
    else if (d->type == DEPEND_REPLACES)
      fprintf(fp, "Obsoletes: %s", product);
    else
      fprintf(fp, "Conflicts: %s", product);

    if (d->vernumber[0] == 0)
    {
      if (d->vernumber[1] < INT_MAX)
        fprintf(fp, " <= %s\n", d->version[1]);
      else
        putc('\n', fp);
    }
    else if (d->vernumber[0] && d->vernumber[1] < INT_MAX)
    {
      if (d->vernumber[0] < INT_MAX && d->vernumber[1] < INT_MAX)
        fprintf(fp, " >= %s, %s <= %s\n", d->version[0], product,
	        d->version[1]);
    }
    else if (d->vernumber[0] != d->vernumber[1])
      fprintf(fp, " >= %s\n", d->version[0]);
    else
      fprintf(fp, " = %s\n", d->version[0]);
  }

 /*
  * Pre/post install commands...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_INSTALL && c->subpackage == subpackage)
      break;

  if (i > 0)
  {
    fprintf(fp, "%%pre%s\n", name);
    for (; i > 0; i --, c ++)
      if (c->type == COMMAND_PRE_INSTALL && c->subpackage == subpackage)
	fprintf(fp, "%s\n", c->command);
  }

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_INSTALL && c->subpackage == subpackage)
      break;

  if (i > 0)
  {
    have_commands = 1;

    fprintf(fp, "%%post%s\n", name);
    for (; i > 0; i --, c ++)
      if (c->type == COMMAND_POST_INSTALL && c->subpackage == subpackage)
	fprintf(fp, "%s\n", c->command);

  }
  else
    have_commands = 0;

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_LITERAL && c->subpackage == subpackage &&
        !strcmp(c->section, "spec"))
      break;

  if (i > 0)
  {
    for (; i > 0; i --, c ++)
      if (c->type == COMMAND_LITERAL && c->subpackage == subpackage &&
	  !strcmp(c->section, "spec"))
	fprintf(fp, "%s\n", c->command);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i' && file->subpackage == subpackage)
      break;

  if (i)
  {
    if (!have_commands)
      fprintf(fp, "%%post%s\n", name);

    fputs("if test \"x$1\" = x1; then\n", fp);
    fputs("	echo Setting up init scripts...\n", fp);

    if (format == PACKAGE_LSB)
    {
     /*
      * Use LSB commands to install the init scripts...
      */

      for (; i > 0; i --, file ++)
	if (tolower(file->type) == 'i' && file->subpackage == subpackage)
	{
	  fprintf(fp, "	/usr/lib/lsb/install_initd /etc/init.d/%s\n", file->dst);
	  fprintf(fp, "	/etc/init.d/%s start\n", file->dst);
	}
    }
    else
    {
     /*
      * Find where the frigging init scripts go...
      */

      fputs("	rcdir=\"\"\n", fp);
      fputs("	for dir in /sbin/rc.d /sbin /etc/rc.d /etc ; do\n", fp);
      fputs("		if test -d $dir/rc3.d -o -h $dir/rc3.d; then\n", fp);
      fputs("			rcdir=\"$dir\"\n", fp);
      fputs("		fi\n", fp);
      fputs("	done\n", fp);
      fputs("	if test \"$rcdir\" = \"\" ; then\n", fp);
      fputs("		echo Unable to determine location of startup scripts!\n", fp);
      fputs("	else\n", fp);
      for (; i > 0; i --, file ++)
	if (tolower(file->type) == 'i' && file->subpackage == subpackage)
	{
	  fputs("		if test -d $rcdir/init.d; then\n", fp);
	  qprintf(fp, "			/bin/rm -f $rcdir/init.d/%s\n", file->dst);
	  qprintf(fp, "			/bin/ln -s %s/init.d/%s "
		      "$rcdir/init.d/%s\n", SoftwareDir, file->dst, file->dst);
	  fputs("		else\n", fp);
	  fputs("			if test -d /etc/init.d; then\n", fp);
	  qprintf(fp, "				/bin/rm -f /etc/init.d/%s\n", file->dst);
	  qprintf(fp, "				/bin/ln -s %s/init.d/%s "
		      "/etc/init.d/%s\n", SoftwareDir, file->dst, file->dst);
	  fputs("			fi\n", fp);
	  fputs("		fi\n", fp);

	  for (runlevels = get_runlevels(dist->files + i, "0123456");
	       isdigit(*runlevels & 255);
	       runlevels ++)
	  {
	    if (*runlevels == '0')
	      number = get_stop(file, 0);
	    else
	      number = get_start(file, 99);

	    qprintf(fp, "		/bin/rm -f $rcdir/rc%c.d/%c%02d%s\n", *runlevels,
		    (*runlevels == '0' || *runlevels == '1' ||
		     *runlevels == '6') ? 'K' : 'S', number, file->dst);
	    qprintf(fp, "		/bin/ln -s %s/init.d/%s "
			"$rcdir/rc%c.d/%c%02d%s\n", SoftwareDir, file->dst,
		    *runlevels,
		    (*runlevels == '0' || *runlevels == '1' ||
		     *runlevels == '6') ? 'K' : 'S', number, file->dst);
	  }

	  qprintf(fp, "		%s/init.d/%s start\n", SoftwareDir, file->dst);
	}

      fputs("	fi\n", fp);
    }

    fputs("fi\n", fp);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i' && file->subpackage == subpackage)
      break;

  if (i)
  {
    have_commands = 1;

    fprintf(fp, "%%preun%s\n", name);
    fputs("if test \"x$1\" = x0; then\n", fp);
    fputs("	echo Cleaning up init scripts...\n", fp);

    if (format == PACKAGE_LSB)
    {
     /*
      * Use LSB commands to remove the init scripts...
      */

      for (; i > 0; i --, file ++)
	if (tolower(file->type) == 'i' && file->subpackage == subpackage)
	{
	  fprintf(fp, "	/etc/init.d/%s stop\n", file->dst);
	  fprintf(fp, "	/usr/lib/lsb/remove_initd /etc/init.d/%s\n", file->dst);
	}
    }
    else
    {
     /*
      * Find where the frigging init scripts go...
      */

      fputs("	rcdir=\"\"\n", fp);
      fputs("	for dir in /sbin/rc.d /sbin /etc/rc.d /etc ; do\n", fp);
      fputs("		if test -d $dir/rc3.d -o -h $dir/rc3.d; then\n", fp);
      fputs("			rcdir=\"$dir\"\n", fp);
      fputs("		fi\n", fp);
      fputs("	done\n", fp);
      fputs("	if test \"$rcdir\" = \"\" ; then\n", fp);
      fputs("		echo Unable to determine location of startup scripts!\n", fp);
      fputs("	else\n", fp);
      for (; i > 0; i --, file ++)
	if (tolower(file->type) == 'i' && file->subpackage == subpackage)
	{
	  qprintf(fp, "		%s/init.d/%s stop\n", SoftwareDir, file->dst);

	  fputs("		if test -d $rcdir/init.d; then\n", fp);
	  qprintf(fp, "			/bin/rm -f $rcdir/init.d/%s\n", file->dst);
	  fputs("		else\n", fp);
	  fputs("			if test -d /etc/init.d; then\n", fp);
	  qprintf(fp, "				/bin/rm -f /etc/init.d/%s\n", file->dst);
	  fputs("			fi\n", fp);
	  fputs("		fi\n", fp);

	  for (runlevels = get_runlevels(dist->files + i, "0123456");
	       isdigit(*runlevels & 255);
	       runlevels ++)
	  {
	    if (*runlevels == '0')
	      number = get_stop(file, 0);
	    else
	      number = get_start(file, 99);

	    qprintf(fp, "		/bin/rm -f $rcdir/rc%c.d/%c%02d%s\n", *runlevels,
		    (*runlevels == '0' || *runlevels == '1' ||
		     *runlevels == '6') ? 'K' : 'S', number, file->dst);
	  }
	}

      fputs("	fi\n", fp);
    }

    fputs("fi\n", fp);
  }
  else
    have_commands = 0;

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_REMOVE && c->subpackage == subpackage)
      break;

  if (i > 0)
  {
    if (!have_commands)
      fprintf(fp, "%%preun%s\n", name);

    for (; i > 0; i --, c ++)
      if (c->type == COMMAND_PRE_REMOVE && c->subpackage == subpackage)
	fprintf(fp, "%s\n", c->command);
  }

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_REMOVE && c->subpackage == subpackage)
      break;

  if (i > 0)
  {
    fprintf(fp, "%%postun%s\n", name);
    for (; i > 0; i --, c ++)
      if (c->type == COMMAND_POST_REMOVE && c->subpackage == subpackage)
	fprintf(fp, "%s\n", c->command);
  }

 /*
  * Description...
  */

  for (i = 0; i < dist->num_descriptions; i ++)
    if (dist->descriptions[i].subpackage == subpackage)
      break;

  if (i < dist->num_descriptions)
  {
    fprintf(fp, "%%description %s\n", name);

    for (; i < dist->num_descriptions; i ++)
      if (dist->descriptions[i].subpackage == subpackage)
	fprintf(fp, "%s\n", dist->descriptions[i].description);
  }

 /*
  * Files...
  */

  fprintf(fp, "%%files%s\n", name);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (file->subpackage == subpackage)
      switch (tolower(file->type))
      {
	case 'c' :
            fprintf(fp, "%%attr(%04o,%s,%s) %%config(noreplace) \"%s\"\n",
	            file->mode, file->user, file->group, file->dst);
            break;
	case 'd' :
            fprintf(fp, "%%attr(%04o,%s,%s) %%dir \"%s\"\n", file->mode,
	            file->user, file->group, file->dst);
            break;
	case 'f' :
	case 'l' :
            fprintf(fp, "%%attr(%04o,%s,%s) \"%s\"\n", file->mode, file->user,
	            file->group, file->dst);
            break;
	case 'i' :
	    if (format == PACKAGE_LSB)
	      fprintf(fp, "%%attr(0555,root,root) \"/etc/init.d/%s\"\n",
		      file->dst);
            else
	      fprintf(fp, "%%attr(0555,root,root) \"%s/init.d/%s\"\n",
	              SoftwareDir, file->dst);
            break;
      }

  return (0);
}


/*
 * End of "$Id: rpm.c 840 2010-12-30 20:30:03Z mike $".
 */

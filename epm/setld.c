/*
 * "$Id: setld.c 833 2010-12-30 00:00:50Z mike $"
 *
 *   Tru64 package gateway for the ESP Package Manager (EPM)
 *
 *   Copyright 2001-2010 by Easy Software Products
 *   Copyright 2001 by Aneesh Kumar (aneesh.kumar@digital.com) at Digital India.
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
 *   make_setld() - Make a Tru64 setld package.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * 'make_setld()' - Make a Tru64 setld package.
 */

int					/* O - 0 = success, 1 = fail */
make_setld(const char     *prodname,	/* I - Product short name */
           const char     *directory,	/* I - Directory for distribution files */
           const char     *platname,	/* I - Platform name */
           dist_t         *dist,	/* I - Distribution information */
           struct utsname *platform)	/* I - Platform information */
{
  int		i, j;			/* Looping vars */
  FILE		*fp;			/* Spec file */
  tarf_t	*tarfile;		/* .tardist file */
  char		name[1024];		/* Full product name */
  char		scpname[1024];		/* XXXVVV control program filename */
  char		miname[1024];		/* XXXVVV.mi filename */
  char		keyname[1024];		/* XXXVVV.key filename */
  char		filename[1024];		/* Destination filename */
  char		subset[1024];		/* Subset name */
  file_t	*file;			/* Current distribution file */
  command_t	*c;			/* Current command */
  char		current[1024];		/* Current directory */
  struct passwd	*pwd;			/* Pointer to user record */
  struct group	*grp;			/* Pointer to group record */
  const char	*runlevels;		/* Run levels */


  REF(platform);

 /*
  * Check the package information to make sure it complies with the
  * extremely limited Tru64 packager requirements.
  */

  if (dist->vernumber < 100 || dist->vernumber > 999)
  {
    fprintf(stderr, "epm: Need a version number between 100 and 999 inclusive.\n"
                    "     The current version number (%d) is out of range.\n",
            dist->vernumber);
    return (1);
  }

  if (strlen(prodname) < 3)
  {
    fprintf(stderr, "epm: Need a product name of at least 3 uppercase characters.\n"
                    "     The current product name (%s) is not acceptable.\n",
            prodname);
    return (1);
  }

  for (i = 0; prodname[i]; i ++)
    if (!isupper(prodname[i] & 255))
    {
      fprintf(stderr, "epm: Need a product name of at least 3 uppercase characters.\n"
                      "     The current product name (%s) is not acceptable.\n",
              prodname);
      return (1);
    }

  for (i = 0; i < dist->num_subpackages; i ++)
  {
    if ((strlen(dist->subpackages[i]) + strlen(prodname)) > 77)
    {
      fprintf(stderr, "epm: Product + subpackage names must be less than 77 characters.\n"
                      "     The current subpackage name (%s) is not acceptable.\n",
              dist->subpackages[i]);
      return (1);
    }

    for (j = 0; dist->subpackages[i][j]; j ++)
      if (!isupper(dist->subpackages[i][j] & 255))
      {
	fprintf(stderr, "epm: Subpackage names may only contain uppercase letters.\n"
                	"     The current subpackage name (%s) is not acceptable.\n",
        	dist->subpackages[i]);
	return (1);
      }
  }

 /*
  * Prepare for packaging...
  */

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
  * Add symlinks for init scripts...
  */

  for (i = 0; i < dist->num_files; i ++)
    if (tolower(dist->files[i].type) == 'i')
    {
     /*
      * Make symlinks for all of the selected run levels...
      */

      for (runlevels = get_runlevels(dist->files + i, "023");
           isdigit(*runlevels & 255);
	   runlevels ++)
      {
	file = add_file(dist, dist->files[i].subpackage);
	file->type = 'l';
	file->mode = 0;
	strcpy(file->user, "root");
	strcpy(file->group, "sys");
	snprintf(file->src, sizeof(file->src), "../init.d/%s",
        	 dist->files[i].dst);

        if (*runlevels == '0')
	  snprintf(file->dst, sizeof(file->dst), "/sbin/rc0.d/K%02d%s",
        	   get_stop(dist->files + i, 0), dist->files[i].dst);
        else
	  snprintf(file->dst, sizeof(file->dst), "/sbin/rc%c.d/S%02d%s",
        	   *runlevels, get_start(dist->files + i, 99),
		   dist->files[i].dst);
      }

     /*
      * Then send the original file to /sbin/init.d...
      */

      file = dist->files + i;

      snprintf(filename, sizeof(filename), "/sbin/init.d/%s", file->dst);
      strcpy(file->dst, filename);
    }

 /*
  * Build package directories...
  *
  * Note: The src/scps directory and the key, inventory, and
  *       subset control programs below should actually go in
  *       a data subdirectory according to the Tru64
  *       documentation, but (at least under 4.0d) the invcutter
  *       program will not be able to find the distribution files
  *       if they are not colocated with these data files.
  */

  if (Verbosity)
    puts("Creating Tru64 (setld) distribution...");

  snprintf(filename, sizeof(filename), "%s/output", directory);
  unlink_directory(filename);

  snprintf(filename, sizeof(filename), "%s/src", directory);
  unlink_directory(filename);

  snprintf(filename, sizeof(filename), "%s/output", directory);
  mkdir(filename, 0777);

  snprintf(filename, sizeof(filename), "%s/src", directory);
  mkdir(filename, 0777);

  snprintf(filename, sizeof(filename), "%s/src/scps", directory);
  mkdir(filename, 0777);

 /*
  * Write the subset control program...
  */

  if (Verbosity)
    puts("Creating subset control program...");

  snprintf(scpname, sizeof(scpname), "%s/src/scps/%sALL%03d.scp", directory,
           prodname, dist->vernumber);

  if ((fp = fopen(scpname, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create subset control program \"%s\" - %s\n",
            scpname, strerror(errno));
    return (1);
  }

  fputs("#!/bin/sh\n", fp);
  fputs("case $ACT in\n", fp);

  fputs("PRE_L)\n", fp);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
    {
      qprintf(fp, "if test -x %s; then\n", file->dst);
      qprintf(fp, "	%s stop\n", file->dst);
      fputs("fi\n", fp);
    }
  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_INSTALL)
      fprintf(fp, "%s\n", c->command);
  fputs(";;\n", fp);

  fputs("POST_L)\n", fp);
  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_INSTALL)
      fprintf(fp, "%s\n", c->command);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'c')
    {
      qprintf(fp, "if test ! -f %s; then\n", file->dst);
      qprintf(fp, "	/bin/cp %s.N %s\n", file->dst, file->dst);
      fputs("fi\n", fp);
    }
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
      qprintf(fp, "%s start\n", file->dst);
  fputs(";;\n", fp);

  fputs("PRE_D)\n", fp);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
      qprintf(fp, "%s stop\n", file->dst);
  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_REMOVE)
      fprintf(fp, "%s\n", c->command);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'c')
    {
      qprintf(fp, "if cmp -s %s.N %s; then\n", file->dst, file->dst);
      qprintf(fp, "	/bin/rm -f %s\n", file->dst);
      fputs("fi\n", fp);
    }
  fputs(";;\n", fp);

  fputs("POST_D)\n", fp);
  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_REMOVE)
      fprintf(fp, "%s\n", c->command);
  fputs(";;\n", fp);

  fputs("esac\n", fp);

  fclose(fp);

 /*
  * Sort the file list by the destination name, since kits need a sorted
  * list...
  */

  sort_dist_files(dist);

 /*
  * Now do the inventory file...
  */

  if (Verbosity)
    puts("Creating master inventory file...");

  snprintf(miname, sizeof(miname), "%s/src/%s%03d.mi", directory, prodname, dist->vernumber);

  if ((fp = fopen(miname, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create master inventory file \"%s\" - %s\n",
            miname, strerror(errno));
    return (1);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
  {
    snprintf(subset, sizeof(subset), "%s%s%03d", prodname,
             file->subpackage ? file->subpackage : "ALL", dist->vernumber);

    switch (tolower(file->type))
    {
      case 'c' :
          fprintf(fp, "2\t.%s.N\t%s\n", file->dst, subset);
	  break;
      case 'd' :
      case 'i' :
      case 'f' :
      case 'l' :
          fprintf(fp, "0\t.%s\t%s\n", file->dst, subset);
	  break;
    }
  }

  fclose(fp);

 /*
  * Create the key file...
  */

  if (Verbosity)
    puts("Creating key file...");

  snprintf(keyname, sizeof(keyname), "%s/src/%s%03d.k", directory, prodname,
           dist->vernumber);

  if ((fp = fopen(keyname, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create key file \"%s\" - %s\n",
            keyname, strerror(errno));
    return (1);
  }

  qprintf(fp, "NAME='%s, %s'\n", dist->product, dist->version);
  fprintf(fp, "CODE=%s\n", prodname);
  fprintf(fp, "VER=%03d\n", dist->vernumber);
  fprintf(fp, "MI=%s%03d.mi\n", prodname, dist->vernumber);
  fputs("COMPRESS=0\n", fp);
  fputs("%%\n", fp);
  qprintf(fp, "%sALL%03d\t.\t0\t'%s, %s'\n", prodname, dist->vernumber,
          dist->product, dist->version);
  for (i = 0; i < dist->num_subpackages; i ++)
    qprintf(fp, "%s%s%03d\t.\t0\t'%s, %s'\n", prodname, dist->subpackages[i],
            dist->vernumber, dist->product, dist->version);
  fclose(fp);

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
      case 'i' :
          snprintf(filename, sizeof(filename), "%s/src%s", directory, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;

      case 'd' :
          snprintf(filename, sizeof(filename), "%s/src%s", directory, file->dst);

	  if (Verbosity > 1)
	    printf("Directory %s...\n", filename);

          make_directory(filename, file->mode, pwd ? pwd->pw_uid : 0,
			 grp ? grp->gr_gid : 0);
          break;

      case 'l' :
          snprintf(filename, sizeof(filename), "%s/src%s", directory, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

          make_link(filename, file->src);
          break;
    }
  }

 /*
  * Build the distribution from the inventory and control files...
  */

  if (Verbosity)
    puts("Building Tru64 (setld) distribution...");

  snprintf(filename, sizeof(filename), "%s/src", directory);
  chdir(filename);

  if (run_command(NULL, "kits %s%03d.k . ../output", prodname, dist->vernumber))
    return (1);

  chdir(current);

 /*
  * Tar and compress the distribution...
  */

  if (Verbosity)
    puts("Creating tar.gz file for distribution...");

  snprintf(filename, sizeof(filename), "%s/%s.tar.gz", directory, name);

  if ((tarfile = tar_open(filename, 1)) == NULL)
    return (1);

  snprintf(filename, sizeof(filename), "%s/output", directory);

  if (tar_directory(tarfile, filename, prodname))
  {
    tar_close(tarfile);
    return (1);
  }

  tar_close(tarfile);

 /*
  * Remove temporary files...
  */

  if (!KeepFiles)
  {
    if (Verbosity)
      puts("Removing temporary distribution files...");

    snprintf(filename, sizeof(filename), "%s/output", directory);
    unlink_directory(filename);

    snprintf(filename, sizeof(filename), "%s/src", directory);
    unlink_directory(filename);
  }

  return (0);
}


/*
 * End of "$Id: setld.c 833 2010-12-30 00:00:50Z mike $".
 */

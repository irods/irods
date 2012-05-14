/*
 * "$Id: swinstall.c 829 2010-12-29 17:39:59Z mike $"
 *
 *   HP-UX package gateway for the ESP Package Manager (EPM).
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
 *   make_swinstall() - Make an HP-UX software distribution package.
 *   write_fileset()  - Write a fileset specification for a subpackage.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * Local functions...
 */

static void	write_fileset(FILE *fp, dist_t *dist, const char *directory,
		              const char *prodname, const char *subpackage);


/*
 * 'make_swinstall()' - Make an HP-UX software distribution package.
 */

int					/* O - 0 = success, 1 = fail */
make_swinstall(const char     *prodname,/* I - Product short name */
               const char     *directory,
					/* I - Directory for distribution files */
               const char     *platname,/* I - Platform name */
               dist_t         *dist,	/* I - Distribution information */
	       struct utsname *platform)/* I - Platform information */
{
  int		i, j;			/* Looping vars */
  FILE		*fp;			/* Spec/script file */
  tarf_t	*tarfile;		/* .tardist file */
  int		linknum;		/* Symlink number */
  const char	*vendor;		/* Pointer into vendor name */
  char		vtag[65],		/* Vendor tag */
		*vtagptr;		/* Pointer into vendor tag */
  char		name[1024];		/* Full product name */
  char		infoname[1024],		/* Info filename */
		preinstall[1024],	/* preinstall script */
		postinstall[1024],	/* postinstall script */
		preremove[1024],	/* preremove script */
		postremove[1024];	/* postremove script */
  char		filename[1024];		/* Destination filename */
  file_t	*file;			/* Current distribution file */
  command_t	*c;			/* Current command */
  const char	*runlevels;		/* Run levels */


  REF(platform);

  if (Verbosity)
    puts("Creating swinstall distribution...");

  if (dist->release[0])
  {
    if (platname[0])
      snprintf(name, sizeof(name), "%s-%s-%s-%s", prodname,
               dist->version, dist->release, platname);
    else
      snprintf(name, sizeof(name), "%s-%s-%s", prodname,
               dist->version, dist->release);
  }
  else if (platname[0])
    snprintf(name, sizeof(name), "%s-%s-%s", prodname, dist->version, platname);
  else
    snprintf(name, sizeof(name), "%s-%s", prodname, dist->version);

 /*
  * Write the preinstall script if needed...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_INSTALL)
      break;

  if (i)
  {
   /*
    * Create the preinstall script...
    */

    snprintf(preinstall, sizeof(preinstall), "%s/%s.preinst", directory,
             prodname);

    if (Verbosity)
      puts("Creating preinstall script...");

    if ((fp = fopen(preinstall, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", preinstall,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
      if (c->type == COMMAND_PRE_INSTALL)
        fprintf(fp, "%s\n", c->command);

    fclose(fp);
  }
  else
    preinstall[0] = '\0';

 /*
  * Write the postinstall script if needed...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_INSTALL)
      break;

  if (!i)
    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
        break;

  if (i)
  {
   /*
    * Create the postinstall script...
    */

    snprintf(postinstall, sizeof(postinstall), "%s/%s.postinst", directory,
             prodname);

    if (Verbosity)
      puts("Creating postinstall script...");

    if ((fp = fopen(postinstall, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", postinstall,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
      if (c->type == COMMAND_POST_INSTALL)
        fprintf(fp, "%s\n", c->command);

    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
	qprintf(fp, "/sbin/init.d/%s start\n", file->dst);

    fclose(fp);
  }
  else
    postinstall[0] = '\0';

 /*
  * Write the preremove script if needed...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_REMOVE)
      break;

  if (!i)
    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
        break;

  if (i)
  {
   /*
    * Then create the remove script...
    */

    if (Verbosity)
      puts("Creating preremove script...");

    snprintf(preremove, sizeof(preremove), "%s/%s.prerm", directory, prodname);

    if ((fp = fopen(preremove, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", preremove,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (j = dist->num_files, file = dist->files; j > 0; j --, file ++)
      if (tolower(file->type) == 'i')
	qprintf(fp, "/sbin/init.d/%s stop\n", file->dst);

    for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
      if (c->type == COMMAND_PRE_REMOVE)
        fprintf(fp, "%s\n", c->command);

    fclose(fp);
  }
  else
    preremove[0] = '\0';

 /*
  * Write the postremove script if needed...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_REMOVE)
      break;

  if (i)
  {
   /*
    * Create the postremove script...
    */

    snprintf(postremove, sizeof(postremove), "%s/%s.postrm", directory,
             prodname);

    if (Verbosity)
      puts("Creating postremove script...");

    if ((fp = fopen(postremove, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", postremove,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
      if (c->type == COMMAND_POST_REMOVE)
        fprintf(fp, "%s\n", c->command);

    fclose(fp);
  }
  else
    postremove[0] = '\0';

 /*
  * Add symlinks for init scripts...
  */

  for (i = 0; i < dist->num_files; i ++)
    if (tolower(dist->files[i].type) == 'i')
    {
     /*
      * Make symlinks for all of the selected run levels...
      */

      for (runlevels = get_runlevels(dist->files + i, "02");
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
	  snprintf(file->dst, sizeof(file->dst), "/sbin/rc0.d/K%02d0%s",
        	   get_stop(dist->files + i, 0), dist->files[i].dst);
        else
	  snprintf(file->dst, sizeof(file->dst), "/sbin/rc%c.d/S%02d0%s",
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
  * Create all symlinks...
  */

  if (Verbosity)
    puts("Creating symlinks...");

  for (i = dist->num_files, file = dist->files, linknum = 0;
       i > 0;
       i --, file ++)
    if (tolower(file->type) == 'l')
    {
      snprintf(filename, sizeof(filename), "%s/%s.link%04d", directory,
               prodname, linknum);
      symlink(file->src, filename);
      linknum ++;
    }

 /*
  * Write the description file(s) for swpackage...
  */

  if (dist->num_descriptions > 0)
  {
    if (Verbosity)
      puts("Creating description file(s)...");

    for (i = 0; i < dist->num_descriptions; i ++)
      if (!dist->descriptions[i].subpackage)
        break;

    if (i < dist->num_descriptions)
    {
      snprintf(filename, sizeof(filename), "%s/%s.desc", directory, prodname);

      if ((fp = fopen(filename, "w")) == NULL)
      {
	fprintf(stderr, "epm: Unable to create description file \"%s\" - %s\n",
        	filename, strerror(errno));
	return (1);
      }

      for (; i < dist->num_descriptions; i ++)
	if (!dist->descriptions[i].subpackage)
	  fprintf(fp, "%s\n", dist->descriptions[i].description);

      fclose(fp);
    }

    for (i = 0; i < dist->num_subpackages; i ++)
    {
      for (j = 0; j < dist->num_descriptions; j ++)
	if (dist->descriptions[j].subpackage == dist->subpackages[i])
          break;

      if (j < dist->num_descriptions)
      {
	snprintf(filename, sizeof(filename), "%s/%s-%s.desc", directory,
        	 prodname, dist->subpackages[i]);

	if ((fp = fopen(filename, "w")) == NULL)
	{
	  fprintf(stderr, "epm: Unable to create description file \"%s\" - %s\n",
        	  filename, strerror(errno));
	  return (1);
	}

	for (; j < dist->num_descriptions; j ++)
	  if (dist->descriptions[j].subpackage == dist->subpackages[i])
	    fprintf(fp, "%s\n", dist->descriptions[j].description);

	fclose(fp);
      }
    }
  }

 /*
  * Write the info file for swpackage...
  */

  if (Verbosity)
    puts("Creating info file...");

  snprintf(infoname, sizeof(infoname), "%s/%s.info", directory, prodname);

  if ((fp = fopen(infoname, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create info file \"%s\" - %s\n", infoname,
            strerror(errno));
    return (1);
  }

 /*
  * Figure out the vendor tag and write a vendor class...
  */

  for (vendor = dist->vendor, vtagptr = vtag; *vendor; vendor ++)
    if (isalnum(*vendor) && vtagptr < (vtag + sizeof(vtag) - 1))
      *vtagptr++ = *vendor;

  *vtagptr = '\0';

  fputs("vendor\n", fp);
  fprintf(fp, "  tag %s\n", vtag);
  fprintf(fp, "  description %s\n", dist->vendor);
  fprintf(fp, "  title %s\n", dist->vendor);
  fputs("end\n", fp);

 /*
  * Then the product class...
  */

  fputs("product\n", fp);
  fprintf(fp, "  tag %s\n", prodname);
  fprintf(fp, "  revision %s\n", dist->version);
  fprintf(fp, "  title %s, %s\n", dist->product, dist->version);

  snprintf(filename, sizeof(filename), "%s/%s.desc", directory, prodname);
  if (!access(filename, 0))
    fprintf(fp, "  description < %s\n", filename);
  if (dist->license[0])
  {
    if (strncmp(dist->license, "./", 2))
      fprintf(fp, "  copyright < %s\n", dist->license);
    else
      fprintf(fp, "  copyright < %s\n", dist->license + 2);
  }
  if (dist->readme[0])
  {
    if (strncmp(dist->readme, "./", 2))
      fprintf(fp, "  readme < %s\n", dist->readme);
    else
      fprintf(fp, "  readme < %s\n", dist->readme + 2);
  }
  fprintf(fp, "  vendor_tag %s\n", vtag);
  fputs("  is_locatable false\n", fp);

  if (dist->num_subpackages > 0)
  {
   /*
    * Write subproduct specifications...
    */

    fputs("  subproduct\n", fp);
    fputs("    tag base\n", fp);
    fputs("    contents fs_base\n", fp);
    fprintf(fp, "    revision %s\n", dist->version);

    for (i = 0; i < dist->num_descriptions; i ++)
      if (!dist->descriptions[i].subpackage)
      {
        fprintf(fp, "    title %s, %s\n", dist->descriptions[i].description,
	        dist->version);
        break;
      }

    if (!access(filename, 0))
      fprintf(fp, "    description < %s\n", filename);
    fputs("  end\n", fp);

    for (i = 0; i < dist->num_subpackages; i ++)
    {
      fputs("  subproduct\n", fp);
      fprintf(fp, "    tag %s\n", dist->subpackages[i]);
      fprintf(fp, "    contents fs_%s\n", dist->subpackages[i]);
      fprintf(fp, "    revision %s\n", dist->version);

      for (j = 0; j < dist->num_descriptions; j ++)
	if (dist->descriptions[j].subpackage == dist->subpackages[i])
	{
          fprintf(fp, "    title %s, %s\n", dist->descriptions[j].description,
	          dist->version);
          break;
	}

      snprintf(filename, sizeof(filename), "%s/%s-%s.desc", directory,
               prodname, dist->subpackages[i]);
      if (!access(filename, 0))
	fprintf(fp, "    description < %s\n", filename);
      fputs("  end\n", fp);
    }
  }

 /*
  * Write filesets...
  */

  write_fileset(fp, dist, directory, prodname, NULL);
  for (i = 0; i < dist->num_subpackages; i ++)
    write_fileset(fp, dist, directory, prodname, dist->subpackages[i]);

  fputs("end\n", fp);

  fclose(fp);

 /*
  * Build the distributions from the spec file...
  */

  if (Verbosity)
    puts("Building swinstall depot.gz distribution...");

  snprintf(filename, sizeof(filename), "%s/%s", directory, prodname);
  mkdir(filename, 0777);

  if (run_command(NULL, "/usr/sbin/swpackage %s-s %s "
                        "-x target_type=tape "
			"-d '|" EPM_GZIP " -9 >%s/%s.depot.gz' %s",
        	  Verbosity == 0 ? "" : "-v ", infoname, directory,
		  name, prodname))
    return (1);

  if (Verbosity)
    puts("Building swinstall binary distribution...");

  snprintf(filename, sizeof(filename), "%s/%s", directory, prodname);
  mkdir(filename, 0777);

  if (run_command(NULL, "/usr/sbin/swpackage %s-s %s -d %s/%s "
                        "-x write_remote_files=true %s",
        	  Verbosity == 0 ? "" : "-v ", infoname, directory,
		  prodname, prodname))
    return (1);

 /*
  * Tar and compress the distribution...
  */

  if (Verbosity)
    puts("Creating depot.tgz file for distribution...");

  snprintf(filename, sizeof(filename), "%s/%s.depot.tgz", directory, name);

  if ((tarfile = tar_open(filename, 1)) == NULL)
    return (1);

  snprintf(filename, sizeof(filename), "%s/%s", directory, prodname);

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

    unlink(infoname);

    if (preinstall[0])
      unlink(preinstall);
    if (postinstall[0])
      unlink(postinstall);
    if (preremove[0])
      unlink(preremove);
    if (postremove[0])
      unlink(postremove);

    while (linknum > 0)
    {
      linknum --;
      snprintf(filename, sizeof(filename), "%s/%s.link%04d", directory,
               prodname, linknum);
      unlink(filename);
    }

    snprintf(filename, sizeof(filename), "%s/%s.desc", directory, prodname);
    unlink(filename);

    for (i = 0; i < dist->num_subpackages; i ++)
    {
      snprintf(filename, sizeof(filename), "%s/%s-%s.desc", directory,
               prodname, dist->subpackages[i]);
      unlink(filename);
    }
  }

  return (0);
}


/*
 * 'write_fileset()' - Write a fileset specification for a subpackage.
 */

static void
write_fileset(FILE       *fp,		/* I - File to write to */
              dist_t     *dist,		/* I - Distribution */
	      const char *directory,	/* I - Output directory */
	      const char *prodname,	/* I - Product name */
	      const char *subpackage)	/* I - Subpackage to write */
{
  int		i;			/* Looping var */
  char		filename[1024];		/* Temporary filename */
  depend_t	*d;			/* Current dependency */
  file_t	*file;			/* Current distribution file */
  int		linknum;		/* Symlink number */


  fputs("  fileset\n", fp);
  fprintf(fp, "    tag fs_%s\n", subpackage ? subpackage : "base");
  fprintf(fp, "    revision %s\n", dist->version);

  for (i = 0; i < dist->num_descriptions; i ++)
    if (dist->descriptions[i].subpackage == subpackage)
    {
      fprintf(fp, "    title %s, %s\n", dist->descriptions[i].description,
              dist->version);
      break;
    }

  for (i = dist->num_depends, d = dist->depends; i > 0; i --, d ++)
    if (d->type == DEPEND_REQUIRES && d->product[0] != '/' &&
        d->subpackage == subpackage)
      break;

  if (i)
  {
    for (; i > 0; i --, d ++)
    {
      if (d->type == DEPEND_REQUIRES && d->product[0] != '/' &&
          d->subpackage == subpackage)
      {
        if (!strcmp(d->product, "_self"))
	  fprintf(fp, "    prerequisites %s", prodname);
	else
	  fprintf(fp, "    prerequisites %s", d->product);

	if (d->vernumber[0] == 0)
	{
	  if (d->vernumber[1] < INT_MAX)
            fprintf(fp, ",r<=%s\n", d->version[1]);
	  else
	    putc('\n', fp);
	}
	else
	  fprintf(fp, ",r>=%s,r<=%s\n", d->version[0], d->version[1]);
      }
    }
  }

  for (i = dist->num_depends, d = dist->depends; i > 0; i --, d ++)
    if (d->type == DEPEND_REPLACES && d->product[0] != '/' &&
        d->subpackage == subpackage)
      break;

  if (i)
  {
    for (; i > 0; i --, d ++)
    {
      if (d->type == DEPEND_REPLACES && d->product[0] != '/' &&
          d->subpackage == subpackage)
      {
        fprintf(fp, "    ancestor %s", d->product);
	if (d->vernumber[0] == 0)
	{
	  if (d->vernumber[1] < INT_MAX)
            fprintf(fp, ",r<=%s\n", d->version[1]);
	  else
	    putc('\n', fp);
	}
	else
	  fprintf(fp, ",r>=%s,r<=%s\n", d->version[0], d->version[1]);
      }
    }
  }

  if (!subpackage)
  {
   /*
    * Write scripts...
    */

    snprintf(filename, sizeof(filename), "%s/%s.preinst", directory, prodname);
    if (!access(filename, 0))
      fprintf(fp, "    preinstall %s\n", filename);

    snprintf(filename, sizeof(filename), "%s/%s.postinst", directory, prodname);
    if (!access(filename, 0))
      fprintf(fp, "    postinstall %s\n", filename);

    snprintf(filename, sizeof(filename), "%s/%s.prerm", directory, prodname);
    if (!access(filename, 0))
      fprintf(fp, "    preremove %s\n", filename);

    snprintf(filename, sizeof(filename), "%s/%s.postrm", directory, prodname);
    if (!access(filename, 0))
      fprintf(fp, "    postremove %s\n", filename);
  }

  for (i = dist->num_files, file = dist->files, linknum = 0;
       i > 0;
       i --, file ++)
  {
    if (file->subpackage != subpackage)
    {
      if (tolower(file->type) == 'l')
        linknum ++;

      continue;
    }

    switch (tolower(file->type))
    {
      case 'd' :
          qprintf(fp, "    file -m %04o -o %s -g %s . %s\n", file->mode,
	          file->user, file->group, file->dst);
          break;
      case 'c' :
          qprintf(fp, "    file -m %04o -o %s -g %s -v %s %s\n", file->mode,
	          file->user, file->group, file->src, file->dst);
          break;
      case 'f' :
      case 'i' :
          qprintf(fp, "    file -m %04o -o %s -g %s %s %s\n", file->mode,
	          file->user, file->group, file->src, file->dst);
          break;
      case 'l' :
          snprintf(filename, sizeof(filename), "%s/%s.link%04d", directory,
	           prodname, linknum);
          linknum ++;
          qprintf(fp, "    file -o %s -g %s %s %s\n", file->user, file->group,
	          filename, file->dst);
          break;
    }
  }

  fputs("  end\n", fp);
}


/*
 * End of "$Id: swinstall.c 829 2010-12-29 17:39:59Z mike $".
 */

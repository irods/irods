/*
 * "$Id: inst.c 670 2006-04-05 15:08:08Z mike $"
 *
 *   IRIX package gateway for the ESP Package Manager (EPM).
 *
 *   Copyright 1999-2006 by Easy Software Products.
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
 *   make_inst()   - Make an IRIX software distribution package.
 *   inst_subsys() - Write a subsystem definition for the product.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * Local functions...
 */

void	inst_subsys(FILE *fp, const char *prodname, dist_t *dist,
		    const char *subpackage, const char *category,
		    const char *section);


/*
 * 'make_inst()' - Make an IRIX software distribution package.
 */

int					/* O - 0 = success, 1 = fail */
make_inst(const char     *prodname,	/* I - Product short name */
          const char     *directory,	/* I - Directory for distribution files */
          const char     *platname,	/* I - Platform name */
          dist_t         *dist,		/* I - Distribution information */
	  struct utsname *platform)	/* I - Platform information */
{
  int		i;			/* Looping var */
  FILE		*fp;			/* Spec/IDB/script file */
  tarf_t	*tarfile;		/* .tardist file */
  char		name[1024];		/* Full product name */
  char		specname[1024];		/* Spec filename */
  char		idbname[1024];		/* IDB filename */
  char		filename[1024],		/* Destination filename */
		srcname[1024],		/* Name of source file in distribution */
		dstname[1024];		/* Name of destination file in distribution */
  char		preinstall[1024],	/* Pre install script */
		postinstall[1024],	/* Post install script */
		preremove[1024],	/* Pre remove script */
		postremove[1024];	/* Post remove script */
  char		subsys[255];		/* Subsystem name */
  file_t	*file;			/* Current distribution file */
  command_t	*c;			/* Current command */
  struct stat	fileinfo;		/* File information */
  const char	*runlevels;		/* Run levels */
  static const char *extensions[] =	/* INST file extensions */
		{
		  "",
		  ".idb",
		  ".man",
		  ".sw"
		};


  REF(platform);

  if (Verbosity)
    puts("Creating inst distribution...");

  if (dist->release[0])
  {
    if (platname[0])
      snprintf(name, sizeof(name), "%s-%s-%s-%s", prodname, dist->version, dist->release,
              platname);
    else
      snprintf(name, sizeof(name), "%s-%s-%s", prodname, dist->version, dist->release);
  }
  else if (platname[0])
    snprintf(name, sizeof(name), "%s-%s-%s", prodname, dist->version, platname);
  else
    snprintf(name, sizeof(name), "%s-%s", prodname, dist->version);

 /*
  * Write the spec file for gendist...
  */

  if (Verbosity)
    puts("Creating spec file...");

  snprintf(specname, sizeof(specname), "%s/spec", directory);

  if ((fp = fopen(specname, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create spec file \"%s\" - %s\n", specname,
            strerror(errno));
    return (1);
  }

  fprintf(fp, "product %s\n", prodname);
  qprintf(fp, "	id \"%s, %s\"\n", dist->product, dist->version);

  fputs("	image sw\n", fp);
  qprintf(fp, "		id \"%s, Software, %s\"\n", dist->product,
          dist->version);
  fprintf(fp, "		version %d\n", dist->vernumber);

  inst_subsys(fp, prodname, dist, NULL, "Software", "sw");
  for (i = 0; i < dist->num_subpackages; i ++)
    inst_subsys(fp, prodname, dist, dist->subpackages[i], "Software", "sw");

  fputs("	endimage\n", fp);

  fputs("	image man\n", fp);
  qprintf(fp, "		id \"%s, Man Pages, %s\"\n", dist->product,
          dist->version);
  fprintf(fp, "		version %d\n", dist->vernumber);

  inst_subsys(fp, prodname, dist, NULL, "Man Pages", "man");
  for (i = 0; i < dist->num_subpackages; i ++)
    inst_subsys(fp, prodname, dist, dist->subpackages[i], "Man Pages", "man");

  fputs("	endimage\n", fp);

  fputs("endproduct\n", fp);

  fclose(fp);

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
	  snprintf(file->dst, sizeof(file->dst), "/etc/rc0.d/K%02d%s",
        	   get_stop(dist->files + i, 0), dist->files[i].dst);
        else
	  snprintf(file->dst, sizeof(file->dst), "/etc/rc%c.d/S%02d%s",
        	   *runlevels, get_start(dist->files + i, 99),
		   dist->files[i].dst);
      }

     /*
      * Then send the original file to /etc/init.d...
      */

      file = dist->files + i;

      snprintf(filename, sizeof(filename), "/etc/init.d/%s", file->dst);
      strcpy(file->dst, filename);
    }

 /*
  * Add preinstall script as needed...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_INSTALL)
      break;

  if (i)
  {
   /*
    * Add the preinstall script file to the list...
    */

    file = add_file(dist, NULL);
    file->type = '1';
    file->mode = 0555;
    strcpy(file->user, "root");
    strcpy(file->group, "sys");
    snprintf(file->src, sizeof(file->src), "%s/%s.preinstall", directory,
             prodname);
    snprintf(file->dst, sizeof(file->dst), "%s/%s.preinstall", SoftwareDir,
             prodname);

   /*
    * Then create the install script...
    */

    if (Verbosity)
      puts("Creating preinstall script...");

    snprintf(preinstall, sizeof(preinstall), "%s/%s.preinstall", directory, prodname);

    if ((fp = fopen(preinstall, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", preinstall,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (; i > 0; i --, c ++)
      if (c->type == COMMAND_PRE_INSTALL)
        fprintf(fp, "%s\n", c->command);

    fclose(fp);
  }
  else
    preinstall[0] = '\0';

 /*
  * Add postinstall script as needed...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_INSTALL)
      break;

  if (i)
  {
   /*
    * Add the postinstall script file to the list...
    */

    file = add_file(dist, NULL);
    file->type = '2';
    file->mode = 0555;
    strcpy(file->user, "root");
    strcpy(file->group, "sys");
    snprintf(file->src, sizeof(file->src), "%s/%s.postinstall", directory,
             prodname);
    snprintf(file->dst, sizeof(file->dst), "%s/%s.postinstall", SoftwareDir,
             prodname);

   /*
    * Then create the install script...
    */

    if (Verbosity)
      puts("Creating postinstall script...");

    snprintf(postinstall, sizeof(postinstall), "%s/%s.postinstall", directory, prodname);

    if ((fp = fopen(postinstall, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", postinstall,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (; i > 0; i --, c ++)
      if (c->type == COMMAND_POST_INSTALL)
        fprintf(fp, "%s\n", c->command);

    fclose(fp);
  }
  else
    postinstall[0] = '\0';

 /*
  * Add preremove script as needed...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_REMOVE)
      break;

  if (i)
  {
   /*
    * Add the preremove script file to the list...
    */

    file = add_file(dist, NULL);
    file->type = '3';
    file->mode = 0555;
    strcpy(file->user, "root");
    strcpy(file->group, "sys");
    snprintf(file->src, sizeof(file->src), "%s/%s.preremove", directory,
             prodname);
    snprintf(file->dst, sizeof(file->dst), "%s/%s.preremove", SoftwareDir,
             prodname);

   /*
    * Then create the install script...
    */

    if (Verbosity)
      puts("Creating preremove script...");

    snprintf(preremove, sizeof(preremove), "%s/%s.preremove", directory, prodname);

    if ((fp = fopen(preremove, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", preremove,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (; i > 0; i --, c ++)
      if (c->type == COMMAND_PRE_REMOVE)
        fprintf(fp, "%s\n", c->command);

    qprintf(fp, "/bin/rm -f %s.copy\n", file->dst);

    fclose(fp);
  }
  else
    preremove[0] = '\0';

 /*
  * Add postremove script as needed...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_REMOVE)
      break;

  if (i)
  {
   /*
    * Add the postremove script file to the list...
    */

    file = add_file(dist, NULL);
    file->type = '4';
    file->mode = 0555;
    strcpy(file->user, "root");
    strcpy(file->group, "sys");
    snprintf(file->src, sizeof(file->src), "%s/%s.postremove", directory,
             prodname);
    snprintf(file->dst, sizeof(file->dst), "%s/%s.postremove", SoftwareDir,
             prodname);

   /*
    * Then create the remove script...
    */

    if (Verbosity)
      puts("Creating postremove script...");

    snprintf(postremove, sizeof(postremove), "%s/%s.postremove", directory, prodname);

    if ((fp = fopen(postremove, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", postremove,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (; i > 0; i --, c ++)
      if (c->type == COMMAND_POST_REMOVE)
        fprintf(fp, "%s\n", c->command);

    qprintf(fp, "/bin/rm -f %s.copy\n", file->dst);

    fclose(fp);
  }
  else
    postremove[0] = '\0';

 /*
  * Sort the file list by the destination name, since gendist needs a sorted
  * list...
  */

  sort_dist_files(dist);

 /*
  * Write the idb file for gendist...
  */

  if (Verbosity)
    puts("Creating idb file...");

  snprintf(idbname, sizeof(idbname), "%s/idb", directory);

  if ((fp = fopen(idbname, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create idb file \"%s\" - %s\n", idbname,
            strerror(errno));
    return (1);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
  {
    if (strstr(file->dst, "/man/") != NULL ||
        strstr(file->dst, "/catman/") != NULL)
      snprintf(subsys, sizeof(subsys), "%s.man.%s", prodname,
               file->subpackage ? file->subpackage : "eoe");
    else
      snprintf(subsys, sizeof(subsys), "%s.sw.%s", prodname,
               file->subpackage ? file->subpackage : "eoe");

    switch (tolower(file->type))
    {
      case '1' :
          qprintf(fp, "f %04o %s %s %s %s %s postop($rbase/%s)\n", file->mode,
	          file->user, file->group, file->dst + 1, file->src, subsys,
		  file->dst + 1);
          break;
      case '2' :
          qprintf(fp, "f %04o %s %s %s %s %s exitop($rbase/%s)\n", file->mode,
	          file->user, file->group, file->dst + 1, file->src, subsys,
		  file->dst + 1);
          break;
      case '3' :
      case '4' :
          qprintf(fp, "f %04o %s %s %s %s %s "
	              "postop(cp $rbase/%s $rbase/%s.copy) "
		      "removeop($rbase/%s.copy; rm -f $rbase/%s.copy)\n",
                  file->mode, file->user, file->group, file->dst + 1,
		  file->src, subsys, file->dst + 1, file->dst + 1,
		  file->dst + 1);
          break;
      case 'c' :
          qprintf(fp, "f %04o %s %s %s %s %s config(suggest)\n", file->mode,
	          file->user, file->group, file->dst + 1, file->src, subsys);
          break;
      case 'd' :
          qprintf(fp, "d %04o %s %s %s - %s\n", file->mode,
	          file->user, file->group, file->dst + 1, subsys);
          break;
      case 'f' :
          qprintf(fp, "f %04o %s %s %s %s %s\n", file->mode,
	          file->user, file->group, file->dst + 1, file->src, subsys);
          break;
      case 'i' :
          qprintf(fp, "f %04o %s %s %s %s %s "
	              "postop(cp $rbase/etc/init.d/%s $rbase/etc/init.d/%s.copy) "
	              "exitop($rbase/etc/init.d/%s start) "
		      "removeop($rbase/etc/init.d/%s.copy stop; rm -f $rbase/etc/init.d/%s.copy)\n",
	          file->mode, file->user, file->group, file->dst + 1,
		  file->src, subsys,
		  file->dst + 12, file->dst + 12,
		  file->dst + 12,
		  file->dst + 12, file->dst + 12);
          break;
      case 'l' :
          qprintf(fp, "l %04o %s %s %s - %s symval(%s)\n", file->mode,
	          file->user, file->group, file->dst + 1, subsys, file->src);
          break;
    }
  }

  fclose(fp);

 /*
  * Build the distribution from the spec file...
  */

  if (Verbosity)
    puts("Building INST binary distribution...");

  if (run_command(NULL, "gendist %s -dist %s -sbase . -idb %s -spec %s",
                  Verbosity == 0 ? "" : "-v", directory, idbname, specname))
    return (1);

 /*
  * Build the tardist file...
  */

  if (Verbosity)
    printf("Writing tardist file:");

  snprintf(filename, sizeof(filename), "%s/%s.tardist", directory, name);
  if ((tarfile = tar_open(filename, 0)) == NULL)
  {
    if (Verbosity)
      puts("");

    fprintf(stderr, "epm: Unable to create file \"%s\" -\n     %s\n",
            filename, strerror(errno));
    return (1);
  }

  for (i = 0; i < 4; i ++)
  {
    snprintf(srcname, sizeof(srcname), "%s/%s%s", directory, prodname, extensions[i]);
    snprintf(dstname, sizeof(dstname), "%s%s", prodname, extensions[i]);

    stat(srcname, &fileinfo);
    if (tar_header(tarfile, TAR_NORMAL, fileinfo.st_mode, fileinfo.st_size,
	           fileinfo.st_mtime, "root", "sys", dstname, NULL) < 0)
    {
      if (Verbosity)
        puts("");

      fprintf(stderr, "epm: Error writing file header - %s\n",
	      strerror(errno));
      return (-1);
    }

    if (tar_file(tarfile, srcname) < 0)
    {
      if (Verbosity)
        puts("");

      fprintf(stderr, "epm: Error writing file data for %s -\n    %s\n",
	      dstname, strerror(errno));
      return (-1);
    }

    if (Verbosity)
    {
      printf(" %s%s", prodname, extensions[i]);
      fflush(stdout);
    }
  }

  tar_close(tarfile);

  if (Verbosity)
  {
    stat(filename, &fileinfo);
    if (fileinfo.st_size > (1024 * 1024))
      printf(" size=%.1fM\n", fileinfo.st_size / 1024.0 / 1024.0);
    else
      printf(" size=%.0fk\n", fileinfo.st_size / 1024.0);
  }

 /*
  * Remove temporary files...
  */

  if (!KeepFiles)
  {
    if (Verbosity)
      puts("Removing temporary distribution files...");

    if (preinstall[0])
      unlink(preinstall);
    if (postinstall[0])
      unlink(postinstall);
    if (preremove[0])
      unlink(preremove);
    if (postremove[0])
      unlink(postremove);

    unlink(idbname);
    unlink(specname);
  }

  return (0);
}


/*
 * 'inst_subsys()' - Write a subsystem definition for the product.
 */

void
inst_subsys(FILE       *fp,		/* I - File to write to */
            const char *prodname,	/* I - Product short name */
	    dist_t     *dist,		/* I - Distribution */
	    const char *subpackage,	/* I - Subpackage name or NULL */
	    const char *category,	/* I - "Software" or "Man Pages" */
	    const char *section)	/* I - "sw" or "man" */
{
  int		i;			/* Looping var */
  depend_t	*d;			/* Current dependency */
  const char	*product;		/* Product for dependency */
  char		selfname[1024];		/* Self product name */
  char		title[1024];		/* Product description/title */


  snprintf(selfname, sizeof(selfname), "%s.%s.eoe", prodname, section);

  if (subpackage)
  {
    fprintf(fp, "		subsys %s\n", subpackage);

    for (i = 0; i < dist->num_descriptions; i ++)
      if (dist->descriptions[i].subpackage == subpackage)
        break;

    if (i < dist->num_descriptions)
      snprintf(title, sizeof(title), "%s %s", dist->product,
               dist->descriptions[i].description);
    else
      strlcpy(title, dist->product, sizeof(title));
  }
  else
  {
    fputs("		subsys eoe default\n", fp);

    strlcpy(title, dist->product, sizeof(title));
  }

  qprintf(fp, "			id \"%s, %s, %s\"\n", title, category,
          dist->version);
  fprintf(fp, "			exp \"%s.%s.%s\"\n", prodname, section,
          subpackage ? subpackage : "eoe");

  for (i = dist->num_depends, d = dist->depends; i > 0; i --, d ++)
    if (d->type == DEPEND_REQUIRES && d->subpackage == subpackage)
      break;

  if (i)
  {
    fputs("			prereq\n", fp);
    fputs("			(\n", fp);
    for (; i > 0; i --, d ++)
      if (d->type == DEPEND_REQUIRES && d->subpackage == subpackage)
      {
	if (!strcmp(d->product, "_self"))
          product = selfname;
	else
          product = d->product;

        if (strchr(product, '.') != NULL)
  	  fprintf(fp, "				%s %d %d\n",
         	  product, d->vernumber[0], d->vernumber[1]);
        else if (product[0] != '/')
  	  fprintf(fp, "				%s.%s.* %d %d\n",
         	  product, section, d->vernumber[0], d->vernumber[1]);
      }
    fputs("			)\n", fp);
  }

  for (i = dist->num_depends, d = dist->depends; i > 0; i --, d ++)
    if (d->type == DEPEND_REPLACES && d->subpackage == subpackage)
    {
      if (!strcmp(d->product, "_self"))
        product = selfname;
      else
        product = d->product;

      if (strchr(product, '.') != NULL)
      {
        fprintf(fp, "			replaces %s %d %d\n",
         	product, d->vernumber[0], d->vernumber[1]);
        fprintf(fp, "			updates %s %d %d\n",
         	product, d->vernumber[0], d->vernumber[1]);
      }
      else if (product[0] != '/')
      {
        fprintf(fp, "			replaces %s.%s.* %d %d\n",
         	product, section, d->vernumber[0], d->vernumber[1]);
        fprintf(fp, "			updates %s.%s.* %d %d\n",
         	product, section, d->vernumber[0], d->vernumber[1]);
      }
    }
    else if (d->type == DEPEND_INCOMPAT && d->subpackage == subpackage)
    {
      if (!strcmp(d->product, "_self"))
        product = selfname;
      else
        product = d->product;

      if (strchr(product, '.') != NULL)
        fprintf(fp, "			incompat %s %d %d\n",
         	product, d->vernumber[0], d->vernumber[1]);
      else if (product[0] != '/')
        fprintf(fp, "			incompat %s.%s.* %d %d\n",
         	product, section, d->vernumber[0], d->vernumber[1]);
    }

  fputs("		endsubsys\n", fp);
}


/*
 * End of "$Id: inst.c 670 2006-04-05 15:08:08Z mike $".
 */

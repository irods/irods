/*
 * "$Id: portable.c 833 2010-12-30 00:00:50Z mike $"
 *
 *   Portable package gateway for the ESP Package Manager (EPM).
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
 *   make_portable()      - Make a portable software distribution package.
 *   clean_distfiles()    - Remove temporary software distribution files...
 *   write_combined()     - Write all of the distribution files in tar files.
 *   write_commands()     - Write commands.
 *   write_common()       - Write the common shell script header.
 *   write_confcheck()    - Write the echo check to find the right echo options.
 *   write_depends()      - Write dependencies.
 *   write_distfiles()    - Write a software distribution...
 *   write_install()      - Write the installation script.
 *   write_instfiles()    - Write the installer files to the tar file...
 *   write_patch()        - Write the patch script.
 *   write_remove()       - Write the removal script.
 *   write_space_checks() - Write disk space checks for the installer.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * Local functions...
 */

static void	clean_distfiles(const char *directory, const char *prodname,
		                const char *platname, dist_t *dist,
				const char *subpackage);
static int	write_combined(const char *title, const char *directory,
		               const char *prodname, const char *platname,
			       dist_t *dist, const char **files,
			       time_t deftime, const char *setup,
			       const char *types);
static int	write_commands(dist_t *dist, FILE *fp, int type,
		               const char *subpackage);
static FILE	*write_common(dist_t *dist, const char *title,
			      int rootsize, int usrsize,
		              const char *filename, const char *subpackage);
static int	write_confcheck(FILE *fp);
static int	write_depends(const char *prodname, dist_t *dist, FILE *fp,
		              const char *subpackage);
static int	write_distfiles(const char *directory, const char *prodname,
		                const char *platname, dist_t *dist,
				time_t deftime, const char *subpackage);
static int	write_install(dist_t *dist, const char *prodname,
			      int rootsize, int usrsize,
		              const char *directory,
		              const char *subpackage);
static int	write_instfiles(tarf_t *tarfile, const char *directory,
		                const char *prodname, const char *platname,
			        const char **files, const char *destdir,
				const char *subpackage);
static int	write_patch(dist_t *dist, const char *prodname,
			    int rootsize, int usrsize,
		            const char *directory,
		            const char *subpackage);
static int	write_remove(dist_t *dist, const char *prodname,
			     int rootsize, int usrsize,
		             const char *directory,
		             const char *subpackage);
static int	write_space_checks(const char *prodname, FILE *fp,
		                   const char *sw, const char *ss,
				   int rootsize, int usrsize);


/*
 * 'make_portable()' - Make a portable software distribution package.
 */

int					/* O - 1 = success, 0 = fail */
make_portable(const char     *prodname,	/* I - Product short name */
              const char     *directory,/* I - Directory for distribution files */
              const char     *platname,	/* I - Platform name */
              dist_t         *dist,	/* I - Distribution information */
	      struct utsname *platform,	/* I - Platform information */
              const char     *setup,	/* I - Setup GUI image */
              const char     *types)	/* I - Setup GUI install types */
{
  int		i;			/* Looping var */
  int		havepatchfiles;		/* 1 if we have patch files, 0 otherwise */
  time_t	deftime;		/* File creation time */
  file_t	*file;			/* Software file */
  static const char	*distfiles[] =	/* Distribution files */
		{
		  "install",
		  "license",
		  "readme",
		  "remove",
		  "ss",
		  "sw",
		  NULL
		};
  static const char	*patchfiles[] =	/* Patch files */
		{
		  "patch",
		  "license",
		  "pss",
		  "psw",
		  "readme",
		  "remove",
		  NULL
		};


  REF(platform);

  if (Verbosity)
    puts("Creating PORTABLE distribution...");

 /*
  * See if we need to make a patch distribution...
  */

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (isupper((int)file->type))
      break;

  havepatchfiles = i > 0;

  deftime = time(NULL);

 /*
  * Build the main package and all of the subpackages...
  */

  if (write_distfiles(directory, prodname, platname, dist, deftime, NULL))
    return (1);

  for (i = 0; i < dist->num_subpackages; i ++)
    if (write_distfiles(directory, prodname, platname, dist, deftime,
                        dist->subpackages[i]))
      return (1);

 /*
  * Create the distribution archives...
  */

  if (write_combined("distribution", directory, prodname, platname, dist,
                     distfiles, deftime, setup, types))
    return (1);

  if (havepatchfiles)
    if (write_combined("patch", directory, prodname, platname, dist,
                       patchfiles, deftime, setup, types))
      return (1);

 /*
  * Cleanup...
  */

  if (!KeepFiles)
  {
    clean_distfiles(directory, prodname, platname, dist, NULL);

    for (i = 0; i < dist->num_subpackages; i ++)
      clean_distfiles(directory, prodname, platname, dist,
                      dist->subpackages[i]);
  }

 /*
  * Return!
  */

  return (0);
}


/*
 * 'clean_distfiles()' - Remove temporary software distribution files...
 */

static void
clean_distfiles(const char *directory,	/* I - Directory */
	        const char *prodname,	/* I - Product name */
                const char *platname,	/* I - Platform name */
	        dist_t     *dist,	/* I - Distribution */
	        const char *subpackage)	/* I - Subpackage */
{
  char		prodfull[255],		/* Full name of product */
		filename[1024];		/* Name of temporary file */


 /*
  * Figure out the full name of the distribution...
  */

  if (subpackage)
    snprintf(prodfull, sizeof(prodfull), "%s-%s", prodname, subpackage);
  else
    strlcpy(prodfull, prodname, sizeof(prodfull));

 /*
  * Remove the distribution files...
  */

  if (Verbosity)
    printf("Removing %s temporary files...\n", prodfull);

  snprintf(filename, sizeof(filename), "%s/%s.install", directory, prodfull);
  unlink(filename);

  snprintf(filename, sizeof(filename), "%s/%s.license", directory, prodfull);
  unlink(filename);

  snprintf(filename, sizeof(filename), "%s/%s.patch", directory, prodfull);
  unlink(filename);

  snprintf(filename, sizeof(filename), "%s/%s.psw", directory, prodfull);
  unlink(filename);

  snprintf(filename, sizeof(filename), "%s/%s.psw", directory, prodfull);
  unlink(filename);

  snprintf(filename, sizeof(filename), "%s/%s.readme", directory, prodfull);
  unlink(filename);

  snprintf(filename, sizeof(filename), "%s/%s.remove", directory, prodfull);
  unlink(filename);

  snprintf(filename, sizeof(filename), "%s/%s.ss", directory, prodfull);
  unlink(filename);

  snprintf(filename, sizeof(filename), "%s/%s.sw", directory, prodfull);
  unlink(filename);
}


/*
 * 'write_combined()' - Write all of the distribution files in tar files.
 */

static int				/* O - 0 on success, -1 on failure */
write_combined(const char *title,	/* I - Title */
               const char *directory,	/* I - Output directory */
	       const char *prodname,	/* I - Base product name */
	       const char *platname,	/* I - Platform name */
	       dist_t     *dist,	/* I - Distribution */
	       const char **files,	/* I - Files */
	       time_t     deftime,	/* I - Default timestamp */
	       const char *setup,	/* I - Setup program */
	       const char *types)	/* I - Setup types file */
{
  int		i;			/* Looping var */
  tarf_t	*tarfile;		/* Distribution tar file */
  char		tarfilename[1024],	/* Name of tar file */
		filename[1024];		/* Name of temporary file */
#ifdef __APPLE__
  FILE		*fp;			/* Plist file... */
#endif /* __APPLE__ */
  struct stat	srcstat;		/* Source file information */
  const char	*destdir;		/* Destination directory */
  const char	*setup_img;		/* Setup image name */


 /*
  * Figure out the filename...
  */

  if (dist->release[0])
    snprintf(tarfilename, sizeof(tarfilename), "%s/%s-%s-%s", directory,
             prodname, dist->version, dist->release);
  else
    snprintf(tarfilename, sizeof(tarfilename), "%s/%s-%s", directory, prodname,
             dist->version);

  if (!strcmp(title, "patch"))
    strlcat(tarfilename, "-patch", sizeof(tarfilename));

  if (platname[0])
  {
    strlcat(tarfilename, "-", sizeof(tarfilename));
    strlcat(tarfilename, platname, sizeof(tarfilename));
  }

  strlcat(tarfilename, ".tar.gz", sizeof(tarfilename));

 /*
  * Open output file...
  */

  if ((tarfile = tar_open(tarfilename, 1)) == NULL)
  {
    fprintf(stderr, "epm: Unable to create output pipe to gzip -\n     %s\n",
            strerror(errno));
    return (-1);
  }

  if (Verbosity)
    printf("Writing %s archive:\n", title);

#ifdef __APPLE__
  if (setup)
  {
   /*
    * Create directories for the setup application...
    */

    if (tar_header(tarfile, TAR_DIR, (mode_t)0755, (size_t)0, deftime, "root", "root",
                   "Install.app", NULL) < 0)
    {
      fprintf(stderr, "epm: Error writing directory header - %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    if (tar_header(tarfile, TAR_DIR, (mode_t)0755, (size_t)0, deftime, "root", "root",
                   "Install.app/Contents", NULL) < 0)
    {
      fprintf(stderr, "epm: Error writing directory header - %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    if (tar_header(tarfile, TAR_DIR, (mode_t)0755, (size_t)0, deftime, "root", "root",
                   "Install.app/Contents/MacOS", NULL) < 0)
    {
      fprintf(stderr, "epm: Error writing directory header - %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    if (tar_header(tarfile, TAR_DIR, (mode_t)0755, (size_t)0, deftime, "root", "root",
                   "Install.app/Contents/Resources", NULL) < 0)
    {
      fprintf(stderr, "epm: Error writing directory header - %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

   /*
    * Then copy the data files...
    */

    snprintf(filename, sizeof(filename), "%s/setup.icns", DataDir);
    stat(filename, &srcstat);

    if (tar_header(tarfile, TAR_NORMAL, srcstat.st_mode & (~0222),
                   srcstat.st_size, srcstat.st_mtime, "root", "root",
		   "Install.app/Contents/Resources/setup.icns", NULL) < 0)
    {
      fprintf(stderr, "epm: Error writing file header - %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    if (tar_file(tarfile, filename) < 0)
    {
      fprintf(stderr, "epm: Error writing file data for setup.icns -\n    %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    if (Verbosity)
      printf("    %7.0fk setup.icns\n", (srcstat.st_size + 1023) / 1024.0);

    snprintf(filename, sizeof(filename), "%s/setup.info", DataDir);
    stat(filename, &srcstat);

    if (tar_header(tarfile, TAR_NORMAL, srcstat.st_mode & (~0222),
                   srcstat.st_size, srcstat.st_mtime, "root", "root",
		   "Install.app/Contents/PkgInfo", NULL) < 0)
    {
      fprintf(stderr, "epm: Error writing file header - %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    if (tar_file(tarfile, filename) < 0)
    {
      fprintf(stderr, "epm: Error writing file data for PkgInfo -\n    %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    if (Verbosity)
      printf("    %7.0fk PkgInfo\n", (srcstat.st_size + 1023) / 1024.0);

    snprintf(filename, sizeof(filename), "%s/%s.setup.plist", directory,
             prodname);
    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Error writing %s -\n    %s\n", filename,
              strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<plist version=\"0.9\">\n"
		"    <dict>\n"
		"	<key>CFBundleInfoDictionaryVersion</key>\n"
		"	<string>6.0</string>\n"
		"	<key>CFBundleExecutable</key>\n"
		"	<string>setup</string>\n"
		"	<key>CFBundleIdentifier</key>\n"
		"	<string>com.easysw.epm.setup</string>\n"
		"	<key>CFBundleVersion</key>\n"
		"	<string>%s</string>\n"
		"	<key>CFBundleDevelopmentRegion</key>\n"
		"	<string>English</string>\n"
		"	<key>NSHumanReadableCopyright</key>\n"
		"	<string>Copyright %s</string>\n"
		"	<key>CFAppleHelpAnchor</key>\n"
		"	<string>help</string>\n"
		"	<key>CFBundleName</key>\n"
		"	<string>Installer for %s</string>\n"
		"	<key>CFBundlePackageType</key>\n"
		"	<string>APPL</string>\n"
		"	<key>CFBundleSignature</key>\n"
		"	<string>FLTK</string>\n"
		"	<key>CFBundleIconFile</key>\n"
		"	<string>setup.icns</string>\n"
		"	<key>CFBundleShortVersionString</key>\n"
		"	<string>%s</string>\n"
		"	<key>CFBundleGetInfoString</key>\n"
		"	<string>%s, Copyright %s</string>\n"
		"    </dict>\n"
		"</plist>\n",
            dist->version,
	    dist->copyright,
	    dist->product,
	    dist->version,
	    dist->version, dist->copyright);
    fclose(fp);

    stat(filename, &srcstat);

    if (tar_header(tarfile, TAR_NORMAL, srcstat.st_mode & (~0222),
                   srcstat.st_size, srcstat.st_mtime, "root", "root",
		   "Install.app/Contents/Info.plist", NULL) < 0)
    {
      fprintf(stderr, "epm: Error writing file header - %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    if (tar_file(tarfile, filename) < 0)
    {
      fprintf(stderr, "epm: Error writing file data for Info.plist -\n    %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    if (!KeepFiles)
      unlink(filename);

    if (Verbosity)
      printf("    %7.0fk Info.plist\n", (srcstat.st_size + 1023) / 1024.0);
  }

  destdir = "Install.app/Contents/Resources/";

#else /* !__APPLE__ */
  destdir = "";
#endif /* __APPLE__ */

 /*
  * Write installer files...
  */

  if (write_instfiles(tarfile, directory, prodname, platname, files, destdir,
                      NULL))
  {
    tar_close(tarfile);
    return (-1);
  }

  for (i = 0; i < dist->num_subpackages; i ++)
    if (write_instfiles(tarfile, directory, prodname, platname, files, destdir,
                	dist->subpackages[i]))
    {
      tar_close(tarfile);
      return (-1);
    }

 /*
  * Now the setup files...
  */

  if (setup)
  {
   /*
    * Include the ESP Software Installation Wizard (setup)...
    */

    if (stat(SetupProgram, &srcstat))
    {
      fprintf(stderr, "epm: Unable to stat GUI setup program %s - %s\n",
	      SetupProgram, strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

#ifdef __APPLE__
    if (tar_header(tarfile, TAR_NORMAL, 0555, srcstat.st_size,
	           srcstat.st_mtime, "root", "root",
		   "Install.app/Contents/MacOS/setup", NULL) < 0)
#else
    if (tar_header(tarfile, TAR_NORMAL, 0555, srcstat.st_size,
	           srcstat.st_mtime, "root", "root", "setup", NULL) < 0)
#endif /* __APPLE__ */
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

    snprintf(filename, sizeof(filename), "%s%s", destdir, setup_img);

    if (tar_header(tarfile, TAR_NORMAL, 0444, srcstat.st_size,
	           srcstat.st_mtime, "root", "root", filename, NULL) < 0)
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
      snprintf(filename, sizeof(filename), "%ssetup.types", destdir);

      if (tar_header(tarfile, TAR_NORMAL, 0444, srcstat.st_size,
		     srcstat.st_mtime, "root", "root", filename, NULL) < 0)
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
    * And finally the uninstall stuff...
    */

#ifdef __APPLE__
    if (tar_header(tarfile, TAR_DIR, 0755, 0, deftime, "root", "root",
                   "Uninstall.app", NULL) < 0)
    {
      fprintf(stderr, "epm: Error writing directory header - %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    if (tar_header(tarfile, TAR_DIR, 0755, 0, deftime, "root", "root",
                   "Uninstall.app/Contents", NULL) < 0)
    {
      fprintf(stderr, "epm: Error writing directory header - %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    if (tar_header(tarfile, TAR_DIR, 0755, 0, deftime, "root", "root",
                   "Uninstall.app/Contents/MacOS", NULL) < 0)
    {
      fprintf(stderr, "epm: Error writing directory header - %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    if (tar_header(tarfile, TAR_DIR, 0755, 0, deftime, "root", "root",
                   "Uninstall.app/Contents/Resources", NULL) < 0)
    {
      fprintf(stderr, "epm: Error writing directory header - %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

   /*
    * Then copy the data files...
    */

    snprintf(filename, sizeof(filename), "%s/uninst.icns", DataDir);
    stat(filename, &srcstat);

    if (tar_header(tarfile, TAR_NORMAL, srcstat.st_mode & (~0222),
                   srcstat.st_size, srcstat.st_mtime, "root", "root",
		   "Uninstall.app/Contents/Resources/uninst.icns", NULL) < 0)
    {
      fprintf(stderr, "epm: Error writing file header - %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    if (tar_file(tarfile, filename) < 0)
    {
      fprintf(stderr, "epm: Error writing file data for uninst.icns -\n    %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    if (Verbosity)
      printf("    %7.0fk uninst.icns\n", (srcstat.st_size + 1023) / 1024.0);

    snprintf(filename, sizeof(filename), "%s/uninst.info", DataDir);
    stat(filename, &srcstat);

    if (tar_header(tarfile, TAR_NORMAL, srcstat.st_mode & (~0222),
                   srcstat.st_size, srcstat.st_mtime, "root", "root",
		   "Uninstall.app/Contents/PkgInfo", NULL) < 0)
    {
      fprintf(stderr, "epm: Error writing file header - %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    if (tar_file(tarfile, filename) < 0)
    {
      fprintf(stderr, "epm: Error writing file data for PkgInfo -\n    %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    if (Verbosity)
      printf("    %7.0fk PkgInfo\n", (srcstat.st_size + 1023) / 1024.0);

    snprintf(filename, sizeof(filename), "%s/%s.uninst.plist", directory,
             prodname);
    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Error writing %s -\n    %s\n", filename,
              strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<plist version=\"0.9\">\n"
		"    <dict>\n"
		"	<key>CFBundleInfoDictionaryVersion</key>\n"
		"	<string>6.0</string>\n"
		"	<key>CFBundleExecutable</key>\n"
		"	<string>uninst</string>\n"
		"	<key>CFBundleIdentifier</key>\n"
		"	<string>com.easysw.epm.uninst</string>\n"
		"	<key>CFBundleVersion</key>\n"
		"	<string>%s</string>\n"
		"	<key>CFBundleDevelopmentRegion</key>\n"
		"	<string>English</string>\n"
		"	<key>NSHumanReadableCopyright</key>\n"
		"	<string>Copyright %s</string>\n"
		"	<key>CFAppleHelpAnchor</key>\n"
		"	<string>help</string>\n"
		"	<key>CFBundleName</key>\n"
		"	<string>Uninstaller for %s</string>\n"
		"	<key>CFBundlePackageType</key>\n"
		"	<string>APPL</string>\n"
		"	<key>CFBundleSignature</key>\n"
		"	<string>FLTK</string>\n"
		"	<key>CFBundleIconFile</key>\n"
		"	<string>uninst.icns</string>\n"
		"	<key>CFBundleShortVersionString</key>\n"
		"	<string>%s</string>\n"
		"	<key>CFBundleGetInfoString</key>\n"
		"	<string>%s, Copyright %s</string>\n"
		"    </dict>\n"
		"</plist>\n",
            dist->version,
	    dist->copyright,
	    dist->product,
	    dist->version,
	    dist->version, dist->copyright);
    fclose(fp);

    stat(filename, &srcstat);

    if (tar_header(tarfile, TAR_NORMAL, srcstat.st_mode & (~0222),
                   srcstat.st_size, srcstat.st_mtime, "root", "root",
		   "Uninstall.app/Contents/Info.plist", NULL) < 0)
    {
      fprintf(stderr, "epm: Error writing file header - %s\n",
	      strerror(errno));
      tar_close(tarfile);
      return (-1);
    }

    if (tar_file(tarfile, filename) < 0)
    {
      tar_close(tarfile);
      fprintf(stderr, "epm: Error writing file data for Info.plist -\n    %s\n",
	      strerror(errno));
      return (-1);
    }

    if (!KeepFiles)
      unlink(filename);

    if (Verbosity)
      printf("    %7.0fk Info.plist\n", (srcstat.st_size + 1023) / 1024.0);
#endif /* __APPLE__ */

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

#ifdef __APPLE__
    if (tar_header(tarfile, TAR_NORMAL, 0555, srcstat.st_size,
	           srcstat.st_mtime, "root", "root",
		   "Uninstall.app/Contents/MacOS/uninst", NULL) < 0)
#else
    if (tar_header(tarfile, TAR_NORMAL, 0555, srcstat.st_size,
	           srcstat.st_mtime, "root", "root", "uninst", NULL) < 0)
#endif /* __APPLE__ */
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

#ifdef __APPLE__
   /*
    * And the image file...
    */

    stat(setup, &srcstat);

    snprintf(filename, sizeof(filename), "Uninstall.app/Contents/Resources/%s",
             setup_img);

    if (tar_header(tarfile, TAR_NORMAL, 0444, srcstat.st_size,
	           srcstat.st_mtime, "root", "root", filename, NULL) < 0)
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
#endif /* __APPLE__ */
  }

  if (Verbosity)
  {
    puts("     ------- ----------------------------------------");
    printf("    %7.0fk %s-%s", tarfile->blocks * 0.5f, prodname, dist->version);
    if (dist->release[0])
      printf("-%s", dist->release);
    if (!strcmp(title, "patch"))
      fputs("-patch", stdout);
    if (platname[0])
      printf("-%s", platname);
    puts(".tar");
  }

  tar_close(tarfile);

  if (Verbosity)
  {
    stat(tarfilename, &srcstat);

    puts("     ------- ----------------------------------------");
    printf("    %7.0fk %s\n", (srcstat.st_size + 1023) / 1024.0,
           tarfilename);
  }

#ifdef __APPLE__
  {
    char	dmgfilename[1024];	/* Disk image filename */


   /*
    * Make a disk image containing the package files...
    */

    if (dist->release[0])
      snprintf(filename, sizeof(filename), "%s/%s-%s-%s", directory, prodname,
	       dist->version, dist->release);
    else
      snprintf(filename, sizeof(filename), "%s/%s-%s", directory, prodname,
	       dist->version);

    if (!strcmp(title, "patch"))
      strlcat(filename, "-patch", sizeof(filename));

    if (platname[0])
    {
      strlcat(filename, "-", sizeof(filename));
      strlcat(filename, platname, sizeof(filename));
    }

    snprintf(dmgfilename, sizeof(dmgfilename), "%s.dmg", filename);

    mkdir(filename, 0777);

    if (run_command(filename, "tar xvzf ../%s", strrchr(tarfilename, '/') + 1))
    {
      fputs("epm: Unable to create disk image template folder!\n", stderr);
      return (1);
    }

    if (run_command(NULL, "hdiutil create -ov -srcfolder %s %s",
		    filename, dmgfilename))
    {
      fputs("epm: Unable to create disk image!\n", stderr);
      return (1);
    }

    if (!KeepFiles)
      unlink_directory(filename);

    if (Verbosity)
    {
      stat(dmgfilename, &srcstat);

      printf("    %7.0fk %s\n", (srcstat.st_size + 1023) / 1024.0,
	     dmgfilename);
    }
  }
#endif /* __APPLE__ */

  return (0);
}


/*
 * 'write_commands()' - Write commands.
 */

static int				/* O - 0 on success, -1 on failure */
write_commands(dist_t     *dist,	/* I - Distribution */
               FILE       *fp,		/* I - File pointer */
               int        type,		/* I - Type of commands to write */
               const char *subpackage)	/* I - Subsystem */
{
  int			i;		/* Looping var */
  command_t		*c;		/* Current command */
  static const char	*commands[] =	/* Command strings */
			{
			  "pre-install",
			  "post-install",
			  "pre-patch",
			  "post-patch",
			  "pre-remove",
			  "post-remove"
			};


  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == type && c->subpackage == subpackage)
      break;

  if (i)
  {
    fprintf(fp, "echo Running %s commands...\n", commands[type]);

    for (; i > 0; i --, c ++)
      if (c->type == type && c->subpackage == subpackage)
        if (fprintf(fp, "%s\n", c->command) < 1)
        {
          perror("epm: Error writing command");
          return (-1);
        }
  }

  return (0);
}


/*
 * 'write_common()' - Write the common shell script header.
 */

static FILE *				/* O - File pointer */
write_common(dist_t     *dist,		/* I - Distribution */
             const char *title,		/* I - "Installation", etc... */
             int        rootsize,	/* I - Size of root files in kbytes */
	     int        usrsize,	/* I - Size of /usr files in kbytes */
             const char *filename,	/* I - Script to create */
	     const char *subpackage)	/* I - Subpackage name */
{
  int	i;				/* Looping var */
  FILE	*fp;				/* File pointer */
  char	line[1024],			/* Line buffer */
	*start,				/* Start of line */
	*ptr;				/* Pointer into line */


 /*
  * Remove any existing copy of the file...
  */

  unlink(filename);

 /*
  * Create the script file...
  */

  if ((fp = fopen(filename, "w")) == NULL)
    return (NULL);

 /*
  * Update the permissions on the file...
  */

  fchmod(fileno(fp), 0755);

 /*
  * Write the standard header...
  */

  fputs("#!/bin/sh\n", fp);
  fprintf(fp, "# %s script for %s version %s.\n", title,
          dist->product, dist->version);
  fputs("# Produced using " EPM_VERSION "; report problems to epm@easysw.com.\n",
        fp);
  fprintf(fp, "#%%product %s", dist->product);
  if (subpackage)
  {
    for (i = 0; i < dist->num_descriptions; i ++)
      if (dist->descriptions[i].subpackage == subpackage)
	break;

    if (i < dist->num_descriptions)
    {
      strlcpy(line, dist->descriptions[i].description, sizeof(line));
      if ((ptr = strchr(line, '\n')) != NULL)
        *ptr = '\0';

      fprintf(fp, " - %s", line);
    }
  }
  fputs("\n", fp);
  fprintf(fp, "#%%vendor %s\n", dist->vendor);
  fprintf(fp, "#%%copyright %s\n", dist->copyright);
  fprintf(fp, "#%%version %s %d\n", dist->version, dist->vernumber);

  for (i = 0; i < dist->num_descriptions; i ++)
    if (dist->descriptions[i].subpackage == subpackage)
      break;

  if (i < dist->num_descriptions)
  {
   /*
    * Just do descriptions for this subpackage...
    */

    for (; i < dist->num_descriptions; i ++)
      if (dist->descriptions[i].subpackage == subpackage)
      {
        strlcpy(line, dist->descriptions[i].description, sizeof(line));

        for (start = line; start; start = ptr)
	{
	  if ((ptr = strchr(start, '\n')) != NULL)
            *ptr++ = '\0';

	  fprintf(fp, "#%%description %s\n", start);
	}
      }
  }
  else
  {
   /*
    * Just do descriptions for the main package...
    */

    for (i = 0; i < dist->num_descriptions; i ++)
      if (dist->descriptions[i].subpackage == NULL)
      {
        strlcpy(line, dist->descriptions[i].description, sizeof(line));

        for (start = line; start; start = ptr)
	{
	  if ((ptr = strchr(start, '\n')) != NULL)
            *ptr++ = '\0';

	  fprintf(fp, "#%%description %s\n", start);
	}
      }
  }

  fprintf(fp, "#%%rootsize %d\n", rootsize);
  fprintf(fp, "#%%usrsize %d\n", usrsize);
  fputs("#\n", fp);

  fputs("PATH=/usr/xpg4/bin:/bin:/usr/bin:/usr/ucb:${PATH}\n", fp);
  fputs("SHELL=/bin/sh\n", fp);
  fputs("case \"`uname`\" in\n", fp);
  fputs("\tDarwin*)\n", fp);
  fputs("\tcase \"`id -un`\" in\n", fp);
  fputs("\t\troot)\n", fp);
  fputs("\t\t;;\n", fp);
  fputs("\t\t*)\n", fp);
  fprintf(fp, "\t\techo Sorry, you must have administrative priviledges to %s this software.\n",
          title[0] == 'I' ? "install" : title[0] == 'R' ? "remove" : "patch");
  fputs("\t\texit 1\n", fp);
  fputs("\t\t;;\n", fp);
  fputs("\tesac\n", fp);
  fputs("\t;;\n", fp);
  fputs("\t*)\n", fp);
  fputs("\tcase \"`id`\" in\n", fp);
  fputs("\t\tuid=0*)\n", fp);
  fputs("\t\t;;\n", fp);
  fputs("\t\t*)\n", fp);
  fprintf(fp, "\t\techo Sorry, you must be root to %s this software.\n",
          title[0] == 'I' ? "install" : title[0] == 'R' ? "remove" : "patch");
  fputs("\t\texit 1\n", fp);
  fputs("\t\t;;\n", fp);
  fputs("\tesac\n", fp);
  fputs("\t;;\n", fp);
  fputs("esac\n", fp);

  qprintf(fp, "echo Copyright %s\n", dist->copyright);
  fprintf(fp, "# Reset umask for %s...\n",
          title[0] == 'I' ? "install" : title[0] == 'R' ? "remove" : "patch");
  fputs("umask 002\n", fp);

  write_confcheck(fp);

 /*
  * Return the file pointer...
  */

  return (fp);
}


/*
 * 'write_confcheck()' - Write the echo check to find the right echo options.
 */

static int				/* O - -1 on error, 0 on success */
write_confcheck(FILE *fp)		/* I - Script file */
{
 /*
  * This is a simplified version of the autoconf test for echo; basically
  * we ignore the Stardent Vistra SVR4 case, since 1) we've never heard of
  * this OS, and 2) it doesn't provide the same functionality, specifically
  * the omission of a newline when prompting the user for some text.
  */

  fputs("# Determine correct echo options...\n", fp);
  fputs("if (echo \"testing\\c\"; echo 1,2,3) | grep c >/dev/null; then\n", fp);
  fputs("	ac_n=-n\n", fp);
  fputs("	ac_c=\n", fp);
  fputs("else\n", fp);
  fputs("	ac_n=\n", fp);
  fputs("	ac_c='\\c'\n", fp);
  fputs("fi\n", fp);

 /*
  * This is a check for the correct options to use with the "tar"
  * command.
  */

  fputs("# Determine correct extract options for the tar command...\n", fp);
  fputs("if test `uname` = Darwin; then\n", fp);
  fputs("	ac_tar=\"tar -xpPf\"\n", fp);
  fputs("else if test \"`tar --help 2>&1 | grep GNU`\" = \"\"; then\n", fp);
  fputs("	ac_tar=\"tar -xpf\"\n", fp);
  fputs("else\n", fp);
  fputs("	ac_tar=\"tar -xpPf\"\n", fp);
  fputs("fi fi\n", fp);

  return (0);
}


/*
 * 'write_depends()' - Write dependencies.
 */

static int				/* O - 0 on success, - 1 on failure */
write_depends(const char *prodname,	/* I - Product name */
              dist_t     *dist,		/* I - Distribution */
              FILE       *fp,		/* I - File pointer */
	      const char *subpackage)	/* I - Subpackage */
{
  int			i;		/* Looping var */
  depend_t		*d;		/* Current dependency */
  const char		*product;	/* Product/file to depend on */
  static const char	*depends[] =	/* Dependency strings */
			{
			  "requires",
			  "incompat",
			  "replaces",
			  "provides"
			};


  for (i = 0, d = dist->depends; i < dist->num_depends; i ++, d ++)
    if (d->subpackage == subpackage)
    {
      if (!strcmp(d->product, "_self"))
        product = prodname;
      else
        product = d->product;

      fprintf(fp, "#%%%s %s %d %d\n", depends[(int)d->type], product,
              d->vernumber[0], d->vernumber[1]);

      switch (d->type)
      {
	case DEPEND_REQUIRES :
            if (product[0] == '/')
            {
             /*
              * Require a file...
              */

              qprintf(fp, "if test ! -r %s -a ! -h %s; then\n",
                      product, product);
              qprintf(fp, "	echo Sorry, you must first install \\'%s\\'!\n",
	              product);
              fputs("	exit 1\n", fp);
              fputs("fi\n", fp);
            }
            else
            {
             /*
              * Require a product...
              */

              fprintf(fp, "if test ! -x %s/%s.remove; then\n",
                      SoftwareDir, product);
              fprintf(fp, "	if test -x %s.install; then\n",
                      product);
              fprintf(fp, "		echo Installing required %s software...\n",
                      product);
              fprintf(fp, "		./%s.install now\n", product);
              fputs("	else\n", fp);
              fprintf(fp, "		echo Sorry, you must first install \\'%s\\'!\n",
	              product);
              fputs("		exit 1\n", fp);
              fputs("	fi\n", fp);
              fputs("fi\n", fp);

              if (d->vernumber[0] > 0 || d->vernumber[1] < INT_MAX)
	      {
	       /*
		* Do version number checking...
		*/

        	fprintf(fp, "installed=`grep \'^#%%version\' "
	                    "%s/%s.remove | awk \'{print $3}\'`\n",
                	SoftwareDir, product);

        	fputs("if test x$installed = x; then\n", fp);
		fputs("	installed=0\n", fp);
		fputs("fi\n", fp);

		fprintf(fp, "if test $installed -lt %d -o $installed -gt %d; then\n",
	        	d->vernumber[0], d->vernumber[1]);
        	fprintf(fp, "	if test -x %s.install; then\n",
                	product);
        	fprintf(fp, "		echo Installing required %s software...\n",
                	product);
        	fprintf(fp, "		./%s.install now\n", product);
        	fputs("	else\n", fp);
        	fprintf(fp, "		echo Sorry, you must first install \\'%s\\' version %s to %s!\n",
	        	product, d->version[0], d->version[1]);
        	fputs("		exit 1\n", fp);
        	fputs("	fi\n", fp);
        	fputs("fi\n", fp);
	      }
            }
	    break;

	case DEPEND_INCOMPAT :
            if (product[0] == '/')
            {
             /*
              * Incompatible with a file...
              */

              qprintf(fp, "if test -r %s -o -h %s; then\n",
                      product, product);
              qprintf(fp, "	echo Sorry, this software is incompatible with \\'%s\\'!\n",
	              product);
              fputs("	echo Please remove it first.\n", fp);
              fputs("	exit 1\n", fp);
              fputs("fi\n", fp);
            }
            else
            {
             /*
              * Incompatible with a product...
              */

              fprintf(fp, "if test -x %s/%s.remove; then\n",
                      SoftwareDir, product);

              if (d->vernumber[0] > 0 || d->vernumber[1] < INT_MAX)
	      {
	       /*
		* Do version number checking...
		*/

        	fprintf(fp, "	installed=`grep \'^#%%version\' "
	                    "%s/%s.remove | awk \'{print $3}\'`\n",
			SoftwareDir, product);

        	fputs("	if test x$installed = x; then\n", fp);
		fputs("		installed=0\n", fp);
		fputs("	fi\n", fp);

		fprintf(fp, "	if test $installed -ge %d -a $installed -le %d; then\n",
	        	d->vernumber[0], d->vernumber[1]);
        	fprintf(fp, "		echo Sorry, this software is incompatible with \\'%s\\' version %s to %s!\n",
	        	product, d->version[0], d->version[1]);
        	fprintf(fp, "		echo Please remove it first by running \\'%s/%s.remove\\'.\n",
	        	SoftwareDir, product);
        	fputs("		exit 1\n", fp);
        	fputs("	fi\n", fp);
	      }
	      else
	      {
        	fprintf(fp, "	echo Sorry, this software is incompatible with \\'%s\\'!\n",
	        	product);
        	fprintf(fp, "	echo Please remove it first by running \\'%s/%s.remove\\'.\n",
	        	SoftwareDir, product);
        	fputs("	exit 1\n", fp);
	      }

              fputs("fi\n", fp);
            }
	    break;

	case DEPEND_REPLACES :
            fprintf(fp, "if test -x %s/%s.remove; then\n", SoftwareDir,
	            product);

            if (d->vernumber[0] > 0 || d->vernumber[1] < INT_MAX)
	    {
	     /*
	      * Do version number checking...
	      */

              fprintf(fp, "	installed=`grep \'^#%%version\' "
	                  "%s/%s.remove | awk \'{print $3}\'`\n",
                      SoftwareDir, product);

              fputs("	if test x$installed = x; then\n", fp);
	      fputs("		installed=0\n", fp);
	      fputs("	fi\n", fp);

	      fprintf(fp, "	if test $installed -ge %d -a $installed -le %d; then\n",
	              d->vernumber[0], d->vernumber[1]);
              fprintf(fp, "		echo Automatically replacing \\'%s\\'...\n",
	              product);
              fprintf(fp, "		%s/%s.remove now\n",
	              SoftwareDir, product);
              fputs("	fi\n", fp);
	    }
	    else
	    {
              fprintf(fp, "	echo Automatically replacing \\'%s\\'...\n",
	              product);
              fprintf(fp, "	%s/%s.remove now\n",
	              SoftwareDir, product);
            }

            fputs("fi\n", fp);
	    break;
      }
    }

  return (0);
}


/*
 * 'write_distfiles()' - Write a software distribution...
 */

static int				/* O - -1 on error, 0 on success */
write_distfiles(const char *directory,	/* I - Directory */
	        const char *prodname,	/* I - Product name */
                const char *platname,	/* I - Platform name */
	        dist_t     *dist,	/* I - Distribution */
		time_t     deftime,	/* I - Default file time */
	        const char *subpackage)	/* I - Subpackage */
{
  int		i;			/* Looping var */
  int		havepatchfiles;		/* 1 if we have patch files, 0 otherwise */
  tarf_t	*tarfile;		/* Distribution tar file */
  char		prodfull[255],		/* Full name of product */
		swname[255],		/* Name of distribution tar file */
		pswname[255],		/* Name of patch tar file */
		filename[1024];		/* Name of temporary file */
  struct stat	srcstat;		/* Source file information */
  file_t	*file;			/* Software file */
  int		rootsize,		/* Size of files in root partition */
		usrsize;		/* Size of files in /usr partition */
  int		prootsize,		/* Size of patch files in root partition */
		pusrsize;		/* Size of patch files in /usr partition */


 /*
  * Figure out the full name of the distribution...
  */

  if (subpackage)
    snprintf(prodfull, sizeof(prodfull), "%s-%s", prodname, subpackage);
  else
    strlcpy(prodfull, prodname, sizeof(prodfull));

 /*
  * See if we need to make a patch distribution...
  */

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (isupper((int)file->type) && file->subpackage == subpackage)
      break;

  havepatchfiles = i > 0;

 /*
  * Copy the license and readme files...
  */

  if (Verbosity)
    printf("Copying %s license and readme files...\n", prodfull);

  if (dist->license[0])
  {
    snprintf(filename, sizeof(filename), "%s/%s.license", directory, prodfull);
    if (copy_file(filename, dist->license, 0444, getuid(), getgid()))
      return (1);
  }

  if (dist->readme[0])
  {
    snprintf(filename, sizeof(filename), "%s/%s.readme", directory, prodfull);
    if (copy_file(filename, dist->readme, 0444, getuid(), getgid()))
      return (1);
  }

 /*
  * Create the non-shared software distribution file...
  */

  if (Verbosity)
    puts("Creating non-shared software distribution file...");

  snprintf(swname, sizeof(swname), "%s.sw", prodfull);
  snprintf(filename, sizeof(filename), "%s/%s", directory, swname);

  unlink(filename);
  if ((tarfile = tar_open(filename, CompressFiles)) == NULL)
  {
    fprintf(stderr, "epm: Unable to create file \"%s\" -\n     %s\n",
            filename, strerror(errno));
    return (1);
  }

  for (i = dist->num_files, file = dist->files, rootsize = 0, prootsize = 0;
       i > 0;
       i --, file ++)
    if (strncmp(file->dst, "/usr", 4) != 0 && file->subpackage == subpackage)
      switch (tolower(file->type))
      {
	case 'f' : /* Regular file */
	case 'c' : /* Config file */
	case 'i' : /* Init script */
            if (stat(file->src, &srcstat))
	    {
	      fprintf(stderr, "epm: Cannot stat %s - %s\n", file->src,
	              strerror(errno));
	      tar_close(tarfile);
	      return (1);
	    }

            rootsize += (srcstat.st_size + 1023) / 1024;

            if (isupper(file->type & 255))
              prootsize += (srcstat.st_size + 1023) / 1024;

           /*
	    * Configuration files are extracted to the config file name with
	    * .N appended; add a bit of script magic to check if the config
	    * file already exists, and if not we copy the .N to the config
	    * file location...
	    */

	    if (tolower(file->type) == 'c')
	      snprintf(filename, sizeof(filename), "%s.N", file->dst);
	    else if (tolower(file->type) == 'i')
	      snprintf(filename, sizeof(filename), "%s/init.d/%s", SoftwareDir,
	               file->dst);
	    else
              strcpy(filename, file->dst);

            if (Verbosity > 1)
	      printf("%s -> %s...\n", file->src, filename);

	    if (tar_header(tarfile, TAR_NORMAL, file->mode, srcstat.st_size,
	                   srcstat.st_mtime, file->user, file->group,
			   filename, NULL) < 0)
	    {
	      fprintf(stderr, "epm: Error writing file header - %s\n",
	              strerror(errno));
	      tar_close(tarfile);
	      return (1);
	    }

	    if (tar_file(tarfile, file->src) < 0)
	    {
	      fprintf(stderr, "epm: Error writing file data - %s\n",
	              strerror(errno));
	      tar_close(tarfile);
	      return (1);
	    }
	    break;

	case 'd' : /* Create directory */
            if (Verbosity > 1)
	      printf("Directory %s...\n", file->dst);

            rootsize ++;

            if (isupper(file->type & 255))
              prootsize ++;
	    break;

	case 'l' : /* Link file */
            if (Verbosity > 1)
	      printf("%s -> %s...\n", file->src, file->dst);

	    if (tar_header(tarfile, TAR_SYMLINK, file->mode, 0, deftime,
	                   file->user, file->group, file->dst, file->src) < 0)
	    {
	      fprintf(stderr, "epm: Error writing link header - %s\n",
	              strerror(errno));
	      tar_close(tarfile);
	      return (1);
	    }

            rootsize ++;

            if (isupper(file->type & 255))
              prootsize ++;
	    break;
      }

  tar_close(tarfile);

 /*
  * Create the shared software distribution file...
  */

  if (Verbosity)
    puts("Creating shared software distribution file...");

  snprintf(swname, sizeof(swname), "%s.ss", prodfull);
  snprintf(filename, sizeof(filename), "%s/%s", directory, swname);

  unlink(filename);
  if ((tarfile = tar_open(filename, CompressFiles)) == NULL)
  {
    fprintf(stderr, "epm: Unable to create file \"%s\" -\n     %s\n",
            filename, strerror(errno));
    return (1);
  }

  for (i = dist->num_files, file = dist->files, usrsize = 0, pusrsize = 0;
       i > 0;
       i --, file ++)
    if (strncmp(file->dst, "/usr", 4) == 0 && file->subpackage == subpackage)
      switch (tolower(file->type))
      {
	case 'f' : /* Regular file */
	case 'c' : /* Config file */
	case 'i' : /* Init script */
            if (stat(file->src, &srcstat))
	    {
	      fprintf(stderr, "epm: Cannot stat %s - %s\n", file->src,
	              strerror(errno));
	      tar_close(tarfile);
	      return (1);
	    }

            usrsize += (srcstat.st_size + 1023) / 1024;

            if (isupper(file->type & 255))
              pusrsize += (srcstat.st_size + 1023) / 1024;

           /*
	    * Configuration files are extracted to the config file name with
	    * .N appended; add a bit of script magic to check if the config
	    * file already exists, and if not we copy the .N to the config
	    * file location...
	    */

	    if (tolower(file->type) == 'c')
	      snprintf(filename, sizeof(filename), "%s.N", file->dst);
	    else if (tolower(file->type) == 'i')
	      snprintf(filename, sizeof(filename), "%s/init.d/%s", SoftwareDir,
	               file->dst);
	    else
              strcpy(filename, file->dst);

            if (Verbosity > 1)
	      printf("%s -> %s...\n", file->src, filename);

	    if (tar_header(tarfile, TAR_NORMAL, file->mode, srcstat.st_size,
	                   srcstat.st_mtime, file->user, file->group,
			   filename, NULL) < 0)
	    {
	      fprintf(stderr, "epm: Error writing file header - %s\n",
	              strerror(errno));
	      tar_close(tarfile);
	      return (1);
	    }

	    if (tar_file(tarfile, file->src) < 0)
	    {
	      fprintf(stderr, "epm: Error writing file data - %s\n",
	              strerror(errno));
	      tar_close(tarfile);
	      return (1);
	    }
	    break;

	case 'd' : /* Create directory */
            if (Verbosity > 1)
	      printf("%s...\n", file->dst);

	    usrsize ++;

            if (isupper(file->type & 255))
              pusrsize ++;
	    break;

	case 'l' : /* Link file */
            if (Verbosity > 1)
	      printf("%s -> %s...\n", file->src, file->dst);

	    if (tar_header(tarfile, TAR_SYMLINK, file->mode, 0, deftime,
	                   file->user, file->group, file->dst, file->src) < 0)
	    {
	      fprintf(stderr, "epm: Error writing link header - %s\n",
	              strerror(errno));
	      tar_close(tarfile);
	      return (1);
	    }

	    usrsize ++;

            if (isupper(file->type & 255))
              pusrsize ++;
	    break;
      }

  tar_close(tarfile);

 /*
  * Create the patch distribution files...
  */

  if (havepatchfiles)
  {
    if (Verbosity)
      puts("Creating non-shared software patch file...");

    snprintf(pswname, sizeof(pswname), "%s.psw", prodfull);
    snprintf(filename, sizeof(filename), "%s/%s", directory, pswname);

    unlink(filename);
    if ((tarfile = tar_open(filename, CompressFiles)) == NULL)
    {
      fprintf(stderr, "epm: Unable to create file \"%s\" -\n     %s\n",
              filename, strerror(errno));
      return (1);
    }

    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (strncmp(file->dst, "/usr", 4) != 0 && file->subpackage == subpackage)
	switch (file->type)
	{
	  case 'C' : /* Config file */
	  case 'F' : /* Regular file */
          case 'I' : /* Init script */
              if (stat(file->src, &srcstat))
	      {
		fprintf(stderr, "epm: Cannot stat %s - %s\n", file->src,
	        	strerror(errno));
		tar_close(tarfile);
		return (1);
	      }

             /*
	      * Configuration files are extracted to the config file name with
	      * .N appended; add a bit of script magic to check if the config
	      * file already exists, and if not we copy the .N to the config
	      * file location...
	      */

	      if (file->type == 'C')
		snprintf(filename, sizeof(filename), "%s.N", file->dst);
	      else if (file->type == 'I')
		snprintf(filename, sizeof(filename), "%s/init.d/%s", SoftwareDir,
		         file->dst);
	      else
        	strcpy(filename, file->dst);

              if (Verbosity > 1)
		printf("%s -> %s...\n", file->src, filename);

	      if (tar_header(tarfile, TAR_NORMAL, file->mode, srcstat.st_size,
	                     srcstat.st_mtime, file->user, file->group,
			     filename, NULL) < 0)
	      {
		fprintf(stderr, "epm: Error writing file header - %s\n",
	        	strerror(errno));
		tar_close(tarfile);
		return (1);
	      }

	      if (tar_file(tarfile, file->src) < 0)
	      {
		fprintf(stderr, "epm: Error writing file data - %s\n",
	        	strerror(errno));
		tar_close(tarfile);
		return (1);
	      }
	      break;

	  case 'd' : /* Create directory */
              if (Verbosity > 1)
		printf("%s...\n", file->dst);
	      break;

	  case 'L' : /* Link file */
              if (Verbosity > 1)
		printf("%s -> %s...\n", file->src, file->dst);

	      if (tar_header(tarfile, TAR_SYMLINK, file->mode, 0, deftime,
	                     file->user, file->group, file->dst, file->src) < 0)
	      {
		fprintf(stderr, "epm: Error writing link header - %s\n",
	        	strerror(errno));
		tar_close(tarfile);
		return (1);
	      }
	      break;
	}

    tar_close(tarfile);

    if (Verbosity)
      puts("Creating shared software patch file...");

    snprintf(pswname, sizeof(pswname), "%s.pss", prodfull);
    snprintf(filename, sizeof(filename), "%s/%s", directory, pswname);

    unlink(filename);
    if ((tarfile = tar_open(filename, CompressFiles)) == NULL)
    {
      fprintf(stderr, "epm: Unable to create file \"%s\" -\n     %s\n",
              filename, strerror(errno));
      return (1);
    }

    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (strncmp(file->dst, "/usr", 4) == 0 && file->subpackage == subpackage)
	switch (file->type)
	{
	  case 'C' : /* Config file */
	  case 'F' : /* Regular file */
          case 'I' : /* Init script */
              if (stat(file->src, &srcstat))
	      {
		fprintf(stderr, "epm: Cannot stat %s - %s\n", file->src,
	        	strerror(errno));
		tar_close(tarfile);
		return (1);
	      }

             /*
	      * Configuration files are extracted to the config file name with
	      * .N appended; add a bit of script magic to check if the config
	      * file already exists, and if not we copy the .N to the config
	      * file location...
	      */

	      if (file->type == 'C')
		snprintf(filename, sizeof(filename), "%s.N", file->dst);
	      else if (file->type == 'I')
		snprintf(filename, sizeof(filename), "%s/init.d/%s",
		         SoftwareDir, file->dst);
	      else
        	strcpy(filename, file->dst);

              if (Verbosity > 1)
		printf("%s -> %s...\n", file->src, filename);

	      if (tar_header(tarfile, TAR_NORMAL, file->mode, srcstat.st_size,
	                     srcstat.st_mtime, file->user, file->group,
			     filename, NULL) < 0)
	      {
		fprintf(stderr, "epm: Error writing file header - %s\n",
	        	strerror(errno));
		tar_close(tarfile);
		return (1);
	      }

	      if (tar_file(tarfile, file->src) < 0)
	      {
		fprintf(stderr, "epm: Error writing file data - %s\n",
	        	strerror(errno));
		tar_close(tarfile);
		return (1);
	      }
	      break;

	  case 'd' : /* Create directory */
              if (Verbosity > 1)
		printf("%s...\n", file->dst);
	      break;

	  case 'L' : /* Link file */
              if (Verbosity > 1)
		printf("%s -> %s...\n", file->src, file->dst);

	      if (tar_header(tarfile, TAR_SYMLINK, file->mode, 0, deftime,
	                     file->user, file->group, file->dst, file->src) < 0)
	      {
		fprintf(stderr, "epm: Error writing link header - %s\n",
	        	strerror(errno));
		tar_close(tarfile);
		return (1);
	      }
	      break;
	}

    tar_close(tarfile);
  }

 /*
  * Create the scripts...
  */

  if (write_install(dist, prodname, rootsize, usrsize, directory, subpackage))
    return (1);

  if (havepatchfiles)
    if (write_patch(dist, prodname, prootsize, pusrsize, directory, subpackage))
      return (1);

  if (write_remove(dist, prodname, rootsize, usrsize, directory, subpackage))
    return (1);

 /*
  * Return...
  */

  return (0);
}


/*
 * 'write_install()' - Write the installation script.
 */

static int				/* O - -1 on error, 0 on success */
write_install(dist_t     *dist,		/* I - Software distribution */
              const char *prodname,	/* I - Product name */
              int        rootsize,	/* I - Size of root files in kbytes */
	      int        usrsize,	/* I - Size of /usr files in kbytes */
	      const char *directory,	/* I - Directory */
	      const char *subpackage)	/* I - Subpackage */
{
  int		i;			/* Looping var */
  int		col;			/* Column in the output */
  FILE		*scriptfile;		/* Install script */
  char		prodfull[255];		/* Full product name */
  char		filename[1024];		/* Name of temporary file */
  file_t	*file;			/* Software file */
  const char	*runlevels;		/* Run levels */
  int		number;			/* Start/stop number */


  if (Verbosity)
    puts("Writing installation script...");

  if (subpackage)
    snprintf(prodfull, sizeof(prodfull), "%s-%s", prodname, subpackage);
  else
    strlcpy(prodfull, prodname, sizeof(prodfull));

  snprintf(filename, sizeof(filename), "%s/%s.install", directory, prodfull);

  if ((scriptfile = write_common(dist, "Installation", rootsize, usrsize,
                                 filename, subpackage)) == NULL)
  {
    fprintf(stderr, "epm: Unable to create installation script \"%s\" -\n"
                    "     %s\n", filename, strerror(errno));
    return (-1);
  }

  fputs("if test \"$*\" = \"now\"; then\n", scriptfile);
  fputs("	echo Software license silently accepted via command-line option.\n", scriptfile);
  fputs("else\n", scriptfile);
  fputs("	echo \"\"\n", scriptfile);
  qprintf(scriptfile, "	echo This installation script will install the %s\n",
          dist->product);
  qprintf(scriptfile, "	echo software version %s on your system.\n",
          dist->version);
  fputs("	echo \"\"\n", scriptfile);
  fputs("	while true ; do\n", scriptfile);
  fputs("		echo $ac_n \"Do you wish to continue? $ac_c\"\n", scriptfile);
  fputs("		read yesno\n", scriptfile);
  fputs("		case \"$yesno\" in\n", scriptfile);
  fputs("			y | yes | Y | Yes | YES)\n", scriptfile);
  fputs("			break\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			n | no | N | No | NO)\n", scriptfile);
  fputs("			exit 1\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			*)\n", scriptfile);
  fputs("			echo Please enter yes or no.\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("		esac\n", scriptfile);
  fputs("	done\n", scriptfile);

  if (dist->license[0])
  {
    fprintf(scriptfile, "	more %s.license\n", prodfull);
    fputs("	echo \"\"\n", scriptfile);
    fputs("	while true ; do\n", scriptfile);
    fputs("		echo $ac_n \"Do you agree with the terms of this license? $ac_c\"\n", scriptfile);
    fputs("		read yesno\n", scriptfile);
    fputs("		case \"$yesno\" in\n", scriptfile);
    fputs("			y | yes | Y | Yes | YES)\n", scriptfile);
    fputs("			break\n", scriptfile);
    fputs("			;;\n", scriptfile);
    fputs("			n | no | N | No | NO)\n", scriptfile);
    fputs("			exit 1\n", scriptfile);
    fputs("			;;\n", scriptfile);
    fputs("			*)\n", scriptfile);
    fputs("			echo Please enter yes or no.\n", scriptfile);
    fputs("			;;\n", scriptfile);
    fputs("		esac\n", scriptfile);
    fputs("	done\n", scriptfile);
  }

  fputs("fi\n", scriptfile);
  fprintf(scriptfile, "if test -x %s/%s.remove; then\n", SoftwareDir, prodfull);
  fprintf(scriptfile, "	echo Removing old versions of %s software...\n",
          prodfull);
  fprintf(scriptfile, "	%s/%s.remove now\n", SoftwareDir, prodfull);
  fputs("fi\n", scriptfile);

  write_space_checks(prodfull, scriptfile, rootsize ? "sw" : NULL,
                     usrsize ? "ss" : NULL, rootsize, usrsize);
  write_depends(prodname, dist, scriptfile, subpackage);
  write_commands(dist, scriptfile, COMMAND_PRE_INSTALL, subpackage);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
        strncmp(file->dst, "/usr", 4) != 0 && file->subpackage == subpackage)
      break;

  if (i)
  {
    fputs("echo Backing up old versions of non-shared files to be installed...\n", scriptfile);

    col = fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
          strncmp(file->dst, "/usr", 4) != 0 && file->subpackage == subpackage)
      {
        if (col > 80)
	  col = qprintf(scriptfile, " \\\n%s", file->dst) - 2;
	else
          col += qprintf(scriptfile, " %s", file->dst);
      }

    fputs("; do\n", scriptfile);
    fputs("	if test -d \"$file\" -o -f \"$file\" -o -h \"$file\"; then\n", scriptfile);
    fputs("		mv -f \"$file\" \"$file.O\"\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
        strncmp(file->dst, "/usr", 4) == 0 && file->subpackage == subpackage)
      break;

  if (i)
  {
    fputs("if test -w /usr ; then\n", scriptfile);
    fputs("	echo Backing up old versions of shared files to be installed...\n", scriptfile);

    col = fputs("	for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
          strncmp(file->dst, "/usr", 4) == 0 && file->subpackage == subpackage)
      {
        if (col > 80)
	  col = qprintf(scriptfile, " \\\n%s", file->dst) - 2;
	else
          col += qprintf(scriptfile, " %s", file->dst);
      }

    fputs("; do\n", scriptfile);
    fputs("		if test -d \"$file\" -o -f \"$file\" -o -h \"$file\"; then\n", scriptfile);
    fputs("			mv -f \"$file\" \"$file.O\"\n", scriptfile);
    fputs("		fi\n", scriptfile);
    fputs("	done\n", scriptfile);
    fputs("fi\n", scriptfile);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'd' && file->subpackage == subpackage)
      break;

  if (i)
  {
    fputs("echo Creating installation directories...\n", scriptfile);

    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'd' && file->subpackage == subpackage)
      {
	qprintf(scriptfile, "if test ! -d %s -a ! -f %s -a ! -h %s; then\n",
        	file->dst, file->dst, file->dst);
	qprintf(scriptfile, "	mkdir -p %s\n", file->dst);
	fputs("else\n", scriptfile);
	qprintf(scriptfile, "	if test -f %s; then\n", file->dst);
	qprintf(scriptfile, "		echo Error: %s already exists as a regular file!\n",
	        file->dst);
	fputs("		exit 1\n", scriptfile);
	fputs("	fi\n", scriptfile);
	fputs("fi\n", scriptfile);
	qprintf(scriptfile, "chown %s %s\n", file->user, file->dst);
	qprintf(scriptfile, "chgrp %s %s\n", file->group, file->dst);
	qprintf(scriptfile, "chmod %o %s\n", file->mode, file->dst);
      }
  }

  fputs("echo Installing software...\n", scriptfile);

  if (rootsize)
  {
    if (CompressFiles)
      fprintf(scriptfile, "gzip -dc %s.sw | $ac_tar -\n", prodfull);
    else
      fprintf(scriptfile, "$ac_tar %s.sw\n", prodfull);
  }

  if (usrsize)
  {
    fputs("if echo Write Test >/usr/.writetest 2>/dev/null; then\n", scriptfile);
    if (CompressFiles)
      fprintf(scriptfile, "	gzip -dc %s.ss | $ac_tar -\n", prodfull);
    else
      fprintf(scriptfile, "	$ac_tar %s.ss\n", prodfull);
    fputs("fi\n", scriptfile);
  }

  fprintf(scriptfile, "if test -d %s; then\n", SoftwareDir);
  fprintf(scriptfile, "	rm -f %s/%s.remove\n", SoftwareDir, prodfull);
  fputs("else\n", scriptfile);
  fprintf(scriptfile, "	mkdir -p %s\n", SoftwareDir);
  fputs("fi\n", scriptfile);
  fprintf(scriptfile, "cp %s.remove %s\n", prodfull, SoftwareDir);
  fprintf(scriptfile, "chmod 544 %s/%s.remove\n", SoftwareDir, prodfull);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'c' && file->subpackage == subpackage)
      break;

  if (i)
  {
    fputs("echo Checking configuration files...\n", scriptfile);

    col = fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'c' && file->subpackage == subpackage)
      {
        if (col > 80)
	  col = qprintf(scriptfile, " \\\n%s", file->dst) - 2;
	else
          col += qprintf(scriptfile, " %s", file->dst);
      }

    fputs("; do\n", scriptfile);
    fputs("	if test ! -f \"$file\"; then\n", scriptfile);
    fputs("		cp \"$file.N\" \"$file\"\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
  }

  fputs("echo Updating file permissions...\n", scriptfile);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (strncmp(file->dst, "/usr", 4) != 0 &&
        strcmp(file->user, "root") != 0 && file->subpackage == subpackage)
      switch (tolower(file->type))
      {
	case 'c' :
	    qprintf(scriptfile, "chown %s %s.N\n", file->user, file->dst);
	    qprintf(scriptfile, "chgrp %s %s.N\n", file->group, file->dst);
	case 'f' :
	    qprintf(scriptfile, "chown %s %s\n", file->user, file->dst);
	    qprintf(scriptfile, "chgrp %s %s\n", file->group, file->dst);
	    break;
      }

  fputs("if test -f /usr/.writetest; then\n", scriptfile);
  fputs("	rm -f /usr/.writetest\n", scriptfile);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (strncmp(file->dst, "/usr", 4) == 0 &&
        strcmp(file->user, "root") != 0 && file->subpackage == subpackage)
      switch (tolower(file->type))
      {
	case 'c' :
	    qprintf(scriptfile, "	chown %s %s.N\n", file->user, file->dst);
	    qprintf(scriptfile, "	chgrp %s %s.N\n", file->group, file->dst);
	case 'f' :
	    qprintf(scriptfile, "	chown %s %s\n", file->user, file->dst);
	    qprintf(scriptfile, "	chgrp %s %s\n", file->group, file->dst);
	    break;
      }
  fputs("fi\n", scriptfile);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i' && file->subpackage == subpackage)
      break;

  if (i)
  {
    fputs("echo Setting up init scripts...\n", scriptfile);

   /*
    * Find where the frigging init scripts go...
    */

    fputs("rcdir=\"\"\n", scriptfile);
    fputs("for dir in /sbin/rc.d /sbin /etc/rc.d /etc ; do\n", scriptfile);
    fputs("	if test -d $dir/rc2.d -o -h $dir/rc2.d -o "
          "-d $dir/rc3.d -o -h $dir/rc3.d; then\n", scriptfile);
    fputs("		rcdir=\"$dir\"\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
    fputs("if test \"$rcdir\" = \"\" ; then\n", scriptfile);
    fputs("	if test -d /usr/local/etc/rc.d; then\n", scriptfile);
    fputs("		for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'i' && file->subpackage == subpackage)
        qprintf(scriptfile, " %s", file->dst);
    fputs("; do\n", scriptfile);
    fputs("			rm -f /usr/local/etc/rc.d/$file.sh\n", scriptfile);
    qprintf(scriptfile, "			ln -s %s/init.d/$file "
                        "/usr/local/etc/rc.d/$file.sh\n",
            SoftwareDir);
    fputs("		done\n", scriptfile);
    fputs("	else\n", scriptfile);
    fputs("		echo Unable to determine location of startup scripts!\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("else\n", scriptfile);
    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i' && file->subpackage == subpackage)
      {
	fputs("	if test -d $rcdir/init.d; then\n", scriptfile);
	qprintf(scriptfile, "		/bin/rm -f $rcdir/init.d/%s\n", file->dst);
	qprintf(scriptfile, "		/bin/ln -s %s/init.d/%s "
                    "$rcdir/init.d/%s\n", SoftwareDir, file->dst, file->dst);
	fputs("	else\n", scriptfile);
	fputs("		if test -d /etc/init.d; then\n", scriptfile);
	qprintf(scriptfile, "			/bin/rm -f /etc/init.d/%s\n", file->dst);
	qprintf(scriptfile, "			/bin/ln -s %s/init.d/%s "
                    "/etc/init.d/%s\n", SoftwareDir, file->dst, file->dst);
	fputs("		fi\n", scriptfile);
	fputs("	fi\n", scriptfile);

	for (runlevels = get_runlevels(dist->files + i, "0235");
             isdigit(*runlevels & 255);
	     runlevels ++)
	{
	  if (*runlevels == '0')
            number = get_stop(file, 0);
	  else
	    number = get_start(file, 99);

          fprintf(scriptfile, "	if test -d $rcdir/rc%c.d; then\n", *runlevels);
	  qprintf(scriptfile, "		/bin/rm -f $rcdir/rc%c.d/%c%02d%s\n",
	          *runlevels, *runlevels == '0' ? 'K' : 'S', number, file->dst);
	  qprintf(scriptfile, "		/bin/ln -s %s/init.d/%s "
                              "$rcdir/rc%c.d/%c%02d%s\n",
                  SoftwareDir, file->dst, *runlevels,
		  *runlevels == '0' ? 'K' : 'S', number, file->dst);
	  fputs("	fi\n", scriptfile);
        }

#ifdef __sgi
        fputs("	if test -x /etc/chkconfig; then\n", scriptfile);
        qprintf(scriptfile, "		/etc/chkconfig -f %s on\n", file->dst);
        fputs("	fi\n", scriptfile);
#endif /* __sgi */

      }

    fputs("fi\n", scriptfile);
  }

  write_commands(dist, scriptfile, COMMAND_POST_INSTALL, subpackage);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i' && file->subpackage == subpackage)
      qprintf(scriptfile, "%s/init.d/%s start\n", SoftwareDir, file->dst);

  fputs("echo Installation is complete.\n", scriptfile);

  fclose(scriptfile);

  return (0);
}


/*
 * 'write_instfiles()' - Write the installer files to the tar file...
 */

static int				/* O - 0 = success, -1 on failure */
write_instfiles(tarf_t     *tarfile,	/* I - Distribution tar file */
                const char *directory,	/* I - Output directory */
                const char *prodname,	/* I - Base product name */
	        const char *platname,	/* I - Platform name */
	        const char **files,	/* I - Files */
		const char *destdir,	/* I - Destination directory in tar file */
	        const char *subpackage)	/* I - Subpackage */
{
  int		i;			/* Looping var */
  char		srcname[1024],		/* Name of source file in distribution */
		dstname[1024],		/* Name of destination file in distribution */
		prodfull[255];		/* Full name of product */
  struct stat	srcstat;		/* Source file information */


  if (subpackage)
    snprintf(prodfull, sizeof(prodfull), "%s-%s", prodname, subpackage);
  else
    strlcpy(prodfull, prodname, sizeof(prodfull));

  for (i = 0; files[i] != NULL; i ++)
  {
    snprintf(srcname, sizeof(srcname), "%s/%s.%s", directory, prodfull, files[i]);
    snprintf(dstname, sizeof(dstname), "%s%s.%s", destdir, prodfull, files[i]);

    if (stat(srcname, &srcstat))
    {
      if (!i)
        break;
      else
        continue;
    }

    if (srcstat.st_size == 0)
      continue;

    if (tar_header(tarfile, TAR_NORMAL, srcstat.st_mode & 07555,
                   srcstat.st_size, srcstat.st_mtime, "root", "root",
		   dstname, NULL) < 0)
    {
      fprintf(stderr, "epm: Error writing file header - %s\n",
	      strerror(errno));
      return (-1);
    }

    if (tar_file(tarfile, srcname) < 0)
    {
      fprintf(stderr, "epm: Error writing file data for %s -\n    %s\n",
	      dstname, strerror(errno));
      return (-1);
    }

    if (Verbosity)
      printf("    %7.0fk %s.%s\n", (srcstat.st_size + 1023) / 1024.0,
	     prodfull, files[i]);
  }

  return (0);
}


/*
 * 'write_patch()' - Write the patch script.
 */

static int				/* O - -1 on error, 0 on success */
write_patch(dist_t     *dist,		/* I - Software distribution */
            const char *prodname,	/* I - Product name */
            int        rootsize,	/* I - Size of root files in kbytes */
	    int        usrsize,		/* I - Size of /usr files in kbytes */
	    const char *directory,	/* I - Directory */
	    const char *subpackage)	/* I - Subpackage */
{
  int		i;			/* Looping var */
  FILE		*scriptfile;		/* Patch script */
  char		filename[1024];		/* Name of temporary file */
  char		prodfull[255];		/* Full product name */
  file_t	*file;			/* Software file */
  const char	*runlevels;		/* Run levels */
  int		number;			/* Start/stop number */


  if (Verbosity)
    puts("Writing patch script...");

  if (subpackage)
    snprintf(prodfull, sizeof(prodfull), "%s-%s", prodname, subpackage);
  else
    strlcpy(prodfull, prodname, sizeof(prodfull));

  snprintf(filename, sizeof(filename), "%s/%s.patch", directory, prodfull);

  if ((scriptfile = write_common(dist, "Patch", rootsize, usrsize,
                                 filename, subpackage)) == NULL)
  {
    fprintf(stderr, "epm: Unable to create patch script \"%s\" -\n"
                    "     %s\n", filename, strerror(errno));
    return (-1);
  }

  fputs("if test \"$*\" = \"now\"; then\n", scriptfile);
  fputs("	echo Software license silently accepted via command-line option.\n", scriptfile);
  fputs("else\n", scriptfile);
  fputs("	echo \"\"\n", scriptfile);
  qprintf(scriptfile, "	echo This installation script will patch the %s\n",
          dist->product);
  qprintf(scriptfile, "	echo software to version %s on your system.\n", dist->version);
  fputs("	echo \"\"\n", scriptfile);
  fputs("	while true ; do\n", scriptfile);
  fputs("		echo $ac_n \"Do you wish to continue? $ac_c\"\n", scriptfile);
  fputs("		read yesno\n", scriptfile);
  fputs("		case \"$yesno\" in\n", scriptfile);
  fputs("			y | yes | Y | Yes | YES)\n", scriptfile);
  fputs("			break\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			n | no | N | No | NO)\n", scriptfile);
  fputs("			exit 1\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			*)\n", scriptfile);
  fputs("			echo Please enter yes or no.\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("		esac\n", scriptfile);
  fputs("	done\n", scriptfile);

  if (dist->license[0])
  {
    fprintf(scriptfile, "	more %s.license\n", prodfull);
    fputs("	echo \"\"\n", scriptfile);
    fputs("	while true ; do\n", scriptfile);
    fputs("		echo $ac_n \"Do you agree with the terms of this license? $ac_c\"\n", scriptfile);
    fputs("		read yesno\n", scriptfile);
    fputs("		case \"$yesno\" in\n", scriptfile);
    fputs("			y | yes | Y | Yes | YES)\n", scriptfile);
    fputs("			break\n", scriptfile);
    fputs("			;;\n", scriptfile);
    fputs("			n | no | N | No | NO)\n", scriptfile);
    fputs("			exit 1\n", scriptfile);
    fputs("			;;\n", scriptfile);
    fputs("			*)\n", scriptfile);
    fputs("			echo Please enter yes or no.\n", scriptfile);
    fputs("			;;\n", scriptfile);
    fputs("		esac\n", scriptfile);
    fputs("	done\n", scriptfile);
  }

  fputs("fi\n", scriptfile);

  write_space_checks(prodfull, scriptfile, rootsize ? "psw" : NULL,
                     usrsize ? "pss" : NULL, rootsize, usrsize);
  write_depends(prodname, dist, scriptfile, subpackage);

  fprintf(scriptfile, "if test ! -x %s/%s.remove; then\n",
          SoftwareDir, prodfull);
  fputs("	echo You do not appear to have the base software installed!\n",
        scriptfile);
  fputs("	echo Please install the full distribution instead.\n", scriptfile);
  fputs("	exit 1\n", scriptfile);
  fputs("fi\n", scriptfile);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i' && file->subpackage == subpackage)
      qprintf(scriptfile, "%s/init.d/%s stop\n", SoftwareDir, file->dst);

  write_commands(dist, scriptfile, COMMAND_PRE_PATCH, subpackage);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (file->type == 'D' && file->subpackage == subpackage)
      break;

  if (i)
  {
    fputs("echo Creating new installation directories...\n", scriptfile);

    for (; i > 0; i --, file ++)
      if (file->type == 'D' && file->subpackage == subpackage)
      {
	qprintf(scriptfile, "if test ! -d %s -a ! -f %s -a ! -h %s; then\n",
        	file->dst, file->dst, file->dst);
	qprintf(scriptfile, "	mkdir -p %s\n", file->dst);
	fputs("else\n", scriptfile);
	qprintf(scriptfile, "	if test -f %s; then\n", file->dst);
	qprintf(scriptfile, "		echo Error: %s already exists as a regular file!\n",
	        file->dst);
	fputs("		exit 1\n", scriptfile);
	fputs("	fi\n", scriptfile);
	fputs("fi\n", scriptfile);
	qprintf(scriptfile, "chown %s %s\n", file->user, file->dst);
	qprintf(scriptfile, "chgrp %s %s\n", file->group, file->dst);
	qprintf(scriptfile, "chmod %o %s\n", file->mode, file->dst);
      }
  }

  fputs("echo Patching software...\n", scriptfile);

  if (rootsize)
  {
    if (CompressFiles)
      fprintf(scriptfile, "gzip -dc %s.psw | $ac_tar -\n", prodfull);
    else
      fprintf(scriptfile, "$ac_tar %s.psw\n", prodfull);
  }

  if (usrsize)
  {
    fputs("if echo Write Test >/usr/.writetest 2>/dev/null; then\n", scriptfile);
    if (CompressFiles)
      fprintf(scriptfile, "	gzip -dc %s.pss | $ac_tar -\n", prodfull);
    else
      fprintf(scriptfile, "	$ac_tar %s.pss\n", prodfull);
    fputs("fi\n", scriptfile);
  }

  fprintf(scriptfile, "rm -f %s/%s.remove\n", SoftwareDir, prodfull);
  fprintf(scriptfile, "cp %s.remove %s\n", prodfull, SoftwareDir);
  fprintf(scriptfile, "chmod 544 %s/%s.remove\n", SoftwareDir, prodfull);

  fputs("echo Updating file permissions...\n", scriptfile);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (strncmp(file->dst, "/usr", 4) != 0 &&
        strcmp(file->user, "root") != 0 && file->subpackage == subpackage)
      switch (file->type)
      {
	case 'C' :
	case 'F' :
	    qprintf(scriptfile, "chown %s %s\n", file->user, file->dst);
	    qprintf(scriptfile, "chgrp %s %s\n", file->group, file->dst);
	    break;
      }

  fputs("if test -f /usr/.writetest; then\n", scriptfile);
  fputs("	rm -f /usr/.writetest\n", scriptfile);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (strncmp(file->dst, "/usr", 4) == 0 &&
        strcmp(file->user, "root") != 0 && file->subpackage == subpackage)
      switch (file->type)
      {
	case 'C' :
	case 'F' :
	    qprintf(scriptfile, "	chown %s %s\n", file->user, file->dst);
	    qprintf(scriptfile, "	chgrp %s %s\n", file->group, file->dst);
	    break;
      }
  fputs("fi\n", scriptfile);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (file->type == 'C' && file->subpackage == subpackage)
      break;

  if (i)
  {
    fputs("echo Checking configuration files...\n", scriptfile);

    fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (file->type == 'C' && file->subpackage == subpackage)
        qprintf(scriptfile, " %s", file->dst);

    fputs("; do\n", scriptfile);
    fputs("	if test ! -f \"$file\"; then\n", scriptfile);
    fputs("		cp \"$file.N\" \"$file\"\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (file->type == 'R' && file->subpackage == subpackage)
      break;

  if (i)
  {
    fputs("echo Removing files that are no longer used...\n", scriptfile);

    fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (file->type == 'R' && file->subpackage == subpackage)
        qprintf(scriptfile, " %s", file->dst);

    fputs("; do\n", scriptfile);
    fputs("	rm -f \"$file\"\n", scriptfile);
    fputs("	if test -d \"$file.O\" -o -f \"$file.O\" -o -h \"$file.O\"; then\n", scriptfile);
    fputs("		mv -f \"$file.O\" \"$file\"\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (file->type == 'I' && file->subpackage == subpackage)
      break;

  if (i)
  {
    fputs("echo Setting up init scripts...\n", scriptfile);

   /*
    * Find where the frigging init scripts go...
    */

    fputs("rcdir=\"\"\n", scriptfile);
    fputs("for dir in /sbin/rc.d /sbin /etc/rc.d /etc ; do\n", scriptfile);
    fputs("	if test -d $dir/rc2.d -o -h $dir/rc2.d -o "
          "-d $dir/rc3.d -o -h $dir/rc3.d; then\n", scriptfile);
    fputs("		rcdir=\"$dir\"\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
    fputs("if test \"$rcdir\" = \"\" ; then\n", scriptfile);
    fputs("	if test -d /usr/local/etc/rc.d; then\n", scriptfile);
    fputs("		for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'I' && file->subpackage == subpackage)
        qprintf(scriptfile, " %s", file->dst);
    fputs("; do\n", scriptfile);
    fputs("			rm -f /usr/local/etc/rc.d/$file.sh\n", scriptfile);
    qprintf(scriptfile, "			ln -s %s/init.d/$file "
                        "/usr/local/etc/rc.d/$file.sh\n",
            SoftwareDir);
    fputs("		done\n", scriptfile);
    fputs("	else\n", scriptfile);
    fputs("		echo Unable to determine location of startup scripts!\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("else\n", scriptfile);
    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i' && file->subpackage == subpackage)
      {
	fputs("	if test -d $rcdir/init.d; then\n", scriptfile);
	qprintf(scriptfile, "		/bin/rm -f $rcdir/init.d/%s\n", file->dst);
	qprintf(scriptfile, "		/bin/ln -s %s/init.d/%s "
                    "$rcdir/init.d/%s\n", SoftwareDir, file->dst, file->dst);
	fputs("	else\n", scriptfile);
	fputs("		if test -d /etc/init.d; then\n", scriptfile);
	qprintf(scriptfile, "			/bin/rm -f /etc/init.d/%s\n", file->dst);
	qprintf(scriptfile, "			/bin/ln -s %s/init.d/%s "
                    "/etc/init.d/%s\n", SoftwareDir, file->dst, file->dst);
	fputs("		fi\n", scriptfile);
	fputs("	fi\n", scriptfile);

	for (runlevels = get_runlevels(dist->files + i, "0235");
             isdigit(*runlevels & 255);
	     runlevels ++)
	{
	  if (*runlevels == '0')
            number = get_stop(file, 0);
	  else
	    number = get_start(file, 99);

          fprintf(scriptfile, "	if test -d $rcdir/rc%c.d; then\n", *runlevels);
	  qprintf(scriptfile, "		/bin/rm -f $rcdir/rc%c.d/%c%02d%s\n",
	          *runlevels, *runlevels == '0' ? 'K' : 'S', number, file->dst);
	  qprintf(scriptfile, "		/bin/ln -s %s/init.d/%s "
                              "$rcdir/rc%c.d/%c%02d%s\n",
		  SoftwareDir, file->dst, *runlevels,
		  *runlevels == '0' ? 'K' : 'S', number, file->dst);
	  fputs("	fi\n", scriptfile);
        }

#ifdef __sgi
        fputs("	if test -x /etc/chkconfig; then\n", scriptfile);
        qprintf(scriptfile, "		/etc/chkconfig -f %s on\n", file->dst);
        fputs("	fi\n", scriptfile);
#endif /* __sgi */
      }

    fputs("fi\n", scriptfile);
  }

  write_commands(dist, scriptfile, COMMAND_POST_PATCH, subpackage);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i' && file->subpackage == subpackage)
      qprintf(scriptfile, "%s/init.d/%s start\n", SoftwareDir, file->dst);

  fputs("echo Patching is complete.\n", scriptfile);

  fclose(scriptfile);

  return (0);
}


/*
 * 'write_remove()' - Write the removal script.
 */

static int				/* O - -1 on error, 0 on success */
write_remove(dist_t     *dist,		/* I - Software distribution */
             const char *prodname,	/* I - Product name */
             int        rootsize,	/* I - Size of root files in kbytes */
	     int        usrsize,	/* I - Size of /usr files in kbytes */
	     const char *directory,	/* I - Directory */
	     const char *subpackage)	/* I - Subpackage */
{
  int		i;			/* Looping var */
  int		col;			/* Current column */
  FILE		*scriptfile;		/* Remove script */
  char		filename[1024];		/* Name of temporary file */
  char		prodfull[255];		/* Full product name */
  file_t	*file;			/* Software file */
  const char	*runlevels;		/* Run levels */
  int		number;			/* Start/stop number */


  if (Verbosity)
    puts("Writing removal script...");

  if (subpackage)
    snprintf(prodfull, sizeof(prodfull), "%s-%s", prodname, subpackage);
  else
    strlcpy(prodfull, prodname, sizeof(prodfull));

  snprintf(filename, sizeof(filename), "%s/%s.remove", directory, prodfull);

  if ((scriptfile = write_common(dist, "Removal", rootsize, usrsize,
                                 filename, subpackage)) == NULL)
  {
    fprintf(stderr, "epm: Unable to create removal script \"%s\" -\n"
                    "     %s\n", filename, strerror(errno));
    return (-1);
  }

  fputs("if test ! \"$*\" = \"now\"; then\n", scriptfile);
  fputs("	echo \"\"\n", scriptfile);
  qprintf(scriptfile, "	echo This removal script will remove the %s\n",
          dist->product);
  qprintf(scriptfile, "	echo software version %s from your system.\n",
          dist->version);
  fputs("	echo \"\"\n", scriptfile);
  fputs("	while true ; do\n", scriptfile);
  fputs("		echo $ac_n \"Do you wish to continue? $ac_c\"\n", scriptfile);
  fputs("		read yesno\n", scriptfile);
  fputs("		case \"$yesno\" in\n", scriptfile);
  fputs("			y | yes | Y | Yes | YES)\n", scriptfile);
  fputs("			break\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			n | no | N | No | NO)\n", scriptfile);
  fputs("			exit 1\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			*)\n", scriptfile);
  fputs("			echo Please enter yes or no.\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("		esac\n", scriptfile);
  fputs("	done\n", scriptfile);
  fputs("fi\n", scriptfile);

 /*
  * Find any removal commands in the list file...
  */

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i' && file->subpackage == subpackage)
      qprintf(scriptfile, "%s/init.d/%s stop\n", SoftwareDir, file->dst);

  write_commands(dist, scriptfile, COMMAND_PRE_REMOVE, subpackage);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i' && file->subpackage == subpackage)
      break;

  if (i)
  {
    fputs("echo Cleaning up init scripts...\n", scriptfile);

   /*
    * Find where the frigging init scripts go...
    */

    fputs("rcdir=\"\"\n", scriptfile);
    fputs("for dir in /sbin/rc.d /sbin /etc/rc.d /etc ; do\n", scriptfile);
    fputs("	if test -d $dir/rc2.d -o -h $dir/rc2.d -o "
          "-d $dir/rc3.d -o -h $dir/rc3.d; then\n", scriptfile);
    fputs("		rcdir=\"$dir\"\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
    fputs("if test \"$rcdir\" = \"\" ; then\n", scriptfile);
    fputs("	if test -d /usr/local/etc/rc.d; then\n", scriptfile);
    fputs("		for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'i' && file->subpackage == subpackage)
        qprintf(scriptfile, " %s", file->dst);
    fputs("; do\n", scriptfile);
    fputs("			rm -f /usr/local/etc/rc.d/$file.sh\n", scriptfile);
    fputs("		done\n", scriptfile);
    fputs("	else\n", scriptfile);
    fputs("		echo Unable to determine location of startup scripts!\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("else\n", scriptfile);
    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i' && file->subpackage == subpackage)
      {
        qprintf(scriptfile, "	%s/init.d/%s stop\n", SoftwareDir, file->dst);

	fputs("	if test -d $rcdir/init.d; then\n", scriptfile);
	qprintf(scriptfile, "		/bin/rm -f $rcdir/init.d/%s\n", file->dst);
	fputs("	else\n", scriptfile);
	fputs("		if test -d /etc/init.d; then\n", scriptfile);
	qprintf(scriptfile, "			/bin/rm -f /etc/init.d/%s\n", file->dst);
	fputs("		fi\n", scriptfile);
	fputs("	fi\n", scriptfile);

	for (runlevels = get_runlevels(dist->files + i, "0235");
             isdigit(*runlevels & 255);
	     runlevels ++)
	{
	  if (*runlevels == '0')
            number = get_stop(file, 0);
	  else
	    number = get_start(file, 99);

          fprintf(scriptfile, "	if test -d $rcdir/rc%c.d; then\n", *runlevels);
	  qprintf(scriptfile, "		/bin/rm -f $rcdir/rc%c.d/%c%02d%s\n",
	          *runlevels, *runlevels == '0' ? 'K' : 'S', number, file->dst);
	  fputs("	fi\n", scriptfile);
        }

#ifdef __sgi
        fputs("	if test -x /etc/chkconfig; then\n", scriptfile);
        qprintf(scriptfile, "		rm -f /etc/config/%s\n", file->dst);
        fputs("	fi\n", scriptfile);
#endif /* __sgi */
      }

    fputs("fi\n", scriptfile);
  }

  fputs("echo Removing/restoring installed files...\n", scriptfile);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
        strncmp(file->dst, "/usr", 4) != 0 && file->subpackage == subpackage)
      break;

  if (i)
  {
    col = fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
          strncmp(file->dst, "/usr", 4) != 0 && file->subpackage == subpackage)
      {
        if (col > 80)
	  col = qprintf(scriptfile, " \\\n%s", file->dst) - 2;
	else
          col += qprintf(scriptfile, " %s", file->dst);
      }

    fputs("; do\n", scriptfile);
    fputs("	rm -f \"$file\"\n", scriptfile);
    fputs("	if test -d \"$file.O\" -o -f \"$file.O\" -o -h \"$file.O\"; then\n", scriptfile);
    fputs("		mv -f \"$file.O\" \"$file\"\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
        strncmp(file->dst, "/usr", 4) == 0 && file->subpackage == subpackage)
      break;

  if (i)
  {
    fputs("if test -w /usr ; then\n", scriptfile);
    col = fputs("	for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
          strncmp(file->dst, "/usr", 4) == 0 && file->subpackage == subpackage)
      {
        if (col > 80)
	  col = qprintf(scriptfile, " \\\n%s", file->dst) - 2;
	else
          col += qprintf(scriptfile, " %s", file->dst);
      }

    fputs("; do\n", scriptfile);
    fputs("		rm -f \"$file\"\n", scriptfile);
    fputs("		if test -d \"$file.O\" -o -f \"$file.O\" -o -h \"$file.O\"; then\n", scriptfile);
    fputs("			mv -f \"$file.O\" \"$file\"\n", scriptfile);
    fputs("		fi\n", scriptfile);
    fputs("	done\n", scriptfile);
    fputs("fi\n", scriptfile);
  }

  fputs("echo Checking configuration files...\n", scriptfile);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'c' && file->subpackage == subpackage)
      break;

  if (i)
  {
    col = fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'c' && file->subpackage == subpackage)
      {
        if (col > 80)
	  col = qprintf(scriptfile, " \\\n%s", file->dst) - 2;
	else
          col += qprintf(scriptfile, " %s", file->dst);
      }

    fputs("; do\n", scriptfile);
    fputs("	if cmp -s \"$file\" \"$file.N\"; then\n", scriptfile);
    fputs("		# Config file not changed\n", scriptfile);
    fputs("		rm -f \"$file\"\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("	rm -f \"$file.N\"\n", scriptfile);
    fputs("done\n", scriptfile);
  }

  for (i = dist->num_files, file = dist->files + i - 1; i > 0; i --, file --)
    if (tolower(file->type) == 'd' && file->subpackage == subpackage)
      break;

  if (i)
  {
    fputs("echo Removing empty installation directories...\n", scriptfile);

    for (; i > 0; i --, file --)
      if (tolower(file->type) == 'd' && file->subpackage == subpackage)
      {
	qprintf(scriptfile, "if test -d %s; then\n", file->dst);
	qprintf(scriptfile, "	rmdir %s >/dev/null 2>&1\n", file->dst);
	fputs("fi\n", scriptfile);
      }
  }

  write_commands(dist, scriptfile, COMMAND_POST_REMOVE, subpackage);

  fprintf(scriptfile, "rm -f %s/%s.remove\n", SoftwareDir, prodfull);

  fputs("echo Removal is complete.\n", scriptfile);

  fclose(scriptfile);

  return (0);
}


/*
 * 'write_space_checks()' - Write disk space checks for the installer.
 */

static int				/* O - 0 on success, -1 on error */
write_space_checks(const char *prodname,/* I - Distribution name */
                   FILE       *fp,	/* I - File to write to */
                   const char *sw,	/* I - / archive */
		   const char *ss,	/* I - /usr archive */
		   int        rootsize,	/* I - / install size in kbytes */
		   int        usrsize)	/* I - /usr install size in kbytes */
{
  fputs("case `uname` in\n", fp);
  fputs("	AIX)\n", fp);
  fputs("	dfroot=`df -k / | tr '\\n' ' '`\n", fp);
  fputs("	dfusr=`df -k /usr | tr '\\n' ' '`\n", fp);
  fputs("	fsroot=`echo $dfroot | awk '{print $15}'`\n", fp);
  fputs("	sproot=`echo $dfroot | awk '{print $11}'`\n", fp);
  fputs("	fsusr=`echo $dfusr | awk '{print $15}'`\n", fp);
  fputs("	spusr=`echo $dfusr | awk '{print $11}'`\n", fp);
  fputs("	;;\n\n", fp);
  fputs("	HP-UX)\n", fp);
  fputs("	dfroot=`df -k / | tr '\\n' ' '`\n", fp);
  fputs("	dfusr=`df -k /usr | tr '\\n' ' '`\n", fp);
  fputs("	fsroot=`echo $dfroot | awk '{print $1}'`\n", fp);
  fputs("	sproot=`echo $dfroot | awk '{print $9}'`\n", fp);
  fputs("	fsusr=`echo $dfusr | awk '{print $1}'`\n", fp);
  fputs("	spusr=`echo $dfusr | awk '{print $9}'`\n", fp);
  fputs("	;;\n\n", fp);
  fputs("	IRIX*)\n", fp);
  fputs("	dfroot=`df -k / | tr '\\n' ' '`\n", fp);
  fputs("	dfusr=`df -k /usr | tr '\\n' ' '`\n", fp);
  fputs("	fsroot=`echo $dfroot | awk '{print $15}'`\n", fp);
  fputs("	sproot=`echo $dfroot | awk '{print $13}'`\n", fp);
  fputs("	fsusr=`echo $dfusr | awk '{print $15}'`\n", fp);
  fputs("	spusr=`echo $dfusr | awk '{print $13}'`\n", fp);
  fputs("	;;\n\n", fp);
  fputs("	SCO*)\n", fp);
  fputs("	dfroot=`df -k -B / | tr '\\n' ' '`\n", fp);
  fputs("	dfusr=`df -k -B /usr | tr '\\n' ' '`\n", fp);
  fputs("	fsroot=`echo $dfroot | awk '{print $13}'`\n", fp);
  fputs("	sproot=`echo $dfroot | awk '{print $11}'`\n", fp);
  fputs("	fsusr=`echo $dfusr | awk '{print $13}'`\n", fp);
  fputs("	spusr=`echo $dfusr | awk '{print $11}'`\n", fp);
  fputs("	;;\n\n", fp);
  fputs("	*)\n", fp);
  fputs("	dfroot=`df -k / | tr '\\n' ' '`\n", fp);
  fputs("	dfusr=`df -k /usr | tr '\\n' ' '`\n", fp);
  fputs("	fsroot=`echo $dfroot | awk '{print $13}'`\n", fp);
  fputs("	sproot=`echo $dfroot | awk '{print $11}'`\n", fp);
  fputs("	fsusr=`echo $dfusr | awk '{print $13}'`\n", fp);
  fputs("	spusr=`echo $dfusr | awk '{print $11}'`\n", fp);
  fputs("	;;\n", fp);
  fputs("esac\n", fp);
  fputs("\n", fp);

  fputs("if test x$sproot = x -o x$spusr = x; then\n", fp);
  fputs("	echo WARNING: Unable to determine available disk space\\; "
        "installing blindly...\n", fp);
  fputs("else\n", fp);
  fputs("	if test x$fsroot = x$fsusr; then\n", fp);
  fprintf(fp, "		if test %d -gt $sproot; then\n", rootsize + usrsize);
  fputs("			echo Not enough free disk space for "
        "software:\n", fp);
  fprintf(fp, "			echo You need %d kbytes but only have "
              "$sproot kbytes available.\n", rootsize + usrsize);
  fputs("			exit 1\n", fp);
  fputs("		fi\n", fp);
  fputs("	else\n", fp);
  fprintf(fp, "		if test %d -gt $sproot; then\n", rootsize);
  fputs("			echo Not enough free disk space for "
        "software:\n", fp);
  fprintf(fp, "			echo You need %d kbytes in / but only have "
              "$sproot kbytes available.\n", rootsize);
  fputs("			exit 1\n", fp);
  fputs("		fi\n", fp);
  fputs("\n", fp);
  fprintf(fp, "		if test %d -gt $spusr; then\n", usrsize);
  fputs("			echo Not enough free disk space for "
        "software:\n", fp);
  fprintf(fp, "			echo You need %d kbytes in /usr but only have "
              "$spusr kbytes available.\n", usrsize);
  fputs("			exit 1\n", fp);
  fputs("		fi\n", fp);
  fputs("	fi\n", fp);
  fputs("fi\n", fp);

  return (0);
}


/*
 * End of "$Id: portable.c 833 2010-12-30 00:00:50Z mike $".
 */

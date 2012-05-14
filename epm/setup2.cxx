//
// "$Id: setup2.cxx 834 2010-12-30 00:08:36Z mike $"
//
//   ESP Software Installation Wizard main entry for the ESP Package Manager (EPM).
//
//   Copyright 1999-2010 by Easy Software Products.
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2, or (at your option)
//   any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
// Contents:
//
//   main()         - Main entry for software wizard...
//   get_dists()    - Get a list of available software products.
//   install_dist() - Install a distribution...
//   license_dist() - Show the license for a distribution...
//   list_cb()      - Handle selections in the software list.
//   load_image()   - Load the setup image file (setup.gif/xpm)...
//   load_readme()  - Load the readme file...
//   load_types()   - Load the installation types from the setup.types file.
//   log_cb()       - Add one or more lines of text to the installation log.
//   next_cb()      - Show software selections or install software.
//   type_cb()      - Handle selections in the type list.
//   update_size()  - Update the total +/- sizes of the installations.
//

#define _DEFINE_GLOBALS_
#include "setup.h"
#include <FL/Fl_GIF_Image.H>
#include <FL/Fl_XPM_Image.H>
#include <FL/x.H>
#include <FL/filename.H>
#include <FL/fl_ask.H>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_PARAM_H
#  include <sys/param.h>
#endif // HAVE_SYS_PARAM_H

#ifdef HAVE_SYS_MOUNT_H
#  include <sys/mount.h>
#endif // HAVE_SYS_MOUNT_H

#ifdef HAVE_SYS_STATFS_H
#  include <sys/statfs.h>
#endif // HAVE_SYS_STATFS_H

#ifdef HAVE_SYS_VFS_H
#  include <sys/vfs.h>
#endif // HAVE_SYS_VFS_H

#ifdef __osf__
// No prototype for statfs under Tru64...
extern "C" {
extern int statfs(const char *, struct statfs *);
}
#endif // __osf__

#ifdef __APPLE__
#  include <Security/Authorization.h>
#  include <Security/AuthorizationTags.h>

AuthorizationRef SetupAuthorizationRef;
#endif // __APPLE__


//
// Panes...
//

#define PANE_WELCOME	0
#define PANE_TYPE	1
#define PANE_SELECT	2
#define PANE_CONFIRM	3
#define PANE_LICENSE	4
#define PANE_INSTALL	5


//
// Define a C API function type for comparisons...
//

extern "C" {
typedef int (*compare_func_t)(const void *, const void *);
}


//
// Local functions...
//

void	get_dists(const char *d);
int	install_dist(const gui_dist_t *dist);
int	license_dist(const gui_dist_t *dist);
void	load_image(void);
void	load_readme(void);
void	load_types(void);
void	log_cb(int fd, int *fdptr);
void	update_sizes(void);


//
// 'main()' - Main entry for software wizard...
//

int				// O - Exit status
main(int  argc,			// I - Number of command-line arguments
     char *argv[])		// I - Command-line arguments
{
  Fl_Window	*w;		// Main window...
  const char	*distdir;	// Distribution directory


  // Use GTK+ scheme for all operating systems...
  Fl::background(230, 230, 230);
  Fl::scheme("gtk+");

#ifdef __APPLE__
  // OSX passes an extra command-line option when run from the Finder.
  // If the first command-line argument is "-psn..." then skip it and use the full path
  // to the executable to figure out the distribution directory...
  if (argc > 1)
  {
    if (strncmp(argv[1], "-psn", 4) == 0)
    {
      char		*ptr;		// Pointer into basedir
      static char	basedir[1024];	// Base directory (static so it can be used below)


      strlcpy(basedir, argv[0], sizeof(basedir));
      if ((ptr = strrchr(basedir, '/')) != NULL)
        *ptr = '\0';
      if ((ptr = strrchr(basedir, '/')) != NULL && !strcasecmp(ptr, "/MacOS"))
      {
        // Got the base directory, now add "Resources" to it...
	*ptr = '\0';
	strlcat(basedir, "/Resources", sizeof(basedir));
	distdir = basedir;
	chdir(basedir);
      }
      else
        distdir = ".";

    }
    else
      distdir = argv[1];
  }
  else
    distdir = ".";
#else
  // Get the directory that has the software in it...
  if (argc > 1)
    distdir = argv[1];
  else
    distdir = ".";
#endif // __APPLE__

  w = make_window();

  Pane[PANE_WELCOME]->show();
  PrevButton->deactivate();
  NextButton->deactivate();

  gui_get_installed();
  get_dists(distdir);

  load_image();
  load_readme();
  load_types();

  w->show();

  while (!w->visible())
    Fl::wait();

#ifdef __APPLE__
  OSStatus status;

  status = AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment,
                               kAuthorizationFlagDefaults,
			       &SetupAuthorizationRef);
  if (status != errAuthorizationSuccess)
  {
    fl_alert("You must have administrative priviledges to install this software!");
    return (1);
  }

  AuthorizationItem	items = { kAuthorizationRightExecute, 0, NULL, 0 };
  AuthorizationRights	rights = { 1, &items };

  status = AuthorizationCopyRights(SetupAuthorizationRef, &rights, NULL,
				   kAuthorizationFlagDefaults |
				       kAuthorizationFlagInteractionAllowed |
				       kAuthorizationFlagPreAuthorize |
				       kAuthorizationFlagExtendRights, NULL);

  if (status != errAuthorizationSuccess)
  {
    fl_alert("You must have administrative priviledges to install this software!");
    return (1);
  }
#else
  if (getuid() != 0)
  {
    fl_alert("You must be logged in as root to run setup!");
    return (1);
  }
#endif // __APPLE__

  NextButton->activate();

  Fl::run();

#ifdef __APPLE__
  AuthorizationFree(SetupAuthorizationRef, kAuthorizationFlagDefaults);
#endif // __APPLE__

  return (0);
}


//
// 'get_dists()' - Get a list of available software products.
//

void
get_dists(const char *d)	// I - Directory to look in
{
  int		i;		// Looping var
  int		num_files;	// Number of files
  dirent	**files;	// Files
  const char	*ext;		// Extension
  gui_dist_t	*temp,		// Pointer to current distribution
		*installed;	// Pointer to installed product
  FILE		*fp;		// File to read from
  char		line[1024],	// Line from file...
		product[64];	// Product name...
  int		lowver, hiver;	// Version numbers for dependencies


  // Get the files in the specified directory...
  if (chdir(d))
  {
    perror("setup: Unable to change to distribution directory");
    exit(1);
  }

  if ((num_files = fl_filename_list(".", &files)) == 0)
  {
    fputs("setup: Error - no software products found!\n", stderr);
    exit(1);
  }

  // Build a distribution list...
  NumDists = 0;
  Dists    = (gui_dist_t *)0;

  for (i = 0; i < num_files; i ++)
  {
    ext = fl_filename_ext(files[i]->d_name);
    if (!strcmp(ext, ".install") || !strcmp(ext, ".patch"))
    {
      // Found a .install script...
      if ((fp = fopen(files[i]->d_name, "r")) == NULL)
      {
        perror("setup: Unable to open installation script");
	exit(1);
      }

      // Add a new distribution entry...
      temp           = gui_add_dist(&NumDists, &Dists);
      temp->type     = PACKAGE_PORTABLE;
      temp->filename = strdup(files[i]->d_name);

      strncpy(temp->product, files[i]->d_name, sizeof(temp->product) - 1);
      *strrchr(temp->product, '.') = '\0';	// Drop .install

      // Read info from the installation script...
      while (fgets(line, sizeof(line), fp) != NULL)
      {
        // Only read distribution info lines...
        if (strncmp(line, "#%", 2))
	  continue;

        // Drop the trailing newline...
        line[strlen(line) - 1] = '\0';

        // Copy data as needed...
	if (strncmp(line, "#%product ", 10) == 0)
	  strncpy(temp->name, line + 10, sizeof(temp->name) - 1);
	else if (strncmp(line, "#%version ", 10) == 0)
	  sscanf(line + 10, "%31s%d", temp->version, &(temp->vernumber));
	else if (strncmp(line, "#%rootsize ", 11) == 0)
	  temp->rootsize = atoi(line + 11);
	else if (strncmp(line, "#%usrsize ", 10) == 0)
	  temp->usrsize = atoi(line + 10);
	else if (strncmp(line, "#%incompat ", 11) == 0 ||
	         strncmp(line, "#%requires ", 11) == 0)
	{
	  lowver = 0;
	  hiver  = 0;

	  if (sscanf(line + 11, "%s%*s%d%*s%d", product, &lowver, &hiver) > 0)
	    gui_add_depend(temp, (line[2] == 'i') ? DEPEND_INCOMPAT :
	                                            DEPEND_REQUIRES,
	        	   product, lowver, hiver);
        }
      }

      fclose(fp);
    }
    else if (!strcmp(ext, ".rpm"))
    {
      // Found an RPM package...
      char	*version,		// Version number
		*size,			// Size of package
		*description;		// Summary string


      // Get the package information...
      snprintf(line, sizeof(line),
               "rpm -qp --qf '%%{NAME}|%%{VERSION}|%%{SIZE}|%%{SUMMARY}\\n' %s",
	       files[i]->d_name);

      if ((fp = popen(line, "r")) == NULL)
      {
        free(files[i]);
	continue;
      }

      if (!fgets(line, sizeof(line), fp))
      {
        free(files[i]);
	continue;
      }

      // Drop the trailing newline...
      line[strlen(line) - 1] = '\0';

      // Grab the different fields...
      if ((version = strchr(line, '|')) == NULL)
        continue;
      *version++ = '\0';

      if ((size = strchr(version, '|')) == NULL)
        continue;
      *size++ = '\0';

      if ((description = strchr(size, '|')) == NULL)
        continue;
      *description++ = '\0';

      // Add a new distribution entry...
      temp           = gui_add_dist(&NumDists, &Dists);
      temp->type     = PACKAGE_RPM;
      temp->filename = strdup(files[i]->d_name);

      strlcpy(temp->product, line, sizeof(temp->product));
      strlcpy(temp->name, description, sizeof(temp->name));
      strlcpy(temp->version, version, sizeof(temp->version));
      temp->vernumber = get_vernumber(version);
      temp->rootsize  = (int)(atof(size) / 1024.0 + 0.5);

      pclose(fp);
    }

    free(files[i]);
  }

  free(files);

  if (NumDists == 0)
  {
    fl_alert("No software found to install!");
    exit(1);
  }

  if (NumDists > 1)
    qsort(Dists, NumDists, sizeof(gui_dist_t), (compare_func_t)gui_sort_dists);

  for (i = 0, temp = Dists; i < NumDists; i ++, temp ++)
  {
    sprintf(line, "%s v%s", temp->name, temp->version);

    if ((installed = gui_find_dist(temp->product, NumInstalled,
                                   Installed)) == NULL)
    {
      strcat(line, " (new)");
      SoftwareList->add(line, 0);
    }
    else if (installed->vernumber > temp->vernumber)
    {
      strcat(line, " (downgrade)");
      SoftwareList->add(line, 0);
    }
    else if (installed->vernumber == temp->vernumber)
    {
      strcat(line, " (installed)");
      SoftwareList->add(line, 0);
    }
    else
    {
      strcat(line, " (upgrade)");
      SoftwareList->add(line, 1);
    }
  }

  update_sizes();
}


//
// 'install_dist()' - Install a distribution...
//

int					// O - Install status
install_dist(const gui_dist_t *dist)	// I - Distribution to install
{
  char		command[1024];		// Command string
  int		fds[2];			// Pipe FDs
  int		status;			// Exit status
#ifndef __APPLE__
  int		pid;			// Process ID
#endif // !__APPLE__


  sprintf(command, "**** %s ****", dist->name);
  InstallLog->add(command);

#ifdef __APPLE__
  // Run the install script using Apple's authorization API...
  FILE		*fp = NULL;
  char		*args[2] = { (char *)"now", NULL };
  OSStatus	astatus;


  astatus = AuthorizationExecuteWithPrivileges(SetupAuthorizationRef,
                                               dist->filename,
                                               kAuthorizationFlagDefaults,
                                               args, &fp);

  if (astatus != errAuthorizationSuccess)
  {
    InstallLog->add("Failed to execute install script!");
    return (1);
  }

  fds[0] = fileno(fp);
#else
  // Fork the command and redirect errors and info to stdout...
  pipe(fds);

  if ((pid = fork()) == 0)
  {
    // Child comes here; start by redirecting stdout and stderr...
    close(1);
    close(2);
    dup(fds[1]);
    dup(fds[1]);

    // Close the original pipes...
    close(fds[0]);
    close(fds[1]);

    // Execute the command; if an error occurs, return it...
    if (dist->type == PACKAGE_PORTABLE)
      execl(dist->filename, dist->filename, "now", (char *)0);
    else
      execlp("rpm", "rpm", "-U", "--nodeps", dist->filename, (char *)0);

    exit(errno);
  }
  else if (pid < 0)
  {
    // Unable to fork!
    sprintf(command, "Unable to install %s:", dist->name);
    InstallLog->add(command);

    sprintf(command, "\t%s", strerror(errno));
    InstallLog->add(command);

    close(fds[0]);
    close(fds[1]);

    return (1);
  }

  // Close the output pipe (used by the child)...
  close(fds[1]);
#endif // __APPLE__

  // Listen for data on the input pipe...
  Fl::add_fd(fds[0], (void (*)(int, void *))log_cb, fds);

  // Show the user that we're busy...
  SetupWindow->cursor(FL_CURSOR_WAIT);

  // Loop until the child is done...
  while (fds[0])	// log_cb() will close and zero fds[0]...
  {
    // Wait for events...
    Fl::wait();

#ifndef __APPLE__
    // Check to see if the child went away...
    if (waitpid(0, &status, WNOHANG) == pid)
      break;
#endif // !__APPLE__
  }

#ifdef __APPLE__
  fclose(fp);
#endif // __APPLE__

  if (fds[0])
  {
    // Close the pipe - have all the data from the child...
    Fl::remove_fd(fds[0]);
    close(fds[0]);
  }
  else
  {
    // Get the child's exit status...
    wait(&status);
  }

  // Show the user that we're ready...
  SetupWindow->cursor(FL_CURSOR_DEFAULT);

  // Return...
  return (status);
}


//
// 'license_dist()' - Show the license for a distribution...
//

int					// O - 0 if accepted, 1 if not
license_dist(const gui_dist_t *dist)	// I - Distribution to license
{
  char		licfile[1024];		// License filename
  struct stat	licinfo;		// License file info
  static int	liclength = 0;		// Size of license file
  static char	liclabel[1024];		// Label for license pane


  // See if we need to show the license file...
  sprintf(licfile, "%s.license", dist->product);
  if (!stat(licfile, &licinfo) && licinfo.st_size != liclength)
  {
    // Save this license's length...
    liclength = licinfo.st_size;

    // Set the title string...
    snprintf(liclabel, sizeof(liclabel), "Software License for %s:", dist->name);
    LicenseFile->label(liclabel);

    // Load the license into the viewer...
    LicenseFile->textfont(FL_HELVETICA);
    LicenseFile->textsize(14);

    gui_load_file(LicenseFile, licfile);

    // Show the license window and wait for the user...
    Pane[PANE_LICENSE]->show();
    Title[PANE_LICENSE]->activate();
    LicenseAccept->clear();
    LicenseDecline->clear();
    NextButton->deactivate();
    CancelButton->activate();

    while (Pane[PANE_LICENSE]->visible())
      Fl::wait();

    Title[PANE_INSTALL]->deactivate();

    CancelButton->deactivate();
    NextButton->deactivate();

    if (LicenseDecline->value())
    {
      // Can't install without acceptance...
      char	message[1024];		// Message for log


      liclength = 0;
      snprintf(message, sizeof(message), "License not accepted for %s!",
               dist->name);
      InstallLog->add(message);
      return (1);
    }
  }

  return (0);
}


//
// 'list_cb()' - Handle selections in the software list.
//

void
list_cb(Fl_Check_Browser *, void *)
{
  int		i, j, k;
  gui_dist_t	*dist,
		*dist2;
  gui_depend_t	*depend;


  if (SoftwareList->nchecked() == 0)
  {
    update_sizes();

    NextButton->deactivate();
    return;
  }

  for (i = 0, dist = Dists; i < NumDists; i ++, dist ++)
    if (SoftwareList->checked(i + 1))
    {
      // Check for required/incompatible products...
      for (j = 0, depend = dist->depends; j < dist->num_depends; j ++, depend ++)
        switch (depend->type)
	{
	  case DEPEND_REQUIRES :
	      if ((dist2 = gui_find_dist(depend->product, NumDists,
	                                 Dists)) != NULL)
	      {
  		// Software is in the list, is it selected?
	        k = dist2 - Dists;

		if (SoftwareList->checked(k + 1))
		  continue;

        	// Nope, select it unless we're unchecked another selection...
		if (SoftwareList->value() != (k + 1))
		  SoftwareList->checked(k + 1, 1);
		else
		{
		  SoftwareList->checked(i + 1, 0);
		  break;
		}
	      }
	      else if ((dist2 = gui_find_dist(depend->product, NumInstalled,
	                                      Installed)) == NULL)
	      {
		// Required but not installed or available!
		fl_alert("%s requires %s to be installed, but it is not available "
	        	 "for installation.", dist->name, depend->product);
		SoftwareList->checked(i + 1, 0);
		break;
	      }
	      break;

          case DEPEND_INCOMPAT :
	      if ((dist2 = gui_find_dist(depend->product, NumInstalled,
	                                 Installed)) != NULL)
	      {
		// Already installed!
		fl_alert("%s is incompatible with %s. Please remove it before "
	        	 "installing this software.", dist->name, dist2->name);
		SoftwareList->checked(i + 1, 0);
		break;
	      }
	      else if ((dist2 = gui_find_dist(depend->product, NumDists,
	                                      Dists)) != NULL)
	      {
  		// Software is in the list, is it selected?
	        k = dist2 - Dists;

		// Software is in the list, is it selected?
		if (!SoftwareList->checked(k + 1))
		  continue;

        	// Yes, tell the user...
		fl_alert("%s is incompatible with %s. Please deselect it before "
	        	 "installing this software.", dist->name, dist2->name);
		SoftwareList->checked(i + 1, 0);
		break;
	      }
	  default :
	      break;
	}
    }

  update_sizes();

  if (SoftwareList->nchecked())
    NextButton->activate();
  else
    NextButton->deactivate();
}


//
// 'load_image()' - Load the setup image file (setup.gif/xpm)...
//

void
load_image(void)
{
  Fl_Image	*img;			// New image


  if (!access("setup.xpm", 0))
    img = new Fl_XPM_Image("setup.xpm");
  else if (!access("setup.gif", 0))
    img = new Fl_GIF_Image("setup.gif");
  else
    img = NULL;

  if (img)
    WelcomeImage->image(img);
}


//
// 'load_readme()' - Load the readme file...
//

void
load_readme(void)
{
  ReadmeFile->textfont(FL_HELVETICA);
  ReadmeFile->textsize(14);

  if (access("setup.readme", 0))
  {
    int		i;			// Looping var
    gui_dist_t	*dist;			// Current distribution
    char	*buffer,		// Text buffer
		*ptr;			// Pointer into buffer


    buffer = new char[1024 + NumDists * 300];

    strcpy(buffer,
           "This program allows you to install the following software:\n"
	   "<ul>\n");
    ptr = buffer + strlen(buffer);

    for (i = NumDists, dist = Dists; i > 0; i --, dist ++)
    {
      sprintf(ptr, "<li>%s, %s\n", dist->name, dist->version);
      ptr += strlen(ptr);
    }

    strcpy(ptr, "</ul>");

    ReadmeFile->value(buffer);

    delete[] buffer;
  }
  else
    gui_load_file(ReadmeFile, "setup.readme");
 
}


//
// 'load_types()' - Load the installation types from the setup.types file.
//

void
load_types(void)
{
  int		i;		// Looping var
  FILE		*fp;		// File to read from
  char		line[1024],	// Line from file
		*lineptr,	// Pointer into line
		*sep;		// Separator
  gui_intype_t	*dt;		// Current install type
  gui_dist_t	*dist;		// Distribution


  NumInstTypes = 0;

  if ((fp = fopen("setup.types", "r")) != NULL)
  {
    dt = NULL;

    while (fgets(line, sizeof(line), fp) != NULL)
    {
      if (line[0] == '#' || line[0] == '\n')
        continue;

      lineptr = line + strlen(line) - 1;
      if (*lineptr == '\n')
        *lineptr = '\0';

      if (!strncasecmp(line, "TYPE", 4) && isspace(line[4]))
      {
        // New type...
	if (NumInstTypes >= (int)(sizeof(InstTypes) / sizeof(InstTypes[0])))
	{
	  fprintf(stderr, "setup: Too many TYPEs (> %d) in setup.types!\n",
	          (int)(sizeof(InstTypes) / sizeof(InstTypes[0])));
	  fclose(fp);
	  exit(1);
	}

        // Skip whitespace...
        for (lineptr = line + 5; isspace(*lineptr); lineptr ++);

	// Copy name and label string...
        dt = InstTypes + NumInstTypes;
	NumInstTypes ++;

	memset(dt, 0, sizeof(gui_intype_t));
	if ((sep = strchr(lineptr, '/')) != NULL)
	  *sep++ = '\0';
	else
	  sep = lineptr;

	strncpy(dt->name, lineptr, sizeof(dt->name));
	strncpy(dt->label, sep, sizeof(dt->label) - 10);
      }
      else if (!strncasecmp(line, "INSTALL", 7) && dt && isspace(line[7]))
      {
        // Install a product...
	if (dt->num_products >= (int)(sizeof(dt->products) / sizeof(dt->products[0])))
	{
	  fprintf(stderr, "setup: Too many INSTALLs (> %d) in setup.types!\n",
	          (int)(sizeof(dt->products) / sizeof(dt->products[0])));
	  fclose(fp);
	  exit(1);
	}

        // Skip whitespace...
        for (lineptr = line + 8; isspace(*lineptr); lineptr ++);

	// Add product to list...
        if ((dist = gui_find_dist(lineptr, NumDists, Dists)) != NULL)
	{
          dt->products[dt->num_products] = dist - Dists;
	  dt->num_products ++;
	  dt->size += dist->rootsize + dist->usrsize;

          if ((dist = gui_find_dist(lineptr, NumInstalled, Installed)) != NULL)
	    dt->size -= dist->rootsize + dist->usrsize;
	}
	else
	  fprintf(stderr, "setup: Unable to find product \"%s\" for \"%s\"!\n",
	          lineptr, dt->label);
      }
      else
      {
        fprintf(stderr, "setup: Bad line - %s\n", line);
	fclose(fp);
	exit(1);
      }
    }

    fclose(fp);
  }
  else
  {
    Title[PANE_TYPE]->hide();
    Title[PANE_SELECT]->position(10, 35);
    Title[PANE_CONFIRM]->position(10, 60);
    Title[PANE_LICENSE]->position(10, 85);
    Title[PANE_INSTALL]->position(10, 110);
  }

  for (i = 0, dt = InstTypes; i < NumInstTypes; i ++, dt ++)
  {
    if (dt->size >= 1024)
      sprintf(dt->label + strlen(dt->label), " (+%.1fm disk space)",
              dt->size / 1024.0);
    else if (dt->size)
      sprintf(dt->label + strlen(dt->label), " (+%dk disk space)", dt->size);

    if ((lineptr = strchr(dt->label, '/')) != NULL)
      TypeButton[i]->label(lineptr + 1);
    else
      TypeButton[i]->label(dt->label);

    TypeButton[i]->show();
  }

  for (; i < (int)(sizeof(TypeButton) / sizeof(TypeButton[0])); i ++)
    TypeButton[i]->hide();
}


//
// 'log_cb()' - Add one or more lines of text to the installation log.
//

void
log_cb(int fd,			// I - Pipe to read from
       int *fdptr)		// O - Pipe to read from
{
  int		bytes;		// Bytes read/to read
  char		*bufptr;	// Pointer into buffer
  static int	bufused = 0;	// Number of bytes used
  static char	buffer[8193];	// Buffer


  bytes = 8192 - bufused;
  if ((bytes = read(fd, buffer + bufused, bytes)) <= 0)
  {
    // End of file; zero the FD to tell the install_dist() function to
    // stop...

    Fl::remove_fd(fd);
    close(fd);
    *fdptr = 0;

    if (bufused > 0)
    {
      // Add remaining text...
      buffer[bufused] = '\0';
      InstallLog->add(buffer);
      bufused = 0;
    }
  }
  else
  {
    // Add bytes to the buffer, then add lines as needed...
    bufused += bytes;
    buffer[bufused] = '\0';

    while ((bufptr = strchr(buffer, '\n')) != NULL)
    {
      *bufptr++ = '\0';
      InstallLog->add(buffer);
      strcpy(buffer, bufptr);
      bufused -= bufptr - buffer;
    }
  }

  InstallLog->bottomline(InstallLog->size());
}


//
// 'next_cb()' - Show software selections or install software.
//

void
next_cb(Fl_Button *, void *)
{
  int		i;			// Looping var
  int		progress;		// Progress so far...
  int		error;			// Errors?
  static char	message[1024];		// Progress message...
  static char	install_type[1024];	// EPM_INSTALL_TYPE env variable
  static int	installing = 0;		// Installing software?


  Wizard->next();

  PrevButton->deactivate();

  if (Wizard->value() == Pane[PANE_TYPE])
  {
    if (NumInstTypes == 0)
      Wizard->next();
  }
  else if (Wizard->value() == Pane[PANE_SELECT])
  {
    if (NumInstTypes)
      PrevButton->activate();

    // Figure out which type is chosen...
    for (i = 0; i < (int)(sizeof(TypeButton) / sizeof(TypeButton[0])); i ++)
      if (TypeButton[i]->value())
        break;

    if (i < (int)(sizeof(TypeButton) / sizeof(TypeButton[0])))
    {
      // Set the EPM_INSTALL_TYPE environment variable...
      snprintf(install_type, sizeof(install_type), "EPM_INSTALL_TYPE=%s",
               InstTypes[i].name);
      putenv(install_type);

      // Skip product selection if this type has a list already...
      if (InstTypes[i].num_products > 0)
        Wizard->next();
    }
  }

  if (Wizard->value() == Pane[PANE_CONFIRM])
  {
    ConfirmList->clear();
    PrevButton->activate();

    for (i = 0; i < NumDists; i ++)
      if (SoftwareList->checked(i + 1))
        ConfirmList->add(SoftwareList->text(i + 1));
  }
  else if (Wizard->value() == Pane[PANE_LICENSE])
    Wizard->next();

  if (Wizard->value() == Pane[PANE_INSTALL] && !installing)
  {
    // Show the licenses for each of the selected software packages...
    installing = 1;

    for (i = 0, progress = 0, error = 0; i < NumDists; i ++)
      if (SoftwareList->checked(i + 1) && license_dist(Dists + i))
      {
        InstallPercent->label("Installation Canceled!");
	Pane[PANE_INSTALL]->redraw();

        CancelButton->label("Close");
	CancelButton->activate();

	fl_beep();
	return;
      }

    // Then do the installs...
    NextButton->deactivate();
    CancelButton->deactivate();
    CancelButton->label("Close");

    for (i = 0, progress = 0, error = 0; i < NumDists; i ++)
      if (SoftwareList->checked(i + 1))
      {
        sprintf(message, "Installing %s v%s...", Dists[i].name,
	        Dists[i].version);

        InstallPercent->value(100.0 * progress / SoftwareList->nchecked());
	InstallPercent->label(message);
	Pane[PANE_INSTALL]->redraw();

        if ((error = install_dist(Dists + i)) != 0)
	  break;

	progress ++;
      }

    InstallPercent->value(100.0);

    if (error)
      InstallPercent->label("Installation Failed!");
    else
      InstallPercent->label("Installation Complete");

    Pane[PANE_INSTALL]->redraw();

    CancelButton->activate();

    fl_beep();

    installing = 0;
  }
  else if (Wizard->value() == Pane[PANE_SELECT] &&
           SoftwareList->nchecked() == 0)
    NextButton->deactivate();

  for (i = 0; i <= PANE_INSTALL; i ++)
  {
    Title[i]->activate();

    if (Pane[i]->visible())
      break;
  }
}


//
// 'type_cb()' - Handle selections in the type list.
//

void
type_cb(Fl_Round_Button *w, void *)	// I - Radio button widget
{
  int		i;		// Looping var
  gui_intype_t	*dt;		// Current install type
  gui_dist_t	*temp,		// Current software
		*installed;	// Installed software


  for (i = 0; i < (int)(sizeof(TypeButton) / sizeof(TypeButton[0])); i ++)
    if (w == TypeButton[i])
      break;

  if (i >= (int)(sizeof(TypeButton) / sizeof(TypeButton[0])))
    return;

  dt = InstTypes + i;

  SoftwareList->check_none();

  // Select all products in the install type
  for (i = 0; i < dt->num_products; i ++)
    SoftwareList->checked(dt->products[i] + 1, 1);

  // And then any upgrade products...
  for (i = 0, temp = Dists; i < NumDists; i ++, temp ++)
  {
    if ((installed = gui_find_dist(temp->product, NumInstalled,
                                   Installed)) != NULL &&
        installed->vernumber < temp->vernumber)
      SoftwareList->checked(i + 1, 1);
  }

  update_sizes();

  NextButton->activate();
}


//
// 'update_size()' - Update the total +/- sizes of the installations.
//

void
update_sizes(void)
{
  int		i;		// Looping var
  gui_dist_t	*dist,		// Distribution
		*installed;	// Installed distribution
  int		rootsize,	// Total root size difference in kbytes
		usrsize;	// Total /usr size difference in kbytes
  struct statfs	rootpart,	// Available root partition
		usrpart;	// Available /usr partition
  int		rootfree,	// Free space on root partition
		usrfree;	// Free space on /usr partition
  static char	sizelabel[1024];// Label for selected sizes...


  // Get the sizes for the selected products...
  for (i = 0, dist = Dists, rootsize = 0, usrsize = 0;
       i < NumDists;
       i ++, dist ++)
    if (SoftwareList->checked(i + 1))
    {
      rootsize += dist->rootsize;
      usrsize  += dist->usrsize;

      if ((installed = gui_find_dist(dist->product, NumInstalled,
                                     Installed)) != NULL)
      {
        rootsize -= installed->rootsize;
	usrsize  -= installed->usrsize;
      }
    }

  // Get the sizes of the root and /usr partition...
#if defined(__sgi) || defined(__svr4__) || defined(__SVR4) || defined(M_XENIX)
  if (statfs("/", &rootpart, sizeof(rootpart), 0))
#else
  if (statfs("/", &rootpart))
#endif // __sgi || __svr4__ || __SVR4 || M_XENIX
    rootfree = 1024;
  else
    rootfree = (int)((double)rootpart.f_bfree * (double)rootpart.f_bsize /
                     1024.0 / 1024.0 + 0.5);

#if defined(__sgi) || defined(__svr4__) || defined(__SVR4) || defined(M_XENIX)
  if (statfs("/usr", &usrpart, sizeof(usrpart), 0))
#else
  if (statfs("/usr", &usrpart))
#endif // __sgi || __svr4__ || __SVR4 || M_XENIX
    usrfree = 1024;
  else
    usrfree = (int)((double)usrpart.f_bfree * (double)usrpart.f_bsize /
                    1024.0 / 1024.0 + 0.5);

  // Display the results to the user...
  if (rootfree == usrfree)
  {
    rootsize += usrsize;

    if (rootsize >= 1024)
      snprintf(sizelabel, sizeof(sizelabel),
               "%+.1fm required, %dm available.", rootsize / 1024.0,
               rootfree);
    else
      snprintf(sizelabel, sizeof(sizelabel),
               "%+dk required, %dm available.", rootsize, rootfree);
  }
  else if (rootsize >= 1024 && usrsize >= 1024)
    snprintf(sizelabel, sizeof(sizelabel),
             "%+.1fm required on /, %dm available,\n"
             "%+.1fm required on /usr, %dm available.",
             rootsize / 1024.0, rootfree, usrsize / 1024.0, usrfree);
  else if (rootsize >= 1024)
    snprintf(sizelabel, sizeof(sizelabel),
             "%+.1fm required on /, %dm available,\n"
             "%+dk required on /usr, %dm available.",
             rootsize / 1024.0, rootfree, usrsize, usrfree);
  else if (usrsize >= 1024)
    snprintf(sizelabel, sizeof(sizelabel),
             "%+dk required on /, %dm available,\n"
             "%+.1fm required on /usr, %dm available.",
             rootsize, rootfree, usrsize / 1024.0, usrfree);
  else
    snprintf(sizelabel, sizeof(sizelabel),
             "%+dk required on /, %dm available,\n"
             "%+dk required on /usr, %dm available.",
             rootsize, rootfree, usrsize, usrfree);

  SoftwareSize->label(sizelabel);
  SoftwareSize->redraw();
}


//
// End of "$Id: setup2.cxx 834 2010-12-30 00:08:36Z mike $".
//

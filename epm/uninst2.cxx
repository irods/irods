//
// "$Id: uninst2.cxx 834 2010-12-30 00:08:36Z mike $"
//
//   ESP Software Removal Wizard main entry for the ESP Package Manager (EPM).
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
//   main()           - Main entry for software wizard...
//   list_cb()        - Handle selections in the software list.
//   load_image()     - Load the setup image file (setup.gif/xpm)...
//   load_readme()    - Load the readme file...
//   log_cb()         - Add one or more lines of text to the removal log.
//   next_cb()        - Show software selections or remove software.
//   remove_dist()    - Remove a distribution...
//   show_installed() - Show the installed software products.
//   update_size()    - Update the total +/- sizes of the installations.
//

#define _DEFINE_GLOBALS_
#include "uninst.h"
#include <FL/x.H>
#include <FL/filename.H>
#include <FL/fl_ask.H>
#include <FL/Fl_XPM_Image.H>
#include <FL/Fl_GIF_Image.H>
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
#define PANE_SELECT	1
#define PANE_CONFIRM	2
#define PANE_REMOVE	3


//
// Local functions...
//

void	load_image(void);
void	load_readme(void);
void	log_cb(int fd, int *fdptr);
int	remove_dist(const gui_dist_t *dist);
void	show_installed(void);
void	update_sizes(void);


//
// 'main()' - Main entry for software wizard...
//

int				// O - Exit status
main(int  argc,			// I - Number of command-line arguments
     char *argv[])		// I - Command-line arguments
{
  Fl_Window	*w;		// Main window...


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
	chdir(basedir);
      }
    }
  }
#endif // __APPLE__

  w = make_window();

  Pane[PANE_WELCOME]->show();
  PrevButton->deactivate();
  NextButton->deactivate();

  gui_get_installed();
  show_installed();

  load_image();
  load_readme();

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
    fl_alert("You must have administrative priviledges to remove this software!");
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
    fl_alert("You must have administrative priviledges to remove this software!");
    return (1);
  }
#else
  if (getuid() != 0)
  {
    fl_alert("You must be logged in as root to run uninstall!");
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

  for (i = 0, dist = Installed; i < NumInstalled; i ++, dist ++)
    if (SoftwareList->checked(i + 1))
    {
      // Check for required/incompatible products...
      for (j = 0, depend = dist->depends; j < dist->num_depends; j ++, depend ++)
        switch (depend->type)
	{
	  case DEPEND_REQUIRES :
	      if ((dist2 = gui_find_dist(depend->product, NumInstalled,
	                                 Installed)) != NULL)
	      {
  		// Software is in the list, is it selected?
	        k = dist2 - Installed;

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
	      else if ((dist2 = gui_find_dist(depend->product, NumInstalled,
	                                      Installed)) != NULL)
	      {
  		// Software is in the list, is it selected?
	        k = dist2 - Installed;

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

  if (access("uninst.readme", 0))
  {
    int		i;			// Looping var
    gui_dist_t	*dist;			// Current distribution
    char	*buffer,		// Text buffer
		*ptr;			// Pointer into buffer


    buffer = new char[1024 + NumInstalled * 400];

    strcpy(buffer, "This program allows you to remove the following "
                   "software:</p>\n"
		   "<ul>\n");
    ptr = buffer + strlen(buffer);

    for (i = NumInstalled, dist = Installed; i > 0; i --, dist ++)
    {
      sprintf(ptr, "<li>%s, %s\n", dist->name, dist->version);
      ptr += strlen(ptr);
    }

    strcpy(ptr, "</ul>");

    ReadmeFile->value(buffer);

    delete[] buffer;
  }
  else
    gui_load_file(ReadmeFile, "uninst.readme");

}


//
// 'log_cb()' - Add one or more lines of text to the removal log.
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
    // End of file; zero the FD to tell the remove_dist() function to
    // stop...

    Fl::remove_fd(fd);
    close(fd);
    *fdptr = 0;

    if (bufused > 0)
    {
      // Add remaining text...
      buffer[bufused] = '\0';
      RemoveLog->add(buffer);
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
      RemoveLog->add(buffer);
      strcpy(buffer, bufptr);
      bufused -= bufptr - buffer;
    }
  }

  RemoveLog->bottomline(RemoveLog->size());
}


//
// 'next_cb()' - Show software selections or remove software.
//

void
next_cb(Fl_Button *, void *)
{
  int		i;		// Looping var
  int		progress;	// Progress so far...
  int		error;		// Errors?
  static char	message[1024];	// Progress message...
  static int	removing = 0;	// Removing software?


  Wizard->next();

  PrevButton->deactivate();

  if (Wizard->value() == Pane[PANE_CONFIRM])
  {
    ConfirmList->clear();
    PrevButton->activate();

    for (i = 0; i < NumInstalled; i ++)
      if (SoftwareList->checked(i + 1))
        ConfirmList->add(SoftwareList->text(i + 1));
  }

  if (Wizard->value() == Pane[PANE_REMOVE] && !removing)
  {
    removing = 1;

    NextButton->deactivate();
    CancelButton->deactivate();
    CancelButton->label("Close");

    for (i = 0, progress = 0, error = 0; i < NumInstalled; i ++)
      if (SoftwareList->checked(i + 1))
      {
        sprintf(message, "Removing %s v%s...", Installed[i].name,
	        Installed[i].version);

        RemovePercent->value(100.0 * progress / SoftwareList->nchecked());
	RemovePercent->label(message);
	Pane[PANE_REMOVE]->redraw();

        if ((error = remove_dist(Installed + i)) != 0)
	  break;

	progress ++;
      }

    RemovePercent->value(100.0);

    if (error)
      RemovePercent->label("Removal Failed!");
    else
      RemovePercent->label("Removal Complete");

    Pane[PANE_REMOVE]->redraw();

    CancelButton->activate();

    fl_beep();

    removing = 0;
  }
  else if (Wizard->value() == Pane[PANE_SELECT] &&
           SoftwareList->nchecked() == 0)
    NextButton->deactivate();
}


//
// 'remove_dist()' - Remove a distribution...
//

int					// O - Remove status
remove_dist(const gui_dist_t *dist)	// I - Distribution to remove
{
  char		command[1024];		// Command string
  int		fds[2];			// Pipe FDs
  int		status;			// Exit status
#ifndef __APPLE__
  int		pid;			// Process ID
#endif // !__APPLE__

  snprintf(command, sizeof(command), "**** %s ****", dist->name);
  RemoveLog->add(command);

  if (dist->type == PACKAGE_PORTABLE)
    snprintf(command, sizeof(command), EPM_SOFTWARE "/%s.remove",
             dist->product);

#ifdef __APPLE__
  // Run the remove script using Apple's authorization API...
  FILE		*fp = NULL;
  char		*args[2] = { (char *)"now", NULL };
  OSStatus	astatus;


  astatus = AuthorizationExecuteWithPrivileges(SetupAuthorizationRef, command, kAuthorizationFlagDefaults,
                                               args, &fp);

  if (astatus != errAuthorizationSuccess)
  {
    RemoveLog->add("Failed to execute remove script!");
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
      execl(command, command, "now", (char *)0);
    else
      execlp("rpm", "rpm", "-e", "--nodeps", dist->product, (char *)0);

    exit(errno);
  }
  else if (pid < 0)
  {
    // Unable to fork!
    sprintf(command, "Unable to remove %s:", dist->name);
    RemoveLog->add(command);

    sprintf(command, "\t%s", strerror(errno));
    RemoveLog->add(command);

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
  UninstallWindow->cursor(FL_CURSOR_WAIT);

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
  UninstallWindow->cursor(FL_CURSOR_DEFAULT);

  // Return...
  return (status);
}


//
// 'show_installed()' - Show the installed software products.
//

void
show_installed()
{
  int		i;		// Looping var
  gui_dist_t	*temp;		// Pointer to current distribution
  char		line[1024];	// Product name and version...


  if (NumInstalled == 0)
  {
    fl_alert("No software found to remove!");
    exit(1);
  }

  for (i = 0, temp = Installed; i < NumInstalled; i ++, temp ++)
  {
    sprintf(line, "%s v%s", temp->name, temp->version);

    SoftwareList->add(line, 0);
  }

  update_sizes();
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
  for (i = 0, dist = Installed, rootsize = 0, usrsize = 0;
       i < NumInstalled;
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
// End of "$Id: uninst2.cxx 834 2010-12-30 00:08:36Z mike $".
//

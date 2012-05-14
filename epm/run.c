/*
 * "$Id: run.c 611 2005-01-11 21:37:42Z mike $"
 *
 *   External program function for the ESP Package Manager (EPM).
 *
 *   Copyright 1999-2005 by Easy Software Products.
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
 *   run_command() - Run an external program.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"
#include <stdarg.h>
#include <fcntl.h>
#include <sys/wait.h>


/*
 * 'run_command()' - Run an external program.
 */

int					/* O - Exit status */
run_command(const char *directory,	/* I - Directory for command or NULL */
            const char *command,	/* I - Command string */
            ...)			/* I - Additional arguments as needed */
{
  va_list	ap;			/* Argument pointer */
  int		pid,			/* Child process ID */
		status,			/* Status of child */
		argc;			/* Number of arguments */
  char		argbuf[10240],		/* Argument buffer */
		*argptr,		/* Argument string pointer */
		*argv[100];		/* Argument strings */


 /*
  * Format the command string...
  */

  va_start(ap, command);
  vsnprintf(argbuf, sizeof(argbuf) - 1, command, ap);
  argbuf[sizeof(argbuf) - 1] = '\0';

  if (Verbosity > 1)
    puts(argbuf);

 /*
  * Parse the argument string; arguments can be separated by whitespace
  * and quoted by " and '...
  */

  argv[0] = argbuf;

  for (argptr = argbuf, argc = 1; *argptr != '\0' && argc < 99; argptr ++)
    if (isspace(*argptr & 255))
    {
      *argptr++ = '\0';

      while (isspace(*argptr & 255))
        argptr ++;

      if (*argptr != '\0')
      {
        argv[argc] = argptr;
	argc ++;
      }

      argptr --;
    }
    else if (*argptr == '\'')
    {
      if (argptr == argv[argc - 1])
        argv[argc - 1] ++;

      for (argptr ++; *argptr && *argptr != '\''; argptr ++)
        if (*argptr == '\\' && argptr[1])
	  strcpy(argptr, argptr + 1);

      if (*argptr == '\'')
        strcpy(argptr, argptr + 1);

      argptr --;
    }
    else if (*argptr == '\"')
    {
      if (argptr == argv[argc - 1])
        argv[argc - 1] ++;

      for (argptr ++; *argptr && *argptr != '\"'; argptr ++)
        if (*argptr == '\\' && argptr[1])
	  strcpy(argptr, argptr + 1);

      if (*argptr == '\"')
        strcpy(argptr, argptr + 1);

      argptr --;
    }

  argv[argc] = NULL;

 /*
  * Execute the command...
  */

  if ((pid = fork()) == 0)
  {
   /*
    * Child comes here...  Redirect stdin, stdout, and stderr to /dev/null
    * if !Verbosity...
    */

    if (Verbosity < 2)
    {
      close(0);
      close(1);
      close(2);

      open("/dev/null", O_RDWR);
      dup(0);
      dup(0);
    }

   /*
    * Change directories...
    */

    if (directory)
      chdir(directory);

   /*
    * Execute the program; if an error occurs, exit with the UNIX error...
    */

    execvp(argv[0], argv);
    fprintf(stderr, "epm: Unable to execute \"%s\" program: %s\n", argv[0],
            strerror(errno));
    exit(errno);
  }
  else if (pid < 0)
  {
   /*
    * Error - can't fork!
    */

    perror("epm: fork failed");
    return (1);
  }

 /*
  * Fork successful - wait for the child and return the error status...
  */

  if (wait(&status) != pid)
  {
    fputs("epm: Got exit status from wrong program!\n", stderr);
    return (1);
  }
  else if (WIFSIGNALED(status))
    return (-WTERMSIG(status));
  else
    return (WEXITSTATUS(status));
}


/*
 * End of "$Id: run.c 611 2005-01-11 21:37:42Z mike $".
 */

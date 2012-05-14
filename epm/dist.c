/*
 * "$Id: dist.c 843 2010-12-31 02:18:18Z mike $"
 *
 *   Distribution functions for the ESP Package Manager (EPM).
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
 *   add_command()         - Add a command to the distribution...
 *   add_depend()          - Add a dependency to the distribution...
 *   add_description()     - Add a description to the distribution.
 *   add_file()            - Add a file to the distribution.
 *   add_subpackage()      - Add a subpackage to the distribution.
 *   find_subpackage()     - Find a subpackage in the distribution.
 *   free_dist()           - Free memory used by a distribution.
 *   getoption()           - Get an option from a file.
 *   get_platform()        - Get the operating system information...
 *   get_runlevels()       - Get the run levels for the specified init script.
 *   get_start()           - Get the start number for an init script.
 *   get_stop()            - Get the stop number for an init script.
 *   new_dist()            - Create a new, empty software distribution.
 *   read_dist()           - Read a software distribution.
 *   sort_dist_files()     - Sort the files in the distribution.
 *   write_dist()          - Write a distribution list file...
 *   compare_files()       - Compare the destination filenames.
 *   expand_name()         - Expand a filename with environment variables.
 *   get_file()            - Read a file into a string...
 *   get_inline()          - Read inline lines into a string...
 *   get_line()            - Get a line from a file, filtering for uname
 *                           lines...
 *   get_string()          - Get a delimited string from a line.
 *   patmatch()            - Pattern matching...
 *   sort_subpackages()    - Compare two subpackage names.
 *   update_architecture() - Normalize the machine architecture name.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"
#include <pwd.h>


/*
 * Some versions of Solaris don't define gethostname()...
 */

#ifdef __sun
extern int	gethostname(char *, int);
#endif /* __sun */


/*
 * Local functions...
 */

static int	compare_files(const file_t *f0, const file_t *f1);
static void	expand_name(char *buffer, char *name, size_t bufsize, int warn);
static char	*get_file(const char *filename, char *buffer, size_t size);
static char	*get_inline(const char *term, FILE *fp, char *buffer,
		            size_t size);
static char	*get_line(char *buffer, int size, FILE *fp,
		          struct utsname *platform, const char *format,
			  int *skip);
static char	*get_string(char **src, char *dst, size_t dstsize);
static int	patmatch(const char *, const char *);
static int	sort_subpackages(char **a, char **b);
static void	update_architecture(char *buffer, size_t bufsize);


/*
 * Conditional "skip" bits...
 */

#define SKIP_SYSTEM	1		/* Not the right system */
#define SKIP_FORMAT	2		/* Not the right format */
#define SKIP_ARCH	4		/* Not the right architecture */
#define SKIP_IF		8		/* Set if the current #if was not satisfied */
#define SKIP_IFACTIVE	16		/* Set if we're in an #if */
#define SKIP_IFSAT	32		/* Set if an #if statement has been satisfied */
#define SKIP_MASK	15		/* Bits to look at */


/*
 * 'add_command()' - Add a command to the distribution...
 */

void
add_command(dist_t     *dist,		/* I - Distribution */
            FILE       *fp,		/* I - Distribution file */
            int        type,		/* I - Command type */
	    const char *command,	/* I - Command string */
	    const char *subpkg,		/* I - Subpackage */
            const char *section)	/* I - Literal text section */
{
  command_t	*temp;			/* New command */
  char		buf[16384];		/* File import buffer */


  if (!strncmp(command, "<<", 2))
  {
    for (command += 2; isspace(*command & 255); command ++);

    command = get_inline(command, fp, buf, sizeof(buf));
  }
  else if (command[0] == '<' && command[1])
  {
    for (command ++; isspace(*command & 255); command ++);

    command = get_file(command, buf, sizeof(buf));
  }

  if (!command)
    return;

  if (dist->num_commands == 0)
    temp = malloc(sizeof(command_t));
  else
    temp = realloc(dist->commands, (dist->num_commands + 1) * sizeof(command_t));

  if (!temp)
  {
    perror("epm: Out of memory allocating a command");
    return;
  }

  dist->commands   = temp;
  temp             += dist->num_commands;
  temp->type       = type;
  temp->command    = strdup(command);
  if (!temp->command)
  {
    perror("epm: Out of memory duplicating a command string");
    return;
  }
  temp->subpackage = subpkg;
  if (section && *section)
  {
    temp->section = strdup(section);
    if (!temp->section)
    {
      perror("epm: Out of memory duplicating a literal section");
      free(temp->command);
      return;
    }
  }
  else
    temp->section = NULL;

  dist->num_commands ++;
}


/*
 * 'add_depend()' - Add a dependency to the distribution...
 */

void
add_depend(dist_t     *dist,		/* I - Distribution */
           int        type,		/* I - Type of dependency */
	   const char *line,		/* I - Line from file */
	   const char *subpkg)		/* I - Subpackage */
{
  int		i;			/* Looping var */
  depend_t	*temp;			/* New dependency */
  char		*ptr;			/* Pointer into string */
  const char	*lineptr;		/* Temporary pointer into line */


 /*
  * Allocate memory for the dependency...
  */

  if (dist->num_depends == 0)
    temp = malloc(sizeof(depend_t));
  else
    temp = realloc(dist->depends, (dist->num_depends + 1) * sizeof(depend_t));

  if (temp == NULL)
  {
    perror("epm: Out of memory allocating a dependency");
    return;
  }

  dist->depends = temp;
  temp          += dist->num_depends;
  dist->num_depends ++;

 /*
  * Initialize the dependency record...
  */

  memset(temp, 0, sizeof(depend_t));

  temp->type       = type;
  temp->subpackage = subpkg;

 /*
  * Get the product name string...
  */

  for (ptr = temp->product; *line && !isspace(*line & 255); line ++)
    if (ptr < (temp->product + sizeof(temp->product) - 1))
      *ptr++ = *line;

  while (isspace(*line & 255))
    line ++;

 /*
  * Get the version strings, if any...
  */

  for (i = 0; *line && i < 2; i ++)
  {
   /*
    * Handle <version, >version, etc.
    */

    if (!isalnum(*line & 255))
    {
      if (*line == '<' && i == 0)
      {
        strcpy(temp->version[0], "0.0");
	i ++;
      }

      while (!isdigit(*line & 255) && *line)
        line ++;
    }

    if (!*line)
      break;

   /*
    * Grab the version string...
    */

    for (ptr = temp->version[i]; *line && !isspace(*line & 255); line ++)
      if (ptr < (temp->version[i] + sizeof(temp->version[i]) - 1))
	*ptr++ = *line;

    while (isspace(*line & 255))
      line ++;

   /*
    * Get the version number, if any...
    */

    for (lineptr = line; isdigit(*lineptr & 255); lineptr ++);

    if (!*line || (!isspace(*lineptr & 255) && *lineptr))
    {
     /*
      * No version number specified, or the next number is a version
      * string...
      */

      temp->vernumber[i] = get_vernumber(temp->version[i]);
    }
    else
    {
     /*
      * Grab the version number directly from the line...
      */

      temp->vernumber[i] = atoi(line);

      for (line = lineptr; isspace(*line & 255); line ++);
    }
  }

 /*
  * Handle assigning default values based on the number of version numbers...
  */

  switch (i)
  {
    case 0 :
        strcpy(temp->version[0], "0.0");
	/* fall through to set max version number */
    case 1 :
        strcpy(temp->version[1], "999.99.99p99");
	temp->vernumber[1] = INT_MAX;
	break;
  }
}


/*
 * 'add_description()' - Add a description to the distribution.
 */

void
add_description(dist_t     *dist,	/* I - Distribution */
                FILE       *fp,		/* I - Source file */
                const char *description,/* I - Description string */
                const char *subpkg)	/* I - Subpackage name */
{
  description_t	*temp;			/* Temporary description array */
  char		buf[16384];		/* File import buffer */


  if (!strncmp(description, "<<", 2))
  {
    for (description += 2; isspace(*description & 255); description ++);

    description = get_inline(description, fp, buf, sizeof(buf));
  }
  else if (description[0] == '<' && description[1])
  {
    for (description ++; isspace(*description & 255); description ++);

    description = get_file(description, buf, sizeof(buf));
  }

  if (description == NULL)
    return;

  if (dist->num_descriptions == 0)
    temp = malloc(sizeof(description_t));
  else
    temp = realloc(dist->descriptions,
                   (dist->num_descriptions + 1) * sizeof(description_t));

  if (temp == NULL)
  {
    perror("epm: Out of memory adding description");
    return;
  }

  dist->descriptions = temp;
  temp               += dist->num_descriptions;
  dist->num_descriptions ++;

  temp->subpackage = subpkg;

  if ((temp->description = strdup(description)) == NULL)
    perror("epm: Out of memory duplicating description");
}


/*
 * 'add_file()' - Add a file to the distribution.
 */

file_t *				/* O - New file */
add_file(dist_t     *dist,		/* I - Distribution */
         const char *subpkg)		/* I - Subpackage name */
{
  file_t	*file;			/* New file */


  if (dist->num_files == 0)
    dist->files = (file_t *)malloc(sizeof(file_t));
  else
    dist->files = (file_t *)realloc(dist->files, sizeof(file_t) *
					         (dist->num_files + 1));

  file = dist->files + dist->num_files;
  dist->num_files ++;

  file->subpackage = subpkg;

  return (file);
}


/*
 * 'add_subpackage()' - Add a subpackage to the distribution.
 */

char *					/* O - Subpackage pointer */
add_subpackage(dist_t     *dist,	/* I - Distribution */
               const char *subpkg)	/* I - Subpackage name */
{
  char	**temp,				/* Temporary array pointer */
	*s;				/* Subpackage pointer */


  if (dist->num_subpackages == 0)
    temp = malloc(sizeof(char *));
  else
    temp = realloc(dist->subpackages,
                   (dist->num_subpackages + 1) * sizeof(char *));

  if (temp == NULL)
    return (NULL);

  dist->subpackages = temp;
  temp              += dist->num_subpackages;
  dist->num_subpackages ++;

  *temp = s = strdup(subpkg);

  if (dist->num_subpackages > 1)
    qsort(dist->subpackages, (size_t)dist->num_subpackages, sizeof(char *),
          (int (*)(const void *, const void *))sort_subpackages);

 /*
  * Return the new string...
  */

  return (s);
}


/*
 * 'find_subpackage()' - Find a subpackage in the distribution.
 */

char *					/* O - Subpackage pointer */
find_subpackage(dist_t     *dist,	/* I - Distribution */
                const char *subpkg)	/* I - Subpackage name */
{
  char	**match;			/* Matching subpackage */


  if (!subpkg || !*subpkg)
    return (NULL);

  if (dist->num_subpackages > 0)
    match = bsearch(&subpkg, dist->subpackages, (size_t)dist->num_subpackages,
                    sizeof(char *),
                    (int (*)(const void *, const void *))sort_subpackages);
  else
    match = NULL;

  if (match != NULL)
    return (*match);
  else
    return (add_subpackage(dist, subpkg));
}


/*
 * 'free_dist()' - Free memory used by a distribution.
 */

void
free_dist(dist_t *dist)			/* I - Distribution to free */
{
  int	i;				/* Looping var */


  if (dist->num_files > 0)
    free(dist->files);

  for (i = 0; i < dist->num_descriptions; i ++)
    free(dist->descriptions[i].description);

  if (dist->num_descriptions)
    free(dist->descriptions);

  for (i = 0; i < dist->num_subpackages; i ++)
    free(dist->subpackages[i]);

  if (dist->num_subpackages)
    free(dist->subpackages);

  for (i = 0; i < dist->num_commands; i ++)
  {
    free(dist->commands[i].command);
    if (dist->commands[i].section)
      free(dist->commands[i].section);
  }

  if (dist->num_commands)
    free(dist->commands);

  if (dist->num_depends)
    free(dist->depends);

  free(dist);
}



/*
 * 'getoption()' - Get an option from a file.
 */

const char *				/* O - Value */
get_option(file_t     *file,		/* I - File */
           const char *name,		/* I - Name of option */
	   const char *defval)		/* I - Default value of option */
{
  char		*ptr;			/* Pointer to option */
  static char	option[256];		/* Copy of file option */


 /*
  * See if the option exists...
  */

  snprintf(option, sizeof(option), "%s(", name);

  if ((ptr = strstr(file->options, option)) == NULL)
    return (defval);

 /*
  * Yes, copy the value and truncate at the first ")"...
  */

  ptr += strlen(option);
  strcpy(option, ptr);	/* option and file->options are of equal size */

  if ((ptr = strchr(option, ')')) != NULL)
  {
    *ptr = '\0';
    return (option);
  }
  else
    return (defval);
}




/*
 * 'get_platform()' - Get the operating system information...
 */

void
get_platform(struct utsname *platform)	/* O - Platform info */
{
  char	*temp;				/* Temporary pointer */
#ifdef __APPLE__
  FILE	*fp;				/* SystemVersion.plist file */
  char	line[1024],			/* Line from file */
	*ptr;				/* Pointer into line */
  int	major, minor;			/* Major and minor release numbers */
#endif /* __APPLE__ */


 /*
  * Get the system identification information...
  */

  uname(platform);

#ifdef __APPLE__
 /*
  * Try to get the OS version number from the SystemVersion.plist file.
  * If not present, use the uname() results for Darwin...
  */

  if ((fp = fopen("/System/Library/CoreServices/SystemVersion.plist", "r")) != NULL)
  {
    ptr = NULL;

    while (fgets(line, sizeof(line), fp) != NULL)
      if ((ptr = strstr(line, "ProductUserVisibleVersion")) != NULL)
        break;
      else if ((ptr = strstr(line, "ProductVersion")) != NULL)
        break;

    if (ptr && fgets(line, sizeof(line), fp) != NULL)
    {
      major = 10;
      minor = 2;

      if ((ptr = strstr(line, "<string>")) != NULL)
        sscanf(ptr + 8, "%d.%d", &major, &minor);

      sprintf(platform->release, "%d.%d", major, minor);
    }
    else
    {
     /*
      * Couldn't find the version number, so assume it is 10.1...
      */

      strcpy(platform->release, "10.1");
    }

    fclose(fp);

    strcpy(platform->sysname, "macosx");
  }
#endif /* __APPLE__ */

 /*
  * Adjust the CPU type accordingly...
  */

#ifdef __sgi
  strcpy(platform->machine, "mips");
#elif defined(__hpux)
  strcpy(platform->machine, "hppa");
#elif defined(_AIX)
  strcpy(platform->machine, "powerpc");
#else
  update_architecture(platform->machine, sizeof(platform->machine));
#endif /* __sgi */

#ifdef _AIX
 /*
  * AIX stores the major and minor version numbers separately;
  * combine them...
  */

  sprintf(platform->release, "%d.%d", atoi(platform->version),
          atoi(platform->release));
#else
 /*
  * Remove any extra junk from the release number - we just want the
  * major and minor numbers...
  */

  while (!isdigit((int)platform->release[0]) && platform->release[0])
    strcpy(platform->release, platform->release + 1);

  if (platform->release[0] == '.')
    strcpy(platform->release, platform->release + 1);

  for (temp = platform->release; *temp && isdigit(*temp & 255); temp ++);

  if (*temp == '.')
    for (temp ++; *temp && isdigit(*temp & 255); temp ++);

  *temp = '\0';
#endif /* _AIX */

 /*
  * Convert the operating system name to lowercase, and strip out
  * hyphens and underscores...
  */

  for (temp = platform->sysname; *temp != '\0'; temp ++)
    if (*temp == '-' || *temp == '_')
    {
      strcpy(temp, temp + 1);
      temp --;
    }
    else
      *temp = tolower(*temp);

 /*
  * SunOS 5.x is really Solaris 2.x or Solaris X, and OSF1 is really
  * Digital UNIX a.k.a. Compaq Tru64 UNIX...
  */

  if (!strcmp(platform->sysname, "sunos") && platform->release[0] >= '5')
  {
    strcpy(platform->sysname, "solaris");

    if (atoi(platform->release + 2) < 7)
      platform->release[0] -= 3;
    else
    {
     /*
      * Strip 5. from the front of the version number...
      */

      for (temp = platform->release; temp[2]; temp ++)
        *temp = temp[2];

      *temp = '\0';
    }
  }
  else if (!strcmp(platform->sysname, "osf1"))
    strcpy(platform->sysname, "tru64"); /* AKA Digital UNIX */
  else if (!strcmp(platform->sysname, "irix64"))
    strcpy(platform->sysname, "irix"); /* IRIX */

#ifdef DEBUG
  printf("sysname = %s\n", platform->sysname);
  printf("release = %s\n", platform->release);
  printf("machine = %s\n", platform->machine);
#endif /* DEBUG */
}


/*
 * 'get_runlevels()' - Get the run levels for the specified init script.
 */

const char *				/* O - Run levels */
get_runlevels(file_t     *file,		/* I - File */
              const char *deflevels)	/* I - Default run levels */
{
  const char	*runlevels;		/* Pointer to runlevels option */


  if ((runlevels = strstr(file->options, "runlevel(")) != NULL)
    runlevels += 9;
  else if ((runlevels = strstr(file->options, "runlevels(")) != NULL)
    runlevels += 10;			/* Compatible mis-spelling... */
  else
    runlevels = deflevels;

  return (runlevels);
}


/*
 * 'get_start()' - Get the start number for an init script.
 */

int					/* O - Start number */
get_start(file_t *file,			/* I - File */
          int    defstart)		/* I - Default start number */
{
  const char	*start;			/* Pointer to start option */


  if ((start = strstr(file->options, "start(")) != NULL)
    return (atoi(start + 6));
  else
    return (defstart);
}


/*
 * 'get_stop()' - Get the stop number for an init script.
 */

int					/* O - Start number */
get_stop(file_t *file,			/* I - File */
          int    defstop)		/* I - Default stop number */
{
  const char	*stop;			/* Pointer to stop option */


  if ((stop = strstr(file->options, "stop(")) != NULL)
    return (atoi(stop + 5));
  else
    return (defstop);
}


/*
 * 'new_dist()' - Create a new, empty software distribution.
 */

dist_t *				/* O - New distribution */
new_dist(void)
{
 /*
  * Create a new, blank distribution...
  */

  return ((dist_t *)calloc(sizeof(dist_t), 1));
}


/*
 * 'read_dist()' - Read a software distribution.
 */

dist_t *				/* O - New distribution */
read_dist(const char     *filename,	/* I - Main distribution list file */
          struct utsname *platform,	/* I - Platform information */
          const char     *format)	/* I - Format of distribution */
{
  FILE		*listfiles[10];		/* File lists */
  int		listlevel;		/* Level in file list */
  char		line[2048],		/* Expanded line from list file */
		buf[1024];		/* Original line from list file */
  int		type;			/* File type */
  char		dst[256],		/* Destination path */
		src[256],		/* Source path */
		pattern[256],		/* Pattern for source files */
		user[32],		/* User */
		group[32],		/* Group */
		*temp,			/* Temporary pointer */
		options[256];		/* File options */
  mode_t	mode;			/* File permissions */
  int		skip;			/* 1 = skip files, 0 = archive files */
  dist_t	*dist;			/* Distribution data */
  file_t	*file;			/* Distribution file */
  struct stat	fileinfo;		/* File information */
  DIR		*dir;			/* Directory */
  DIRENT	*dent;			/* Directory entry */
  struct passwd	*pwd;			/* Password entry */
  const char	*subpkg;		/* Subpackage */


 /*
  * Create a new, blank distribution...
  */

  dist = new_dist();

 /*
  * Open the main list file...
  */

  if ((listfiles[0] = fopen(filename, "r")) == NULL)
  {
    fprintf(stderr, "epm: Unable to open list file \"%s\" -\n     %s\n",
            filename, strerror(errno));
    return (NULL);
  }

 /*
  * Find any product descriptions, etc. in the list file...
  */

  if (Verbosity)
    puts("Searching for product information...");

  skip      = 0;
  listlevel = 0;
  subpkg    = NULL;

  do
  {
    while (get_line(buf, sizeof(buf), listfiles[listlevel], platform, format,
                    &skip) != NULL)
    {
     /*
      * Do variable substitution...
      */

      line[0] = buf[0]; /* Don't expand initial $ */
      expand_name(line + 1, buf + 1, sizeof(line) - 1,
                  strncmp(buf, "%if", 3) || strncmp(buf, "%elseif", 7));

     /*
      * Check line for config stuff...
      */

      if (line[0] == '%')
      {
       /*
        * Find whitespace...
	*/

        for (temp = line; !isspace(*temp & 255) && *temp; temp ++);
	for (; isspace(*temp & 255); *temp++ = '\0');

       /*
        * Process directive...
        */

	if (!strcmp(line, "%include"))
	{
	  listlevel ++;

	  if ((listfiles[listlevel] = fopen(temp, "r")) == NULL)
	  {
	    fprintf(stderr, "epm: Unable to include \"%s\" -\n     %s\n", temp,
	            strerror(errno));
	    listlevel --;
	  }
	}
	else if (!strcmp(line, "%description"))
	  add_description(dist, listfiles[listlevel], temp, subpkg);
	else if (!strcmp(line, "%preinstall"))
          add_command(dist, listfiles[listlevel], COMMAND_PRE_INSTALL, temp,
	              subpkg, NULL);
	else if (!strcmp(line, "%install") || !strcmp(line, "%postinstall"))
          add_command(dist, listfiles[listlevel], COMMAND_POST_INSTALL, temp,
	              subpkg, NULL);
	else if (!strcmp(line, "%remove") || !strcmp(line, "%preremove"))
          add_command(dist, listfiles[listlevel], COMMAND_PRE_REMOVE, temp,
	              subpkg, NULL);
	else if (!strcmp(line, "%postremove"))
          add_command(dist, listfiles[listlevel], COMMAND_POST_REMOVE, temp,
	              subpkg, NULL);
	else if (!strcmp(line, "%prepatch"))
          add_command(dist, listfiles[listlevel], COMMAND_PRE_PATCH, temp,
	              subpkg, NULL);
	else if (!strcmp(line, "%patch") || !strcmp(line, "%postpatch"))
          add_command(dist, listfiles[listlevel], COMMAND_POST_PATCH, temp,
	              subpkg, NULL);
	else if (!strncmp(line, "%literal(", 9))
        {
          char	*ptr,			/* Pointer to parenthesis */
		*section;		/* Section for literal text */


	  section = line + 9;
	  if ((ptr = strchr(section, ')')) != NULL)
	  {
	    *ptr = '\0';

	    add_command(dist, listfiles[listlevel], COMMAND_LITERAL, temp,
			subpkg, section);
	  }
	  else
	    fputs("epm: Ignoring bad %literal(section) line in list file.\n",
		  stderr);
        }
        else if (!strcmp(line, "%product"))
	{
          if (!dist->product[0])
            strcpy(dist->product, temp);
	  else
	    fputs("epm: Ignoring %product line in list file.\n", stderr);
	}
	else if (!strcmp(line, "%copyright"))
	{
          if (!dist->copyright[0])
            strcpy(dist->copyright, temp);
	  else
	    fputs("epm: Ignoring %copyright line in list file.\n", stderr);
	}
	else if (!strcmp(line, "%vendor"))
	{
          if (!dist->vendor[0])
            strcpy(dist->vendor, temp);
	  else
	    fputs("epm: Ignoring %vendor line in list file.\n", stderr);
	}
	else if (!strcmp(line, "%packager"))
	{
          if (!dist->packager[0])
            strcpy(dist->packager, temp);
	  else
	    fputs("epm: Ignoring %packager line in list file.\n", stderr);
	}
	else if (!strcmp(line, "%license"))
	{
          if (!dist->license[0])
            strcpy(dist->license, temp);
	  else
	    fputs("epm: Ignoring %license line in list file.\n", stderr);
	}
	else if (!strcmp(line, "%readme"))
	{
          if (!dist->readme[0])
            strcpy(dist->readme, temp);
	  else
	    fputs("epm: Ignoring %readme line in list file.\n", stderr);
	}
	else if (!strcmp(line, "%subpackage"))
	{
	  subpkg = find_subpackage(dist, temp);
	}
	else if (!strcmp(line, "%version"))
	{
          if (!dist->version[0])
	  {
	    if (strchr(temp, ':'))
	    {
	     /*
	      * Grab the epoch...
	      */

	      dist->epoch = strtol(temp, &temp, 10);

	      if (*temp == ':')
	        temp ++;
	    }

            strlcpy(dist->version, temp, sizeof(dist->version));
	    if ((temp = strchr(dist->version, ' ')) != NULL)
	    {
	      *temp++ = '\0';
	      dist->vernumber = atoi(temp);
	    }
	    else
	      dist->vernumber = get_vernumber(dist->version);

            if ((temp = strrchr(dist->version, '-')) != NULL)
	    {
	      *temp++ = '\0';
	      strlcpy(dist->release, temp, sizeof(dist->release));
	    }
	  }
	}
	else if (!strcmp(line, "%release"))
	{
	  strlcpy(dist->release, temp, sizeof(dist->release));
	  dist->vernumber += atoi(temp);
	}
	else if (!strcmp(line, "%incompat"))
	  add_depend(dist, DEPEND_INCOMPAT, temp, subpkg);
	else if (!strcmp(line, "%provides"))
	  add_depend(dist, DEPEND_PROVIDES, temp, subpkg);
	else if (!strcmp(line, "%replaces"))
	  add_depend(dist, DEPEND_REPLACES, temp, subpkg);
	else if (!strcmp(line, "%requires"))
	  add_depend(dist, DEPEND_REQUIRES, temp, subpkg);
	else
	{
	  fprintf(stderr, "epm: Unknown directive \"%s\" ignored!\n", line);
	  fprintf(stderr, "     %s %s\n", line, temp);
	}
      }
      else if (line[0] == '$')
      {
       /*
        * Define a variable...
	*/

        if (line[1] == '{' && (temp = strchr(line + 2, '}')) != NULL)
	{
	 /*
	  * Remove {} from name...
	  */

	  strcpy(temp, temp + 1);
          strcpy(line + 1, line + 2);
        }
	else if (line[1] == '(' && (temp = strchr(line + 2, ')')) != NULL)
	{
	 /*
	  * Remove () from name...
	  */

	  strcpy(temp, temp + 1);
          strcpy(line + 1, line + 2);
        }

        if ((temp = strchr(line + 1, '=')) != NULL)
	{
	 /*
	  * Only define the variable if it is not in the environment
	  * or on the command-line.
	  */

	  *temp = '\0';

	  if (getenv(line + 1) == NULL)
	  {
	    *temp = '=';
            if ((temp = strdup(line + 1)) != NULL)
	      putenv(temp);
	  }
	}
      }
      else
      {
        type = line[0];
	if (!isspace(line[1] & 255))
	{
	  fprintf(stderr, "epm: Expected whitespace after file type - %s\n",
	          line);
	  continue;
	}

        temp = line + 2;
	mode = strtol(temp, &temp, 8);
	if (temp == (line + 2))
	{
	  fprintf(stderr, "epm: Expected file permissions after file type - %s\n",
	          line);
	  continue;
	}

        if (get_string(&temp, user, sizeof(user)) == NULL)
	{
	  fprintf(stderr, "epm: Expected user after file permissions - %s\n",
	          line);
	  continue;
	}

        if (get_string(&temp, group, sizeof(group)) == NULL)
	{
	  fprintf(stderr, "epm: Expected group after user - %s\n",
	          line);
	  continue;
	}

        if (get_string(&temp, dst, sizeof(dst)) == NULL)
	{
	  fprintf(stderr, "epm: Expected destination after group - %s\n",
	          line);
	  continue;
	}

        get_string(&temp, src, sizeof(src));

        get_string(&temp, options, sizeof(options));

	if (tolower(type) == 'd' || type == 'R')
	{
	  strcpy(options, src);
	  src[0] = '\0';
	}

#ifdef __osf__ /* Remap group "sys" to "system" */
        if (!strcmp(group, "sys"))
	  strcpy(group, "system");
#elif defined(__linux) /* Remap group "sys" to "root" */
        if (!strcmp(group, "sys"))
	  strcpy(group, "root");
#endif /* __osf__ */

        if ((temp = strrchr(src, '/')) == NULL)
	  temp = src;
	else
	  temp ++;

        for (; *temp; temp ++)
	  if (strchr("*?[", *temp))
	    break;

        if (*temp)
	{
	 /*
	  * Add using wildcards...
	  */

          if ((temp = strrchr(src, '/')) == NULL)
	    temp = src;
	  else
	    *temp++ = '\0';

	  strlcpy(pattern, temp, sizeof(pattern));

          if (dst[strlen(dst) - 1] != '/')
	    strncat(dst, "/", sizeof(dst) - 1);

          if (temp == src)
	    dir = opendir(".");
	  else
	    dir = opendir(src);

          if (dir == NULL)
	    fprintf(stderr, "epm: Unable to open directory \"%s\" - %s\n",
	            src, strerror(errno));
          else
	  {
	   /*
	    * Make sure we have a directory separator...
	    */

	    if (temp > src)
	      temp[-1] = '/';

	    while ((dent = readdir(dir)) != NULL)
	    {
	      strcpy(temp, dent->d_name);
	      if (stat(src, &fileinfo))
	        continue; /* Skip files we can't read */

              if (S_ISDIR(fileinfo.st_mode))
	        continue; /* Skip directories */

              if (!patmatch(dent->d_name, pattern))
	        continue;

              file = add_file(dist, subpkg);

              file->type = type;
	      file->mode = mode;
              strcpy(file->src, src);
	      strcpy(file->dst, dst);
	      strncat(file->dst, dent->d_name, sizeof(file->dst) - 1);
	      strcpy(file->user, user);
	      strcpy(file->group, group);
	      strcpy(file->options, options);
	    }

            closedir(dir);
	  }
	}
	else
	{
         /*
	  * Add single file...
	  */

          file = add_file(dist, subpkg);

          file->type = type;
	  file->mode = mode;
          strcpy(file->src, src);
	  strcpy(file->dst, dst);
	  strcpy(file->user, user);
	  strcpy(file->group, group);
	  strcpy(file->options, options);
	}
      }
    }

    fclose(listfiles[listlevel]);
    listlevel --;
  }
  while (listlevel >= 0);

  if (!dist->packager[0])
  {
   /*
    * Assign a default packager name...
    */

    gethostname(buf, sizeof(buf));

    setpwent();
    if ((pwd = getpwuid(getuid())) != NULL)
      snprintf(dist->packager, sizeof(dist->packager), "%s@%s", pwd->pw_name,
               buf);
    else
      snprintf(dist->packager, sizeof(dist->packager), "unknown@%s", buf);
  }

  sort_dist_files(dist);

  return (dist);
}


/*
 * 'sort_dist_files()' - Sort the files in the distribution.
 */

void
sort_dist_files(dist_t *dist)		/* I - Distribution to sort */
{
  int		i;			/* Looping var */
  file_t	*file;			/* File in distribution */


 /*
  * Sort the files...
  */

  if (dist->num_files > 1)
    qsort(dist->files, (size_t)dist->num_files, sizeof(file_t),
          (int (*)(const void *, const void *))compare_files);

 /*
  * Remove duplicates...
  */

  for (i = dist->num_files - 1, file = dist->files; i > 0; i --, file ++)
    if (!strcmp(file[0].dst, file[1].dst))
    {
      if (file[0].type == file[1].type && file[0].mode == file[1].mode &&
          !strcmp(file[0].src, file[1].src) &&
          !strcmp(file[0].user, file[1].user) &&
	  !strcmp(file[0].group, file[1].group) &&
	  !strcmp(file[0].options, file[1].options))
      {
       /*
        * Ignore exact duplicates...
	*/

	memcpy(file, file + 1, i * sizeof(file_t));
	dist->num_files --;
	file --;
      }
      else
        fprintf(stderr, "epm: Duplicate destination path \"%s\" with different info!\n"
	                "     \"%c %04o %s %s\" from source \"%s\"\n"
			"     \"%c %04o %s %s\" from source \"%s\"\n",
	        file[0].dst,
		file[0].type, file[0].mode, file[0].user, file[0].group, file[0].src,
		file[1].type, file[1].mode, file[1].user, file[1].group, file[1].src);
    }
}


/*
 * 'write_dist()' - Write a distribution list file...
 */

int					/* O - 0 on success, -1 on failure */
write_dist(const char *listname,	/* I - File to write to */
           dist_t     *dist)		/* I - Distribution to write */
{
  int		i;			/* Looping var */
  int		is_inline;		/* Inline text? */
  char		listbck[1024],		/* Backup filename */
		*ptr;			/* Pointer into command string */
  FILE		*listfile;		/* Output file */
  file_t	*file;			/* Current file entry */
  time_t	curtime;		/* Current time */
  struct tm	*curdate;		/* Current date */
  char		curstring[256];		/* Current date/time string */
  const char	*subpkg;		/* Current subpackage */
  static const char *commands[] =	/* Command strings */
		{
		  "%preinstall",
		  "%postinstall",
		  "%prepatch",
		  "%postpatch",
		  "%preremove",
		  "%postremove"
		},
		*depends[] =		/* Dependency strings */
		{
		  "%requires",
		  "%incompat",
		  "%replaces",
		  "%provides"
		};


 /*
  * Make a backup of the list file...
  */

  snprintf(listbck, sizeof(listbck), "%s.O", listname);

  rename(listname, listbck);

 /*
  * Open the list file...
  */

  if ((listfile = fopen(listname, "w")) == NULL)
  {
    rename(listbck, listname);
    return (-1);
  }

 /*
  * Write the list file...
  */

  curtime = time(NULL);
  curdate = localtime(&curtime);

  strftime(curstring, sizeof(curstring), "# List file created on %c by "
           EPM_VERSION "\n", curdate);
  fputs(curstring, listfile);

  if (dist->product[0])
    fprintf(listfile, "%%product %s\n", dist->product);
  if (dist->version[0])
    fprintf(listfile, "%%version %s %d\n", dist->version, dist->vernumber);
  if (dist->release[0])
    fprintf(listfile, "%%release %s\n", dist->release);
  if (dist->copyright[0])
    fprintf(listfile, "%%copyright %s\n", dist->copyright);
  if (dist->vendor[0])
    fprintf(listfile, "%%vendor %s\n", dist->vendor);
  if (dist->packager[0])
    fprintf(listfile, "%%packager %s\n", dist->packager);
  if (dist->license[0])
    fprintf(listfile, "%%license %s\n", dist->license);
  if (dist->readme[0])
    fprintf(listfile, "%%readme %s\n", dist->readme);

  subpkg = NULL;
  for (i = 0; i < dist->num_descriptions; i ++)
  {
    if (dist->descriptions[i].subpackage != subpkg)
    {
      subpkg = dist->descriptions[i].subpackage;
      fprintf(listfile, "%%subpackage %s\n", subpkg ? subpkg : "");
    }

    if (strchr(dist->descriptions[i].description, '\n') != NULL)
      fprintf(listfile, "%%description <<EPM-END-INLINE\n%s\nEPM-END-INLINE\n",
              dist->descriptions[i].description);
    else
      fprintf(listfile, "%%description %s\n",
              dist->descriptions[i].description);
  }

  for (i = 0; i < dist->num_depends; i ++)
  {
    if (dist->depends[i].subpackage != subpkg)
    {
      subpkg = dist->depends[i].subpackage;
      fprintf(listfile, "%%subpackage %s\n", subpkg ? subpkg : "");
    }

    fprintf(listfile, "%s %s", depends[(int)dist->depends[i].type],
            dist->depends[i].product);

    if (dist->depends[i].version[0][0] ||
        dist->depends[i].version[1][0])
    {
      fprintf(listfile, " %s %d %s %d\n",
              dist->depends[i].version[0],
	      dist->depends[i].vernumber[0],
              dist->depends[i].version[1],
	      dist->depends[i].vernumber[1]);
    }
    else
      putc('\n', listfile);
  }

  for (i = 0; i < dist->num_commands; i ++)
  {
    if (dist->commands[i].subpackage != subpkg)
    {
      subpkg = dist->commands[i].subpackage;
      fprintf(listfile, "%%subpackage %s\n", subpkg ? subpkg : "");
    }

    fputs(commands[(int)dist->commands[i].type], listfile);

    is_inline = strchr(dist->commands[i].command, '\n') != NULL;

    if (is_inline)
      fputs(" <<EPM-END-INLINE\n", listfile);
    else
      putc(' ', listfile);

    for (ptr = dist->commands[i].command; *ptr; ptr ++)
    {
      if (*ptr == '$' && !is_inline)
        putc(*ptr, listfile);

      putc(*ptr, listfile);
    }
    putc('\n', listfile);

    if (is_inline)
      fputs("EPM-END-INLINE\n", listfile);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
  {
    if (file->subpackage != subpkg)
    {
      subpkg = file->subpackage;
      fprintf(listfile, "%%subpackage %s\n", subpkg ? subpkg : "");
    }

    fprintf(listfile, "%c %04o %s %s %s %s",
	    file->type, file->mode, file->user, file->group,
	    file->dst, file->src);

    if (file->options[0])
      fprintf(listfile, "%s\n", file->options);
    else
      putc('\n', listfile);
  }

  return (fclose(listfile));
}


/*
 * 'compare_files()' - Compare the destination filenames.
 */

static int				/* O - Result of comparison */
compare_files(const file_t *f0,		/* I - First file */
              const file_t *f1)		/* I - Second file */
{
  return (strcmp(f0->dst, f1->dst));
}


/*
 * 'expand_name()' - Expand a filename with environment variables.
 */

static void
expand_name(char   *buffer,		/* O - Output string */
            char   *name,		/* I - Input string */
	    size_t bufsize,		/* I - Size of output string */
	    int    warn)		/* I - Warn when not set? */
{
  char	var[255],			/* Environment variable name */
	*varptr,			/* Current position in name */
	delim;				/* Delimiter character */


  bufsize --;
  while (*name != '\0' && bufsize > 0)
  {
    if (*name == '$')
    {
      name ++;
      if (*name == '$')
      {
       /*
        * Insert a lone $...
	*/

	*buffer++ = *name++;
	bufsize --;
	continue;
      }
      else if (*name == '{' || *name == '(')
      {
       /*
        * Bracketed variable name...
	*/

        delim = *name == '{' ? '}' : ')';

	for (varptr = var, name ++; *name != delim && *name; name ++)
          if (varptr < (var + sizeof(var) - 1))
	    *varptr++ = *name;

        if (*name == delim)
	  name ++;
      }
      else
      {
       /*
        * Unbracketed variable name...
	*/

	for (varptr = var; !strchr("/ \t\r\n-", *name) && *name; name ++)
          if (varptr < (var + sizeof(var) - 1))
	    *varptr++ = *name;
      }

      *varptr = '\0';

      if ((varptr = getenv(var)) != NULL)
      {
	strlcpy(buffer, varptr, bufsize + 1);
        bufsize -= strlen(buffer);
	buffer  += strlen(buffer);
      }
      else if (warn)
        fprintf(stderr, "epm: Variable \"%s\" undefined!\n", var);
    }
    else
    {
      *buffer++ = *name++;
      bufsize --;
    }
  }

  *buffer = '\0';
}


/*
 * 'get_file()' - Read a file into a string...
 */

static char *				/* O  - Pointer to string or NULL on EOF */
get_file(const char *filename,		/* I  - File to read from */
         char       *buffer,		/* IO - String buffer */
	 size_t     size)		/* I  - Size of string buffer */
{
  FILE		*fp;			/* File buffer */
  struct stat	info;			/* File information */
  char		*expand;		/* Expansion buffer */


  if (stat(filename, &info))
  {
    fprintf(stderr, "epm: Unable to stat \"%s\" - %s\n", filename,
            strerror(errno));
    return (NULL);
  }

  if (info.st_size > (size - 1))
  {
    fprintf(stderr,
            "epm: File \"%s\" is too large (%d bytes) for buffer (%d bytes)\n",
            filename, (int)info.st_size, (int)size - 1);
    return (NULL);
  }

  if ((fp = fopen(filename, "r")) == NULL)
  {
    fprintf(stderr, "epm: Unable to open \"%s\" - %s\n", filename,
            strerror(errno));
    return (NULL);
  }

  if ((fread(buffer, 1, (size_t)info.st_size, fp)) < info.st_size)
  {
    fprintf(stderr, "epm: Unable to read \"%s\" - %s\n", filename,
            strerror(errno));
    fclose(fp);
    return (NULL);
  }

  fclose(fp);

  if (buffer[info.st_size - 1] == '\n')
    buffer[info.st_size - 1] = '\0';
  else
    buffer[info.st_size] = '\0';

  if (strchr(buffer, '$') != NULL)
  {
   /*
    * Do variable expansion before returning...
    */

    expand = strdup(buffer);

    expand_name(buffer, expand, size, 1);

    free(expand);
  }

  return (buffer);
}


/*
 * 'get_inline()' - Read inline lines into a string...
 */

static char *				/* O  - Pointer to string or NULL on EOF */
get_inline(const char *term,		/* I  - Termination string */
           FILE       *fp,		/* I  - File to read from */
           char       *buffer,		/* IO - String buffer */
	   size_t     size)		/* I  - Size of string buffer */
{
  char	*bufptr;			/* Pointer into buffer */
  int	left;				/* Remaining bytes in buffer */
  int	termlen;			/* Length of termination string */
  int	linelen;			/* Length of line */
  char	*expand;			/* Expansion buffer */


  bufptr  = buffer;
  left    = size;
  termlen = strlen(term);

  if (termlen == 0)
    return (NULL);

  while (fgets(bufptr, left, fp) != NULL)
  {
    if (!strncmp(bufptr, term, (size_t)termlen) && bufptr[termlen] == '\n')
    {
      *bufptr = '\0';
      break;
    }

    linelen = strlen(bufptr);
    left    -= linelen;
    bufptr  += linelen;

    if (left < 2)
    {
      fputs("epm: Inline script too long!\n", stderr);
      break;
    }
  }

  if (bufptr > buffer)
  {
    bufptr --;
    if (*bufptr == '\n')
      *bufptr = '\0';

    if (strchr(buffer, '$') != NULL)
    {
     /*
      * Do variable expansion before returning...
      */

      expand = strdup(buffer);

      expand_name(buffer, expand, size, 1);

      free(expand);
    }

    return (buffer);
  }
  else
    return (NULL);
}


/*
 * 'get_line()' - Get a line from a file, filtering for uname lines...
 */

static char *				/* O - String read or NULL at EOF */
get_line(char           *buffer,	/* I - Buffer to read into */
         int            size,		/* I - Size of buffer */
	 FILE           *fp,		/* I - File to read from */
	 struct utsname *platform,	/* I - Platform information */
         const char     *format,	/* I - Distribution format */
	 int            *skip)		/* IO - Skip lines? */
{
  int		op,			/* Operation (0 = OR, 1 = AND) */
		namelen,		/* Length of system name + version */
		len,			/* Length of string */
		match;			/* 1 = match, 0 = not */
  char		*ptr,			/* Pointer into value */
		*bufptr,		/* Pointer into buffer */
		namever[255],		/* Name + version */
		value[255];		/* Value string */
  const char	*var;			/* Variable value */


  while (fgets(buffer, size, fp) != NULL)
  {
   /*
    * Skip comment and blank lines...
    */

    if (buffer[0] == '#' || buffer[0] == '\n')
      continue;

   /*
    * See if this is a %system, %format, or conditional line...
    */

    if (!strncmp(buffer, "%system ", 8))
    {
     /*
      * Yes, do filtering based on the OS (+version)...
      */

      *skip &= ~SKIP_SYSTEM;

      if (strcmp(buffer + 8, "all\n"))
      {
	namelen = strlen(platform->sysname);
        bufptr  = buffer + 8;
	snprintf(namever, sizeof(namever), "%s-%s", platform->sysname,
	         platform->release);

	while (isspace(*bufptr & 255))
	  bufptr ++;

        if (*bufptr != '!')
	  *skip |= SKIP_SYSTEM;

        while (*bufptr)
	{
	 /*
          * Skip leading whitespace...
	  */

	  while (isspace(*bufptr & 255))
	    bufptr ++;

          if (!*bufptr)
	    break;

          if (*bufptr == '!')
	  {
	    op = 1;
	    bufptr ++;
	  }
	  else
	    op = 0;

	  for (ptr = value;
	       *bufptr && !isspace(*bufptr & 255) &&
	           ptr < (value + sizeof(value) - 1);
	       *ptr++ = *bufptr++);

	  *ptr = '\0';

          if (!strncmp(value, "dunix", 5))
	    memcpy(value, "tru64", 5); /* Keep existing nul/version */
          else if (!strncmp(value, "darwin", 6) &&
	           !strcmp(platform->sysname, "macosx"))
	    memcpy(value, "macosx", 6); /* Keep existing nul/version */

          if ((ptr = strchr(value, '-')) != NULL)
	    len = ptr - value;
	  else
	    len = strlen(value);

          if (len < namelen)
	    match = 0;
	  else
	    match = !strncasecmp(value, namever, strlen(value)) ?
	                SKIP_SYSTEM : 0;

          if (op)
	    *skip |= match;
	  else
	    *skip &= ~match;
        }
      }
    }
    else if (!strncmp(buffer, "%format ", 8))
    {
     /*
      * Yes, do filtering based on the distribution format...
      */

      *skip &= ~SKIP_FORMAT;

      if (strcmp(buffer + 8, "all\n"))
      {
        bufptr = buffer + 8;

	while (isspace(*bufptr & 255))
	  bufptr ++;

        if (*bufptr != '!')
	  *skip  |= SKIP_FORMAT;

        while (*bufptr)
	{
	 /*
          * Skip leading whitespace...
	  */

	  while (isspace(*bufptr & 255))
	    bufptr ++;

          if (!*bufptr)
	    break;

          if (*bufptr == '!')
	  {
	    op = 1;
	    bufptr ++;
	  }
	  else
	    op = 0;

	  for (ptr = value;
	       *bufptr && !isspace(*bufptr & 255) &&
	           ptr < (value + sizeof(value) - 1);
	       *ptr++ = *bufptr++);

	  *ptr = '\0';

	  match = !strcasecmp(value, format) ? SKIP_FORMAT : 0;

          if (op)
	    *skip |= match;
	  else
	    *skip &= ~match;
        }
      }
    }
    else if (!strncmp(buffer, "%arch ", 6))
    {
     /*
      * Yes, do filtering based on the current architecture...
      */

      *skip &= ~SKIP_ARCH;

      if (strcmp(buffer + 6, "all\n"))
      {
        bufptr = buffer + 8;

	while (isspace(*bufptr & 255))
	  bufptr ++;

        if (*bufptr != '!')
	  *skip |= SKIP_ARCH;

        while (*bufptr)
	{
	 /*
          * Skip leading whitespace...
	  */

	  while (isspace(*bufptr & 255))
	    bufptr ++;

          if (!*bufptr)
	    break;

          if (*bufptr == '!')
	  {
	    op = 1;
	    bufptr ++;
	  }
	  else
	    op = 0;

	  for (ptr = value;
	       *bufptr && !isspace(*bufptr & 255) &&
	           ptr < (value + sizeof(value) - 1);
	       *ptr++ = *bufptr++);

	  *ptr = '\0';

          update_architecture(value, sizeof(value));

	  match = !strcasecmp(value, platform->machine) ? SKIP_ARCH : 0;

          if (op)
	    *skip |= match;
	  else
	    *skip &= ~match;
        }
      }
    }
    else if (!strncmp(buffer, "%ifdef ", 7) ||
             !strncmp(buffer, "%elseifdef ", 11))
    {
     /*
      * Yes, do filtering based on the presence of variables...
      */

      if ((*skip & SKIP_IFACTIVE) && buffer[1] != 'e')
      {
       /*
        * Nested %if...
	*/

        fputs("epm: Warning, nested %ifdef's are not supported!\n", stderr);
	continue;
      }

      if (buffer[1] == 'e')
	bufptr = buffer + 11;
      else
	bufptr = buffer + 7;

      *skip |= SKIP_IF | SKIP_IFACTIVE;

      if (*skip & SKIP_IFSAT)
        continue;

      while (isspace(*bufptr & 255))
	bufptr ++;

      if (*bufptr == '!')
        *skip &= ~SKIP_IF;

      while (*bufptr)
      {
       /*
        * Skip leading whitespace...
	*/

	while (isspace(*bufptr & 255))
	  bufptr ++;

        if (!*bufptr)
	  break;

       /*
        * Get the variable name...
	*/

        if (*bufptr == '!')
	{
	  op = 1;
	  bufptr ++;
	}
	else
	  op = 0;

	for (ptr = value;
	     *bufptr && !isspace(*bufptr & 255) &&
	         ptr < (value + sizeof(value) - 1);
	     *ptr++ = *bufptr++);

	*ptr = '\0';

	match = (getenv(value) != NULL) ? SKIP_IF : 0;

        if (op)
	  *skip |= match;
	else
	  *skip &= ~match;

        if (match)
	  *skip |= SKIP_IFSAT;
      }
    }
    else if (!strncmp(buffer, "%if ", 4) || !strncmp(buffer, "%elseif ", 8))
    {
     /*
      * Yes, do filtering based on the value of variables...
      */

      if ((*skip & SKIP_IFACTIVE) && buffer[1] != 'e')
      {
       /*
        * Nested %if...
	*/

        fputs("epm: Warning, nested %if's are not supported!\n", stderr);
	continue;
      }

      if (buffer[1] == 'e')
        bufptr = buffer + 8;
      else
	bufptr = buffer + 4;

      *skip |= SKIP_IF | SKIP_IFACTIVE;

      if (*skip & SKIP_IFSAT)
        continue;

      while (isspace(*bufptr & 255))
	bufptr ++;

      if (*bufptr == '!')
        *skip &= ~SKIP_IF;

      while (*bufptr)
      {
       /*
        * Skip leading whitespace...
	*/

	while (isspace(*bufptr & 255))
	  bufptr ++;

        if (!*bufptr)
	  break;

       /*
        * Get the variable name...
	*/

        if (*bufptr == '!')
	{
	  op = 1;
	  bufptr ++;
	}
	else
	  op = 0;

	for (ptr = value;
	     *bufptr && !isspace(*bufptr & 255) &&
	         ptr < (value + sizeof(value) - 1);
	     *ptr++ = *bufptr++);

	*ptr = '\0';

        match = ((var = getenv(value)) != NULL && *var) ? SKIP_IF : 0;

        if (op)
	  *skip |= match;
	else
	  *skip &= ~match;

        if (match)
	  *skip |= SKIP_IFSAT;
      }
    }
    else if (!strcmp(buffer, "%else\n"))
    {
     /*
      * Handle "else" condition of %ifdef statement...
      */

      if (!(*skip & SKIP_IFACTIVE))
      {
       /*
        * No matching %if/%ifdef statement...
	*/

        fputs("epm: Warning, no matching %if or %ifdef for %else!\n", stderr);
	break;
      }

      if (*skip & SKIP_IFSAT)
        *skip |= SKIP_IF;
      else
      {
	*skip &= ~SKIP_IF;
	*skip |= SKIP_IFSAT;
      }
    }
    else if (!strcmp(buffer, "%endif\n"))
    {
     /*
      * Cancel any filtering based on environment variables.
      */

      if (!(*skip & SKIP_IFACTIVE))
      {
       /*
        * No matching %if/%ifdef statement...
	*/

        fputs("epm: Warning, no matching %if or %ifdef for %endif!\n", stderr);
	break;
      }

      *skip &= ~(SKIP_IF | SKIP_IFACTIVE | SKIP_IFSAT);
    }
    else if (!(*skip & SKIP_MASK))
    {
     /*
      * Otherwise strip any trailing newlines and return the string!
      */

      if (buffer[strlen(buffer) - 1] == '\n')
        buffer[strlen(buffer) - 1] = '\0';

      return (buffer);
    }
  }

  return (NULL);
}


/*
 * 'get_string()' - Get a delimited string from a line.
 */

static char *				/* O  - String or NULL */
get_string(char   **src,		/* IO - Source string */
           char   *dst,			/* O  - Destination string */
	   size_t dstsize)		/* I  - Size of destination string */
{
  char	*srcptr,			/* Current source pointer */
	*dstptr,			/* Current destination pointer */
	*dstend,			/* End of destination string */
	quote;				/* Quoting char */


 /*
  * Initialize things...
  */

  srcptr = *src;
  dstptr = dst;
  dstend = dst + dstsize - 1;

  *dstptr = '\0';

 /*
  * Skip leading whitespace...
  */

  while (isspace(*srcptr & 255))
    srcptr ++;

  if (!*srcptr)
  {
    *src = srcptr;

    return (NULL);
  }

 /*
  * Grab the next string...
  */

  while (*srcptr && !isspace(*srcptr & 255))
  {
    if (*srcptr == '\\')
    {
      srcptr ++;

      if (!*srcptr)
      {
        fputs("epm: Expected character after backslash!\n", stderr);

        *src = srcptr;
        *dst = '\0';

	return (NULL);
      }
    }
    else if (*srcptr == '\'' || *srcptr == '\"')
    {
     /*
      * Read a quoted string...
      */

      quote = *srcptr++;

      while (*srcptr != quote && *srcptr)
      {
        if (*srcptr == '\\')
	{
	  srcptr ++;

	  if (!*srcptr)
	  {
            fputs("epm: Expected character after backslash!\n", stderr);

            *src = srcptr;
            *dst = '\0';

	    return (NULL);
	  }
	}

	if (dstptr < dstend)
	  *dstptr++ = *srcptr;

	srcptr ++;
      }

      if (!*srcptr)
      {
        fprintf(stderr, "epm: Expected end quote %c!\n", quote);

        *src = srcptr;
	*dst = '\0';

	return (NULL);
      }

      srcptr ++;
      continue;
    }

    if (dstptr < dstend)
      *dstptr++ = *srcptr;

    srcptr ++;
  }

  *dstptr = '\0';

 /*
  * Skip leading whitespace...
  */

  while (isspace(*srcptr & 255))
    srcptr ++;

 /*
  * Return the string and string pointer...
  */

  *src = srcptr;

  return (dst);
}


/*
 * 'patmatch()' - Pattern matching...
 */

static int				/* O - 1 if match, 0 if no match */
patmatch(const char *s,			/* I - String to match against */
         const char *pat)		/* I - Pattern to match against */
{
 /*
  * Range check the input...
  */

  if (s == NULL || pat == NULL)
    return (0);

 /*
  * Loop through the pattern and match strings, and stop if we come to a
  * point where the strings don't match or we find a complete match.
  */

  while (*s != '\0' && *pat != '\0')
  {
    if (*pat == '*')
    {
     /*
      * Wildcard - 0 or more characters...
      */

      pat ++;
      if (*pat == '\0')
        return (1);	/* Last pattern char is *, so everything matches now... */

     /*
      * Test all remaining combinations until we get to the end of the string.
      */

      while (*s != '\0')
      {
        if (patmatch(s, pat))
	  return (1);

	s ++;
      }
    }
    else if (*pat == '?')
    {
     /*
      * Wildcard - 1 character...
      */

      pat ++;
      s ++;
      continue;
    }
    else if (*pat == '[')
    {
     /*
      * Match a character from the input set [chars]...
      */

      pat ++;
      while (*pat != ']' && *pat != '\0')
        if (*s == *pat)
	  break;
	else if (pat[1] == '-' && *s >= pat[0] && *s <= pat[2])
	  break;
	else
	  pat ++;

      if (*pat == ']' || *pat == '\0')
        return (0);

      while (*pat != ']' && *pat != '\0')
        pat ++;

      if (*pat == ']')
        pat ++;

      s ++;

      continue;
    }
    else if (*pat == '\\')
    {
     /*
      * Handle quoted characters...
      */

      pat ++;
    }

   /*
    * Stop if the pattern and string don't match...
    */

    if (*pat++ != *s++)
      return (0);
  }

 /*
  * If we are at the end of the string, see if the pattern remaining is
  * "*"...
  */

  while (*pat == '*')
    pat ++;
    
 /*
  * Done parsing the pattern and string; return 1 if the last character matches
  * and 0 otherwise...
  */

  return (*s == *pat);
}


/*
 * 'sort_subpackages()' - Compare two subpackage names.
 */

static int				/* O - Result of comparison */
sort_subpackages(char **a,		/* I - First subpackage */
                 char **b)		/* I - Second subpackage */
{
  return (strcmp(*a, *b));
}


/*
 * 'update_architecture()' - Normalize the machine architecture name.
 */

static void
update_architecture(char   *buffer,	/* I - String buffer */
                    size_t bufsize)	/* I - Size of string buffer */
{
  char	*temp;				/* Pointer into string buffer */


 /*
  * Convert the name to lowercase with no underscores or dashes.
  */

  for (temp = buffer; *temp != '\0'; temp ++)
    if (*temp == '-' || *temp == '_')
    {
      char *ptr;			/* Pointer into string */

      for (ptr = temp; ptr[1]; ptr ++)
        *ptr = ptr[1];
      *ptr = '\0';

      temp --;
    }
    else
      *temp = tolower(*temp);

 /*
  * Convert common synonyms to generic names...
  */

  if (strstr(buffer, "86"))
  {
    if (strstr(buffer, "64"))
      strlcpy(buffer, "x86_64", bufsize);
    else
      strlcpy(buffer, "intel", bufsize);
  }
  else if (!strncmp(buffer, "arm", 3))
    strlcpy(buffer, "arm", bufsize);
  else if (!strncmp(buffer, "ppc", 3))
    strlcpy(buffer, "powerpc", bufsize);
  else if (!strncmp(buffer, "sun", 3))
    strlcpy(buffer, "sparc", bufsize);
}


/*
 * End of "$Id: dist.c 843 2010-12-31 02:18:18Z mike $".
 */

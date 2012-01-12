/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "rods.h"
#include "parseCommandLine.h"
#include "rcMisc.h"

void usage (char *prog);

int
main(int argc, char **argv)
{
    int status;
    rodsArguments_t myRodsArgs;
    rodsEnv myEnv;

    status = parseCmdLineOpt(argc, argv, "vVh", 0, &myRodsArgs);
    if (status) {
       printf("Use -h for help\n");
       exit(1);
    }

    if (myRodsArgs.help==True) {
       usage(argv[0]);
       exit(0);
    }

    status = getRodsEnv (&myEnv);
    if (status != 0) {
      printf("Failed with error %d\n", status);
      exit(2);
    }

    printf ("%s\n", myEnv.rodsCwd);

    exit (0);
}


void usage (char *prog)
{
  printf("Shows your iRODS Current Working Directory.\n");
  printf("Usage: %s [-vVh]\n", prog);
  printf(" -v  verbose\n");
  printf(" -V  very verbose\n");
  printf(" -h  this help\n");
  printReleaseInfo("ipwd");
}

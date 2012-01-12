/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "rods.h"
#include "parseCommandLine.h"
#include "rcMisc.h"

void usage (char *prog);

int
main(int argc, char **argv)
{
    char *myErrName;
    int errorCode;
    char *mySubErrName;

    if (argc != 2) {
       printf("Use -h for help\n");
       exit(1);
    }

    if (strcmp(argv[1],"-h") ==0) {
       usage(argv[0]);
       exit(0);
    }

    errorCode = atoi(argv[1]);

    if (errorCode > 0) errorCode = -errorCode;

    myErrName = rodsErrorName(errorCode, &mySubErrName);
    printf("irods error: %d %s %s\n",
	       errorCode, myErrName, mySubErrName);

    exit (0);
}


void usage (char *prog)
{
  printf("Converts an irods error code to text.\n");
  printf("Usage: %s [-vVh] errorNumber\n", prog);
  printf("The errorNumber can be preceeded with minus sign (-) or not\n");
  printf(" -h  this help\n");
  printReleaseInfo("ierror");
}

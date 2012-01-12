/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* 
 * irm - The irods rm utility
*/

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"
#include "rmUtil.h"
void usage ();

int
main(int argc, char **argv) {
    int status;
    rodsEnv myEnv;
    rErrMsg_t errMsg;
    rcComm_t *conn;
    rodsArguments_t myRodsArgs;
    char *optStr;
    rodsPathInp_t rodsPathInp;
    

    optStr = "hfruUvVfn:";
   
    status = parseCmdLineOpt (argc, argv, optStr, 0, &myRodsArgs);
    if (status < 0) {
        printf("Use -h for help.\n");
        exit (1);
    }
    if (myRodsArgs.help==True) {
       usage();
       exit(0);
    }

    if (argc - optind <= 0) {
        rodsLog (LOG_ERROR, "irm: no input");
        printf("Use -h for help.\n");
        exit (2);
    }

    status = getRodsEnv (&myEnv);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status, "main: getRodsEnv error. ");
        exit (1);
    }

    status = parseCmdLinePath (argc, argv, optind, &myEnv,
      UNKNOWN_OBJ_T, NO_INPUT_T, 0, &rodsPathInp);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status, "main: parseCmdLinePath error. ");
        printf("Use -h for help.\n");
        exit (1);
    }

    conn = rcConnect (myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
      myEnv.rodsZone, 1, &errMsg);

    if (conn == NULL) {
        exit (2);
    }
   
    status = clientLogin(conn);
    if (status != 0) {
        rcDisconnect(conn);
        exit (7);
    }

    status = rmUtil (conn, &myEnv, &myRodsArgs, &rodsPathInp);

    rcDisconnect(conn);

    if (status < 0) {
	exit (3);
    } else {
        exit(0);
    }

}

void 
usage ()
{
   char *msgs[]={
   "Usage : irm [-rUfvVh] [-n replNum] dataObj|collection ... ",
"Remove one or more data-object or collection from iRODS space. By default, ",
"the data-objects are moved to the trash collection (/myZone/trash) unless",
"either the -f option or the -n option is used.",
" ",
"The -U option allows the unregistering of the data object or collection",
"without deleting the physical file. Normally, a normal user cannot",
"unregister a data object if the physical file is located in a resource",
"vault. The acNoChkFilePathPerm rule allows this check to be bypassed.", 
" ",
"The irmtrash command should be used to delete data-objects in the trash",
"collection.",   
"Options are:",
" -f  force - Immediate removal of data-objects without putting them in trash .",
" -n  replNum  - the replica to remove; if not specified remove all replicas.",
"     This option is applicable only to the removal of data object and",
"     will be ignored for collection removal.",
" -r  recursive - remove the whole subtree; the collection, all data-objects",
"     in the collection, and any subcollections and sub-data-objects in the",
"     collection.",
" -U  unregister the file or collection",
" -v  verbose",
" -V  Very verbose",
" -h  this help",
""};
   int i;
   for (i=0;;i++) {
      if (strlen(msgs[i])==0) break;
      printf("%s\n",msgs[i]);
   }
   printReleaseInfo("irm");
}


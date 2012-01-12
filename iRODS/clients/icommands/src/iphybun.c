/*** Copyright (c), The Regents of the University of California            ***
 *** For more infophybunation please refer to files in the COPYRIGHT directory ***/
/* 
 * iphybun - The irods phybun utility
*/

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"
#include "phybunUtil.h"
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
    

    optStr = "hR:";
   
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
        rodsLog (LOG_ERROR, "iphybun: no input");
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

    status = phybunUtil (conn, &myEnv, &myRodsArgs, &rodsPathInp);

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
   "Usage : iphybun [-h] -R resource collection ... ",
"iphybun allows system admin to physically bundle files in a collection into",
"a number of tar files to make it more efficient to store these files on tape.",
"The tar files are placed into the /myZone/bundle/.... collection with file",
"names - collection.aRandomNumber. A new tar file will be created whenever",
"the number of subfiles exceeds 512 or the total size of the subfiles",
"exceeds 4 GBytes. A replica is registered for each bundled sub-files with",
"a fictitious resource - 'bundleResc' and a physical file path pointing to",
"the logical path of the tar bundle file. Whenever this copy of the subfile",
"is accessed, the tar file is untarred and staged automatically to the ",
"'cache' resource. Each extracted file is registered as a replica of its",
"corresponding subfiles.",
" ",
"A tar bundle file can be replicated or trimmed independently from its",
"corresponding subfiles. But it cannot be renamed or moved to trash.",
"It can be removed with the 'irm -f' command. But this  will trigger the ",
"staging of the subfiles before the tar file is removed.", 
"The -R flag specifies the resource of the bundle tar file. This input is",
"mandatory. The input resource must be a 'cache' type resource or a", 
"resource group that contains at least one 'cache' type resource."  

"Options are:",
" -R  resource - The resource where the bundle file is located",
" -h  this help",
""};
   int i;
   for (i=0;;i++) {
      if (strlen(msgs[i])==0) break;
      printf("%s\n",msgs[i]);
   }
   printReleaseInfo("iphybun");
}


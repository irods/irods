/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* 
 * ireg - The irods reg utility
*/

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"
#include "regUtil.h"
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
    int nArgv;
    

    optStr = "D:fhkKCG:R:vVZ";
   
    status = parseCmdLineOpt (argc, argv, optStr, 1, &myRodsArgs);

    if (status < 0) {
	printf("use -h for help.\n");
        exit (1);
    }

    if (myRodsArgs.help==True) {
       usage();
       exit(0);
    }

    nArgv = argc - optind;

    if (nArgv != 2) {      /* must have 2 inputs */
        usage ();
        exit (1);
    }

    status = getRodsEnv (&myEnv);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status, "main: getRodsEnv error. ");
        exit (1);
    }

    if ((*argv[optind] != '/' && strcmp (argv[optind], UNMOUNT_STR) != 0) || 
      *argv[optind + 1] != '/') { 
	rodsLog (LOG_ERROR,
	 "Input path must be absolute");
	exit (1);
    }

    status = parseCmdLinePath (argc, argv, optind, &myEnv,
      UNKNOWN_FILE_T, UNKNOWN_OBJ_T, 0, &rodsPathInp);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status, "main: parseCmdLinePath error. "); 
	printf("use -h for help.\n");
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

    status = regUtil (conn, &myEnv, &myRodsArgs, &rodsPathInp);

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
"Usage : ireg [-hfCkKvV] [--repl] [-D dataType] [-R resource] [-G rescGroup]",
"               physicalFilePath, irodsPath",
" ",
"Register a file or a directory of files and subdirectory into iRODS.",
"The file or the directory of files must already exist on the server where",
"the resource is located.  The full path must be supplied for both the",
"physicalFilePath and the irodsPath.",
" ",
"With the -C option, the entire content beneath the physicalFilePath",
"(files and subdirectories) will be recursively registered beneath the",
"irodsPath. For example, the command:",
" ",
"    ireg -C /tmp/src1 /tempZone/home/myUser/src1",
" ",
"grafts all files and subdirectories beneath the directory /tmp/src1 to",
"the collection /tempZone/home/myUser/src1", 
" ",
"An admin user will be able to register any Unix directory. But for a regular",
"user, he/she needs to have a UNIX account on the server with the same name as",
"his/her iRODS user account and only UNIX directories created with this",
"account can be registered by the user. Access control to the registered data",
"will be based on the access permission of the registeed collection.",
" ",
"For security reasons, file permissions are checked to make sure that",
"the client has the proper permission for the registration.",   
"The acNoChkFilePathPerm rule in core.re can be used to bypass the path",
"checking.",
" ",
" ",
"Options are:",
" -G  rescGroup - specifies the resource group of the resource. This must be",
"     input together with the -R option",
" -R  resource - specifies the resource to store to. This can also be specified",
"     in your environment or via a rule set up by the administrator.",
" -C  the specified path is a directory. The default assumes the path is a file.",
" -f  Force. If the target collection already exists, register the files and",
"     subdirectories that have not already been registered in the directory.",
" -k  calculate a checksum on the iRODS client and store with the file details.",
" -K  calculate a checksum on the iRODS server and store with the file details.",
" --repl  register the physical path as a replica.",
" -v  verbose",
" -V  Very verbose",
" -h  this help",
""};
   int i;
   for (i=0;;i++) {
      if (strlen(msgs[i])==0) break;
      printf("%s\n",msgs[i]);
   }
   printReleaseInfo("ireg");
}

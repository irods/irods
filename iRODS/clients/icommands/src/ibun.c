/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* 
 * ibun - bundle files operation
*/

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"
#include "bunUtil.h"
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
    

    optStr = "bhR:cxD:f";
   
    status = parseCmdLineOpt (argc, argv, optStr, 0, &myRodsArgs);

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

    status = parseCmdLinePath (argc, argv, optind, &myEnv,
      UNKNOWN_OBJ_T, UNKNOWN_OBJ_T, 0, &rodsPathInp);

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

    status = bunUtil (conn, &myEnv, &myRodsArgs, &rodsPathInp);

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
"Usage : ibun -x [-hb] [-R resource] structFilePath",
"               irodsCollection",
"Usage : ibun -c [-hf] [-R resource] [-D dataType] structFilePath",
"               irodsCollection",

" ",
"Bundle file operations. This command allows structured files such as ",
"tar files to be uploaded and downloaded to/from iRODS.",
" ",
"A tar file containing many small files can be created with normal unix",
"tar command on the client and then uploaded to the iRODS server as a",
"normal iRODS file. The 'ibun -x' command can then be used to extract/untar",
"the uploaded tar file. The extracted subfiles and subdirectories will",
"appeared as normal iRODS files and sub-collections. The 'ibun -c' command",
"can be used to tar/bundle an iRODS collection into a tar file.",
" ",
"For example, to upload a directory mydir to iRODS:",
" ",
"    tar -chlf mydir.tar -C /x/y/z/mydir .",
"    iput -Dtar mydir.tar .",
"    ibun -x mydir.tar mydir", 
" ",
"Note the use of -C option with the tar command which will tar the",
"content of mydir but without including the directory mydir in the paths.",
"The 'ibun -x' command extracts the tar file into the mydir collection.",
"The target mydir collection does not have to exist nor be empty.",
"If a subfile already exists in the target collection, the ingestion",
"of this subfile will fail (unless the -f flag is set) but the process",
"will continue.",
" ",
"It is generally a good practice to tag the tar file using the -Dtar flag",
"when uploading the file using iput. But if the tag is not made,",
"the server assumes it is a tar dataType. The dataType tag can be added",
"afterward with the isysmeta command. For example:",
"    isysmeta mod /tempZone/home/rods/mydir.tar datatype 'tar file'",
" ",
"The following command bundles the iRods collection mydir into a tar file:",
" ",
"    ibun -cDtar mydir1.tar mydir", 
" ",
"If a copy of a file to be bundled does not exist on the target resource,",
"a replica will automatically be made on the target resource.",
"Again, if the -D flag is not use, the bundling will be done using tar.",
" ",
"The -b option when used with the -x option, specifies bulk registration",
"which does up to 50 rgistrations at a time to reduce overhead.",
" ",
"Options are:",
" -b  bulk registration when used with -x to reduce overhead",
" -R  resource - specifies the resource to store to. This is optional",
"     in your environment",
" -D  dataType - the struct file data type. Valid only if the struct file",
"     does not exist. Currently only one dataType - 't' which specifies",
"     a tar file type is supported. If -D is not specified, the default is",
"     a tar file type",
" -x  extract the structFile and register the extracted files and directories",
"     under the input irodsCollection", 
" -c  bundle the files and sub-collection underneath the input irodsCollection",
"     and store it in the structFilePath",  
" -f  force overwrite the struct file (-c) or the subfiles (-x).", 
" -h  this help",
""};
   int i;
   for (i=0;;i++) {
      if (strlen(msgs[i])==0) break;
      printf("%s\n",msgs[i]);
   }
   printReleaseInfo("ibun");
}

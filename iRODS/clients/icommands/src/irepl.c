/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* 
 * irepl - The irods repl utility
*/

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"
#include "replUtil.h"
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
    int reconnFlag;
    

    optStr = "aBG:QMhrvVn:PR:S:TX:U";
   
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
        rodsLog (LOG_ERROR, "irepl: no input");
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

    if (myRodsArgs.reconnect == True) {
        reconnFlag = RECONN_TIMEOUT;
    } else {
        reconnFlag = NO_RECONN;
    }

    conn = rcConnect (myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
      myEnv.rodsZone, reconnFlag, &errMsg);

    if (conn == NULL) {
        exit (2);
    }
   
    status = clientLogin(conn);
    if (status != 0) {
        rcDisconnect(conn);
        exit (7);
    }

    if (myRodsArgs.progressFlag == True) {
        gGuiProgressCB = (irodsGuiProgressCallbak) iCommandProgStat;
    }

    status = replUtil (conn, &myEnv, &myRodsArgs, &rodsPathInp);

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
"Usage : irepl [-aBMPQrTvV] [-n replNum] [-R destResource] [-S srcResource]",
"[-G resourceGroup] [-X restartFile]  dataObj|collection ... ",
" ",
"Replicate a file in iRODS to another storage resource.",
" ",
"The -Q option specifies the use of the RBUDP transfer mechanism which uses",
"the UDP protocol for data transfer. The UDP protocol is very efficient",
"if the network is very robust with few packet losses. Two environment",
"variables - rbudpSendRate and rbudpPackSize are used to tune the RBUDP",
"data transfer. rbudpSendRate is used to throttle the send rate in ",
"kbits/sec. The default rbudpSendRate is 600,000. rbudpPackSize is used",
"to set the packet size. The dafault rbudpPackSize is 8192.",
" ",
"The -X option specifies that the restart option is on and the restartFile",
"input specifies a local file that contains the restart info. If the ",
"restartFile does not exist, it will be created and used for recording ",
"subsequent restart info. If it exists and is not empty, the restart info",
"contained in this file will be used for restarting the operation.",
"Note that the restart operation only works for uploading directories and",
"the path input must be identical to the one that generated the restart file",
" ",
"The -T option will renew the socket connection between the client and ",
"server after 10 minutes of connection. This gets around the problem of",
"sockets getting timed out by the firewall as reported by some users.",
" ",
"Note that if -a and -U options are used together, it means update all",
"stale copies and the resource group specified with the -R is ignored.",
"If both the -a and -R are used and the -R specifies a resource group,",
"then a copy is made for each resource in the resource group. If only the",
"-a option is used and the object is stored in one of the resources in a",
"resource group, then a copy is made for each resource in the resource group",
"and any stale copy in the resource group will also be updated.", 
" ",
"Note that replication is always within a zone.  For cross-zone duplication",
"see irsync which can operate within a zone or across zones.",
" ",
"Options are:",
" -a  all - only meaningful if input resource [-R resource] is a resource group.",
"     Replicate to all the resources in the resource group.",
" -B  Backup mode - if a good copy already exists in this",
"     resource, don't make another copy.",
" -G  resourceGroup  - specifies the source resource group of the data object",
"     to be replicated. If specified, only copies stored in this resource",
"     group will be replicated. This option is valid only if the path to be",
"     replicated is a collection.",
" -P  output the progress of the replication.",
" -Q  use RBUDP (datagram) protocol for the data transfer",
" -U  Update (Synchronize) all old replica with the latest copy.",
" -M  admin - admin user uses this option to backup/replicate other users files",
" -r  recursive - copy the whole subtree",
" -n  replNum  - the replica to copy, typically not needed",
" -R  destResource - specifies the destination resource to store to.",
"     This can also be specified in your environment or via a rule set up", 
"     by the administrator.",
" -S  srcResource - specifies the source resource of the data object to be",
"     replicated. If specified, only copies stored in this resource will",
"     be replicated. Otherwise, one of the copy will be replicated.",
" -T  renew socket connection after 10 minutes",
" -v  verbose",
" -V  Very verbose",
" -X  restartFile - specifies that the restart option is on and the",
"     restartFile input specifies a local file that contains the restart info.",
" -h  this help",
" ",
"Also see 'irsync' for other types of iRODS/local synchronization.",
""};
   int i;
   for (i=0;;i++) {
      if (strlen(msgs[i])==0) break;
      printf("%s\n",msgs[i]);
   }
   printReleaseInfo("irepl");
}

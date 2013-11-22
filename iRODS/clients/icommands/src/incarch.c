/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* 
 * incarch.c - Archive irods NETCDF time series data utility
*/

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"
#include "ncarchUtil.h"
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
    

    optStr = "hs:Z";
   
    status = parseCmdLineOpt (argc, argv, optStr, 1, &myRodsArgs);

    if (status < 0) {
        printf("Use -h for help.\n");
        exit (1);
    }

    if (myRodsArgs.help==True) {
       usage();
       exit(0);
    }

    if (argc - optind <= 0) {
        rodsLog (LOG_ERROR, "incattr: no input");
        printf("Use -h for help.\n");
        exit (2);
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

    status = ncarchUtil (conn, &myEnv, &myRodsArgs, &rodsPathInp);

    printErrorStack(conn->rError);

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
"Usage : incarch [-h] [-s fileSize][--new startingTimeIndex]",
"sourceObjPath aggCollection",
" ",
"Archive open ended NETCDF time series data",
" ",
"This command can be used to archive a special class of NETCDF data -",
"open ended time series data that is being updated continuously. Typically",
"these are data served by Opendap servers such as Thredds Data Server (TDS)",
"and ERDDAP servers which aggregate multiple NETCDF files in a time series",
"logically and time series is presented to users as a single file. The time",
"dimension must have a case insensitive name 'time' containing the number of",
"seconds since the Epoch in values. Access of these NETCDF files can be",
"achieved through the TDS and ERDDAP drivers of iRODS once the URL of these",
"files have been registered with iCAT (see examples in", 
"https://www.irods.org/index.php/NETCDF). This command can then be used to",
"incrementally archive these time series data to an iRODS 'aggregate",
"collection'. An 'aggregate collection' is a collection containing multiple",
"NETCDF files in a time series tied together by the 'time' dimension.",
" ",
"Archival of the time series data is done by incrementally adding NETCDF",
"files to the 'aggregate collection'. In addition, the '.aggInfo' file",
"which contains info such as starting time and ending time of each NETCDF",
"files, is updated. The '.aggInfo' file is needed for a collection to",
"be used as an 'aggregate collection'.", 
" ",
"The input sourceObjPath is the path of the time series data to be archived.",
"Typically, this path represents an iRODS data object created through",
"registration (ireg) with the physical path representing a TDS or ERDDAP URL.", 
" ",
"The input aggCollection is the path of the target 'aggregate collection'.",
" ",
"The --new option specifies that a brand new archive is to be started and",
"the startingTimeIndex associated with this option specifies the 'time'",
"dimension index to start the archive. For example, if the 'time' dimension",
"has an array length (can be obtained with a 'inc --noattr' command) of 1000.",
"a startingTimeIndex of 0 means that the new archive starts from the",
"beginning of the time series and a value of 999 specifies the archive starts",
"from the end. If this option is not specified, an incremental backup from",
"an existing archive is assumed.",
" ",
"The -s option specifies the approximate file size limit in MBytes of each",
"target file in the 'aggregate collection'. i.e, if the incremental backup",
"exceeds this limit, an archive file with size approximately equal to this",
"limit is created and new files are created to archive the additional data.",
"The default limit is 1 GBytes. It should be noted this limit is not strictly",
"enforced and up to 50% can be exceeded for the last file of an achive",
"operation to prevent creating small archive files.",
" ", 
"Options are:",
" -s fileSize - The size limit of each target archive file in MBytes.",
"       The default limit is 1 GBytes.",
"--new startingTimeIndex - Start a brand new time series archive.",
"       startingTimeIndex specifies the starting time index of the archive.",
" -h  this help",
""};
   int i;
   for (i=0;;i++) {
      if (strlen(msgs[i])==0) break;
      printf("%s\n",msgs[i]);
   }
   printReleaseInfo("incattr");
}

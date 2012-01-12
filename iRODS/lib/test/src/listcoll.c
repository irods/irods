/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* listcoll.c - test the two sets of calls to list the contents of a
 * collection:
 * 1) 
 */

#include "rodsClient.h" 
#include "miscUtil.h" 

int
printCollection (rcComm_t *conn, char *collection, int flags);
int
printCollectionNat (rcComm_t *conn, char *collection, int flags);

int
main(int argc, char **argv)
{
    rcComm_t *conn;
    rodsEnv myEnv;
    char *optStr;
    rErrMsg_t errMsg;
    int status;
    rodsArguments_t rodsArgs;
    int flag = 0;
    int nativeAPI = 0;

    optStr = "alL";

    status = parseCmdLineOpt (argc, argv, optStr, 1, &rodsArgs);

    if (status < 0) {
        printf("parseCmdLineOpt error, status = %d.\n", status);
        exit (1);
    }

    if (argc - optind <= 0) {
        rodsLog (LOG_ERROR, "no input");
        exit (2);
    }

    memset (&errMsg, 0, sizeof (rErrMsg_t));

    status = getRodsEnv (&myEnv);

    if (status < 0) {
	fprintf (stderr, "getRodsEnv error, status = %d\n", status);
	exit (1);
    }


    conn = rcConnect (myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
      myEnv.rodsZone, 1, &errMsg);

    if (conn == NULL) {
        fprintf (stderr, "rcConnect error\n");
        exit (1);
    }

    status = clientLogin(conn);
    if (status != 0) {
        rcDisconnect(conn);
        exit (7);
    }

    flag = setQueryFlag (&rodsArgs);

    if (rodsArgs.all == True) nativeAPI = 1;

    if (nativeAPI == 1) {
	status = printCollectionNat (conn, argv[optind], flag);
    } else {
        status = printCollection (conn, argv[optind], flag);
    }

    rcDisconnect (conn);

    exit (0);
} 

int
printCollection (rcComm_t *conn, char *collection, int flags)
{
    int status;
    collHandle_t collHandle;
    collEnt_t collEnt;

    status = rclOpenCollection (conn, collection, flags, &collHandle);
    if (status < 0) {
        fprintf (stderr, "rclOpenCollection of %s error. status = %d\n",
          collection, status);
        return status;
    }

    while ((status = rclReadCollection (conn, &collHandle, &collEnt)) >= 0) {
        if (collEnt.objType == DATA_OBJ_T) {
	    printf ("D - %s/%s\n", collEnt.collName, collEnt.dataName);
	    printf ("      collName - %s\n", collEnt.collName);
	    printf ("      dataName - %s\n", collEnt.dataName);
	    printf ("      dataId - %s\n", collEnt.dataId);
	    if (collHandle.flags > 0) {
	        printf ("      ownerName - %s\n", collEnt.ownerName);
	        printf ("      resource - %s\n", collEnt.resource);
		printf ("      replNum - %d\n", collEnt.replNum);
		printf ("      dataSize - %lld\n", collEnt.dataSize);
		printf ("      replStatus - %d\n", collEnt.replStatus);
                printf ("      modifyTime (UNIX clock in string) - %s\n",
                  collEnt.modifyTime);

		if ((collHandle.flags & VERY_LONG_METADATA_FG) != 0) {
		    printf ("      phyPath - %s\n", collEnt.phyPath);
		    printf ("      chksum - %s\n", collEnt.chksum);
		    printf ("      createTime (UNIX clock in string) - %s\n", 
		      collEnt.createTime);
		}
	    }
	} else if (collEnt.objType == COLL_OBJ_T) {
	    printf ("C - %s\n", collEnt.collName);
	    printf ("      collOwner %s\n", collEnt.ownerName);
	    /* recursive print */
	    printCollection (conn, collEnt.collName, collHandle.flags);
	}
    }
    rclCloseCollection (&collHandle);

    if (status < 0 && status != CAT_NO_ROWS_FOUND) {
        return (status);
    } else {
	return (0);
    }
}
	
int
printCollectionNat (rcComm_t *conn, char *collection, int flags)
{
    int status;
    openCollInp_t openCollInp;
    collEnt_t *collEnt;
    int handleInx;

    memset (&openCollInp, 0, sizeof (openCollInp));
    rstrcpy (openCollInp.collName, collection, MAX_NAME_LEN);
    openCollInp.flags = flags;
    handleInx = rcOpenCollection (conn, &openCollInp);
    if (handleInx < 0) {
        fprintf (stderr, "rcOpenCollection of %s error. status = %d\n",
          collection, handleInx);
        return (handleInx);
    }

    while ((status = rcReadCollection (conn, handleInx, &collEnt)) >= 0) {
        if (collEnt->objType == DATA_OBJ_T) {
	    printf ("D - %s/%s\n", collEnt->collName, collEnt->dataName);
	    printf ("      collName - %s\n", collEnt->collName);
	    printf ("      dataName - %s\n", collEnt->dataName);
	    printf ("      dataId - %s\n", collEnt->dataId);
	    if (flags > 0) {
	        printf ("      ownerName - %s\n", collEnt->ownerName);
	        printf ("      resource - %s\n", collEnt->resource);
		printf ("      replNum - %d\n", collEnt->replNum);
		printf ("      dataSize - %lld\n", collEnt->dataSize);
		printf ("      replStatus - %d\n", collEnt->replStatus);
                printf ("      modifyTime (UNIX clock in string) - %s\n",
                  collEnt->modifyTime);

		if ((flags & VERY_LONG_METADATA_FG) != 0) {
		    printf ("      phyPath - %s\n", collEnt->phyPath);
		    printf ("      chksum - %s\n", collEnt->chksum);
		    printf ("      createTime (UNIX clock in string) - %s\n", 
		      collEnt->createTime);
		}
	    }
	} else if (collEnt->objType == COLL_OBJ_T) {
	    printf ("C - %s\n", collEnt->collName);
	    printf ("      collOwner %s\n", collEnt->ownerName);
	    /* recursive print */
            printCollection (conn, collEnt->collName, flags);
	}
	freeCollEnt (collEnt);
    }

    rcCloseCollection (conn, handleInx);

    if (status < 0 && status != CAT_NO_ROWS_FOUND) {
        return (status);
    } else {
	return (0);
    }
}
	

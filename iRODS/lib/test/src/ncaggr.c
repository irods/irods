/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* nctest.c - test the iRODS NETCDF api */

#include "rodsClient.h" 

#define TEST_PATH1 "./ncdata/HFRadarCurrent"

#define TEST_OBJ_PATH "/oneZone/home/rods/ncdata/HFRadarCurrent/SFCurrent1_1.nc"
#define TEST_OBJ_COLL "/oneZone/home/rods/ncdata/HFRadarCurrent"
#if 0
#define ARCH_COLLECTION "/wanZone/home/rods/agg/HFRADAR, Alaska - North Slope, 6km Resolution, Hourly RTV"
#define ARCH_SRC_OBJ_PATH "/wanZone/home/rods/tds/HFRNET.AKNS.RTV/HFRADAR, Alaska - North Slope, 6km Resolution, Hourly RTV"
#else
#define ARCH_COLLECTION "/oneZone/home/rods/agg/HFRADAR, Alaska - North Slope, 6km Resolution, Hourly RTV"
#define ARCH_SRC_OBJ_PATH "/oneZone/home/rods/tds/HF RADAR, Alaska, North Slope/HFRADAR, Alaska - North Slope, 6km Resolution, Hourly RTV"
#endif
#if 0
#define RESC "hpResc1"
#else
#define RESC "demoResc1"
#endif
int
genNcAggInfo (char *testPath, ncAggInfo_t *ncAggInfo);
int 
setNcAggElement (int ncid, ncInqOut_t *ncInqOut, ncAggElement_t *ncAggElement);
unsigned int
getIntVar (int ncid, int varid, int dataType, rodsLong_t inx);
int
addNcAggElement (ncAggElement_t *ncAggElement, ncAggInfo_t *ncAggInfo);
int
testGetAggElement (rcComm_t *conn, char *objPath);
int
testGetAggInfo (rcComm_t *conn, char *collPath);
int
testAggNcOpr (rcComm_t *conn, char *collPath);
int
testArch (rcComm_t *conn, char *objPath, char *archCollection);
int
main(int argc, char **argv)
{
    char *testPath;
    int status;
    rcComm_t *conn;
    rodsEnv myRodsEnv;
    rErrMsg_t errMsg;

    if (argc <= 1) {
        testPath = TEST_PATH1;
    } else {
	testPath = argv[1];
    }

    status = getRodsEnv (&myRodsEnv);

    if (status < 0) {
        fprintf (stderr, "getRodsEnv error, status = %d\n", status);
        exit (1);
    }


    conn = rcConnect (myRodsEnv.rodsHost, myRodsEnv.rodsPort,
      myRodsEnv.rodsUserName, myRodsEnv.rodsZone, 0, &errMsg);

    if (conn == NULL) {
        fprintf (stderr, "rcConnect error\n");
        exit (1);
    }
    status = clientLogin(conn);
    if (status != 0) {
        fprintf (stderr, "clientLogin error\n");
       rcDisconnect(conn);
       exit (2);
    }
    status = testGetAggElement (conn, TEST_OBJ_PATH);
    if (status < 0) {
        fprintf (stderr, "testGetAggElement of %s failed. status = %d\n",
        TEST_OBJ_PATH, status);
        exit (1);
    }

    status = testGetAggInfo (conn, TEST_OBJ_COLL);
    if (status < 0) {
        fprintf (stderr, "testGetAggInfo of %s failed. status = %d\n",
        TEST_OBJ_COLL, status);
        exit (1);
    }

    status = testAggNcOpr (conn, TEST_OBJ_COLL);
    if (status < 0) {
        fprintf (stderr, "testAggNcOpr of %s failed. status = %d\n",
        TEST_OBJ_COLL, status);
        exit (1);
    }

    status = testArch (conn, ARCH_SRC_OBJ_PATH, ARCH_COLLECTION);
    if (status < 0) {
        fprintf (stderr, "testArch of %s in %s failed. status = %d\n",
        ARCH_SRC_OBJ_PATH, ARCH_COLLECTION, status);
        exit (1);
    }

#if 0
    status = genNcAggInfo (testPath, &ncAggInfo);

    if (status < 0) {
        fprintf (stderr, "genNcAggInfo of %s failed. status = %d\n", 
        testPath, status);
        exit (1);
    }
#endif
    exit (0);
}

int
testGetAggElement (rcComm_t *conn, char *objPath)
{
    ncOpenInp_t ncOpenInp;
    
    ncAggElement_t *ncAggElement = NULL;
    int status;

    bzero (&ncOpenInp, sizeof (ncOpenInp));
    rstrcpy (ncOpenInp.objPath, objPath, MAX_NAME_LEN);
    status = rcNcGetAggElement (conn, &ncOpenInp, &ncAggElement);

    return status;
}

int
testGetAggInfo (rcComm_t *conn, char *collPath)
{
    ncOpenInp_t ncOpenInp;
    ncAggInfo_t *ncAggInfo = NULL;
    int i, status;
    int ncid;
    ncInqInp_t ncInqInp;
    ncInqOut_t *ncInqOut = NULL;
    ncGetVarInp_t ncGetVarInp;
    ncGetVarOut_t *ncGetVarOut = NULL;
    ncGenVarOut_t *var;
    rodsLong_t start[NC_MAX_DIMS], stride[NC_MAX_DIMS], count[NC_MAX_DIMS];
    ncCloseInp_t ncCloseInp;

    bzero (&ncOpenInp, sizeof (ncOpenInp));
    rstrcpy (ncOpenInp.objPath, collPath, MAX_NAME_LEN);
    ncOpenInp.mode = NC_WRITE;
    addKeyVal (&ncOpenInp.condInput, DEST_RESC_NAME_KW, RESC);
    status = rcNcGetAggInfo (conn, &ncOpenInp, &ncAggInfo);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcGetAggInfo error for %s", ncOpenInp.objPath);
        return status;
    }
    ncOpenInp.mode = NC_NOWRITE;
    status = rcNcOpen (conn, &ncOpenInp, &ncid);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcOpen error for %s", ncOpenInp.objPath);
        return status;
    }
    bzero (&ncInqInp, sizeof (ncInqInp));
    ncInqInp.ncid = ncid;
    ncInqInp.paramType = NC_ALL_TYPE;
    ncInqInp.flags = NC_ALL_FLAG;

    status = rcNcInq (conn, &ncInqInp, &ncInqOut);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcInq error for %s", ncOpenInp.objPath);
        return status;
    }
    if (status < 0) return status;

    /* rcNcGetVarsByType - do the last var and 3 */
    var =  &ncInqOut->var[ncInqOut->nvars-1];

    for (i = 0; i < var->nvdims; i++) {
        int dimId = var->dimId[i];
        if (strcasecmp (ncInqOut->dim[dimId].name, "time") == 0) {
            start[i] = 699;
            count[i] = 4;
        } else {
            start[i] = 0;
            count[i] = ncInqOut->dim[dimId].arrayLen;
        }
        stride[i] = 1;
    }
    bzero (&ncGetVarInp, sizeof (ncGetVarInp));
    ncGetVarInp.dataType = var->dataType;
    ncGetVarInp.ncid = ncid;
    ncGetVarInp.varid = var->id;
    ncGetVarInp.ndim = var->nvdims;
    ncGetVarInp.start = start;
    ncGetVarInp.count = count;
    ncGetVarInp.stride = stride;

    status = rcNcGetVarsByType (conn, &ncGetVarInp, &ncGetVarOut);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcGetVarsByType error for %s of %s", var->name,
          ncOpenInp.objPath);
        return status;
    }
    bzero (&ncCloseInp, sizeof (ncCloseInp_t));
    ncCloseInp.ncid = ncid;
    rcNcClose (conn, &ncCloseInp);

    return status;
}

int
testAggNcOpr (rcComm_t *conn, char *collPath)
{
    ncOpenInp_t ncOpenInp;
    int ncid;
    int status;
    ncCloseInp_t ncCloseInp;

    bzero (&ncOpenInp, sizeof (ncOpenInp));
    rstrcpy (ncOpenInp.objPath, collPath, MAX_NAME_LEN);
    ncOpenInp.mode = NC_NOWRITE | NC_NETCDF4;
    addKeyVal (&ncOpenInp.condInput, DEST_RESC_NAME_KW, "hpResc1");
    status = rcNcOpen (conn, &ncOpenInp, &ncid);
    if (status < 0) return status;
    bzero (&ncCloseInp, sizeof (ncCloseInp_t));
    ncCloseInp.ncid = ncid;
    rcNcClose (conn, &ncCloseInp);

    return status;
}

int
testArch (rcComm_t *conn, char *objPath, char *archCollection)
{
    ncArchTimeSeriesInp_t ncArchTimeSeriesInp;
    int status;

    bzero (&ncArchTimeSeriesInp, sizeof (ncArchTimeSeriesInp));
    rstrcpy (ncArchTimeSeriesInp.objPath, objPath, MAX_NAME_LEN);
    rstrcpy (ncArchTimeSeriesInp.aggCollection, archCollection, MAX_NAME_LEN);

    status = rcNcArchTimeSeries (conn, &ncArchTimeSeriesInp);

    return status;
}

#if 0
int
genNcAggInfo (char *dirPath, ncAggInfo_t *ncAggInfo)
{
    DIR *dirPtr;
    struct dirent *myDirent;
    int status = 0;
    char childPath[MAX_NAME_LEN];
    struct stat statbuf;
    ncInqInp_t ncInqInp;
    ncInqOut_t *ncInqOut = NULL;
    int ncid;
    ncAggElement_t ncAggElement;

    if (dirPath == NULL || ncAggInfo == NULL) return USER__NULL_INPUT_ERR;

    dirPtr = opendir (dirPath);
    if (dirPtr == NULL) {
        rodsLog (LOG_ERROR,
        "genNcAggInfo: opendir local dir error for %s, errno = %d\n",
         dirPath, errno);
        return (USER_INPUT_PATH_ERR);
    }
    bzero (ncAggInfo, sizeof (ncAggInfo_t));
    bzero (&ncAggElement, sizeof (ncAggElement));
    rstrcpy (ncAggInfo->ncObjectName, dirPath, MAX_NAME_LEN);
    while ((myDirent = readdir (dirPtr)) != NULL) {
        if (strcmp (myDirent->d_name, ".") == 0 ||
          strcmp (myDirent->d_name, "..") == 0) {
            continue;
        }
        snprintf (childPath, MAX_NAME_LEN, "%s/%s",
          dirPath, myDirent->d_name);
        status = stat (childPath, &statbuf);
        if (status != 0) {
            rodsLog (LOG_ERROR,
              "genNcAggInfo: stat error for %s, errno = %d\n",
              childPath, errno);
            continue;
        } else if (statbuf.st_mode & S_IFDIR) {
            rodsLog (LOG_ERROR,
              "genNcAggInfo: %s is a directory, errno = %d\n",
              childPath, errno);
            continue;
        }
        status = nc_open (childPath, NC_NOWRITE, &ncid);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "genNcAggInfo: nc_open %s error, status = %d, %s",
              childPath, status, nc_strerror(status));
            continue;
        }
        /* do the general inq */
        bzero (&ncInqInp, sizeof (ncInqInp));
        ncInqInp.ncid = ncid;
        ncInqInp.paramType = NC_ALL_TYPE;
        ncInqInp.flags = NC_ALL_FLAG;

        status = ncInq (&ncInqInp, &ncInqOut);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "genNcAggInfo: rcNcInq error for %s", childPath);
            return status;
        }
        status = setNcAggElement (ncid, ncInqOut, &ncAggElement);
        nc_close (ncid);
        if (status < 0) break;
        rstrcpy (ncAggElement.objPath, childPath, MAX_NAME_LEN);
        status = addNcAggElement (&ncAggElement, ncAggInfo);
        if (status < 0) break;
        bzero (&ncAggElement, sizeof (ncAggElement));
    }
    closedir (dirPtr);
    return 0;
}

int 
setNcAggElement (int ncid, ncInqOut_t *ncInqOut, ncAggElement_t *ncAggElement)
{
    int i, j;

    if (ncInqOut == NULL || ncAggElement == NULL) return USER__NULL_INPUT_ERR;

    for (i = 0; i < ncInqOut->ndims; i++) {
        if (strcasecmp (ncInqOut->dim[i].name, "time") == 0) break;
    }
    if (i >= ncInqOut->ndims) {
        /* no match */
        rodsLog (LOG_ERROR, 
          "genNcAggInfo: 'time' dim does not exist");
        return NETCDF_DIM_MISMATCH_ERR;
    }
    for (j = 0; j < ncInqOut->nvars; j++) {
        if (strcmp (ncInqOut->dim[i].name, ncInqOut->var[j].name) == 0) {
            break;
        }
    }

    if (j >= ncInqOut->nvars) {
        /* no match */
        rodsLog (LOG_ERROR, 
          "genNcAggInfo: 'time' var does not exist");
        return NETCDF_DIM_MISMATCH_ERR;
    }


    ncAggElement->arraylen = ncInqOut->dim[i].arrayLen;

    ncAggElement->startTime = getNcIntVar (ncid, ncInqOut->var[j].id, 
      ncInqOut->var[j].dataType, 0);
    ncAggElement->endTime = getNcIntVar (ncid, ncInqOut->var[j].id, 
      ncInqOut->var[j].dataType, ncInqOut->dim[i].arrayLen - 1);

    return 0;
}

unsigned int
getIntVar (int ncid, int varid, int dataType, rodsLong_t inx)
{
    size_t start[1], count[1];
    short int myshort;
    int myint;
    rodsLong_t mylong;
    float myfloat;
    double mydouble;
    unsigned int retint; 
    int status;
    

    start[0] = inx;
    count[0] = 1;

    if (dataType == NC_SHORT || dataType == NC_USHORT) {
        status = nc_get_vara (ncid, varid, start, count, (void *) &myshort);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "genNcAggInfo: nc_get_vara error, status = %d, %s",
              status, nc_strerror(status));
            return NETCDF_GET_VARS_ERR - status;
        }
        retint = (unsigned int) myshort;
    } else if (dataType == NC_INT || dataType == NC_UINT) {
        status = nc_get_vara (ncid, varid, start, count, (void *) &myint);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "genNcAggInfo: nc_get_vara error, status = %d, %s",
              status, nc_strerror(status));
            return NETCDF_GET_VARS_ERR - status;
        }
        retint = (unsigned int) myint;
    } else if (dataType == NC_INT64 || dataType == NC_UINT64) {
        status = nc_get_vara (ncid, varid, start, count, (void *) &mylong);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "genNcAggInfo: nc_get_vara error, status = %d, %s",
              status, nc_strerror(status));
            return NETCDF_GET_VARS_ERR - status;
        }
        retint = (unsigned int) mylong;
    } else if (dataType == NC_FLOAT) {
        status = nc_get_vara (ncid, varid, start, count, (void *) &myfloat);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "genNcAggInfo: nc_get_vara error, status = %d, %s",
              status, nc_strerror(status));
            return NETCDF_GET_VARS_ERR - status;
        }
        retint = (unsigned int) myfloat;
    } else if (dataType == NC_DOUBLE) {
        status = nc_get_vara (ncid, varid, start, count, (void *) &mydouble);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "genNcAggInfo: nc_get_vara error, status = %d, %s",
              status, nc_strerror(status));
            return NETCDF_GET_VARS_ERR - status;
        }
        retint = (unsigned int) mydouble;
    } else {
        rodsLog (LOG_ERROR,
          "getNcTypeStr: Unsupported dataType %d", dataType);
        return (NETCDF_INVALID_DATA_TYPE);
    }
    
    return retint;
}

int
addNcAggElement (ncAggElement_t *ncAggElement, ncAggInfo_t *ncAggInfo)
{
    int newNumFiles, i, j;
    ncAggElement_t *newElement;

    if (ncAggInfo == NULL || ncAggElement == NULL) return USER__NULL_INPUT_ERR;

    if ((ncAggInfo->numFiles % PTR_ARRAY_MALLOC_LEN) == 0) {
        newNumFiles =  ncAggInfo->numFiles + PTR_ARRAY_MALLOC_LEN;
        newElement = (ncAggElement_t *) calloc (newNumFiles, 
          sizeof (ncAggElement_t));
        if (ncAggInfo->numFiles > 0) {
            if (ncAggInfo->ncAggElement == NULL) {
                rodsLog (LOG_ERROR,
                  "addNcAggElement: numFiles > 0 but cAggElement == NULL");
                return (NETCDF_VAR_COUNT_OUT_OF_RANGE);
            }
            memcpy (newElement, ncAggInfo->ncAggElement, 
              ncAggInfo->numFiles * sizeof (newElement));
            free (ncAggInfo->ncAggElement);
        } 
        ncAggInfo->ncAggElement = newElement;
    }

    if (ncAggInfo->numFiles <= 0) {
        ncAggInfo->ncAggElement[0] = *ncAggElement;
    } else {
        for (i = 0; i < ncAggInfo->numFiles; i++) {
            ncAggElement_t *myElement = &ncAggInfo->ncAggElement[i];
            if (ncAggElement->startTime < myElement->startTime || 
              (ncAggElement->startTime == myElement->startTime &&
              ncAggElement->endTime < myElement->endTime)) {
                /* insert */
                for (j = ncAggInfo->numFiles; j > i; j--) {
                    ncAggInfo->ncAggElement[j] = ncAggInfo->ncAggElement[j-1];
                }
                ncAggInfo->ncAggElement[i] = *ncAggElement;
                break;
            }
        }
        if (i >= ncAggInfo->numFiles) {
            /* not inserted yet. put it at the end */
            ncAggInfo->ncAggElement[i] = *ncAggElement;
        }
    }
    ncAggInfo->numFiles++;            
    return 0;
}
#endif

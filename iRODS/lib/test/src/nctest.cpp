/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* nctest.c - test the iRODS NETCDF api */

#include "rodsClient.h" 

/* a copy of sfc_pres_temp.nc can be found in ../netcdf/sfc_pres_temp.nc */
#if 0
#define TEST_PATH1 "/wanZone/home/rods/hdf5/SDS.h5"
#define TEST_PATH1 "/wanZone/home/rods/netcdf/sfc_pres_temp.nc"
#define TEST_PATH2 "/wanZone/home/rods/netcdf/pres_temp_4D.nc"
#define TEST_PATH2 "/oneZone/home/rods/erddap/erdCalcofiBio"
#endif
#define TEST_PATH1 "/oneZone/home/rods/hdf5/group.h5"
#define TEST_PATH2 "/oneZone/home/rods/pydap/glider/glider114.nc"

#define NC_OUTFILE	"ncoutfile.nc"
int
myInqVar (rcComm_t *conn, int ncid, char *name, int *dataType, int *ndim);
int
nctest1 (rcComm_t *conn, char *ncpath, char *grpPath);
int
nctest2 (rcComm_t *conn, char *ncpath);
int
nctestold (rcComm_t *conn, char *ncpath);
int
printVarOut (ncGenVarOut_t *var);
int
regNcGlobalAttr (rcComm_t *conn, char *objPath, ncInqOut_t *ncInqOut,
int adminFlag);

int
main(int argc, char **argv)
{
    rcComm_t *conn;
    rodsEnv myRodsEnv;
    rErrMsg_t errMsg;
    char *testPath;
    int status;

int ncid;
#if 0
status = nc_open ("http://coastwatch.pfeg.noaa.gov/erddap/tabledap/nosCoopsMAT.nc?stationID,stationName,longitude,latitude,time,dcp,sensor,AT,X,N,R&time>=2012-07-19T00:00:00Z", NC_NETCDF4, &ncid);
status = nc_open ("http://coastwatch.pfeg.noaa.gov/erddap/tabledap/nosCoopsMAT?stationID,stationName,longitude,latitude,time,dcp,sensor,AT,X,N,R&time>=2012-07-23", NC_NETCDF4, &ncid);
status = nc_open ("http://coastwatch.pfeg.noaa.gov/erddap/tabledap/nosCoopsMAT&time>=2012-07-23", NC_NETCDF4, &ncid);
printf ("nc_open status = %d  %s\n", status,  nc_strerror(status));
exit (0);
#endif

    if (argc <= 1) {
        testPath = TEST_PATH2;
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
#if 0	/* specify a specific group name */
    status = nctest1 (conn, TEST_PATH1, "/Data_new");
    status = nctest1 (conn, TEST_PATH1, "INQ");
    if (status < 0) {
	fprintf (stderr, "nctest1 of %s failed. status = %d\n", 
        TEST_PATH1, status);
    }
    status = nctestold (conn, testPath);

    if (status < 0) {
        fprintf (stderr, "nctestold of %s failed. status = %d\n",
        testPath, status);
    }
#endif

    status = nctest1 (conn, testPath, NULL);

    if (status < 0) {
        fprintf (stderr, "nctest1 of %s failed. status = %d\n", 
        testPath, status);
    }

#if 0
    status = nctest2 (conn, TEST_PATH1);
    if (status < 0) {
        fprintf (stderr, "nctest2 of %s failed. status = %d\n",
        TEST_PATH1, status);
    }
    status = nctest2 (conn, TEST_PATH2);

    if (status < 0) {
        fprintf (stderr, "nctest2 of %s failed. status = %d\n",
        TEST_PATH2, status);
    }
#endif
    rcDisconnect(conn);

    exit (0);
}

int
nctestold (rcComm_t *conn, char *ncpath)
{
    int status, i;
    ncOpenInp_t ncOpenInp;
    ncCloseInp_t ncCloseInp;
    ncInqIdInp_t ncInqIdInp;
    ncInqWithIdOut_t *ncInqWithIdOut = NULL;
    ncGetVarInp_t ncGetVarInp;
    ncGetVarOut_t *ncGetVarOut = NULL;
    int ncid1 = 0;
    int *levdimid1 = NULL;
    int levdimlen1 = 0;
    int *timedimid1 = NULL;
    int timedimlen1 = 0;
    int *londimid1 = NULL;
    int londimlen1 = 0;
    int lontype1 = 0;
    int *latdimid1 = NULL;
    int latdimlen1 = 0;
    int lattype1 = 0;
    int lonvarid1;
    int latvarid1;
    int tempvarid1;
    int temptype1 = 0;
    int presvarid1;
    int prestype1 = 0;
    int lonndim, latndim, tempndim, presndim;
    rodsLong_t start[NC_MAX_DIMS], stride[NC_MAX_DIMS], count[NC_MAX_DIMS];
#ifdef LIB_CF
    nccfGetVarInp_t nccfGetVarInp;
    nccfGetVarOut_t *nccfGetVarOut = NULL;
    float *mydata;
#endif

    printf ("----- nctest1 for %s ------\n\n\n", ncpath);

    /* open an nc object */
    bzero (&ncOpenInp, sizeof (ncOpenInp_t));
    rstrcpy (ncOpenInp.objPath, ncpath, MAX_NAME_LEN);
    ncOpenInp.mode = NC_NOWRITE;

    status = rcNcOpen (conn, &ncOpenInp, &ncid1);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcOpen error for %s", ncOpenInp.objPath);
	return status;
    }

    /* inq the time length */
    bzero (&ncInqIdInp, sizeof (ncInqIdInp));
    ncInqIdInp.paramType = NC_DIM_T;
    ncInqIdInp.ncid = ncid1;
    rstrcpy (ncInqIdInp.name, "time", MAX_NAME_LEN);
    status = rcNcInqId (conn, &ncInqIdInp, &timedimid1);
    if (status < 0) {
        printf ("No time dim\n");
        timedimlen1 = 0;
    } else {
        ncInqIdInp.myid = *timedimid1;
        status = rcNcInqWithId (conn, &ncInqIdInp, &ncInqWithIdOut);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "rcNcInqWithId error for dim %s of %s", ncInqIdInp.name,
              ncOpenInp.objPath);
            return status;
        } else {
            printf ("%s ---- dim length = %lld\n", ncInqIdInp.name,
              ncInqWithIdOut->mylong);
            timedimlen1 = ncInqWithIdOut->mylong;
            free (ncInqWithIdOut);
            ncInqWithIdOut = NULL;
        }
    }

    /* inq the level length */
    bzero (&ncInqIdInp, sizeof (ncInqIdInp));
    ncInqIdInp.paramType = NC_DIM_T;
    ncInqIdInp.ncid = ncid1;
    rstrcpy (ncInqIdInp.name, "level", MAX_NAME_LEN);
    status = rcNcInqId (conn, &ncInqIdInp, &levdimid1);
    if (status < 0) {
	printf ("No level dim\n");
	levdimlen1 = 0;
    } else {
        ncInqIdInp.myid = *levdimid1;
        status = rcNcInqWithId (conn, &ncInqIdInp, &ncInqWithIdOut);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "rcNcInqWithId error for dim %s of %s", ncInqIdInp.name,
              ncOpenInp.objPath);
            return status;
        } else {
            printf ("%s ---- dim length = %lld\n", ncInqIdInp.name,
              ncInqWithIdOut->mylong);
            levdimlen1 = ncInqWithIdOut->mylong;
            free (ncInqWithIdOut);
            ncInqWithIdOut = NULL;
        }
    }

    /* inq the dimension length */
    bzero (&ncInqIdInp, sizeof (ncInqIdInp));
    ncInqIdInp.paramType = NC_DIM_T;
    ncInqIdInp.ncid = ncid1;
    rstrcpy (ncInqIdInp.name, "longitude", MAX_NAME_LEN);
    status = rcNcInqId (conn, &ncInqIdInp, &londimid1);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcInqId error for dim %s of %s", ncInqIdInp.name,
	  ncOpenInp.objPath);
        return status;
    }

    ncInqIdInp.myid = *londimid1;
    status = rcNcInqWithId (conn, &ncInqIdInp, &ncInqWithIdOut);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcInqWithId error for dim %s of %s", ncInqIdInp.name,
          ncOpenInp.objPath);
        return status;
    } else {
	printf ("%s ---- dim length = %lld\n", ncInqIdInp.name, 
	  ncInqWithIdOut->mylong);
	londimlen1 = ncInqWithIdOut->mylong;
	free (ncInqWithIdOut);
	ncInqWithIdOut = NULL;
    }

    rstrcpy (ncInqIdInp.name, "latitude", MAX_NAME_LEN);
    status = rcNcInqId (conn, &ncInqIdInp, &latdimid1);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcInqId error for dim %s of %s", ncInqIdInp.name,
	  ncOpenInp.objPath);
        return status;
    }

    ncInqIdInp.myid = *latdimid1;
    status = rcNcInqWithId (conn, &ncInqIdInp, &ncInqWithIdOut);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcInqWithId error for dim %s of %s", ncInqIdInp.name,
          ncOpenInp.objPath);
        return status;
    } else {
        printf ("%s ---- dim length = %lld\n", ncInqIdInp.name,
          ncInqWithIdOut->mylong);
	latdimlen1 = ncInqWithIdOut->mylong;
        free (ncInqWithIdOut);
	ncInqWithIdOut = NULL;
    }

    /* do the variables */
    lonvarid1 = myInqVar (conn, ncid1, "longitude", &lontype1, &lonndim);
    if (lonvarid1 < 0)  return status;

    latvarid1 = myInqVar (conn, ncid1, "latitude", &lattype1, &latndim);
    if (latvarid1 < 0)  return status;

    tempvarid1 = myInqVar (conn, ncid1, "temperature", &temptype1, &tempndim);
    if (tempvarid1 < 0)  return status;

    presvarid1 = myInqVar (conn, ncid1, "pressure", &prestype1, &presndim);
    if (presvarid1 < 0)  return status;

    /* get the variable values */
    start[0] = 0;
    count[0] = londimlen1;
    stride[0] = 1;
    bzero (&ncGetVarInp, sizeof (ncGetVarInp));
    ncGetVarInp.dataType = lontype1;
    ncGetVarInp.ncid = ncid1;
    ncGetVarInp.varid = lonvarid1;
    ncGetVarInp.ndim = lonndim;
    ncGetVarInp.start = start;
    ncGetVarInp.count = count;
    ncGetVarInp.stride = stride;
   
    status = rcNcGetVarsByType (conn, &ncGetVarInp, &ncGetVarOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcGetVarsByType error for longitude of %s", ncInqIdInp.name,
          ncOpenInp.objPath);
        return status;
    } else {
	printf ("longitude value: \n");
	for (i = 0; i < ncGetVarOut->dataArray->len; i++) {
	    float *fdata = (float *)  ncGetVarOut->dataArray->buf;
	    printf ("  %.2f", fdata[i]);
	}
	printf ("\n");
	freeNcGetVarOut (&ncGetVarOut);
    }

    start[0] = 0;
    count[0] = latdimlen1;
    stride[0] = 1;
    ncGetVarInp.dataType = lattype1;
    ncGetVarInp.varid = latvarid1;
    ncGetVarInp.ndim = latndim;

    status = rcNcGetVarsByType (conn, &ncGetVarInp, &ncGetVarOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcGetVarsByType error for latitude of %s", ncInqIdInp.name,
          ncOpenInp.objPath);
        return status;
    } else {
        printf ("latitude value: \n");
        for (i = 0; i < ncGetVarOut->dataArray->len; i++) {
            float *fdata = (float *)  ncGetVarOut->dataArray->buf;
            printf ("  %.2f", fdata[i]);
        }
        printf ("\n");
	freeNcGetVarOut (&ncGetVarOut);
    }

    if (timedimlen1 > 0) {
        start[0] = 0;
        start[1] = 0;
        start[2] = 0;
        start[3] = 0;
	count[0] = timedimlen1;
	count[1] = levdimlen1;
        count[2] = latdimlen1;
        count[3] = londimlen1;
        stride[0] = 1;
        stride[1] = 1;
        stride[2] = 1;
        stride[3] = 1;
    } else {
        start[0] = 0;
        start[1] = 0;
        count[0] = latdimlen1;
        count[1] = londimlen1;
        stride[0] = 1;
        stride[1] = 1;
    }
    ncGetVarInp.dataType = prestype1;
    ncGetVarInp.varid = presvarid1;
    ncGetVarInp.ndim = presndim;

    status = rcNcGetVarsByType (conn, &ncGetVarInp, &ncGetVarOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcGetVarsByType error for pressure of %s", ncInqIdInp.name,
          ncOpenInp.objPath);
        return status;
    } else {
        printf ("pressure value: \n");
        for (i = 0; i < ncGetVarOut->dataArray->len; i++) {
            float *fdata = (float *)  ncGetVarOut->dataArray->buf;
            printf ("  %.2f", fdata[i]);
        }
        printf ("\n");
	freeNcGetVarOut (&ncGetVarOut);
    }

    ncGetVarInp.dataType = temptype1;
    ncGetVarInp.varid = tempvarid1;
    ncGetVarInp.ndim = tempndim;

    status = rcNcGetVarsByType (conn, &ncGetVarInp, &ncGetVarOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcGetVarsByType error for temperature of %s", ncInqIdInp.name,
          ncOpenInp.objPath);
        return status;
    } else {
        printf ("temperature value: \n");
        for (i = 0; i < ncGetVarOut->dataArray->len; i++) {
            float *fdata = (float *)  ncGetVarOut->dataArray->buf;
            printf ("  %.2f", fdata[i]);
        }
        printf ("\n");
	freeNcGetVarOut (&ncGetVarOut);
    }

    /* close the file */
    bzero (&ncCloseInp, sizeof (ncCloseInp_t));
    ncCloseInp.ncid = ncid1;
    status = rcNcClose (conn, &ncCloseInp);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcClose error for %s", ncOpenInp.objPath);
        return status;
    }

#ifdef LIB_CF
    /* open an nc object */
    bzero (&ncOpenInp, sizeof (ncOpenInp_t));
    rstrcpy (ncOpenInp.objPath, TEST_PATH1, MAX_NAME_LEN);
    ncOpenInp.mode = NC_NOWRITE;

    status = rcNcOpen (conn, &ncOpenInp, &ncid1);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcOpen error for %s", ncOpenInp.objPath);
        return status;
    }

    tempvarid1 = myInqVar (conn, ncid1, "temperature", &temptype1, &tempndim);
    if (tempvarid1 < 0)  return status;

    presvarid1 = myInqVar (conn, ncid1, "pressure", &prestype1, &presndim);
    if (presvarid1 < 0)  return status;

    /* pressure subset */
    bzero (&nccfGetVarInp, sizeof (nccfGetVarInp));
    nccfGetVarInp.ncid = ncid1;
    nccfGetVarInp.varid = presvarid1;
    nccfGetVarInp.latRange[0] = 30.0;
    nccfGetVarInp.latRange[1] = 41.0;
    nccfGetVarInp.lonRange[0] = -120.0;
    nccfGetVarInp.lonRange[1] = -96.0;
    nccfGetVarInp.maxOutArrayLen = 1000;

    status = rcNccfGetVara (conn, &nccfGetVarInp, &nccfGetVarOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNccfGetVara error for %s", ncOpenInp.objPath);
        return status;
    }

    printf (
      "pressure subset: nlat = %d, nlon = %d, dataType = %d, arrayLen = %d\n", 
      nccfGetVarOut->nlat, nccfGetVarOut->nlon, 
      nccfGetVarOut->dataArray->type, nccfGetVarOut->dataArray->len);
    mydata = (float *) nccfGetVarOut->dataArray->buf;
    printf ("pressure values: ");
    for (i = 0; i <  nccfGetVarOut->dataArray->len; i++) {
	printf (" %f", mydata[i]);
    }
    printf ("\n");
    freeNccfGetVarOut (&nccfGetVarOut);

    /* temperature subset */
    nccfGetVarInp.varid = tempvarid1;

    status = rcNccfGetVara (conn, &nccfGetVarInp, &nccfGetVarOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNccfGetVara error for %s", ncOpenInp.objPath);
        return status;
    }

    printf (
      "temperature subset: nlat = %d, nlon = %d, dataType = %d, arrLen = %d\n",
      nccfGetVarOut->nlat, nccfGetVarOut->nlon,
      nccfGetVarOut->dataArray->type, nccfGetVarOut->dataArray->len);
    mydata = (float *) nccfGetVarOut->dataArray->buf;
    printf ("temperature values: ");
    for (i = 0; i <  nccfGetVarOut->dataArray->len; i++) {
        printf (" %f", mydata[i]);
    }
    printf ("\n");
    freeNccfGetVarOut (&nccfGetVarOut);

    /* close the file */
    bzero (&ncCloseInp, sizeof (ncCloseInp_t));
    ncCloseInp.ncid = ncid1;
    status = rcNcClose (conn, &ncCloseInp);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcClose error for %s", ncOpenInp.objPath);
        return status;
    }
#endif
    return 0;
} 

int
nctest1 (rcComm_t *conn, char *ncpath, char *grpPath)
{
    int status;
    ncOpenInp_t ncOpenInp;
    ncCloseInp_t ncCloseInp;
    int ncid = 0;
    int ncid1 = 0;
    int grpNcid = 0;
    ncInqInp_t ncInqInp;
    ncInqOut_t *ncInqOut = NULL;
    ncRegGlobalAttrInp_t ncRegGlobalAttrInp;

    printf ("----- nctest1 for %s ------\n\n\n", ncpath);

    /* open an nc object */
    bzero (&ncOpenInp, sizeof (ncOpenInp_t));
    rstrcpy (ncOpenInp.objPath, ncpath, MAX_NAME_LEN);
    ncOpenInp.mode = NC_NOWRITE;
    addKeyVal (&ncOpenInp.condInput, NO_STAGING_KW, "");

    status = rcNcOpen (conn, &ncOpenInp, &ncid);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcOpen error for %s", ncOpenInp.objPath);
	return status;
    }

    if (grpPath != NULL) {
	if (strcmp (grpPath, "INQ") == 0) {
	     /* inq the group name */
            ncInqGrpsInp_t ncInqGrpsInp;
            ncInqGrpsOut_t *ncInqGrpsOut = NULL;

	    bzero (&ncInqGrpsInp, sizeof (ncInqGrpsInp));
	    ncInqGrpsInp.ncid = ncid;
	    status = rcNcInqGrps (conn, &ncInqGrpsInp, &ncInqGrpsOut);
	    if (status < 0) {
                rodsLogError (LOG_ERROR, status, "rcNcInqGrps error");
                return status;
            }
	    if (ncInqGrpsOut->ngrps <= 0) {
               rodsLogError (LOG_ERROR, status, "rcNcInqGrps ngrps <= 0");
                return status;
            }
	    rstrcpy (ncOpenInp.objPath, ncInqGrpsOut->grpName[0], MAX_NAME_LEN);
	    freeNcInqGrpsOut (&ncInqGrpsOut);
	} else {
            rstrcpy (ncOpenInp.objPath, grpPath, MAX_NAME_LEN);
	}
	printf ("opening group %s\n", ncOpenInp.objPath);
        ncOpenInp.rootNcid = ncid;
        status = rcNcOpenGroup (conn, &ncOpenInp, &grpNcid);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "rcNcOpenGroup error for %s", ncOpenInp.objPath);
            return status;
        }
	ncid1 = grpNcid;
    } else {
	ncid1 = ncid;
    }
    /* do the general inq */
    bzero (&ncInqInp, sizeof (ncInqInp));
    ncInqInp.ncid = ncid1;
    ncInqInp.paramType = NC_ALL_TYPE;
    ncInqInp.flags = NC_ALL_FLAG;

    status = rcNcInq (conn, &ncInqInp, &ncInqOut);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcInq error for %s", ncOpenInp.objPath);
        return status;
    }
    status = dumpNcInqOut (conn, ncpath, ncid1, 20, ncInqOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "dumpNcInqOut error for %s", ncOpenInp.objPath);
        return status;
    }

#if 0
    status = dumpNcInqOutToNcFile (conn, ncid1, ncInqOut, NC_OUTFILE);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "dumpNcInqOutToNcFile error for %s", ncOpenInp.objPath);
        return status;
    }

    status = regNcGlobalAttr (conn, ncOpenInp.objPath, ncInqOut, 0);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "regNcGlobalAttr error for %s", ncOpenInp.objPath);
        return status;
    }
#endif
    bzero (& ncRegGlobalAttrInp, sizeof (ncRegGlobalAttrInp));
    rstrcpy (ncRegGlobalAttrInp.objPath, ncpath, MAX_NAME_LEN);
    status = rcNcRegGlobalAttr (conn, &ncRegGlobalAttrInp);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcRegGlobalAttr error for %s", ncRegGlobalAttrInp.objPath);
        return status;
    }
    freeNcInqOut (&ncInqOut);

    /* close the file */
    bzero (&ncCloseInp, sizeof (ncCloseInp_t));
    ncCloseInp.ncid = ncid1;
    status = rcNcClose (conn, &ncCloseInp);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcClose error for %s", ncOpenInp.objPath);
        return status;
    }
    if (grpNcid == ncid1) {
        /* close the root ncid */
        ncCloseInp.ncid = ncid;
        status = rcNcClose (conn, &ncCloseInp);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "rcNcClose error for %s", ncOpenInp.objPath);
            return status;
        }
    }
    return status;
} 

int
myInqVar (rcComm_t *conn, int ncid, char *name, int *dataType, int *ndim)
{
    int status, i;
    ncInqIdInp_t ncInqIdInp;
    ncInqWithIdOut_t *ncInqWithIdOut = NULL;
    int *myid = NULL;
    int tmpId;

    bzero (&ncInqIdInp, sizeof (ncInqIdInp));
    ncInqIdInp.ncid = ncid;
    ncInqIdInp.paramType = NC_VAR_T;
    rstrcpy (ncInqIdInp.name, name, MAX_NAME_LEN);
    status = rcNcInqId (conn, &ncInqIdInp, &myid);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcInqId error for var %s ", ncInqIdInp.name);
	return status;
    }

    ncInqIdInp.myid = *myid;
    status = rcNcInqWithId (conn, &ncInqIdInp, &ncInqWithIdOut);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcInqWithId error for dim %s", ncInqIdInp.name);
	return status;
    } else {
        printf ("%s ---- type = %d natts = %d, ndim = %d", ncInqIdInp.name,
          ncInqWithIdOut->dataType, ncInqWithIdOut->natts, ncInqWithIdOut->ndim);
        printf ("   dimid:");
        for (i = 0; i <  ncInqWithIdOut->ndim; i++) {
            printf ("    %d",  ncInqWithIdOut->intArray[i]);
        }
        printf ("\n");
	*dataType = ncInqWithIdOut->dataType;
	*ndim = ncInqWithIdOut->ndim;
	if (ncInqWithIdOut->intArray != NULL) free (ncInqWithIdOut->intArray);
	free (ncInqWithIdOut);
	ncInqWithIdOut = NULL;
    }
    tmpId = *myid;
    free (myid);
    return tmpId;
}

int
nctest2 (rcComm_t *conn, char *ncpath)
{
    int status, i;
    ncOpenInp_t ncOpenInp;
    ncCloseInp_t ncCloseInp;
    ncInqInp_t ncInqInp;
    ncInqOut_t *ncInqOut = NULL;
    ncInqOut_t *timeDimNcInqOut, *levelDimNcInqOut, *lonDimNcInqOut, 
      *latDimNcInqOut;
    ncInqOut_t *lonVarNcInqOut, *latVarNcInqOut, *tempVarNcInqOut, 
      *presVarNcInqOut;
    int ncid;
    ncGetVarInp_t ncGetVarInp;
    ncGetVarOut_t *ncGetVarOut = NULL;
    rodsLong_t start[NC_MAX_DIMS], stride[NC_MAX_DIMS], count[NC_MAX_DIMS];
#ifdef LIB_CF
    nccfGetVarInp_t nccfGetVarInp;
    nccfGetVarOut_t *nccfGetVarOut = NULL;
    float *mydata;
#endif
    printf ("----- nctest2 for %s ------\n\n\n", ncpath);

    /* open an nc object */
    bzero (&ncOpenInp, sizeof (ncOpenInp_t));
    rstrcpy (ncOpenInp.objPath, ncpath, MAX_NAME_LEN);
    ncOpenInp.mode = NC_NOWRITE;

    status = rcNcOpen (conn, &ncOpenInp, &ncid);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcOpen error for %s", ncOpenInp.objPath);
        return status;
    }

    /* inq the time dim length */

    bzero (&ncInqInp, sizeof (ncInqInp));
    ncInqInp.ncid = ncid;
    ncInqInp.paramType = NC_DIM_TYPE;
    rstrcpy (ncInqInp.name, "time", MAX_NAME_LEN);
    ncInqInp.flags = 0;

    status = rcNcInq (conn, &ncInqInp, &ncInqOut);
    if (status < 0) {
	timeDimNcInqOut = NULL;
    } else {
	timeDimNcInqOut = ncInqOut;
        printf ("%s ---- dim length = %lld\n", ncInqInp.name,
          timeDimNcInqOut->dim[0].arrayLen);

    }

   /* inq the level length */
    rstrcpy (ncInqInp.name, "level", MAX_NAME_LEN);
    ncInqInp.flags = 0;

    status = rcNcInq (conn, &ncInqInp, &ncInqOut);
    if (status < 0) {
	levelDimNcInqOut = NULL;
    } else {
        levelDimNcInqOut = ncInqOut;
        printf ("%s ---- dim length = %lld\n", ncInqInp.name,
          levelDimNcInqOut->dim[0].arrayLen);
    }

   /* inq the longitude length */
    rstrcpy (ncInqInp.name, "longitude", MAX_NAME_LEN);
    ncInqInp.flags = 0;

    status = rcNcInq (conn, &ncInqInp, &ncInqOut);
    if (status < 0) {
	lonDimNcInqOut = NULL;
    } else {
        lonDimNcInqOut = ncInqOut;
        printf ("%s ---- dim length = %lld\n", ncInqInp.name,
          lonDimNcInqOut->dim[0].arrayLen);
    }

   /* inq the latitude length */
    rstrcpy (ncInqInp.name, "latitude", MAX_NAME_LEN);
    ncInqInp.flags = 0;

    status = rcNcInq (conn, &ncInqInp, &ncInqOut);
    if (status < 0) {
	latDimNcInqOut = NULL;
    } else {
        latDimNcInqOut = ncInqOut;
        printf ("%s ---- dim length = %lld\n", ncInqInp.name,
          latDimNcInqOut->dim[0].arrayLen);
    }

    /* inq the longitude var */

    bzero (&ncInqInp, sizeof (ncInqInp));
    ncInqInp.ncid = ncid;
    ncInqInp.paramType = NC_VAR_TYPE;
    rstrcpy (ncInqInp.name, "longitude", MAX_NAME_LEN);
    ncInqInp.flags = 0;

    status = rcNcInq (conn, &ncInqInp, &ncInqOut);
    if (status < 0) {
        lonVarNcInqOut = NULL;
    } else {
        lonVarNcInqOut = ncInqOut;
	printVarOut (lonVarNcInqOut->var);
    }

   /* inq the latitude var */
    rstrcpy (ncInqInp.name, "latitude", MAX_NAME_LEN);
    ncInqInp.flags = 0;

    status = rcNcInq (conn, &ncInqInp, &ncInqOut);
    if (status < 0) {
        latVarNcInqOut = NULL;
    } else {
        latVarNcInqOut = ncInqOut;
	printVarOut (latVarNcInqOut->var);
    }

   /* inq the temperature var */
    rstrcpy (ncInqInp.name, "temperature", MAX_NAME_LEN);
    ncInqInp.flags = 0;

    status = rcNcInq (conn, &ncInqInp, &ncInqOut);
    if (status < 0) {
        tempVarNcInqOut = NULL;
    } else {
        tempVarNcInqOut = ncInqOut;
        printVarOut (tempVarNcInqOut->var);
    }

   /* inq the pressure var */
    rstrcpy (ncInqInp.name, "pressure", MAX_NAME_LEN);
    ncInqInp.flags = 0;

    status = rcNcInq (conn, &ncInqInp, &ncInqOut);
    if (status < 0) {
        presVarNcInqOut = NULL;
    } else {
        presVarNcInqOut = ncInqOut;
        printVarOut (presVarNcInqOut->var);
    }

    /* get the variable values */
    start[0] = 0;
    count[0] = lonDimNcInqOut->dim[0].arrayLen;
    stride[0] = 1;
    bzero (&ncGetVarInp, sizeof (ncGetVarInp));
    ncGetVarInp.dataType = lonVarNcInqOut->var->dataType;
    ncGetVarInp.ncid = ncid;
    ncGetVarInp.varid = lonVarNcInqOut->var->id;
    ncGetVarInp.ndim = lonVarNcInqOut->var->nvdims;
    ncGetVarInp.start = start;
    ncGetVarInp.count = count;
    ncGetVarInp.stride = stride;

    status = rcNcGetVarsByType (conn, &ncGetVarInp, &ncGetVarOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcGetVarsByType error for longitude of %s", 
	  lonVarNcInqOut->var->name, ncOpenInp.objPath);
        return status;
    } else {
        printf ("longitude value: \n");
        for (i = 0; i < ncGetVarOut->dataArray->len; i++) {
            float *fdata = (float *)  ncGetVarOut->dataArray->buf;
            printf ("  %.2f", fdata[i]);
        }
        printf ("\n");
        freeNcGetVarOut (&ncGetVarOut);
    }

    /* variable latitude */
    start[0] = 0;
    count[0] = latDimNcInqOut->dim[0].arrayLen;
    stride[0] = 1;
    ncGetVarInp.dataType = latVarNcInqOut->var->dataType;
    ncGetVarInp.varid = latVarNcInqOut->var->id;
    ncGetVarInp.ndim = latVarNcInqOut->var->nvdims;

    status = rcNcGetVarsByType (conn, &ncGetVarInp, &ncGetVarOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcGetVarsByType error for latitude of %s", 
	  latVarNcInqOut->var->name, ncOpenInp.objPath);
        return status;
    } else {
        printf ("latitude value: \n");
        for (i = 0; i < ncGetVarOut->dataArray->len; i++) {
            float *fdata = (float *)  ncGetVarOut->dataArray->buf;
            printf ("  %.2f", fdata[i]);
        }
        printf ("\n");
        freeNcGetVarOut (&ncGetVarOut);
    }

    if (timeDimNcInqOut != NULL) {
        start[0] = 0;
        start[1] = 0;
        start[2] = 0;
        start[3] = 0;
        count[0] = timeDimNcInqOut->dim->arrayLen;
        count[1] = levelDimNcInqOut->dim->arrayLen;
        count[2] = latDimNcInqOut->dim->arrayLen;
        count[3] = lonDimNcInqOut->dim->arrayLen;
        stride[0] = 1;
        stride[1] = 1;
        stride[2] = 1;
        stride[3] = 1;
    } else {
        start[0] = 0;
        start[1] = 0;
        count[0] = latDimNcInqOut->dim->arrayLen;
        count[1] = lonDimNcInqOut->dim->arrayLen;
        stride[0] = 1;
        stride[1] = 1;
    }
    ncGetVarInp.dataType = presVarNcInqOut->var->dataType;
    ncGetVarInp.varid = presVarNcInqOut->var->id;
    ncGetVarInp.ndim = presVarNcInqOut->var->nvdims;

    status = rcNcGetVarsByType (conn, &ncGetVarInp, &ncGetVarOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcGetVarsByType error for pressure of %s", 
	  presVarNcInqOut->var->name, ncOpenInp.objPath);
        return status;
    } else {
        printf ("pressure value: \n");
        for (i = 0; i < ncGetVarOut->dataArray->len; i++) {
            float *fdata = (float *)  ncGetVarOut->dataArray->buf;
            printf ("  %.2f", fdata[i]);
        }
        printf ("\n");
        freeNcGetVarOut (&ncGetVarOut);
    }

    /* temperature variable */
    ncGetVarInp.dataType = tempVarNcInqOut->var->dataType;
    ncGetVarInp.varid = tempVarNcInqOut->var->id;
    ncGetVarInp.ndim = tempVarNcInqOut->var->nvdims;

    status = rcNcGetVarsByType (conn, &ncGetVarInp, &ncGetVarOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcGetVarsByType error for temperature of %s", 
	  tempVarNcInqOut->var->name, ncOpenInp.objPath);
        return status;
    } else {
        printf ("temperature value: \n");
        for (i = 0; i < ncGetVarOut->dataArray->len; i++) {
            float *fdata = (float *)  ncGetVarOut->dataArray->buf;
            printf ("  %.2f", fdata[i]);
        }
        printf ("\n");
        freeNcGetVarOut (&ncGetVarOut);
    }

    freeNcInqOut (&timeDimNcInqOut);
    freeNcInqOut (&levelDimNcInqOut);
    freeNcInqOut (&lonDimNcInqOut);
    freeNcInqOut (&latDimNcInqOut);
    freeNcInqOut (&lonVarNcInqOut);
    freeNcInqOut (&latVarNcInqOut);

#ifdef LIB_CF
    /* open an nc object */
    bzero (&ncOpenInp, sizeof (ncOpenInp_t));
    rstrcpy (ncOpenInp.objPath, TEST_PATH1, MAX_NAME_LEN);
    ncOpenInp.mode = NC_NOWRITE;

    status = rcNcOpen (conn, &ncOpenInp, &ncid);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcOpen error for %s", ncOpenInp.objPath);
        return status;
    }

#if 0
    tempvarid1 = myInqVar (conn, ncid1, "temperature", &temptype1, &tempndim);
    if (tempvarid1 < 0)  return status;

    presvarid1 = myInqVar (conn, ncid1, "pressure", &prestype1, &presndim);
    if (presvarid1 < 0)  return status;
#endif

    /* pressure subset */
    bzero (&nccfGetVarInp, sizeof (nccfGetVarInp));
    nccfGetVarInp.ncid = ncid;
    nccfGetVarInp.varid = presVarNcInqOut->var->id;
    nccfGetVarInp.latRange[0] = 30.0;
    nccfGetVarInp.latRange[1] = 41.0;
    nccfGetVarInp.lonRange[0] = -120.0;
    nccfGetVarInp.lonRange[1] = -96.0;
    nccfGetVarInp.maxOutArrayLen = 1000;

    status = rcNccfGetVara (conn, &nccfGetVarInp, &nccfGetVarOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNccfGetVara error for %s", ncOpenInp.objPath);
        return status;
    }

    printf (
      "pressure subset: nlat = %d, nlon = %d, dataType = %d, arrayLen = %d\n",
      nccfGetVarOut->nlat, nccfGetVarOut->nlon,
      nccfGetVarOut->dataArray->type, nccfGetVarOut->dataArray->len);
    mydata = (float *) nccfGetVarOut->dataArray->buf;
    printf ("pressure values: ");
    for (i = 0; i <  nccfGetVarOut->dataArray->len; i++) {
        printf (" %f", mydata[i]);
    }
    printf ("\n");
    freeNccfGetVarOut (&nccfGetVarOut);

    /* temperature subset */
    nccfGetVarInp.varid = tempVarNcInqOut->var->id;

    status = rcNccfGetVara (conn, &nccfGetVarInp, &nccfGetVarOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNccfGetVara error for %s", ncOpenInp.objPath);
        return status;
    }

    printf (
      "temperature subset: nlat = %d, nlon = %d, dataType = %d, arrLen = %d\n",
      nccfGetVarOut->nlat, nccfGetVarOut->nlon,
      nccfGetVarOut->dataArray->type, nccfGetVarOut->dataArray->len);
    mydata = (float *) nccfGetVarOut->dataArray->buf;
    printf ("temperature values: ");
    for (i = 0; i <  nccfGetVarOut->dataArray->len; i++) {
        printf (" %f", mydata[i]);
    }
    printf ("\n");
    freeNccfGetVarOut (&nccfGetVarOut);
#endif

    /* close the file */
    bzero (&ncCloseInp, sizeof (ncCloseInp_t));
    ncCloseInp.ncid = ncid;
    status = rcNcClose (conn, &ncCloseInp);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "rcNcClose error for %s", ncOpenInp.objPath);
        return status;
    }
    freeNcInqOut (&tempVarNcInqOut);
    freeNcInqOut (&presVarNcInqOut);

    return 0;
}

int
printVarOut (ncGenVarOut_t *var)
{
    int i;

    printf ("%s ---- type = %d natts = %d, ndim = %d\n", var->name,
      var->dataType, var->natts, var->nvdims);
    printf ("   dimid:");
    for (i = 0; i < var->nvdims; i++) {
	printf ("    %d",  var->dimId[i]);
    }
    printf ("\n");
    printf ("   attribute:\n");
    for (i = 0; i < var->natts; i++) {
	char tempStr[NAME_LEN];
	void *bufPtr = (char *) var->att[i].value.dataArray->buf;
        ncValueToStr (var->att[i].dataType, &bufPtr, tempStr);
        printf ("    %s = %s\n",  var->att[i].name, tempStr);
    }
    printf ("\n");

    return 0;
}

int
regNcGlobalAttr (rcComm_t *conn, char *objPath, ncInqOut_t *ncInqOut,
int adminFlag)
{
    int i, status;
    void *valuePtr;
    modAVUMetadataInp_t modAVUMetadataInp;
    char tempStr[NAME_LEN];

    bzero (&modAVUMetadataInp, sizeof (modAVUMetadataInp));
    if (!adminFlag) {
        modAVUMetadataInp.arg0 = "add"; 
    } else {
        modAVUMetadataInp.arg0 = "adda";    /* admin mod */
    }
    modAVUMetadataInp.arg1 = "-d";
    modAVUMetadataInp.arg2 = objPath;
    modAVUMetadataInp.arg5 = "";		/* unit */

    /* global attrbutes */
    for (i = 0; i < ncInqOut->ngatts; i++) {
        valuePtr = ncInqOut->gatt[i].value.dataArray->buf;
        modAVUMetadataInp.arg3 = ncInqOut->gatt[i].name;
        valuePtr = ncInqOut->gatt[i].value.dataArray->buf;
        if (ncInqOut->gatt[i].dataType == NC_CHAR) {
            /* assume it is a string */
            modAVUMetadataInp.arg4 = (char *) valuePtr;
        } else {
            ncValueToStr (ncInqOut->gatt[i].dataType, &valuePtr, 
              tempStr);
            modAVUMetadataInp.arg4 = tempStr;
        }
        status = rcModAVUMetadata (conn, &modAVUMetadataInp);
#if 0
        if (status < 0) {
            if (status == CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME &&
              strcmp (modAVUMetadataInp.arg0, "add") == 0) {
                modAVUMetadataInp.arg0 = "mod";
                status = rcModAVUMetadata (conn, &modAVUMetadataInp);
                modAVUMetadataInp.arg0 = "add";
                if (status < 0) {
                    rodsLogError (LOG_ERROR, status,
                      "regNcGlobalAttr: rcModAVUMetadata err for %s, attr = %s",
                      objPath, modAVUMetadataInp.arg3);
                    return status;
		}
            } else {
                rodsLogError (LOG_ERROR, status,
                  "regNcGlobalAttr: rcModAVUMetadata error for %s, attr = %s", 
                  objPath, modAVUMetadataInp.arg3);
                return status;
	    }
        }
#else
        if (status < 0) {
            if (status == CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME) {
                status = 0;
            } else {
                rodsLogError (LOG_ERROR, status,
                  "regNcGlobalAttr: rcModAVUMetadata error for %s, attr = %s",
                  objPath, modAVUMetadataInp.arg3);
                return status;
            }
        }
#endif
    }
    return status;
}

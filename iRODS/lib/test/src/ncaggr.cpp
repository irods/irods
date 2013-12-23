/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* nctest.c - test the iRODS NETCDF api */

#include "rodsClient.hpp"

#define TEST_PATH1 "./ncdata/HFRadarCurrent"

#define TEST_OBJ_PATH "/oneZone/home/rods/ncdata/HFRadarCurrent/SFCurrent1_1.nc"
#define TEST_OBJ_COLL "/oneZone/home/rods/ncdata/HFRadarCurrent"
#define ARCH_COLLECTION "/oneZone/home/rods/agg/HFRADAR, Alaska - North Slope, 6km Resolution, Hourly RTV"
#define ARCH_SRC_OBJ_PATH "/oneZone/home/rods/tds/HF RADAR, Alaska, North Slope/HFRADAR, Alaska - North Slope, 6km Resolution, Hourly RTV"
#define RESC "demoResc1"
int
genNcAggInfo( char *testPath, ncAggInfo_t *ncAggInfo );
int
setNcAggElement( int ncid, ncInqOut_t *ncInqOut, ncAggElement_t *ncAggElement );
unsigned int
getIntVar( int ncid, int varid, int dataType, rodsLong_t inx );
int
addNcAggElement( ncAggElement_t *ncAggElement, ncAggInfo_t *ncAggInfo );
int
testGetAggElement( rcComm_t *conn, char *objPath );
int
testGetAggInfo( rcComm_t *conn, char *collPath );
int
testAggNcOpr( rcComm_t *conn, char *collPath );
int
testArch( rcComm_t *conn, char *objPath, char *archCollection );
int
main( int argc, char **argv ) {
    char *testPath;
    int status;
    rcComm_t *conn;
    rodsEnv myRodsEnv;
    rErrMsg_t errMsg;

    if ( argc <= 1 ) {
        testPath = TEST_PATH1;
    }
    else {
        testPath = argv[1];
    }

    status = getRodsEnv( &myRodsEnv );

    if ( status < 0 ) {
        fprintf( stderr, "getRodsEnv error, status = %d\n", status );
        exit( 1 );
    }


    conn = rcConnect( myRodsEnv.rodsHost, myRodsEnv.rodsPort,
                      myRodsEnv.rodsUserName, myRodsEnv.rodsZone, 0, &errMsg );

    if ( conn == NULL ) {
        fprintf( stderr, "rcConnect error\n" );
        exit( 1 );
    }
    status = clientLogin( conn );
    if ( status != 0 ) {
        fprintf( stderr, "clientLogin error\n" );
        rcDisconnect( conn );
        exit( 2 );
    }
    status = testGetAggElement( conn, TEST_OBJ_PATH );
    if ( status < 0 ) {
        fprintf( stderr, "testGetAggElement of %s failed. status = %d\n",
                 TEST_OBJ_PATH, status );
        exit( 1 );
    }

    status = testGetAggInfo( conn, TEST_OBJ_COLL );
    if ( status < 0 ) {
        fprintf( stderr, "testGetAggInfo of %s failed. status = %d\n",
                 TEST_OBJ_COLL, status );
        exit( 1 );
    }

    status = testAggNcOpr( conn, TEST_OBJ_COLL );
    if ( status < 0 ) {
        fprintf( stderr, "testAggNcOpr of %s failed. status = %d\n",
                 TEST_OBJ_COLL, status );
        exit( 1 );
    }

    status = testArch( conn, ARCH_SRC_OBJ_PATH, ARCH_COLLECTION );
    if ( status < 0 ) {
        fprintf( stderr, "testArch of %s in %s failed. status = %d\n",
                 ARCH_SRC_OBJ_PATH, ARCH_COLLECTION, status );
        exit( 1 );
    }

    exit( 0 );
}

int
testGetAggElement( rcComm_t *conn, char *objPath ) {
    ncOpenInp_t ncOpenInp;

    ncAggElement_t *ncAggElement = NULL;
    int status;

    bzero( &ncOpenInp, sizeof( ncOpenInp ) );
    rstrcpy( ncOpenInp.objPath, objPath, MAX_NAME_LEN );
    status = rcNcGetAggElement( conn, &ncOpenInp, &ncAggElement );

    return status;
}

int
testGetAggInfo( rcComm_t *conn, char *collPath ) {
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

    bzero( &ncOpenInp, sizeof( ncOpenInp ) );
    rstrcpy( ncOpenInp.objPath, collPath, MAX_NAME_LEN );
    ncOpenInp.mode = NC_WRITE;
    addKeyVal( &ncOpenInp.condInput, DEST_RESC_NAME_KW, RESC );
    status = rcNcGetAggInfo( conn, &ncOpenInp, &ncAggInfo );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "rcNcGetAggInfo error for %s", ncOpenInp.objPath );
        return status;
    }
    ncOpenInp.mode = NC_NOWRITE;
    status = rcNcOpen( conn, &ncOpenInp, &ncid );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "rcNcOpen error for %s", ncOpenInp.objPath );
        return status;
    }
    bzero( &ncInqInp, sizeof( ncInqInp ) );
    ncInqInp.ncid = ncid;
    ncInqInp.paramType = NC_ALL_TYPE;
    ncInqInp.flags = NC_ALL_FLAG;

    status = rcNcInq( conn, &ncInqInp, &ncInqOut );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "rcNcInq error for %s", ncOpenInp.objPath );
        return status;
    }
    if ( status < 0 ) {
        return status;
    }

    /* rcNcGetVarsByType - do the last var and 3 */
    var =  &ncInqOut->var[ncInqOut->nvars - 1];

    for ( i = 0; i < var->nvdims; i++ ) {
        int dimId = var->dimId[i];
        if ( strcasecmp( ncInqOut->dim[dimId].name, "time" ) == 0 ) {
            start[i] = 699;
            count[i] = 4;
        }
        else {
            start[i] = 0;
            count[i] = ncInqOut->dim[dimId].arrayLen;
        }
        stride[i] = 1;
    }
    bzero( &ncGetVarInp, sizeof( ncGetVarInp ) );
    ncGetVarInp.dataType = var->dataType;
    ncGetVarInp.ncid = ncid;
    ncGetVarInp.varid = var->id;
    ncGetVarInp.ndim = var->nvdims;
    ncGetVarInp.start = start;
    ncGetVarInp.count = count;
    ncGetVarInp.stride = stride;

    status = rcNcGetVarsByType( conn, &ncGetVarInp, &ncGetVarOut );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "rcNcGetVarsByType error for %s of %s", var->name,
                      ncOpenInp.objPath );
        return status;
    }
    bzero( &ncCloseInp, sizeof( ncCloseInp_t ) );
    ncCloseInp.ncid = ncid;
    rcNcClose( conn, &ncCloseInp );

    return status;
}

int
testAggNcOpr( rcComm_t *conn, char *collPath ) {
    ncOpenInp_t ncOpenInp;
    int ncid;
    int status;
    ncCloseInp_t ncCloseInp;

    bzero( &ncOpenInp, sizeof( ncOpenInp ) );
    rstrcpy( ncOpenInp.objPath, collPath, MAX_NAME_LEN );
    ncOpenInp.mode = NC_NOWRITE | NC_NETCDF4;
    addKeyVal( &ncOpenInp.condInput, DEST_RESC_NAME_KW, "hpResc1" );
    status = rcNcOpen( conn, &ncOpenInp, &ncid );
    if ( status < 0 ) {
        return status;
    }
    bzero( &ncCloseInp, sizeof( ncCloseInp_t ) );
    ncCloseInp.ncid = ncid;
    rcNcClose( conn, &ncCloseInp );

    return status;
}

int
testArch( rcComm_t *conn, char *objPath, char *archCollection ) {
    ncArchTimeSeriesInp_t ncArchTimeSeriesInp;
    int status;

    bzero( &ncArchTimeSeriesInp, sizeof( ncArchTimeSeriesInp ) );
    rstrcpy( ncArchTimeSeriesInp.objPath, objPath, MAX_NAME_LEN );
    rstrcpy( ncArchTimeSeriesInp.aggCollection, archCollection, MAX_NAME_LEN );

    status = rcNcArchTimeSeries( conn, &ncArchTimeSeriesInp );

    return status;
}

/*** Copyright (c) 2010 Data Intensive Cyberinfrastructure Foundation. All rights reserved.    ***
 *** For full copyright notice please refer to files in the COPYRIGHT directory                ***/
/* Written by Jean-Yves Nief of CCIN2P3 and copyright assigned to Data Intensive Cyberinfrastructure Foundation */

#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "scanUtil.h"
#include "miscUtil.h"
#include "rcGlobalExtern.h"
#include "irods_hierarchy_parser.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
using namespace boost::filesystem;

int
scanObj( rcComm_t *conn,
         rodsArguments_t *myRodsArgs,
         rodsPathInp_t *rodsPathInp,
         const char * hostname ) {

    if ( rodsPathInp->numSrc != 1 ) {
        rodsLog( LOG_ERROR, "scanObj: gave %i input source path, should give one and only one", rodsPathInp->numSrc );
        return USER_INPUT_PATH_ERR;
    }

    char * inpPathO = rodsPathInp->srcPath[0].outPath;
    if ( rodsPathInp->srcPath[0].objType == LOCAL_FILE_T ||
            rodsPathInp->srcPath[0].objType == LOCAL_DIR_T ) {
        path p( inpPathO );
        if ( !exists( p ) ) {
            rodsLog( LOG_ERROR, "scanObj: %s does not exist", inpPathO );
            return USER_INPUT_PATH_ERR;
        }
        /* don't do anything if it is symlink */
        if ( is_symlink( p ) ) {
            return 0;
        }

        char inpPath[ LONG_NAME_LEN ];
        snprintf( inpPath, sizeof( inpPath ), "%s", inpPathO );
        // if it is part of a mounted collection, abort
        if ( is_directory( p ) ) {
            if ( int status = checkIsMount( conn, inpPath ) ) {
                rodsLog( LOG_ERROR, "The directory %s or one of its "
                         "subdirectories to be scanned is declared as being "
                         "used for a mounted collection: abort!", inpPath );
                return status;
            }
        }
        return scanObjDir( conn, myRodsArgs, inpPath, hostname );
    }
    else if ( rodsPathInp->srcPath[0].objType == UNKNOWN_OBJ_T ||
              rodsPathInp->srcPath[0].objType == COLL_OBJ_T ) {
        return scanObjCol( conn, myRodsArgs, inpPathO );
    }
    else {
        rodsLog( LOG_ERROR, "scanObj: %s does not exist", inpPathO );
        return USER_INPUT_PATH_ERR;
    }
}

int
scanObjDir( rcComm_t *conn, rodsArguments_t *myRodsArgs, const char *inpPath, const char *hostname ) {
    int status = 0;
    char fullPath[LONG_NAME_LEN] = "\0";

    /* check if it is a directory */
    path srcDirPath( inpPath );
    if ( is_symlink( srcDirPath ) ) {
        /* don't do anything if it is symlink */
        return 0;
    }
    else if ( !is_directory( srcDirPath ) ) {
        status = chkObjExist( conn, inpPath, hostname );
        return status;
    }

    // This variable will contain either the 0, or the last error
    // encountered as the loop below iterates through all of the
    // entries in the physical directory.
    int return_status = 0;
    directory_iterator end_itr; // default construction yields past-the-end
    for ( directory_iterator itr( srcDirPath ); itr != end_itr; ++itr ) {
        path cp = itr->path();
        snprintf( fullPath, LONG_NAME_LEN, "%s",
                  cp.c_str() );
        if ( is_symlink( cp ) ) {
            /* don't do anything if it is symlink */
            continue;
        }
        else if ( is_directory( cp ) ) {
            if ( myRodsArgs->recursive == True ) {
                status = scanObjDir( conn, myRodsArgs, fullPath, hostname );
            }
        }
        else {
            status = chkObjExist( conn, fullPath, hostname );
        }
        if (status != 0) {
            return_status = status;
        }
    }
    return return_status;
}

int
scanObjCol( rcComm_t *conn, rodsArguments_t *myRodsArgs, const char *inpPath ) {
    int isColl, status;
    genQueryInp_t genQueryInp1, genQueryInp2;
    genQueryOut_t *genQueryOut1 = NULL, *genQueryOut2 = NULL;
    char condStr1[MAX_NAME_LEN], condStr2[MAX_NAME_LEN];
    char firstPart[MAX_NAME_LEN] = "";

    /* check if inpPath is a file or a collection */
    const char *lastPart = strrchr( inpPath, '/' ) + 1;
    strncpy( firstPart, inpPath, strlen( inpPath ) - strlen( lastPart ) - 1 );
    memset( &genQueryInp1, 0, sizeof( genQueryInp1 ) );
    addInxIval( &genQueryInp1.selectInp, COL_COLL_ID, 1 );
    genQueryInp1.maxRows = MAX_SQL_ROWS;

    snprintf( condStr1, MAX_NAME_LEN, "='%s'", firstPart );
    addInxVal( &genQueryInp1.sqlCondInp, COL_COLL_NAME, condStr1 );
    snprintf( condStr1, MAX_NAME_LEN, "='%s'", lastPart );
    addInxVal( &genQueryInp1.sqlCondInp, COL_DATA_NAME, condStr1 );

    status =  rcGenQuery( conn, &genQueryInp1, &genQueryOut1 );
    if ( status == CAT_NO_ROWS_FOUND ) {
        isColl = 1;
    }
    else {
        isColl = 0;
    }

    /* for each files check if the physical file associated to it exists on the
       physical resource */
    memset( &genQueryInp2, 0, sizeof( genQueryInp2 ) );
    addInxIval( &genQueryInp2.selectInp, COL_D_DATA_PATH, 1 );
    addInxIval( &genQueryInp2.selectInp, COL_DATA_SIZE, 1 );
    addInxIval( &genQueryInp2.selectInp, COL_R_LOC, 1 );
    addInxIval( &genQueryInp2.selectInp, COL_R_ZONE_NAME, 1 );
    addInxIval( &genQueryInp2.selectInp, COL_DATA_NAME, 1 );
    addInxIval( &genQueryInp2.selectInp, COL_COLL_NAME, 1 );
    addInxIval( &genQueryInp2.selectInp, COL_D_RESC_HIER, 1 );
    genQueryInp2.maxRows = MAX_SQL_ROWS;

    if ( isColl ) {
        if ( myRodsArgs->recursive == True ) {
            snprintf( condStr2, MAX_NAME_LEN, "like '%s%s'", inpPath, "%" );
        }
        else {
            snprintf( condStr2, MAX_NAME_LEN, "='%s'", inpPath );
        }
        addInxVal( &genQueryInp2.sqlCondInp, COL_COLL_NAME, condStr2 );
    }
    else {
        snprintf( condStr2, MAX_NAME_LEN, "='%s'", firstPart );
        addInxVal( &genQueryInp2.sqlCondInp, COL_COLL_NAME, condStr2 );
        snprintf( condStr2, MAX_NAME_LEN, "='%s'", lastPart );
        addInxVal( &genQueryInp2.sqlCondInp, COL_DATA_NAME, condStr2 );
    }

    /* check if the physical file corresponding to the iRODS object does exist */
    status =  rcGenQuery( conn, &genQueryInp2, &genQueryOut2 );
    if ( status == 0 ) {
        status = statPhysFile( conn, genQueryOut2 );
    }
    else {
        printf( "Could not find the requested data object or collection in iRODS.\n" );
    }
    while ( status == 0 && genQueryOut2->continueInx > 0 ) {
        genQueryInp2.continueInx = genQueryOut2->continueInx;
        status = rcGenQuery( conn, &genQueryInp2, &genQueryOut2 );
        if ( status == 0 ) {
            status = statPhysFile( conn, genQueryOut2 );
        }
    }

    freeGenQueryOut( &genQueryOut1 );
    freeGenQueryOut( &genQueryOut2 );

    return status;

}

int
statPhysFile( rcComm_t *conn, genQueryOut_t *genQueryOut2 ) {
    int rc = 0;

    for ( int i = 0; i < genQueryOut2->rowCnt; i++ ) {
        sqlResult_t *dataPathStruct = getSqlResultByInx( genQueryOut2, COL_D_DATA_PATH );
        sqlResult_t *dataSizeStruct = getSqlResultByInx( genQueryOut2, COL_DATA_SIZE );
        sqlResult_t *locStruct = getSqlResultByInx( genQueryOut2, COL_R_LOC );
        sqlResult_t *zoneStruct = getSqlResultByInx( genQueryOut2, COL_R_ZONE_NAME );
        sqlResult_t *dataNameStruct = getSqlResultByInx( genQueryOut2, COL_DATA_NAME );
        sqlResult_t *collNameStruct = getSqlResultByInx( genQueryOut2, COL_COLL_NAME );
        sqlResult_t *rescHierStruct = getSqlResultByInx( genQueryOut2, COL_D_RESC_HIER );
        if ( dataPathStruct == NULL || dataSizeStruct == NULL || locStruct == NULL ||
                zoneStruct == NULL || dataNameStruct == NULL || collNameStruct == NULL ||
                rescHierStruct == NULL ) {
            printf( "getSqlResultByInx returned null in statPhysFile." );
            return -1;
        }
        char *dataPath = &dataPathStruct->value[dataPathStruct->len * i];
        char *loc = &locStruct->value[locStruct->len * i];
        char *zone = &zoneStruct->value[zoneStruct->len * i];
        char *dataName = &dataNameStruct->value[dataNameStruct->len * i];
        char *collName = &collNameStruct->value[collNameStruct->len * i];
        char *rescHier = &rescHierStruct->value[rescHierStruct->len * i];

        /* check if the physical file does exist on the filesystem */
        fileStatInp_t fileStatInp;
        rstrcpy( fileStatInp.addr.hostAddr, loc, sizeof( fileStatInp.addr.hostAddr ) );
        rstrcpy( fileStatInp.addr.zoneName, zone, sizeof( fileStatInp.addr.zoneName ) );
        rstrcpy( fileStatInp.fileName, dataPath, sizeof( fileStatInp.fileName ) );
        rstrcpy( fileStatInp.rescHier, rescHier, sizeof( fileStatInp.rescHier ) );
        snprintf( fileStatInp.objPath, sizeof( fileStatInp.objPath ), "%s/%s", collName, dataName );
        rodsStat_t *fileStatOut;
        int status = rcFileStat( conn, &fileStatInp, &fileStatOut );
        if ( SYS_NO_API_PRIV == status ) {
            printf( "User must be a rodsadmin to scan iRODS data objects.\n" );
            rc = status;
        }
        else if ( status < 0 ) {
            printf( "Physical file %s on server %s is missing, corresponding to "
                    "iRODS object %s/%s\n", dataPath, loc, collName, dataName );
            rc = status;
        }

    }

    return rc;

}

int
store_single_attribute_genquery_to_vector(
    rcComm_t* conn,
    genQueryInp_t& genQueryInp,
    std::vector<std::string>& results ) {
    genQueryOut_t *genQueryOut = NULL;
    const int status = rcGenQuery( conn, &genQueryInp, &genQueryOut );
    if( status < 0 ) {
        freeGenQueryOut( &genQueryOut );
        return status;
    }

    for( int i = 0; i < genQueryOut->rowCnt; i++ ) {
        results.push_back( genQueryOut->sqlResult[0].value + i * genQueryOut->sqlResult[0].len );
    }
    freeGenQueryOut( &genQueryOut );
    return status;
}

int
get_hierarchies_from_data_path(
    rcComm_t* conn,
    std::vector<std::string>& hierarchies,
    const char* data_path ) {
    genQueryInp_t genQueryInp;
    char condStr[MAX_NAME_LEN];

    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    addInxIval( &genQueryInp.selectInp, COL_D_RESC_HIER, 1 );
    genQueryInp.maxRows = MAX_SQL_ROWS;
    snprintf( condStr, MAX_NAME_LEN, "='%s'", data_path );
    addInxVal( &genQueryInp.sqlCondInp, COL_D_DATA_PATH, condStr );

    const int status = store_single_attribute_genquery_to_vector( conn, genQueryInp, hierarchies );
    clearGenQueryInp( &genQueryInp );
    return status;
}

int
get_resource_names_for_host(
    rcComm_t* conn,
    std::vector<std::string>& resource_names,
    const char* hostname ) {
    genQueryInp_t genQueryInp;
    char condStr[MAX_NAME_LEN];

    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    addInxIval( &genQueryInp.selectInp, COL_R_RESC_NAME, 1 );
    genQueryInp.maxRows = MAX_SQL_ROWS;
    snprintf( condStr, MAX_NAME_LEN, "like '%s%%' || ='%s'", hostname, hostname );
    addInxVal( &genQueryInp.sqlCondInp, COL_R_LOC, condStr );

    const int status = store_single_attribute_genquery_to_vector( conn, genQueryInp, resource_names );
    clearGenQueryInp( &genQueryInp );
    return status;
}

int
chkObjExist(
    rcComm_t *conn,
    const char *inpPath,
    const char *hostname ) {
    const char* not_registered_message = "%s is not registered in iRODS\n";

    // Get resource hierarchies which contain the given input path
    std::vector<std::string> hierarchies;
    int status = get_hierarchies_from_data_path( conn, hierarchies, inpPath );
    if( status < 0 ) {
        if( CAT_NO_ROWS_FOUND == status ) {
            printf( not_registered_message, inpPath );
        }
        return status;
    }

    // Gather all resources for this hostname
    std::vector<std::string> resource_names;
    status = get_resource_names_for_host( conn, resource_names, hostname );
    if( status < 0 ) {
        if( CAT_NO_ROWS_FOUND == status ) {
            printf( not_registered_message, inpPath );
        }
        return status;
    }

    // Traverse the leaves of the list of hierarchies which are associated with the given pathname,
    // and the resources on this host. Intersection indicates that data path is registered with iRODS.
    for( std::vector<std::string>::iterator hier_it = hierarchies.begin(); hier_it != hierarchies.end(); ++hier_it ) {
        std::string leaf;
        irods::hierarchy_parser parser;
        parser.set_string( *hier_it );
        parser.last_resc( leaf );
        for( std::vector<std::string>::iterator name_it = resource_names.begin(); name_it != resource_names.end(); ++name_it ) {
            if( leaf == *name_it ) {
                return status;
            }
        }
    }

    // Data path is registered but resides on a different host
    printf( not_registered_message, inpPath );
    return CAT_NO_ROWS_FOUND;
}

int
checkIsMount( rcComm_t *conn, const char *inpPath ) {
    int i, minLen, status, status1;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    char condStr[MAX_NAME_LEN], *dirMPath;

    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    addInxIval( &genQueryInp.selectInp, COL_COLL_INFO1, 1 );
    genQueryInp.maxRows = MAX_SQL_ROWS;

    snprintf( condStr, MAX_NAME_LEN, "='%s'", "mountPoint" );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_TYPE, condStr );

    status1 = rcGenQuery( conn, &genQueryInp, &genQueryOut );
    if ( status1 == CAT_NO_ROWS_FOUND ) {
        status = 0;  /* there is no mounted collection, so no potential problem */
    }
    else {  /* check if inpPath is part of one of the mounted collections */
        status = 0;
        for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
            dirMPath = genQueryOut->sqlResult[0].value;
            dirMPath += i * genQueryOut->sqlResult[0].len;
            if ( strlen( dirMPath ) <= strlen( inpPath ) ) {
                minLen = strlen( dirMPath );
            }
            else {
                minLen = strlen( inpPath );
            }
            if ( strncmp( dirMPath, inpPath, minLen ) == 0 ) {
                status = -1;
            }
        }
    }

    clearGenQueryInp( &genQueryInp );
    freeGenQueryOut( &genQueryOut );

    return status;

}

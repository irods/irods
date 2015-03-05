/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* objMetaOpr.c - metadata operation at the object level */

#ifndef windows_platform

#include <sys/types.h>
#include <sys/wait.h>
#endif
#include "objMetaOpr.hpp"
#include "resource.hpp"
#include "collection.hpp"
#include "specColl.hpp"
#include "genQuery.hpp"
#include "icatHighLevelRoutines.hpp"
#include "miscUtil.hpp"
#include "rodsClient.hpp"
#include "rsIcatOpr.hpp"

// =-=-=-=-=-=-
#include "irods_resource_redirect.hpp"
#include "irods_hierarchy_parser.hpp"

int
svrCloseQueryOut( rsComm_t *rsComm, genQueryOut_t *genQueryOut ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *junk = NULL;
    int status;

    if ( genQueryOut->continueInx <= 0 ) {
        return 0;
    }

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    /* maxRows = 0 specifies that the genQueryOut should be closed */
    genQueryInp.maxRows = 0;;
    genQueryInp.continueInx = genQueryOut->continueInx;

    status =  rsGenQuery( rsComm, &genQueryInp, &junk );

    return status;
}

int
isData( rsComm_t *rsComm, char *objName, rodsLong_t *dataId ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    char tmpStr[MAX_NAME_LEN];
    char logicalEndName[MAX_NAME_LEN];
    char logicalParentDirName[MAX_NAME_LEN];
    int status;

    status = splitPathByKey( objName,
                             logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/' );
    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    snprintf( tmpStr, MAX_NAME_LEN, "='%s'", logicalEndName );
    addInxVal( &genQueryInp.sqlCondInp, COL_DATA_NAME, tmpStr );
    addInxIval( &genQueryInp.selectInp, COL_D_DATA_ID, 1 );
    snprintf( tmpStr, MAX_NAME_LEN, "='%s'", logicalParentDirName );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_NAME, tmpStr );
    addInxIval( &genQueryInp.selectInp, COL_COLL_ID, 1 );
    genQueryInp.maxRows = 2;
    status =  rsGenQuery( rsComm, &genQueryInp, &genQueryOut );
    if ( status >= 0 ) {
        sqlResult_t *dataIdRes;

        if ( ( dataIdRes = getSqlResultByInx( genQueryOut, COL_D_DATA_ID ) ) ==
                NULL ) {
            rodsLog( LOG_ERROR,
                     "isData: getSqlResultByInx for COL_D_DATA_ID failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        if ( dataId != NULL ) {
            *dataId = strtoll( dataIdRes->value, 0, 0 );
        }
        freeGenQueryOut( &genQueryOut );
    }

    clearGenQueryInp( &genQueryInp );
    return status;
}

int // JMC - backport 4680
getPhyPath(
    rsComm_t* _comm,
    char*     _obj_name,
    char*     _resource,
    char*     _phy_path,
    char*     _resc_hier ) {
    // =-=-=-=-=-=-=-
    // if no resc hier is specified, call resolve on the object to
    // ask the resource composition to pick a valid hier for open
    std::string  resc_hier;

    if ( 0 == strlen( _resc_hier ) ) {
        dataObjInp_t data_inp;
        memset( &data_inp, 0, sizeof( data_inp ) );
        snprintf( data_inp.objPath, sizeof( data_inp.objPath ), "%s", _obj_name );
        irods::error ret = irods::resolve_resource_hierarchy(
                               irods::OPEN_OPERATION,
                               _comm,
                               &data_inp,
                               resc_hier );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return SYS_INVALID_INPUT_PARAM;
        }
    }
    else {
        resc_hier = _resc_hier;
    }

    std::string              root_resc;
    irods::hierarchy_parser parser;
    parser.set_string( resc_hier );
    parser.first_resc( root_resc );

    // =-=-=-=-=-=-=-
    // perform a genquery to get the physical path of the data object
    // as the hier reolve does not do that for us
    genQueryOut_t* gen_out = NULL;
    char tmp_str                [ MAX_NAME_LEN ];
    char logical_end_name       [ MAX_NAME_LEN ];
    char logical_parent_dir_name[ MAX_NAME_LEN ];

    // =-=-=-=-=-=-=-
    // split the object path by the last delimiter /
    int status = splitPathByKey(
                     _obj_name,
                     logical_parent_dir_name, MAX_NAME_LEN,
                     logical_end_name, MAX_NAME_LEN, '/' );

    genQueryInp_t  gen_inp;
    memset( &gen_inp, 0, sizeof( genQueryInp_t ) );

    // =-=-=-=-=-=-=-
    // add query to the struct for the data object name
    snprintf( tmp_str, MAX_NAME_LEN, "='%s'", logical_end_name );
    addInxVal( &gen_inp.sqlCondInp, COL_DATA_NAME, tmp_str );

    // =-=-=-=-=-=-=-
    // add query to the struct for the collection name
    snprintf( tmp_str, MAX_NAME_LEN, "='%s'", logical_parent_dir_name );
    addInxVal( &gen_inp.sqlCondInp, COL_COLL_NAME, tmp_str );

    // =-=-=-=-=-=-=-
    // add query to the struct for the resource hierarchy
    snprintf( tmp_str, MAX_NAME_LEN, "='%s'", resc_hier.c_str() );
    addInxVal( &gen_inp.sqlCondInp, COL_D_RESC_HIER, tmp_str );

    // =-=-=-=-=-=-=-
    // include request for data path and resource hierarchy
    addInxIval( &gen_inp.selectInp, COL_D_DATA_PATH, 1 );

    // =-=-=-=-=-=-=-
    // request only 2 results in the set
    gen_inp.maxRows = 2;
    status = rsGenQuery( _comm, &gen_inp, &gen_out );
    if ( status >= 0 ) {
        // =-=-=-=-=-=-=-
        // extract the physical path from the query
        sqlResult_t* phy_path_res = getSqlResultByInx( gen_out, COL_D_DATA_PATH );
        if ( !phy_path_res ) {
            rodsLog( LOG_ERROR,
                     "getPhyPath: getSqlResultByInx for COL_D_DATA_PATH failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        // =-=-=-=-=-=-=-
        // copy the results to the out variables
        strncpy( _phy_path,  phy_path_res->value, MAX_NAME_LEN );
        strncpy( _resource,  root_resc.c_str(),   root_resc.size() );
        strncpy( _resc_hier, resc_hier.c_str(),   resc_hier.size() );

        freeGenQueryOut( &gen_out );
    }

    clearGenQueryInp( &gen_inp );

    return status;

}

int
isCollAllKinds( rsComm_t *rsComm, char *objName, rodsLong_t *collId ) {
    dataObjInp_t dataObjInp;
    int status;
    rodsObjStat_t *rodsObjStatOut = NULL;

    bzero( &dataObjInp, sizeof( dataObjInp ) );
    rstrcpy( dataObjInp.objPath, objName, MAX_NAME_LEN );
    status = collStatAllKinds( rsComm, &dataObjInp, &rodsObjStatOut );
    if ( status >= 0 && collId != NULL && NULL != rodsObjStatOut ) { // JMC cppcheck - nullptr
        *collId = strtoll( rodsObjStatOut->dataId, 0, 0 );
    }
    if ( rodsObjStatOut != NULL ) {
        freeRodsObjStat( rodsObjStatOut );
    }
    return status;
}

int
isColl( rsComm_t *rsComm, char *objName, rodsLong_t *collId ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    char tmpStr[MAX_NAME_LEN];
    int status;

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    snprintf( tmpStr, MAX_NAME_LEN, "='%s'", objName );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_NAME, tmpStr );
    addInxIval( &genQueryInp.selectInp, COL_COLL_ID, 1 );
    genQueryInp.maxRows = 2;
    status =  rsGenQuery( rsComm, &genQueryInp, &genQueryOut );
    if ( status >= 0 ) {
        sqlResult_t *collIdRes;

        if ( ( collIdRes = getSqlResultByInx( genQueryOut, COL_COLL_ID ) ) ==
                NULL ) {
            rodsLog( LOG_ERROR,
                     "isColl: getSqlResultByInx for COL_D_DATA_ID failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        if ( collId != NULL ) {
            *collId = strtoll( collIdRes->value, 0, 0 );
        }
        freeGenQueryOut( &genQueryOut );
    }

    clearGenQueryInp( &genQueryInp );
    return status;
}

int
isUser( rsComm_t *rsComm, char *objName ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    char tmpStr[NAME_LEN];
    int status;

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    snprintf( tmpStr, NAME_LEN, "='%s'", objName );
    addInxVal( &genQueryInp.sqlCondInp, COL_USER_NAME, tmpStr );
    addInxIval( &genQueryInp.selectInp, COL_USER_ID, 1 );
    genQueryInp.maxRows = 2;
    status =  rsGenQuery( rsComm, &genQueryInp, &genQueryOut );
    freeGenQueryOut( &genQueryOut );
    clearGenQueryInp( &genQueryInp );
    return status;
}

int
isResc( rsComm_t *rsComm, char *objName ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    char tmpStr[NAME_LEN];
    int status;

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    snprintf( tmpStr, NAME_LEN, "='%s'", objName );
    addInxVal( &genQueryInp.sqlCondInp, COL_R_RESC_NAME, tmpStr );
    addInxIval( &genQueryInp.selectInp, COL_R_RESC_ID, 1 );
    genQueryInp.maxRows = 2;
    status =  rsGenQuery( rsComm, &genQueryInp, &genQueryOut );
    freeGenQueryOut( &genQueryOut );
    clearGenQueryInp( &genQueryInp );
    return status;
}

int
isMeta( rsComm_t*, char* ) {
    /* needs to be filled in later */
    return INVALID_OBJECT_TYPE;
}

int
isToken( rsComm_t*, char* ) {
    /* needs to be filled in later */
    return INVALID_OBJECT_TYPE;
}

int
getObjType( rsComm_t *rsComm, char *objName, char * objType ) {
    if ( isData( rsComm, objName, NULL ) >= 0 ) {
        strcpy( objType, "-d" );
    }
    else if ( isColl( rsComm, objName, NULL ) >= 0 ) {
        strcpy( objType, "-c" );
    }
    else if ( isResc( rsComm, objName ) == 0 ) {
        strcpy( objType, "-r" );
    }
    else if ( isUser( rsComm, objName ) == 0 ) {
        strcpy( objType, "-u" );
    }
    else if ( isMeta( rsComm, objName ) == 0 ) {
        strcpy( objType, "-m" );
    }
    else if ( isToken( rsComm, objName ) == 0 ) {
        strcpy( objType, "-t" );
    }
    else {
        return INVALID_OBJECT_TYPE;
    }
    return 0;
}

int
addAVUMetadataFromKVPairs( rsComm_t *rsComm, char *objName, char *inObjType,
                           keyValPair_t *kVP ) {
    char  objType[10];
    if ( strcmp( inObjType, "-1" ) ) {
        if ( strlen( inObjType ) >= sizeof( objType ) ) {
            rodsLog( LOG_ERROR, "inObjType: [%s] must be fewer than %ju characters", inObjType, ( uintmax_t )sizeof( objType ) );
            return SYS_INVALID_INPUT_PARAM;
        }
        strcpy( objType, inObjType );
    }
    else {
        int status = getObjType( rsComm, objName, objType );
        if ( status < 0 ) {
            return status;
        }
    }

    for ( int i = 0; i < kVP->len ; i++ ) {
        /* Call rsModAVUMetadata to call chlAddAVUMetadata.
           rsModAVUMetadata connects to the icat-enabled server if the
           local host isn't.
        */
        modAVUMetadataInp_t modAVUMetadataInp;
        memset( &modAVUMetadataInp, 0, sizeof( modAVUMetadataInp ) );
        modAVUMetadataInp.arg0 = "add";
        modAVUMetadataInp.arg1 = objType;
        modAVUMetadataInp.arg2 = objName;
        modAVUMetadataInp.arg3 = kVP->keyWord[i];
        modAVUMetadataInp.arg4 = kVP->value[i];
        modAVUMetadataInp.arg5 = "";
        int status = rsModAVUMetadata( rsComm, &modAVUMetadataInp );
        if ( status < 0 ) {
            return status;
        }
    }
    return 0;
}
// =-=-=-=-=-=-=-
// JMC - backport 4836
int
setAVUMetadataFromKVPairs( rsComm_t *rsComm, char *objName, char *inObjType,
                           keyValPair_t *kVP ) {

    char  objType[10];
    if ( strcmp( inObjType, "-1" ) ) {
        snprintf( objType, sizeof( objType ), "%s", inObjType );
    }
    else {
        int status = getObjType( rsComm, objName, objType );
        if ( status < 0 ) {
            return status;
        }
    }
    for ( int i = 0; i < kVP->len ; i++ ) {
        /* Call rsModAVUMetadata to call chlSetAVUMetadata.
           rsModAVUMetadata connects to the icat-enabled server if the
           local host isn't.
        */
        modAVUMetadataInp_t modAVUMetadataInp;
        memset( &modAVUMetadataInp, 0, sizeof( modAVUMetadataInp ) );
        modAVUMetadataInp.arg0 = "set";
        modAVUMetadataInp.arg1 = objType;
        modAVUMetadataInp.arg2 = objName;
        modAVUMetadataInp.arg3 = kVP->keyWord[i];
        modAVUMetadataInp.arg4 = kVP->value[i];
        modAVUMetadataInp.arg5 = NULL;
        int status = rsModAVUMetadata( rsComm, &modAVUMetadataInp );
        if ( status < 0 ) {
            return status;
        }
    }
    return 0;
}
// =-=-=-=-=-=-=-
int
getStructFileType( specColl_t *specColl ) {
    if ( specColl == NULL ) {
        return -1;
    }

    if ( specColl->collClass == STRUCT_FILE_COLL ) {
        return ( int ) specColl->type;
    }
    else {
        return -1;
    }
}

int
removeAVUMetadataFromKVPairs( rsComm_t *rsComm, char *objName, char *inObjType,
                              keyValPair_t *kVP ) {
    char  objType[10];
    if ( strcmp( inObjType, "-1" ) ) {
        snprintf( objType, sizeof( objType ), "%s", inObjType );
    }
    else {
        int status = getObjType( rsComm, objName, objType );
        if ( status < 0 ) {
            return status;
        }
    }

    for ( int i = 0; i < kVP->len ; i++ ) {
        /* Call rsModAVUMetadata to call chlAddAVUMetadata.
           rsModAVUMetadata connects to the icat-enabled server if the
           local host isn't.
        */
        modAVUMetadataInp_t modAVUMetadataInp;
        modAVUMetadataInp.arg0 = "rm";
        modAVUMetadataInp.arg1 = objType;
        modAVUMetadataInp.arg2 = objName;
        modAVUMetadataInp.arg3 = kVP->keyWord[i];
        modAVUMetadataInp.arg4 = kVP->value[i];
        modAVUMetadataInp.arg5 = "";
        int status = rsModAVUMetadata( rsComm, &modAVUMetadataInp );
        if ( status < 0 ) {
            return status;
        }
    }
    return 0;
}

int
getTokenId( rsComm_t *rsComm, char *tokenNameSpace, char *tokenName ) {

    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    char tmpStr[MAX_NAME_LEN];
    char tmpStr2[MAX_NAME_LEN];
    int status;

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    snprintf( tmpStr, NAME_LEN, "='%s'", tokenNameSpace );
    snprintf( tmpStr2, NAME_LEN, "='%s'", tokenName );
    addInxVal( &genQueryInp.sqlCondInp, COL_TOKEN_NAMESPACE, tmpStr );
    addInxVal( &genQueryInp.sqlCondInp, COL_TOKEN_NAME, tmpStr2 );
    addInxIval( &genQueryInp.selectInp, COL_TOKEN_ID, 1 );
    genQueryInp.maxRows = 2;
    status =  rsGenQuery( rsComm, &genQueryInp, &genQueryOut );
    clearGenQueryInp( &genQueryInp );
    if ( status >= 0 ) {
        sqlResult_t *tokenIdRes;

        if ( ( tokenIdRes = getSqlResultByInx( genQueryOut, COL_TOKEN_ID ) ) ==
                NULL ) {
            rodsLog( LOG_ERROR,
                     "getTokenId: getSqlResultByInx for COL_TOKEN_ID failed" );
            freeGenQueryOut( &genQueryOut );
            return UNMATCHED_KEY_OR_INDEX;
        }
        status = atoi( tokenIdRes->value );
        freeGenQueryOut( &genQueryOut );
    }
    return status;
}

int
getUserId( rsComm_t *rsComm, char *userName, char *zoneName ) {

    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    char tmpStr[MAX_NAME_LEN];
    char tmpStr2[MAX_NAME_LEN];
    int status;

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    snprintf( tmpStr, NAME_LEN, "='%s'", userName );
    snprintf( tmpStr2, NAME_LEN, "='%s'", zoneName );
    addInxVal( &genQueryInp.sqlCondInp, COL_USER_NAME, tmpStr );
    addInxVal( &genQueryInp.sqlCondInp, COL_USER_ZONE, tmpStr2 );
    addInxIval( &genQueryInp.selectInp, COL_USER_ID, 1 );
    genQueryInp.maxRows = 2;
    status =  rsGenQuery( rsComm, &genQueryInp, &genQueryOut );
    clearGenQueryInp( &genQueryInp );
    if ( status >= 0 ) {
        sqlResult_t *userIdRes;

        if ( ( userIdRes = getSqlResultByInx( genQueryOut, COL_USER_ID ) ) ==
                NULL ) {
            rodsLog( LOG_ERROR,
                     "getUserId: getSqlResultByInx for COL_USER_ID failed" );
            freeGenQueryOut( &genQueryOut );
            return UNMATCHED_KEY_OR_INDEX;
        }
        status = atoi( userIdRes->value );
        freeGenQueryOut( &genQueryOut );
    }
    return status;
}


int
checkPermitForDataObject( rsComm_t *rsComm, char *objName, int userId, int operId ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    char t1[MAX_NAME_LEN]; // JMC cppcheck - snprintf out of bounds
    char t11[MAX_NAME_LEN]; // JMC cppcheck - snprintf out of bounds
    char t2[MAX_NAME_LEN]; // JMC cppcheck - snprintf out of bounds
    char t3[MAX_NAME_LEN]; // JMC cppcheck - snprintf out of bounds
    char logicalEndName[MAX_NAME_LEN];
    char logicalParentDirName[MAX_NAME_LEN];
    int status;

    status = splitPathByKey( objName,
                             logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/' );
    snprintf( t1, MAX_NAME_LEN, " = '%s'", logicalEndName );
    snprintf( t11, MAX_NAME_LEN, " = '%s'", logicalParentDirName );
    snprintf( t2, MAX_NAME_LEN, " = '%i'", userId );
    snprintf( t3, MAX_NAME_LEN, " >= '%i' ", operId );

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    addInxIval( &genQueryInp.selectInp, COL_D_DATA_ID, 1 );
    addInxVal( &genQueryInp.sqlCondInp, COL_DATA_NAME, t1 );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_NAME, t11 );
    addInxVal( &genQueryInp.sqlCondInp, COL_DATA_ACCESS_USER_ID, t2 );
    addInxVal( &genQueryInp.sqlCondInp, COL_DATA_ACCESS_TYPE, t3 );
    genQueryInp.maxRows = 2;
    status =  rsGenQuery( rsComm, &genQueryInp, &genQueryOut );
    clearGenQueryInp( &genQueryInp );
    if ( status >= 0 ) {
        freeGenQueryOut( &genQueryOut );
        return 1;
    }
    else {
        return 0;
    }
}

int
checkPermitForCollection( rsComm_t *rsComm, char *objName, int userId, int operId ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    char t1[MAX_NAME_LEN]; // JMC cppcheck - snprintf out of bounds
    char t2[MAX_NAME_LEN]; // JMC cppcheck - snprintf out of bounds
    char t4[MAX_NAME_LEN]; // JMC cppcheck - snprintf out of bounds
    int status;

    snprintf( t1, MAX_NAME_LEN, " = '%s'", objName );
    snprintf( t2, MAX_NAME_LEN, " = '%i'", userId );
    snprintf( t4, MAX_NAME_LEN, " >= '%i' ", operId );

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    addInxIval( &genQueryInp.selectInp, COL_COLL_ID, 1 );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_NAME, t1 );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_ACCESS_USER_ID, t2 );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_ACCESS_TYPE, t4 );
    genQueryInp.maxRows = 2;
    status =  rsGenQuery( rsComm, &genQueryInp, &genQueryOut );
    clearGenQueryInp( &genQueryInp );
    if ( status >= 0 ) {
        freeGenQueryOut( &genQueryOut );
        return 1;
    }
    else {
        return 0;
    }
}

int
checkPermitForResource( rsComm_t *rsComm, char *objName, int userId, int operId ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    char t1[MAX_NAME_LEN]; // JMC cppcheck - snprintf out of bounds
    char t2[MAX_NAME_LEN]; // JMC cppcheck - snprintf out of bounds
    char t4[MAX_NAME_LEN]; // JMC cppcheck - snprintf out of bounds
    int status;

    snprintf( t1, MAX_NAME_LEN, " = '%s'", objName );
    snprintf( t2, MAX_NAME_LEN, " = '%i'", userId );
    snprintf( t4, MAX_NAME_LEN, " >= '%i' ", operId );

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    addInxIval( &genQueryInp.selectInp, COL_R_RESC_ID, 1 );
    addInxVal( &genQueryInp.sqlCondInp, COL_R_RESC_NAME, t1 );
    addInxVal( &genQueryInp.sqlCondInp, COL_RESC_ACCESS_USER_ID, t2 );
    addInxVal( &genQueryInp.sqlCondInp, COL_RESC_ACCESS_TYPE, t4 );
    genQueryInp.maxRows = 2;
    status =  rsGenQuery( rsComm, &genQueryInp, &genQueryOut );
    clearGenQueryInp( &genQueryInp );
    if ( status >= 0 ) {
        freeGenQueryOut( &genQueryOut );
        return 1;
    }
    else {
        return 0;
    }
}


int
checkPermissionByObjType( rsComm_t *rsComm, char *objName, char *objType, char *user, char *zone, char *oper ) {
    int i;
    int operId;
    int userId;
    operId = getTokenId( rsComm, "access_type", oper );
    if ( operId < 0 ) {
        return operId;
    }

    userId = getUserId( rsComm, user, zone );
    if ( userId < 0 ) {
        return userId;
    }

    if ( !strcmp( objType, "-d" ) ) {
        i = checkPermitForDataObject( rsComm, objName, userId, operId );
    }
    else  if ( !strcmp( objType, "-c" ) ) {
        i = checkPermitForCollection( rsComm, objName, userId, operId );
    }
    else  if ( !strcmp( objType, "-r" ) ) {
        i = checkPermitForResource( rsComm, objName, userId, operId );
    }
    /*
    else  if (!strcmp(objType,"-m"))
      i = checkPermitForMetadata(rsComm, objName, userId, operId);
    */
    else {
        i = INVALID_OBJECT_TYPE;
    }
    return i;
}

/* checkDupReplica - Check if a given object with a given rescName
 * and physical path already exist. If it does, returns the replNum.
 * JMC - backport 4497 */
int
checkDupReplica( rsComm_t *rsComm, rodsLong_t dataId, char *rescName,
                 char *filePath ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    char tmpStr[MAX_NAME_LEN];
    int status;

    if ( rsComm == NULL || rescName == NULL || filePath == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    bzero( &genQueryInp, sizeof( genQueryInp_t ) );

    snprintf( tmpStr, MAX_NAME_LEN, "='%s'", rescName );
    addInxVal( &genQueryInp.sqlCondInp, COL_D_RESC_NAME, tmpStr );
    snprintf( tmpStr, MAX_NAME_LEN, "='%s'", filePath );
    addInxVal( &genQueryInp.sqlCondInp, COL_D_DATA_PATH, tmpStr );
    snprintf( tmpStr, MAX_NAME_LEN, "='%lld'", dataId );
    addInxVal( &genQueryInp.sqlCondInp, COL_D_DATA_ID, tmpStr );

    addInxIval( &genQueryInp.selectInp, COL_DATA_REPL_NUM, 1 );
    genQueryInp.maxRows = 2;
    status =  rsGenQuery( rsComm, &genQueryInp, &genQueryOut );
    clearGenQueryInp( &genQueryInp );
    if ( status >= 0 ) {
        int intReplNum;
        sqlResult_t *replNum;
        if ( ( replNum = getSqlResultByInx( genQueryOut, COL_DATA_REPL_NUM ) ) ==
                NULL ) {
            rodsLog( LOG_ERROR,
                     "checkDupReplica: getSqlResultByInx COL_DATA_REPL_NUM failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        intReplNum = atoi( replNum->value );
        freeGenQueryOut( &genQueryOut );
        return intReplNum;
    }
    else {
        return status;
    }
}
// =-=-=-=-=-=-=-
// JMC - backport 4552
int
getNumSubfilesInBunfileObj( rsComm_t *rsComm, char *objPath ) {
    int status;
    genQueryOut_t *genQueryOut = NULL;
    genQueryInp_t genQueryInp;
    int totalRowCount;
    char condStr[MAX_NAME_LEN];

    bzero( &genQueryInp, sizeof( genQueryInp ) );
    genQueryInp.maxRows = 1;
    genQueryInp.options = RETURN_TOTAL_ROW_COUNT;

    snprintf( condStr, MAX_NAME_LEN, "='%s'", objPath );
    addInxVal( &genQueryInp.sqlCondInp, COL_D_DATA_PATH, condStr );
    snprintf( condStr, MAX_NAME_LEN, "='%s'", BUNDLE_RESC_CLASS );
    addInxVal( &genQueryInp.sqlCondInp, COL_R_CLASS_NAME, condStr );
    addKeyVal( &genQueryInp.condInput, ZONE_KW, objPath );

    addInxIval( &genQueryInp.selectInp, COL_COLL_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DATA_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DATA_SIZE, 1 );

    status =  rsGenQuery( rsComm, &genQueryInp, &genQueryOut );
    if ( genQueryOut == NULL || status < 0 ) {
        clearGenQueryInp( &genQueryInp );
        if ( status == CAT_NO_ROWS_FOUND ) {
            return 0;
        }
        else {
            return status;
        }
    }
    totalRowCount = genQueryOut->totalRowCount;
    freeGenQueryOut( &genQueryOut );
    /* clear result */
    genQueryInp.maxRows = 0;
    status =  rsGenQuery( rsComm, &genQueryInp, &genQueryOut );
    clearGenQueryInp( &genQueryInp );

    return totalRowCount;
}
// =-=-=-=-=-=-=-



/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* collection.c - collection operation */

#include "collection.hpp"
#include "specColl.hpp"
#include "genQuery.hpp"
#include "rodsClient.hpp"

/* checkCollAccessPerm - Check whether the user is allowed to perform
 * operation specified by the accessPerm string on the collection.
 * Various accessPerm are defined in  icatDefines.h. (e.g.,
 * ACCESS_DELETE_OBJECT for create and delete, ACCESS_READ_OBJECT for read).
 * A return of >= 0 means allow and a negative value means disallow.
 */

int
checkCollAccessPerm( rsComm_t *rsComm, char *collection, char *accessPerm ) {
    char accStr[LONG_NAME_LEN];
    char condStr[MAX_NAME_LEN];
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    int status;

    if ( collection == NULL || accessPerm == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    memset( &genQueryInp, 0, sizeof( genQueryInp ) );

    snprintf( accStr, LONG_NAME_LEN, "%s", rsComm->clientUser.userName );
    addKeyVal( &genQueryInp.condInput, USER_NAME_CLIENT_KW, accStr );

    snprintf( accStr, LONG_NAME_LEN, "%s", rsComm->clientUser.rodsZone );
    addKeyVal( &genQueryInp.condInput, RODS_ZONE_CLIENT_KW, accStr );

    snprintf( accStr, LONG_NAME_LEN, "%s", accessPerm );
    addKeyVal( &genQueryInp.condInput, ACCESS_PERMISSION_KW, accStr );

    snprintf( condStr, MAX_NAME_LEN, "='%s'", collection );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_NAME, condStr );

    addInxIval( &genQueryInp.selectInp, COL_COLL_ID, 1 );

    genQueryInp.maxRows = MAX_SQL_ROWS;

    status =  rsGenQuery( rsComm, &genQueryInp, &genQueryOut );

    clearGenQueryInp( &genQueryInp );
    if ( status >= 0 ) {
        freeGenQueryOut( &genQueryOut );
    }

    return status;
}

/*
   Try a specific-query for the a particular Data-object in collection
   query first, for performance improvements, before going back to a
   general-query.  This is called from rsQueryDataObjInCollReCur below.
   See the svn commit log (r5682) for the specific query definition.
 */
int trySpecificQueryDataObjInCollReCur(
    rsComm_t *rsComm,
    char *collection,
    genQueryOut_t **genQueryOut ) {

    int status = 0;
    char collNamePercent[MAX_NAME_LEN + 2];

    specificQueryInp_t specificQueryInp;

    rstrcpy( collNamePercent, collection, MAX_NAME_LEN );
    rstrcat( collNamePercent, "/%", MAX_NAME_LEN );

    memset( &specificQueryInp, 0, sizeof( specificQueryInp_t ) );

    specificQueryInp.maxRows = MAX_SQL_ROWS;
    specificQueryInp.continueInx = 0;

    specificQueryInp.sql = "DataObjInCollReCur";
    specificQueryInp.args[0] = collection;
    specificQueryInp.args[1] = collNamePercent;

    status = rsSpecificQuery( rsComm, &specificQueryInp, genQueryOut );

    if ( status == 0 ) {
        /*
           Set the attriInx values so the server-side code can locate
           the fields (avoid a UNMATCHED_KEY_OR_INDEX error) the way
           general-query is handled.  The specific-query can't set these
           because the columns being returned are not known (unlike for
           general-query).  So this code assumes the DataObjInCollReCur
           is defined correctly and is returning the following columns.
           The result->attriCnt == 6 test is a sanity check.
         */
        genQueryOut_t *result;
        result = *genQueryOut;
        if ( result->attriCnt == 7 ) {

            result->sqlResult[0].attriInx = COL_D_DATA_ID;
            result->sqlResult[1].attriInx = COL_COLL_NAME;
            result->sqlResult[2].attriInx = COL_DATA_NAME;
            result->sqlResult[3].attriInx = COL_DATA_REPL_NUM;
            result->sqlResult[4].attriInx = COL_D_RESC_NAME;
            result->sqlResult[5].attriInx = COL_D_DATA_PATH;
            result->sqlResult[6].attriInx = COL_D_RESC_HIER;
        }
    }
    return status;
}



int
rsQueryDataObjInCollReCur( rsComm_t *rsComm, char *collection,
                           genQueryInp_t *genQueryInp, genQueryOut_t **genQueryOut, char *accessPerm,
                           int singleFlag ) {
    char collQCond[MAX_NAME_LEN * 2];
    int status = 0;
    char accStr[LONG_NAME_LEN];

    if ( genQueryInp == NULL ||
            collection  == NULL ||
            genQueryOut == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    memset( genQueryInp, 0, sizeof( genQueryInp_t ) );

    genAllInCollQCond( collection, collQCond );

    addInxVal( &genQueryInp->sqlCondInp, COL_COLL_NAME, collQCond );

    addInxIval( &genQueryInp->selectInp, COL_D_DATA_ID, 1 );
    addInxIval( &genQueryInp->selectInp, COL_COLL_NAME, 1 );
    addInxIval( &genQueryInp->selectInp, COL_DATA_NAME, 1 );

    if ( singleFlag == 0 ) {
        addInxIval( &genQueryInp->selectInp, COL_DATA_REPL_NUM, 1 );
        addInxIval( &genQueryInp->selectInp, COL_D_RESC_NAME, 1 );
        addInxIval( &genQueryInp->selectInp, COL_D_DATA_PATH, 1 );
    }

    addInxIval( &genQueryInp->selectInp, COL_D_RESC_HIER, 1 );

    if ( accessPerm != NULL ) {
        snprintf( accStr, LONG_NAME_LEN, "%s", rsComm->clientUser.userName );
        addKeyVal( &genQueryInp->condInput, USER_NAME_CLIENT_KW, accStr );

        snprintf( accStr, LONG_NAME_LEN, "%s", rsComm->clientUser.rodsZone );
        addKeyVal( &genQueryInp->condInput, RODS_ZONE_CLIENT_KW, accStr );

        snprintf( accStr, LONG_NAME_LEN, "%s", accessPerm );
        addKeyVal( &genQueryInp->condInput, ACCESS_PERMISSION_KW, accStr );
        /* have to set it to 1 because it only check the first one */
        genQueryInp->maxRows = 1;
        status =  rsGenQuery( rsComm, genQueryInp, genQueryOut );
        rmKeyVal( &genQueryInp->condInput, USER_NAME_CLIENT_KW );
        rmKeyVal( &genQueryInp->condInput, RODS_ZONE_CLIENT_KW );
        rmKeyVal( &genQueryInp->condInput, ACCESS_PERMISSION_KW );
    }
    else {
        genQueryInp->maxRows = MAX_SQL_ROWS;
        status = trySpecificQueryDataObjInCollReCur(
                     rsComm,
                     collection,
                     genQueryOut );
        if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
            rodsLog(
                LOG_NOTICE,
                "Note: DataObjInCollReCur specific-Query failed (not defined?), running standard query, status=%d",
                status );
            /* remove the level 0 error msg added by the specific-query failure */
            status = freeRErrorContent( &rsComm->rError );
            /* fall back to the general-query call which used before this
               specific-query was added (post 3.3.1) */
            genQueryInp->maxRows = MAX_SQL_ROWS;
            status =  rsGenQuery( rsComm, genQueryInp, genQueryOut );
        }

    }

    return status;

}

int
rsQueryCollInColl( rsComm_t *rsComm, char *collection,
                   genQueryInp_t *genQueryInp, genQueryOut_t **genQueryOut ) {
    char collQCond[MAX_NAME_LEN];
    int status;

    if ( collection == NULL || genQueryOut == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    memset( genQueryInp, 0, sizeof( genQueryInp_t ) );

    snprintf( collQCond, MAX_NAME_LEN, "='%s'", collection );

    addInxVal( &genQueryInp->sqlCondInp, COL_COLL_PARENT_NAME, collQCond );

    addInxIval( &genQueryInp->selectInp, COL_COLL_NAME, 1 );

    genQueryInp->maxRows = MAX_SQL_ROWS;

    status =  rsGenQuery( rsComm, genQueryInp, genQueryOut );

    return status;
}

int
isCollEmpty( rsComm_t *rsComm, char *collection ) {
    collInp_t openCollInp;
    collEnt_t *collEnt;
    int handleInx;
    int entCnt = 0;

    if ( rsComm == NULL || collection == NULL ) {
        rodsLog( LOG_ERROR,
                 "isCollEmpty: Input rsComm or collection is NULL" );
        return True;
    }

    memset( &openCollInp, 0, sizeof( openCollInp ) );
    rstrcpy( openCollInp.collName, collection, MAX_NAME_LEN );
    /* cannot query recur because collection is sorted in wrong order */
    openCollInp.flags = 0;
    handleInx = rsOpenCollection( rsComm, &openCollInp );
    if ( handleInx < 0 ) {
        rodsLog( LOG_ERROR,
                 "isCollEmpty: rsOpenCollection of %s error. status = %d",
                 openCollInp.collName, handleInx );
        return True;
    }

    while ( rsReadCollection( rsComm, &handleInx, &collEnt ) >= 0 ) {
        entCnt++;
        free( collEnt );    /* just free collEnt but not content */
    }

    rsCloseCollection( rsComm, &handleInx );

    if ( entCnt > 0 ) {
        return False;
    }
    else {
        return True;
    }
}

/* collStat - query metaData of a collection. */

int
collStat( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
          rodsObjStat_t **rodsObjStatOut ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    int status;
    char condStr[MAX_NAME_LEN];
    sqlResult_t *dataId;
    sqlResult_t *ownerName;
    sqlResult_t *ownerZone;
    sqlResult_t *createTime;
    sqlResult_t *modifyTime;
    sqlResult_t *collType;
    sqlResult_t *collInfo1;
    sqlResult_t *collInfo2;

    /* see if objPath is a collection */
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );

    snprintf( condStr, MAX_NAME_LEN, "='%s'", dataObjInp->objPath );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_NAME, condStr );

    addInxIval( &genQueryInp.selectInp, COL_COLL_ID, 1 );
    /* XXXX COL_COLL_NAME added for queueSpecColl */
    addInxIval( &genQueryInp.selectInp, COL_COLL_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_OWNER_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_OWNER_ZONE, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_CREATE_TIME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_MODIFY_TIME, 1 );
    /* XXXX may want to do this if spec coll is supported */
    addInxIval( &genQueryInp.selectInp, COL_COLL_TYPE, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_INFO1, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_INFO2, 1 );

    genQueryInp.maxRows = MAX_SQL_ROWS;

    status =  rsGenQuery( rsComm, &genQueryInp, &genQueryOut );
    if ( status >= 0 ) {
        *rodsObjStatOut = ( rodsObjStat_t * ) malloc( sizeof( rodsObjStat_t ) );
        memset( *rodsObjStatOut, 0, sizeof( rodsObjStat_t ) );
        ( *rodsObjStatOut )->objType = COLL_OBJ_T;
        status = ( int )COLL_OBJ_T;
        if ( ( dataId = getSqlResultByInx( genQueryOut,
                                           COL_COLL_ID ) ) == NULL ) {
            rodsLog( LOG_ERROR,
                     "_rsObjStat: getSqlResultByInx for COL_COLL_ID failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else if ( ( ownerName = getSqlResultByInx( genQueryOut,
                                COL_COLL_OWNER_NAME ) ) == NULL ) {
            rodsLog( LOG_ERROR,
                     "_rsObjStat:getSqlResultByInx for COL_COLL_OWNER_NAME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else if ( ( ownerZone = getSqlResultByInx( genQueryOut,
                                COL_COLL_OWNER_ZONE ) ) == NULL ) {
            rodsLog( LOG_ERROR,
                     "_rsObjStat:getSqlResultByInx for COL_COLL_OWNER_ZONE failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else if ( ( createTime = getSqlResultByInx( genQueryOut,
                                 COL_COLL_CREATE_TIME ) ) == NULL ) {
            rodsLog( LOG_ERROR,
                     "_rsObjStat:getSqlResultByInx for COL_COLL_CREATE_TIME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else if ( ( modifyTime = getSqlResultByInx( genQueryOut,
                                 COL_COLL_MODIFY_TIME ) ) == NULL ) {
            rodsLog( LOG_ERROR,
                     "_rsObjStat:getSqlResultByInx for COL_COLL_MODIFY_TIME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else if ( ( collType = getSqlResultByInx( genQueryOut,
                               COL_COLL_TYPE ) ) == NULL ) {
            rodsLog( LOG_ERROR,
                     "_rsObjStat:getSqlResultByInx for COL_COLL_TYPE failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else if ( ( collInfo1 = getSqlResultByInx( genQueryOut,
                                COL_COLL_INFO1 ) ) == NULL ) {
            rodsLog( LOG_ERROR,
                     "_rsObjStat:getSqlResultByInx for COL_COLL_INFO1 failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else if ( ( collInfo2 = getSqlResultByInx( genQueryOut,
                                COL_COLL_INFO2 ) ) == NULL ) {
            rodsLog( LOG_ERROR,
                     "_rsObjStat:getSqlResultByInx for COL_COLL_INFO2 failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else {
            rstrcpy( ( *rodsObjStatOut )->dataId, dataId->value, NAME_LEN );
            rstrcpy( ( *rodsObjStatOut )->ownerName, ownerName->value,
                     NAME_LEN );
            rstrcpy( ( *rodsObjStatOut )->ownerZone, ownerZone->value,
                     NAME_LEN );
            rstrcpy( ( *rodsObjStatOut )->createTime, createTime->value,
                     TIME_LEN );
            rstrcpy( ( *rodsObjStatOut )->modifyTime, modifyTime->value,
                     TIME_LEN );

            if ( strlen( collType->value ) > 0 ) {
                specCollCache_t *specCollCache = 0;

                if ( ( specCollCache =
                            matchSpecCollCache( dataObjInp->objPath ) ) != NULL ) {
                    replSpecColl( &specCollCache->specColl,
                                  &( *rodsObjStatOut )->specColl );
                }
                else {
                    status = queueSpecCollCache( rsComm, genQueryOut, // JMC - backport 4680?
                                                 dataObjInp->objPath );
                    if ( status < 0 ) {
                        return status;
                    }
                    replSpecColl( &SpecCollCacheHead->specColl,
                                  &( *rodsObjStatOut )->specColl );
                }


            }
        }
    }

    clearGenQueryInp( &genQueryInp );
    freeGenQueryOut( &genQueryOut );

    return status;
}

/* collStatAllKinds - Stat of a collection. The path can be part of a
 * specColl or not.
 */

int
collStatAllKinds( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                  rodsObjStat_t **rodsObjStatOut ) {
    int status;

    *rodsObjStatOut = NULL;
    addKeyVal( &dataObjInp->condInput, SEL_OBJ_TYPE_KW, "collection" );
    status = _rsObjStat( rsComm, dataObjInp, rodsObjStatOut );
    rmKeyVal( &dataObjInp->condInput, SEL_OBJ_TYPE_KW );
    if ( status >= 0 ) {
        if ( ( *rodsObjStatOut )->objType != COLL_OBJ_T ) {
            status = OBJ_PATH_DOES_NOT_EXIST;
        }
    }
    if ( status < 0 && *rodsObjStatOut != NULL ) {
        freeRodsObjStat( *rodsObjStatOut );
        *rodsObjStatOut = NULL;
    }
    return status;
}

/* mk the collection resursively */

int
rsMkCollR( rsComm_t *rsComm, const char *startColl, const char *destColl ) {
    int status;
    int startLen;
    int pathLen, tmpLen;
    char tmpPath[MAX_NAME_LEN];

    startLen = strlen( startColl );
    pathLen = strlen( destColl );

    rstrcpy( tmpPath, destColl, MAX_NAME_LEN );

    tmpLen = pathLen;

    while ( tmpLen > startLen ) {
        if ( isCollAllKinds( rsComm, tmpPath, NULL ) >= 0 ) {
            break;
        }

        /* Go backward */

        while ( tmpLen && tmpPath[tmpLen] != '/' ) {
            tmpLen --;
        }
        tmpPath[tmpLen] = '\0';
    }

    /* Now we go forward and make the required coll */
    while ( tmpLen < pathLen ) {
        collInp_t collCreateInp;

        /* Put back the '/' */
        tmpPath[tmpLen] = '/';
        memset( &collCreateInp, 0, sizeof( collCreateInp ) );
        rstrcpy( collCreateInp.collName, tmpPath, MAX_NAME_LEN );
        status = rsCollCreate( rsComm, &collCreateInp );

        if ( status == CAT_NAME_EXISTS_AS_DATAOBJ && isTrashPath( tmpPath ) ) {
            /* name conflict with a data object in the trash collection */
            dataObjCopyInp_t dataObjRenameInp;
            memset( &dataObjRenameInp, 0, sizeof( dataObjRenameInp ) );

            dataObjRenameInp.srcDataObjInp.oprType =
                dataObjRenameInp.destDataObjInp.oprType = RENAME_DATA_OBJ;
            rstrcpy( dataObjRenameInp.srcDataObjInp.objPath, tmpPath,
                     MAX_NAME_LEN );
            rstrcpy( dataObjRenameInp.destDataObjInp.objPath, tmpPath,
                     MAX_NAME_LEN );
            appendRandomToPath( dataObjRenameInp.destDataObjInp.objPath );

            status = rsDataObjRename( rsComm, &dataObjRenameInp );
            if ( status >= 0 ) {
                status = rsCollCreate( rsComm, &collCreateInp );
            }
        }
        /* something may be added by rsCollCreate */
        clearKeyVal( &collCreateInp.condInput );
        if ( status < 0 ) {
            // =-=-=-=-=-=-=-
            // JMC - backport 4819
            if ( status == CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME ) {
                rodsLog( LOG_DEBUG,
                         "rsMkCollR: rsCollCreate - coll %s already exist.stat = %d",
                         tmpPath, status );
                status = 0;
            }
            else {
                rodsLog( LOG_ERROR,
                         "rsMkCollR: rsCollCreate failed for %s, status =%d",
                         tmpPath, status );
            }
            // =-=-=-=-=-=-=-
            return status;
        }
        while ( tmpLen && tmpPath[tmpLen] != '\0' ) {
            tmpLen ++;
        }
    }
    return 0;
}

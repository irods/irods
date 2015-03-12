#include "rodsPath.hpp"
#include "rodsErrorTable.hpp"
#include "rodsLog.hpp"
#include "lsUtil.hpp"
#include "miscUtil.hpp"

char zoneHint[MAX_NAME_LEN]; // JMC - backport 4416

int
lsUtil( rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
        rodsPathInp_t *rodsPathInp ) {

    if ( rodsPathInp == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    if ( myRodsArgs->ticket == True ) {
        if ( myRodsArgs->ticketString == NULL ) {
            rodsLog( LOG_ERROR,
                     "initCondForPut: NULL ticketString error" );
            return USER__NULL_INPUT_ERR;
        }
        else {
            setSessionTicket( conn, myRodsArgs->ticketString );
        }
    }

    genQueryInp_t genQueryInp;
    initCondForLs( &genQueryInp );

    int savedStatus = 0;
    for ( int i = 0; i < rodsPathInp->numSrc; i++ ) {
        int status = 0;
        rstrcpy( zoneHint, rodsPathInp->srcPath[i].inPath, MAX_NAME_LEN ); // // JMC - backport 4416
        if ( rodsPathInp->srcPath[i].objType == UNKNOWN_OBJ_T ||
                rodsPathInp->srcPath[i].objState == UNKNOWN_ST ) {
            status = getRodsObjType( conn, &rodsPathInp->srcPath[i] );
            if ( rodsPathInp->srcPath[i].objState == NOT_EXIST_ST ) {
                if ( status == NOT_EXIST_ST ) {
                    rodsLog( LOG_ERROR,
                             "lsUtil: srcPath %s does not exist or user lacks access permission",
                             rodsPathInp->srcPath[i].outPath );
                }
                savedStatus = USER_INPUT_PATH_ERR;
                continue;
            }
        }

        if ( rodsPathInp->srcPath[i].objType == DATA_OBJ_T ) {
            status = lsDataObjUtil( conn, &rodsPathInp->srcPath[i],
                                    myRodsArgs, &genQueryInp );
        }
        else if ( rodsPathInp->srcPath[i].objType ==  COLL_OBJ_T ) {
            status = lsCollUtil( conn, &rodsPathInp->srcPath[i],
                                 myRodsEnv, myRodsArgs );
        }
        else {
            /* should not be here */
            rodsLog( LOG_ERROR,
                     "lsUtil: invalid ls objType %d for %s",
                     rodsPathInp->srcPath[i].objType, rodsPathInp->srcPath[i].outPath );
            return USER_INPUT_PATH_ERR;
        }
        /* XXXX may need to return a global status */
        if ( status < 0 && status != CAT_NO_ROWS_FOUND &&
                status != SYS_SPEC_COLL_OBJ_NOT_EXIST ) {
            rodsLogError( LOG_ERROR, status,
                          "lsUtil: ls error for %s, status = %d",
                          rodsPathInp->srcPath[i].outPath, status );
            savedStatus = status;
        }
    }
    return savedStatus;
}

int
lsDataObjUtil( rcComm_t *conn, rodsPath_t *srcPath,
               rodsArguments_t *rodsArgs, genQueryInp_t *genQueryInp ) {
    int status = 0;


    if ( rodsArgs->longOption == True ) {
        if ( srcPath->rodsObjStat != NULL &&
                srcPath->rodsObjStat->specColl != NULL ) {
            if ( srcPath->rodsObjStat->specColl->collClass == LINKED_COLL ) {
                lsDataObjUtilLong( conn, srcPath->rodsObjStat->specColl->objPath,
                                   rodsArgs, genQueryInp );
            }
            else {
                lsSpecDataObjUtilLong( srcPath, rodsArgs );
            }
        }
        else {
            lsDataObjUtilLong( conn, srcPath->outPath, rodsArgs, genQueryInp );
        }
    }
    else if ( rodsArgs->bundle == True ) { // JMC - backport 4536
        lsSubfilesInBundle( conn, srcPath->outPath );
    }
    else {
        printLsStrShort( srcPath->outPath );
        if ( rodsArgs->accessControl == True ) {
            printDataAcl( conn, srcPath->dataId );
        }
    }

    if ( srcPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "lsDataObjUtil: NULL srcPath input" );
        return USER__NULL_INPUT_ERR;
    }

    return status;
}

int
printLsStrShort( char *srcPath ) {
    char srcElement[MAX_NAME_LEN];

    getLastPathElement( srcPath, srcElement );

    printf( "  %s\n", srcPath );

    return 0;
}

int
lsDataObjUtilLong( rcComm_t *conn, char *srcPath,
                   rodsArguments_t *rodsArgs, genQueryInp_t *genQueryInp ) {
    int status;
    genQueryOut_t *genQueryOut = NULL;
    char myColl[MAX_NAME_LEN], myData[MAX_NAME_LEN];
    char condStr[MAX_NAME_LEN];
    int queryFlags;

    queryFlags = setQueryFlag( rodsArgs );
    setQueryInpForData( queryFlags, genQueryInp );
    genQueryInp->maxRows = MAX_SQL_ROWS;

    memset( myColl, 0, MAX_NAME_LEN );
    memset( myData, 0, MAX_NAME_LEN );

    if ( ( status = splitPathByKey(
                        srcPath, myColl, MAX_NAME_LEN, myData, MAX_NAME_LEN, '/' ) ) < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "setQueryInpForLong: splitPathByKey for %s error, status = %d",
                      srcPath, status );
        return status;
    }

    snprintf( condStr, MAX_NAME_LEN, "='%s'", myColl );
    addInxVal( &genQueryInp->sqlCondInp, COL_COLL_NAME, condStr );
    snprintf( condStr, MAX_NAME_LEN, "='%s'", myData );
    addInxVal( &genQueryInp->sqlCondInp, COL_DATA_NAME, condStr );

    status =  rcGenQuery( conn, genQueryInp, &genQueryOut );

    if ( status < 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            rodsLog( LOG_ERROR, "%s does not exist or user lacks access permission",
                     srcPath );
        }
        else {
            rodsLogError( LOG_ERROR, status,
                          "lsDataObjUtilLong: rcGenQuery error for %s", srcPath );
        }
        return status;
    }
    printLsLong( conn, rodsArgs, genQueryOut );

    return 0;
}

int
printLsLong( rcComm_t *conn, rodsArguments_t *rodsArgs,
             genQueryOut_t *genQueryOut ) {
    int i = 0;
    sqlResult_t *dataName = 0, *replNum = 0, *dataSize = 0, *rescName = 0, *rescHier = 0,
                 *replStatus = 0, *dataModify = 0, *dataOwnerName = 0, *dataId = 0;
    sqlResult_t *chksumStr = 0, *dataPath = 0, *dataType = 0; // JMC - backport 4636
    char *tmpDataId = 0;
    int queryFlags = 0;

    if ( genQueryOut == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    queryFlags = setQueryFlag( rodsArgs );

    if ( rodsArgs->veryLongOption == True ) {
        if ( ( chksumStr = getSqlResultByInx( genQueryOut, COL_D_DATA_CHECKSUM ) )
                == NULL ) {
            rodsLog( LOG_ERROR,
                     "printLsLong: getSqlResultByInx for COL_D_DATA_CHECKSUM failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        if ( ( dataPath = getSqlResultByInx( genQueryOut, COL_D_DATA_PATH ) )
                == NULL ) {
            rodsLog( LOG_ERROR,
                     "printLsLong: getSqlResultByInx for COL_D_DATA_PATH failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        if ( ( dataType = getSqlResultByInx( genQueryOut, COL_DATA_TYPE_NAME ) ) == NULL ) { // JMC - backport 4636

            rodsLog( LOG_ERROR,
                     "printLsLong: getSqlResultByInx for COL_DATA_TYPE_NAME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
    }

    if ( ( dataId = getSqlResultByInx( genQueryOut, COL_D_DATA_ID ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "printLsLong: getSqlResultByInx for COL_D_DATA_ID failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataName = getSqlResultByInx( genQueryOut, COL_DATA_NAME ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "printLsLong: getSqlResultByInx for COL_DATA_NAME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( replNum = getSqlResultByInx( genQueryOut, COL_DATA_REPL_NUM ) ) ==
            NULL ) {
        rodsLog( LOG_ERROR,
                 "printLsLong: getSqlResultByInx for COL_DATA_REPL_NUM failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataSize = getSqlResultByInx( genQueryOut, COL_DATA_SIZE ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "printLsLong: getSqlResultByInx for COL_DATA_SIZE failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( rescName = getSqlResultByInx( genQueryOut, COL_D_RESC_NAME ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "printLsLong: getSqlResultByInx for COL_D_RESC_NAME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( rescHier = getSqlResultByInx( genQueryOut, COL_D_RESC_HIER ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "printLsLong: getSqlResultByInx for COL_D_RESC_HIER failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( replStatus = getSqlResultByInx( genQueryOut, COL_D_REPL_STATUS ) ) ==
            NULL ) {
        rodsLog( LOG_ERROR,
                 "printLsLong: getSqlResultByInx for COL_D_REPL_STATUS failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataModify = getSqlResultByInx( genQueryOut, COL_D_MODIFY_TIME ) ) ==
            NULL ) {
        rodsLog( LOG_ERROR,
                 "printLsLong: getSqlResultByInx for COL_D_MODIFY_TIME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataOwnerName = getSqlResultByInx( genQueryOut, COL_D_OWNER_NAME ) ) ==
            NULL ) {
        rodsLog( LOG_ERROR,
                 "printLsLong: getSqlResultByInx for COL_D_OWNER_NAME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
        collEnt_t collEnt;

        bzero( &collEnt, sizeof( collEnt ) );

        collEnt.dataName = &dataName->value[dataName->len * i];
        collEnt.replNum = atoi( &replNum->value[replNum->len * i] );
        collEnt.dataSize = strtoll( &dataSize->value[dataSize->len * i], 0, 0 );
        collEnt.resource = &rescName->value[rescName->len * i];
        collEnt.resc_hier = &rescHier->value[rescHier->len * i];
        collEnt.ownerName = &dataOwnerName->value[dataOwnerName->len * i];
        collEnt.replStatus = atoi( &replStatus->value[replStatus->len * i] );
        collEnt.modifyTime = &dataModify->value[dataModify->len * i];
        if ( rodsArgs->veryLongOption == True ) {
            collEnt.chksum = &chksumStr->value[chksumStr->len * i];
            collEnt.phyPath = &dataPath->value[dataPath->len * i];
            collEnt.dataType = &dataType->value[dataType->len * i];  // JMC - backport 4636
        }

        printDataCollEntLong( &collEnt, queryFlags );

        if ( rodsArgs->accessControl == True ) {
            tmpDataId = &dataId->value[dataId->len * i];
            printDataAcl( conn, tmpDataId );
        }
    }

    return 0;
}

int
lsSpecDataObjUtilLong( rodsPath_t *srcPath, rodsArguments_t *rodsArgs ) {
    char sizeStr[NAME_LEN];
    int status;
    rodsObjStat_t *rodsObjStat = srcPath->rodsObjStat;

    snprintf( sizeStr, NAME_LEN, "%lld", rodsObjStat->objSize );
    status = printSpecLsLong( srcPath->outPath, rodsObjStat->ownerName,
                              sizeStr, rodsObjStat->modifyTime, rodsObjStat->specColl,
                              rodsArgs );

    return status;
}


int
printLsShort( rcComm_t *conn,  rodsArguments_t *rodsArgs,
              genQueryOut_t *genQueryOut ) {
    int i;

    sqlResult_t *dataName, *dataId;
    char *tmpDataName, *tmpDataId;

    if ( genQueryOut == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    if ( ( dataId = getSqlResultByInx( genQueryOut, COL_D_DATA_ID ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "printLsShort: getSqlResultByInx for COL_D_DATA_ID failed" ); // JMC - backport 4516
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataName = getSqlResultByInx( genQueryOut, COL_DATA_NAME ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "printLsShort: getSqlResultByInx for COL_DATA_NAME failed" ); // JMC - backport 4516
        return UNMATCHED_KEY_OR_INDEX;
    }

    for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
        tmpDataName = &dataName->value[dataName->len * i];
        printLsStrShort( tmpDataName );

        if ( rodsArgs->accessControl == True ) {
            tmpDataId = &dataId->value[dataId->len * i];
            printDataAcl( conn, tmpDataId );
        }
    }

    return 0;
}

int
initCondForLs( genQueryInp_t *genQueryInp ) {
    if ( genQueryInp == NULL ) {
        rodsLog( LOG_ERROR,
                 "initCondForLs: NULL genQueryInp input" );
        return USER__NULL_INPUT_ERR;
    }

    memset( genQueryInp, 0, sizeof( genQueryInp_t ) );

    return 0;
}

int
lsCollUtil( rcComm_t *conn, rodsPath_t *srcPath, rodsEnv *myRodsEnv,
            rodsArguments_t *rodsArgs ) {
    int savedStatus = 0;
    char *srcColl;
    int status;
    int queryFlags;
    collHandle_t collHandle;
    collEnt_t collEnt;

    if ( srcPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "lsCollUtil: NULL srcPath input" );
        return USER__NULL_INPUT_ERR;
    }

    srcColl = srcPath->outPath;

    /* print this collection */
    printf( "%s:\n", srcColl );

    if ( rodsArgs->accessControl == True ) {
        printCollAcl( conn, srcColl );
        printCollInheritance( conn, srcColl );
    }

    queryFlags = DATA_QUERY_FIRST_FG;
    if ( rodsArgs->veryLongOption == True ) {
        /* need to check veryLongOption first since it will have both
         * veryLongOption and longOption flags on. */
        queryFlags = queryFlags | VERY_LONG_METADATA_FG | NO_TRIM_REPL_FG;
    }
    else if ( rodsArgs->longOption == True ) {
        queryFlags |= LONG_METADATA_FG | NO_TRIM_REPL_FG;;
    }

    status = rclOpenCollection( conn, srcColl, queryFlags,
                                &collHandle );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "lsCollUtil: rclOpenCollection of %s error. status = %d",
                 srcColl, status );
        return status;
    }
    while ( ( status = rclReadCollection( conn, &collHandle, &collEnt ) ) >= 0 ) {
        if ( collEnt.objType == DATA_OBJ_T ) {
            // =-=-=-=-=-=-=-
            // JMC - backport 4536
            if ( rodsArgs->bundle == True ) {
                char myPath[MAX_NAME_LEN];
                snprintf( myPath, MAX_NAME_LEN, "%s/%s", collEnt.collName,
                          collEnt.dataName );
                lsSubfilesInBundle( conn, myPath );
            }
            else {
                printDataCollEnt( &collEnt, queryFlags );
                if ( rodsArgs->accessControl == True ) {
                    printDataAcl( conn, collEnt.dataId );
                }
                // =-=-=-=-=-=-=-
            }
        }
        else {
            printCollCollEnt( &collEnt, queryFlags );
            /* need to drill down */
            if ( rodsArgs->recursive == True &&
                    strcmp( collEnt.collName, "/" ) != 0 ) {
                rodsPath_t tmpPath;
                memset( &tmpPath, 0, sizeof( tmpPath ) );
                rstrcpy( tmpPath.outPath, collEnt.collName, MAX_NAME_LEN );
                status = lsCollUtil( conn, &tmpPath, myRodsEnv, rodsArgs );
                if ( status < 0 ) {
                    savedStatus = status;
                }
            }
        }
    }
    rclCloseCollection( &collHandle );
    if ( savedStatus < 0 && savedStatus != CAT_NO_ROWS_FOUND ) {
        return savedStatus;
    }
    else {
        return 0;
    }
}

int
printDataCollEnt( collEnt_t *collEnt, int flags ) {
    if ( ( flags & ( LONG_METADATA_FG | VERY_LONG_METADATA_FG ) ) == 0 ) {
        /* short */
        printLsStrShort( collEnt->dataName );
    }
    else {
        printDataCollEntLong( collEnt, flags );
    }
    return 0;
}

int
printDataCollEntLong( collEnt_t *collEnt, int flags ) {
    char *tmpReplStatus;
    char localTimeModify[TIME_LEN];
    char typeStr[NAME_LEN];

    if ( collEnt->replStatus == OLD_COPY ) {
        tmpReplStatus = " ";
    }
    else {
        tmpReplStatus = "&";
    }

    getLocalTimeFromRodsTime( collEnt->modifyTime, localTimeModify );
    if ( collEnt->specColl.collClass == NO_SPEC_COLL ||
            collEnt->specColl.collClass == LINKED_COLL ) {
        printf( "  %-12.12s %6d %s %12lld %16.16s %s %s\n",
                collEnt->ownerName, collEnt->replNum, collEnt->resc_hier,
                collEnt->dataSize, localTimeModify, tmpReplStatus, collEnt->dataName );
    }
    else {
        getSpecCollTypeStr( &collEnt->specColl, typeStr );
        printf( "  %-12.12s %6.6s %s %12lld %16.16s %s %s\n",
                collEnt->ownerName, typeStr, collEnt->resc_hier,
                collEnt->dataSize, localTimeModify, tmpReplStatus, collEnt->dataName );
    }


    if ( ( flags & VERY_LONG_METADATA_FG ) != 0 ) {
        printf( "    %s    %s    %s\n", collEnt->chksum, collEnt->dataType, collEnt->phyPath ); // JMC - backport 4636

    }
    return 0;
}

int
printCollCollEnt( collEnt_t *collEnt, int flags ) {
    char typeStr[NAME_LEN];

    typeStr[0] = '\0';
    if ( collEnt->specColl.collClass != NO_SPEC_COLL ) {
        getSpecCollTypeStr( &collEnt->specColl, typeStr );
    }
    if ( ( flags & LONG_METADATA_FG ) != 0 ) {
        printf( "  C- %s  %s\n", collEnt->collName, typeStr );
    }
    else if ( ( flags & VERY_LONG_METADATA_FG ) != 0 ) {
        if ( collEnt->specColl.collClass == NO_SPEC_COLL ) {
            printf( "  C- %s\n", collEnt->collName );
        }
        else {
            if ( collEnt->specColl.collClass == MOUNTED_COLL ||
                    collEnt->specColl.collClass == LINKED_COLL ) {
                printf( "  C- %s  %6.6s  %s  %s   %s\n",
                        collEnt->collName, typeStr,
                        collEnt->specColl.objPath, collEnt->specColl.phyPath,
                        collEnt->specColl.rescHier );
            }
            else {
                printf( "  C- %s  %6.6s  %s  %s;;;%s;;;%d\n",
                        collEnt->collName, typeStr,
                        collEnt->specColl.objPath, collEnt->specColl.cacheDir,
                        collEnt->specColl.rescHier, collEnt->specColl.cacheDirty );
            }
        }
    }
    else {
        /* short */
        printf( "  C- %s\n", collEnt->collName );
    }
    return 0;
}

int
printSpecLsLong( char *objPath, char *ownerName, char *objSize,
                 char *modifyTime, specColl_t *specColl, rodsArguments_t *rodsArgs ) {
    char srcElement[MAX_NAME_LEN];
    collEnt_t collEnt;
    int queryFlags;

    bzero( &collEnt, sizeof( collEnt ) );

    getLastPathElement( objPath, srcElement );
    collEnt.dataName = objPath;
    collEnt.ownerName = ownerName;
    collEnt.dataSize = strtoll( objSize, 0, 0 );
    collEnt.resource = specColl->resource;
    collEnt.resc_hier = specColl->rescHier;
    collEnt.modifyTime = modifyTime;
    collEnt.specColl = *specColl;
    collEnt.objType = DATA_OBJ_T;
    queryFlags = setQueryFlag( rodsArgs );

    printDataCollEntLong( &collEnt, queryFlags );
    return 0;
}

int
printDataAcl( rcComm_t *conn, char *dataId ) {
    genQueryOut_t *genQueryOut = NULL;
    int status;
    int i;
    sqlResult_t *userName, *userZone, *dataAccess;
    char *userNameStr, *userZoneStr, *dataAccessStr;

    status = queryDataObjAcl( conn, dataId, zoneHint, &genQueryOut ); // JMC - backport 4516

    printf( "        ACL - " );

    if ( status < 0 ) {
        printf( "\n" );
        return status;
    }

    if ( ( userName = getSqlResultByInx( genQueryOut, COL_USER_NAME ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "printDataAcl: getSqlResultByInx for COL_USER_NAME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( userZone = getSqlResultByInx( genQueryOut, COL_USER_ZONE ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "printDataAcl: getSqlResultByInx for COL_USER_ZONE failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataAccess = getSqlResultByInx( genQueryOut, COL_DATA_ACCESS_NAME ) )
            == NULL ) {
        rodsLog( LOG_ERROR,
                 "printDataAcl: getSqlResultByInx for COL_DATA_ACCESS_NAME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
        userNameStr = &userName->value[userName->len * i];
        userZoneStr = &userZone->value[userZone->len * i];
        dataAccessStr = &dataAccess->value[dataAccess->len * i];
        printf( "%s#%s:%s   ", userNameStr, userZoneStr, dataAccessStr );
    }

    printf( "\n" );

    freeGenQueryOut( &genQueryOut );

    return status;
}

int
printCollAcl( rcComm_t *conn, char *collName ) {
    genQueryOut_t *genQueryOut = NULL;
    int status;
    int i;
    sqlResult_t *userName, *userZone, *dataAccess ;
    char *userNameStr, *userZoneStr, *dataAccessStr;

    printf( "        ACL - " );
    /* First try a specific-query.  If this is defined, it should be
        used as it returns the group names without expanding them to
        individual users and this is important to some sites (iPlant,
        in particular).  If this fails, go on the the general-query.
     */
    status = queryCollAclSpecific( conn, collName, zoneHint, &genQueryOut );
    if ( status >= 0 ) {
        int i, j;
        for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
            char *tResult[10];
            char empty = 0;
            char typeStr[8];
            tResult[3] = 0;

            for ( j = 0; j < 10; j++ ) {
                tResult[j] = &empty;
                if ( j < genQueryOut->attriCnt ) {
                    tResult[j] = genQueryOut->sqlResult[j].value;
                    tResult[j] += i * genQueryOut->sqlResult[j].len;
                }
            }
            typeStr[0] = '\0';
            if ( tResult[3] != 0 && strncmp( tResult[3], "rodsgroup", 9 ) == 0 ) {
                strncpy( typeStr, "g:", 3 );
            }
            printf( "%s%s#%s:%s   ",  typeStr, tResult[0], tResult[1], tResult[2] );

        }
        printf( "\n" );
        freeGenQueryOut( &genQueryOut );
        return status;
    }

    status = queryCollAcl( conn, collName, zoneHint, &genQueryOut ); // JMC - backport 4516

    if ( status < 0 ) {
        printf( "\n" );
        freeGenQueryOut( &genQueryOut );
        return status;
    }

    if ( ( userName = getSqlResultByInx( genQueryOut, COL_COLL_USER_NAME ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "printCollAcl: getSqlResultByInx for COL_COLL_USER_NAME failed" );
        freeGenQueryOut( &genQueryOut );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( userZone = getSqlResultByInx( genQueryOut, COL_COLL_USER_ZONE ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "printCollAcl: getSqlResultByInx for COL_COLL_USER_ZONE failed" );
        freeGenQueryOut( &genQueryOut );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataAccess = getSqlResultByInx( genQueryOut, COL_COLL_ACCESS_NAME ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "printCollAcl: getSqlResultByInx for COL_COLL_ACCESS_NAME failed" );
        freeGenQueryOut( &genQueryOut );
        return UNMATCHED_KEY_OR_INDEX;
    }

    for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
        userNameStr = &userName->value[userName->len * i];
        userZoneStr = &userZone->value[userZone->len * i];
        dataAccessStr = &dataAccess->value[dataAccess->len * i];
        printf( "%s#%s:%s   ",  userNameStr, userZoneStr, dataAccessStr );
    }

    printf( "\n" );

    freeGenQueryOut( &genQueryOut );

    return status;
}

int
printCollInheritance( rcComm_t *conn, char *collName ) {
    genQueryOut_t *genQueryOut = NULL;
    int status;
    sqlResult_t *inheritResult;
    char *inheritStr;

    status = queryCollInheritance( conn, collName, &genQueryOut );

    if ( status < 0 ) {
        freeGenQueryOut( &genQueryOut );
        return status;
    }

    if ( ( inheritResult = getSqlResultByInx( genQueryOut, COL_COLL_INHERITANCE ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "printCollInheritance: getSqlResultByInx for COL_COLL_INHERITANCE failed" );
        freeGenQueryOut( &genQueryOut );
        return UNMATCHED_KEY_OR_INDEX;
    }

    inheritStr = &inheritResult->value[0];
    printf( "        Inheritance - " );
    if ( *inheritStr == '1' ) {
        printf( "Enabled\n" );
    }
    else {
        printf( "Disabled\n" );
    }

    freeGenQueryOut( &genQueryOut );

    return status;
}

void
printCollOrDir( char *myName, objType_t myType, rodsArguments_t *rodsArgs,
                specColl_t *specColl ) {
    char *typeStr;

    if ( rodsArgs->verbose == False ) {
        return;
    }

    if ( myType == COLL_OBJ_T ) {
        typeStr = "C";
    }
    else {
        typeStr = "D";
    }

    if ( specColl != NULL ) {
        char objType[NAME_LEN];
        int status;
        status = getSpecCollTypeStr( specColl, objType );
        if ( status < 0 ) {
            objType[0] = '\0';
        }
        fprintf( stdout, "%s- %s    %-5.5s :\n", typeStr, myName, objType );
    }
    else {
        fprintf( stdout, "%s- %s :\n", typeStr, myName );
    }
}

int
lsSubfilesInBundle( rcComm_t *conn, char *srcPath ) {
    int i, status;
    genQueryOut_t *genQueryOut = NULL;
    genQueryInp_t genQueryInp;
    char condStr[MAX_NAME_LEN];
    sqlResult_t *dataName, *collection, *dataSize;
    char *dataNameStr, *collectionStr, *dataSizeStr;
    int continueInx = 1;


    fprintf( stdout, "Bundle file: %s\n", srcPath );
    fprintf( stdout, "Subfiles:\n" );
    bzero( &genQueryInp, sizeof( genQueryInp ) );
    genQueryInp.maxRows = MAX_SQL_ROWS;

    snprintf( condStr, MAX_NAME_LEN, "='%s'", srcPath );
    addInxVal( &genQueryInp.sqlCondInp, COL_D_DATA_PATH, condStr );
    snprintf( condStr, MAX_NAME_LEN, "='%s'", BUNDLE_RESC_CLASS );
    addInxVal( &genQueryInp.sqlCondInp, COL_R_CLASS_NAME, condStr );
    addKeyVal( &genQueryInp.condInput, ZONE_KW, srcPath );

    addInxIval( &genQueryInp.selectInp, COL_COLL_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DATA_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DATA_SIZE, 1 );

    while ( continueInx > 0 ) {
        status =  rcGenQuery( conn, &genQueryInp, &genQueryOut );

        if ( status < 0 ) {
            if ( status != CAT_NO_ROWS_FOUND ) {
                rodsLogError( LOG_ERROR, status,
                              "lsSubfilesInBundle: rsGenQuery error for %s",
                              srcPath );
            }
            clearGenQueryInp( &genQueryInp );
            return status;
        }

        if ( ( collection = getSqlResultByInx( genQueryOut, COL_COLL_NAME ) ) ==
                NULL ) {
            rodsLog( LOG_ERROR,
                     "lsSubfilesInBundle: getSqlResultByInx for COL_COLL_NAME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        if ( ( dataName = getSqlResultByInx( genQueryOut, COL_DATA_NAME ) ) ==
                NULL ) {
            rodsLog( LOG_ERROR,
                     "lsSubfilesInBundle: getSqlResultByInx for COL_DATA_NAME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        if ( ( dataSize = getSqlResultByInx( genQueryOut, COL_DATA_SIZE ) ) ==
                NULL ) {
            rodsLog( LOG_ERROR,
                     "lsSubfilesInBundle: getSqlResultByInx for COL_DATA_SIZE failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
            collectionStr = &collection->value[collection->len * i];
            dataNameStr = &dataName->value[dataName->len * i];
            dataSizeStr = &dataSize->value[dataSize->len * i];
            fprintf( stdout, "    %s/%s    %s\n",
                     collectionStr, dataNameStr, dataSizeStr );
        }

        continueInx = genQueryInp.continueInx =
                          genQueryOut->continueInx;
        freeGenQueryOut( &genQueryOut );
    }
    clearGenQueryInp( &genQueryInp );
    return status;
}





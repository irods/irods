#include "rcMisc.h"
#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "miscUtil.h"
#include "rodsLog.h"
#include "chksumUtil.h"
#include "rcGlobalExtern.h"

#include <sys/time.h>

#include "json.hpp"

#include <cstdio>
#include <string>

static int ChksumCnt = 0;
static int FailedChksumCnt = 0;

int chksumUtil(rcComm_t* conn,
               rodsEnv* myRodsEnv,
               rodsArguments_t* myRodsArgs,
               rodsPathInp_t* rodsPathInp)
{
    if ( rodsPathInp == nullptr ) {
        return USER__NULL_INPUT_ERR;
    }

    collInp_t collInp;
    dataObjInp_t dataObjInp;
    int savedStatus = initCondForChksum( myRodsArgs, &dataObjInp, &collInp );

    if ( savedStatus < 0 ) {
        return savedStatus;
    }

    for ( int i = 0; i < rodsPathInp->numSrc; i++ ) {
        if ( rodsPathInp->srcPath[i].objType == UNKNOWN_OBJ_T ) {
            getRodsObjType( conn, &rodsPathInp->srcPath[i] );
            if ( rodsPathInp->srcPath[i].objState == NOT_EXIST_ST ) {
                rodsLog( LOG_ERROR, "chksumUtil: srcPath %s does not exist", rodsPathInp->srcPath[i].outPath );
                savedStatus = USER_INPUT_PATH_ERR;
                continue;
            }
        }

        int status = 0;
        if ( rodsPathInp->srcPath[i].objType == DATA_OBJ_T ) {
            rmKeyVal( &dataObjInp.condInput, TRANSLATED_PATH_KW );
            status = chksumDataObjUtil( conn, rodsPathInp->srcPath[i].outPath, myRodsArgs, &dataObjInp );
        }
        else if ( rodsPathInp->srcPath[i].objType ==  COLL_OBJ_T ) {
            addKeyVal( &dataObjInp.condInput, TRANSLATED_PATH_KW, "" );
            status = chksumCollUtil( conn, rodsPathInp->srcPath[i].outPath, myRodsEnv, myRodsArgs, &dataObjInp, &collInp );
        }
        else {
            /* should not be here */
            rodsLog( LOG_ERROR, "chksumUtil: invalid chksum objType %d for %s",
                     rodsPathInp->srcPath[i].objType, rodsPathInp->srcPath[i].outPath );
            return USER_INPUT_PATH_ERR;
        }

        /* XXXX may need to return a global status */
        if (status < 0 && status != CAT_NO_ROWS_FOUND) {
            rodsLogError( LOG_ERROR, status, "chksumUtil: chksum error for %s, status = %d",
                          rodsPathInp->srcPath[i].outPath, status );
            savedStatus = status;
        }
    }

    return savedStatus;
}

int chksumDataObjUtil(rcComm_t* conn,
                      char* srcPath,
                      rodsArguments_t* rodsArgs,
                      dataObjInp_t* dataObjInp)
{
    if (!srcPath) {
        rodsLog( LOG_ERROR, "chksumDataObjUtil: NULL srcPath input" );
        return USER__NULL_INPUT_ERR;
    }

    timeval startTime;

    if (rodsArgs->verbose == True) {
        gettimeofday(&startTime, (struct timezone*) nullptr);
    }

    rstrcpy(dataObjInp->objPath, srcPath, MAX_NAME_LEN);

    char* chksumStr = nullptr;
    int status = rcDataObjChksum(conn, dataObjInp, &chksumStr);

    ChksumCnt++;

    if (status < 0) {
        if (CHECK_VERIFICATION_RESULTS == status) {
            try {
                for (auto&& vr : nlohmann::json::parse(chksumStr)) {
                    std::printf("%s\n", vr.at("message").get_ref<const std::string&>().c_str());
                }
            }
            catch (const std::exception& e) {
                std::printf("ERROR: %s\n", e.what());
            }
        }

        FailedChksumCnt++;
        rodsLogError(LOG_ERROR, status, "chksumDataObjUtil: rcDataObjChksum error for %s", dataObjInp->objPath);
        printErrorStack(conn->rError);
        freeRError(conn->rError);
        conn->rError = nullptr;
        return status;
    }

    char myDir[MAX_NAME_LEN];
    char myFile[MAX_NAME_LEN];
    status = splitPathByKey(dataObjInp->objPath, myDir, MAX_NAME_LEN, myFile, MAX_NAME_LEN, '/');

    if (0 == status && rodsArgs->silent == False) {
        if (chksumStr) {
            printf("    %s    %s\n", myFile, chksumStr);
            free(chksumStr);
        }

        if (rodsArgs->verbose == True) {
            timeval endTime;
            gettimeofday(&endTime, (struct timezone*) nullptr);
            printTiming(conn, dataObjInp->objPath, -1, nullptr, &startTime, &endTime);
        }
    }

    return status;
}

int chksumCollUtil(rcComm_t* conn,
                   char* srcColl,
                   rodsEnv* myRodsEnv,
                   rodsArguments_t *rodsArgs,
                   dataObjInp_t *dataObjInp,
                   collInp_t *collInp)
{
    if (!srcColl) {
        rodsLog( LOG_ERROR, "chksumCollUtil: NULL srcColl input" );
        return USER__NULL_INPUT_ERR;
    }

    if (rodsArgs->silent == False) {
        fprintf(stdout, "C- %s:\n", srcColl);
    }

    int savedStatus = 0;
    char srcChildPath[MAX_NAME_LEN];
    collHandle_t collHandle{};
    collEnt_t collEnt;
    int queryFlags = 0;

    if ( rodsArgs->resource == True ) {
        queryFlags = INCLUDE_CONDINPUT_IN_QUERY;
        replKeyVal( &dataObjInp->condInput, &collHandle.dataObjInp.condInput );
    }

    int status = rclOpenCollection( conn, srcColl, queryFlags, &collHandle );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "chksumCollUtil: rclOpenCollection of %s error. status = %d", srcColl, status );
        return status;
    }

    if (collHandle.rodsObjStat->specColl &&
        collHandle.rodsObjStat->specColl->collClass != LINKED_COLL)
    {
        /* no trim for mounted coll */
        rclCloseCollection( &collHandle );
        return 0;
    }

    // Holds the number of collections seen during the first iteration of
    // the collection.
    int collections_left_to_handle = 0;

    // Process all data objects directly under this collection first. Collections will
    // be handled separately. This is required so that the output is clean and organized.
    while ( ( status = rclReadCollection( conn, &collHandle, &collEnt ) ) >= 0 ) {
        if ( collEnt.objType == DATA_OBJ_T ) {
            snprintf( srcChildPath, MAX_NAME_LEN, "%s/%s", collEnt.collName, collEnt.dataName );
            // screen unnecessary call to chksumDataObjUtil if user input a resource.
            int status = chksumDataObjUtil( conn, srcChildPath, rodsArgs, dataObjInp );
            if ( status < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "chksumCollUtil:chksumDataObjU failed for %s.stat = %d",
                              srcChildPath, status );
                // need to set global error here.
                savedStatus = status;
            }
        }
        else if ( collEnt.objType == COLL_OBJ_T ) {
            ++collections_left_to_handle;
        }
    }

    rclCloseCollection( &collHandle );

    // Process any collections seen during the first iteration.
    if (collections_left_to_handle > 0) {
        clearKeyVal(&collHandle.dataObjInp.condInput);
        replKeyVal( &dataObjInp->condInput, &collHandle.dataObjInp.condInput );

        // Re-open the collection and handle only the collections.
        status = rclOpenCollection( conn, srcColl, queryFlags, &collHandle );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR, "chksumCollUtil: rclOpenCollection of %s error. status = %d", srcColl, status );
            return status;
        }

        while ( collections_left_to_handle > 0 && ( status = rclReadCollection( conn, &collHandle, &collEnt ) ) >= 0 ) {
            if ( collEnt.objType == COLL_OBJ_T ) {
                --collections_left_to_handle;

                dataObjInp_t childDataObjInp;
                childDataObjInp = *dataObjInp;

                if ( collEnt.specColl.collClass != NO_SPEC_COLL ) {
                    childDataObjInp.specColl = &collEnt.specColl;
                }
                else {
                    childDataObjInp.specColl = nullptr;
                }

                int status = chksumCollUtil( conn, collEnt.collName, myRodsEnv, rodsArgs, &childDataObjInp, collInp );
                if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
                    return status;
                }
            }
        }
    }

    if ( savedStatus < 0 ) {
        return savedStatus;
    }

    if ( status == CAT_NO_ROWS_FOUND ) {
        return 0;
    }

    return status;
}

int initCondForChksum(rodsArguments_t* rodsArgs, dataObjInp_t* dataObjInp, collInp_t* collInp)
{
    if ( dataObjInp == nullptr ) {
        rodsLog( LOG_ERROR, "initCondForChksum: NULL dataObjInp input" );
        return USER__NULL_INPUT_ERR;
    }

    memset( dataObjInp, 0, sizeof( dataObjInp_t ) );
    memset( collInp, 0, sizeof( collInp_t ) );

    if ( rodsArgs == nullptr ) {
        return 0;
    }

    if ( rodsArgs->force == True && rodsArgs->verifyChecksum ) {
        rodsLog( LOG_ERROR, "initCondForChksum: the 'K' and 'f' option cannot be used together" );
        return USER_OPTION_INPUT_ERR;
    }

    if (rodsArgs->silent == True) {
        if (rodsArgs->verifyChecksum == True) {
            rodsLog(LOG_ERROR, "initCondForChksum: the 'K' and 'silent' option cannot be used together");
            return USER_OPTION_INPUT_ERR;
        }
        else if (rodsArgs->verify == True) {
            rodsLog(LOG_ERROR, "initCondForChksum: the 'verify' and 'silent' option cannot be used together");
            return USER_OPTION_INPUT_ERR;
        }
    }

    if ( rodsArgs->all == True && rodsArgs->replNum == True ) {
        rodsLog( LOG_ERROR, "initCondForChksum: the 'n' and 'a' option cannot be used together" );
        return USER_OPTION_INPUT_ERR;
    }

    if ( rodsArgs->all == True && rodsArgs->resource == True ) {
        rodsLog( LOG_ERROR, "initCondForChksum: the 'R' and 'a' option cannot be used together" );
        return USER_OPTION_INPUT_ERR;
    }

    if ( rodsArgs->replNum == True && rodsArgs->resource == True ) {
        rodsLog( LOG_ERROR, "initCondForChksum: the 'n' and 'R' option cannot be used together" );
        return USER_OPTION_INPUT_ERR;
    }

    if ( rodsArgs->admin == True ) {
        addKeyVal( &dataObjInp->condInput, ADMIN_KW, "" );
    }

    if ( rodsArgs->force == True ) {
        addKeyVal( &dataObjInp->condInput, FORCE_CHKSUM_KW, "" );
        addKeyVal( &collInp->condInput, FORCE_CHKSUM_KW, "" );
    }

    if ( rodsArgs->all == True ) {
        addKeyVal( &dataObjInp->condInput, CHKSUM_ALL_KW, "" );
        addKeyVal( &collInp->condInput, CHKSUM_ALL_KW, "" );
    }

    if ( rodsArgs->verifyChecksum == True || rodsArgs->verify == True ) {
        addKeyVal( &dataObjInp->condInput, VERIFY_CHKSUM_KW, "" );
        addKeyVal( &collInp->condInput, VERIFY_CHKSUM_KW, "" );
    }

    if ( rodsArgs->replNum == True ) {
        addKeyVal( &dataObjInp->condInput, REPL_NUM_KW,
                   rodsArgs->replNumValue );
    }

    if ( rodsArgs->resource == True ) {
        addKeyVal( &dataObjInp->condInput, RESC_NAME_KW,
                   rodsArgs->resourceString );
    }

    if (rodsArgs->noCompute == True) {
        addKeyVal(&dataObjInp->condInput, NO_COMPUTE_KW, "");
        addKeyVal(&collInp->condInput, NO_COMPUTE_KW, "");
    }

    /* XXXXX need to add -u register cond */

    dataObjInp->openFlags = O_RDONLY;

    return 0;
}


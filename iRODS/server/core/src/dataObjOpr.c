/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* dataObjOpr.c - data object operations */

#ifndef windows_platform
// JMC #include <pthread.h> 
#include <sys/types.h>
#include <sys/wait.h>
#endif
#include "dataObjOpr.h"
#include "objMetaOpr.h"
#include "resource.h"
#include "collection.h"
#include "specColl.h"
#include "physPath.h"
#include "modDataObjMeta.h"
#include "ruleExecSubmit.h"
#include "ruleExecDel.h"
#include "genQuery.h"
#include "icatHighLevelRoutines.h"
#include "reSysDataObjOpr.h"
#include "miscUtil.h"
#include "rodsClient.h"
#include "rsIcatOpr.h"

// =-=-=-=-=-=-=-
// eirods include
#include "eirods_resource_backport.h"
#include "eirods_log.h"
#include "eirods_stacktrace.h"

int
getDataObjInfo( 
    rsComm_t*       rsComm, 
    dataObjInp_t*   dataObjInp, 
    dataObjInfo_t** dataObjInfoHead,
    char*           accessPerm, 
    int             ignoreCondInput ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    int i, status;
    dataObjInfo_t *dataObjInfo;
    char condStr[MAX_NAME_LEN]; 
    char *tmpStr;
    sqlResult_t *dataId, *collId, *replNum, *version, *dataType, *dataSize,
        *rescGroupName, *rescName, *hierString, *filePath, *dataOwnerName, *dataOwnerZone,
        *replStatus, *statusString, *chksum, *dataExpiry, *dataMapId, 
        *dataComments, *dataCreate, *dataModify, *dataMode, *dataName, *collName;
    char *tmpDataId, *tmpCollId, *tmpReplNum, *tmpVersion, *tmpDataType, 
        *tmpDataSize, *tmpRescGroupName, *tmpRescName, *tmpHierString, *tmpFilePath, 
        *tmpDataOwnerName, *tmpDataOwnerZone, *tmpReplStatus, *tmpStatusString, 
        *tmpChksum, *tmpDataExpiry, *tmpDataMapId, *tmpDataComments, 
        *tmpDataCreate, *tmpDataModify, *tmpDataMode, *tmpDataName, *tmpCollName;
    char accStr[LONG_NAME_LEN];
    int qcondCnt;
    int writeFlag;

    *dataObjInfoHead = NULL;

    qcondCnt = initDataObjInfoQuery (dataObjInp, &genQueryInp, 
                                     ignoreCondInput);

    if (qcondCnt < 0) {
        return (qcondCnt);
    }

    /* need to do RESC_NAME_KW here because not all query need this */
    if (ignoreCondInput == 0 && (tmpStr =
                                 getValByKey (&dataObjInp->condInput, RESC_NAME_KW)) != NULL) {
        snprintf (condStr, NAME_LEN, "='%s'", tmpStr);
        addInxVal (&genQueryInp.sqlCondInp, COL_D_RESC_NAME, condStr);
        qcondCnt++;
    }

    /* need to do RESC_HIER_STR_KW here because not all query need this */
    if (false && ignoreCondInput == 0 &&
        (tmpStr = getValByKey (&dataObjInp->condInput, RESC_HIER_STR_KW)) != NULL) {
        snprintf (condStr, MAX_NAME_LEN, "='%s'", tmpStr);
        addInxVal (&genQueryInp.sqlCondInp, COL_D_RESC_HIER, condStr);
        qcondCnt++;
    }

    addInxIval (&genQueryInp.selectInp, COL_D_DATA_ID, 1);
    addInxIval (&genQueryInp.selectInp, COL_DATA_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_COLL_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_D_COLL_ID, 1);
    addInxIval (&genQueryInp.selectInp, COL_DATA_REPL_NUM, 1);
    addInxIval (&genQueryInp.selectInp, COL_DATA_VERSION, 1);
    addInxIval (&genQueryInp.selectInp, COL_DATA_TYPE_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_DATA_SIZE, 1);
    addInxIval (&genQueryInp.selectInp, COL_D_RESC_GROUP_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_D_RESC_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_D_DATA_PATH, 1);
    addInxIval (&genQueryInp.selectInp, COL_D_OWNER_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_D_OWNER_ZONE, 1);
    addInxIval (&genQueryInp.selectInp, COL_D_REPL_STATUS, 1);
    addInxIval (&genQueryInp.selectInp, COL_D_DATA_STATUS, 1);
    addInxIval (&genQueryInp.selectInp, COL_D_DATA_CHECKSUM, 1);
    addInxIval (&genQueryInp.selectInp, COL_D_EXPIRY, 1);
    addInxIval (&genQueryInp.selectInp, COL_D_MAP_ID, 1);
    addInxIval (&genQueryInp.selectInp, COL_D_COMMENTS, 1);
    addInxIval (&genQueryInp.selectInp, COL_D_CREATE_TIME, 1);
    addInxIval (&genQueryInp.selectInp, COL_D_MODIFY_TIME, 1);
    addInxIval (&genQueryInp.selectInp, COL_DATA_MODE, 1);
    addInxIval (&genQueryInp.selectInp, COL_D_RESC_HIER, 1);

    if (accessPerm != NULL) {
        snprintf (accStr, LONG_NAME_LEN, "%s", rsComm->clientUser.userName);
        addKeyVal (&genQueryInp.condInput, USER_NAME_CLIENT_KW, accStr);

        snprintf (accStr, LONG_NAME_LEN, "%s", rsComm->clientUser.rodsZone);
        addKeyVal (&genQueryInp.condInput, RODS_ZONE_CLIENT_KW, accStr);

        snprintf (accStr, LONG_NAME_LEN, "%s", accessPerm);
        addKeyVal (&genQueryInp.condInput, ACCESS_PERMISSION_KW, accStr);
    }

    genQueryInp.maxRows = MAX_SQL_ROWS;

    status =  rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

    clearGenQueryInp (&genQueryInp);

    if (status < 0) {
        if (status !=CAT_NO_ROWS_FOUND) {
            rodsLog (LOG_NOTICE,
                     "getDataObjInfo: rsGenQuery error, status = %d",
                     status);
        }
        return (status);
    }

    if (genQueryOut == NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: NULL genQueryOut");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    if ((dataOwnerName =
         getSqlResultByInx (genQueryOut, COL_D_OWNER_NAME)) == NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_D_OWNER_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataName =
         getSqlResultByInx (genQueryOut, COL_DATA_NAME)) == NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_DATA_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((collName =
         getSqlResultByInx (genQueryOut, COL_COLL_NAME)) == NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_COLL_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataId = getSqlResultByInx (genQueryOut, COL_D_DATA_ID)) == NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_D_DATA_ID failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((collId = getSqlResultByInx (genQueryOut, COL_D_COLL_ID)) == NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_D_COLL_ID failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((replNum = getSqlResultByInx (genQueryOut, COL_DATA_REPL_NUM)) == 
        NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_DATA_REPL_NUM failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((version = getSqlResultByInx (genQueryOut, COL_DATA_VERSION)) == 
        NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_DATA_VERSION failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataType = getSqlResultByInx (genQueryOut, COL_DATA_TYPE_NAME)) == 
        NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_DATA_TYPE_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataSize = getSqlResultByInx (genQueryOut, COL_DATA_SIZE)) == NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_DATA_SIZE failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((rescGroupName = 
         getSqlResultByInx ( genQueryOut, COL_D_RESC_GROUP_NAME)) == NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo:getSqlResultByInx for COL_D_RESC_GROUP_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((rescName = getSqlResultByInx (genQueryOut, COL_D_RESC_NAME)) == 
        NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_D_RESC_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((hierString = getSqlResultByInx (genQueryOut, COL_D_RESC_HIER)) == 
        NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_D_RESC_HIER failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((filePath = getSqlResultByInx (genQueryOut, COL_D_DATA_PATH)) == 
        NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_D_DATA_PATH failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataOwnerZone = 
         getSqlResultByInx (genQueryOut, COL_D_OWNER_ZONE)) == NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_D_OWNER_ZONE failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((replStatus = 
         getSqlResultByInx (genQueryOut, COL_D_REPL_STATUS)) == NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_D_REPL_STATUS failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((statusString = 
         getSqlResultByInx (genQueryOut, COL_D_DATA_STATUS)) == NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_D_DATA_STATUS failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((chksum = 
         getSqlResultByInx (genQueryOut, COL_D_DATA_CHECKSUM)) == NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_D_DATA_CHECKSUM failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataExpiry = 
         getSqlResultByInx (genQueryOut, COL_D_EXPIRY)) == NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_D_EXPIRY failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataMapId =
         getSqlResultByInx (genQueryOut, COL_D_MAP_ID)) == NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_D_MAP_ID failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataComments = 
         getSqlResultByInx (genQueryOut, COL_D_COMMENTS)) == NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_D_COMMENTS failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataCreate = 
         getSqlResultByInx (genQueryOut, COL_D_CREATE_TIME)) == NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_D_CREATE_TIME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataModify =
         getSqlResultByInx (genQueryOut, COL_D_MODIFY_TIME)) == NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_D_MODIFY_TIME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataMode =
         getSqlResultByInx (genQueryOut, COL_DATA_MODE)) == NULL) {
        rodsLog (LOG_NOTICE,
                 "getDataObjInfo: getSqlResultByInx for COL_DATA_MODE failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    writeFlag = getWriteFlag (dataObjInp->openFlags);

    for (i = 0;i < genQueryOut->rowCnt; i++) {
        dataObjInfo = (dataObjInfo_t *) malloc (sizeof (dataObjInfo_t));
        memset (dataObjInfo, 0, sizeof (dataObjInfo_t));

        tmpDataId = &dataId->value[dataId->len * i];
        tmpCollId = &collId->value[collId->len * i];
        tmpReplNum = &replNum->value[replNum->len * i];
        tmpVersion = &version->value[version->len * i];
        tmpDataType = &dataType->value[dataType->len * i];
        tmpDataSize = &dataSize->value[dataSize->len * i];
        tmpRescGroupName = &rescGroupName->value[rescGroupName->len * i];
        tmpRescName = &rescName->value[rescName->len * i];
        tmpHierString = &hierString->value[hierString->len * i];
        tmpFilePath = &filePath->value[filePath->len * i];
        tmpDataOwnerName = &dataOwnerName->value[dataOwnerName->len * i];
        tmpDataOwnerZone = &dataOwnerZone->value[dataOwnerZone->len * i];
        tmpReplStatus = &replStatus->value[replStatus->len * i];
        tmpStatusString = &statusString->value[statusString->len * i];
        tmpChksum = &chksum->value[chksum->len * i];
        tmpDataExpiry = &dataExpiry->value[dataExpiry->len * i];
        tmpDataMapId = &dataMapId->value[dataMapId->len * i];
        tmpDataComments = &dataComments->value[dataComments->len * i];
        tmpDataCreate = &dataCreate->value[dataCreate->len * i];
        tmpDataModify = &dataModify->value[dataModify->len * i];
        tmpDataMode = &dataMode->value[dataMode->len * i];
        tmpDataName = &dataName->value[dataName->len * i];
        tmpCollName = &collName->value[collName->len * i];

        snprintf (dataObjInfo->objPath, MAX_NAME_LEN, "%s/%s",tmpCollName, tmpDataName);
        rstrcpy (dataObjInfo->rescName, tmpRescName, NAME_LEN);
        rstrcpy (dataObjInfo->rescHier, tmpHierString, MAX_NAME_LEN);
        //status = resolveResc (tmpRescName, &dataObjInfo->rescInfo);
        /*if (status < 0) {
            rodsLog( LOG_DEBUG,"getDataObjInfo: resolveResc error for %s, status = %d",
                    tmpRescName, status);
            #if 0	// this could happen for remote zone resource 
            return (status);
            #endif
        }*/
        // =-=-=-=-=-=-=-
        // get the resource info struct given a resource name
        dataObjInfo->rescInfo = new rescInfo_t;
        eirods::error err = eirods::get_resc_info( dataObjInfo->rescName, *dataObjInfo->rescInfo );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "failed to get resource info [";
            msg << dataObjInfo->rescName << "]";
            eirods::log( PASSMSG( msg.str(), err ) );
            return err.code();
        }

        std::string hier( tmpHierString );
        std::string resc( tmpRescName );

        rstrcpy (dataObjInfo->rescGroupName, tmpRescGroupName, NAME_LEN);
        rstrcpy (dataObjInfo->dataType, tmpDataType, NAME_LEN);
        dataObjInfo->dataSize = strtoll (tmpDataSize, 0, 0);
        rstrcpy (dataObjInfo->chksum, tmpChksum, NAME_LEN);
        rstrcpy (dataObjInfo->version, tmpVersion, NAME_LEN);
        rstrcpy (dataObjInfo->filePath, tmpFilePath, MAX_NAME_LEN);
        rstrcpy (dataObjInfo->dataOwnerName, tmpDataOwnerName, NAME_LEN);
        rstrcpy (dataObjInfo->dataOwnerZone, tmpDataOwnerZone, NAME_LEN);
        dataObjInfo->replNum = atoi (tmpReplNum);
        dataObjInfo->replStatus = atoi (tmpReplStatus);
        rstrcpy (dataObjInfo->statusString, tmpStatusString, LONG_NAME_LEN);
        dataObjInfo->dataId = strtoll (tmpDataId, 0, 0);
        dataObjInfo->collId = strtoll (tmpCollId, 0, 0);
        dataObjInfo->dataMapId = atoi (tmpDataMapId);
        rstrcpy (dataObjInfo->dataComments, tmpDataComments, LONG_NAME_LEN);
        rstrcpy (dataObjInfo->dataExpiry, tmpDataExpiry, TIME_LEN);
        rstrcpy (dataObjInfo->dataCreate, tmpDataCreate, TIME_LEN);
        rstrcpy (dataObjInfo->dataModify, tmpDataModify, TIME_LEN);
        rstrcpy (dataObjInfo->dataMode, tmpDataMode, NAME_LEN);
        dataObjInfo->writeFlag = writeFlag;
        
        dataObjInfo->next = 0;

        queDataObjInfo (dataObjInfoHead, dataObjInfo, 1, 0);

    } // for i
    
    freeGenQueryOut (&genQueryOut);

    return (qcondCnt);
}

int
sortObjInfo (
    dataObjInfo_t **dataObjInfoHead, 
    dataObjInfo_t **currentArchInfo,
    dataObjInfo_t **currentCacheInfo, 
    dataObjInfo_t **oldArchInfo,
    dataObjInfo_t **oldCacheInfo, 
    dataObjInfo_t **downCurrentInfo,
    dataObjInfo_t **downOldInfo,
    const char* resc_hier)
{

    dataObjInfo_t *tmpDataObjInfo, *nextDataObjInfo;
    // int rescClassInx;
    int topFlag;
    dataObjInfo_t *currentBundleInfo = NULL;
    dataObjInfo_t *oldBundleInfo = NULL;

    *currentArchInfo = *currentCacheInfo = *oldArchInfo = *oldCacheInfo = NULL;
    *downCurrentInfo = *downOldInfo = NULL;

    tmpDataObjInfo = *dataObjInfoHead;

    while (tmpDataObjInfo != NULL) {

        nextDataObjInfo = tmpDataObjInfo->next;
        tmpDataObjInfo->next = NULL;

            
        if (tmpDataObjInfo->rescInfo                 == NULL || 
            tmpDataObjInfo->rescInfo->rodsServerHost == NULL) {
            topFlag = 0;

        } else if (tmpDataObjInfo->rescInfo->rescStatus == INT_RESC_STATUS_DOWN) {
            /* the resource is down */
            if (tmpDataObjInfo->replStatus > 0) {
                queDataObjInfo (downCurrentInfo, tmpDataObjInfo, 1, 1);
            } else {
                queDataObjInfo (downOldInfo, tmpDataObjInfo, 1, 1);
            }

            tmpDataObjInfo = nextDataObjInfo;

            continue;
        } else {
            rodsServerHost_t *rodsServerHost = (rodsServerHost_t *) tmpDataObjInfo->rescInfo->rodsServerHost;
            
            if ( rodsServerHost && rodsServerHost->localFlag != LOCAL_HOST) {
                topFlag = 0;
            } else {
                /* queue local host at the head */
                topFlag = 1;
            }
        }

        std::string class_type;
        eirods::error prop_err = eirods::get_resource_property<std::string>( 
                                      tmpDataObjInfo->rescInfo->rescName, 
                                      eirods::RESOURCE_CLASS, 
                                      class_type );

        bool hier_match = false;
        if(resc_hier != NULL && (strcmp(resc_hier, tmpDataObjInfo->rescHier) == 0)) {
            hier_match = true;
        }
        
        // if the resc hierarchy is defined match it
        if(resc_hier != NULL && hier_match) {
            queDataObjInfo(currentCacheInfo, tmpDataObjInfo, 1, topFlag);
        }

        else if(resc_hier != NULL && !hier_match) {
            queDataObjInfo (oldCacheInfo, tmpDataObjInfo, 1, topFlag);
        }
        
        // rescClassInx = tmpDataObjInfo->rescInfo->rescClassInx;
        else if (tmpDataObjInfo->replStatus > 0) {
#if 0 // JMC - legacy resource
            if (RescClass[rescClassInx].classType == ARCHIVAL_CL) {
                queDataObjInfo (currentArchInfo, tmpDataObjInfo, 1, topFlag);
            } else if (RescClass[rescClassInx].classType == COMPOUND_CL) {
                queDataObjInfo (&currentCompInfo, tmpDataObjInfo, 1, topFlag);
            } else if (RescClass[rescClassInx].classType == BUNDLE_CL) {
                queDataObjInfo (&currentBundleInfo, tmpDataObjInfo, 1, topFlag);
            } else {
                queDataObjInfo (currentCacheInfo, tmpDataObjInfo, 1, topFlag);
            }
#else
            if( "archive" == class_type ) {
                queDataObjInfo (currentArchInfo, tmpDataObjInfo, 1, topFlag);
            } else if( "compound" == class_type ) {
                rodsLog( LOG_ERROR, "sortObj :: class_type == compound" );
            } else if( "bundle" == class_type ) {
                queDataObjInfo (&currentBundleInfo, tmpDataObjInfo, 1, topFlag);
            } else {
                queDataObjInfo (currentCacheInfo, tmpDataObjInfo, 1, topFlag);
            }
#endif // JMC - legacy resource
        } else {
#if 0 // JMC - legacy resource
            if (RescClass[rescClassInx].classType == ARCHIVAL_CL) {
                queDataObjInfo (oldArchInfo, tmpDataObjInfo, 1, topFlag);
            } else if (RescClass[rescClassInx].classType == COMPOUND_CL) {
                queDataObjInfo (&oldCompInfo, tmpDataObjInfo, 1, topFlag);
            } else if (RescClass[rescClassInx].classType == BUNDLE_CL) {
                queDataObjInfo (&oldBundleInfo, tmpDataObjInfo, 1, topFlag);
            } else {
                queDataObjInfo (oldCacheInfo, tmpDataObjInfo, 1, topFlag);
            }
#else
            if( "archive" == class_type ) {
                queDataObjInfo (oldArchInfo, tmpDataObjInfo, 1, topFlag);
            } else if( "compound" == class_type ) {
                rodsLog( LOG_ERROR, "sortObj :: class_type == compound" );
            } else if( "bundle" == class_type ) {
                queDataObjInfo (&oldBundleInfo, tmpDataObjInfo, 1, topFlag);
            } else {
                queDataObjInfo (oldCacheInfo, tmpDataObjInfo, 1, topFlag);
            }
#endif // JMC - legacy resource

        }
        tmpDataObjInfo = nextDataObjInfo;
    } // while

    /* combine ArchInfo and CompInfo. COMPOUND_CL before BUNDLE_CL */
    //queDataObjInfo (oldArchInfo, oldCompInfo, 0, 0);
    queDataObjInfo (oldArchInfo, oldBundleInfo, 0, 0);
    //queDataObjInfo (currentArchInfo, currentCompInfo, 0, 0);
    queDataObjInfo (currentArchInfo, currentBundleInfo, 0, 0);
    return (0);
}

/* sortObjInfoForOpen - Sort the dataObjInfo in dataObjInfoHead for open.
 * If RESC_HIER_STR_KW is set matching resc obj is first.
 * If it is for read (writeFlag == 0), discard old copies, then cache first,
 * archval second.
 * If it is for write, (writeFlag > 0), resource in DEST_RESC_NAME_KW first,
 * then current cache, current archival, old cache and old archival.
 */ 
int
sortObjInfoForOpen (rsComm_t *rsComm, dataObjInfo_t **dataObjInfoHead, 
                    keyValPair_t *condInput, int writeFlag)
{
    int result = 0;
    char* resc_hier = getValByKey(condInput, RESC_HIER_STR_KW);
    if(!resc_hier) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - No resource hierarchy specified in keywords.";
        eirods::log(ERROR(SYS_INVALID_INPUT_PARAM, msg.str()));
        result = SYS_INVALID_INPUT_PARAM;
    } else {
        dataObjInfo_t* found_info = NULL;
        dataObjInfo_t* prev_info = NULL;
        for(dataObjInfo_t* dataObjInfo = *dataObjInfoHead;
            result >= 0 && found_info == NULL && dataObjInfo != NULL;
            dataObjInfo = dataObjInfo->next) {
            if(strcmp(resc_hier, dataObjInfo->rescHier) == 0) {
                found_info = dataObjInfo;
            } else {
                prev_info = dataObjInfo;
            }
        }
        if(found_info == NULL) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - No data object found matching resource hierarchy: \"";
            msg << resc_hier;
            msg << "\"";
            eirods::log(ERROR(EIRODS_HIERARCHY_ERROR, msg.str()));
            result = EIRODS_HIERARCHY_ERROR;
        } else {
            if(prev_info == NULL) {
                // our object is at the head of the list. So delete the rest of the list, if any and we are done.
                if(found_info->next != NULL) {
                    freeAllDataObjInfo(found_info->next);
                    found_info->next = NULL;
                }
            } else {
                // remove our object from the list. delete that list then make our object the head.
                prev_info->next = found_info->next;
                found_info->next = NULL;
                freeAllDataObjInfo(*dataObjInfoHead);
                *dataObjInfoHead = found_info;
            }
        }
    }
    return result;
    
#if 0
    dataObjInfo_t *currentArchInfo, *currentCacheInfo, *oldArchInfo, 
        *oldCacheInfo, *downCurrentInfo, *downOldInfo;
    int status = 0;
    sortObjInfo (dataObjInfoHead, &currentArchInfo, &currentCacheInfo,
                 &oldArchInfo, &oldCacheInfo, &downCurrentInfo, &downOldInfo);

    *dataObjInfoHead = currentCacheInfo;
    queDataObjInfo (dataObjInfoHead, currentArchInfo, 0, 0);
    if (writeFlag == 0) {
        /* For read only */
        if (*dataObjInfoHead != NULL) {
            /* we already have a good copy */
            freeAllDataObjInfo (oldCacheInfo);
            freeAllDataObjInfo (oldArchInfo);
        } else if (downCurrentInfo != NULL) {
            /* don't have a good copy */
            /* the old current copy is not available */
            freeAllDataObjInfo (oldCacheInfo);
            freeAllDataObjInfo (oldArchInfo);
            status = SYS_RESC_IS_DOWN;
        } else {
            /* don't have a good copy. use the old one */
            queDataObjInfo (dataObjInfoHead, oldCacheInfo, 0, 0);
            queDataObjInfo (dataObjInfoHead, oldArchInfo, 0, 0);
        }
        freeAllDataObjInfo (downCurrentInfo);
        freeAllDataObjInfo (downOldInfo);
    } else {    /* write */
        char *rescName; 
        queDataObjInfo (dataObjInfoHead, oldCacheInfo, 0, 0);
        queDataObjInfo (dataObjInfoHead, oldArchInfo, 0, 0);
        
        if (*dataObjInfoHead == NULL) {
            /* no working copy. */
            std::string resc_name;
            eirods::error name_err = eirods::resolve_resource_name( "", condInput, resc_name );
            if( !name_err.ok() ) {
                freeAllDataObjInfo (downCurrentInfo);
                freeAllDataObjInfo (downOldInfo);
                status = SYS_RESC_IS_DOWN;

            } else {
                int resc_status = -1;
                eirods::error prop_err = eirods::get_resource_property<int>( 
                                             resc_name, 
                                             eirods::RESOURCE_STATUS, 
                                             resc_status );
                // JMC - legacy resource if (getRescStatus (rsComm, NULL, condInput) == INT_RESC_STATUS_DOWN) {
                if( resc_status == INT_RESC_STATUS_DOWN ) {
                        freeAllDataObjInfo (downCurrentInfo);
                        freeAllDataObjInfo (downOldInfo);
                        status = SYS_RESC_IS_DOWN;
                } else {
                        queDataObjInfo (dataObjInfoHead, downCurrentInfo, 0, 0);
                        queDataObjInfo (dataObjInfoHead, downOldInfo, 0, 0);
                }
            }
	    } else {
            freeAllDataObjInfo (downCurrentInfo);
            freeAllDataObjInfo (downOldInfo);
            if ((rescName = getValByKey (condInput, DEST_RESC_NAME_KW)) != NULL ||
                (rescName = getValByKey (condInput, DEF_RESC_NAME_KW)) != NULL ||
                (rescName = getValByKey (condInput, BACKUP_RESC_NAME_KW)) != NULL) {
                 
                requeDataObjInfoByResc (dataObjInfoHead, rescName, writeFlag,1);
            }
        }
    }
    return (status);
#endif // #if 0
}

int
getNumDataObjInfo (dataObjInfo_t *dataObjInfoHead) 
{
    dataObjInfo_t *tmpDataObjInfo;
    int numInfo = 0;

    tmpDataObjInfo = dataObjInfoHead;
    while (tmpDataObjInfo != NULL) {
        numInfo++;
        tmpDataObjInfo = tmpDataObjInfo->next;
    }
    return (numInfo);
}

int
sortDataObjInfoRandom (dataObjInfo_t **dataObjInfoHead)
{
    dataObjInfo_t *myDataObjInfo[50];
    dataObjInfo_t *tmpDataObjInfo;
    int i, j, tmpCnt, order;
    int numInfo = getNumDataObjInfo (*dataObjInfoHead);

    if (numInfo <= 1) {
        return (0);
    }

    if (numInfo > 50) {
        rodsLog (LOG_NOTICE,
                 "sortDataObjInfoRandom: numInfo %d > 50, setting it to 50", numInfo);
        numInfo = 50;
    }

    memset (myDataObjInfo, 0, numInfo * sizeof (rescGrpInfo_t *));
    /* fill the array randomly */

    tmpCnt = numInfo;
    tmpDataObjInfo = *dataObjInfoHead;
    while (tmpDataObjInfo != NULL) {
        if (tmpCnt > 1) {
            order = random() % tmpCnt;
        } else {
            order = 0;
        }
        for (i = 0, j = 0; i < numInfo; i ++) {
            if (myDataObjInfo[i] == NULL) {
                if (order <= j) {
                    myDataObjInfo[i] = tmpDataObjInfo;
                    break;
                }
                j ++;
            }
        }
        tmpCnt --;
        tmpDataObjInfo = tmpDataObjInfo->next;
    }

    /* now que the array in order */

    *dataObjInfoHead = NULL;
    for (i = 0; i < numInfo; i ++) {
        queDataObjInfo (dataObjInfoHead, myDataObjInfo[i], 1, 1);
    }

    return (0);
}

/* requeDataObjInfoByResc - requeue the dataObjInfo in the 
 * dataObjInfoHead by putting dataObjInfo stored in preferredResc
 * at the top of the queue.
 * return 0 if dataObjInfo with preferredResc exiists.
 * Otherwise, return -1.
 */

int
requeDataObjInfoByResc (dataObjInfo_t **dataObjInfoHead, 
                        char *preferredResc, int writeFlag, int topFlag)
{

    dataObjInfo_t *tmpDataObjInfo, *prevDataObjInfo;
    int status = -1;

    if (preferredResc == NULL || *dataObjInfoHead == NULL) {
        return (0);
    }

    tmpDataObjInfo = *dataObjInfoHead;
    if (tmpDataObjInfo->next == NULL) {
        /* just one */
        if (strcmp (preferredResc, tmpDataObjInfo->rescInfo->rescName) == 0 ||  // JMC - backport 4543
            strcmp (preferredResc, tmpDataObjInfo->rescGroupName) == 0) {
            return (0);
        } else {
            return (-1);
        }
    }
    prevDataObjInfo = NULL;
    while (tmpDataObjInfo != NULL) {
        if (tmpDataObjInfo->rescInfo != NULL) {
            if (strcmp (preferredResc, tmpDataObjInfo->rescInfo->rescName) 
                == 0 || strcmp (preferredResc, tmpDataObjInfo->rescGroupName) 
                == 0) {
                if (writeFlag > 0 || tmpDataObjInfo->replStatus > 0) {
                    if (prevDataObjInfo != NULL) {
                        prevDataObjInfo->next = tmpDataObjInfo->next;
                        queDataObjInfo (dataObjInfoHead, tmpDataObjInfo, 1,
                                        topFlag);
                    }
                    if (topFlag > 0) {              
                        return (0);
                    } else {
                        status = 0;
                    }
                }
            }
        }
        prevDataObjInfo = tmpDataObjInfo;
        tmpDataObjInfo = tmpDataObjInfo->next;
    }

    return (status);
}

int
requeDataObjInfoByReplNum (dataObjInfo_t **dataObjInfoHead, int replNum)
{
    dataObjInfo_t *tmpDataObjInfo, *prevDataObjInfo;
    int status = -1;

    if (dataObjInfoHead == NULL || *dataObjInfoHead == NULL) {
        return (-1);
    }

    tmpDataObjInfo = *dataObjInfoHead;
    if (tmpDataObjInfo->next == NULL) {
        /* just one */
        if (replNum == tmpDataObjInfo->replNum) {
            return (0);
        } else {
            return (-1);
        }
    }
    prevDataObjInfo = NULL;
    while (tmpDataObjInfo != NULL) {
        if (replNum == tmpDataObjInfo->replNum) {
            if (prevDataObjInfo != NULL) {
                prevDataObjInfo->next = tmpDataObjInfo->next;
                queDataObjInfo (dataObjInfoHead, tmpDataObjInfo, 1, 1);
            }
            status = 0;
            break;
        }
        prevDataObjInfo = tmpDataObjInfo;
        tmpDataObjInfo = tmpDataObjInfo->next;
    }

    return (status);
}

dataObjInfo_t *
chkCopyInResc (dataObjInfo_t *dataObjInfoHead, rescGrpInfo_t *myRescGrpInfo, const char* destRescHier)
{
    rescGrpInfo_t *tmpRescGrpInfo;
    rescInfo_t *tmpRescInfo;
    dataObjInfo_t *tmpDataObjInfo;

    tmpDataObjInfo = dataObjInfoHead;
    while (tmpDataObjInfo != NULL) {
        tmpRescGrpInfo = myRescGrpInfo;
        while (tmpRescGrpInfo != NULL) {
            tmpRescInfo = tmpRescGrpInfo->rescInfo;
            // No longer good enough to check if the resource names are the same. We have to verify that the resource hiearchies
            // match as well. - hcj
            if (strcmp (tmpDataObjInfo->rescInfo->rescName, tmpRescInfo->rescName) == 0 &&
                (destRescHier == NULL || strcmp(tmpDataObjInfo->rescHier, destRescHier) == 0)) { 
                return (tmpDataObjInfo);
            }
            tmpRescGrpInfo = tmpRescGrpInfo->next;
        }
        tmpDataObjInfo = tmpDataObjInfo->next;
    }
    return (NULL);
}

/* matchAndTrimRescGrp - check for matching rescName in dataObjInfoHead
 * and rescGrpInfoHead. If there is a match, unlink and free the 
 * rescGrpInfo in rescGrpInfoHead so that they wont be replicated 
 * If trimjFlag - set what to trim. Valid input are : TRIM_MATCHED_RESC_INFO, 
 * TRIM_MATCHED_OBJ_INFO and TRIM_UNMATCHED_OBJ_INFO
 */

int
matchAndTrimRescGrp (dataObjInfo_t **dataObjInfoHead, 
                     rescGrpInfo_t **rescGrpInfoHead, int trimjFlag, 
                     dataObjInfo_t **trimmedDataObjInfo)
{
    rescGrpInfo_t *tmpRescGrpInfo;
    rescGrpInfo_t *prevRescGrpInfo;
    rescInfo_t *tmpRescInfo;
    dataObjInfo_t *tmpDataObjInfo, *prevDataObjInfo, *nextDataObjInfo;
    int matchFlag; 
    char rescGroupName[NAME_LEN];

    if (trimmedDataObjInfo != NULL) *trimmedDataObjInfo = NULL;

    if (*rescGrpInfoHead != NULL) {
	    rstrcpy (rescGroupName, (*rescGrpInfoHead)->rescGroupName, NAME_LEN);
    } else {
	    rescGroupName[0] = '\0';
    } 

    tmpDataObjInfo = *dataObjInfoHead;
    prevDataObjInfo = NULL;

    while( tmpDataObjInfo != NULL ) {
        matchFlag = 0;
        nextDataObjInfo = tmpDataObjInfo->next;
        tmpRescGrpInfo = *rescGrpInfoHead;
        prevRescGrpInfo = NULL;
        
        while (tmpRescGrpInfo != NULL) {
            tmpRescInfo = tmpRescGrpInfo->rescInfo;
            if (strcmp (tmpDataObjInfo->rescInfo->rescName,tmpRescInfo->rescName) == 0) {
		        matchFlag = 1;
		        break;
           
            }
             
	        prevRescGrpInfo = tmpRescGrpInfo;
            tmpRescGrpInfo = tmpRescGrpInfo->next;

        } // while

	    if (matchFlag == 1) {
            if (trimjFlag & TRIM_MATCHED_RESC_INFO) {
                if (tmpRescGrpInfo == *rescGrpInfoHead) {
                    *rescGrpInfoHead = tmpRescGrpInfo->next;
                
                } else {
                    prevRescGrpInfo->next = tmpRescGrpInfo->next;
                
                }

                free (tmpRescGrpInfo);

            } else if( trimjFlag & REQUE_MATCHED_RESC_INFO ) {
                if (tmpRescGrpInfo->next != NULL) {
                    #if 0 // JMC - legacy resource
                    /* queue to bottom */
                    if (tmpRescGrpInfo == *rescGrpInfoHead) {
                        *rescGrpInfoHead = tmpRescGrpInfo->next;

                    } else {
                        prevRescGrpInfo->next = tmpRescGrpInfo->next;

                    }

		            queRescGrp (rescGrpInfoHead, tmpRescGrpInfo, BOTTOM_FLAG);

                    #else
                    rodsLog( LOG_ERROR, "matchAndTrimRescGrp - calling REQUE_MATCHED_RESC_INFO with non null next ptr." );
                    #endif
		        } // if tmpRescGrpInfo->next

	        }

            if (trimjFlag & TRIM_MATCHED_OBJ_INFO) {
                if (tmpDataObjInfo == *dataObjInfoHead) {
                    *dataObjInfoHead = tmpDataObjInfo->next;

                } else {
                    prevDataObjInfo->next = tmpDataObjInfo->next;

                }

                if (trimmedDataObjInfo != NULL) {
                    queDataObjInfo (trimmedDataObjInfo, tmpDataObjInfo, 1, 0);

                } else {
                    free (tmpDataObjInfo); 

		        }

            } else {
		        prevDataObjInfo = tmpDataObjInfo;

	        }

        } else {
            /* no match */
            if( ( trimjFlag & TRIM_UNMATCHED_OBJ_INFO ) ||
                ( ( trimjFlag & TRIM_MATCHED_OBJ_INFO ) && 
                  strlen( rescGroupName ) > 0 &&
                  strcmp( tmpDataObjInfo->rescGroupName, rescGroupName ) == 0 ) ) {
                    /* take it out */
                if (tmpDataObjInfo == *dataObjInfoHead) {
                    *dataObjInfoHead = tmpDataObjInfo->next;
                
                } else {
                    prevDataObjInfo->next = tmpDataObjInfo->next;
                
                }

                if (trimmedDataObjInfo != NULL) {
                    queDataObjInfo (trimmedDataObjInfo, tmpDataObjInfo, 1, 0);
                
                } else {
                    free (tmpDataObjInfo);

                }

            } else {
                prevDataObjInfo = tmpDataObjInfo;
            
            }

	    } // else

        tmpDataObjInfo = nextDataObjInfo;
    
    } // while

    return (0);
}

/* sortObjInfoForRepl - sort the data object given in dataObjInfoHead.
 * Put the current copies in dataObjInfoHead. Delete old copies if
 * deleteOldFlag allowed or put them in oldDataObjInfoHead for further
 * process if not allowed.
 */

int
sortObjInfoForRepl (
    dataObjInfo_t** dataObjInfoHead,     // RESC_HIER
    dataObjInfo_t** oldDataObjInfoHead,  // DST_RESC_HIER
    int             deleteOldFlag,
    const char*     resc_hier,
    const char*     dst_resc_hier ) {
    // =-=-=-=-=-=-=-
    // trap bad pointers
    if( !*dataObjInfoHead ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    // =-=-=-=-=-=-=-
    // short circuit sort for the case where a dst resc hier is provided
    // which means the choice for to which repl to update is made already
    if( dst_resc_hier ) {
        dataObjInfo_t* tmp_info = *dataObjInfoHead;
        while( tmp_info ) {
            // =-=-=-=-=-=-=-
            // compare resc hier strings, if its a match this is our 
            // choice so queue it up on the oldDataObjInfoHead
            if( 0 == strcmp( dst_resc_hier, tmp_info->rescHier ) ) {
                queDataObjInfo( oldDataObjInfoHead, tmp_info, 1, 1 );
                tmp_info = 0;
            
            } else {
                tmp_info = tmp_info->next;

            }

        } // while tmp_info

    } // if dst_resc_hier

    dataObjInfo_t *currentArchInfo = 0, *currentCacheInfo = 0, *oldArchInfo = 0,
                  *oldCacheInfo = 0,    *downCurrentInfo = 0,  *downOldInfo = 0;

    sortObjInfo( dataObjInfoHead, &currentArchInfo, &currentCacheInfo,
                 &oldArchInfo, &oldCacheInfo, &downCurrentInfo, &downOldInfo, resc_hier);
    
    freeAllDataObjInfo (downOldInfo);
    *dataObjInfoHead = currentCacheInfo;
    queDataObjInfo (dataObjInfoHead, currentArchInfo, 0, 0);
    queDataObjInfo (dataObjInfoHead, downCurrentInfo, 0, 0);
    if (*dataObjInfoHead != NULL) {
        if (deleteOldFlag == 0) {  
            /* multi copy not allowed. have to keep old copy in  
             * oldDataObjInfoHead for further processing */
            *oldDataObjInfoHead = oldCacheInfo;
            queDataObjInfo (oldDataObjInfoHead, oldArchInfo, 0, 0);
        } else {
            freeAllDataObjInfo (oldCacheInfo);
            freeAllDataObjInfo (oldArchInfo);
        }
    } else {
        queDataObjInfo (dataObjInfoHead, oldCacheInfo, 0, 0);
        queDataObjInfo (dataObjInfoHead, oldArchInfo, 0, 0);
    }
    if (*dataObjInfoHead == NULL) 
        return SYS_RESC_IS_DOWN;
    else

        return (0);
}

#if 0 // JMC - UNUSED
/* dataObjExist - check whether the data object given in dataObjInp exist.
   Returns 1 if exists, 0 ==> does not exist
*/

int
dataObjExist (rsComm_t *rsComm, dataObjInp_t *dataObjInp)
{
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    int status;

    status = initDataObjInfoQuery (dataObjInp, &genQueryInp, 0);

    if (status < 0) {
        return (status);
    }

    addInxIval (&genQueryInp.selectInp, COL_D_DATA_ID, 1);
    genQueryInp.maxRows = MAX_SQL_ROWS;

    status =  rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

    clearGenQueryInp (&genQueryInp);
    freeGenQueryOut (&genQueryOut);

    if (status < 0) {
        return (0);
    } else {
        return (1);
    }
}
#endif // JMC - UNUSED

/* initDataObjInfoQuery - initialize the genQueryInp based on dataObjInp.
 * returns the qcondCnt - count of sqlCondInp based on condition input.
 */
         
int
initDataObjInfoQuery (dataObjInp_t *dataObjInp, genQueryInp_t *genQueryInp,
                      int ignoreCondInput)
{
    char myColl[MAX_NAME_LEN], myData[MAX_NAME_LEN];
    char condStr[MAX_NAME_LEN];
    char *tmpStr;
    int status;
    int qcondCnt = 0;

    memset (genQueryInp, 0, sizeof (genQueryInp_t));

    if ((tmpStr = getValByKey (&dataObjInp->condInput, QUERY_BY_DATA_ID_KW)) 
        == NULL) {
        memset (myColl, 0, MAX_NAME_LEN);
        memset (myData, 0, MAX_NAME_LEN);

        if ((status = splitPathByKey (
                 dataObjInp->objPath, myColl, myData, '/')) < 0) {
            rodsLog (LOG_NOTICE,
                     "initDataObjInfoQuery: splitPathByKey for %s error, status = %d",
                     dataObjInp->objPath, status);
            return (status);
        }
        snprintf (condStr, MAX_NAME_LEN, "='%s'", myColl);
        addInxVal (&genQueryInp->sqlCondInp, COL_COLL_NAME, condStr);
        snprintf (condStr, MAX_NAME_LEN, "='%s'", myData);
        addInxVal (&genQueryInp->sqlCondInp, COL_DATA_NAME, condStr);
    } else {
        snprintf (condStr, MAX_NAME_LEN, "='%s'", tmpStr);
        addInxVal (&genQueryInp->sqlCondInp, COL_D_DATA_ID, condStr);
    }

    if (ignoreCondInput == 0 && (tmpStr =
                                 getValByKey (&dataObjInp->condInput, REPL_NUM_KW)) != NULL) {
        snprintf (condStr, NAME_LEN, "='%s'", tmpStr);
        addInxVal (&genQueryInp->sqlCondInp, COL_DATA_REPL_NUM, condStr);
        qcondCnt++;
    }

    return (qcondCnt);
}

/* chkOrphanFile - check whether a filePath is a orphan file. 
 *    return - 1 - the file is orphan.
 *             0 - 0 the file is not an orphan.
 *             -ive - query error.
 *             If it is not orphan and dataObjInfo != NULL, output ObjID, 
 *             replNum and objPath to dataObjInfo.
 */

int
chkOrphanFile (rsComm_t *rsComm, char *filePath, char *rescName,
               dataObjInfo_t *dataObjInfo)
{
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    int status;
    char condStr[MAX_NAME_LEN];

    memset (&genQueryInp, 0, sizeof (genQueryInp_t));

    snprintf (condStr, MAX_NAME_LEN, "='%s'", filePath);
    addInxVal (&genQueryInp.sqlCondInp, COL_D_DATA_PATH, condStr);
    snprintf (condStr, MAX_NAME_LEN, "='%s'", rescName);
    addInxVal (&genQueryInp.sqlCondInp, COL_D_RESC_NAME, condStr);

    addInxIval (&genQueryInp.selectInp, COL_D_DATA_ID, 1);
    addInxIval (&genQueryInp.selectInp, COL_COLL_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_DATA_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_DATA_REPL_NUM, 1);
    addInxIval (&genQueryInp.selectInp, COL_D_RESC_HIER, 1);
    
    genQueryInp.maxRows = MAX_SQL_ROWS;

    status =  rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

    clearGenQueryInp (&genQueryInp);
    if (status < 0) {
        if (status == CAT_NO_ROWS_FOUND) {
            rsComm->perfStat.orphanCnt ++;
            return (1);
        } else {
            rodsLog (LOG_ERROR,
                     "chkOrphanFile: rsGenQuery error for %s, status = %d",
                     filePath, status);
            /* we have unexpected query error. Assume the file is not
             * orphan */ 
            return (status);
        }
    } else {
        sqlResult_t *dataId, *replNum, *dataName, *collName, *rescHier;
        rsComm->perfStat.nonOrphanCnt ++;

        if ((collName =
             getSqlResultByInx (genQueryOut, COL_COLL_NAME)) == NULL) {
            rodsLog (LOG_NOTICE,
                     "chkOrphanFile: getSqlResultByInx for COL_COLL_NAME failed");
            return (UNMATCHED_KEY_OR_INDEX);
        }

        if ((dataName = getSqlResultByInx (genQueryOut, COL_DATA_NAME)) 
            == NULL) {
            rodsLog (LOG_NOTICE,
                     "chkOrphanFile: getSqlResultByInx for COL_DATA_NAME failed");
            return (UNMATCHED_KEY_OR_INDEX);
        }

        if ((dataId = getSqlResultByInx (genQueryOut, COL_D_DATA_ID)) == NULL) {
            rodsLog (LOG_NOTICE,
                     "chkOrphanFile: getSqlResultByInx for COL_D_DATA_ID failed");
            return (UNMATCHED_KEY_OR_INDEX);
        }

        if ((replNum = getSqlResultByInx (genQueryOut, COL_DATA_REPL_NUM)) ==
            NULL) {
            rodsLog (LOG_NOTICE,
                     "chkOrphanFile: getSqlResultByInx for COL_DATA_REPL_NUM failed");
            return (UNMATCHED_KEY_OR_INDEX);
        }

        if ((rescHier = getSqlResultByInx (genQueryOut, COL_D_RESC_HIER)) ==
            NULL) {
            rodsLog (LOG_NOTICE,
                     "chkOrphanFile: getSqlResultByInx for COL_D_RESC_HIER failed");
            return (UNMATCHED_KEY_OR_INDEX);
        }

        if (dataObjInfo != NULL) {
            dataObjInfo->dataId = strtoll (dataId->value, 0, 0);
            dataObjInfo->replNum = atoi (replNum->value);
            snprintf (dataObjInfo->objPath, MAX_NAME_LEN, "%s/%s",
                      collName->value, dataName->value);
            rstrcpy(dataObjInfo->rescHier, rescHier->value, MAX_NAME_LEN);
        }

        freeGenQueryOut (&genQueryOut);

        return (0);
    }
}


/* chkOrphanDir - check whether a filePath is a orphan directory.
 *    return - 1 - the dir is orphan.
 *             0 - 0 the dir is not an orphan.
 *             -ive - query error.
 */

int
chkOrphanDir (rsComm_t *rsComm, char *dirPath, char *rescName)
{
#ifndef USE_BOOST_FS
    DIR *dirPtr;
    struct dirent *myDirent;
    struct stat statbuf;
#endif
    char subfilePath[MAX_NAME_LEN];
    int savedStatus = 1;
    int status = 0;

#ifdef USE_BOOST_FS
    path srcDirPath (dirPath);
    if (!exists(srcDirPath) || !is_directory(srcDirPath)) {
#else
        dirPtr = opendir (dirPath);
        if (dirPtr == NULL) {
#endif
            rodsLog (LOG_ERROR,
                     "chkOrphanDir: opendir error for %s, errno = %d",
                     dirPath, errno);
            return (UNIX_FILE_OPENDIR_ERR - errno);
        }
#ifdef USE_BOOST_FS
        directory_iterator end_itr; // default construction yields past-the-end
        for (directory_iterator itr(srcDirPath); itr != end_itr;++itr) {
            path p = itr->path();
            snprintf (subfilePath, MAX_NAME_LEN, "%s",
                      p.c_str ());
#else
            while ((myDirent = readdir (dirPtr)) != NULL) {
                if (strcmp (myDirent->d_name, ".") == 0 ||
                    strcmp (myDirent->d_name, "..") == 0) {
                    continue;
                }
                snprintf (subfilePath, MAX_NAME_LEN, "%s/%s",
                          dirPath, myDirent->d_name);
#endif

#ifdef USE_BOOST_FS
                if (!exists (p)) {
#else
                    status = stat (subfilePath, &statbuf);

                    if (status != 0) {
#endif
                        rodsLog (LOG_ERROR,
                                 "chkOrphanDir: stat error for %s, errno = %d",
                                 subfilePath, errno);
                        savedStatus = UNIX_FILE_STAT_ERR - errno;
                        continue;
                    }
#ifdef USE_BOOST_FS
                    if (is_directory (p)) {
#else
                        if ((statbuf.st_mode & S_IFDIR) != 0) {
#endif
                            status = chkOrphanDir (rsComm, subfilePath, rescName);
#ifdef USE_BOOST_FS
                        } else if (is_regular_file (p)) {
#else
                        } else if ((statbuf.st_mode & S_IFREG) != 0) {
#endif
                            status = chkOrphanFile (rsComm, subfilePath, rescName, NULL);
                        }
                        if (status == 0) {
                            /* not orphan */
#ifndef USE_BOOST_FS
                            closedir (dirPtr);
#endif
                            return status;    
                        } else if (status < 0) {
                            savedStatus = status;
                        }
                    }
#ifndef USE_BOOST_FS
                    closedir (dirPtr);
#endif
                    return savedStatus;
                }


/* resolveSingleReplCopy - given the dataObjInfoHead (up-to-date copies) 
 * and oldDataObjInfoHead (stale copies) and the destRescGrpInfo,
 * sort through the single copy requirement for repl.
 * If there is a good copy in every resc in the rescGroup, return 
 * HAVE_GOOD_COPY. Otherwise, trim the resc in the rescGroup so only the one 
 * with no copies are left. The copies required to be overwritten are
 * placed in destDataObjInfo. The old dataObjInfo that do not need to
 * be overwritten are placed in oldDataObjInfoHead which may be needed
 * for compound resource staging.
 * A returned value of CAT_NO_ROWS_FOUND mean there is condition in
 * condInput but none in dataObjInfoHead or oldDataObjInfoHead match
 * the condition. i.e., no source for the replication
 */ 
int 
resolveSingleReplCopy ( dataObjInfo_t **dataObjInfoHead, 
                        dataObjInfo_t **oldDataObjInfoHead, 
                        rescGrpInfo_t **destRescGrpInfo,
                        dataObjInfo_t **destDataObjInfo, 
                        keyValPair_t *condInput ) {
    int status = 0;
    dataObjInfo_t *matchedDataObjInfo = NULL;
    dataObjInfo_t *matchedOldDataObjInfo = NULL;

    /* see if dataObjInfoHead and oldDataObjInfoHead matches the condInput */
    status = matchDataObjInfoByCondInput (dataObjInfoHead, oldDataObjInfoHead,
                                          condInput, &matchedDataObjInfo, &matchedOldDataObjInfo);
    if (status < 0) {
        return status;
    }

    if (matchedDataObjInfo != NULL) {
        /* que the matched one on top */
        queDataObjInfo (dataObjInfoHead, matchedDataObjInfo, 0, 1);
        queDataObjInfo (oldDataObjInfoHead, matchedOldDataObjInfo, 0, 1);
    } else if (matchedOldDataObjInfo != NULL) {
        /* The source of replication is an old copy. Queue dataObjInfoHead 
         * to oldDataObjInfoHead */ 
        queDataObjInfo (oldDataObjInfoHead, *dataObjInfoHead, 0, 1);
        *dataObjInfoHead = matchedOldDataObjInfo;
    }

    if ((*destRescGrpInfo)->next == NULL ||
        strlen ((*destRescGrpInfo)->rescGroupName) == 0) {
        /* single target resource */
        char* destRescHier = getValByKey(condInput, DEST_RESC_HIER_STR_KW);
        if ((*destDataObjInfo = chkCopyInResc (*dataObjInfoHead, 
                                               *destRescGrpInfo,
                                               destRescHier)) != NULL) {
            /* have a good copy already */
            *destDataObjInfo = NULL; // JMC - backport 4594
            return (HAVE_GOOD_COPY);
        }
    } else {
        /* target resource is a resource group with multi resources */
        matchAndTrimRescGrp (dataObjInfoHead, destRescGrpInfo, 
                             TRIM_MATCHED_RESC_INFO, NULL);
        if (*destRescGrpInfo == NULL) {
            /* have a good copy in all resc in resc group */
            return (HAVE_GOOD_COPY);
        }
    }
    /* handle the old dataObj */
    if (getValByKey (condInput, ALL_KW) != NULL) {
        dataObjInfo_t *trimmedDataObjInfo = NULL;
        /* replicate to all resc. trim the resc that has a match and
         * the DataObjInfo that does not have a match */ 
        matchAndTrimRescGrp (oldDataObjInfoHead, destRescGrpInfo, 
                             TRIM_MATCHED_RESC_INFO|TRIM_UNMATCHED_OBJ_INFO, &trimmedDataObjInfo);
        *destDataObjInfo = *oldDataObjInfoHead;
        *oldDataObjInfoHead = trimmedDataObjInfo;
    } else {
        char* destRescHier = getValByKey(condInput, DEST_RESC_HIER_STR_KW);
        *destDataObjInfo = chkCopyInResc (*oldDataObjInfoHead, *destRescGrpInfo, destRescHier);
        if (*destDataObjInfo != NULL) {
            /* see if there is any resc that is not used */
            matchAndTrimRescGrp (oldDataObjInfoHead, destRescGrpInfo, 
                                 TRIM_MATCHED_RESC_INFO, NULL);
            if (*destRescGrpInfo != NULL) {
                /* just creat a new one in myRescGrpInfo */
                *destDataObjInfo = NULL;
            } else { // // JMC - backport 4594
                dequeDataObjInfo (oldDataObjInfoHead, *destDataObjInfo);
            }
        }
    }
    return (NO_GOOD_COPY);
}

int
resolveInfoForPhymv (dataObjInfo_t **dataObjInfoHead,
                     dataObjInfo_t **oldDataObjInfoHead, 
                     rescGrpInfo_t **destRescGrpInfo,
                     keyValPair_t *condInput, 
                     int multiCopyFlag ) {
    int status;
    dataObjInfo_t *matchedDataObjInfo = NULL;
    dataObjInfo_t *matchedOldDataObjInfo = NULL;


    status = matchDataObjInfoByCondInput (dataObjInfoHead, oldDataObjInfoHead,
                                          condInput, &matchedDataObjInfo, &matchedOldDataObjInfo);

    if (status < 0) {
        return status;
    }

    if (matchedDataObjInfo != NULL) {
        /* put the matched in oldDataObjInfoHead. should not be anything in
         * oldDataObjInfoHead */
        *oldDataObjInfoHead = *dataObjInfoHead;
        *dataObjInfoHead = matchedDataObjInfo;
    }

    if (multiCopyFlag) {
        matchAndTrimRescGrp (dataObjInfoHead, destRescGrpInfo,
                             REQUE_MATCHED_RESC_INFO, NULL);
        matchAndTrimRescGrp (oldDataObjInfoHead, destRescGrpInfo,
                             REQUE_MATCHED_RESC_INFO, NULL);
    } else {
        matchAndTrimRescGrp (dataObjInfoHead, destRescGrpInfo, 
                             TRIM_MATCHED_RESC_INFO|TRIM_MATCHED_OBJ_INFO, NULL);
        matchAndTrimRescGrp (oldDataObjInfoHead, destRescGrpInfo, 
                             TRIM_MATCHED_RESC_INFO, NULL);
    }

    if (*destRescGrpInfo == NULL) {
        if (*dataObjInfoHead == NULL) {
            return (CAT_NO_ROWS_FOUND);
        } else {
            /* have a good copy in all resc in resc group */
            rodsLog (LOG_ERROR,
                     "resolveInfoForPhymv: %s already have copy in the resc",
                     (*dataObjInfoHead)->objPath);
            return (SYS_COPY_ALREADY_IN_RESC);
        }
    } else {
        return (0);
    }
}

/* matchDataObjInfoByCondInput - given dataObjInfoHead and oldDataObjInfoHead
 * put all DataObjInfo that match condInput into matchedDataObjInfo and
 * matchedOldDataObjInfo. The unmatch one stay in dataObjInfoHead and
 * oldDataObjInfoHead.
 * A CAT_NO_ROWS_FOUND means there is condition in CondInput, but none 
 * in dataObjInfoHead or oldDataObjInfoHead matches the condition 
 */ 
int matchDataObjInfoByCondInput (dataObjInfo_t **dataObjInfoHead,
                                 dataObjInfo_t **oldDataObjInfoHead, keyValPair_t *condInput,
                                 dataObjInfo_t **matchedDataObjInfo, dataObjInfo_t **matchedOldDataObjInfo)

{

    int replNumCond;
    int replNum;
    int rescCond;
    bool destHierCond = false;
    char *tmpStr, *rescName;
    char* rescHier = NULL;
    char* destRescHier = NULL;
    dataObjInfo_t *prevDataObjInfo, *nextDataObjInfo, *tmpDataObjInfo;

    if (dataObjInfoHead == NULL || *dataObjInfoHead == NULL ||
        oldDataObjInfoHead == NULL || matchedDataObjInfo == NULL ||
        matchedOldDataObjInfo == NULL) {
        rodsLog (LOG_ERROR,
                 "requeDataObjInfoByCondInput: NULL dataObjInfo input");
        return (USER__NULL_INPUT_ERR);
    }

    if ((tmpStr = getValByKey (condInput, REPL_NUM_KW)) != NULL) {
        replNum = atoi (tmpStr);
        replNumCond = 1;
    } else {
        replNumCond = 0;
    }

    destRescHier = getValByKey(condInput, DEST_RESC_HIER_STR_KW);
    rescHier = getValByKey(condInput, RESC_HIER_STR_KW);
    if(destRescHier != NULL && rescHier != NULL) {
        destHierCond = true;
    }

    // We only use the resource name if the resource hierarchy is not set
    if (!destHierCond && (rescName = getValByKey (condInput, RESC_NAME_KW)) != NULL) {
        rescCond = 1;
    } else {
        rescCond = 0;
    }

    if (replNumCond + rescCond == 0 && !destHierCond) {
        return (0);
    }
    *matchedDataObjInfo = NULL;
    *matchedOldDataObjInfo = NULL;

    tmpDataObjInfo = *dataObjInfoHead;
    prevDataObjInfo = NULL;
    while (tmpDataObjInfo != NULL) {
        nextDataObjInfo = tmpDataObjInfo->next;
        if (replNumCond == 1 && replNum == tmpDataObjInfo->replNum) {
            if (prevDataObjInfo != NULL) {
                prevDataObjInfo->next = tmpDataObjInfo->next;
            } else {
                *dataObjInfoHead = (*dataObjInfoHead)->next;
            }
            queDataObjInfo (matchedDataObjInfo, tmpDataObjInfo, 1, 0);
        } else if(destHierCond &&
                  (strcmp(rescHier, tmpDataObjInfo->rescHier) == 0 ||
                   strcmp(destRescHier, tmpDataObjInfo->rescHier) == 0)) {
            if (prevDataObjInfo != NULL) {
                prevDataObjInfo->next = tmpDataObjInfo->next;
            } else {
                *dataObjInfoHead = (*dataObjInfoHead)->next;
            }
            queDataObjInfo (matchedDataObjInfo, tmpDataObjInfo, 1, 0);
        } else if (rescCond == 1 && 
                   (strcmp (rescName, tmpDataObjInfo->rescGroupName) == 0 ||
                    strcmp (rescName, tmpDataObjInfo->rescName) == 0)) {
            if (prevDataObjInfo != NULL) {
                prevDataObjInfo->next = tmpDataObjInfo->next;
            } else {
                *dataObjInfoHead = (*dataObjInfoHead)->next;
            }
            /* que single to the bottom */
            queDataObjInfo (matchedDataObjInfo, tmpDataObjInfo, 1, 0);
        } else {
            prevDataObjInfo = tmpDataObjInfo;
        }
        tmpDataObjInfo = nextDataObjInfo;
    }

    tmpDataObjInfo = *oldDataObjInfoHead;
    prevDataObjInfo = NULL;
    while (tmpDataObjInfo != NULL) {
        nextDataObjInfo = tmpDataObjInfo->next;
        if (replNumCond == 1 && replNum == tmpDataObjInfo->replNum) {
            if (prevDataObjInfo != NULL) {
                prevDataObjInfo->next = tmpDataObjInfo->next;
            } else {
                *oldDataObjInfoHead = (*oldDataObjInfoHead)->next;
            }
            queDataObjInfo (matchedOldDataObjInfo, tmpDataObjInfo, 1, 0);
        } else if(destHierCond &&
                  (strcmp(rescHier, tmpDataObjInfo->rescHier) == 0 ||
                   strcmp(destRescHier, tmpDataObjInfo->rescHier) == 0)) {
            if (prevDataObjInfo != NULL) {
                prevDataObjInfo->next = tmpDataObjInfo->next;
            } else {
                *oldDataObjInfoHead = (*oldDataObjInfoHead)->next;
            }
            queDataObjInfo (matchedDataObjInfo, tmpDataObjInfo, 1, 0);
        } else if (rescCond == 1 &&
                   (strcmp (rescName, tmpDataObjInfo->rescGroupName) == 0 ||
                    strcmp (rescName, tmpDataObjInfo->rescName)) == 0) {
            if (prevDataObjInfo != NULL) {
                prevDataObjInfo->next = tmpDataObjInfo->next;
            } else {
                *oldDataObjInfoHead = (*oldDataObjInfoHead)->next;
            }
            queDataObjInfo (matchedOldDataObjInfo, tmpDataObjInfo, 1, 0);
        } else {
            prevDataObjInfo = tmpDataObjInfo;
        }
        tmpDataObjInfo = nextDataObjInfo;
    }

    if (*matchedDataObjInfo == NULL && *matchedOldDataObjInfo == NULL) {
        return (CAT_NO_ROWS_FOUND);
    } else {
        return (replNumCond + rescCond);
    }
}

int
    resolveInfoForTrim (dataObjInfo_t **dataObjInfoHead,
                        keyValPair_t *condInput)
{
    int i, status;
    dataObjInfo_t *matchedDataObjInfo = NULL;
    dataObjInfo_t *matchedOldDataObjInfo = NULL;
    dataObjInfo_t *oldDataObjInfoHead = NULL;
    dataObjInfo_t *tmpDataObjInfo, *prevDataObjInfo;
    int matchedInfoCnt, unmatchedInfoCnt, matchedOldInfoCnt, 
        unmatchedOldInfoCnt;
    int minCnt;
    char *tmpStr;
    int condFlag;
    int toTrim;

    // char* resc_hier = getValByKey(condInput, DEST_RESC_HIER_STR_KW);
    sortObjInfoForRepl( dataObjInfoHead, &oldDataObjInfoHead, 0, NULL, NULL );

    status = matchDataObjInfoByCondInput (dataObjInfoHead, &oldDataObjInfoHead,
                                          condInput, &matchedDataObjInfo, &matchedOldDataObjInfo);

    if (status < 0) {
        freeAllDataObjInfo (*dataObjInfoHead);
        freeAllDataObjInfo (oldDataObjInfoHead);
        *dataObjInfoHead = NULL;
        if (status == CAT_NO_ROWS_FOUND) {
            return 0;
        } else {
            rodsLog(LOG_NOTICE, "%s - Failed during matching of data objects.", __FUNCTION__);
            return status;
        }
    }
    condFlag = status;  /* cond exist if condFlag > 0 */

    if (matchedDataObjInfo == NULL && matchedOldDataObjInfo == NULL) {
        if (dataObjInfoHead != NULL && condFlag == 0) {
            /* at least have some good copies */
            /* see if we can trim some old copies */
            matchedOldDataObjInfo = oldDataObjInfoHead;
            oldDataObjInfoHead = NULL;
            /* also trim good copy too since there is no condiftion 12/1/09 */
            matchedDataObjInfo = *dataObjInfoHead;
            *dataObjInfoHead = NULL;
        } else {
            /* don't trim anything */
            if (dataObjInfoHead != NULL ) {
                freeAllDataObjInfo (*dataObjInfoHead);
                *dataObjInfoHead = NULL; // JMC cppcheck - nullptr
            }
            freeAllDataObjInfo (oldDataObjInfoHead);
            return (0);
        }
    }

    matchedInfoCnt = getDataObjInfoCnt (matchedDataObjInfo);
    unmatchedInfoCnt = getDataObjInfoCnt (*dataObjInfoHead);
    unmatchedOldInfoCnt = getDataObjInfoCnt (oldDataObjInfoHead);

    /* free the unmatched one first */

    freeAllDataObjInfo (*dataObjInfoHead);
    freeAllDataObjInfo (oldDataObjInfoHead);
    *dataObjInfoHead = oldDataObjInfoHead = NULL;

    if ((tmpStr = getValByKey (condInput, COPIES_KW)) != NULL) {
        minCnt = atoi (tmpStr);
        if (minCnt <= 0) {
            minCnt = DEF_MIN_COPY_CNT;
        }
    } else {
        minCnt = DEF_MIN_COPY_CNT;
    }

    toTrim = unmatchedInfoCnt + matchedInfoCnt - minCnt;

    if (toTrim > matchedInfoCnt) {      /* cannot trim more than match */
        toTrim = matchedInfoCnt;
    }

    if (toTrim >= 0) {
        /* trim all old */
        *dataObjInfoHead = matchedOldDataObjInfo;

        /* take some off the bottom - since cache are queued at top. Want
         * to trim them first */
        for (i = 0; i < matchedInfoCnt - toTrim; i++) {
            prevDataObjInfo = NULL;
            tmpDataObjInfo = matchedDataObjInfo;
            while (tmpDataObjInfo != NULL) {
                if (tmpDataObjInfo->next == NULL) {
                    if (prevDataObjInfo == NULL) {

                        matchedDataObjInfo = NULL;
                    } else {
                        prevDataObjInfo->next = NULL;
                    }
                    freeDataObjInfo (tmpDataObjInfo);
                    break;
                }
                prevDataObjInfo = tmpDataObjInfo;
                tmpDataObjInfo = tmpDataObjInfo->next;
            }
        }
        queDataObjInfo (dataObjInfoHead, matchedDataObjInfo, 0, 1);
    } else {
        /* negative toTrim. see if we can trim some matchedOldDataObjInfo */
        freeAllDataObjInfo (matchedDataObjInfo);
        matchedOldInfoCnt = getDataObjInfoCnt (matchedOldDataObjInfo);
        toTrim = matchedOldInfoCnt + unmatchedOldInfoCnt + toTrim;
        if (toTrim > matchedOldInfoCnt)
            toTrim = matchedOldInfoCnt;

        if (toTrim <= 0) {
            freeAllDataObjInfo (matchedOldDataObjInfo);
        } else {
            /* take some off the bottom - since cache are queued at top. Want
             * to trim them first */
            for (i = 0; i < matchedOldInfoCnt - toTrim; i++) {
                prevDataObjInfo = NULL;
                tmpDataObjInfo = matchedOldDataObjInfo;
                while (tmpDataObjInfo != NULL) {
                    if (tmpDataObjInfo->next == NULL) {
                        if (prevDataObjInfo == NULL) {
                            matchedOldDataObjInfo = NULL;
                        } else {
                            prevDataObjInfo->next = NULL;
                        }
                        freeDataObjInfo (tmpDataObjInfo);
                        break;
                    }
                    prevDataObjInfo = tmpDataObjInfo;
                    tmpDataObjInfo = tmpDataObjInfo->next;
                }
            }
            queDataObjInfo (dataObjInfoHead, matchedOldDataObjInfo, 0, 1);
        }
    }
    return (0);
}

int
    requeDataObjInfoByDestResc (dataObjInfo_t **dataObjInfoHead,
                                keyValPair_t *condInput, int writeFlag, int topFlag)
{
    char *rescName;
    int status = -1; 
    if ((rescName = getValByKey (condInput, DEST_RESC_NAME_KW)) != NULL || 
        (rescName = getValByKey (condInput, BACKUP_RESC_NAME_KW)) != NULL ||
        (rescName = getValByKey (condInput, DEF_RESC_NAME_KW)) != NULL) { 

        
        status = requeDataObjInfoByResc (dataObjInfoHead, rescName, 
                                         writeFlag, topFlag);
    }
    return (status);
}
#if 0 // JMC - UNUSED
int
    requeDataObjInfoBySrcResc (dataObjInfo_t **dataObjInfoHead,
                               keyValPair_t *condInput, int writeFlag, int topFlag)
{
    char *rescName;

    if ((rescName = getValByKey (condInput, RESC_NAME_KW)) != NULL) {
        requeDataObjInfoByResc (dataObjInfoHead, rescName, writeFlag, topFlag);
    }
    return (0);
}
#endif // JMC - UNUSED
int
    getDataObjInfoIncSpecColl (rsComm_t *rsComm, dataObjInp_t *dataObjInp, 
                               dataObjInfo_t **dataObjInfo)
{
    int status, writeFlag;
    specCollPerm_t specCollPerm;
    writeFlag = getWriteFlag (dataObjInp->openFlags);
    if (writeFlag > 0) {
        specCollPerm = WRITE_COLL_PERM;
        if (rsComm->clientUser.authInfo.authFlag <= PUBLIC_USER_AUTH) {
            rodsLog (LOG_NOTICE,
                     "getDataObjInfoIncSpecColl:open for write not allowed for user %s",
                     rsComm->clientUser.userName);
            return (SYS_NO_API_PRIV);
        }
    } else {
        specCollPerm = READ_COLL_PERM;
    }

    if (dataObjInp->specColl != NULL && 
        dataObjInp->specColl->collClass != NO_SPEC_COLL &&
        dataObjInp->specColl->collClass != LINKED_COLL) {
        /* if it is linked, it already has been resolved */
        status = resolvePathInSpecColl (rsComm, dataObjInp->objPath,
                                        specCollPerm, 0, dataObjInfo);
        if (status == SYS_SPEC_COLL_OBJ_NOT_EXIST &&
            dataObjInfo != NULL) {
            freeDataObjInfo (*dataObjInfo);
            dataObjInfo = NULL;
        }
    } else if ((status = resolvePathInSpecColl (rsComm, dataObjInp->objPath,
                                                specCollPerm, 1, dataObjInfo)) >= 0) {
        /* check if specColl in cache. May be able to save one query */
    } else if (getValByKey (&dataObjInp->condInput,
                            IRODS_ADMIN_RMTRASH_KW) != NULL &&
               rsComm->proxyUser.authInfo.authFlag == LOCAL_PRIV_USER_AUTH) {
        status = getDataObjInfo (rsComm, dataObjInp, dataObjInfo,
                                 NULL, 0);
    } else if (writeFlag > 0 && dataObjInp->oprType != REPLICATE_OPR) {
        status = getDataObjInfo (rsComm, dataObjInp, dataObjInfo,
                                 ACCESS_DELETE_OBJECT, 0);
    } else {
        status = getDataObjInfo (rsComm, dataObjInp, dataObjInfo,
                                 ACCESS_READ_OBJECT, 0);
    }

    if (status < 0 && dataObjInp->specColl == NULL) {
        int status2;
        status2 = resolvePathInSpecColl (rsComm, dataObjInp->objPath,
                                         specCollPerm, 0, dataObjInfo);
        if (status2 < 0) {
            if (status2 == SYS_SPEC_COLL_OBJ_NOT_EXIST &&
                dataObjInfo != NULL) {
                freeDataObjInfo (*dataObjInfo);
                *dataObjInfo = NULL;
            }
        }
        if (status2 >= 0) status = 0;
    }
    if (status >= 0) {
        if ((*dataObjInfo)->specColl != NULL) {
            if ((*dataObjInfo)->specColl->collClass == LINKED_COLL) {
                /* already been tranlated */
                rstrcpy (dataObjInp->objPath, (*dataObjInfo)->objPath,
                         MAX_NAME_LEN);
                free ((*dataObjInfo)->specColl);
                (*dataObjInfo)->specColl = NULL;
            } else if (getStructFileType ((*dataObjInfo)->specColl) >= 0) {
                dataObjInp->numThreads = NO_THREADING;
            }
        }
    }
    return (status);
}

int regNewObjSize (rsComm_t *rsComm, char *objPath, int replNum,
                   rodsLong_t newSize)
{
    dataObjInfo_t dataObjInfo;
    keyValPair_t regParam;
    modDataObjMeta_t modDataObjMetaInp;
    char tmpStr[MAX_NAME_LEN]; 
    int status;

    if (objPath == NULL) return USER__NULL_INPUT_ERR;

    memset (&dataObjInfo, 0, sizeof (dataObjInfo));
    memset (&regParam, 0, sizeof (regParam));
    memset (&modDataObjMetaInp, 0, sizeof (modDataObjMetaInp));

    rstrcpy (dataObjInfo.objPath, objPath, MAX_NAME_LEN);
    dataObjInfo.replNum = replNum;
    snprintf (tmpStr, MAX_NAME_LEN, "%lld", newSize);
    addKeyVal (&regParam, DATA_SIZE_KW, tmpStr);

    modDataObjMetaInp.dataObjInfo = &dataObjInfo;
    modDataObjMetaInp.regParam = &regParam;
    status = rsModDataObjMeta (rsComm, &modDataObjMetaInp);
    if (status < 0) {
        rodsLog (LOG_ERROR,
                 "regNewObjSize: rsModDataObjMeta error for %s, status = %d",
                 objPath, status);
    }

    return (status);
}

#if 0 // JMC - legacy resource
int
getCacheDataInfoForRepl (rsComm_t *rsComm, dataObjInfo_t *srcDataObjInfoHead,
dataObjInfo_t *destDataObjInfoHead, dataObjInfo_t *compDataObjInfo, 
dataObjInfo_t **outDataObjInfo)
{
    char *rescGroupName;
    int status;

                    rescGroupName = compDataObjInfo->rescGroupName;

                    if (strlen (rescGroupName) == 0) {
                        rescGrpInfo_t *rescGrpInfo = NULL;
                        rescGrpInfo_t *tmpRescGrpInfo;
 
        /* no input rescGrp. Try to find one that matches rescInfo. */
	status = getRescGrpOfResc (rsComm, compDataObjInfo->rescInfo,&rescGrpInfo);
	  
	if (status < 0) {
	    rodsLog (LOG_NOTICE,
              "getCacheDataInfoForRepl:getRescGrpOfResc err for %s. stat=%d",
              compDataObjInfo->rescInfo->rescName, status);
	    return status;
	}
        tmpRescGrpInfo = rescGrpInfo;

        while (tmpRescGrpInfo != NULL) {
	    status = getCacheDataInfoInRescGrp (srcDataObjInfoHead,
	      destDataObjInfoHead, tmpRescGrpInfo->rescGroupName,
	      compDataObjInfo, outDataObjInfo);
	    if (status >= 0) {
		/* update the rescGroupName */
		rstrcpy (compDataObjInfo->rescGroupName, 
		  tmpRescGrpInfo->rescGroupName, NAME_LEN);
		rstrcpy ((*outDataObjInfo)->rescGroupName, 
		  tmpRescGrpInfo->rescGroupName, NAME_LEN);
		freeAllRescGrpInfo (rescGrpInfo);
		return 0;
	    } else {
	        status = getNonGrpCacheDataInfoInRescGrp (srcDataObjInfoHead,destDataObjInfoHead, tmpRescGrpInfo,compDataObjInfo, outDataObjInfo);
			if (status >= 0) {
				// =-=-=-=-=-=-=-
				// JMC - backport 4549
                /* see if we are in the same resource grp as cache */
                if (strlen ((*outDataObjInfo)->rescGroupName) > 0) {
                    rescInfo_t *myRescInfo = NULL;
                    if (getRescInGrp( rsComm, compDataObjInfo->rescInfo->rescName,
                                      (*outDataObjInfo)->rescGroupName, &myRescInfo) >= 0) {
                        rstrcpy (compDataObjInfo->rescGroupName,(*outDataObjInfo)->rescGroupName, NAME_LEN);
                        freeAllRescGrpInfo (rescGrpInfo);
                        status = SYS_NO_CACHE_RESC_IN_GRP;
                    } else {
                        status = getCacheDataInfoInRescGrp (srcDataObjInfoHead,
                                                            destDataObjInfoHead, rescGroupName, compDataObjInfo, outDataObjInfo);
                        if (status < 0) {
                            rescGrpInfo_t *tmpRescGrpInfo;
                            /* maybe the cache copy does not have a group associated with it */
                            status = resolveRescGrp (rsComm, rescGroupName, &tmpRescGrpInfo);
                            if (status >= 0) {
                                status = getNonGrpCacheDataInfoInRescGrp (srcDataObjInfoHead,
                                                                          destDataObjInfoHead, tmpRescGrpInfo,
                                                                          compDataObjInfo, outDataObjInfo);
                                if (status >= 0) {
                                    /* update the rescGroupName */
                                    rstrcpy (compDataObjInfo->rescGroupName,
                                             tmpRescGrpInfo->rescGroupName, NAME_LEN);
                                    rstrcpy ((*outDataObjInfo)->rescGroupName,
                                             tmpRescGrpInfo->rescGroupName, NAME_LEN);
                                }
                                freeAllRescGrpInfo (tmpRescGrpInfo);
                            }
                        }
                    }
                    return status;
                }

/* getNonGrpCacheDataInfoInRescGrp - get the cache dataObjInfo even
 * if it does not belong to the same group */

int
getNonGrpCacheDataInfoInRescGrp (dataObjInfo_t *srcDataObjInfoHead,
dataObjInfo_t *destDataObjInfoHead, rescGrpInfo_t *rescGrpInfo,
dataObjInfo_t *compDataObjInfo, dataObjInfo_t **outDataObjInfo)
{

    dataObjInfo_t *srcDataObjInfo;
    rescGrpInfo_t *tmpRescGrpInfo;

    srcDataObjInfo = srcDataObjInfoHead;
    while (srcDataObjInfo != NULL) {
        // every resource is a cache now, and there are no resource groups
	    // JMC - legacy resource if (getRescClass (srcDataObjInfo->rescInfo) == CACHE_CL) {
            tmpRescGrpInfo = rescGrpInfo;
            while (tmpRescGrpInfo != NULL) {
                if (strcmp (srcDataObjInfo->rescInfo->rescName,
                  tmpRescGrpInfo->rescInfo->rescName) == 0) {
                    *outDataObjInfo = srcDataObjInfo;
                    return 0;
                }
                tmpRescGrpInfo = tmpRescGrpInfo->next;
	        }
	    // JMC - legacy resource }
	
        srcDataObjInfo = srcDataObjInfo->next;
    }

    /* try destDataObjInfoHead but up to compDataObjInfo because
     * they have been updated and are good copies */

    srcDataObjInfo = destDataObjInfoHead;
    while (srcDataObjInfo != NULL) {
	if (srcDataObjInfo == compDataObjInfo) break;
        // every resource is a cache now, and there are no resource groups
        // JMC - legacy resource if (getRescClass (srcDataObjInfo->rescInfo) == CACHE_CL) {
            tmpRescGrpInfo = rescGrpInfo;
            while (tmpRescGrpInfo != NULL) {
                if (strcmp (srcDataObjInfo->rescInfo->rescName,
                  tmpRescGrpInfo->rescInfo->rescName) == 0) {
                    *outDataObjInfo = srcDataObjInfo;
                    return 0;
                }
                tmpRescGrpInfo = tmpRescGrpInfo->next;
            }
        // JMC - legacy resource }
        srcDataObjInfo = srcDataObjInfo->next;
    }
    return SYS_NO_CACHE_RESC_IN_GRP;

}

int
getCacheDataInfoInRescGrp (dataObjInfo_t *srcDataObjInfoHead,
dataObjInfo_t *destDataObjInfoHead, char *rescGroupName,
dataObjInfo_t *compDataObjInfo, dataObjInfo_t **outDataObjInfo)
{
    dataObjInfo_t *srcDataObjInfo;

    srcDataObjInfo = srcDataObjInfoHead;
    while (srcDataObjInfo != NULL) {
        if (strcmp (srcDataObjInfo->rescGroupName, rescGroupName) == 0) {
            /* same group */
            // JMC - legacy resource - if (getRescClass (srcDataObjInfo->rescInfo) == CACHE_CL) {
                *outDataObjInfo = srcDataObjInfo;
                return 0;
            // JMC - legacy resource }
        }

        srcDataObjInfo = srcDataObjInfo->next;
    }

    /* try destDataObjInfoHead but up to compDataObjInfo because
     * they have been updated and are good copies */

    srcDataObjInfo = destDataObjInfoHead;
    while (srcDataObjInfo != NULL) {
	if (srcDataObjInfo == compDataObjInfo) break;
        if (strcmp (srcDataObjInfo->rescGroupName, rescGroupName) == 0) {
            /* same group */
            // JMC - legacy resource if (getRescClass (srcDataObjInfo->rescInfo) == CACHE_CL) {
                *outDataObjInfo = srcDataObjInfo;
                return 0;
            // JMC - legacy resource }
        }
        srcDataObjInfo = srcDataObjInfo->next;
    }
    return SYS_NO_CACHE_RESC_IN_GRP;
}

int // JMC - backport 4547
getDataObjByClass ( dataObjInfo_t *dataObjInfoHead, int rescClass,
                    dataObjInfo_t **outDataObjInfo) {
    if (outDataObjInfo == NULL) return USER__NULL_INPUT_ERR;
    *outDataObjInfo = NULL;
    while (tmpDataObjInfo != NULL) {

       std::string resc_class;
       eirods::error ret =  eirods::get_resource_property( 
                                tmpDataObjInfo->rescInfo->rescName, 
                                eirods::RESOURCE_CLASS, 
                                resc_class );
       if( !ret.ok() ) {
           eirods::log( PASS( ret ) );
           return -1;
       }

       for( int i = 0; i < NumRescClass; ++i ) {
           if( resc_class == std::string( RescClass[i].className ) &&
               RescClass[i].classType == rescClass ) {
               // JMC - legacy resource - if (getRescClass (tmpDataObjInfo->rescInfo) == rescClass) {
               *outDataObjInfo = tmpDataObjInfo;
               return 0;
           }

       } // for i 

       tmpDataObjInfo = tmpDataObjInfo->next;

    } // while
    return -1;
}

#endif // JMC - legacy resource


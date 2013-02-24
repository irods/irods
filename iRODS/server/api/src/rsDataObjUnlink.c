/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See dataObjUnlink.h for a description of this API call.*/

#include "dataObjUnlink.h"
#include "rodsLog.h"
#include "icatDefines.h"
#include "fileUnlink.h"
#include "unregDataObj.h"
#include "objMetaOpr.h"
#include "dataObjOpr.h"
#include "specColl.h"
#include "resource.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "reGlobalsExtern.h"
#include "reDefines.h"
#include "rmCollOld.h"
#include "rmColl.h"
#include "dataObjRename.h"
#include "subStructFileUnlink.h"
#include "modDataObjMeta.h"
#include "phyBundleColl.h"
#include "dataObjRepl.h"
#include "regDataObj.h"
#include "physPath.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_resource_backport.h"
#include "eirods_resource_redirect.h"
#include "eirods_hierarchy_parser.h"

int
rsDataObjUnlink (rsComm_t *rsComm, dataObjInp_t *dataObjUnlinkInp)
{
    int status;
    ruleExecInfo_t rei;
    int trashPolicy;
    dataObjInfo_t *dataObjInfoHead = NULL;
    rodsServerHost_t *rodsServerHost = NULL;
    int rmTrashFlag = 0;
    specCollCache_t *specCollCache = NULL;

    resolveLinkedPath (rsComm, dataObjUnlinkInp->objPath, &specCollCache,
                       &dataObjUnlinkInp->condInput);
    status = getAndConnRcatHost (rsComm, MASTER_RCAT,
                                 dataObjUnlinkInp->objPath, &rodsServerHost);

    if (status < 0 || NULL == rodsServerHost ) { // JMC cppcheck - nullptr
        return (status);
    } else if (rodsServerHost->rcatEnabled == REMOTE_ICAT) {
        rcDataObjUnlink (rodsServerHost->conn, dataObjUnlinkInp);
        return status;
    }

#if 0
    // =-=-=-=-=-=-=-
    // working on the "home zone", determine if we need to redirect to a different
    // server in this zone for this operation.  if there is a RESC_HIER_STR_KW then
    // we know that the redirection decision has already been made
    int         local = LOCAL_HOST;
    std::string hier;
    if( getValByKey( &dataObjUnlinkInp->condInput, RESC_HIER_STR_KW ) == NULL ) {
        rodsServerHost_t* host  =  0;
        eirods::error ret = eirods::resource_redirect( eirods::EIRODS_OPEN_OPERATION, rsComm, 
                                                       dataObjUnlinkInp, hier, host, local );
        if( !ret.ok() ) { 
            std::stringstream msg;
            msg << "rsDataObjUnlink :: failed in eirods::resource_redirect for [";
            msg << dataObjUnlinkInp->objPath << "]";
            eirods::log( PASSMSG( msg.str(), ret ) );
            return ret.code();
        }
       
        // =-=-=-=-=-=-=-
        // we resolved the redirect and have a host, set the hier str for subsequent
        // api calls, etc.
        addKeyVal( &dataObjUnlinkInp->condInput, RESC_HIER_STR_KW, hier.c_str() );

    } // if keyword

    if( LOCAL_HOST != local ) {
        rodsLog( LOG_NOTICE, "%s - resource_redirect requested remote server for [%s], hier string [%s]", 
                 __FUNCTION__, dataObjUnlinkInp->objPath, hier.c_str() );
    }
#endif

    if (getValByKey (
            &dataObjUnlinkInp->condInput, IRODS_ADMIN_RMTRASH_KW) != NULL ||
        getValByKey (
            &dataObjUnlinkInp->condInput, IRODS_RMTRASH_KW) != NULL) {
        if (isTrashPath (dataObjUnlinkInp->objPath) == False) {
            return (SYS_INVALID_FILE_PATH);
        }
        rmTrashFlag = 1;
    }

    dataObjUnlinkInp->openFlags = O_WRONLY;  /* set the permission checking */
    status = getDataObjInfoIncSpecColl (rsComm, dataObjUnlinkInp, 
                                        &dataObjInfoHead);

    if (status < 0) return (status);

    if (rmTrashFlag == 1) {
        char *tmpAge;
        int ageLimit;
        if ((tmpAge = getValByKey (&dataObjUnlinkInp->condInput, AGE_KW))
          != NULL) {
           ageLimit = atoi (tmpAge) * 60;
           if ((time (0) - atoi (dataObjInfoHead->dataModify)) < ageLimit) {
               /* younger than ageLimit. Nothing to do */
               freeAllDataObjInfo (dataObjInfoHead);
               return 0;
           }
       }
    }

    if (dataObjUnlinkInp->oprType == UNREG_OPR ||
        getValByKey (&dataObjUnlinkInp->condInput, FORCE_FLAG_KW) != NULL ||
        getValByKey (&dataObjUnlinkInp->condInput, REPL_NUM_KW) != NULL ||
        dataObjInfoHead->specColl != NULL || rmTrashFlag == 1) {
        status = _rsDataObjUnlink (rsComm, dataObjUnlinkInp, &dataObjInfoHead);
    } else {
        initReiWithDataObjInp (&rei, rsComm, dataObjUnlinkInp);
        status = applyRule ("acTrashPolicy", NULL, &rei, NO_SAVE_REI);
        trashPolicy = rei.status;

        if (trashPolicy != NO_TRASH_CAN) {
            status = rsMvDataObjToTrash (rsComm, dataObjUnlinkInp, 
                                         &dataObjInfoHead);
            freeAllDataObjInfo (dataObjInfoHead);
            return status;
        } else {
            status = _rsDataObjUnlink (rsComm, dataObjUnlinkInp, 
                                       &dataObjInfoHead);
        }
    }

    initReiWithDataObjInp (&rei, rsComm, dataObjUnlinkInp);
    rei.doi = dataObjInfoHead;
    rei.status = status;
    rei.status = applyRule ("acPostProcForDelete", NULL, &rei, NO_SAVE_REI);

    if (rei.status < 0) {
        rodsLog (LOG_NOTICE,
                 "rsDataObjUnlink: acPostProcForDelete error for %s. status = %d",
                 dataObjUnlinkInp->objPath, rei.status);
    }

    /* dataObjInfoHead may be outdated */
    freeAllDataObjInfo (dataObjInfoHead);

    return (status);
}

int
_rsDataObjUnlink (rsComm_t *rsComm, dataObjInp_t *dataObjUnlinkInp,
                  dataObjInfo_t **dataObjInfoHead)
{
    int status;
    int retVal = 0;
    dataObjInfo_t *tmpDataObjInfo, *myDataObjInfoHead;

    status = chkPreProcDeleteRule (rsComm, dataObjUnlinkInp, *dataObjInfoHead);
    if (status < 0) return status;


    myDataObjInfoHead = *dataObjInfoHead;
    if (strstr (myDataObjInfoHead->dataType, BUNDLE_STR) != NULL) { // JMC - backport 4658
        int numSubfiles; // JMC - backport 4552
        if (rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
            return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
        }
        if (getValByKey (&dataObjUnlinkInp->condInput, REPL_NUM_KW) != NULL) {
            return SYS_CANT_MV_BUNDLE_DATA_BY_COPY;
        }
                
        // =-=-=-=-=-=-=-
        // JMC - backport 4552
        numSubfiles = getNumSubfilesInBunfileObj (rsComm, myDataObjInfoHead->objPath);
        if (numSubfiles > 0) {
            if (getValByKey (&dataObjUnlinkInp->condInput, EMPTY_BUNDLE_ONLY_KW) != NULL) {
                /* not empty. Nothing to do */
                return 0;
            } else {
                status = _unbunAndStageBunfileObj (rsComm, dataObjInfoHead, NULL, 1);
                if (status < 0) {
                    /* go ahead and unlink the obj if the phy file does not 
                     * exist or have problem untaring it */
                    if (getErrno (status) != EEXIST && getIrodsErrno (status) != SYS_TAR_STRUCT_FILE_EXTRACT_ERR) {
                        rodsLogError( LOG_ERROR, status, "_rsDataObjUnlink:_unbunAndStageBunfileObj err for %s",
                                      myDataObjInfoHead->objPath);
                        return (status);
                    }
                } // status < 0
                /* dataObjInfoHead may be outdated */
                *dataObjInfoHead = NULL;
                status = getDataObjInfoIncSpecColl (rsComm, dataObjUnlinkInp, dataObjInfoHead );

                if (status < 0) return (status);
            } // else
        } // if numSubfiles
    } // if strcmp
    // =-=-=-=-=-=-=-

    tmpDataObjInfo = *dataObjInfoHead;
    while (tmpDataObjInfo != NULL) {
        status = dataObjUnlinkS (rsComm, dataObjUnlinkInp, tmpDataObjInfo);
        if (status < 0) {
            if (retVal == 0) {
                retVal = status;
            }
        }
        if (dataObjUnlinkInp->specColl != NULL)         /* do only one */
            break;
        tmpDataObjInfo = tmpDataObjInfo->next;
    }

    if ((*dataObjInfoHead)->specColl == NULL)
        resolveDataObjReplStatus (rsComm, dataObjUnlinkInp);

    return (retVal);
}

/* resolveDataObjReplStatus - a dirty copy may be deleted leaving no
 * dirty copy. In that case, pick the newest copy and mark it dirty
 */
int
resolveDataObjReplStatus (rsComm_t *rsComm, dataObjInp_t *dataObjUnlinkInp)
{
    int status;
    dataObjInfo_t *dataObjInfoHead = NULL;
    dataObjInfo_t *newestDataObjInfo = NULL;
    dataObjInfo_t *tmpDataObjInfo;

    if (getValByKey (&dataObjUnlinkInp->condInput, RESC_NAME_KW) == NULL &&
        getValByKey (&dataObjUnlinkInp->condInput, REPL_NUM_KW) == NULL) {
        return 0;
    } 
    status = getDataObjInfo (rsComm, dataObjUnlinkInp,
                             &dataObjInfoHead, ACCESS_DELETE_OBJECT, 1);

    if (status < 0) return status;

    tmpDataObjInfo = dataObjInfoHead;
    while (tmpDataObjInfo != NULL) {
        if (tmpDataObjInfo->replStatus == 0) {
            if (newestDataObjInfo == NULL) {
                newestDataObjInfo = tmpDataObjInfo;
            } else if (atoi (tmpDataObjInfo->dataModify) >
                       atoi (newestDataObjInfo->dataModify)) {
                newestDataObjInfo = tmpDataObjInfo;
            }
        } else {
            newestDataObjInfo = NULL;
            break;
        }
        tmpDataObjInfo = tmpDataObjInfo->next;
    }

    /* modify the repl status */
    if (newestDataObjInfo != NULL) {
        keyValPair_t regParam;
        char tmpStr[MAX_NAME_LEN];
        modDataObjMeta_t modDataObjMetaInp;

        memset (&regParam, 0, sizeof (regParam));
        memset (&modDataObjMetaInp, 0, sizeof (modDataObjMetaInp));
        snprintf (tmpStr, MAX_NAME_LEN, "%d", NEWLY_CREATED_COPY);
        addKeyVal (&regParam, REPL_STATUS_KW, tmpStr);
        modDataObjMetaInp.dataObjInfo = newestDataObjInfo;
        modDataObjMetaInp.regParam = &regParam;

        status = rsModDataObjMeta (rsComm, &modDataObjMetaInp);

        clearKeyVal (&regParam);
    }
    freeAllDataObjInfo (dataObjInfoHead);
    return (status);
}

int
dataObjUnlinkS (rsComm_t *rsComm, dataObjInp_t *dataObjUnlinkInp,
                dataObjInfo_t *dataObjInfo)
{
    int status;
    unregDataObj_t unregDataObjInp;

    if( dataObjInfo->specColl == NULL ) {
        if( dataObjUnlinkInp->oprType            == UNREG_OPR && 
            rsComm->clientUser.authInfo.authFlag != LOCAL_PRIV_USER_AUTH ) {
            ruleExecInfo_t rei;

        initReiWithDataObjInp (&rei, rsComm, dataObjUnlinkInp);
        rei.doi = dataObjInfo;
        rei.status = DO_CHK_PATH_PERM;         /* default */ // JMC - backport 4758
        
        applyRule ("acSetChkFilePathPerm", NULL, &rei, NO_SAVE_REI);
        if (rei.status != NO_CHK_PATH_PERM) {
            char *outVaultPath;
            rodsServerHost_t *rodsServerHost;
            status = resolveHostByRescInfo (dataObjInfo->rescInfo, 
              &rodsServerHost);
            if (status < 0) return status;
            /* unregistering but not an admin user */
            status = matchVaultPath (rsComm, dataObjInfo->filePath, rodsServerHost, &outVaultPath);
            if (status != 0) {
            /* in the vault */
                    rodsLog (LOG_DEBUG,
                             "dataObjUnlinkS: unregistering in vault file %s",
                             dataObjInfo->filePath);
                    return CANT_UNREG_IN_VAULT_FILE;
                }
            }
        }
        
        unregDataObjInp.dataObjInfo = dataObjInfo;
        unregDataObjInp.condInput = &dataObjUnlinkInp->condInput;
        status = rsUnregDataObj (rsComm, &unregDataObjInp);

        if (status < 0) {
            rodsLog (LOG_NOTICE,
                     "dataObjUnlinkS: rsUnregDataObj error for %s. status = %d",
                     dataObjUnlinkInp->objPath, status);
            return status;
        }
    }
    
    if (dataObjUnlinkInp->oprType != UNREG_OPR) {
        status = l3Unlink (rsComm, dataObjInfo);
        if (status < 0) {
            int myError = getErrno (status);
            rodsLog (LOG_NOTICE,
                     "dataObjUnlinkS: l3Unlink error for %s. status = %d",
                     dataObjUnlinkInp->objPath, status);
            /* allow ENOENT to go on and unregister */
            if (myError != ENOENT && myError != EACCES) {
                char orphanPath[MAX_NAME_LEN];
                int status1 = 0;
                rodsLog (LOG_NOTICE,
                         "dataObjUnlinkS: orphan file %s", dataObjInfo->filePath);
                while (1) { 
                    if (isOrphanPath (dataObjUnlinkInp->objPath) == 
                        NOT_ORPHAN_PATH) {
                        /* don't rename orphan path */
                        status1 = rsMkOrphanPath (rsComm, dataObjInfo->objPath,
                                                  orphanPath);
                        if (status1 < 0) break;
                        /* reg the orphan path */
                        rstrcpy (dataObjInfo->objPath, orphanPath,MAX_NAME_LEN);
                    }
                    status1 = svrRegDataObj (rsComm, dataObjInfo);
                    if (status1 == CAT_NAME_EXISTS_AS_DATAOBJ ||
                        status1 == CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME) {
                        continue;
                    } else if (status1 < 0) {
                        rodsLogError (LOG_ERROR, status1,
                                      "dataObjUnlinkS: svrRegDataObj of orphan %s error",
                                      dataObjInfo->objPath);
                    }
                    break;
                }
                return (status);
            } else {
                status = 0;
            }
        }
    }

    return (status);
}

int
l3Unlink (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo)
{
    fileUnlinkInp_t fileUnlinkInp;
    int status;

    // =-=-=-=-=-=-=-
    // JMC - legacy resource  if (getRescClass (dataObjInfo->rescInfo) == BUNDLE_CL) return 0;
    std::string resc_class;
    eirods::error prop_err = eirods::get_resource_property<std::string>( dataObjInfo->rescInfo->rescName, "class", resc_class );
    if( prop_err.ok() ) {
        if( resc_class == "bundle" ) {//BUNDLE_CL ) {
            return 0;
        }
    } else {
        std::stringstream msg;
        msg << "l3Unlink - failed to get proprty [class] for resource [";
        msg << dataObjInfo->rescInfo->rescName;
        msg << "]";
        eirods::log( ERROR( -1, msg.str() ) );
        return -1;
    }
    // =-=-=-=-=-=-=-

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    eirods::error ret = eirods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if( !ret.ok() ) {
        eirods::log( PASSMSG( "l3Unlink - failed in get_loc_for_hier_String", ret ) );
        return -1;
    }


    if (dataObjInfo->rescInfo->rescStatus == INT_RESC_STATUS_DOWN) 
        return SYS_RESC_IS_DOWN;

    if (getStructFileType (dataObjInfo->specColl) >= 0) {
        subFile_t subFile;
        memset (&subFile, 0, sizeof (subFile));
        rstrcpy ( subFile.subFilePath, dataObjInfo->subPath,MAX_NAME_LEN);
        rstrcpy ( subFile.addr.hostAddr, location.c_str(), NAME_LEN );
        subFile.specColl = dataObjInfo->specColl;
        status = rsSubStructFileUnlink (rsComm, &subFile);
    } else {
#if 0 // JMC - legacy resource 
        rescTypeInx = dataObjInfo->rescInfo->rescTypeInx;
        switch (RescTypeDef[rescTypeInx].rescCat) {
        case FILE_CAT:
#endif // JMC - legacy resource 
            memset (&fileUnlinkInp, 0, sizeof (fileUnlinkInp));
            fileUnlinkInp.fileType = static_cast< fileDriverType_t >( -1 );//= (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
            rstrcpy (fileUnlinkInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN);
            rstrcpy (fileUnlinkInp.rescHier, dataObjInfo->rescHier, MAX_NAME_LEN);
            rstrcpy (fileUnlinkInp.addr.hostAddr, location.c_str(), NAME_LEN);
            rstrcpy (fileUnlinkInp.objPath, dataObjInfo->objPath, MAX_NAME_LEN);
            status = rsFileUnlink (rsComm, &fileUnlinkInp);
#if 0 // JMC - legacy resource 
            break;

        default:
            rodsLog (LOG_NOTICE,
                     "l3Unlink: rescCat type %d is not recognized",
                     RescTypeDef[rescTypeInx].rescCat);
            status = SYS_INVALID_RESC_TYPE;
            break;
	}
#endif // JMC - legacy resource 
    }
    return (status);
}

int
rsMvDataObjToTrash (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                    dataObjInfo_t **dataObjInfoHead)
{
    int status;
    char trashPath[MAX_NAME_LEN];
    dataObjCopyInp_t dataObjRenameInp;

    if (strstr ((*dataObjInfoHead)->dataType, BUNDLE_STR) != NULL) { // JMC - backport 4658
        return SYS_CANT_MV_BUNDLE_DATA_TO_TRASH;
    }

    if (getValByKey (&dataObjInp->condInput, DATA_ACCESS_KW) == NULL) {
        addKeyVal (&dataObjInp->condInput, DATA_ACCESS_KW,
                   ACCESS_DELETE_OBJECT);
    }

    status = getDataObjInfo (rsComm, dataObjInp, dataObjInfoHead,
                             ACCESS_DELETE_OBJECT, 0);
    if (status < 0) {
        rodsLog (LOG_NOTICE,
                 "rsMvDataObjToTrash: getDataObjInfo error for %s. status = %d",
                 dataObjInp->objPath, status);
        return (status);
    }

    status = chkPreProcDeleteRule (rsComm, dataObjInp, *dataObjInfoHead);
    if (status < 0) return status;


    status = rsMkTrashPath (rsComm, dataObjInp->objPath, trashPath);

    if (status < 0) {
        return (status);
    }

    memset (&dataObjRenameInp, 0, sizeof (dataObjRenameInp));

    dataObjRenameInp.srcDataObjInp.oprType =
        dataObjRenameInp.destDataObjInp.oprType = RENAME_DATA_OBJ;

    rstrcpy (dataObjRenameInp.destDataObjInp.objPath, trashPath, MAX_NAME_LEN);
    rstrcpy (dataObjRenameInp.srcDataObjInp.objPath, dataObjInp->objPath,
             MAX_NAME_LEN);

    status = rsDataObjRename (rsComm, &dataObjRenameInp);

    while (status == CAT_NAME_EXISTS_AS_DATAOBJ || 
           status == CAT_NAME_EXISTS_AS_COLLECTION || 
           status == SYS_PHY_PATH_INUSE || 
           getErrno (status) == EISDIR) {
        appendRandomToPath (dataObjRenameInp.destDataObjInp.objPath);
        status = rsDataObjRename (rsComm, &dataObjRenameInp);
    }
    if (status < 0) {
        rodsLog (LOG_ERROR,
                 "rsMvDataObjToTrash: rcDataObjRename error for %s, status = %d",
                 dataObjRenameInp.destDataObjInp.objPath, status);
        return (status);
    }
    return (status);
}

int
chkPreProcDeleteRule (rsComm_t *rsComm, dataObjInp_t *dataObjUnlinkInp,
                      dataObjInfo_t *dataObjInfoHead)
{
    dataObjInfo_t *tmpDataObjInfo;
    ruleExecInfo_t rei;
    int status = 0;

    initReiWithDataObjInp (&rei, rsComm, dataObjUnlinkInp);
    tmpDataObjInfo = dataObjInfoHead;
    while (tmpDataObjInfo != NULL) {
        /* have to go through the loop to test each copy (resource). */
        rei.doi = tmpDataObjInfo;

        status = applyRule ("acDataDeletePolicy", NULL, &rei, NO_SAVE_REI);

        if (status < 0 && status != NO_MORE_RULES_ERR &&
            status != SYS_DELETE_DISALLOWED) {
            rodsLog (LOG_ERROR,
                     "chkPreProcDeleteRule: acDataDeletePolicy err for %s. stat = %d",
                     dataObjUnlinkInp->objPath, status);
            return (status);
        }

        if (rei.status == SYS_DELETE_DISALLOWED) {
            rodsLog (LOG_ERROR,
                     "chkPreProcDeleteRule:acDataDeletePolicy disallowed delete of %s",
                     dataObjUnlinkInp->objPath);
            return (rei.status);
        }
        tmpDataObjInfo = tmpDataObjInfo->next;
    }
    return status;
}

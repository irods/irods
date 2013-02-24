/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsPhyBundleColl.c. See phyBundleColl.h for a description of
 * this API call.*/

#include "phyBundleColl.h"
#include "objMetaOpr.h"
#include "resource.h"
#include "collection.h"
#include "specColl.h"
#include "physPath.h"
#include "dataObjOpr.h"
#include "miscServerFunct.h"
#include "openCollection.h"
#include "readCollection.h"
#include "closeCollection.h"
#include "dataObjRepl.h"
#include "dataObjUnlink.h"
#include "dataObjCreate.h"
#include "syncMountedColl.h"
#include "regReplica.h"
#include "unbunAndRegPhyBunfile.h"
#include "fileChksum.h"
#include "eirods_stacktrace.h"

// =-=-=-=-=-=-=-
// eirods resource includes
#include "eirods_resource_backport.h"
#include "eirods_resource_redirect.h"
#include "eirods_stacktrace.h"


static rodsLong_t OneGig = (1024*1024*1024);

int
rsPhyBundleColl( rsComm_t*                 rsComm, 
                 structFileExtAndRegInp_t* phyBundleCollInp ) {
    int               status         = -1; 
    specCollCache_t*  specCollCache  = 0;
    char*             destRescName   = 0;

    resolveLinkedPath (rsComm, phyBundleCollInp->objPath, &specCollCache,
                       &phyBundleCollInp->condInput);

    resolveLinkedPath (rsComm, phyBundleCollInp->collection,
                       &specCollCache, NULL);

    if ((destRescName = getValByKey (&phyBundleCollInp->condInput, 
                                     DEST_RESC_NAME_KW)) == NULL) {
        return USER_NO_RESC_INPUT_ERR;
    }

    if (isLocalZone (phyBundleCollInp->collection) == 0) { 
        /* can only do local zone */
        return SYS_INVALID_ZONE_NAME;
    }

#if 0 // JMC - legacy resource
    status = _getRescInfo (rsComm, destRescName, &rescGrpInfo);
    if (status < 0 || NULL == rescGrpInfo ) { // JMC cppcheck - nullptr
        rodsLog (LOG_ERROR,
                 "rsPhyBundleColl: _getRescInfo of %s error for %s. stat = %d",
                 destRescName, phyBundleCollInp->collection, status);
        return status;
    }
#endif // JMC - legacy resource

    rescGrpInfo_t rescGrpInfo;
    rescGrpInfo.rescInfo = 0;
    eirods::error err = eirods::get_resc_grp_info( destRescName, rescGrpInfo );
    if( !err.ok() ) {
        eirods::log( PASS( false, -1, "rsPhyBundleColl - failed.", err ) );
        return -1;
    }

    // =-=-=-=-=-=-=-
    // working on the "home zone", determine if we need to redirect to a different
    // server in this zone for this operation.  if there is a RESC_HIER_STR_KW then
    // we know that the redirection decision has already been made
    dataObjInp_t      data_inp;
    bzero( &data_inp, sizeof( data_inp ) );
    rstrcpy( data_inp.objPath, phyBundleCollInp->objPath, NAME_LEN );
    bzero( &data_inp.condInput, sizeof( data_inp.condInput ) );
    addKeyVal( &data_inp.condInput, DEST_RESC_NAME_KW, destRescName );

    std::string       hier;
    int               local = LOCAL_HOST;
    rodsServerHost_t* host  =  0;
    char* hier_kw = getValByKey( &phyBundleCollInp->condInput, RESC_HIER_STR_KW );
    if( hier_kw == NULL ) {
        eirods::error ret = eirods::resource_redirect( eirods::EIRODS_CREATE_OPERATION, rsComm, 
						   &data_inp, hier, host, local );
        if( !ret.ok() ) { 
            std::stringstream msg;
            msg << "rsPhyBundleColl - failed in eirods::resource_redirect for [";
            msg << data_inp.objPath << "]";
            eirods::log( PASSMSG( msg.str(), ret ) );
            return ret.code();
        }
   
        // =-=-=-=-=-=-=-
        // we resolved the redirect and have a host, set the hier str for subsequent
        // api calls, etc.
        addKeyVal( &phyBundleCollInp->condInput, RESC_HIER_STR_KW, hier.c_str() );

    }

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    eirods::error ret = eirods::get_loc_for_hier_string( hier, location );
    if( !ret.ok() ) {
        eirods::log( PASSMSG( "rsPhyBundleColl - failed in get_loc_for_hier_String", ret ) );
        return -1;
    }



    rodsHostAddr_t rescAddr;
    bzero (&rescAddr, sizeof (rescAddr));

    rstrcpy (rescAddr.hostAddr, location.c_str(), NAME_LEN);
    rodsServerHost_t* rodsServerHost = 0;
    int remoteFlag = resolveHost (&rescAddr, &rodsServerHost);



    if (remoteFlag == LOCAL_HOST) {
        status = _rsPhyBundleColl( rsComm, phyBundleCollInp, &rescGrpInfo );
    } else if (remoteFlag == REMOTE_HOST) {
        status = remotePhyBundleColl( rsComm, phyBundleCollInp, rodsServerHost );
    } else if (remoteFlag < 0) {
        status = remoteFlag;
    }

    
    return status;
}

int
_rsPhyBundleColl( rsComm_t*                 rsComm, 
                  structFileExtAndRegInp_t* phyBundleCollInp,
                  rescGrpInfo_t*            rescGrpInfo ) {
    rescInfo_t* myRescInfo = rescGrpInfo->rescInfo;
    char*       myRescName = myRescInfo->rescName;
    
    collInp_t collInp;
    bzero (&collInp, sizeof (collInp));
    rstrcpy (collInp.collName, phyBundleCollInp->collection, MAX_NAME_LEN);
    collInp.flags = RECUR_QUERY_FG | VERY_LONG_METADATA_FG | NO_TRIM_REPL_FG;
        
    int handleInx = rsOpenCollection (rsComm, &collInp);

    if (handleInx < 0) {
        rodsLog (LOG_ERROR,
                 "_rsPhyBundleColl: rsOpenCollection of %s error. status = %d",
                 collInp.collName, handleInx);
        return (handleInx);
    }

    if (CollHandle[handleInx].rodsObjStat->specColl != NULL) {
        rodsLog (LOG_ERROR,
                 "_rsPhyBundleColl: unable to bundle special collection %s",
                 collInp.collName);
        rsCloseCollection (rsComm, &handleInx);
        return (0);
    }

    /* create the bundle file */ 
    char* dataType  = getValByKey (&phyBundleCollInp->condInput, DATA_TYPE_KW); // JMC - backport 4658
    
    dataObjInp_t dataObjInp;
    int   l1descInx = createPhyBundleDataObj (rsComm, phyBundleCollInp->collection,
                                              rescGrpInfo, &dataObjInp, dataType ); // JMC - backport 4658

    if (l1descInx < 0) {
        return l1descInx;
    }

    // =-=-=-=-=-=-=-
    // JMC - backport 4528
    int chksumFlag    = -1;
    if (getValByKey (&phyBundleCollInp->condInput, VERIFY_CHKSUM_KW) != NULL) {
        L1desc[l1descInx].chksumFlag = VERIFY_CHKSUM;
        chksumFlag = 1;
    } else {
        chksumFlag = 0;
    }
    // =-=-=-=-=-=-=-
    // JMC - backport 4771
    int maxSubFileCnt = -1; // JMC - backport 4528, 4771
    if (getValByKey (&phyBundleCollInp->condInput, MAX_SUB_FILE_KW) != NULL) {
        maxSubFileCnt = atoi(getValByKey (&phyBundleCollInp->condInput, MAX_SUB_FILE_KW));
    } else {
        maxSubFileCnt = MAX_SUB_FILE_CNT;
    }

    // =-=-=-=-=-=-=-
    char phyBunDir[MAX_NAME_LEN];
    createPhyBundleDir (rsComm, L1desc[l1descInx].dataObjInfo->filePath, 
                        phyBunDir);


    curSubFileCond_t     curSubFileCond;
    bunReplCacheHeader_t bunReplCacheHeader;
    bzero (&bunReplCacheHeader, sizeof (bunReplCacheHeader));
    bzero (&curSubFileCond, sizeof (curSubFileCond));
    
    int        status      = -1;
    int        savedStatus =  0;
    collEnt_t* collEnt     =  0;
    while ((status = rsReadCollection (rsComm, &handleInx, &collEnt)) >= 0) {
        if (collEnt->objType == DATA_OBJ_T) {
            if (curSubFileCond.collName[0] == '\0') {
                /* a new dataObj.  */
                rstrcpy (curSubFileCond.collName, collEnt->collName, 
                         MAX_NAME_LEN);
                rstrcpy (curSubFileCond.dataName, collEnt->dataName, 
                         MAX_NAME_LEN);
                curSubFileCond.dataId = strtoll (collEnt->dataId, 0, 0);
            } else if (strcmp (curSubFileCond.collName, collEnt->collName) != 0
                       || strcmp (curSubFileCond.dataName, collEnt->dataName) != 0) {
                /* a new file, need to handle the old one */
                if (bunReplCacheHeader.numSubFiles >= maxSubFileCnt || // JMC - backport 4771
                    bunReplCacheHeader.totSubFileSize + collEnt->dataSize > 
                    MAX_BUNDLE_SIZE * OneGig) {
                    /* bundle is full */
                    status = bundlleAndRegSubFiles (rsComm, l1descInx,
                                                    phyBunDir, phyBundleCollInp->collection,
                                                    &bunReplCacheHeader, chksumFlag); // JMC - backport 4528
                    if (status < 0) {
                        rodsLog (LOG_ERROR,
                                 "_rsPhyBundleColl:bunAndRegSubFiles err for %s,stst=%d",
                                 phyBundleCollInp->collection, status);
                        savedStatus = status;
                    } else {
                        /* create a new bundle file */
                        l1descInx = createPhyBundleDataObj (rsComm,
                                                            phyBundleCollInp->collection, rescGrpInfo,
                                                            &dataObjInp, dataType); // JMC - backport 4658

                        if (l1descInx < 0) {
                            rodsLog (LOG_ERROR,
                                     "_rsPhyBundleColl:createPhyBundleDataObj err for %s,stat=%d",
                                     phyBundleCollInp->collection, l1descInx);
                            return (l1descInx);
                        }

                        createPhyBundleDir (rsComm,
                                            L1desc[l1descInx].dataObjInfo->filePath, phyBunDir);
                        /* need to reset subPhyPath since phyBunDir has 
                         * changed */
                        /* At this point subPhyPath[0] == 0 if it has gone
                         * through replAndAddSubFileToDir below. != 0 if it has
                         * not and already a good cache copy */
                        if (curSubFileCond.subPhyPath[0] != '\0')
                            setSubPhyPath (phyBunDir, curSubFileCond.dataId,
                                           curSubFileCond.subPhyPath);

                    }
                }       /* end of new bundle file */
                status = replAndAddSubFileToDir (rsComm, &curSubFileCond, myRescName, phyBunDir, &bunReplCacheHeader);
                if (status < 0) {
                    savedStatus = status;
                    rodsLog (LOG_ERROR,
                             "_rsPhyBundleColl:replAndAddSubFileToDir err for %s,sta=%d",
                             curSubFileCond.subPhyPath, status);
                }
                curSubFileCond.bundled = 0;
                curSubFileCond.subPhyPath[0] = 
                    curSubFileCond.cachePhyPath[0] = '\0';
                rstrcpy (curSubFileCond.collName, collEnt->collName, 
                         MAX_NAME_LEN);
                rstrcpy (curSubFileCond.dataName, collEnt->dataName, 
                         MAX_NAME_LEN);
                curSubFileCond.dataId = strtoll (collEnt->dataId, 0, 0);
            }   /* end of  name compare */
 
            if (curSubFileCond.bundled > 0) {
                /* already bundled. skip */
            } else if (isDataObjBundled (rsComm, collEnt)) {
                /* already bundled, skip */
                curSubFileCond.bundled = 1;
                curSubFileCond.subPhyPath[0] = '\0';
                curSubFileCond.cachePhyPath[0] = '\0';
                /* XXXX there was a bug that if dataSize == 0, replStatus is 0. 
                 * This bug has been fixed since 3.1 */
            } else if( ( collEnt->replStatus > 0 || curSubFileCond.subPhyPath[0] == '\0') &&  // JMC - backport 4755
                       strcmp (collEnt->resource, myRescName) == 0) {
                /* have a good copy in cache resource */
                setSubPhyPath (phyBunDir, curSubFileCond.dataId, curSubFileCond.subPhyPath);
                rstrcpy (curSubFileCond.cachePhyPath, collEnt->phyPath, MAX_NAME_LEN);
                curSubFileCond.cacheReplNum = collEnt->replNum;
                curSubFileCond.subFileSize = collEnt->dataSize;
            }
        
        } // if data obj

        free (collEnt);     /* just free collEnt but not content */

    } // while 
    /* handle any remaining */

    status = replAndAddSubFileToDir (rsComm, &curSubFileCond,
                                     myRescName, phyBunDir, &bunReplCacheHeader);
    if (status < 0) {
        savedStatus = status;
        rodsLog (LOG_ERROR,
                 "_rsPhyBundleColl:replAndAddSubFileToDir err for %s,stat=%d",
                 curSubFileCond.subPhyPath, status);
    }

    status = bundlleAndRegSubFiles (rsComm, l1descInx, phyBunDir, 
                                    phyBundleCollInp->collection, &bunReplCacheHeader, chksumFlag); // JMC - backport 4528
    if (status < 0) {
        rodsLog (LOG_ERROR,
                 "_rsPhyBundleColl:bunAndRegSubFiles err for %s,stat=%d",
                 phyBundleCollInp->collection, status);
    }
    if (status >= 0 && savedStatus < 0) {
        return savedStatus;
    } else {
        return status;
    }
}

int
replAndAddSubFileToDir (rsComm_t *rsComm, curSubFileCond_t *curSubFileCond,
                        char *myRescName, char *phyBunDir, bunReplCacheHeader_t *bunReplCacheHeader)
{
    int status;
    dataObjInfo_t dataObjInfo;

    if (curSubFileCond->bundled == 1) {
        return 0; 
    }

    bzero (&dataObjInfo, sizeof (dataObjInfo));
    /* next dataObj. See if we need to replicate */
    if (curSubFileCond->subPhyPath[0] == '\0') {
        /* don't have a good cache copy yet. make one */
        status = replDataObjForBundle (rsComm, curSubFileCond->collName,
                                       curSubFileCond->dataName, myRescName, 
                                       0, 0, 1, &dataObjInfo);
        if (status >= 0) {
            setSubPhyPath (phyBunDir, curSubFileCond->dataId, 
                           curSubFileCond->subPhyPath);
            rstrcpy (curSubFileCond->cachePhyPath, dataObjInfo.filePath, 
                     MAX_NAME_LEN);
            curSubFileCond->cacheReplNum = dataObjInfo.replNum;
            curSubFileCond->subFileSize = dataObjInfo.dataSize;
        }
    }
    status = addSubFileToDir (curSubFileCond, bunReplCacheHeader);
    if (status < 0) {
        rodsLog (LOG_ERROR,
                 "_rsPhyBundleColl:addSubFileToDir error for %s,stst=%d",
                 curSubFileCond->subPhyPath, status);
    }
    return status;
}

int
bundlleAndRegSubFiles (rsComm_t *rsComm, int l1descInx, char *phyBunDir, 
                       char *collection, bunReplCacheHeader_t *bunReplCacheHeader, int chksumFlag) // JMC - backport 4528
{
    int status;
    openedDataObjInp_t dataObjCloseInp;
    bunReplCache_t *tmpBunReplCache, *nextBunReplCache;
    regReplica_t regReplicaInp;
    dataObjInp_t dataObjUnlinkInp;
    keyValPair_t regParam; // JMC - backport 4528
    modDataObjMeta_t modDataObjMetaInp; // JMC - backport 4528

    int savedStatus = 0;

    bzero (&dataObjCloseInp, sizeof (dataObjCloseInp));
    dataObjCloseInp.l1descInx = l1descInx;
    if (bunReplCacheHeader->numSubFiles == 0) {
        bzero (&dataObjUnlinkInp, sizeof (dataObjUnlinkInp));
        rstrcpy (dataObjUnlinkInp.objPath,
                 L1desc[l1descInx].dataObjInfo->objPath, MAX_NAME_LEN);
        dataObjUnlinkS (rsComm, &dataObjUnlinkInp,
                        L1desc[l1descInx].dataObjInfo);
        L1desc[l1descInx].bytesWritten = 0;
        rsDataObjClose (rsComm, &dataObjCloseInp);
        bzero (bunReplCacheHeader, sizeof (bunReplCacheHeader_t));
        return 0;
    }

    status = phyBundle (rsComm, L1desc[l1descInx].dataObjInfo, phyBunDir,
                        collection, CREATE_TAR_OPR); // JMC - backport 4643
    if (status < 0) {
        rodsLog (LOG_ERROR,
                 "bundlleAndRegSubFiles: rsStructFileSync of %s error. stat = %d",
                 L1desc[l1descInx].dataObjInfo->objPath, status);
        rmLinkedFilesInUnixDir (phyBunDir);
        rmdir (phyBunDir);
        rsDataObjClose (rsComm, &dataObjCloseInp);
        tmpBunReplCache = bunReplCacheHeader->bunReplCacheHead;
        while (tmpBunReplCache != NULL) {
            nextBunReplCache = tmpBunReplCache->next;
            free (tmpBunReplCache);
            tmpBunReplCache = nextBunReplCache; // JMC - backport 4579
        }
        bzero (bunReplCacheHeader, sizeof (bunReplCacheHeader_t));
        return status;
    } else {
        /* mark it was written so the size would be adjusted */
        L1desc[l1descInx].bytesWritten = 1;
    }

    /* now register a replica for each subfile */
    tmpBunReplCache = bunReplCacheHeader->bunReplCacheHead;

    if (tmpBunReplCache == NULL) {
        rmdir (phyBunDir);
        bzero (&dataObjUnlinkInp, sizeof (dataObjUnlinkInp));
        rstrcpy (dataObjUnlinkInp.objPath, 
                 L1desc[l1descInx].dataObjInfo->objPath, MAX_NAME_LEN);
        dataObjUnlinkS (rsComm, &dataObjUnlinkInp, 
                        L1desc[l1descInx].dataObjInfo);
        L1desc[l1descInx].bytesWritten = 0;
        rsDataObjClose (rsComm, &dataObjCloseInp);
        bzero (bunReplCacheHeader, sizeof (bunReplCacheHeader_t));
        return 0;
    }

    bzero (&regReplicaInp, sizeof (regReplicaInp));
    regReplicaInp.srcDataObjInfo = (dataObjInfo_t*)malloc (sizeof (dataObjInfo_t));
    regReplicaInp.destDataObjInfo = (dataObjInfo_t*)malloc (sizeof (dataObjInfo_t));
    bzero (regReplicaInp.srcDataObjInfo, sizeof (dataObjInfo_t));
    bzero (regReplicaInp.destDataObjInfo, sizeof (dataObjInfo_t));
    addKeyVal (&regReplicaInp.condInput, IRODS_ADMIN_KW, "");
    rstrcpy (regReplicaInp.destDataObjInfo->rescName, BUNDLE_RESC, NAME_LEN);
    rstrcpy (regReplicaInp.destDataObjInfo->filePath, 
             L1desc[l1descInx].dataObjInfo->objPath, MAX_NAME_LEN);
    // =-=-=-=-=-=-=-
    // JMC - backport 4528
    if (chksumFlag != 0) {
        bzero (&modDataObjMetaInp, sizeof (modDataObjMetaInp));
        bzero (&regParam, sizeof (regParam));
        modDataObjMetaInp.dataObjInfo = regReplicaInp.destDataObjInfo;
        modDataObjMetaInp.regParam = &regParam;
    }
    // =-=-=-=-=-=-=-
            
    /* close here because dataObjInfo is still being used */

    rsDataObjClose (rsComm, &dataObjCloseInp);
    while (tmpBunReplCache != NULL) {
        char subPhyPath[MAX_NAME_LEN];

        nextBunReplCache = tmpBunReplCache->next;
        /* rm the hard link here */
        snprintf (subPhyPath, MAX_NAME_LEN, "%s/%lld", phyBunDir, 
                  tmpBunReplCache->dataId);
        // =-=-=-=-=-=-=-
        // JMC - backport 4528
        if (chksumFlag != 0) {
            status = fileChksum (UNIX_FILE_TYPE, rsComm, regReplicaInp.destDataObjInfo->filePath,
                                 subPhyPath, "", tmpBunReplCache->chksumStr);
            if (status < 0) {
                savedStatus = status;
                rodsLogError (LOG_ERROR, status,"bundlleAndRegSubFiles: fileChksum error for %s",tmpBunReplCache->objPath);
            }
        }
        // =-=-=-=-=-=-=-
        unlink (subPhyPath);
        /* register the replica */
        rstrcpy (regReplicaInp.srcDataObjInfo->objPath, 
                 tmpBunReplCache->objPath, MAX_NAME_LEN);
        regReplicaInp.srcDataObjInfo->dataId = 
            regReplicaInp.destDataObjInfo->dataId =
            tmpBunReplCache->dataId; 
        regReplicaInp.srcDataObjInfo->replNum = tmpBunReplCache->srcReplNum;
        status = rsRegReplica (rsComm, &regReplicaInp);
        if (status < 0) {
            savedStatus = status;
            rodsLog (LOG_ERROR,
                     "bundlleAndRegSubFiles: rsRegReplica error for %s. stat = %d",
                     tmpBunReplCache->objPath, status);
        }
        // =-=-=-=-=-=-=-
        // JMC - backport 4528
        if (chksumFlag != 0) {
            addKeyVal (&regParam, CHKSUM_KW, tmpBunReplCache->chksumStr);
            status = rsModDataObjMeta (rsComm, &modDataObjMetaInp);
            clearKeyVal (&regParam);
            if (status < 0) {
                savedStatus = status;
                rodsLogError (LOG_ERROR, status, "bundlleAndRegSubFiles: rsModDataObjMeta error for %s.", tmpBunReplCache->objPath);
            }
        }
        // =-=-=-=-=-=-=-
        free (tmpBunReplCache);
        tmpBunReplCache = nextBunReplCache;
    }
    clearKeyVal (&regReplicaInp.condInput);
    free (regReplicaInp.srcDataObjInfo);
    free (regReplicaInp.destDataObjInfo);
    bzero (bunReplCacheHeader, sizeof (bunReplCacheHeader_t)); 
    rmdir (phyBunDir);

    if (status >= 0 && savedStatus < 0) {
        return savedStatus;
    } else {
        return status;
    }
}

/* phyBundle
 * Valid oprType are CREATE_TAR_OPR and ADD_TO_TAR_OPR
 */
int
phyBundle (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, char *phyBunDir,
           char *collection, int oprType ) // JMC - backport 4643
{
    structFileOprInp_t structFileOprInp;
    int status = 0;
    char *dataType; // JMC - backport 4633
    int myOprType = oprType; // JMC - backport 4657

    dataType = dataObjInfo->dataType;
#if 0 // this is now handled by libarchive which can add to existing archives without
      // the need for unbundling
    if ((oprType & ADD_TO_TAR_OPR) != 0) { // JMC - backport 4657
        /* need to extract the content of the exsisting zipped file */
        if( dataType != NULL &&
            ( strstr( dataType, GZIP_TAR_DT_STR ) != NULL   || // JMC - backport 4658
              strstr (dataType, BZIP2_TAR_DT_STR) != NULL)) { 
            /* don't need to do this file zipFile */
            /* strstr (dataType, ZIP_DT_STR) != NULL)) {  */
            status = unbunPhyBunFile (rsComm, dataObjInfo->objPath,
                                      dataObjInfo->rescInfo,  dataObjInfo->filePath, phyBunDir, 
                                      dataType, PRESERVE_DIR_CONT);
            if (status < 0) return status;
            /* take out ADD_TO_TAR_OPR */
            myOprType = myOprType ^ ADD_TO_TAR_OPR;
        }
    }
#endif

    bzero (&structFileOprInp, sizeof (structFileOprInp));
    addKeyVal( &structFileOprInp.condInput, RESC_HIER_STR_KW, dataObjInfo->rescHier );
 
    structFileOprInp.specColl = (specColl_t*)malloc (sizeof (specColl_t));
    memset (structFileOprInp.specColl, 0, sizeof (specColl_t));
    structFileOprInp.specColl->type = TAR_STRUCT_FILE_T;

    /* collection and objPath are only important for reg CollInfo2 */
    rstrcpy (structFileOprInp.specColl->collection,collection, MAX_NAME_LEN);
    rstrcpy (structFileOprInp.specColl->objPath,dataObjInfo->objPath, MAX_NAME_LEN);
    structFileOprInp.specColl->collClass = STRUCT_FILE_COLL;
    rstrcpy (structFileOprInp.specColl->resource, dataObjInfo->rescName,NAME_LEN);
    rstrcpy (structFileOprInp.specColl->phyPath,dataObjInfo->filePath, MAX_NAME_LEN);
    addKeyVal( &structFileOprInp.condInput, RESC_HIER_STR_KW, dataObjInfo->rescHier );

    rstrcpy (structFileOprInp.specColl->cacheDir, phyBunDir, MAX_NAME_LEN);
    structFileOprInp.specColl->cacheDirty = 1;
    /* don't reg CollInfo2 */
    structFileOprInp.oprType = NO_REG_COLL_INFO | myOprType; // JMC - backport 4657
    if( dataType != NULL &&// JMC - backport 4633
        ( strstr( dataType, GZIP_TAR_DT_STR)  != NULL || // JMC - backport 4658
          strstr( dataType, BZIP2_TAR_DT_STR) != NULL ||
          strstr( dataType, ZIP_DT_STR)       != NULL ) ) {
        addKeyVal (&structFileOprInp.condInput, DATA_TYPE_KW, dataType);
    }

    status = rsStructFileSync (rsComm, &structFileOprInp);

    free (structFileOprInp.specColl);
    /* rm the the exsisting files in the original tar files */
    if ((oprType & ADD_TO_TAR_OPR) != 0 && (myOprType & ADD_TO_TAR_OPR) == 0) { // JMC - backport 4657 
        rmUnlinkedFilesInUnixDir (phyBunDir); // JMC - backport 4666 
    }

    if (status < 0) {
        rodsLog (LOG_ERROR,
                 "phyBundle: rsStructFileSync of %s error. stat = %d",
                 dataObjInfo->objPath, status);
    }

    return status;
}

int
addSubFileToDir (curSubFileCond_t *curSubFileCond,
                 bunReplCacheHeader_t *bunReplCacheHeader)
{
    int status;
    bunReplCache_t *bunReplCache;

    /* add a link */
    status = link (curSubFileCond->cachePhyPath, curSubFileCond->subPhyPath);
    if (status < 0) {
        rodsLog (LOG_ERROR,
                 "addSubFileToDir: link error %s to %s. errno = %d",
                 curSubFileCond->cachePhyPath, curSubFileCond->subPhyPath, errno);
        return (UNIX_FILE_LINK_ERR - errno);
    }
    bunReplCache = (bunReplCache_t*)malloc (sizeof (bunReplCache_t));
    bzero (bunReplCache, sizeof (bunReplCache_t));
    bunReplCache->dataId = curSubFileCond->dataId;
    snprintf (bunReplCache->objPath, MAX_NAME_LEN, "%s/%s", 
              curSubFileCond->collName, curSubFileCond->dataName);
    bunReplCache->srcReplNum = curSubFileCond->cacheReplNum;
    bunReplCache->next = bunReplCacheHeader->bunReplCacheHead;
    bunReplCacheHeader->bunReplCacheHead = bunReplCache;
    bunReplCacheHeader->numSubFiles++;
    bunReplCacheHeader->totSubFileSize += curSubFileCond->subFileSize;

    return 0;
}
    
int
setSubPhyPath (char *phyBunDir, rodsLong_t dataId, char *subPhyPath)
{
    snprintf (subPhyPath, MAX_NAME_LEN, "%s/%lld", phyBunDir, dataId);
    return 0;
}

int
isDataObjBundled (rsComm_t *rsComm, collEnt_t *collEnt)
{
    if (strcmp (collEnt->resource, BUNDLE_RESC) == 0) {
        if (collEnt->replStatus > 0) { 
            return 1;
        } else {
            /* XXXXXX need to remove this outdated copy */
            return 0;
        }
    } else {
        return 0;
    }
}

int
replDataObjForBundle (rsComm_t *rsComm, char *collName, char *dataName,
                      char *rescName, char* rescHier, char* dstRescHier, 
                      int adminFlag, dataObjInfo_t *outCacheObjInfo)
{
    transferStat_t transStat;
    dataObjInp_t dataObjInp;
    int status;

    if (outCacheObjInfo != NULL)
        memset (outCacheObjInfo, 0, sizeof (dataObjInfo_t));
    memset (&dataObjInp, 0, sizeof (dataObjInp_t));
    memset (&transStat, 0, sizeof (transStat));

    snprintf (dataObjInp.objPath, MAX_NAME_LEN, "%s/%s", collName, dataName);
    addKeyVal (&dataObjInp.condInput, BACKUP_RESC_NAME_KW,   rescName);
    if( rescHier ) {
        addKeyVal (&dataObjInp.condInput, RESC_HIER_STR_KW,      rescHier);  
    }
    if( dstRescHier ) { 
        addKeyVal (&dataObjInp.condInput, DEST_RESC_HIER_STR_KW, dstRescHier); 
    }
    if (adminFlag > 0) 
        addKeyVal (&dataObjInp.condInput, IRODS_ADMIN_KW, "");

    status = _rsDataObjRepl (rsComm, &dataObjInp, &transStat,
                             outCacheObjInfo);
    clearKeyVal (&dataObjInp.condInput);
    return status;
}

int
createPhyBundleDir (rsComm_t *rsComm, char *bunFilePath, 
                    char *outPhyBundleDir)
{
    /* the dir where we put the files to bundle is in phyPath.dir */
    snprintf (outPhyBundleDir, MAX_NAME_LEN, "%s.dir",  bunFilePath);
    mkdirR ("/", outPhyBundleDir, getDefDirMode ());
    return (0);
}

int
createPhyBundleDataObj (rsComm_t *rsComm, char *collection, 
                        rescGrpInfo_t *rescGrpInfo, dataObjInp_t *dataObjInp, char* dataType ) // JMC - backport 4658
{
    int myRanNum;
    int l1descInx;
    int status;
    int rescTypeInx = rescGrpInfo->rescInfo->rescTypeInx;

    /* XXXXXX We do bundle only with UNIX_FILE_TYPE for now */

    std::string type;
    eirods::error err = eirods::get_resource_property< std::string >( rescGrpInfo->rescInfo->rescName, "type", type );
    if( !err.ok() ) {
        eirods::log( PASS( false, -1, "createPhyBundleDataObj failed.", err ) );    
    }
    // JMC - legacy resource - if (RescTypeDef[rescTypeInx].driverType != UNIX_FILE_TYPE) {
    if( "unix file system" != type ) { // JMC :: need a constant for this?
        rodsLog (LOG_ERROR,
                 "createPhyBundleFile: resource %s is not UNIX_FILE_TYPE",
                 rescGrpInfo->rescInfo->rescName);
        return SYS_INVALID_RESC_TYPE;
    } 
#if 0 // JMC legacy resources
    else if (getRescClass (rescGrpInfo->rescInfo) != CACHE_CL) {
        return SYS_NO_CACHE_RESC_IN_GRP;
    }

#endif // JMC legacy resources
        
    do {
        int loopCnt = 0;
        bzero (dataObjInp, sizeof (dataObjInp_t));
        while (1) {
            myRanNum = random ();
            status = rsMkBundlePath (rsComm, collection, dataObjInp->objPath, 
                                     myRanNum);
            if (status < 0) {
                rodsLog (LOG_ERROR,
                         "createPhyBundleFile: getPhyBundlePath err for %s.stat = %d",
                         collection, status);
                return status;
            }
            /* check if BundlePath already existed */
            if (isData (rsComm, dataObjInp->objPath, NULL) >= 0) {
                if (loopCnt >= 100) {
                    break;
                } else {
                    loopCnt++;
                    continue;
                }
            } else {
                break;
            }
        }

        if (dataType != NULL && strstr (dataType, BUNDLE_STR) != NULL) { // JMC - backport 4658
            addKeyVal (&dataObjInp->condInput, DATA_TYPE_KW, dataType);
        } else {
            /* assume it is TAR_BUNDLE_DT_STR */
            addKeyVal (&dataObjInp->condInput, DATA_TYPE_KW, TAR_BUNDLE_DT_STR);
        }

        if (dataType != NULL && strstr (dataType, ZIP_DT_STR) != NULL) { // JMC - backport 4664
            /* zipFile type. must end with .zip */
            int len = strlen (dataObjInp->objPath);
            if (strcmp (&dataObjInp->objPath[len - 4], ".zip") != 0) {
                strcat (dataObjInp->objPath, ".zip");
            }
        }

        l1descInx = _rsDataObjCreateWithRescInfo (rsComm, dataObjInp,
                                                  rescGrpInfo->rescInfo, rescGrpInfo->rescGroupName);

        clearKeyVal (&dataObjInp->condInput);
    } while (l1descInx == OVERWRITE_WITHOUT_FORCE_FLAG);

    if (l1descInx >= 0) {
        l3Close (rsComm, l1descInx);
        L1desc[l1descInx].l3descInx = 0;
        if (dataType != NULL && strstr (dataType, ZIP_DT_STR) != NULL) // JMC - backport 4664
            l3Unlink (rsComm, L1desc[l1descInx].dataObjInfo);
    }

    return l1descInx;
}

/* rsMkBundlePath - set the BundlePath to 
 * /zone/bundle/home/.../collection.myRanNum. Make all the necessary 
 * parent collections. The output is put in bundlePath.
 */

int
rsMkBundlePath (rsComm_t *rsComm, char *collection, char *bundlePath, 
                int myRanNum)
{
    int status;
    char *tmpStr;
    char startBundlePath[MAX_NAME_LEN];
    char destBundleColl[MAX_NAME_LEN], myFile[MAX_NAME_LEN];
    char *bundlePathPtr;

    bundlePathPtr = bundlePath;
    *bundlePathPtr = '/';
    bundlePathPtr++;
    tmpStr = collection + 1;
    /* copy the zone */
    while (*tmpStr != '\0') {
        *bundlePathPtr = *tmpStr;
        bundlePathPtr ++;
        if (*tmpStr == '/') {
            tmpStr ++;
            break;
        }
        tmpStr ++;
    }

    if (*tmpStr == '\0') {
        rodsLog (LOG_ERROR,
                 "rsMkBundlePath: input path %s too short", collection);
        return (USER_INPUT_PATH_ERR);
    }

    /* cannot bundle trash and bundle */
    if (strncmp (tmpStr, "trash/", 6) == 0 || 
        strncmp (tmpStr, "bundle/", 7) == 0) {
        rodsLog (LOG_ERROR,
                 "rsMkBundlePath: cannot bundle trash or bundle path %s", collection);
        return (USER_INPUT_PATH_ERR);
    }


    /* don't want to go back beyond /myZone/bundle/home */
    *bundlePathPtr = '\0';
    rstrcpy (startBundlePath, bundlePath, MAX_NAME_LEN);

    snprintf (bundlePathPtr, MAX_NAME_LEN, "bundle/%s.%d", tmpStr, myRanNum);

    if ((status = splitPathByKey (bundlePath, destBundleColl, myFile, '/')) 
        < 0) {
        rodsLog (LOG_ERROR,
                 "rsMkBundlePath: splitPathByKey error for %s ", bundlePath);
        return (USER_INPUT_PATH_ERR);
    }

    status = rsMkCollR (rsComm, startBundlePath, destBundleColl);

    if (status < 0) {
        rodsLog (LOG_ERROR,
                 "rsMkBundlePath: rsMkCollR error for startPath %s, destPath %s ",
                 startBundlePath, destBundleColl);
    }

    return (status);
}

int
remotePhyBundleColl (rsComm_t *rsComm, 
                     structFileExtAndRegInp_t *phyBundleCollInp, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "remotePhyBundleColl: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcPhyBundleColl (rodsServerHost->conn, phyBundleCollInp);
    return status;
}


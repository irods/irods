/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* objDesc.h - header file for objDesc.c
 */



#ifndef OBJ_DESC_HPP
#define OBJ_DESC_HPP

#include "rods.hpp"
#include "objInfo.hpp"
#include "dataObjInpOut.hpp"
#include "fileRename.hpp"
#include "miscUtil.hpp"
#include "structFileSync.hpp"
#include "structFileExtAndReg.hpp"
#include "dataObjOpenAndStat.hpp"

#include "boost/any.hpp"

#define NUM_L1_DESC     1026    /* number of L1Desc */

#define CHK_ORPHAN_CNT_LIMIT  20  /* number of failed check before stopping */
/* definition for getNumThreads */

#define REG_CHKSUM      1
#define VERIFY_CHKSUM   2

/* values in l1desc_t is the desired value. values in dataObjInfo are
 * the values in rcat */
typedef struct l1desc {
    int l3descInx;
    int inuseFlag;
    int oprType;
    int openType;
    int oprStatus;
    int dataObjInpReplFlag;
    dataObjInp_t *dataObjInp;
    dataObjInfo_t *dataObjInfo;
    dataObjInfo_t *otherDataObjInfo;
    int copiesNeeded;
    rodsLong_t bytesWritten;    /* mark whether it has been written */
    rodsLong_t dataSize;        /* this is the target size. The size in
                                 * dataObjInfo is the registered size */
    int replStatus;     /* the replica status */
    int chksumFlag;     /* parsed from condition */
    int srcL1descInx;
    char chksum[NAME_LEN]; /* the input chksum */
#ifdef LOG_TRANSFERS
    struct timeval openStartTime;
#endif
    int remoteL1descInx;
    int stageFlag;
    int purgeCacheFlag; // JMC - backport 4537
    int lockFd; // JMC - backport 4604
    boost::any pluginData;
    dataObjInfo_t *replDataObjInfo; /* if non NULL, repl to this dataObjInfo
                                     * on close */
    rodsServerHost_t *remoteZoneHost;
    char in_pdmo[MAX_NAME_LEN];
} l1desc_t;

extern "C" {

    int
    initL1Desc();

    int
    allocL1Desc();

    int
    freeL1Desc( int fileInx );

    int
    fillL1desc( int l1descInx, dataObjInp_t *dataObjInp,
                dataObjInfo_t *dataObjInfo, int replStatus, rodsLong_t dataSize );
    int
    getL1descIndexByDataObjInfo( const dataObjInfo_t * dataObjInfo );
    int
    getNumThreads( rsComm_t *rsComm, rodsLong_t dataSize, int inpNumThr,
                   keyValPair_t *condInput, char *destRescName, char *srcRescName );
    int
    initDataOprInp( dataOprInp_t *dataOprInp, int l1descInx, int oprType );
    int
    initDataObjInfoForRepl( dataObjInfo_t *destDataObjInfo,
                            dataObjInfo_t *srcDataObjInfo, const char *_resc_name );
    int
    convL3descInx( int l3descInx );
    int
    initDataObjInfoWithInp( dataObjInfo_t *dataObjInfo, dataObjInp_t *dataObjInp );
    int
    allocL1desc();
    int
    freeL1desc( int l1descInx );
    int
    closeAllL1desc( rsComm_t *rsComm );
    int
    initSpecCollDesc();
    int
    allocSpecCollDesc();
    int
    freeSpecCollDesc( int specCollInx );
    int
    initL1desc();
    int
    initCollHandle();
    int
    allocCollHandle();
    int
    freeCollHandle( int handleInx );
    int
    rsInitQueryHandle( queryHandle_t *queryHandle, rsComm_t *rsComm );
    int
    allocAndSetL1descForZoneOpr( int l3descInx, dataObjInp_t *dataObjInp,
                                 rodsServerHost_t *remoteZoneHost, openStat_t *openStat );
    int
    isL1descInuse();

}

#endif  /* OBJ_DESC_H */


#ifndef IRODS_OBJ_DESC_HPP
#define IRODS_OBJ_DESC_HPP

#include "rods.h"
#include "objInfo.h"
#include "dataObjInpOut.h"
#include "fileRename.h"
#include "miscUtil.h"
#include "structFileSync.h"
#include "structFileExtAndReg.h"
#include "dataObjOpenAndStat.h"
#include "rodsConnect.h"

#include "boost/any.hpp"

#include <string>

#define NUM_L1_DESC     1026    /* number of L1Desc */

#define CHK_ORPHAN_CNT_LIMIT  20  /* number of failed check before stopping */
/* definition for getNumThreads */

#define REG_CHKSUM      1
#define VERIFY_CHKSUM   2

// Values in l1desc are the desired values.
// Values in dataObjInfo are the values in the catalog.
//
// If you modify this data type, you must also update init_l1desc().
//
// DO NOT call std::memset() on this data type. Use init_l1desc().
// DO NOT call std::memcpy() on this data type. Use the assignment operator.
typedef struct l1desc
{
    l1desc() = default;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    l1desc(const l1desc&) = default;
    auto operator=(const l1desc&) -> l1desc& = default;

    l1desc(l1desc&&) = default;
    auto operator=(l1desc&&) -> l1desc& = default;
#pragma clang diagnostic pop

    ~l1desc() = default;

    int l3descInx;
    int inuseFlag;
    int oprType;
    int openType;
    int oprStatus;
    int dataObjInpReplFlag;
    dataObjInp_t *dataObjInp;
    dataObjInfo_t *dataObjInfo;
    [[deprecated]] dataObjInfo_t *otherDataObjInfo;
    int copiesNeeded;
    rodsLong_t bytesWritten; /* mark whether it has been written */
    rodsLong_t dataSize;     /* this is the target size. The size in dataObjInfo is the registered size */
    int replStatus;          /* the replica status */
    int chksumFlag;          /* parsed from condition */
    int srcL1descInx;
    char chksum[NAME_LEN]; /* the input chksum */
    int remoteL1descInx;
    int stageFlag;
    int purgeCacheFlag;
    int lockFd;
    [[deprecated]] boost::any pluginData;
    [[deprecated]] dataObjInfo_t *replDataObjInfo; // if non NULL, repl to this dataObjInfo on close.
    rodsServerHost_t *remoteZoneHost;
    char in_pdmo[MAX_NAME_LEN];

    std::string replica_token;
} l1desc_t;

int
initL1Desc();

auto init_l1desc(l1desc& _l1d) -> void;

int
allocL1Desc();

int freeL1desc_struct(l1desc& _l1desc);

int freeL1desc(const int l1descInx);

int
fillL1desc( int l1descInx, dataObjInp_t *dataObjInp,
            dataObjInfo_t *dataObjInfo, int replStatus, rodsLong_t dataSize );
int
getL1descIndexByDataObjInfo( const dataObjInfo_t * dataObjInfo );
int
getNumThreads( rsComm_t *rsComm, rodsLong_t dataSize, int inpNumThr,
               keyValPair_t *condInput, char *destRescName, char *srcRescName, int oprType );
int
initDataOprInp( dataOprInp_t *dataOprInp, int l1descInx, int oprType );
int
convL3descInx( int l3descInx );
int
initDataObjInfoWithInp( dataObjInfo_t *dataObjInfo, dataObjInp_t *dataObjInp );
int
allocL1desc();
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

namespace irods
{
    /// \brief Allocates and populates an L1 descriptor based on the provided inputs.
    ///
    /// \param[in] _inp
    /// \param[in] _info
    /// \param[in] _data_size The expected size at close of the replica which is being opened.
    ///
    /// \returns Generated L1 descriptor index
    ///
    /// \since 4.2.9
    auto populate_L1desc_with_inp(
        DataObjInp& _inp,
        DataObjInfo& _info,
        const rodsLong_t _data_size) -> int;
} // namespace irods

#endif // IRODS_OBJ_DESC_HPP

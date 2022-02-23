#ifndef DATA_OBJ_OPR_HPP
#define DATA_OBJ_OPR_HPP

#include "irods/rods.h"
#include "irods/objInfo.h"
#include "irods/dataObjInpOut.h"
#include "irods/ruleExecSubmit.h"
#include "irods/rcGlobalExtern.h"
#include "irods/rsGlobalExtern.hpp"
//#include "irods/reIn2p3SysRule.hpp"
#include "irods/irods_file_object.hpp"
#include "irods/irods_linked_list_iterator.hpp"

#include <string>

/* definition for trimjFlag in matchAndTrimRescGrp */
#define TRIM_MATCHED_RESC_INFO          0x1
#define REQUE_MATCHED_RESC_INFO         0x2
#define TRIM_MATCHED_OBJ_INFO           0x4
#define TRIM_UNMATCHED_OBJ_INFO         0x8

#define MULTI_COPIES_PER_RESC "MULTI_COPIES_PER_RESC"

irods::error validate_logical_path(const std::string&);
    
irods::error resolve_hierarchy_for_resc_from_cond_input(
    rsComm_t*,
    const std::string&,
    std::string& );
int
getDataObjInfo( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                dataObjInfo_t **dataObjInfoHead, char *accessPerm, int ignoreCondInput );
int
updateDataObjReplStatus( rsComm_t *rsComm, int l1descInx, int replStatus );
int
dataObjExist( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int
sortObjInfoForOpen( dataObjInfo_t **dataObjInfoHead,
                    keyValPair_t *condInput, int writeFlag );
int create_and_sort_data_obj_info_for_open(
		const std::string& resc_hier,
		const irods::file_object_ptr file_obj,
		dataObjInfo_t **data_obj_info_head);
int
sortDataObjInfoRandom( dataObjInfo_t **dataObjInfoHead );
int
requeDataObjInfoByResc( dataObjInfo_t **dataObjInfoHead, const char *preferedResc,
                        int writeFlag, int topFlag );

int
initDataObjInfoQuery( dataObjInp_t *dataObjInp, genQueryInp_t *genQueryInp,
                      int ignoreCondInput );
int
chkOrphanFile( rsComm_t *rsComm, const char *filePath, const char *rescName,
               dataObjInfo_t *dataObjInfo );
int
chkOrphanDir( rsComm_t *rsComm, char *dirPath, const char *rescName );
int
getNumDataObjInfo( dataObjInfo_t *dataObjInfoHead );
int
matchDataObjInfoByCondInput( dataObjInfo_t **dataObjInfoHead,
                             dataObjInfo_t **oldDataObjInfoHead, keyValPair_t *condInput,
                             dataObjInfo_t **matchedDataObjInfo, dataObjInfo_t **matchedOldDataObjInfo );
int
matchAndTrimRescGrp( dataObjInfo_t **dataObjInfoHead,
                     const std::string& _resc_name,
                     int trimjFlag,
                     dataObjInfo_t **trimmedDataObjInfo );
int
requeDataObjInfoBySrcResc( dataObjInfo_t **dataObjInfoHead,
                           keyValPair_t *condInput, int writeFlag, int topFlag );
int
getDataObjInfoIncSpecColl( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                           dataObjInfo_t **dataObjInfo );
int
regNewObjSize( rsComm_t *rsComm, char *objPath, int replNum,
               rodsLong_t newSize );
int
getDataObjByClass( dataObjInfo_t *dataObjInfoHead, int rescClass,
                   dataObjInfo_t **outDataObjInfo );

irods::linked_list_iterator<dataObjInfo_t> begin(dataObjInfo_t* _objects) noexcept;

irods::linked_list_iterator<const dataObjInfo_t> begin(const dataObjInfo_t* _objects) noexcept;

irods::linked_list_iterator<dataObjInfo_t> end(dataObjInfo_t* _objects) noexcept;

irods::linked_list_iterator<const dataObjInfo_t> end(const dataObjInfo_t* _objects) noexcept;

// returns true if an object exists in [_objects] that satisfies [_pred].
//
// [UnaryPredicate]
// - signature: bool(const dataObjInfo_t&)
// - must return true for the required element.
template <typename UnaryPredicate>
bool contains_replica_if(const dataObjInfo_t* _objects, UnaryPredicate&& _pred) {
    auto e = end(_objects);
    return std::find_if(begin(_objects), e, std::forward<UnaryPredicate>(_pred)) != e;
}

bool contains_replica(const dataObjInfo_t* _objects, const std::string& _resc_name);

bool contains_replica(const dataObjInfo_t* _objects, int _replica_number);

#endif  /* DATA_OBJ_OPR_H */

#ifndef SPEC_COLL_HPP
#define SPEC_COLL_HPP

// This is needed for the SpecialCollPerm and because it is in lib,
// it cannot be pulled into this file, unfortunately.
#include "objInfo.h"

struct DataObjInfo;
struct DataObjInp;
struct GenQueryOut;
struct rodsObjStat;
struct RsComm;
struct SpecColl;
struct SpecCollCache;

int modCollInfo2(RsComm* rsComm, SpecColl* specColl, int clearFlag);

int querySpecColl(RsComm* rsComm, const char* objPath, GenQueryOut** genQueryOut);

int queueSpecCollCache(RsComm* rsComm, GenQueryOut* genQueryOut, const char* objPath); // JMC - backport 4680

int queueSpecCollCacheWithObjStat(rodsObjStat* rodsObjStatOut);

SpecCollCache* matchSpecCollCache(const char* objPath);

int getSpecCollCache(RsComm* rsComm, const char* objPath, int inCachOnly, SpecCollCache** specCollCache);

int statPathInSpecColl(RsComm* rsComm, char* objPath, int inCachOnly, rodsObjStat** rodsObjStatOut);

int specCollSubStat(RsComm* rsComm,
                    SpecColl* specColl,
                    char* subPath,
                    SpecialCollPerm specCollPerm,
                    DataObjInfo** dataObjInfo);

int resolvePathInSpecColl(RsComm* rsComm,
                          char* objPath,
                          SpecialCollPerm specCollPerm,
                          int inCachOnly,
                          dataObjInfo_t** dataObjInfo);

int resolveLinkedPath(RsComm* rsComm,
                      char* objPath,
                      SpecCollCache** specCollCache,
                      KeyValPair* condInput);

namespace irods
{
    /// \brief Get the special collection type for the data object indicated by the input
    ///
    /// \param[in,out] _comm
    /// \param[in,out] _inp Input which is passed to rsObjStat to get data object information
    ///
    /// \returns The SpecialCollClass stored in the returned stat information, or an error code
    /// \retval NO_SPEC_COLL If stat contains no special collection info or data object does not exist
    /// \retval USER_INPUT_PATH_ERR If the objPath in _inp refers to a collection
    ///
    /// \since 4.2.9
    auto get_special_collection_type_for_data_object(RsComm& _comm, DataObjInp& _inp) -> int;

    /// \brief Creates L1 descriptor and physical file for objects in special collections and bundleResc
    ///
    /// \param[in,out] _comm
    /// \param[in,out] _inp Input used to describe the object or replica to be created
    ///
    /// \returns The generated L1 descriptor index, or an error on failure
    /// \retval SYS_COPY_ALREADY_IN_RESC If a physical replica exists in a resolved special collection path
    /// \retval SYS_FILE_DESC_OUT_OF_RANGE If the L1 index returned is in [0,2]
    ///
    /// \since 4.2.9
    auto data_object_create_in_special_collection(RsComm* _comm, DataObjInp& _inp) -> int;
} // namespace irods

#endif	// SPEC_COLL_HPP

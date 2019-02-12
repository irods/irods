#ifndef IRODS_FILESYSTEM_CONFIG_HPP
#define IRODS_FILESYSTEM_CONFIG_HPP

// clang-format off
#if defined(RODS_SERVER) || defined(RODS_CLERVER)
    #define rxComm                  rsComm_t

    #define rxOpenCollection        rsOpenCollection
    #define rxReadCollection        rsReadCollection
    #define rxCloseCollection       rsCloseCollection
    #define rxObjStat               rsObjStat
    #define rxModColl               rsModColl
    #define rxCollCreate            rsCollCreate
    #define rxRmColl                rsRmColl 
    #define rxDataObjCopy           rsDataObjCopy
    #define rxDataObjRename         rsDataObjRename
    #define rxDataObjUnlink         rsDataObjUnlink
    #define rxDataObjChksum         rsDataObjChksum
    #define rxModAccessControl      rsModAccessControl
    #define rxModAVUMetadata        rsModAVUMetadata
#else
    #define rxComm                  rcComm_t

    #define rxOpenCollection        rcOpenCollection
    #define rxReadCollection        rcReadCollection
    #define rxCloseCollection       rcCloseCollection
    #define rxObjStat               rcObjStat
    #define rxModColl               rcModColl
    #define rxCollCreate            rcCollCreate
    #define rxRmColl                rcRmColl
    #define rxDataObjCopy           rcDataObjCopy
    #define rxDataObjRename         rcDataObjRename
    #define rxDataObjUnlink         rcDataObjUnlink
    #define rxDataObjChksum         rcDataObjChksum
    #define rxModAccessControl      rcModAccessControl
    #define rxModAVUMetadata        rcModAVUMetadata
#endif // RODS_SERVER or RODS_CLERVER
// clang-format on

#endif // IRODS_FILESYSTEM_CONFIG_HPP

#ifndef IRODS_FILESYSTEM_CONFIG_HPP
#define IRODS_FILESYSTEM_CONFIG_HPP

#undef NAMESPACE_IMPL
#undef rxComm
#undef rxSpecificQuery
#undef rxOpenCollection
#undef rxReadCollection
#undef rxCloseCollection
#undef rxObjStat
#undef rxModColl
#undef rxCollCreate
#undef rxRmColl
#undef rxDataObjCopy
#undef rxDataObjRename
#undef rxDataObjUnlink
#undef rxDataObjChksum
#undef rxModAccessControl
#undef rxModAVUMetadata

// clang-format off
#ifdef IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
    #define NAMESPACE_IMPL                          server

    #define rxComm                                  rsComm_t

    #define rxSpecificQuery                         rsSpecificQuery
    #define rxOpenCollection                        rsOpenCollection
    #define rxReadCollection                        rsReadCollection
    #define rxCloseCollection                       rsCloseCollection
    #define rxObjStat                               rsObjStat
    #define rxModColl                               rsModColl
    #define rxCollCreate                            rsCollCreate
    #define rxRmColl                                rsRmColl 
    #define rxDataObjCopy                           rsDataObjCopy
    #define rxDataObjRename                         rsDataObjRename
    #define rxDataObjUnlink                         rsDataObjUnlink
    #define rxDataObjChksum                         rsDataObjChksum
    #define rxModAccessControl                      rsModAccessControl
    #define rxModAVUMetadata                        rsModAVUMetadata
    #define rxModDataObjMeta                        rsModDataObjMeta
    #define rx_atomic_apply_metadata_operations     rs_atomic_apply_metadata_operations
#else
    #define NAMESPACE_IMPL                          client

    #define rxComm                                  rcComm_t

    #define rxSpecificQuery                         rcSpecificQuery
    #define rxOpenCollection                        rcOpenCollection
    #define rxReadCollection                        rcReadCollection
    #define rxCloseCollection                       rcCloseCollection
    #define rxObjStat                               rcObjStat
    #define rxModColl                               rcModColl
    #define rxCollCreate                            rcCollCreate
    #define rxRmColl                                rcRmColl
    #define rxDataObjCopy                           rcDataObjCopy
    #define rxDataObjRename                         rcDataObjRename
    #define rxDataObjUnlink                         rcDataObjUnlink
    #define rxDataObjChksum                         rcDataObjChksum
    #define rxModAccessControl                      rcModAccessControl
    #define rxModAVUMetadata                        rcModAVUMetadata
    #define rxModDataObjMeta                        rc_data_object_modify_info
    #define rx_atomic_apply_metadata_operations     rc_atomic_apply_metadata_operations
#endif // IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
// clang-format on

#endif // IRODS_FILESYSTEM_CONFIG_HPP

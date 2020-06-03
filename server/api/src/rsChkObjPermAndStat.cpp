/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsChkObjPermAndStat.c
 */

#include "chkObjPermAndStat.h"
#include "apiHeaderAll.h"
#include "icatHighLevelRoutines.hpp"
#include "miscServerFunct.hpp"
#include "irods_configuration_keywords.hpp"
#include "rsChkObjPermAndStat.hpp"
#include "rsOpenCollection.hpp"
#include "rsReadCollection.hpp"
#include "rsCloseCollection.hpp"
#include "rsPhyBundleColl.hpp"

int
saveCollEntForChkColl( collEnt_t *collEnt );
int
freeCollEntForChkColl( collEnt_t *collEnt );

int
rsChkObjPermAndStat( rsComm_t *rsComm,
                     chkObjPermAndStat_t *chkObjPermAndStatInp ) {
    int status;
    rodsServerHost_t *rodsServerHost = NULL;

    status = getAndConnRcatHost(
                 rsComm,
                 SLAVE_RCAT,
                 ( const char* )chkObjPermAndStatInp->objPath,
                 &rodsServerHost );
    if ( status < 0 || rodsServerHost == NULL ) { // JMC cppcheck
        return status;
    }
    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
        std::string svc_role;
        irods::error ret = get_catalog_service_role(svc_role);
        if(!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsChkObjPermAndStat( rsComm, chkObjPermAndStatInp );
        } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            status = SYS_NO_RCAT_SERVER_ERR;
        } else {
            rodsLog(
                LOG_ERROR,
                "role not supported [%s]",
                svc_role.c_str() );
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }
    }
    else {
        status = rcChkObjPermAndStat( rodsServerHost->conn,
                                      chkObjPermAndStatInp );
    }

    return status;
}

int
_rsChkObjPermAndStat( rsComm_t *rsComm,
                      chkObjPermAndStat_t *chkObjPermAndStatInp ) {
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        int status;

        if ( ( chkObjPermAndStatInp->flags & CHK_COLL_FOR_BUNDLE_OPR ) != 0 ) {
            status = chkCollForBundleOpr( rsComm, chkObjPermAndStatInp );
        }
        else {
            rodsLog( LOG_ERROR,
                     "_rsChkObjPermAndStat: rsChkObjPermAndStat of %s error. flags = %d",
                     chkObjPermAndStatInp->objPath, chkObjPermAndStatInp->flags );
            return SYS_OPR_FLAG_NOT_SUPPORT;
        }
        return status;
    } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        return SYS_NO_RCAT_SERVER_ERR;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        return SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }
}

int
chkCollForBundleOpr( rsComm_t *rsComm,
                     chkObjPermAndStat_t *chkObjPermAndStatInp ) {
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        int status;
        collInp_t openCollInp;
        collEnt_t *collEnt = NULL;
        collEnt_t *curCollEnt = NULL;
        int handleInx;
        int curCopyGood = False;
        char *resource;
        char *resc_hier;
        rodsLong_t myId;
        char myPath[MAX_NAME_LEN];

        if ( ( resource = getValByKey( &chkObjPermAndStatInp->condInput,
                                       RESC_NAME_KW ) ) == NULL ) {
            rodsLog( LOG_ERROR,
                     "chkCollForBundleOpr: RESC_NAME_KW not specified for %s",
                     chkObjPermAndStatInp->objPath );
            return SYS_INVALID_RESC_INPUT;
        }

        if ( ( resc_hier = getValByKey( &chkObjPermAndStatInp->condInput,
                                        RESC_HIER_STR_KW ) ) == NULL ) {
            rodsLog( LOG_ERROR,
                     "chkCollForBundleOpr: RESC_HIER_STR_KW not specified for %s",
                     chkObjPermAndStatInp->objPath );
            return SYS_INVALID_RESC_INPUT;
        }

        memset( &openCollInp, 0, sizeof( openCollInp ) );
        rstrcpy( openCollInp.collName, chkObjPermAndStatInp->objPath, MAX_NAME_LEN );
        openCollInp.flags =
            RECUR_QUERY_FG | LONG_METADATA_FG | NO_TRIM_REPL_FG;
        handleInx = rsOpenCollection( rsComm, &openCollInp );
        if ( handleInx < 0 ) {
            rodsLog( LOG_ERROR,
                     "chkCollForBundleOpr: rsOpenCollection of %s error. status = %d",
                     openCollInp.collName, handleInx );
            return handleInx;
        }
        while ( ( status = rsReadCollection( rsComm, &handleInx, &collEnt ) ) >= 0 ) {
            if ( collEnt->specColl.collClass != NO_SPEC_COLL ) {
                if ( strcmp( resource, collEnt->specColl.resource ) != 0 ) {
                    rodsLog( LOG_ERROR,
                             "chkCollForBundleOpr: specColl resc %s does not match %s",
                             collEnt->specColl.resource, resource );
                    rsCloseCollection( rsComm, &handleInx );
                    freeCollEntForChkColl( collEnt );
                    freeCollEntForChkColl( curCollEnt );
                    return SYS_COPY_NOT_EXIST_IN_RESC;
                }
                /* check permission */
                myId = chlCheckAndGetObjectID( rsComm, "-c",
                                               collEnt->specColl.collection, ACCESS_READ_OBJECT );
                if ( myId < 0 ) {
                    status = myId;
                    rodsLog( LOG_ERROR,
                             "chkCollForBundleOpr: no accPerm to specColl %s. status = %d",
                             collEnt->specColl.collection, status );
                    rsCloseCollection( rsComm, &handleInx );
                    freeCollEntForChkColl( collEnt );
                    freeCollEntForChkColl( curCollEnt );
                    return status;
                }
                free( collEnt );
                collEnt = NULL;
                continue;
            }

            if ( collEnt->objType == DATA_OBJ_T ) {
                if ( curCollEnt == NULL ) {
                    curCollEnt = collEnt;
                    saveCollEntForChkColl( collEnt );
                    if ( collEnt->replStatus > 0 &&
                            strcmp( resource,  collEnt->resource ) == 0 &&
                            strcmp( resc_hier, collEnt->resc_hier ) == 0 ) {
                        curCopyGood = True;
                    }
                }
                else {
                    if ( strcmp( curCollEnt->dataName, collEnt->dataName ) == 0 &&
                            strcmp( curCollEnt->collName, collEnt->collName ) == 0 ) {
                        if ( collEnt->replStatus                     >  0 &&
                                strcmp( resource,  collEnt->resource ) == 0 &&
                                strcmp( resc_hier, collEnt->resc_hier ) == 0 ) {
                            /* a good copy */
                            freeCollEntForChkColl( curCollEnt );
                            curCopyGood = True;
                            curCollEnt = collEnt;
                            saveCollEntForChkColl( collEnt );
                        }
                    }
                    else {
                        /* encounter a new data obj */
                        snprintf( myPath, MAX_NAME_LEN, "%s/%s",
                                  curCollEnt->collName, curCollEnt->dataName );

                        if ( curCopyGood == False ) {
                            irods::physical_object obj;
                            status = replDataObjForBundle( rsComm,
                                                           curCollEnt->collName, curCollEnt->dataName,
                                                           resource, curCollEnt->resc_hier, resc_hier, 0, obj);

                            if ( status < 0 ) {
                                rodsLog( LOG_ERROR,
                                         "chkCollForBundleOpr: %s no good copy in %s [%d]",
                                         myPath, resource, status );
                                rsCloseCollection( rsComm, &handleInx );
                                freeCollEntForChkColl( curCollEnt );
                                return SYS_COPY_NOT_EXIST_IN_RESC;
                            }
                        }
                        else {

                        }
                        freeCollEntForChkColl( curCollEnt );
                        curCopyGood = False;
                        curCollEnt = NULL;

                        /* we have a good copy. Check the permission */
                        myId = chlCheckAndGetObjectID( rsComm, "-d", myPath,
                                                       ACCESS_READ_OBJECT );
                        if ( myId < 0 && myId != CAT_UNKNOWN_FILE ) {
                            /* could return CAT_UNKNOWN_FILE if mounted files */
                            status = myId;
                            rodsLog( LOG_ERROR,
                                     "chkCollForBundleOpr: no accPerm to %s. status = %d",
                                     myPath, status );
                            rsCloseCollection( rsComm, &handleInx );
                            freeCollEntForChkColl( collEnt );
                            return status;
                        }
                        else {
                            /* copy is OK */
                            curCollEnt = collEnt;
                            saveCollEntForChkColl( collEnt );
                            collEnt = NULL;
                            if ( curCollEnt->replStatus > 0 &&
                                    strcmp( resource, curCollEnt->resource ) == 0 &&
                                    strcmp( resc_hier, curCollEnt->resc_hier ) == 0 ) {
                                /* a good copy */
                                curCopyGood = True;
                            }
                        }
                    }
                }
            }
            else {
                free( collEnt );
            }
        }

        /* handle what's left */
        if (NULL != curCollEnt) {
            if (False == curCopyGood) {
                irods::physical_object obj;
                status = replDataObjForBundle(rsComm,
                                              curCollEnt->collName,
                                              curCollEnt->dataName,
                                              resource,
                                              curCollEnt->resc_hier,
                                              resc_hier,
                                              0,
                                              obj);
                freeCollEntForChkColl(curCollEnt);
                if (status < 0) {
                    const auto err{ERROR(status,
                                         (boost::format("[%s] does not have a good copy in [%s]") %
                                          chkObjPermAndStatInp->objPath % resource).str().c_str())};
                    irods::log(err);
                }
            }
            else {
                freeCollEntForChkColl(curCollEnt);
            }
        }
        rsCloseCollection(rsComm, &handleInx);
        return 0;
    } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        return SYS_NO_RCAT_SERVER_ERR;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        return SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }
}

/* saveCollEntForChkColl - save some of entries in collEnt_t used by
 * chkCollForBundleOpr. These entry need to be saved because it could
 * be freed if the query has continuation.
 */
int
saveCollEntForChkColl( collEnt_t *collEnt ) {
    if ( collEnt == NULL ) {
        return 0;
    }
    if ( collEnt->collName != NULL ) {
        collEnt->collName = strdup( collEnt->collName );
    }
    if ( collEnt->dataName != NULL ) {
        collEnt->dataName = strdup( collEnt->dataName );
    }
    if ( collEnt->resource != NULL ) {
        collEnt->resource = strdup( collEnt->resource );
    }
    if ( collEnt->resc_hier != NULL ) {
        collEnt->resc_hier = strdup( collEnt->resc_hier );
    }
    return 0;
}

int
freeCollEntForChkColl( collEnt_t *collEnt ) {
    if ( collEnt == NULL ) {
        return 0;
    }
    if ( collEnt->collName != NULL ) {
        free( collEnt->collName );
    }
    if ( collEnt->dataName != NULL ) {
        free( collEnt->dataName );
    }
    if ( collEnt->resource != NULL ) {
        free( collEnt->resource );
    }

    free( collEnt );
    return 0;
}

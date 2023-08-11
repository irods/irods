#include "irods/getHostForGet.h"
#include "irods/getHostForPut.h"
#include "irods/rodsLog.h"
#include "irods/rsGlobalExtern.hpp"
#include "irods/rcGlobalExtern.h"
#include "irods/getRemoteZoneResc.h"
#include "irods/dataObjCreate.h"
#include "irods/objMetaOpr.hpp"
#include "irods/resource.hpp"
#include "irods/collection.hpp"
#include "irods/specColl.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/openCollection.h"
#include "irods/readCollection.h"
#include "irods/closeCollection.h"
#include "irods/dataObjOpr.hpp"
#include "irods/rsGetHostForGet.hpp"
#include "irods/irods_resource_backport.hpp"
#include "irods/irods_resource_redirect.hpp"

#include <cstring>

int rsGetHostForGet(
    rsComm_t*     rsComm,
    dataObjInp_t* dataObjInp,
    char**        outHost ) {
    rodsServerHost_t *rodsServerHost;
    const int remoteFlag = getAndConnRemoteZone(rsComm, dataObjInp, &rodsServerHost, REMOTE_OPEN);
    if (remoteFlag < 0) {
        return remoteFlag;
    }
    else if (REMOTE_HOST == remoteFlag) {
        const int status = rcGetHostForGet(rodsServerHost->conn, dataObjInp, outHost);
        if (status < 0) {
            return status;
        }
    }
    else {
        // =-=-=-=-=-=-=-
        // default behavior
        *outHost = strdup( THIS_ADDRESS );

        // =-=-=-=-=-=-=-
        // working on the "home zone", determine if we need to redirect to a different
        // server in this zone for this operation.  if there is a RESC_HIER_STR_KW then
        // we know that the redirection decision has already been made
        if ( isColl( rsComm, dataObjInp->objPath, NULL ) < 0 ) {
            std::string hier{};
            if ( getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW ) == NULL ) {
                try {
                    auto result = irods::resolve_resource_hierarchy(irods::OPEN_OPERATION, rsComm, *dataObjInp);
                    hier = std::get<std::string>(result);
                }   
                catch (const irods::exception& e ) { 
                    irods::log(e);
                    std::free(*outHost); // NOLINT(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
                    *outHost = nullptr;
                    return e.code();
                }
                addKeyVal( &dataObjInp->condInput, RESC_HIER_STR_KW, hier.c_str() );
            } // if keyword

            // =-=-=-=-=-=-=-
            // extract the host location from the resource hierarchy
            std::string location;
            irods::error ret = irods::get_loc_for_hier_string( hier, location );
            if ( !ret.ok() ) {
                irods::log( PASSMSG( "rsGetHostForGet - failed in get_loc_for_hier_string", ret ) );
                return -1;
            }

            // =-=-=-=-=-=-=-
            // set the out variable
            std::free(*outHost); // NOLINT(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
            *outHost = strdup( location.c_str() );

        } // if not a collection
    }

    return 0;

}

#if 0 // unused #1472
int
getBestRescForGet( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                   rescInfo_t **outRescInfo ) {
    collInp_t collInp;
    hostSearchStat_t hostSearchStat;
    int status, i;
    int maxInx = -1;

    if ( dataObjInp == NULL || outRescInfo == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    *outRescInfo = NULL;

    std::memset(&hostSearchStat, 0, sizeof(hostSearchStat));
    std::memset(&collInp, 0, sizeof(collInp));
    rstrcpy( collInp.collName, dataObjInp->objPath, MAX_NAME_LEN );
    /* assume it is a collection */
    status = getRescForGetInColl( rsComm, &collInp, &hostSearchStat );
    if ( status < 0 ) {
        /* try dataObj */
        status = getRescForGetInDataObj( rsComm, dataObjInp, &hostSearchStat );
    }

    for ( i = 0; i < hostSearchStat.numHost; i++ ) {
        if ( maxInx < 0 ||
                hostSearchStat.count[i] > hostSearchStat.count[maxInx] ) {
            maxInx = i;
        }
    }
    if ( maxInx >= 0 ) {
        *outRescInfo = hostSearchStat.rescInfo[maxInx];
    }

    return status;
}


int
getRescForGetInColl( rsComm_t *rsComm, collInp_t *collInp,
                     hostSearchStat_t *hostSearchStat ) {
    collEnt_t *collEnt;
    int handleInx;
    int status;

    if ( collInp == NULL || hostSearchStat == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    handleInx = rsOpenCollection( rsComm, collInp );
    if ( handleInx < 0 ) {
        return handleInx;
    }

    while ( ( status = rsReadCollection( rsComm, &handleInx, &collEnt ) ) >= 0 ) {
        if ( collEnt->objType == DATA_OBJ_T ) {
            dataObjInp_t dataObjInp;
            std::memset(&dataObjInp, 0, sizeof(dataObjInp));
            snprintf( dataObjInp.objPath, MAX_NAME_LEN, "%s/%s",
                      collEnt->collName, collEnt->dataName );
            status = getRescForGetInDataObj( rsComm, &dataObjInp,
                                             hostSearchStat );
            if ( status < 0 ) {
                rodsLog( LOG_NOTICE,
                         "getRescForGetInColl: getRescForGetInDataObj %s err, stat=%d",
                         dataObjInp.objPath, status );
            }
        }
        else if ( collEnt->objType == COLL_OBJ_T ) {
            collInp_t myCollInp;
            std::memset(&myCollInp, 0, sizeof(myCollInp));
            rstrcpy( myCollInp.collName, collEnt->collName, MAX_NAME_LEN );
            status = getRescForGetInColl( rsComm, &myCollInp, hostSearchStat );
            if ( status < 0 ) {
                rodsLog( LOG_NOTICE,
                         "getRescForGetInColl: getRescForGetInColl %s err, stat=%d",
                         collEnt->collName, status );
            }
        }
        free( collEnt );    /* just free collEnt but not content */
        if ( hostSearchStat->totalCount >= MAX_HOST_TO_SEARCH ) {
            /* done */
            rsCloseCollection( rsComm, &handleInx );
            return 0;
        }
    }
    rsCloseCollection( rsComm, &handleInx );
    return 0;
}

int
getRescForGetInDataObj( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                        hostSearchStat_t *hostSearchStat ) {
    int status, i;
    dataObjInfo_t *dataObjInfoHead = NULL;

    if ( dataObjInp == NULL || hostSearchStat == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    status = getDataObjInfoIncSpecColl( rsComm, dataObjInp, &dataObjInfoHead );
    if ( status < 0 ) {
        return status;
    }

    sortObjInfoForOpen( &dataObjInfoHead, &dataObjInp->condInput, 0 );
    if ( dataObjInfoHead ) {
        if ( hostSearchStat->numHost >= MAX_HOST_TO_SEARCH ||
                hostSearchStat->totalCount >= MAX_HOST_TO_SEARCH ) {
            freeAllDataObjInfo( dataObjInfoHead );
            return 0;
        }
        for ( i = 0; i < hostSearchStat->numHost; i++ ) {
            if ( dataObjInfoHead->rescInfo == hostSearchStat->rescInfo[i] ) {
                hostSearchStat->count[i]++;
                hostSearchStat->totalCount++;
                freeAllDataObjInfo( dataObjInfoHead );
                return 0;
            }
        }
        /* no match. add one */
        hostSearchStat->rescInfo[hostSearchStat->numHost] =
            dataObjInfoHead->rescInfo;
        hostSearchStat->count[hostSearchStat->numHost] = 1;
        hostSearchStat->numHost++;
        hostSearchStat->totalCount++;
    }
    freeAllDataObjInfo( dataObjInfoHead );
    return 0;
}
#endif

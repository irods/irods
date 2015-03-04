/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rsIcatOpr.c - misc Icat operation
 */



#include "rsIcatOpr.hpp"
#include "rodsConnect.h"
#include "rsGlobalExtern.hpp"
#include "readServerConfig.hpp"
#include "icatHighLevelRoutines.hpp"
#include "irods_server_properties.hpp"

#ifdef RODS_CAT
int
connectRcat() {
    int status = 0;
    rodsServerHost_t *tmpRodsServerHost;
    int gotRcatHost = 0;


    if ( IcatConnState == INITIAL_DONE ) {
        return 0;
    }

    /* zone has not been initialized yet. can't use getRcatHost */
    tmpRodsServerHost = ServerHostHead;

    while ( tmpRodsServerHost != NULL ) {
        if ( tmpRodsServerHost->rcatEnabled == LOCAL_ICAT ||
                tmpRodsServerHost->rcatEnabled == LOCAL_SLAVE_ICAT ) {
            if ( tmpRodsServerHost->localFlag == LOCAL_HOST ) {

                // capture server properties
                irods::error result = irods::server_properties::getInstance().capture_if_needed();
                if ( !result.ok() ) {
                    irods::log( PASSMSG( "failed to read server configuration", result ) );
                }

                status = chlOpen();

                if ( status < 0 ) {
                    rodsLog( LOG_NOTICE,
                             "connectRcat: chlOpen Error. Status = %d", status );
                }
                else {
                    IcatConnState = INITIAL_DONE;
                    gotRcatHost ++;
                }
            }
            else {
                gotRcatHost ++;
            }
        }
        tmpRodsServerHost = tmpRodsServerHost->next;
    }

    if ( gotRcatHost == 0 ) {
        if ( status >= 0 ) {
            status = SYS_NO_ICAT_SERVER_ERR;
        }
        rodsLog( LOG_SYS_FATAL,
                 "initServerInfo: no rcatHost error, status = %d",
                 status );
    }
    else {
        status = 0;
    }

    return status;
}

int
disconnectRcat() {
    int status;

    if ( IcatConnState == INITIAL_DONE ) {
        if ( ( status = chlClose() ) != 0 ) {
            rodsLog( LOG_NOTICE,
                     "initInfoWithRcat: chlClose Error. Status = %d",
                     status );
        }
        IcatConnState = INITIAL_NOT_DONE;
    }
    else {
        status = 0;
    }
    return status;
}

int
resetRcat() {
    IcatConnState = INITIAL_NOT_DONE;
    return 0;
}

#endif

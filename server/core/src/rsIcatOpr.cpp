#include "irods/rsIcatOpr.hpp"
#include "irods/rodsConnect.h"
#include "irods/rsGlobalExtern.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/miscServerFunct.hpp"

int connectRcat()
{
    if (IcatConnState == INITIAL_DONE) {
        return 0;
    }

    int status = 0;
    int gotRcatHost = 0;

    // Zone has not been initialized yet. Can't use getRcatHost() here.
    rodsServerHost_t* tmpRodsServerHost = ServerHostHead;

    while (tmpRodsServerHost) {
        if (tmpRodsServerHost->rcatEnabled == LOCAL_ICAT ||
            tmpRodsServerHost->rcatEnabled == LOCAL_SLAVE_ICAT)
        {
            if (tmpRodsServerHost->localFlag == LOCAL_HOST) {
                status = chlOpen();

                if (status < 0) {
                    rodsLog(LOG_NOTICE, "connectRcat: chlOpen Error. Status = %d", status);
                }
                else {
                    IcatConnState = INITIAL_DONE;
                    ++gotRcatHost;
                }
            }
            else {
                ++gotRcatHost;
            }
        }

        tmpRodsServerHost = tmpRodsServerHost->next;
    }

    if (gotRcatHost == 0) {
        if (status >= 0) {
            status = SYS_NO_ICAT_SERVER_ERR;
        }

        rodsLog(LOG_SYS_FATAL, "initServerInfo: no rcatHost error, status = %d", status);
    }
    else {
        status = 0;
    }

    return status;
}

int disconnectRcat()
{
    int status;

    if (IcatConnState == INITIAL_DONE) {
        status = chlClose();

        if (status != 0) {
            rodsLog(LOG_NOTICE, "initInfoWithRcat: chlClose Error. Status = %d", status);
        }

        IcatConnState = INITIAL_NOT_DONE;
    }
    else {
        status = 0;
    }

    return status;
}

int resetRcat()
{
    IcatConnState = INITIAL_NOT_DONE;
    return 0;
}

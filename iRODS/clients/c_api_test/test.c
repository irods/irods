
// A Haiku for C
// This test keeps C++ out
// Or it breaks the build

#include "rods.h"
#include "rodsType.h"
#include "rodsClient.h"
#include "miscUtil.h"
#include "rodsPath.h"
#include "rcConnect.h"
#include "dataObjOpen.hpp"
#include "dataObjRead.hpp"
#include "dataObjChksum.hpp"
#include "dataObjClose.hpp"

int main () {
    rodsEnv myEnv;
    int status = getRodsEnv( &myEnv );
    if ( status != 0 ) {
        printf( "getRodsEnv failed.\n" );
        return -1;
    }
    rErrMsg_t errMsg;

    rcComm_t* conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName, myEnv.rodsZone, 1, &errMsg );

    if ( ! conn ) {
        printf( "rcConnect failed\n");
        return -1;
    }
    else {
        printf( "Success!\n");
        rcDisconnect( conn );
    }
    return 0;
}

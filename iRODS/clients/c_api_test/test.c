
// A Haiku for C
// This test keeps C++ out
// Or it breaks the build

#include "rodsType.hpp"
#include "rodsErrorTable.hpp"
#include "rodsClient.hpp"
#include "miscUtil.hpp"
#include "rodsPath.hpp"
#include "rcConnect.hpp"
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

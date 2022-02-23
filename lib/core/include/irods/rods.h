#ifndef RODS_H__
#define RODS_H__

#include "irods/rodsDef.h"
#include "irods/rodsVersion.h"
#include "irods/rodsError.h"
#include "irods/rodsErrorTable.h"
#include "irods/rodsLog.h"
#include "irods/stringOpr.h"
#include "irods/rodsType.h"
#include "irods/rodsKeyWdDef.h"
#include "irods/objInfo.h"
#include "irods/getRodsEnv.h"
#include "irods/dataObjInpOut.h"
#include "irods/rodsGenQuery.h"
#include "irods/parseCommandLine.h"
#include "irods/obf.h"
#include "irods/rcConnect.h"

#ifdef _WIN32
    #include <stdio.h>

    #ifndef snprintf
        #define snprintf _snprintf
    #endif // snprintf

    #ifndef vsnprintf
        #define vsnprintf _vsnprintf
    #endif // vsnprintf

    #ifndef strtoll
        #define strtoll _strtoi64
    #endif // strtoll

    #ifndef random
        #define random rand
    #endif // random
#endif // _WIN32

#endif // RODS_H__

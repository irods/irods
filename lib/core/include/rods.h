/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rods.h - common header file for rods
 */



#ifndef RODS_H__
#define RODS_H__

#include "rodsDef.h"
#include "rodsVersion.h"
#include "rodsError.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "stringOpr.h"
#include "rodsType.h"
#include "rodsKeyWdDef.h"
#include "objInfo.h"
#include "getRodsEnv.h"
#include "dataObjInpOut.h"
#include "rodsGenQuery.h"
#include "parseCommandLine.h"
#include "obf.h"
#include "rodsXmsg.h"
#include "rcConnect.h"

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



#endif	// RODS_H__

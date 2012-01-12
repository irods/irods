/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rods.h - common header file for rods
 */



#ifndef RODS_H
#define RODS_H

#include "rodsDef.h"
#include "rodsVersion.h"
#include "rodsError.h"
#include "rodsLog.h"
#include "stringOpr.h"
#include "rodsType.h"
#include "rodsKeyWdDef.h"
#include "sockComm.h"
#include "objInfo.h"
#include "getRodsEnv.h"
#include "dataObjInpOut.h"
#include "rodsGenQuery.h"
#include "packStruct.h"
#include "md5Checksum.h"
#include "authenticate.h"
#include "parseCommandLine.h"
#include "obf.h"
#include "rodsXmsg.h"
#include "igsi.h"
#include "ikrb.h"
#include "rodsQuota.h"
#include "osauth.h"

#ifdef _WIN32
#include "IRodsLib3.h"
#include <stdio.h>
#include "winsock2.h"

#if defined(_WIN32)
#ifndef snprintf
#define snprintf _snprintf
#endif snprintf
#endif _WIN32

#if defined(_WIN32)
#ifndef vsnprintf
#define vsnprintf _vsnprintf
#endif vsnprintf
#endif _WIN32

#if defined(_WIN32)
#ifndef strtoll
#define strtoll _strtoi64
#endif strtoll
#endif _WIN32

#if defined(_WIN32)
#ifndef random
#define random rand
#endif random
#endif _WIN32


#endif



#endif	/* RODS_H */

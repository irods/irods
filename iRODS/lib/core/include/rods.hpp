/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rods.h - common header file for rods
 */



#ifndef RODS_HPP
#define RODS_HPP

#include "rodsDef.h"
#include "rodsVersion.hpp"
#include "rodsError.hpp"
#include "rodsLog.hpp"
#include "stringOpr.hpp"
#include "rodsType.hpp"
#include "rodsKeyWdDef.hpp"
#include "objInfo.hpp"
#include "getRodsEnv.hpp"
#include "dataObjInpOut.hpp"
#include "rodsGenQuery.hpp"
#include "md5Checksum.hpp"
//#include "authenticate.hpp"
#include "parseCommandLine.hpp"
#include "obf.hpp"
#include "rodsXmsg.hpp"
#include "rodsQuota.hpp"
#include "osauth.hpp"
#include "sslSockComm.hpp"

#ifdef _WIN32
#include "IRodsLib3.hpp"
#include <stdio.h>
#include "winsock2.hpp"

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

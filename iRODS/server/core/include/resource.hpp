/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* resource.h - header file for resource.c
 */



#ifndef RESOURCE_HPP
#define RESOURCE_HPP


#include "rods.hpp"
#include "objInfo.hpp"
#include "dataObjInpOut.hpp"
#include "ruleExecSubmit.hpp"
#include "rcGlobalExtern.hpp"
#include "rsGlobalExtern.hpp"
#include "reIn2p3SysRule.hpp"
#include "reSysDataObjOpr.hpp"

/* definition for the flag in queRescGrp and queResc */
#define BOTTOM_FLAG     0
#define TOP_FLAG        1
#define BY_TYPE_FLAG    2

#define MAX_ELAPSE_TIME 1800 /* max time in seconds above which the load 
* info is considered to be out of date */

extern "C" {
    int
    getMultiCopyPerResc( rsComm_t* );  // JMC - backport 4556
}

#endif	/* RESOURCE_H */

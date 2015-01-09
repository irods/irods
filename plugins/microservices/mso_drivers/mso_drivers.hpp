/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file    mso_drivers.hpp
 *
 * @brief   Declarations for the mso_drivers microservices.
 */



#ifndef MSODRIVERSMS_HPP_
#define  MSODRIVERSMS_HPP_

#include "rods.h"
#include "reGlobalsExtern.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"

int
msiobjget_http( msParam_t*  inRequestPath, msParam_t* inFileMode,
                msParam_t* inFileFlags, msParam_t* inCacheFilename,
                ruleExecInfo_t* rei );
int
msiobjput_http( msParam_t*  inMSOPath, msParam_t*  inCacheFilename,
                msParam_t*  inFileSize, ruleExecInfo_t* rei );

int
msiobjget_slink( msParam_t*  inRequestPath, msParam_t* inFileMode,
                 msParam_t* inFileFlags, msParam_t* inCacheFilename,
                 ruleExecInfo_t* rei );
int
msiobjput_slink( msParam_t*  inMSOPath, msParam_t*  inCacheFilename,
                 msParam_t*  inFileSize, ruleExecInfo_t* rei );

int
msiobjget_irods( msParam_t*  inRequestPath, msParam_t* inFileMode,
                 msParam_t* inFileFlags, msParam_t* inCacheFilename,
                 ruleExecInfo_t* rei );
int
msiobjput_irods( msParam_t*  inMSOPath, msParam_t*  inCacheFilename,
                 msParam_t*  inFileSize, ruleExecInfo_t* rei );

#endif	/*  MSODRIVERSMS_HPP_ */

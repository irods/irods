/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file	msoDriversMS.h
 *
 * @brief	Declarations for the msoDrivers microservices.
 */



#ifndef MSODRIVERSMS_H
#define  MSODRIVERSMS_H

#include "rods.h"
#include "reGlobalsExtern.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"

int
msiobjget_test(msParam_t*  inRequestPath, msParam_t* inFileMode,
               msParam_t* inFileFlags, msParam_t* inCacheFilename,
               ruleExecInfo_t* rei );
int
msiobjput_test(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,
               msParam_t*  inFileSize, ruleExecInfo_t* rei );

int
msiobjget_http(msParam_t*  inRequestPath, msParam_t* inFileMode,
               msParam_t* inFileFlags, msParam_t* inCacheFilename,
               ruleExecInfo_t* rei );
int
msiobjput_http(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,
               msParam_t*  inFileSize, ruleExecInfo_t* rei );

int
msiobjget_z3950(msParam_t*  inRequestPath, msParam_t* inFileMode,
               msParam_t* inFileFlags, msParam_t* inCacheFilename,
               ruleExecInfo_t* rei );
int
msiobjput_z3950(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,
               msParam_t*  inFileSize, ruleExecInfo_t* rei );

int
msiobjget_dbo(msParam_t*  inRequestPath, msParam_t* inFileMode,
               msParam_t* inFileFlags, msParam_t* inCacheFilename,
               ruleExecInfo_t* rei );
int
msiobjput_dbo(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,
               msParam_t*  inFileSize, ruleExecInfo_t* rei );

int
msiobjget_slink(msParam_t*  inRequestPath, msParam_t* inFileMode,
               msParam_t* inFileFlags, msParam_t* inCacheFilename,
               ruleExecInfo_t* rei );
int
msiobjput_slink(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,
               msParam_t*  inFileSize, ruleExecInfo_t* rei );

int
msiobjget_irods(msParam_t*  inRequestPath, msParam_t* inFileMode,
               msParam_t* inFileFlags, msParam_t* inCacheFilename,
               ruleExecInfo_t* rei );
int
msiobjput_irods(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,
               msParam_t*  inFileSize, ruleExecInfo_t* rei );
int
msiobjget_srb(msParam_t*  inRequestPath, msParam_t* inFileMode,
               msParam_t* inFileFlags, msParam_t* inCacheFilename,
               ruleExecInfo_t* rei );
int
msiobjput_srb(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,
               msParam_t*  inFileSize, ruleExecInfo_t* rei );


#endif	/*  MSODRIVERSMS_H */

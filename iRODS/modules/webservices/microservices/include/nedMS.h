/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file	nedMS.h
 *
 * @brief	Declarations for the ned microservices.
 */



#ifndef NEDMS_H
#define NEDMS_H

#include "rods.h"
#include "reGlobalsExtern.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"






int  msiObjByName(msParam_t* inObjByNameParam,
		  msParam_t* outRaParam,
		  msParam_t* outDecParam,
		  msParam_t* outTypParam,
		  ruleExecInfo_t* rei );


#endif	/* NEDMS_H */

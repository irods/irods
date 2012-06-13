/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file	examplesMS.h
 *
 * @brief	Declarations for the msiProperties* microservices.
 */



#ifndef EXAMPLESMS_H
#define EXAMPLESMS_H

#include "rods.h"
#include "reGlobalsExtern.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"



int
msiHello( msParam_t *name, msParam_t *outParam,
        ruleExecInfo_t *rei );


int
msiGetRescAddr( msParam_t *rescName, msParam_t *outAddress,
        ruleExecInfo_t *rei );


#endif	/* EXAMPLESMS_H */

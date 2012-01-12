/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file	guinotMS.h
 *
 * @brief	Declarations for the msiGuinot* microservices.
 */



#ifndef GUINOTMS_H
#define GUINOTMS_H

#include "rods.h"
#include "reGlobalsExtern.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"

int msiGetFormattedSystemTime(msParam_t* outParam, 
			      msParam_t* inpParam,
			      msParam_t* inpFormatParam, 
			      ruleExecInfo_t *rei);

#endif	/* GUINOTMS_H */

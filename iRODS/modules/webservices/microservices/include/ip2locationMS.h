/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file	stockQuoteMS.h
 *
 * @brief	Declarations for the msiStockuote* microservices.
 */



#ifndef IP2LOCATIONMS_H
#define IP2LOCATIONMS_H

#include "rods.h"
#include "reGlobalsExtern.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"







int msiIp2location(msParam_t* inIpParam,msParam_t* inLicParam, msParam_t* outLocParam, ruleExecInfo_t* rei );


#endif	/* IP2LOCATIONMS_H */

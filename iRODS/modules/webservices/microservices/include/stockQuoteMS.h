/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file	stockQuoteMS.h
 *
 * @brief	Declarations for the msiStockuote* microservices.
 */



#ifndef STOCKQUOTEMS_H
#define STOCKQUOTEMS_H

#include "rods.h"
#include "reGlobalsExtern.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"







int msiGetQuote( msParam_t* inSymbolParam, msParam_t* outQuoteParam, ruleExecInfo_t* rei );

#endif	/* STOCKQUOTEMS_H */

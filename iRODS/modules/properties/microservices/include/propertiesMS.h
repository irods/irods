/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file	propertiesMS.h
 *
 * @brief	Declarations for the msiProperties* microservices.
 */



#ifndef PROPERTIESMS_H
#define PROPERTIESMS_H

#include "rods.h"
#include "reGlobalsExtern.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"






int msiPropertiesNew(	msParam_t* listParam, ruleExecInfo_t* rei );
int msiPropertiesClear(	msParam_t* listParam, ruleExecInfo_t* rei );
int msiPropertiesClone( msParam_t* listParam, msParam_t* cloneParam, ruleExecInfo_t* rei );

int msiPropertiesAdd(	msParam_t* listParam, msParam_t* keywordParam, msParam_t* valueParam, ruleExecInfo_t* rei );
int msiPropertiesRemove(	msParam_t* listParam, msParam_t* keywordParam, ruleExecInfo_t* rei );
int msiPropertiesGet(	msParam_t* listParam, msParam_t* keywordParam, msParam_t* valueParam, ruleExecInfo_t* rei );
int msiPropertiesSet(	msParam_t* listParam, msParam_t* keywordParam, msParam_t* valueParam, ruleExecInfo_t* rei );
int msiPropertiesExists(	msParam_t* listParam, msParam_t* keywordParam, msParam_t* trueFalseParam, ruleExecInfo_t* rei );

int msiPropertiesToString( msParam_t* listParam, msParam_t* stringParam, ruleExecInfo_t* rei );
int msiPropertiesFromString( msParam_t* stringParam, msParam_t* listParam, ruleExecInfo_t* rei );

#endif	/* PROPERTIESMS_H */

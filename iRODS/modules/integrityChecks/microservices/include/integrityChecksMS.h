/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file	integrityChecks.h
 *
 * @brief	Declarations for the msiIntegrityChecks* microservices.
 */



#ifndef INTEGRITYCHECKS_H
#define INTEGRITYCHECKS_H

#include "rods.h"
#include "rodsClient.h"
#include "rcMisc.h"



int msiVerifyOwner (msParam_t* collinp, msParam_t* ownerinp, 
	msParam_t* bufout, msParam_t* statout, ruleExecInfo_t *rei);
int msiVerifyAVU (msParam_t* collinp, msParam_t* avunameinp, msParam_t* avuvalueinp, msParam_t* avuattrsinp, 
	msParam_t* bufout, msParam_t* statout, ruleExecInfo_t* rei);
int msiVerifyACL (msParam_t* collinp, msParam_t* userinp, msParam_t* authinp, msParam_t* notflaginp, 
	msParam_t* bufout, msParam_t* statout, ruleExecInfo_t *rei);
int msiVerifyExpiry (msParam_t* collinp, msParam_t* timeinp, msParam_t* typeinp, 
	msParam_t* bufout, msParam_t* statout, ruleExecInfo_t* rei);
int msiVerifyDataType (msParam_t* collinp, msParam_t* datatypeinp, 
	msParam_t* bufout, msParam_t* statout, ruleExecInfo_t* rei);
int msiVerifyFileSizeRange (msParam_t* collinp, msParam_t* minsizeinp, msParam_t* maxsizeinp, 
	msParam_t* bufout, msParam_t* statout, ruleExecInfo_t *rei);


#endif	/* INTEGRITYCHECKS_H */

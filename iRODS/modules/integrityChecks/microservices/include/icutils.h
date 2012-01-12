/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file	icutils.h
 *
 * @brief	Declarations for the msiIntegrityChecks* microservices.
 */



#ifndef ICUTILS
#define ICUTILS

#include "rods.h"
#include "rodsGenQuery.h"

int msiListFields (msParam_t *collinp, msParam_t *fieldinp, 
	msParam_t *bufout, msParam_t* statout, ruleExecInfo_t *rei);

/* junk functions */
int msiTestForEachExec (msParam_t* mPout1, ruleExecInfo_t *rei);
int msiHiThere (msParam_t* mPout1, ruleExecInfo_t *rei);
int msiTestWritePosInt (msParam_t* mPout1, ruleExecInfo_t *rei);

/* helper monkey function */
int verifyCollOwners (genQueryOut_t* gqout, char* ownerlist, bytesBuf_t* mybuf);
int verifyCollAVU (genQueryOut_t* gqout, char* myavuname, char* myavuvalue, char* myavuattr, bytesBuf_t* mybuf);
int verifyCollACL (genQueryOut_t* gqout, char* myaclname, char* myacltype, bytesBuf_t* mybuf);

#endif	/* ICUTILS */

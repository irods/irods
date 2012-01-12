/*** Copyright (c) 2009 Data Intensive Cyberinfrastructure Foundation. All rights reserved.    ***
 *** For full copyright notice please refer to files in the COPYRIGHT directory                ***/
/* Written by Jean-Yves Nief of CCIN2P3 and copyright assigned to Data Intensive Cyberinfrastructure Foundation */

/* univMSSDriver.h - header file for univMSSDriver.c
 */


#ifndef UNIV_MSS_DRIVER_H
#define UNIV_MSS_DRIVER_H

#define UNIV_MSS_INTERF_SCRIPT "univMSSInterface.sh"

#include "rods.h"
#include "rcConnect.h"
#include "msParam.h"
#include "physPath.h"
#include "reIn2p3SysRule.h"

int
univMSSFileUnlink (rsComm_t *rsComm, char *filename);
int
univMSSFileMkdir (rsComm_t *rsComm, char *filename, int mode);
int
univMSSFileChmod (rsComm_t *rsComm, char *filename, int mode);
int univMSSFileStat (rsComm_t *rsComm, char *filename, struct stat *statbuf);
int
univMSSStageToCache (rsComm_t *rsComm, fileDriverType_t cacheFileType,
int mode, int flags, char *filename,
char *cacheFilename, rodsLong_t dataSize,
keyValPair_t *condInput);
int
univMSSSyncToArch (rsComm_t *rsComm, fileDriverType_t cacheFileType,
int mode, int flags, char *filename,
char *cacheFilename,  rodsLong_t dataSize,
keyValPair_t *condInput);
int 
univMSSFileRename (rsComm_t *rsComm, char *oldFileName, char *newFileName);

#endif	/* UNIV_MSS_DRIVER_H */

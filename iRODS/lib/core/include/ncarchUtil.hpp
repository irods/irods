/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* ncarchUtil.h - Header for for ncarchUtil.c */

#ifndef NCARCHUTIL_H
#define NCARCHUTIL_H

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"

#ifdef  __cplusplus
extern "C" {
#endif


int
ncarchUtil (rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
rodsPathInp_t *rodsPathInp);
int
initCondForNcarch (rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
ncArchTimeSeriesInp_t *ncArchTimeSeriesInp);
#ifdef  __cplusplus
}
#endif

#endif	/* NCARCHUTIL_H */

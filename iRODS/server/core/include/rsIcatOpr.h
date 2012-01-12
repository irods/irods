/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rsIcatOpr.h - common header file rsIcatOpr.c
 */



#ifndef RS_ICAT_OPR_H
#define RS_ICAT_OPR_H

#include "rods.h"
#include "getRodsEnv.h"

int
connectRcat (rsComm_t *rsComm);
int
disconnectRcat (rsComm_t *rsComm);
int
resetRcat (rsComm_t *rsComm);
#endif	/* RS_ICAT_OPR_H */

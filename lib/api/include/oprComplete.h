/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#ifndef OPR_COMPLETE_H__
#define OPR_COMPLETE_H__

/* This is Object File I/O type API call */

#include "rcConnect.h"

#if defined(RODS_SERVER)
#define RS_OPR_COMPLETE rsOprComplete
/* prototype for the server handler */
int
rsOprComplete( rsComm_t *rsComm, int *retval );
#else
#define RS_OPR_COMPLETE NULL
#endif

int
rcOprComplete( rcComm_t *conn, int retval );

#endif	/* OPR_COMPLETE_H */

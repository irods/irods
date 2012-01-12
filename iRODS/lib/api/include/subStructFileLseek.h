/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* subStructFileLseek.h  
 */

#ifndef SUB_STRUCT_FILE_LSEEK_H
#define SUB_STRUCT_FILE_LSEEK_H

/* This is Object File I/O type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "structFileDriver.h"
#include "fileLseek.h"

typedef struct SubStructFileLseekInp {
    rodsHostAddr_t addr;
    structFileType_t type;
    int fd;
    rodsLong_t offset;
    int whence;
} subStructFileLseekInp_t;

#define SubStructFileLseekInp_PI "struct RHostAddr_PI; int type; int fd; double offset; int whence;"

#if defined(RODS_SERVER)
#define RS_SUB_STRUCT_FILE_LSEEK rsSubStructFileLseek
/* prototype for the server handler */
int
rsSubStructFileLseek (rsComm_t *rsComm, subStructFileLseekInp_t *subStructFileLseekInp,
fileLseekOut_t **subStructFileLseekOut);
int
_rsSubStructFileLseek (rsComm_t *rsComm, subStructFileLseekInp_t *subStructFileLseekInp,
fileLseekOut_t **subStructFileLseekOut);
int
remoteSubStructFileLseek (rsComm_t *rsComm, subStructFileLseekInp_t *subStructFileLseekInp, 
fileLseekOut_t **subStructFileLseekOut, rodsServerHost_t *rodsServerHost);
#else
#define RS_SUB_STRUCT_FILE_LSEEK NULL
#endif

/* prototype for the client call */
int
rcSubStructFileLseek (rcComm_t *conn, subStructFileLseekInp_t *subStructFileLseekInp,
fileLseekOut_t **subStructFileLseekOut);

#endif	/* SUB_STRUCT_FILE_LSEEK_H */
